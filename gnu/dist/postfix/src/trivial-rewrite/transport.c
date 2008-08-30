/*++
/* NAME
/*	transport 3
/* SUMMARY
/*	transport mapping
/* SYNOPSIS
/*	#include "transport.h"
/*
/*	TRANSPORT_INFO *transport_pre_init(maps_name, maps)
/*	const char *maps_name;
/*	const char *maps;
/*
/*	void	transport_post_init(info)
/*	TRANSPORT_INFO *info;
/*
/*	int	transport_lookup(info, address, rcpt_domain, channel, nexthop)
/*	TRANSPORT_INFO *info;
/*	const char *address;
/*	const char *rcpt_domain;
/*	VSTRING *channel;
/*	VSTRING *nexthop;
/*
/*	void	transport_free(info);
/*	TRANSPORT_INFO * info;
/* DESCRIPTION
/*	This module implements access to the table that maps transport
/*	user@domain addresses to (channel, nexthop) tuples.
/*
/*	transport_pre_init() performs initializations that should be
/*	done before the process enters the chroot jail, and
/*	before calling transport_lookup().
/*
/*	transport_post_init() can be invoked after entering the chroot
/*	jail, and must be called before before calling transport_lookup().
/*
/*	transport_lookup() finds the channel and nexthop for the given
/*	domain, and returns 1 if something was found.	Otherwise, 0
/*	is returned.
/* DIAGNOSTICS
/*	The global \fIdict_errno\fR is non-zero when the lookup
/*	should be tried again.
/* SEE ALSO
/*	maps(3), multi-dictionary search
/*	strip_addr(3), strip extension from address
/*	transport(5), format of transport map
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
#include <events.h>

/* Global library. */

#include <strip_addr.h>
#include <mail_params.h>
#include <maps.h>
#include <match_parent_style.h>
#include <mail_proto.h>

/* Application-specific. */

#include "transport.h"

static int transport_match_parent_style;

#define STR(x)	vstring_str(x)

static void transport_wildcard_init(TRANSPORT_INFO *);

/* transport_pre_init - pre-jail initialization */

TRANSPORT_INFO *transport_pre_init(const char *transport_maps_name,
				           const char *transport_maps)
{
    TRANSPORT_INFO *tp;

    tp = (TRANSPORT_INFO *) mymalloc(sizeof(*tp));
    tp->transport_path = maps_create(transport_maps_name, transport_maps,
				     DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
				     | DICT_FLAG_NO_REGSUB);
    tp->wildcard_channel = tp->wildcard_nexthop = 0;
    tp->transport_errno = 0;
    tp->expire = 0;
    return (tp);
}

/* transport_post_init - post-jail initialization */

void    transport_post_init(TRANSPORT_INFO *tp)
{
    transport_match_parent_style = match_parent_style(VAR_TRANSPORT_MAPS);
    transport_wildcard_init(tp);
}

/* transport_free - destroy transport info */

void    transport_free(TRANSPORT_INFO *tp)
{
    if (tp->transport_path)
	maps_free(tp->transport_path);
    if (tp->wildcard_channel)
	vstring_free(tp->wildcard_channel);
    if (tp->wildcard_nexthop)
	vstring_free(tp->wildcard_nexthop);
    myfree((char *) tp);
}

/* update_entry - update from transport table entry */

static void update_entry(const char *new_channel, const char *new_nexthop,
			         const char *rcpt_domain, VSTRING *channel,
			         VSTRING *nexthop)
{

    /*
     * :[nexthop] means don't change the channel, and don't change the
     * nexthop unless a non-default nexthop is specified. Thus, a right-hand
     * side of ":" is the transport table equivalent of a NOOP.
     */
    if (*new_channel == 0) {			/* :[nexthop] */
	if (*new_nexthop != 0)
	    vstring_strcpy(nexthop, new_nexthop);
    }

    /*
     * transport[:[nexthop]] means change the channel, and reset the nexthop
     * to the default unless a non-default nexthop is specified.
     */
    else {
	vstring_strcpy(channel, new_channel);
	if (*new_nexthop != 0)
	    vstring_strcpy(nexthop, new_nexthop);
	else if (strcmp(STR(channel), MAIL_SERVICE_ERROR) != 0)
	    vstring_strcpy(nexthop, rcpt_domain);
	else
	    vstring_strcpy(nexthop, "Address is undeliverable");
    }
}

/* find_transport_entry - look up and parse transport table entry */

static int find_transport_entry(TRANSPORT_INFO *tp, const char *key,
				        const char *rcpt_domain, int flags,
				        VSTRING *channel, VSTRING *nexthop)
{
    char   *saved_value;
    const char *host;
    const char *value;

    /*
     * Reset previous error history.
     */
    dict_errno = 0;

#define FOUND		1
#define NOTFOUND	0

    /*
     * Look up an entry with extreme prejudice.
     * 
     * XXX Should report lookup failure status to caller instead of aborting.
     */
    if ((value = maps_find(tp->transport_path, key, flags)) == 0)
	return (NOTFOUND);

    /*
     * It would be great if we could specify a recipient address in the
     * lookup result. Unfortunately, we cannot simply run the result through
     * a parser that recognizes "transport:user@domain" because the lookup
     * result can have arbitrary content (especially in the case of the error
     * mailer).
     */
    else {
	saved_value = mystrdup(value);
	host = split_at(saved_value, ':');
	update_entry(saved_value, host ? host : "", rcpt_domain,
		     channel, nexthop);
	myfree(saved_value);
	return (FOUND);
    }
}

