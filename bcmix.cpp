#include "include\UniversalCode.h"

// Initialize the code. k - digit bitlengths; N - number of codewords
void BCMix::code_init(vector<uint8_t> k,int N) {
int i=0;
    for(auto it=k.begin();it!=k.end();it++,i++)
        blocksSz[i]=k[i];
    for(;i<max_digit;i++)
        blocksSz[i]=2;
    DigitsNum.clear();
    precalcCodesAndSizes(N);
    prevCodeDigits=DigitsNum[curCodeSz];
}

BCMix::BCMix(vector<uint8_t> k,int N) {
    code_init(k,N);
    buidTableDecode();
}

// Create the codeword set
void BCMix::precalcCodesAndSizes(uint32_t maxCode)
{
	myCode = new uint32_t[maxCode + 1];
	myCodeSz = new uint32_t[maxCode + 1];

	uint32_t blocksMask[max_digit+10];
	pws[0] = 1;
	for (uint32_t i = 1; i <= max_digit; i++)
	{
		blocksMask[i - 1] = (1 << blocksSz[i - 1]) - 1;
		pws[i] = pws[i - 1] * blocksMask[i - 1];
	}
	uint32_t cum = 0, pwsI = 0, len = blocksSz[0], i;
	for (i = 0; i <= maxCode; i++)
	{
		if (cum >= pws[pwsI])
		{
			cum -= pws[pwsI];
			pwsI++;
			len += blocksSz[pwsI];
		}

        DigitsNum.push_back(pwsI+1);
		uint32_t code = 0, shift = 0, cumCopy = cum;
		for (uint32_t j = 0; j < pwsI; j++)
		{
			uint32_t val = cumCopy % blocksMask[j];
			cumCopy /= blocksMask[j];
			code |= val << shift;
			shift += blocksSz[j];
		}
		code |= blocksMask[pwsI] << shift;

		myCode[i] = code;
		myCodeSz[i] = shift + blocksSz[pwsI];

		cum++;
	}
}

// Create the lookup tables for fast decoding
void BCMix::buidTableDecode()
{
	uint32_t masks[32];
	uint32_t pws[32];
	uint32_t endSum[32];

	uint8_t digitsInCode=DigitsNum[curCodeSz];

	for (uint32_t i = 0; i < 32; i++)
	{
		masks[i] = (1 << blocksSz[i]) - 1;
	}
	pws[0] = 1;
	endSum[0] = 0;
	for (uint32_t i = 1; i < 32; i++)
	{
		pws[i] = pws[i - 1] * masks[i - 1];
		endSum[i] = endSum[i - 1] + pws[i - 1];
	}

	uint32_t block1I, block2I, block3I, blockI = 0;
	uint32_t blockLenSum;

	blockLenSum = 0;
	while (blockLenSum + blocksSz[blockI] <= blockLen)
	{
		blockLenSum += blocksSz[blockI];
		blockI++;
	}
	block1I = blockI;

	blockLenSum = 0;
	while (blockLenSum + blocksSz[blockI] <= blockLen)
	{
		blockLenSum += blocksSz[blockI];
		blockI++;
	}
	block2I = blockI;

	blockLenSum = 0;
	while (blockLenSum + blocksSz[blockI] <= blockLen)
	{
		blockLenSum += blocksSz[blockI];
		blockI++;
	}
	block3I = blockI;

	for (uint32_t i = 0; i < blockSz; i++) // i iterates over all possible binary vectors
	{
		uint32_t myL = 0, myN = 0, myShift = 0, tShift = 0;

		for (uint32_t j = 0; j < block1I; j++)
		{
			if(j < digitsInCode-1)
                myShift += blocksSz[j];

			uint32_t val = (i >> tShift) & masks[j];

			if (val == masks[j] || j >= digitsInCode-1)
			{
				myN = 1;
				myL += endSum[j];
				break;
			}
			else
			{
				myL += val * pws[j];
			}

			tShift += blocksSz[j];
		}

		ts[0].L[i] = myL;
		ts[0].n[i] = myN;
		ts[0].shift[i] = myShift;
	}

	for (uint32_t i = 0; i < blockSz; i++)
	{
		uint32_t myL = 0, myN = 0, myShift = 0, tShift = 0;

		for (uint32_t j = block1I; j < block2I; j++)
		{
			if(j < digitsInCode-1)
                myShift += blocksSz[j];

			uint32_t val = (i >> tShift) & masks[j];

			if (val == masks[j] || j >= digitsInCode-1)
			{
				myN = 1;
				myL += endSum[j];
				break;
			}
			else
			{
				myL += val * pws[j];
			}

			tShift += blocksSz[j];
		}

		ts[1].L[i] = myL;
		ts[1].n[i] = myN;
		ts[1].shift[i] = myShift;
	}

	for (uint32_t i = 0; i < blockSz; i++)
	{
		uint32_t myL = 0, myN = 0, myShift = 0, tShift = 0;

		for (uint32_t j = block2I; j < block3I; j++)
		{
			if(j < digitsInCode-1)
                myShift += blocksSz[j];

			uint32_t val = (i >> tShift) & masks[j];

			if (val == masks[j] || j >= digitsInCode-1)
			{
				myN = 1;
				myL += endSum[j];
				break;
			}
			else
			{
				myL += val * pws[j];
			}

			tShift += blocksSz[j];
		}

		ts[2].L[i] = myL;
		ts[2].n[i] = myN;
		ts[2].shift[i] = myShift;
	}

	ts[0].nxtTable = &ts[1];
	ts[1].nxtTable = &ts[2];
	ts[2].nxtTable = &ts[2];
}

// Read and decode 1 codeword
uint32_t BCMix::get_symbol() {
uint32_t freeBytes = (64 - bitLen) >> 3;
TableDecode* table = &ts[0];
		bitStream |= (*(uint64_t*)(void*)(buffer + file8Pos)) << bitLen;
		bitLen += freeBytes << 3;
		file8Pos += freeBytes;
uint32_t  _n=0,res=0;
		while(!_n)
		{
			uint32_t block = bitStream & blockMask;
			uint32_t _shift = table->shift[block];
			uint32_t _L = table->L[block];
            _n = table->n[block];
			table = table->nxtTable;
			bitStream >>= _shift;
			bitLen -= _shift;
			res += _L;
        }
        return res;
}

// Add one codeword to the code
void BCMix::codeIncrease() {
    curCodeSz++;
    // If we need to lengthen the sequence of code digits
    if(DigitsNum[curCodeSz]>prevCodeDigits) {
        prevCodeDigits++;
        buidTableDecode();
    }
}

// Output the i-th codeword
void BCMix::flush_to_byte(uint32_t i) {
uint64_t code = myCode[i];
uint32_t sz = myCodeSz[i];
		curCode |= code << curSz;
		if(DigitsNum[i]==DigitsNum[curCodeSz]) {
            uint8_t dSz=blocksSz[DigitsNum[i]-1];
            sz-=dSz;
            curSz += sz;
            uint64_t mask=(((uint64_t)1<<curSz)-1);
            curCode&=mask;
   		} else
            curSz += sz;
		if (curSz >= 32)
		{
			buffer32[pos] = (uint32_t)curCode;
			pos++;
			curCode >>= 32;
			curSz -= 32;
		}
}
