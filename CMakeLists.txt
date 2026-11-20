cmake_minimum_required(VERSION 3.16)
project(smallcpp LANGUAGES C CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/InstallPrefix.cmake")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

# The test target is defined after the other targets. It builds the
# ordinary targets and the test drivers, then runs CTest.
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
    src/prefix.cpp
    src/config.cpp)
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

set(SMALLCPP_RUNTIME_SCPP_DIR "${CMAKE_BINARY_DIR}/runtime-scpp")
set(SMALLCPP_RUNTIME_SCPP_STAMP "${SMALLCPP_RUNTIME_SCPP_DIR}/stamp")
set(SMALLCPP_RUNTIME_ARRAY_O "${SMALLCPP_RUNTIME_SCPP_DIR}/array.o")
set(SMALLCPP_RUNTIME_STRING_O "${SMALLCPP_RUNTIME_SCPP_DIR}/string.o")
set(SMALLCPP_RUNTIME_CLI_O "${SMALLCPP_RUNTIME_SCPP_DIR}/cli.o")
add_custom_command(
    OUTPUT "${SMALLCPP_RUNTIME_ARRAY_O}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SMALLCPP_RUNTIME_SCPP_DIR}"
    COMMAND "$<TARGET_FILE:scpp>" -I "${CMAKE_SOURCE_DIR}/include"
            -c -o "${SMALLCPP_RUNTIME_ARRAY_O}"
            "${CMAKE_SOURCE_DIR}/src/runtime/array.scpp"
    DEPENDS scpp
            "${CMAKE_SOURCE_DIR}/src/runtime/array.scpp"
            "${CMAKE_SOURCE_DIR}/include/array.shpp"
            "${CMAKE_SOURCE_DIR}/include/exceptions.shpp"
    VERBATIM)
add_custom_command(
    OUTPUT "${SMALLCPP_RUNTIME_STRING_O}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SMALLCPP_RUNTIME_SCPP_DIR}"
    COMMAND "$<TARGET_FILE:scpp>" -I "${CMAKE_SOURCE_DIR}/include"
            -c -o "${SMALLCPP_RUNTIME_STRING_O}"
            "${CMAKE_SOURCE_DIR}/src/runtime/string.scpp"
    DEPENDS scpp
            "${CMAKE_SOURCE_DIR}/src/runtime/string.scpp"
            "${CMAKE_SOURCE_DIR}/include/string.shpp"
            "${CMAKE_SOURCE_DIR}/include/array.shpp"
            "${CMAKE_SOURCE_DIR}/include/exceptions.shpp"
            "${CMAKE_SOURCE_DIR}/include/vector.shpp"
    VERBATIM)
add_custom_command(
    OUTPUT "${SMALLCPP_RUNTIME_CLI_O}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SMALLCPP_RUNTIME_SCPP_DIR}"
    COMMAND "$<TARGET_FILE:scpp>" -I "${CMAKE_SOURCE_DIR}/include"
            -c -o "${SMALLCPP_RUNTIME_CLI_O}"
            "${CMAKE_SOURCE_DIR}/src/runtime/cli.scpp"
    DEPENDS scpp
            "${CMAKE_SOURCE_DIR}/src/runtime/cli.scpp"
            "${CMAKE_SOURCE_DIR}/include/cli.shpp"
            "${CMAKE_SOURCE_DIR}/include/exceptions.shpp"
    VERBATIM)
add_custom_command(
    OUTPUT "${SMALLCPP_RUNTIME_SCPP_STAMP}"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${SMALLCPP_RUNTIME_SCPP_DIR}/c"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SMALLCPP_RUNTIME_SCPP_DIR}/c"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:smallcpp_runtime>"
            "${SMALLCPP_RUNTIME_SCPP_DIR}/c-only.a"
    COMMAND "${CMAKE_COMMAND}" -E chdir "${SMALLCPP_RUNTIME_SCPP_DIR}/c"
            "${CMAKE_AR}" x "${SMALLCPP_RUNTIME_SCPP_DIR}/c-only.a"
    COMMAND "${CMAKE_COMMAND}" -E rm -f "$<TARGET_FILE:smallcpp_runtime>"
    COMMAND "${CMAKE_COMMAND}"
            -DARCHIVE=$<TARGET_FILE:smallcpp_runtime>
            -DSTAGE=${SMALLCPP_RUNTIME_SCPP_DIR}/c
            -DAR=${CMAKE_AR}
            -DCLI=${SMALLCPP_RUNTIME_CLI_O}
            -DSTRING=${SMALLCPP_RUNTIME_STRING_O}
            -DARRAY=${SMALLCPP_RUNTIME_ARRAY_O}
            -P "${CMAKE_SOURCE_DIR}/cmake/JoinRuntime.cmake"
    COMMAND "${CMAKE_COMMAND}" -E touch "${SMALLCPP_RUNTIME_SCPP_STAMP}"
    DEPENDS smallcpp_runtime
            "${SMALLCPP_RUNTIME_CLI_O}"
            "${SMALLCPP_RUNTIME_STRING_O}"
            "${SMALLCPP_RUNTIME_ARRAY_O}"
            "${CMAKE_SOURCE_DIR}/cmake/JoinRuntime.cmake"
    VERBATIM)
