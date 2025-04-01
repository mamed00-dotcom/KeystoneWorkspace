#include "eapp_utils.h"
#include "string.h"
#include "edge_call.h"
#include <syscall.h>

#define OCALL_PRINT_INT 2

unsigned long ocall_print_int(int value);

int main() {
    const char* str = "Hello from the last lebululu!";
    int count = 0;

    for (int i = 0; str[i]; i++) {
        if (str[i] == 'l') count++;
    }

    ocall_print_int(count);

    EAPP_RETURN(0);
}

unsigned long ocall_print_int(int value) {
    unsigned long retval;
    ocall(OCALL_PRINT_INT, &value, sizeof(int), &retval, sizeof(unsigned long));
    return retval;
}

