# Rebuild libsmallcpp_runtime.a with the language objects before the
# C objects. A single pass over the archive then sees Array after
# String, and the collector after both.
if(NOT ARCHIVE OR NOT STAGE OR NOT AR OR NOT CLI OR NOT STRING OR NOT ARRAY)
    message(FATAL_ERROR "JoinRuntime is missing a path")
endif()
file(GLOB members "${STAGE}/*.o")
if(NOT members)
    message(FATAL_ERROR "JoinRuntime found no C objects in ${STAGE}")
endif()
execute_process(
    COMMAND ${AR} rcs ${ARCHIVE} ${CLI} ${STRING} ${ARRAY} ${members}
    RESULT_VARIABLE joined)
if(NOT joined EQUAL 0)
    message(FATAL_ERROR "ar could not build ${ARCHIVE}")
endif()
