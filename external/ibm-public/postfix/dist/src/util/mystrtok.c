/*	$NetBSD: mystrtok.c,v 1.3.2.1 2023/12/25 12:43:38 martin Exp $	*/

/*++
/* NAME
/*	mystrtok 3
/* SUMMARY
/*	safe tokenizer
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*mystrtok(bufp, delimiters)
/*	char	**bufp;
/*	const char *delimiters;
/*
/*	char	*mystrtokq(bufp, delimiters, parens)
/*	char	**bufp;
/*	const char *delimiters;
/*	const char *parens;
/*
/*	char	*mystrtokdq(bufp, delimiters)
/*	char	**bufp;
/*	const char *delimiters;
/*
/*	char	*mystrtok_cw(bufp, delimiters, blame)
/*	char	**bufp;
/*	const char *delimiters;
/*	const char *blame;
/*
/*	char	*mystrtokq_cw(bufp, delimiters, parens, blame)
/*	char	**bufp;
/*	const char *delimiters;
/*	const char *parens;
/*	const char *blame;
/*
/*	char	*mystrtokdq_cw(bufp, delimiters, blame)
/*	char	**bufp;
/*	const char *delimiters;
/*	const char *blame;
/* DESCRIPTION
/*	mystrtok() splits a buffer on the specified \fIdelimiters\fR.
/*	Tokens are delimited by runs of delimiters, so this routine
/*	cannot return zero-length tokens.
/*
/*	mystrtokq() is like mystrtok() but will not split text
/*	between balanced parentheses.  \fIparens\fR specifies the
/*	opening and closing parenthesis (one of each).  The set of
/*	\fIparens\fR must be distinct from the set of \fIdelimiters\fR.
/*
/*	mystrtokdq() is like mystrtok() but will not split text
/*	between double quotes. The backslash character may be used
/*	to escape characters. The double quote and backslash
/*	character must not appear in the set of \fIdelimiters\fR.
/*
/*	The \fIbufp\fR argument specifies the start of the search; it
/*	is updated with each call. The input is destroyed.
/*
/*	The result value is the next token, or a null pointer when the
/*	end of the buffer was reached.
/*
/*	mystrtok_cw(), mystrtokq_cw(), and mystrtokdq_cw, log a
/*	warning and return null when the result would look like
/*	comment. The \fBblame\fR argument provides context for
/*	warning messages. Specify a null pointer to disable the
/*	comment check.
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

/* Utility library. */

#include <msg.h>
#include <stringops.h>

/* mystrtok_warn - warn for #comment after other text */

static void mystrtok_warn(const char *start, const char *bufp, const char *blame)
{
    msg_warn("%s: #comment after other text is not allowed: %s %.20s...",
	     blame, start, bufp);
}

/* mystrtok - ABI compatibility wrapper */

#undef mystrtok

char   *mystrtok(char **src, const char *sep)
{
    return (mystrtok_cw(src, sep, (char *) 0));
}

/* mystrtok - safe tokenizer */

char   *mystrtok_cw(char **src, const char *sep, const char *blame)
{
    char   *start = *src;
    char   *end;

    /*
     * Skip over leading delimiters.
     */
    start += strspn(start, sep);
    if (*start == 0) {
	*src = start;
	return (0);
    }

    /*
     * Separate off one token.
     */
    end = start + strcspn(start, sep);
    if (*end != 0)
	*end++ = 0;
    *src = end;

    if (blame && *start == '#') {
	mystrtok_warn(start, *src, blame);
	return (0);
    } else {
	return (start);
    }
}

/* mystrtokq - ABI compatibility wrapper */

#undef mystrtokq

char   *mystrtokq(char **src, const char *sep, const char *parens)
{
    return (mystrtokq_cw(src, sep, parens, (char *) 0));
}

/* mystrtokq_cw - safe tokenizer with quoting support */

char   *mystrtokq_cw(char **src, const char *sep, const char *parens,
		             const char *blame)
{
    char   *start = *src;
    static char *cp;
    int     ch;
    int     level;

    /*
     * Skip over leading delimiters.
     */
    start += strspn(start, sep);
    if (*start == 0) {
	*src = start;
	return (0);
    }

    /*
     * Parse out the next token.
     */
    for (level = 0, cp = start; (ch = *(unsigned char *) cp) != 0; cp++) {
	if (ch == parens[0]) {
	    level++;
	} else if (level > 0 && ch == parens[1]) {
	    level--;
	} else if (level == 0 && strchr(sep, ch) != 0) {
	    *cp++ = 0;
	    break;
	}
    }
    *src = cp;

    if (blame && *start == '#') {
	mystrtok_warn(start, *src, blame);
	return (0);
    } else {
	return (start);
    }
}

/* mystrtokdq - ABI compatibility wrapper */

#undef mystrtokdq

char   *mystrtokdq(char **src, const char *sep)
{
    return (mystrtokdq_cw(src, sep, (char *) 0));
}

/* mystrtokdq_cw - safe tokenizer, double quote and backslash support */

