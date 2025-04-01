#include <keystone.h>     // For Keystone::Enclave and Keystone::Params
#include <edge_call.h>    // For incoming_call_dispatch, register_call
#include <stdio.h>        // For printf
#include <string.h>       

// Match these IDs with the enclave side
#define OCALL_PRINT_STRING 1
#define OCALL_PRINT_INT    2

// Forward declare the host logic
unsigned long print_string(char* str);
unsigned long print_int(int* value);

void print_string_wrapper(void* buffer);
void print_int_wrapper(void* buffer);

// ---------------------------------------------------
// Actual host functions
unsigned long print_string(char* str) {
    printf("%s\n", str);
    return 0;
}

unsigned long print_int(int* value) {
    printf("Résultat reçu depuis l'enclave: %d\n", *value);
    return 0;
}

// ---------------------------------------------------
// Wrappers to parse the edge_call struct
void print_string_wrapper(void* buffer) {
    struct edge_call* edge_call = (struct edge_call*)buffer;
    uintptr_t call_args;
    unsigned long ret_val;
    size_t arg_len;

    if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
        return;
    }

    // Actually call the host function
    ret_val = print_string((char*)call_args);

    // Return data
    uintptr_t data_section = edge_call_data_ptr();
    memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
    if (edge_call_setup_ret(edge_call, (void*)data_section, sizeof(unsigned long))) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    } else {
        edge_call->return_data.call_status = CALL_STATUS_OK;
    }
}

void print_int_wrapper(void* buffer) {
    struct edge_call* edge_call = (struct edge_call*)buffer;
    uintptr_t call_args;
    unsigned long ret_val;
    size_t arg_len;

    if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
        return;
    }

    ret_val = print_int((int*)call_args);

    uintptr_t data_section = edge_call_data_ptr();
    memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
    if (edge_call_setup_ret(edge_call, (void*)data_section, sizeof(unsigned long))) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    } else {
        edge_call->return_data.call_status = CALL_STATUS_OK;
    }
}

// ---------------------------------------------------
// Main host entry
int main(int argc, char** argv) {
  
  Keystone::Enclave enclave;
  Keystone::Params params;

  // We can do up to ~1MB of free mem and untrusted mem
  params.setFreeMemSize(1024 * 1024);
  params.setUntrustedSize(1024 * 1024);

  // enclave initialization 
  enclave.init(argv[1], argv[2], argv[3], params);

  // This sets the global incoming_call_dispatch
  enclave.registerOcallDispatch(incoming_call_dispatch);

  // Then we register our calls
  register_call(OCALL_PRINT_STRING, print_string_wrapper);
  register_call(OCALL_PRINT_INT,    print_int_wrapper);

  // Init the edge call region
  edge_call_init_internals(
      (uintptr_t) enclave.getSharedBuffer(),
      enclave.getSharedBufferSize()
  );

  // Final we run
  enclave.run();

  return 0;
}

