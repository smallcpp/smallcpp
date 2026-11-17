if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED OBJECT OR NOT DEFINED PROGRAM)
    message(FATAL_ERROR "missing-main check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" -c -o "${OBJECT}" "${SOURCE}"
    RESULT_VARIABLE object_status
    ERROR_VARIABLE object_error)
if(NOT object_status EQUAL 0)
    message(FATAL_ERROR "scpp -c rejected a translation unit that has no main\n${object_error}")
endif()

execute_process(
    COMMAND "${COMPILER}" -o "${PROGRAM}" "${SOURCE}"
    RESULT_VARIABLE link_status
    ERROR_VARIABLE link_error
    OUTPUT_VARIABLE link_output)
if(link_status EQUAL 0)
    message(FATAL_ERROR "scpp linked a program that has no main")
endif()
set(shown "${link_error}${link_output}")
string(FIND "${shown}" "program has no main" found)
if(found EQUAL -1)
    message(FATAL_ERROR "diagnostic does not name main\n${shown}")
endif()
