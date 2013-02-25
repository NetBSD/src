/*	$NetBSD: smtp_reply_footer.c,v 1.1.1.1.12.1 2013/02/25 00:27:19 tls Exp $	*/

/*++
/* NAME
/*	smtp_reply_footer 3
/* SUMMARY
/*	SMTP reply footer text support
/* SYNOPSIS
/*	#include <smtp_reply_footer.h>
/*
/*	int	smtp_reply_footer(buffer, start, template, filter,
/*					lookup, context)
/*	VSTRING	*buffer;
/*	ssize_t	start;
/*	char	*template;
/*	const char *filter;
/*	const char *(*lookup) (const char *name, char *context);
/*	char	*context;
/* DESCRIPTION
/*	smtp_reply_footer() expands a reply template, and appends
/*	the result to an existing reply text.
/*
/*	Arguments:
/* .IP buffer
/*	Result buffer. This should contain a properly formatted
/*	one-line or multi-line SMTP reply, with or without the final
/*	<CR><LF>. The reply code and optional enhanced status code
/*	will be replicated in the footer text.  One space character
/*	after the SMTP reply code is replaced by '-'. If the existing
/*	reply ends in <CR><LF>, the result text will also end in
/*	<CR><LF>.
/* .IP start
/*	The beginning of the SMTP reply that the footer will be
/*	appended to. This supports applications that buffer up
/*	multiple responses in one buffer.
/* .IP template
/*	Template text, with optional $name attributes that will be
/*	expanded. The two-character sequence "\n" is replaced by a
/*	line break followed by a copy of the original SMTP reply
/*	code and optional enhanced status code.
/* .IP filter
/*	The set of characters that are allowed in attribute expansion.
/* .IP lookup
/*	Attribute name/value lookup function. The result value must
/*	be a null for a name that is not found, otherwise a pointer
/*	to null-terminated string.
/* .IP context
/*	Call-back context for the lookup function.
/* SEE ALSO
/*	mac_expand(3) macro expansion
/* DIAGNOSTICS
/*	smtp_reply_footer() returns 0 upon success, -1 if the
/*	existing reply text is malformed.
/*
/*	Fatal errors: memory allocation problem.
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
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>

/* Global library. */

#include <dsn_util.h>
#include <smtp_reply_footer.h>

/* SLMs. */

#define STR	vstring_str

int     smtp_reply_footer(VSTRING *buffer, ssize_t start,
			          char *template,
			          const char *filter,
			          MAC_EXP_LOOKUP_FN lookup,
			          char *context)
{
    const char *myname = "smtp_reply_footer";
    char   *cp;
    char   *next;
    char   *end;
    ssize_t dsn_len;
    int     crlf_at_end = 0;

    /*
     * Sanity check.
     */
    if (start < 0 || start > VSTRING_LEN(buffer))
	msg_panic("%s: bad start: %ld", myname, (long) start);
    if (*template == 0)
	msg_panic("%s: empty template", myname);

    /*
     * Scan and patch the original response. If the response is not what we
     * expect, we stop making changes.
     */
    for (cp = STR(buffer) + start, end = cp + strlen(cp);;) {
	if (!ISDIGIT(cp[0]) || !ISDIGIT(cp[1]) || !ISDIGIT(cp[2])
	    || (cp[3] != ' ' && cp[3] != '-'))
	    return (-1);
	cp[3] = '-';
	if ((next = strstr(cp, "\r\n")) == 0) {
	    next = end;
	    break;
	}
	cp = next + 2;
	if (cp == end) {
	    crlf_at_end = 1;
	    break;
	}
    }

    /*
     * Truncate text after the first null, and truncate the trailing CRLF.
     */
    if (next < vstring_end(buffer))
	vstring_truncate(buffer, next - STR(buffer));

    /*
     * Append the footer text one line at a time. Caution: before we append
     * parts from the buffer to itself, we must extend the buffer first,
     * otherwise we would have a dangling pointer "read" bug.
     */
    dsn_len = dsn_valid(STR(buffer) + start + 4);
    for (cp = template, end = cp + strlen(cp);;) {
	if ((next = strstr(cp, "\\n")) != 0) {
	    *next = 0;
	} else {
	    next = end;
	}
	/* Append a clone of the SMTP reply code. */
	vstring_strcat(buffer, "\r\n");
	VSTRING_SPACE(buffer, 3);
	vstring_strncat(buffer, STR(buffer) + start, 3);
	vstring_strcat(buffer, next != end ? "-" : " ");
	/* Append a clone of the optional enhanced status code. */
	if (dsn_len > 0) {
	    VSTRING_SPACE(buffer, dsn_len);
	    vstring_strncat(buffer, STR(buffer) + start + 4, (int) dsn_len);
	    vstring_strcat(buffer, " ");
	}
	/* Append one line of footer text. */
	mac_expand(buffer, cp, MAC_EXP_FLAG_APPEND, filter, lookup, context);
	if (next < end) {
	    *next = '\\';
	    cp = next + 2;
	} else
	    break;
    }
    if (crlf_at_end)
	vstring_strcat(buffer, "\r\n");
    return (0);
}
