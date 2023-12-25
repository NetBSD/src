/*	$NetBSD: map_search.c,v 1.3.2.1 2023/12/25 12:43:32 martin Exp $	*/

/*++
/* NAME
/*	map_search_search 3
/* SUMMARY
/*	lookup table search list support
/* SYNOPSIS
/*	#include <map_search_search.h>
/*
/*	typedef struct {
/* .in +4
/*	    char   *map_type_name;	/* type:name, owned */
/*	    char   *search_order;	/* null or owned */
/* .in -4
/*	} MAP_SEARCH;
/*
/*	void	map_search_init(
/*	const NAME_CODE *search_actions)
/*
/*	const MAP_SEARCH *map_search_create(
/*	const char *map_spec)
/*
/*	const MAP_SEARCH *map_search_lookup(
/*	const char *map_spec);
/* DESCRIPTION
/*	This module implements configurable search order support
/*	for Postfix lookup tables.
/*
/*	map_search_init() must be called once, before other functions
/*	in this module.
/*
/*	map_search_create() creates a MAP_SEARCH instance for
/*	map_spec, ignoring duplicate requests.
/*
/*	map_search_lookup() looks up the MAP_SEARCH instance that
/*	was created by map_search_create().
/*
/*	Arguments:
/* .IP search_actions
/*	The mapping from search action string form to numeric form.
/*	The numbers must be in the range [1..126] (inclusive). The
/*	value 0 is reserved for the MAP_SEARCH.search_order terminator,
/*	and the value MAP_SEARCH_CODE_UNKNOWN is reserved for the
/*	'not found' result. The argument is copied (the pointer
/*	value, not the table).
/* .IP map_spec
/*	lookup table and optional search order: either maptype:mapname,
/*	or { maptype:mapname, { search = name, name }}. The search
/*	attribute is optional. The comma is equivalent to whitespace.
/* DIAGNOSTICS
/*	map_search_create() returns a null pointer when a map_spec
/*	is a) malformed, b) specifies an unexpected attribute name,
/*	c) the search attribute contains an unknown name. Thus,
/*	map_search_create() will never return a search_order that
/*	contains the value MAP_SEARCH_CODE_UNKNOWN.
/*
/*	Panic: interface violations. Fatal errors: out of memory.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

 /*
  * Utility library.
  */
#include <htable.h>
#include <msg.h>
#include <mymalloc.h>
#include <name_code.h>
#include <stringops.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <map_search.h>

 /*
  * Application-specific.
  */
static HTABLE *map_search_table;
static const NAME_CODE *map_search_actions;

#define STR(x) vstring_str(x)

/* map_search_init - one-time initialization */

void    map_search_init(const NAME_CODE *search_actions)
{
    if (map_search_table != 0 || map_search_actions != 0)
	msg_panic("map_search_init: multiple calls");
    map_search_table = htable_create(100);
    map_search_actions = search_actions;
}

/* map_search_create - store MAP_SEARCH instance */

