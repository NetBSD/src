/*	$NetBSD: smtp_reply_footer.c,v 1.3 2020/03/18 19:05:16 christos Exp $	*/

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
/*	const char *template;
/*	const char *filter;
/*	const char *(*lookup) (const char *name, void *context);
/*	void	*context;
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
/*	The two-character sequence "\c" at the start of the template
/*	suppresses the line break between the reply text and the
/*	template text.
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
/*	smtp_reply_footer() returns 0 upon success, -1 if the existing
/*	reply text is malformed, -2 in the case of a template macro
/*	parsing error (an undefined macro value is not an error).
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>

/* Global library. */

#include <dsn_util.h>
#include <smtp_reply_footer.h>

/* SLMs. */

#define STR	vstring_str

int     smtp_reply_footer(VSTRING *buffer, ssize_t start,
			          const char *template,
			          const char *filter,
			          MAC_EXP_LOOKUP_FN lookup,
			          void *context)
{
    const char *myname = "smtp_reply_footer";
    char   *cp;
    char   *next;
    char   *end;
    ssize_t dsn_len;			/* last status code length */
    ssize_t dsn_offs = -1;		/* last status code offset */
    int     crlf_at_end = 0;
    ssize_t reply_code_offs = -1;	/* last SMTP reply code offset */
    ssize_t reply_patch_undo_len;	/* length without final CRLF */
    int     mac_expand_error = 0;
    int     line_added;
    char   *saved_template;

    /*
     * Sanity check.
     */
    if (start < 0 || start > VSTRING_LEN(buffer))
	msg_panic("%s: bad start: %ld", myname, (long) start);
    if (*template == 0)
	msg_panic("%s: empty template", myname);

    /*
     * Scan the original response without making changes. If the response is
     * not what we expect, report an error. Otherwise, remember the offset of
     * the last SMTP reply code.
     */
    for (cp = STR(buffer) + start, end = cp + strlen(cp);;) {
	if (!ISDIGIT(cp[0]) || !ISDIGIT(cp[1]) || !ISDIGIT(cp[2])
	    || (cp[3] != ' ' && cp[3] != '-'))
	    return (-1);
	reply_code_offs = cp - STR(buffer);
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
    if (reply_code_offs < 0)
	return (-1);

    /*
     * Truncate text after the first null, and truncate the trailing CRLF.
     */
    if (next < vstring_end(buffer))
	vstring_truncate(buffer, next - STR(buffer));
    reply_patch_undo_len = VSTRING_LEN(buffer);

    /*
     * Append the footer text one line at a time. Caution: before we append
     * parts from the buffer to itself, we must extend the buffer first,
     * otherwise we would have a dangling pointer "read" bug.
     * 
     * XXX mac_expand() has no template length argument, so we must
     * null-terminate the template in the middle.
     */
    dsn_offs = reply_code_offs + 4;
    dsn_len = dsn_valid(STR(buffer) + dsn_offs);
    line_added = 0;
    saved_template = mystrdup(template);
    for (cp = saved_template, end = cp + strlen(cp);;) {
	if ((next = strstr(cp, "\\n")) != 0) {
	    *next = 0;
	} else {
	    next = end;
	}
	if (cp == saved_template && strncmp(cp, "\\c", 2) == 0) {
	    /* Handle \c at start of template. */
	    cp += 2;
	} else {
	    /* Append a clone of the SMTP reply code. */
	    vstring_strcat(buffer, "\r\n");
	    VSTRING_SPACE(buffer, 3);
	    vstring_strncat(buffer, STR(buffer) + reply_code_offs, 3);
	    vstring_strcat(buffer, next != end ? "-" : " ");
	    /* Append a clone of the optional enhanced status code. */
	    if (dsn_len > 0) {
		VSTRING_SPACE(buffer, dsn_len);
		vstring_strncat(buffer, STR(buffer) + dsn_offs, dsn_len);
		vstring_strcat(buffer, " ");
	    }
	    line_added = 1;
	}
	/* Append one line of footer text. */
	mac_expand_error = (mac_expand(buffer, cp, MAC_EXP_FLAG_APPEND, filter,
				       lookup, context) & MAC_PARSE_ERROR);
	if (mac_expand_error)
	    break;
	if (next < end) {
	    cp = next + 2;
	} else
	    break;
    }
    myfree(saved_template);
    /* Discard appended text after error, or finalize the result. */
    if (mac_expand_error) {
	vstring_truncate(buffer, reply_patch_undo_len);
	VSTRING_TERMINATE(buffer);
    } else if (line_added > 0) {
	STR(buffer)[reply_code_offs + 3] = '-';
    }
    /* Restore CRLF at end. */
    if (crlf_at_end)
	vstring_strcat(buffer, "\r\n");
    return (mac_expand_error ? -2 : 0);
}

#ifdef TEST

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <msg.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <msg_vstream.h>

struct test_case {
    const char *title;
    const char *orig_reply;
    const char *template;
    const char *filter;
    int     expected_status;
    const char *expected_reply;
};

#define NO_FILTER	((char *) 0)
#define NO_TEMPLATE	"NO_TEMPLATE"
#define NO_ERROR	(0)
#define BAD_SMTP	(-1)
#define BAD_MACRO	(-2)

static const struct test_case test_cases[] = {
    {"missing reply", "", NO_TEMPLATE, NO_FILTER, BAD_SMTP, 0},
    {"long smtp_code", "1234 foo", NO_TEMPLATE, NO_FILTER, BAD_SMTP, 0},
    {"short smtp_code", "12 foo", NO_TEMPLATE, NO_FILTER, BAD_SMTP, 0},
    {"good+bad smtp_code", "321 foo\r\n1234 foo", NO_TEMPLATE, NO_FILTER, BAD_SMTP, 0},
    {"1-line no dsn", "550 Foo", "\\c footer", NO_FILTER, NO_ERROR, "550 Foo footer"},
    {"1-line no dsn", "550 Foo", "Bar", NO_FILTER, NO_ERROR, "550-Foo\r\n550 Bar"},
    {"2-line no dsn", "550-Foo\r\n550 Bar", "Baz", NO_FILTER, NO_ERROR, "550-Foo\r\n550-Bar\r\n550 Baz"},
    {"1-line with dsn", "550 5.1.1 Foo", "Bar", NO_FILTER, NO_ERROR, "550-5.1.1 Foo\r\n550 5.1.1 Bar"},
    {"2-line with dsn", "550-5.1.1 Foo\r\n450 4.1.1 Bar", "Baz", NO_FILTER, NO_ERROR, "550-5.1.1 Foo\r\n450-4.1.1 Bar\r\n450 4.1.1 Baz"},
    {"bad macro", "220 myhostname", "\\c ${whatever", NO_FILTER, BAD_MACRO, 0},
    {"bad macroCRLF", "220 myhostname\r\n", "\\c ${whatever", NO_FILTER, BAD_MACRO, 0},
    {"good macro", "220 myhostname", "\\c $whatever", NO_FILTER, NO_ERROR, "220 myhostname DUMMY"},
    {"good macroCRLF", "220 myhostname\r\n", "\\c $whatever", NO_FILTER, NO_ERROR, "220 myhostname DUMMY\r\n"},
    0,
};

static const char *lookup(const char *name, int unused_mode, void *context)
{
    return "DUMMY";
}

int     main(int argc, char **argv)
{
    const struct test_case *tp;
    int     status;
    VSTRING *buf = vstring_alloc(10);
    void   *context = 0;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    for (tp = test_cases; tp->title != 0; tp++) {
	vstring_strcpy(buf, tp->orig_reply);
	status = smtp_reply_footer(buf, 0, tp->template, tp->filter,
				   lookup, context);
	if (status != tp->expected_status) {
	    msg_warn("test \"%s\": status %d, expected %d",
		     tp->title, status, tp->expected_status);
	} else if (status < 0 && strcmp(STR(buf), tp->orig_reply) != 0) {
	    msg_warn("test \"%s\": result \"%s\", expected \"%s\"",
		     tp->title, STR(buf), tp->orig_reply);
	} else if (status == 0 && strcmp(STR(buf), tp->expected_reply) != 0) {
	    msg_warn("test \"%s\": result \"%s\", expected \"%s\"",
		     tp->title, STR(buf), tp->expected_reply);
	} else {
	    msg_info("test \"%s\": pass", tp->title);
	}
    }
    vstring_free(buf);
    exit(0);
}

#endif
