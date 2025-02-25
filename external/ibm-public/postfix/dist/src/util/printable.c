/*	$NetBSD: printable.c,v 1.4 2025/02/25 19:15:52 christos Exp $	*/

/*++
/* NAME
/*	printable 3
/* SUMMARY
/*	mask non-printable characters
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	int	util_utf8_enable;
/*
/*	char	*printable(buffer, replacement)
/*	char	*buffer;
/*	int	replacement;
/*
/*	char	*printable_except(buffer, replacement, except)
/*	char	*buffer;
/*	int	replacement;
/*	const char *except;
/* DESCRIPTION
/*	printable() replaces non-printable characters
/*	in its input with the given replacement.
/*
/*	util_utf8_enable controls whether UTF8 is considered printable.
/*	With util_utf8_enable equal to zero, non-ASCII text is replaced.
/*
/*	Arguments:
/* .IP buffer
/*	The null-terminated input string.
/* .IP replacement
/*	Replacement value for characters in \fIbuffer\fR that do not
/*	pass the ASCII isprint(3) test or that are not valid UTF8.
/* .IP except
/*	Null-terminated sequence of non-replaced ASCII characters.
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
/*	Amawalk, NY 10501, USA
/*--*/

/* System library. */

#include "sys_defs.h"
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include "stringops.h"
#include "parse_utf8_char.h"

int     util_utf8_enable = 0;

/* printable -  binary compatibility */

#undef printable

char   *printable(char *, int);

char   *printable(char *string, int replacement)
{
    return (printable_except(string, replacement, (char *) 0));
}

/* printable_except -  pass through printable or other preserved characters */

char   *printable_except(char *string, int replacement, const char *except)
{
    char   *cp;
    char   *last;
    int     ch;

    /*
     * In case of a non-UTF8 sequence (bad leader byte, bad non-leader byte,
     * over-long encodings, out-of-range code points, etc), replace the first
     * byte, and try to resynchronize at the next byte.
     */
#define PRINT_OR_EXCEPT(ch) (ISPRINT(ch) || (except && strchr(except, ch)))

    for (cp = string; (ch = *(unsigned char *) cp) != 0; cp++) {
	if (util_utf8_enable == 0) {
	    if (ISASCII(ch) && PRINT_OR_EXCEPT(ch))
		continue;
	} else if ((last = parse_utf8_char(cp, 0)) == cp) {	/* ASCII */
	    if (PRINT_OR_EXCEPT(ch))
		continue;
	} else if (last != 0) {			/* Other UTF8 */
	    cp = last;
	    continue;
	}
	*cp = replacement;
    }
    return (string);
}

#ifdef TEST

#include <stdlib.h>
#include <string.h>
#include <msg.h>
#include <msg_vstream.h>
#include <mymalloc.h>
#include <vstream.h>

 /*
  * Test cases for 1-, 2-, and 3-byte encodings. Originally contributed by
  * Viktor Dukhovni, and annotated using translate.google.com.
  * 
  * See valid_utf8_string.c for single-error tests.
  * 
  * XXX Need a test for 4-byte encodings, preferably with strings that can be
  * displayed.
  */
struct testcase {
    const char *name;
    const char *input;
    const char *expected;;
};
static const struct testcase testcases[] = {
    {"Printable ASCII",
	"printable", "printable"
    },
    {"ASCII with control character",
	"non\bn-printable", "non?n-printable"
    },
    {"Latin accented text, no error",
	"na\303\257ve", "na\303\257ve"
    },
    {"Latin text, with error",
	"na\303ve", "na?ve"
    },
    {"Viktor, Cyrillic, no error",
	"\320\262\320\270\320\272\321\202\320\276\321\200",
	"\320\262\320\270\320\272\321\202\320\276\321\200"
    },
    {"Viktor, Cyrillic, two errors",
	"\320\262\320\320\272\272\321\202\320\276\321\200",
	"\320\262?\320\272?\321\202\320\276\321\200"
    },
    {"Viktor, Hebrew, no error",
	"\327\225\327\231\327\247\327\230\327\225\326\274\327\250",
	"\327\225\327\231\327\247\327\230\327\225\326\274\327\250"
    },
    {"Viktor, Hebrew, with error",
	"\327\225\231\327\247\327\230\327\225\326\274\327\250",
	"\327\225?\327\247\327\230\327\225\326\274\327\250"
    },
    {"Chinese (Simplified), no error",
	"\344\270\255\345\233\275\344\272\222\350\201\224\347\275\221\347"
	"\273\234\345\217\221\345\261\225\347\212\266\345\206\265\347\273"
	"\237\350\256\241\346\212\245\345\221\212",
	"\344\270\255\345\233\275\344\272\222\350\201\224\347\275\221\347"
	"\273\234\345\217\221\345\261\225\347\212\266\345\206\265\347\273"
	"\237\350\256\241\346\212\245\345\221\212"
    },
    {"Chinese (Simplified), with errors",
	"\344\270\255\345\344\272\222\350\224\347\275\221\347"
	"\273\234\345\217\221\345\261\225\347\212\266\345\206\265\347\273"
	"\237\350\256\241\346\212\245\345",
	"\344\270\255?\344\272\222??\347\275\221\347"
	"\273\234\345\217\221\345\261\225\347\212\266\345\206\265\347\273"
	"\237\350\256\241\346\212\245?"
    },
};

int     main(int argc, char **argv)
{
    const struct testcase *tp;
    int     pass;
    int     fail;

#define NUM_TESTS	sizeof(testcases)/sizeof(testcases[0])

    msg_vstream_init(basename(argv[0]), VSTREAM_ERR);
    util_utf8_enable = 1;

    for (pass = fail = 0, tp = testcases; tp < testcases + NUM_TESTS; tp++) {
	char   *input;
	char   *actual;
	int     ok = 0;

	/*
	 * Notes:
	 * 
	 * - The input is modified, therefore it must be copied.
	 * 
	 * - The msg(3) functions use printable() which interferes when logging
	 * inputs and outputs. Use vstream_fprintf() instead.
	 */
	vstream_fprintf(VSTREAM_ERR, "RUN  %s\n", tp->name);
	input = mystrdup(tp->input);
	actual = printable(input, '?');

	if (strcmp(actual, tp->expected) != 0) {
	    vstream_fprintf(VSTREAM_ERR, "input: >%s<, got: >%s<, want: >%s<\n",
			    tp->input, actual, tp->expected);
	} else {
	    vstream_fprintf(VSTREAM_ERR, "input: >%s<, got and want: >%s<\n",
			    tp->input, actual);
	    ok = 1;
	}
	if (ok) {
	    vstream_fprintf(VSTREAM_ERR, "PASS %s\n", tp->name);
	    pass++;
	} else {
	    vstream_fprintf(VSTREAM_ERR, "FAIL %s\n", tp->name);
	    fail++;
	}
	myfree(input);
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    return (fail > 0);
}

#endif
