#include <string.h>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <windows.h>
#include "include\UniversalCode.h"
#include "include\WordBasedText.h"
#include "include\BitIoStream.hpp"
#include "include\Adaptive.hpp"

#pragma GCC diagnostic push

double m_time;
LARGE_INTEGER start, finish, freq;

uint8_t buffer1[25000000],buffer2[25000000],buffer3[25000000];

#define measure_start()  \
	QueryPerformanceFrequency(&freq); \
	QueryPerformanceCounter(&start);

#define measure_end() QueryPerformanceCounter(&finish); \
					m_time = ((finish.QuadPart - start.QuadPart) / (double)freq.QuadPart);

using namespace std;

int main(int argc, char** argv) {
    if(argc<2)
        cout<<"Incorrect number of command line arguments.";
    else {
        string ifname(argv[1]);
        int iter=atoi(argv[2]);
        std::ofstream out(ifname+".enc", std::ios::binary);
        WordBasedText *wa;
        BCMix *bmix,*bmix1;
        AdaptiveN *encmix,*encmix1;
        VitterM *encvm;
        Vitter *encv;
        int vs,vms,alg2s,alg4s;             // Size of the text encoded with different algorithms
        vector<uint8_t> bmix_code{4,2,2,2,1,2}; // BCMix code for Alg. 2
        double vt=0,vmt=0,alg2t=0,alg4t=0;  // Encoding/decoding time
        wa = new WordBasedText(ifname,1);   // Pre-process the text
        wa->prepare_adaptive();             // Convert words to numbers accordingly to their leftmost occurrences

        cout<<endl<<"================ Encoding. ================== iterations="<<iter<<endl;
        for(int i=0;i<iter;i++) {
                // Classical Vitter
                encv = new Vitter(wa);
                measure_start();
                    vs=encv->encode();
                measure_end();
                vt += m_time;
                // Modified Vitter
                encvm = new VitterM(wa);
                measure_start();
                    vms=encvm->encode();
                measure_end();
                vmt += m_time;
                // Algorithm 2
                bmix = new BCMix(bmix_code,wa->getMaxSymb());
                encmix = new AdaptiveN(wa, (BCMix*)bmix);
                measure_start();
                    alg2s=encmix->encode1();
                measure_end();
                alg2t += m_time;
                // Algorithm 2 + Algorithm 4
                bmix1 = new BCMix({},wa->getMaxSymb());
                encmix1 = new AdaptiveN(wa, (BCMix*)bmix1);
                measure_start();
                    alg4s=encmix1->encode2();
                measure_end();
                alg4t += m_time;
                if(i==iter-1) {
                    encvm->serialize(buffer1);
                    bmix->serialize(buffer2);
                    bmix1->serialize(buffer3);
                }
                delete encmix1;
                delete bmix1;
                delete bmix;
                delete encmix;
                delete encvm;
                delete encv;
                cout<<".";
            }

        cout<<endl;
        vt/=iter; vmt/=iter; alg2t/=iter; alg4t/=iter;
        cout<<" Vitter time= "<<vt<<" VitterM time= "<<vmt<<" NewAlg2 time="<<alg2t<<" NewAlg4 time="<<alg4t<<endl;
        cout<<" Vitter size= "<<vs<<" VitterM size= "<<vms<<" NewAlg2 size="<<alg2s<<" NewAlg4 size="<<alg4s<<endl;

        cout<<endl<<"================ Decoding. ================== iterations="<<iter<<endl;
        vt=vmt=alg2t=alg4t=0;
        for(int k=0;k<iter;k++) {
                // Modified Vitter
                encvm = new VitterM(wa);
                encvm->load(buffer1,vms);
                measure_start();
                    encvm->decode();
                measure_end();
                encvm->checkDecode();
                vmt += m_time;
                // Algorithm 2
                bmix = new BCMix(bmix_code,wa->getMaxSymb());
                bmix->load(buffer2,alg2s);
                encmix = new AdaptiveN(wa, (BCMix*)bmix);
                measure_start();
                    encmix->decode1();
                measure_end();
                encmix->checkDecode();
                alg2t += m_time;
                // Algorithm 4
                bmix1 = new BCMix({},wa->getMaxSymb());
                bmix1->load(buffer3,alg4s);
                encmix1 = new AdaptiveN(wa, (BCMix*)bmix1);
                measure_start();
                    encmix1->decode2();
                measure_end();
                encmix1->checkDecode();
                alg4t += m_time;
                cout<<".";
                delete encmix1;
                delete bmix1;
                delete bmix;
                delete encmix;
                delete encvm;
        }
        vt/=iter; vmt/=iter; alg2t/=iter; alg4t/=iter;
        cout<<" Vitter time= "<<vt<<" VitterM time= "<<vmt<<" NewAlg2 time="<<alg2t<<" NewAlg4 time="<<alg4t<<endl;
    }
	system("pause");
}

