/*	$NetBSD: dict_test_helper.c,v 1.2 2026/05/09 18:49:21 christos Exp $	*/

/*++
/* NAME
/*	dict_test_helper 3
/* SUMMARY
/*	dictionary test helpers
/* SYNOPSIS
/*	#include <dict_test_helper.h>
/*
/*	DICT	*dict_open_and_capture_msg(
/*	const char *type_name,
/*	int	open_flags,
/*	int	dict_flags,
/*	VSTRING	*out_msg_buf)
/*
/*	char	*dict_compose_spec(
/*	const char *dict_type,
/*	const char **component_specs,
/*	int	open_flags,
/*	int	dict_flags,
/*	VSTRING	*out_composite_spec,
/*	ARGV	*out_reg_component_specs)
/*
/*	chat char *dict_get_and_capture_msg(
/*	DICT	*dict,
/*	const char *key,
/*	VSTRING	*out_msg_buf)
/*
/*	struct dict_get_verify_data {
/* .in +4
/*	const char *key;
/*	const char *want_value;
/*	int     want_error;
/*	const char *want_msg;
/* .in -4
/*	}
/*
/*	int dict_get_and_verify(
/*	DICT	*dict,
/*	const char *key,
/*	const char *want_value,
/*	int	want_error,
/*	const char *want_msg)
/*
/*	int	dict_get_and_verify_bulk(
/*	DICT	*dict,
/*	const struct dict_get_verify_data *data)
/* DESCRIPTION
/*	This module contains common code for dictionary tests.
/*
/*	All functions that capture msg(3) output clear the output
/*	VSTRING buffer first.
/*
/*	dict_open_and_capture_msg() invokes dict_open() while
/*	capturing msg(3) output to a VSTRING buffer.
/*
/*	dict_compose_spec() constructs a composite dictionary spec with
/*	the form "\fIdict_type:{component_specs[0],...}\fR". It records in
/*	out_reg_component_specs the names under which the component_specs
/*	dictionaries will be registered with dict_register(), with each
/*	name having the form "\fItype:name(open_flags,dict_flags). The
/*	result value is the out_composite_spec string value.
/*
/*	dict_get_and_capture_msg() invokes dict_get() while capturing
/*	msg(3) output to a VSTRING buffer.
/*
/*	dict_get_and_verify() invokes dict_get() and verifies that
/*	expectations are met. The want_value argument requires an exact
/*	match; specify null if the expected lookup result is "not found".
/*	The want_error argument requires an exact match; specify zero
/*	(DICT_ERR_NONE) or one of the other expected DICT_ERR_*
/*	values. The want_msg argument requires a substring match;
/*	specify null if no msg(3) output is expected. The result value
/*	is PASS or FAIL.
/*
/*	dict_get_and_verify_bulk() provides a convenient interface for
/*	multiple lookup tests.
/* LICENSE
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
#include <argv.h>
#include <dict.h>
#include <dict_test_helper.h>
#include <msg.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring.h>

#define LEN(x)	VSTRING_LEN(x)
#define STR(x)	vstring_str(x)

 /*
  * TODO(wietse) factor this out to common testing header file.
  */
#define PASS	1
#define FAIL	0

 /*
  * TODO(wietse) make this a proper VSTREAM interface or test helper API.
  */

/* vstream_swap - capture msg(3) output for testing */

static void vstream_swap(VSTREAM *one, VSTREAM *two)
{
    VSTREAM save;

    save = *one;
    *one = *two;
    *two = save;
}

/* dict_open_and_capture_msg - open dictionary and capture msg(3) output */

DICT   *dict_open_and_capture_msg(const char *type_name, int open_flags,
			               int dict_flags, VSTRING *out_msg_buf)
{
    VSTREAM *memory_stream;
    DICT   *dict;

    VSTRING_RESET(out_msg_buf);
    VSTRING_TERMINATE(out_msg_buf);
    if ((memory_stream = vstream_memopen(out_msg_buf, O_WRONLY)) == 0)
	msg_fatal("open memory stream: %m");
    vstream_swap(VSTREAM_ERR, memory_stream);
    dict = dict_open(type_name, open_flags, dict_flags);
    vstream_swap(memory_stream, VSTREAM_ERR);
    (void) vstream_fclose(memory_stream);
    return (dict);
}

/* dict_compose_spec - compose aggregate spec and component registered specs */