char   *mystrtokdq_cw(char **src, const char *sep, const char *blame)
{
    char   *cp = *src;
    char   *start;

    /*
     * Skip leading delimiters.
     */
    cp += strspn(cp, sep);

    /*
     * Skip to next unquoted space or comma.
     */
    if (*cp == 0) {
	start = 0;
    } else {
	int     in_quotes;

	for (in_quotes = 0, start = cp; *cp; cp++) {
	    if (*cp == '\\') {
		if (*++cp == 0)
		    break;
	    } else if (*cp == '"') {
		in_quotes = !in_quotes;
	    } else if (!in_quotes && strchr(sep, *(unsigned char *) cp) != 0) {
		*cp++ = 0;
		break;
	    }
	}
    }
    *src = cp;

    if (blame && start && *start == '#') {
	mystrtok_warn(start, *src, blame);
	return (0);
    } else {
	return (start);
    }
}

#ifdef TEST

 /*
  * Test program.
  */
#include "msg.h"
#include "mymalloc.h"

 /*
  * The following needs to be large enough to include a null terminator in
  * every testcase.expected field.
  */
#define EXPECT_SIZE	5

struct testcase {
    const char *action;
    const char *input;
    const char *expected[EXPECT_SIZE];
};
static const struct testcase testcases[] = {
    {"mystrtok", ""},
    {"mystrtok", "  foo  ", {"foo"}},
    {"mystrtok", "  foo  bar  ", {"foo", "bar"}},
    {"mystrtokq", ""},
    {"mystrtokq", "foo bar", {"foo", "bar"}},
    {"mystrtokq", "{ bar }  ", {"{ bar }"}},
    {"mystrtokq", "foo { bar } baz", {"foo", "{ bar }", "baz"}},
    {"mystrtokq", "foo{ bar } baz", {"foo{ bar }", "baz"}},
    {"mystrtokq", "foo { bar }baz", {"foo", "{ bar }baz"}},
    {"mystrtokdq", ""},
    {"mystrtokdq", "  foo  ", {"foo"}},
    {"mystrtokdq", "  foo  bar  ", {"foo", "bar"}},
    {"mystrtokdq", "  foo\\ bar  ", {"foo\\ bar"}},
    {"mystrtokdq", "  foo \\\" bar", {"foo", "\\\"", "bar"}},
    {"mystrtokdq", "  foo \" bar baz\"  ", {"foo", "\" bar baz\""}},
    {"mystrtok_cw", "#after text"},
    {"mystrtok_cw", "before-text #after text", {"before-text"}},
    {"mystrtokq_cw", "#after text"},
    {"mystrtokq_cw", "{ before text } #after text", "{ before text }"},
    {"mystrtokdq_cw", "#after text"},
    {"mystrtokdq_cw", "\"before text\" #after text", {"\"before text\""}},
};

int     main(void)
{
    const struct testcase *tp;
    char   *actual;
    int     pass;
    int     fail;
    int     match;
    int     n;

#define NUM_TESTS       sizeof(testcases)/sizeof(testcases[0])
#define STR_OR_NULL(s)	((s) ? (s) : "null")

    for (pass = fail = 0, tp = testcases; tp < testcases + NUM_TESTS; tp++) {
	char   *saved_input = mystrdup(tp->input);
	char   *cp = saved_input;

	msg_info("RUN test case %ld %s >%s<",
		 (long) (tp - testcases), tp->action, tp->input);
#if 0
	msg_info("action=%s", tp->action);
	msg_info("input=%s", tp->input);
	for (n = 0; tp->expected[n]; tp++)
	    msg_info("expected[%d]=%s", n, tp->expected[n]);
#endif

	for (n = 0; n < EXPECT_SIZE; n++) {
	    if (strcmp(tp->action, "mystrtok") == 0) {
		actual = mystrtok(&cp, CHARS_SPACE);
	    } else if (strcmp(tp->action, "mystrtokq") == 0) {
		actual = mystrtokq(&cp, CHARS_SPACE, CHARS_BRACE);
	    } else if (strcmp(tp->action, "mystrtokdq") == 0) {
		actual = mystrtokdq(&cp, CHARS_SPACE);
	    } else if (strcmp(tp->action, "mystrtok_cw") == 0) {
		actual = mystrtok_cw(&cp, CHARS_SPACE, "test");
	    } else if (strcmp(tp->action, "mystrtokq_cw") == 0) {
		actual = mystrtokq_cw(&cp, CHARS_SPACE, CHARS_BRACE, "test");
	    } else if (strcmp(tp->action, "mystrtokdq_cw") == 0) {
		actual = mystrtokdq_cw(&cp, CHARS_SPACE, "test");
	    } else {
		msg_panic("invalid command: %s", tp->action);
	    }
	    if ((match = (actual && tp->expected[n]) ?
		 (strcmp(actual, tp->expected[n]) == 0) :
		 (actual == tp->expected[n])) != 0) {
		if (actual == 0) {
		    msg_info("PASS test %ld", (long) (tp - testcases));
		    pass++;
		    break;
		}
	    } else {
		msg_warn("expected: >%s<, got: >%s<",
			 STR_OR_NULL(tp->expected[n]), STR_OR_NULL(actual));
		msg_info("FAIL test %ld", (long) (tp - testcases));
		fail++;
		break;
	    }
	}
	if (n >= EXPECT_SIZE)
	    msg_panic("need to increase EXPECT_SIZE");
	myfree(saved_input);
    }
    return (fail > 0);
}

#endif
