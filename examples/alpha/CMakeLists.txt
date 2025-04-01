cmake_minimum_required(VERSION 3.10)
project(alpha_example C CXX ASM)

# Keystone SDK Setup
if(NOT DEFINED KEYSTONE_SDK_DIR)
  set(KEYSTONE_SDK_DIR "$ENV{HOME}/keystone/sdk")
endif()

# Keystone libraries
set(KEYSTONE_LIB_EAPP ${KEYSTONE_SDK_DIR}/lib/libkeystone-eapp.a)
set(KEYSTONE_LIB_EDGE ${KEYSTONE_SDK_DIR}/lib/libkeystone-edge.a)
set(KEYSTONE_LIB_HOST ${KEYSTONE_SDK_DIR}/lib/libkeystone-host.a)

# Our host and enclave config
set(eapp_bin alpha)               # enclave binary name
set(eapp_src eapp/alpha.c)        # enclave source
set(host_bin alpha-runner)        # host binary name
set(host_src host/main.cpp)       # host source
set(package_name "alpha.ke")      
set(package_script "./alpha-runner alpha eyrie-rt loader.bin") 
set(eyrie_plugins "io_syscall linux_syscall env_setup")

# Enclave (EApp) executable
add_executable(${eapp_bin} ${eapp_src})

# Link exactly like addition: -nostdlib -static
target_link_libraries(${eapp_bin} "-nostdlib -static"
  ${KEYSTONE_LIB_EAPP}
  ${KEYSTONE_LIB_EDGE}
)

# Include dirs for the enclave
target_include_directories(${eapp_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/app
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
)

# Host app
add_executable(${host_bin} ${host_src})
target_link_libraries(${host_bin}
  ${KEYSTONE_LIB_HOST}
  ${KEYSTONE_LIB_EDGE}
)

# set host to C++11 just like addition
set_target_properties(${host_bin}
  PROPERTIES CXX_STANDARD 11
             CXX_STANDARD_REQUIRED YES
             CXX_EXTENSIONS NO
)

target_include_directories(${host_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/host
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
)

# Eyrie runtime (lightweight OS code)
set(eyrie_files_to_copy .options_log eyrie-rt loader.bin)
add_eyrie_runtime(${eapp_bin}-eyrie
  ${eyrie_plugins}
  ${eyrie_files_to_copy}
)

# Package up alpha.ke with host + enclave + runtime
add_keystone_package(${eapp_bin}-package
  ${package_name}
  ${package_script}
  ${eyrie_files_to_copy}  # eyrie-rt + loader.bin
  ${eapp_bin}             # alpha (enclave)
  ${host_bin}             # alpha-runner (host)
)

# ensure eapp_bin is built before packaging
add_dependencies(${eapp_bin}-package ${eapp_bin}-eyrie)

# add to the main build target
add_dependencies(examples ${eapp_bin}-package)

