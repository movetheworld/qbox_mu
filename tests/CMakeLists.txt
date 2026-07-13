# Place test executables under <build>/tests/... mirroring the source tree,
# rather than in the build root.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "")

gs_configure_linux_test_library_path()

macro(add_test)
    _add_test(${ARGN})
    cmake_parse_arguments(_gs_at "" "NAME" "" ${ARGN})
    if(_gs_at_NAME)
        gs_set_test_runtime_env(${_gs_at_NAME})
    endif()
endmacro()

add_subdirectory(components)
add_subdirectory(sync)
add_subdirectory(utils)
if(NOT WITHOUT_QEMU)
    add_subdirectory(qbox)
endif()
