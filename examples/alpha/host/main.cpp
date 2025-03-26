#include <stdio.h>
#include <string.h>
#include "keystone.h"
#include "edge_call.h"
#include "edge_wrapper.h"

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: %s <enclave> <runtime> <loader>\n", argv[0]);
        return 1;
    }

    Keystone::Enclave enclave;
    Keystone::Params params;
    params.setUntrustedSize(1024 * 1024);  // 1MB UTM

    if (enclave.init(argv[1], argv[2], argv[3], params) != Keystone::Error::Success) {
        printf("[Host] Failed to initialize enclave.\n");
        return 1;
    }

    edge_init(&enclave);  // Register OCALL dispatcher

    uintptr_t retval = 0;
    Keystone::Error err;

    do {
        err = enclave.run(&retval);
        // If there's an OCALL, the lambda in edge_wrapper handles it.
    } while (err == Keystone::Error::EdgeCallHost);

    if (err != Keystone::Error::Success) {
        printf("[Host] Enclave exited with error.\n");
        return 1;
    }

    printf("[Host] Enclave final return value: %lu\n", retval);
    return 0;
}

