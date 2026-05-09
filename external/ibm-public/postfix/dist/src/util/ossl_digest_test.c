/*	$NetBSD: ossl_digest_test.c,v 1.2 2026/05/09 18:49:23 christos Exp $	*/

/*++
/* NAME
/*	ossl_digest_test 1t
/* SUMMARY
/*	ossl_digest unit tests
/* SYNOPSIS
/*	./ossl_digest_test
/* DESCRIPTION
/*	ossl_digest_test runs and logs each configured test, reports if
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
#include <stdio.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <argv.h>
#include <hex_code.h>
#include <msg.h>
#include <mymalloc.h>
#include <ossl_digest.h>
#include <stringops.h>
#include <vstring.h>

#ifdef USE_TLS

#define LEN(x)	VSTRING_LEN(x)
#define STR(x)	vstring_str(x)

#define PASS	1
#define FAIL	0

/* get_error_string - return error info as one string */

static char *get_error_string(void)
{
    VSTRING *err_string;
    ARGV   *err_list;

    err_list = ossl_digest_get_errors();
    err_string = vstring_alloc(100);
    argv_join(err_string, err_list, '\n');
    argv_free(err_list);
    return (vstring_export(err_string));
}

static int reports_bad_digest_name(void)
{
    const char *bad_digest_name = "doesnotexist";
    OSSL_DGST *dgst;
    int     status = PASS;

    if ((dgst = ossl_digest_new(bad_digest_name)) != 0) {
	msg_warn("want: NULL, got: %p", (void *) dgst);
	ossl_digest_free(dgst);
	status = FAIL;
    } else {
	char   *err_string;

	err_string = get_error_string();
	if (*err_string && strstr(err_string, bad_digest_name) == 0) {
	    status = FAIL;
	    msg_warn("want: '%s', got: '%s'", bad_digest_name, err_string);
	}
	myfree(err_string);
    }
    return (status);
}

static int computes_sha256_digests(void)
{
    OSSL_DGST *dgst;
    struct DGST_TEST {
	const char *in;
	const char *want_hex;
    };
    static const struct DGST_TEST sha256_tests[] = {
	{"",
	    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
	},
	{"one",
	    "7692c3ad3540bb803c020b3aee66cd8887123234ea0c6e7143c0add73ff431ed",
	},
	{"two",
	    "3fc4ccfe745870e2c0d99f71f30ff0656c8dedd41cc1d7d3d376b0dbe685e2f3",
	},
	{0},
    };
    const struct DGST_TEST *dp;
    int     status = PASS;

    if ((dgst = ossl_digest_new("sha256")) == 0) {
	char   *err_string = get_error_string();

	msg_warn("want: noerror, got: '%s'", err_string);
	myfree(err_string);
	status = FAIL;
    } else {
	VSTRING *out = vstring_alloc(10);
	VSTRING *got_hex = vstring_alloc(10);

	for (dp = sha256_tests; dp->in != 0; dp++) {
	    if (ossl_digest_data(dgst, dp->in, strlen(dp->in), out) < 0) {
		char   *err_string = get_error_string();

		msg_warn("want: noerror, got: '%s'", err_string);
		myfree(err_string);
		status = FAIL;
		continue;
	    }
	    hex_encode(got_hex, STR(out), LEN(out));
	    lowercase(STR(got_hex));
	    if (strcmp(STR(got_hex), dp->want_hex) != 0) {
		msg_warn("got: '%s', want: '%s'", STR(got_hex), dp->want_hex);
		status = FAIL;
		continue;
	    }
	}
	ossl_digest_free(dgst);
	vstring_free(got_hex);
	vstring_free(out);
    }
    return (status);
}

static int returns_sha256_output_size(void)
{
    OSSL_DGST *dgst;
    ssize_t got_size, want_size = 256 / 8;
    int     status = PASS;

    if ((dgst = ossl_digest_new("sha256")) == 0) {
	char   *err_string;

	err_string = get_error_string();
	msg_warn("want: noerror, got: '%s'", err_string);
	myfree(err_string);
	status = FAIL;
    } else {
	if ((got_size = ossl_digest_get_size(dgst)) != want_size) {
	    msg_warn("want: %ld, got: %ld", (long) got_size, (long) want_size);
	    status = FAIL;
	}
	ossl_digest_free(dgst);
    }
    return (status);
}

struct TEST_CASE {
    const char *label;
    int     (*action) (void);
};

static const struct TEST_CASE test_cases[] = {
    {"reports_bad_digest_name", reports_bad_digest_name,},
    /* TODO(wietse) test ossl_digest_log_errors() */
    {"computes_sha256_digests", computes_sha256_digests,},
    {"returns_sha256_output_size", returns_sha256_output_size,},
    {0},
};

int     main(int argc, char **argv)
{
    static int tests_passed = 0;
    static int tests_failed = 0;
    const struct TEST_CASE *tp;

    for (tp = test_cases; tp->label; tp++) {
	msg_info("RUN  %s", tp->label);
	if (tp->action() == PASS) {
	    msg_info("PASS %s", tp->label);
	    tests_passed += 1;
	} else {
	    msg_info("FAIL %s", tp->label);
	    tests_failed += 1;
	}
    }
    msg_info("PASS=%d FAIL=%d", tests_passed, tests_failed);
    exit(tests_failed != 0);
}

#else

int     main(int argc, char **argv)
{
    msg_fatal("this program requires `#define USE_TLS'");
}

#endif
