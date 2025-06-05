# AttestTwo Example

This example demonstrates a two-enclave attestation system using Keystone, where one enclave (the attestor) verifies the integrity of another enclave (the target).

## Overview

The system consists of three main components:

1. **Target Enclave** (`attesttwo_target`): A simple enclave that serves as the target for attestation. It has no runtime logic and just returns immediately.

2. **Attestor Enclave** (`attesttwo_attestor`): The enclave responsible for performing the attestation. It:
   - Receives a nonce from the host
   - Gets the target enclave's path
   - Generates deterministic checkpoints based on the nonce
   - Performs attestation on the target enclave
   - Returns an attestation report

3. **Host Application** (`attesttwo_runner`): Manages both enclaves and handles communication between them. It:
   - Initializes both enclaves
   - Generates a random nonce
   - Provides the target enclave path
   - Receives and processes the attestation report

## Communication Flow

1. The host initializes both enclaves
2. The attestor enclave requests a nonce from the host
3. The host generates a random 16-byte hex nonce
4. The attestor enclave requests the target enclave path
5. The attestor generates two deterministic checkpoints based on the nonce
6. The attestor performs attestation on the target enclave
7. The attestor sends the attestation report back to the host

## Building and Running

### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Run

```bash
./attesttwo-runner attesttwo_target attesttwo_attestor eyrie-rt loader.bin --sm-bin fw_jump.bin
```

## Security Features

- Uses a random nonce to prevent replay attacks
- Performs attestation on the target enclave's binary
- Generates deterministic checkpoints based on the nonce
- Uses secure communication channels between enclaves and host

## Implementation Details

### Target Enclave
- Located in `eapp_target/attesttwo_target.c`
- Minimal implementation that just returns
- Serves as the target for attestation

### Attestor Enclave
- Located in `eapp_attestor/attesttwo_attestor.c`
- Implements the attestation logic
- Uses OCALLs to communicate with the host
- Generates deterministic checkpoints based on the nonce

### Host Application
- Located in `host/attesttwo-runner.cpp`
- Manages both enclaves
- Handles OCALLs from the attestor enclave
- Provides necessary data to the attestor
- Processes the attestation report

## OCALL Interface

The system uses the following OCALLs:

1. `OCALL_PRINT_BUFFER`: Print a buffer from the enclave
2. `OCALL_PRINT_VALUE`: Print a value from the enclave
3. `OCALL_COPY_REPORT`: Copy the attestation report
4. `OCALL_GET_NONCE`: Get a random nonce
5. `OCALL_GET_TARGET`: Get the target enclave path 
