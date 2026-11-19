cmake_minimum_required(VERSION 3.16)
project(smallcpp LANGUAGES C CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/InstallPrefix.cmake")

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

find_path(SMALLCPP_SQLITE3_H sqlite3.h)
find_library(SMALLCPP_SQLITE3_LIB sqlite3)
if(NOT SMALLCPP_SQLITE3_H OR NOT SMALLCPP_SQLITE3_LIB)
    message(FATAL_ERROR
        "sqlite3 development files are required to build smallcpp_sqlite")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 gtk4)
if(NOT GTK4_FOUND)
    message(FATAL_ERROR
        "GTK 4 development files are required to build smallcpp_gtk")
endif()

add_subdirectory(src/compiler)

add_library(smallcpp_cli STATIC src/cli/CommandLine.cpp)
target_include_directories(smallcpp_cli PUBLIC ${CMAKE_SOURCE_DIR}/src/cli)
target_compile_features(smallcpp_cli PUBLIC cxx_std_17)
if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(smallcpp_cli PUBLIC -fno-rtti)
endif()

add_executable(preproc
    src/preproc/main.cpp
    src/preproc/process.cpp
    src/prefix.cpp)
target_include_directories(preproc PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(preproc PRIVATE smallcpp_cli)
target_compile_features(preproc PRIVATE cxx_std_17)
if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(preproc PRIVATE -fno-rtti)
endif()
target_compile_definitions(preproc PRIVATE
    "SMALLCPP_VERSION=\"0.1\""
    "SMALLCPP_PREFIX=\"${CMAKE_INSTALL_PREFIX}\""
    "SMALLCPP_STDINC=\"${CMAKE_SOURCE_DIR}/include\"")

add_library(smallcpp_runtime STATIC
    src/runtime/heap.c
    src/runtime/trace.c
    src/runtime/image.c
    src/runtime/io.c
    src/runtime/except.c
    src/runtime/lines.c)
target_include_directories(smallcpp_runtime PUBLIC src/runtime)
target_link_libraries(smallcpp_runtime PUBLIC ${CMAKE_DL_LIBS})

find_program(SMALLCPP_CLANGXX NAMES clang++ HINTS /usr/lib/llvm-22/bin)
if(NOT SMALLCPP_CLANGXX)
    message(FATAL_ERROR "clang++ is required to link an executable")
endif()

add_library(smallcpp_emit STATIC
    src/prefix.cpp
    src/emit/codegen.cpp
    src/emit/link.cpp
    src/emit/lower_scalar.cpp
    src/emit/lower_call.cpp
    src/emit/lower_vararg.cpp
    src/emit/lower_aggregate.cpp
    src/emit/lower_namespace.cpp
    src/emit/lower_class.cpp
    src/emit/lower_typeid.cpp
    src/emit/lower_alloc.cpp
    src/emit/lower_except.cpp
    src/emit/lower_print.cpp
    src/emit/debug.cpp)
target_include_directories(smallcpp_emit PUBLIC ${CMAKE_SOURCE_DIR}/src/emit)
target_include_directories(smallcpp_emit PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(smallcpp_emit PUBLIC smallcpp_parse ${SMALLCPP_LLVM_LIBS})
smallcpp_use_llvm(smallcpp_emit)
target_compile_definitions(smallcpp_emit PRIVATE
    "SMALLCPP_PREFIX=\"${CMAKE_INSTALL_PREFIX}\""
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

add_executable(scpp
    src/scpp/main.cpp
    src/scpp/stackmap.cpp
    src/prefix.cpp)
target_include_directories(scpp PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(scpp PRIVATE smallcpp_cli)
target_compile_features(scpp PRIVATE cxx_std_17)
if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(scpp PRIVATE -fno-rtti)
endif()
target_compile_definitions(scpp PRIVATE
    "SMALLCPP_VERSION=\"0.1\""
    "SMALLCPP_PREFIX=\"${CMAKE_INSTALL_PREFIX}\""
    "SMALLCPP_PREPROC=\"$<TARGET_FILE:preproc>\""
    "SMALLCPP_CC=\"$<TARGET_FILE:scpp_cc>\""
    "SMALLCPP_CLANGXX=\"${SMALLCPP_CLANGXX}\""
    "SMALLCPP_RUNTIME=\"$<TARGET_FILE:smallcpp_runtime>\"")
add_dependencies(scpp preproc scpp_cc smallcpp_runtime)

set(SMALLCPP_SQLITE_OBJECT "${CMAKE_BINARY_DIR}/sqlite.o")
set(SMALLCPP_SQLITE_ARCHIVE "${CMAKE_SOURCE_DIR}/lib/libsmallcpp_sqlite.a")
add_custom_command(
    OUTPUT "${SMALLCPP_SQLITE_OBJECT}"
    COMMAND "$<TARGET_FILE:scpp>" -c -o "${SMALLCPP_SQLITE_OBJECT}"
            "${CMAKE_SOURCE_DIR}/src/sqlite/sqlite.scpp"
    DEPENDS scpp
            "${CMAKE_SOURCE_DIR}/src/sqlite/sqlite.scpp"
            "${CMAKE_SOURCE_DIR}/include/sqlite"
            "${CMAKE_SOURCE_DIR}/include/exceptions"
    VERBATIM)
add_custom_command(
    OUTPUT "${SMALLCPP_SQLITE_ARCHIVE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_SOURCE_DIR}/lib"
    COMMAND "${CMAKE_AR}" rcs "${SMALLCPP_SQLITE_ARCHIVE}"
            "${SMALLCPP_SQLITE_OBJECT}"
    DEPENDS "${SMALLCPP_SQLITE_OBJECT}"
    VERBATIM)
add_custom_target(smallcpp_sqlite ALL DEPENDS "${SMALLCPP_SQLITE_ARCHIVE}")

set(SMALLCPP_GTK_OBJECTS "")
foreach(name state basic layout input text)
    set(object "${CMAKE_BINARY_DIR}/gtk-${name}.o")
    list(APPEND SMALLCPP_GTK_OBJECTS "${object}")
    add_custom_command(
        OUTPUT "${object}"
        COMMAND "$<TARGET_FILE:scpp>" -c -o "${object}"
                "${CMAKE_SOURCE_DIR}/src/gtk/${name}.scpp"
        DEPENDS scpp
                "${CMAKE_SOURCE_DIR}/src/gtk/${name}.scpp"
                "${CMAKE_SOURCE_DIR}/src/gtk/gtk-native"
                "${CMAKE_SOURCE_DIR}/src/gtk/gtk-support"
                "${CMAKE_SOURCE_DIR}/src/gtk/gtk-globals"
                "${CMAKE_SOURCE_DIR}/include/gtk"
                "${CMAKE_SOURCE_DIR}/include/exceptions"
        VERBATIM)
endforeach()
set(SMALLCPP_GTK_ARCHIVE "${CMAKE_SOURCE_DIR}/lib/libsmallcpp_gtk.a")
add_custom_command(
    OUTPUT "${SMALLCPP_GTK_ARCHIVE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_SOURCE_DIR}/lib"
    COMMAND "${CMAKE_AR}" rcs "${SMALLCPP_GTK_ARCHIVE}"
            ${SMALLCPP_GTK_OBJECTS}
    DEPENDS ${SMALLCPP_GTK_OBJECTS}
    VERBATIM)
add_custom_target(smallcpp_gtk ALL DEPENDS "${SMALLCPP_GTK_ARCHIVE}")

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "Object code libraries")
endif()
include(GNUInstallDirs)
install(TARGETS scpp preproc scpp_cc smallcpp_runtime
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
set_source_files_properties(
    "${SMALLCPP_SQLITE_ARCHIVE}"
    "${SMALLCPP_GTK_ARCHIVE}"
    PROPERTIES GENERATED TRUE)
install(FILES
    "${SMALLCPP_SQLITE_ARCHIVE}"
    "${SMALLCPP_GTK_ARCHIVE}"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

add_subdirectory(tests)
