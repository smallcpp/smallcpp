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
