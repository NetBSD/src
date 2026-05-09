/*	$NetBSD: nbdb_surrogate_test.c,v 1.1.1.1 2026/05/09 18:39:18 christos Exp $	*/

/*++
/* NAME
/*      nbdb_surrogate_test 1t
/* SUMMARY
/*      nbdb_surrogate unit test
/* SYNOPSIS
/*      ./nbdb_surrogate_test
/* DESCRIPTION
/*      nbdb_surrogate_test runs and logs each configured test, reports if
/*      a test is a PASS or FAIL, and returns an exit status of zero if
/*      all tests are a PASS.
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
#include <dict_db.h>
#include <dict.h>
#include <mkmap.h>
#include <msg.h>
#include <msg_vstream.h>
#include <mymalloc.h>
#include <stringops.h>
#include <vstream.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <nbdb_util.h>
#include <nbdb_surrogate.h>

 /*
  * Testing library.
  */
#include <msg_capture.h>

 /*
  * Application-specific.
  */
int     dict_allow_surrogate = 1;

#define PASS    (0)
#define FAIL    (1)

#define STR(x)	vstring_str(x)

typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
} TEST_CASE;

static int redirects_dict_open_hash_to_surrogate(const TEST_CASE *tp)
{
    MSG_CAPTURE *capture = msg_capt_create(100);
    DICT   *dict;
    int     status = PASS;
    const char *want_warning = "support for 'hash:ignored' is not available";
    const char *got_warning;

    /*
     * Setup.
     */
    nbdb_surr_init();

    /*
     * Test and verify.
     */
    msg_capt_start(capture);
    dict = dict_open("hash:ignored", O_RDONLY, 0);
    msg_capt_stop(capture);
    if ((dict->flags & DICT_FLAG_SURROGATE) == 0) {
	msg_warn("dict_open(\"hash:...\", ...) did not return surrogate");
	status = FAIL;
    } else if (strstr(got_warning = msg_capt_expose(capture),
		      want_warning) == 0) {
	msg_warn("got warning: '%s', want: '%s'",
		 got_warning, want_warning);
	status = FAIL;
    }
    dict_close(dict);
    msg_capt_free(capture);

    return (status);
}

static int redirects_dict_open_btree_to_surrogate(const TEST_CASE *tp)
{
    MSG_CAPTURE *capture = msg_capt_create(100);
    DICT   *dict;
    int     status = PASS;
    const char *want_warning = "support for 'btree:ignored' is not available";
    const char *got_warning;

    /*
     * Setup.
     */
    nbdb_surr_init();

    /*
     * Test and verify.
     */
    msg_capt_start(capture);
    dict = dict_open("btree:ignored", O_RDONLY, 0);
    msg_capt_stop(capture);
    if ((dict->flags & DICT_FLAG_SURROGATE) == 0) {
	msg_warn("dict_open(\"btree:...\", ...) did not return surrogate");
	status = FAIL;
    } else if (strstr(got_warning = msg_capt_expose(capture),
		      want_warning) == 0) {
	msg_warn("got warning: '%s', want: '%s'",
		 got_warning, want_warning);
	status = FAIL;
    }
    dict_close(dict);
    msg_capt_free(capture);

    return (status);
}

 /*
  * TODO(wietse) also verify the msg_fatal() calls. That requires conversion
  * to PTEST.
  */
static const TEST_CASE test_cases[] = {
    {.label = "redirects_dict_open_hash_to_surrogate",
	redirects_dict_open_hash_to_surrogate,
    },
    {.label = "redirects_dict_open_btree_to_surrogate",
	redirects_dict_open_btree_to_surrogate,
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