/* transport_wildcard_init - (re) initialize wild-card lookup result */

static void transport_wildcard_init(TRANSPORT_INFO *tp)
{
    VSTRING *channel = vstring_alloc(10);
    VSTRING *nexthop = vstring_alloc(10);

    /*
     * Both channel and nexthop may be zero-length strings. Therefore we must
     * use something else to represent "wild-card does not exist". We use
     * null VSTRING pointers, for historical reasons.
     */
    if (tp->wildcard_channel)
	vstring_free(tp->wildcard_channel);
    if (tp->wildcard_nexthop)
	vstring_free(tp->wildcard_nexthop);

    /*
     * Technically, the wildcard lookup pattern is redundant. A static map
     * (keys always match, result is fixed string) could achieve the same:
     * 
     * transport_maps = hash:/etc/postfix/transport static:xxx:yyy
     * 
     * But the user interface of such an approach would be less intuitive. We
     * tolerate the continued existence of wildcard lookup patterns because
     * of human interface considerations.
     */
#define WILDCARD	"*"
#define FULL		0
#define PARTIAL		DICT_FLAG_FIXED

    if (find_transport_entry(tp, WILDCARD, "", FULL, channel, nexthop)) {
	tp->transport_errno = 0;
	tp->wildcard_channel = channel;
	tp->wildcard_nexthop = nexthop;
	if (msg_verbose)
	    msg_info("wildcard_{chan:hop}={%s:%s}",
		     vstring_str(channel), vstring_str(nexthop));
    } else {
	tp->transport_errno = dict_errno;
	vstring_free(channel);
	vstring_free(nexthop);
	tp->wildcard_channel = 0;
	tp->wildcard_nexthop = 0;
    }
    tp->expire = event_time() + 30;		/* XXX make configurable */
}

/* transport_lookup - map a transport domain */

int     transport_lookup(TRANSPORT_INFO *tp, const char *addr,
			         const char *rcpt_domain,
			         VSTRING *channel, VSTRING *nexthop)
{
    char   *stripped_addr;
    char   *ratsign = 0;
    const char *name;
    const char *next;
    int     found;

#define STREQ(x,y)	(strcmp((x), (y)) == 0)
#define DISCARD_EXTENSION ((char **) 0)

    /*
     * The null recipient is rewritten to the local mailer daemon address.
     */
    if (*addr == 0) {
	msg_warn("transport_lookup: null address - skipping table lookup");
	return (NOTFOUND);
    }

    /*
     * Look up the full address with the FULL flag to include regexp maps in
     * the query.
     */
    if ((ratsign = strrchr(addr, '@')) == 0 || ratsign[1] == 0)
	msg_panic("transport_lookup: bad address: \"%s\"", addr);

    if (find_transport_entry(tp, addr, rcpt_domain, FULL, channel, nexthop))
	return (FOUND);
    if (dict_errno != 0)
	return (NOTFOUND);

    /*
     * If the full address did not match, and there is an address extension,
     * look up the stripped address with the PARTIAL flag to avoid matching
     * partial lookup keys with regular expressions.
     */
    if ((stripped_addr = strip_addr(addr, DISCARD_EXTENSION,
				    *var_rcpt_delim)) != 0) {
	found = find_transport_entry(tp, stripped_addr, rcpt_domain, PARTIAL,
				     channel, nexthop);

	myfree(stripped_addr);
	if (found)
	    return (FOUND);
	if (dict_errno != 0)
	    return (NOTFOUND);
    }

    /*
     * If the full and stripped address lookup fails, try domain name lookup.
     * 
     * Keep stripping domain components until nothing is left or until a
     * matching entry is found.
     * 
     * After checking the full domain name, check for .upper.domain, to
     * distinguish between the parent domain and it's decendants, a la
     * sendmail and tcp wrappers.
     * 
     * Before changing the DB lookup result, make a copy first, in order to
     * avoid DB cache corruption.
     * 
     * Specify that the lookup key is partial, to avoid matching partial keys
     * with regular expressions.
     */
    for (name = ratsign + 1; *name != 0; name = next) {
	if (find_transport_entry(tp, name, rcpt_domain, PARTIAL, channel, nexthop))
	    return (FOUND);
	if (dict_errno != 0)
	    return (NOTFOUND);
	if ((next = strchr(name + 1, '.')) == 0)
	    break;
	if (transport_match_parent_style == MATCH_FLAG_PARENT)
	    next++;
    }

    /*
     * Fall back to the wild-card entry.
     */
    if (tp->transport_errno || event_time() > tp->expire)
	transport_wildcard_init(tp);
    if (tp->transport_errno) {
	dict_errno = tp->transport_errno;
	return (NOTFOUND);
    } else if (tp->wildcard_channel) {
	update_entry(STR(tp->wildcard_channel), STR(tp->wildcard_nexthop),
		     rcpt_domain, channel, nexthop);
	return (FOUND);
    }

    /*
     * We really did not find it.
     */
    return (NOTFOUND);
}
