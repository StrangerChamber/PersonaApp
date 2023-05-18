#include "App.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include "Enclave_u.h"
#include "sgx_urts.h"
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>


#define MAX_PACKETS 50000

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

//ALL CODE BELOW WAS TAKEN FROM SGX SAMPLE CODE 
//ITS JUST SETTING UP THE ENCLAVE
typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }
    
    if (idx == ttl)
    	printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}


int initialize_enclave(void)
{
    char token_path[MAX_PATH] = {'\0'};
    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;
    
    /* Step 1: try to retrieve the launch token saved by last transaction 
     *         if there is no token, then create a new one.
     */
    /* try to get the token saved in $HOME */
    const char *home_dir = getpwuid(getuid())->pw_dir;
    
    if (home_dir != NULL && 
        (strlen(home_dir)+strlen("/")+sizeof(TOKEN_FILENAME)+1) <= MAX_PATH) {
        /* compose the token path */
        strncpy(token_path, home_dir, strlen(home_dir));
        strncat(token_path, "/", strlen("/"));
        strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME)+1);
    } else {
        /* if token path is too long or $HOME is NULL */
        strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
    }

    FILE *fp = fopen(token_path, "rb");
    if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
        printf("Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
    }

    if (fp != NULL) {
        /* read the token from saved file */
        size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
        if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
            /* if token is invalid, clear the buffer */
            memset(&token, 0x0, sizeof(sgx_launch_token_t));
            printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
        }
    }
    /* Step 2: call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        if (fp != NULL) fclose(fp);
        return -1;
    }

    /* Step 3: save the launch token if it is updated */
    if (updated == FALSE || fp == NULL) {
        /* if the token is not updated, or file handler is invalid, do not perform saving */
        if (fp != NULL) fclose(fp);
        return 0;
    }

    /* reopen the file with write capablity */
    fp = freopen(token_path, "wb", fp);
    if (fp == NULL) return 0;
    size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
    if (write_num != sizeof(sgx_launch_token_t))
        printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
    fclose(fp);
    return 0;
}


//this is super helpful for debugging i can print from the enclave
void ocall_print_string(const char *str)
{
    printf("%s", str);
}

// Function to calculate the time difference between two timeval structures
struct timeval timeval_subtract(struct timeval t1, struct timeval t2) {
    struct timeval result;
    result.tv_sec = t1.tv_sec - t2.tv_sec;
    result.tv_usec = t1.tv_usec - t2.tv_usec;

    if (result.tv_usec < 0) {
        result.tv_sec--;
        result.tv_usec += 1000000;
    }

    return result;
}

//FUNCTION FOR PRINTING THE TIMEVAL OF A PACKET
void print_timeval(struct timeval tv) {
    time_t timestamp = tv.tv_sec;
    struct tm *local_time = localtime(&timestamp);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    printf("Timestamp: %s.%06d\n", time_str, tv.tv_usec);
}



