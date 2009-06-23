/*	$NetBSD: vbuf_print.c,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

/*++
/* NAME
/*	vbuf_print 3
/* SUMMARY
/*	formatted print to generic buffer
/* SYNOPSIS
/*	#include <stdarg.h>
/*	#include <vbuf_print.h>
/*
/*	VBUF	*vbuf_print(bp, format, ap)
/*	VBUF	*bp;
/*	const char *format;
/*	va_list	ap;
/* DESCRIPTION
/*	vbuf_print() appends data to the named buffer according to its
/*	\fIformat\fR argument. It understands the s, c, d, u, o, x, X, p, e,
/*	f and g format types, the l modifier, field width and precision,
/*	sign, and padding with zeros or spaces.
/*
/*	In addition, vbuf_print() recognizes the %m format specifier
/*	and expands it to the error message corresponding to the current
/*	value of the global \fIerrno\fR variable.
/* REENTRANCY
/* .ad
/* .fi
/*	vbuf_print() allocates a static buffer. After completion
/*	of the first vbuf_print() call, this buffer is safe for
/*	reentrant vbuf_print() calls by (asynchronous) terminating
/*	signal handlers or by (synchronous) terminating error
/*	handlers. vbuf_print() initialization typically happens
/*	upon the first formatted output to a VSTRING or VSTREAM.
/*
/*	However, it is up to the caller to ensure that the destination
/*	VSTREAM or VSTRING buffer is protected against reentrant usage.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include "sys_defs.h"
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>			/* 44bsd stdarg.h uses abort() */
#include <stdio.h>			/* sprintf() prototype */
#include <float.h>			/* range of doubles */
#include <errno.h>
#include <limits.h>			/* CHAR_BIT */

/* Application-specific. */

#include "msg.h"
#include "vbuf.h"
#include "vstring.h"
#include "vbuf_print.h"

 /*
  * What we need here is a *sprintf() routine that can ask for more room (as
  * in 4.4 BSD). However, that functionality is not widely available, and I
  * have no plans to maintain a complete 4.4 BSD *sprintf() alternative.
  * 
  * This means we're stuck with plain old ugly sprintf() for all non-trivial
  * conversions. We cannot use snprintf() even if it is available, because
  * that routine truncates output, and we want everything. Therefore, it is
  * up to us to ensure that sprintf() output always stays within bounds.
  * 
  * Due to the complexity of *printf() format strings we cannot easily predict
  * how long results will be without actually doing the conversions. A trick
  * used by some people is to print to a temporary file and to read the
  * result back. In programs that do a lot of formatting, that might be too
  * expensive.
  * 
  * Guessing the output size of a string (%s) conversion is not hard. The
  * problem is with numerical results. Instead of making an accurate guess we
  * take a wide margin when reserving space.  The INT_SPACE margin should be
  * large enough to hold the result from any (octal, hex, decimal) integer
  * conversion that has no explicit width or precision specifiers. With
  * floating-point numbers, use a similar estimate, and add DBL_MAX_10_EXP
  * just to be sure.
  */
#define INT_SPACE	((CHAR_BIT * sizeof(long)) / 2)
#define DBL_SPACE	((CHAR_BIT * sizeof(double)) / 2 + DBL_MAX_10_EXP)
#define PTR_SPACE	((CHAR_BIT * sizeof(char *)) / 2)

 /*
  * Helper macros... Note that there is no need to check the result from
  * VSTRING_SPACE() because that always succeeds or never returns.
  */
#define VBUF_SKIP(bp)	{ \
	while ((bp)->cnt > 0 && *(bp)->ptr) \
	    (bp)->ptr++, (bp)->cnt--; \
    }

#define VSTRING_ADDNUM(vp, n) { \
	VSTRING_SPACE(vp, INT_SPACE); \
	sprintf(vstring_end(vp), "%d", n); \
	VBUF_SKIP(&vp->vbuf); \
    }

#define VBUF_STRCAT(bp, s) { \
	unsigned char *_cp = (unsigned char *) (s); \
	int _ch; \
	while ((_ch = *_cp++) != 0) \
	    VBUF_PUT((bp), _ch); \
    }

/* vbuf_print - format string, vsprintf-like interface */

