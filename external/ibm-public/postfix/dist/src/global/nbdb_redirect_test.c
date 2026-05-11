/*	$NetBSD: nbdb_redirect_test.c,v 1.2.2.2 2026/05/11 17:13:49 martin Exp $	*/

/*++
/* NAME
/*      nbdb_redirect_test 1t
/* SUMMARY
/*      nbdb_redirect unit test
/* SYNOPSIS
/*      ./nbdb_redirect_test
/* DESCRIPTION
/*      nbdb_redirect_test runs and logs each configured test, reports if
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
#include <dict_cdb.h>
#include <dict_db.h>
#include <dict.h>
#include <dict_inline.h>
#include <dict_lmdb.h>
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
#include <nbdb_redirect.h>

 /*
  * Testing library.
  */
#include <msg_capture.h>
#include <mock_dict.h>

#define PASS    (0)
#define FAIL    (1)

#define STR(x)	vstring_str(x)

char   *var_db_type = 0;
char   *var_cache_db_type = 0;
char   *var_nbdb_cust_map = DEF_NBDB_CUST_MAP;
bool    var_nbdb_log_redirect = 0;

 /*
  * Tests that a dict_open() request for 'hash' or 'btree' will redirect to
  * $default_database_type or $default_cache_db_type. These use mock cdb and
  * lmdb clients that will respond with their type when queried for "whoami".
  * 
  * The redirect unit tests do not cover the 'bulk create' code paths; those
  * will be tested with postmap/postalias 'whole program' tests.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
} TEST_CASE;

static void setup_common_mock_open_info(void)
{
    nbdb_level = NBDB_LEV_CODE_REDIRECT;

    nbdb_reindexing_lockout = 0;
    setup_mock_cdb("{{whoami = mock cdb}}");
    setup_mock_lmdb("{{whoami = mock lmdb}}");
    nbdb_rdr_init();
}

static int redirects_dict_open_hash_to_def_db_type(const TEST_CASE *tp)
{
    MSG_CAPTURE *capture;
    const char *got_logging;
    const char *got_whoami;
    DICT   *dict;
    int     status = PASS;
    const char *want_whoami;
    const char *want_logging_list[] = {
	"redirecting hash:ignored to cdb:ignored", 0,
    };
    int     n;

    /*
     * Test with and without logging.
     */
    for (n = 0; n < 2; n++) {
	const char *want_logging = want_logging_list[n];

	/*
	 * Setup. Use the mapping hash->cdb.
	 */
	setup_common_mock_open_info();
	var_db_type = "cdb";
	capture = msg_capt_create(100);
	var_nbdb_log_redirect = want_logging != 0;

	/*
	 * Verify that 'hash' redirects to $default_database_type ('cdb').
	 */
	want_whoami = "mock cdb";
	msg_capt_start(capture);
	dict = dict_open3(DICT_TYPE_HASH, "ignored", O_RDONLY, 0);
	msg_capt_stop(capture);
	got_logging = msg_capt_expose(capture);

	if ((got_whoami = dict_get(dict, "whoami")) == 0) {
	    msg_warn("'whoami' query returned 'not found'");
	    status = FAIL;
	} else if (strstr(got_whoami, want_whoami) == 0) {
	    msg_warn("unexpected result; got: '%s', want: '%s'",
		     got_whoami, want_whoami);
	    status = FAIL;
	} else if (want_logging == 0 && *got_logging != 0) {
	    msg_warn("unexpected logging: got: '%s'", got_logging);
	    status = FAIL;
	} else if (want_logging && strstr(got_logging, want_logging) == 0) {
	    msg_warn("unexpected logging: got: '%s', want: '%s'",
		     got_logging, want_logging);
	    status = FAIL;
	}
	dict_close(dict);
    }

    return (status);
}

static int redirects_dict_open_btree_to_def_cache_db_type(const TEST_CASE *tp)
{
    MSG_CAPTURE *capture;
    const char *got_logging;
    const char *got_whoami;
    DICT   *dict;
    int     status = PASS;
    const char *want_whoami;
    const char *want_logging_list[] = {
	"redirecting btree:ignored to lmdb:ignored", 0,
    };
    int     n;

    /*
     * Test with and without logging.
     */
    for (n = 0; n < 2; n++) {
	const char *want_logging = want_logging_list[n];

	/*
	 * Setup. Use the mapping btree->lmdb.
	 */
	setup_common_mock_open_info();
	var_cache_db_type = "lmdb";
	capture = msg_capt_create(100);
        var_nbdb_log_redirect = want_logging != 0;

	/*
	 * Verify that 'btree' redirects to $default_cache_db_type ('lmdb').
	 */
	want_whoami = "mock lmdb";
	msg_capt_start(capture);
	dict = dict_open3(DICT_TYPE_BTREE, "ignored", O_RDONLY, 0);
	msg_capt_stop(capture);
	got_logging = msg_capt_expose(capture);

	if ((got_whoami = dict_get(dict, "whoami")) == 0) {
	    msg_warn("'whoami' query returned 'not found'");
	    status = FAIL;
	} else if (strstr(got_whoami, "mock lmdb") == 0) {
	    msg_warn("unexpected result; got: '%s', want: '%s'",
		     got_whoami, want_whoami);
	    status = FAIL;
	} else if (want_logging == 0 && *got_logging != 0) {
	    msg_warn("unexpected logging: got: '%s'", got_logging);
	    status = FAIL;
	} else if (want_logging && strstr(got_logging, want_logging) == 0) {
	    msg_warn("unexpected logging: got: '%s', want: '%s'",
		     got_logging, want_logging);
	    status = FAIL;
	}
	dict_close(dict);
    }

    return (status);
}

