/*	$NetBSD: transport.c,v 1.4 2022/10/08 16:12:50 christos Exp $	*/

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
/*	jail, and must be called before calling transport_lookup().
/*
/*	transport_lookup() finds the channel and nexthop for the given
/*	domain, and returns 1 if something was found.	Otherwise, 0
/*	is returned.
/* DIAGNOSTICS
/*	info->transport_path->error is non-zero when the lookup
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
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
#include <mail_addr_find.h>
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
				     | DICT_FLAG_NO_REGSUB
				     | DICT_FLAG_UTF8_REQUEST);
    tp->wildcard_channel = tp->wildcard_nexthop = 0;
    tp->wildcard_errno = 0;
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
    myfree((void *) tp);
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
	else if (strcmp(STR(channel), MAIL_SERVICE_ERROR) != 0
		 && strcmp(STR(channel), MAIL_SERVICE_RETRY) != 0)
	    vstring_strcpy(nexthop, rcpt_domain);
	else
	    vstring_strcpy(nexthop, "Address is undeliverable");
    }
}

/* parse_transport_entry - parse transport table entry */

static void parse_transport_entry(const char *value, const char *rcpt_domain,
				          VSTRING *channel, VSTRING *nexthop)
{
    char   *saved_value;
    const char *host;

#define FOUND		1
#define NOTFOUND	0

    /*
     * It would be great if we could specify a recipient address in the
     * lookup result. Unfortunately, we cannot simply run the result through
     * a parser that recognizes "transport:user@domain" because the lookup
     * result can have arbitrary content (especially in the case of the error
     * mailer).
     */
    saved_value = mystrdup(value);
    host = split_at(saved_value, ':');
    update_entry(saved_value, host ? host : "", rcpt_domain, channel, nexthop);
    myfree(saved_value);
}

/* transport_wildcard_init - (re) initialize wild-card lookup result */

