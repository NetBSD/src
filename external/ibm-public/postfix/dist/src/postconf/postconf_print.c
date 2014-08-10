/*	$NetBSD: postconf_print.c,v 1.1.1.1.2.2 2014/08/10 07:12:49 tls Exp $	*/

/*++
/* NAME
/*	postconf_print 3
/* SUMMARY
/*	basic line printing support
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	pcf_print_line(fp, mode, const char *fmt, ...)
/*	VSTREAM	*fp;
/*	int	mode;
/*	const char *fmt;
/* DESCRIPTION
/*	pcf_print_line() formats text, normalized whitespace, and
/*	optionally folds long lines.
/*
/*	Arguments:
/* .IP fp
/*	Output stream.
/* .IP mode
/*	Bit-wise OR of zero or more of the following (other flags
/*	are ignored):
/* .RS
/* .IP PCF_FOLD_LINE
/*	Fold long lines.
/* .RE
/* .IP fmt
/*	Format string.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
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
#include <string.h>
#include <stdarg.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>

/* Application-specific. */

#include <postconf.h>

/* SLMs. */

#define STR(x)	vstring_str(x)

/* pcf_print_line - show line possibly folded, and with normalized whitespace */

void    pcf_print_line(VSTREAM *fp, int mode, const char *fmt,...)
{
    va_list ap;
    static VSTRING *buf = 0;
    char   *start;
    char   *next;
    int     line_len = 0;
    int     word_len;

    /*
     * One-off initialization.
     */
    if (buf == 0)
	buf = vstring_alloc(100);

    /*
     * Format the text.
     */
    va_start(ap, fmt);
    vstring_vsprintf(buf, fmt, ap);
    va_end(ap);

    /*
     * Normalize the whitespace. We don't use the line_wrap() routine because
     * 1) that function does not normalize whitespace between words and 2) we
     * want to normalize whitespace even when not wrapping lines.
     * 
     * XXX Some parameters preserve whitespace: for example, smtpd_banner and
     * smtpd_reject_footer. If we have to preserve whitespace between words,
     * then perhaps readlline() can be changed to canonicalize whitespace
     * that follows a newline.
     */
    for (start = STR(buf); *(start += strspn(start, PCF_SEPARATORS)) != 0; start = next) {
	word_len = strcspn(start, PCF_SEPARATORS);
	if (*(next = start + word_len) != 0)
	    *next++ = 0;
	if (word_len > 0 && line_len > 0) {
	    if ((mode & PCF_FOLD_LINE) == 0
		|| line_len + word_len < PCF_LINE_LIMIT) {
		vstream_fputs(" ", fp);
		line_len += 1;
	    } else {
		vstream_fputs("\n" PCF_INDENT_TEXT, fp);
		line_len = PCF_INDENT_LEN;
	    }
	}
	vstream_fputs(start, fp);
	line_len += word_len;
    }
    vstream_fputs("\n", fp);
}
