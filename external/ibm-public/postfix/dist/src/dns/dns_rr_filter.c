/*	$NetBSD: dns_rr_filter.c,v 1.2 2017/02/14 01:16:44 christos Exp $	*/

/*++
/* NAME
/*	dns_rr_filter 3
/* SUMMARY
/*	DNS resource record filter
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	void	dns_rr_filter_compile(title, map_names)
/*	const char *title;
/*	const char *map_names;
/* INTERNAL INTERFACES
/*	int	dns_rr_filter_execute(rrlist)
/*	DNS_RR	**rrlist;
/*
/*	MAPS	*dns_rr_filter_maps;
/* DESCRIPTION
/*	This module implements a simple filter for dns_lookup*()
/*	results.
/*
/*	dns_rr_filter_compile() initializes a result filter.  The
/*	title and map_names arguments are as with maps_create().
/*	This function may be invoked more than once; only the last
/*	filter takes effect.
/*
/*	dns_rr_filter_execute() converts each resource record in the
/*	specified list with dns_strrecord to ASCII form and matches
/*	that against the specified maps. If a match is found it
/*	executes the corresponding action.  Currently, only the
/*	"ignore" action is implemented. This removes the matched
/*	record from the list. The result is 0 in case of success,
/*	-1 in case of error.
/*
/*	dns_rr_filter_maps is updated by dns_rr_filter_compile().
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
  * System library.
  */
#include <sys_defs.h>
#include <ctype.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

 /*
  * Utility library.
  */
#include <msg.h>
#include <vstring.h>
#include <myaddrinfo.h>

 /*
  * Global library.
  */
#include <maps.h>

 /*
  * DNS library.
  */
#define LIBDNS_INTERNAL
#include <dns.h>

 /*
  * Application-specific.
  */
MAPS   *dns_rr_filter_maps;

static DNS_RR dns_rr_filter_error[1];

#define STR vstring_str

/* dns_rr_filter_compile - compile dns result filter */

void    dns_rr_filter_compile(const char *title, const char *map_names)
{
    if (dns_rr_filter_maps != 0)
	maps_free(dns_rr_filter_maps);
    dns_rr_filter_maps = maps_create(title, map_names,
				     DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
}

/* dns_rr_action - execute action from filter map */

static DNS_RR *dns_rr_action(const char *cmd, DNS_RR *rr, const char *rr_text)
{
    const char *cmd_args = cmd + strcspn(cmd, " \t");
    int     cmd_len = cmd_args - cmd;

    while (*cmd_args && ISSPACE(*cmd_args))
	cmd_args++;

#define STREQUAL(x,y,l) (strncasecmp((x), (y), (l)) == 0 && (y)[l] == 0)

    if (STREQUAL(cmd, "IGNORE", cmd_len)) {
	msg_info("ignoring DNS RR: %s", rr_text);
	return (0);
    } else {
	msg_warn("%s: unknown DNS filter action: \"%s\"", 
		 dns_rr_filter_maps->title, cmd);
	return (dns_rr_filter_error);
    }
    return (rr);
}

/* dns_rr_filter_execute - filter DNS lookup result */

int     dns_rr_filter_execute(DNS_RR **rrlist)
{
    static VSTRING *buf = 0;
    DNS_RR **rrp;
    DNS_RR *rr;
    const char *map_res;
    DNS_RR *act_res;

    /*
     * Convert the resource record to string form, then search the maps for a
     * matching action.
     */
    if (buf == 0)
	buf = vstring_alloc(100);
    for (rrp = rrlist; (rr = *rrp) != 0; /* see below */ ) {
	map_res = maps_find(dns_rr_filter_maps, dns_strrecord(buf, rr),
			    DICT_FLAG_NONE);
	if (map_res != 0) {
	    if ((act_res = dns_rr_action(map_res, rr, STR(buf))) == 0) {
		*rrp = rr->next;		/* do not advance in the list */
		rr->next = 0;
		dns_rr_free(rr);
		continue;
	    } else if (act_res == dns_rr_filter_error) {
		return (-1);
	    }
	} else if (dns_rr_filter_maps->error) {
	    return (-1);
	}
	rrp = &(rr->next);			/* do advance in the list */
    }
    return (0);
}
