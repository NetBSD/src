/*	$NetBSD: nbdb_process_test.c,v 1.2.2.2 2026/05/11 17:13:51 martin Exp $	*/

/*++
/* NAME
/*	nbdb_process_test 1t
/* SUMMARY
/*	nbdb_process unit test
/* SYNOPSIS
/*	./nbdb_process_test
/* DESCRIPTION
/*	nbdb_process_test runs and logs each configured test, reports if
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
#include <dict_cdb.h>
#include <dict_lmdb.h>
#include <msg.h>
#include <msg_vstream.h>
#include <mock_dict.h>
#include <mock_stat.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <allowed_prefix.h>
#include <nbdb_util.h>
#include <mail_params.h>
#include <msg_capture.h>

 /*
  * Testing library.
  */
#include <mock_spawn_command.h>
#include <mock_stat.h>
#include <mock_open_as.h>

 /*
  * Application-specific.
  */
#include <nbdb_index_as.h>
#include <nbdb_safe.h>
#include <nbdb_index_as.h>
#include <nbdb_process.h>

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

 /*
  * Scaffolding for static parameter dependencies.
  */
char   *var_command_dir = DEF_COMMAND_DIR;
char   *var_export_environ = "TZ MAIL_CONFIG LANG";
char   *var_servname = "nbdb_reindexd";
char   *var_db_type = DICT_TYPE_CDB;
char   *var_cache_db_type = DICT_TYPE_LMDB;
char   *var_nbdb_cust_map = DEF_NBDB_CUST_MAP;

 /*
  * Test-dependent configuration parameters.
  */
bool    var_nbdb_enable = 0;

char   *var_nbdb_allow_root_pfxs = DEF_NBDB_ALLOW_ROOT_PFXS;
char   *var_nbdb_allow_user_pfxs = DEF_NBDB_ALLOW_USER_PFXS;

ALLOWED_PREFIX *parsed_allow_root_pfxs;
ALLOWED_PREFIX *parsed_allow_user_pfxs;

 /*
  * Test structure. TODO(wietse) readability problem: there are many test
  * inputs, their values have a lot of overlap, and the tests data are far
  * away from the code that uses it. For readability sake, consider using
  * dedicated test-case functions with test data in local variables.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);

    /*
     * Mock dependencies.
     */
    MOCK_STAT_REQ mock_stat_source;
    MOCK_STAT_REQ mock_stat_leg_idx;
    MOCK_STAT_REQ mock_stat_parent;
    MOCK_STAT_REQ mock_stat_new_idx;
    MOCK_OPEN_AS_REQ mock_open;
    MOCK_SPAWN_CMD_REQ mock_spawn;

    /*
     * nbdb_process() inputs and expected outputs.
     */
    struct {
	const char *in_new_type;
	const char *in_source_path;
	int     want_status;
	const char *want_why;
	const char *want_msg;
    }       process;

    /*
     * Configuration parameter settings.
     */
    struct {
	const char *migr_level;
	const char *allow_root_pfxs;
	const char *allow_user_pfxs;
    }       params;
} TEST_CASE;

#define PASS    (0)
#define FAIL    (1)

/* test_nbdb_process */

