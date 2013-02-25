/*	$NetBSD: ip_match.c,v 1.1.1.1.12.1 2013/02/25 00:27:32 tls Exp $	*/

/*++
/* NAME
/*	ip_match 3
/* SUMMARY
/*	IP address pattern matching
/* SYNOPSIS
/*	#include <ip_match.h>
/*
/*	char	*ip_match_parse(byte_codes, pattern)
/*	VSTRING	*byte_codes;
/*	char	*pattern;
/*
/*	char	*ip_match_save(byte_codes)
/*	const VSTRING *byte_codes;
/*
/*	int	ip_match_execute(byte_codes, addr_bytes)
/*	cost char *byte_codes;
/*	const char *addr_bytes;
/*
/*	char	*ip_match_dump(printable, byte_codes)
/*	VSTRING	*printable;
/*	const char *byte_codes;
/* DESCRIPTION
/*	This module supports IP address pattern matching. See below
/*	for a description of the supported address pattern syntax.
/*
/*	This implementation aims to minimize the cost of encoding
/*	the pattern in internal form, while still providing good
/*	matching performance in the typical case.   The first byte
/*	of an encoded pattern specifies the expected address family
/*	(for example, AF_INET); other details of the encoding are
/*	private and are subject to change.
/*
/*	ip_match_parse() converts the user-specified pattern to
/*	internal form. The result value is a null pointer in case
/*	of success, or a pointer into the byte_codes buffer with a
/*	detailed problem description.
/*
/*	ip_match_save() saves the result from ip_match_parse() for
/*	longer-term usage. The result should be passed to myfree().
/*
/*	ip_match_execute() matches a binary network in addr_bytes
/*	against a byte-code array in byte_codes. It is an error to
/*	use different address families for the byte_codes and addr_bytes
/*	arguments (the first byte-code value contains the expected
/*	address family).  The result is non-zero in case of success.
/*
/*	ip_match_dump() produces an ASCII dump of a byte-code array.
/*	The dump is supposed to be identical to the input pattern
/*	modulo upper/lower case or leading nulls with IPv6).  This
/*	function is primarily a debugging aid.
/*
/*	Arguments
/* .IP addr_bytes
/*	Binary network address in network-byte order.
/* .IP byte_codes
/*	Byte-code array produced by ip_match_parse().
/* .IP pattern
/*	Human-readable address pattern.
/* .IP printable
/*	storage for ASCII dump of a byte-code array.
/* IPV4 PATTERN SYNTAX
/* .ad
/* .fi
/*	An IPv4 address pattern has four fields separated by ".".
/*	Each field is either a decimal number, or a sequence inside
/*	"[]" that contains one or more ";"-separated decimal
/*	numbers or number..number ranges.
/*
/*	Examples of patterns are 1.2.3.4 (matches itself, as one
/*	would expect) and 1.2.3.[2,4,6..8] (matches 1.2.3.2, 1.2.3.4,
/*	1.2.3.6, 1.2.3.7, 1.2.3.8).
/*
/*	Thus, any pattern field can be a sequence inside "[]", but
/*	a "[]" sequence cannot span multiple address fields, and
/*	a pattern field cannot contain both a number and a "[]"
/*	sequence at the same time.
/*
/*	This means that the pattern 1.2.[3.4] is not valid (the
/*	sequence [3.4] cannot span two address fields) and the
/*	pattern 1.2.3.3[6..9] is also not valid (the last field
/*	cannot be both number 3 and sequence [6..9] at the same
/*	time).
/*
/*	The syntax for IPv4 patterns is as follows:
/*
/* .in +5
/*	v4pattern = v4field "." v4field "." v4field "." v4field
/* .br
/*	v4field = v4octet | "[" v4sequence "]"
/* .br
/*	v4octet = any decimal number in the range 0 through 255
/* .br
/*	v4sequence = v4seq_member | v4sequence ";" v4seq_member
/* .br
/*	v4seq_member = v4octet | v4octet ".." v4octet
/* .in
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this
/*	software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/socket.h>
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <ip_match.h>

 /*
  * Token values. The in-band values are also used as byte-code values.
  */
