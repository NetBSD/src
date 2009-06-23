/*	$NetBSD: dict_ni.c,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

/*++
/* NAME
/*	dict_ni 3
/* SUMMARY
/*	dictionary manager interface to NetInfo
/* SYNOPSIS
/*	#include <dict_ni.h>
/*
/*	DICT	*dict_ni_open(path, dummy, dict_flags)
/*	char	*path;
/*	int	dummy;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_ni_open() `opens' the named NetInfo database. The result is
/*	a pointer to a structure that can be used to access the dictionary
/*	using the generic methods documented in dict_open(3).
/* DIAGNOSTICS
/*	dict_ni_register() returns 0 in case of success, -1 in case
/*	of problems.
/*	Fatal errors: NetInfo errors, out of memory.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/*	netinfo(3N) data base subroutines
/* AUTHOR(S)
/*	Pieter Schoenmakers
/*	Eindhoven University of Technology
/*	P.O. Box 513
/*	5600 MB Eindhoven
/*	The Netherlands
/*--*/

#include "sys_defs.h"

#ifdef HAS_NETINFO

/* System library. */

#include <stdio.h>
#include <netinfo/ni.h>

/* Utility library. */

#include "dict.h"
#include "dict_ni.h"
#include "msg.h"
#include "mymalloc.h"
#include "stringops.h"

typedef struct {
    DICT    dict;			/* my super */
    char   *path;			/* directory path */
} DICT_NI;

 /*
  * We'd like other possibilities, but that is not possible in the current
  * dictionary setup...  An example of a different setup: use `members' for
  * multi-valued lookups (to be compatible with /aliases), and `value' for
  * single-valued tables.
  */
#define NETINFO_PROP_KEY	"name"
#define NETINFO_PROP_VALUE	"members"
#define NETINFO_VALUE_SEP	 ","

#define NETINFO_MAX_DOMAIN_DEPTH	100

/* Hard worker doing lookups.	Returned value is statically allocated and
   reused each call.  */
static const char *dict_ni_do_lookup(char *path, char *key_prop,
			              const char *key_value, char *val_prop)
{
    unsigned int result_cap = 0;
    static char *result = 0;

    char   *return_val = 0;
    ni_namelist values;
    int     depth = 0;
    void   *domain;
    void   *next_domain;
    char   *query;
    ni_status r;
    ni_id   dir;

    dict_errno = 0;

    if (msg_verbose)
	msg_info("ni_lookup %s %s=%s", path, key_prop, key_value);

    r = ni_open(NULL, ".", &domain);
    if (r != NI_OK) {
	msg_warn("ni_open `.': %d", r);
	return NULL;
    }
    query = alloca(strlen(path) + strlen(key_prop) + 3 + strlen(key_value));
    sprintf(query, "%s/%s=%s", path, key_prop, key_value);

    for (;;) {

	/*
	 * What does it _mean_ if we find the directory but not the value?
	 */
	if (ni_pathsearch(domain, &dir, query) == NI_OK
	    && ni_lookupprop(domain, &dir, val_prop, &values) == NI_OK)
	    if (values.ni_namelist_len <= 0)
		ni_namelist_free(&values);
	    else {
		unsigned int i, l, n;

		for (i = l = 0; i < values.ni_namelist_len; i++)
		    l += 1 + strlen(values.ni_namelist_val[i]);
		if (result_cap < l) {
		    if (result)
			myfree(result);
		    result_cap = l + 100;
		    result = mymalloc(result_cap);
		}
		for (i = l = 0; i < values.ni_namelist_len; i++) {
		    n = strlen(values.ni_namelist_val[i]);
		    memcpy(result + l, values.ni_namelist_val[i], n);
		    l += n;
		    if (i < values.ni_namelist_len - 1)
			result[l++] = ',';
		}
		result[l] = '\0';
		return_val = result;
		break;
	    }

	if (++depth >= NETINFO_MAX_DOMAIN_DEPTH) {
	    msg_warn("ni_open: domain depth limit");
	    break;
	}
	r = ni_open(domain, "..", &next_domain);
	if (r != NI_OK) {
	    if (r != NI_FAILED)
		msg_warn("ni_open `..': %d", r);
	    break;
	}
	ni_free(domain);
	domain = next_domain;
    }

    ni_free(domain);

    return return_val;
}

/* dict_ni_lookup - find table entry */

static const char *dict_ni_lookup(DICT *dict, const char *key)
{
    DICT_NI *d = (DICT_NI *) dict;

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, key);
	key = lowercase(vstring_str(dict->fold_buf));
    }
    return dict_ni_do_lookup(d->dict.name, NETINFO_PROP_KEY,
			     key, NETINFO_PROP_VALUE);
}

/* dict_ni_close - disassociate from NetInfo map */

static void dict_ni_close(DICT *dict)
{
    DICT_NI *d = (DICT_NI *) dict;

    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_ni_open - create association with NetInfo map */

DICT   *dict_ni_open(const char *path, int unused_flags, int dict_flags)
{
    DICT_NI *d = (void *) dict_alloc(DICT_TYPE_NETINFO, path, sizeof(*d));

    d->dict.lookup = dict_ni_lookup;
    d->dict.close = dict_ni_close;
    d->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	d->dict.fold_buf = vstring_alloc(10);

    return (DICT_DEBUG (&d->dict));
}

#endif
