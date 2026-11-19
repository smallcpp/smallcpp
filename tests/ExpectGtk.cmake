if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED PROGRAM)
    message(FATAL_ERROR "gtk check is missing a path")
endif()
if(NOT DEFINED EXPECT_OUT)
    message(FATAL_ERROR "gtk check is missing its line")
endif()

set(compile_command "${COMPILER}")
if(DEFINED LIB_DIRS AND NOT "${LIB_DIRS}" STREQUAL "")
    foreach(dir IN LISTS LIB_DIRS)
        list(APPEND compile_command -L "${dir}")
    endforeach()
endif()
if(DEFINED LIBS AND NOT "${LIBS}" STREQUAL "")
    foreach(lib IN LISTS LIBS)
        list(APPEND compile_command -l "${lib}")
    endforeach()
endif()
list(APPEND compile_command "${SOURCE}" "${PROGRAM}")
execute_process(
    COMMAND ${compile_command}
    RESULT_VARIABLE compile_status
    ERROR_VARIABLE compile_error)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "scpp did not write an executable\n${compile_error}")
endif()

find_program(XVFB_RUN xvfb-run)
if(NOT XVFB_RUN)
    message(FATAL_ERROR
        "xvfb-run is required to run a gtk example")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        --unset=WAYLAND_DISPLAY
        --unset=DISPLAY
        GDK_BACKEND=x11
        "${XVFB_RUN}" -a
        -s "-screen 0 1280x1024x24"
        "${PROGRAM}"
    RESULT_VARIABLE run_status
    OUTPUT_VARIABLE program_output
    ERROR_VARIABLE program_error)
if(run_status EQUAL 77)
    message("${program_output}")
    return()
endif()
if(NOT run_status EQUAL 0)
    message(FATAL_ERROR
        "exit ${run_status}\n${program_output}\n${program_error}")
endif()

set(expected "${EXPECT_OUT}")
string(APPEND expected "\n")
if(NOT program_output STREQUAL "${expected}")
    message(FATAL_ERROR "wrote '${program_output}'")
endif()