add_custom_target(smallcpp_runtime_scpp ALL DEPENDS "${SMALLCPP_RUNTIME_SCPP_STAMP}")


find_program(SMALLCPP_CLANGXX NAMES clang++ HINTS /usr/lib/llvm-22/bin)
if(NOT SMALLCPP_CLANGXX)
    message(FATAL_ERROR "clang++ is required to link an executable")
endif()

add_library(smallcpp_emit STATIC
    src/prefix.cpp
    src/config.cpp
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
    src/prefix.cpp
    src/config.cpp)
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

set(SMALLCPP_FORMAT_BIN "${CMAKE_SOURCE_DIR}/bin/scpp-format")
# scpp names a link's objects from each source base name in the working
# directory. format and the language server both compile main.scpp.
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/format-link")
add_custom_command(
    OUTPUT "${SMALLCPP_FORMAT_BIN}"
    COMMAND "${CMAKE_COMMAND}" -E env
            "SMALLCPP_RUNTIME=$<TARGET_FILE:smallcpp_runtime>"
            "$<TARGET_FILE:scpp>" -I "${CMAKE_SOURCE_DIR}/include"
            -o "${SMALLCPP_FORMAT_BIN}"
            "${CMAKE_SOURCE_DIR}/src/format/main.scpp"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/format-link"
    DEPENDS scpp smallcpp_runtime_scpp
            "${CMAKE_SOURCE_DIR}/src/format/main.scpp"
            "${CMAKE_SOURCE_DIR}/src/format/format.shpp"
            "${SMALLCPP_RUNTIME_SCPP_STAMP}"
    VERBATIM)
add_custom_target(scpp_format ALL DEPENDS "${SMALLCPP_FORMAT_BIN}")

