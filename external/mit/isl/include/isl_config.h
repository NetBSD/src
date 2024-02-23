/* isl_config.h.  Generated from isl_config.h.in by configure.  */
/* isl_config.h.in.  Generated from configure.ac by autoheader.  */

/* most gcc compilers know a function __attribute__((__warn_unused_result__))
   */
#define GCC_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))

/* define if the compiler supports basic C++11 syntax */
#define HAVE_CXX11 1

/* define if the compiler supports basic C++17 syntax */
#define HAVE_CXX17 1

/* Define to 1 if you have the declaration of `ffs', and to 0 if you don't. */
#define HAVE_DECL_FFS 1

/* Define to 1 if you have the declaration of `mp_get_memory_functions', and
   to 0 if you don't. */
#define HAVE_DECL_MP_GET_MEMORY_FUNCTIONS 1

/* Define to 1 if you have the declaration of `snprintf', and to 0 if you
   don't. */
#define HAVE_DECL_SNPRINTF 1

/* Define to 1 if you have the declaration of `strcasecmp', and to 0 if you
   don't. */
#define HAVE_DECL_STRCASECMP 1

/* Define to 1 if you have the declaration of `strncasecmp', and to 0 if you
   don't. */
#define HAVE_DECL_STRNCASECMP 1

/* Define to 1 if you have the declaration of `_BitScanForward', and to 0 if
   you don't. */
#define HAVE_DECL__BITSCANFORWARD 0

/* Define to 1 if you have the declaration of `_snprintf', and to 0 if you
   don't. */
#define HAVE_DECL__SNPRINTF 0

/* Define to 1 if you have the declaration of `_stricmp', and to 0 if you
   don't. */
#define HAVE_DECL__STRICMP 0

/* Define to 1 if you have the declaration of `_strnicmp', and to 0 if you
   don't. */
#define HAVE_DECL__STRNICMP 0

/* Define to 1 if you have the declaration of `__builtin_ffs', and to 0 if you
   don't. */
#define HAVE_DECL___BUILTIN_FFS 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `gmp' library (-lgmp). */
#define HAVE_LIBGMP 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* define if your compiler has __attribute__ */
#define HAVE___ATTRIBUTE__ 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "isl"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "isl-development@googlegroups.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "isl"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "isl 0.26"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "isl"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.26"

/* The size of `char', as computed by sizeof. */
/* #undef SIZEOF_CHAR */

/* The size of `int', as computed by sizeof. */
/* #undef SIZEOF_INT */

/* The size of `long', as computed by sizeof. */
/* #undef SIZEOF_LONG */

/* The size of `short', as computed by sizeof. */
/* #undef SIZEOF_SHORT */

/* The size of `void*', as computed by sizeof. */
/* #undef SIZEOF_VOIDP */

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* use gmp to implement isl_int */
#define USE_GMP_FOR_MP /**/

/* use imath to implement isl_int */
/* #undef USE_IMATH_FOR_MP */

/* Use small integer optimization */
/* #undef USE_SMALL_INT_OPT */

/* Version number of package */
#define VERSION "0.26"

#include <isl_config_post.h>
