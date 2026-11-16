if(NOT COMPILER OR NOT VERIFY OR NOT SOURCE OR NOT BITCODE)
    message(FATAL_ERROR "compile_return is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" "${SOURCE}" "${BITCODE}"
    RESULT_VARIABLE compile_status)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "shell did not write a verified module")
endif()

execute_process(
    COMMAND "${VERIFY}" "${BITCODE}"
    RESULT_VARIABLE verify_status)
if(NOT verify_status EQUAL 0)
    message(FATAL_ERROR "module did not verify")
endif()
