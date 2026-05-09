/*	$NetBSD: nbdb_index_as_test.c,v 1.2 2026/05/09 18:49:17 christos Exp $	*/

/*++
/* NAME
/*	nbdb_index_as_test 1t
/* SUMMARY
/*	index_as unit test
/* SYNOPSIS
/*	./nbdb_index_as_test
/* DESCRIPTION
/*	index_as_test runs and logs each configured test, reports if
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
#include <vstream.h>
#include <vstring.h>
#include <spawn_command.h>

 /*
  * Global library.
  */
#include <nbdb_util.h>

 /*
  * Testing library.
  */
#include <mock_spawn_command.h>

 /*
  * Application-specific.
  */
#include <nbdb_index_as.h>

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

 /*
  * Scaffolding for parameter dependencies.
  */
char   *var_command_dir = DEF_COMMAND_DIR;
char   *var_export_environ = "TZ MAIL_CONFIG LANG";

 /*
  * Test structure. Some tests may bring their own.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);

    /*
     * nbdb_index_as() mock dependencies.
     */
    MOCK_SPAWN_CMD_REQ mock_spawn;

    /*
     * nbdb_index_as() inputs and expected outputs.
     */
    struct index_info {
	const char *in_command;
	const char *in_new_type;
	const char *in_source_path;
	uid_t   in_uid;
	gid_t   in_gid;
	int     want_status;
	const char *want_why;
    }       index;
} TEST_CASE;

#define PASS    (0)
#define FAIL    (1)

/* test_nbdb_index_as */

static int test_nbdb_index_as(const TEST_CASE *tp)
{
    int     got_status;
    VSTRING *got_why;
    int     pass = 1;

    setup_mock_spawn_command(&tp->mock_spawn);
    got_why = vstring_alloc(100);
    got_status =
	nbdb_index_as(tp->index.in_command, tp->index.in_new_type,
		      tp->index.in_source_path, tp->index.in_uid,
		      tp->index.in_gid, got_why);

    if (got_status != tp->index.want_status) {
	msg_warn("got status '%d', want '%d'", got_status,
		 tp->index.want_status);
	pass = 0;
    } else if (got_status != 0
	       && strstr(STR(got_why), tp->index.want_why) == 0) {
	msg_warn("got reason '%s' want '%s'", STR(got_why),
		 tp->index.want_why);
	pass = 0;
    }
    vstring_free(got_why);
    return (pass ? PASS : FAIL);
}

 /*
  * The list of test cases.
  */
static const TEST_CASE test_cases[] = {
    {.label = "normal_completion",
	.action = test_nbdb_index_as,
	.mock_spawn = {
	    .want_args = {
		.argv = (char *[]) {DEF_COMMAND_DIR "/postmap", "lmdb:/path/to/foo", 0,},
		.uid = 0,
		.gid = 0,
		.export = (char *[]) {"TZ", "MAIL_CONFIG", "LANG", 0,},
		.time_limit = 100,
	    },
	    .out_status = NBDB_STAT_OK,
	    .out_text = 0,
	},
	.index = {
	    .in_command = "postmap",
	    .in_new_type = "lmdb",
	    .in_source_path = "/path/to/foo",
	    .in_uid = 0,
	    .in_gid = 0,
	    .want_status = NBDB_STAT_OK,
	    .want_why = 0,
	},
    },
    {.label = "propagates_non_zero_status_and_text",
	.action = test_nbdb_index_as,
	.mock_spawn = {
	    .want_args = {
		.argv = (char *[]) {DEF_COMMAND_DIR "/postmap", "lmdb:/path/to/foo", 0,},
		.uid = 0,
		.gid = 0,
		.export = (char *[]) {"TZ", "MAIL_CONFIG", "LANG", 0,},
		.time_limit = 100,
	    },
	    .out_status = NBDB_STAT_ERROR,
	    .out_text = "sorry, I cannot do that",
	},
	.index = {
	    .in_command = "postmap",
	    .in_new_type = "lmdb",
	    .in_source_path = "/path/to/foo",
	    .in_uid = 0,
	    .in_gid = 0,
	    .want_status = NBDB_STAT_ERROR,
	    .want_why = "I cannot do that",
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