const MAP_SEARCH *map_search_create(const char *map_spec)
{
    char   *copy_of_map_spec = 0;
    char   *bp = 0;
    const char *const_err;
    char   *heap_err = 0;
    VSTRING *search_order = 0;
    const char *map_type_name;
    char   *attr_name_val = 0;
    char   *attr_name = 0;
    char   *attr_value = 0;
    MAP_SEARCH *map_search;
    char   *atom;
    int     code;

    /*
     * Sanity check.
     */
    if (map_search_table == 0 || map_search_actions == 0)
	msg_panic("map_search_create: missing initialization");

    /*
     * Allow exact duplicates. This won't catch duplicates that differ only
     * in their use of whitespace or comma.
     */
    if ((map_search =
	 (MAP_SEARCH *) htable_find(map_search_table, map_spec)) != 0)
	return (map_search);

    /*
     * Macro for readability and safety. Let the compiler worry about code
     * duplication and redundant conditions.
     */
#define MAP_SEARCH_CREATE_RETURN(x) do { \
	if (copy_of_map_spec) myfree(copy_of_map_spec); \
	if (heap_err) myfree(heap_err); \
	if (search_order) vstring_free(search_order); \
	return (x); \
    } while (0)

    /*
     * Long form specifies maptype_mapname and optional search attribute.
     */
    if (*map_spec == CHARS_BRACE[0]) {
	bp = copy_of_map_spec = mystrdup(map_spec);
	if ((heap_err = extpar(&bp, CHARS_BRACE, EXTPAR_FLAG_STRIP)) != 0) {
	    msg_warn("malformed map specification: '%s'", heap_err);
	    MAP_SEARCH_CREATE_RETURN(0);
	} else if ((map_type_name = mystrtokq(&bp, CHARS_COMMA_SP,
					      CHARS_BRACE)) == 0) {
	    msg_warn("empty map specification: '%s'", map_spec);
	    MAP_SEARCH_CREATE_RETURN(0);
	}
    } else {
	map_type_name = map_spec;
    }

    /*
     * Sanity check the map spec before parsing attributes.
     */
    if (strchr(map_type_name, ':') == 0) {
	msg_warn("malformed map specification: '%s'", map_spec);
	msg_warn("expected maptype:mapname instead of '%s'", map_type_name);
	MAP_SEARCH_CREATE_RETURN(0);
    }

    /*
     * Parse the attribute list. XXX This does not detect multiple attributes
     * with the same attribute name.
     */
    if (bp != 0) {
	while ((attr_name_val = mystrtokq(&bp, CHARS_COMMA_SP, CHARS_BRACE)) != 0) {
	    if (*attr_name_val == CHARS_BRACE[0]) {
		if ((heap_err = extpar(&attr_name_val, CHARS_BRACE,
				       EXTPAR_FLAG_STRIP)) != 0) {
		    msg_warn("malformed map attribute: %s", heap_err);
		    MAP_SEARCH_CREATE_RETURN(0);
		}
	    }
	    if ((const_err = split_nameval(attr_name_val, &attr_name,
					   &attr_value)) != 0) {
		msg_warn("malformed map attribute in '%s': '%s'",
			 map_spec, const_err);
		MAP_SEARCH_CREATE_RETURN(0);
	    }
	    if (strcasecmp(attr_name, MAP_SEARCH_ATTR_NAME_SEARCH) != 0) {
		msg_warn("unknown map attribute in '%s': '%s'",
			 map_spec, attr_name);
		MAP_SEARCH_CREATE_RETURN(0);
	    }
	}
    }

    /*
     * Parse the search list if any.
     */
    if (attr_name != 0) {
	search_order = vstring_alloc(10);
	while ((atom = mystrtok(&attr_value, CHARS_COMMA_SP)) != 0) {
	    if ((code = name_code(map_search_actions, NAME_CODE_FLAG_NONE,
				  atom)) == MAP_SEARCH_CODE_UNKNOWN) {
		msg_warn("unknown search type '%s' in '%s'", atom, map_spec);
		MAP_SEARCH_CREATE_RETURN(0);
	    }
	    VSTRING_ADDCH(search_order, code);
	}
	VSTRING_TERMINATE(search_order);
    }

    /*
     * Bundle up the result.
     */
    map_search = (MAP_SEARCH *) mymalloc(sizeof(*map_search));
    map_search->map_type_name = mystrdup(map_type_name);
    if (search_order) {
	map_search->search_order = vstring_export(search_order);
	search_order = 0;
    } else {
	map_search->search_order = 0;
    }

    /*
     * Save the ACL to cache.
     */
    (void) htable_enter(map_search_table, map_spec, map_search);

    MAP_SEARCH_CREATE_RETURN(map_search);
}

/* map_search_lookup - lookup MAP_SEARCH instance */

const MAP_SEARCH *map_search_lookup(const char *map_spec)
{

    /*
     * Sanity check.
     */
    if (map_search_table == 0 || map_search_actions == 0)
	msg_panic("map_search_lookup: missing initialization");

    return ((MAP_SEARCH *) htable_find(map_search_table, map_spec));
}

 /*
  * Test driver.
  */
#ifdef TEST
#include <stdlib.h>

 /*
  * Test search actions.
  */
#define TEST_NAME_1	"one"
#define TEST_NAME_2	"two"
#define TEST_CODE_1	1
#define TEST_CODE_2	2

#define BAD_NAME	"bad"

static const NAME_CODE search_actions[] = {
    TEST_NAME_1, TEST_CODE_1,
    TEST_NAME_2, TEST_CODE_2,
    0, MAP_SEARCH_CODE_UNKNOWN,
};

/* Helpers to simplify tests. */

static const char *string_or_null(const char *s)
{
    return (s ? s : "(null)");
}

static char *escape_order(VSTRING *buf, const char *search_order)
{
    return (STR(escape(buf, search_order, strlen(search_order))));
}

