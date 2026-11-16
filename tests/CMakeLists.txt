add_executable(parse_test parse_main.c)
target_link_libraries(parse_test PRIVATE smallcpp_parse)

add_custom_command(TARGET parse_test POST_BUILD
    COMMAND $<TARGET_FILE:parse_test>
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/accept.scpp
    COMMENT "Parsing tests/examples/accept.scpp"
    VERBATIM)
