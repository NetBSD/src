/*++
/* NAME
/*	transport 3
/* SUMMARY
/*	transport mapping
/* SYNOPSIS
/*	#include "transport.h"
/*
/*	void	transport_init()
/*
/*	int	transport_lookup(domain, channel, nexthop)
/*	const char *domain;
/*	VSTRING *channel;
/*	VSTRING *nexthop;
/* DESCRIPTION
/*	This module implements access to the table that maps transport
/*	domains to (channel, nexthop) tuples.
/*
/*	transport_init() performs initializations that should be
/*	done before the process enters the chroot jail, and
/*	before calling transport_lookup().
/*
/*	transport_lookup() finds the channel and nexthop for the given
/*	domain, and returns 1 if something was found.	Otherwise, 0
/*	is returned.
/* DIAGNOSTICS
/*	The global \fIdict_errno\fR is non-zero when the lookup
/*	should be tried again.
/* SEE ALSO
/*	maps(3), multi-dictionary search
/*	transport(5), format of transport map
/* FILES
/*	/etc/postfix/transport*
/* CONFIGURATION PARAMETERS
/*	transport_maps, names of maps to be searched.
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
#include <stringops.h>
#include <mymalloc.h>
#include <vstring.h>
#include <split_at.h>
#include <dict.h>

/* Global library. */

#include <mail_params.h>
#include <maps.h>

/* Application-specific. */

#include "transport.h"

static MAPS *transport_path;

/* transport_init - pre-jail initialization */

void    transport_init(void)
{
    if (transport_path)
	msg_panic("transport_init: repeated call");
    transport_path = maps_create("transport", var_transport_maps,
				 DICT_FLAG_LOCK);
}

/* transport_lookup - map a transport domain */

int     transport_lookup(const char *domain, VSTRING *channel, VSTRING *nexthop)
{
    char   *low_domain = lowercase(mystrdup(domain));
    const char *name;
    const char *value;
    const char *host;
    char   *saved_value;
    char   *transport;
    int     found = 0;

#define FULL	0
#define PARTIAL		DICT_FLAG_FIXED

    int     maps_flag = FULL;

    if (transport_path == 0)
	msg_panic("transport_lookup: missing initialization");

    /*
     * Keep stripping domain components until nothing is left or until a
     * matching entry is found.
     * 
     * After checking the full name, check for .upper.domain, to distinguish
     * between the upper domain and it's decendants, ala sendmail and tcp
     * wrappers.
     * 
     * Before changing the DB lookup result, make a copy first, in order to
     * avoid DB cache corruption.
     * 
     * Specify if a key is partial or full, to avoid matching partial keys with
     * regular expressions.
     */
    for (name = low_domain; name != 0; name = strchr(name + 1, '.')) {
	if ((value = maps_find(transport_path, name, maps_flag)) != 0) {
	    saved_value = mystrdup(value);
	    if ((host = split_at(saved_value, ':')) == 0 || *host == 0)
		host = domain;
	    if (*(transport = saved_value) == 0)
		transport = var_def_transport;
	    vstring_strcpy(channel, transport);
	    (void) split_at(vstring_str(channel), ':');
	    if (*vstring_str(channel) == 0)
		msg_fatal("null transport is not allowed: %s = %s",
			  VAR_DEF_TRANSPORT, var_def_transport);
	    vstring_strcpy(nexthop, host);
	    myfree(saved_value);
	    found = 1;
	    break;
	} else if (dict_errno != 0) {
	    msg_fatal("transport table lookup problem");
	}
	maps_flag = PARTIAL;
    }
    myfree(low_domain);
    return (found);
}
