/*	$NetBSD: nbdb_util_test.c,v 1.2.2.2 2026/05/11 17:13:49 martin Exp $	*/

/*++
/* NAME
/*	nbdb_util_test 1t
/* SUMMARY
/*	nbdb_util unit test
/* SYNOPSIS
/*	./nbdb_util_test
/* DESCRIPTION
/*	nbdb_util_test runs and logs each configured test, reports if
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
#include <mail_params.h>
#include <nbdb_util.h>
#include <nbdb_redirect.h>

 /*
  * Testing library.
  */
#include <mock_dict.h>
#include <msg_capture.h>

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

#define PASS    (0)
#define FAIL    (1)

#define BOOL_TO_TEXT(b) ((b) ? "true" : "false")

 /*
  * Scaffolding.
  */

char   *var_nbdb_cust_map = DEF_NBDB_CUST_MAP;

/* dummy nbdb_rdr_init() support */

void    nbdb_rdr_init(void)
{
}

 /*
  * tests for nbdb_util_init(),
  */
struct nbdb_util_init_test {
    const char *label;
    int     (*action) (const struct nbdb_util_init_test *);
    const char *in_level;
    bool    in_reindexing_lockout;
    int     want_level;
};

#define STR_OR_NULL(s) ((s) ? (s) : "(null)")

static int test_nbdb_util_init(const struct nbdb_util_init_test * tp)
{
    nbdb_util_init(tp->in_level);
    if (nbdb_level != tp->want_level) {
	msg_warn("unexpected nbdb_level, got: %d, want: %d",
		 nbdb_level, tp->want_level);
	return (FAIL);
    }
    return (PASS);
}

static const struct nbdb_util_init_test nbdb_util_init_tests[] = {
    {.label = "nbdb_util_init_level_none",
	.action = test_nbdb_util_init,
	.in_level = "disable",
	.want_level = NBDB_LEV_CODE_NONE,
    },
    {.label = "nbdb_util_init_level_redirect",
	.action = test_nbdb_util_init,
	.in_level = "enable-redirect",
	.want_level = NBDB_LEV_CODE_REDIRECT,
    },
    {.label = "nbdb_util_init_level_reindex",
	.action = test_nbdb_util_init,
	.in_level = "enable-reindex",
	.want_level = NBDB_LEV_CODE_REINDEX,
    },
    {.label = "nbdb_util_init_level_reindexing_with_lockout",
	.action = test_nbdb_util_init,
	.in_level = "enable-reindex",
	.in_reindexing_lockout = true,
	.want_level = NBDB_LEV_CODE_REINDEX,
    },
    /* TODO: trap msg_fatal() call after bad level name. */
    {0},
};

 /*
  * Tests for nbdb_is_legacy_type().
  */
struct nbdb_is_legacy_type_test {
    const char *label;
    int     (*action) (const struct nbdb_is_legacy_type_test *);
    const char *in_type;
    bool    want_result;
};

static int test_nbdb_is_legacy_type(const struct nbdb_is_legacy_type_test * tp)
{
    bool    got_result;

    got_result = nbdb_is_legacy_type(tp->in_type);
    if (got_result != tp->want_result) {
	msg_warn("unexpected result; got '%s', want: ''%s'",
		 BOOL_TO_TEXT(got_result), BOOL_TO_TEXT(tp->want_result));
	return (FAIL);
    }
    return (PASS);
}

static const struct nbdb_is_legacy_type_test nbdb_is_legacy_type_tests[] = {
    {.label = "nbdb_is_legacy_type_hash_true",
	.action = test_nbdb_is_legacy_type,
	.in_type = "hash",
	.want_result = true,
    },
    {.label = "nbdb_is_legacy_type_btree_true",
	.action = test_nbdb_is_legacy_type,
	.in_type = "btree",
	.want_result = true,
    },
    {.label = "nbdb_is_legacy_type_lmdb_false",
	.action = test_nbdb_is_legacy_type,
	.in_type = "lmdb",
	.want_result = false,
    },
    {.label = "nbdb_is_legacy_type_cdb_false",
	.action = test_nbdb_is_legacy_type,
	.in_type = "cdb",
	.want_result = false,
    },
    {0},
};

 /*
  * Tests for find_non_leg_suffix().
  */
struct find_non_leg_suffix_test {
    const char *label;
    int     (*action) (const struct find_non_leg_suffix_test *);
    const char *in_type;
    const char *want_suffix;
};

#define STR_OR_NULL(s) ((s) ? (s) : "(null)")

