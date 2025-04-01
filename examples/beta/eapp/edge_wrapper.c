#include "edge_wrapper.h"
#include "edge_call.h" // for ocall(...)
#include <string.h>     // for strlen if needed

#define OCALL_PRINT_INT 2

unsigned long ocall_print_int(int value)
{
    unsigned long retval;
    ocall(OCALL_PRINT_INT, &value, sizeof(int), &retval, sizeof(unsigned long));
    return retval;
}

