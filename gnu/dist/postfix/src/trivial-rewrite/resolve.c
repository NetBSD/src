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
/*	void	resolve_proto(context, stream)
/*	RES_CONTEXT *context;
/*	VSTREAM	*stream;
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <split_at.h>
#include <valid_hostname.h>
#include <stringops.h>
#include <mymalloc.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <resolve_local.h>
#include <mail_conf.h>
#include <quote_822_local.h>
#include <tok822.h>
#include <domain_list.h>
#include <string_list.h>
#include <match_parent_style.h>
#include <maps.h>
#include <mail_addr_find.h>
#include <valid_mailhost_addr.h>

/* Application-specific. */

#include "trivial-rewrite.h"
#include "transport.h"

 /*
  * The job of the address resolver is to map one recipient address to a
  * triple of (channel, nexthop, recipient). The channel is the name of the
  * delivery service specified in master.cf, the nexthop is (usually) a
  * description of the next host to deliver to, and recipient is the final
  * recipient address. The latter may differ from the input address as the
  * result of stripping multiple layers of sender-specified routing.
  * 
  * Addresses are resolved by their domain name. Known domain names are
  * categorized into classes: local, virtual alias, virtual mailbox, relay,
  * and everything else. Finding the address domain class is a matter of
  * table lookups.
  * 
  * Different address domain classes generally use different delivery channels,
  * and may use class dependent ways to arrive at the corresponding nexthop
  * information. With classes that do final delivery, the nexthop is
  * typically the local machine hostname.
  * 
  * The transport lookup table provides a means to override the domain class
  * channel and/or nexhop information for specific recipients or for entire
  * domain hierarchies.
  * 
  * This works well in the general case. The only bug in this approach is that
  * the structure of the nexthop information is transport dependent.
  * Typically, the nexthop specifies a hostname, hostname + TCP Port, or the
  * pathname of a UNIX-domain socket. However, with the error transport the
  * nexthop field contains free text with the reason for non-delivery.
  * 
  * Therefore, a transport map entry that overrides the channel but not the
  * nexthop information (or vice versa) may produce surprising results. In
  * particular, the free text nexthop information for the error transport is
  * likely to confuse regular delivery agents; and conversely, a hostname or
  * socket pathname is not an adequate text as reason for non-delivery.
  * 
  * In the code below, rcpt_domain specifies the domain name that we will use
  * when the transport table specifies a non-default channel but no nexthop
  * information (we use a generic text when that non-default channel is the
  * error transport).
  */

#define STR	vstring_str

 /*
  * Some of the lists that define the address domain classes.
  */
static DOMAIN_LIST *relay_domains;
static STRING_LIST *virt_alias_doms;
static STRING_LIST *virt_mailbox_doms;

static MAPS *relocated_maps;

/* resolve_addr - resolve address according to rule set */

