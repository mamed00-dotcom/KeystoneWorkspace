#include <keystone.h>
#include <edge_call.h>
#include <stdio.h>
#include <string.h>

#include "edge_wrapper.h"  // for print_int_wrapper

int main(int argc, char** argv) {
  Keystone::Enclave enclave;
  Keystone::Params params;

  params.setFreeMemSize(1024 * 1024);
  params.setUntrustedSize(1024 * 1024);

  enclave.init(argv[1], argv[2], argv[3], params);

  enclave.registerOcallDispatch(incoming_call_dispatch);

  // register our OCALL wrapper
  register_call(OCALL_PRINT_INT, print_int_wrapper);

  edge_call_init_internals(
      (uintptr_t) enclave.getSharedBuffer(),
      enclave.getSharedBufferSize()
  );

  enclave.run();
  return 0;
}

