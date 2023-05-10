#include "Enclave.h"
#include "sgx_trts.h"
#include "Enclave_t.h"
#include <iostream>
#include <unistd.h>
//#include <cstdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_BUF_LEN 100
#define PERSONA_COUNT 3

int urlCount = 0;
int personaCount = 0;

//ENCLAVE ENTRY POINT
void getpersona(char * buffer, size_t len){
  
    //#printf("Inside the enclave, buffer variable is: %s\n",buffer);
    uint64_t sleepCount = 5; //hardcode for now to make sure producer consumer relationship is correct

    //create instances of class for my different personas (hard code 3 for now)
    class Persona jack;
    class Persona jody;
    class Persona keith;

    //get them filled with all the right data
    //the time data is arbitrary as of now and is in seconds
    
    char* cnn = "www.cnn.com";
    char* msn = "www.msn.com";
    char* aol = "www.aol.com";
    char * twitter = "www.twitter.com";
    char * instagram = "www.instagram.com";  
    char * stackOF = "www.stackoverflow.com";
    char* google = "www.google.com";
    char * gmail = "www.gmail.com";
    char* outlook = "www.outlook.com";
    char* wordscapes = "www.wordscapes.com";
    char* cozi = "www.cozi.com";

    jack.addInfo(twitter, 10);
    jack.addInfo(instagram, 20);
    jack.addInfo(stackOF, 10);
    jack.addInfo(google, 5);
    jack.addInfo(gmail, 50);

    
    jody.addInfo(twitter, 3);
    jody.addInfo(outlook, 16);
    jody.addInfo(wordscapes, 30);
    jody.addInfo(google, 20);
    jody.addInfo(cozi, 10);


    keith.addInfo(cnn, 50);
    keith.addInfo(msn, 10);
    keith.addInfo(aol, 60);
    keith.addInfo(gmail, 14);
    keith.addInfo(outlook, 89);


   //put all personas into a vector of personas
 
    vector<Persona> personas;
    personas.push_back(jack);
    personas.push_back(jody);
    personas.push_back(keith);
    
    //printf("concat string is: %s\n", personas[personaCount].urls[urlCount]);    
    memcpy(buffer,(char*)personas[personaCount].urls[urlCount],strlen(personas[personaCount].urls[urlCount])+1);
    //*time = personas[personaCount].time[urlCount]; 
    urlCount++;
    //check if i need to go to next persona
    if(urlCount == personas[personaCount].urls.size()){
        urlCount = 0;
        personaCount++;
    }
    if (personaCount == personas.size()){
	    personaCount = 0;
    }
    //printf("Printing the buffer variable and time variable while still in enclave: %s, %d\n", buffer, *time);
    return;
}

//this is so i can print from within enclave to help debug
int printf(const char* fmt, ...)
{
  char buf[MAX_BUF_LEN] = {'\0'};
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, MAX_BUF_LEN, fmt, ap);
  va_end(ap);
  ocall_print_string(buf);
  return 1;
}
