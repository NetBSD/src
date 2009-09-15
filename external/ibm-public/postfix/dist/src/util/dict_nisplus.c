/*	$NetBSD: dict_nisplus.c,v 1.1.1.1.2.2 2009/09/15 06:03:56 snj Exp $	*/

/*++
/* NAME
/*	dict_nisplus 3
/* SUMMARY
/*	dictionary manager interface to NIS+ maps
/* SYNOPSIS
/*	#include <dict_nisplus.h>
/*
/*	DICT	*dict_nisplus_open(map, open_flags, dict_flags)
/*	const char *map;
/*	int	dummy;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_nisplus_open() makes the specified NIS+ map accessible via
/*	the generic dictionary operations described in dict_open(3).
/*	The \fIdummy\fR argument is not used.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* DIAGNOSTICS
/*	Fatal errors:
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Geoff Gibbs
/*	UK-HGMP-RC
/*	Hinxton
/*	Cambridge
/*	CB10 1SB, UK
/*
/*	based on the code for dict_nis.c et al by :-
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef HAS_NISPLUS
#include <rpcsvc/nis.h>			/* for nis_list */
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <stringops.h>
#include <dict.h>
#include <dict_nisplus.h>

#ifdef HAS_NISPLUS

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    char   *template;			/* parsed query template */
    int     column;			/* NIS+ field number (start at 1) */
} DICT_NISPLUS;

 /*
  * Begin quote from nis+(1):
  * 
  * The following text represents a  context-free  grammar  that defines  the
  * set of legal  NIS+ names. The terminals in this grammar are the
  * characters `.' (dot),  `['  (open  bracket), `]'  (close  bracket),  `,'
  * (comma),  `=' (equals) and whitespace. Angle brackets (`<' and `>'),
  * which delineate  non- terminals,  are  not  part of the grammar. The
  * character `|' (vertical bar) is used to separate alternate productions
  * and should be read as ``this production OR this production''.
  * 
  * name                ::=    . | <simple name> | <indexed name>
  * 
  * simple name         ::=    <string>. | <string>.<simple name>
  * 
  * indexed name        ::=    <search criterion>,<simple name>
  * 
  * search criterion    ::=    [ <attribute list> ]
  * 
  * attribute list      ::=    <attribute> |  <attribute>,<attribute list>
  * 
  * attribute           ::=    <string> = <string>
  * 
  * string              ::=    ISO Latin 1 character set except  the character
  * '/'  (slash).  The initial character may not be a terminal character or
  * the characters '@' (at), '+' (plus), or (`-') hyphen.
  * 
  * Terminals that appear in strings must be  quoted   with  `"' (double quote).
  * The `"' character may be quoted by quoting it with itself `""'.
  * 
  * End quote fron nis+(1).
  * 
  * This NIS client always quotes the entire query string (the value part of
  * [attribute=value],file.domain.) so the issue with initial characters
  * should not be applicable. One wonders what restrictions are applicable
  * when a string is quoted, but the manual doesn't specify what can appear
  * between quotes, and we don't want to get burned.
  */

 /*
  * SLMs.
  */
#define STR(x) vstring_str(x)

/* dict_nisplus_lookup - find table entry */

