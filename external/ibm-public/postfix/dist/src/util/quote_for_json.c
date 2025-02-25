/*	$NetBSD: quote_for_json.c,v 1.2 2025/02/25 19:15:52 christos Exp $	*/

/*++
/* NAME
/*	quote_for_json 3
/* SUMMARY
/*	quote UTF-8 string value for JSON
/* SYNOPSIS
/*	#include <quote_for_json.h>
/*
/*	char	*quote_for_json(
/*	VSTRING	*result,
/*	const char *in,
/*	ssize_t	len)
/*
/*	char	*quote_for_json_append(
/*	VSTRING	*result,
/*	const char *in,
/*	ssize_t	len)
/* DESCRIPTION
/*	quote_for_json() takes well-formed UTF-8 encoded text,
/*	quotes that text compliant with RFC 4627, and returns a
/*	pointer to the resulting text. The input may contain null
/*	bytes, but the output will not.
/*
/*	quote_for_json() produces short (two-letter) escape sequences
/*	for common control characters, double quote and backslash.
/*	It will not quote "/" (0x2F), and will quote DEL (0x7f) as
/*	\u007F to make it printable. The input byte sequence "\uXXXX"
/*	is quoted like any other text (the "\" is escaped as "\\").
/*
/*	quote_for_json() does not perform UTF-8 validation. The caller
/*	should use valid_utf8_string() or printable() as appropriate.
/*
/*	quote_for_json_append() appends the output to the result buffer.
/*
/*	Arguments:
/* .IP result
/*	Storage for the result, resized automatically.
/* .IP in
/*	Pointer to the input byte sequence.
/* .IP len
/*	The length of the input byte sequence, or a negative number
/*	when the byte sequence is null-terminated.
/* DIAGNOSTICS
/*	Fatal error: memory allocation error.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
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
#include <stringops.h>
#include <vstring.h>

#define STR(x) vstring_str(x)

/* quote_for_json_append - quote JSON string, append result */

char   *quote_for_json_append(VSTRING *result, const char *text, ssize_t len)
{
    const char *cp;
    int     ch;

    if (len < 0)
	len = strlen(text);

    for (cp = text; len > 0; len--, cp++) {
	ch = *(const unsigned char *) cp;
	if (UNEXPECTED(ISCNTRL(ch))) {
	    switch (ch) {
	    case '\b':
		VSTRING_ADDCH(result, '\\');
		VSTRING_ADDCH(result, 'b');
		break;
	    case '\f':
		VSTRING_ADDCH(result, '\\');
		VSTRING_ADDCH(result, 'f');
		break;
	    case '\n':
		VSTRING_ADDCH(result, '\\');
		VSTRING_ADDCH(result, 'n');
		break;
	    case '\r':
		VSTRING_ADDCH(result, '\\');
		VSTRING_ADDCH(result, 'r');
		break;
	    case '\t':
		VSTRING_ADDCH(result, '\\');
		VSTRING_ADDCH(result, 't');
		break;
	    default:
		/* All other controls including DEL and NUL. */
		vstring_sprintf_append(result, "\\u%04X", ch);
		break;
	    }
	} else {
	    switch (ch) {
	    case '\\':
	    case '"':
		VSTRING_ADDCH(result, '\\');
		/* FALLTHROUGH */
	    default:
		/* Includes malformed UTF-8. */
		VSTRING_ADDCH(result, ch);
		break;
	    }
	}
    }
    VSTRING_TERMINATE(result);
    return (STR(result));
}

/* quote_for_json - quote JSON string */

char   *quote_for_json(VSTRING *result, const char *text, ssize_t len)
{
    VSTRING_RESET(result);
    return (quote_for_json_append(result, text, len));
}

#ifdef TEST

 /*
  * System library.
  */
#include <stdlib.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <msg_vstream.h>

typedef struct TEST_CASE {
    const char *label;			/* identifies test case */
    char   *(*fn) (VSTRING *, const char *, ssize_t);
    const char *input;			/* input string */
    ssize_t input_len;			/* -1 or input length */
    const char *exp_res;		/* expected result */
} TEST_CASE;

#define PASS	(0)
#define FAIL	(1)

 /*
  * The test cases.
  */
static const TEST_CASE test_cases[] = {
    {"ordinary ASCII text", quote_for_json,
	" abcABC012.,[]{}/", -1, " abcABC012.,[]{}/",
    },
    {"quote_for_json_append", quote_for_json_append,
	"foo", -1, " abcABC012.,[]{}/foo",
    },
    {"common control characters", quote_for_json,
	"\b\f\r\n\t", -1, "\\b\\f\\r\\n\\t",
    },
    {"uncommon control characters and DEL", quote_for_json,
	"\0\01\037\040\176\177", 6, "\\u0000\\u0001\\u001F ~\\u007F",
    },
    {"malformed UTF-8", quote_for_json,
	"\\*\\uasd\\u007F\x80", -1, "\\\\*\\\\uasd\\\\u007F\x80",
    },
    0,
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;
    VSTRING *res_buf = vstring_alloc(100);

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    for (tp = test_cases; tp->label != 0; tp++) {
	int     test_fail = 0;
	char   *res;

	msg_info("RUN  %s", tp->label);
	res = tp->fn(res_buf, tp->input, tp->input_len);
	if (strcmp(res, tp->exp_res) != 0) {
	    msg_warn("test case '%s': got '%s', want '%s'",
		     tp->label, res, tp->exp_res);
	    test_fail = 1;
	}
	if (test_fail) {
	    fail++;
	    msg_info("FAIL %s", tp->label);
	    test_fail = 1;
	} else {
	    msg_info("PASS %s", tp->label);
	    pass++;
	}
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    vstring_free(res_buf);
    exit(fail != 0);
}

#endif
