#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <cstdio>
using namespace std;


#if defined(__cplusplus)
extern "C" {
#endif


//DEFINE MY CLASS HERE
class Persona{
   
    public:
   
    void addInfo(char* url, int seconds){
    	//fill both vectors
    	urls.push_back(url);
    	time.push_back(seconds);
    	return;
    }

    vector<char*> urls;//here is the list of URLs to be accessed 
    vector<int> time;//here is the time in seconds that each url should be visited
};


//ECALL definition
//void getpersona(char*  buffer, size_t len);

//used for debugging
int printf(const char* fmt, ...);


#if defined(__cplusplus)
}
#endif

#endif
