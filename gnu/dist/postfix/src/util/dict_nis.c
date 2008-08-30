/*++
/* NAME
/*	dict_nis 3
/* SUMMARY
/*	dictionary manager interface to NIS maps
/* SYNOPSIS
/*	#include <dict_nis.h>
/*
/*	DICT	*dict_nis_open(map, open_flags, dict_flags)
/*	const char *map;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_nis_open() makes the specified NIS map accessible via
/*	the generic dictionary operations described in dict_open(3).
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* DIAGNOSTICS
/*	Fatal errors: out of memory, attempt to update NIS map.
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

#include "sys_defs.h"
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

#ifdef HAS_NIS

#include <rpcsvc/ypclnt.h>
#ifndef YPERR_BUSY
#define YPERR_BUSY  16
#endif
#ifndef YPERR_ACCESS
#define YPERR_ACCESS  15
#endif

#endif

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "vstring.h"
#include "stringops.h"
#include "dict.h"
#include "dict_nis.h"

#ifdef HAS_NIS

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
} DICT_NIS;

 /*
  * Class variables, so that multiple maps can share this info.
  */
static char dict_nis_disabled[1];
static char *dict_nis_domain;

/* dict_nis_init - NIS binding */

static void dict_nis_init(void)
{
    const char *myname = "dict_nis_init";

    if (yp_get_default_domain(&dict_nis_domain) != 0
	|| dict_nis_domain == 0 || *dict_nis_domain == 0
	|| strcasecmp(dict_nis_domain, "(none)") == 0) {
	dict_nis_domain = dict_nis_disabled;
	msg_warn("%s: NIS domain name not set - NIS lookups disabled", myname);
    }
    if (msg_verbose)
	msg_info("%s: NIS domain %s", myname, dict_nis_domain);
}

/* dict_nis_strerror - map error number to string */

static char *dict_nis_strerror(int err)
{

    /*
     * Grr. There should be a standard function for this.
     */
    switch (err) {
	case YPERR_BADARGS:
	return ("args to function are bad");
    case YPERR_RPC:
	return ("RPC failure - domain has been unbound");
    case YPERR_DOMAIN:
	return ("can't bind to server on this domain");
    case YPERR_MAP:
	return ("no such map in server's domain");
    case YPERR_KEY:
	return ("no such key in map");
    case YPERR_YPERR:
	return ("internal yp server or client error");
    case YPERR_RESRC:
	return ("resource allocation failure");
    case YPERR_NOMORE:
	return ("no more records in map database");
    case YPERR_PMAP:
	return ("can't communicate with portmapper");
    case YPERR_YPBIND:
	return ("can't communicate with ypbind");
    case YPERR_YPSERV:
	return ("can't communicate with ypserv");
    case YPERR_NODOM:
	return ("local domain name not set");
    case YPERR_BADDB:
	return ("yp database is bad");
    case YPERR_VERS:
	return ("yp version mismatch");
    case YPERR_ACCESS:
	return ("access violation");
    case YPERR_BUSY:
	return ("database busy");
    default:
	return ("unknown NIS lookup error");
    }
}

/* dict_nis_lookup - find table entry */

static const char *dict_nis_lookup(DICT *dict, const char *key)
{
    DICT_NIS *dict_nis = (DICT_NIS *) dict;
    static char *result;
    int     result_len;
    int     err;
    static VSTRING *buf;

    /*
     * Sanity check.
     */
    if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	msg_panic("dict_nis_lookup: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

    dict_errno = 0;
    if (dict_nis_domain == dict_nis_disabled)
	return (0);

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
     * See if this NIS map was written with one null byte appended to key and
     * value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	err = yp_match(dict_nis_domain, dict_nis->dict.name,
		       (void *) key, strlen(key) + 1,
		       &result, &result_len);
	if (err == 0) {
	    dict->flags &= ~DICT_FLAG_TRY0NULL;
	    return (result);
	}
    }

    /*
     * See if this NIS map was written with no null byte appended to key and
     * value. This should never be the case, but better play safe.
     */
    if (dict->flags & DICT_FLAG_TRY0NULL) {
	err = yp_match(dict_nis_domain, dict_nis->dict.name,
		       (void *) key, strlen(key),
		       &result, &result_len);
	if (err == 0) {
	    dict->flags &= ~DICT_FLAG_TRY1NULL;
	    if (buf == 0)
		buf = vstring_alloc(10);
	    vstring_strncpy(buf, result, result_len);
	    return (vstring_str(buf));
	}
    }

    /*
     * When the NIS lookup fails for reasons other than "key not found", keep
     * logging warnings, and hope that someone will eventually notice the
     * problem and fix it.
     */
    if (err != YPERR_KEY) {
	msg_warn("lookup %s, NIS domain %s, map %s: %s",
		 key, dict_nis_domain, dict_nis->dict.name,
		 dict_nis_strerror(err));
	dict_errno = DICT_ERR_RETRY;
    }
    return (0);
}

/* dict_nis_close - close NIS map */

static void dict_nis_close(DICT *dict)
{
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_nis_open - open NIS map */

DICT   *dict_nis_open(const char *map, int open_flags, int dict_flags)
{
    DICT_NIS *dict_nis;

    if (open_flags != O_RDONLY)
	msg_fatal("%s:%s map requires O_RDONLY access mode",
		  DICT_TYPE_NIS, map);

    dict_nis = (DICT_NIS *) dict_alloc(DICT_TYPE_NIS, map, sizeof(*dict_nis));
    dict_nis->dict.lookup = dict_nis_lookup;
    dict_nis->dict.close = dict_nis_close;
    dict_nis->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if ((dict_flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	dict_nis->dict.flags |= (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL);
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict_nis->dict.fold_buf = vstring_alloc(10);
    if (dict_nis_domain == 0)
	dict_nis_init();
    return (DICT_DEBUG (&dict_nis->dict));
}

#endif
