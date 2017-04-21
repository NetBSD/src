/*	$NetBSD: dsn_filter.c,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

/*++
/* NAME
/*	dsn_filter 3
/* SUMMARY
/*	filter delivery status code or text
/* SYNOPSIS
/*	#include <dsn_filter.h>
/*
/*	DSN_FILTER *dsn_filter_create(
/*	const char *title,
/*	const char *map_names)
/*
/*	DSN	*dsn_filter_lookup(
/*	DSN_FILTER *fp,
/*	DSN	*dsn)
/*
/*	void	dsn_filter_free(
/*	DSN_FILTER *fp)
/* DESCRIPTION
/*	This module maps (bounce or defer non-delivery status code
/*	and text) into replacement (bounce or defer non-delivery
/*	status code and text), or maps (success status code and
/*	text) into replacement (success status code and text). Other
/*	DSN attributes are passed through without modification.
/*
/*	dsn_filter_create() instantiates a delivery status filter.
/*
/*	dsn_filter_lookup() queries the specified filter. The input
/*	DSN must be a success, bounce or defer DSN. If a match is
/*	found a non-delivery status must map to a non-delivery
/*	status, a success status must map to a success status, and
/*	the text must be non-empty. The result is a null pointer
/*	when no valid match is found. Otherwise, the result is
/*	overwritten upon each call.  This function must not be
/*	called with the result from a dsn_filter_lookup() call.
/*
/*	dsn_filter_free() destroys the specified delivery status
/*	filter.
/*
/*	Arguments:
/* .IP title
/*	Origin of the mapnames argument, typically a configuration
/*	parameter name. This is reported in diagnostics.
/* .IP mapnames
/*	List of lookup tables, separated by whitespace or comma.
/* .IP fp
/*	filter created with dsn_filter_create()
/* .IP dsn
/*	A success, bounce or defer DSN data structure. The
/*	dsn_filter_lookup() result value is in part a shallow copy
/*	of this argument.
/* SEE ALSO
/*	maps(3) multi-table search
/* DIAGNOSTICS
/*	Panic: invalid dsn argument; recursive call. Fatal error:
/*	memory allocation problem. Warning: invalid DSN lookup
/*	result.
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

 /*
  * System libraries.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <maps.h>
#include <dsn.h>
#include <dsn_util.h>
#include <maps.h>
#include <dsn_filter.h>

 /*
  * Private data structure.
  */
struct DSN_FILTER {
    MAPS   *maps;			/* Replacement (status, text) */
    VSTRING *buffer;			/* Status code and text */
    DSN_SPLIT dp;			/* Parsing aid */
    DSN     dsn;			/* Shallow copy */
};

 /*
  * SLMs.
  */
#define STR(x) vstring_str(x)

/* dsn_filter_create - create delivery status filter */

DSN_FILTER *dsn_filter_create(const char *title, const char *map_names)
{
    static const char myname[] = "dsn_filter_create";
    DSN_FILTER *fp;

    if (msg_verbose)
	msg_info("%s: %s %s", myname, title, map_names);

    fp = (DSN_FILTER *) mymalloc(sizeof(*fp));
    fp->buffer = vstring_alloc(100);
    fp->maps = maps_create(title, map_names, DICT_FLAG_LOCK);
    return (fp);
}

/* dsn_filter_lookup - apply delivery status filter */

DSN    *dsn_filter_lookup(DSN_FILTER *fp, DSN *dsn)
{
    static const char myname[] = "dsn_filter_lookup";
    const char *result;
    int     ndr_dsn = 0;

    if (msg_verbose)
	msg_info("%s: %s %s", myname, dsn->status, dsn->reason);

    /*
     * XXX Instead of hard-coded '4' etc., use some form of encapsulation
     * when reading or updating the status class field.
     */
#define IS_SUCCESS_DSN(s) (dsn_valid(s) && (s)[0] == '2')
#define IS_NDR_DSN(s) (dsn_valid(s) && ((s)[0] == '4' || (s)[0] == '5'))

    /*
     * Sanity check. We filter only success/bounce/defer DSNs.
     */
    if (IS_SUCCESS_DSN(dsn->status))
	ndr_dsn = 0;
    else if (IS_NDR_DSN(dsn->status))
	ndr_dsn = 1;
    else
	msg_panic("%s: dsn argument with bad status code: %s",
		  myname, dsn->status);

    /*
     * Sanity check. A delivery status filter must not be invoked with its
     * own result.
     */
    if (dsn->reason == fp->dsn.reason)
	msg_panic("%s: recursive call is not allowed", myname);

    /*
     * Look up replacement status and text.
     */
    vstring_sprintf(fp->buffer, "%s %s", dsn->status, dsn->reason);
    if ((result = maps_find(fp->maps, STR(fp->buffer), 0)) != 0) {
	/* Sanity check. Do not allow success<=>error mappings. */
	if ((ndr_dsn == 0 && !IS_SUCCESS_DSN(result))
	    || (ndr_dsn != 0 && !IS_NDR_DSN(result))) {
	    msg_warn("%s: bad status code: %s", fp->maps->title, result);
	    return (0);
	} else {
	    vstring_strcpy(fp->buffer, result);
	    dsn_split(&fp->dp, "can't happen", STR(fp->buffer));
	    (void) DSN_ASSIGN(&fp->dsn, DSN_STATUS(fp->dp.dsn),
			      (result[0] == '4' ? "delayed" :
			       result[0] == '5' ? "failed" :
			       dsn->action),
			      fp->dp.text, dsn->dtype, dsn->dtext,
			      dsn->mtype, dsn->mname);
	    return (&fp->dsn);
	}
    }
    return (0);
}

/* dsn_filter_free - destroy delivery status filter */

void    dsn_filter_free(DSN_FILTER *fp)
{
    static const char myname[] = "dsn_filter_free";

    if (msg_verbose)
	msg_info("%s: %s", myname, fp->maps->title);

    maps_free(fp->maps);
    vstring_free(fp->buffer);
    myfree((void *) fp);
}
