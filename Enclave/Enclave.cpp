#include "Enclave.h"
#include "sgx_trts.h"
#include "Enclave_t.h"

#define MAX_BUF_LEN 1000

int personaCount = 0;
class Persona jack;
class Persona kelly;
int timeIndex1 = 0;
int timeIndex2 = 0;
int index2 = 0;//using this to iterate over each packet
int index3 = 0;

void addNewPacket(int persona,const unsigned char *raw_packet, int length, double timeToWait){
	if(persona == 1){
		jack.addPacket(raw_packet, length);
		jack.addTime(timeToWait);
	}else{
		kelly.addPacket(raw_packet, length);
                kelly.addTime(timeToWait);
	}
	return;
}

double getTime(int persona){
	double TTW;
	if(persona == 1){
		TTW = jack.getTTW(timeIndex1);
		timeIndex1++;
		if(timeIndex1 == jack.getPacketCount()){
                	timeIndex1 = 0;
        	}
	}else{
		TTW = kelly.getTTW(timeIndex2);
                timeIndex2++;
                if(timeIndex2 == kelly.getPacketCount()){
                        timeIndex2 = 0;
                }
	}
	return TTW;
}


void getPacket(int persona, unsigned char *buffer, size_t len){
	class Packet pack;
	if(persona == 1){
		pack = jack.getPacket(index2);
		memcpy(buffer, pack.data.data(), pack.data.size());
		index2++;
		if(index2 == jack.getPacketCount()-1){
			index2 = 0;
			personaCount++;
		}
	}else{
		pack = kelly.getPacket(index3);
                memcpy(buffer, pack.data.data(), pack.data.size());
                index3++;
                if(index3 == kelly.getPacketCount()-1){
                        index3 = 0;
                        personaCount++;
                }
	}
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
