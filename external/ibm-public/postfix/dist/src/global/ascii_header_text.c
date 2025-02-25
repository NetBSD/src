/*	$NetBSD: ascii_header_text.c,v 1.2 2025/02/25 19:15:45 christos Exp $	*/

/*++
/* NAME
/*	ascii_header_text 3h
/* SUMMARY
/*	message header content formatting
/* SYNOPSIS
/*	#include <ascii_header_text.h>
/*
/*	char   *make_ascii_header_text(
/*	VSTRING *result,
/*	int	flags,
/*	const char *str)
/* DESCRIPTION
/*	make_ascii_header_text() takes an ASCII input string and formats
/*	the content for use in a header phrase or comment.
/*
/*	The result value is a pointer to the result buffer string content,
/*	or null to indicate that no output was produced (the input was
/*	empty, or all ASCII whitespace).
/*
/*	Arguments:
/* .IP result
/*	The buffer that the output will overwrite. The result is
/*	null-terminated.
/* .IP flags
/*	One of HDR_FLAG_PHRASE or HDR_FLAG_COMMENT is required. Other
/*	flags are optional.
/* .RS
/* .IP	HDR_TEXT_FLAG_PHRASE
/*	Generate header content that will be used as a phrase, for
/*	example the full name content in "From: full-name <addr-spec>".
/* .IP HDR_TEXT_FLAG_COMMENT
/*	Generate header content that will be used as a comment, for
/*	example the full name in "From: addr-spec (full-name)".
/* .RE
/* .IP	str
/*	Pointer to null-terminated input storage.
/* DIAGNOSTICS
/*	Panic: invalid flags argument.
/* SEE ALSO
/*	rfc2047_code(3), encode header content
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
/*
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <ctype.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <ascii_header_text.h>
#include <msg.h>
#include <stringops.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <lex_822.h>
#include <mail_params.h>
#include <tok822.h>

 /*
  * Self.
  */
#include <ascii_header_text.h>

 /*
  * SLMs.
  */
#define STR	vstring_str
#define LEN	VSTRING_LEN

/* make_ascii_header_text - make header text for phrase or comment */

char   *make_ascii_header_text(VSTRING *result, int flags, const char *str)
{
    const char myname[] = "make_ascii_header_text";
    const char *cp;
    int     ch;
    int     target;

    /*
     * Quote or escape ASCII-only content. This factors out code from the
     * Postfix 2.9 cleanup daemon, without introducing visible changes for
     * text that contains only non-control characters and well-formed
     * comments. See TODO()s for some basic improvements that would allow
     * long inputs to be folded over multiple lines.
     */
    VSTRING_RESET(result);
    switch (target = (flags & HDR_TEXT_MASK_TARGET)) {

	/*
	 * Generate text for a phrase (for example, the full name in "From:
	 * full-name <addr-spec>").
	 * 
	 * TODO(wietse) add a tok822_externalize() option to replace whitespace
	 * between phrase tokens with newline, so that a long full name can
	 * be folded. This is a user-visible change; do this early in a
	 * development cycle to find out if this breaks compatibility.
	 */
    case HDR_TEXT_FLAG_PHRASE:{
	    TOK822 *dummy_token;
	    TOK822 *token;

	    if (str[strcspn(str, "%!" LEX_822_SPECIALS)] == 0) {
		token = tok822_scan_limit(str, &dummy_token,
					  var_token_limit);
	    } else {
		token = tok822_alloc(TOK822_QSTRING, str);
	    }
	    if (token) {
		tok822_externalize(result, token, TOK822_STR_NONE);
		tok822_free_tree(token);
		VSTRING_TERMINATE(result);
		return (STR(result));
	    } else {
		/* No output was generated. */
		return (0);
	    }
	}
	break;

	/*
	 * Generate text for comment content, for example, the full name in
	 * "From: addr-spec (full-name)". We do not quote "(", ")", or "\" as
	 * that would be a user-visible change, but we do fix unbalanced
	 * parentheses or a backslash at the end.
	 * 
	 * TODO(wietse): Replace whitespace with newline, so that a long full
	 * name can be folded). This is a user-visible change; do this early
	 * in a development cycle to find out if this breaks compatibility.
	 */
    case HDR_TEXT_FLAG_COMMENT:{
	    int     pc;

	    for (pc = 0, cp = str; (ch = *cp) != 0; cp++) {
		if (ch == '\\') {
		    if (cp[1] == 0)
			continue;
		    VSTRING_ADDCH(result, ch);
		    ch = *++cp;
		} else if (ch == '(') {
		    pc++;
		} else if (ch == ')') {
		    if (pc < 1)
			continue;
		    pc--;
		}
		VSTRING_ADDCH(result, ch);
	    }
	    while (pc-- > 0)
		VSTRING_ADDCH(result, ')');
	    VSTRING_TERMINATE(result);
	    return (LEN(result) && !allspace(STR(result)) ? STR(result) : 0);
	}
	break;
    default:
	msg_panic("%s: unknown target '0x%x'", myname, target);
    }
}

