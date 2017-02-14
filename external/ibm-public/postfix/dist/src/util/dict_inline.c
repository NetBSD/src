/*	$NetBSD: dict_inline.c,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

/*++
/* NAME
/*	dict_inline 3
/* SUMMARY
/*	dictionary manager interface for inline table
/* SYNOPSIS
/*	#include <dict_inline.h>
/*
/*	DICT	*dict_inline_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_inline_open() opens a read-only, in-memory table.
/*	Example: "\fBinline:{\fIkey_1=value_1, ..., key_n=value_n\fR}".
/*	The longer form with { key = value } allows values that
/*	contain whitespace or comma.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <dict.h>
#include <dict_ht.h>
#include <dict_inline.h>

/* Application-specific. */

/* dict_inline_open - open inline table */

DICT   *dict_inline_open(const char *name, int open_flags, int dict_flags)
{
    DICT   *dict;
    char   *cp, *saved_name = 0;
    size_t  len;
    char   *nameval, *vname, *value;
    const char *err = 0;
    char   *xperr = 0;
    int     count = 0;

    /*
     * Clarity first. Let the optimizer worry about redundant code.
     */
#define DICT_INLINE_RETURN(x) do { \
	    DICT *__d = (x); \
	    if (saved_name != 0) \
		myfree(saved_name); \
	    if (xperr != 0) \
		myfree(xperr); \
	    return (__d); \
	} while (0)

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	DICT_INLINE_RETURN(dict_surrogate(DICT_TYPE_INLINE, name,
					  open_flags, dict_flags,
				  "%s:%s map requires O_RDONLY access mode",
					  DICT_TYPE_INLINE, name));

    /*
     * UTF-8 syntax check.
     */
    if (DICT_NEED_UTF8_ACTIVATION(util_utf8_enable, dict_flags)
	&& allascii(name) == 0
	&& valid_utf8_string(name, strlen(name)) == 0)
	DICT_INLINE_RETURN(dict_surrogate(DICT_TYPE_INLINE, name,
					  open_flags, dict_flags,
					  "bad UTF-8 syntax: \"%s:%s\"; "
					  "need \"%s:{name=value...}\"",
					  DICT_TYPE_INLINE, name,
					  DICT_TYPE_INLINE));

    /*
     * Parse the table into its constituent name=value pairs.
     */
    if ((len = balpar(name, CHARS_BRACE)) == 0 || name[len] != 0
	|| *(cp = saved_name = mystrndup(name + 1, len - 2)) == 0)
	DICT_INLINE_RETURN(dict_surrogate(DICT_TYPE_INLINE, name,
					  open_flags, dict_flags,
					  "bad syntax: \"%s:%s\"; "
					  "need \"%s:{name=value...}\"",
					  DICT_TYPE_INLINE, name,
					  DICT_TYPE_INLINE));

    /*
     * Reuse the "internal" dictionary type.
     */
    dict = dict_open3(DICT_TYPE_HT, name, open_flags, dict_flags);
    dict_type_override(dict, DICT_TYPE_INLINE);
    while ((nameval = mystrtokq(&cp, CHARS_COMMA_SP, CHARS_BRACE)) != 0) {
	if ((nameval[0] != CHARS_BRACE[0]
	     || (err = xperr = extpar(&nameval, CHARS_BRACE, EXTPAR_FLAG_STRIP)) == 0)
	    && (err = split_nameval(nameval, &vname, &value)) != 0)
	    break;

	/* No duplicate checks. See comments in dict_thash.c. */
	dict->update(dict, vname, value);
	count += 1;
    }
    if (err != 0 || count == 0) {
	dict->close(dict);
	DICT_INLINE_RETURN(dict_surrogate(DICT_TYPE_INLINE, name,
					  open_flags, dict_flags,
					  "%s: \"%s:%s\"; "
					  "need \"%s:{name=value...}\"",
					  err != 0 ? err : "empty table",
					  DICT_TYPE_INLINE, name,
					  DICT_TYPE_INLINE));
    }
    dict->owner.status = DICT_OWNER_TRUSTED;

    DICT_INLINE_RETURN(DICT_DEBUG (dict));
}
