cmake_minimum_required(VERSION 3.10)
project(aee C)

# Version
set(AEE_VERSION_MAJOR 1)
set(AEE_VERSION_MINOR 0)

# Options
option(BUILD_XAE "Build X11 version" ON)

# Find required packages
find_package(Curses REQUIRED)
if(BUILD_XAE)
    find_package(X11 REQUIRED)
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
check_include_files(nl_types.h HAS_NL_TYPES)

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
)

# AEE executable
add_executable(aee ${AEE_SOURCES} new_curse.c)
target_link_libraries(aee PRIVATE ${CURSES_LIBRARIES})
target_include_directories(aee PRIVATE ${CURSES_INCLUDE_DIRS})

# XAE executable
if(BUILD_XAE)
    add_executable(xae ${AEE_SOURCES} Xcurse.c xif.c)
    target_compile_definitions(xae PRIVATE XAE xae11)
    target_link_libraries(xae PRIVATE ${X11_LIBRARIES})
    target_include_directories(xae PRIVATE ${X11_INCLUDE_DIR})
endif()

# Installation
install(TARGETS aee
    RUNTIME DESTINATION bin
)

if(BUILD_XAE)
    install(TARGETS xae
        RUNTIME DESTINATION bin
    )
endif()

install(FILES aee.1
    DESTINATION share/man/man1
)

install(FILES help.ae
    DESTINATION share/aee
)

# Create symlink for rae
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink aee rae
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)")
