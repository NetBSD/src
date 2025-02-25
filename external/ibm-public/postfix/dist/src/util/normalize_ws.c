/*	$NetBSD: normalize_ws.c,v 1.2 2025/02/25 19:15:52 christos Exp $	*/

/*++
/* NAME
/*	normalize_ws 3
/* SUMMARY
/*	neutralize isspace()-matching control characters
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*normalize_ws(char *str)
/* DESCRIPTION
/*	normalize_ws() takes a null-terminated string and substitutes
/*	ASCII SPACE for any ASCII control characters that match
/*	isspace(). This neutralizes line break etc. characters in the
/*	value portion of { name = value }. The function substitutes
/*	bytes in place, and returns its str argument.
/*
/*	This function requires that input is encoded in ASCII or UTF-8.
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
#include <string.h>
#include <stdlib.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <msg_vstream.h>
#include <mymalloc.h>
#include <stringops.h>

/* normalize_ws - replace isspace() members with space characters */

char   *normalize_ws(char *str)
{
    char *cp;

    for (cp = str; *(cp += strcspn(cp, "\t\n\v\f\r")); *cp = ' ')
	 /* void */ ;
    return (str);
}

#ifdef TEST

typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
    const char *input;
    const char *want;
} TEST_CASE;

#define PASS	(0)
#define FAIL	(1)

#define BUFLEN 2

/* if this test fails, isspace() may have changed */

static int normalizes_all_isspace_members(const TEST_CASE *unused)
{
    int     ch;
    char    input[BUFLEN];
    char    want[BUFLEN];
    char   *got;

    for (ch = 0; ISASCII(ch); ch++) {
	input[0] = ch;
	input[1] = 0;
	want[0] = ISSPACE(ch) ? ' ' : ch;
	want[1] = 0;
	got = normalize_ws(input);
	if (got != input) {
	    msg_warn("got %p, want %p", got, input);
	    return (FAIL);
	}
	if (memcmp(got, want, BUFLEN) != 0) {
	    msg_warn("got '{0x%02x 0x%02x}', want '{0x%02x 0x%02x}",
		     got[0], got[1], want[0], want[1]);
	    return (FAIL);
	}
    }
    return (PASS);
}

static int test_normalize_ws(const TEST_CASE *tp)
{
    char   *input = mystrdup(tp->input);
    char   *got;

    got = normalize_ws(input);
    if (strcmp(got, tp->want) != 0) {
	msg_warn("got '%s', want '%s'", got, tp->want);
	return (FAIL);
    }
    myfree(input);
    return (PASS);
}

static const TEST_CASE test_cases[] = {
    {"normalizes all isspace members", normalizes_all_isspace_members,},
    {"normalizes_first", test_normalize_ws, "\tfoo", " foo",},
    {"normalizes_middle", test_normalize_ws, "fo\to", "fo o",},
    {"normalizes_last", test_normalize_ws, "foo\t", "foo ",},
    {"normalizes_multiple", test_normalize_ws, "\tfo\to\t", " fo o ",},
    {0,}
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
