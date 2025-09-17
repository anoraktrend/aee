cmake_minimum_required(VERSION 3.10)
project(aee C)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  # Settings specific to Release build
  add_definitions(-DNDEBUG) # Disable assertions and debug code
  set(CMAKE_OPTIMIZATION_FLAGS "-O3") # Enable aggressive optimizations
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Settings specific to Debug build
  set(CMAKE_OPTIMIZATION_FLAGS "-Og -g -DTEST_TERMINAL") # Enable debug information and minimal optimizations
endif()

# Set compile flags based on the build type
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_OPTIMIZATION_FLAGS} -Wno-format-security -Wno-unused-result -Wno-error")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_OPTIMIZATION_FLAGS}")
# Version
set(AEE_VERSION_MAJOR 1)
set(AEE_VERSION_MINOR 0)

# Find required packages
find_package(Curses REQUIRED)

# Option to disable catgets support
option(NO_CATGETS "Disable catgets message catalog support" OFF)
if(NOT NO_CATGETS)
    check_include_files(nl_types.h HAS_NL_TYPES)
    if(HAS_NL_TYPES)
        add_definitions(-DHAVE_NL_TYPES_H)
    endif()
else()
    add_definitions(-DNO_CATGETS)
    message(STATUS "Building without catgets support")
endif()


# Configure compiler flags
include(CheckIncludeFiles)
check_include_files(stdlib.h HAS_STDLIB)
check_include_files(stdarg.h HAS_STDARG)
check_include_files(unistd.h HAS_UNISTD)
check_include_files(ctype.h HAS_CTYPE)
check_include_files("sys/ioctl.h" HAS_SYS_IOCTL)
check_include_files("sys/wait.h" HAS_SYS_WAIT)
check_include_files(locale.h HAS_LOCALE)
# Common source files
set(AEE_SOURCES
    aee.c
    control.c
    format.c
    localize.c
    srch_rep.c
    delete.c
    mark.c
    motion.c
    keys.c
    help.c
    windows.c
    journal.c
    file.c
    curses_compat.c
)

# Set the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set up sysroot if specified
if(DEFINED CMAKE_SYSROOT)
    set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT} ${CMAKE_FIND_ROOT_PATH})
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

# Find Curses with ncurses support
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

# Include current source directory
include_directories(${CMAKE_SOURCE_DIR})

# AEE executable
add_executable(aee ${AEE_SOURCES})
target_link_libraries(aee PRIVATE ${CURSES_LIBRARIES})
target_include_directories(aee PRIVATE ${CURSES_INCLUDE_DIR})
target_compile_definitions(aee PRIVATE
    AEE_VERSION_MAJOR=${AEE_VERSION_MAJOR}
    AEE_VERSION_MINOR=${AEE_VERSION_MINOR}
    HAS_STDLIB
    HAS_STDARG
    HAS_UNISTD
    HAS_CTYPE
    HAS_SYS_IOCTL
    HAS_SYS_WAIT
    HAS_LOCALE
    HAS_NL_TYPES
    BSD_SELECT
    NOCATGETS
)

# Installation
install(TARGETS aee
    RUNTIME DESTINATION bin
)

install(FILES aee.1
    DESTINATION share/man/man1
)

install(FILES help.ae
    DESTINATION share/aee
)

# Create symlink for rae
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink aee rae
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)")
