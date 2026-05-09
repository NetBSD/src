/*	$NetBSD: nbdb_safe_test.c,v 1.2 2026/05/09 18:49:17 christos Exp $	*/

/*++
/*	nbdb_safe_test 1t
/* SUMMARY
/*	nbdb_safe unit test
/* SYNOPSIS
/*	./nbdb_safe_test
/* DESCRIPTION
/*	nbdb_safe_test runs and logs each configured test, reports if
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
#include <errno.h>
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
  * Global library.
  */
#include <allowed_prefix.h>
#include <mail_conf.h>
#include <mail_params.h>

 /*
  * Application-specific.
  */
#include <nbdb_safe.h>

 /*
  * Scaffolding.
  */
ALLOWED_PREFIX *parsed_allow_root_pfxs;
ALLOWED_PREFIX *parsed_allow_user_pfxs;

char   *var_nbdb_allow_root_pfxs;
char   *var_nbdb_allow_user_pfxs;

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

 /*
  * Test structure.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
    bool    want_bool;
    /* nbdb_safe_for_uid inputs. */
    uid_t   uid;
    struct stat st;
    /* nbdb_safe_to_index_as_legacy_index_owner inputs. */
    const char *source_path;
    struct stat source_st;
    const char *leg_idx_path;
    struct stat leg_idx_st;
    const char *parent_dir;
    struct stat parent_dir_st;
    const char *allow_root_pfxs;
    const char *allow_user_pfxs;
    const char *want_why;
} TEST_CASE;

#define PASS    (0)
#define FAIL    (1)

/* test_nbdb_safe_for_uid - single-line vote */

static int test_nbdb_safe_for_uid(const TEST_CASE *tp)
{
    bool    got_bool;

    /* Run the code under test. */
    got_bool = nbdb_safe_for_uid(tp->uid, &tp->st);

#define bool_to_text(b) ((b) ? "true" : "false")

    /* Verify the result. */
    if (got_bool != tp->want_bool) {
	msg_warn("got result: `%s', want `%s''",
		 bool_to_text(got_bool), bool_to_text(tp->want_bool));
	return (FAIL);
    }
    return (PASS);
}

/* test_nbdb_safe_to_index_as_legacy_index_owner - majority vote */

static int test_nbdb_safe_to_index_as_legacy_index_owner(const TEST_CASE *tp)
{
    static VSTRING *got_why;
    bool    got_bool;

    if (got_why == 0)
	got_why = vstring_alloc(100);

    /* Run the code under test. */
    VSTRING_RESET(got_why);
    var_nbdb_allow_root_pfxs = (char *) tp->allow_root_pfxs;
    var_nbdb_allow_user_pfxs = (char *) tp->allow_user_pfxs;
    parsed_allow_root_pfxs = allowed_prefix_create(var_nbdb_allow_root_pfxs);
    parsed_allow_user_pfxs = allowed_prefix_create(var_nbdb_allow_user_pfxs);
    got_bool = nbdb_safe_to_index_as_legacy_index_owner(
					    tp->source_path, &tp->source_st,
					  tp->leg_idx_path, &tp->leg_idx_st,
					 tp->parent_dir, &tp->parent_dir_st,
							got_why);
    allowed_prefix_free(parsed_allow_root_pfxs);
    allowed_prefix_free(parsed_allow_user_pfxs);

#define STR_OR_NULL(s) (((s) && *(s)) ? (s) : "(null)")

    /* Verify the result. */
    if (!LEN(got_why) != !tp->want_why) {
	msg_warn("got warning: '%s', want: %s",
		 STR_OR_NULL(STR(got_why)), STR_OR_NULL(tp->want_why));
	return (FAIL);
    }
    if (tp->want_why && strstr(STR(got_why), tp->want_why) == 0) {
	msg_warn("got warning: '%s', want: '%s'",
		 STR(got_why), tp->want_why);
	return (FAIL);
    }
    if (got_bool != tp->want_bool) {
	msg_warn("got result: '%s', want: '%s'",
		 bool_to_text(got_bool), bool_to_text(tp->want_bool));
	return (FAIL);
    }
    return (PASS);
}

 /*
  * The list of test cases.
  * 
  * TODO(wietse) Almost all tests take a 'good' test case and make one small
  * mutation. Figure out a way to mutate a copy of a 'good' test cases, and
  * store only the delta. That would make tests much more readable.
  */
