/*	$NetBSD: nbdb_sniffer_test.c,v 1.2 2026/05/09 18:49:17 christos Exp $	*/

/*++
/* NAME
/*	nbdb_sniffer_test 1t
/* SUMMARY
/*	nbdb_sniffer unit test
/* SYNOPSIS
/*	./nbdb_sniffer_test
/* DESCRIPTION
/*	nbdb_sniffer_test runs and logs each configured test, reports if
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
#include <open_as.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Global library.
  */

 /*
  * Testing library.
  */
#include <mock_open_as.h>

 /*
  * Application-specific.
  */
#include <nbdb_sniffer.h>

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

 /*
  * Test structure. Some tests may bring their own.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);

    /*
     * nbdb_parse_as_xxx() inputs and expectations.
     */
    struct {
	const char *in_line;
	bool    out_want_postmap;
	bool    out_want_postalias;
    }       parse_as;

    /*
     * nbdb_get_index_cmd_as() mock dependencies.
     */
    MOCK_OPEN_AS_REQ mock_open;

    /*
     * nbdb_get_index_cmd_as() inputs and expectations.
     */
    struct {
	const char *in_path;
	uid_t   in_uid;
	gid_t   in_gid;
	const char *want_cmd;
	const char *want_why;
	int     want_errno;
    }       index_cmd;
} TEST_CASE;

#define PASS    (0)
#define FAIL    (1)

/* test_parse_line - single-line vote */

static int test_parse_line(const TEST_CASE *tp)
{
    bool    got_postmap, got_postalias;
    VSTRING *buffer;

    buffer = vstring_alloc(strlen(tp->parse_as.in_line));
    vstring_strcpy(buffer, tp->parse_as.in_line);
    got_postmap = nbdb_parse_as_postmap(buffer);

    vstring_strcpy(buffer, tp->parse_as.in_line);
    got_postalias = nbdb_parse_as_postalias(buffer);
    vstring_free(buffer);

#define bool_to_text(b) ((b) ? "true" : "false")

    /* Verify the results. */
    if (got_postmap != tp->parse_as.out_want_postmap) {
	msg_warn("got postmap: `%s', want `%s''",
		 bool_to_text(got_postmap),
		 bool_to_text(tp->parse_as.out_want_postmap));
	return (FAIL);
    }
    if (got_postalias != tp->parse_as.out_want_postalias) {
	msg_warn("got postalias: `%s', want `%s''",
		 bool_to_text(got_postalias),
		 bool_to_text(tp->parse_as.out_want_postalias));
	return (FAIL);
    }
    return (PASS);
}

/* test_index_command - multi-line vote */

static int test_index_command(const TEST_CASE *tp)
{
    static VSTRING *msg_buf;
    static VSTRING *got_why;
    const char *got_cmd;

    if (msg_buf == 0) {
	msg_buf = vstring_alloc(100);
	got_why = vstring_alloc(100);
    }
    /* Run the test with fake file data. */
    VSTRING_RESET(got_why);
    setup_mock_vstream_fopen_as(&tp->mock_open);
    got_cmd = nbdb_get_index_cmd_as(tp->index_cmd.in_path,
				    tp->index_cmd.in_uid,
				    tp->index_cmd.in_gid,
				    got_why);

#define CMD_OR_NULL(s)	((s) ? (s) : "(null)")

    /* Verify the results. */
    if (!got_cmd != !tp->index_cmd.want_cmd) {
	msg_warn("got command: `%s', want `%s''",
		 CMD_OR_NULL(got_cmd), CMD_OR_NULL(tp->index_cmd.want_cmd));
	return (FAIL);
    }
    if (tp->index_cmd.want_cmd == 0) {
	if (LEN(got_why) == 0
	    || strstr(STR(got_why), tp->index_cmd.want_why) == 0) {
	    msg_warn("got warning: '%s', want: '%s'",
		     STR(got_why), tp->index_cmd.want_why);
	    return (FAIL);
	}
	if (tp->index_cmd.want_errno != errno) {
	    msg_warn("got errno: '%s', want: '%s'",
		     strerror(errno), strerror(tp->index_cmd.want_errno));
	    return (FAIL);
	}
    } else {
	if (tp->index_cmd.want_errno != 0)
	    msg_panic("want non-null result AND errno=%d",
		      tp->index_cmd.want_errno);
	if (strcmp(got_cmd, tp->index_cmd.want_cmd) != 0) {
	    msg_warn("got command ``%s'', want ``%s''",
		     got_cmd, tp->index_cmd.want_cmd);
	    return (FAIL);
	}
    }
    return (PASS);
}

 /*
  * The list of test cases.
  */
