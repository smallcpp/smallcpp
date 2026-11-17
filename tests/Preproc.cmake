function(add_pp name)
    set(extra "")
    if(ARGC GREATER 1)
        list(GET ARGV 1 extra)
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

function(add_pp_fail name needle)
    set(extra "")
    if(ARGC GREATER 2)
        list(GET ARGV 2 extra)
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
    NAME pp_no_keep
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:preproc>
        -DNEEDLE=unknown option
        -DCLI_ARGS=--keep
        -DEXPECT_STATUS=2
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
