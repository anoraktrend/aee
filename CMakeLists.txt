cmake_minimum_required(VERSION 3.15)
project(aee C)

# Version
set(AEE_VERSION_MAJOR 1)
set(AEE_VERSION_MINOR 0)

# Set the C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build type configuration
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(NDEBUG)
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3 -march=native)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Og -g)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# Include CMake modules
include(CheckIncludeFiles)

# Check for required headers
check_include_files(stdlib.h HAS_STDLIB)
check_include_files(stdarg.h HAS_STDARG)
check_include_files(unistd.h HAS_UNISTD)
check_include_files(ctype.h HAS_CTYPE)
check_include_files("sys/ioctl.h" HAS_SYS_IOCTL)
check_include_files("sys/wait.h" HAS_SYS_WAIT)
check_include_files(locale.h HAS_LOCALE)

# Find Intl for gettext support
find_package(Intl REQUIRED)

# Find Readline for line editing
find_library(READLINE_LIBRARY readline)
if(READLINE_LIBRARY)
    message(STATUS "Found readline: ${READLINE_LIBRARY}")
else()
    message(STATUS "Readline not found, line editing will not be available")
endif()

# Common source files
set(AEE_SOURCES
    src/aee.c
    src/control.c
    src/format.c
    src/localize.c
    src/srch_rep.c
    src/delete.c
    src/mark.c
    src/motion.c
    src/keys.c
    src/help.c
    src/windows.c
    src/journal.c
    src/file.c
    src/lsp.c
    src/highlighting.c
)

# Find Threads
find_package(Threads REQUIRED)

# Add cJSON for JSON parsing
add_subdirectory(third_party/cJSON)

if(NOT CURSES_FOUND)
    set(CURSES_NEED_WIDE FALSE)
    find_package(Curses QUIET)
    
    if(NOT CURSES_FOUND)
        set(CURSES_NEED_NCURSES FALSE)
        find_package(Curses REQUIRED)
        message(STATUS "Found Curses: ${CURSES_LIBRARIES}")
    else()
        message(STATUS "Found ncurses: ${CURSES_LIBRARIES}")
    endif()
else()
    message(STATUS "Found ncursesw (wide character support): ${CURSES_LIBRARIES}")
    add_compile_definitions(HAVE_NCURSESW)
endif()

# AEE executable
add_executable(aee ${AEE_SOURCES})

# Target properties
target_include_directories(aee PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/third_party/cJSON ${CURSES_INCLUDE_DIRS})
target_link_libraries(aee PRIVATE ${CURSES_LIBRARIES} Threads::Threads cjson Intl::Intl)
if(READLINE_LIBRARY)
    target_link_libraries(aee PRIVATE ${READLINE_LIBRARY})
endif()
target_compile_definitions(aee PRIVATE
    AEE_VERSION_MAJOR=${AEE_VERSION_MAJOR}
    AEE_VERSION_MINOR=${AEE_VERSION_MINOR}
    $<$<BOOL:${HAS_STDLIB}>:HAS_STDLIB>
    $<$<BOOL:${HAS_STDARG}>:HAS_STDARG>
    $<$<BOOL:${HAS_UNISTD}>:HAS_UNISTD>
    $<$<BOOL:${HAS_CTYPE}>:HAS_CTYPE>
    $<$<BOOL:${HAS_SYS_IOCTL}>:HAS_SYS_IOCTL>
    $<$<BOOL:${HAS_SYS_WAIT}>:HAS_SYS_WAIT>
    $<$<BOOL:${HAS_LOCALE}>:HAS_LOCALE>
    $<$<BOOL:${READLINE_LIBRARY}>:HAVE_READLINE>
    NO_CATGETS
    BSD_SELECT
)

# Installation
include(GNUInstallDirs)
install(TARGETS aee
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES aee.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)

install(FILES help.ae
    DESTINATION ${CMAKE_INSTALL_DATADIR}/aee
)

# Create symlink for rae
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink aee rae
    WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})")