#define IP_MATCH_CODE_OPEN	'['	/* in-band */
#define IP_MATCH_CODE_CLOSE	']'	/* in-band */
#define IP_MATCH_CODE_OVAL	'N'	/* in-band */
#define IP_MATCH_CODE_RANGE	'R'	/* in-band */
#define IP_MATCH_CODE_EOF	'\0'	/* in-band */
#define IP_MATCH_CODE_ERR	256	/* out-of-band */

 /*
  * SLMs.
  */
#define STR	vstring_str
#define LEN	VSTRING_LEN

/* ip_match_save - make longer-term copy of byte code */

char   *ip_match_save(const VSTRING *byte_codes)
{
    char   *dst;

    dst = mymalloc(LEN(byte_codes));
    return (memcpy(dst, STR(byte_codes), LEN(byte_codes)));
}

/* ip_match_dump - byte-code pretty printer */

char   *ip_match_dump(VSTRING *printable, const char *byte_codes)
{
    const char *myname = "ip_match_dump";
    const unsigned char *bp;
    int     octet_count = 0;
    int     ch;

    /*
     * Sanity check. Use different dumping loops for AF_INET and AF_INET6.
     */
    if (*byte_codes != AF_INET)
	msg_panic("%s: malformed byte-code header", myname);

    /*
     * Pretty-print and sanity-check the byte codes. Note: the loops in this
     * code have no auto-increment at the end of the iteration. Instead, each
     * byte-code handler bumps the byte-code pointer appropriately.
     */
    VSTRING_RESET(printable);
    bp = (const unsigned char *) byte_codes + 1;
    for (;;) {

	/*
	 * Simple numeric field.
	 */
	if ((ch = *bp++) == IP_MATCH_CODE_OVAL) {
	    vstring_sprintf_append(printable, "%d", *bp);
	    bp += 1;
	}

	/*
	 * Wild-card numeric field.
	 */
	else if (ch == IP_MATCH_CODE_OPEN) {
	    vstring_sprintf_append(printable, "[");
	    for (;;) {
		/* Numeric range. */
		if ((ch = *bp++) == IP_MATCH_CODE_RANGE) {
		    vstring_sprintf_append(printable, "%d..%d", bp[0], bp[1]);
		    bp += 2;
		}
		/* Number. */
		else if (ch == IP_MATCH_CODE_OVAL) {
		    vstring_sprintf_append(printable, "%d", *bp);
		    bp += 1;
		}
		/* End-of-wildcard. */
		else if (ch == IP_MATCH_CODE_CLOSE) {
		    break;
		}
		/* Corruption. */
		else {
		    msg_panic("%s: unexpected byte code (decimal %d) "
			      "after \"%s\"", myname, ch, STR(printable));
		}
		/* Output the wild-card field separator and repeat the loop. */
		if (*bp != IP_MATCH_CODE_CLOSE)
		    vstring_sprintf_append(printable, ";");
	    }
	    vstring_sprintf_append(printable, "]");
	}

	/*
	 * Corruption.
	 */
	else {
	    msg_panic("%s: unexpected byte code (decimal %d) after \"%s\"",
		      myname, ch, STR(printable));
	}

	/*
	 * Require four octets, not one more, not one less.
	 */
	if (++octet_count == 4) {
	    if (*bp != 0)
		msg_panic("%s: unexpected byte code (decimal %d) after \"%s\"",
			  myname, ch, STR(printable));
	    return (STR(printable));
	}
	if (*bp == 0)
	    msg_panic("%s: truncated byte code after \"%s\"",
		      myname, STR(printable));

	/*
	 * Output the address field separator and repeat the loop.
	 */
	vstring_sprintf_append(printable, ".");
    }
}

/* ip_match_print_code_prefix - printable byte-code prefix */

static char *ip_match_print_code_prefix(const char *byte_codes, size_t len)
{
    static VSTRING *printable = 0;
    const char *fmt;
    const char *bp;

    /*
     * This is primarily for emergency debugging so we don't care about
     * non-reentrancy.
     */
    if (printable == 0)
	printable = vstring_alloc(100);
    else
	VSTRING_RESET(printable);

    /*
     * Use decimal for IPv4 and hexadecimal otherwise, so that address octet
     * values are easy to recognize.
     */
    fmt = (*byte_codes == AF_INET ? "%d " : "%02x ");
    for (bp = byte_codes; bp < byte_codes + len; bp++)
	vstring_sprintf_append(printable, fmt, *(const unsigned char *) bp);

    return (STR(printable));
}

