/*	$NetBSD: mail_addr_map.c,v 1.3 2020/03/18 19:05:16 christos Exp $	*/

/*++
/* NAME
/*	mail_addr_map 3
/* SUMMARY
/*	generic address mapping
/* SYNOPSIS
/*	#include <mail_addr_map.h>
/*
/*	ARGV	*mail_addr_map_internal(path, address, propagate)
/*	MAPS	*path;
/*	const char *address;
/*	int	propagate;
/*
/*	ARGV	*mail_addr_map_opt(path, address, propagate, in_form,
/*					query_form, out_form)
/*	MAPS	*path;
/*	const char *address;
/*	int	propagate;
/*	int	in_form;
/*	int	query_form;
/*	int	out_form;
/* DESCRIPTION
/*	mail_addr_map_*() returns the translation for the named address,
/*	or a null pointer if none is found.
/*
/*	With mail_addr_map_internal(), the search address and results
/*	are in internal (unquoted) form.
/*
/*	mail_addr_map_opt() gives more control, at the cost of additional
/*	conversions between internal and external forms.
/*
/*	When the \fBpropagate\fR argument is non-zero,
/*	address extensions that aren't explicitly matched in the lookup
/*	table are propagated to the result addresses. The caller is
/*	expected to pass the lookup result to argv_free().
/*
/*	Lookups are performed by mail_addr_find_*(). When the result has the
/*	form \fI@otherdomain\fR, the result is the original user in
/*	\fIotherdomain\fR.
/*
/*	Arguments:
/* .IP path
/*	Dictionary search path (see maps(3)).
/* .IP address
/*	The address to be looked up in external (quoted) form, or
/*	in the form specified with the in_form argument.
/* .IP query_form
/*	Database query address forms, either MA_FORM_INTERNAL (unquoted
/*	form), MA_FORM_EXTERNAL (quoted form), MA_FORM_EXTERNAL_FIRST
/*	(external, then internal if the forms differ), or
/*	MA_FORM_INTERNAL_FIRST (internal, then external if the forms
/*	differ).
/* .IP in_form
/* .IP out_form
/*	Input and output address forms, either MA_FORM_INTERNAL (unquoted
/*	form) or MA_FORM_EXTERNAL (quoted form).
/* DIAGNOSTICS
/*	Warnings: map lookup returns a non-address result.
/*
/*	The path->error value is non-zero when the lookup
/*	failed with a non-permanent error.
/* SEE ALSO
/*	mail_addr_find(3), mail address matching
/*	mail_addr_crunch(3), mail address parsing and rewriting
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <dict.h>
#include <argv.h>
#include <mymalloc.h>

/* Global library. */

#include <quote_822_local.h>
#include <mail_addr_find.h>
#include <mail_addr_crunch.h>
#include <mail_addr_map.h>

/* Application-specific. */

#define STR	vstring_str
#define LEN	VSTRING_LEN

/* mail_addr_map - map a canonical address */

