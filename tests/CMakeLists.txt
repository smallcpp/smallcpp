add_executable(parse_test parse_main.c)
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

add_executable(verify_module verify_module.cpp)
target_link_libraries(verify_module PRIVATE ${SMALLCPP_LLVM_LIBS})
smallcpp_use_llvm(verify_module)

add_test(
    NAME compile_return
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:shell>
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
            -DCOMPILER=$<TARGET_FILE:shell>
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
            -DCOMPILER=$<TARGET_FILE:shell>
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/${source}
            -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/${name}-run
            -DCHECK=${check}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunProgram.cmake)
endfunction()

add_run_example(return return-zero.scpp status)
add_run_example(print print-literal.scpp print)
add_run_example(format format-value.scpp status)

add_test(
    NAME collect_live_root
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:shell>
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
