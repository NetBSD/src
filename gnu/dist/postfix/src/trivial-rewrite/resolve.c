/*++
/* NAME
/*	resolve 3
/* SUMMARY
/*	mail address resolver
/* SYNOPSIS
/*	#include "trivial-rewrite.h"
/*
/*	void	resolve_init(void)
/*
/*	void	resolve_proto(stream)
/*	VSTREAM	*stream;
/*
/*	void	resolve_addr(rule, addr, result)
/*	char	*rule;
/*	char	*addr;
/*	VSTRING *result;
/* DESCRIPTION
/*	This module implements the trivial address resolving engine.
/*	It distinguishes between local and remote mail, and optionally
/*	consults one or more transport tables that map a destination
/*	to a transport, nexthop pair.
/*
/*	resolve_init() initializes data structures that are private
/*	to this module. It should be called once before using the
/*	actual resolver routines.
/*
/*	resolve_proto() implements the client-server protocol:
/*	read one address in FQDN form, reply with a (transport,
/*	nexthop, internalized recipient) triple.
/*
/*	resolve_addr() gives direct access to the address resolving
/*	engine. It resolves an internalized address to a (transport,
/*	nexthop, internalized recipient) triple.
/* STANDARDS
/* DIAGNOSTICS
/*	Problems and transactions are logged to the syslog daemon.
/* BUGS
/* SEE ALSO
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
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <split_at.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <mail_addr.h>
#include <rewrite_clnt.h>
#include <resolve_local.h>
#include <mail_conf.h>
#include <quote_822_local.h>
#include <tok822.h>

/* Application-specific. */

#include "trivial-rewrite.h"
#include "transport.h"

#define STR	vstring_str

/* resolve_addr - resolve address according to rule set */

