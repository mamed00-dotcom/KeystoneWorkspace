# Nom des binaires
set(eapp1_bin sum)
set(eapp2_bin mult)
set(host_bin e_com-runner)
set(host_src host/host.cpp)

# Chemin du package et nom
set(package_name "e_com.ke")
set(package_script "./e_com-runner sum mult eyrie-rt loader.bin")
set(eyrie_plugins "io_syscall linux_syscall env_setup")

# Dépendances EAPP
set(eapp_common_includes
  PUBLIC ${KEYSTONE_SDK_DIR}/include/app
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
)

# Enclave 1 : somme
add_executable(${eapp1_bin} eapp_sum/sum.c)
target_link_libraries(${eapp1_bin} "-nostdlib -static" ${KEYSTONE_LIB_EAPP} ${KEYSTONE_LIB_EDGE})
target_include_directories(${eapp1_bin} ${eapp_common_includes})

# Enclave 2 : produit
add_executable(${eapp2_bin} eapp_mult/mult.c)
target_link_libraries(${eapp2_bin} "-nostdlib -static" ${KEYSTONE_LIB_EAPP} ${KEYSTONE_LIB_EDGE})
target_include_directories(${eapp2_bin} ${eapp_common_includes})

# Host commun
add_executable(${host_bin} ${host_src})
target_link_libraries(${host_bin} ${KEYSTONE_LIB_HOST} ${KEYSTONE_LIB_EDGE})
set_target_properties(${host_bin}
  PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO
)
target_include_directories(${host_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/host
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge)

# Eyrie runtime
set(eyrie_files_to_copy .options_log eyrie-rt loader.bin)
add_eyrie_runtime(e_com-eyrie
  ${eyrie_plugins}
  ${eyrie_files_to_copy})

# Packaging
add_keystone_package(e_com-package
  ${package_name}
  ${package_script}
  ${eyrie_files_to_copy} ${eapp1_bin} ${eapp2_bin} ${host_bin})

add_dependencies(e_com-package e_com-eyrie)

# Ajouter au build global
add_dependencies(examples e_com-package)
