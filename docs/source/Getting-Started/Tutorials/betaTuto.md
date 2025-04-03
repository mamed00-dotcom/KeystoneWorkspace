# Beta Project

## Overview

The **beta** project demonstrates secure communication between a Keystone enclave application (eapp) and a host application using custom OCALL wrappers (`edge_wrapper`). This setup enhances code modularity and maintainability.

## Project Structure

```
beta/
├── CMakeLists.txt
├── app.lds
├── eapp/
│   ├── beta.c
│   ├── edge_wrapper.c
│   └── edge_wrapper.h
└── host/
    ├── main.cpp
    ├── edge_wrapper.cpp
    └── edge_wrapper.h
```

## Why use `edge_wrapper`?

Using `edge_wrapper` files allows separating OCALL logic from the main source files, significantly improving code readability, modularity, and making it easier to maintain or extend OCALL functionalities in larger projects.

## Explanation of the Linker Script (`app.lds`)

The linker script `app.lds` explicitly defines how sections (`.text`, `.data`, `.bss`) of the enclave binary are laid out in memory. This is essential for the Keystone enclave runtime, which expects a particular structure. It specifies the entry point (`_start`) and optionally reserves heap space for dynamic memory allocation if needed.

### Contents of `app.lds`:

```ld
ENTRY(_start)

SECTIONS {
  .text : { *(.text*) }
  .data : { *(.data*) }
  .bss  : { *(.bss*) *(COMMON) }

  /* optional: reserve space for dynamic memory allocation (malloc)
  .malloc (NOLOAD) : ALIGN(8) {
    __malloc_start = .;
    . = . + 0x10000;
    __malloc_end = .;
  }
  */

  . = ALIGN(0x1000);
  _end = .;
}
```

### Explanation:
- `ENTRY(_start)`: Defines the entry point of your enclave program. The runtime looks for `_start` to begin execution.
- `.text`, `.data`, `.bss`: Standard ELF sections for executable code, initialized data, and uninitialized data respectively.
- The optional `.malloc` section reserves memory if dynamic allocation (malloc) is required in your enclave.
- `_end`: Symbol marking the end of the enclave’s memory layout.

## Explanation of `CMakeLists.txt`

The `CMakeLists.txt` is carefully structured to handle the complexity of building Keystone enclaves and host applications:

### Setting Keystone Paths and Libraries

- Defines paths to the Keystone SDK and associated libraries (eapp, edge, host).

```cmake
set(KEYSTONE_SDK_DIR "$ENV{HOME}/keystone/sdk")
set(KEYSTONE_LIB_EAPP ${KEYSTONE_SDK_DIR}/lib/libkeystone-eapp.a)
set(KEYSTONE_LIB_EDGE ${KEYSTONE_SDK_DIR}/lib/libkeystone-edge.a)
set(KEYSTONE_LIB_HOST ${KEYSTONE_SDK_DIR}/lib/libkeystone-host.a)
```

### Defining Executables

- Specifies enclave (`beta`) and host (`beta-runner`) binaries clearly.
- Includes `edge_wrapper` files in source definitions to ensure proper compilation.

```cmake
set(eapp_bin beta)
set(eapp_src
    eapp/beta.c
    eapp/edge_wrapper.c
)

set(host_bin beta-runner)
set(host_src
    host/main.cpp
    host/edge_wrapper.cpp
)
```

### Linker and Compiler Flags

- Ensures enclave binaries are built with the necessary flags (`-nostdlib -static`) for compatibility with Keystone enclaves.
- Applies a custom linker script (`app.lds`) specifying the entry point (`_start`).

```cmake
set_target_properties(${eapp_bin}
  PROPERTIES
    LINK_FLAGS "-nostdlib -static -T ${CMAKE_CURRENT_SOURCE_DIR}/app.lds"
)
```

### Include Directories

- Sets correct include paths to ensure access to Keystone SDK headers during compilation.

```cmake
target_include_directories(${eapp_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/app
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
)

target_include_directories(${host_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/host
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
)
```

### Eyrie Runtime and Packaging

- Integrates Eyrie runtime (`eyrie-rt`) necessary for the enclave environment.
- Automates packaging into `beta.ke`, bundling enclave and host binaries with runtime.

```cmake
add_eyrie_runtime(${eapp_bin}-eyrie
  ${eyrie_plugins}
  ${eyrie_files_to_copy}
)

add_keystone_package(${eapp_bin}-package
  ${package_name}
  ${package_script}
  ${eyrie_files_to_copy}
  ${eapp_bin}
  ${host_bin}
)

add_dependencies(${eapp_bin}-package ${eapp_bin}-eyrie)
```

## Building the Project

- We should clean first in every project we make to prevent making multiple `keystone-examples-*` subdirectories inside the the build-generic64/buildroot.build/build path 

```sh
make BUILDROOT_TARGET=keystone-examples-dirclean
```

- Then we run the following from your Keystone :

```sh
make -j$(nproc)
```

- We run the Buildroot image in QEmu:
```sh
make run
```

## Running the Enclave

Load Keystone driver and run your enclave package on your target Keystone system:

```sh
modprobe keystone-driver
./beta.ke
```



