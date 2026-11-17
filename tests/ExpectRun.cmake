if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED PROGRAM OR NOT DEFINED EXPECT)
    message(FATAL_ERROR "run check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" "${SOURCE}" "${PROGRAM}"
    RESULT_VARIABLE compile_status
    ERROR_VARIABLE compile_error)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "scpp did not write an executable\n${compile_error}")
endif()

execute_process(
    COMMAND "${PROGRAM}"
    RESULT_VARIABLE run_status
    OUTPUT_VARIABLE program_output)
if(NOT run_status EQUAL EXPECT)
    message(FATAL_ERROR "exit ${run_status}, expected ${EXPECT}")
endif()

if(DEFINED EXPECT_OUT)
    set(expected "${EXPECT_OUT}")
    if(DEFINED EXPECT_NL)
        string(APPEND expected "\n")
    endif()
    if(NOT program_output STREQUAL "${expected}")
        message(FATAL_ERROR "wrote '${program_output}'")
    endif()
endif()
