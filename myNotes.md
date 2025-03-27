
# Memom Example for Keystone

This repository branch introduces the **memom** example as a new Keystone enclave application. It demonstrates a simple enclave that prints a custom message and is built following the same pattern as the official "hello" example provided by Keystone.

## Overview

- **Enclave Application (EApp):** 
  The enclave code is written in C and located in the `examples/memom/eapp` directory. It simply prints a message to standard output when executed.

- **Host Application:** 
  The host code, written in C++, initializes the enclave and starts its execution. It is located in `examples/memom/host/mhost.cpp` (the target is named `memom-runner`).

- **Packaging:** 
  The build system uses CMake macros (defined in the Keystone SDK) to generate the Eyrie runtime (including files like `eyrie-rt` and `loader.bin`) and then packages everything into a self-extracting archive (`memom.ke`).

## Directory Structure

```
keystone/
└── examples/
    └── memom/
        ├── CMakeLists.txt         # Build instructions for the memom example.
        ├── eapp/
        │   └── memom.c            # Enclave application source.
        └── host/
            └── mhost.cpp          # Host application source.
```

## How It Was Added

1. **Created the memom Example Folder:** 
   A new folder `memom` was created under the `examples` directory. Inside this folder, two subdirectories were created: 
   - `eapp/` for the enclave application code. 
   - `host/` for the host application code.

2. **Implemented the Enclave (EApp) Code:** 
   In `eapp/memom.c`, a simple C program was written that prints a message:
```c
   #include <stdio.h>
   int main() {
       printf("Hello from my custom Keystone Enclave (memom)!\n");
       return 0;
   }
```

3. **Implemented the Host Code:** 
   In `host/mhost.cpp`, the host code was modeled after the official "hello" example. It initializes the enclave, sets memory parameters, registers an ocall dispatch function (or uses `nullptr`), and then runs the enclave:
   
```cpp
#include "edge/edge_call.h"
#include "host/keystone.h"

using namespace Keystone;

int
main(int argc, char** argv) {
  Enclave enclave;
  Params params;

  params.setFreeMemSize(256 * 1024);
  params.setUntrustedSize(256 * 1024);

  enclave.init(argv[1], argv[2], argv[3], params);

  enclave.registerOcallDispatch(incoming_call_dispatch);
  edge_call_init_internals(
      (uintptr_t)enclave.getSharedBuffer(), enclave.getSharedBufferSize());

  enclave.run();

  return 0;
}
```

4. **Configured CMake:** 
   A new `CMakeLists.txt` was added in the `memom` folder. This file sets up the build targets for the enclave, host, runtime, and packaging of the memom example:
```cmake
set(eapp_bin memom)
set(eapp_src eapp/memom.c)
set(host_bin memom-runner)
set(host_src host/mhost.cpp)
set(package_name "memom.ke")
set(package_script "./memom-runner memom eyrie-rt loader.bin")
set(eyrie_plugins "io_syscall linux_syscall env_setup")


# Enclave Application (EApp)
add_executable(${eapp_bin} ${eapp_src})
target_link_libraries(${eapp_bin} "-static")


# Host Application
add_executable(${host_bin} ${host_src})
target_link_libraries(${host_bin} ${KEYSTONE_LIB_HOST} ${KEYSTONE_LIB_EDGE})


# Eyrie Runtime
set(eyrie_files_to_copy .options_log eyrie-rt loader.bin)
add_eyrie_runtime(${eapp_bin}-eyrie
  ${eyrie_plugins}
  ${eyrie_files_to_copy})


# Packaging into a .ke archive
add_keystone_package(${eapp_bin}-package
  ${package_name}
  ${package_script}
  ${eyrie_files_to_copy} ${eapp_bin} ${host_bin})

add_dependencies(${eapp_bin}-package ${eapp_bin}-eyrie)


# Register the package target with the top-level "examples" target
add_dependencies(examples ${eapp_bin}-package)
```

5. **Registered the Example:** 
   The top-level `CMakeLists.txt` in `~/keystone/examples` was updated to include the new example:
   ```cmake
   add_subdirectory(memom)
   ```

## Build and Run Instructions

1. **Build the Keystone Repository:** 
   From the root directory:
   ```bash
   cd ~/keystone
   make -j$(nproc)
   ```

2. **Run the QEMU Image:** 
   After a successful build, run:
   ```bash
   make run
   ```

3. **Run the memom Example in QEMU:** 
   Once inside the QEMU environment, navigate to the directory where examples are located (typically `/usr/share/keystone/examples` or `/root`), then run:
   ```bash
   ./memom.ke
   ```
   You should see the output:
   ```
   Hello from my custom Keystone Enclave (memom)!
   ```

## Further Notes
- **CMake Customization:** 
  The template is based on the “hello” example. Any additional libraries or include directories required by your application can be added in the `CMakeLists.txt`.
- **Cmake TEmplate**
```CMake
### Set variables for enclave app and host :
set(eapp_bin        memom)               # The name of The enclave app
set(eapp_src        eapp/memom.c)        # Source for The EApp

set(host_bin        memom-runner)        # The name of The host app
set(host_src        host/memom-runner.cpp) 

###  Name of the final .ke archive
set(package_name    "memom.ke")

###The command line to run after the .ke file is extracted on the device
### Typically includes the host app, the EApp name, and the Eyrie runtime files
set(package_script  "./memom-runner memom eyrie-rt loader.bin")

### Eyrie plugins could be "linux_syscall io_syscall env_setup" or "none" 
### depending on which syscalls you need inside the enclave
set(eyrie_plugins   "linux_syscall io_syscall env_setup")

### Eyrie runtime files to copy
set(eyrie_files_to_copy  .options_log eyrie-rt loader.bin)

#------------------------------------------------------------------------------
### EApp (enclave code)

add_executable(${eapp_bin} ${eapp_src})
target_link_libraries(${eapp_bin}
  -nostdlib -static
  ${KEYSTONE_LIB_EAPP}
  ${KEYSTONE_LIB_EDGE}
)

# Optionally add include paths for the EApp
target_include_directories(${eapp_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/app
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
)

#------------------------------------------------------------------------------
### Host (untrusted code)

add_executable(${host_bin} ${host_src})
target_link_libraries(${host_bin}
  ${KEYSTONE_LIB_HOST}
  ${KEYSTONE_LIB_EDGE}
  # ${KEYSTONE_LIB_VERIFIER}  # Uncomment if you need attestation
)

### If the host is C++, you may want to set compiler standards:
set_target_properties(${host_bin}
  PROPERTIES CXX_STANDARD 17
             CXX_STANDARD_REQUIRED YES
             CXX_EXTENSIONS NO
)

### Optionally add include paths for the Host
target_include_directories(${host_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/common
  PUBLIC ${KEYSTONE_SDK_DIR}/include/host
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
  # PUBLIC ${KEYSTONE_SDK_DIR}/include/verifier  # If you need attestation
)

#------------------------------------------------------------------------------
### Add Eyrie runtime (macro defined in the Keystone SDK macros.cmake)

add_eyrie_runtime(${eapp_bin}-eyrie
  ${eyrie_plugins}
  ${eyrie_files_to_copy}
)

#------------------------------------------------------------------------------
### Packaging into a .ke self-extracting archive

add_keystone_package(${eapp_bin}-package
  ${package_name}
  ${package_script}
  ${eyrie_files_to_copy}
  ${eapp_bin}
  ${host_bin}
)

add_dependencies(${eapp_bin}-package ${eapp_bin}-eyrie)
```


