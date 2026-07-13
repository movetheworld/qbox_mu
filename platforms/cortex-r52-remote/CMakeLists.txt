set(LIBQEMU_TARGETS aarch64)

add_executable(cortex-r52-vp
    src/main.cc
)

add_executable(remote_cpu_r52
    src/remote_cpu.cc
)

target_link_libraries(cortex-r52-vp
    ${TARGET_LIBS}
)

target_link_libraries(remote_cpu_r52
    ${TARGET_LIBS}
)

target_include_directories(
    cortex-r52-vp PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/global_peripheral_initiator/include>
)

target_include_directories(
    remote_cpu_r52 PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/global_peripheral_initiator/include>
)

set_target_properties(cortex-r52-vp PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../lib/libqemu;$ORIGIN/../../../lib"
)

set_target_properties(remote_cpu_r52 PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../lib/libqemu;$ORIGIN/../../../lib"
)

gs_enable_testing()
