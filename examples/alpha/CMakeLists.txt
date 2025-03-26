cmake_minimum_required(VERSION 3.10)
project(alpha_example C CXX ASM)

# Keystone SDK Setup
if (NOT DEFINED KEYSTONE_SDK_DIR)
  set(KEYSTONE_SDK_DIR "$ENV{HOME}/keystone/sdk")
endif()

# Keystone libraries
set(KEYSTONE_LIB_EAPP ${KEYSTONE_SDK_DIR}/lib/libkeystone-eapp.a)
set(KEYSTONE_LIB_EDGE ${KEYSTONE_SDK_DIR}/lib/libkeystone-edge.a)
set(KEYSTONE_LIB_HOST ${KEYSTONE_SDK_DIR}/lib/libkeystone-host.a)

# Host App and Test Setup
set(host_bin alpha-runner)
set(host_src host/main.cpp host/edge_wrapper.cpp)
set(eyrie_plugins "io_syscall linux_syscall env_setup")
set(package_name "alpha.ke")
set(package_script "./alpha-runner alpha eyrie-rt loader.bin")

# EApp binary and sources
set(eapp_bin alpha)
set(eapp_src eapp/alpha.c eapp/edge_wrapper.c)

# Files to copy into packaging
set(eyrie_files_to_copy .options_log eyrie-rt loader.bin)

# Enclave Application
add_executable(${eapp_bin} ${eapp_src})
set_target_properties(${eapp_bin}
  PROPERTIES LINK_FLAGS "-nostdlib -static -T ${CMAKE_CURRENT_SOURCE_DIR}/app.lds")

# Include directories for EApp
target_include_directories(${eapp_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/app
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/eapp
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/host
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/eapp/malloc)

# Link libraries for EApp
target_link_libraries(${eapp_bin}
  -static
  ${KEYSTONE_LIB_EAPP}
  ${KEYSTONE_LIB_EDGE})

# Host Application
add_executable(${host_bin} ${host_src})
set_target_properties(${host_bin}
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

target_include_directories(${host_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/common
  PUBLIC ${KEYSTONE_SDK_DIR}/include/host
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/host)

target_link_libraries(${host_bin}
  ${KEYSTONE_LIB_HOST}
  ${KEYSTONE_LIB_EDGE})

# Add Eyrie runtime
add_eyrie_runtime(${eapp_bin}-eyrie
  ${eyrie_plugins}
  ${eyrie_files_to_copy})

# Package the enclave
add_keystone_package(${eapp_bin}-package
  ${package_name}
  ${package_script}
  ${eyrie_files_to_copy} ${eapp_bin} ${host_bin})

add_dependencies(${eapp_bin}-package ${eapp_bin}-eyrie)

# Add to main build target
add_dependencies(examples ${eapp_bin}-package)

