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
add_parse_example(parse_print_format print-format.scpp FALSE)
add_parse_example(parse_reject_class reject-class.scpp TRUE)
add_parse_example(parse_reject_designated reject-designated.scpp TRUE)
add_parse_example(parse_reject_second_base reject-second-base.scpp TRUE)
