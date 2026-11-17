if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED PROGRAM)
    message(FATAL_ERROR "uncaught check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" -g -o "${PROGRAM}" "${SOURCE}"
    RESULT_VARIABLE compile_status
    ERROR_VARIABLE compile_error)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "scpp -g did not write an executable\n${compile_error}")
endif()

execute_process(
    COMMAND "${PROGRAM}"
    RESULT_VARIABLE run_status
    OUTPUT_VARIABLE program_output
    ERROR_VARIABLE program_error)
if(run_status EQUAL 0)
    message(FATAL_ERROR "uncaught exception returned\n${program_output}${program_error}")
endif()

set(shown "${program_output}${program_error}")
if(NOT shown MATCHES "uncaught exception: missing")
    message(FATAL_ERROR "diagnostic is missing the message\n${shown}")
endif()
get_filename_component(source_name "${SOURCE}" NAME)
if(NOT shown MATCHES "leaf at ${source_name}:[0-9]+")
    message(FATAL_ERROR "stack is missing leaf\n${shown}")
endif()
if(NOT shown MATCHES "main at ${source_name}:[0-9]+")
    message(FATAL_ERROR "stack is missing main\n${shown}")
endif()
