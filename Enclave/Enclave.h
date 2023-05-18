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

class Packet{
public: 
	vector<unsigned char> data;
	Packet(){}

	Packet(const unsigned char * raw_data, int lengths){
		data.assign(raw_data, raw_data+lengths);
	}
};

//DEFINE MY CLASS HERE
class Persona{
   
    public:
   
    void addTime(double timeTW){
    	//add time to wait to vector
	timeToWait.push_back(timeTW);
	return;
    }

    void addPacket(const unsigned char *raw_data, int length){
	    packets.emplace_back(raw_data, length);
    }

    const Packet& getPacket(int index) const{
	    return packets.at(index);
    }

    int getPacketCount(){
	    return packets.size();
    }

    double getTTW(int index){
	    return timeToWait.at(index);
    }

    vector<Packet> packets;
    vector<char*> urls;//here is the list of URLs to be accessed 
    vector<double> timeToWait;//here is the time in seconds that each url should be visited
    vector<double> mean;//mean time between packets sent
    vector<double> std_dev;//standard deviation of the time between packet sends
};


//ECALL definition
//void getpersona(char*  buffer, size_t len);

//used for debugging
int printf(const char* fmt, ...);


#if defined(__cplusplus)
}
#endif

#endif
