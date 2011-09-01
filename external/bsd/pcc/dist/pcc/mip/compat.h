/*	Id: compat.h,v 1.5 2011/06/09 19:24:46 plunky Exp 	*/	
/*	$NetBSD: compat.h,v 1.1.1.2 2011/09/01 12:47:12 plunky Exp $	*/

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

#ifndef HAVE_BASENAME
char *basename(char *);
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
