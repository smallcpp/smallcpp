if(NOT COMPILER OR NOT VERIFY OR NOT SOURCE OR NOT BITCODE OR NOT MODE)
    message(FATAL_ERROR "compile check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" "${SOURCE}" "${BITCODE}"
    RESULT_VARIABLE compile_status)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "shell did not write a verified module")
endif()

execute_process(
    COMMAND "${VERIFY}" "${MODE}" "${BITCODE}"
    RESULT_VARIABLE verify_status)
if(NOT verify_status EQUAL 0)
    message(FATAL_ERROR "module did not verify")
endif()
