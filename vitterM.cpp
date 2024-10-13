// Vitter's algorithm with NYT symbol frequency count. Code taken from https://code.google.com/archive/p/compression-code/downloads and modified
#include <memory.h>
#include "include\Adaptive.hpp"

VitterM::VitterM(WordBasedText* w):BackwardEncoder(w) {
    huff = huff_init (size, size);
    buffer = (unsigned char*)malloc(w->Nwords*2);
}

//  initialize an adaptive coder
//  for alphabet size, and count
//  of nodes to be used

HCoder* VitterM::huff_init (unsigned size, unsigned root)
{
  //  default: all alphabet symbols are used

  if( !root || root > size )
      root = size;

  //  create the initial escape node
  //  at the tree root

  if( root <<= 1 )
    root--;

  huff = (HCoder*)malloc (root * sizeof(HTable) + sizeof(HCoder));
  memset (huff->table + 1, 0, root * sizeof(HTable));
  memset (huff, 0, sizeof(HCoder));

  if( huff->size = size )
      huff->map = (unsigned*)calloc (size, sizeof(unsigned));

  huff->esc = huff->root = root;
  len=mc=ArcChar=ArcBit=0;
  return huff;
}

void VitterM::arc_put1 (unsigned bit)
{
    ArcChar <<= 1;

    if( bit )
        ArcChar |= 1;

    if( ++ArcBit < 8 )
        return;

    buffer[len]=ArcChar;

    len++;
    ArcChar = ArcBit = 0;
}

unsigned VitterM::arc_get1 ()
{
    if( !ArcBit )
        ArcChar = buffer[len], ArcBit = 8, len++;

    return ArcChar >> --ArcBit & 1;
}

int VitterM::serialize(uint8_t* external_buf) {
    memcpy(external_buf,buffer,len);
    return len;
}

void VitterM::load(uint8_t* external_buf,int sz) {
    memcpy(buffer,external_buf,sz);
}

// split escape node to incorporate new symbol

unsigned VitterM::huff_split (HCoder *huff, unsigned symbol)
{
unsigned pair, node;

    //  is the tree already full???

    if( pair = huff->esc )
        huff->esc--;
    else
        return 0;

    //  if this is the last symbol, it moves into
    //  the escape node's old position, and
    //  huff->esc is set to zero.

    //  otherwise, the escape node is promoted to
    //  parent a new escape node and the new symbol.

    if( node = huff->esc ) {
        huff->table[pair].down = node;
        huff->table[pair].weight = 1;
        huff->table[node].up = pair;
        huff->esc--;
    } else
        pair = 0, node = 1;

    //  initialize the new symbol node

    huff->table[node].symbol = symbol;
    huff->table[node].weight = 0;
    huff->table[node].down = 0;
    huff->map[symbol] = node;

    //  initialize a new escape node.

    huff->table[huff->esc].weight = 0;
    huff->table[huff->esc].down = 0;
    huff->table[huff->esc].up = pair;
    return node;
}

//  swap leaf to group leader position
//  return symbol's new node

unsigned VitterM::huff_leader (HCoder *huff, unsigned node)
{
unsigned weight = huff->table[node].weight;
unsigned leader = node, prev, symbol;

    while( weight == huff->table[leader + 1].weight )
        leader++;

    if( leader == node )
        return node;

    // swap the leaf nodes

    symbol = huff->table[node].symbol;
    prev = huff->table[leader].symbol;

    huff->table[leader].symbol = symbol;
    huff->table[node].symbol = prev;
    huff->map[symbol] = leader;
    huff->map[prev] = node;
    return leader;
}

//  slide internal node up over all leaves of equal weight;
//  or exchange leaf with next smaller weight internal node

//  return node's new position

unsigned VitterM::huff_slide (HCoder *huff, unsigned node)
{
unsigned next = node;
HTable swap[1];

    *swap = huff->table[next++];

    // if we're sliding an internal node, find the
    // highest possible leaf to exchange with

    if( swap->weight & 1 )
      while( swap->weight > huff->table[next + 1].weight )
          next++;

    //  swap the two nodes

    huff->table[node] = huff->table[next];
    huff->table[next] = *swap;

    huff->table[next].up = huff->table[node].up;
    huff->table[node].up = swap->up;

    //  repair the symbol map and tree structure

    if( swap->weight & 1 ) {
        huff->table[swap->down].up = next;
        huff->table[swap->down - 1].up = next;
        huff->map[huff->table[node].symbol] = node;
    } else {
        huff->table[huff->table[node].down - 1].up = node;
        huff->table[huff->table[node].down].up = node;
        huff->map[swap->symbol] = next;
    }

    return next;
}

//  increment symbol weight and re balance the tree.