static const char *dict_nisplus_lookup(DICT *dict, const char *key)
{
    const char *myname = "dict_nisplus_lookup";
    DICT_NISPLUS *dict_nisplus = (DICT_NISPLUS *) dict;
    static VSTRING *quoted_key;
    static VSTRING *query;
    static VSTRING *retval;
    nis_result *reply;
    int     count;
    const char *cp;
    int     last_col;
    int     ch;

    /*
     * Initialize.
     */
    dict_errno = 0;
    if (quoted_key == 0) {
	query = vstring_alloc(100);
	retval = vstring_alloc(100);
	quoted_key = vstring_alloc(100);
    }

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, key);
	key = lowercase(vstring_str(dict->fold_buf));
    }

    /*
     * Check that the lookup key does not contain characters disallowed by
     * nis+(1).
     * 
     * XXX Many client implementations don't seem to care about disallowed
     * characters.
     */
    VSTRING_RESET(quoted_key);
    VSTRING_ADDCH(quoted_key, '"');
    for (cp = key; (ch = *(unsigned const char *) cp) != 0; cp++) {
	if ((ISASCII(ch) && !ISPRINT(ch)) || (ch > 126 && ch < 160)) {
	    msg_warn("map %s:%s: lookup key with non-printing character 0x%x:"
		     " ignoring this request",
		     dict->type, dict->name, ch);
	    return (0);
	} else if (ch == '"') {
	    VSTRING_ADDCH(quoted_key, '"');
	}
	VSTRING_ADDCH(quoted_key, ch);
    }
    VSTRING_ADDCH(quoted_key, '"');
    VSTRING_TERMINATE(quoted_key);

    /*
     * Plug the key into the query template, which typically looks something
     * like the following: [alias=%s],mail_aliases.org_dir.my.nisplus.domain.
     * 
     * XXX The nis+ documentation defines a length limit for simple names like
     * a.b.c., but defines no length limit for (the components of) indexed
     * names such as [x=y],a.b.c. Our query length is limited because Postfix
     * addresses (in envelopes or in headers) have a finite length.
     */
    vstring_sprintf(query, dict_nisplus->template, STR(quoted_key));
    reply = nis_list(STR(query), FOLLOW_LINKS | FOLLOW_PATH, NULL, NULL);

    /*
     * When lookup succeeds, the result may be ambiguous, or the requested
     * column may not exist.
     */
    if (reply->status == NIS_SUCCESS) {
	if ((count = NIS_RES_NUMOBJ(reply)) != 1) {
	    msg_warn("ambiguous match (%d results) for %s in NIS+ map %s:"
		     " ignoring this request",
		     count, key, dict_nisplus->dict.name);
	    nis_freeresult(reply);
	    return (0);
	} else {
	    last_col = NIS_RES_OBJECT(reply)->zo_data
		.objdata_u.en_data.en_cols.en_cols_len - 1;
	    if (dict_nisplus->column > last_col)
		msg_fatal("requested column %d > max column %d in table %s",
			  dict_nisplus->column, last_col,
			  dict_nisplus->dict.name);
	    vstring_strcpy(retval,
			   NIS_RES_OBJECT(reply)->zo_data.objdata_u
			   .en_data.en_cols.en_cols_val[dict_nisplus->column]
			   .ec_value.ec_value_val);
	    if (msg_verbose)
		msg_info("%s: %s, column %d -> %s", myname, STR(query),
			 dict_nisplus->column, STR(retval));
	    nis_freeresult(reply);
	    return (STR(retval));
	}
    }

    /*
     * When the NIS+ lookup fails for reasons other than "key not found",
     * keep logging warnings, and hope that someone will eventually notice
     * the problem and fix it.
     */
    else {
	if (reply->status != NIS_NOTFOUND
	    && reply->status != NIS_PARTIAL) {
	    msg_warn("lookup %s, NIS+ map %s: %s",
		     key, dict_nisplus->dict.name,
		     nis_sperrno(reply->status));
	    dict_errno = DICT_ERR_RETRY;
	} else {
	    if (msg_verbose)
		msg_info("%s: not found: query %s", myname, STR(query));
	}
	nis_freeresult(reply);
	return (0);
    }
}

/* dict_nisplus_close - close NISPLUS map */

static void dict_nisplus_close(DICT *dict)
{
    DICT_NISPLUS *dict_nisplus = (DICT_NISPLUS *) dict;

    myfree(dict_nisplus->template);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_nisplus_open - open NISPLUS map */

DICT   *dict_nisplus_open(const char *map, int open_flags, int dict_flags)
{
    const char *myname = "dict_nisplus_open";
    DICT_NISPLUS *dict_nisplus;
    char   *col_field;

    /*
     * Sanity check.
     */
    if (open_flags != O_RDONLY)
	msg_fatal("%s:%s map requires O_RDONLY access mode",
		  DICT_TYPE_NISPLUS, map);

    /*
     * Initialize. This is a read-only map with fixed strings, not with
     * regular expressions.
     */
    dict_nisplus = (DICT_NISPLUS *)
	dict_alloc(DICT_TYPE_NISPLUS, map, sizeof(*dict_nisplus));
    dict_nisplus->dict.lookup = dict_nisplus_lookup;
    dict_nisplus->dict.close = dict_nisplus_close;
    dict_nisplus->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict_nisplus->dict.fold_buf = vstring_alloc(10);

    /*
     * Convert the query template into an indexed name and column number. The
     * query template looks like:
     * 
     * [attribute=%s;attribute=value...];simple.name.:column
     * 
     * One instance of %s gets to be replaced by a version of the lookup key;
     * other attributes must specify fixed values. The reason for using ';'
     * is that the comma character is special in main.cf. When no column
     * number is given at the end of the map name, we use a default column.
     */
    dict_nisplus->template = mystrdup(map);
    translit(dict_nisplus->template, ";", ",");
    if ((col_field = strstr(dict_nisplus->template, ".:")) != 0) {
	col_field[1] = 0;
	col_field += 2;
	if (!alldig(col_field) || (dict_nisplus->column = atoi(col_field)) < 1)
	    msg_fatal("bad column field in NIS+ map name: %s", map);
    } else {
	dict_nisplus->column = 1;
    }
    if (msg_verbose)
	msg_info("%s: opened NIS+ table %s for column %d",
		 myname, dict_nisplus->template, dict_nisplus->column);
    return (DICT_DEBUG (&dict_nisplus->dict));
}

#endif
