cmake_minimum_required(VERSION 3.13)
project(attesttwo C CXX)

# -----------------------------------------------------------------------------
# 1) Locate Keystone SDK
# -----------------------------------------------------------------------------
if(NOT DEFINED ENV{KEYSTONE_SDK_DIR})
  message(FATAL_ERROR "Please set environment variable KEYSTONE_SDK_DIR to your Keystone SDK root")
endif()
set(KEYSTONE_SDK_DIR $ENV{KEYSTONE_SDK_DIR})

# -----------------------------------------------------------------------------
# 2) Nom des binaires (unique names to avoid conflicts)
# -----------------------------------------------------------------------------
set(eapp1_bin   attesttwo_target)
set(eapp2_bin   attesttwo_attestor)
set(host_bin    attesttwo_runner)
set(host_src    host/attesttwo-runner.cpp)

# -----------------------------------------------------------------------------
# 3) Chemin du package et nom
# -----------------------------------------------------------------------------
set(package_name       "attesttwo.ke")
set(package_script     "./${host_bin} ${eapp1_bin} ${eapp2_bin} eyrie-rt loader.bin --sm-bin fw_jump.bin")
set(eyrie_plugins      "none")

# -----------------------------------------------------------------------------
# 4) Dépendances EAPP (common include dirs for enclaves)
# -----------------------------------------------------------------------------
set(eapp_common_includes
  PUBLIC ${KEYSTONE_SDK_DIR}/include/app
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
)

# -----------------------------------------------------------------------------
# 5) Enclave 1 : attesttwo_target (dummy “target” enclave)
# -----------------------------------------------------------------------------
add_executable(${eapp1_bin} eapp_target/attesttwo_target.c)
target_link_libraries(${eapp1_bin}
  "-nostdlib -static"
  ${KEYSTONE_LIB_EAPP}
  ${KEYSTONE_LIB_EDGE}
)
target_include_directories(${eapp1_bin} ${eapp_common_includes})

# -----------------------------------------------------------------------------
# 6) Enclave 2 : attesttwo_attestor (attestation logic)
# -----------------------------------------------------------------------------
add_executable(${eapp2_bin} eapp_attestor/attesttwo_attestor.c)
target_link_libraries(${eapp2_bin}
  "-nostdlib -static"
  ${KEYSTONE_LIB_EAPP}
  ${KEYSTONE_LIB_EDGE}
)
target_include_directories(${eapp2_bin} ${eapp_common_includes})

# -----------------------------------------------------------------------------
# 7) Host : attesttwo_runner
# -----------------------------------------------------------------------------
add_executable(${host_bin} ${host_src})
target_link_libraries(${host_bin}
  ${KEYSTONE_LIB_HOST}
  ${KEYSTONE_LIB_EDGE}
  ${KEYSTONE_LIB_VERIFIER}
)
set_target_properties(${host_bin} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
target_include_directories(${host_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/common
  PUBLIC ${KEYSTONE_SDK_DIR}/include/host
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
  PUBLIC ${KEYSTONE_SDK_DIR}/include/verifier
)

# -----------------------------------------------------------------------------
# 8) Eyrie runtime (copy eyrie-rt, loader.bin, etc.)
# -----------------------------------------------------------------------------
set(eyrie_files_to_copy
  .options_log
  eyrie-rt
  loader.bin
)
add_eyrie_runtime(attesttwo-eyrie
  ${eyrie_plugins}
  ${eyrie_files_to_copy}
)

# -----------------------------------------------------------------------------
# 9) Packaging
# -----------------------------------------------------------------------------
add_keystone_package(attesttwo-package
  ${package_name}
  ${package_script}

  # Files to include in the .ke:
  ${eyrie_files_to_copy}      # eyrie-rt, loader.bin, etc.
  ${eapp1_bin}                # enclave A (“attesttwo_target”)
  ${eapp2_bin}                # enclave B (“attesttwo_attestor”)
  ${host_bin}                 # host executable (“attesttwo_runner”)
  ${fw_bin}                   # SM firmware binary (passed in via -Dfw_bin=…)
)

add_dependencies(attesttwo-package attesttwo-eyrie)
add_dependencies(examples         attesttwo-package)

