#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <optional>
#include <ctime>

#include "host/keystone.h"
#include "edge/edge_common.h"
#include "edge/edge_call.h"       // for edge_call_init_internals
#include "verifier/report.h"	  // for report implementation
#include "verifier/test_dev_key.h" // for _sanctum_dev_public_key

using namespace Keystone;

//─── OCALL IDs ───────────────────────────────────────────────────────────────
#define OCALL_PRINT_BUFFER 1
#define OCALL_PRINT_VALUE  2
#define OCALL_COPY_REPORT  3
#define OCALL_GET_NONCE    4
#define OCALL_GET_TARGET   5

//─── Global state ─────────────────────────────────────────────────────────────
// Buffer to store the raw report bytes coming from the enclave:
static std::vector<uint8_t> g_report_bytes;

// The target enclave’s path (passed into OCALL_GET_TARGET):
static std::string g_target_path;

// Shared‐buffer size (filled before enclave.run()):
static size_t g_shared_size = 0;

//───────────── SharedBuffer Helper ────────────────────────────────────────────
class SharedBuffer {
public:
    SharedBuffer(void* buf_addr, size_t buf_len)
        : edge_call_((struct edge_call*)buf_addr),
          buffer_((uintptr_t)buf_addr),
          buffer_len_(buf_len) {}

    // Return (ptr, size) for the enclave’s call arguments, or set BAD_OFFSET.
    std::optional<std::pair<uintptr_t, size_t>>
    get_call_args_ptr_or_set_bad_offset() {
        size_t arg_len = edge_call_->call_arg_size;
        if (edge_call_->call_arg_offset > buffer_len_) {
            edge_call_->return_data.call_status = CALL_STATUS_BAD_OFFSET;
            return std::nullopt;
        }
        uintptr_t call_args = buffer_ + edge_call_->call_arg_offset;
        return std::make_pair(call_args, arg_len);
    }

    // Fetching “nonce” string from enclave, or set BAD_OFFSET
    std::optional<std::string> get_nonce_or_bad_offset() {
        auto maybe = get_call_args_ptr_or_set_bad_offset();
        if (!maybe.has_value()) return std::nullopt;
        auto [ptr, len] = maybe.value();
        if (len == 0) {
            edge_call_->return_data.call_status = CALL_STATUS_BAD_OFFSET;
            return std::nullopt;
        }
        char* cstr = (char*)ptr;
        if (strnlen(cstr, len) == len) {  // no NUL within len
            edge_call_->return_data.call_status = CALL_STATUS_BAD_OFFSET;
            return std::nullopt;
        }
        return std::string(cstr);
    }

    // Fetching “target_path” string from enclave, or set BAD_OFFSET for the next step
    std::optional<std::string> get_target_or_bad_offset() {
        auto maybe = get_call_args_ptr_or_set_bad_offset();
        if (!maybe.has_value()) return std::nullopt;
        auto [ptr, len] = maybe.value();
        if (len == 0) {
            edge_call_->return_data.call_status = CALL_STATUS_BAD_OFFSET;
            return std::nullopt;
        }
        char* cstr = (char*)ptr;
        if (strnlen(cstr, len) == len) {  // no NUL within len
            edge_call_->return_data.call_status = CALL_STATUS_BAD_OFFSET;
            return std::nullopt;
        }
        return std::string(cstr);
    }

    // Writing back a wrapped string (for either nonce or target)
    void setup_wrapped_ret_or_bad_ptr(const std::string& s) {
        struct edge_data wrapper;
        wrapper.size = s.length() + 1;  // include trailing '\0'
        size_t offset_to_data = sizeof(struct edge_call) + sizeof(struct edge_data);
        wrapper.offset = offset_to_data;

        // Copy the string bytes (with trailing '\0'):
        uintptr_t data_dst = buffer_ + offset_to_data;
        memcpy((void*)data_dst, s.c_str(), s.length() + 1);

        // Copy edge_data struct immediately after the edge_call header
        uintptr_t wrap_dst = buffer_ + sizeof(struct edge_call);
        memcpy((void*)wrap_dst, &wrapper, sizeof(struct edge_data));

        // Tell enclave: ret_offset = sizeof(edge_call), ret_size = sizeof(edge_data)
        edge_call_->return_data.call_ret_offset = sizeof(struct edge_call);
        edge_call_->return_data.call_ret_size   = sizeof(struct edge_data);
        edge_call_->return_data.call_status     = CALL_STATUS_OK;
    }

