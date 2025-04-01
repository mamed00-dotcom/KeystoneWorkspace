cmake_minimum_required(VERSION 3.10)
project(beta_example C CXX ASM)

if(NOT DEFINED KEYSTONE_SDK_DIR)
  set(KEYSTONE_SDK_DIR "$ENV{HOME}/keystone/sdk")
endif()

set(KEYSTONE_LIB_EAPP ${KEYSTONE_SDK_DIR}/lib/libkeystone-eapp.a)
set(KEYSTONE_LIB_EDGE ${KEYSTONE_SDK_DIR}/lib/libkeystone-edge.a)
set(KEYSTONE_LIB_HOST ${KEYSTONE_SDK_DIR}/lib/libkeystone-host.a)

set(eapp_bin beta)
set(eapp_src
    eapp/beta.c
    eapp/edge_wrapper.c  # add your new file
)
set(host_bin beta-runner)
set(host_src
    host/main.cpp
    host/edge_wrapper.cpp # add your new file
)

set(package_name "beta.ke")
set(package_script "./beta-runner beta eyrie-rt loader.bin")
set(eyrie_plugins "io_syscall linux_syscall env_setup")

# ============= Enclave
add_executable(${eapp_bin} ${eapp_src})
target_link_libraries(${eapp_bin} "-nostdlib -static"
  ${KEYSTONE_LIB_EAPP}
  ${KEYSTONE_LIB_EDGE}
)
set_target_properties(${eapp_bin}
  PROPERTIES
    LINK_FLAGS "-nostdlib -static -T ${CMAKE_CURRENT_SOURCE_DIR}/app.lds"
)
target_include_directories(${eapp_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/app
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
)

# ============= Host
add_executable(${host_bin} ${host_src})
target_link_libraries(${host_bin}
  ${KEYSTONE_LIB_HOST}
  ${KEYSTONE_LIB_EDGE}
)
set_target_properties(${host_bin}
  PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
target_include_directories(${host_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/host
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
)

# ============= Eyrie
set(eyrie_files_to_copy .options_log eyrie-rt loader.bin)
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
add_dependencies(examples ${eapp_bin}-package)

