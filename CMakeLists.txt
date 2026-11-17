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

add_executable(scpp
    src/shell/main.cpp
    src/shell/codegen.cpp
    src/shell/link.cpp
    src/shell/lower_scalar.cpp
    src/shell/lower_call.cpp
    src/shell/lower_aggregate.cpp
    src/shell/lower_namespace.cpp
    src/shell/lower_class.cpp
    src/shell/lower_alloc.cpp
    src/shell/lower_except.cpp
    src/shell/lower_print.cpp)
target_link_libraries(scpp PRIVATE smallcpp_parse smallcpp_cli ${SMALLCPP_LLVM_LIBS})
smallcpp_use_llvm(scpp)

add_library(smallcpp_runtime STATIC
    src/runtime/heap.c
    src/runtime/trace.c
    src/runtime/image.c
    src/runtime/io.c
    src/runtime/main.c)
target_include_directories(smallcpp_runtime PUBLIC src/runtime)
target_link_libraries(smallcpp_runtime PUBLIC ${CMAKE_DL_LIBS})

find_program(SMALLCPP_CLANGXX NAMES clang++ HINTS /usr/lib/llvm-22/bin)
if(NOT SMALLCPP_CLANGXX)
    message(FATAL_ERROR "clang++ is required to link an executable")
endif()
target_compile_definitions(scpp PRIVATE
    "SMALLCPP_VERSION=\"0.1\""
    "SMALLCPP_CLANGXX=\"${SMALLCPP_CLANGXX}\""
    "SMALLCPP_RUNTIME_LIB=\"$<TARGET_FILE:smallcpp_runtime>\"")
add_dependencies(scpp smallcpp_runtime)

add_subdirectory(tests)