ARGV   *mail_addr_map_opt(MAPS *path, const char *address, int propagate,
			          int in_form, int query_form, int out_form)
{
    VSTRING *buffer = 0;
    const char *myname = "mail_addr_map";
    const char *string;
    char   *ratsign;
    char   *extension = 0;
    ARGV   *argv = 0;
    int     i;
    VSTRING *int_address = 0;
    VSTRING *ext_address = 0;
    const char *int_addr;

    /*
     * Optionally convert input from external form. We prefer internal-form
     * input to avoid unnecessary input conversion in mail_addr_find_opt().
     */
    if (in_form == MA_FORM_EXTERNAL) {
	int_address = vstring_alloc(100);
	unquote_822_local(int_address, address);
	int_addr = STR(int_address);
	in_form = MA_FORM_INTERNAL;
    } else {
	int_addr = address;
    }

    /*
     * Look up the full address; if no match is found, look up the address
     * with the extension stripped off, and remember the unmatched extension.
     */
    if ((string = mail_addr_find_opt(path, int_addr, &extension,
				     in_form, query_form,
				     MA_FORM_EXTERNAL,
				     MA_FIND_DEFAULT)) != 0) {

	/*
	 * Prepend the original user to @otherdomain, but do not propagate
	 * the unmatched address extension. Convert the address to external
	 * form just like the mail_addr_find_opt() output.
	 */
	if (*string == '@') {
	    buffer = vstring_alloc(100);
	    if ((ratsign = strrchr(int_addr, '@')) != 0)
		vstring_strncpy(buffer, int_addr, ratsign - int_addr);
	    else
		vstring_strcpy(buffer, int_addr);
	    if (extension)
		vstring_truncate(buffer, LEN(buffer) - strlen(extension));
	    vstring_strcat(buffer, string);
	    ext_address = vstring_alloc(2 * LEN(buffer));
	    quote_822_local(ext_address, STR(buffer));
	    string = STR(ext_address);
	}

	/*
	 * Canonicalize the result, and propagate the unmatched extension to
	 * each address found.
	 */
	argv = mail_addr_crunch_opt(string, propagate ? extension : 0,
				    MA_FORM_EXTERNAL, out_form);
	if (buffer)
	    vstring_free(buffer);
	if (ext_address)
	    vstring_free(ext_address);
	if (msg_verbose)
	    for (i = 0; i < argv->argc; i++)
		msg_info("%s: %s -> %d: %s", myname, address, i, argv->argv[i]);
	if (argv->argc == 0) {
	    msg_warn("%s lookup of %s returns non-address result \"%s\"",
		     path->title, address, string);
	    argv = argv_free(argv);
	    path->error = DICT_ERR_RETRY;
	}
    }

    /*
     * No match found.
     */
    else {
	if (msg_verbose)
	    msg_info("%s: %s -> %s", myname, address,
		     path->error ? "(try again)" : "(not found)");
    }

    /*
     * Cleanup.
     */
    if (extension)
	myfree(extension);
    if (int_address)
	vstring_free(int_address);

    return (argv);
}

#ifdef TEST

/*
 * SYNOPSIS
 *	mail_addr_map pass_tests | fail_tests
 * DESCRIPTION
 *	mail_addr_map performs the specified set of built-in
 *	unit tests. With 'pass_tests', all tests must pass, and
 *	with 'fail_tests' all tests must fail.
 * DIAGNOSTICS
 *	When a unit test fails, the program prints details of the
 *	failed test.
 *
 *	The program terminates with a non-zero exit status when at
 *	least one test does not pass with 'pass_tests', or when at
 *	least one test does not fail with 'fail_tests'.
 */

/* System library. */

#include <sys_defs.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Utility library. */

#include <argv.h>
#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>

/* Global library. */

#include <canon_addr.h>
#include <mail_addr_map.h>
#include <mail_params.h>

/* Application-specific. */

#define STR	vstring_str

typedef struct {
    const char *testname;
    const char *database;
    int     propagate;
    const char *delimiter;
    int     in_form;
    int     query_form;
    int     out_form;
    const char *address;
    const char *expect_argv[2];
    int     expect_argc;
} MAIL_ADDR_MAP_TEST;

#define DONT_PROPAGATE_UNMATCHED_EXTENSION	0
#define DO_PROPAGATE_UNMATCHED_EXTENSION	1
#define NO_RECIPIENT_DELIMITER			""
#define PLUS_RECIPIENT_DELIMITER		"+"
#define DOT_RECIPIENT_DELIMITER			"."

 /*
  * All these tests must pass, so that we know that mail_addr_map_opt() works
  * as intended. mail_addr_map() has always been used for maps that expect
  * external-form queries, so there are no tests here for internal-form
  * queries.
  */
