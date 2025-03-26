#include "app/eapp_utils.h"
#include "app/string.h"
#include "app/syscall.h"
#include "app/malloc.h"
#include "edge_wrapper.h"

void EAPP_ENTRY eapp_entry() {
  edge_init();

  const char* msg = "hello world!\n";
  const char* msg2 = "2nd hello world!\n";

  // Print both messages
  unsigned long ret = ocall_print_buffer((char*)msg, strlen(msg));
  ocall_print_buffer((char*)msg2, strlen(msg2));

  ocall_print_value(ret);

  // Receive string from host
  struct edge_data pkgstr;
  ocall_get_string(&pkgstr);

  if (pkgstr.size == 0) {
    ocall_print_value(0);  // No data received
    EAPP_RETURN(0);
  }

  void* host_str = malloc(pkgstr.size); // assign it another value to determine the issue behind the 0
  if (!host_str) {
    ocall_print_value(0);  // malloc failed
    EAPP_RETURN(0);
  }

  copy_from_shared(host_str, pkgstr.offset, pkgstr.size);

  // Count occurrences of 'l'
  int ct = 0;
  for (int i = 0; i < pkgstr.size; i++) {
    if (((char*)host_str)[i] == 'l') {
      ct++;
    }
  }

  ocall_print_value(ct);

  EAPP_RETURN(ct);
}

