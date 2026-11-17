if(NOT DEFINED COMPILER OR NOT DEFINED LLC OR NOT DEFINED CLANG OR NOT DEFINED SOURCE
        OR NOT DEFINED BITCODE OR NOT DEFINED OBJECT OR NOT DEFINED PROGRAM
        OR NOT DEFINED DRIVER OR NOT DEFINED RUNTIME OR NOT DEFINED INCLUDE)
    message(FATAL_ERROR "collector check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" "${SOURCE}" "${BITCODE}"
    RESULT_VARIABLE compile_status)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "scpp did not write a verified module")
endif()

execute_process(
    COMMAND "${LLC}" -filetype=obj -frame-pointer=all -o "${OBJECT}" "${BITCODE}"
    RESULT_VARIABLE llc_status)
if(NOT llc_status EQUAL 0)
    message(FATAL_ERROR "llc did not write an object")
endif()

execute_process(
    COMMAND "${CLANG}" -I "${INCLUDE}" -o "${PROGRAM}" "${OBJECT}" "${DRIVER}" "${RUNTIME}"
    RESULT_VARIABLE link_status)
if(NOT link_status EQUAL 0)
    message(FATAL_ERROR "clang did not link the collector")
endif()

execute_process(
    COMMAND "${PROGRAM}"
    RESULT_VARIABLE run_status)
if(NOT run_status EQUAL 0)
    message(FATAL_ERROR "collector did not see the live pointer (${run_status})")
endif()
