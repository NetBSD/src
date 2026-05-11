/*	$NetBSD: dict_sqlite_test.c,v 1.2.2.2 2026/05/11 17:13:47 martin Exp $	*/

/*++
/* NAME
/*	dict_sqlite_test 1t
/* SUMMARY
/*	dict_sqlite unit test
/* SYNOPSIS
/*	./dict_sqlite_test
/* DESCRIPTION
/*	dict_sqlite_test runs and logs each configured test, reports if
/*	a test is a PASS or FAIL, and returns an exit status of zero if
/*	all tests are a PASS.
/*
/*	Each test creates a temporary test database and a corresponding
/*	Postfix sqlite client configuration file, both having unique
/*	names. Otherwise, each test is hermetic.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema porcupine.org
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

 /*
  * Global library.
  */
#include <dict_sqlite.h>

 /*
  * TODO(wietse) make this a proper VSTREAM interface or test helper API.
  */

/* vstream_swap - capture output for testing */

static void vstream_swap(VSTREAM *one, VSTREAM *two)
{
    VSTREAM save;

    save = *one;
    *one = *two;
    *two = save;
}

 /*
  * Override the printable.c module because it may break some tests.
  * 
  * TODO(wietse) move this to a fake_printable.c module that can override all
  * printable.c global symbols.
  */
int     util_utf8_enable;

char   *printable_except(char *string, int replacement, const char *except)
{
    return (string);
}

 /*
  * Scaffolding for dict_sqlite(3) tests.
  */

/* create_and_populate_db - create an empty database and optionally populate */

static void create_and_populate_db(char *dbpath, const char *commands)
{
    int     fd;

    /*
     * Create an empty database file with a unique name. Assume that an
     * adversary cannot rename or remove the file.
     */
    if ((fd = mkstemp(dbpath)) < 0)
	msg_fatal("mkstemp(\"%s\"): %m", dbpath);
    if (close(fd) < 0)
	msg_fatal("close %s: %m", dbpath);

    /*
     * TODO(wietse) Open the database file, prepare and execute commands to
     * populate the database, and close the database.
     */
    if (commands) {
	msg_fatal("commands are not yet supported");
    }
}

/* create_and_populate_cf - create sqlite_table(5) configuration file */

static void create_and_populate_cf(char *cfpath, const char *dbpath,
				           const char *cftext)
{
    int     fd;
    VSTREAM *fp;

    /*
     * Create an empty sqlite_table(5) configuration file with a unique name.
     * Assume that an adversary cannot rename or remove the file.
     */
    if ((fd = mkstemp(cfpath)) < 0)
	msg_fatal("mkstemp(\"%s\"): %m", cfpath);
    if ((fp = vstream_fdopen(fd, O_WRONLY)) == 0)
	msg_fatal("vstream_fdopen: %m");
    (void) vstream_fprintf(fp, "%s\ndbpath = %s\n", cftext, dbpath);
    if (vstream_fclose(fp) != 0)
	msg_fatal("vstream_fdclose: %m");
}

 /*
  * Test structure. Some tests may come their own.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
    const char *commands;		/* commands or null */
    const char *settings;		/* sqlite_table(5) */
    const char *exp_warning;		/* substring match or null */
} TEST_CASE;

#define PASS    (0)
#define FAIL    (1)

#define PATH_TEMPLATE	"/tmp/test-XXXXXXX"

/* test_flag_non_recommended_query - flag non-recommended query payloads */

static int test_flag_non_recommended_query(const TEST_CASE *tp)
{
    static VSTRING *msg_buf;
    VSTREAM *memory_stream;
    const char template[] = PATH_TEMPLATE;
    char    dbpath[sizeof(template)];
    char    cfpath[sizeof(template)];
    DICT   *dict;

    if (msg_buf == 0)
	msg_buf = vstring_alloc(100);

    /* Prepare scaffolding database and configuration files. */
    memcpy(dbpath, template, sizeof(dbpath));
    create_and_populate_db(dbpath, tp->commands);
    memcpy(cfpath, template, sizeof(cfpath));
    create_and_populate_cf(cfpath, dbpath, tp->settings);

    /* Run the test with custom STDERR stream. */
    VSTRING_RESET(msg_buf);
    VSTRING_TERMINATE(msg_buf);
    if ((memory_stream = vstream_memopen(msg_buf, O_WRONLY)) == 0)
	msg_fatal("open memory stream: %m");
    vstream_swap(VSTREAM_ERR, memory_stream);
    if ((dict = dict_sqlite_open(cfpath, O_RDONLY, DICT_FLAG_UTF8_REQUEST)) != 0)
	dict_close(dict);
    vstream_swap(memory_stream, VSTREAM_ERR);
    if (vstream_fclose(memory_stream))
	msg_fatal("close memory stream: %m");

    /* Cleanup scaffolding database and configuration files. */
    if (unlink(dbpath) < 0)
	msg_fatal("unlink %s: %m", dbpath);
    if (unlink(cfpath) < 0)
	msg_fatal("unlink %s: %m", cfpath);

    /* Verify the results. */
    if (tp->exp_warning == 0 && VSTRING_LEN(msg_buf) > 0) {
	msg_warn("got warning ``%s'', want ``null''", vstring_str(msg_buf));
	return (FAIL);
    }
    if (tp->exp_warning != 0
	&& strstr(vstring_str(msg_buf), tp->exp_warning) == 0) {
	msg_warn("got warning ``%s'', want ``%s''",
		 vstring_str(msg_buf), tp->exp_warning);
	return (FAIL);
    }
    return (PASS);
}

 /*
  * The list of test cases.
  */
static const TEST_CASE test_cases[] = {

    /*
     * Tests to flag non-recommended query forms. These create an empty test
     * database, and open it with the dict_sqlite client without querying it.
     */
    {.label = "no_dynamic_payload",
	.action = test_flag_non_recommended_query,
	.settings = "query = select a from b where c = 5",
    },
    {.label = "dynamic_payload_inside_recommended_quotes",
	.action = test_flag_non_recommended_query,
	.settings = "query = select a from b where c = 'xx%syy'",
    },
    {.label = "dynamic_payload_without_quotes",
	.action = test_flag_non_recommended_query,
	.settings = "query = select s from b where c = xx%syy",
	.exp_warning = "contains >%s< without the recommended '' quotes",
    },
    {.label = "payload_inside_double_quotes",
	.action = test_flag_non_recommended_query,
	.settings = "query = select s from b where c = \"xx%syy\"",
	.exp_warning = "contains >%s< without the recommended '' quotes",
    },

    /*
     * TODO: Tests that actually populate a test database, and that query it
     * with the dict_sqlite client.
     */
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