static int redirects_mkmap_open_hash_to_def_db_type(const TEST_CASE *tp)
{
    MSG_CAPTURE *capture;
    const char *got_logging;
    const char *got_whoami;
    MKMAP  *mkmap;
    int     status = PASS;
    const char *want_whoami;
    const char *want_logging_list[] = {
	"Redirecting hash:ignored to cdb:ignored", 0,
    };
    int     n;

    /*
     * Test with and without logging.
     */
    for (n = 0; n < 2; n++) {
	const char *want_logging = want_logging_list[n];

	/*
	 * Setup. Use the mapping hash->cdb.
	 */
	setup_common_mock_open_info();
	var_db_type = "cdb";
	capture = msg_capt_create(100);
	var_nbdb_log_redirect = want_logging != 0;

	/*
	 * Verify that mkmap_open("hash"...) redirects to
	 * $default_database_type ('cdb').
	 */
	want_whoami = "mock cdb";
	msg_capt_start(capture);
	mkmap = mkmap_open(DICT_TYPE_HASH, "ignored", O_RDONLY, 0);
	msg_capt_stop(capture);
	got_logging = msg_capt_expose(capture);

	if ((got_whoami = dict_get(mkmap->dict, "whoami")) == 0) {
	    msg_warn("'whoami' query returned 'not found'");
	    status = FAIL;
	} else if (strstr(got_whoami, want_whoami) == 0) {
	    msg_warn("unexpected result; got: '%s', want: '%s'",
		     got_whoami, want_whoami);
	    status = FAIL;
	} else if (want_logging == 0 && *got_logging != 0) {
	    msg_warn("unexpected logging: got: '%s'", got_logging);
	    status = FAIL;
	} else if (want_logging == 0 && *got_logging != 0) {
	    msg_warn("unexpected logging: got: '%s'", got_logging);
	    status = FAIL;
	}
	mkmap_close(mkmap);
    }

    return (status);
}

static int redirects_mkmap_open_btree_to_def_cache_db_type(const TEST_CASE *tp)
{
    MSG_CAPTURE *capture;
    const char *got_logging;
    const char *got_whoami;
    MKMAP  *mkmap;
    int     status = PASS;
    const char *want_whoami;
    const char *want_logging_list[] = {
	"redirecting btree:ignored to lmdb:ignored", 0,
    };
    int     n;

    /*
     * Test with and without logging.
     */
    for (n = 0; n < 2; n++) {
	const char *want_logging = want_logging_list[n];

	/*
	 * Setup. Use the mapping btree->lmdb.
	 */
	setup_common_mock_open_info();
	var_cache_db_type = "lmdb";
	capture = msg_capt_create(100);
	var_nbdb_log_redirect = want_logging != 0;

	/*
	 * Verify that mkmap_open("btree"...) redirects to
	 * $default_cache_db_type ('lmdb').
	 */
	want_whoami = "mock lmdb";
	msg_capt_start(capture);
	mkmap = mkmap_open(DICT_TYPE_BTREE, "ignored", O_RDONLY, 0);
	msg_capt_stop(capture);
	got_logging = msg_capt_expose(capture);

	if ((got_whoami = dict_get(mkmap->dict, "whoami")) == 0) {
	    msg_warn("'whoami' query returned 'not found'");
	    status = FAIL;
	} else if (strstr(got_whoami, want_whoami) == 0) {
	    msg_warn("unexpected result; got: '%s', want: '%s'",
		     got_whoami, want_whoami);
	    status = FAIL;
	} else if (want_logging == 0 && *got_logging != 0) {
	    msg_warn("unexpected logging: got: '%s'", got_logging);
	    status = FAIL;
	} else if (want_logging && strstr(got_logging, want_logging) == 0) {
	    msg_warn("unexpected logging: got: '%s', want: '%s'",
		     got_logging, want_logging);
	    status = FAIL;
	}
	mkmap_close(mkmap);
    }

    return (status);
}

