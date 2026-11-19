foreach(required IN ITEMS CMAKE_COMMAND BUILD_DIR WORK SOURCE)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

file(REMOVE_RECURSE "${WORK}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${WORK}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE shown_out
    ERROR_VARIABLE shown_err)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "install failed (${result})\n${shown_out}${shown_err}")
endif()

foreach(relative IN ITEMS
        bin/scpp bin/preproc bin/scpp_cc
        include/exceptions
        lib/libsmallcpp_runtime.a
        lib/libsmallcpp_sqlite.a
        lib/libsmallcpp_gtk.a)
    if(NOT EXISTS "${WORK}/${relative}")
        message(FATAL_ERROR "install is missing ${relative}")
    endif()
endforeach()

set(program "${WORK}/return-zero")
execute_process(
    COMMAND "${WORK}/bin/scpp" -o "${program}" "${SOURCE}"
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE shown_out
    ERROR_VARIABLE shown_err)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "installed scpp failed (${result})\n${shown_out}${shown_err}")
endif()
execute_process(COMMAND "${program}" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "installed program exited ${result}")
endif()

file(WRITE "${WORK}/lib/libsmallcpp_runtime.a" "not an archive\n")
execute_process(
    COMMAND "${WORK}/bin/scpp" -o "${WORK}/return-zero-bad" "${SOURCE}"
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE shown_out
    ERROR_VARIABLE shown_err)
if(result EQUAL 0)
    message(FATAL_ERROR "scpp linked the source-tree runtime\n${shown_out}${shown_err}")
endif()

file(WRITE "${WORK}/use-header.scpp"
    "#include <exceptions>\nint main(void) { return 0; }\n")
file(WRITE "${WORK}/include/exceptions" "#error installed-prefix-header\n")
execute_process(
    COMMAND "${WORK}/bin/scpp" -o "${WORK}/use-header" "${WORK}/use-header.scpp"
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE shown_out
    ERROR_VARIABLE shown_err)
if(result EQUAL 0)
    message(FATAL_ERROR "scpp used a header outside the install prefix")
endif()
string(FIND "${shown_out}${shown_err}" "installed-prefix-header" needle_at)
if(needle_at EQUAL -1)
    message(FATAL_ERROR
        "diagnostic did not name the installed header\n${shown_out}${shown_err}")
endif()