static int test_find_non_leg_suffix(const struct find_non_leg_suffix_test * tp)
{
    const char *got_suffix;

    got_suffix = nbdb_find_non_leg_suffix(tp->in_type);
    if (!got_suffix != !tp->want_suffix) {
	msg_warn("unexpected suffix; got: '%s', want: '%s'",
		 STR_OR_NULL(got_suffix), STR_OR_NULL(tp->want_suffix));
	return (FAIL);
    }
    if (got_suffix && strcmp(got_suffix, tp->want_suffix) != 0) {
	msg_warn("unexpected suffix; got: '%s',	want: '%s'",
		 got_suffix, tp->want_suffix);
	return (FAIL);
    }
    return (PASS);
}

static const struct find_non_leg_suffix_test find_non_leg_suffix_tests[] = {
    {.label = "find_non_leg_suffix_lmdb",
	.action = test_find_non_leg_suffix,
	.in_type = "lmdb",
	.want_suffix = ".lmdb",
    },
    {.label = "find_non_leg_suffix_cdb",
	.action = test_find_non_leg_suffix,
	.in_type = "cdb",
	.want_suffix = ".cdb",
    },
    {.label = "find_non_leg_suffix_hash",
	.action = test_find_non_leg_suffix,
	.in_type = "hash",
	.want_suffix = 0,
    },
    {.label = "find_non_leg_suffix_btree",
	.action = test_find_non_leg_suffix,
	.in_type = "btree",
	.want_suffix = 0,
    },
    {0},
};

 /*
  * Tests for map_leg_type().
  */
struct map_leg_type_test {
    const char *label;
    int     (*action) (const struct map_leg_type_test *);
    const char *in_db_type;
    const char *in_cache_db_type;
    const char *in_custom_map;
    const char *in_type;
    const char *in_name;
    const char *want_type;
    const char *want_why;
    const char *want_warning;
};

static int test_map_leg_type(const struct map_leg_type_test * tp)
{
    VSTRING *got_why = vstring_alloc(100);
    MSG_CAPTURE *capture = msg_capt_create(100);
    char   *saved_db_type = var_db_type;
    char   *saved_cache_db_type = var_cache_db_type;
    char   *saved_cust_map = var_nbdb_cust_map;
    const char *got_type;
    const char *got_warning;
    int     status = PASS;

    /* Setup. */
    setup_mock_cdb("{{x = x}}");
    setup_mock_lmdb("{{x = x}}");
    dict_allow_surrogate = 1;
    var_nbdb_cust_map = (char *) tp->in_custom_map;
    var_db_type = (char *) tp->in_db_type;
    var_cache_db_type = (char *) tp->in_cache_db_type;

    msg_capt_start(capture);
    nbdb_util_init("enable-redirect");
    got_type = nbdb_map_leg_type(tp->in_type, tp->in_name,
				 (const DICT_OPEN_INFO **) 0, got_why);
    msg_capt_stop(capture);
    got_warning = msg_capt_expose(capture);

    /* Verify results. */
    if (!LEN(got_why) != !tp->want_why) {
	msg_warn("got error text: `%s', want: `%s'",
		 STR(got_why), STR_OR_NULL(tp->want_why));
	status = FAIL;
    } else if (tp->want_why && strstr(STR(got_why), tp->want_why) == 0) {
	msg_warn("got error text: `%s', want: `%s'",
		 STR(got_why), tp->want_why);
	status = FAIL;
    } else if (!tp->want_warning != !got_warning[0]) {
	msg_warn("got warning: '%s', want: '%s'",
		 got_warning, STR_OR_NULL(tp->want_warning));
	status = FAIL;
    } else if (tp->want_warning && strstr(got_warning, tp->want_warning) == 0) {
	msg_warn("got warning: '%s', want: '%s'",
		 got_warning, tp->want_warning);
	status = FAIL;
    } else if (!got_type != !tp->want_type) {
	msg_warn("got unexpected type: '%s', want: '%s'",
		 STR_OR_NULL(got_type), STR_OR_NULL(tp->want_type));
	status = FAIL;
    } else if (got_type && strcmp(got_type, tp->want_type) != 0) {
	msg_warn("got unexpected type: '%s', want: '%s'",
		 got_type, tp->want_type);
	status = FAIL;
    }
    var_nbdb_cust_map = saved_cust_map;
    var_db_type = saved_db_type;
    var_cache_db_type = saved_cache_db_type;
    vstring_free(got_why);
    msg_capt_free(capture);
    return (status);
}

