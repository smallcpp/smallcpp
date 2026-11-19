foreach(required IN ITEMS CMAKE_COMMAND BUILD_DIR WORK SOURCE CLANG)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

find_program(NM nm)
if(NOT NM)
    message(FATAL_ERROR "nm is required")
endif()

function(ini_value lines key out)
    set(found "")
    set(seen 0)
    string(LENGTH "${key}" key_len)
    foreach(line IN LISTS lines)
        string(LENGTH "${line}" line_len)
        if(line_len LESS_EQUAL key_len)
            continue()
        endif()
        string(SUBSTRING "${line}" 0 "${key_len}" head)
        if(NOT head STREQUAL "${key}")
            continue()
        endif()
        string(SUBSTRING "${line}" "${key_len}" 1 sep)
        if(NOT sep STREQUAL "=")
            continue()
        endif()
        math(EXPR rest_at "${key_len} + 1")
        string(SUBSTRING "${line}" "${rest_at}" -1 found)
        math(EXPR seen "${seen} + 1")
    endforeach()
    if(NOT seen EQUAL 1)
        message(FATAL_ERROR "key ${key} appears ${seen} times")
    endif()
    set("${out}" "${found}" PARENT_SCOPE)
endfunction()

function(expect_install prefix)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}"
        WORKING_DIRECTORY "${WORK}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE shown_out
        ERROR_VARIABLE shown_err)
    set(install_result "${result}" PARENT_SCOPE)
    set(install_output "${shown_out}${shown_err}" PARENT_SCOPE)
endfunction()

function(run_installed)
    execute_process(
        COMMAND "${prefix}/bin/scpp" ${ARGN}
        WORKING_DIRECTORY "${WORK}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE shown_out
        ERROR_VARIABLE shown_err)
    set(run_result "${result}" PARENT_SCOPE)
    set(run_output "${shown_out}${shown_err}" PARENT_SCOPE)
endfunction()

file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")

set(bad_prefix "${WORK}/bad missing")
unset(ENV{HOME})
expect_install("${bad_prefix}")
if(install_result EQUAL 0)
    message(FATAL_ERROR "install succeeded without HOME\n${install_output}")
endif()
if(NOT install_output MATCHES "HOME")
    message(FATAL_ERROR
        "install failed without a HOME diagnostic:\n${install_output}")
endif()

set(ENV{HOME} "relative/home")
expect_install("${WORK}/bad relative")
if(install_result EQUAL 0)
    message(FATAL_ERROR "install succeeded with a relative HOME\n${install_output}")
endif()
if(NOT install_output MATCHES "HOME")
    message(FATAL_ERROR
        "relative HOME failed without a HOME diagnostic:\n${install_output}")
endif()
if(EXISTS "${WORK}/relative/home/.config/smallcpp/config.ini")
    message(FATAL_ERROR "relative HOME wrote config.ini")
endif()

set(logical "${WORK}/logical prefix")
set(stage "${WORK}/stage")
set(dest_home "${WORK}/dest home")
file(MAKE_DIRECTORY "${dest_home}")
set(ENV{HOME} "${dest_home}")
set(ENV{DESTDIR} "${stage}")
expect_install("${logical}")
unset(ENV{DESTDIR})
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "DESTDIR install failed (${install_result})\n${install_output}")
endif()
if(NOT EXISTS "${stage}${logical}/bin/scpp")
    message(FATAL_ERROR "DESTDIR install did not stage bin/scpp")
endif()
set(dest_ini "${dest_home}/.config/smallcpp/config.ini")
file(READ "${dest_ini}" dest_text)
string(FIND "${dest_text}" "${stage}" stage_at)
if(NOT stage_at EQUAL -1)
    message(FATAL_ERROR "config.ini records the DESTDIR staging path:\n${dest_text}")
endif()
file(STRINGS "${dest_ini}" dest_lines)
ini_value("${dest_lines}" "include" dest_include)
if(NOT dest_include STREQUAL "${logical}/include")
    message(FATAL_ERROR
        "DESTDIR include is '${dest_include}', expected '${logical}/include'")