    // Capturing raw report bytes from enclave, or set BAD_OFFSET
    std::optional<std::pair<uintptr_t, size_t>> get_report_bytes_or_bad_offset() {
        auto maybe = get_call_args_ptr_or_set_bad_offset();
        if (!maybe.has_value()) return std::nullopt;
        auto [ptr, len] = maybe.value();
        return std::make_pair(ptr, len);
    }

    // 5) Mark OK without returning any data
    void set_ok() {
        edge_call_->return_data.call_status = CALL_STATUS_OK;
    }

private:
    struct edge_call* const edge_call_;
    uintptr_t const       buffer_;
    size_t const          buffer_len_;
};

//───────────── OCALL Wrappers ──────────────────────────────────────────────────

// OCALL_PRINT_BUFFER: enclave passes a C-string to print
static void print_buffer_wrapper(SharedBuffer& sb) {
    auto maybe = sb.get_call_args_ptr_or_set_bad_offset();
    if (!maybe.has_value()) return;
    auto [ptr, len] = maybe.value();
    printf("[Attestor] says: %s\n", (char*)ptr);
    sb.set_ok();
}

// OCALL_PRINT_VALUE: enclave passes an unsigned long to print
static void print_value_wrapper(SharedBuffer& sb) {
    auto maybe = sb.get_call_args_ptr_or_set_bad_offset();
    if (!maybe.has_value()) return;
    auto [ptr, len] = maybe.value();
    unsigned long v = *(unsigned long*)ptr;
    printf("[Attestor] value: %lu\n", v);
    sb.set_ok();
}

// OCALL_COPY_REPORT: enclave passes raw report bytes; host must capture them
static void copy_report_wrapper(SharedBuffer& sb) {
    auto maybe = sb.get_report_bytes_or_bad_offset();
    if (!maybe.has_value()) return;
    auto [ptr, len] = maybe.value();
    g_report_bytes.resize(len);
    memcpy(g_report_bytes.data(), (uint8_t*)ptr, len);
    sb.set_ok();
}

// OCALL_GET_NONCE: enclave requests a nonce string; host generates & returns it
static void get_nonce_wrapper(SharedBuffer& sb) {
    // 1) Generate a 16-byte hex nonce (plus trailing '\0')
    unsigned long rnd = ((unsigned long)rand() << 32) ^ ((unsigned long)rand());
    char nonce_buf[32];
    snprintf(nonce_buf, sizeof(nonce_buf), "%016lx", rnd);

    std::string s = std::string(nonce_buf) + "\0";
    sb.setup_wrapped_ret_or_bad_ptr(s);
}

// OCALL_GET_TARGET: enclave requests the target path; host returns g_target_path
static void get_target_wrapper(SharedBuffer& sb) {
    std::string s = g_target_path + "\0";
    sb.setup_wrapped_ret_or_bad_ptr(s);
}

//───────────── OCALL Dispatcher ────────────────────────────────────────────────
static void ocall_dispatcher(void* buffer) {
    SharedBuffer sb(buffer, g_shared_size);
    struct edge_call* ec = (struct edge_call*)buffer;
    switch (ec->call_id) {
      case OCALL_PRINT_BUFFER:
        print_buffer_wrapper(sb);
        break;
      case OCALL_PRINT_VALUE:
        print_value_wrapper(sb);
        break;
      case OCALL_COPY_REPORT:
        copy_report_wrapper(sb);
        break;
      case OCALL_GET_NONCE:
        get_nonce_wrapper(sb);
        break;
      case OCALL_GET_TARGET:
        get_target_wrapper(sb);
        break;
      default:
        // Unknown OCALL ID -> ignore
        break;
    }
}

