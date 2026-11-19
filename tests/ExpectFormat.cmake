cmake_policy(SET CMP0054 NEW)

if(NOT DEFINED FORMAT OR NOT DEFINED MODE)
    message(FATAL_ERROR "format check is missing FORMAT or MODE")
endif()

if(NOT DEFINED WORK)
    set(WORK "${CMAKE_CURRENT_BINARY_DIR}/format-work")
endif()
file(MAKE_DIRECTORY "${WORK}")

function(require_status result expected)
    if(NOT result EQUAL expected)
        message(FATAL_ERROR
            "scpp-format status ${result}, expected ${expected}\n${out}${err}")
    endif()
endfunction()

if(MODE STREQUAL "help" OR MODE STREQUAL "version"
        OR MODE STREQUAL "unknown" OR MODE STREQUAL "missing"
        OR MODE STREQUAL "stdin-inplace")
    set(args "")
    if(MODE STREQUAL "help")
        set(args --help)
        set(expect_status 0)
        set(needle "Usage: scpp-format")
        set(stream out)
    elseif(MODE STREQUAL "version")
        set(args --version)
        set(expect_status 0)
        set(needle "scpp-format: v0.1")
        set(stream out)
    elseif(MODE STREQUAL "unknown")
        set(args --not-an-option)
        set(expect_status 2)
        set(needle "Usage:")
        set(stream err)
    elseif(MODE STREQUAL "missing")
        set(args "${WORK}/missing.scpp")
        set(expect_status 1)
        set(needle "cannot read file")
        set(stream err)
    else()
        set(args -i -)
        set(expect_status 1)
        set(needle "cannot rewrite standard input in place")
        set(stream err)
    endif()
    execute_process(
        COMMAND "${FORMAT}" ${args}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err)
    require_status("${result}" "${expect_status}")
    if(stream STREQUAL "out")
        set(shown "${out}")
    else()
        set(shown "${err}")
    endif()
    string(FIND "${shown}" "${needle}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR
            "format output is missing '${needle}'\n${out}${err}")
    endif()
    return()
endif()

if(NOT DEFINED BEFORE OR NOT DEFINED AFTER)
    message(FATAL_ERROR "format check is missing a snippet")
endif()
file(READ "${BEFORE}" before_text)
file(READ "${AFTER}" after_text)

if(MODE STREQUAL "stdin")
    execute_process(
        COMMAND "${FORMAT}" -
        INPUT_FILE "${BEFORE}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err)
    require_status("${result}" 0)
    if(NOT out STREQUAL after_text)
        message(FATAL_ERROR
            "stdin format differs\n--- got ---\n${out}--- want ---\n${after_text}")
    endif()
    return()
endif()

if(MODE STREQUAL "inplace")
    set(dest "${WORK}/sample.scpp")
    file(REMOVE "${dest}")
    file(WRITE "${dest}" "${before_text}")
    execute_process(
        COMMAND "${FORMAT}" -i -v "${dest}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err)
    require_status("${result}" 0)
    if(NOT out STREQUAL "")
        message(FATAL_ERROR "in-place wrote to stdout:\n${out}")
    endif()
    string(FIND "${err}" "${dest}" named)
    if(named EQUAL -1)
        message(FATAL_ERROR "verbose in-place did not name the file:\n${err}")
    endif()
    file(READ "${dest}" got)
    if(NOT got STREQUAL after_text)
        message(FATAL_ERROR
            "in-place result differs\n--- got ---\n${got}--- want ---\n${after_text}")
    endif()
    return()
endif()

execute_process(
    COMMAND "${FORMAT}" "${BEFORE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err)
require_status("${result}" 0)
if(NOT out STREQUAL after_text)
    message(FATAL_ERROR
        "format differs for ${BEFORE}\n--- got ---\n${out}--- want ---\n${after_text}")
endif()

set(again "${WORK}/again.scpp")
file(WRITE "${again}" "${out}")
execute_process(
    COMMAND "${FORMAT}" "${again}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE twice
    ERROR_VARIABLE err)
require_status("${result}" 0)
if(NOT twice STREQUAL out)
    message(FATAL_ERROR "format is not stable for ${BEFORE}")
endif()
