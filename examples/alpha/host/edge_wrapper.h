#ifndef HOST_EDGE_WRAPPER_H
#define HOST_EDGE_WRAPPER_H

#include "edge/edge_call.h"
#include "host/keystone.h"

// Define OCALLs
#define OCALL_PRINT_BUFFER 1
#define OCALL_PRINT_VALUE 2
#define OCALL_GET_STRING   4

// OCALL dispatcher setup
void edge_init(Keystone::Enclave* enclave);

// OCALL handler declarations
void handle_ocall_print_value(void* buffer);
void handle_ocall_print_buffer(void* buffer);
void handle_ocall_get_string(void* buffer);

#endif // HOST_EDGE_WRAPPER_H

