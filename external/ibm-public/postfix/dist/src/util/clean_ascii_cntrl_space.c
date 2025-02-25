/*	$NetBSD: clean_ascii_cntrl_space.c,v 1.2 2025/02/25 19:15:51 christos Exp $	*/

/*++
/* NAME
/*	clean_ascii_cntrl_space 3h
/* SUMMARY
/*	censor control characters and normalize whitespace
/* SYNOPSIS
/*	#include <clean_ascii_cntrl_space.h>
/*
/*	char   *clean_ascii_cntrl_space(
/*	VSTRING *result,
/*	const char *str,
/*	ssize_t	len)
/* DESCRIPTION
/*	clean_ascii_cntrl_space() sanitizes input by replacing ASCII
/*	control characters with ASCII SPACE, by replacing a sequence
/*	of multiple ASCII SPACE characters with one ASCII SPACE, and by
/*	eliminating leading and trailing ASCII SPACE.
/*
/*	The result value is a pointer to the result buffer's string
/*	content, or null when no output was generated (for example
/*	all input characters were ASCII SPACE, or were replaced with
/*	ASCII SPACE).
/*
/*	Arguments:
/* .IP result
/*	The buffer that the output will overwrite. The result is
/*	null-terminated.
/* .IP	str
/*	Pointer to input storage.
/* .IP len
/*	The number of input bytes.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <ctype.h>

 /*
  * Utility library.
  */
#include <stringops.h>
#include <vstring.h>
#include <clean_ascii_cntrl_space.h>

 /*
  * SLMs.
  */
#define STR	vstring_str
#define LEN	VSTRING_LEN
#define END	vstring_end

/* clean_ascii_cntrl_space - replace control characters, normalize whitespace */

char   *clean_ascii_cntrl_space(VSTRING *result, const char *str, ssize_t len)
{
    const char *cp;
    const char *str_end;
    int     ch, prev_ch;

    VSTRING_RESET(result);
    for (cp = str, prev_ch = ' ', str_end = str + len; cp < str_end; cp++) {
	ch = *(unsigned char *) cp;
	if (ISCNTRL(ch))
	    ch = ' ';
	if (ch == ' ' && (prev_ch == ' ' || cp[1] == 0))
	    continue;
	VSTRING_ADDCH(result, ch);
	prev_ch = ch;
    }
    if (LEN(result) > 0 && END(result)[-1] == ' ')
	vstring_truncate(result, LEN(result) - 1);
    VSTRING_TERMINATE(result);
    return ((LEN(result) == 0 || allspace(STR(result))) ? 0 : STR(result));
}

#ifdef TEST
#include <stdlib.h>
#include <string.h>

#include <msg.h>
#include <msg_vstream.h>
#include <clean_ascii_cntrl_space.h>

 /*
  * Test structure. Some tests generate their own.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
    const char *input;
    ssize_t inlen;
    const char *exp_output;
} TEST_CASE;

#define NO_OUTPUT       ((char *) 0)

#define PASS    (0)
#define FAIL    (1)

static VSTRING *input;
static VSTRING *exp_output;

static int test_clean_ascii_cntrl_space(const TEST_CASE *tp)
{
    static VSTRING *result;
    const char *got;

    if (result == 0)
	result = vstring_alloc(100);

    got = clean_ascii_cntrl_space(result, tp->input, tp->inlen);

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

static int passes_all_non_controls(const TEST_CASE *tp)
{
    TEST_CASE test;
    int     ch;

    VSTRING_RESET(input);
    for (ch = 0x21; ch < 0x7f; ch++)
	VSTRING_ADDCH(input, ch);
    VSTRING_TERMINATE(input);
    test.label = tp->label;
    test.action = test_clean_ascii_cntrl_space;
    test.input = STR(input);
    test.inlen = strlen(STR(input));
    test.exp_output = test.input;
    return (test_clean_ascii_cntrl_space(&test));
}

static int replaces_all_controls(const TEST_CASE *tp)
{
    TEST_CASE test;
    int     ch;

    test.label = tp->label;
    test.action = test_clean_ascii_cntrl_space;

    for (ch = 1; ch <= 0x7f; ch++) {
	if (ch == 0x20)
	    ch = 0x7f;
	vstring_sprintf(input, "0x%02x>%c<", ch, ch);
	test.input = STR(input);
	test.inlen = LEN(input);
	vstring_sprintf(exp_output, "0x%02x> <", ch);
	test.exp_output = STR(exp_output);
	if (test_clean_ascii_cntrl_space(&test) != PASS)
	    return (FAIL);
    }
    return (PASS);
}

static const TEST_CASE test_cases[] = {
    {"passes_all_non_controls",
	passes_all_non_controls,
    },
    {"replaces_all_controls",
	replaces_all_controls,
    },
    {"collapses_multiple_controls",
	test_clean_ascii_cntrl_space,
	"x\x01\x02yx", 4, "x y"
    },
    {"collapses_control_space",
	test_clean_ascii_cntrl_space,
	"x\x01 y", 4, "x y"
    },
    {"collapses_space_control",
	test_clean_ascii_cntrl_space,
	"x \x01y", 4, "x y"
    },
    {"collapses_multiple_space",
	test_clean_ascii_cntrl_space,
	"x  y", 4, "x y"
    },
    {"strips_leading_control_space",
	test_clean_ascii_cntrl_space,
	"\x01 xy", 4, "xy",
    },
    {"strips_leading_space_control",
	test_clean_ascii_cntrl_space,
	" \x01xy", 4, "xy",
    },
    {"strips_trailing_space_control",
	test_clean_ascii_cntrl_space,
	"x  \x01", 5, "x",
    },
    {"strips_trailing_control_space",
	test_clean_ascii_cntrl_space,
	"x\x01\x01  ", 5, "x",
    },
    {0},
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    input = vstring_alloc(300);
    exp_output = vstring_alloc(300);

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
