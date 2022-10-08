/*	$NetBSD: vbuf_print.c,v 1.4 2022/10/08 16:12:50 christos Exp $	*/

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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
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
#include <limits.h>			/* CHAR_BIT, INT_MAX */

/* Application-specific. */

#include "msg.h"
#include "mymalloc.h"
#include "vbuf.h"
#include "vstring.h"
#include "vbuf_print.h"

 /*
  * What we need here is a *sprintf() routine that can ask for more room (as
  * in 4.4 BSD). However, that functionality is not widely available, and I
  * have no plans to maintain a complete 4.4 BSD *sprintf() alternative.
  * 
  * Postfix vbuf_print() was implemented when many mainstream systems had no
  * usable snprintf() implementation (usable means: return the length,
  * excluding terminator, that the output would have if the buffer were large
  * enough). For example, GLIBC before 2.1 (1999) snprintf() did not
  * distinguish between formatting error and buffer size error, while SUN had
  * no snprintf() implementation before Solaris 2.6 (1997).
  * 
  * For the above reasons, vbuf_print() was implemented with sprintf() and a
  * generously-sized output buffer. Current vbuf_print() implementations use
  * snprintf(), and report an error if the output does not fit (in that case,
  * the old sprintf()-based implementation would have had a buffer overflow
  * vulnerability). The old implementation is still available for building
  * Postfix on ancient systems.
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
#ifndef NO_SNPRINTF
#define VBUF_SNPRINTF(bp, sz, fmt, arg) do { \
	ssize_t _ret; \
	if (VBUF_SPACE((bp), (sz)) != 0) \
	    return (bp); \
	_ret = snprintf((char *) (bp)->ptr, (bp)->cnt, (fmt), (arg)); \
	if (_ret < 0) \
	    msg_panic("%s: output error for '%s'", myname, mystrdup(fmt)); \
	if (_ret >= (bp)->cnt) \
	    msg_panic("%s: output for '%s' exceeds space %ld", \
		      myname, mystrdup(fmt), (long) (bp)->cnt); \
	VBUF_SKIP(bp); \
    } while (0)
#else
#define VBUF_SNPRINTF(bp, sz, fmt, arg) do { \
	if (VBUF_SPACE((bp), (sz)) != 0) \
	    return (bp); \
	sprintf((char *) (bp)->ptr, (fmt), (arg)); \
	VBUF_SKIP(bp); \
    } while (0)
#endif

#define VBUF_SKIP(bp) do { \
	while ((bp)->cnt > 0 && *(bp)->ptr) \
	    (bp)->ptr++, (bp)->cnt--; \
    } while (0)

#define VSTRING_ADDNUM(vp, n) do { \
	VBUF_SNPRINTF(&(vp)->vbuf, INT_SPACE, "%d", n); \
    } while (0)

#define VBUF_STRCAT(bp, s) do { \
	unsigned char *_cp = (unsigned char *) (s); \
	int _ch; \
	while ((_ch = *_cp++) != 0) \
	    VBUF_PUT((bp), _ch); \
    } while (0)

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
    int     saved_errno = errno;	/* VBUF_SPACE() may clobber it */

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
	     * %-?+?0?([0-9]+|\*)?(\.([0-9]+|\*))?l?[a-zA-Z]
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
		if (width < 0) {
		    msg_warn("%s: bad width %d in %.50s",
			     myname, width, format);
		    width = 0;
		} else
		    VSTRING_ADDNUM(fmt, width);
		cp++;
	    } else {				/* hard-coded field width */
		for (width = 0; ch = *cp, ISDIGIT(ch); cp++) {
		    int     digit = ch - '0';

		    if (width > INT_MAX / 10
			|| (width *= 10) > INT_MAX - digit)
			msg_panic("%s: bad width %d... in %.50s",
				  myname, width, format);
		    width += digit;
		    VSTRING_ADDCH(fmt, ch);
		}
	    }
	    if (*cp == '.') {			/* width/precision separator */
		VSTRING_ADDCH(fmt, *cp++);
		if (*cp == '*') {		/* dynamic precision */
		    prec = va_arg(ap, int);
		    if (prec < 0) {
			msg_warn("%s: bad precision %d in %.50s",
				 myname, prec, format);
			prec = -1;
		    } else
			VSTRING_ADDNUM(fmt, prec);
		    cp++;
		} else {			/* hard-coded precision */
		    for (prec = 0; ch = *cp, ISDIGIT(ch); cp++) {
			int     digit = ch - '0';

			if (prec > INT_MAX / 10
			    || (prec *= 10) > INT_MAX - digit)
			    msg_panic("%s: bad precision %d... in %.50s",
				      myname, prec, format);
			prec += digit;
			VSTRING_ADDCH(fmt, ch);
		    }
		}
	    } else {
		prec = -1;
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
		if (long_flag)
		    msg_panic("%s: %%l%c is not supported", myname, *cp);
		s = va_arg(ap, char *);
		if (prec >= 0 || (width > 0 && width > strlen(s))) {
		    VBUF_SNPRINTF(bp, (width > prec ? width : prec) + INT_SPACE,
				  vstring_str(fmt), s);
		} else {
		    VBUF_STRCAT(bp, s);
		}
		break;
	    case 'c':				/* integral-valued argument */
		if (long_flag)
		    msg_panic("%s: %%l%c is not supported", myname, *cp);
		/* FALLTHROUGH */
	    case 'd':
	    case 'u':
	    case 'o':
	    case 'x':
	    case 'X':
		if (long_flag)
		    VBUF_SNPRINTF(bp, (width > prec ? width : prec) + INT_SPACE,
				  vstring_str(fmt), va_arg(ap, long));
		else
		    VBUF_SNPRINTF(bp, (width > prec ? width : prec) + INT_SPACE,
				  vstring_str(fmt), va_arg(ap, int));
		break;
	    case 'e':				/* float-valued argument */
	    case 'f':
	    case 'g':
		/* C99 *printf ignore the 'l' modifier. */
		VBUF_SNPRINTF(bp, (width > prec ? width : prec) + DBL_SPACE,
			      vstring_str(fmt), va_arg(ap, double));
		break;
	    case 'm':
		/* Ignore the 'l' modifier, width and precision. */
		VBUF_STRCAT(bp, saved_errno ?
			    strerror(saved_errno) : "Application error");
		break;
	    case 'p':
		if (long_flag)
		    msg_panic("%s: %%l%c is not supported", myname, *cp);
		VBUF_SNPRINTF(bp, (width > prec ? width : prec) + PTR_SPACE,
			      vstring_str(fmt), va_arg(ap, char *));
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

#ifdef TEST
#include <argv.h>
#include <msg_vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>

int     main(int argc, char **argv)
{
    VSTRING *ibuf = vstring_alloc(100);

    msg_vstream_init(argv[0], VSTREAM_ERR);

    while (vstring_fgets_nonl(ibuf, VSTREAM_IN)) {
	ARGV   *args = argv_split(vstring_str(ibuf), CHARS_SPACE);
	char   *cp;

	if (args->argc == 0 || *(cp = args->argv[0]) == '#') {
	     /* void */ ;
	} else if (args->argc != 2 || *cp != '%') {
	    msg_warn("usage: format number");
	} else {
	    char   *fmt = cp++;
	    int     lflag;

	    /* Determine the vstring_sprintf() argument type. */
	    cp += strspn(cp, "+-*0123456789.");
	    if ((lflag = (*cp == 'l')) != 0)
		cp++;
	    if (cp[1] != 0) {
		msg_warn("bad format: \"%s\"", fmt);
	    } else {
		VSTRING *obuf = vstring_alloc(1);
		char   *val = args->argv[1];

		/* Test the worst-case memory allocation. */
#ifdef CA_VSTRING_CTL_EXACT
		vstring_ctl(obuf, CA_VSTRING_CTL_EXACT, CA_VSTRING_CTL_END);
#endif
		switch (*cp) {
		case 'c':
		case 'd':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
		    if (lflag)
			vstring_sprintf(obuf, fmt, atol(val));
		    else
			vstring_sprintf(obuf, fmt, atoi(val));
		    msg_info("\"%s\"", vstring_str(obuf));
		    break;
		case 's':
		    vstring_sprintf(obuf, fmt, val);
		    msg_info("\"%s\"", vstring_str(obuf));
		    break;
		case 'f':
		case 'g':
		    vstring_sprintf(obuf, fmt, atof(val));
		    msg_info("\"%s\"", vstring_str(obuf));
		    break;
		default:
		    msg_warn("bad format: \"%s\"", fmt);
		    break;
		}
		vstring_free(obuf);
	    }
	}
	argv_free(args);
    }
    vstring_free(ibuf);
    return (0);
}

#endif
