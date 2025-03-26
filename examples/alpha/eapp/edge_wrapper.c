#include <string.h>
#include "app/eapp_utils.h"
#include "app/string.h"
#include "app/syscall.h"
#include "edge_wrapper.h"
#include "edge/edge_call.h" 

#define OCALL_PRINT_BUFFER 1
#define OCALL_PRINT_VALUE 2
#define OCALL_GET_STRING 4

void edge_init() {

}

void ocall_print_value(unsigned long val) {
  unsigned long val_ = val;
  ocall(OCALL_PRINT_VALUE, &val_, sizeof(unsigned long), 0, 0);
}

unsigned long ocall_print_buffer(char* data, size_t data_len) {
  unsigned long retval;
  ocall(OCALL_PRINT_BUFFER, data, data_len, &retval, sizeof(unsigned long));
  return retval;
}

void ocall_get_string(struct edge_data* retdata) {
  ocall(OCALL_GET_STRING, NULL, 0, retdata, sizeof(struct edge_data));
}

