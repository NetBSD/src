/*	$NetBSD: valid_utf8_string.c,v 1.3 2025/02/25 19:15:52 christos Exp $	*/

/*++
/* NAME
/*	valid_utf8_string 3
/* SUMMARY
/*	predicate if string is valid UTF-8
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	int	valid_utf8_string(str, len)
/*	const char *str;
/*	ssize_t	len;
/*
/*	int	valid_utf8_stringz(str)
/*	const char *str;
/*	ssize_t	len;
/* DESCRIPTION
/*	valid_utf8_string() determines if all bytes in a string
/*	satisfy parse_utf8_char(3h) checks. See there for any
/*	implementation limitations.
/*
/*	valid_utf8_stringz() determines the same for zero-terminated
/*	strings.
/*
/*	A zero-length string is considered valid.
/* DIAGNOSTICS
/*	The result value is zero when the caller specifies a negative
/*	length, or a string that does not pass parse_utf8_char(3h) checks.
/* SEE ALSO
/*	parse_utf8_char(3h), parse one UTF-8 multibyte character
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
/*	porcupine.org
/*	Amawalk, NY 10501, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <stringops.h>
#include <parse_utf8_char.h>

/* valid_utf8_string - validate string according to RFC 3629 */

int     valid_utf8_string(const char *str, ssize_t len)
{
    const char *ep = str + len;
    const char *cp;
    const char *last;

    if (len < 0)
	return (0);
    if (len == 0)
	return (1);

    /*
     * Ideally, the compiler will inline parse_utf8_char().
     */
    for (cp = str; cp < ep; cp++) {
	if ((last = parse_utf8_char(cp, ep)) != 0)
	    cp = last;
	else
	    return (0);
    }
    return (1);
}

/* valid_utf8_stringz - validate string according to RFC 3629 */

int     valid_utf8_stringz(const char *str)
{
    const char *cp;
    const char *last;

    /*
     * Ideally, the compiler will inline parse_utf8_char(), propagate the
     * null pointer constant value, and eliminate code branches that test
     * whether 0 != 0.
     */
    for (cp = str; *cp; cp++) {
	if ((last = parse_utf8_char(cp, 0)) != 0)
	    cp = last;
	else
	    return (0);
    }
    return (1);
}

 /*
  * Stand-alone test program. Each string is a line without line terminator.
  */
#ifdef TEST
#include <stdlib.h>
#include <string.h>
#include <msg.h>
#include <vstream.h>
#include <msg_vstream.h>

 /*
  * Test cases for 1-, 2-, and 3-byte encodings. See printable.c for UTF8
  * parser resychronization tests.
  * 
  * XXX Need a test for 4-byte encodings, preferably with strings that can be
  * displayed.
  * 
  * XXX Need tests with hand-crafted over-long encodings and surrogates.
  */
struct testcase {
    const char *name;
    const char *input;
    int     expected;
};

#define T_VALID		(1)
#define T_INVALID	(0)
#define valid_to_str(v)	((v) ? "VALID" : "INVALID")

