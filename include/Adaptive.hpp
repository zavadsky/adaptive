#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <deque>
#include "BitIoStream.hpp"
#include "WordBasedText.h"

using namespace std;

class BackwardEncoder {
public:
    BackwardEncoder(WordBasedText*);
    BackwardEncoder(WordBasedText*,vector<uint32_t>);
    vector<uint32_t> get_dic(){return dic;};
    void checkDecode();
    virtual void serial(string){};
protected:
    WordBasedText* text;
    unsigned NYT;
    vector<uint32_t> out;
    vector<uint32_t> dic;
};

// Our method
class AdaptiveN : public BackwardEncoder {
private:
    vector<int> m;
    vector<int> rev;
    vector<int> ind;
    vector<int> f,f1;
    UniversalCode* C;
    vector<uint8_t> bestDigitsSz;
    int j;
    const static int max_var_digit=12,init_j=26;
    void update1(int);
    void update_code(uint32_t);
    void update_decode(uint32_t);
    void precalcBestBlocks();
    void BCS(uint8_t[], uint32_t, uint64_t, uint64_t, uint64_t, uint64_t);
    const static int max_digit=40;
    int bestFullBitsSz = 10000000,bestBlocksNum;
public:
    int mc=0;
    uint32_t* PSum;
    uint64_t  getSum(uint32_t, uint32_t);
    double rate=1.68;
    AdaptiveN(WordBasedText*,UniversalCode*);
    AdaptiveN(WordBasedText*,UniversalCode*,vector<uint32_t>);
    double encode();
    double encode1();
    double encode2();
    void print_code();
    void update();
    int decode();
    int decode1();
    int decode2();
};

typedef struct {
    unsigned up,      // next node up the tree
        down,         // pair of down nodes
        symbol,       // node symbol value
        weight;       // node weight
} HTable;

typedef struct {
    unsigned esc,     // the current tree height
        root,         // the root of the tree
        size,         // the alphabet size
        *map;         // mapping for symbols to nodes
    HTable table[1];  // the coding table starts here
} HCoder;

// Classic Vitter algorithm
class Vitter {
private:
    FILE *In, *Out;
    unsigned char ArcBit = 0;
    int ArcChar = 0;
    HCoder *huff;
    WordBasedText* text;
    int len=0;
    const int size = (1<<24);
    void update(string);
    HCoder *huff_init (unsigned size, unsigned root);
    void arc_put1 (unsigned bit);
    unsigned arc_get1 ();
    unsigned huff_split (HCoder *, unsigned);
    unsigned huff_leader (HCoder *, unsigned);
    unsigned huff_slide (HCoder *, unsigned);
    void huff_increment (HCoder *, unsigned);
    void huff_scale (HCoder *, unsigned);
    void huff_sendid (HCoder *, unsigned);
    void huff_encode (HCoder *, unsigned);
    unsigned huff_readid (HCoder*);
public:
    Vitter(WordBasedText*);
    ~Vitter(){delete[] huff;};
    double encode();
};

// Modified Vitter algorithm
class VitterM : public BackwardEncoder {
private:
    FILE *In, *Out;
    unsigned char ArcBit = 0;
    int ArcChar = 0;
    HCoder *huff;
    int len=0,mc=0;
    const int size = (1<<24);
    void update(string);
    HCoder *huff_init (unsigned size, unsigned root);
    void arc_put1 (unsigned bit);
    unsigned arc_get1 ();
    unsigned huff_split (HCoder *, unsigned);
    unsigned huff_leader (HCoder *, unsigned);
    unsigned huff_slide (HCoder *, unsigned);
    void huff_increment (HCoder *, unsigned);
    void huff_sendid (HCoder *, unsigned);
    void huff_encode (HCoder *, unsigned);
    unsigned huff_decode (HCoder *);
    unsigned huff_readid (HCoder*);
    unsigned char *buffer;
public:
    VitterM(WordBasedText*);
    double encode();
    unsigned decode ();
    int serialize(uint8_t*);
    void load(uint8_t*,int);
    ~VitterM(){delete huff;};
};

void print_vector(vector<uint32_t>,string);