static int test_nbdb_process(const TEST_CASE *tp)
{
    MSG_CAPTURE *capture;
    const char *got_msg;
    int     got_status;
    VSTRING *got_why;
    int     pass = 1;

    nbdb_util_init(tp->params.migr_level);
    setup_mock_cdb("{{x = x}}");
    setup_mock_lmdb("{{x = x}}");
    if (nbdb_level >= NBDB_LEV_CODE_REINDEX) {
	var_nbdb_allow_root_pfxs = (char *) tp->params.allow_root_pfxs;
	var_nbdb_allow_user_pfxs = (char *) tp->params.allow_user_pfxs;

	parsed_allow_root_pfxs = allowed_prefix_create(var_nbdb_allow_root_pfxs);
	parsed_allow_user_pfxs = allowed_prefix_create(var_nbdb_allow_user_pfxs);

	setup_mock_stat(&tp->mock_stat_source);
	setup_mock_stat(&tp->mock_stat_leg_idx);
	setup_mock_stat(&tp->mock_stat_parent);
	setup_mock_stat(&tp->mock_stat_new_idx);
	setup_mock_vstream_fopen_as(&tp->mock_open);
	setup_mock_spawn_command(&tp->mock_spawn);
    }
    capture = msg_capt_create(100);
    got_why = vstring_alloc(100);

    msg_capt_start(capture);
    got_status =
	nbdb_process(tp->process.in_new_type, tp->process.in_source_path,
		     got_why);
    msg_capt_stop(capture);
    got_msg = msg_capt_expose(capture);

    if (tp->process.want_why == 0 && LEN(got_why) > 0) {
	msg_warn("got unexpected reason '%s', want (none)", STR(got_why));
	pass = 0;
    }
    if (tp->process.want_msg == 0 && *got_msg != 0) {
	msg_warn("got unexpected message '%s', want (none)", got_msg);
	pass = 0;
    }
    if (tp->process.want_msg && strstr(got_msg, tp->process.want_msg) == 0) {
	msg_warn("got unexpected message '%s', want '%s'",
		 got_msg, tp->process.want_msg);
	pass = 0;
    }
    if (got_status != tp->process.want_status) {
	msg_warn("got status '%d', want '%d'", got_status,
		 tp->process.want_status);
	pass = 0;
    } else if (got_status != 0
	       && strstr(STR(got_why), tp->process.want_why) == 0) {
	msg_warn("got reason '%s', want '%s'", STR(got_why),
		 tp->process.want_why);
	pass = 0;
    }
    vstring_free(got_why);
    msg_capt_free(capture);
    if (nbdb_level >= NBDB_LEV_CODE_REINDEX) {
	allowed_prefix_free(parsed_allow_root_pfxs);
	allowed_prefix_free(parsed_allow_user_pfxs);
    }
    return (pass ? PASS : FAIL);
}

 /*
  * The list of test cases. If you want to figure out what the tests do, the
  * data below shows their recipe.
  */
