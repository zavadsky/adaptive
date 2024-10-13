#ifndef RMD_H
#define RMD_H
#include <stdint.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <memory>
#include "WordBasedText.h";

using namespace std;

class WordBasedText;

// Universal code
class UniversalCode {
public:
    UniversalCode(){buffer = (unsigned char*)calloc(50000000,1);buffer32=(uint32_t*)buffer;};
    virtual int encode(vector<uint64_t>){};
    virtual void code_init(vector<uint8_t>,int){};
    vector<uint32_t> code;
    uint32_t out[100];
    virtual void flush_to_byte(uint32_t){};
    int cur_byte=0,cur_bit=7,cur_value=0;
    virtual void reset() {cur_byte=0;cur_bit=7;curCodeSz=0;};
    virtual int process8bytes(int){};
    unsigned char* get_out(){return buffer;};
    virtual void dump(){};
    virtual int code_size(){return 0;};
    virtual uint32_t get_symbol(){};
    virtual void buidTableDecode(){};
    virtual ~UniversalCode(){delete[] buffer;};
    int serialize(uint8_t* external_buf){memcpy(external_buf,buffer,code_size());    return code_size();};
    void load(uint8_t* external_buf,int csize){memcpy(buffer,external_buf,csize);};
    virtual void codeIncrease(){};
    virtual uint8_t getDigitsNum(){};
/*private:

protected:*/
    unsigned char* buffer;
    uint32_t* buffer32;
    uint32_t curCodeSz = 0;

};
/*
class RMD: public UniversalCode
{
    public:
        RMD(vector<int>,int);
        virtual ~RMD();
        int encode(vector<uint64_t>);
        void code_output(int);
        void rmd_to_file(string);
        int code_size(){return cur_byte*4+1;};
    protected:

    private:
        int t=sizeof(unsigned int)*8;
        void flush_to_byte(uint32_t);
        void flush_to_byte32(uint32_t);
};*/

const int Tbyte_size=12,loop_n=64/Tbyte_size,Tsize=1<<Tbyte_size,Tshort_size=4,Tshort=1<<Tshort_size,Tmask=Tsize-1;

//===================== BCMix =============================
#define blockLen (10)
#define blockSz (1 << blockLen)
#define blockMask (blockSz - 1)

struct TableDecode
{
	uint32_t L[blockSz];
	uint8_t n[blockSz];
	uint8_t shift[blockSz];

	TableDecode* nxtTable;
};

// Binary mixed-digit code
class BCMix: public UniversalCode {
public:
    BCMix(vector<uint8_t>,int);
    virtual ~BCMix(){delete[] myCode; delete[] myCodeSz;};
    void reset() {pos=curCode=curSz=bitLen=bitStream=file8Pos=curCodeSz=0;};
    int code_size(){return (pos+1)*4;};
    uint32_t get_symbol();
    void code_init(vector<uint8_t>,int);
    void buidTableDecode();
    void codeIncrease();
    uint8_t getDigitsNum(){return DigitsNum[curCodeSz];};

private:
    TableDecode ts[3];
    const static int max_digit=32;
    uint8_t blocksSz[max_digit];
    uint32_t pws[max_digit+10];
    vector<uint8_t> DigitsNum;
    uint32_t* myCode;
    uint32_t* myCodeSz;
    uint32_t pos = 0,file8Pos=0,bestBlocksNum;
    uint64_t curCode = 0;
    uint32_t curSz = 0;
    uint64_t bitStream = 0;
	uint32_t bitLen = 0;
	uint8_t prevCodeDigits = 0;

    void flush_to_byte(uint32_t);
    void flush_to_byte32(uint32_t);
    void precalcCodesAndSizes(uint32_t);
};
//=====================================================================

#endif // RMD_H
