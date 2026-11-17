if(NOT DEFINED COMPILER OR NOT DEFINED VERIFY OR NOT DEFINED SOURCE OR NOT DEFINED BITCODE)
    message(FATAL_ERROR "output option check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" -o "${BITCODE}" "${SOURCE}"
    RESULT_VARIABLE compile_status
    ERROR_VARIABLE compile_error)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "shell -o did not write a verified module\n${compile_error}")
endif()

execute_process(
    COMMAND "${VERIFY}" return "${BITCODE}"
    RESULT_VARIABLE verify_status)
if(NOT verify_status EQUAL 0)
    message(FATAL_ERROR "module check failed")
endif()
