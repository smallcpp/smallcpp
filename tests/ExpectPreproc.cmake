if(NOT DEFINED PREPROC OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "preproc check is missing a path")
endif()

set(args "")
if(DEFINED EXTRA)
    separate_arguments(args NATIVE_COMMAND "${EXTRA}")
endif()

if(DEFINED STDOUT)
    execute_process(
        COMMAND "${PREPROC}" ${args} "${SOURCE}"
        RESULT_VARIABLE status
        OUTPUT_VARIABLE actual
        ERROR_VARIABLE shown)
else()
    if(NOT DEFINED OUTPUT)
        message(FATAL_ERROR "preproc check is missing a path")
    endif()
    execute_process(
        COMMAND "${PREPROC}" ${args} -o "${OUTPUT}" "${SOURCE}"
        RESULT_VARIABLE status
        ERROR_VARIABLE shown)
endif()

if(NOT EXISTS "${SOURCE}")
    message(FATAL_ERROR "preproc removed its input")
endif()

if(DEFINED EXPECT_FOUND)
    if(status EQUAL 0)
        return()
    endif()
    string(FIND "${shown}" "cannot include" missing)
    if(missing EQUAL -1)
        return()
    endif()
    message(FATAL_ERROR "default include was not searched\n${shown}")
endif()

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
    return()
endif()

if(NOT status EQUAL 0)
    message(FATAL_ERROR "preproc failed\n${shown}")
endif()

if(NOT DEFINED STDOUT)
    file(READ "${OUTPUT}" actual)
endif()
file(READ "${EXPECT}" expected)
if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "output mismatch\n--- actual ---\n${actual}\n--- expected ---\n${expected}")
endif()
