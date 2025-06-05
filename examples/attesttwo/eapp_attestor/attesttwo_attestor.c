// the idea behind this project is to make an example of two enclaves with no external libraries, only with Keystone headers for OCALLs, shared-buffer copy, and attest_enclave().
#include "app/eapp_utils.h"
#include "app/syscall.h"
#include "edge/edge_common.h"   // defines struct edge_data and copy_from_shared()



#define OCALL_PRINT_BUFFER  1
#define OCALL_PRINT_VALUE   2
#define OCALL_COPY_REPORT   3
#define OCALL_GET_NONCE     4
#define OCALL_GET_TARGET    5
//------------------------------------------------------------------------------------------------
// Minimal type defs (enclave cannot rely on standard‐library typedefs):
//   size_t    as unsigned long (64-bit on RISC‐V)
//   uint32_t  as unsigned int  (32-bit)
//   uint8_t   as unsigned char (8-bit)
typedef unsigned long  size_t;
typedef unsigned int   uint32_t;
typedef unsigned char  uint8_t;

//------------------------------------------------------------------------------------------------
// Helper: custom strlen (count bytes until first '\0')
static size_t my_strlen(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

// Helper: custom memset (fill `n` bytes at `dest` with byte `c`)
static void *my_memset(void *dest, int c, size_t n) {
    unsigned char *p = (unsigned char *)dest;
    size_t i;
    for (i = 0; i < n; i++) {
        p[i] = (unsigned char)c;
    }
    return dest;
}

// Helper: convert a single hex character to its 4-bit value (0..15). Returns 0 on invalid.
static uint8_t hex_digit_value(char c) {
    if (c >= '0' && c <= '9') {
        return (uint8_t)(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return (uint8_t)(c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F') {
        return (uint8_t)(c - 'A' + 10);
    }
    return 0;
}

// Helper: convert hexstr[i] and hexstr[i+1] (two hex chars) into a byte (from 0 to 255).
static uint8_t hex_to_byte(const char *hexstr, size_t i) {
    uint8_t hi = hex_digit_value(hexstr[i]);
    uint8_t lo = hex_digit_value(hexstr[i + 1]);
    return (uint8_t)((hi << 4) | lo);
}

//------------------------------------------------------------------------------------------------
int main() {
    // 1- OCALL to fetch the 16-hex-digit nonce (plus NUL).
    struct edge_data ret_nonce;
    ocall(OCALL_GET_NONCE, NULL, 0, &ret_nonce, sizeof(struct edge_data));

    // Copy into local buffer (up to 32 bytes).
    char nonce_buf[32];
    if (ret_nonce.size > sizeof(nonce_buf)) {
        ret_nonce.size = sizeof(nonce_buf);
    }
    copy_from_shared(nonce_buf, ret_nonce.offset, ret_nonce.size);
    // Ensure we have a trailing '\0':
    nonce_buf[ret_nonce.size - 1] = '\0';

    // 2- OCALL to fetch the target enclave path (NUL-terminated string).
    struct edge_data ret_target;
    ocall(OCALL_GET_TARGET, NULL, 0, &ret_target, sizeof(struct edge_data));

    // Copy into local buffer (up to 256 bytes).
    char target_path[256];
    if (ret_target.size > sizeof(target_path)) {
        ret_target.size = sizeof(target_path);
    }
    copy_from_shared(target_path, ret_target.offset, ret_target.size);
    // Ensure trailing '\0':
    target_path[ret_target.size - 1] = '\0';

    // 3- Derive two “checkpoints” (threshold1, threshold2) in [1..10000]
    //    from the 16-hex-digit nonce, without any standard‐library calls.
    //    Parse first 8 hex chars -> 32-bit integer first32:
    uint32_t first32 = 0;
    {
        size_t k;
        for (k = 0; k < 8; k += 2) {
            first32 = (first32 << 8) | hex_to_byte(nonce_buf, k);
        }
    }
    // Parse next 8 hex chars -> 32-bit integer second32:
    uint32_t second32 = 0;
    {
        size_t k;
        for (k = 8; k < 16; k += 2) {
            second32 = (second32 << 8) | hex_to_byte(nonce_buf, k);
        }
    }

    // Compute thresholds in 1..10000
    unsigned long threshold1 = (first32 % 10000) + 1;
    unsigned long threshold2 = (second32 % 10000) + 1;
    if (threshold2 == threshold1) {
        // ensure they differ; bump threshold2 by 1 (wrap to 1 if needed)
        threshold2 = (threshold1 % 10000) + 1;
    }

    // 4) Loop from 1 to 10000; OCALL_PRINT_VALUE when i == threshold1 or threshold2
    {
        unsigned long i;
        for (i = 1; i <= 10000; i++) {
            if (i == threshold1 || i == threshold2) {
                ocall(OCALL_PRINT_VALUE, &i, sizeof(unsigned long), 0, 0);
            }
        }
    }

    // 5) Produce a 4096-byte attestation report for “target_path”, embedding nonce.
    char report_buf[4096];
    my_memset(report_buf, 0, sizeof(report_buf));

    // The host must have exported KA_KE=<absolute_path_to_target_path>
    attest_enclave((void *)report_buf, nonce_buf, my_strlen(nonce_buf) + 1);

    // 6) OCALL to send back all 4096 bytes of the report to the host.
    ocall(OCALL_COPY_REPORT, report_buf, sizeof(report_buf), 0, 0);

    EAPP_RETURN(0);
}