/* ip_match_execute - byte-code matching engine */

int     ip_match_execute(const char *byte_codes, const char *addr_bytes)
{
    const char *myname = "ip_match_execute";
    const unsigned char *bp;
    const unsigned char *ap;
    int     octet_count = 0;
    int     ch;
    int     matched;

    /*
     * Sanity check. Use different execute loops for AF_INET and AF_INET6.
     */
    if (*byte_codes != AF_INET)
	msg_panic("%s: malformed byte-code header (decimal %d)",
		  myname, *(const unsigned char *) byte_codes);

    /*
     * Match the address bytes against the byte codes. Avoid problems with
     * (char -> int) sign extension on architectures with signed characters.
     */
    bp = (const unsigned char *) byte_codes + 1;
    ap = (const unsigned char *) addr_bytes;

    for (octet_count = 0; octet_count < 4; octet_count++, ap++) {

	/*
	 * Simple numeric field.
	 */
	if ((ch = *bp++) == IP_MATCH_CODE_OVAL) {
	    if (*ap == *bp)
		bp += 1;
	    else
		return (0);
	}

	/*
	 * Wild-card numeric field.
	 */
	else if (ch == IP_MATCH_CODE_OPEN) {
	    matched = 0;
	    for (;;) {
		/* Numeric range. */
		if ((ch = *bp++) == IP_MATCH_CODE_RANGE) {
		    if (!matched)
			matched = (*ap >= bp[0] && *ap <= bp[1]);
		    bp += 2;
		}
		/* Number. */
		else if (ch == IP_MATCH_CODE_OVAL) {
		    if (!matched)
			matched = (*ap == *bp);
		    bp += 1;
		}
		/* End-of-wildcard. */
		else if (ch == IP_MATCH_CODE_CLOSE) {
		    break;
		}
		/* Corruption. */
		else {
		    size_t  len = (const char *) bp - byte_codes - 1;

		    msg_panic("%s: unexpected byte code (decimal %d) "
			      "after \"%s\"", myname, ch,
			      ip_match_print_code_prefix(byte_codes, len));
		}
	    }
	    if (matched == 0)
		return (0);
	}

	/*
	 * Corruption.
	 */
	else {
	    size_t  len = (const char *) bp - byte_codes - 1;

	    msg_panic("%s: unexpected byte code (decimal %d) after \"%s\"",
		   myname, ch, ip_match_print_code_prefix(byte_codes, len));
	}
    }
    return (1);
}

/* ip_match_next_token - carve out the next token from input pattern */

static int ip_match_next_token(char **pstart, char **psaved_start, int *poval)
{
    unsigned char *cp;
    int     oval;			/* octet value */
    int     type;			/* token value */

    /*
     * Return a literal, error, or EOF token. Update the read pointer to the
     * start of the next token or leave it at the string terminator.
     */
#define IP_MATCH_RETURN_TOK(next, type) \
    do { *pstart = (char *) (next); return (type); } while (0)

    /*
     * Return a token that contains an IPv4 address octet value.
     */
#define IP_MATCH_RETURN_TOK_VAL(next, type, oval) do { \
	*poval = (oval); IP_MATCH_RETURN_TOK((next), type); \
    } while (0)

    /*
     * Light-weight tokenizer. Each result is an IPv4 address octet value, a
     * literal character value, error, or EOF.
     */
    *psaved_start = *pstart;
    cp = (unsigned char *) *pstart;
    if (ISDIGIT(*cp)) {
	oval = *cp - '0';
	type = IP_MATCH_CODE_OVAL;
	for (cp += 1; ISDIGIT(*cp); cp++) {
	    oval *= 10;
	    oval += *cp - '0';
	    if (oval > 255)
		type = IP_MATCH_CODE_ERR;
	}
	IP_MATCH_RETURN_TOK_VAL(cp, type, oval);
    } else {
	IP_MATCH_RETURN_TOK(*cp ? cp + 1 : cp, *cp);
    }
}

/* ipmatch_print_parse_error - formatted parsing error, with context */

