# A fresh configuration installs to $HOME/opt/smallcpp. HOME is expanded
# here, so the cache holds an absolute path and not a literal tilde.
# -DCMAKE_INSTALL_PREFIX and, on CMake 3.29+, the CMAKE_INSTALL_PREFIX
# environment variable still win. A later configure keeps the cached path.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT
   AND (CMAKE_VERSION VERSION_LESS 3.29 OR NOT DEFINED ENV{CMAKE_INSTALL_PREFIX}))
    if("$ENV{HOME}" STREQUAL "")
        message(FATAL_ERROR
            "Cannot select the default smallcpp install prefix: HOME is not set; "
            "set HOME to an absolute path or configure with -DCMAKE_INSTALL_PREFIX=<path>")
    endif()
    if(NOT IS_ABSOLUTE "$ENV{HOME}")
        message(FATAL_ERROR
            "Cannot select the default smallcpp install prefix: HOME must be absolute "
            "(got '$ENV{HOME}'); configure with -DCMAKE_INSTALL_PREFIX=<path> to override it")
    endif()

    get_filename_component(smallcpp_install_home "$ENV{HOME}" ABSOLUTE)
    set(CMAKE_INSTALL_PREFIX "${smallcpp_install_home}/opt/smallcpp" CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT FALSE)
endif()