static void resolve_addr(RES_CONTEXT *rp, char *sender, char *addr,
			         VSTRING *channel, VSTRING *nexthop,
			         VSTRING *nextrcpt, int *flags)
{
    const char *myname = "resolve_addr";
    VSTRING *addr_buf = vstring_alloc(100);
    TOK822 *tree = 0;
    TOK822 *saved_domain = 0;
    TOK822 *domain = 0;
    char   *destination;
    const char *blame = 0;
    const char *rcpt_domain;
    ssize_t addr_len;
    ssize_t loop_count;
    ssize_t loop_max;
    char   *local;
    char   *oper;
    char   *junk;
    const char *relay;

    *flags = 0;
    vstring_strcpy(channel, "CHANNEL NOT UPDATED");
    vstring_strcpy(nexthop, "NEXTHOP NOT UPDATED");
    vstring_strcpy(nextrcpt, "NEXTRCPT NOT UPDATED");

    /*
     * The address is in internalized (unquoted) form.
     * 
     * In an ideal world we would parse the externalized address form as given
     * to us by the sender.
     * 
     * However, in the real world we have to look for routing characters like
     * %@! in the address local-part, even when that information is quoted
     * due to the presence of special characters or whitespace. Although
     * technically incorrect, this is needed to stop user@domain@domain relay
     * attempts when forwarding mail to a Sendmail MX host.
     * 
     * This suggests that we parse the address in internalized (unquoted) form.
     * Unfortunately, if we do that, the unparser generates incorrect white
     * space between adjacent non-operator tokens. Example: ``first last''
     * needs white space, but ``stuff[stuff]'' does not. This is is not a
     * problem when unparsing the result from parsing externalized forms,
     * because the parser/unparser were designed for valid externalized forms
     * where ``stuff[stuff]'' does not happen.
     * 
     * As a workaround we start with the quoted form and then dequote the
     * local-part only where needed. This will do the right thing in most
     * (but not all) cases.
     */
    addr_len = strlen(addr);
    quote_822_local(addr_buf, addr);
    tree = tok822_scan_addr(vstring_str(addr_buf));

    /*
     * The optimizer will eliminate tests that always fail, and will replace
     * multiple expansions of this macro by a GOTO to a single instance.
     */
#define FREE_MEMORY_AND_RETURN { \
	if (saved_domain) \
	    tok822_free_tree(saved_domain); \
	if(tree) \
	    tok822_free_tree(tree); \
	if (addr_buf) \
	    vstring_free(addr_buf); \
	return; \
    }

    /*
     * Preliminary resolver: strip off all instances of the local domain.
     * Terminate when no destination domain is left over, or when the
     * destination domain is remote.
     * 
     * XXX To whom it may concern. If you change the resolver loop below, or
     * quote_822_local.c, or tok822_parse.c, be sure to re-run the tests
     * under "make resolve_clnt_test" in the global directory.
     */
