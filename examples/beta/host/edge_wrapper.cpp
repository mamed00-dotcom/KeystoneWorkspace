#include <stdio.h>
#include <string.h>     // for memcpy
#include "edge_call.h"
#include "edge_wrapper.h"

#define OCALL_PRINT_INT 2

// actual printing logic
static unsigned long print_int(int* value) {
    printf("[host] Count from enclave: %d\n", *value);
    return 0;
}

void print_int_wrapper(void* buffer) {
    struct edge_call* edge_call = (struct edge_call*) buffer;
    uintptr_t call_args;
    size_t arg_len;
    unsigned long ret_val;

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