void VitterM::huff_increment (HCoder *huff, unsigned node)
{
unsigned up;
  //  obviate swapping a parent with its child:
  //    increment the leaf and proceed
  //    directly to its parent.

  //  otherwise, promote leaf to group leader position in the tree

  if( huff->table[node].up == node + 1 )
    huff->table[node].weight += 2, node++;
  else
    node = huff_leader (huff, node);

  //  increase the weight of each node and slide
  //  over any smaller weights ahead of it
  //  until reaching the root

  //  internal nodes work upwards from
  //  their initial positions; while
  //  symbol nodes slide over first,
  //  then work up from their final
  //  positions.

  while( huff->table[node].weight += 2, up = huff->table[node].up ) {
    while( huff->table[node].weight > huff->table[node + 1].weight )
        node = huff_slide (huff, node);

    if( huff->table[node].weight & 1 )
        node = up;
    else
        node = huff->table[node].up;
  }
}

//  send the bits for an escaped symbol

void VitterM::huff_sendid (HCoder *huff, unsigned symbol)
{
unsigned empty = 0, max;

    //  count the number of empty symbols
    //  before the symbol in the table

    while( symbol-- )
      if( !huff->map[symbol] )
        empty++;

    //  send LSB of this count first, using
    //  as many bits as are required for
    //  the maximum possible count

    if( max = huff->size - (huff->root - huff->esc) / 2 - 1 )
      do arc_put1 (empty & 1), empty >>= 1;
      while( max >>= 1 );
}

//  encode the next symbol

void VitterM::huff_encode (HCoder *huff, unsigned symbol)
{
unsigned emit = 1, bit;
unsigned up, idx, node;

    if( symbol < huff->size )
        node = huff->map[symbol];
    else
        return;

    //  for a new symbol, direct the receiver to the escape node
    //  but refuse input if table is already full.

    if( !(idx = node) )
      if( !(idx = huff->esc) )
        return;

    //  accumulate the code bits by
    //  working up the tree from
    //  the node to the root

    while( up = huff->table[idx].up )
        emit <<= 1, emit |= idx & 1, idx = up;

    //  send the code, root selector bit first

    if(node) {
        while( bit = emit & 1, emit >>= 1 ) {
            arc_put1 (bit);
        }
    }
    //  send identification and incorporate
    //  new symbols into the tree

    if( !node ) {
        node = huff_split(huff, symbol);
    }

    //  adjust and re-balance the tree

    huff_increment (huff, node);
}

unsigned VitterM::huff_decode (HCoder *huff)
{
unsigned node = huff->root;
unsigned symbol, down,bit;

    //  work down the tree from the root
    //  until reaching either a leaf
    //  or the escape node.  A one
    //  bit means go left, a zero
    //  means go right.

    while( down = huff->table[node].down ) {
      if( bit=arc_get1 () )
        node = down - 1;  // the left child preceeds the right child
      else
        node = down;
    }

    if( node == huff->esc )
      if( huff->esc ) {
        symbol=mc++;
        node = huff_split (huff, symbol);
      } else
            return 0;
    else
        symbol = huff->table[node].symbol;
    if(symbol==NYT) {
        huff_increment (huff, node);
        node = huff_split (huff, mc);
        symbol=mc++;
    }
    //  increment weights and rebalance
    //  the coding tree

    huff_increment (huff, node);
    return symbol;
}

unsigned VitterM::decode ()
{
    delete huff;
    huff = huff_init (size, size);
      uint32_t i=0;
      huff_encode(huff,NYT);
      huff_encode(huff,mc++);
      out[i++]=0;
      while( i<text->Nwords ) {
        int symbol=huff_decode(huff);
        out[i++]=symbol;
      }
      return i;
}

//  read the identification bits
//  for an escaped symbol

unsigned VitterM::huff_readid (HCoder *huff)
{
unsigned empty = 0, bit = 1, max, symbol;

    //  receive the symbol, LSB first, reading
    //  only the number of bits necessary to
    //  transmit the maximum possible symbol value

    if( max = huff->size - (huff->root - huff->esc) / 2 - 1 )
      do empty |= arc_get1 () ? bit : 0, bit <<= 1;
      while( max >>= 1 );

    //  the count is of unmapped symbols
    //  in the table before the new one

    for( symbol = 0; symbol < huff->size; symbol++ )
      if( !huff->map[symbol] )
        if( !empty-- )
            return symbol;

    //  oops!  our count is too big, either due
    //  to a bit error, or a short node count
    //  given to huff_init.

    return 0;
}

double VitterM::encode() {
std::string word;
set<string> occurred;
unsigned i=0;
    len=0;
    while(i<text->Nwords) {
		int symbol=text->numbers[i];

		if(!huff->map[symbol]) {
            huff_encode(huff, NYT);
		}
        huff_encode(huff, symbol);
       i++;
    }
    return len;
}
