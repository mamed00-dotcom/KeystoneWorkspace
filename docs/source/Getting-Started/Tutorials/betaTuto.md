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

### Keystone SDK Setup
- Specifies paths to Keystone SDK and libraries necessary to build enclave (`libkeystone-eapp.a`) and host (`libkeystone-host.a`).

### Enclave (EApp) Configuration
- Builds enclave executable `beta` from sources:
  - `beta.c`
  - `edge_wrapper.c`
- Links the enclave with Keystone libraries statically.
- Uses custom linker script (`app.lds`).

### Host Configuration
- Builds host executable `beta-runner` from sources:
  - `main.cpp`
  - `edge_wrapper.cpp`
- Configures host compilation to use C++11.

### Eyrie Runtime Setup
- Includes required runtime plugins (`io_syscall`, `linux_syscall`, `env_setup`).
- Packages enclave and host binaries along with the runtime into a Keystone enclave package (`beta.ke`).

## Building the Project

- We should clean first in every project we make to prevent making multiple keystone-examples-* subdirectories inside the the build-generic64/buildroot.build/build path 

```sh
make BUILDROOT_TARGET=keystone-examples-dirclean
```

- Then we run the following from your Keystone :

```sh
make -j$(nproc)
```

Check the built enclave package:

## Running the Enclave

Load Keystone driver and run your enclave package on your target Keystone system:

```sh
modprobe keystone-driver
./beta.ke
```



