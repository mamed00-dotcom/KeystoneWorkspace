#ifndef HOST_EDGE_WRAPPER_H
#define HOST_EDGE_WRAPPER_H

#include <edge_call.h>

// same numeric IDs must match the enclave
#define OCALL_PRINT_INT 2

#ifdef __cplusplus
extern "C" {
#endif

void print_int_wrapper(void* buffer);

#ifdef __cplusplus
}
#endif

#endif // HOST_EDGE_WRAPPER_H