static const TEST_CASE test_cases[] = {

    /*
     * Line-level tests.
     */
    {.label = "key_space_value",
	.action = test_parse_line,
	.parse_as = {
	    .in_line = "foo bar",
	    .out_want_postmap = true,
	    .out_want_postalias = false,
	},
    },
    {.label = "colon_in_quoted_key_space_text",
	.action = test_parse_line,
	.parse_as = {
	    .in_line = "\"foobar:\" baz",
	    .out_want_postmap = true,
	    .out_want_postalias = false,
	},
    },
    {.label = "quoted_key_colon_value",
	.action = test_parse_line,
	.parse_as = {
	    .in_line = "\"foo bar\":baz",
	    .out_want_postmap = false,
	    .out_want_postalias = true,
	},
    },
    {.label = "key_space_colon_text",
	.action = test_parse_line,
	.parse_as = {
	    .in_line = "foo :bar",
	    .out_want_postmap = true,
	    .out_want_postalias = true,
	},
    },
    {.label = "key_colon_value",
	.action = test_parse_line,
	.parse_as = {
	    .in_line = "foo:bar",
	    .out_want_postmap = false,
	    .out_want_postalias = true,
	},
    },
    {.label = "colon_value",
	.action = test_parse_line,
	.parse_as = {
	    .in_line = ":bar",
	    .out_want_postmap = false,
	    .out_want_postalias = false,
	},
    },
    {.label = "key_without value",
	.action = test_parse_line,
	.parse_as = {
	    .in_line = "bar",
	    .out_want_postmap = false,
	    .out_want_postalias = false,
	},
    },
    {.label = "key_colon_without value",
	.action = test_parse_line,
	.parse_as = {
	    .in_line = "bar:",
	    .out_want_postmap = false,
	    .out_want_postalias = false,
	},
    },

    /*
     * File-level tests (multiple lines with majority voting).
     */
    {.label = "file_open_error_enoent",
	.action = test_index_command,
	.index_cmd = {
	    .in_path = "path/to/file",
	    .want_why = "open lookup table source file",
	    .want_errno = ENOENT,
	},
	.mock_open = {
	    .want_path = "path/to/file",
	    .out_data = 0,
	    .out_errno = ENOENT,
	},
    },
    {.label = "file_open_error_eacces",
	.action = test_index_command,
	.index_cmd = {
	    .in_path = "path/to/file",
	    .in_uid = 1,
	    .want_why = "open lookup table source file",
	    .want_errno = EACCES,
	},
	.mock_open = {
	    .want_path = "path/to/file",
	    .want_uid = 1,
	    .out_errno = EACCES,
	},
    },
    {.label = "mostly_postmap",
	.action = test_index_command,
	.index_cmd = {
	    .in_path = "path/to/file",
	    .want_cmd = "postmap",
	},
	.mock_open = {
	    .want_path = "path/to/file",
	    .out_data = "foo:bar\nfoo bar\nfoo :bar",
	},
    },
    {.label = "mostly_postalias",
	.action = test_index_command,
	.index_cmd = {
	    .in_path = "path/to/file",
	    .want_cmd = "postalias",
	},
	.mock_open = {
	    .want_path = "path/to/file",
	    .out_data = "foo:bar\n\"foo bar\":baz\nfoo :bar",
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
