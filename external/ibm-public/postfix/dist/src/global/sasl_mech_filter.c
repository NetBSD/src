/*	$NetBSD: sasl_mech_filter.c,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	sasl_mech_filter 3
/* SUMMARY
/*	Filter SASL mechanism names
/* SYNOPSIS
/*	#include sasl_mech_filter.h
/*
/*	const char *sasl_mech_filter(
/*	STRING_LIST *filter,
/*	const char *words)
/* DESCRIPTION
/*	sasl_mech_filter() applies the specified filter to a list
/*	of SASL mechanism names. The filter is case-insensitive,
/*	but preserves the case of input words.
/*
/*	Arguments:
/* .IP filter
/*	Null pointer or filter specification. If this is a nulll
/*	pointer, no filter will be applied.
/* .IP words
/*	List of SASL authentication mechanisms (separated by blanks).
/*	If the string is empty, the filter will not be applied.
/* DIAGNOSTICS
/*	Fatal errors: memory allocation, table lookup error.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Original author:
/*	Till Franke
/*	SuSE Rhein/Main AG
/*	65760 Eschborn, Germany
/*
/*	Adopted by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

#include <sasl_mech_filter.h>

#ifdef USE_SASL_AUTH

/* sasl_mech_filter - filter a SASL mechanism list */

const char *sasl_mech_filter(STRING_LIST *filter,
			             const char *words)
{
    const char myname[] = "sasl_mech_filter";
    static VSTRING *buf;
    char   *mech_list;
    char   *save_mech;
    char   *mech;

    /*
     * NOOP if there is no filter, or if the mechanism list is empty.
     */
    if (filter == 0 || *words == 0)
	return (words);

    if (buf == 0)
	buf = vstring_alloc(10);

    VSTRING_RESET(buf);
    VSTRING_TERMINATE(buf);

    save_mech = mech_list = mystrdup(words);

    while ((mech = mystrtok(&mech_list, " \t")) != 0) {
	if (string_list_match(filter, mech)) {
	    if (VSTRING_LEN(buf) > 0)
		VSTRING_ADDCH(buf, ' ');
	    vstring_strcat(buf, mech);
	    if (msg_verbose)
		msg_info("%s: keep SASL mechanism: '%s'", myname, mech);
	} else if (filter->error) {
	    msg_fatal("%s: SASL mechanism filter failed for: '%s'",
		      myname, mech);
	} else {
	    if (msg_verbose)
		msg_info("%s: drop SASL mechanism: '%s'", myname, mech);
	}
    }
    myfree(save_mech);

    return (vstring_str(buf));
}

#endif