char   *dict_compose_spec(const char *dict_type,
			          const char **component_specs,
			          int open_flags, int dict_flags,
			          VSTRING *out_composite_spec,
			          ARGV *out_reg_component_specs)
{
    VSTRING *reg_spec = vstring_alloc(100);
    const char **cpp;

    /*
     * A dictionary spec is formatted as "type:name", and a dictionary is
     * registered with dict_register() as "type:name(open_flags,dict_flags)".
     * The latter form is used to share dictionary instances that have the
     * exact same properties.
     * 
     * Build the composite dictionary spec from the dict_type and component
     * dictionary specs, and build the list of component specs decorated with
     * open_flags and initial dict_flags such as locking.
     * 
     * Normally, these decorated specs are used for registering tables with
     * dict_register() and for looking them up with dict_handle(). For
     * testing, we need those names to determine whether a component
     * dictionary is registered.
     * 
     * The dict_flags in a registered component spec may differ from actual
     * dictionary flags: when a dictionary is opened, it may add dict_flags
     * that describe its own properties such as whether the table's left-hand
     * side is a fixed string or a pattern.
     */
    argv_truncate(out_reg_component_specs, 0);
    vstring_strcpy(out_composite_spec, dict_type);
    vstring_strcat(out_composite_spec, ":{");
    for (cpp = component_specs; *cpp; cpp++) {
	vstring_strcat(out_composite_spec, *cpp);
	if (cpp[1])
	    vstring_strcat(out_composite_spec, ",");
	dict_make_registered_name(reg_spec, *cpp, open_flags, dict_flags);
	argv_add(out_reg_component_specs, STR(reg_spec), (char *) 0);
    }
    vstring_strcat(out_composite_spec, "}");
    vstring_free(reg_spec);
    return (STR(out_composite_spec));
}

/* dict_get_and_capture_msg - deploy dict_get() and capture msg(3) output */

const char *dict_get_and_capture_msg(DICT *dict, const char *key,
				             VSTRING *out_msg_buf)
{
    VSTREAM *memory_stream;
    const char *value;

    VSTRING_RESET(out_msg_buf);
    VSTRING_TERMINATE(out_msg_buf);
    if ((memory_stream = vstream_memopen(out_msg_buf, O_WRONLY)) == 0)
	msg_fatal("open memory stream: %m");
    vstream_swap(VSTREAM_ERR, memory_stream);
    value = dict_get(dict, key);
    vstream_swap(memory_stream, VSTREAM_ERR);
    (void) vstream_fclose(memory_stream);
    return (value);
}

/* dict_get_and_verify - deploy dict_get() and verify results */

int     dict_get_and_verify(DICT *dict, const char *key, const char *want_value,
			            int want_error, const char *want_msg)
{
    VSTRING *msg_buf = vstring_alloc(100);
    int     ret = PASS;
    const char *got;

    got = dict_get_and_capture_msg(dict, key, msg_buf);
    if (LEN(msg_buf) > 0 && want_msg == 0) {
	msg_warn("unexpected error message: '%s'", STR(msg_buf));
	ret = FAIL;
    } else if (want_msg != 0 && strstr(STR(msg_buf), want_msg) == 0) {
	msg_warn("unexpected error message: got '%s', want '%s'",
		 STR(msg_buf), want_msg);
	ret = FAIL;
    } else if (dict->error != want_error) {
	msg_warn("unexpected lookup error for '%s': got '%d', want '%d",
		 key, dict->error, want_error);
	ret = FAIL;
    } else if (got == 0) {
	if (want_value != 0) {
	    msg_warn("unexpected lookup result for '%s': got '%s', want '%s'",
		     key, "NOTFOUND", want_value);
	    ret = FAIL;
	}
    } else {
	if (want_value == 0) {
	    msg_warn("unexpected lookup result for '%s': got '%s', want '%s'",
		     key, got, "NOTFOUND");
	    ret = FAIL;
	} else if (strcmp(got, want_value) != 0) {
	    msg_warn("unexpected lookup result for '%s': got '%s', want '%s'",
		     key, got, want_value);
	    ret = FAIL;
	}
    }
    vstring_free(msg_buf);
    return (ret);
}

/* dict_get_and_verify_bulk - dict_get_and_verify() wrapper for bulk usage */

int     dict_get_and_verify_bulk(DICT *dict,
			           const struct dict_get_verify_data * data)
{
    int     ret = PASS;
    const struct dict_get_verify_data *dp;

    for (dp = data; dp->key; dp++) {
	if (dict_get_and_verify(dict, dp->key, dp->want_value,
				dp->want_error, dp->want_msg) == FAIL)
	    ret = FAIL;
    }
    return (ret);
}