static int nbdb_dict_xxx_open_handles_mapping_error(const TEST_CASE *tp)
{
    DICT   *dict;
    int     status = PASS;
    MSG_CAPTURE *capture;
    const char *got_warning;
    const char *want_warning;

    /*
     * Setup.
     */
    nbdb_util_init("enable-reindex");
    dict_allow_surrogate = 1;
    capture = msg_capt_create(100);
    var_nbdb_log_redirect = false;

    /*
     * Run the test for nbdb_dict_hash_open().
     */
    var_db_type = "foo";

    msg_capt_start(capture);
    dict = dict_open3(DICT_TYPE_HASH, "ignored", O_RDONLY, 0);
    msg_capt_stop(capture);

    got_warning = msg_capt_expose(capture);
    want_warning = "unsupported database type: 'foo'";

    if ((dict->flags & DICT_FLAG_SURROGATE) == 0) {
	msg_warn("unexpected hash redirect; got: non-surrogate "
		 "dictionary, want: surrogate");
	status = FAIL;
    } else if (strstr(got_warning, want_warning) == 0) {
	msg_warn("got unexpected warning: '%s', want: '%s'",
		 got_warning, want_warning);
	status = FAIL;
    }
    dict_close(dict);

    /*
     * Run the test for nbdb_dict_btree_open().
     */
    var_cache_db_type = "bar";

    msg_capt_start(capture);
    dict = dict_open3(DICT_TYPE_BTREE, "ignored", O_RDONLY, 0);
    msg_capt_stop(capture);

    got_warning = msg_capt_expose(capture);
    want_warning = "unsupported database type: 'bar'";

    if ((dict->flags & DICT_FLAG_SURROGATE) == 0) {
	msg_warn("unexpected btree redirect; got: non-surrogate "
		 "dictionary, want: surrogate");
	status = FAIL;
    } else if (strstr(got_warning, want_warning) == 0) {
	msg_warn("got unexpected warning: '%s', want: '%s'",
		 got_warning, want_warning);
	status = FAIL;
    }
    dict_close(dict);

    msg_capt_free(capture);
    return (status);
}

static int nbdb_dict_xxx_open_handles_no_suffix_error(const TEST_CASE *tp)
{
    DICT   *dict;
    int     status = PASS;
    MSG_CAPTURE *capture;
    const char *got_warning;
    const char *want_warning;

    /*
     * Setup.
     */
    nbdb_util_init("enable-reindex");
    dict_allow_surrogate = 1;
    capture = msg_capt_create(100);
    var_nbdb_log_redirect = false;

    /*
     * Run the test for nbdb_dict_hash_open().
     */
    var_db_type = "static";

    msg_capt_start(capture);
    dict = dict_open3(DICT_TYPE_HASH, "ignored", O_RDONLY, 0);
    msg_capt_stop(capture);

    got_warning = msg_capt_expose(capture);
    want_warning = "'static' has no known pathname suffix";

    if ((dict->flags & DICT_FLAG_SURROGATE) == 0) {
	msg_warn("unexpected hash redirect; got: non-surrogate "
		 "dictionary, want: surrogate");
	status = FAIL;
    } else if (strstr(got_warning, want_warning) == 0) {
	msg_warn("got unexpected warning: '%s', want: '%s'",
		 got_warning, want_warning);
	status = FAIL;
    }
    dict_close(dict);

    /*
     * Run the test for nbdb_dict_btree_open().
     */
    var_cache_db_type = "static";

    msg_capt_start(capture);
    dict = dict_open3(DICT_TYPE_BTREE, "ignored", O_RDONLY, 0);
    msg_capt_stop(capture);

    got_warning = msg_capt_expose(capture);
    want_warning = "'static' has no known pathname suffix";

    if ((dict->flags & DICT_FLAG_SURROGATE) == 0) {
	msg_warn("unexpected btree redirect; got: non-surrogate "
		 "dictionary, want: surrogate");
	status = FAIL;
    } else if (strstr(got_warning, want_warning) == 0) {
	msg_warn("got unexpected warning: '%s', want: '%s'",
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
    {.label = "redirects_dict_open_hash_to_def_db_type",
	redirects_dict_open_hash_to_def_db_type,
    },
    {.label = "redirects_dict_open_btree_to_def_cache_db_type",
	redirects_dict_open_btree_to_def_cache_db_type,
    },
    {.label = "redirects_mkmap_open_hash_to_def_db_type",
	redirects_mkmap_open_hash_to_def_db_type,
    },
    {.label = "redirects_mkmap_open_btree_to_def_cache_db_type",
	redirects_mkmap_open_btree_to_def_cache_db_type,
    },
    {.label = "nbdb_dict_xxx_open_handles_mapping_error",
	nbdb_dict_xxx_open_handles_mapping_error,
    },
    {.label = "nbdb_dict_xxx_open_handles_no_suffix_error",
	nbdb_dict_xxx_open_handles_no_suffix_error,
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