static const struct map_leg_type_test map_leg_type_tests[] = {
    {"map_leg_type:default_hash_to_cdb",
	.action = test_map_leg_type,
	.in_db_type = "cdb",
	.in_cache_db_type = "lmdb",
	.in_custom_map = "",
	.in_type = "hash",
	.in_name = "/some/where/blah",
	.want_type = "cdb",
    },
    {"map_leg_type:default_btree_to_lmdb",
	.action = test_map_leg_type,
	.in_db_type = "cdb",
	.in_cache_db_type = "lmdb",
	.in_custom_map = "",
	.in_type = "btree",
	.in_name = "/some/where/blah",
	.want_type = "lmdb",
    },
    {"map_leg_type:custom_hash_with_path_to_lmdb",
	.action = test_map_leg_type,
	.in_db_type = "cdb",
	.in_cache_db_type = "lmdb",
	.in_custom_map = "inline:{{hash:/some/where/blah = lmdb}}",
	.in_type = "hash",
	.in_name = "/some/where/blah",
	.want_type = "lmdb",
    },
    {"map_leg_type:custom_btree_with_path_to_cdb",
	.action = test_map_leg_type,
	.in_db_type = "cdb",
	.in_cache_db_type = "lmdb",
	.in_custom_map = "inline:{{btree:/some/where/blah = cdb}}",
	.in_type = "btree",
	.in_name = "/some/where/blah",
	.want_type = "cdb",
    },
    {"map_leg_type:custom_hash_to_lmdb",
	.action = test_map_leg_type,
	.in_db_type = "cdb",
	.in_cache_db_type = "lmdb",
	.in_custom_map = "inline:{{hash = lmdb}}",
	.in_type = "hash",
	.in_name = "/some/where/blah",
	.want_type = "lmdb",
    },
    {"map_leg_type:custom_mapping_type_must_not_use_hash",
	.action = test_map_leg_type,
	.in_db_type = "cdb",
	.in_cache_db_type = "lmdb",
	.in_custom_map = "hash:/whatever",
	.in_type = "hash",
	.in_name = "/some/where/blah",
	.want_type = 0,
	.want_why = "lookup error for 'hash:/some/where/blah'",
	.want_warning = "mapping must not use legacy type: 'hash'"
    },
    {"map_leg_type:custom_mapping_type_must_not_use_btree",
	.action = test_map_leg_type,
	.in_db_type = "cdb",
	.in_cache_db_type = "lmdb",
	.in_custom_map = "btree:/whatever",
	.in_type = "btree",
	.in_name = "/some/where/blah",
	.want_type = 0,
	.want_why = "lookup error for 'btree:/some/where/blah'",
	.want_warning = "mapping must not use legacy type: 'btree'",
    },
    {"map_leg_type:custom_hash_to_composite",
	.action = test_map_leg_type,
	.in_db_type = "cdb",
	.in_cache_db_type = "lmdb",
	.in_custom_map = "inline:{{hash = proxy:lmdb}}",
	.in_type = "hash",
	.in_name = "/some/where/blah",
	.want_type = 0,
	.want_why = "bad non-legacy database type syntax: 'proxy:lmdb'",
    },
    {"map_leg_type:custom_hash_to_legacy",
	.action = test_map_leg_type,
	.in_db_type = "cdb",
	.in_cache_db_type = "lmdb",
	.in_custom_map = "inline:{{hash = btree}}",
	.in_type = "hash",
	.in_name = "/some/where/blah",
	.want_type = 0,
	.want_why = "mapping from 'hash' to legacy type 'btree'",
    },
    {"map_leg_type:custom_hash_to_unsupported",
	.action = test_map_leg_type,
	.in_db_type = "cdb",
	.in_cache_db_type = "lmdb",
	.in_custom_map = "inline:{{hash = foo}}",
	.in_type = "hash",
	.in_name = "/some/where/blah",
	.want_type = 0,
	.want_why = "unsupported database type: 'foo'",
    },
    {0},
};

int     main(int argc, char **argv)
{
    int     pass = 0;
    int     fail = 0;

#define RUN_TEST(tp) do { \
	int     test_failed; \
	msg_info("RUN  %s", tp->label); \
	test_failed = tp->action(tp); \
	if (test_failed) { \
	    msg_info("FAIL %s", tp->label); \
	    fail++; \
	} else { \
	    msg_info("PASS %s", tp->label); \
	    pass++; \
	} \
    } while (0)

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);
    /* nbdb_util_init tests. */
    {
	const struct nbdb_util_init_test *tp;

	for (tp = nbdb_util_init_tests; tp->label != 0; tp++)
	    RUN_TEST(tp);
    }
    /* nbdb_is_legacy_type() tests. */
    {
	const struct nbdb_is_legacy_type_test *tp;

	for (tp = nbdb_is_legacy_type_tests; tp->label != 0; tp++)
	    RUN_TEST(tp);
    }
    /* find_non_leg_suffix() tests. */
    {
	const struct find_non_leg_suffix_test *tp;

	for (tp = find_non_leg_suffix_tests; tp->label != 0; tp++)
	    RUN_TEST(tp);
    }
    /* map_leg_type() tests. */
    {
	const struct map_leg_type_test *tp;

	for (tp = map_leg_type_tests; tp->label != 0; tp++)
	    RUN_TEST(tp);
    }

    msg_info("PASS=%d FAIL=%d", pass, fail);
    exit(fail != 0);
}
