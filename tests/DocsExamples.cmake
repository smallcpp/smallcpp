# Examples named by the reader documents.
# Each running program prints one line. A fragment compiles with -c.
add_test(
    NAME doc_ds-array
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/ds-array.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-ds-array
        -DEXPECT=0
        "-DEXPECT_OUT=array 2 h i"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_ds-hash-equal
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/ds-hash-equal.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-ds-hash-equal
        -DEXPECT=0
        "-DEXPECT_OUT=hash-equal 1 30 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_ds-hash
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/ds-hash.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-ds-hash
        -DEXPECT=0
        "-DEXPECT_OUT=hash 12 1 1 12"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_ds-list
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/ds-list.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-ds-list
        -DEXPECT=0
        "-DEXPECT_OUT=list 3 -1 8 9 2"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_ds-string-build
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/ds-string-build.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-ds-string-build
        -DEXPECT=0
        "-DEXPECT_OUT=string-build ZxPQabcdeZ ZxPQabcdeZ 10 Z 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_ds-string-find
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/ds-string-find.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-ds-string-find
        -DEXPECT=0
        "-DEXPECT_OUT=string-find 3 1 1 3 0 Ad"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_ds-string-split
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/ds-string-split.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-ds-string-split
        -DEXPECT=0
        "-DEXPECT_OUT=string-split 3 a-b-c a b a"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_ds-tree
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/ds-tree.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-ds-tree
        -DEXPECT=0
        "-DEXPECT_OUT=tree 9 9 3 3 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_ds-vector
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/ds-vector.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-ds-vector
        -DEXPECT=0
        "-DEXPECT_OUT=vector 0 10 20 40 2"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_gtk-alert-present
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-alert-present.scpp
        -DOBJECT=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-alert-present.o
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCompileObject.cmake)
add_test(
    NAME doc_gtk-alert
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-alert.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-alert
        "-DEXPECT_OUT=gtk-alert 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-alert PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-app-run
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-app-run.scpp
        -DOBJECT=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-app-run.o
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCompileObject.cmake)
add_test(
    NAME doc_gtk-app
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-app.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-app
        "-DEXPECT_OUT=gtk-app 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-app PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-box
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-box.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-box
        "-DEXPECT_OUT=gtk-box 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-box PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-button
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-button.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-button
        "-DEXPECT_OUT=gtk-button 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-button PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-check
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-check.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-check
        "-DEXPECT_OUT=gtk-check 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-check PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-column
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-column.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-column
        "-DEXPECT_OUT=gtk-column 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-column PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-dialog
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-dialog.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-dialog
        "-DEXPECT_OUT=gtk-dialog 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-dialog PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-dropdown
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-dropdown.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-dropdown
        "-DEXPECT_OUT=gtk-dropdown 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-dropdown PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-entry
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-entry.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-entry
        "-DEXPECT_OUT=gtk-entry 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-entry PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-file-open
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-file-open.scpp
        -DOBJECT=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-file-open.o
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCompileObject.cmake)
add_test(
    NAME doc_gtk-file-save
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-file-save.scpp
        -DOBJECT=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-file-save.o
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCompileObject.cmake)
add_test(
    NAME doc_gtk-file
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-file.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-file
        "-DEXPECT_OUT=gtk-file 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-file PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-header
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-header.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-header
        "-DEXPECT_OUT=gtk-header 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-header PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-label
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-label.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-label
        "-DEXPECT_OUT=gtk-label 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-label PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-menu
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-menu.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-menu
        "-DEXPECT_OUT=gtk-menu 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-menu PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-notebook
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-notebook.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-notebook
        "-DEXPECT_OUT=gtk-notebook 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-notebook PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-paned
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-paned.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-paned
        "-DEXPECT_OUT=gtk-paned 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-paned PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-picture
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-picture.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-picture
        "-DEXPECT_OUT=gtk-picture 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-picture PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-popover-popdown
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-popover-popdown.scpp
        -DOBJECT=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-popover-popdown.o
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCompileObject.cmake)
add_test(
    NAME doc_gtk-popover-popup
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-popover-popup.scpp
        -DOBJECT=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-popover-popup.o
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCompileObject.cmake)
add_test(
    NAME doc_gtk-popover
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-popover.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-popover
        "-DEXPECT_OUT=gtk-popover 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-popover PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-progress
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-progress.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-progress
        "-DEXPECT_OUT=gtk-progress 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-progress PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-scrolled
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-scrolled.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-scrolled
        "-DEXPECT_OUT=gtk-scrolled 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-scrolled PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-signal
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-signal.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-signal
        "-DEXPECT_OUT=gtk-signal 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-signal PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-spin
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-spin.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-spin
        "-DEXPECT_OUT=gtk-spin 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-spin PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-spinner
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-spinner.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-spinner
        "-DEXPECT_OUT=gtk-spinner 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-spinner PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-text
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-text.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-text
        "-DEXPECT_OUT=gtk-text 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-text PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-widget
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-widget.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-widget
        "-DEXPECT_OUT=gtk-widget 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-widget PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_gtk-window
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/gtk-window.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-gtk-window
        "-DEXPECT_OUT=gtk-window 1"
        "-DLIBS=${SMALLCPP_GTK_LIBS}"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectGtk.cmake)