#ifdef TEST

#include <stdlib.h>
#include <string.h>
#include <msg.h>
#include <msg_vstream.h>

 /*
  * Test structure. Some tests generate their own.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
    int	flags;
    const char *input;
    const char *exp_output;
} TEST_CASE;

#define PASS    (0)
#define FAIL    (1)

#define NO_OUTPUT       ((char *) 0)

static int test_make_ascii_header_text(const TEST_CASE *tp)
{
    static VSTRING *result;
    const char *got;

    if (result == 0)
        result = vstring_alloc(100);

    got = make_ascii_header_text(result, tp->flags, tp->input);

    if (!got != !tp->exp_output) {
        msg_warn("got result ``%s'', want ``%s''",
                 got ? got : "null",
                 tp->exp_output ? tp->exp_output : "null");
        return (FAIL);
    }
    if (got && strcmp(got, tp->exp_output) != 0) {
        msg_warn("got result ``%s'', want ``%s''", got, tp->exp_output);
        return (FAIL);
    }
    return (PASS);
}

static const TEST_CASE test_cases[] = {

    /*
     * Phrase tests.
     */
    {"phrase_without_special",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_PHRASE, "abc def", "abc def"
    },
    {"phrase_with_special",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_PHRASE, "foo@bar", "\"foo@bar\""
    },
    {"phrase_with_space_only",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_PHRASE, " ", NO_OUTPUT
    },
    {"phrase_empty",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_PHRASE, "", NO_OUTPUT
    },

    /*
     * Comment tests.
     */
    {"comment_with_unopened_parens",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_COMMENT, ")foo )bar", "foo bar"
    },
    {"comment_with_unclosed_parens",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_COMMENT, "(foo (bar", "(foo (bar))"
    },
    {"comment_with_backslash_in_text",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_COMMENT, "foo\\bar", "foo\\bar"
    },
    {"comment_with_backslash_at_end",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_COMMENT, "foo\\", "foo"
    },
    {"comment_with_backslash_backslash_at_end",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_COMMENT, "foo\\\\", "foo\\\\"
    },
    {"comment_with_space_only",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_COMMENT, " ", NO_OUTPUT
    },
    {"comment_empty",
	test_make_ascii_header_text,
	HDR_TEXT_FLAG_COMMENT, "", NO_OUTPUT
    },
    {0},
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    for (tp = test_cases; tp->label != 0; tp++) {
        int     test_failed;

        msg_info("RUN  %s", tp->label);
        test_failed = tp->action(tp);
        if (test_failed) {
            msg_info("FAIL %s", tp->label);
            fail++;
        } else {
            msg_info("PASS %s", tp->label);
            pass++;
        }
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    exit(fail != 0);
}

#endif
