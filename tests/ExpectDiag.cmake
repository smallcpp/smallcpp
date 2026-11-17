if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED BITCODE OR NOT DEFINED NEEDLE)
    message(FATAL_ERROR "diagnostic check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" "${SOURCE}" "${BITCODE}"
    RESULT_VARIABLE compile_status
    ERROR_VARIABLE compile_error
    OUTPUT_VARIABLE compile_output)
if(compile_status EQUAL 0)
    message(FATAL_ERROR "compiler accepted a bad program")
endif()

set(shown "${compile_error}${compile_output}")
string(FIND "${shown}" "${NEEDLE}" found)
if(found EQUAL -1)
    message(FATAL_ERROR "diagnostic does not name the problem\n${shown}")
endif()

if(DEFINED COUNT)
    string(REPLACE "${NEEDLE}" "" stripped "${shown}")
    string(LENGTH "${shown}" full_length)
    string(LENGTH "${stripped}" stripped_length)
    string(LENGTH "${NEEDLE}" needle_length)
    math(EXPR times "(${full_length} - ${stripped_length}) / ${needle_length}")
    if(times LESS COUNT)
        message(FATAL_ERROR "saw '${NEEDLE}' ${times} times")
    endif()
endif()
