/*++
/* NAME
/*	mail_scan 3
/* SUMMARY
/*	intra-mail read routine
/* SYNOPSIS
/*	#include <mail_proto.h>
/*
/*	int	mail_scan(stream, format, ...)
/*	VSTREAM *stream;
/*	const char *format;
/*
/*	void	mail_scan_register(letter, name, scan_fn)
/*	int	letter;
/*	const char *name;
/*	int	(*scan_fn)(const char *string, char *result);
/* DESCRIPTION
/*	mail_scan() reads one or more null-delimited strings from
/*	the named stream, each string being converted according to the
/*	contents of the \fIformat\fR argument.
/*	The result value is the number of successful conversions.
/*
/*	mail_scan_register() registers an input conversion function
/*	for the specified letter.
/*
/*	Arguments:
/* .IP stream
/*	Stream to read from.
/* .IP format
/*	Format string, which is interpreted as follows:
/* .RS
/* .IP "white space"
/*	White space in the format string is ignored.
/* .IP %s
/*	The corresponding argument has type (VSTRING *).
/* .IP %d
/*	The corresponding argument has type (int *).
/* .IP %ld
/*	The corresponding argument has type (long *).
/* .IP %letter
/*	Call the input conversion routine that was registered for
/*	the specified letter.
/* .PP
/*	Anything else in a format string is a fatal error.
/* .RE
/* .IP letter
/*	Format letter that is bound to the \fIscan_fn\fR input
/*	conversion function.
/* .IP name
/*	Descriptive string for verbose logging.
/* .IP scan_fn
/*	An input conversion function. It takes as arguments:
/* .RS
/* .IP string
/*	The null-terminated string to be converted.
/* .IP result
/*	A character pointer to the result. It is up to the function
/*	to do any necessary casts to the data-specific type.
/* .RE
/* .PP
/*	The function result values are as follows:
/* .RS
/* .IP MAIL_SCAN_ERROR
/*	The expected input could not be read.
/* .IP MAIL_SCAN_DONE
/*	The operation completed successfully.
/* .IP MAIL_SCAN_MORE
/*	More input is expected.
/* .RE
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

#include <sys_defs.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <mymalloc.h>

/* Global library. */

#include "mail_proto.h"

 /*
  * Provision for the user to register type-specific input conversion
  * routines for applications with unusual requirements.
  */
typedef struct {
    int     letter;
    char   *name;
    MAIL_SCAN_FN scanner;
} MAIL_SCAN;

MAIL_SCAN *mail_scan_tab = 0;
int     mail_scan_tablen = 0;

/* mail_scan_register - register scanner function */

void    mail_scan_register(int letter, const char *name, MAIL_SCAN_FN scanner)
{
    MAIL_SCAN *tp;

    for (tp = 0; tp < mail_scan_tab + mail_scan_tablen; tp++)
	if (tp->letter == letter)
	    msg_panic("mail_scan_register: redefined letter: %c", letter);

    /*
     * Tight fit allocation (as in: Robin Hood and the men that wear tights).
     */
    if (mail_scan_tab == 0) {
	mail_scan_tablen = 1;
	mail_scan_tab = (MAIL_SCAN *)
	    mymalloc(sizeof(*mail_scan_tab) * mail_scan_tablen);
    } else {
	mail_scan_tablen++;
	mail_scan_tab = (MAIL_SCAN *)
	    myrealloc((char *) mail_scan_tab,
		      sizeof(*mail_scan_tab) * mail_scan_tablen);
    }
    tp = mail_scan_tab + mail_scan_tablen - 1;
    tp->letter = letter;
    tp->name = mystrdup(name);
    tp->scanner = scanner;
}

/* mail_scan_any - read one null-delimited string from stream */

static int mail_scan_any(VSTREAM *stream, VSTRING *vp, char *what)
{
    if (vstring_fgets_null(vp, stream) == 0) {
	msg_warn("end of input while receiving %s data from service %s",
		 what, VSTREAM_PATH(stream));
	return (-1);
    }
    if (msg_verbose)
	msg_info("mail_scan_any: read %s: %s", what, vstring_str(vp));
    return (0);
}

/* mail_scan_other - read user-defined type from stream */

static int mail_scan_other(VSTREAM *stream, VSTRING *vp,
			           MAIL_SCAN *tp, char *result)
{
    int     ret;

    while ((ret = mail_scan_any(stream, vp, tp->name)) == 0) {
	switch (tp->scanner(vstring_str(vp), result)) {
	case MAIL_SCAN_MORE:
	    break;
	case MAIL_SCAN_DONE:
	    return (0);
	case MAIL_SCAN_ERROR:
	    return (-1);
	}
    }
    return (ret);
}

/* mail_scan_long - read long integer from stream */

static int mail_scan_long(VSTREAM *stream, VSTRING *vp, long *lp)
{
    int     ret;
    char    junk;

    if ((ret = mail_scan_any(stream, vp, "long integer")) == 0) {
	if (sscanf(vstring_str(vp), "%ld%c", lp, &junk) != 1) {
	    msg_warn("mail_scan_long: bad long long: %s", vstring_str(vp));
	    ret = -1;
	}
    }
    return (ret);
}

/* mail_scan_int - read integer from stream */

static int mail_scan_int(VSTREAM *stream, VSTRING *vp, int *lp)
{
    int     ret;
    char    junk;

    if ((ret = mail_scan_any(stream, vp, "integer")) == 0) {
	if (sscanf(vstring_str(vp), "%d%c", lp, &junk) != 1) {
	    msg_warn("mail_scan_int: bad integer: %s", vstring_str(vp));
	    ret = -1;
	}
    }
    return (ret);
}

/* mail_scan - read null-delimited data from stream */

int     mail_scan(VSTREAM *stream, const char *fmt,...)
{
    va_list ap;
    int     count;

    va_start(ap, fmt);
    count = mail_vscan(stream, fmt, ap);
    va_end(ap);
    return (count);
}

/* mail_vscan - read null-delimited data from stream */

int     mail_vscan(VSTREAM *stream, const char *fmt, va_list ap)
{
    const char *cp;
    int     lflag;
    int     count;
    int     error;
    static VSTRING *tmp;
    MAIL_SCAN *tp;

    if (tmp == 0)
	tmp = vstring_alloc(100);

    for (count = 0, error = 0, cp = fmt; error == 0 && *cp != 0; cp++) {
	if (ISSPACE(*cp))
	    continue;
	if (*cp != '%')
	    msg_fatal("mail_scan: bad format: %.*s>%c<%s",
		      (int) (cp - fmt), fmt, *cp, cp + 1);
	if ((lflag = (*++cp == 'l')) != 0)
	    cp++;

	switch (*cp) {
	case 's':
	    error = mail_scan_any(stream, va_arg(ap, VSTRING *), "string");
	    break;
	case 'd':
	    if (lflag)
		error = mail_scan_long(stream, tmp, va_arg(ap, long *));
	    else
		error = mail_scan_int(stream, tmp, va_arg(ap, int *));
	    break;
	default:
	    for (tp = mail_scan_tab; tp < mail_scan_tab + mail_scan_tablen; tp++)
		if (tp->letter == *cp) {
		    if (msg_verbose)
			msg_info("mail_scan: %s", tp->name);
		    error = mail_scan_other(stream, tmp, tp, va_arg(ap, char *));
		    break;
		}
	    if (tp >= mail_scan_tab + mail_scan_tablen)
		msg_fatal("mail_scan: bad format: %.*s>%c<%s",
			  (int) (cp - fmt), fmt, *cp, cp + 1);
	}
	if (error == 0)
	    count++;
    }
    return (count);
}