endif()

set(home "${WORK}/home dir")
set(prefix "${WORK}/prefix dir")
file(MAKE_DIRECTORY "${home}")
set(ENV{HOME} "${home}")
expect_install("${prefix}")
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "install failed (${install_result})\n${install_output}")
endif()

foreach(relative IN ITEMS
        bin/scpp bin/preproc bin/scpp_cc
        include/exceptions
        lib/libsmallcpp_runtime.a
        lib/libsmallcpp_sqlite.a
        lib/libsmallcpp_gtk.a)
    if(NOT EXISTS "${prefix}/${relative}")
        message(FATAL_ERROR "install is missing ${relative}")
    endif()
endforeach()

set(ini "${home}/.config/smallcpp/config.ini")
if(NOT EXISTS "${ini}")
    message(FATAL_ERROR "install did not write ${ini}")
endif()
file(STRINGS "${ini}" ini_lines)
set(want include library runtime libdl libstdc++ libm libgcc_s libgcc libc)
foreach(line IN LISTS ini_lines)
    string(FIND "${line}" "=" eq)
    if(eq LESS 0)
        message(FATAL_ERROR "ini line has no '=': ${line}")
    endif()
    string(SUBSTRING "${line}" 0 "${eq}" key)
    list(FIND want "${key}" at)
    if(at LESS 0)
        message(FATAL_ERROR "unexpected ini key ${key}")
    endif()
endforeach()

ini_value("${ini_lines}" "include" cfg_include)
ini_value("${ini_lines}" "library" cfg_library)
ini_value("${ini_lines}" "runtime" cfg_runtime)
if(NOT cfg_include STREQUAL "${prefix}/include")
    message(FATAL_ERROR "include is '${cfg_include}'")
endif()
if(NOT cfg_library STREQUAL "${prefix}/lib")
    message(FATAL_ERROR "library is '${cfg_library}'")
endif()
if(NOT cfg_runtime STREQUAL "${prefix}/lib/libsmallcpp_runtime.a")
    message(FATAL_ERROR "runtime is '${cfg_runtime}'")
endif()

execute_process(
    COMMAND "${CLANG}" -print-file-name=libm.a
    RESULT_VARIABLE result
    OUTPUT_VARIABLE libm_file
    ERROR_VARIABLE libm_err
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "clang could not name libm.a\n${libm_err}")
endif()
ini_value("${ini_lines}" "libm" cfg_libm)
if(NOT cfg_libm STREQUAL "${libm_file}")
    message(FATAL_ERROR "libm is '${cfg_libm}', clang names '${libm_file}'")
endif()
if(NOT cfg_libm MATCHES "libm\\.a$")
    message(FATAL_ERROR "libm is not libm.a: ${cfg_libm}")
endif()
foreach(key IN ITEMS libdl libstdc++ libm libgcc_s libgcc libc)
    ini_value("${ini_lines}" "${key}" cfg_path)
    if(NOT IS_ABSOLUTE "${cfg_path}" OR NOT EXISTS "${cfg_path}")
        message(FATAL_ERROR "${key} is not an existing file: ${cfg_path}")
    endif()
endforeach()

set(program "${WORK}/return-zero")
run_installed(-o "${program}" "${SOURCE}")
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "installed scpp failed (${run_result})\n${run_output}")
endif()
execute_process(COMMAND "${program}" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "installed program exited ${result}")
endif()

file(COPY "${prefix}/lib/libsmallcpp_runtime.a"
    DESTINATION "${WORK}/saved-lib")
file(WRITE "${prefix}/lib/libsmallcpp_runtime.a" "not an archive\n")
run_installed(-o "${WORK}/return-zero-bad" "${SOURCE}")
if(run_result EQUAL 0)
    message(FATAL_ERROR "scpp linked a runtime other than the config path\n${run_output}")
endif()
file(COPY "${WORK}/saved-lib/libsmallcpp_runtime.a"
    DESTINATION "${prefix}/lib")

