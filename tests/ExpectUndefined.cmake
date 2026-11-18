if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED PROGRAM)
    message(FATAL_ERROR "link check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" "${SOURCE}" "${PROGRAM}"
    RESULT_VARIABLE link_status
    ERROR_VARIABLE link_error
    OUTPUT_VARIABLE link_output)
if(link_status EQUAL 0)
    message(FATAL_ERROR "scpp linked an undefined call")
endif()

set(shown "${link_error}${link_output}")
string(FIND "${shown}" "undefined" undefined_at)
string(FIND "${shown}" "missing" missing_at)
if(undefined_at EQUAL -1 OR missing_at EQUAL -1)
    message(FATAL_ERROR "link diagnostic does not name the symbol\n${shown}")
endif()
