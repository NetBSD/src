/*++
/* NAME
/*	mail_print 3
/* SUMMARY
/*	intra-mail system write routine
/* SYNOPSIS
/*	#include <mail_proto.h>
/*
/*	int	mail_print(stream, format, ...)
/*	VSTREAM *stream;
/*	const char *format;
/*
/*	int	mail_vprint(stream, format, ap)
/*	VSTREAM *stream;
/*	const char *format;
/*	va_list	ap;
/*
/*	void	mail_print_register(letter, name, print_fn)
/*	int	letter;
/*	const char *name;
/*	void	(*print_fn)(VSTREAM *stream, const char *data);
/* DESCRIPTION
/*	mail_print() prints one or more null-delimited strings to
/*	the named stream, each string being converted according to the
/*	contents of the \fIformat\fR argument.
/*
/*	mail_vprint() provides an alternative interface.
/*
/*	mail_print_register() registers the named print function for
/*	the specified letter, for the named type.
/*
/*	Arguments:
/* .IP stream
/*	The stream to print to.
/* .IP format
/*	Format string, which is interpreted as follows:
/* .RS
/* .IP "white space"
/*	White space in the format string is ignored.
/* .IP %s
/*	The corresponding argument has type (char *).
/* .IP %d
/*	The corresponding argument has type (int).
/* .IP %ld
/*	The corresponding argument has type (long).
/* .IP %letter
/*	Call the print routine that was registered for the specified letter.
/* .PP
/*	Anything else in a format string is a fatal error.
/* .RE
/* .IP letter
/*	Format letter that is bound to the \fIprint_fn\fR print function.
/* .IP name
/*	Descriptive string for verbose logging.
/* .IP print_fn
/*	A print function. It takes as arguments:
/* .RS
/* .IP stream
/*	The stream to print to.
/* .IP data
/*	A generic data pointer. It is up to the function
/*	to do any necessary casts to the data-specific type.
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
#include <mymalloc.h>
#include <vstream.h>

/* Global library. */

#include "mail_proto.h"

 /*
  * Provision for the user to register type-specific scanners for
  * applications with unusual requirements.
  */
typedef struct {
    int     letter;
    char   *name;
    MAIL_PRINT_FN print_fn;
} MAIL_PRINT;

MAIL_PRINT *mail_print_tab = 0;
int     mail_print_tablen = 0;

/* mail_print_register - register printer function */

void    mail_print_register(int letter, const char *name, MAIL_PRINT_FN print_fn)
{
    MAIL_PRINT *tp;

    for (tp = 0; tp < mail_print_tab + mail_print_tablen; tp++)
	if (tp->letter == letter)
	    msg_panic("mail_print_register: redefined letter: %c", letter);

    /*
     * Tight fit allocation. We're not registering lots of characters.
     */
    if (mail_print_tab == 0) {
	mail_print_tablen = 1;
	mail_print_tab = (MAIL_PRINT *)
	    mymalloc(sizeof(*mail_print_tab) * mail_print_tablen);
    } else {
	mail_print_tablen++;
	mail_print_tab = (MAIL_PRINT *)
	    myrealloc((char *) mail_print_tab,
		      sizeof(*mail_print_tab) * mail_print_tablen);
    }
    tp = mail_print_tab + mail_print_tablen - 1;
    tp->letter = letter;
    tp->name = mystrdup(name);
    tp->print_fn = print_fn;
}

/* mail_vprint - print null-delimited data to stream */

int     mail_vprint(VSTREAM *stream, const char *fmt, va_list ap)
{
    const char *cp;
    int     lflag;
    char   *sval;
    int     ival;
    long    lval;
    int     error;
    MAIL_PRINT *tp;

    for (cp = fmt; (error = vstream_ferror(stream)) == 0 && *cp != 0; cp++) {
	if (ISSPACE(*cp))
	    continue;
	if (*cp != '%')
	    msg_fatal("mail_vprint: bad format: %.*s>%c<%s",
		      (int) (cp - fmt), fmt, *cp, cp + 1);
	if ((lflag = (*++cp == 'l')) != 0)
	    cp++;

	switch (*cp) {
	case 's':
	    sval = va_arg(ap, char *);
	    if (msg_verbose)
		msg_info("print string: %s", sval);
	    vstream_fputs(sval, stream);
	    VSTREAM_PUTC(0, stream);
	    break;
	case 'd':
	    if (lflag) {
		lval = va_arg(ap, long);
		if (msg_verbose)
		    msg_info("print long: %ld", lval);
		vstream_fprintf(stream, "%ld", lval);
	    } else {
		ival = va_arg(ap, int);
		if (msg_verbose)
		    msg_info("print int: %d", ival);
		vstream_fprintf(stream, "%d", ival);
	    }
	    VSTREAM_PUTC(0, stream);
	    break;
	default:
	    for (tp = mail_print_tab; tp < mail_print_tab + mail_print_tablen; tp++)
		if (tp->letter == *cp) {
		    if (msg_verbose)
			msg_info("print %s", tp->name);
		    tp->print_fn(stream, va_arg(ap, char *));
		    break;
		}
	    if (tp >= mail_print_tab + mail_print_tablen)
		msg_fatal("mail_vprint: bad format: %.*s>%c<%s",
			  (int) (cp - fmt), fmt, *cp, cp + 1);
	}
    }
    return (error);
}

/* mail_print - print null-delimited data to stream */

int     mail_print(VSTREAM *stream, const char *fmt,...)
{
    int     status;
    va_list ap;

    va_start(ap, fmt);
    status = mail_vprint(stream, fmt, ap);
    va_end(ap);
    return (status);
}