#define RESOLVE_LOCAL(domain) \
    resolve_local(STR(tok822_internalize(addr_buf, domain, TOK822_STR_DEFL)))

    dict_errno = 0;

    for (loop_count = 0, loop_max = addr_len + 100; /* void */ ; loop_count++) {

	/*
	 * Grr. resolve_local() table lookups may fail. It may be OK for
	 * local file lookup code to abort upon failure, but with
	 * network-based tables it is preferable to return an error
	 * indication to the requestor.
	 */
	if (dict_errno) {
	    *flags |= RESOLVE_FLAG_FAIL;
	    FREE_MEMORY_AND_RETURN;
	}

	/*
	 * XXX Should never happen, but if this happens with some
	 * pathological address, then that is not sufficient reason to
	 * disrupt the operation of an MTA.
	 */
	if (loop_count > loop_max) {
	    msg_warn("resolve_addr: <%s>: giving up after %ld iterations",
		     addr, (long) loop_count);
	    break;
	}

	/*
	 * Strip trailing dot at end of domain, but not dot-dot or at-dot.
	 * This merely makes diagnostics more accurate by leaving bogus
	 * addresses alone.
	 */
	if (tree->tail
	    && tree->tail->type == '.'
	    && tok822_rfind_type(tree->tail, '@') != 0
	    && tree->tail->prev->type != '.'
	    && tree->tail->prev->type != '@')
	    tok822_free_tree(tok822_sub_keep_before(tree, tree->tail));

	/*
	 * Strip trailing @.
	 */
	if (var_resolve_nulldom
	    && tree->tail
	    && tree->tail->type == '@')
	    tok822_free_tree(tok822_sub_keep_before(tree, tree->tail));

	/*
	 * Strip (and save) @domain if local.
	 */
	if ((domain = tok822_rfind_type(tree->tail, '@')) != 0) {
	    if (domain->next && RESOLVE_LOCAL(domain->next) == 0)
		break;
	    tok822_sub_keep_before(tree, domain);
	    if (saved_domain)
		tok822_free_tree(saved_domain);
	    saved_domain = domain;
	    domain = 0;				/* safety for future change */
	}

	/*
	 * After stripping the local domain, if any, replace foo%bar by
	 * foo@bar, site!user by user@site, rewrite to canonical form, and
	 * retry.
	 */
	if (tok822_rfind_type(tree->tail, '@')
	    || (var_swap_bangpath && tok822_rfind_type(tree->tail, '!'))
	    || (var_percent_hack && tok822_rfind_type(tree->tail, '%'))) {
	    rewrite_tree(&local_context, tree);
	    continue;
	}

	/*
	 * If the local-part is a quoted string, crack it open when we're
	 * permitted to do so and look for routing operators. This is
	 * technically incorrect, but is needed to stop relaying problems.
	 * 
	 * XXX Do another feeble attempt to keep local-part info quoted.
	 */
	if (var_resolve_dequoted
	    && tree->head && tree->head == tree->tail
	    && tree->head->type == TOK822_QSTRING
	    && ((oper = strrchr(local = STR(tree->head->vstr), '@')) != 0
		|| (var_percent_hack && (oper = strrchr(local, '%')) != 0)
	     || (var_swap_bangpath && (oper = strrchr(local, '!')) != 0))) {
	    if (*oper == '%')
		*oper = '@';
	    tok822_internalize(addr_buf, tree->head, TOK822_STR_DEFL);
	    if (*oper == '@') {
		junk = mystrdup(STR(addr_buf));
		quote_822_local(addr_buf, junk);
		myfree(junk);
	    }
	    tok822_free(tree->head);
	    tree->head = tok822_scan(STR(addr_buf), &tree->tail);
	    rewrite_tree(&local_context, tree);
	    continue;
	}

	/*
	 * An empty local-part or an empty quoted string local-part becomes
	 * the local MAILER-DAEMON, for consistency with our own From:
	 * message headers.
	 */
	if (tree->head && tree->head == tree->tail
	    && tree->head->type == TOK822_QSTRING
	    && VSTRING_LEN(tree->head->vstr) == 0) {
	    tok822_free(tree->head);
	    tree->head = 0;
	}
	/* XXX must be localpart only, not user@domain form. */
	if (tree->head == 0)
	    tree->head = tok822_scan(var_empty_addr, &tree->tail);

	/*
	 * We're done. There are no domains left to strip off the address,
	 * and all null local-part information is sanitized.
	 */
	domain = 0;
	break;
    }

    vstring_free(addr_buf);
    addr_buf = 0;

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

    /*
     * Transform the recipient address back to internal form.
     * 
     * XXX This may produce incorrect results if we cracked open a quoted
     * local-part with routing operators; see discussion above at the top of
     * the big loop.
     * 
     * XXX We explicitly disallow domain names in bare network address form. A
     * network address destination should be formatted according to RFC 2821:
     * it should be enclosed in [], and an IPv6 address should have an IPv6:
     * prefix.
     */
    tok822_internalize(nextrcpt, tree, TOK822_STR_DEFL);
    rcpt_domain = strrchr(STR(nextrcpt), '@') + 1;
    if (rcpt_domain == 0)
	msg_panic("no @ in address: \"%s\"", STR(nextrcpt));
    if (*rcpt_domain == '[') {
	if (!valid_mailhost_literal(rcpt_domain, DONT_GRIPE))
	    *flags |= RESOLVE_FLAG_ERROR;
    } else if (!valid_hostname(rcpt_domain, DONT_GRIPE)) {
	if (var_resolve_num_dom && valid_hostaddr(rcpt_domain, DONT_GRIPE)) {
	    vstring_insert(nextrcpt, rcpt_domain - STR(nextrcpt), "[", 1);
	    vstring_strcat(nextrcpt, "]");
	    rcpt_domain = strrchr(STR(nextrcpt), '@') + 1;
	    if (resolve_local(rcpt_domain))	/* XXX */
		domain = 0;
	} else {
	    *flags |= RESOLVE_FLAG_ERROR;
	}
    }
    tok822_free_tree(tree);
    tree = 0;

    /*
     * XXX Short-cut invalid address forms.
     */
    if (*flags & RESOLVE_FLAG_ERROR) {
	*flags |= RESOLVE_CLASS_DEFAULT;
	FREE_MEMORY_AND_RETURN;
    }

    /*
     * Recognize routing operators in the local-part, even when we do not
     * recognize ! or % as valid routing operators locally. This is needed to
     * prevent backup MX hosts from relaying third-party destinations through
     * primary MX hosts, otherwise the backup host could end up on black
     * lists. Ignore local swap_bangpath and percent_hack settings because we
     * can't know how the next MX host is set up.
     */
    if (strcmp(STR(nextrcpt) + strcspn(STR(nextrcpt), "@!%") + 1, rcpt_domain))
	*flags |= RESOLVE_FLAG_ROUTED;

    /*
     * With local, virtual, relay, or other non-local destinations, give the
     * highest precedence to transport associated nexthop information.
     * 
     * Otherwise, with relay or other non-local destinations, the relayhost
     * setting overrides the recipient domain name, and the sender-dependent
     * relayhost overrides both.
     * 
     * XXX Nag if the recipient domain is listed in multiple domain lists. The
     * result is implementation defined, and may break when internals change.
     * 
     * For now, we distinguish only a fixed number of address classes.
     * Eventually this may become extensible, so that new classes can be
     * configured with their own domain list, delivery transport, and
     * recipient table.
     */
