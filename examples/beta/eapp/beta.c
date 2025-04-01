#include "eapp_utils.h"
#include "string.h"
#include "edge_call.h"
#include <syscall.h>

#include "edge_wrapper.h"  // For ocall_print_int()

int main() {
    const char* str = "la la la la la!";
    int count = 0;

    for (int i = 0; str[i]; i++) {
        if (str[i] == 'l') count++;
    }

    ocall_print_int(count);

    EAPP_RETURN(0);
}

