/*	$NetBSD: xprintf.c,v 1.2 1999/02/07 17:23:40 christos Exp $	*/

/*
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "rtldenv.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>

static __inline long long xva_arg __P((va_list *, int));

#define SZ_SHORT	0x01
#define	SZ_INT		0x02
#define	SZ_LONG		0x04
#define	SZ_QUAD		0x08
#define	SZ_UNSIGNED	0x10
#define	SZ_MASK		0x0f

static __inline long long
xva_arg(ap, size)
	va_list *ap;
	int size;
{
	switch (size) {
	case SZ_SHORT:
		return va_arg(*ap, short);
	default:
	case SZ_INT:
		return va_arg(*ap, int);
	case SZ_LONG:
		return va_arg(*ap, long);
	case SZ_QUAD:
		return va_arg(*ap, long long);
	case SZ_SHORT|SZ_UNSIGNED:
		return va_arg(*ap, unsigned short);
	case SZ_INT|SZ_UNSIGNED:
		return va_arg(*ap, unsigned int);
	case SZ_LONG|SZ_UNSIGNED:
		return va_arg(*ap, unsigned long);
	case SZ_QUAD|SZ_UNSIGNED:
		return va_arg(*ap, unsigned long long);
	}
}
/*
 * Non-mallocing printf, for use by malloc and rtld itself.
 * This avoids putting in most of stdio.
 *
 * deals withs formats %x, %p, %s, and %d.
 */
size_t
xvsnprintf(
    char *buf,
    size_t buflen,
    const char *fmt,
    va_list ap)
{
    char *bp = buf;
    char * const ep = buf + buflen - 4;
    int size;

    while (*fmt != NULL && bp < ep) {
	switch (*fmt) {
	case '\\': {
	    if (fmt[1] != '\0')
		*bp++ = *++fmt;
	    continue;
	}
	case '%': {
	    size = SZ_INT;
rflag:	    switch (fmt[1]) {
	    case 'h':
		size = (size & SZ_MASK) | SZ_SHORT;
		fmt++;
		goto rflag;
	    case 'l':
		size = (size & SZ_MASK) | SZ_LONG;
		fmt++;
		if (fmt[1] == 'l') {
	    case 'q':
		    size = (size & SZ_MASK) | SZ_QUAD;
		    fmt++;
		}
		goto rflag;
	    case 'u':
		size |= SZ_UNSIGNED;
		/* FALLTHROUGH*/
	    case 'd': {
		long long sval;
		unsigned long long uval;
		char digits[sizeof(int) * 3], *dp = digits;

		if (fmt[1] == 'd') {
		    sval = xva_arg(&ap, size);
		    if (sval < 0) {
			if ((sval << 1) == 0) {
			    /*
			     * We can't flip the sign of this since
			     * it's can't represented as a postive
			     * number in two complement, handle the
			     * first digit.  After that, it can be
			     * flipped since it is now not 2^(n-1).
			     */
			    *dp++ = '0' - (sval % 10);
			    sval /= 10;
			}
			*bp++ = '-';
			uval = -sval;
		    } else {
			uval = sval;
		    }
		} else {
		    uval = xva_arg(&ap, size);
		}
		do {
		    *dp++ = '0' + (uval % 10);
		    uval /= 10;
		} while (uval != 0);
		do {
		    *bp++ = *--dp;
		} while (dp != digits && bp < ep);
		fmt += 2;
		break;
	    }
	    case 'x': case 'p': {
		unsigned long val = va_arg(ap, unsigned long);
		unsigned long mask = ~(~0UL >> 4);
		int bits = sizeof(val) * 8 - 4;
		const char hexdigits[] = "0123456789abcdef";
		if (fmt[1] == 'p') {
		    *bp++ = '0';
		    *bp++ = 'x';
		}
		/* handle the border case */
		if (val == 0) {
		    *bp++ = '0';
		    fmt += 2;
		    break;
		}
		/* suppress 0s */
		while ((val & mask) == 0)
		    bits -= 4, mask >>= 4;

		/* emit the hex digits */
		while (bits >= 0 && bp < ep) {
		    *bp++ = hexdigits[(val & mask) >> bits];
		    bits -= 4, mask >>= 4;
		}
		fmt += 2;
		break;
	    }
	    case 's': {
		const char *str = va_arg(ap, const char *);
		int len;
		if (str == NULL)
		    str = "(null)";

		len = strlen(str);
		if (ep - bp < len)
		    len = ep - bp;
		memcpy(bp, str, len);
		bp += len;
		fmt += 2;
		break;
	    }
	    default:
		*bp++ = *fmt;
		break;
	    }
	    break;
	}
	default:
	    *bp++ = *fmt++;
	    break;
	}
    }
	    

    *bp = '\0';
    return bp - buf;
}

void
xvprintf(
    const char *fmt,
    va_list ap)
{
    char buf[256];
    (void) write(2, buf, xvsnprintf(buf, sizeof(buf), fmt, ap));
}

void
xprintf(
    const char *fmt,
    ...)
{
    va_list ap;
    va_start(ap, fmt);

    xvprintf(fmt, ap);

    va_end(ap);
}

void
xsnprintf(
    char *buf,
    size_t buflen,
    const char *fmt,
    ...)
{
    va_list ap;
    va_start(ap, fmt);

    xvprintf(fmt, ap);

    va_end(ap);
}

const char *
xstrerror(
    int error)
{
    if (error >= sys_nerr) {
	static char buf[128];
	xsnprintf(buf, sizeof(buf), "Unknown error: %d", error);
	return buf;
    }
    return sys_errlist[error];
}

void
xerrx(
    int eval,
    const char *fmt,
    ...)
{
    va_list ap;
    va_start(ap, fmt);
    xvprintf(fmt, ap);
    va_end(ap);

    exit(eval);
}

void
xerr(
    int eval,
    const char *fmt,
    ...)
{
    int saved_errno = errno;

    va_list ap;
    va_start(ap, fmt);
    xvprintf(fmt, ap);
    va_end(ap);

    xprintf(": %s\n", xstrerror(saved_errno));
    exit(eval);
}

void
xwarn(
    const char *fmt,
    ...)
{
    int saved_errno = errno;

    va_list ap;
    va_start(ap, fmt);
    xvprintf(fmt, ap);
    va_end(ap);

    xprintf(": %s\n", xstrerror(saved_errno));
    errno = saved_errno;
}

void
xwarnx(
    const char *fmt,
    ...)
{
    va_list ap;
    va_start(ap, fmt);
    xvprintf(fmt, ap);
    va_end(ap);
}

void
xassert(
    const char *file,
    int line,
    const char *failedexpr)
{
    xprintf("assertion \"%s\" failed: file \"%s\", line %d\n",
	    failedexpr, file, line);
    abort();
    /* NOTREACHED */
}