#define STREQ(x,y) (strcmp((x), (y)) == 0)

    dict_errno = 0;
    if (domain != 0) {

	/*
	 * Virtual alias domain.
	 */
	if (virt_alias_doms
	    && string_list_match(virt_alias_doms, rcpt_domain)) {
	    if (var_helpful_warnings) {
		if (virt_mailbox_doms
		    && string_list_match(virt_mailbox_doms, rcpt_domain))
		    msg_warn("do not list domain %s in BOTH %s and %s",
			     rcpt_domain, VAR_VIRT_ALIAS_DOMS,
			     VAR_VIRT_MAILBOX_DOMS);
		if (relay_domains
		    && domain_list_match(relay_domains, rcpt_domain))
		    msg_warn("do not list domain %s in BOTH %s and %s",
			     rcpt_domain, VAR_VIRT_ALIAS_DOMS,
			     VAR_RELAY_DOMAINS);
#if 0
		if (strcasecmp(rcpt_domain, var_myorigin) == 0)
		    msg_warn("do not list $%s (%s) in %s",
			   VAR_MYORIGIN, var_myorigin, VAR_VIRT_ALIAS_DOMS);
#endif
	    }
	    vstring_strcpy(channel, MAIL_SERVICE_ERROR);
	    vstring_sprintf(nexthop, "User unknown%s",
			    var_show_unk_rcpt_table ?
			    " in virtual alias table" : "");
	    *flags |= RESOLVE_CLASS_ALIAS;
	} else if (dict_errno != 0) {
	    msg_warn("%s lookup failure", VAR_VIRT_ALIAS_DOMS);
	    *flags |= RESOLVE_FLAG_FAIL;
	    FREE_MEMORY_AND_RETURN;
	}

	/*
	 * Virtual mailbox domain.
	 */
	else if (virt_mailbox_doms
		 && string_list_match(virt_mailbox_doms, rcpt_domain)) {
	    if (var_helpful_warnings) {
		if (relay_domains
		    && domain_list_match(relay_domains, rcpt_domain))
		    msg_warn("do not list domain %s in BOTH %s and %s",
			     rcpt_domain, VAR_VIRT_MAILBOX_DOMS,
			     VAR_RELAY_DOMAINS);
	    }
	    vstring_strcpy(channel, RES_PARAM_VALUE(rp->virt_transport));
	    vstring_strcpy(nexthop, rcpt_domain);
	    blame = rp->virt_transport_name;
	    *flags |= RESOLVE_CLASS_VIRTUAL;
	} else if (dict_errno != 0) {
	    msg_warn("%s lookup failure", VAR_VIRT_MAILBOX_DOMS);
	    *flags |= RESOLVE_FLAG_FAIL;
	    FREE_MEMORY_AND_RETURN;
	} else {

	    /*
	     * Off-host relay destination.
	     */
	    if (relay_domains
		&& domain_list_match(relay_domains, rcpt_domain)) {
		vstring_strcpy(channel, RES_PARAM_VALUE(rp->relay_transport));
		blame = rp->relay_transport_name;
		*flags |= RESOLVE_CLASS_RELAY;
	    } else if (dict_errno != 0) {
		msg_warn("%s lookup failure", VAR_RELAY_DOMAINS);
		*flags |= RESOLVE_FLAG_FAIL;
		FREE_MEMORY_AND_RETURN;
	    }

	    /*
	     * Other off-host destination.
	     */
	    else {
		vstring_strcpy(channel, RES_PARAM_VALUE(rp->def_transport));
		blame = rp->def_transport_name;
		*flags |= RESOLVE_CLASS_DEFAULT;
	    }

	    /*
	     * With off-host delivery, sender-dependent or global relayhost
	     * override the recipient domain.
	     */
	    if (rp->snd_relay_info
		&& (relay = mail_addr_find(rp->snd_relay_info, *sender ?
					   sender : var_null_relay_maps_key,
					   (char **) 0)) != 0)
		vstring_strcpy(nexthop, relay);
	    else if (*RES_PARAM_VALUE(rp->relayhost))
		vstring_strcpy(nexthop, RES_PARAM_VALUE(rp->relayhost));
	    else
		vstring_strcpy(nexthop, rcpt_domain);
	}
    }

    /*
     * Local delivery.
     * 
     * XXX Nag if the domain is listed in multiple domain lists. The effect is
     * implementation defined, and may break when internals change.
     */
    else {
	if (var_helpful_warnings) {
	    if (virt_alias_doms
		&& string_list_match(virt_alias_doms, rcpt_domain))
		msg_warn("do not list domain %s in BOTH %s and %s",
			 rcpt_domain, VAR_MYDEST, VAR_VIRT_ALIAS_DOMS);
	    if (virt_mailbox_doms
		&& string_list_match(virt_mailbox_doms, rcpt_domain))
		msg_warn("do not list domain %s in BOTH %s and %s",
			 rcpt_domain, VAR_MYDEST, VAR_VIRT_MAILBOX_DOMS);
	}
	vstring_strcpy(channel, RES_PARAM_VALUE(rp->local_transport));
	vstring_strcpy(nexthop, rcpt_domain);
	blame = rp->local_transport_name;
	*flags |= RESOLVE_CLASS_LOCAL;
    }

    /*
     * An explicit main.cf transport:nexthop setting overrides the nexthop.
     * 
     * XXX We depend on this mechanism to enforce per-recipient concurrencies
     * for local recipients. With "local_transport = local:$myhostname" we
     * force mail for any domain in $mydestination/${proxy,inet}_interfaces
     * to share the same queue.
     */
    if ((destination = split_at(STR(channel), ':')) != 0 && *destination)
	vstring_strcpy(nexthop, destination);

    /*
     * Sanity checks.
     */
    if (*STR(channel) == 0) {
	if (blame == 0)
	    msg_panic("%s: null blame", myname);
	msg_warn("file %s/%s: parameter %s: null transport is not allowed",
		 var_config_dir, MAIN_CONF_FILE, blame);
	*flags |= RESOLVE_FLAG_FAIL;
	FREE_MEMORY_AND_RETURN;
    }
    if (*STR(nexthop) == 0)
	msg_panic("%s: null nexthop", myname);

    /*
     * The transport map can selectively override any transport and/or
     * nexthop host info that is set up above. Unfortunately, the syntax for
     * nexthop information is transport specific. We therefore need sane and
     * intuitive semantics for transport map entries that specify a channel
     * but no nexthop.
     * 
     * With non-error transports, the initial nexthop information is the
     * recipient domain. However, specific main.cf transport definitions may
     * specify a transport-specific destination, such as a host + TCP socket,
     * or the pathname of a UNIX-domain socket. With less precedence than
     * main.cf transport definitions, a main.cf relayhost definition may also
     * override nexthop information for off-host deliveries.
     * 
     * With the error transport, the nexthop information is free text that
     * specifies the reason for non-delivery.
     * 
     * Because nexthop syntax is transport specific we reset the nexthop
     * information to the recipient domain when the transport table specifies
     * a transport without also specifying the nexthop information.
     * 
     * Subtle note: reset nexthop even when the transport table does not change
     * the transport. Otherwise it is hard to get rid of main.cf specified
     * nexthop information.
     * 
     * XXX Don't override the virtual alias class (error:User unknown) result.
     */
    if (rp->transport_info && !(*flags & RESOLVE_CLASS_ALIAS)) {
	if (transport_lookup(rp->transport_info, STR(nextrcpt),
			     rcpt_domain, channel, nexthop) == 0
	    && dict_errno != 0) {
	    msg_warn("%s lookup failure", rp->transport_maps_name);
	    *flags |= RESOLVE_FLAG_FAIL;
	    FREE_MEMORY_AND_RETURN;
	}
    }

    /*
     * Bounce recipients that have moved, regardless of domain address class.
     * We do this last, in anticipation of transport maps that can override
     * the recipient address.
     * 
     * The downside of not doing this in delivery agents is that this table has
     * no effect on local alias expansion results. Such mail will have to
     * make almost an entire iteration through the mail system.
     */
