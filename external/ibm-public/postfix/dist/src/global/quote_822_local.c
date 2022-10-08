/*	$NetBSD: quote_822_local.c,v 1.3 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	quote_822_local 3
/* SUMMARY
/*	quote local part of mailbox
/* SYNOPSIS
/*	#include <quote_822_local.h>
/*
/*	VSTRING	*quote_822_local(dst, src)
/*	VSTRING	*dst;
/*	const char *src;
/*
/*	VSTRING	*quote_822_local_flags(dst, src, flags)
/*	VSTRING	*dst;
/*	const char *src;
/*	int	flags;
/*
/*	VSTRING	*unquote_822_local(dst, src)
/*	VSTRING	*dst;
/*	const char *src;
/* DESCRIPTION
/*	quote_822_local() quotes the local part of a mailbox and
/*	returns a result that can be used in message headers as
/*	specified by RFC 822 (actually, an 8-bit clean version of
/*	RFC 822). It implements an 8-bit clean version of RFC 822.
/*
/*	quote_822_local_flags() provides finer control.
/*
/*	unquote_822_local() transforms the local part of a mailbox
/*	address to unquoted (internal) form.
/*
/*	Arguments:
/* .IP dst
/*	The result.
/* .IP src
/*	The input address.
/* .IP flags
/*	Bit-wise OR of zero or more of the following.
/* .RS
/* .IP QUOTE_FLAG_8BITCLEAN
/*	In violation with RFCs, treat 8-bit text as ordinary text.
/* .IP QUOTE_FLAG_EXPOSE_AT
/*	In violation with RFCs, treat `@' as an ordinary character.
/* .IP QUOTE_FLAG_APPEND
/*	Append to the result buffer, instead of overwriting it.
/* .IP QUOTE_FLAG_BARE_LOCALPART
/*	The input is a localpart without @domain part.
/* .RE
/* STANDARDS
/*	RFC 822 (ARPA Internet Text Messages)
/* BUGS
/*	The code assumes that the domain is RFC 822 clean.
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

#include <vstring.h>

/* Global library. */

/* Application-specific. */

#include "quote_822_local.h"

/* Local stuff. */

#define YES	1
#define	NO	0

/* is_822_dot_string - is this local-part an rfc 822 dot-string? */

static int is_822_dot_string(const char *local_part, const char *end, int flags)
{
    const char *cp;
    int     ch;

    /*
     * Detect any deviations from a sequence of atoms separated by dots. We
     * could use lookup tables to speed up some of the work, but hey, how
     * large can a local-part be anyway?
     * 
     * RFC 822 expects 7-bit data. Rather than quoting every 8-bit character
     * (and still passing it on as 8-bit data) we leave 8-bit data alone.
     */
    if (local_part == end || local_part[0] == 0 || local_part[0] == '.')
	return (NO);
    for (cp = local_part; cp < end && (ch = *(unsigned char *) cp) != 0; cp++) {
	if (ch == '.' && (cp + 1) < end && cp[1] == '.')
	    return (NO);
	if (ch > 127 && !(flags & QUOTE_FLAG_8BITCLEAN))
	    return (NO);
	if (ch == ' ')
	    return (NO);
	if (ISCNTRL(ch))
	    return (NO);
	if (ch == '(' || ch == ')'
	    || ch == '<' || ch == '>'
	    || (ch == '@' && !(flags & QUOTE_FLAG_EXPOSE_AT)) || ch == ','
	    || ch == ';' || ch == ':'
	    || ch == '\\' || ch == '"'
	    || ch == '[' || ch == ']')
	    return (NO);
    }
    if (cp[-1] == '.')
	return (NO);
    return (YES);
}

/* make_822_quoted_string - make quoted-string from local-part */

static VSTRING *make_822_quoted_string(VSTRING *dst, const char *local_part,
				               const char *end, int flags)
{
    const char *cp;
    int     ch;

    /*
     * Put quotes around the result, and prepend a backslash to characters
     * that need quoting when they occur in a quoted-string.
     */
    VSTRING_ADDCH(dst, '"');
    for (cp = local_part; cp < end && (ch = *(unsigned char *) cp) != 0; cp++) {
	if ((ch > 127 && !(flags & QUOTE_FLAG_8BITCLEAN))
	    || ch == '"' || ch == '\\' || ch == '\r')
	    VSTRING_ADDCH(dst, '\\');
	VSTRING_ADDCH(dst, ch);
    }
    VSTRING_ADDCH(dst, '"');
    return (dst);
}

/* quote_822_local_flags - quote local part of mailbox according to rfc 822 */

