# Runtime evidence. Each entry runs one short program.
function(add_exec_status name source expect)
    string(REGEX REPLACE "^out-" "" feature "${source}")
    add_test(
        NAME exec_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCOMPILER=$<TARGET_FILE:scpp>
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/${source}.scpp
            -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/exec-${name}
            -DEXPECT=${expect}
            "-DEXPECT_OUT=${feature} ${expect}"
            -DEXPECT_NL=1
            -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
endfunction()

function(add_exec_line name source expect line)
    add_test(
        NAME exec_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCOMPILER=$<TARGET_FILE:scpp>
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/${source}.scpp
            -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/exec-${name}
            -DEXPECT=${expect}
            "-DEXPECT_OUT=${line}"
            -DEXPECT_NL=1
            -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
endfunction()

add_exec_status(exec-if exec-if 2)
add_exec_status(exec-else exec-else 3)
add_exec_status(exec-while exec-while 3)
add_exec_status(exec-do exec-do 1)
add_exec_status(exec-for exec-for 6)
add_exec_status(exec-for-decl exec-for-decl 6)
add_exec_status(exec-switch exec-switch 7)
add_exec_status(exec-break exec-break 4)
add_exec_status(exec-continue exec-continue 5)
add_exec_status(exec-goto exec-goto 5)
add_exec_status(exec-void exec-void 4)
add_exec_status(exec-bool exec-bool 1)
add_exec_status(exec-false exec-false 1)
add_exec_status(exec-char exec-char 65)
add_exec_status(exec-short exec-short 12)
add_exec_status(exec-long exec-long 9)
add_exec_status(exec-float exec-float 1)
add_exec_status(exec-double exec-double 1)
add_exec_status(exec-signed exec-signed 1)
add_exec_status(exec-unsigned exec-unsigned 1)
add_exec_status(exec-literal exec-literal 42)
add_exec_status(exec-cast exec-cast 65)
add_exec_status(exec-conditional exec-conditional 2)
add_exec_status(exec-add exec-add 5)
add_exec_status(exec-sub exec-sub 5)
add_exec_status(exec-mul exec-mul 12)
add_exec_status(exec-div exec-div 3)
add_exec_status(exec-mod exec-mod 1)
add_exec_status(exec-shift exec-shift 8)
add_exec_status(exec-shr exec-shr 4)
add_exec_status(exec-lt exec-lt 1)
add_exec_status(exec-gt exec-gt 1)
add_exec_status(exec-le exec-le 1)
add_exec_status(exec-ge exec-ge 1)
add_exec_status(exec-eq exec-eq 1)
add_exec_status(exec-ne exec-ne 1)
add_exec_status(exec-bit-and exec-bit-and 2)
add_exec_status(exec-bit-or exec-bit-or 3)
add_exec_status(exec-bit-xor exec-bit-xor 2)
add_exec_status(exec-complement exec-complement 1)
add_exec_status(exec-negate exec-negate 1)
add_exec_status(exec-not exec-not 1)
add_exec_status(exec-logical-and exec-logical-and 1)
add_exec_status(exec-logical-or exec-logical-or 1)
add_exec_status(exec-assign exec-assign 6)
add_exec_status(exec-add-assign exec-add-assign 5)
add_exec_status(exec-sub-assign exec-sub-assign 5)
add_exec_status(exec-mul-assign exec-mul-assign 12)
add_exec_status(exec-div-assign exec-div-assign 4)
add_exec_status(exec-mod-assign exec-mod-assign 1)
add_exec_status(exec-shl-assign exec-shl-assign 8)
add_exec_status(exec-shr-assign exec-shr-assign 4)
add_exec_status(exec-and-assign exec-and-assign 2)
add_exec_status(exec-or-assign exec-or-assign 3)
add_exec_status(exec-xor-assign exec-xor-assign 2)
add_exec_status(exec-post-inc exec-post-inc 9)
add_exec_status(exec-pre-inc exec-pre-inc 5)
add_exec_status(exec-post-dec exec-post-dec 7)
add_exec_status(exec-pre-dec exec-pre-dec 3)
add_exec_status(exec-parameter exec-parameter 7)
add_exec_status(exec-string exec-string 1)
add_exec_status(exec-global exec-global 6)
add_exec_status(exec-empty-params exec-empty-params 7)
add_exec_status(exec-namespace exec-namespace 9)
add_exec_status(exec-method exec-method 6)
add_exec_status(exec-const-method exec-const-method 8)
add_exec_status(exec-ctor exec-ctor 4)
add_exec_status(exec-init-list exec-init-list 7)
add_exec_status(exec-two-bases exec-two-bases 6)
add_exec_status(exec-virtual exec-virtual 4)
add_exec_status(exec-override exec-override 6)
add_exec_status(exec-base-virtual exec-base-virtual 7)
add_exec_status(exec-multi-virtual exec-multi-virtual 7)
add_exec_status(exec-virtual-base exec-virtual-base 5)
add_exec_status(exec-pure-virtual exec-pure-virtual 8)
add_exec_status(exec-virtual-dtor exec-virtual-dtor 3)
add_exec_status(exec-virtual-collect exec-virtual-collect 9)
add_exec_status(exec-base-call exec-base-call 1)
add_exec_status(exec-new-array exec-new-array 5)
add_exec_status(exec-new-pointer exec-new-pointer 3)
add_exec_status(exec-delete exec-delete 9)
add_exec_status(exec-format-text exec-format-text 1)
add_exec_status(call out-call 2)
add_exec_status(overload-call out-overload-call 3)
add_exec_status(default-call out-default-call 7)
add_exec_status(varargs out-varargs 1)
add_exec_status(static out-static 5)
add_exec_status(inline out-inline 6)
add_exec_status(typedef out-typedef 6)
add_exec_status(register out-register 4)
add_exec_status(volatile out-volatile 1)
add_exec_status(const-var out-const-var 3)
add_exec_status(local out-local 4)
add_exec_status(global out-global 0)
add_exec_status(comma out-comma 2)
add_exec_status(array out-array 4)
add_exec_status(pointer out-pointer 5)
add_exec_status(array-param out-array-param 6)
add_exec_status(bitfield out-bitfield 3)
add_exec_status(designated-init out-designated-init 2)
add_exec_status(brace out-brace 1)
add_exec_status(enum out-enum 4)
add_exec_status(union out-union 3)
add_exec_status(using-name out-using-name 2)
add_exec_status(using-namespace out-using-namespace 2)
add_exec_status(qualified-call out-qualified-call 2)
add_exec_status(nullptr out-nullptr 1)
add_exec_status(struct out-struct 1)
add_exec_status(sizeof out-sizeof 4)
add_exec_status(empty-list out-empty-list 1)
add_exec_line(exception-message exception-message 0 "exception-message missing")
add_exec_status(exception-blank exception-blank 0)
add_exec_status(exec-catch exec-catch 7)
add_exec_status(exec-catch-all exec-catch-all 6)
add_exec_status(exec-rethrow exec-rethrow 4)
add_exec_status(exec-dtor-call exec-dtor-call 4)
add_exec_status(exec-delete-dtor exec-delete-dtor 5)
add_exec_status(exec-collect-dtor exec-collect-dtor 8)

add_exec_line(varargs-counts varargs-counts 0 "varargs-counts 4 5 6")
add_exec_line(varargs-types varargs-types 0 "varargs-types 7 8 9")
add_exec_line(overload-both overload-both 0 "overload-both 1 3")
add_exec_line(overload-type overload-type 0 "overload-type 3 8")
add_exec_line(overload-pointer overload-pointer 0 "overload-pointer 1 2")
add_exec_line(inherit-field inherit-field 0 "inherit-field 4")
add_exec_line(inherit-chain inherit-chain 0 "inherit-chain 5")
add_exec_line(inherit-hide inherit-hide 0 "inherit-hide 3")
add_exec_line(virtual-through-base virtual-through-base 0 "virtual-through-base 6")
add_exec_line(virtual-base-body virtual-base-body 0 "virtual-base-body 4 4")
add_exec_line(default-both default-both 0 "default-both 7 5")
add_exec_line(short-circuit-and short-circuit-and 0 "short-circuit-and 0 0")
add_exec_line(short-circuit-or short-circuit-or 0 "short-circuit-or 1 0")
add_exec_line(this-field this-field 0 "this-field 4")
add_exec_line(libc libc 0 "libc 5 12")
add_exec_line(typeid typeid 0 "typeid 1 1 1")
add_exec_line(string-grow string-grow 0 "string 11 16 hello world")
add_exec_line(neg-index neg-index 0 "neg-index b a 30 20")
add_exec_line(vector-int vector-int 0 "vector 5 10 20 30 40 50")
add_exec_line(list-int list-int 0 "list 4 5 6 5")
add_exec_line(tree-int tree-int 0 "tree 2 1 2 3")
add_exec_line(hash-int hash-int 0 "hash 3 11 30")
add_exec_line(vector-edit vector-edit 0 "vector-edit 1 -1 5 15 4 40 2")
add_exec_line(list-edit list-edit 0 "list-edit 1 -1 4 7 4 8 5")
add_exec_line(tree-edit tree-edit 0 "tree-edit 2 -1 2 4 4 9 3")
add_exec_line(hash-edit hash-edit 0 "hash-edit 3 0 1 3 30 4 40 2 25 0")

add_test(
    NAME exec_exec-print-two
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/exec-print-two.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/exec-exec-print-two
        -DEXPECT=0
        "-DEXPECT_OUT=2 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)

function(add_diag name needle)
    set(command
        ${CMAKE_COMMAND}
            -DCOMPILER=$<TARGET_FILE:scpp>
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/${name}.scpp
            -DBITCODE=${CMAKE_CURRENT_BINARY_DIR}/${name}.bc
            "-DNEEDLE=${needle}")
    if(ARGC GREATER 2)
        list(GET ARGV 2 also)
        list(APPEND command "-DALSO=${also}")
    endif()
    list(APPEND command -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectDiag.cmake)
    add_test(NAME diag_${name} COMMAND ${command})
endfunction()

add_test(
    NAME diag_bad-recover
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/bad-recover.scpp
        -DBITCODE=${CMAKE_CURRENT_BINARY_DIR}/bad-recover.bc
        "-DNEEDLE=syntax error, unexpected ';'"
        -DCOUNT=2
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectDiag.cmake)
add_diag(bad-delete "delete operand is not a pointer")
add_diag(bad-unknown "unknown name: missing")
add_diag(bad-type "cannot convert void")
add_diag(bad-const-assign "assignment to const")
add_diag(bad-member "no such member: z")
add_diag(bad-divzero "division by zero")
add_diag(bad-sizeof-void "sizeof void")
add_diag(bad-long-double "long double is not in this phase")
add_diag(bad-throw "bare throw is not in a handler")
add_diag(bad-namespace "unknown namespace: missing")
add_diag(bad-dtor-name "destructor name does not match the class")
add_diag(bad-ctor-name "constructor name does not match the class")
add_diag(bad-format "not enough arguments for format string")
add_diag(bad-string "unterminated string")
add_diag(bad-comment "unterminated comment")
add_diag(bad-char "unterminated character constant")
add_diag(bad-character "invalid character '@'")
add_diag(bad-array-return "function returns an array")
add_diag(bad-empty-struct "syntax error, unexpected '}'")
add_diag(bad-unnamed-bit "syntax error, unexpected ':'")
add_diag(bad-redefine "name is already defined: n")
add_test(
    NAME diag_bad-unknown-call
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/bad-unknown-call.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/bad-unknown-call
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectUndefined.cmake)
add_diag(bad-no-match "no matching function: f")
add_diag(bad-ambiguous "ambiguous call: f")
add_diag(bad-this "this is not allowed here")
add_diag(bad-auto "declaration has no type")
add_diag(bad-shift-float "shift of a float")
add_diag(bad-complement-float "complement of a float")
add_diag(bad-bitwise-float "bitwise operator on a float")
add_diag(bad-access "cannot access")
add_diag(bad-const-method "const method")
add_diag(bad-no-ctor "no matching constructor")
add_diag(bad-unknown-type "incomplete type")
add_diag(bad-too-many "too many initializers")
add_diag(bad-brace-class "brace initialization of a class is not in this phase")
add_diag(bad-default "default arguments must be trailing")
add_diag(bad-void-param "void parameter")
add_diag(bad-escape "bad string escape")
add_diag(bad-wide "wide strings are not in this phase")
add_diag(bad-not-pointer "not a pointer")
add_diag(bad-subscript "not an array or pointer")
add_diag(bad-address-bit "address of a bit-field")
add_diag(bad-duplicate-const "duplicate const")
add_diag(bad-using "unknown namespace: missing")
add_diag(bad-not-function "not a function: n")
add_diag(bad-incompatible "incompatible return type: f")
add_diag(bad-array-size "not a constant integer")
add_diag(bad-nested "nested designators are not in this phase")
add_diag(bad-namespace-value "namespace is not a value")
add_diag(bad-operands "invalid operands")
add_diag(bad-call-constant "call is not constant")
add_diag(bad-abstract "abstract class")
add_diag(bad-verbose-syntax "int x = ;" "expected IDENTIFIER")
add_diag(bad-verbose-semantic "return missing;")