static MAIL_ADDR_MAP_TEST pass_tests[] = {
    {
	"1 external -external-> external, no extension",
	"inline:{ aa@example.com=bb@example.com }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_EXTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
	"aa@example.com",
	{"bb@example.com"}, 1,
    },
    {
	"2 external -external-> external, extension, propagation",
	"inline:{ aa@example.com=bb@example.com }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_EXTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
	"aa+ext@example.com",
	{"bb+ext@example.com"}, 1,
    },
    {
	"3 external -external-> external, extension, no propagation, no match",
	"inline:{ aa@example.com=bb@example.com }",
	DONT_PROPAGATE_UNMATCHED_EXTENSION, NO_RECIPIENT_DELIMITER,
	MA_FORM_EXTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
	"aa+ext@example.com",
	{0}, 0,
    },
    {
	"4 external -external-> external, extension, full match",
	"inline:{{cc+ext@example.com=dd@example.com,ee@example.com}}",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_EXTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
	"cc+ext@example.com",
	{"dd@example.com", "ee@example.com"}, 2,
    },
    {
	"5 external -external-> external, no extension, quoted",
	"inline:{ {\"a a\"@example.com=\"b b\"@example.com} }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_EXTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
	"\"a a\"@example.com",
	{"\"b b\"@example.com"}, 1,
    },
    {
	"6 external -external-> external, extension, propagation, quoted",
	"inline:{ {\"a a\"@example.com=\"b b\"@example.com} }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_EXTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
	"\"a a+ext\"@example.com",
	{"\"b b+ext\"@example.com"}, 1,
    },
    {
	"7 internal -external-> internal, no extension, propagation, embedded space",
	"inline:{ {\"a a\"@example.com=\"b b\"@example.com} }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_INTERNAL, MA_FORM_EXTERNAL, MA_FORM_INTERNAL,
	"a a@example.com",
	{"b b@example.com"}, 1,
    },
    {
	"8 internal -external-> internal, extension, propagation, embedded space",
	"inline:{ {\"a a\"@example.com=\"b b\"@example.com} }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_INTERNAL, MA_FORM_EXTERNAL, MA_FORM_INTERNAL,
	"a a+ext@example.com",
	{"b b+ext@example.com"}, 1,
    },
    {
	"9 internal -external-> internal, no extension, propagation, embedded space",
	"inline:{ {a_a@example.com=\"b b\"@example.com} }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_INTERNAL, MA_FORM_EXTERNAL, MA_FORM_INTERNAL,
	"a_a@example.com",
	{"b b@example.com"}, 1,
    },
    {
	"10 internal -external-> internal, extension, propagation, embedded space",
	"inline:{ {a_a@example.com=\"b b\"@example.com} }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_INTERNAL, MA_FORM_EXTERNAL, MA_FORM_INTERNAL,
	"a_a+ext@example.com",
	{"b b+ext@example.com"}, 1,
    },
    {
	"11 internal -external-> internal, no extension, @domain",
	"inline:{ {@example.com=@example.net} }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_INTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
	"a@a@example.com",
	{"\"a@a\"@example.net"}, 1,
    },
    {
        "12 external -external-> external, extension, propagation",
        "inline:{ aa@example.com=bb@example.com }",
        DO_PROPAGATE_UNMATCHED_EXTENSION, DOT_RECIPIENT_DELIMITER,
        MA_FORM_EXTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
        "aa.ext@example.com",
        {"bb.ext@example.com"}, 1,
    },
    0,
};

 /*
  * All these tests must fail, so that we know that the tests work.
  */
static MAIL_ADDR_MAP_TEST fail_tests[] = {
    {
	"selftest 1 external -external-> external, no extension, quoted",
	"inline:{ {\"a a\"@example.com=\"b b\"@example.com} }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_EXTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
	"\"a a\"@example.com",
	{"\"bXb\"@example.com"}, 1,
    },
    {
	"selftest 2 external -external-> external, no extension, quoted",
	"inline:{ {\"a a\"@example.com=\"b b\"@example.com} }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_EXTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
	"\"aXa\"@example.com",
	{"\"b b\"@example.com"}, 1,
    },
    {
	"selftest 3 external -external-> external, no extension, quoted",
	"inline:{ {\"a a\"@example.com=\"b b\"@example.com} }",
	DO_PROPAGATE_UNMATCHED_EXTENSION, PLUS_RECIPIENT_DELIMITER,
	MA_FORM_EXTERNAL, MA_FORM_EXTERNAL, MA_FORM_EXTERNAL,
	"\"a a\"@example.com",
	{0}, 0,
    },
    0,
};

