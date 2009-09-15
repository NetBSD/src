/*	$NetBSD: match_parent_style.c,v 1.1.1.1.2.2 2009/09/15 06:02:47 snj Exp $	*/

/*++
/* NAME
/*	match_parent_style 3
/* SUMMARY
/*	parent domain matching control
/* SYNOPSIS
/*	#include <match_parent_style.h>
/*
/*	int	match_parent_style(name)
/*	const char *name;
/* DESCRIPTION
/*	This module queries configuration parameters for the policy that
/*	controls how wild-card parent domain names are used by various
/*	postfix lookup mechanisms.
/*
/*	match_parent_style() looks up "name" in the
/*      parent_domain_matches_subdomain configuration parameter
/*	and returns either MATCH_FLAG_PARENT (parent domain matches
/*	subdomains) or MATCH_FLAG_NONE.
/* DIAGNOSTICS
/*	Fatal error: out of memory, name listed under both parent wild card
/*	matching policies.
/* SEE ALSO
/*	string_list(3) plain string matching
/*	domain_list(3) match host name patterns
/*	namadr_list(3) match host name/address patterns
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

/* Utility library. */

/* Global library. */

#include <string_list.h>
#include <mail_params.h>
#include <match_parent_style.h>

/* Application-specific. */

static STRING_LIST *match_par_dom_list;

int     match_parent_style(const char *name)
{
    int     result;

    /*
     * Initialize on the fly.
     */
    if (match_par_dom_list == 0)
	match_par_dom_list =
	    string_list_init(MATCH_FLAG_NONE, var_par_dom_match);

    /*
     * Look up the parent domain matching policy.
     */
    if (string_list_match(match_par_dom_list, name))
	result = MATCH_FLAG_PARENT;
    else
	result = 0;
    return (result);
}
