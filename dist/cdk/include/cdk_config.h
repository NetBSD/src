/* include/cdk_config.h.  Generated automatically by configure.  */
/* 
 * $Id: cdk_config.h,v 1.2 2004/04/26 05:15:17 simonb Exp $
 */

#ifndef CDK_CONFIG_H
#define CDK_CONFIG_H 1

#define HAVE_DIRENT_H 1
#define HAVE_GETCWD 1
#define HAVE_GETOPT_H 1
#define HAVE_GETOPT_HEADER 1
/* #undef HAVE_LIBXT */
/* #undef HAVE_LIB_XAW */
#define HAVE_LIMITS_H 1
#define HAVE_LSTAT 1
#define HAVE_MKTIME 1
/* #undef HAVE_NCURSES_H */
#define HAVE_RADIXSORT 1
#define HAVE_START_COLOR 1
/* #undef HAVE_STDLIB_H */
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_TYPE_CHTYPE 1
#define HAVE_UNISTD_H 1
/* #undef NCURSES */
#define RETSIGTYPE void
#define STDC_HEADERS 1
#define TYPE_CHTYPE_IS_SCALAR 1

#if !defined(HAVE_LSTAT) && !defined(lstat)
#define lstat(f,b) stat(f,b)
#endif

#endif /* CDK_CONFIG_H */
