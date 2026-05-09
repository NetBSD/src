/*	$NetBSD: pol_stats_test.c,v 1.1.1.1 2026/05/09 18:39:19 christos Exp $	*/

/*++
/* NAME
/*      pol_stats_test 1t
/* SUMMARY
/*      pol_stats unit tests
/* SYNOPSIS
/*      ./pol_stats_test
/* DESCRIPTION
/*      pol_stats_test runs and logs each configured test, reports if a
/*      test is a PASS or FAIL, and returns an exit status of zero
/*      if all tests are a PASS.
/* LICENSE
/* .ad
/* .fi
/*      The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      porcupine.org
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
#include <msg_vstream.h>
#include <stringops.h>
#include <vstring.h>

 /*
  * Application-specific.
  */
#include <pol_stats.h>

 /*
  * Convenience wrappers for feature tests at index 0.
  */
#define pol_stat_activate_first(stats, name) \
        pol_stat_activate((stats), 0, (name))
#define pol_stat_decide_first(stats, name, status) \
        pol_stat_decide((stats), 0, (name), (status))

struct first_test_data {
    const char *target_name;
    const char *final_name;
    int     final_status;
};

 /*
  * Ditto, at index 1. Originally these were different from the definitions
  * for the first field. Keep the definitions separate so that tests can be
  * differentiated again if needed.
  */
#define pol_stat_activate_second(stats, name) \
        pol_stat_activate((stats), 1, (name))
#define pol_stat_decide_second(stats, name, status) \
        pol_stat_decide((stats), 1, (name), (status))

struct second_test_data {
    const char *target_name;
    const char *final_name;
    int     final_status;
};

 /*
  * Tests cases and test data.
  */
typedef struct TEST_CASE {
    const char *label;
    const char *want;
    int     (*action) (const struct TEST_CASE *);
    struct first_test_data first_data;
    struct second_test_data second_data;
} TEST_CASE;

static POL_STATS *pstats;
static VSTRING *buf;

static int test_pol_stats(const TEST_CASE *tp)
{
    char   *got;
    int     errors = 0;

    pol_stats_revert(pstats);
    VSTRING_RESET(buf);

    if (tp->first_data.target_name) {
	pol_stat_activate_first(pstats, tp->first_data.target_name);
	if (tp->first_data.final_name)
	    pol_stat_decide_first(pstats, tp->first_data.final_name,
				  tp->first_data.final_status);
    }
    if (tp->second_data.target_name) {
	pol_stat_activate_second(pstats, tp->second_data.target_name);
	if (tp->second_data.final_name)
	    pol_stat_decide_second(pstats, tp->second_data.final_name,
				   tp->second_data.final_status);
    }
    pol_stats_format(buf, pstats);
    got = vstring_str(buf);
    if (strcmp(got, tp->want) != 0) {
	msg_warn("got '%s', want '%s", got, tp->want);
	errors++;
    }
    return (errors == 0);
}

static TEST_CASE test_cases[] = {
    /* Tests for single feature at index 0 */
    {"first_compliant", "first", test_pol_stats,
	{"first", "first", POL_STAT_COMPLIANT}, {},
    },
    {"first_undecided", "first?", test_pol_stats,
	{"first", 0, POL_STAT_COMPLIANT}, {},
    },
    {"first_unspecified_violation", "!first", test_pol_stats,
	{"first", "first", POL_STAT_VIOLATION}, {},
    },
    {"first_downgraded_violation", "!first:low", test_pol_stats,
	{"first", "low", POL_STAT_VIOLATION}, {},
    },
    {"first_downgraded_compliant", "first:none", test_pol_stats,
	{"first", "none", POL_STAT_COMPLIANT}, {},
    },
    /* Tests for single feature at index 1 */
    {"second_compliant", "second", test_pol_stats,
	{}, {"second", "second", POL_STAT_COMPLIANT},
    },
    {"second_explicit_downgraded_compliant", "second:other", test_pol_stats,
	{}, {"second", "other", POL_STAT_COMPLIANT},
    },
    {"second_explicit_downgraded_violation", "!second:other", test_pol_stats,
	{}, {"second", "other", POL_STAT_VIOLATION},
    },
    /* Combined policy status tests. */
    {"multi_feature_compliant", "first/second:none", test_pol_stats,
	{"first", "first", POL_STAT_COMPLIANT},
	{"second", "none", POL_STAT_COMPLIANT},
    },
    {"multi_feature_violation", "!first/second:none", test_pol_stats,
	{"first", "first", POL_STAT_VIOLATION},
	{"second", "none", POL_STAT_COMPLIANT},
    },
    {"multi_feature_violation", "first/!second:none", test_pol_stats,
	{"first", "first", POL_STAT_COMPLIANT},
	{"second", "none", POL_STAT_VIOLATION},
    },
    0,
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);
    pstats = pol_stats_create();
    buf = vstring_alloc(100);

    for (tp = test_cases; tp->label != 0; tp++) {
	msg_info("RUN  %s", tp->label);
	if (tp->action(tp) == 0) {
	    fail++;
	    msg_info("FAIL %s", tp->label);
	} else {
	    msg_info("PASS %s", tp->label);
	    pass++;
	}
    }
    pol_stats_free(pstats);
    vstring_free(buf);

    msg_info("PASS=%d FAIL=%d", pass, fail);
    exit(fail != 0);
}
