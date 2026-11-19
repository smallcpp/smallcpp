if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED OBJECT)
    message(FATAL_ERROR "compile check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" -c -o "${OBJECT}" "${SOURCE}"
    RESULT_VARIABLE compile_status
    ERROR_VARIABLE compile_error)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "scpp -c failed\n${compile_error}")
endif()
