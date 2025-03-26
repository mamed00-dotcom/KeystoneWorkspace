#include "edge_wrapper.h"
#include <iostream>
#include <cstring>

void edge_init(Keystone::Enclave* enclave) {
    enclave->registerOcallDispatch([](void* buffer) {
        struct edge_call* edge_call = (struct edge_call*) buffer;

        switch (edge_call->call_id) {
            case OCALL_PRINT_BUFFER:
                handle_ocall_print_buffer(buffer);
                break;
            case OCALL_PRINT_VALUE:
                handle_ocall_print_value(buffer);
                break;
            case OCALL_GET_STRING:
                handle_ocall_get_string(buffer);
                break;
            default:
                std::cerr << "[Host] Unknown OCALL ID: " << edge_call->call_id << std::endl;
                break;
        }
    });
}

void handle_ocall_print_buffer(void* buffer) {
    struct edge_call* edge_call = (struct edge_call*) buffer;
    uintptr_t data_ptr;
    size_t data_len;

    if (edge_call_args_ptr(edge_call, &data_ptr, &data_len) != 0) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
        return;
    }

    void* data = (void*) data_ptr;
    fwrite(data, 1, data_len, stdout);
    fflush(stdout);

    unsigned long ret = data_len;
    uintptr_t ret_ptr = (uintptr_t) &ret;
    size_t ret_size = sizeof(unsigned long);
    edge_call->return_data.call_status = CALL_STATUS_OK;
    edge_call_ret_ptr(edge_call, &ret_ptr, &ret_size);
}

void handle_ocall_print_value(void* buffer) {
    struct edge_call* edge_call = (struct edge_call*) buffer;
    uintptr_t data_ptr;
    size_t data_len;

    if (edge_call_args_ptr(edge_call, &data_ptr, &data_len) != 0) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
        return;
    }

    void* data = (void*) data_ptr;
    unsigned long val = *((unsigned long*) data);
    std::cout << "[Host] Value: " << val << std::endl;

    edge_call->return_data.call_status = CALL_STATUS_OK;
}

void handle_ocall_get_string(void* buffer) {
    struct edge_call* edge_call = (struct edge_call*) buffer;

    const char* str = "Hello from Host!";
    uintptr_t str_ptr = (uintptr_t) str;
    size_t str_len = strlen(str);

    edge_call->return_data.call_status = CALL_STATUS_OK;
    edge_call_ret_ptr(edge_call, &str_ptr, &str_len);
}

