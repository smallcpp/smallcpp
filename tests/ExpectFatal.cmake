if(NOT DEFINED COMPILER OR NOT DEFINED RUNTIME OR NOT DEFINED SOURCE
        OR NOT DEFINED PROGRAM)
    message(FATAL_ERROR "fatal signal check is missing a path")
endif()

set(ENV{SMALLCPP_RUNTIME} "${RUNTIME}")
execute_process(
    COMMAND "${COMPILER}" -g -o "${PROGRAM}" "${SOURCE}"
    RESULT_VARIABLE compile_status
    ERROR_VARIABLE compile_error)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR
        "scpp -g did not write an executable\n${compile_error}")
endif()

execute_process(
    COMMAND "${PROGRAM}"
    RESULT_VARIABLE run_status
    OUTPUT_VARIABLE program_output
    ERROR_VARIABLE program_error)

set(shown "${program_output}${program_error}")
if(NOT run_status MATCHES "Segmentation fault")
    message(FATAL_ERROR
        "expected SIGSEGV, status ${run_status}\n${shown}")
endif()
if(NOT shown MATCHES "fatal signal SIGSEGV")
    message(FATAL_ERROR "stack is missing the signal\n${shown}")
endif()
get_filename_component(source_name "${SOURCE}" NAME)
if(NOT shown MATCHES "leaf at ${source_name}:[0-9]+")
    message(FATAL_ERROR "stack is missing leaf\n${shown}")
endif()
if(NOT shown MATCHES "main at ${source_name}:[0-9]+")
    message(FATAL_ERROR "stack is missing main\n${shown}")
endif()