int     main(int argc, char **argv)
{
    /* Test cases with inputs and expected outputs. */
    typedef struct TEST_CASE {
	const char *map_spec;
	int     exp_return;		/* 0=fail, 1=success */
	const char *exp_map_type_name;	/* 0 or match */
	const char *exp_search_order;	/* 0 or match */
    } TEST_CASE;
    static TEST_CASE test_cases[] = {
	{"type", 0, 0, 0},
	{"type:name", 1, "type:name", 0},
	{"{type:name}", 1, "type:name", 0},
	{"{type:name", 0, 0, 0},	/* } */
	{"{type}", 0, 0, 0},
	{"{type:name foo}", 0, 0, 0},
	{"{type:name foo=bar}", 0, 0, 0},
	{"{type:name search_order=}", 1, "type:name", ""},
	{"{type:name search_order=one, two}", 0, 0, 0},
	{"{type:name {search_order=one, two}}", 1, "type:name", "\01\02"},
	{"{type:name {search_order=one, two, bad}}", 0, 0, 0},
	{"{inline:{a=b} {search_order=one, two}}", 1, "inline:{a=b}", "\01\02"},
	{"{inline:{a=b, c=d} {search_order=one, two}}", 1, "inline:{a=b, c=d}", "\01\02"},
	{0},
    };
    TEST_CASE *test_case;

    /* Actual results. */
    const MAP_SEARCH *map_search_from_create;
    const MAP_SEARCH *map_search_from_create_2nd;
    const MAP_SEARCH *map_search_from_lookup;

    /* Findings. */
    int     tests_failed = 0;
    int     test_failed;

    /* Scratch */
    VSTRING *expect_escaped = vstring_alloc(100);
    VSTRING *actual_escaped = vstring_alloc(100);

    map_search_init(search_actions);

    for (tests_failed = 0, test_case = test_cases; test_case->map_spec;
	 tests_failed += test_failed, test_case++) {
	test_failed = 0;
	msg_info("test case %d: '%s'",
		 (int) (test_case - test_cases), test_case->map_spec);
	map_search_from_create = map_search_create(test_case->map_spec);
	if (!test_case->exp_return != !map_search_from_create) {
	    if (map_search_from_create)
		msg_warn("test case %d return expected %s actual {%s, %s}",
			 (int) (test_case - test_cases),
			 test_case->exp_return ? "success" : "fail",
			 map_search_from_create->map_type_name,
			 escape_order(actual_escaped,
				      map_search_from_create->search_order));
	    else
		msg_warn("test case %d return expected %s actual %s",
			 (int) (test_case - test_cases), "success",
			 map_search_from_create ? "success" : "fail");
	    test_failed = 1;
	    continue;
	}
	if (test_case->exp_return == 0)
	    continue;
	map_search_from_lookup = map_search_lookup(test_case->map_spec);
	if (map_search_from_create != map_search_from_lookup) {
	    msg_warn("test case %d map_search_lookup expected=%p actual=%p",
		     (int) (test_case - test_cases),
		     map_search_from_create, map_search_from_lookup);
	    test_failed = 1;
	}
	map_search_from_create_2nd = map_search_create(test_case->map_spec);
	if (map_search_from_create != map_search_from_create_2nd) {
	    msg_warn("test case %d repeated map_search_create "
		     "expected=%p actual=%p",
		     (int) (test_case - test_cases),
		     map_search_from_create, map_search_from_create_2nd);
	    test_failed = 1;
	}
	if (strcmp(string_or_null(test_case->exp_map_type_name),
		   string_or_null(map_search_from_create->map_type_name))) {
	    msg_warn("test case %d map_type_name expected=%s actual=%s",
		     (int) (test_case - test_cases),
		     string_or_null(test_case->exp_map_type_name),
		     string_or_null(map_search_from_create->map_type_name));
	    test_failed = 1;
	}
	if (strcmp(string_or_null(test_case->exp_search_order),
		   string_or_null(map_search_from_create->search_order))) {
	    msg_warn("test case %d search_order expected=%s actual=%s",
		     (int) (test_case - test_cases),
		     escape_order(expect_escaped,
			       string_or_null(test_case->exp_search_order)),
		     escape_order(actual_escaped,
		     string_or_null(map_search_from_create->search_order)));
	    test_failed = 1;
	}
    }
    vstring_free(expect_escaped);
    vstring_free(actual_escaped);

    if (tests_failed)
	msg_info("tests failed: %d", tests_failed);
    exit(tests_failed != 0);
}

#endif
