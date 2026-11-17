if(NOT DEFINED PREPROC OR NOT DEFINED SOURCE OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "preproc check is missing a path")
endif()

set(args "")
if(DEFINED EXTRA)
    separate_arguments(args NATIVE_COMMAND "${EXTRA}")
endif()

execute_process(
    COMMAND "${PREPROC}" ${args} "${SOURCE}" "${OUTPUT}"
    RESULT_VARIABLE status
    ERROR_VARIABLE shown)

if(DEFINED EXPECT_FAIL)
    if(status EQUAL 0)
        message(FATAL_ERROR "preproc succeeded\n${shown}")
    endif()
    if(DEFINED NEEDLE)
        string(FIND "${shown}" "${NEEDLE}" found)
        if(found EQUAL -1)
            message(FATAL_ERROR "diagnostic is missing '${NEEDLE}'\n${shown}")
        endif()
    endif()
    if(NOT EXISTS "${SOURCE}")
        message(FATAL_ERROR "preproc removed its input")
    endif()
    return()
endif()

if(NOT status EQUAL 0)
    message(FATAL_ERROR "preproc failed\n${shown}")
endif()
if(NOT EXISTS "${SOURCE}")
    message(FATAL_ERROR "preproc removed its input")
endif()

file(READ "${OUTPUT}" actual)
file(READ "${EXPECT}" expected)
if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "output mismatch\n--- actual ---\n${actual}\n--- expected ---\n${expected}")
endif()
