/*	$NetBSD: readlline.c,v 1.3 2025/02/25 19:15:52 christos Exp $	*/

/*++
/* NAME
/*	readlline 3
/* SUMMARY
/*	read logical line
/* SYNOPSIS
/*	#include <readlline.h>
/*
/*	VSTRING	*readllines(buf, fp, lineno, first_line)
/*	VSTRING	*buf;
/*	VSTREAM	*fp;
/*	int	*lineno;
/*	int	*first_line;
/*
/*	VSTRING	*readlline(buf, fp, lineno)
/*	VSTRING	*buf;
/*	VSTREAM	*fp;
/*	int	*lineno;
/* DESCRIPTION
/*	readllines() reads one logical line from the named stream.
/* .IP "blank lines and comments"
/*	Empty lines and whitespace-only lines are ignored, as
/*	are lines whose first non-whitespace character is a `#'.
/* .IP "multi-line text"
/*	A logical line starts with non-whitespace text. A line that
/*	starts with whitespace continues a logical line.
/* .PP
/*	The result value is the input buffer argument or a null pointer
/*	when no input is found.
/*
/*	readlline() is a backwards-compatibility wrapper.
/*
/*	Arguments:
/* .IP buf
/*	A variable-length buffer for input. The result is null terminated.
/* .IP fp
/*	Handle to an open stream.
/* .IP lineno
/*	A null pointer, or a pointer to an integer that is incremented
/*	after reading a physical line.
/* .IP first_line
/*	A null pointer, or a pointer to an integer that will contain
/*	the line number of the first non-blank, non-comment line
/*	in the result logical line.
/* DIAGNOSTICS
/*	Warning: a continuation line that does not continue preceding text.
/*	The invalid input is ignored, to avoid complicating caller code.
/* SECURITY
/* .ad
/* .fi
/*	readlline() imposes no logical line length limit therefore it
/*	should be used for reading trusted information only.
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
#include <ctype.h>

/* Utility library. */

#include "msg.h"
#include "vstream.h"
#include "vstring.h"
#include "readlline.h"

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)
#define END(x) vstring_end(x)

/* readllines - read one logical line */

VSTRING *readllines(VSTRING *buf, VSTREAM *fp, int *lineno, int *first_line)
{
    int     ch;
    int     next;
    ssize_t start;
    char   *cp;
    int     my_lineno = 0, my_first_line, got_null = 0;

    VSTRING_RESET(buf);

    if (lineno == 0)
	lineno = &my_lineno;
    if (first_line == 0)
	first_line = &my_first_line;

    /*
     * Ignore comment lines, all whitespace lines, and empty lines. Terminate
     * at EOF or at the beginning of the next logical line.
     */
    for (;;) {
	/* Read one line, possibly not newline terminated. */
	start = LEN(buf);
	while ((ch = VSTREAM_GETC(fp)) != VSTREAM_EOF && ch != '\n') {
	    VSTRING_ADDCH(buf, ch);
	    if (ch == 0)
		got_null = 1;
	}
	if (ch == '\n' || LEN(buf) > start)
	    *lineno += 1;
	/* Ignore comment line, all whitespace line, or empty line. */
	for (cp = STR(buf) + start; cp < END(buf) && ISSPACE(*cp); cp++)
	     /* void */ ;
	if (cp == END(buf) || *cp == '#')
	    vstring_truncate(buf, start);
	if (start == 0)
	    *first_line = *lineno;
	/* Terminate at EOF or at the beginning of the next logical line. */
	if (ch == VSTREAM_EOF)
	    break;
	if (LEN(buf) > 0) {
	    if ((next = VSTREAM_GETC(fp)) != VSTREAM_EOF)
		vstream_ungetc(fp, next);
	    if (next != '#' && !ISSPACE(next))
		break;
	}
    }
    VSTRING_TERMINATE(buf);

    /*
     * This code does not care about embedded null bytes, but callers do.
     */
    if (got_null) {
	const char *why = "text after null byte may be ignored";

	if (*first_line == *lineno)
	    msg_warn("%s, line %d: %s",
		     VSTREAM_PATH(fp), *lineno, why);
	else
	    msg_warn("%s, line %d-%d: %s",
		     VSTREAM_PATH(fp), *first_line, *lineno, why);
    }

    /*
     * Invalid input: continuing text without preceding text. Allowing this
     * would complicate "postconf -e", which implements its own multi-line
     * parsing routine. Do not abort, just warn, so that critical programs
     * like postmap do not leave behind a truncated table.
     */
    if (LEN(buf) > 0 && ISSPACE(*STR(buf))) {
	msg_warn("%s: logical line must not start with whitespace: \"%.30s%s\"",
		 VSTREAM_PATH(fp), STR(buf),
		 LEN(buf) > 30 ? "..." : "");
	return (readllines(buf, fp, lineno, first_line));
    }

    /*
     * Done.
     */
    return (LEN(buf) > 0 ? buf : 0);
}

 /*
  * Stand-alone test program.
  */
#ifdef TEST
#include <stdlib.h>
#include <string.h>
#include <msg.h>
#include <msg_vstream.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Test cases. Note: the input and exp_output fields are converted with
  * unescape(). Embedded null bytes must be specified as \\0.
  */
struct testcase {
    const char *name;
    const char *input;
    const char *exp_output;
    int     exp_first_line;
    int     exp_last_line;
};

