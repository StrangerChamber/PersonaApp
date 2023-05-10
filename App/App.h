#ifndef _APP_H_
#define _APP_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include <unistd.h>
#include <pwd.h>
#include <thread>
#include "sgx_error.h"       /* sgx_status_t */
#include "sgx_eid.h"     /* sgx_enclave_id_t */
#include "sgx_urts.h"
#include "Enclave_u.h"
using namespace std;

#define MAX_BUF_LEN 100 //NEED BETTER LABELS //THIS IS FOR THE THREAD BUFFER
#define BUFSIZE 1000 //THIS IS FOR THE BUFFER FOR GET REQUESTS
#define MAX_PATH FILENAME_MAX

#ifndef TRUE
# define TRUE 1
#endif

#ifndef FALSE
# define FALSE 0
#endif

# define TOKEN_FILENAME   "enclave.token"
# define ENCLAVE_FILENAME "enclave.signed.so"

extern sgx_enclave_id_t global_eid;    /* global enclave id */

#if defined(__cplusplus)
extern "C" {
#endif
//dont think i'll need these functions below
void edger8r_array_attributes(void);
void edger8r_type_attributes(void);
void edger8r_pointer_attributes(void);
void edger8r_function_attributes(void);

void ecall_libc_functions(void);
void ecall_libcxx_functions(void);
void ecall_thread_functions(void);

#if defined(__cplusplus)
}
#endif

#endif
