/*	$NetBSD: allowed_prefix_test.c,v 1.2.2.2 2026/05/11 17:13:47 martin Exp $	*/

/*++
/* NAME
/*	allowed_prefix_test 1t
/* SUMMARY
/*	allowed_prefix unit test
/* SYNOPSIS
/*	./allowed_prefix_test
/* DESCRIPTION
/*	allowed_prefix_test runs and logs each configured test, reports if
/*	a test is a PASS or FAIL, and returns an exit status of zero if
/*	all tests are a PASS.
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
#include <stdlib.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <stringops.h>
#include <msg_vstream.h>
#include <vstring.h>
#include <vstream.h>

 /*
  * Global library.
  */
#include <allowed_prefix.h>

 /*
  * Testing library.
  */
#include <msg_capture.h>

 /*
  * Test structure. Some tests may bring their own.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
    const char *candidate;
    const char *parents;
    const bool want_match;
    const char *want_warning;
} TEST_CASE;

#define PASS    (0)
#define FAIL    (1)

#define bool_to_text(b) ((b) ? "true" : "false")

/* test_allowed_prefixes - generic test wrapper */

static int test_allowed_prefixes(const TEST_CASE *tp)
{
    MSG_CAPTURE *capture = msg_capt_create(100);
    ALLOWED_PREFIX *ap;
    bool    got_match;
    const char *got_warning;

    /* Run the test with captured VSTREAM_ERR stream. */
    msg_capt_start(capture);
    ap = allowed_prefix_create(tp->parents);
    got_match = allowed_prefix_match(ap, tp->candidate);
    allowed_prefix_free(ap);
    msg_capt_stop(capture);

    /* Verify the results. */
    got_warning = msg_capt_expose(capture);
    if (tp->want_warning == 0 && *got_warning != 0) {
	msg_warn("got warning ``%s'', want ``null''", got_warning);
	return (FAIL);
    }
    if (tp->want_warning != 0
	&& strstr(got_warning, tp->want_warning) == 0) {
	msg_warn("got warning ``%s'', want ``%s''",
		 got_warning, tp->want_warning);
	return (FAIL);
    }
    if (tp->want_match != got_match) {
	msg_warn("got match ``%s'', want ``%s''",
		 bool_to_text(got_match), bool_to_text(tp->want_match));
	return (FAIL);
    }
    msg_capt_free(capture);
    return (PASS);
}

 /*
  * The list of test cases.
  */
static const TEST_CASE test_cases[] = {
    {.label = "all_absolute_non_root_with_match",
	.action = test_allowed_prefixes,
	.parents = "/etc/postfix /etc/other, /foo/bar",
	.candidate = "/etc/other/foo",
	.want_match = true,
    },
    {.label = "all_absolute_non_root_no_match",
	.action = test_allowed_prefixes,
	.parents = "/etc/postfix /etc/other, /foo/bar",
	.candidate = "/etc/foo/other",
	.want_match = false,
    },
    {.label = "all_absolute_root_candidate_short_overlap",
	.action = test_allowed_prefixes,
	.parents = "/etc/postfix /etc/other, /foo/bar",
	.candidate = "/",
	.want_match = false,
    },
    {.label = "all_absolute_root_parent_with_match",
	.action = test_allowed_prefixes,
	.parents = "/etc/postfix, /, /foo/bar",
	.candidate = "/whatever",
	.want_match = true,
    },
    {.label = "one_relative_root_parent_with_match",
	.action = test_allowed_prefixes,
	.parents = "/etc/postfix foo /foo/bar",
	.candidate = "/foo/bar/whatever",
	.want_match = true,
	.want_warning = "to allowlist prefix with relative pathname: 'foo'",
    },
    {.label = "absolute_parent_relative_candidate_no_match",
	.action = test_allowed_prefixes,
	.parents = "/etc/postfix /etc/other /foo/bar",
	.candidate = "bar",
	.want_match = false,
	.want_warning = "to allowlist prefix with relative pathname: 'bar'",
    },
    {.label = "absolute_parent_dot_dot_candidate_no_match",
	.action = test_allowed_prefixes,
	.parents = "/etc/postfix /etc/other /foo/bar",
	.candidate = "/etc/postfix/../other",
	.want_match = false,
	.want_warning = "allowlist prefix with '/../': '/etc/postfix/../other'",
    },
    {.label = "trusted_root_directory_allows_all",
	.action = test_allowed_prefixes,
	.parents = "/ /foo/bar",
	.candidate = "/whatever",
	.want_match = true,
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
