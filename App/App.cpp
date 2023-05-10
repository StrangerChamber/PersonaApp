#include "App.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include "Enclave_u.h"
#include "sgx_urts.h"

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

//constant for how i define my get request
//right now this always return back bad request
//GONNA NEED TO CHANGE WHEN DOING MORE ADVANCED FUNCTIONALITY
const char* httprequest = "GET / HTTP/1.1\r\n"
    "Host: google.com/\r\n"
    "Upgrade-Insecure-Requests: 1\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    "Connection: keep-alive\r\n"
    "If-Modified-Since: Mon, 18 Jul 2016 02:36:04 GMT\r\n"
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/60.0.3112.50 Safari/537.36\r\n"
    "\r\n";

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


int main(){
    int sock;
    int err;
    char buffer[MAX_BUF_LEN]= "initial string";
    srand(time(NULL));
    //init enclave
    if(initialize_enclave() < 0){
        printf("Enclave Init Failed\n");
        return -1; 
    }

    //get next persona and their behavior
    while(true){
        //loop is infinite for now

    	//get next url
        getpersona(global_eid, buffer, MAX_BUF_LEN);               
       
       	//socket setup code
        printf("the buffer Is: %s\n", buffer);
        //printf("time is: %d\n", *time);
	char* hostname = buffer;
        char* service = "80";
        struct addrinfo hints, *res;
	
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family = AF_INET;

	if((err = getaddrinfo(hostname, service, &hints, &res)) != 0) {
	   printf("error %d : %s\n", err, gai_strerror(err));
             return 1;
         }

         sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
         if(sock < 0) {
             perror("socket");
             return 1;
         }

         if(connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
            perror("connect");
             return 1;
         }  
        

         //send request
         int send_result = send(sock, httprequest, strlen(httprequest), 0);
         printf("sent %d bytes to %s\n", send_result, buffer);

         char requestBuf[BUFSIZE + 1] = {0};

         //receive response
         int bytes_read = recv(sock, requestBuf, BUFSIZE, 0);
         printf("end recv: received %d bytes\n", bytes_read);

        requestBuf[BUFSIZE] = '\0';
        printf("Message back:\n");
        puts(requestBuf); 
        
	freeaddrinfo(res);

        //SLEEP UNTIL READY TO DO NEXT REQUEST 
        int newTime = rand() % 20 + 1;
	sleep(newTime);
	printf("after sleep\n");
    }
    
    //when done close out enclave 
    sgx_destroy_enclave(global_eid);
    return 1;
}