set_tests_properties(doc_gtk-window PROPERTIES
    SKIP_REGULAR_EXPRESSION "SKIP gtk display"
    SKIP_RETURN_CODE 77)
add_test(
    NAME doc_lang-array
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-array.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-array
        -DEXPECT=0
        "-DEXPECT_OUT=lang-array 5"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-assign
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-assign.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-assign
        -DEXPECT=0
        "-DEXPECT_OUT=lang-assign 2"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-auto
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-auto.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-auto
        -DEXPECT=0
        "-DEXPECT_OUT=lang-auto 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-bases
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-bases.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-bases
        -DEXPECT=0
        "-DEXPECT_OUT=lang-bases 6"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-bitfield
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-bitfield.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-bitfield
        -DEXPECT=0
        "-DEXPECT_OUT=lang-bitfield 5"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-brace
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-brace.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-brace
        -DEXPECT=0
        "-DEXPECT_OUT=lang-brace 2 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-break
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-break.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-break
        -DEXPECT=0
        "-DEXPECT_OUT=lang-break 6"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-call
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-call.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-call
        -DEXPECT=0
        "-DEXPECT_OUT=lang-call 5"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-cast
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-cast.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-cast
        -DEXPECT=0
        "-DEXPECT_OUT=lang-cast 65"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-catch-all
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-catch-all.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-catch-all
        -DEXPECT=0
        "-DEXPECT_OUT=lang-catch-all 6"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-class
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-class.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-class
        -DEXPECT=0
        "-DEXPECT_OUT=lang-class 6"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-comma
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-comma.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-comma
        -DEXPECT=0
        "-DEXPECT_OUT=lang-comma 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-conditional
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-conditional.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-conditional
        -DEXPECT=0
        "-DEXPECT_OUT=lang-conditional 2"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-const-method
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-const-method.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-const-method
        -DEXPECT=0
        "-DEXPECT_OUT=lang-const-method 8"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-const
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-const.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-const
        -DEXPECT=0
        "-DEXPECT_OUT=lang-const 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-continue
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-continue.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-continue
        -DEXPECT=0
        "-DEXPECT_OUT=lang-continue 8"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-default
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-default.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-default
        -DEXPECT=0
        "-DEXPECT_OUT=lang-default 7 5"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-delete
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-delete.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-delete
        -DEXPECT=0
        "-DEXPECT_OUT=lang-delete 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-designated
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-designated.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-designated
        -DEXPECT=0
        "-DEXPECT_OUT=lang-designated 1 2"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-do
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-do.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-do
        -DEXPECT=0
        "-DEXPECT_OUT=lang-do 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-dtor
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-dtor.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-dtor
        -DEXPECT=0
        "-DEXPECT_OUT=lang-dtor 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-empty
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-empty.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-empty
        -DEXPECT=0
        "-DEXPECT_OUT=lang-empty 7"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-enum
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-enum.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-enum
        -DEXPECT=0
        "-DEXPECT_OUT=lang-enum 4"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-extern
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-extern.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-extern
        -DEXPECT=0
        "-DEXPECT_OUT=lang-extern 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-fn-array
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-fn-array.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-fn-array
        -DEXPECT=0
        "-DEXPECT_OUT=lang-fn-array 7 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-fn-ptr
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-fn-ptr.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-fn-ptr
        -DEXPECT=0
        "-DEXPECT_OUT=lang-fn-ptr 7"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-for-decl
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-for-decl.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-for-decl
        -DEXPECT=0
        "-DEXPECT_OUT=lang-for-decl 6"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-for
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-for.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-for
        -DEXPECT=0
        "-DEXPECT_OUT=lang-for 6"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-format
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-format.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-format
        -DEXPECT=0
        "-DEXPECT_OUT=lang-format n 4"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-global
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-global.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-global
        -DEXPECT=0
        "-DEXPECT_OUT=lang-global 6"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-goto
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-goto.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-goto
        -DEXPECT=0
        "-DEXPECT_OUT=lang-goto 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-if
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-if.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-if
        -DEXPECT=0
        "-DEXPECT_OUT=lang-if 2"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-inc
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-inc.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-inc
        -DEXPECT=0
        "-DEXPECT_OUT=lang-inc 4 6 6 4"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-init-list
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-init-list.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-init-list
        -DEXPECT=0
        "-DEXPECT_OUT=lang-init-list 7"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-inline
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-inline.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-inline
        -DEXPECT=0
        "-DEXPECT_OUT=lang-inline 6"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-main
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-main.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-main
        -DEXPECT=0
        "-DEXPECT_OUT=lang-main 0"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-namespace
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-namespace.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-namespace
        -DEXPECT=0
        "-DEXPECT_OUT=lang-namespace 9"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-new
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-new.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-new
        -DEXPECT=0
        "-DEXPECT_OUT=lang-new 5 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-null
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-null.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-null
        -DEXPECT=0
        "-DEXPECT_OUT=lang-null 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-ops
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-ops.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-ops
        -DEXPECT=0
        "-DEXPECT_OUT=lang-ops 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-overload
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-overload.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-overload
        -DEXPECT=0
        "-DEXPECT_OUT=lang-overload 3 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-pointer
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-pointer.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-pointer
        -DEXPECT=0
        "-DEXPECT_OUT=lang-pointer 6"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-print
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-print.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-print
        -DEXPECT=0
        "-DEXPECT_OUT=lang-print 2"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-protected
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-protected.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-protected
        -DEXPECT=0
        "-DEXPECT_OUT=lang-protected 4"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-pure
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-pure.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-pure
        -DEXPECT=0
        "-DEXPECT_OUT=lang-pure 8"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-register
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-register.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-register
        -DEXPECT=0
        "-DEXPECT_OUT=lang-register 4"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-rethrow
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-rethrow.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-rethrow
        -DEXPECT=0
        "-DEXPECT_OUT=lang-rethrow 4"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-short-circuit
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-short-circuit.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-short-circuit
        -DEXPECT=0
        "-DEXPECT_OUT=lang-short-circuit 0 0 1 0"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-sizeof
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-sizeof.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-sizeof
        -DEXPECT=0
        "-DEXPECT_OUT=lang-sizeof 4"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-static
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-static.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-static
        -DEXPECT=0
        "-DEXPECT_OUT=lang-static 5"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-string
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-string.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-string
        -DEXPECT=0
        "-DEXPECT_OUT=lang-string hello"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-struct
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-struct.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-struct
        -DEXPECT=0
        "-DEXPECT_OUT=lang-struct 7"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-switch
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-switch.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-switch
        -DEXPECT=0
        "-DEXPECT_OUT=lang-switch 7"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-this
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-this.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-this
        -DEXPECT=0
        "-DEXPECT_OUT=lang-this 4"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-throw
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-throw.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-throw
        -DEXPECT=0
        "-DEXPECT_OUT=lang-throw missing"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-typedef
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-typedef.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-typedef
        -DEXPECT=0
        "-DEXPECT_OUT=lang-typedef 6"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-typeid
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-typeid.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-typeid
        -DEXPECT=0
        "-DEXPECT_OUT=lang-typeid 1 1 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-types
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-types.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-types
        -DEXPECT=0
        "-DEXPECT_OUT=lang-types 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-union
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-union.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-union
        -DEXPECT=0
        "-DEXPECT_OUT=lang-union 65"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-using-namespace
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-using-namespace.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-using-namespace
        -DEXPECT=0
        "-DEXPECT_OUT=lang-using-namespace 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-using
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-using.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-using
        -DEXPECT=0
        "-DEXPECT_OUT=lang-using 2"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-varargs
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-varargs.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-varargs
        -DEXPECT=0
        "-DEXPECT_OUT=lang-varargs 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-virtual-base
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-virtual-base.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-virtual-base
        -DEXPECT=0
        "-DEXPECT_OUT=lang-virtual-base 5"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-virtual-dtor
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-virtual-dtor.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-virtual-dtor
        -DEXPECT=0
        "-DEXPECT_OUT=lang-virtual-dtor 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-virtual
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-virtual.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-virtual
        -DEXPECT=0
        "-DEXPECT_OUT=lang-virtual 4"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-void
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-void.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-void
        -DEXPECT=0
        "-DEXPECT_OUT=lang-void 4"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-volatile
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-volatile.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-volatile
        -DEXPECT=0
        "-DEXPECT_OUT=lang-volatile 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_lang-while
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/lang-while.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-lang-while
        -DEXPECT=0
        "-DEXPECT_OUT=lang-while 3"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-abort
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-abort.scpp
        -DOBJECT=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-abort.o
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCompileObject.cmake)
add_test(
    NAME doc_rt-accept
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-accept.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-accept
        -DEXPECT=0
        "-DEXPECT_OUT=rt-accept 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-access
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-access.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-access
        -DEXPECT=0
        "-DEXPECT_OUT=rt-access 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-acos
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-acos.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-acos
        -DEXPECT=0
        "-DEXPECT_OUT=rt-acos 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-acosf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-acosf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-acosf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-acosf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-asin
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-asin.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-asin
        -DEXPECT=0
        "-DEXPECT_OUT=rt-asin 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-asinf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-asinf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-asinf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-asinf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-atan
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-atan.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-atan
        -DEXPECT=0
        "-DEXPECT_OUT=rt-atan 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-atan2
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-atan2.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-atan2
        -DEXPECT=0
        "-DEXPECT_OUT=rt-atan2 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-atan2f
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-atan2f.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-atan2f
        -DEXPECT=0
        "-DEXPECT_OUT=rt-atan2f 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-atanf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-atanf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-atanf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-atanf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-atoi
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-atoi.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-atoi
        -DEXPECT=0
        "-DEXPECT_OUT=rt-atoi 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-atol
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-atol.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-atol
        -DEXPECT=0
        "-DEXPECT_OUT=rt-atol 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-basename
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-basename.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-basename
        -DEXPECT=0
        "-DEXPECT_OUT=rt-basename 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-bind
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-bind.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-bind
        -DEXPECT=0
        "-DEXPECT_OUT=rt-bind 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cbrt
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cbrt.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cbrt
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cbrt 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cbrtf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cbrtf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cbrtf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cbrtf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-ceil
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-ceil.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-ceil
        -DEXPECT=0
        "-DEXPECT_OUT=rt-ceil 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-ceilf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-ceilf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-ceilf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-ceilf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-chdir
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-chdir.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-chdir
        -DEXPECT=0
        "-DEXPECT_OUT=rt-chdir 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-clearerr
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-clearerr.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-clearerr
        -DEXPECT=0
        "-DEXPECT_OUT=rt-clearerr 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-action
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-action.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-action
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-action 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-add-option
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-add-option.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-add-option
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-add-option 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-add-positional
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-add-positional.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-add-positional
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-add-positional 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-capture
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-capture.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-capture
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-capture 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-command
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-command.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-command
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-command 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-contains
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-contains.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-contains
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-contains 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-count
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-count.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-count
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-count 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-has-value
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-has-value.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-has-value
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-has-value 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-help
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-help.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-help
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-help 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-option
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-option.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-option
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-option 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-parse
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-parse.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-parse
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-parse 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-positional-at
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-positional-at.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-positional-at
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-positional-at 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-positional-count
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-positional-count.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-positional-count
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-positional-count 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-positional
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-positional.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-positional
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-positional 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-remainder-at
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-remainder-at.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-remainder-at
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-remainder-at 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-remainder-count
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-remainder-count.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-remainder-count
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-remainder-count 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-switch
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-switch.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-switch
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-switch 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-value-at
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-value-at.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-value-at
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-value-at 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-value-count
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-value-count.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-value-count
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-value-count 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-value
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-value.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-value
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-value 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-version
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-version.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-version
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-version 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cli-was-provided
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cli-was-provided.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cli-was-provided
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cli-was-provided 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-clock
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-clock.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-clock
        -DEXPECT=0
        "-DEXPECT_OUT=rt-clock 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-close
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-close.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-close
        -DEXPECT=0
        "-DEXPECT_OUT=rt-close 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-closedir
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-closedir.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-closedir
        -DEXPECT=0
        "-DEXPECT_OUT=rt-closedir 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cond-broadcast
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cond-broadcast.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cond-broadcast
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cond-broadcast 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cond-destroy
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cond-destroy.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cond-destroy
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cond-destroy 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cond-init
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cond-init.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cond-init
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cond-init 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cond-signal
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cond-signal.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cond-signal
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cond-signal 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cond-wait
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cond-wait.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cond-wait
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cond-wait 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-connect
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-connect.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-connect
        -DEXPECT=0
        "-DEXPECT_OUT=rt-connect 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-copy-file-range
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-copy-file-range.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-copy-file-range
        -DEXPECT=0
        "-DEXPECT_OUT=rt-copy-file-range 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cos
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cos.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cos
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cos 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cosf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cosf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cosf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cosf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-cosh
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-cosh.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-cosh
        -DEXPECT=0
        "-DEXPECT_OUT=rt-cosh 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-coshf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-coshf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-coshf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-coshf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-dirname
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-dirname.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-dirname
        -DEXPECT=0
        "-DEXPECT_OUT=rt-dirname 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-errno-location
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-errno-location.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-errno-location
        -DEXPECT=0
        "-DEXPECT_OUT=rt-errno-location 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-errno
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-errno.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-errno
        -DEXPECT=0
        "-DEXPECT_OUT=rt-errno 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-exception
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-exception.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-exception
        -DEXPECT=0
        "-DEXPECT_OUT=rt-exception 1 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-exit
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-exit.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-exit
        -DEXPECT=0
        "-DEXPECT_OUT=rt-exit 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-exp
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-exp.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-exp
        -DEXPECT=0
        "-DEXPECT_OUT=rt-exp 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-expf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-expf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-expf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-expf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fabs
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fabs.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fabs
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fabs 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fabsf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fabsf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fabsf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fabsf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fclose
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fclose.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fclose
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fclose 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-feof
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-feof.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-feof
        -DEXPECT=0
        "-DEXPECT_OUT=rt-feof 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-ferror
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-ferror.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-ferror
        -DEXPECT=0
        "-DEXPECT_OUT=rt-ferror 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fflush
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fflush.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fflush
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fflush 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fgets
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fgets.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fgets
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fgets 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-floor
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-floor.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-floor
        -DEXPECT=0
        "-DEXPECT_OUT=rt-floor 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-floorf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-floorf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-floorf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-floorf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fmax
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fmax.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fmax
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fmax 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fmaxf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fmaxf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fmaxf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fmaxf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fmin
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fmin.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fmin
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fmin 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fminf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fminf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fminf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fminf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fopen
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fopen.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fopen
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fopen 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fprintf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fprintf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fprintf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fprintf 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fputs
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fputs.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fputs
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fputs 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fread
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fread.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fread
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fread 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-free
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-free.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-free
        -DEXPECT=0
        "-DEXPECT_OUT=rt-free 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-freeaddrinfo
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-freeaddrinfo.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-freeaddrinfo
        -DEXPECT=0
        "-DEXPECT_OUT=rt-freeaddrinfo 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fseek
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fseek.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fseek
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fseek 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-ftell
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-ftell.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-ftell
        -DEXPECT=0
        "-DEXPECT_OUT=rt-ftell 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-fwrite
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-fwrite.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-fwrite
        -DEXPECT=0
        "-DEXPECT_OUT=rt-fwrite 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-gai-strerror
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-gai-strerror.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-gai-strerror
        -DEXPECT=0
        "-DEXPECT_OUT=rt-gai-strerror 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-getaddrinfo
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-getaddrinfo.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-getaddrinfo
        -DEXPECT=0
        "-DEXPECT_OUT=rt-getaddrinfo 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-getcwd
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-getcwd.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-getcwd
        -DEXPECT=0
        "-DEXPECT_OUT=rt-getcwd 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-getenv
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-getenv.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-getenv
        -DEXPECT=0
        "-DEXPECT_OUT=rt-getenv 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-getline
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-getline.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-getline
        -DEXPECT=0
        "-DEXPECT_OUT=rt-getline 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-getrandom
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-getrandom.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-getrandom
        -DEXPECT=0
        "-DEXPECT_OUT=rt-getrandom 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-glob
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-glob.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-glob
        -DEXPECT=0
        "-DEXPECT_OUT=rt-glob 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-globfree
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-globfree.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-globfree
        -DEXPECT=0
        "-DEXPECT_OUT=rt-globfree 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-htonl
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-htonl.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-htonl
        -DEXPECT=0
        "-DEXPECT_OUT=rt-htonl 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-htons
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-htons.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-htons
        -DEXPECT=0
        "-DEXPECT_OUT=rt-htons 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-inet-pton
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-inet-pton.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-inet-pton
        -DEXPECT=0
        "-DEXPECT_OUT=rt-inet-pton 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-ioctl
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-ioctl.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-ioctl
        -DEXPECT=0
        "-DEXPECT_OUT=rt-ioctl 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-isatty
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-isatty.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-isatty
        -DEXPECT=0
        "-DEXPECT_OUT=rt-isatty 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-kill
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-kill.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-kill
        -DEXPECT=0
        "-DEXPECT_OUT=rt-kill 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-labs
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-labs.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-labs
        -DEXPECT=0
        "-DEXPECT_OUT=rt-labs 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-listen
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-listen.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-listen
        -DEXPECT=0
        "-DEXPECT_OUT=rt-listen 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-log
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-log.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-log
        -DEXPECT=0
        "-DEXPECT_OUT=rt-log 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-log10
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-log10.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-log10
        -DEXPECT=0
        "-DEXPECT_OUT=rt-log10 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-log10f
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-log10f.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-log10f
        -DEXPECT=0
        "-DEXPECT_OUT=rt-log10f 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-log2
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-log2.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-log2
        -DEXPECT=0
        "-DEXPECT_OUT=rt-log2 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-log2f
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-log2f.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-log2f
        -DEXPECT=0
        "-DEXPECT_OUT=rt-log2f 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-logf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-logf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-logf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-logf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-lseek
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-lseek.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-lseek
        -DEXPECT=0
        "-DEXPECT_OUT=rt-lseek 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-lstat
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-lstat.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-lstat
        -DEXPECT=0
        "-DEXPECT_OUT=rt-lstat 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-malloc
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-malloc.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-malloc
        -DEXPECT=0
        "-DEXPECT_OUT=rt-malloc 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-mkdir
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-mkdir.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-mkdir
        -DEXPECT=0
        "-DEXPECT_OUT=rt-mkdir 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-mkdtemp
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-mkdtemp.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-mkdtemp
        -DEXPECT=0
        "-DEXPECT_OUT=rt-mkdtemp 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-mkstemp
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-mkstemp.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-mkstemp
        -DEXPECT=0
        "-DEXPECT_OUT=rt-mkstemp 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-mutex-destroy
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-mutex-destroy.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-mutex-destroy
        -DEXPECT=0
        "-DEXPECT_OUT=rt-mutex-destroy 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-mutex-init
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-mutex-init.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-mutex-init
        -DEXPECT=0
        "-DEXPECT_OUT=rt-mutex-init 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-mutex-lock
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-mutex-lock.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-mutex-lock
        -DEXPECT=0
        "-DEXPECT_OUT=rt-mutex-lock 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-mutex-unlock
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-mutex-unlock.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-mutex-unlock
        -DEXPECT=0
        "-DEXPECT_OUT=rt-mutex-unlock 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-nanosleep
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-nanosleep.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-nanosleep
        -DEXPECT=0
        "-DEXPECT_OUT=rt-nanosleep 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-open
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-open.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-open
        -DEXPECT=0
        "-DEXPECT_OUT=rt-open 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-opendir
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-opendir.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-opendir
        -DEXPECT=0
        "-DEXPECT_OUT=rt-opendir 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-perror
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-perror.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-perror
        -DEXPECT=0
        "-DEXPECT_OUT=rt-perror 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-pipe
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-pipe.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-pipe
        -DEXPECT=0
        "-DEXPECT_OUT=rt-pipe 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-posix-spawnp
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-posix-spawnp.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-posix-spawnp
        -DEXPECT=0
        "-DEXPECT_OUT=rt-posix-spawnp 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-pow
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-pow.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-pow
        -DEXPECT=0
        "-DEXPECT_OUT=rt-pow 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-powf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-powf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-powf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-powf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-printf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-printf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-printf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-printf 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-pthread-create
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-pthread-create.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-pthread-create
        -DEXPECT=0
        "-DEXPECT_OUT=rt-pthread-create 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-pthread-detach
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-pthread-detach.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-pthread-detach
        -DEXPECT=0
        "-DEXPECT_OUT=rt-pthread-detach 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-pthread-join
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-pthread-join.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-pthread-join
        -DEXPECT=0
        "-DEXPECT_OUT=rt-pthread-join 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-pthread-self
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-pthread-self.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-pthread-self
        -DEXPECT=0
        "-DEXPECT_OUT=rt-pthread-self 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-read
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-read.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-read
        -DEXPECT=0
        "-DEXPECT_OUT=rt-read 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-readdir
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-readdir.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-readdir
        -DEXPECT=0
        "-DEXPECT_OUT=rt-readdir 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-realloc
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-realloc.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-realloc
        -DEXPECT=0
        "-DEXPECT_OUT=rt-realloc 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-realpath
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-realpath.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-realpath
        -DEXPECT=0
        "-DEXPECT_OUT=rt-realpath 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-recv
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-recv.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-recv
        -DEXPECT=0
        "-DEXPECT_OUT=rt-recv 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-remove
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-remove.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-remove
        -DEXPECT=0
        "-DEXPECT_OUT=rt-remove 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-rename
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-rename.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-rename
        -DEXPECT=0
        "-DEXPECT_OUT=rt-rename 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-rewind
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-rewind.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-rewind
        -DEXPECT=0
        "-DEXPECT_OUT=rt-rewind 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-rmdir
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-rmdir.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-rmdir
        -DEXPECT=0
        "-DEXPECT_OUT=rt-rmdir 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-round
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-round.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-round
        -DEXPECT=0
        "-DEXPECT_OUT=rt-round 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-roundf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-roundf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-roundf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-roundf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sem-destroy
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sem-destroy.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sem-destroy
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sem-destroy 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sem-init
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sem-init.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sem-init
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sem-init 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sem-post
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sem-post.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sem-post
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sem-post 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sem-wait
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sem-wait.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sem-wait
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sem-wait 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-send
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-send.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-send
        -DEXPECT=0
        "-DEXPECT_OUT=rt-send 1"
        -DEXPECT_NL=1
        "-DLIBS=pthread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-setenv
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-setenv.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-setenv
        -DEXPECT=0
        "-DEXPECT_OUT=rt-setenv 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-setsockopt
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-setsockopt.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-setsockopt
        -DEXPECT=0
        "-DEXPECT_OUT=rt-setsockopt 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sigaction
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sigaction.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sigaction
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sigaction 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sin
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sin.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sin
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sin 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sinf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sinf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sinf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sinf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sinh
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sinh.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sinh
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sinh 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sinhf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sinhf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sinhf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sinhf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-snprintf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-snprintf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-snprintf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-snprintf 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-socket
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-socket.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-socket
        -DEXPECT=0
        "-DEXPECT_OUT=rt-socket 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-spawn-actions-destroy
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-spawn-actions-destroy.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-spawn-actions-destroy
        -DEXPECT=0
        "-DEXPECT_OUT=rt-spawn-actions-destroy 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-spawn-actions-init
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-spawn-actions-init.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-spawn-actions-init
        -DEXPECT=0
        "-DEXPECT_OUT=rt-spawn-actions-init 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-spawn-addclose
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-spawn-addclose.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-spawn-addclose
        -DEXPECT=0
        "-DEXPECT_OUT=rt-spawn-addclose 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-spawn-adddup2
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-spawn-adddup2.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-spawn-adddup2
        -DEXPECT=0
        "-DEXPECT_OUT=rt-spawn-adddup2 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-spawnattr-destroy
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-spawnattr-destroy.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-spawnattr-destroy
        -DEXPECT=0
        "-DEXPECT_OUT=rt-spawnattr-destroy 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-spawnattr-init
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-spawnattr-init.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-spawnattr-init
        -DEXPECT=0
        "-DEXPECT_OUT=rt-spawnattr-init 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-spawnattr-setflags
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-spawnattr-setflags.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-spawnattr-setflags
        -DEXPECT=0
        "-DEXPECT_OUT=rt-spawnattr-setflags 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-spawnattr-setpgroup
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-spawnattr-setpgroup.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-spawnattr-setpgroup
        -DEXPECT=0
        "-DEXPECT_OUT=rt-spawnattr-setpgroup 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sqrt
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sqrt.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sqrt
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sqrt 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-sqrtf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-sqrtf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-sqrtf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-sqrtf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-stat
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-stat.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-stat
        -DEXPECT=0
        "-DEXPECT_OUT=rt-stat 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-stdarg
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-stdarg.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-stdarg
        -DEXPECT=0
        "-DEXPECT_OUT=rt-stdarg 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-stderr
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-stderr.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-stderr
        -DEXPECT=0
        "-DEXPECT_OUT=rt-stderr 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-stdin
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-stdin.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-stdin
        -DEXPECT=0
        "-DEXPECT_OUT=rt-stdin 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-stdout
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-stdout.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-stdout
        -DEXPECT=0
        "-DEXPECT_OUT=rt-stdout 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-strerror
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-strerror.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-strerror
        -DEXPECT=0
        "-DEXPECT_OUT=rt-strerror 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-strtod
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-strtod.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-strtod
        -DEXPECT=0
        "-DEXPECT_OUT=rt-strtod 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-strtol
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-strtol.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-strtol
        -DEXPECT=0
        "-DEXPECT_OUT=rt-strtol 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-tan
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-tan.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-tan
        -DEXPECT=0
        "-DEXPECT_OUT=rt-tan 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-tanf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-tanf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-tanf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-tanf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-tanh
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-tanh.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-tanh
        -DEXPECT=0
        "-DEXPECT_OUT=rt-tanh 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-tanhf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-tanhf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-tanhf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-tanhf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-time
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-time.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-time
        -DEXPECT=0
        "-DEXPECT_OUT=rt-time 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-trunc
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-trunc.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-trunc
        -DEXPECT=0
        "-DEXPECT_OUT=rt-trunc 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-truncf
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-truncf.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-truncf
        -DEXPECT=0
        "-DEXPECT_OUT=rt-truncf 1"
        -DEXPECT_NL=1
        "-DLIBS=m"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-unlink
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-unlink.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-unlink
        -DEXPECT=0
        "-DEXPECT_OUT=rt-unlink 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-unsetenv
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-unsetenv.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-unsetenv
        -DEXPECT=0
        "-DEXPECT_OUT=rt-unsetenv 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-waitpid
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-waitpid.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-waitpid
        -DEXPECT=0
        "-DEXPECT_OUT=rt-waitpid 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_rt-write
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/rt-write.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-rt-write
        -DEXPECT=0
        "-DEXPECT_OUT=rt-write 1"
        -DEXPECT_NL=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_sql-execute
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/sql-execute.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-sql-execute
        -DEXPECT=0
        "-DEXPECT_OUT=sql-execute 1"
        -DEXPECT_NL=1
        "-DLIBS=smallcpp_sqlite;sqlite3"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_sql-statement
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/sql-statement.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-sql-statement
        -DEXPECT=0
        "-DEXPECT_OUT=sql-statement 1"
        -DEXPECT_NL=1
        "-DLIBS=smallcpp_sqlite;sqlite3"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_sql-timeout
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/sql-timeout.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-sql-timeout
        -DEXPECT=0
        "-DEXPECT_OUT=sql-timeout 1"
        -DEXPECT_NL=1
        "-DLIBS=smallcpp_sqlite;sqlite3"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_sql-transaction
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/sql-transaction.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-sql-transaction
        -DEXPECT=0
        "-DEXPECT_OUT=sql-transaction 1"
        -DEXPECT_NL=1
        "-DLIBS=smallcpp_sqlite;sqlite3"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
add_test(
    NAME doc_sql-value
    COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:scpp>
        -DSOURCE=${CMAKE_SOURCE_DIR}/docs/examples/sql-value.scpp
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/doc-sql-value
        -DEXPECT=0
        "-DEXPECT_OUT=sql-value 1"
        -DEXPECT_NL=1
        "-DLIBS=smallcpp_sqlite;sqlite3"
        "-DLIB_DIRS=${CMAKE_SOURCE_DIR}/lib"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake)