VBUF   *vbuf_print(VBUF *bp, const char *format, va_list ap)
{
    const char *myname = "vbuf_print";
    static VSTRING *fmt;		/* format specifier */
    unsigned char *cp;
    int     width;			/* width and numerical precision */
    int     prec;			/* are signed for overflow defense */
    unsigned long_flag;			/* long or plain integer */
    int     ch;
    char   *s;

    /*
     * Assume that format strings are short.
     */
    if (fmt == 0)
	fmt = vstring_alloc(INT_SPACE);

    /*
     * Iterate over characters in the format string, picking up arguments
     * when format specifiers are found.
     */
    for (cp = (unsigned char *) format; *cp; cp++) {
	if (*cp != '%') {
	    VBUF_PUT(bp, *cp);			/* ordinary character */
	} else if (cp[1] == '%') {
	    VBUF_PUT(bp, *cp++);		/* %% becomes % */
	} else {

	    /*
	     * Handle format specifiers one at a time, since we can only deal
	     * with arguments one at a time. Try to determine the end of the
	     * format specifier. We do not attempt to fully parse format
	     * strings, since we are ging to let sprintf() do the hard work.
	     * In regular expression notation, we recognize:
	     * 
	     * %-?0?([0-9]+|\*)?\.?([0-9]+|\*)?l?[a-zA-Z]
	     * 
	     * which includes some combinations that do not make sense. Garbage
	     * in, garbage out.
	     */
	    VSTRING_RESET(fmt);			/* clear format string */
	    VSTRING_ADDCH(fmt, *cp++);
	    if (*cp == '-')			/* left-adjusted field? */
		VSTRING_ADDCH(fmt, *cp++);
	    if (*cp == '+')			/* signed field? */
		VSTRING_ADDCH(fmt, *cp++);
	    if (*cp == '0')			/* zero-padded field? */
		VSTRING_ADDCH(fmt, *cp++);
	    if (*cp == '*') {			/* dynamic field width */
		width = va_arg(ap, int);
		VSTRING_ADDNUM(fmt, width);
		cp++;
	    } else {				/* hard-coded field width */
		for (width = 0; ch = *cp, ISDIGIT(ch); cp++) {
		    width = width * 10 + ch - '0';
		    VSTRING_ADDCH(fmt, ch);
		}
	    }
	    if (width < 0) {
		msg_warn("%s: bad width %d in %.50s", myname, width, format);
		width = 0;
	    }
	    if (*cp == '.')			/* width/precision separator */
		VSTRING_ADDCH(fmt, *cp++);
	    if (*cp == '*') {			/* dynamic precision */
		prec = va_arg(ap, int);
		VSTRING_ADDNUM(fmt, prec);
		cp++;
	    } else {				/* hard-coded precision */
		for (prec = 0; ch = *cp, ISDIGIT(ch); cp++) {
		    prec = prec * 10 + ch - '0';
		    VSTRING_ADDCH(fmt, ch);
		}
	    }
	    if (prec < 0) {
		msg_warn("%s: bad precision %d in %.50s", myname, prec, format);
		prec = 0;
	    }
	    if ((long_flag = (*cp == 'l')) != 0)/* long whatever */
		VSTRING_ADDCH(fmt, *cp++);
	    if (*cp == 0)			/* premature end, punt */
		break;
	    VSTRING_ADDCH(fmt, *cp);		/* type (checked below) */
	    VSTRING_TERMINATE(fmt);		/* null terminate */

	    /*
	     * Execute the format string - let sprintf() do the hard work for
	     * non-trivial cases only. For simple string conversions and for
	     * long string conversions, do a direct copy to the output
	     * buffer.
	     */
	    switch (*cp) {
	    case 's':				/* string-valued argument */
		s = va_arg(ap, char *);
		if (prec > 0 || (width > 0 && width > strlen(s))) {
		    if (VBUF_SPACE(bp, (width > prec ? width : prec) + INT_SPACE))
			return (bp);
		    sprintf((char *) bp->ptr, vstring_str(fmt), s);
		    VBUF_SKIP(bp);
		} else {
		    VBUF_STRCAT(bp, s);
		}
		break;
	    case 'c':				/* integral-valued argument */
	    case 'd':
	    case 'u':
	    case 'o':
	    case 'x':
	    case 'X':
		if (VBUF_SPACE(bp, (width > prec ? width : prec) + INT_SPACE))
		    return (bp);
		if (long_flag)
		    sprintf((char *) bp->ptr, vstring_str(fmt), va_arg(ap, long));
		else
		    sprintf((char *) bp->ptr, vstring_str(fmt), va_arg(ap, int));
		VBUF_SKIP(bp);
		break;
	    case 'e':				/* float-valued argument */
	    case 'f':
	    case 'g':
		if (VBUF_SPACE(bp, (width > prec ? width : prec) + DBL_SPACE))
		    return (bp);
		sprintf((char *) bp->ptr, vstring_str(fmt), va_arg(ap, double));
		VBUF_SKIP(bp);
		break;
	    case 'm':
		VBUF_STRCAT(bp, strerror(errno));
		break;
	    case 'p':
		if (VBUF_SPACE(bp, (width > prec ? width : prec) + PTR_SPACE))
		    return (bp);
		sprintf((char *) bp->ptr, vstring_str(fmt), va_arg(ap, char *));
		VBUF_SKIP(bp);
		break;
	    default:				/* anything else is bad */
		msg_panic("vbuf_print: unknown format type: %c", *cp);
		/* NOTREACHED */
		break;
	    }
	}
    }
    return (bp);
}
