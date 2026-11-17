if(NOT DEFINED COMPILER OR NOT DEFINED NEEDLE)
    message(FATAL_ERROR "command-line check is missing a path")
endif()

if(NOT DEFINED EXPECT_STATUS)
    set(EXPECT_STATUS 0)
endif()

set(args "")
if(DEFINED CLI_ARGS)
    separate_arguments(args NATIVE_COMMAND "${CLI_ARGS}")
endif()

execute_process(
    COMMAND "${COMPILER}" ${args}
    RESULT_VARIABLE status
    OUTPUT_VARIABLE shown_out
    ERROR_VARIABLE shown_err)
if(NOT status EQUAL EXPECT_STATUS)
    message(FATAL_ERROR "scpp status ${status}, expected ${EXPECT_STATUS}\n${shown_out}${shown_err}")
endif()

string(FIND "${shown_out}${shown_err}" "${NEEDLE}" found)
if(found EQUAL -1)
    message(FATAL_ERROR "command-line output is missing '${NEEDLE}'\n${shown_out}${shown_err}")
endif()
