add_executable(command_line_test EXCLUDE_FROM_ALL command_line_test.cpp)
target_link_libraries(command_line_test PRIVATE smallcpp_cli)
add_test(NAME command_line COMMAND command_line_test)

add_test(
    NAME scpp_help
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DNEEDLE=Usage:
        -DCLI_ARGS=--help
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME scpp_version
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DNEEDLE=scpp: v0.1
        -DCLI_ARGS=--version
        -DEXPECT_STATUS=0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)
add_test(
    NAME scpp_unknown_option
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DNEEDLE=unknown option
        -DCLI_ARGS=--not-an-option
        -DEXPECT_STATUS=2
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCli.cmake)

add_executable(parse_test EXCLUDE_FROM_ALL parse_main.c)
target_link_libraries(parse_test PRIVATE smallcpp_parse)

add_custom_command(TARGET parse_test POST_BUILD
    COMMAND $<TARGET_FILE:parse_test>
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/accept.scpp
    COMMENT "Parsing tests/examples/accept.scpp"
    VERBATIM)

# Parser examples. expect_fail is TRUE when parse_test must exit non-zero.
# Add later checks here, including the AST check when that lands.
function(add_parse_example name source expect_fail)
    add_test(
        NAME ${name}
        COMMAND $<TARGET_FILE:parse_test>
                ${CMAKE_CURRENT_SOURCE_DIR}/examples/${source})
    if(expect_fail)
        set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
    endif()
endfunction()

add_parse_example(parse_accept accept.scpp FALSE)
add_parse_example(parse_namespace namespace.scpp FALSE)
add_parse_example(parse_class_base class-base.scpp FALSE)
add_parse_example(parse_try_catch try-catch.scpp FALSE)
add_parse_example(parse_catch_all catch-all.scpp FALSE)
add_parse_example(parse_variadic variadic.scpp FALSE)
add_parse_example(parse_print_format print-format.scpp FALSE)
add_parse_example(parse_tree_shape tree-shape.scpp FALSE)
add_parse_example(parse_null null.scpp FALSE)
add_parse_example(parse_delete delete.scpp FALSE)
add_parse_example(parse_reject_delete reject-delete.scpp TRUE)
add_parse_example(parse_reject_class reject-class.scpp TRUE)
add_parse_example(parse_ctor_dtor ctor-dtor.scpp FALSE)
add_parse_example(parse_default_arg default-arg.scpp FALSE)
add_parse_example(parse_using using.scpp FALSE)
add_parse_example(parse_overload overload.scpp FALSE)
add_parse_example(parse_multi_base multi-base.scpp FALSE)
add_parse_example(parse_const_method const-method.scpp FALSE)
add_parse_example(parse_new_array new-array.scpp FALSE)
add_parse_example(parse_bool bool.scpp FALSE)
add_parse_example(parse_bit_field bit-field.scpp FALSE)
add_parse_example(parse_designated designated.scpp FALSE)
add_parse_example(parse_empty_params empty-params.scpp FALSE)
add_parse_example(parse_brace_init brace-init.scpp FALSE)

add_executable(verify_module EXCLUDE_FROM_ALL verify_module.cpp)
target_link_libraries(verify_module PRIVATE ${SMALLCPP_LLVM_LIBS})
smallcpp_use_llvm(verify_module)

add_test(
    NAME compile_output_option
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DVERIFY=$<TARGET_FILE:verify_module>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/return-zero.scpp
        -DBITCODE=${CMAKE_CURRENT_BINARY_DIR}/output-option.bc
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectOutputOption.cmake)

add_test(
    NAME compile_return
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DVERIFY=$<TARGET_FILE:verify_module>
        -DMODE=return
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/return-zero.scpp
        -DBITCODE=${CMAKE_CURRENT_BINARY_DIR}/answer.bc
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CompileReturn.cmake)
# One short example per scalar feature. The mode name is the file stem.
function(add_compile_example name)
    add_test(
        NAME compile_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCOMPILER=$<TARGET_FILE:scpp>
            -DVERIFY=$<TARGET_FILE:verify_module>
            -DMODE=${name}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/${name}.scpp
            -DBITCODE=${CMAKE_CURRENT_BINARY_DIR}/${name}.bc
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CompileReturn.cmake)
endfunction()

