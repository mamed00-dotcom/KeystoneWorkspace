#include "app/eapp_utils.h"
#include "app/syscall.h"

int main() {
    // This enclave has no runtime logic; it just returns.
    // Its ELF on disk is what the Attestor will hash/measure.
    EAPP_RETURN(0);
}