static void PRINTFLIKE(5, 6) ipmatch_print_parse_error(VSTRING *reply,
						               char *start,
						               char *here,
						               char *next,
						        const char *fmt,...)
{
    va_list ap;
    int     start_width;
    int     here_width;

    /*
     * Format the error type.
     */
    va_start(ap, fmt);
    vstring_vsprintf(reply, fmt, ap);
    va_end(ap);

    /*
     * Format the error context. The syntax is complex enough that it is
     * worth the effort to precisely indicate what input is in error.
     * 
     * XXX Workaround for %.*s to avoid output when a zero width is specified.
     */
    if (start != 0) {
	start_width = here - start;
	here_width = next - here;
	vstring_sprintf_append(reply, " at \"%.*s>%.*s<%s\"",
			       start_width, start_width == 0 ? "" : start,
			     here_width, here_width == 0 ? "" : here, next);
    }
}

/* ip_match_parse - parse an entire wild-card address pattern */

char   *ip_match_parse(VSTRING *byte_codes, char *pattern)
{
    int     octet_count;
    char   *saved_cp;
    char   *cp;
    int     token_type;
    int     look_ahead;
    int     oval;
    int     saved_oval;

    /*
     * Simplify this if we change to {} for wildcard notation.
     */
#define FIND_TERMINATOR(start, cp) do { \
	int _level = 0; \
	for (cp = (start) ; *cp; cp++) { \
	    if (*cp == '[') _level++; \
	    if (*cp != ']') continue; \
	    if (--_level == 0) break; \
	} \
    } while (0)

    /*
     * Strip [] around the entire pattern.
     */
    if (*pattern == '[') {
	FIND_TERMINATOR(pattern, cp);
	if (cp[0] == 0) {
	    vstring_sprintf(byte_codes, "missing \"]\" character");
	    return (STR(byte_codes));
	}
	if (cp[1] == 0) {
	    *cp = 0;
	    pattern += 1;
	}
    }

    /*
     * Sanity check. In this case we can't show any error context.
     */
    if (*pattern == 0) {
	vstring_sprintf(byte_codes, "empty address pattern");
	return (STR(byte_codes));
    }

    /*
     * Simple parser with on-the-fly encoding. For now, IPv4 support only.
     * Use different parser loops for IPv4 and IPv6.
     */
    VSTRING_RESET(byte_codes);
    VSTRING_ADDCH(byte_codes, AF_INET);
    octet_count = 0;
    cp = pattern;

    /*
     * Require four address fields separated by ".", each field containing a
     * numeric octet value or a sequence inside []. The loop head has no test
     * and does not step the loop variable. The tokenizer advances the loop
     * variable, and the loop termination logic is inside the loop.
     */
    for (;;) {
	switch (token_type = ip_match_next_token(&cp, &saved_cp, &oval)) {

	    /*
	     * Numeric address field.
	     */
	case IP_MATCH_CODE_OVAL:
	    VSTRING_ADDCH(byte_codes, IP_MATCH_CODE_OVAL);
	    VSTRING_ADDCH(byte_codes, oval);
	    break;

	    /*
	     * Wild-card address field.
	     */
	case IP_MATCH_CODE_OPEN:
	    VSTRING_ADDCH(byte_codes, IP_MATCH_CODE_OPEN);
	    /* Require ";"-separated numbers or numeric ranges. */
	    for (;;) {
		token_type = ip_match_next_token(&cp, &saved_cp, &oval);
		if (token_type == IP_MATCH_CODE_OVAL) {
		    saved_oval = oval;
		    look_ahead = ip_match_next_token(&cp, &saved_cp, &oval);
		    /* Numeric range. */
		    if (look_ahead == '.') {
			/* Brute-force parsing. */
			if (ip_match_next_token(&cp, &saved_cp, &oval) == '.'
			    && ip_match_next_token(&cp, &saved_cp, &oval)
			    == IP_MATCH_CODE_OVAL
			    && saved_oval <= oval) {
			    VSTRING_ADDCH(byte_codes, IP_MATCH_CODE_RANGE);
			    VSTRING_ADDCH(byte_codes, saved_oval);
			    VSTRING_ADDCH(byte_codes, oval);
			    look_ahead =
				ip_match_next_token(&cp, &saved_cp, &oval);
			} else {
			    ipmatch_print_parse_error(byte_codes, pattern,
						      saved_cp, cp,
						      "numeric range error");
			    return (STR(byte_codes));
			}
		    }
		    /* Single number. */
		    else {
			VSTRING_ADDCH(byte_codes, IP_MATCH_CODE_OVAL);
			VSTRING_ADDCH(byte_codes, saved_oval);
		    }
		    /* Require ";" or end-of-wildcard. */
		    token_type = look_ahead;
		    if (token_type == ';') {
			continue;
		    } else if (token_type == IP_MATCH_CODE_CLOSE) {
			break;
		    } else {
			ipmatch_print_parse_error(byte_codes, pattern,
						  saved_cp, cp,
						  "need \";\" or \"%c\"",
						  IP_MATCH_CODE_CLOSE);
			return (STR(byte_codes));
		    }
		} else {
		    ipmatch_print_parse_error(byte_codes, pattern, saved_cp, cp,
					      "need decimal number 0..255");
		    return (STR(byte_codes));
		}
	    }
	    VSTRING_ADDCH(byte_codes, IP_MATCH_CODE_CLOSE);
	    break;

	    /*
	     * Invalid field.
	     */
	default:
	    ipmatch_print_parse_error(byte_codes, pattern, saved_cp, cp,
				      "need decimal number 0..255 or \"%c\"",
				      IP_MATCH_CODE_OPEN);
	    return (STR(byte_codes));
	}
	octet_count += 1;

	/*
	 * Require four address fields. Not one more, not one less.
	 */
	if (octet_count == 4) {
	    if (*cp != 0) {
		(void) ip_match_next_token(&cp, &saved_cp, &oval);
		ipmatch_print_parse_error(byte_codes, pattern, saved_cp, cp,
					  "garbage after pattern");
		return (STR(byte_codes));
	    }
	    VSTRING_ADDCH(byte_codes, 0);
	    return (0);
	}

	/*
	 * Require "." before the next address field.
	 */
	if (ip_match_next_token(&cp, &saved_cp, &oval) != '.') {
	    ipmatch_print_parse_error(byte_codes, pattern, saved_cp, cp,
				      "need \".\"");
	    return (STR(byte_codes));
	}
    }
}