void    resolve_addr(char *addr, VSTRING *channel, VSTRING *nexthop,
		             VSTRING *nextrcpt, int *flags)
{
    char   *myname = "resolve_addr";
    VSTRING *addr_buf = vstring_alloc(100);
    TOK822 *tree;
    TOK822 *saved_domain = 0;
    TOK822 *domain = 0;
    char   *destination;

    *flags = 0;

    /*
     * The address is in internalized (unquoted) form, so we must externalize
     * it first before we can parse it.
     */
    quote_822_local(addr_buf, addr);
    tree = tok822_scan_addr(vstring_str(addr_buf));

    /*
     * Preliminary resolver: strip off all instances of the local domain.
     * Terminate when no destination domain is left over, or when the
     * destination domain is remote.
     */
#define RESOLVE_LOCAL(domain) \
    resolve_local(STR(tok822_internalize(addr_buf, domain, TOK822_STR_DEFL)))

    while (tree->head) {

	/*
	 * Strip trailing dot or @.
	 */
	if (tree->tail->type == '.' || tree->tail->type == '@') {
	    tok822_free_tree(tok822_sub_keep_before(tree, tree->tail));
	    continue;
	}

	/*
	 * A lone empty string becomes the postmaster.
	 */
	if (tree->head == tree->tail && tree->head->type == TOK822_QSTRING
	    && VSTRING_LEN(tree->head->vstr) == 0) {
	    tok822_free(tree->head);
	    tree->head = tok822_scan(MAIL_ADDR_POSTMASTER, &tree->tail);
	    rewrite_tree(REWRITE_CANON, tree);
	}

	/*
	 * Strip (and save) @domain if local.
	 */
	if ((domain = tok822_rfind_type(tree->tail, '@')) != 0) {
	    if (RESOLVE_LOCAL(domain->next) == 0)
		break;
	    tok822_sub_keep_before(tree, domain);
	    if (saved_domain)
		tok822_free_tree(saved_domain);
	    saved_domain = domain;
	}

	/*
	 * After stripping the local domain, if any, replace foo%bar by
	 * foo@bar, site!user by user@site, rewrite to canonical form, and
	 * retry.
	 * 
	 * Otherwise we're done.
	 */
	if (tok822_rfind_type(tree->tail, '@')
	    || (var_swap_bangpath && tok822_rfind_type(tree->tail, '!'))
	    || (var_percent_hack && tok822_rfind_type(tree->tail, '%'))) {
	    rewrite_tree(REWRITE_CANON, tree);
	} else {
	    domain = 0;
	    break;
	}
    }

    /*
     * If the destination is non-local, recognize routing operators in the
     * address localpart. This is needed to prevent backup MX hosts from
     * relaying third-party destinations through primary MX hosts, otherwise
     * the backup host could end up on black lists. Ignore local
     * swap_bangpath and percent_hack settings because we can't know how the
     * primary MX host is set up.
     */
    if (domain && domain->prev)
	if (tok822_rfind_type(domain->prev, '@') != 0
	    || tok822_rfind_type(domain->prev, '!') != 0
	    || tok822_rfind_type(domain->prev, '%') != 0)
	    *flags |= RESOLVE_FLAG_ROUTED;

    /*
     * Make sure the resolved envelope recipient has the user@domain form. If
     * no domain was specified in the address, assume the local machine. See
     * above for what happens with an empty address.
     */
    if (domain == 0) {
	if (saved_domain) {
	    tok822_sub_append(tree, saved_domain);
	    saved_domain = 0;
	} else {
	    tok822_sub_append(tree, tok822_alloc('@', (char *) 0));
	    tok822_sub_append(tree, tok822_scan(var_myhostname, (TOK822 **) 0));
	}
    }
    tok822_internalize(nextrcpt, tree, TOK822_STR_DEFL);

    /*
     * The transport map overrides any transport and next-hop host info that
     * is set up below. For a long time, it was not possible to override
     * routing of mail that resolves locally, because Postfix used a
     * zero-length next-hop hostname result to indicate local delivery, and
     * transport maps cannot return zero-length hostnames.
     */
    if (*var_transport_maps
    && transport_lookup(strrchr(STR(nextrcpt), '@') + 1, channel, nexthop)) {
	 /* void */ ;
    }

    /*
     * Non-local delivery, presumably. Set up the default remote transport
     * specified with var_def_transport. Use the destination's mail exchanger
     * unless a default mail relay is specified with var_relayhost.
     */
    else if (domain != 0) {
	vstring_strcpy(channel, var_def_transport);
	if ((destination = split_at(STR(channel), ':')) != 0 && *destination)
	    vstring_strcpy(nexthop, destination);
	else if (*var_relayhost)
	    vstring_strcpy(nexthop, var_relayhost);
	else
	    tok822_internalize(nexthop, domain->next, TOK822_STR_DEFL);
	if (*STR(channel) == 0)
	    msg_fatal("null transport is not allowed: %s = %s",
		      VAR_DEF_TRANSPORT, var_def_transport);
    }

    /*
     * Local delivery. Set up the default local transport and the default
     * next-hop hostname (myself).
     */
    else {
	vstring_strcpy(channel, var_local_transport);
	if ((destination = split_at(STR(channel), ':')) == 0
	    || *destination == 0)
	    destination = var_myhostname;
	vstring_strcpy(nexthop, destination);
	if (*STR(channel) == 0)
	    msg_fatal("null transport is not allowed: %s = %s",
		      VAR_LOCAL_TRANSPORT, var_local_transport);
    }
    if (*STR(nexthop) == 0)
	msg_panic("%s: null nexthop", myname);

    /*
     * Clean up.
     */
    if (saved_domain)
	tok822_free_tree(saved_domain);
    tok822_free_tree(tree);
    vstring_free(addr_buf);
}

/* Static, so they can be used by the network protocol interface only. */

static VSTRING *channel;
static VSTRING *nexthop;
static VSTRING *nextrcpt;
static VSTRING *query;

/* resolve_proto - read request and send reply */

int     resolve_proto(VSTREAM *stream)
{
    int     flags;

    if (attr_scan(stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_ADDR, query,
		  ATTR_TYPE_END) != 1)
	return (-1);

    resolve_addr(STR(query), channel, nexthop, nextrcpt, &flags);

    if (msg_verbose)
	msg_info("%s -> (`%s' `%s' `%s' `%d')", STR(query), STR(channel),
		 STR(nexthop), STR(nextrcpt), flags);

    attr_print(stream, ATTR_FLAG_NONE,
	       ATTR_TYPE_STR, MAIL_ATTR_TRANSPORT, STR(channel),
	       ATTR_TYPE_STR, MAIL_ATTR_NEXTHOP, STR(nexthop),
	       ATTR_TYPE_STR, MAIL_ATTR_RECIP, STR(nextrcpt),
	       ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
	       ATTR_TYPE_END);

    if (vstream_fflush(stream) != 0) {
	msg_warn("write resolver reply: %m");
	return (-1);
    }
    return (0);
}

/* resolve_init - module initializations */

void    resolve_init(void)
{
    query = vstring_alloc(100);
    channel = vstring_alloc(100);
    nexthop = vstring_alloc(100);
    nextrcpt = vstring_alloc(100);
}