add_compile_example(if)
add_compile_example(else)
add_compile_example(while)
add_compile_example(do)
add_compile_example(for)
add_compile_example(for-decl)
add_compile_example(switch)
add_compile_example(break)
add_compile_example(continue)
add_compile_example(goto)
add_compile_example(void)
add_compile_example(bool-type)
add_compile_example(char)
add_compile_example(short)
add_compile_example(long)
add_compile_example(float)
add_compile_example(double)
add_compile_example(signed)
add_compile_example(unsigned)
add_compile_example(empty-list)
add_compile_example(parameter)
add_compile_example(local)
add_compile_example(global)
add_compile_example(const-var)
add_compile_example(volatile)
add_compile_example(true)
add_compile_example(false)
add_compile_example(literal)
add_compile_example(cast)
add_compile_example(sizeof)
add_compile_example(comma)
add_compile_example(conditional)
add_compile_example(add)
add_compile_example(sub)
add_compile_example(mul)
add_compile_example(div)
add_compile_example(mod)
add_compile_example(shift)
add_compile_example(compare)
add_compile_example(bit-and)
add_compile_example(bit-or)
add_compile_example(bit-xor)
add_compile_example(complement)
add_compile_example(negate)
add_compile_example(not)
add_compile_example(logical-and)
add_compile_example(logical-or)
add_compile_example(assign)
add_compile_example(add-assign)
add_compile_example(post-inc)
add_compile_example(pre-inc)
add_compile_example(call)
add_compile_example(overload-call)
add_compile_example(default-call)
add_compile_example(varargs)
add_compile_example(static)
add_compile_example(extern)
add_compile_example(inline)
add_compile_example(typedef)
add_compile_example(string)
add_compile_example(register)
add_compile_example(struct)
add_compile_example(union)
add_compile_example(enum)
add_compile_example(array)
add_compile_example(pointer)
add_compile_example(array-param)
add_compile_example(bitfield)
add_compile_example(designated-init)
add_compile_example(brace)
add_compile_example(namespace-fn)
add_compile_example(qualified-call)
add_compile_example(using-name)
add_compile_example(using-namespace)
add_compile_example(method)
add_compile_example(const-method-call)
add_compile_example(ctor)
add_compile_example(dtor)
add_compile_example(init-list)
add_compile_example(two-bases)
add_compile_example(virtual-call)
add_compile_example(base-call)
add_compile_example(new-object)
add_compile_example(new-array-call)
add_compile_example(new-pointer)
add_compile_example(delete-ptr)
add_compile_example(nullptr)
add_compile_example(throw-value)
add_compile_example(rethrow)
add_compile_example(catch-typed)
add_compile_example(catch-all-handler)
add_compile_example(print-literal)
add_compile_example(format-literal)

find_program(SMALLCPP_LLC NAMES llc HINTS /usr/lib/llvm-22/bin)
find_program(SMALLCPP_CLANG NAMES clang HINTS /usr/lib/llvm-22/bin)
if(NOT SMALLCPP_LLC OR NOT SMALLCPP_CLANG)
    message(FATAL_ERROR "llc and clang are required to run the collector")
endif()

function(add_run_example name source check)
    add_test(
        NAME run_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCOMPILER=$<TARGET_FILE:scpp>
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/${source}
            -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/${name}-run
            -DCHECK=${check}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunProgram.cmake)
endfunction()

add_run_example(return return-zero.scpp status)
add_run_example(print print-literal.scpp print)
add_run_example(format format-value.scpp status)

include(${CMAKE_CURRENT_SOURCE_DIR}/Evidence.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Preproc.cmake)

add_test(
    NAME collect_live_root
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DLLC=${SMALLCPP_LLC}
        -DCLANG=${SMALLCPP_CLANG}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/live-root.scpp
        -DBITCODE=${CMAKE_CURRENT_BINARY_DIR}/live-root.bc
        -DOBJECT=${CMAKE_CURRENT_BINARY_DIR}/live-root.o
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/live-root-run
        -DDRIVER=${CMAKE_CURRENT_SOURCE_DIR}/live_root.c
        -DRUNTIME=$<TARGET_FILE:smallcpp_runtime>
        -DINCLUDE=${CMAKE_SOURCE_DIR}/src/runtime
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CollectLive.cmake)

add_test(
    NAME cc_ast_text
    COMMAND ${CMAKE_COMMAND}
        -DCC=$<TARGET_FILE:scpp_cc>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/return-zero.scpp
        -DTEXT=${CMAKE_CURRENT_BINARY_DIR}/return-zero.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectAst.cmake)