//───────────── Run a Single Enclave ─────────────────────────────────────────────
static void run_enclave(const std::string& eapp_path,
                        const std::string& runtime_path,
                        const std::string& loader_path) {
    Keystone::Enclave enclave;
    Keystone::Params params;
    params.setFreeMemSize(2 * 1024 * 1024);
    params.setUntrustedSize(2 * 1024 * 1024);

    enclave.init(eapp_path.c_str(),
                 runtime_path.c_str(),
                 loader_path.c_str(),
                 params);

    // Capture shared buffer size for dispatcher
    g_shared_size = enclave.getSharedBufferSize();

    // Tell edge_call where the shared buffer is
    edge_call_init_internals((uintptr_t)enclave.getSharedBuffer(),
                             enclave.getSharedBufferSize());

    // Register our OCALL dispatcher
    enclave.registerOcallDispatch(ocall_dispatcher);

    // Run until enclave calls EAPP_RETURN()
    uintptr_t retval = 0;
    enclave.run(&retval);
}

//───────────── main() ──────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    if (argc < 7) {
        fprintf(stderr,
                "Usage: %s <target-eapp> <attestor-eapp> <runtime> <loader> "
                "--sm-bin <fw-path>\n",
                argv[0]);
        return 1;
    }

    // argv breakdown:
    //   argv[1] = target enclave ELF
    //   argv[2] = attestor enclave ELF
    //   argv[3] = eyrie runtime (e.g. "eyrie-rt")
    //   argv[4] = loader (e.g. "loader.bin")
    //   argv[5] = "--sm-bin" (ignored here)
    //   argv[6] = <firmware_path>
    g_target_path  = argv[1];
    std::string attestor_path = argv[2];
    std::string runtime_path  = argv[3];
    std::string loader_path   = argv[4];
    (void)argv[5];  // ps : “--sm-bin” is not used by host code
    const char* fw_bin_path   = argv[6];

    srand((unsigned)time(NULL));

    // 1) Run the “target” enclave to confirm it loads
    printf("1) Launching Target Enclave (just to confirm it loads)...\n");
    run_enclave(g_target_path, runtime_path, loader_path);
    printf("   --> Target enclave returned immediately.\n\n");

    // 2) Run the “attestor” enclave to produce an attestation report
    printf("2) Launching Attestor Enclave (will produce report for Target)...\n");
    run_enclave(attestor_path, runtime_path, loader_path);
    printf("   --> Attestor enclave returned. Report length = %zu bytes.\n\n",
           g_report_bytes.size());

    // 3) Verify the report on the host (signature-only check here)
    if (g_report_bytes.empty()) {
        fprintf(stderr, "ERROR: No report received from Attestor enclave.\n");
        return 1;
    }

    // Deserialize the report
    Report final_report;
    final_report.fromBytes(g_report_bytes.data());

    // Check signature only (full hash+nonce check can be added later)
    if (final_report.checkSignaturesOnly(_sanctum_dev_public_key)) {
        printf("*** Attestation report signature is VALID. ***\n");
    } else {
        printf("*** Attestation report signature is INVALID. ***\n");
    }

    // Print the 16-byte nonce embedded in the report’s data section
    const void* data_section = final_report.getDataSection();
    size_t      data_size    = final_report.getDataSize();
    if (data_size == 17) {
        char returned_nonce[18];
        memcpy(returned_nonce, data_section, data_size);
        returned_nonce[data_size] = '\0';
        printf("Nonce in report: %s (length=%zu)\n", returned_nonce, data_size);
    } else {
        printf("Warning: unexpected data_size=%zu in report\n", data_size);
    }

    return 0;
}

