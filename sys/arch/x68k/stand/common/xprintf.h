/*
 *	declarations of tiny printf/err functions
 *
 *	written by ITOH Yasufumi
 *	public domain
 *
 *	$NetBSD: xprintf.h,v 1.2.8.1 2011/03/05 15:10:08 bouyer Exp $
 */

#include <sys/cdefs.h>
#ifdef __STDC__
#include <stdarg.h>
#endif

size_t xvsnprintf(char *buf, size_t len, const char *fmt, va_list ap);
size_t xsnprintf(char *buf, size_t len, const char *fmt, ...);
size_t xvfdprintf(int fd, const char *fmt, va_list ap);
size_t xprintf(const char *fmt, ...);
size_t xerrprintf(const char *fmt, ...);
__dead void xerr(int eval, const char *fmt, ...)
				__attribute__((noreturn));
__dead void xerrx(int eval, const char *fmt, ...)
				__attribute__((noreturn));
void xwarn(const char *fmt, ...);
void xwarnx(const char *fmt, ...);
