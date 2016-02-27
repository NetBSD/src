/* tools/clang/include/clang/Config/config.h.  Generated from config.h.in by configure.  */
/* This generated file is for internal use. Do not include it from headers. */

#ifdef CLANG_CONFIG_H
#error config.h can only be included once
#else
#define CLANG_CONFIG_H

/* Bug report URL. */
#define BUG_REPORT_URL "http://llvm.org/bugs/"

/* Default OpenMP runtime used by -fopenmp. */
#define CLANG_DEFAULT_OPENMP_RUNTIME "libomp"

/* Multilib suffix for libdir. */
#define CLANG_LIBDIR_SUFFIX ""

/* Relative directory for resource files */
#define CLANG_RESOURCE_DIR ""

/* Directories clang will search for headers */
#define C_INCLUDE_DIRS "/usr/include/clang-3.8:/usr/include"

/* Default <path> to all compiler invocations for --sysroot=<path>. */
#define DEFAULT_SYSROOT ""

/* Directory where gcc is installed. */
#define GCC_INSTALL_PREFIX ""

/* Define if we have libxml2 */
/* #undef CLANG_HAVE_LIBXML */

#define PACKAGE_STRING "LLVM 3.8.0"

/* The LLVM product name and version */
#define BACKEND_PACKAGE_STRING PACKAGE_STRING

/* Linker version detected at compile time. */
#define HOST_LINK_VERSION "1"

#endif
