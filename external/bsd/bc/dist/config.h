/* config.h  -- hand edited for NetBSD */
#define PACKAGE "bc"

/* Package VERSION number */
#define VERSION "nb1.0"

/* COPYRIGHT notice for BC target */
#define BC_COPYRIGHT "Copyright (C) 1991-1994, 1997, 2000, 2012-2017 "\
        "Free Software Foundation, Inc.\n"\
        "Copyright (C) 2004, 2005, 2016-2017 Philip A. Nelson"

/* Define to use the BSD libedit library. */
#define LIBEDIT

/* Define to `size_t' if <sys/types.h> and <stddef.h> don't define.  */
#define ptrdiff_t size_t

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `isgraph' function. */
#define HAVE_ISGRAPH 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `setvbuf' function. */
#define HAVE_SETVBUF 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#undef YYTEXT_POINTER

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
#undef _POSIX_1_SOURCE

/* Define to 1 if you need to in order for `stat' and other things to work. */
#undef _POSIX_SOURCE

/* Define to empty if `const' does not conform to ANSI C. */
#undef const

/* Define to `size_t' if <sys/types.h> does not define. */
#undef ptrdiff_t

/* Define to `unsigned' if <sys/types.h> does not define. */
#undef size_t

/* NetBSD bc does not print a welcome! */
#define QUIET
