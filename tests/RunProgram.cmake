if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED PROGRAM OR NOT DEFINED CHECK)
    message(FATAL_ERROR "run check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" "${SOURCE}" "${PROGRAM}"
    RESULT_VARIABLE compile_status)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "scpp did not write an executable")
endif()

execute_process(
    COMMAND "${PROGRAM}"
    RESULT_VARIABLE run_status
    OUTPUT_VARIABLE program_output)
if(NOT run_status EQUAL 0)
    message(FATAL_ERROR "program exited ${run_status}")
endif()

if(CHECK STREQUAL "print")
    if(NOT program_output STREQUAL "count is 1\n")
        message(FATAL_ERROR "print wrote '${program_output}'")
    endif()
endif()