static void transport_wildcard_init(TRANSPORT_INFO *tp)
{
    VSTRING *channel = vstring_alloc(10);
    VSTRING *nexthop = vstring_alloc(10);
    const char *value;

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

    if ((value = maps_find(tp->transport_path, WILDCARD, FULL)) != 0) {
	parse_transport_entry(value, "", channel, nexthop);
	tp->wildcard_errno = 0;
	tp->wildcard_channel = channel;
	tp->wildcard_nexthop = nexthop;
	if (msg_verbose)
	    msg_info("wildcard_{chan:hop}={%s:%s}",
		     vstring_str(channel), vstring_str(nexthop));
    } else {
	tp->wildcard_errno = tp->transport_path->error;
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
    char   *ratsign = 0;
    const char *value;

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
     * Look up the full and extension-stripped address, then match the domain
     * and subdomains. Try the external form before the backwards-compatible
     * internal form.
     */
#define LOOKUP_STRATEGY \
	(MA_FIND_FULL | MA_FIND_NOEXT | MA_FIND_DOMAIN | \
	(transport_match_parent_style == MATCH_FLAG_PARENT ? \
		MA_FIND_PDMS : MA_FIND_PDDMDS))

    if ((ratsign = strrchr(addr, '@')) == 0 || ratsign[1] == 0)
	msg_panic("transport_lookup: bad address: \"%s\"", addr);

    if ((value = mail_addr_find_strategy(tp->transport_path, addr, (char **) 0,
					 LOOKUP_STRATEGY)) != 0) {
	parse_transport_entry(value, rcpt_domain, channel, nexthop);
	return (FOUND);
    }
    if (tp->transport_path->error != 0)
	return (NOTFOUND);

    /*
     * Fall back to the wild-card entry.
     */
    if (tp->wildcard_errno || event_time() > tp->expire)
	transport_wildcard_init(tp);
    if (tp->wildcard_errno) {
	tp->transport_path->error = tp->wildcard_errno;
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

#ifdef TEST

 /*
  * Proof-of-concept test program. Read an address from stdin, and spit out
  * the lookup result.
  */

#include <string.h>

#include <mail_conf.h>
#include <vstream.h>
#include <vstring_vstream.h>

static NORETURN usage(const char *progname)
{
    msg_fatal("usage: %s [-v] database", progname);
}

int     main(int argc, char **argv)
{
    VSTRING *buffer = vstring_alloc(100);
    VSTRING *channel = vstring_alloc(100);
    VSTRING *nexthop = vstring_alloc(100);
    TRANSPORT_INFO *tp;
    char   *bp;
    char   *addr_field;
    char   *rcpt_domain;
    char   *expect_channel;
    char   *expect_nexthop;
    int     status;
    int     ch;
    int     errs = 0;

    /*
     * Parse JCL.
     */
    while ((ch = GETOPT(argc, argv, "v")) > 0) {
	switch (ch) {
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    usage(argv[0]);
	}
    }
    if (argc != optind + 1)
	usage(argv[0]);

    /*
     * Initialize.
     */
#define UPDATE(var, val) do { myfree(var); var = mystrdup(val); } while (0)

    mail_conf_read();				/* XXX eliminate dependency. */
    UPDATE(var_rcpt_delim, "+");
    UPDATE(var_mydomain, "localdomain");
    UPDATE(var_myorigin, "localhost.localdomain");
    UPDATE(var_mydest, "localhost.localdomain");

    tp = transport_pre_init("transport map", argv[optind]);
    transport_post_init(tp);

    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	bp = STR(buffer);

	/*
	 * Parse the input and expectations. XXX We can't expect empty
	 * fields, so require '-' instead.
	 */
	if ((addr_field = mystrtok(&bp, ":")) == 0)
	    msg_fatal("no address field");
	if ((rcpt_domain = strrchr(addr_field, '@')) == 0)
	    msg_fatal("no recipient domain");
	rcpt_domain += 1;
	expect_channel = mystrtok(&bp, ":");
	expect_nexthop = mystrtok(&bp, ":");
	if ((expect_channel != 0) != (expect_nexthop != 0))
	    msg_fatal("specify both channel and nexthop, or specify neither");
	if (expect_channel) {
	    if (strcmp(expect_channel, "-") == 0)
		*expect_channel = 0;
	    if (strcmp(expect_nexthop, "-") == 0)
		*expect_nexthop = 0;
	    vstring_strcpy(channel, "DEFAULT");
	    vstring_strcpy(nexthop, rcpt_domain);
	}
	if (mystrtok(&bp, ":") != 0)
	    msg_fatal("garbage after nexthop field");

	/*
	 * Lookups.
	 */
	status = transport_lookup(tp, addr_field, rcpt_domain,
				  channel, nexthop);

	/*
	 * Enforce expectations.
	 */
	if (expect_nexthop && status) {
	    vstream_printf("%s:%s -> %s:%s \n",
			   addr_field, rcpt_domain,
			   STR(channel), STR(nexthop));
	    vstream_fflush(VSTREAM_OUT);
	    if (strcmp(expect_channel, STR(channel)) != 0) {
		msg_warn("expect channel '%s' but got '%s'",
			 expect_channel, STR(channel));
		errs = 1;
	    }
	    if (strcmp(expect_nexthop, STR(nexthop)) != 0) {
		msg_warn("expect nexthop '%s' but got '%s'",
			 expect_nexthop, STR(nexthop));
		errs = 1;
	    }
	} else if (expect_nexthop && !status) {
	    vstream_printf("%s:%s -> %s\n", addr_field, rcpt_domain,
			   tp->transport_path->error ?
			   "(try again)" : "(not found)");
	    vstream_fflush(VSTREAM_OUT);
	    msg_warn("expect channel '%s' but got none", expect_channel);
	    msg_warn("expect nexthop '%s' but got none", expect_nexthop);
	    errs = 1;
	} else if (!status) {
	    vstream_printf("%s:%s -> %s\n", addr_field, rcpt_domain,
			   tp->transport_path->error ?
			   "(try again)" : "(not found)");
	}
    }
    transport_free(tp);
    vstring_free(nexthop);
    vstring_free(channel);
    vstring_free(buffer);
    exit(errs != 0);
}

#endif