/* canon_addr_external - surrogate to avoid trivial-rewrite dependency */

VSTRING *canon_addr_external(VSTRING *result, const char *addr)
{
    return (vstring_strcpy(result, addr));
}

static int compare(const char *testname,
		           const char **expect_argv, int expect_argc,
		           char **result_argv, int result_argc)
{
    int     n;
    int     err = 0;

    if (expect_argc != 0 && result_argc != 0) {
	for (n = 0; n < expect_argc && n < result_argc; n++) {
	    if (strcmp(expect_argv[n], result_argv[n]) != 0) {
		msg_warn("fail test %s: expect[%d]='%s', result[%d]='%s'",
			 testname, n, expect_argv[n], n, result_argv[n]);
		err = 1;
	    }
	}
    }
    if (expect_argc != result_argc) {
	msg_warn("fail test %s: expects %d results but there were %d",
		 testname, expect_argc, result_argc);
	for (n = expect_argc; n < result_argc; n++)
	    msg_info("no expect to match result[%d]='%s'", n, result_argv[n]);
	for (n = result_argc; n < expect_argc; n++)
	    msg_info("no result to match expect[%d]='%s'", n, expect_argv[n]);
	err = 1;
    }
    return (err);
}

static char *progname;

static NORETURN usage(void)
{
    msg_fatal("usage: %s pass_test | fail_test", progname);
}

int     main(int argc, char **argv)
{
    MAIL_ADDR_MAP_TEST *test;
    MAIL_ADDR_MAP_TEST *tests;
    int     errs = 0;

#define UPDATE(dst, src) { if (dst) myfree(dst); dst = mystrdup(src); }

    /*
     * Parse JCL.
     */
    progname = argv[0];
    if (argc != 2) {
	usage();
    } else if (strcmp(argv[1], "pass_tests") == 0) {
	tests = pass_tests;
    } else if (strcmp(argv[1], "fail_tests") == 0) {
	tests = fail_tests;
    } else {
	usage();
    }

    /*
     * Initialize.
     */
    mail_params_init();

    /*
     * A read-eval-print loop, because specifying C strings with quotes and
     * backslashes is painful.
     */
    for (test = tests; test->testname; test++) {
	ARGV   *result;
	int     fail = 0;

	if (mail_addr_form_to_string(test->in_form) == 0) {
	    msg_warn("test %s: bad in_form field: %d",
		     test->testname, test->in_form);
	    fail = 1;
	    continue;
	}
	if (mail_addr_form_to_string(test->query_form) == 0) {
	    msg_warn("test %s: bad query_form field: %d",
		     test->testname, test->query_form);
	    fail = 1;
	    continue;
	}
	if (mail_addr_form_to_string(test->out_form) == 0) {
	    msg_warn("test %s: bad out_form field: %d",
		     test->testname, test->out_form);
	    fail = 1;
	    continue;
	}
	MAPS   *maps = maps_create("test", test->database, DICT_FLAG_LOCK
			     | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);

	UPDATE(var_rcpt_delim, test->delimiter);
	result = mail_addr_map_opt(maps, test->address, test->propagate,
			   test->in_form, test->query_form, test->out_form);
	if (compare(test->testname, test->expect_argv, test->expect_argc,
	       result ? result->argv : 0, result ? result->argc : 0) != 0) {
	    msg_info("database = %s", test->database);
	    msg_info("propagate = %d", test->propagate);
	    msg_info("delimiter = '%s'", var_rcpt_delim);
	    msg_info("in_form = %s", mail_addr_form_to_string(test->in_form));
	    msg_info("query_form = %s", mail_addr_form_to_string(test->query_form));
	    msg_info("out_form = %s", mail_addr_form_to_string(test->out_form));
	    msg_info("address = %s", test->address);
	    fail = 1;
	}
	maps_free(maps);
	if (result)
	    argv_free(result);

	/*
	 * It is an error if a test does not pass or fail as intended.
	 */
	errs += (tests == pass_tests ? fail : !fail);
    }
    return (errs != 0);
}

#endif
