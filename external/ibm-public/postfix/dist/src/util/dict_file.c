/*	$NetBSD: dict_file.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	dict_file_to_buf 3
/* SUMMARY
/*	include content from file as blob
/* SYNOPSIS
/*	#include <dict.h>
/*
/*	VSTRING	*dict_file_to_buf(
/*	DICT	*dict,
/*	const char *pathnames)
/*
/*	VSTRING	*dict_file_to_b64(
/*	DICT	*dict,
/*	const char *pathnames)
/*
/*	VSTRING	*dict_file_from_b64(
/*	DICT	*dict,
/*	const char *value)
/*
/*	char	*dict_file_get_error(
/*	DICT	*dict)
/*
/*	void	dict_file_purge_buffers(
/*	DICT	*dict)
/*
/*	const char *dict_file_lookup(
/*	DICT	*dict)
/* DESCRIPTION
/*	dict_file_to_buf() reads the content of the specified files,
/*	with names separated by CHARS_COMMA_SP, while inserting a
/*	gratuitous newline character between files. It returns a
/*	pointer to a buffer which is owned by the DICT, or a null
/*	pointer in case of error.
/*
/*	dict_file_to_b64() invokes dict_file_to_buf() and converts
/*	the result to base64. It returns a pointer to a buffer which
/*	is owned by the DICT, or a null pointer in case of error.
/*
/*	dict_file_from_b64() converts a value from base64. It returns
/*	a pointer to a buffer which is owned by the DICT, or a null
/*	pointer in case of error.
/*
/*	dict_file_purge_buffers() disposes of dict_file-related
/*	memory that are associated with this DICT.
/*
/*	dict_file_get_error() should be called only after error;
/*	it returns a description of the problem. Storage is owned
/*	by the caller.
/*
/*	dict_file_lookup() wraps the dictionary lookup method and
/*	decodes the base64 lookup result. The dictionary must be
/*	opened with DICT_FLAG_SRC_RHS_IS_FILE. Sets dict->error to
/*	DICT_ERR_CONFIG if the content is invalid. Decoding is not
/*	built into the dict->lookup() method, because that would
/*	complicate the implementation of map nesting (inline, thash),
/*	map composition (pipemap, unionmap), and map proxying.
/* DIAGNOSTICS
/*	Panic: interface violation.
/*
/*	In case of error the VSTRING result value is a null pointer, and
/*	an error description can be retrieved with dict_file_get_error().
/*	The storage is owned by the caller.
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
#include <sys/stat.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <base64_code.h>
#include <dict.h>
#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * SLMs.
  */
#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

/* dict_file_to_buf - read files into a buffer */

VSTRING *dict_file_to_buf(DICT *dict, const char *pathnames)
{
    struct stat st;
    VSTREAM *fp = 0;
    ARGV   *argv;
    char  **cpp;

    /* dict_file_to_buf() postcondition: dict->file_buf exists. */
    if (dict->file_buf == 0)
	dict->file_buf = vstring_alloc(100);

#define DICT_FILE_RETURN(retval) do { \
	argv_free(argv); \
	if (fp) vstream_fclose(fp); \
	return (retval); \
    } while (0);

    argv = argv_split(pathnames, CHARS_COMMA_SP);
    if (argv->argc == 0) {
	vstring_sprintf(dict->file_buf, "empty pathname list: >>%s<<'",
			pathnames);
	DICT_FILE_RETURN(0);
    }
    VSTRING_RESET(dict->file_buf);
    for (cpp = argv->argv; *cpp; cpp++) {
	if ((fp = vstream_fopen(*cpp, O_RDONLY, 0)) == 0
	    || fstat(vstream_fileno(fp), &st) < 0) {
	    vstring_sprintf(dict->file_buf, "open %s: %m", *cpp);
	    DICT_FILE_RETURN(0);
	}
	if (st.st_size > SSIZE_T_MAX - LEN(dict->file_buf)) {
	    vstring_sprintf(dict->file_buf, "file too large: %s", pathnames);
	    DICT_FILE_RETURN(0);
	}
	if (vstream_fread_app(fp, dict->file_buf, st.st_size) != st.st_size) {
	    vstring_sprintf(dict->file_buf, "read %s: %m", *cpp);
	    DICT_FILE_RETURN(0);
	}
	(void) vstream_fclose(fp);
	fp = 0;
	if (cpp[1] != 0)
	    VSTRING_ADDCH(dict->file_buf, '\n');
    }
    VSTRING_TERMINATE(dict->file_buf);
    DICT_FILE_RETURN(dict->file_buf);
}

/* dict_file_to_b64 - read files into a base64-encoded buffer */

VSTRING *dict_file_to_b64(DICT *dict, const char *pathnames)
{
    ssize_t helper;

    if (dict_file_to_buf(dict, pathnames) == 0)
	return (0);
    if (dict->file_b64 == 0)
	dict->file_b64 = vstring_alloc(100);
    helper = (LEN(dict->file_buf) + 2) / 3;
    if (helper > SSIZE_T_MAX / 4) {
	vstring_sprintf(dict->file_buf, "file too large: %s", pathnames);
	return (0);
    }
    VSTRING_RESET(dict->file_b64);
    VSTRING_SPACE(dict->file_b64, helper * 4);
    return (base64_encode(dict->file_b64, STR(dict->file_buf),
			  LEN(dict->file_buf)));
}

/* dict_file_from_b64 - convert value from base64 */

VSTRING *dict_file_from_b64(DICT *dict, const char *value)
{
    ssize_t helper;
    VSTRING *result;

    if (dict->file_buf == 0)
	dict->file_buf = vstring_alloc(100);
    helper = strlen(value) / 4;
    VSTRING_RESET(dict->file_buf);
    VSTRING_SPACE(dict->file_buf, helper * 3);
    result = base64_decode(dict->file_buf, value, strlen(value));
    if (result == 0)
	vstring_sprintf(dict->file_buf, "malformed BASE64 value: %.30s", value);
    return (result);
}

/* dict_file_get_error - return error text */

char   *dict_file_get_error(DICT *dict)
{
    if (dict->file_buf == 0)
	msg_panic("dict_file_get_error: no buffer");
    return (mystrdup(STR(dict->file_buf)));
}

/* dict_file_purge_buffers - purge file buffers */

void    dict_file_purge_buffers(DICT *dict)
{
    if (dict->file_buf) {
	vstring_free(dict->file_buf);
	dict->file_buf = 0;
    }
    if (dict->file_b64) {
	vstring_free(dict->file_b64);
	dict->file_b64 = 0;
    }
}

/* dict_file_lookup - look up and decode dictionary entry */

const char *dict_file_lookup(DICT *dict, const char *key)
{
    const char myname[] = "dict_file_lookup";
    const char *res;
    VSTRING *unb64;
    char   *err;

    if ((dict->flags & DICT_FLAG_SRC_RHS_IS_FILE) == 0)
	msg_panic("%s: dictionary opened without DICT_FLAG_SRC_RHS_IS_FILE",
		  myname);
    if ((res = dict->lookup(dict, key)) == 0)
	return (0);
    if ((unb64 = dict_file_from_b64(dict, res)) == 0) {
	err = dict_file_get_error(dict);
	msg_warn("table %s:%s: key %s: %s", dict->type, dict->name, key, err);
	myfree(err);
	dict->error = DICT_ERR_CONFIG;
	return (0);
    }
    return STR(unb64);
}
