#ifndef EDGE_WRAPPER_H
#define EDGE_WRAPPER_H

#include <stddef.h>
#include "edge/edge_call.h"  

#define OCALL_PRINT_BUFFER 1
#define OCALL_PRINT_VALUE 2
#define OCALL_GET_STRING  4

void edge_init(); 
unsigned long ocall_print_buffer(char* str, size_t len);
void ocall_print_value(unsigned long val);
void ocall_get_string(struct edge_data* retdata);

#endif // EDGE_WRAPPER_H