static const struct testcase testcases[] = {
    {"Printable ASCII",
	"printable", T_VALID,
    },
    {"Latin script, accented, no error",
	"na\303\257ve", T_VALID,
    },
    {"Latin script, accented, missing non-leading byte",
	"na\303ve", T_INVALID,
    },
    {"Latin script, accented, missing leading byte",
	"na\257ve", T_INVALID,
    },
    {"Viktor, Cyrillic, no error",
	"\320\262\320\270\320\272\321\202\320\276\321\200", T_VALID,
    },
    {"Viktor, Cyrillic, missing non-leading byte",
	"\320\262\320\320\272\321\202\320\276\321\200", T_INVALID,
    },
    {"Viktor, Cyrillic, missing leading byte",
	"\320\262\270\320\272\321\202\320\276\321\200", T_INVALID,
    },
    {"Viktor, Cyrillic, truncated",
	"\320\262\320\270\320\272\321\202\320\276\321", T_INVALID,
    },
    {"Viktor, Hebrew, no error",
	"\327\225\327\231\327\247\327\230\327\225\326\274\327\250", T_VALID,
    },
    {"Viktor, Hebrew, missing leading byte",
	"\327\225\231\327\247\327\230\327\225\326\274\327\250", T_INVALID,
    },
    {"Chinese (Simplified), no error",
	"\344\270\255\345\233\275\344\272\222\350\201\224\347\275\221\347"
	"\273\234\345\217\221\345\261\225\347\212\266\345\206\265\347\273"
	"\237\350\256\241\346\212\245\345\221\212", T_VALID,
    },
    {"Chinese (Simplified), missing leading byte",
	"\344\270\255\345\233\275\344\272\222\350\201\224\275\221\347"
	"\273\234\345\217\221\345\261\225\347\212\266\345\206\265\347\273"
	"\237\350\256\241\346\212\245\345\221\212", T_INVALID,
    },
    {"Chinese (Simplified), missing first non-leading byte",
	"\344\270\255\345\233\275\344\272\222\350\201\224\347\221\347"
	"\273\234\345\217\221\345\261\225\347\212\266\345\206\265\347\273"
	"\237\350\256\241\346\212\245\345\221\212", T_INVALID,
    },
    {"Chinese (Simplified), missing second non-leading byte",
	"\344\270\255\345\233\275\344\272\222\350\201\224\347\275\347"
	"\273\234\345\217\221\345\261\225\347\212\266\345\206\265\347\273"
	"\237\350\256\241\346\212\245\345\221\212", T_INVALID,
    },
    {"Chinese (Simplified), truncated",
	"\344\270\255\345\233\275\344\272\222\350\201\224\347\275\221\347"
	"\273\234\345\217\221\345\261\225\347\212\266\345\206\265\347\273"
	"\237\350\256\241\346\212\245\345", T_INVALID,
    },
};

int     main(int argc, char **argv)
{
    const struct testcase *tp;
    int     pass;
    int     fail;

#define NUM_TESTS       sizeof(testcases)/sizeof(testcases[0])

    msg_vstream_init(basename(argv[0]), VSTREAM_ERR);
    util_utf8_enable = 1;

    for (pass = fail = 0, tp = testcases; tp < testcases + NUM_TESTS; tp++) {
	int     actual_l;
	int     actual_z;
	int     ok = 0;

	/*
	 * Notes:
	 * 
	 * - The msg(3) functions use printable() which interferes when logging
	 * inputs and outputs. Use vstream_fprintf() instead.
	 */
	vstream_fprintf(VSTREAM_ERR, "RUN  %s\n", tp->name);
	actual_l = valid_utf8_string(tp->input, strlen(tp->input));
	actual_z = valid_utf8_stringz(tp->input);

	if (actual_l != tp->expected) {
	    vstream_fprintf(VSTREAM_ERR,
			  "input: >%s<, 'actual_l' got: >%s<, want: >%s<\n",
			    tp->input, valid_to_str(actual_l),
			    valid_to_str(tp->expected));
	} else if (actual_z != tp->expected) {
	    vstream_fprintf(VSTREAM_ERR,
			  "input: >%s<, 'actual_z' got: >%s<, want: >%s<\n",
			    tp->input, valid_to_str(actual_z),
			    valid_to_str(tp->expected));
	} else {
	    vstream_fprintf(VSTREAM_ERR, "input: >%s<, got and want: >%s<\n",
			    tp->input, valid_to_str(actual_l));
	    ok = 1;
	}
	if (ok) {
	    vstream_fprintf(VSTREAM_ERR, "PASS %s\n", tp->name);
	    pass++;
	} else {
	    vstream_fprintf(VSTREAM_ERR, "FAIL %s\n", tp->name);
	    fail++;
	}
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    return (fail > 0);
}

#endif