add_test(
    NAME cc_ast_dot
    COMMAND ${CMAKE_COMMAND}
        -DCC=$<TARGET_FILE:scpp_cc>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/return-zero.scpp
        -DDOT=${CMAKE_CURRENT_BINARY_DIR}/return-zero.dot
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectAst.cmake)
add_test(
    NAME cc_ast_both
    COMMAND ${CMAKE_COMMAND}
        -DCC=$<TARGET_FILE:scpp_cc>
        -DVERIFY=$<TARGET_FILE:verify_module>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/return-zero.scpp
        -DTEXT=${CMAKE_CURRENT_BINARY_DIR}/return-zero-both.txt
        -DDOT=${CMAKE_CURRENT_BINARY_DIR}/return-zero-both.dot
        -DBITCODE=${CMAKE_CURRENT_BINARY_DIR}/return-zero-both.bc
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectAst.cmake)

set(phase_include ${CMAKE_CURRENT_SOURCE_DIR}/driver/inc)
add_test(
    NAME phase_emit_llvm
    COMMAND ${CMAKE_COMMAND}
        -DSCPP=$<TARGET_FILE:scpp>
        -DPREPROC=$<TARGET_FILE:preproc>
        -DCC=$<TARGET_FILE:scpp_cc>
        -DCLANG=${SMALLCPP_CLANGXX}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/driver/use-define.scpp
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/phase-emit.ll
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/phase-emit
        -DLOG=${CMAKE_CURRENT_BINARY_DIR}/phase-emit.log
        -DPHASE_ARGS=-DFOO=0|-I${phase_include}|-i|-g|-L${phase_include}|--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-emit.ll
        -DPREPROC_HAS=-D|FOO=0|-I|${phase_include}|-i
        -DPREPROC_LACKS=-g|-L|--emit-llvm|-c|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-emit.ll
        -DCC_HAS=-g|--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-emit.ll
        -DCC_LACKS=-D|-I|-i|-L|-c|FOO=0|${phase_include}
        -DCLANG_ABSENT=1
        -DMAGIC=bc
        -DKEEP_I=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPhases.cmake)
add_test(
    NAME phase_object
    COMMAND ${CMAKE_COMMAND}
        -DSCPP=$<TARGET_FILE:scpp>
        -DPREPROC=$<TARGET_FILE:preproc>
        -DCC=$<TARGET_FILE:scpp_cc>
        -DCLANG=${SMALLCPP_CLANGXX}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/return-zero.scpp
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/phase-object.o
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/phase-object
        -DLOG=${CMAKE_CURRENT_BINARY_DIR}/phase-object.log
        -DPHASE_ARGS=-g|-c|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-object.o
        -DPREPROC_LACKS=-g|-c|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-object.o|-L|--emit-llvm
        -DCC_HAS=-g|-c|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-object.o
        -DCC_LACKS=-L|--emit-llvm|-D|-I|-i
        -DCLANG_ABSENT=1
        -DMAGIC=elf
        -DKEEP_I=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPhases.cmake)
add_test(
    NAME phase_link
    COMMAND ${CMAKE_COMMAND}
        -DSCPP=$<TARGET_FILE:scpp>
        -DPREPROC=$<TARGET_FILE:preproc>
        -DCC=$<TARGET_FILE:scpp_cc>
        -DCLANG=${SMALLCPP_CLANGXX}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/return-zero.scpp
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/phase-link
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/phase-link-work
        -DLOG=${CMAKE_CURRENT_BINARY_DIR}/phase-link.log
        -DPHASE_ARGS=-DNAME=1|-I${phase_include}|-g|-L${phase_include}|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-link
        -DPREPROC_HAS=-D|NAME=1|-I|${phase_include}
        -DPREPROC_LACKS=-g|-L|-c|--emit-llvm|-o|-i|${CMAKE_CURRENT_BINARY_DIR}/phase-link
        -DCC_HAS=-g|-c|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-link.o
        -DCC_LACKS=-D|-I|-L|--emit-llvm|-i|NAME=1|${CMAKE_CURRENT_BINARY_DIR}/phase-link
        -DCLANG_HAS=-g|-L|${phase_include}|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-link
        -DCLANG_LACKS=-D|-I|-c|--emit-llvm|-i|NAME=1
        -DMAGIC=elf
        -DKEEP_I=1
        -DKEEP_O=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPhases.cmake)