file(READ "${ini}" saved_ini)
file(RENAME "${prefix}/include" "${WORK}/headers")
string(REPLACE "include=${prefix}/include" "include=${WORK}/headers" moved "${saved_ini}")
file(WRITE "${ini}" "${moved}")
file(WRITE "${WORK}/use-header.scpp"
    "#include <exceptions>\nint main(void)\n{\n    return 0;\n}\n")
run_installed(-o "${WORK}/use-header" "${WORK}/use-header.scpp")
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR
        "scpp did not find <exceptions> through config.ini\n${run_output}")
endif()

file(MAKE_DIRECTORY "${WORK}/dash-i")
file(WRITE "${WORK}/dash-i/exceptions" "#error dash-i-wins\n")
run_installed(-I "${WORK}/dash-i" -o "${WORK}/dash-header" "${WORK}/use-header.scpp")
if(run_result EQUAL 0)
    message(FATAL_ERROR "explicit -I did not win over config.ini")
endif()
string(FIND "${run_output}" "dash-i-wins" needle_at)
if(needle_at EQUAL -1)
    message(FATAL_ERROR "diagnostic did not name -I\n${run_output}")
endif()

file(RENAME "${WORK}/headers" "${prefix}/include")
file(WRITE "${ini}" "${saved_ini}")

file(READ "${prefix}/include/exceptions" saved_exceptions)
file(WRITE "${prefix}/include/exceptions" "#error installed-prefix-header\n")
run_installed(-o "${WORK}/use-header" "${WORK}/use-header.scpp")
if(run_result EQUAL 0)
    message(FATAL_ERROR "scpp used a header outside the install prefix")
endif()
string(FIND "${run_output}" "installed-prefix-header" needle_at)
if(needle_at EQUAL -1)
    message(FATAL_ERROR
        "diagnostic did not name the installed header\n${run_output}")
endif()
file(WRITE "${prefix}/include/exceptions" "${saved_exceptions}")

file(WRITE "${WORK}/use-math.scpp" [=[
#include <math>
int main(void)
{
    volatile double value;
    value = 4.0;
    if (sqrt(value) == 2.0)
    {
        return 0;
    }
    return 1;
}
]=])
run_installed(-o "${WORK}/use-math" "${WORK}/use-math.scpp")
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "math link failed (${run_result})\n${run_output}")
endif()
execute_process(COMMAND "${WORK}/use-math" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "math program exited ${result}")
endif()
execute_process(
    COMMAND "${NM}" "${WORK}/use-math"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE symbols
    ERROR_VARIABLE nm_err)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "nm failed\n${nm_err}")
endif()
set(symbols "${symbols}\n")
if(NOT symbols MATCHES "[0-9A-Fa-f]+ [A-Za-z] sqrt\n")
    message(FATAL_ERROR "link did not pull sqrt from libm.a\n${symbols}")
endif()

set(missing "${WORK}/missing-libm.a")
string(REPLACE "libm=${cfg_libm}" "libm=${missing}" broken "${saved_ini}")
file(WRITE "${ini}" "${broken}")
run_installed(-o "${WORK}/math-missing" "${WORK}/use-math.scpp")
if(run_result EQUAL 0)
    message(FATAL_ERROR "link succeeded with a missing libm path\n${run_output}")
endif()
string(FIND "${run_output}" "${missing}" needle_at)
if(needle_at EQUAL -1)
    message(FATAL_ERROR "link did not use the recorded libm path\n${run_output}")
endif()

run_installed(-l m -o "${WORK}/math-named" "${WORK}/use-math.scpp")
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "explicit -l m did not win\n${run_output}")
endif()

file(WRITE "${ini}" "${saved_ini}")
file(RENAME "${ini}" "${ini}.aside")
run_installed(-o "${WORK}/use-header-fallback" "${WORK}/use-header.scpp")
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR
        "missing config.ini failed the compile\n${run_output}")
endif()
file(RENAME "${ini}.aside" "${ini}")