static const TEST_CASE test_cases[] = {

    {.label = "normal_completion",
	.action = test_nbdb_process,
	.mock_stat_source = {
	    .want_path = "/path/to/file",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_leg_idx = {
	    .want_path = "/path/to/file.db",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_parent = {
	    .want_path = "/path/to",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_new_idx = {
	    .want_path = "/path/to/file.cdb",
	    .out_errno = ENOENT,
	},
	.mock_open = {
	    .want_path = "/path/to/file",
	    .want_uid = 0,
	    .want_gid = 0,
	    .out_errno = 0,
	    .out_data = "foo bar\nfoo :bar",
	},
	.mock_spawn = {
	    .want_args = {
		.argv = (char *[]) {DEF_COMMAND_DIR "/postmap", "cdb:/path/to/file", 0,},
		.uid = 0,
		.gid = 0,
		.export = (char *[]) {"TZ", "MAIL_CONFIG", "LANG", 0,},
		.time_limit = 100,
	    },
	    .out_status = 0,
	    .out_text = 0,
	},
	.params = {
	    .allow_root_pfxs = "/path/to",
	    .allow_user_pfxs = "/home/foo",
	    .migr_level = "enable-reindex",
	},
	.process = {
	    .in_new_type = "hash",
	    .in_source_path = "/path/to/file",
	    .want_status = NBDB_STAT_OK,
	    .want_why = 0,
	    .want_msg = "successfully executed",
	},
    },

    {.label = "respects_migr_level_disable",
	.action = test_nbdb_process,
	.params = {
	    .migr_level = "disable",
	},
	.process = {
	    .want_status = NBDB_STAT_ERROR,
	    .want_why = "service is disabled",
	},
    },

    {.label = "propagates_nbdb_index_as_error",
	.action = test_nbdb_process,
	.mock_stat_source = {
	    .want_path = "/path/to/file",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_leg_idx = {
	    .want_path = "/path/to/file.db",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_parent = {
	    .want_path = "/path/to",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_new_idx = {
	    .want_path = "/path/to/file.cdb",
	    .out_errno = ENOENT,
	},
	.mock_open = {
	    .want_path = "/path/to/file",
	    .want_uid = 0,
	    .want_gid = 0,
	    .out_errno = 0,
	    .out_data = "foo bar\nfoo :bar",
	},
	.mock_spawn = {
	    .want_args = {
		.argv = (char *[]) {DEF_COMMAND_DIR "/postmap", "cdb:/path/to/file", 0,},
		.uid = 0,
		.gid = 0,
		.export = (char *[]) {"TZ", "MAIL_CONFIG", "LANG", 0,},
		.time_limit = 100,
	    },
	    .out_status = NBDB_STAT_ERROR,
	    .out_text = "sorry, I cannot do that",
	},
	.params = {
	    .allow_root_pfxs = "/path/to",
	    .allow_user_pfxs = "/home/foo",
	    .migr_level = "enable-reindex",
	},
	.process = {
	    .in_new_type = "hash",
	    .in_source_path = "/path/to/file",
	    .want_status = NBDB_STAT_ERROR,
	    .want_why = "cannot do that",
	},
    },

    {.label = "propagates_nbdb_safe_to_index_error",
	.action = test_nbdb_process,
	.mock_stat_source = {
	    .want_path = "/path/to/file",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_leg_idx = {
	    .want_path = "/path/to/file.db",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_parent = {
	    .want_path = "/path/to",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IWGRP,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_new_idx = {
	    .want_path = "/path/to/file.cdb",
	    .out_errno = ENOENT,
	},
	.mock_open = {
	    .want_path = "/path/to/file",
	    .want_uid = 0,
	    .want_gid = 0,
	    .out_errno = 0,
	    .out_data = "foo bar\nfoo :bar",
	},
	.params = {
	    .allow_root_pfxs = "/path/to",
	    .allow_user_pfxs = "/home/foo",
	    .migr_level = "enable-reindex",
	},
	.process = {
	    .in_new_type = "hash",
	    .in_source_path = "/path/to/file",
	    .want_status = NBDB_STAT_ERROR,
	    .want_why = "owned or writable by other user",
	},
    },

    {.label = "propagates_nbdb_sniffer_error",
	.action = test_nbdb_process,
	.mock_stat_source = {
	    .want_path = "/path/to/file",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_leg_idx = {
	    .want_path = "/path/to/file.db",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_parent = {
	    .want_path = "/path/to",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 0,.st_gid = 0,},
	},
	.mock_stat_new_idx = {
	    .want_path = "/path/to/file.cdb",
	    .out_errno = ENOENT,
	},
	.mock_open = {
	    .want_path = "/path/to/file",
	    .want_uid = 0,
	    .want_gid = 0,
	    .out_errno = EACCES,
	    .out_data = 0,
	},
	.params = {
	    .allow_root_pfxs = "/path/to",
	    .allow_user_pfxs = "/home/foo",
	    .migr_level = "enable-reindex",
	},
	.process = {
	    .in_new_type = "hash",
	    .in_source_path = "/path/to/file",
	    .want_status = NBDB_STAT_ERROR,
	    .want_why = "open lookup table source file /path/to/file",
	},
    },

    /*
     * Pathname prefix safety is enforced by nbdb_safe_to_index_as_xxx but it
     * is important enough to have its own end-to-end test.
     */
    {.label = "respects_allow_root_prefix",
	.action = test_nbdb_process,
	.mock_stat_source = {
	    .want_path = "/path/to/file",
	    .out_errno = 0,
	    .out_st = {
		.st_mode = S_IRWXU,.st_uid = 1,.st_gid = 0,
	    },
	},
	.mock_stat_leg_idx = {
	    .want_path = "/path/to/file.db",
	    .out_errno = 0,
	    .out_st = {
		.st_mode = S_IRWXU,.st_uid = 1,.st_gid = 0,
	    },
	},
	.mock_stat_parent = {
	    .want_path = "/path/to",
	    .out_errno = 0,
	    .out_st = {
		.st_mode = S_IRWXU,.st_uid = 1,.st_gid = 0,
	    },
	},
	.mock_stat_new_idx = {
	    .want_path = "/path/to/file.cdb",
	    .out_errno = ENOENT,
	},
	.mock_open = {
	    .want_path = "/path/to/file",
	    .want_uid = 1,
	    .want_gid = 0,
	    .out_errno = 0,
	    .out_data = "foo bar\nfoo :bar",
	},
	.params = {
	    .allow_root_pfxs = "/other/path",
	    .allow_user_pfxs = "/home/foo",
	    .migr_level = "enable-reindex",
	},
	.process = {
	    .in_new_type = "hash",
	    .in_source_path = "/path/to/file",
	    .want_status = NBDB_STAT_ERROR,
	    .want_why = "could not execute command 'postmap cdb:/path/to/file': table /path/to/file has an unexpected pathname",
	},
    },

    {.label = "normal_completion_non_root_case",
	.action = test_nbdb_process,
	.mock_stat_source = {
	    .want_path = "/path/to/file",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 1,.st_gid = 0,},
	},
	.mock_stat_leg_idx = {
	    .want_path = "/path/to/file.db",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 1,.st_gid = 0,},
	},
	.mock_stat_parent = {
	    .want_path = "/path/to",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 1,.st_gid = 0,},
	},
	.mock_stat_new_idx = {
	    .want_path = "/path/to/file.cdb",
	    .out_errno = ENOENT,
	},
	.mock_open = {
	    .want_path = "/path/to/file",
	    .want_uid = 1,
	    .want_gid = 0,
	    .out_errno = 0,
	    .out_data = "foo bar\nfoo :bar",
	},
	.mock_spawn = {
	    .want_args = {
		.argv = (char *[]) {DEF_COMMAND_DIR "/postmap", "cdb:/path/to/file", 0,},
		.uid = 1,
		.gid = 0,
		.export = (char *[]) {"TZ", "MAIL_CONFIG", "LANG", 0,},
		.time_limit = 100,
	    },
	    .out_status = 0,
	    .out_text = 0,
	},
	.params = {
	    .allow_root_pfxs = "/somewhere",
	    .allow_user_pfxs = "/path/to",
	    .migr_level = "enable-reindex",
	},
	.process = {
	    .in_new_type = "hash",
	    .in_source_path = "/path/to/file",
	    .want_status = NBDB_STAT_OK,
	    .want_why = 0,
	    .want_msg = "successfully executed",
	},
    },

    {.label = "respects_allow_user_prefix",
	.action = test_nbdb_process,
	.mock_stat_source = {
	    .want_path = "/path/to/file",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 1,.st_gid = 0,},
	},
	.mock_stat_leg_idx = {
	    .want_path = "/path/to/file.db",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 1,.st_gid = 0,},
	},
	.mock_stat_parent = {
	    .want_path = "/path/to",
	    .out_errno = 0,
	    .out_st = {.st_mode = S_IRWXU,.st_uid = 1,.st_gid = 0,},
	},
	.mock_stat_new_idx = {
	    .want_path = "/path/to/file.cdb",
	    .out_errno = ENOENT,
	},
	.mock_open = {
	    .want_path = "/path/to/file",
	    .want_uid = 0,
	    .want_gid = 0,
	    .out_errno = 0,
	    .out_data = "foo bar\nfoo :bar",
	},
	.params = {
	    .allow_root_pfxs = "/somewhere",
	    .allow_user_pfxs = "/other/path",
	    .migr_level = "enable-reindex",
	},
	.process = {
	    .in_new_type = "hash",
	    .in_source_path = "/path/to/file",
	    .want_status = NBDB_STAT_ERROR,
	    .want_why = "unexpected pathname",
	},
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