static const TEST_CASE test_cases[] = {

    /*
     * nbdb_safe_for_uid tests.
     */
    {.label = "test_safe_for_root:good_configuration",
	.action = test_nbdb_safe_for_uid,
	.uid = 0,.st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.want_bool = true,
    },
    {.label = "test_safe_for_root:not_owned_by_root",
	.action = test_nbdb_safe_for_uid,
	.uid = 0,.st = {.st_uid = 1,.st_mode = S_IRWXU,},
	.want_bool = false,
    },
    {.label = "test_safe_for_root:group_writable",
	.action = test_nbdb_safe_for_uid,
	.uid = 0,.st = {.st_uid = 0,.st_mode = S_IWGRP,},
	.want_bool = false,
    },
    {.label = "test_safe_for_root:other_writable",
	.action = test_nbdb_safe_for_uid,
	.uid = 0,.st = {.st_uid = 0,.st_mode = S_IWOTH,},
	.want_bool = false,
    },
    {.label = "test_safe_for_non_root:good_root_owned_configuration",
	.action = test_nbdb_safe_for_uid,
	.uid = 1,.st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.want_bool = true,
    },
    {.label = "test_safe_for_non_root:good_user_owned_configuration",
	.action = test_nbdb_safe_for_uid,
	.uid = 1,.st = {.st_uid = 1,.st_mode = S_IRWXU,},
	.want_bool = true,
    },
    {.label = "test_safe_for_non_root:root_owned_group_write",
	.action = test_nbdb_safe_for_uid,
	.uid = 1,.st = {.st_uid = 0,.st_mode = S_IWGRP,},
	.want_bool = false,
    },
    {.label = "test_safe_for_non_root:root_owned_other_write",
	.action = test_nbdb_safe_for_uid,
	.uid = 1,.st = {.st_uid = 0,.st_mode = S_IWOTH,},
	.want_bool = false,
    },
    {.label = "test_safe_for_non_root:user_owned_group_write",
	.action = test_nbdb_safe_for_uid,
	.uid = 1,.st = {.st_uid = 1,.st_mode = S_IWGRP,},
	.want_bool = false,
    },
    {.label = "test_safe_for_non_root:user_owned_other_write",
	.action = test_nbdb_safe_for_uid,
	.uid = 1,.st = {.st_uid = 1,.st_mode = S_IWOTH,},
	.want_bool = false,
    },
    {.label = "test_safe_for_non_root:other_owned",
	.action = test_nbdb_safe_for_uid,
	.uid = 1,.st = {.st_uid = 2,.st_mode = 0,},
	.want_bool = false,
    },

    /*
     * The above tests cover all nbdb_safe_for_uid code paths. The tests
     * below do not need to cover all those code paths again, just enough to
     * demonstrate that nbdb_safe_to_index_as_legacy_index_owner() will
     * propagate nbdb_safe_for_uid() results properly.
     */

    /* nbdb_safe_to_index_as_legacy_index_owner tests. */
    {.label = "safe_to_index_as_root:good_configuration",
	.action = test_nbdb_safe_to_index_as_legacy_index_owner,
	.source_path = "/etc/postfix/access",
	.source_st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.leg_idx_path = "/etc/postfix/access.db",
	.leg_idx_st = {.st_uid = 0,},
	.parent_dir = "/etc/postfix",
	.parent_dir_st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.allow_root_pfxs = "/etc/postfix",
	.allow_user_pfxs = "",
	.want_bool = true,
    },
    /* nbdb_safe_to_index_as_legacy_index_owner tests. */
    {.label = "safe_to_index_as_root:bad_pathname_prefix",
	.action = test_nbdb_safe_to_index_as_legacy_index_owner,
	.source_path = "/etc/postfix/access",
	.source_st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.leg_idx_path = "/etc/postfix/access.db",
	.leg_idx_st = {.st_uid = 0,},
	.parent_dir = "/etc/postfix",
	.parent_dir_st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.allow_root_pfxs = "/etc/posttfix",
	.allow_user_pfxs = "",
	.want_bool = false,
	.want_why = "table /etc/postfix/access has an unexpected pathname",
    },
    {.label = "safe_to_index_as_root:bad_source_owner",
	.action = test_nbdb_safe_to_index_as_legacy_index_owner,
	.source_path = "/etc/postfix/access",
	.source_st = {.st_uid = 1,.st_mode = S_IRWXU,},
	.leg_idx_path = "/etc/postfix/access.db",
	.leg_idx_st = {.st_uid = 0,},
	.parent_dir = "/etc/postfix",
	.parent_dir_st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.allow_root_pfxs = "/etc/postfix",
	.allow_user_pfxs = "",
	.want_bool = false,
	.want_why = "'/etc/postfix/access' is owned or writable by other user",
    },
    {.label = "safe_to_index_as_root:source_group_write",
	.action = test_nbdb_safe_to_index_as_legacy_index_owner,
	.source_path = "/etc/postfix/access",
	.source_st = {.st_uid = 0,.st_mode = S_IWGRP,},
	.leg_idx_path = "/etc/postfix/access.db",
	.leg_idx_st = {.st_uid = 0,},
	.parent_dir = "/etc/postfix",
	.parent_dir_st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.allow_root_pfxs = "/etc/postfix",
	.allow_user_pfxs = "",
	.want_bool = false,
	.want_why = "'/etc/postfix/access' is owned or writable by other user",
    },
    {.label = "safe_to_index_as_root:source_other_write",
	.action = test_nbdb_safe_to_index_as_legacy_index_owner,
	.source_path = "/etc/postfix/access",
	.source_st = {.st_uid = 0,.st_mode = S_IWOTH,},
	.leg_idx_path = "/etc/postfix/access.db",
	.leg_idx_st = {.st_uid = 0,},
	.parent_dir = "/etc/postfix",
	.parent_dir_st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.allow_root_pfxs = "/etc/postfix",
	.allow_user_pfxs = "",
	.want_bool = false,
	.want_why = "'/etc/postfix/access' is owned or writable by other user",
    },
    {.label = "safe_to_index_as_root:bad_parent_owner",
	.action = test_nbdb_safe_to_index_as_legacy_index_owner,
	.source_path = "/etc/postfix/access",
	.source_st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.leg_idx_path = "/etc/postfix/access.db",
	.leg_idx_st = {.st_uid = 0,},
	.parent_dir = "/etc/postfix",
	.parent_dir_st = {.st_uid = 1,.st_mode = S_IRWXU,},
	.allow_root_pfxs = "/etc/postfix",
	.allow_user_pfxs = "",
	.want_bool = false,
	.want_why = "'/etc/postfix' is owned or writable by other user",
    },
    {.label = "safe_to_index_as_root:parent_group_write",
	.action = test_nbdb_safe_to_index_as_legacy_index_owner,
	.source_path = "/etc/postfix/access",
	.source_st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.leg_idx_path = "/etc/postfix/access.db",
	.leg_idx_st = {.st_uid = 0,},
	.parent_dir = "/etc/postfix",
	.parent_dir_st = {.st_uid = 0,.st_mode = S_IWGRP,},
	.allow_root_pfxs = "/etc/postfix",
	.allow_user_pfxs = "",
	.want_bool = false,
	.want_why = "'/etc/postfix' is owned or writable by other user",
    },
    {.label = "safe_to_index_as_root:parent_other_write",
	.action = test_nbdb_safe_to_index_as_legacy_index_owner,
	.source_path = "/etc/postfix/access",
	.source_st = {.st_uid = 0,.st_mode = S_IRWXU,},
	.leg_idx_path = "/etc/postfix/access.db",
	.leg_idx_st = {.st_uid = 0,},
	.parent_dir = "/etc/postfix",
	.parent_dir_st = {.st_uid = 0,.st_mode = S_IWOTH,},
	.allow_root_pfxs = "/etc/postfix",
	.allow_user_pfxs = "",
	.want_bool = false,
	.want_why = "'/etc/postfix' is owned or writable by other user",
    },
    /* TODO(wietse) tests for non-root user. */

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