#define IGNORE_ADDR_EXTENSION   ((char **) 0)

    if (relocated_maps != 0) {
	const char *newloc;

	if ((newloc = mail_addr_find(relocated_maps, STR(nextrcpt),
				     IGNORE_ADDR_EXTENSION)) != 0) {
	    vstring_strcpy(channel, MAIL_SERVICE_ERROR);
	    /* 5.1.6 is the closest match, but not perfect. */
	    vstring_sprintf(nexthop, "5.1.6 User has moved to %s", newloc);
	} else if (dict_errno != 0) {
	    msg_warn("%s lookup failure", VAR_RELOCATED_MAPS);
	    *flags |= RESOLVE_FLAG_FAIL;
	    FREE_MEMORY_AND_RETURN;
	}
    }

    /*
     * Bounce recipient addresses that start with `-'. External commands may
     * misinterpret such addresses as command-line options.
     * 
     * In theory I could say people should always carefully set up their
     * master.cf pipe mailer entries with `--' before the first non-option
     * argument, but mistakes will happen regardless.
     * 
     * Therefore the protection is put in place here, where it cannot be
     * bypassed.
     */
    if (var_allow_min_user == 0 && STR(nextrcpt)[0] == '-') {
	*flags |= RESOLVE_FLAG_ERROR;
	FREE_MEMORY_AND_RETURN;
    }

    /*
     * Clean up.
     */
    FREE_MEMORY_AND_RETURN;
}

