/*	Id: compat.h,v 1.6 2015/07/24 08:26:05 ragge Exp 	*/	
/*	$NetBSD: compat.h,v 1.1.1.3 2016/02/09 20:29:14 plunky Exp $	*/

/*
 * Just compatibility function prototypes.
 * Public domain.
 */

#ifndef COMPAT_H
#define COMPAT_H

#ifndef HAVE_STRLCPY
#include <stddef.h>	/* size_t */
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_STRLCAT
#include <stddef.h>	/* size_t */
size_t strlcat(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_GETOPT
extern char *optarg;
extern int optind;
int getopt(int, char * const [], const char *);
#endif

#ifndef HAVE_MKSTEMP
int mkstemp(char *);
#endif

#ifndef HAVE_FFS
int ffs(int);
#endif

#ifndef HAVE_SNPRINTF
#include <stddef.h>	/* size_t */
int snprintf(char *str, size_t count, const char *fmt, ...);
#endif

#ifndef HAVE_VSNPRINTF
#include <stddef.h>	/* size_t */
#include <stdarg.h>	/* va_list */
int vsnprintf(char *str, size_t count, const char *fmt, va_list args);
#endif

#endif
