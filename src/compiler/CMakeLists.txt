find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(SmallcppParser smallcpp.y
    ${CMAKE_CURRENT_BINARY_DIR}/smallcpp.tab.c
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/smallcpp.tab.h)
FLEX_TARGET(SmallcppScanner smallcpp.l
    ${CMAKE_CURRENT_BINARY_DIR}/smallcpp.lex.c)
ADD_FLEX_BISON_DEPENDENCY(SmallcppScanner SmallcppParser)

add_library(smallcpp_parse STATIC
    ast.c
    diagnostic.c
    ${BISON_SmallcppParser_OUTPUTS}
    ${FLEX_SmallcppScanner_OUTPUTS})
target_include_directories(smallcpp_parse PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR})