/* Static, so they can be used by the network protocol interface only. */

static VSTRING *channel;
static VSTRING *nexthop;
static VSTRING *nextrcpt;
static VSTRING *query;
static VSTRING *sender;

/* resolve_proto - read request and send reply */

int     resolve_proto(RES_CONTEXT *context, VSTREAM *stream)
{
    int     flags;

    if (attr_scan(stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
		  ATTR_TYPE_STR, MAIL_ATTR_ADDR, query,
		  ATTR_TYPE_END) != 2)
	return (-1);

    resolve_addr(context, STR(sender), STR(query),
		 channel, nexthop, nextrcpt, &flags);

    if (msg_verbose)
	msg_info("`%s' -> `%s' -> (`%s' `%s' `%s' `%d')",
		 STR(sender), STR(query), STR(channel),
		 STR(nexthop), STR(nextrcpt), flags);

    attr_print(stream, ATTR_FLAG_NONE,
	       ATTR_TYPE_INT, MAIL_ATTR_FLAGS, server_flags,
	       ATTR_TYPE_STR, MAIL_ATTR_TRANSPORT, STR(channel),
	       ATTR_TYPE_STR, MAIL_ATTR_NEXTHOP, STR(nexthop),
	       ATTR_TYPE_STR, MAIL_ATTR_RECIP, STR(nextrcpt),
	       ATTR_TYPE_INT, MAIL_ATTR_FLAGS, flags,
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
    sender = vstring_alloc(100);
    query = vstring_alloc(100);
    channel = vstring_alloc(100);
    nexthop = vstring_alloc(100);
    nextrcpt = vstring_alloc(100);

    if (*var_virt_alias_doms)
	virt_alias_doms =
	    string_list_init(MATCH_FLAG_NONE, var_virt_alias_doms);

    if (*var_virt_mailbox_doms)
	virt_mailbox_doms =
	    string_list_init(MATCH_FLAG_NONE, var_virt_mailbox_doms);

    if (*var_relay_domains)
	relay_domains =
	    domain_list_init(match_parent_style(VAR_RELAY_DOMAINS),
			     var_relay_domains);

    if (*var_relocated_maps)
	relocated_maps =
	    maps_create(VAR_RELOCATED_MAPS, var_relocated_maps,
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
}