static const struct testcase testcases[] = {
    {"leading space before non-comment",
	" abcde\nfghij\n",
	"fghij",
	2, 2
	/* Expect "logical line must not start with whitespace" */
    },
    {"leading space before leading comment",
	" #abcde\nfghij\n",
	"fghij",
	2, 2
    },
    {"leading #comment at beginning of line",
	"#abc\ndef",
	"def",
	2, 2,
    },
    {"empty line before non-comment",
	"\nabc\n",
	"abc",
	2, 2,
    },
    {"whitespace line before non-comment",
	" \nabc\n",
	"abc",
	2, 2,
    },
    {"missing newline at end of non-comment",
	"abc def",
	"abc def",
	1, 1,
    },
    {"missing newline at end of comment",
	"#abc def",
	"",
	1, 1,
    },
    {"embedded null, single-line",
	"abc\\0def",
	"abc\\0def",
	1, 1,
	/* Expect "line 1: text after null byte may be ignored" */
    },
    {"embedded null, multiline",
	"abc\\0\n def",
	"abc\\0 def",
	1, 2,
	/* Expect "line 1-2: text after null byte may be ignored" */
    },
    {"embedded null in comment",
	"#abc\\0\ndef",
	"def",
	2, 2,
	/* Expect "line 2: text after null byte may be ignored" */
    },
    {"multiline input",
	"abc\n def\n",
	"abc def",
	1, 2,
    },
    {"multiline input with embedded #comment after space",
	"abc\n #def\n ghi",
	"abc ghi",
	1, 3,
    },
    {"multiline input with embedded #comment flush left",
	"abc\n#def\n ghi",
	"abc ghi",
	1, 3,
    },
    {"multiline input with embedded whitespace line",
	"abc\n \n ghi",
	"abc ghi",
	1, 3,
    },
    {"multiline input with embedded empty line",
	"abc\n\n ghi",
	"abc ghi",
	1, 3,
    },
    {"multiline input with embedded #comment after space",
	"abc\n #def\n",
	"abc",
	1, 2,
    },
    {"multiline input with embedded #comment flush left",
	"abc\n#def\n",
	"abc",
	1, 2,
    },
    {"empty line at end of file",
	"\n",
	"",
	1, 1,
    },
    {"whitespace line at end of file",
	"\n \n",
	"",
	2, 2,
    },
    {"whitespace at end of file",
	"abc\n ",
	"abc",
	1, 2,
    },
};

int     main(int argc, char **argv)
{
    const struct testcase *tp;
    VSTRING *inp_buf = vstring_alloc(100);
    VSTRING *exp_buf = vstring_alloc(100);
    VSTRING *out_buf = vstring_alloc(100);
    VSTRING *esc_buf = vstring_alloc(100);
    VSTREAM *fp;
    int     last_line;
    int     first_line;
    int     pass;
    int     fail;

#define NUM_TESTS       sizeof(testcases)/sizeof(testcases[0])

    msg_vstream_init(basename(argv[0]), VSTREAM_ERR);
    util_utf8_enable = 1;

    for (pass = fail = 0, tp = testcases; tp < testcases + NUM_TESTS; tp++) {
	int     ok = 0;

	vstream_fprintf(VSTREAM_ERR, "RUN  %s\n", tp->name);
	unescape(inp_buf, tp->input);
	unescape(exp_buf, tp->exp_output);
	if ((fp = vstream_memopen(inp_buf, O_RDONLY)) == 0)
	    msg_panic("open memory stream for reading: %m");
	vstream_control(fp, CA_VSTREAM_CTL_PATH("memory buffer"),
			CA_VSTREAM_CTL_END);
	last_line = 0;
	if (readllines(out_buf, fp, &last_line, &first_line) == 0) {
	    VSTRING_RESET(out_buf);
	    VSTRING_TERMINATE(out_buf);
	}
	if (LEN(out_buf) != LEN(exp_buf)) {
	    msg_warn("unexpected output length, got: %ld, want: %ld",
		     (long) LEN(out_buf), (long) LEN(exp_buf));
	} else if (memcmp(STR(out_buf), STR(exp_buf), LEN(out_buf)) != 0) {
	    msg_warn("unexpected output: got: >%s<, want: >%s<",
		     STR(escape(esc_buf, STR(out_buf), LEN(out_buf))),
		     tp->exp_output);
	} else if (first_line != tp->exp_first_line) {
	    msg_warn("unexpected first_line: got: %d, want: %d",
		     first_line, tp->exp_first_line);
	} else if (last_line != tp->exp_last_line) {
	    msg_warn("unexpected last_line: got: %d, want: %d",
		     last_line, tp->exp_last_line);
	} else {
	    vstream_fprintf(VSTREAM_ERR, "got and want: >%s<\n",
			    tp->exp_output);
	    ok = 1;
	}
	if (ok) {
	    vstream_fprintf(VSTREAM_ERR, "PASS %s\n", tp->name);
	    pass++;
	} else {
	    vstream_fprintf(VSTREAM_ERR, "FAIL %s\n", tp->name);
	    fail++;
	}
	vstream_fclose(fp);
    }
    vstring_free(inp_buf);
    vstring_free(exp_buf);
    vstring_free(out_buf);
    vstring_free(esc_buf);

    msg_info("PASS=%d FAIL=%d", pass, fail);
    return (fail > 0);
}

#endif