int main(int argc, char* argv[]){
        //-----------VARIABLES------------- 
    
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    if(argc<4){
        printf("program needs 4 args ./program pcapFile1Name TargetIP1 pcapFile2 TargetIP2");
        return 1;
    }
    const char *pcap_file1 = argv[1];
    const char *ip_address1 = argv[2]; //stefan ip = 192.168.50.53 //snow09 ip = 128.138.158.26
    const char* pcap_file2 = argv[3];
    const char* ip_address2 = argv[4];
    struct pcap_pkthdr header;
    const unsigned char *packet;
    struct ip *iph;
    struct timeval prev;
    int packet_count_sent1 = 0;
    int packet_count_sent2 = 0;
    double sum_time_diffs = 0.0;
    double time_diffs1[MAX_PACKETS];
    double time_diffs2[MAX_PACKETS];
    int packetLength1[MAX_PACKETS];
    int packetLength2[MAX_PACKETS];
    int index = 0;
    double timeToWait=0.0;
    struct timeval first_packet_timestamp;
    struct timeval last_packet_timestamp;
    int first_packet = 1;
    unsigned char* packet_buffer = (unsigned char*) malloc(MAX_BUF_LEN * sizeof(unsigned char));; 

    //init enclave
    if(initialize_enclave() < 0){
        printf("Enclave Init Failed\n");
        return -1; 
    }



    //OPEN PCAP FILE
    handle = pcap_open_offline(pcap_file1, errbuf);
    if (handle == NULL) {
        printf("Error opening pcap file: %s\n", errbuf);
        return 1;
    }

    //ITERATE THROUGH ALL PACKETS
    while ((packet = pcap_next(handle, &header)) != NULL) {
        iph = (struct ip *)(packet + 14);
        
        // Check if packet MATCHES CORRECT SENT IP ADDRESS
	if (strcmp(inet_ntoa(iph->ip_src), ip_address1)==0 || strcmp(inet_ntoa(iph->ip_dst), ip_address1) == 0) {
            packet_count_sent1++;

            //STORE FIRST PACKET INFO FOR USE LATER
            if (first_packet) {
                first_packet_timestamp = header.ts;
                first_packet = 0;
            }

            //STORE LAST PACKET INFO FOR LATER USE
            last_packet_timestamp = header.ts;


            //FIND TIME ELAPSED BETWEEN PACKETS
            struct timeval current_timestamp = header.ts;
            if(index>0){
                struct timeval diffs = timeval_subtract(current_timestamp, prev);
                double difference_in_seconds = diffs.tv_sec + diffs.tv_usec / 1e6;
                time_diffs1[index] = difference_in_seconds;
                sum_time_diffs += time_diffs1[index];
                prev = current_timestamp;
            }
            else{
                time_diffs1[index] = 0.0;
                prev = current_timestamp;
            }
	    

	    //ECALL to add packet info to persona
	    sgx_status_t check0 = addNewPacket(global_eid,1, packet, header.caplen, time_diffs1[index]);
	    
	    packetLength1[index] = header.caplen;
	    index++;
       	    struct timeval diff = timeval_subtract(last_packet_timestamp, first_packet_timestamp);
	    if((int)diff.tv_sec>=5){
        	break;
	    }
       	}
    }

    //CALCULATE TOTAL TIME ELAPSED FOR SANITY CHECK
    struct timeval dif = timeval_subtract(last_packet_timestamp, first_packet_timestamp);
    double difference_in_seconds = dif.tv_sec + dif.tv_usec / 1e6;

    printf("The time difference between the first and last packets is: %.6f seconds\n", difference_in_seconds);
    printf("the number of packets added is: %i \n", index);
 
    pcap_close(handle);
    index=0;
    first_packet = 1;

	
    //END OF PROCESSING FIRST FILE NOW DO SECOND FILE
    
    //OPEN PCAP FILE
    handle = pcap_open_offline(pcap_file2, errbuf);
    if (handle == NULL) {
        printf("Error opening pcap file: %s\n", errbuf);
        return 1;
    }

    //ITERATE THROUGH ALL PACKETS
    while ((packet = pcap_next(handle, &header)) != NULL) {
    	iph = (struct ip *)(packet + 14);

        // Check if packet MATCHES CORRECT SENT IP ADDRESS
        if (strcmp(inet_ntoa(iph->ip_src), ip_address2)==0 || strcmp(inet_ntoa(iph->ip_dst), ip_address2) == 0) {
            packet_count_sent2++;

            //STORE FIRST PACKET INFO FOR USE LATER
            if (first_packet) {
                first_packet_timestamp = header.ts;
                first_packet = 0;
            }

            //STORE LAST PACKET INFO FOR LATER USE
            last_packet_timestamp = header.ts;


            //FIND TIME ELAPSED BETWEEN PACKETS
            struct timeval current_timestamp = header.ts;
            if(index>0){
                struct timeval diffs = timeval_subtract(current_timestamp, prev);
                double difference_in_seconds = diffs.tv_sec + diffs.tv_usec / 1e6;
                time_diffs2[index] = difference_in_seconds;
                sum_time_diffs += time_diffs1[index];
                prev = current_timestamp;
            }
            else{
                time_diffs2[index] = 0;
		prev = current_timestamp;
            }


            //ECALL to add packet info to persona
            sgx_status_t check0 = addNewPacket(global_eid,2, packet, header.caplen, time_diffs2[index]);

            packetLength2[index] = header.caplen;
            index++;
            struct timeval diff = timeval_subtract(last_packet_timestamp, first_packet_timestamp);
            if((int)diff.tv_sec>=5){
                break;
            }
        }
    }

    //CALCULATE TOTAL TIME ELAPSED FOR SANITY CHECK
    dif = timeval_subtract(last_packet_timestamp, first_packet_timestamp);
    difference_in_seconds = dif.tv_sec + dif.tv_usec / 1e6;
    printf("The time difference between the first and last packet from file 2 is: %.6f seconds\n", difference_in_seconds);
    printf("the number of packets added from file 2 is: %i \n", index);
    //open network device to send packets over
    pcap_close(handle);
    index=0;
	 
	    
    //now time to open network device and start sending packets   
	    
    pcap_t *DevHandle = pcap_open_live("eno1", BUFSIZE, 100000, 1000, errbuf);
    if( DevHandle == NULL){
	    fprintf(stderr, "couldn't open device : %s\n", errbuf);
    	    return 2;
    }

    int personaCheck;
    //main control loop
    if(packet_count_sent1<packet_count_sent2){
    	personaCheck = packet_count_sent1;
    }else{
	personaCheck = packet_count_sent2;
    }	
    int personaCount = 1;
    while(true){
        if(index == personaCheck){
		index = 0;
		if(personaCount == 1){
			personaCount = 2;
		}else{
			personaCount = 1;
		}
		
	}
	if(personaCount == 1){	
		//get next packet
		//printf("here\n");
		sgx_status_t check = getPacket(global_eid,1, packet_buffer, MAX_BUF_LEN);
		if(check != SGX_SUCCESS){
			printf("getpacket1 failed with: %x\n", check);
			return 2;
		}
		//printf("above time\n");
		//get time to wait
		sgx_status_t check2 = getTime(global_eid, &timeToWait,1);
		if(check2 != SGX_SUCCESS){
			printf("get time1 failed with: %x\n", check2);
			return 2;
		}
	
        	if(packet_buffer == NULL){
			printf("buf is null");
			return 2;
		}	

		//inject packet	
        	if(pcap_inject(DevHandle, packet_buffer, packetLength1[index]) == -1){
			fprintf(stderr, "Error sending packet: %s\n", pcap_geterr(DevHandle));
	    	}
	}else{
		//get next packet
                sgx_status_t check = getPacket(global_eid,2, packet_buffer, MAX_BUF_LEN);
                if(check != SGX_SUCCESS){
                        printf("getpacket2 failed with: %x\n", check);
                        return 2;
                }

                //get time to wait
                sgx_status_t check2 = getTime(global_eid, &timeToWait,2);
                if(check2 != SGX_SUCCESS){
                        printf("get time2 failed with: %x\n", check2);
                        return 2;
                }

                if(packet_buffer == NULL){
                        printf("buf is null");
                        return 2;
                }
		//printf("second inject\n");
                //inject packet
                if(pcap_inject(DevHandle, packet_buffer, packetLength2[index]) == -1){
                        fprintf(stderr, "Error sending packet: %s\n", pcap_geterr(DevHandle));
                }
	}
	
	//wait	
	printf("time to wait is: %f\n", timeToWait);
	struct timespec ts;
        ts.tv_sec = (time_t)timeToWait; // extract the whole seconds part
        ts.tv_nsec = (long)((timeToWait	- ts.tv_sec) * 1e9); // extract the fractional part and convert from seconds to nanoseconds
	index++;
	nanosleep(&ts, NULL);
    }

    pcap_close(DevHandle);
    //when done close out enclave 
    sgx_destroy_enclave(global_eid);
    return 1;
}
