/*
 *	declarations of tiny printf/err functions
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: xprintf.h,v 1.1 1998/09/01 19:51:57 itohy Exp $
 */

#include <sys/cdefs.h>
#ifdef __STDC__
#include <stdarg.h>
#endif

size_t xvsnprintf __P((char *buf, size_t len, const char *fmt, va_list ap));
size_t xsnprintf __P((char *buf, size_t len, const char *fmt, ...));
size_t xvfdprintf __P((int fd, const char *fmt, va_list ap));
size_t xprintf __P((const char *fmt, ...));
size_t xerrprintf __P((const char *fmt, ...));
__dead void xerr __P((int eval, const char *fmt, ...))
				__attribute__((noreturn));
__dead void xerrx __P((int eval, const char *fmt, ...))
				__attribute__((noreturn));
void xwarn __P((const char *fmt, ...));
void xwarnx __P((const char *fmt, ...));
