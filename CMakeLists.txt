cmake_minimum_required(VERSION 3.16)
project(smallcpp LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

# The test target builds the ordinary targets first. Test programs are not
# among them; CTest builds those before it runs the suite.
set(CMAKE_SKIP_TEST_ALL_DEPENDENCY FALSE)
enable_testing()

find_package(LLVM 22.1 REQUIRED CONFIG)
message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(SMALLCPP_LLVM_LIBS core support bitwriter bitreader analysis nativecodegen)

function(smallcpp_use_llvm target)
    target_include_directories(${target} SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE ${LLVM_DEFINITIONS_LIST})
    target_compile_features(${target} PRIVATE cxx_std_17)
    if(NOT LLVM_ENABLE_RTTI)
        target_compile_options(${target} PRIVATE -fno-rtti)
    endif()
endfunction()

add_subdirectory(src/compiler)

add_library(smallcpp_cli STATIC src/cli/CommandLine.cpp)
target_include_directories(smallcpp_cli PUBLIC ${CMAKE_SOURCE_DIR}/src/cli)
target_compile_features(smallcpp_cli PUBLIC cxx_std_17)
if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(smallcpp_cli PUBLIC -fno-rtti)
endif()

add_executable(preproc
    src/preproc/main.cpp
    src/preproc/process.cpp)
target_link_libraries(preproc PRIVATE smallcpp_cli)
target_compile_features(preproc PRIVATE cxx_std_17)
if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(preproc PRIVATE -fno-rtti)
endif()
target_compile_definitions(preproc PRIVATE "SMALLCPP_VERSION=\"0.1\"")

add_library(smallcpp_runtime STATIC
    src/runtime/heap.c
    src/runtime/trace.c
    src/runtime/image.c
    src/runtime/io.c)
target_include_directories(smallcpp_runtime PUBLIC src/runtime)
target_link_libraries(smallcpp_runtime PUBLIC ${CMAKE_DL_LIBS})

find_program(SMALLCPP_CLANGXX NAMES clang++ HINTS /usr/lib/llvm-22/bin)
if(NOT SMALLCPP_CLANGXX)
    message(FATAL_ERROR "clang++ is required to link an executable")
endif()

add_library(smallcpp_emit STATIC
    src/emit/codegen.cpp
    src/emit/link.cpp
    src/emit/lower_scalar.cpp
    src/emit/lower_call.cpp
    src/emit/lower_aggregate.cpp
    src/emit/lower_namespace.cpp
    src/emit/lower_class.cpp
    src/emit/lower_alloc.cpp
    src/emit/lower_except.cpp
    src/emit/lower_print.cpp
    src/emit/debug.cpp)
target_include_directories(smallcpp_emit PUBLIC ${CMAKE_SOURCE_DIR}/src/emit)
target_link_libraries(smallcpp_emit PUBLIC smallcpp_parse ${SMALLCPP_LLVM_LIBS})
smallcpp_use_llvm(smallcpp_emit)
target_compile_definitions(smallcpp_emit PRIVATE
    "SMALLCPP_CLANGXX=\"${SMALLCPP_CLANGXX}\""
    "SMALLCPP_RUNTIME_LIB=\"$<TARGET_FILE:smallcpp_runtime>\"")
add_dependencies(smallcpp_emit smallcpp_runtime)

add_executable(scpp_cc
    src/compiler/main.cpp
    src/compiler/dump.c)
target_link_libraries(scpp_cc PRIVATE smallcpp_emit smallcpp_cli)
target_compile_features(scpp_cc PRIVATE cxx_std_17)
if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(scpp_cc PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()
target_compile_definitions(scpp_cc PRIVATE "SMALLCPP_VERSION=\"0.1\"")
add_dependencies(scpp_cc smallcpp_runtime)

add_executable(scpp src/scpp/main.cpp)
target_link_libraries(scpp PRIVATE smallcpp_cli)
target_compile_features(scpp PRIVATE cxx_std_17)
if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(scpp PRIVATE -fno-rtti)
endif()
target_compile_definitions(scpp PRIVATE
    "SMALLCPP_VERSION=\"0.1\""
    "SMALLCPP_PREPROC=\"$<TARGET_FILE:preproc>\""
    "SMALLCPP_CC=\"$<TARGET_FILE:scpp_cc>\""
    "SMALLCPP_CLANGXX=\"${SMALLCPP_CLANGXX}\""
    "SMALLCPP_RUNTIME=\"$<TARGET_FILE:smallcpp_runtime>\"")
add_dependencies(scpp preproc scpp_cc smallcpp_runtime)

add_subdirectory(tests)
