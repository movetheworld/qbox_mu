find_package(Python3 COMPONENTS Interpreter REQUIRED)

gs_configure_linux_test_library_path()

macro(gs_add_test test)
    add_test(NAME ${test} COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${test}.py -e ${CMAKE_BINARY_DIR}/cortex-r52-vp -l ${CMAKE_SOURCE_DIR}/platforms/cortex-r52-remote/conf.lua)
    gs_set_test_runtime_env(${test})
    set_tests_properties(${test} PROPERTIES TIMEOUT 30)
endmacro()

gs_add_test(cortex_r52)
