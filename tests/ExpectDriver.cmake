if(NOT DEFINED CHECK OR NOT DEFINED COMPILER OR NOT DEFINED CLANG OR NOT DEFINED WORK
        OR NOT DEFINED SOURCE_DIR OR NOT DEFINED RETURN_ZERO)
    message(FATAL_ERROR "driver check is missing a path")
endif()

file(MAKE_DIRECTORY "${WORK}")

function(run_scpp)
    execute_process(
        COMMAND "${COMPILER}" ${ARGN}
        WORKING_DIRECTORY "${WORK}"
        RESULT_VARIABLE status
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "scpp failed (${status}) for ${CHECK}\n${out}${err}")
    endif()
endfunction()

function(expect_status program expected)
    execute_process(
        COMMAND "${program}"
        RESULT_VARIABLE status)
    if(NOT status EQUAL "${expected}")
        message(FATAL_ERROR "${program} exited ${status}, expected ${expected}")
    endif()
endfunction()

function(require_file path)
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "missing ${path}")
    endif()
endfunction()

function(require_absent path)
    if(EXISTS "${path}")
        message(FATAL_ERROR "unexpected ${path}")
    endif()
endfunction()

function(expect_bitcode path)
    execute_process(
        COMMAND python3 -c "import pathlib,sys; b=pathlib.Path(sys.argv[1]).read_bytes()[:4]; sys.exit(0 if b==bytes([0x42,0x43,0xc0,0xde]) else 1)"
            "${path}"
        RESULT_VARIABLE status)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "${path} is not LLVM bitcode")
    endif()
endfunction()

function(make_lib dir source name)
    file(MAKE_DIRECTORY "${dir}")
    execute_process(
        COMMAND "${CLANG}" -x c -c -o "${dir}/${name}.o" "${source}"
        RESULT_VARIABLE status
        ERROR_VARIABLE err)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "clang did not compile ${source}\n${err}")
    endif()
    execute_process(
        COMMAND ar rcs "${dir}/lib${name}.a" "${dir}/${name}.o"
        RESULT_VARIABLE status
        ERROR_VARIABLE err)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "ar did not write lib${name}.a\n${err}")
    endif()
endfunction()

if(CHECK STREQUAL "default")
    run_scpp(-i "${RETURN_ZERO}")
    require_file("${WORK}/a.out")
    require_absent("${WORK}/a.out.i")
    require_absent("${WORK}/a.out.o")
    expect_status("${WORK}/a.out" 0)
elseif(CHECK STREQUAL "keep")
    run_scpp(-i -o "${WORK}/dropped" "${RETURN_ZERO}")
    require_file("${WORK}/dropped")
    require_absent("${WORK}/dropped.i")
    require_absent("${WORK}/dropped.o")
    run_scpp(-i -k -o "${WORK}/kept" "${RETURN_ZERO}")
    require_file("${WORK}/kept")
    require_file("${WORK}/kept.i")
    require_file("${WORK}/kept.o")
    run_scpp(-i --keep -o "${WORK}/kept-long" "${RETURN_ZERO}")
    require_file("${WORK}/kept-long")
    require_file("${WORK}/kept-long.i")
    require_file("${WORK}/kept-long.o")
elseif(CHECK STREQUAL "preprocess")
    run_scpp(-i -E "${SOURCE_DIR}/emit-left.scpp" "${SOURCE_DIR}/emit-right.scpp")
    require_file("${WORK}/emit-left.i")
    require_file("${WORK}/emit-right.i")
    require_absent("${WORK}/emit-left.o")
    require_absent("${WORK}/emit-right.o")
    require_absent("${WORK}/emit-left.bc")
    require_absent("${WORK}/emit-right.bc")
    file(READ "${WORK}/emit-left.i" left)
    file(READ "${WORK}/emit-right.i" right)
    string(FIND "${left}" "int main" left_at)
    string(FIND "${right}" "int main" right_at)
    if(left_at EQUAL -1 OR right_at EQUAL -1)
        message(FATAL_ERROR "preprocessor output is missing main")
    endif()
elseif(CHECK STREQUAL "emit-llvm")
    run_scpp(-i --emit-llvm "${SOURCE_DIR}/emit-left.scpp" "${SOURCE_DIR}/emit-right.scpp")
    expect_bitcode("${WORK}/emit-left.bc")
    expect_bitcode("${WORK}/emit-right.bc")
    require_absent("${WORK}/emit-left.i")
    require_absent("${WORK}/emit-right.i")
    require_absent("${WORK}/emit-left.o")
    require_absent("${WORK}/emit-right.o")
    require_absent("${WORK}/a.out")
elseif(CHECK STREQUAL "library")
    make_lib("${WORK}/libs" "${SOURCE_DIR}/extra-eleven.c" scppa)
    make_lib("${WORK}/libs" "${SOURCE_DIR}/more-one.c" scppb)
    run_scpp(-i -o "${WORK}/libs-prog" -L "${WORK}/libs" -l scppa -l scppb "${SOURCE_DIR}/libs.scpp")
    expect_status("${WORK}/libs-prog" 12)
elseif(CHECK STREQUAL "include")
    run_scpp(-i -o "${WORK}/picked-first" "-I${SOURCE_DIR}/inc-first:${SOURCE_DIR}/inc-second"
        "${SOURCE_DIR}/picked.scpp")
    expect_status("${WORK}/picked-first" 11)
    run_scpp(-i -o "${WORK}/picked-second" "-I${SOURCE_DIR}/inc-second:${SOURCE_DIR}/inc-first"
        "${SOURCE_DIR}/picked.scpp")
    expect_status("${WORK}/picked-second" 22)
elseif(CHECK STREQUAL "library-order")
    make_lib("${WORK}/lib-first" "${SOURCE_DIR}/extra-eleven.c" scppa)
    make_lib("${WORK}/lib-second" "${SOURCE_DIR}/extra-twenty.c" scppa)
    run_scpp(-i -o "${WORK}/order-first" "-L${WORK}/lib-first:${WORK}/lib-second" -l scppa
        "${SOURCE_DIR}/only-a.scpp")
    expect_status("${WORK}/order-first" 11)
    run_scpp(-i -o "${WORK}/order-second" "-L${WORK}/lib-second:${WORK}/lib-first" -l scppa
        "${SOURCE_DIR}/only-a.scpp")
    expect_status("${WORK}/order-second" 22)
else()
    message(FATAL_ERROR "unknown driver check ${CHECK}")
endif()