add_test(
    NAME phase_ignore_short
    COMMAND ${CMAKE_COMMAND}
        -DSCPP=$<TARGET_FILE:scpp>
        -DPREPROC=$<TARGET_FILE:preproc>
        -DCC=$<TARGET_FILE:scpp_cc>
        -DCLANG=${SMALLCPP_CLANGXX}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/driver/env-ident.scpp
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/phase-ignore.ll
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/phase-ignore
        -DLOG=${CMAKE_CURRENT_BINARY_DIR}/phase-ignore.log
        -DPPENV=0
        -DEXPECT_STATUS=1
        -DNEEDLE=PPENV
        "-DABSENT=unknown option"
        -DPHASE_ARGS=-i|--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-ignore.ll
        -DPREPROC_HAS=-i
        -DPREPROC_LACKS=--ignore-environment|-g|-L|-c|--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-ignore.ll
        -DCC_HAS=--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-ignore.ll
        -DCC_LACKS=-i|--ignore-environment|-D|-I|-L|-g
        -DCLANG_ABSENT=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPhases.cmake)
add_test(
    NAME phase_ignore_long
    COMMAND ${CMAKE_COMMAND}
        -DSCPP=$<TARGET_FILE:scpp>
        -DPREPROC=$<TARGET_FILE:preproc>
        -DCC=$<TARGET_FILE:scpp_cc>
        -DCLANG=${SMALLCPP_CLANGXX}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/driver/env-ident.scpp
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/phase-ignore-long.ll
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/phase-ignore-long
        -DLOG=${CMAKE_CURRENT_BINARY_DIR}/phase-ignore-long.log
        -DPPENV=0
        -DEXPECT_STATUS=1
        -DNEEDLE=PPENV
        "-DABSENT=unknown option"
        -DPHASE_ARGS=--ignore-environment|--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-ignore-long.ll
        -DPREPROC_HAS=--ignore-environment
        -DPREPROC_LACKS=-i|-g|-L|-c|--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-ignore-long.ll
        -DCC_HAS=--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-ignore-long.ll
        -DCC_LACKS=-i|--ignore-environment|-D|-I|-L|-g
        -DCLANG_ABSENT=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPhases.cmake)
add_test(
    NAME phase_import_env
    COMMAND ${CMAKE_COMMAND}
        -DSCPP=$<TARGET_FILE:scpp>
        -DPREPROC=$<TARGET_FILE:preproc>
        -DCC=$<TARGET_FILE:scpp_cc>
        -DCLANG=${SMALLCPP_CLANGXX}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/driver/env-ident.scpp
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/phase-import.ll
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/phase-import
        -DLOG=${CMAKE_CURRENT_BINARY_DIR}/phase-import.log
        -DPPENV=0
        -DPHASE_ARGS=--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-import.ll
        -DPREPROC_LACKS=-i|--ignore-environment|-g|-L|-c|--emit-llvm
        -DCC_HAS=--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-import.ll
        -DCC_LACKS=-i|--ignore-environment|-D|-I|-L|-g
        -DCLANG_ABSENT=1
        -DMAGIC=bc
        -DKEEP_I=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPhases.cmake)
add_test(
    NAME phase_conflict
    COMMAND ${CMAKE_COMMAND}
        -DSCPP=$<TARGET_FILE:scpp>
        -DPREPROC=$<TARGET_FILE:preproc>
        -DCC=$<TARGET_FILE:scpp_cc>
        -DCLANG=${SMALLCPP_CLANGXX}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/return-zero.scpp
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/phase-conflict.o
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/phase-conflict
        -DLOG=${CMAKE_CURRENT_BINARY_DIR}/phase-conflict.log
        -DEXPECT_STATUS=2
        -DNEEDLE=cannot be combined
        -DPHASE_ARGS=-c|--emit-llvm|-o|${CMAKE_CURRENT_BINARY_DIR}/phase-conflict.o
        -DNO_PHASES=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectPhases.cmake)

add_test(
    NAME run_ledger
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/ledger-run
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/examples/ledger
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectLedger.cmake)
add_test(
    NAME debug_answer
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/driver/debug-answer.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/debug-answer
        -DPLAIN=${CMAKE_CURRENT_BINARY_DIR}/debug-answer-plain
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectDebug.cmake)

add_test(
    NAME diag_no_main
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/bad-no-main.scpp
        -DOBJECT=${CMAKE_CURRENT_BINARY_DIR}/bad-no-main.o
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/bad-no-main
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectNoMain.cmake)

add_test(
    NAME build_test_programs
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
            --target command_line_test parse_test verify_module)
set_tests_properties(build_test_programs PROPERTIES FIXTURES_SETUP smallcpp_test_programs)
get_property(smallcpp_tests DIRECTORY PROPERTY TESTS)
foreach(name IN LISTS smallcpp_tests)
    if(NOT name STREQUAL "build_test_programs")
        set_property(TEST ${name} APPEND PROPERTY FIXTURES_REQUIRED smallcpp_test_programs)
    endif()
endforeach()