#ifdef TEST

 /*
  * Dummy main program for regression tests.
  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>

int     main(int argc, char **argv)
{
    VSTRING *byte_codes = vstring_alloc(100);
    VSTRING *line_buf = vstring_alloc(100);
    char   *bufp;
    char   *err;
    char   *user_pattern;
    char   *user_address;
    int     echo_input = !isatty(0);

    /*
     * Iterate over the input stream. The input format is a pattern, followed
     * by optional addresses to match against.
     */
    while (vstring_fgets_nonl(line_buf, VSTREAM_IN)) {
	bufp = STR(line_buf);
	if (echo_input) {
	    vstream_printf("> %s\n", bufp);
	    vstream_fflush(VSTREAM_OUT);
	}
	if (*bufp == '#')
	    continue;
	if ((user_pattern = mystrtok(&bufp, " \t")) == 0)
	    continue;

	/*
	 * Parse and dump the pattern.
	 */
	if ((err = ip_match_parse(byte_codes, user_pattern)) != 0) {
	    vstream_printf("Error: %s\n", err);
	} else {
	    vstream_printf("Code: %s\n",
			   ip_match_dump(line_buf, STR(byte_codes)));
	}
	vstream_fflush(VSTREAM_OUT);

	/*
	 * Match the optional patterns.
	 */
	while ((user_address = mystrtok(&bufp, " \t")) != 0) {
	    struct in_addr netw_addr;

	    switch (inet_pton(AF_INET, user_address, &netw_addr)) {
	    case 1:
		vstream_printf("Match %s: %s\n", user_address,
			       ip_match_execute(STR(byte_codes),
						(char *) &netw_addr.s_addr) ?
			       "yes" : "no");
		break;
	    case 0:
		vstream_printf("bad address syntax: %s\n", user_address);
		break;
	    case -1:
		vstream_printf("%s: %m\n", user_address);
		break;
	    }
	    vstream_fflush(VSTREAM_OUT);
	}
    }
    vstring_free(line_buf);
    vstring_free(byte_codes);
    exit(0);
}

#endif
