cmake_minimum_required(VERSION 2.8)
project(libcbor)
include(CTest)

SET(CBOR_VERSION_MAJOR "0")
SET(CBOR_VERSION_MINOR "5")
SET(CBOR_VERSION_PATCH "0")
SET(CBOR_VERSION ${CBOR_VERSION_MAJOR}.${CBOR_VERSION_MINOR}.${CBOR_VERSION_PATCH})

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)
include(CheckIncludeFiles)

include(TestBigEndian)
test_big_endian(BIG_ENDIAN)
if(BIG_ENDIAN)
    add_definitions(-DIS_BIG_ENDIAN)
endif()

option(CBOR_CUSTOM_ALLOC "Custom, dynamically defined allocator support" OFF)
option(CBOR_PRETTY_PRINTER "Include a pretty-printing routine" ON)
set(CBOR_BUFFER_GROWTH "2" CACHE STRING "Factor for buffer growth & shrinking")

option(WITH_TESTS "[TEST] Build unit tests (requires CMocka" OFF)
if(WITH_TESTS)
    add_definitions(-DWITH_TESTS)
endif(WITH_TESTS)

option(WITH_EXAMPLES "Build examples" ON)

option(HUGE_FUZZ "[TEST] Fuzz through 8GB of data in the test. Do not use with memory instrumentation!" OFF)
if(HUGE_FUZZ)
    add_definitions(-DHUGE_FUZZ)
endif(HUGE_FUZZ)

option(SANE_MALLOC "[TEST] Assume that malloc will not allocate multi-GB blocks. Tests only, platform specific" OFF)
if(SANE_MALLOC)
    add_definitions(-DSANE_MALLOC)
endif(SANE_MALLOC)

option(PRINT_FUZZ "[TEST] Print the fuzzer input" OFF)
if(PRINT_FUZZ)
    add_definitions(-DPRINT_FUZZ)
endif(PRINT_FUZZ)

option(SANITIZE "Enable ASan & a few compatible sanitizers in Debug mode" ON)

set(CPACK_GENERATOR "DEB" "TGZ" "RPM")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Pavel Kalvoda")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6")
set(CPACK_PACKAGE_VERSION_MAJOR ${CBOR_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${CBOR_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${CBOR_VERSION_PATCH})

include(CPack)

if(MINGW)
    # https://github.com/PJK/libcbor/issues/13
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
elseif(NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()

if(MSVC)
    # This just doesn't work right -- https://msdn.microsoft.com/en-us/library/5ft82fed.aspx
    set(CBOR_RESTRICT_SPECIFIER "")
else()
    set(CBOR_RESTRICT_SPECIFIER "restrict")

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -Wall -pedantic -g -ggdb -DDEBUG=true")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -flto -Wall -pedantic -DNDEBUG")

    if(SANITIZE)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} \
            -fsanitize-trap=undefined -fsanitize=address \
            -fsanitize-trap=bounds -fsanitize=alignment")
    endif()
endif()

set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-g")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto")


include(CheckTypeSize)
check_type_size("size_t" SIZEOF_SIZE_T)
if(SIZEOF_SIZE_T LESS 8)
    message(WARNING "Your size_t is less than 8 bytes. Long items with 64b length specifiers might not work as expected. Make sure to run the tests!")
else()
    add_definitions(-DEIGHT_BYTE_SIZE_T)
endif()

enable_testing()

set(CTEST_MEMORYCHECK_COMMAND "/usr/bin/valgrind")
set(MEMORYCHECK_COMMAND_OPTIONS "--tool=memcheck --track-origins=yes --leak-check=full --error-exitcode=1")

add_custom_target(coverage
                  COMMAND ctest
                  COMMAND lcov --capture --directory . --output-file coverage.info
                  COMMAND genhtml coverage.info --highlight --legend --output-directory coverage_html
                  COMMAND echo "Coverage report ready: file://${CMAKE_CURRENT_BINARY_DIR}/coverage_html/index.html")
include_directories(src)


option(COVERAGE "Enable code coverage instrumentation" OFF)
if (COVERAGE)
    message("Configuring code coverage instrumentation")
    if(NOT CMAKE_C_COMPILER MATCHES "gcc")
        message(WARNING "Gcov instrumentation only works with GCC")
    endif()
    # https://gcc.gnu.org/onlinedocs/gcc/Debugging-Options.html
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fprofile-arcs -ftest-coverage --coverage")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g -fprofile-arcs -ftest-coverage --coverage")
endif (COVERAGE)


# We want to generate configuration.h from the template and make it so that it is accessible using the same
# path during both library build and installed header use, without littering the source dir.
# Using cbor/configuration.h in the build dir works b/c headers will be installed to <prefix>/cbor
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/cbor/configuration.h.in ${PROJECT_BINARY_DIR}/cbor/configuration.h)
install(FILES ${PROJECT_BINARY_DIR}/cbor/configuration.h DESTINATION include/cbor)
# Make the header visible at compile time
include_directories(${PROJECT_BINARY_DIR})

subdirs(src)

if (WITH_TESTS)
    subdirs(test)
endif (WITH_TESTS)

if (WITH_EXAMPLES)
     subdirs(examples)
endif (WITH_EXAMPLES)