set(SMALLCPP_SQLITE_OBJECT "${CMAKE_BINARY_DIR}/sqlite-obj/sqlite.o")
set(SMALLCPP_SQLITE_ARCHIVE "${CMAKE_SOURCE_DIR}/lib/libsmallcpp_sqlite.a")
add_custom_command(
    OUTPUT "${SMALLCPP_SQLITE_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/sqlite-obj"
    COMMAND "$<TARGET_FILE:scpp>" -I "${CMAKE_SOURCE_DIR}/include"
            -c -o "${SMALLCPP_SQLITE_OBJECT}"
            "${CMAKE_SOURCE_DIR}/src/sqlite/sqlite.scpp"
    DEPENDS scpp
            "${CMAKE_SOURCE_DIR}/src/sqlite/sqlite.scpp"
            "${CMAKE_SOURCE_DIR}/include/sqlite"
            "${CMAKE_SOURCE_DIR}/include/sqlite.shpp"
            "${CMAKE_SOURCE_DIR}/src/sqlite/state.shpp"
            "${CMAKE_SOURCE_DIR}/include/exceptions"
            "${CMAKE_SOURCE_DIR}/include/exceptions.shpp"
            "${SMALLCPP_RUNTIME_SCPP_STAMP}"
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
    set(object "${CMAKE_BINARY_DIR}/gtk-obj/gtk-${name}.o")
    list(APPEND SMALLCPP_GTK_OBJECTS "${object}")
    add_custom_command(
        OUTPUT "${object}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/gtk-obj"
        COMMAND "$<TARGET_FILE:scpp>" -I "${CMAKE_SOURCE_DIR}/include"
                -c -o "${object}"
                "${CMAKE_SOURCE_DIR}/src/gtk/${name}.scpp"
        DEPENDS scpp
                "${CMAKE_SOURCE_DIR}/src/gtk/${name}.scpp"
                "${CMAKE_SOURCE_DIR}/src/gtk/gtk-native"
                "${CMAKE_SOURCE_DIR}/src/gtk/gtk-support"
                "${CMAKE_SOURCE_DIR}/src/gtk/gtk-globals"
                "${CMAKE_SOURCE_DIR}/include/gtk"
                "${CMAKE_SOURCE_DIR}/include/gtk.shpp"
                "${CMAKE_SOURCE_DIR}/src/gtk/layout.shpp"
                "${CMAKE_SOURCE_DIR}/src/gtk/state.shpp"
                "${CMAKE_SOURCE_DIR}/src/gtk/key.shpp"
                "${CMAKE_SOURCE_DIR}/include/exceptions"
                "${CMAKE_SOURCE_DIR}/include/exceptions.shpp"
                "${SMALLCPP_RUNTIME_SCPP_STAMP}"
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
    "${SMALLCPP_FORMAT_BIN}"
    "${SMALLCPP_SQLITE_ARCHIVE}"
    "${SMALLCPP_GTK_ARCHIVE}"
    PROPERTIES GENERATED TRUE)
install(PROGRAMS "${SMALLCPP_FORMAT_BIN}"
    DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES
    "${SMALLCPP_SQLITE_ARCHIVE}"
    "${SMALLCPP_GTK_ARCHIVE}"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/InstallConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/cmake/InstallConfig.cmake"
    @ONLY)
install(SCRIPT "${CMAKE_BINARY_DIR}/cmake/InstallConfig.cmake")

set(SMALLCPP_LSP_DIR "${CMAKE_SOURCE_DIR}/src/lsp")
set(SMALLCPP_LSP_BIN "${CMAKE_SOURCE_DIR}/bin/scpp-lsp")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lsp-link")
add_custom_command(
    OUTPUT "${SMALLCPP_LSP_BIN}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_SOURCE_DIR}/bin"
    COMMAND "${CMAKE_COMMAND}" -E env
        "SMALLCPP_RUNTIME=$<TARGET_FILE:smallcpp_runtime>"
        "$<TARGET_FILE:scpp>"
        -o "${SMALLCPP_LSP_BIN}"
        -I "${CMAKE_SOURCE_DIR}/include"
        -I "${SMALLCPP_LSP_DIR}"
        -L "${CMAKE_SOURCE_DIR}/lib"
        -l smallcpp_parse
        "${SMALLCPP_LSP_DIR}/json.scpp"
        "${SMALLCPP_LSP_DIR}/check.scpp"
        "${SMALLCPP_LSP_DIR}/protocol.scpp"
        "${SMALLCPP_LSP_DIR}/main.scpp"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/lsp-link"
    DEPENDS scpp smallcpp_parse smallcpp_runtime_scpp
        "${SMALLCPP_LSP_DIR}/lsp"
        "${SMALLCPP_LSP_DIR}/lsp.shpp"
        "${SMALLCPP_LSP_DIR}/json.scpp"
        "${SMALLCPP_LSP_DIR}/check.scpp"
        "${SMALLCPP_LSP_DIR}/protocol.scpp"
        "${SMALLCPP_LSP_DIR}/main.scpp"
        "${SMALLCPP_RUNTIME_SCPP_STAMP}"
    VERBATIM)
add_custom_target(scpp_lsp ALL DEPENDS "${SMALLCPP_LSP_BIN}")
set_source_files_properties("${SMALLCPP_LSP_BIN}" PROPERTIES GENERATED TRUE)
install(PROGRAMS "${SMALLCPP_LSP_BIN}" DESTINATION "${CMAKE_INSTALL_BINDIR}")

add_subdirectory(tests)

# CMake reserves the target name "test" (CMP0037). The built-in
# target can depend only on all, so an excluded driver stays out of
# `build test`. OLD keeps this project target, and generation leaves
# it in place. It depends on the drivers and on every target the
# default build includes, then runs CTest.
function(attach_test_drivers)
    set(CMAKE_WARN_DEPRECATED OFF)
    cmake_policy(SET CMP0037 OLD)
    add_custom_target(test
        COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process $(ARGS)
        USES_TERMINAL)

    set(pending "${CMAKE_SOURCE_DIR}")
    set(ordinary "")
    while(pending)
        list(POP_FRONT pending dir)
        get_property(targets DIRECTORY "${dir}"
            PROPERTY BUILDSYSTEM_TARGETS)
        get_property(subs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
        list(APPEND pending ${subs})
        list(APPEND ordinary ${targets})
    endwhile()

    foreach(name IN LISTS ordinary)
        if(name STREQUAL "test")
            continue()
        endif()
        get_target_property(excluded ${name} EXCLUDE_FROM_ALL)
        if(excluded)
            continue()
        endif()
        add_dependencies(test ${name})
    endforeach()
    foreach(name IN LISTS ARGN)
        if(NOT TARGET ${name})
            message(FATAL_ERROR
                "Test driver '${name}' is not a target")
        endif()
        add_dependencies(test ${name})
    endforeach()
endfunction()

attach_test_drivers(
    command_line_test
    prefix_test
    parse_test
    verify_module
    lsp_test)
