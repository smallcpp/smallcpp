function(add_pp name)
    # -i keeps the process environment out of examples that do not ask for it.
    set(extra "-i")
    if(ARGC GREATER 1)
        set(extra "-i ${ARGV1}")
    endif()
    add_test(
        NAME pp_${name}
        COMMAND ${CMAKE_COMMAND}
            -DPREPROC=$<TARGET_FILE:preproc>
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/preproc/${name}.c
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/preproc/${name}.txt
            -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/pp-${name}.i
            "-DEXTRA=${extra}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPreproc.cmake)
endfunction()

function(add_pp_env name extra env)
    add_test(
        NAME pp_${name}
        COMMAND ${CMAKE_COMMAND}
            -DPREPROC=$<TARGET_FILE:preproc>
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/preproc/${name}.c
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/preproc/${name}.txt
            -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/pp-${name}.i
            "-DEXTRA=${extra}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPreproc.cmake)
    set_tests_properties(pp_${name} PROPERTIES ENVIRONMENT "${env}")
endfunction()

function(add_pp_fail name needle)
    set(extra "-i")
    if(ARGC GREATER 2)
        set(extra "-i ${ARGV2}")
    endif()
    add_test(
        NAME pp_${name}
        COMMAND ${CMAKE_COMMAND}
            -DPREPROC=$<TARGET_FILE:preproc>
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/preproc/${name}.c
            -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/pp-${name}.i
            -DEXPECT_FAIL=1
            "-DNEEDLE=${needle}"
            "-DEXTRA=${extra}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPreproc.cmake)
endfunction()

add_pp(object)
add_pp(function)
add_pp(undef)
add_pp(nested)
add_pp(argument)
add_pp(paste)
add_pp(paste-keep)
add_pp(define-macro)
add_pp(include)
add_pp(include-angle "-I ${CMAKE_CURRENT_SOURCE_DIR}/preproc/angle")
add_pp(include-guard)
add_pp(include-from-macro)
add_pp(if-else)
add_pp(elif)
add_pp(ifdef)
add_pp(ifndef)
add_pp(nested-if)
add_pp(defined)
add_pp(forward)
add_pp(comment)
add_pp(string)
add_pp(stringify)
add_pp(not-called)
add_pp(empty)
add_pp(splice)
add_pp(line)
add_pp(file-line)
add_pp(parens)
add_pp(logic)
add_pp(redefine-same)
add_pp(dash-d "-DFOO=3")
add_pp(dash-d-one "-DBAR")
add_pp(undef-d "-DFOO=4 -UFOO")
add_pp(define-after-u "-UFOO -DFOO=4")
add_pp(force-include "-include ${CMAKE_CURRENT_SOURCE_DIR}/preproc/inc/early.h")
add_pp(force-two "-include ${CMAKE_CURRENT_SOURCE_DIR}/preproc/inc/early.h -include ${CMAKE_CURRENT_SOURCE_DIR}/preproc/inc/later.h")
add_pp(force-search "-I ${CMAKE_CURRENT_SOURCE_DIR}/preproc/inc -include early.h")
add_pp(i-wins "-I ${CMAKE_CURRENT_SOURCE_DIR}/preproc/stdinc")
add_pp(nostdinc-keeps-i "-nostdinc -I ${CMAKE_CURRENT_SOURCE_DIR}/preproc/angle")
add_pp_env(env "" "PPENV=42")
add_pp_env(env-body "" "PPENV=1 + 2")
add_pp_env(env-expand "" "PPENV=PPOTHER")
add_pp_env(env-override "-DPPENV=2" "PPENV=1")
add_pp_env(env-undef "-UPPENV" "PPENV=1")
add_pp_env(env-d-then-u "-DPPENV=9 -UPPENV" "PPENV=1")
add_pp_env(env-u-then-d "-UPPENV -DPPENV=9" "PPENV=1")
add_pp_env(ignore-short "-i" "PPENV=42")
add_pp_env(ignore-long "--ignore-environment" "PPENV=42")
add_pp_fail(error boom)
add_pp_fail(pragma "pragma is rejected")
add_pp_fail(paste-bad "does not form one token")
add_pp_fail(cycle "expansion did not finish")
add_pp_fail(redefine "already defined")
add_pp_fail(missing-include "cannot include")
add_pp_fail(unknown-directive "unknown directive")
add_pp_fail(variadic "variadic macros are rejected")
add_pp_fail(unclosed-if "missing")
add_pp_fail(else-else "else after")
add_pp_fail(divzero "division by zero")
add_pp_fail(nostdinc "cannot include" "-nostdinc")

add_test(
    NAME pp_help
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        -DNEEDLE=Usage:
        -DCLI_ARGS=--help
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME pp_version
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        -DNEEDLE=preproc: v0.1
        -DCLI_ARGS=--version
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME pp_stdout
    COMMAND ${CMAKE_COMMAND}
        -DPREPROC=$<TARGET_FILE:preproc>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/preproc/stdout.c
        -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/preproc/stdout.txt
        -DEXTRA=-i
        -DSTDOUT=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPreproc.cmake)
add_test(
    NAME pp_stdinc
    COMMAND ${CMAKE_COMMAND}
        -DPREPROC=$<TARGET_FILE:preproc>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/preproc/nostdinc.c
        -DEXTRA=-i
        -DEXPECT_FOUND=1
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/pp-stdinc.i
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPreproc.cmake)
add_test(
    NAME pp_two_files
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        -DNEEDLE=only one input file
        "-DCLI_ARGS=${CMAKE_CURRENT_SOURCE_DIR}/preproc/object.c ${CMAKE_CURRENT_SOURCE_DIR}/preproc/object.c"
        -DEXPECT_STATUS=2
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME pp_help_ignore
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        "-DNEEDLE=-i, --ignore-environment"
        -DCLI_ARGS=--help
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME pp_help_include
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        -DNEEDLE=-include
        -DCLI_ARGS=--help
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME pp_help_nostdinc
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        -DNEEDLE=-nostdinc
        -DCLI_ARGS=--help
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME pp_help_undef
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        "-DNEEDLE=-U, --undef"
        -DCLI_ARGS=--help
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME pp_help_define
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        "-DNEEDLE=-D, --define"
        -DCLI_ARGS=--help
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME pp_help_dir
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        "-DNEEDLE=-I, --include-dir"
        -DCLI_ARGS=--help
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME pp_help_output
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        "-DNEEDLE=-o, --output"
        -DCLI_ARGS=--help
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME pp_no_keep
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        -DNEEDLE=unknown option
        -DCLI_ARGS=--keep
        -DEXPECT_STATUS=2
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
