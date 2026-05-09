/*	$NetBSD: dict_union_test.c,v 1.1.1.1 2026/05/09 18:39:23 christos Exp $	*/

/*++
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
#include <argv.h>
#include <dict_union.h>
#include <msg.h>
#include <msg_vstream.h>
#include <stringops.h>
#include <vstring.h>

 /*
  * Testing library.
  */
#include <dict_test_helper.h>

#define LEN(x)	VSTRING_LEN(x)
#define STR(x)	vstring_str(x)

 /*
  * TODO(wietse) move these to common testing header file.
  */
#define PASS	1
#define FAIL	0

static VSTRING *msg_buf;

static int valid_refcounts_for_good_composite_syntax(void)
{
    DICT   *dict;
    int     open_flags = O_RDONLY;
    int     dict_flags = DICT_FLAG_LOCK;
    VSTRING *composite_spec = vstring_alloc(100);
    ARGV   *reg_component_specs = argv_alloc(3);
    const char *component_specs[] = {
	"static:one",
	"static:two",
	"inline:{foo=three}",
	0,
    };
    char  **cpp;

    dict_compose_spec(DICT_TYPE_UNION, component_specs, open_flags, dict_flags,
		      composite_spec, reg_component_specs);
    dict = dict_open_and_capture_msg(STR(composite_spec), open_flags, dict_flags,
				     msg_buf);

#undef RETURN
#define RETURN(x) do { \
	if (dict) dict_close(dict); \
	vstring_free(composite_spec); \
	argv_free(reg_component_specs); \
	return (x); \
    } while (0);

    if (LEN(msg_buf) > 0) {
	msg_warn("unexpected dict_open() warning: got '%s'", STR(msg_buf));
	RETURN(FAIL);
    }
    for (cpp = reg_component_specs->argv; *cpp; cpp++) {
	if (dict_handle(*cpp) == 0) {
	    msg_warn("table '%s' is not registered after dict_open()", *cpp);
	    RETURN(FAIL);
	}
    }
    dict_close(dict);
    dict = 0;
    for (cpp = reg_component_specs->argv; *cpp; cpp++) {
	if (dict_handle(*cpp) != 0) {
	    msg_warn("table '%s' is still registered after dict_close()", *cpp);
	    RETURN(FAIL);
	}
    }
    RETURN(PASS);
}

static int valid_refcounts_for_bad_composite_syntax(void)
{
    DICT   *dict;
    int     open_flags = O_RDONLY;
    int     dict_flags = DICT_FLAG_LOCK;
    VSTRING *composite_spec = vstring_alloc(100);
    ARGV   *reg_component_specs = argv_alloc(3);
    const char *component_specs[] = {
	"static:one",
	"static:two",
	"inline{foo=three}",
	0,
    };
    char  **cpp;
    const char *want_msg = "bad syntax:";

    dict_compose_spec(DICT_TYPE_UNION, component_specs, open_flags, dict_flags,
		      composite_spec, reg_component_specs);
    dict = dict_open_and_capture_msg(STR(composite_spec), open_flags,
				     dict_flags, msg_buf);

#undef RETURN
#define RETURN(x) do { \
	dict_close(dict); \
	vstring_free(composite_spec); \
	argv_free(reg_component_specs); \
	return (x); \
    } while (0);

    if (LEN(msg_buf) == 0) {
	msg_warn("missing dict_open() warning: want '%s'", want_msg);
	RETURN(FAIL);
    }
    if (strstr(STR(msg_buf), want_msg) == 0) {
	msg_warn("unexpected warning message: got '%s', want '%s'",
		 STR(msg_buf), want_msg);
	RETURN(FAIL);
    }
    for (cpp = reg_component_specs->argv; *cpp; cpp++) {
	if (dict_handle(*cpp) != 0) {
	    msg_warn("table '%s' is registered after failed dict_open()",
		     *cpp);
	    RETURN(FAIL);
	}
    }
    RETURN(PASS);
}

static int propagates_notfound_and_found(void)
{
    DICT   *dict;
    int     open_flags = O_RDONLY;
    int     dict_flags = DICT_FLAG_LOCK;
    const char *dict_spec = ("unionmap:{static:one,static:two,"
			     "inline:{foo=three}}");
    static struct dict_get_verify_data expectations[] = {
	{.key = "foo",.want_value = "one,two,three"},
	{.key = "bar",.want_value = "one,two"},
	{0},
    };
    int     ret;

    dict = dict_open_and_capture_msg(dict_spec, open_flags, dict_flags,
				     msg_buf);
    if (LEN(msg_buf) > 0) {
	msg_warn("unexpected dict_open() warning: got '%s'", STR(msg_buf));
	ret = FAIL;
    } else {
	ret = dict_get_and_verify_bulk(dict, expectations);
    }
    dict_close(dict);
    return (ret);
}

static int propagates_error(void)
{
    DICT   *dict;
    int     open_flags = O_RDONLY;
    int     dict_flags = DICT_FLAG_LOCK;
    const char *dict_spec = "unionmap:{static:one,fail:fail}";
    static struct dict_get_verify_data expectations[] = {
	{.key = "foo",.want_value = 0,.want_error = DICT_ERR_RETRY},
	{0},
    };
    int     ret;

    dict = dict_open_and_capture_msg(dict_spec, open_flags, dict_flags,
				     msg_buf);
    if (LEN(msg_buf) > 0) {
	msg_warn("unexpected dict_open() warning: got '%s'", STR(msg_buf));
	ret = FAIL;
    } else {
	ret = dict_get_and_verify_bulk(dict, expectations);
    }
    dict_close(dict);
    return (ret);
}

static int no_comma_for_not_found(void)
{
    DICT   *dict;
    int     open_flags = O_RDONLY;
    int     dict_flags = DICT_FLAG_LOCK;
    const char *dict_spec = "unionmap:{regexp:{{/a|c/ 1}},regexp:{{/b|c/ 2}}}";
    static struct dict_get_verify_data expectations[] = {
	{.key = "x",.want_value = 0},
	{.key = "a",.want_value = "1"},
	{.key = "b",.want_value = "2"},
	{.key = "c",.want_value = "1,2"},
	{0},
    };
    int     ret;

    dict = dict_open_and_capture_msg(dict_spec, open_flags, dict_flags,
				     msg_buf);
    if (LEN(msg_buf) > 0) {
	msg_warn("unexpected dict_open() warning: got '%s'", STR(msg_buf));
	ret = FAIL;
    } else {
	ret = dict_get_and_verify_bulk(dict, expectations);
    }
    dict_close(dict);
    return (ret);
}

struct TEST_CASE {
    const char *label;
    int     (*action) (void);
};

static const struct TEST_CASE test_cases[] = {
    {"valid_refcounts_for_good_composite_syntax", valid_refcounts_for_good_composite_syntax,},
    {"valid_refcounts_for_bad_composite_syntax", valid_refcounts_for_bad_composite_syntax,},
    {"propagates_notfound_and_found", propagates_notfound_and_found,},
    {"propagates_error", propagates_error,},
    {"no_comma_for_not_found", no_comma_for_not_found,},
    {0},
};

int     main(int argc, char **argv)
{
    static int tests_passed = 0;
    static int tests_failed = 0;
    const struct TEST_CASE *tp;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    msg_buf = vstring_alloc(100);
    dict_allow_surrogate = 1;

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
    vstring_free(msg_buf);

    msg_info("PASS=%d FAIL=%d", tests_passed, tests_failed);
    exit(tests_failed != 0);
}
