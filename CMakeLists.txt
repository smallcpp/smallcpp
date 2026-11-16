cmake_minimum_required(VERSION 3.16)
project(smallcpp LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

enable_testing()

find_package(LLVM 22.1 REQUIRED CONFIG)
message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(SMALLCPP_LLVM_LIBS core support bitwriter bitreader analysis)

function(smallcpp_use_llvm target)
    target_include_directories(${target} SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE ${LLVM_DEFINITIONS_LIST})
    target_compile_features(${target} PRIVATE cxx_std_17)
    if(NOT LLVM_ENABLE_RTTI)
        target_compile_options(${target} PRIVATE -fno-rtti)
    endif()
endfunction()

add_subdirectory(src/compiler)

add_executable(shell src/shell/main.cpp src/shell/codegen.cpp)
target_link_libraries(shell PRIVATE smallcpp_parse ${SMALLCPP_LLVM_LIBS})
smallcpp_use_llvm(shell)

add_subdirectory(tests)
