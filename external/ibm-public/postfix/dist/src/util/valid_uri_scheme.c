/*	$NetBSD: valid_uri_scheme.c,v 1.2 2025/02/25 19:15:52 christos Exp $	*/

/*++
/* NAME
/*	valid_uri_scheme 3
/* SUMMARY
/*	validate scheme:// prefix
/* SYNOPSIS
/*	#include <valid_uri_scheme.h>
/*
/*	int	valid_uri_scheme(const char *str)
/* DESCRIPTION
/*	valid_uri_scheme() takes a null-terminated string and returns
/*	the length of a valid scheme:// prefix, or zero if no valid
/*	prefix was found.
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
#include <stdlib.h>

 /*
  * Utility library.
  */
#include <valid_uri_scheme.h>
#include <msg.h>
#include <msg_vstream.h>
#include <stringops.h>

/* valid_uri_scheme - predicate that string starts with scheme:// */

ssize_t valid_uri_scheme(const char *str)
{
    const char *cp = str;
    int     ch = *cp++;

    /* Per RFC 3986, a valid scheme starts with ALPHA. */
    if (!ISALPHA(ch))
	return (0);

    while ((ch = *cp++) != 0) {
	/* A valid scheme continues with ALPHA | DIGIT | '+' | '-'. */
	if (ISALNUM(ch) || ch == '+' || ch == '-')
	    continue;
	/* A valid scheme is followed by "://". */
	if (ch == ':' && *cp++ == '/' && *cp++ == '/')
	    return (cp - str);
	/* Not a valid scheme. */
	break;
    }
    /* Not a valid scheme. */
    return (0);
}

#ifdef TEST

typedef struct TEST_CASE {
    const char *label;
    const char *input;
    const ssize_t want;
} TEST_CASE;

#define PASS    (0)
#define FAIL    (1)

static const TEST_CASE test_cases[] = {
    {"accepts_alpha_scheme", "abcd://blah", sizeof("abcd://") - 1},
    {"accepts_mixed_scheme", "a-bcd+123://blah", sizeof("a-bcd+123://") - 1},
    {"rejects_minus_first", "-bcd+123://blah'", 0},
    {"rejects_plus_first", "+123://blah", 0},
    {"rejects_digit_first", "123://blah", 0},
    {"rejects_other_first", "?123://blah", 0},
    {"rejects_other_middle", "abcd?123://blah", 0},
    {"rejects_other_end", "abcd-123?://blah", 0},
    {"rejects_non_scheme", "inet:host:port", 0},
    {"rejects_no_colon", "inet", 0},
    {"rejects_colon_slash", "abcd:/blah", 0},
    {"rejects_empty", "", 0},
    {0,}
};

static int test_validate_scheme(const TEST_CASE *tp)
{
    int     got;

    got = valid_uri_scheme(tp->input);
    if (got != tp->want) {
	msg_warn("got '%ld', want '%ld'", (long) got, (long) tp->want);
	return (FAIL);
    }
    return (PASS);
}

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    for (tp = test_cases; tp->label != 0; tp++) {
	int     test_failed;

	msg_info("RUN  %s", tp->label);
	test_failed = test_validate_scheme(tp);
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
