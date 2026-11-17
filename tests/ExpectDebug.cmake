if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED PROGRAM OR NOT DEFINED PLAIN)
    message(FATAL_ERROR "debug check is missing a path")
endif()

execute_process(
    COMMAND "${COMPILER}" -g -o "${PROGRAM}" "${SOURCE}"
    RESULT_VARIABLE compile_status
    ERROR_VARIABLE compile_error)
if(NOT compile_status EQUAL 0)
    message(FATAL_ERROR "scpp -g did not write an executable\n${compile_error}")
endif()

execute_process(
    COMMAND "${COMPILER}" -o "${PLAIN}" "${SOURCE}"
    RESULT_VARIABLE plain_status
    ERROR_VARIABLE plain_error)
if(NOT plain_status EQUAL 0)
    message(FATAL_ERROR "scpp without -g did not write an executable\n${plain_error}")
endif()

find_program(DWARFDUMP NAMES llvm-dwarfdump)
if(NOT DWARFDUMP)
    message(FATAL_ERROR "llvm-dwarfdump is required")
endif()
get_filename_component(source_name "${SOURCE}" NAME)

execute_process(
    COMMAND "${DWARFDUMP}" --name main "${PROGRAM}"
    OUTPUT_VARIABLE debug_dump
    ERROR_VARIABLE debug_error)
if(NOT debug_dump MATCHES "DW_AT_name" OR NOT debug_dump MATCHES "main")
    message(FATAL_ERROR "debug info does not name main\n${debug_dump}${debug_error}")
endif()
if(NOT debug_dump MATCHES "${source_name}")
    message(FATAL_ERROR "debug info does not name ${source_name}\n${debug_dump}")
endif()
if(NOT debug_dump MATCHES "\"int\"")
    message(FATAL_ERROR "debug info does not name the return type\n${debug_dump}")
endif()

execute_process(
    COMMAND "${DWARFDUMP}" --name main "${PLAIN}"
    OUTPUT_VARIABLE plain_dump)
if(plain_dump MATCHES "${source_name}")
    message(FATAL_ERROR "an executable built without -g names ${source_name}\n${plain_dump}")
endif()

find_program(DEBUGGER gdb)
set(debugger_name gdb)
if(NOT DEBUGGER)
    find_program(DEBUGGER lldb)
    set(debugger_name lldb)
endif()
if(NOT DEBUGGER)
    message(FATAL_ERROR "gdb or lldb is required")
endif()

if(debugger_name STREQUAL "gdb")
    execute_process(
        COMMAND "${DEBUGGER}" -batch -nx -q
            -ex "set pagination off"
            -ex "set confirm off"
            -ex "break main"
            -ex run
            -ex frame
            -ex list
            --args "${PROGRAM}"
        OUTPUT_VARIABLE session
        ERROR_VARIABLE session_error
        RESULT_VARIABLE session_status)
else()
    execute_process(
        COMMAND "${DEBUGGER}" --batch
            -o "breakpoint set --name main"
            -o run
            -o "frame info"
            -o "source list"
            "${PROGRAM}"
        OUTPUT_VARIABLE session
        ERROR_VARIABLE session_error
        RESULT_VARIABLE session_status)
endif()
set(shown "${session}${session_error}")
if(NOT session_status EQUAL 0)
    message(FATAL_ERROR "${debugger_name} status ${session_status}\n${shown}")
endif()
if(NOT shown MATCHES "${source_name}" OR NOT shown MATCHES "main" OR NOT shown MATCHES "return 0")
    message(FATAL_ERROR "${debugger_name} did not name the source line\n${shown}")
endif()
message(STATUS "debugger ${debugger_name}")
