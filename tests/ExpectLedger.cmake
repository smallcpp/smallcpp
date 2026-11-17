if(NOT DEFINED COMPILER OR NOT DEFINED PROGRAM OR NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "ledger check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" -o "${PROGRAM}"
        "${SOURCE_DIR}/money.scpp"
        "${SOURCE_DIR}/account.scpp"
        "${SOURCE_DIR}/book.scpp"
        "${SOURCE_DIR}/report.scpp"
        "${SOURCE_DIR}/main.scpp"
    RESULT_VARIABLE compile_status
    ERROR_VARIABLE compile_error)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "scpp did not link the ledger\n${compile_error}")
endif()

execute_process(
    COMMAND "${PROGRAM}"
    RESULT_VARIABLE run_status
    OUTPUT_VARIABLE program_output)
if(NOT run_status EQUAL 0)
    message(FATAL_ERROR "exit ${run_status}, expected 0")
endif()
if(NOT program_output STREQUAL "cash 40\n")
    message(FATAL_ERROR "wrote '${program_output}'")
endif()