VSTRING *quote_822_local_flags(VSTRING *dst, const char *mbox, int flags)
{
    const char *start;			/* first byte of localpart */
    const char *end;			/* first byte after localpart */
    const char *colon;

    /*
     * According to RFC 822, a local-part is a dot-string or a quoted-string.
     * We first see if the local-part is a dot-string. If it is not, we turn
     * it into a quoted-string. Anything else would be too painful. But
     * first, skip over any source route that precedes the local-part.
     */
    if (mbox[0] == '@' && (colon = strchr(mbox, ':')) != 0)
	start = colon + 1;
    else
	start = mbox;
    if ((flags & QUOTE_FLAG_BARE_LOCALPART) != 0
	|| (end = strrchr(start, '@')) == 0)
	end = start + strlen(start);
    if ((flags & QUOTE_FLAG_APPEND) == 0)
	VSTRING_RESET(dst);
    if (is_822_dot_string(start, end, flags)) {
	return (vstring_strcat(dst, mbox));
    } else {
	vstring_strncat(dst, mbox, start - mbox);
	make_822_quoted_string(dst, start, end, flags & QUOTE_FLAG_8BITCLEAN);
	return (vstring_strcat(dst, end));
    }
}

/* unquote_822_local - unquote local part of mailbox according to rfc 822 */

VSTRING *unquote_822_local(VSTRING *dst, const char *mbox)
{
    const char *start;			/* first byte of localpart */
    const char *colon;
    const char *cp;
    int     in_quote = 0;
    const char *bare_at_src;
    int     bare_at_dst_pos = -1;

    /* Don't unquote a routing prefix. Is this still possible? */
    if (mbox[0] == '@' && (colon = strchr(mbox, ':')) != 0) {
	start = colon + 1;
	vstring_strncpy(dst, mbox, start - mbox);
    } else {
	start = mbox;
	VSTRING_RESET(dst);
    }
    /* Locate the last unquoted '@'. */
    for (cp = start; *cp; cp++) {
	if (*cp == '"') {
	    in_quote = !in_quote;
	    continue;
	} else if (*cp == '@') {
	    if (!in_quote) {
		bare_at_dst_pos = VSTRING_LEN(dst);
		bare_at_src = cp;
	    }
	} else if (*cp == '\\') {
	    if (cp[1] == 0)
		continue;
	    cp++;
	}
	VSTRING_ADDCH(dst, *cp);
    }
    /* Don't unquote text after the last unquoted '@'. */
    if (bare_at_dst_pos >= 0) {
	vstring_truncate(dst, bare_at_dst_pos);
	vstring_strcat(dst, bare_at_src);
    } else
	VSTRING_TERMINATE(dst);
    return (dst);
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Read an unquoted address from stdin, and
  * show the quoted and unquoted results. Specify <> to test behavior for an
  * empty unquoted address.
  */
#include <ctype.h>
#include <string.h>

#include <msg.h>
#include <name_mask.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring_vstream.h>

#define STR	vstring_str

int     main(int unused_argc, char **argv)
{
    VSTRING *in = vstring_alloc(100);
    VSTRING *out = vstring_alloc(100);
    char   *cmd;
    char   *bp;
    int     flags;

    while (vstring_fgets_nonl(in, VSTREAM_IN)) {
	bp = STR(in);
	if ((cmd = mystrtok(&bp, CHARS_SPACE)) != 0) {
	    while (ISSPACE(*bp))
		bp++;
	    if (*bp == 0) {
		msg_warn("missing argument");
		continue;
	    }
	    if (strcmp(bp, "<>") == 0)
		bp = "";
	    if (strcmp(cmd, "quote") == 0) {
		quote_822_local(out, bp);
		vstream_printf("'%s' quoted '%s'\n", bp, STR(out));
	    } else if (strcmp(cmd, "quote_with_flags") == 0) {
		if ((cmd = mystrtok(&bp, CHARS_SPACE)) == 0) {
		    msg_warn("missing flags");
		    continue;
		}
		while (ISSPACE(*bp))
		    bp++;
		flags = quote_flags_from_string(cmd);
		quote_822_local_flags(out, bp, flags);
		vstream_printf("'%s' quoted flags=%s '%s'\n",
			       bp, quote_flags_to_string((VSTRING *) 0, flags), STR(out));
	    } else if (strcmp(cmd, "unquote") == 0) {
		unquote_822_local(out, bp);
		vstream_printf("'%s' unquoted '%s'\n", bp, STR(out));
	    } else {
		msg_warn("unknown command: %s", cmd);
	    }
	    vstream_fflush(VSTREAM_OUT);
	}
    }
    vstring_free(in);
    vstring_free(out);
    return (0);
}

#endif
