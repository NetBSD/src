/*++
/* NAME
/*	smtpd_check 3
/* SUMMARY
/*	SMTP client request filtering
/* SYNOPSIS
/*	#include "smtpd.h"
/*	#include "smtpd_check.h"
/*
/*	void	smtpd_check_init()
/*
/*	char	*smtpd_check_client(state)
/*	SMTPD_STATE *state;
/*
/*	char	*smtpd_check_helo(state, helohost)
/*	SMTPD_STATE *state;
/*	char	*helohost;
/*
/*	char	*smtpd_check_mail(state, sender)
/*	SMTPD_STATE *state;
/*	char	*sender;
/*
/*	char	*smtpd_check_rcpt(state, recipient)
/*	SMTPD_STATE *state;
/*	char	*recipient;
/*
/*	char	*smtpd_check_rcptmap(state, recipient)
/*	SMTPD_STATE *state;
/*	char	*recipient;
/*
/*	char	*smtpd_check_etrn(state, destination)
/*	SMTPD_STATE *state;
/*	char	*destination;
/*
/*	char	*smtpd_check_data(state)
/*	SMTPD_STATE *state;
/* DESCRIPTION
/*	This module implements additional checks on SMTP client requests.
/*	A client request is validated in the context of the session state.
/*	The result is either an error response (including the numerical
/*	code) or the result is a null pointer in case of success.
/*
/*	smtpd_check_init() initializes. This function should be called
/*	once during the process life time.
/*
/*	Each of the following routines scrutinizes the argument passed to
/*	an SMTP command such as HELO, MAIL FROM, RCPT TO, or scrutinizes
/*	the initial client connection request.  The administrator can
/*	specify what restrictions apply.
/*
/*	Restrictions are specified via configuration parameters named
/*	\fIsmtpd_{client,helo,sender,recipient}_restrictions.\fR Each
/*	configuration parameter specifies a list of zero or more
/*	restrictions that are applied in the order as specified.
/*
/*	Restrictions that can appear in some or all restriction
/*	lists:
/* .IP reject
/* .IP defer
/* .IP permit
/*	Reject, defer or permit the request unconditionally. This is to be used
/*	at the end of a restriction list in order to make the default
/*	action explicit.
/* .IP reject_unknown_client
/*	Reject the request when the client hostname could not be found.
/*	The \fIunknown_client_reject_code\fR configuration parameter
/*	specifies the reject status code (default: 450).
/* .IP permit_mynetworks
/*	Allow the request when the client address matches the \fImynetworks\fR
/*	configuration parameter.
/* .IP maptype:mapname
/*	Meaning depends on context: client name/address, helo name, sender
/*	or recipient address.
/*	Perform a lookup in the specified access table. Reject the request
/*	when the lookup result is REJECT or when the result begins with a
/*	4xx or 5xx status code. Other numerical status codes are not
/*	permitted. DUNNO suppresses less precise searches in the same map, but
/*	neither allows nor denies the request. Allow the request otherwise.
/*	The \fIaccess_map_reject_code\fR configuration parameter specifies
/*	the reject status code (default: 554).
/* .IP "check_client_access maptype:mapname"
/*	Look up the client host name or any of its parent domains, or
/*	the client address or any network obtained by stripping octets
/*	from the address.
/* .IP "check_helo_access maptype:mapname"
/*	Look up the HELO/EHLO hostname or any of its parent domains.
/* .IP "check_sender_access maptype:mapname"
/*	Look up the resolved sender address, any parent domains of the
/*	resolved sender address domain, or the localpart@.
/* .IP "check_recipient_access maptype:mapname"
/*	Look up the resolved recipient address in the named access table,
/*	any parent domains of the recipient domain, and the localpart@.
/* .IP "check_recipient_maps"
/*	Reject recipients not listed as valid local, virtual or relay
/*	recipients.
/* .IP reject_rbl_client rbl.domain.tld
/*	Look up the reversed client network address in the specified
/*	real-time blackhole DNS zone.  The \fIrbl_reply_maps\fR configuration
/*	parameter is used to generate the template for the reject message.
/*	If it is not specified, or the rbl domain cannot be found, then a
/*	default template is used.  The \fImaps_rbl_reject_code\fR
/*	configuration parameter specifies the reject status code used in
/*	the default template (default: 554).
/* .IP reject_rhsbl_client rbl.domain.tld
/* .IP reject_rhsbl_sender rbl.domain.tld
/* .IP reject_rhsbl_recipient rbl.domain.tld
/*	Look up the client/sender/recipient domain name in the specified
/*	real-time blackhole DNS zone.  The \fIrbl_reply_maps\fR configuration
/*	parameter is used to generate the template for the reject message.
/*	If it is not specified, or the rbl domain cannot be found, then a
/*	default template is used.  The \fImaps_rbl_reject_code\fR
/*	configuration parameter specifies the reject status code used in
/*	the default template (default: 554).
/* .IP permit_naked_ip_address
/*	Permit the use of a naked IP address (without enclosing [])
/*	in HELO/EHLO commands.
/*	This violates the RFC. You must enable this for some popular
/*	PC mail clients.
/* .IP reject_non_fqdn_hostname
/* .IP reject_non_fqdn_sender
/* .IP reject_non_fqdn_recipient
/*	Require that the HELO, MAIL FROM or RCPT TO commands specify
/*	a fully-qualified domain name. The non_fqdn_reject_code parameter
/*	specifies the error code (default: 504).
/* .IP reject_invalid_hostname
/*	Reject the request when the HELO/EHLO hostname does not satisfy RFC
/*	requirements.  The underscore is considered a legal hostname character,
/*	and so is a trailing dot.
/*	The \fIinvalid_hostname_reject_code\fR configuration parameter
/*	specifies the reject status code (default:501).
/* .IP reject_unknown_hostname
/*	Reject the request when the HELO/EHLO hostname has no A or MX record.
/*	The \fIunknown_hostname_reject_code\fR configuration
/*	parameter specifies the reject status code (default: 450).
/* .IP reject_unknown_sender_domain
/*	Reject the request when the resolved sender address has no
/*	DNS A or MX record.
/*	The \fIunknown_address_reject_code\fR configuration parameter
/*	specifies the reject status code (default: 450).
/* .IP reject_unknown_recipient_domain
/*	Reject the request when the resolved recipient address has no
/*	DNS A or MX record.
/*	The \fIunknown_address_reject_code\fR configuration parameter
/*	specifies the reject status code (default: 450).
/* .IP permit_auth_destination
/*	Permit the request when the resolved recipient domain matches the
/*	\fIrelay_domains\fR configuration parameter or a subdomain thereof,
/*	or when the destination somehow resolves locally ($inet_interfaces,
/*	$mydestination, $virtual_alias_domains, or $virtual_mailbox_domains).
/* .IP reject_unauth_destination
/*	Reject the request when the resolved recipient domain does not match
/*	the \fIrelay_domains\fR configuration parameter or a subdomain
/*	thereof, and when the destination does not somehow resolve locally
/*	($inet_interfaces, $mydestination, $virtual_alias_domains, or
/*	$virtual_mailbox_domains).
/*	The \fIrelay_domains_reject_code\fR configuration parameter specifies
/*	the reject status code (default: 554).
/* .IP reject_unauth_pipelining
/*	Reject the request when the client has already sent the next request
/*	without being told that the server implements SMTP command pipelining.
/*	Reject the DATA command when the client sends message content before
/*	Postfix has sent the DATA command reply.
/* .IP permit_mx_backup
/*	Allow the request when all primary MX hosts for the recipient
/*	are in the networks specified with the $permit_mx_backup_networks
/*	configuration parameter, or when the local system is the final
/*	destination.
/* .IP restriction_classes
/*	Defines a list of parameter names, each parameter being a list
/*	of restrictions that can be used anywhere a restriction is legal.
/* .PP
/*	smtpd_check_client() validates the client host name or address.
/*	Relevant configuration parameters:
/* .IP smtpd_client_restrictions
/*	Restrictions on the names or addresses of clients that may connect
/*	to this SMTP server.
/* .PP
/*	smtpd_check_helo() validates the hostname provided with the
/*	HELO/EHLO commands. Relevant configuration parameters:
/* .IP smtpd_helo_restrictions
/*	Restrictions on the hostname that is sent with the HELO/EHLO
/*	command.
/* .PP
/*	smtpd_check_mail() validates the sender address provided with
/*	a MAIL FROM request. Relevant configuration parameters:
/* .IP smtpd_sender_restrictions
/*	Restrictions on the sender address that is sent with the MAIL FROM
/*	command.
/* .PP
/*	smtpd_check_rcpt() validates the recipient address provided
/*	with an RCPT TO request. Relevant configuration parameters:
/* .IP smtpd_recipient_restrictions
/*	Restrictions on the recipient address that is sent with the RCPT
/*	TO command.
/* .PP
/*	smtpd_check_rcptmap() validates the recipient address provided
/*	with an RCPT TO request and sets the rcptmap_checked flag.
/*	Relevant configuration parameters:
/* .IP local_recipients_map
/*	Tables of user names (not addresses) that exist in $mydestination.
/*	Mail for local users not in these tables is rejected.
/* .PP
/*	smtpd_check_etrn() validates the domain name provided with the
/*	ETRN command, and other client-provided information. Relevant
/*	configuration parameters:
/* .IP smtpd_etrn_restrictions
/*	Restrictions on the hostname that is sent with the HELO/EHLO
/*	command.
/* .PP
/*	smtpd_check_size() checks if a message with the given size can
/*	be received (zero means that the message size is unknown).  The
/*	message is rejected when:
/* .IP \(bu
/*	The message size exceeds the non-zero bound specified with the
/*	\fImessage_size_limit\fR configuration parameter. This is a
/*	permanent error.
/* .IP \(bu
/*	The message would cause the available queue file system space
/*	to drop below the bound specified with the \fImin_queue_free\fR
/*	configuration parameter. This is a temporary error.
/* .IP \(bu
/*	The message would use up more than half the available queue file
/*	system space. This is a temporary error.
/* .PP
/*	Arguments:
/* .IP name
/*	The client hostname, or \fIunknown\fR.
/* .IP addr
/*	The client address.
/* .IP helohost
/*	The hostname given with the HELO command.
/* .IP sender
/*	The sender address given with the MAIL FROM command.
/* .IP recipient
/*	The recipient address given with the RCPT TO or VRFY command.
/* .IP size
/*	The message size given with the MAIL FROM command (zero if unknown).
/* BUGS
/*	Policies like these should not be hard-coded in C, but should
/*	be user-programmable instead.
/* SEE ALSO
/*	namadr_list(3) host access control
/*	domain_list(3) domain access control
/*	fsspace(3) free file system space
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <netdb.h>
#include <setjmp.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <split_at.h>
#include <fsspace.h>
#include <stringops.h>
#include <valid_hostname.h>
#include <argv.h>
#include <mymalloc.h>
#include <dict.h>
#include <htable.h>
#include <ctable.h>
#include <mac_expand.h>

/* DNS library. */

#include <dns.h>

/* Global library. */

#include <string_list.h>
#include <namadr_list.h>
#include <domain_list.h>
#include <mail_params.h>
#include <canon_addr.h>
#include <resolve_clnt.h>
#include <mail_error.h>
#include <resolve_local.h>
#include <own_inet_addr.h>
#include <mail_conf.h>
#include <maps.h>
#include <mail_addr_find.h>
#include <match_parent_style.h>
#include <strip_addr.h>
#include <virtual8_maps.h>
#include <cleanup_user.h>
#include <record.h>
#include <rec_type.h>
#include <mail_proto.h>
#include <mail_addr.h>

/* Application-specific. */

#include "smtpd.h"
#include "smtpd_sasl_glue.h"
#include "smtpd_check.h"

 /*
  * Eject seat in case of parsing problems.
  */
static jmp_buf smtpd_check_buf;

 /*
  * Results of restrictions.
  */
#define SMTPD_CHECK_DUNNO	0	/* indifferent */
#define SMTPD_CHECK_OK		1	/* explicitly permit */
#define SMTPD_CHECK_REJECT	2	/* explicitly reject */

 /*
  * Intermediate results. These are static to avoid unnecessary stress on the
  * memory manager routines.
  */
static VSTRING *error_text;
static CTABLE *smtpd_resolve_cache;
static CTABLE *smtpd_rbl_cache;

 /*
  * Pre-opened SMTP recipient maps so we can reject mail for unknown users.
  * XXX This does not belong here and will eventually become part of the
  * trivial-rewrite resolver.
  */
static MAPS *local_rcpt_maps;
static MAPS *rcpt_canon_maps;
static MAPS *canonical_maps;
static MAPS *virt_alias_maps;
static MAPS *virt_mailbox_maps;
static MAPS *relay_rcpt_maps;

#ifdef TEST

static STRING_LIST *virt_alias_doms;
static STRING_LIST *virt_mailbox_doms;

#endif

 /*
  * Response templates for various rbl domains.
  */
static MAPS *rbl_reply_maps;

 /*
  * Pre-opened sender to login name mapping.
  */
static MAPS *smtpd_sender_login_maps;

 /*
  * Pre-opened access control lists.
  */
static DOMAIN_LIST *relay_domains;
static NAMADR_LIST *mynetworks;
static NAMADR_LIST *perm_mx_networks;

 /*
  * How to do parent domain wildcard matching, if any.
  */
static int access_parent_style;

 /*
  * Pre-parsed restriction lists.
  */
static ARGV *client_restrctions;
static ARGV *helo_restrctions;
static ARGV *mail_restrctions;
static ARGV *rcpt_restrctions;
static ARGV *etrn_restrctions;
static ARGV *data_restrctions;

static HTABLE *smtpd_rest_classes;

 /*
  * Pre-parsed expansion filter.
  */
static VSTRING *expand_filter;

 /*
  * The routine that recursively applies restrictions.
  */
static int generic_checks(SMTPD_STATE *, ARGV *, const char *, const char *, const char *);

 /*
  * Recipient table check.
  */
static int check_rcpt_maps(SMTPD_STATE *state, const char *recipient);

 /*
  * Reject context.
  */
#define SMTPD_NAME_CLIENT	"Client host"
#define SMTPD_NAME_HELO		"Helo command"
#define SMTPD_NAME_SENDER	"Sender address"
#define SMTPD_NAME_RECIPIENT	"Recipient address"
#define SMTPD_NAME_ETRN		"Etrn command"
#define SMTPD_NAME_DATA		"Data command"

 /*
  * YASLM.
  */
#define STR	vstring_str
#define CONST_STR(x)	((const char *) vstring_str(x))

 /*
  * If some decision can't be made due to a temporary error, then change
  * other decisions into deferrals.
  * 
  * XXX Deferrals can be postponed only with restrictions that are based on
  * client-specified information: this restricts their use to parameters
  * given in HELO, MAIL FROM, RCPT TO commands.
  * 
  * XXX Deferrals must not be postponed after client hostname lookup failure.
  * The reason is that the effect of access tables may depend on whether a
  * client hostname is available or not. Thus, the reject_unknown_client
  * restriction must defer immediately when lookup fails, otherwise incorrect
  * results happen with:
  * 
  * reject_unknown_client, hostname-based white-list, reject
  * 
  * XXX With warn_if_reject, don't raise the defer_if_permit flag when a
  * reject-style restriction fails. Instead, log the warning for the
  * resulting defer message.
  * 
  * XXX With warn_if_reject, do raise the defer_if_reject flag when a
  * permit-style restriction fails. Otherwise, we could reject legitimate
  * mail.
  */
static void PRINTFLIKE(3, 4) defer_if(SMTPD_DEFER *, int, const char *,...);

#define DEFER_IF_REJECT2(state, class, fmt, a1, a2) \
    defer_if(&(state)->defer_if_reject, (class), (fmt), (a1), (a2))
#define DEFER_IF_REJECT3(state, class, fmt, a1, a2, a3) \
    defer_if(&(state)->defer_if_reject, (class), (fmt), (a1), (a2), (a3))
#define DEFER_IF_PERMIT2(state, class, fmt, a1, a2) do { \
    if ((state)->warn_if_reject == 0) \
	defer_if(&(state)->defer_if_permit, (class), (fmt), (a1), (a2)); \
    else \
	(void) smtpd_check_reject((state), (class), (fmt), (a1), (a2)); \
    } while (0)

 /*
  * Cached RBL lookup state.
  */
typedef struct {
    char   *txt;			/* TXT record or "" */
} SMTPD_RBL_STATE;

static void *rbl_pagein(const char *, void *);
static void rbl_pageout(void *, void *);

 /*
  * Context for RBL $name expansion.
  */
typedef struct {
    SMTPD_STATE *state;			/* general state */
    SMTPD_RBL_STATE *rbl_state;		/* cached RBL state */
    const char *domain;			/* query domain */
    const char *what;			/* rejected value */
    const char *class;			/* name of rejected value */
} SMTPD_RBL_EXPAND_CONTEXT;

/* resolve_pagein - page in an address resolver result */

static void *resolve_pagein(const char *addr, void *unused_context)
{
    static VSTRING *query;
    RESOLVE_REPLY *reply;

    /*
     * Initialize on the fly.
     */
    if (query == 0)
	query = vstring_alloc(10);

    /*
     * Initialize.
     */
    reply = (RESOLVE_REPLY *) mymalloc(sizeof(*reply));
    resolve_clnt_init(reply);

    /*
     * Resolve the address.
     */
    canon_addr_internal(query, addr);
    resolve_clnt_query(STR(query), reply);
    lowercase(STR(reply->recipient));

    /*
     * Save the result.
     */
    return ((void *) reply);
}

/* resolve_pageout - page out an address resolver result */

static void resolve_pageout(void *data, void *unused_context)
{
    RESOLVE_REPLY *reply = (RESOLVE_REPLY *) data;

    resolve_clnt_free(reply);
    myfree((void *) reply);
}

/* smtpd_check_parse - pre-parse restrictions */

static ARGV *smtpd_check_parse(const char *checks)
{
    char   *saved_checks = mystrdup(checks);
    ARGV   *argv = argv_alloc(1);
    char   *bp = saved_checks;
    char   *name;

    /*
     * Pre-parse the restriction list, and open any dictionaries that we
     * encounter. Dictionaries must be opened before entering the chroot
     * jail.
     */
    while ((name = mystrtok(&bp, " \t\r\n,")) != 0) {
	argv_add(argv, name, (char *) 0);
	if (strchr(name, ':') && dict_handle(name) == 0)
	    dict_register(name, dict_open(name, O_RDONLY, DICT_FLAG_LOCK));
    }
    argv_terminate(argv);

    /*
     * Cleanup.
     */
    myfree(saved_checks);
    return (argv);
}

/* has_required - make sure required restriction is present */

static int has_required(ARGV *restrictions, char **required)
{
    char  **rest;
    char  **reqd;
    ARGV   *expansion;

    /*
     * Recursively check list membership.
     */
    for (rest = restrictions->argv; *rest; rest++) {
	if (strcmp(*rest, WARN_IF_REJECT) == 0 && rest[1] != 0) {
	    rest += 1;
	    continue;
	}
	for (reqd = required; *reqd; reqd++)
	    if (strcmp(*rest, *reqd) == 0)
		return (1);
	if ((expansion = (ARGV *) htable_find(smtpd_rest_classes, *rest)) != 0)
	    if (has_required(expansion, required))
		return (1);
    }
    return (0);
}

/* fail_required - handle failure to use required restriction */

static void fail_required(char *name, char **required)
{
    char   *myname = "fail_required";
    char  **reqd;
    VSTRING *example;

    /*
     * Sanity check.
     */
    if (required[0] == 0)
	msg_panic("%s: null required list", myname);

    /*
     * Go bust.
     */
    example = vstring_alloc(10);
    for (reqd = required; *reqd; reqd++)
	vstring_sprintf_append(example, "%s%s", *reqd,
			  reqd[1] == 0 ? "" : reqd[2] == 0 ? " or " : ", ");
    msg_fatal("parameter \"%s\": specify at least one working instance of: %s",
	      name, STR(example));
}

/* smtpd_check_init - initialize once during process lifetime */

void    smtpd_check_init(void)
{
    char   *saved_classes;
    const char *name;
    const char *value;
    char   *cp;
    static char *rcpt_required[] = {
	CHECK_RELAY_DOMAINS,
	REJECT_UNAUTH_DEST,
	REJECT_ALL,
	DEFER_ALL,
	DEFER_IF_PERMIT,
	0,
    };

    /*
     * Pre-open access control lists before going to jail.
     */
    mynetworks =
	namadr_list_init(match_parent_style(VAR_MYNETWORKS),
			 var_mynetworks);
    relay_domains =
	domain_list_init(match_parent_style(VAR_RELAY_DOMAINS),
			 var_relay_domains);
    perm_mx_networks =
	namadr_list_init(match_parent_style(VAR_PERM_MX_NETWORKS),
			 var_perm_mx_networks);

    /*
     * Pre-parse and pre-open the recipient maps.
     */
    local_rcpt_maps = maps_create(VAR_LOCAL_RCPT_MAPS, var_local_rcpt_maps,
				  DICT_FLAG_LOCK);
    rcpt_canon_maps = maps_create(VAR_RCPT_CANON_MAPS, var_rcpt_canon_maps,
				  DICT_FLAG_LOCK);
    canonical_maps = maps_create(VAR_CANONICAL_MAPS, var_canonical_maps,
				 DICT_FLAG_LOCK);
    virt_alias_maps = maps_create(VAR_VIRT_ALIAS_MAPS, var_virt_alias_maps,
				  DICT_FLAG_LOCK);
    virt_mailbox_maps = virtual8_maps_create(VAR_VIRT_MAILBOX_MAPS,
					     var_virt_mailbox_maps,
					     DICT_FLAG_LOCK);
    relay_rcpt_maps = maps_create(VAR_RELAY_RCPT_MAPS, var_relay_rcpt_maps,
				  DICT_FLAG_LOCK);

#ifdef TEST
    virt_alias_doms = string_list_init(MATCH_FLAG_NONE, var_virt_alias_doms);
    virt_mailbox_doms = string_list_init(MATCH_FLAG_NONE, var_virt_mailbox_doms);
#endif

    access_parent_style = match_parent_style(SMTPD_ACCESS_MAPS);

    /*
     * Templates for RBL rejection replies.
     */
    rbl_reply_maps = maps_create(VAR_RBL_REPLY_MAPS, var_rbl_reply_maps,
				 DICT_FLAG_LOCK);

    /*
     * Sender to login name mapping.
     */
    smtpd_sender_login_maps = maps_create(VAR_SMTPD_SND_AUTH_MAPS,
					  var_smtpd_snd_auth_maps,
					  DICT_FLAG_LOCK);

    /*
     * error_text is used for returning error responses.
     */
    error_text = vstring_alloc(10);

    /*
     * Initialize the resolved address cache. Note: the cache persists across
     * SMTP sessions so we cannot make it dependent on session state.
     */
    smtpd_resolve_cache = ctable_create(100, resolve_pagein,
					resolve_pageout, (void *) 0);

    /*
     * Initialize the RBL lookup cache. Note: the cache persists across SMTP
     * sessions so we cannot make it dependent on session state.
     */
    smtpd_rbl_cache = ctable_create(100, rbl_pagein, rbl_pageout, (void *) 0);

    /*
     * Pre-parse the restriction lists. At the same time, pre-open tables
     * before going to jail.
     */
    client_restrctions = smtpd_check_parse(var_client_checks);
    helo_restrctions = smtpd_check_parse(var_helo_checks);
    mail_restrctions = smtpd_check_parse(var_mail_checks);
    rcpt_restrctions = smtpd_check_parse(var_rcpt_checks);
    etrn_restrctions = smtpd_check_parse(var_etrn_checks);
    data_restrctions = smtpd_check_parse(var_data_checks);

    /*
     * Parse the pre-defined restriction classes.
     */
    smtpd_rest_classes = htable_create(1);
    if (*var_rest_classes) {
	cp = saved_classes = mystrdup(var_rest_classes);
	while ((name = mystrtok(&cp, " \t\r\n,")) != 0) {
	    if ((value = mail_conf_lookup_eval(name)) == 0 || *value == 0)
		msg_fatal("restriction class `%s' needs a definition", name);
	    htable_enter(smtpd_rest_classes, name,
			 (char *) smtpd_check_parse(value));
	}
	myfree(saved_classes);
    }

    /*
     * This is the place to specify definitions for complex restrictions such
     * as check_relay_domains in terms of more elementary restrictions.
     */
#if 0
    htable_enter(smtpd_rest_classes, "check_relay_domains",
	    smtpd_check_parse("permit_mydomain reject_unauth_destination"));
#endif

    /*
     * People screw up the relay restrictions too often. Require that they
     * list at least one restriction that rejects mail by default.
     */
#ifndef TEST
    if (!has_required(rcpt_restrctions, rcpt_required))
	fail_required(VAR_RCPT_CHECKS, rcpt_required);
#endif

    /*
     * Expand the expansion filter :-)
     */
    expand_filter = vstring_alloc(10);
    unescape(expand_filter, var_smtpd_exp_filter);
}

/* log_whatsup - log as much context as we have */

static void log_whatsup(SMTPD_STATE *state, const char *whatsup,
			        const char *text)
{
    VSTRING *buf = vstring_alloc(100);

    vstring_sprintf(buf, "%s: %s: %s from %s: %s;",
		    state->queue_id ? state->queue_id : "NOQUEUE",
		    whatsup, state->where, state->namaddr, text);
    if (state->sender)
	vstring_sprintf_append(buf, " from=<%s>", state->sender);
    if (state->recipient)
	vstring_sprintf_append(buf, " to=<%s>", state->recipient);
    if (state->protocol)
	vstring_sprintf_append(buf, " proto=%s", state->protocol);
    if (state->helo_name)
	vstring_sprintf_append(buf, " helo=<%s>", state->helo_name);
    msg_info("%s", STR(buf));
    vstring_free(buf);
}

/* smtpd_check_reject - do the boring things that must be done */

static int smtpd_check_reject(SMTPD_STATE *state, int error_class,
			              char *format,...)
{
    va_list ap;
    int     warn_if_reject;
    const char *whatsup;

    /*
     * Do not reject mail if we were asked to warn only. However,
     * configuration errors cannot be converted into warnings.
     */
    if (state->warn_if_reject && error_class != MAIL_ERROR_SOFTWARE) {
	warn_if_reject = 1;
	whatsup = "reject_warning";
    } else {
	warn_if_reject = 0;
	whatsup = "reject";
    }

    /*
     * Update the error class mask, and format the response. XXX What about
     * multi-line responses? For now we cheat and send whitespace.
     */
    state->error_mask |= error_class;
    va_start(ap, format);
    vstring_vsprintf(error_text, format, ap);
    va_end(ap);

    /*
     * Validate the response, that is, the response must begin with a
     * three-digit status code, and the first digit must be 4 or 5. If the
     * response is bad, log a warning and send a generic response instead.
     */
    if ((STR(error_text)[0] != '4' && STR(error_text)[0] != '5')
	|| !ISDIGIT(STR(error_text)[1]) || !ISDIGIT(STR(error_text)[2])
	|| ISDIGIT(STR(error_text)[3])) {
	msg_warn("response code configuration error: %s", STR(error_text));
	vstring_strcpy(error_text, "450 Service unavailable");
    }
    printable(STR(error_text), ' ');

    /*
     * Force this rejection into deferral because of some earlier temporary
     * error that may have prevented us from accepting mail, and report the
     * earlier problem instead.
     */
    if (!warn_if_reject && state->defer_if_reject.active && STR(error_text)[0] == '5') {
	state->warn_if_reject = state->defer_if_reject.active = 0;
	return (smtpd_check_reject(state, state->defer_if_reject.class,
				 "%s", STR(state->defer_if_reject.reason)));
    }

    /*
     * Soft bounce safety net.
     * 
     * XXX The code below also appears in the Postfix SMTP server reply output
     * routine. It is duplicated here in order to avoid discrepancies between
     * the reply codes that are shown in "reject" logging and the reply codes
     * that are actually sent to the SMTP client.
     * 
     * Implementing the soft_bounce safety net in the SMTP server reply output
     * routine has the advantage that it covers all 5xx replies, including
     * SMTP protocol or syntax errors, which makes soft_bounce great for
     * non-destructive tests (especially by people who are paranoid about
     * losing mail).
     * 
     * We could eliminate the code duplication and implement the soft_bounce
     * safety net only in the code below. But then the safety net would cover
     * the UCE restrictions only. This would be at odds with documentation
     * which says soft_bounce changes all 5xx replies into 4xx ones.
     */
    if (var_soft_bounce && STR(error_text)[0] == '5')
	STR(error_text)[0] = '4';

    /*
     * Log what is happening. When the sysadmin discards policy violation
     * postmaster notices, this may be the only trace left that service was
     * rejected. Print the request, client name/address, and response.
     */
    log_whatsup(state, whatsup, STR(error_text));

    return (warn_if_reject ? 0 : SMTPD_CHECK_REJECT);
}

/* defer_if - prepare to change our mind */

static void defer_if(SMTPD_DEFER *defer, int error_class, const char *fmt,...)
{
    va_list ap;

    /*
     * Keep the first reason for this type of deferral, to minimize
     * confusion.
     */
    if (defer->active == 0) {
	defer->active = 1;
	defer->class = error_class;
	if (defer->reason == 0)
	    defer->reason = vstring_alloc(10);
	va_start(ap, fmt);
	vstring_vsprintf(defer->reason, fmt, ap);
	va_end(ap);
    }
}

/* reject_dict_retry - reject with temporary failure if dict lookup fails */

static void reject_dict_retry(SMTPD_STATE *state, const char *reply_name)
{
    longjmp(smtpd_check_buf, smtpd_check_reject(state, MAIL_ERROR_RESOURCE,
					"%d <%s>: Temporary lookup failure",
						451, reply_name));
}

/* checkv8_maps_find - reject with temporary failure if dict lookup fails */

static const char *checkv8_maps_find(SMTPD_STATE *state, const char *reply_name,
				             MAPS *maps, const char *key)
{
    const char *result;

    dict_errno = 0;
    if ((result = virtual8_maps_find(maps, key)) == 0
	&& dict_errno == DICT_ERR_RETRY)
	reject_dict_retry(state, reply_name);
    return (result);
}

/* check_mail_addr_find - reject with temporary failure if dict lookup fails */

static const char *check_mail_addr_find(SMTPD_STATE *state,
					        const char *reply_name,
					        MAPS *maps, const char *key,
					        char **ext)
{
    const char *result;

    dict_errno = 0;
    if ((result = mail_addr_find(maps, key, ext)) == 0
	&& dict_errno == DICT_ERR_RETRY)
	reject_dict_retry(state, reply_name);
    return (result);
}

/* reject_unknown_client - fail if client hostname is unknown */

static int reject_unknown_client(SMTPD_STATE *state)
{
    char   *myname = "reject_unknown_client";

    if (msg_verbose)
	msg_info("%s: %s %s", myname, state->name, state->addr);

    if (strcasecmp(state->name, "unknown") == 0)
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
		 "%d Client host rejected: cannot find your hostname, [%s]",
				   state->peer_code == 5 ?
				   var_unk_client_code : 450,
				   state->addr));
    return (SMTPD_CHECK_DUNNO);
}

/* permit_mynetworks - succeed if client is in a trusted network */

static int permit_mynetworks(SMTPD_STATE *state)
{
    char   *myname = "permit_mynetworks";

    if (msg_verbose)
	msg_info("%s: %s %s", myname, state->name, state->addr);

    if (namadr_list_match(mynetworks, state->name, state->addr))
	return (SMTPD_CHECK_OK);
    return (SMTPD_CHECK_DUNNO);
}

/* dup_if_truncate - save hostname and truncate if it ends in dot */

static char *dup_if_truncate(char *name)
{
    int     len;
    char   *result;

    /*
     * Truncate hostnames ending in dot but not dot-dot.
     * 
     * XXX This should not be distributed all over the code. Problem is,
     * addresses can enter the system via multiple paths: networks, local
     * forward/alias/include files, even as the result of address rewriting.
     */
    if ((len = strlen(name)) > 1
	&& name[len - 1] == '.'
	&& name[len - 2] != '.') {
	result = mystrndup(name, len - 1);
    } else
	result = name;
    return (result);
}

/* reject_invalid_hostaddr - fail if host address is incorrect */

static int reject_invalid_hostaddr(SMTPD_STATE *state, char *addr,
				        char *reply_name, char *reply_class)
{
    char   *myname = "reject_invalid_hostaddr";
    int     len;
    char   *test_addr;
    int     stat;

    if (msg_verbose)
	msg_info("%s: %s", myname, addr);

    if (addr[0] == '[' && (len = strlen(addr)) > 2 && addr[len - 1] == ']') {
	test_addr = mystrndup(&addr[1], len - 2);
    } else
	test_addr = addr;

    /*
     * Validate the address.
     */
    if (!valid_hostaddr(test_addr, DONT_GRIPE))
	stat = smtpd_check_reject(state, MAIL_ERROR_POLICY,
				  "%d <%s>: %s rejected: invalid ip address",
				var_bad_name_code, reply_name, reply_class);
    else
	stat = SMTPD_CHECK_DUNNO;

    /*
     * Cleanup.
     */
    if (test_addr != addr)
	myfree(test_addr);

    return (stat);
}

/* reject_invalid_hostname - fail if host/domain syntax is incorrect */

static int reject_invalid_hostname(SMTPD_STATE *state, char *name,
				        char *reply_name, char *reply_class)
{
    char   *myname = "reject_invalid_hostname";
    char   *test_name;
    int     stat;

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

    /*
     * Truncate hostnames ending in dot but not dot-dot.
     */
    test_name = dup_if_truncate(name);

    /*
     * Validate the hostname.
     */
    if (!valid_hostname(test_name, DONT_GRIPE))
	stat = smtpd_check_reject(state, MAIL_ERROR_POLICY,
				  "%d <%s>: %s rejected: Invalid name",
				var_bad_name_code, reply_name, reply_class);
    else
	stat = SMTPD_CHECK_DUNNO;

    /*
     * Cleanup.
     */
    if (test_name != name)
	myfree(test_name);

    return (stat);
}

/* reject_non_fqdn_hostname - fail if host name is not in fqdn form */

static int reject_non_fqdn_hostname(SMTPD_STATE *state, char *name,
				        char *reply_name, char *reply_class)
{
    char   *myname = "reject_non_fqdn_hostname";
    char   *test_name;
    int     stat;

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

    /*
     * Truncate hostnames ending in dot but not dot-dot.
     */
    test_name = dup_if_truncate(name);

    /*
     * Validate the hostname.
     */
    if (!valid_hostname(test_name, DONT_GRIPE) || !strchr(test_name, '.'))
	stat = smtpd_check_reject(state, MAIL_ERROR_POLICY,
		      "%d <%s>: %s rejected: need fully-qualified hostname",
				var_non_fqdn_code, reply_name, reply_class);
    else
	stat = SMTPD_CHECK_DUNNO;

    /*
     * Cleanup.
     */
    if (test_name != name)
	myfree(test_name);

    return (stat);
}

/* reject_unknown_hostname - fail if name has no A, AAAA or MX record */

static int reject_unknown_hostname(SMTPD_STATE *state, char *name,
				        char *reply_name, char *reply_class)
{
    char   *myname = "reject_unknown_hostname";
    int     dns_status;

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

#ifdef T_AAAA
#define RR_ADDR_TYPES	T_A, T_AAAA
#else
#define RR_ADDR_TYPES	T_A
#endif

    dns_status = dns_lookup_types(name, 0, (DNS_RR **) 0, (VSTRING *) 0,
				  (VSTRING *) 0, RR_ADDR_TYPES, T_MX, 0);
    if (dns_status == DNS_NOTFOUND)
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				   "%d <%s>: %s rejected: Host not found",
				   var_unk_name_code,
				   reply_name, reply_class));
    else if (dns_status != DNS_OK)
	DEFER_IF_PERMIT2(state, MAIL_ERROR_POLICY,
			 "450 <%s>: %s rejected: Host not found",
			 reply_name, reply_class);
    return (SMTPD_CHECK_DUNNO);
}

/* reject_unknown_mailhost - fail if name has no A, AAAA or MX record */

static int reject_unknown_mailhost(SMTPD_STATE *state, const char *name,
		            const char *reply_name, const char *reply_class)
{
    char   *myname = "reject_unknown_mailhost";
    int     dns_status;

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

    dns_status = dns_lookup_types(name, 0, (DNS_RR **) 0, (VSTRING *) 0,
				  (VSTRING *) 0, RR_ADDR_TYPES, T_MX, 0);
    if (dns_status == DNS_NOTFOUND)
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				   "%d <%s>: %s rejected: Domain not found",
				   var_unk_addr_code,
				   reply_name, reply_class));
    else if (dns_status != DNS_OK)
	DEFER_IF_PERMIT2(state, MAIL_ERROR_POLICY,
			 "450 <%s>: %s rejected: Domain not found",
			 reply_name, reply_class);
    return (SMTPD_CHECK_DUNNO);
}

static int permit_auth_destination(SMTPD_STATE *state, char *recipient);

/* check_relay_domains - OK/FAIL for message relaying */

static int check_relay_domains(SMTPD_STATE *state, char *recipient,
			               char *reply_name, char *reply_class)
{
    char   *myname = "check_relay_domains";

#if 1
    static int once;

    if (once == 0) {
	once = 1;
	msg_warn("the \"%s\" restriction is going away; use \"%s\" instead",
		 CHECK_RELAY_DOMAINS, REJECT_UNAUTH_DEST);
    }
#endif

    if (msg_verbose)
	msg_info("%s: %s", myname, recipient);

    /*
     * Permit if the client matches the relay_domains list.
     */
    if (domain_list_match(relay_domains, state->name))
	return (SMTPD_CHECK_OK);

    /*
     * Permit authorized destinations.
     */
    if (permit_auth_destination(state, recipient) == SMTPD_CHECK_OK)
	return (SMTPD_CHECK_OK);

    /*
     * Deny relaying between sites that both are not in relay_domains.
     */
    return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
			       "%d <%s>: %s rejected: Relay access denied",
			       var_relay_code, reply_name, reply_class));
}

/* permit_auth_destination - OK for message relaying */

static int permit_auth_destination(SMTPD_STATE *state, char *recipient)
{
    char   *myname = "permit_auth_destination";
    const RESOLVE_REPLY *reply;

    if (msg_verbose)
	msg_info("%s: %s", myname, recipient);

    /*
     * Resolve the address.
     */
    reply = (const RESOLVE_REPLY *)
	ctable_locate(smtpd_resolve_cache, recipient);
    if (reply->flags & RESOLVE_FLAG_FAIL)
	reject_dict_retry(state, recipient);

    /*
     * Handle special case that is not supposed to happen.
     */
    if (strrchr(CONST_STR(reply->recipient), '@') == 0)
	return (SMTPD_CHECK_OK);

    /*
     * Skip source-routed non-local or virtual mail (uncertain destination).
     */
    if (var_allow_untrust_route == 0 && (reply->flags & RESOLVE_FLAG_ROUTED))
	return (SMTPD_CHECK_DUNNO);

    /*
     * Permit final delivery: the destination matches mydestination,
     * virtual_alias_domains, or virtual_mailbox_domains.
     */
    if (reply->flags & RESOLVE_CLASS_FINAL)
	return (SMTPD_CHECK_OK);

    /*
     * Permit if the destination matches the relay_domains list.
     */
    if (reply->flags & RESOLVE_CLASS_RELAY)
	return (SMTPD_CHECK_OK);

    /*
     * Skip when not matched
     */
    return (SMTPD_CHECK_DUNNO);
}

/* reject_unauth_destination - FAIL for message relaying */

static int reject_unauth_destination(SMTPD_STATE *state, char *recipient)
{
    char   *myname = "reject_unauth_destination";

    if (msg_verbose)
	msg_info("%s: %s", myname, recipient);

    /*
     * Skip authorized destination.
     */
    if (permit_auth_destination(state, recipient) == SMTPD_CHECK_OK)
	return (SMTPD_CHECK_DUNNO);

    /*
     * Reject relaying to sites that are not listed in relay_domains.
     */
    return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
			       "%d <%s>: Relay access denied",
			       var_relay_code, recipient));
}

/* reject_unauth_pipelining - reject improper use of SMTP command pipelining */

static int reject_unauth_pipelining(SMTPD_STATE *state,
		            const char *reply_name, const char *reply_class)
{
    char   *myname = "reject_unauth_pipelining";

    if (msg_verbose)
	msg_info("%s: %s", myname, state->where);

    if (state->client != 0
	&& SMTPD_STAND_ALONE(state) == 0
	&& vstream_peek(state->client) > 0
	&& (strcasecmp(state->protocol, "ESMTP") != 0
	    || strcasecmp(state->where, "DATA") == 0)) {
	return (smtpd_check_reject(state, MAIL_ERROR_PROTOCOL,
	   "503 <%s>: %s rejected: Improper use of SMTP command pipelining",
				   reply_name, reply_class));
    }
    return (SMTPD_CHECK_DUNNO);
}

/* all_auth_mx_addr - match host addresses against permit_mx_backup_networks */

static int all_auth_mx_addr(SMTPD_STATE *state, char *host,
		            const char *reply_name, const char *reply_class)
{
    char   *myname = "all_auth_mx_addr";
    struct in_addr addr;
    DNS_RR *rr;
    DNS_RR *addr_list;
    int     dns_status;

    if (msg_verbose)
	msg_info("%s: host %s", myname, host);

    /*
     * If we can't lookup the host, defer.
     */
#define NOPE           0
#define YUP            1

    /*
     * Verify that all host addresses are within permit_mx_backup_networks.
     */
    dns_status = dns_lookup(host, T_A, 0, &addr_list, (VSTRING *) 0, (VSTRING *) 0);
    if (dns_status != DNS_OK) {
	DEFER_IF_REJECT3(state, MAIL_ERROR_POLICY,
	"450 <%s>: %s rejected: Unable to look up host %s as mail exchanger",
			 reply_name, reply_class, host);
	return (NOPE);
    }
    for (rr = addr_list; rr != 0; rr = rr->next) {
	if (rr->data_len > sizeof(addr)) {
	    msg_warn("%s: skipping address length %d for host %s",
		     state->queue_id, rr->data_len, host);
	    continue;
	}
	memcpy((char *) &addr, rr->data, sizeof(addr));
	if (msg_verbose)
	    msg_info("%s: checking: %s", myname, inet_ntoa(addr));

	if (!namadr_list_match(perm_mx_networks, host, inet_ntoa(addr))) {

	    /*
	     * Reject: at least one IP address is not listed in
	     * permit_mx_backup_networks.
	     */
	    if (msg_verbose)
		msg_info("%s: address %s for %s does not match %s",
		       myname, inet_ntoa(addr), host, VAR_PERM_MX_NETWORKS);
	    dns_rr_free(addr_list);
	    return (NOPE);
	}
    }
    dns_rr_free(addr_list);
    return (YUP);
}

/* has_my_addr - see if this host name lists one of my network addresses */

static int has_my_addr(SMTPD_STATE *state, const char *host,
		            const char *reply_name, const char *reply_class)
{
    char   *myname = "has_my_addr";
    struct in_addr addr;
    char  **cpp;
    struct hostent *hp;

    if (msg_verbose)
	msg_info("%s: host %s", myname, host);

    /*
     * If we can't lookup the host, defer rather than reject.
     */
#define YUP	1
#define NOPE	0

    if ((hp = gethostbyname(host)) == 0) {
	DEFER_IF_REJECT3(state, MAIL_ERROR_POLICY,
	  "450 <%s>: %s rejected: Unable to look up mail exchanger host %s",
			 reply_name, reply_class, host);
	return (NOPE);
    }
    if (hp->h_addrtype != AF_INET || hp->h_length != sizeof(addr)) {
	msg_warn("address type %d length %d for %s",
		 hp->h_addrtype, hp->h_length, host);
	return (NOPE);
    }
    for (cpp = hp->h_addr_list; *cpp; cpp++) {
	memcpy((char *) &addr, *cpp, sizeof(addr));
	if (msg_verbose)
	    msg_info("%s: addr %s", myname, inet_ntoa(addr));
	if (own_inet_addr(&addr))
	    return (YUP);
	if (proxy_inet_addr(&addr))
	    return (YUP);
    }
    if (msg_verbose)
	msg_info("%s: host %s: no match", myname, host);

    return (NOPE);
}

/* i_am_mx - is this machine listed as MX relay */

static int i_am_mx(SMTPD_STATE *state, DNS_RR *mx_list,
		           const char *reply_name, const char *reply_class)
{
    const char *myname = "permit_mx_backup";
    DNS_RR *mx;

    /*
     * Compare hostnames first. Only if no name match is found, go through
     * the trouble of host address lookups.
     */
    for (mx = mx_list; mx != 0; mx = mx->next) {
	if (msg_verbose)
	    msg_info("%s: resolve hostname: %s", myname, (char *) mx->data);
	if (resolve_local((char *) mx->data))
	    return (YUP);
    }

    /*
     * Argh. Do further DNS lookups and match interface addresses.
     */
    for (mx = mx_list; mx != 0; mx = mx->next) {
	if (msg_verbose)
	    msg_info("%s: address lookup: %s", myname, (char *) mx->data);
	if (has_my_addr(state, (char *) mx->data, reply_name, reply_class))
	    return (YUP);
    }

    /*
     * This machine is not listed as MX relay.
     */
    if (msg_verbose)
	msg_info("%s: I am not listed as MX relay", myname);
    return (NOPE);
}

/* permit_mx_primary - authorize primary MX relays */

static int permit_mx_primary(SMTPD_STATE *state, DNS_RR *mx_list,
		            const char *reply_name, const char *reply_class)
{
    DNS_RR *mx;
    unsigned int best_pref;

    /*
     * Find the preference of the primary MX hosts.
     */
    for (best_pref = 0xffff, mx = mx_list; mx != 0; mx = mx->next)
	if (mx->pref < best_pref)
	    best_pref = mx->pref;

    /*
     * See if each best MX host has all IP addresses in
     * permit_mx_backup_networks.
     */
    for (mx = mx_list; mx != 0; mx = mx->next) {
	if (mx->pref != best_pref)
	    continue;
	if (!all_auth_mx_addr(state, (char *) mx->data, reply_name, reply_class))
	    return (NOPE);
    }

    /*
     * All IP addresses of the best MX hosts are within
     * permit_mx_backup_networks.
     */
    return (YUP);
}

/* permit_mx_backup - permit use of me as MX backup for recipient domain */

static int permit_mx_backup(SMTPD_STATE *state, const char *recipient,
		            const char *reply_name, const char *reply_class)
{
    char   *myname = "permit_mx_backup";
    const RESOLVE_REPLY *reply;
    const char *domain;

    DNS_RR *mx_list;
    int     dns_status;

    if (msg_verbose)
	msg_info("%s: %s", myname, recipient);

    /*
     * Resolve the address.
     */
    reply = (const RESOLVE_REPLY *)
	ctable_locate(smtpd_resolve_cache, recipient);
    if (reply->flags & RESOLVE_FLAG_FAIL)
	reject_dict_retry(state, recipient);

    /*
     * If the destination is local, it is acceptable, because we are
     * supposedly MX for our own address.
     */
    if ((domain = strrchr(CONST_STR(reply->recipient), '@')) == 0)
	return (SMTPD_CHECK_OK);
    domain += 1;
    if (reply->flags & RESOLVE_CLASS_LOCAL)
	return (SMTPD_CHECK_OK);

    /*
     * Skip source-routed non-local or virtual mail (uncertain destination).
     */
    if (var_allow_untrust_route == 0 && (reply->flags & RESOLVE_FLAG_ROUTED))
	return (SMTPD_CHECK_DUNNO);

    /*
     * The destination is local, or it is a local virtual destination.
     */
    if (reply->flags & RESOLVE_CLASS_FINAL)
	return (SMTPD_CHECK_OK);

    if (msg_verbose)
	msg_info("%s: not local: %s", myname, recipient);

    /*
     * Skip numerical forms that didn't match the local system.
     */
    if (domain[0] == '[' && domain[strlen(domain) - 1] == ']')
	return (SMTPD_CHECK_DUNNO);

    /*
     * Look up the list of MX host names for this domain. If no MX host is
     * found, perhaps it is a CNAME for the local machine. Clients aren't
     * supposed to send CNAMEs in SMTP commands, but it happens anyway. If we
     * can't look up the destination, play safe and turn reject into defer.
     */
    dns_status = dns_lookup(domain, T_MX, 0, &mx_list,
			    (VSTRING *) 0, (VSTRING *) 0);
    if (dns_status == DNS_NOTFOUND)
	return (has_my_addr(state, domain, reply_name, reply_class) ?
		SMTPD_CHECK_OK : SMTPD_CHECK_DUNNO);
    if (dns_status != DNS_OK) {
	DEFER_IF_REJECT2(state, MAIL_ERROR_POLICY,
			 "450 <%s>: %s rejected: Unable to look up mail exchanger information",
			 reply_name, reply_class);
	return (SMTPD_CHECK_DUNNO);
    }

    /*
     * First, see if we match any of the MX host names listed.
     */
    if (!i_am_mx(state, mx_list, reply_name, reply_class)) {
	dns_rr_free(mx_list);
	return (SMTPD_CHECK_DUNNO);
    }

    /*
     * Optionally, see if the primary MX hosts are in a restricted list of
     * networks.
     */
    if (*var_perm_mx_networks
	&& !permit_mx_primary(state, mx_list, reply_name, reply_class)) {
	dns_rr_free(mx_list);
	return (SMTPD_CHECK_DUNNO);
    }

    /*
     * The destination passed all requirements.
     */
    dns_rr_free(mx_list);
    return (SMTPD_CHECK_OK);
}

/* reject_non_fqdn_address - fail if address is not in fqdn form */

static int reject_non_fqdn_address(SMTPD_STATE *state, char *addr,
				        char *reply_name, char *reply_class)
{
    char   *myname = "reject_non_fqdn_address";
    char   *domain;
    char   *test_dom;
    int     stat;

    if (msg_verbose)
	msg_info("%s: %s", myname, addr);

    /*
     * Locate the domain information.
     */
    if ((domain = strrchr(addr, '@')) != 0)
	domain++;
    else
	domain = "";

    /*
     * Skip forms that we can't handle yet.
     */
    if (domain[0] == '[' && domain[strlen(domain) - 1] == ']')
	return (SMTPD_CHECK_DUNNO);

    /*
     * Truncate names ending in dot but not dot-dot.
     */
    test_dom = dup_if_truncate(domain);

    /*
     * Validate the domain.
     */
    if (!*test_dom || !valid_hostname(test_dom, DONT_GRIPE) || !strchr(test_dom, '.'))
	stat = smtpd_check_reject(state, MAIL_ERROR_POLICY,
		       "%d <%s>: %s rejected: need fully-qualified address",
				var_non_fqdn_code, reply_name, reply_class);
    else
	stat = SMTPD_CHECK_DUNNO;

    /*
     * Cleanup.
     */
    if (test_dom != domain)
	myfree(test_dom);

    return (stat);
}

/* reject_unknown_address - fail if address does not resolve */

static int reject_unknown_address(SMTPD_STATE *state, const char *addr,
		            const char *reply_name, const char *reply_class)
{
    char   *myname = "reject_unknown_address";
    const RESOLVE_REPLY *reply;
    const char *domain;

    if (msg_verbose)
	msg_info("%s: %s", myname, addr);

    /*
     * Resolve the address.
     */
    reply = (const RESOLVE_REPLY *) ctable_locate(smtpd_resolve_cache, addr);
    if (reply->flags & RESOLVE_FLAG_FAIL)
	reject_dict_retry(state, addr);

    /*
     * Skip local destinations and non-DNS forms.
     */
    if ((domain = strrchr(CONST_STR(reply->recipient), '@')) == 0)
	return (SMTPD_CHECK_DUNNO);
    domain += 1;
    if (reply->flags & RESOLVE_CLASS_FINAL)
	return (SMTPD_CHECK_DUNNO);
    if (domain[0] == '[' && domain[strlen(domain) - 1] == ']')
	return (SMTPD_CHECK_DUNNO);

    /*
     * Look up the name in the DNS.
     */
    return (reject_unknown_mailhost(state, domain, reply_name, reply_class));
}

/* check_table_result - translate table lookup result into pass/reject */

static int check_table_result(SMTPD_STATE *state, const char *table,
			              const char *value, const char *datum,
			              const char *reply_name,
			              const char *reply_class,
			              const char *def_acl)
{
    char   *myname = "check_table_result";
    int     code;
    ARGV   *restrictions;
    jmp_buf savebuf;
    int     status;
    const char *cmd_text;
    int     cmd_len;

    /*
     * Parse into command and text. Do not change the input.
     */
    cmd_text = value + strcspn(value, " \t");
    cmd_len = cmd_text - value;
    while (*cmd_text && ISSPACE(*cmd_text))
	cmd_text++;

    if (msg_verbose)
	msg_info("%s: %s %s %s", myname, table, value, datum);

#define STREQUAL(x,y,l) (strncasecmp((x), (y), (l)) == 0 && (y)[l] == 0)

    /*
     * DUNNO means skip this table. Silently ignore optional text.
     */
    if (STREQUAL(value, "DUNNO", cmd_len))
	return (SMTPD_CHECK_DUNNO);

    /*
     * REJECT means NO. Use optional text or generate a generic error
     * response.
     */
    if (STREQUAL(value, "REJECT", cmd_len)) {
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				   "%d <%s>: %s rejected: %s",
			       var_access_map_code, reply_name, reply_class,
				   *cmd_text ? cmd_text : "Access denied"));
    }

    /*
     * FILTER means deliver to content filter. But we may still change our
     * mind, and reject/discard the message for other reasons.
     */
    if (STREQUAL(value, "FILTER", cmd_len)) {
	if (*cmd_text == 0) {
	    msg_warn("access map %s entry \"%s\" has FILTER entry without value",
		     table, datum);
	    return (SMTPD_CHECK_DUNNO);
	} else if (strchr(cmd_text, ':') == 0) {
	    msg_warn("access map %s entry \"%s\" requires transport:destination",
		     table, datum);
	    return (SMTPD_CHECK_DUNNO);
	} else {
	    vstring_sprintf(error_text, "<%s>: %s triggers FILTER %s",
			    reply_name, reply_class, cmd_text);
	    log_whatsup(state, "filter", STR(error_text));
#ifndef TEST
	    rec_fprintf(state->dest->stream, REC_TYPE_FILT, "%s", cmd_text);
#endif
	    return (SMTPD_CHECK_DUNNO);
	}
    }

    /*
     * HOLD means deliver later. But we may still change our mind, and
     * reject/discard the message for other reasons.
     */
    if (STREQUAL(value, "HOLD", cmd_len)) {
	vstring_sprintf(error_text, "<%s>: %s %s", reply_name, reply_class,
			*cmd_text ? cmd_text : "triggers HOLD action");
	log_whatsup(state, "hold", STR(error_text));
#ifndef TEST
	rec_fprintf(state->dest->stream, REC_TYPE_FLGS, "%d",
		    CLEANUP_FLAG_HOLD);
#endif
	return (SMTPD_CHECK_DUNNO);
    }

    /*
     * DISCARD means silently discard and claim successful delivery.
     * 
     * XXX Set some global flag that disables all further restrictions.
     * Triggering a "reject" or "hold" action after "discard" is silly.
     */
    if (STREQUAL(value, "DISCARD", cmd_len)) {
	vstring_sprintf(error_text, "<%s>: %s %s", reply_name, reply_class,
			*cmd_text ? cmd_text : "triggers DISCARD action");
	log_whatsup(state, "discard", STR(error_text));
#ifndef TEST
	rec_fprintf(state->dest->stream, REC_TYPE_FLGS, "%d",
		    CLEANUP_FLAG_DISCARD);
#endif
	return (SMTPD_CHECK_OK);
    }

    /*
     * All-numeric result probably means OK - some out-of-band authentication
     * mechanism uses this as time stamp.
     */
    if (alldig(value))
	return (SMTPD_CHECK_OK);

    /*
     * 4xx or 5xx means NO as well. smtpd_check_reject() will validate the
     * response status code.
     */
    if (cmd_len == 3 && *cmd_text
	&& ISDIGIT(value[0]) && ISDIGIT(value[1]) && ISDIGIT(value[2])) {
	code = atoi(value);
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				   "%d <%s>: %s rejected: %s",
				   code, reply_name, reply_class, cmd_text));
    }

    /*
     * OK or RELAY means YES. Ignore trailing text.
     */
    if (STREQUAL(value, "OK", cmd_len) || STREQUAL(value, "RELAY", cmd_len))
	return (SMTPD_CHECK_OK);

    /*
     * Unfortunately, maps must be declared ahead of time so they can be
     * opened before we go to jail. We could insist that the RHS can only
     * contain a pre-defined restriction class name, but that would be too
     * restrictive. Instead we warn if an access table references any map.
     * 
     * XXX Don't use passwd files or address rewriting maps as access tables.
     */
    if (strchr(value, ':') != 0) {
	msg_warn("SMTPD access map %s has entry with lookup table: %s",
		 table, value);
	msg_warn("do not specify lookup tables inside SMTPD access maps");
	msg_warn("define a restriction class and specify its name instead.");
	longjmp(smtpd_check_buf, smtpd_check_reject(state, MAIL_ERROR_SOFTWARE,
					 "451 Server configuration error"));
    }

    /*
     * Don't get carried away with recursion.
     */
    if (state->recursion > 100) {
	msg_warn("SMTPD access map %s entry %s causes unreasonable recursion",
		 table, value);
	longjmp(smtpd_check_buf, smtpd_check_reject(state, MAIL_ERROR_SOFTWARE,
					 "451 Server configuration error"));
    }

    /*
     * Recursively evaluate the restrictions given in the right-hand side. In
     * the dark ages, an empty right-hand side meant OK. Make some
     * discouraging comments.
     * 
     * XXX Jump some hoops to avoid a minute memory leak in case of a file
     * configuration error.
     */
#define ADDROF(x) ((char *) &(x))

    restrictions = argv_split(value, " \t\r\n,");
    memcpy(ADDROF(savebuf), ADDROF(smtpd_check_buf), sizeof(savebuf));
    status = setjmp(smtpd_check_buf);
    if (status != 0) {
	argv_free(restrictions);
	memcpy(ADDROF(smtpd_check_buf), ADDROF(savebuf),
	       sizeof(smtpd_check_buf));
	longjmp(smtpd_check_buf, status);
    }
    if (restrictions->argc == 0) {
	msg_warn("SMTPD access map %s entry %s has empty value",
		 table, value);
	status = SMTPD_CHECK_OK;
    } else {
	status = generic_checks(state, restrictions, reply_name,
				reply_class, def_acl);
    }
    argv_free(restrictions);
    return (status);
}

/* check_access - table lookup without substring magic */

static int check_access(SMTPD_STATE *state, const char *table, const char *name,
		              int flags, int *found, const char *reply_name,
			        const char *reply_class, const char *def_acl)
{
    char   *myname = "check_access";
    char   *low_name = lowercase(mystrdup(name));
    const char *value;
    DICT   *dict;

#define CHK_ACCESS_RETURN(x,y) { *found = y; myfree(low_name); return(x); }
#define FULL	0
#define PARTIAL	DICT_FLAG_FIXED
#define FOUND	1
#define MISSED	0

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

    if ((dict = dict_handle(table)) == 0)
	msg_panic("%s: dictionary not found: %s", myname, table);
    if (flags == 0 || (flags & dict->flags) != 0) {
	if ((value = dict_get(dict, low_name)) != 0)
	    CHK_ACCESS_RETURN(check_table_result(state, table, value, name,
						 reply_name, reply_class,
						 def_acl), FOUND);
	if (dict_errno != 0)
	    msg_fatal("%s: table lookup problem", table);
    }
    CHK_ACCESS_RETURN(SMTPD_CHECK_DUNNO, MISSED);
}

/* check_domain_access - domainname-based table lookup */

static int check_domain_access(SMTPD_STATE *state, const char *table,
			               const char *domain, int flags,
			               int *found, const char *reply_name,
			               const char *reply_class,
			               const char *def_acl)
{
    char   *myname = "check_domain_access";
    char   *low_domain = lowercase(mystrdup(domain));
    char   *name;
    char   *next;
    const char *value;
    DICT   *dict;

    if (msg_verbose)
	msg_info("%s: %s", myname, domain);

    /*
     * Try the name and its parent domains. Including top-level domains.
     * 
     * Helo names can end in ".". The test below avoids lookups of the empty
     * key, because Berkeley DB cannot deal with it. [Victor Duchovni, Morgan
     * Stanley].
     */
#define CHK_DOMAIN_RETURN(x,y) { *found = y; myfree(low_domain); return(x); }

    if ((dict = dict_handle(table)) == 0)
	msg_panic("%s: dictionary not found: %s", myname, table);
    for (name = low_domain; *name != 0; name = next) {
	if (flags == 0 || (flags & dict->flags) != 0) {
	    if ((value = dict_get(dict, name)) != 0)
		CHK_DOMAIN_RETURN(check_table_result(state, table, value,
					    domain, reply_name, reply_class,
						     def_acl), FOUND);
	    if (dict_errno != 0)
		msg_fatal("%s: table lookup problem", table);
	}
	if ((next = strchr(name + 1, '.')) == 0)
	    break;
	if (access_parent_style == MATCH_FLAG_PARENT)
	    next += 1;
	flags = PARTIAL;
    }
    CHK_DOMAIN_RETURN(SMTPD_CHECK_DUNNO, MISSED);
}

/* check_addr_access - address-based table lookup */

static int check_addr_access(SMTPD_STATE *state, const char *table,
			             const char *address, int flags,
			             int *found, const char *reply_name,
			             const char *reply_class,
			             const char *def_acl)
{
    char   *myname = "check_addr_access";
    char   *addr;
    const char *value;
    DICT   *dict;

    if (msg_verbose)
	msg_info("%s: %s", myname, address);

    /*
     * Try the address and its parent networks.
     */
#define CHK_ADDR_RETURN(x,y) { *found = y; return(x); }

    addr = STR(vstring_strcpy(error_text, address));

    if ((dict = dict_handle(table)) == 0)
	msg_panic("%s: dictionary not found: %s", myname, table);
    do {
	if (flags == 0 || (flags & dict->flags) != 0) {
	    if ((value = dict_get(dict, addr)) != 0)
		CHK_ADDR_RETURN(check_table_result(state, table, value, address,
						   reply_name, reply_class,
						   def_acl), FOUND);
	    if (dict_errno != 0)
		msg_fatal("%s: table lookup problem", table);
	}
	flags = PARTIAL;
    } while (split_at_right(addr, '.'));

    CHK_ADDR_RETURN(SMTPD_CHECK_DUNNO, MISSED);
}

/* check_namadr_access - OK/FAIL based on host name/address lookup */

static int check_namadr_access(SMTPD_STATE *state, const char *table,
			               const char *name, const char *addr,
			               int flags, int *found,
			               const char *reply_name,
			               const char *reply_class,
			               const char *def_acl)
{
    char   *myname = "check_namadr_access";
    int     status;

    if (msg_verbose)
	msg_info("%s: name %s addr %s", myname, name, addr);

    /*
     * Look up the host name, or parent domains thereof. XXX A domain
     * wildcard may pre-empt a more specific address table entry.
     */
    if ((status = check_domain_access(state, table, name, flags,
				      found, reply_name, reply_class,
				      def_acl)) != 0 || *found)
	return (status);

    /*
     * Look up the network address, or parent networks thereof.
     */
    if ((status = check_addr_access(state, table, addr, flags,
				    found, reply_name, reply_class,
				    def_acl)) != 0 || *found)
	return (status);

    /*
     * Undecided when the host was not found.
     */
    return (SMTPD_CHECK_DUNNO);
}

/* check_mail_access - OK/FAIL based on mail address lookup */

static int check_mail_access(SMTPD_STATE *state, const char *table,
			             const char *addr, int *found,
			             const char *reply_name,
			             const char *reply_class,
			             const char *def_acl)
{
    char   *myname = "check_mail_access";
    const RESOLVE_REPLY *reply;
    const char *domain;
    int     status;
    char   *local_at;
    char   *bare_addr;
    char   *bare_at;

    if (msg_verbose)
	msg_info("%s: %s", myname, addr);

    /*
     * Resolve the address.
     */
    reply = (const RESOLVE_REPLY *) ctable_locate(smtpd_resolve_cache, addr);
    if (reply->flags & RESOLVE_FLAG_FAIL)
	reject_dict_retry(state, addr);

    /*
     * Garbage in, garbage out. Every address from canon_addr_internal() and
     * from resolve_clnt_query() must be fully qualified.
     */
    if ((domain = strrchr(CONST_STR(reply->recipient), '@')) == 0) {
	msg_warn("%s: no @domain in address: %s", myname,
		 CONST_STR(reply->recipient));
	return (0);
    }
    domain += 1;

    /*
     * In case of address extensions.
     */
    if (*var_rcpt_delim == 0) {
	bare_addr = 0;
    } else {
	bare_addr = strip_addr(addr, (char **) 0, *var_rcpt_delim);
    }

#define CHECK_MAIL_ACCESS_RETURN(x) \
	{ if (bare_addr) myfree(bare_addr); return(x); }

    /*
     * Source-routed (non-local or virtual) recipient addresses are too
     * suspicious for returning an "OK" result. The complicated expression
     * below was brought to you by the keyboard of Victor Duchovni, Morgan
     * Stanley and hacked up a bit by Wietse.
     */
#define SUSPICIOUS(reply, reply_class) \
	(var_allow_untrust_route == 0 \
	&& (reply->flags & RESOLVE_FLAG_ROUTED) \
	&& strcmp(reply_class, SMTPD_NAME_RECIPIENT) == 0)

    /*
     * Look up user+foo@domain if the address has an extension, user@domain
     * otherwise.
     */
    if ((status = check_access(state, table, CONST_STR(reply->recipient), FULL,
			       found, reply_name, reply_class, def_acl)) != 0
	|| *found)
	CHECK_MAIL_ACCESS_RETURN(status == SMTPD_CHECK_OK
				 && SUSPICIOUS(reply, reply_class) ?
				 SMTPD_CHECK_DUNNO : status);

    /*
     * Try user@domain if the address has an extension.
     */
    if (bare_addr)
	if ((status = check_access(state, table, bare_addr, PARTIAL,
			      found, reply_name, reply_class, def_acl)) != 0
	    || *found)
	    CHECK_MAIL_ACCESS_RETURN(status == SMTPD_CHECK_OK
				     && SUSPICIOUS(reply, reply_class) ?
				     SMTPD_CHECK_DUNNO : status);

    /*
     * Look up the domain name, or parent domains thereof.
     */
    if ((status = check_domain_access(state, table, domain, PARTIAL,
			      found, reply_name, reply_class, def_acl)) != 0
	|| *found)
	CHECK_MAIL_ACCESS_RETURN(status == SMTPD_CHECK_OK
				 && SUSPICIOUS(reply, reply_class) ?
				 SMTPD_CHECK_DUNNO : status);

    /*
     * Look up user+foo@ if the address has an extension, user@ otherwise.
     * XXX This leaks a little memory if map lookup is aborted.
     */
    local_at = mystrndup(CONST_STR(reply->recipient),
			 domain - CONST_STR(reply->recipient));
    status = check_access(state, table, local_at, PARTIAL, found,
			  reply_name, reply_class, def_acl);
    myfree(local_at);
    if (status != 0 || *found)
	CHECK_MAIL_ACCESS_RETURN(status == SMTPD_CHECK_OK
				 && SUSPICIOUS(reply, reply_class) ?
				 SMTPD_CHECK_DUNNO : status);

    /*
     * Look up user@ if the address has an extension. XXX Same problem here.
     */
    if (bare_addr) {
	bare_at = strrchr(bare_addr, '@');
	local_at = (bare_at ? mystrndup(bare_addr, bare_at + 1 - bare_addr) :
		    mystrdup(bare_addr));
	status = check_access(state, table, local_at, PARTIAL, found,
			      reply_name, reply_class, def_acl);
	myfree(local_at);
	if (status != 0 || *found)
	    CHECK_MAIL_ACCESS_RETURN(status == SMTPD_CHECK_OK
				     && SUSPICIOUS(reply, reply_class) ?
				     SMTPD_CHECK_DUNNO : status);
    }

    /*
     * Undecided when no match found.
     */
    CHECK_MAIL_ACCESS_RETURN(SMTPD_CHECK_DUNNO);
}

/* smtpd_expand_unknown - report unknown macro name */

static void smtpd_expand_unknown(const char *name)
{
    msg_warn("unknown macro name \"%s\" in expansion request", name);
}

/* smtpd_expand_addr - return address or substring thereof */

static const char *smtpd_expand_addr(VSTRING *buf, const char *addr,
				           const char *name, int prefix_len)
{
    const char *p;
    const char *suffix;

    /*
     * Return NULL only for unknown names in expansion requests.
     */
    if (addr == 0)
	return ("");

    suffix = name + prefix_len;

    /*
     * MAIL_ATTR_SENDER or MAIL_ATTR_RECIP.
     */
    if (*suffix == 0) {
	if (*addr)
	    return (addr);
	else
	    return ("<>");
    }

    /*
     * "sender_name" or "recipient_name".
     */
#define STREQ(x,y) (*(x) == *(y) && strcmp((x), (y)) == 0)

    else if (STREQ(suffix, MAIL_ATTR_S_NAME)) {
	if (*addr) {
	    if ((p = strrchr(addr, '@')) != 0) {
		vstring_strncpy(buf, addr, p - addr);
		return (STR(buf));
	    } else {
		return (addr);
	    }
	} else
	    return ("<>");
    }

    /*
     * "sender_domain" or "recipient_domain".
     */
    else if (STREQ(suffix, MAIL_ATTR_S_DOMAIN)) {
	if (*addr) {
	    if ((p = strrchr(addr, '@')) != 0) {
		return (p + 1);
	    } else {
		return ("");
	    }
	} else
	    return ("");
    }

    /*
     * Unknown. Return NULL to indicate an "unknown name" error.
     */
    else {
	smtpd_expand_unknown(name);
	return (0);
    }
}

/* smtpd_expand_lookup - generic SMTP attribute $name expansion */

static const char *smtpd_expand_lookup(const char *name, int unused_mode,
				               char *context)
{
    SMTPD_STATE *state = (SMTPD_STATE *) context;

    if (state->expand_buf == 0)
	state->expand_buf = vstring_alloc(10);

    if (msg_verbose > 1)
	msg_info("smtpd_expand_lookup: ${%s}", name);

#define STREQN(x,y,n) (*(x) == *(y) && strncmp((x), (y), (n)) == 0)
#define CONST_LEN(x)  (sizeof(x) - 1)

    /*
     * Don't query main.cf parameters, as the result of expansion could
     * reveal system-internal information in server replies.
     * 
     * Return NULL only for non-existent names.
     */
    if (STREQ(name, MAIL_ATTR_CLIENT)) {
	return (state->namaddr);
    } else if (STREQ(name, MAIL_ATTR_CLIENT_ADDR)) {
	return (state->addr);
    } else if (STREQ(name, MAIL_ATTR_CLIENT_NAME)) {
	return (state->name);
    } else if (STREQ(name, MAIL_ATTR_HELO_NAME)) {
	return (state->helo_name ? state->helo_name : "");
    } else if (STREQN(name, MAIL_ATTR_SENDER, CONST_LEN(MAIL_ATTR_SENDER))) {
	return (smtpd_expand_addr(state->expand_buf, state->sender,
				  name, CONST_LEN(MAIL_ATTR_SENDER)));
    } else if (STREQN(name, MAIL_ATTR_RECIP, CONST_LEN(MAIL_ATTR_RECIP))) {
	return (smtpd_expand_addr(state->expand_buf, state->recipient,
				  name, CONST_LEN(MAIL_ATTR_RECIP)));
    } else {
	smtpd_expand_unknown(name);
	return (0);
    }
}

/* rbl_pagein - look up an RBL lookup result */

static void *rbl_pagein(const char *query, void *unused_context)
{
    DNS_RR *txt_list;
    VSTRING *why;
    int     dns_status;
    SMTPD_RBL_STATE *rbl;

    /*
     * Do the query. If the DNS lookup produces no definitive reply, give the
     * requestor the benefit of the doubt. We can't block all email simply
     * because an RBL server is unavailable.
     */
    why = vstring_alloc(10);
    dns_status = dns_lookup(query, T_A, 0, (DNS_RR **) 0,
			    (VSTRING *) 0, why);
    if (dns_status != DNS_OK && dns_status != DNS_NOTFOUND)
	msg_warn("%s: RBL lookup error: %s", query, STR(why));
    vstring_free(why);
    if (dns_status != DNS_OK)
	return (0);

    /*
     * Save the result. Yes, we cache negative results as well as positive
     * results.
     */
    rbl = (SMTPD_RBL_STATE *) mymalloc(sizeof(*rbl));
    if (dns_lookup(query, T_TXT, 0, &txt_list,
		   (VSTRING *) 0, (VSTRING *) 0) == DNS_OK) {
	rbl->txt = mystrndup(txt_list->data, 512);
	dns_rr_free(txt_list);
    } else
	rbl->txt = mystrdup("");
    return ((void *) rbl);
}

/* rbl_pageout - discard an RBL lookup result */

static void rbl_pageout(void *data, void *unused_context)
{
    SMTPD_RBL_STATE *rbl = (SMTPD_RBL_STATE *) data;

    if (rbl != 0) {
	if (rbl->txt)
	    myfree(rbl->txt);
	myfree((char *) rbl);
    }
}

/* rbl_expand_lookup - RBL specific $name expansion */

static const char *rbl_expand_lookup(const char *name, int mode,
				             char *context)
{
    SMTPD_RBL_EXPAND_CONTEXT *rbl_exp = (SMTPD_RBL_EXPAND_CONTEXT *) context;
    SMTPD_RBL_STATE *rbl = rbl_exp->rbl_state;
    SMTPD_STATE *state = rbl_exp->state;

    if (state->expand_buf == 0)
	state->expand_buf = vstring_alloc(10);

    if (msg_verbose > 1)
	msg_info("rbl_expand_lookup: ${%s}", name);

    /*
     * Be sure to return NULL only for non-existent names.
     */
    if (STREQ(name, MAIL_ATTR_RBL_CODE)) {
	vstring_sprintf(state->expand_buf, "%d", var_maps_rbl_code);
	return (STR(state->expand_buf));
    } else if (STREQ(name, MAIL_ATTR_RBL_DOMAIN)) {
	return (rbl_exp->domain);
    } else if (STREQ(name, MAIL_ATTR_RBL_REASON)) {
	return (rbl->txt);
    } else if (STREQ(name, MAIL_ATTR_RBL_TXT)) {/* LaMont compat */
	return (rbl->txt);
    } else if (STREQ(name, MAIL_ATTR_RBL_WHAT)) {
	return (rbl_exp->what);
    } else if (STREQ(name, MAIL_ATTR_RBL_CLASS)) {
	return (rbl_exp->class);
    } else {
	return (smtpd_expand_lookup(name, mode, (char *) state));
    }
}

/* rbl_reject_reply - format reply after RBL reject */

static int rbl_reject_reply(SMTPD_STATE *state, SMTPD_RBL_STATE *rbl,
			            const char *rbl_domain,
			            const char *what,
			            const char *reply_class)
{
    const char *myname = "rbl_reject_reply";
    VSTRING *why = 0;
    const char *template = 0;
    char   *low_name;
    SMTPD_RBL_EXPAND_CONTEXT rbl_exp;
    int     result;

    /*
     * Use the server-specific reply template or use the default one.
     */
    if (*var_rbl_reply_maps) {
	low_name = lowercase(mystrdup(rbl_domain));
	template = maps_find(rbl_reply_maps, low_name, 0);
	myfree(low_name);
    }
    why = vstring_alloc(100);
    rbl_exp.state = state;
    rbl_exp.rbl_state = rbl;
    rbl_exp.domain = rbl_domain;
    rbl_exp.what = what;
    rbl_exp.class = reply_class;
    for (;;) {
	if (template == 0)
	    template = var_def_rbl_reply;
	if (mac_expand(why, template, MAC_EXP_FLAG_NONE,
		       STR(expand_filter), rbl_expand_lookup,
		       (char *) &rbl_exp) == 0)
	    break;
	if (template == var_def_rbl_reply)
	    msg_fatal("%s: bad default rbl reply template: %s",
		      myname, var_def_rbl_reply);
	msg_warn("%s: bad rbl reply template for domain %s: %s",
		 myname, rbl_domain, template);
	template = 0;				/* pretend not found */
    }
    result = smtpd_check_reject(state, MAIL_ERROR_POLICY, "%s", STR(why));

    /*
     * Clean up.
     */
    vstring_free(why);

    return (result);
}

/* reject_rbl_addr - reject if address in real-time blackhole list */

static int reject_rbl_addr(SMTPD_STATE *state, const char *rbl_domain,
			           const char *addr, const char *reply_class)
{
    char   *myname = "reject_rbl";
    ARGV   *octets;
    VSTRING *query;
    int     i;
    SMTPD_RBL_STATE *rbl;

    if (msg_verbose)
	msg_info("%s: %s %s", myname, reply_class, addr);

    /*
     * IPv4 only for now
     */
#ifdef INET6
    if (inet_pton(AF_INET, addr, &a) != 1)
	return SMTPD_CHECK_DUNNO;
#endif

    /*
     * Reverse the client IPV4 address, tack on the RBL domain name and query
     * the DNS for an A record.
     */
    query = vstring_alloc(100);
    octets = argv_split(addr, ".");
    for (i = octets->argc - 1; i >= 0; i--) {
	vstring_strcat(query, octets->argv[i]);
	vstring_strcat(query, ".");
    }
    argv_free(octets);
    vstring_strcat(query, rbl_domain);
    rbl = (SMTPD_RBL_STATE *) ctable_locate(smtpd_rbl_cache, STR(query));
    vstring_free(query);

    /*
     * If the record exists, the address is blacklisted.
     */
    if (rbl == 0) {
	return (SMTPD_CHECK_DUNNO);
    } else {
	return (rbl_reject_reply(state, rbl, rbl_domain, addr, reply_class));
    }
}

/* reject_rbl_domain - reject if domain in real-time blackhole list */

static int reject_rbl_domain(SMTPD_STATE *state, const char *rbl_domain,
			          const char *what, const char *reply_class)
{
    char   *myname = "reject_rbl_domain";
    VSTRING *query;
    SMTPD_RBL_STATE *rbl;
    const char *domain;

    if (msg_verbose)
	msg_info("%s: %s %s", myname, reply_class, what);

    /*
     * Extract the domain, tack on the RBL domain name and query the DNS for
     * an A record.
     */
    if ((domain = strrchr(what, '@')) != 0) {
	domain += 1;
	if (domain[0] == '[')
	    return (SMTPD_CHECK_DUNNO);
    } else
	domain = what;
    if (domain[0] == 0)
	return (SMTPD_CHECK_DUNNO);

    query = vstring_alloc(100);
    vstring_sprintf(query, "%s.%s", domain, rbl_domain);
    rbl = (SMTPD_RBL_STATE *) ctable_locate(smtpd_rbl_cache, STR(query));
    vstring_free(query);

    /*
     * If the record exists, the domain is blacklisted.
     */
    if (rbl == 0) {
	return (SMTPD_CHECK_DUNNO);
    } else {
	return (rbl_reject_reply(state, rbl, rbl_domain, what, reply_class));
    }
}

/* reject_maps_rbl - reject if client address in real-time blackhole list */

static int reject_maps_rbl(SMTPD_STATE *state)
{
    char   *myname = "reject_maps_rbl";
    char   *saved_domains = mystrdup(var_maps_rbl_domains);
    char   *bp = saved_domains;
    char   *rbl_domain;
    int     result = SMTPD_CHECK_DUNNO;
    static int warned;

    if (msg_verbose)
	msg_info("%s: %s", myname, state->addr);

    if (warned == 0) {
	warned++;
	msg_warn("restriction %s is going away. Please use %s <domain> instead",
		 REJECT_MAPS_RBL, REJECT_RBL_CLIENT);
    }
    while ((rbl_domain = mystrtok(&bp, " \t\r\n,")) != 0) {
	result = reject_rbl_addr(state, rbl_domain, state->addr,
				 SMTPD_NAME_CLIENT);
	if (result != SMTPD_CHECK_DUNNO)
	    break;
    }

    /*
     * Clean up.
     */
    myfree(saved_domains);

    return (result);
}

/* reject_sender_login_mismatch - reject login/sender ownership mismatch */

static int reject_sender_login_mismatch(SMTPD_STATE *state, const char *sender)
{
    const RESOLVE_REPLY *reply;
    const char *login = 0;
    const char *owner = 0;

    /*
     * If the sender address is owned by a login name, or if the client has
     * logged in, then require that the client is logged in as the owner of
     * the sender address.
     */
    reply = (const RESOLVE_REPLY *) ctable_locate(smtpd_resolve_cache, sender);
    if (reply->flags & RESOLVE_FLAG_FAIL)
	reject_dict_retry(state, sender);
    owner = check_mail_addr_find(state, sender, smtpd_sender_login_maps,
				 STR(reply->recipient), (char **) 0);
#ifdef USE_SASL_AUTH
    if (var_smtpd_sasl_enable && state->sasl_username != 0)
	login = state->sasl_username;
#endif
    if (login) {
	if (owner == 0 || strcasecmp(login, owner) != 0)
	    return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
		  "553 <%s>: Sender address rejected: not owned by user %s",
				       sender, login));
    } else {
	if (owner)
	    return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
		"553 <%s>: Sender address rejected: not logged in as owner",
				       sender));
    }
    return (SMTPD_CHECK_DUNNO);
}

/* is_map_command - restriction has form: check_xxx_access type:name */

static int is_map_command(SMTPD_STATE *state, const char *name,
			          const char *command, char ***argp)
{

    /*
     * This is a three-valued function: (a) this is not a check_xxx_access
     * command, (b) this is a malformed check_xxx_access command, (c) this is
     * a well-formed check_xxx_access command. That's too clumsy for function
     * result values, so we use regular returns for (a) and (c), and use long
     * jumps for the error case (b).
     */
    if (strcasecmp(name, command) != 0) {
	return (0);
    } else if (*(*argp + 1) == 0 || strchr(*(*argp += 1), ':') == 0) {
	msg_warn("restriction %s requires maptype:mapname", command);
	longjmp(smtpd_check_buf, smtpd_check_reject(state, MAIL_ERROR_SOFTWARE,
					 "451 Server configuration error"));
    } else {
	return (1);
    }
}

/* generic_checks - generic restrictions */

static int generic_checks(SMTPD_STATE *state, ARGV *restrictions,
			          const char *reply_name,
			          const char *reply_class,
			          const char *def_acl)
{
    char   *myname = "generic_checks";
    char  **cpp;
    const char *name;
    int     status = 0;
    ARGV   *list;
    int     found;
    int     saved_recursion = state->recursion++;

    if (msg_verbose)
	msg_info("%s: START", myname);

    for (cpp = restrictions->argv; (name = *cpp) != 0; cpp++) {

	if (msg_verbose)
	    msg_info("%s: name=%s", myname, name);

	/*
	 * Pseudo restrictions.
	 */
	if (strcasecmp(name, WARN_IF_REJECT) == 0) {
	    if (state->warn_if_reject == 0)
		state->warn_if_reject = state->recursion;
	    continue;
	}

	/*
	 * Spoof the is_map_command() routine, so that we do not have to make
	 * special cases for the implicit short-hand access map notation.
	 */
#define NO_DEF_ACL	0

	if (strchr(name, ':') != 0) {
	    if (def_acl == NO_DEF_ACL) {
		msg_warn("specify one of (%s, %s, %s, %s, %s) before %s restriction \"%s\"",
			 CHECK_CLIENT_ACL, CHECK_HELO_ACL, CHECK_SENDER_ACL,
			 CHECK_RECIP_ACL, CHECK_ETRN_ACL, reply_class, name);
		longjmp(smtpd_check_buf, smtpd_check_reject(state,
		    MAIL_ERROR_SOFTWARE, "451 Server configuration error"));
	    }
	    name = def_acl;
	    cpp -= 1;
	}

	/*
	 * Generic restrictions.
	 */
	if (strcasecmp(name, PERMIT_ALL) == 0) {
	    status = SMTPD_CHECK_OK;
	    if (cpp[1] != 0 && state->warn_if_reject == 0)
		msg_warn("restriction `%s' after `%s' is ignored",
			 cpp[1], PERMIT_ALL);
	} else if (strcasecmp(name, DEFER_ALL) == 0) {
	    status = smtpd_check_reject(state, MAIL_ERROR_POLICY,
				    "%d <%s>: %s rejected: Try again later",
				   var_defer_code, reply_name, reply_class);
	    if (cpp[1] != 0 && state->warn_if_reject == 0)
		msg_warn("restriction `%s' after `%s' is ignored",
			 cpp[1], DEFER_ALL);
	} else if (strcasecmp(name, REJECT_ALL) == 0) {
	    status = smtpd_check_reject(state, MAIL_ERROR_POLICY,
				      "%d <%s>: %s rejected: Access denied",
				  var_reject_code, reply_name, reply_class);
	    if (cpp[1] != 0 && state->warn_if_reject == 0)
		msg_warn("restriction `%s' after `%s' is ignored",
			 cpp[1], REJECT_ALL);
	} else if (strcasecmp(name, REJECT_UNAUTH_PIPE) == 0) {
	    status = reject_unauth_pipelining(state, reply_name, reply_class);
	} else if (strcasecmp(name, DEFER_IF_PERMIT) == 0) {
	    DEFER_IF_PERMIT2(state, MAIL_ERROR_POLICY,
			 "450 <%s>: %s rejected: defer_if_permit requested",
			     reply_name, reply_class);
	} else if (strcasecmp(name, DEFER_IF_REJECT) == 0) {
	    DEFER_IF_REJECT2(state, MAIL_ERROR_POLICY,
			 "450 <%s>: %s rejected: defer_if_reject requested",
			     reply_name, reply_class);
	}

	/*
	 * Client name/address restrictions.
	 */
	else if (strcasecmp(name, REJECT_UNKNOWN_CLIENT) == 0) {
	    status = reject_unknown_client(state);
	} else if (strcasecmp(name, PERMIT_MYNETWORKS) == 0) {
	    status = permit_mynetworks(state);
	} else if (is_map_command(state, name, CHECK_CLIENT_ACL, &cpp)) {
	    status = check_namadr_access(state, *cpp, state->name, state->addr,
					 FULL, &found, state->namaddr,
					 SMTPD_NAME_CLIENT, def_acl);
	} else if (strcasecmp(name, REJECT_MAPS_RBL) == 0) {
	    status = reject_maps_rbl(state);
	} else if (strcasecmp(name, REJECT_RBL_CLIENT) == 0
		   || strcasecmp(name, REJECT_RBL) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument", name);
	    else
		status = reject_rbl_addr(state, *(cpp += 1), state->addr,
					 SMTPD_NAME_CLIENT);
	} else if (strcasecmp(name, REJECT_RHSBL_CLIENT) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument",
			 name);
	    else {
		cpp += 1;
		if (strcasecmp(state->name, "unknown") != 0)
		    status = reject_rbl_domain(state, *cpp, state->name,
					       SMTPD_NAME_CLIENT);
	    }
	}

	/*
	 * HELO/EHLO parameter restrictions.
	 */
	else if (is_map_command(state, name, CHECK_HELO_ACL, &cpp)) {
	    if (state->helo_name)
		status = check_domain_access(state, *cpp, state->helo_name,
					     FULL, &found, state->helo_name,
					     SMTPD_NAME_HELO, def_acl);
	} else if (strcasecmp(name, REJECT_INVALID_HOSTNAME) == 0) {
	    if (state->helo_name) {
		if (*state->helo_name != '[')
		    status = reject_invalid_hostname(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
		else
		    status = reject_invalid_hostaddr(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
	    }
	} else if (strcasecmp(name, REJECT_UNKNOWN_HOSTNAME) == 0) {
	    if (state->helo_name) {
		if (*state->helo_name != '[')
		    status = reject_unknown_hostname(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
		else
		    status = reject_invalid_hostaddr(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
	    }
	} else if (strcasecmp(name, PERMIT_NAKED_IP_ADDR) == 0) {
	    msg_warn("restriction %s is deprecated. Use %s instead",
		     PERMIT_NAKED_IP_ADDR, PERMIT_MYNETWORKS);
	    if (state->helo_name) {
		if (state->helo_name[strspn(state->helo_name, "0123456789.")] == 0
		&& (status = reject_invalid_hostaddr(state, state->helo_name,
				   state->helo_name, SMTPD_NAME_HELO)) == 0)
		    status = SMTPD_CHECK_OK;
	    }
	} else if (strcasecmp(name, REJECT_NON_FQDN_HOSTNAME) == 0) {
	    if (state->helo_name) {
		if (*state->helo_name != '[')
		    status = reject_non_fqdn_hostname(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
		else
		    status = reject_invalid_hostaddr(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
	    }
	}

	/*
	 * Sender mail address restrictions.
	 */
	else if (is_map_command(state, name, CHECK_SENDER_ACL, &cpp)) {
	    if (state->sender && *state->sender)
		status = check_mail_access(state, *cpp, state->sender,
					   &found, state->sender,
					   SMTPD_NAME_SENDER, def_acl);
	    if (state->sender && !*state->sender)
		status = check_access(state, *cpp, var_smtpd_null_key, FULL,
				      &found, state->sender,
				      SMTPD_NAME_SENDER, def_acl);
	} else if (strcasecmp(name, REJECT_UNKNOWN_ADDRESS) == 0) {
	    if (state->sender && *state->sender)
		status = reject_unknown_address(state, state->sender,
					  state->sender, SMTPD_NAME_SENDER);
	} else if (strcasecmp(name, REJECT_UNKNOWN_SENDDOM) == 0) {
	    if (state->sender && *state->sender)
		status = reject_unknown_address(state, state->sender,
					  state->sender, SMTPD_NAME_SENDER);
	} else if (strcasecmp(name, REJECT_NON_FQDN_SENDER) == 0) {
	    if (state->sender && *state->sender)
		status = reject_non_fqdn_address(state, state->sender,
					  state->sender, SMTPD_NAME_SENDER);
	} else if (strcasecmp(name, REJECT_SENDER_LOGIN_MISMATCH) == 0) {
	    if (state->sender && *state->sender)
		status = reject_sender_login_mismatch(state, state->sender);
	} else if (strcasecmp(name, REJECT_RHSBL_SENDER) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument", name);
	    else {
		cpp += 1;
		if (state->sender && *state->sender)
		    status = reject_rbl_domain(state, *cpp, state->sender,
					       SMTPD_NAME_SENDER);
	    }
	}

	/*
	 * Recipient mail address restrictions.
	 */
	else if (is_map_command(state, name, CHECK_RECIP_ACL, &cpp)) {
	    if (state->recipient)
		status = check_mail_access(state, *cpp, state->recipient,
					   &found, state->recipient,
					   SMTPD_NAME_RECIPIENT, def_acl);
	} else if (strcasecmp(name, PERMIT_MX_BACKUP) == 0) {
	    if (state->recipient)
		status = permit_mx_backup(state, state->recipient,
				    state->recipient, SMTPD_NAME_RECIPIENT);
	} else if (strcasecmp(name, PERMIT_AUTH_DEST) == 0) {
	    if (state->recipient)
		status = permit_auth_destination(state, state->recipient);
	} else if (strcasecmp(name, REJECT_UNAUTH_DEST) == 0) {
	    if (state->recipient)
		status = reject_unauth_destination(state, state->recipient);
	} else if (strcasecmp(name, CHECK_RELAY_DOMAINS) == 0) {
	    if (state->recipient)
		status = check_relay_domains(state, state->recipient,
				    state->recipient, SMTPD_NAME_RECIPIENT);
	    if (cpp[1] != 0 && state->warn_if_reject == 0)
		msg_warn("restriction `%s' after `%s' is ignored",
			 cpp[1], CHECK_RELAY_DOMAINS);
	} else if (strcasecmp(name, PERMIT_SASL_AUTH) == 0) {
	    if (var_smtpd_sasl_enable)
#ifdef USE_SASL_AUTH
		status = permit_sasl_auth(state,
					  SMTPD_CHECK_OK, SMTPD_CHECK_DUNNO);
#else
		msg_warn("restriction `%s' ignored: no SASL support", name);
#endif
	} else if (strcasecmp(name, REJECT_UNKNOWN_RCPTDOM) == 0) {
	    if (state->recipient)
		status = reject_unknown_address(state, state->recipient,
				    state->recipient, SMTPD_NAME_RECIPIENT);
	} else if (strcasecmp(name, REJECT_NON_FQDN_RCPT) == 0) {
	    if (state->recipient)
		status = reject_non_fqdn_address(state, state->recipient,
				    state->recipient, SMTPD_NAME_RECIPIENT);
	} else if (strcasecmp(name, REJECT_RHSBL_RECIPIENT) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument", name);
	    else {
		cpp += 1;
		if (state->recipient)
		    status = reject_rbl_domain(state, *cpp, state->recipient,
					       SMTPD_NAME_RECIPIENT);
	    }
	} else if (strcasecmp(name, CHECK_RCPT_MAPS) == 0) {
	    if (state->recipient && *state->recipient)
		status = check_rcpt_maps(state, state->recipient);
	}

	/*
	 * ETRN domain name restrictions.
	 */
	else if (is_map_command(state, name, CHECK_ETRN_ACL, &cpp)) {
	    if (state->etrn_name)
		status = check_domain_access(state, *cpp, state->etrn_name,
					     FULL, &found, state->etrn_name,
					     SMTPD_NAME_ETRN, def_acl);
	}

	/*
	 * User-defined restriction class.
	 */
	else if ((list = (ARGV *) htable_find(smtpd_rest_classes, name)) != 0) {
	    status = generic_checks(state, list, reply_name,
				    reply_class, def_acl);
	}

	/*
	 * Error: undefined restriction name.
	 */
	else {
	    msg_warn("unknown smtpd restriction: \"%s\"", name);
	    longjmp(smtpd_check_buf, smtpd_check_reject(state,
		    MAIL_ERROR_SOFTWARE, "451 Server configuration error"));
	}
	if (msg_verbose)
	    msg_info("%s: name=%s status=%d", myname, name, status);

	if (state->warn_if_reject >= state->recursion)
	    state->warn_if_reject = 0;

	if (status != 0)
	    break;

	if (state->defer_if_permit.active && state->defer_if_reject.active)
	    break;
    }
    if (msg_verbose && name == 0)
	msg_info("%s: END", myname);

    state->recursion = saved_recursion;

    return (status);
}

/* smtpd_check_client - validate client name or address */

char   *smtpd_check_client(SMTPD_STATE *state)
{
    int     status;

    /*
     * Initialize.
     */
    if (state->name == 0 || state->addr == 0)
	return (0);

#define SMTPD_CHECK_RESET() { \
	state->recursion = 0; \
	state->warn_if_reject = 0; \
	state->defer_if_reject.active = 0; \
    }

    /*
     * Reset the defer_if_permit flag.
     */
    state->defer_if_permit.active = 0;

    /*
     * Apply restrictions in the order as specified.
     */
    SMTPD_CHECK_RESET();
    status = setjmp(smtpd_check_buf);
    if (status == 0 && client_restrctions->argc)
	status = generic_checks(state, client_restrctions, state->namaddr,
				SMTPD_NAME_CLIENT, CHECK_CLIENT_ACL);
    state->defer_if_permit_client = state->defer_if_permit.active;

    return (status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);
}

/* smtpd_check_helo - validate HELO hostname */

char   *smtpd_check_helo(SMTPD_STATE *state, char *helohost)
{
    int     status;
    char   *saved_helo;

    /*
     * Initialize.
     */
    if (helohost == 0)
	return (0);

    /*
     * Minor kluge so that we can delegate work to the generic routine and so
     * that we can syslog the recipient with the reject messages.
     */
#define SMTPD_CHECK_PUSH(backup, current, new) { \
	backup = current; \
	current = (new ? mystrdup(new) : 0); \
    }

#define SMTPD_CHECK_POP(current, backup) { \
	if (current) myfree(current); \
	current = backup; \
    }

    SMTPD_CHECK_PUSH(saved_helo, state->helo_name, helohost);

#define SMTPD_CHECK_HELO_RETURN(x) { \
	SMTPD_CHECK_POP(state->helo_name, saved_helo); \
	return (x); \
    }

    /*
     * Restore the defer_if_permit flag to its value before HELO/EHLO, and do
     * not set the flag when it was already raised by a previous protocol
     * stage.
     */
    state->defer_if_permit.active = state->defer_if_permit_client;

    /*
     * Apply restrictions in the order as specified.
     */
    SMTPD_CHECK_RESET();
    status = setjmp(smtpd_check_buf);
    if (status == 0 && helo_restrctions->argc)
	status = generic_checks(state, helo_restrctions, state->helo_name,
				SMTPD_NAME_HELO, CHECK_HELO_ACL);
    state->defer_if_permit_helo = state->defer_if_permit.active;

    SMTPD_CHECK_HELO_RETURN(status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);
}

/* smtpd_check_mail - validate sender address, driver */

char   *smtpd_check_mail(SMTPD_STATE *state, char *sender)
{
    int     status;
    char   *saved_sender;

    /*
     * Initialize.
     */
    if (sender == 0)
	return (0);

    /*
     * Minor kluge so that we can delegate work to the generic routine and so
     * that we can syslog the recipient with the reject messages.
     */
    SMTPD_CHECK_PUSH(saved_sender, state->sender, sender);

#define SMTPD_CHECK_MAIL_RETURN(x) { \
	SMTPD_CHECK_POP(state->sender, saved_sender); \
	return (x); \
    }

    /*
     * Restore the defer_if_permit flag to its value before MAIL FROM, and do
     * not set the flag when it was already raised by a previous protocol
     * stage. The client may skip the helo/ehlo.
     */
    state->defer_if_permit.active = state->defer_if_permit_client
	| state->defer_if_permit_helo;

    /*
     * Apply restrictions in the order as specified.
     */
    SMTPD_CHECK_RESET();
    status = setjmp(smtpd_check_buf);
    if (status == 0 && mail_restrctions->argc)
	status = generic_checks(state, mail_restrctions, sender,
				SMTPD_NAME_SENDER, CHECK_SENDER_ACL);
    state->defer_if_permit_sender = state->defer_if_permit.active;

    SMTPD_CHECK_MAIL_RETURN(status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);
}

/* smtpd_check_rcpt - validate recipient address, driver */

char   *smtpd_check_rcpt(SMTPD_STATE *state, char *recipient)
{
    int     status;
    char   *saved_recipient;
    char   *err;

    /*
     * Initialize.
     */
    if (recipient == 0)
	return (0);

    /*
     * XXX 2821: Section 3.6 requires that "postmaster" be accepted even when
     * specified without a fully qualified domain name.
     */
    if (strcasecmp(recipient, "postmaster") == 0)
	return (0);

    /*
     * Minor kluge so that we can delegate work to the generic routine and so
     * that we can syslog the recipient with the reject messages.
     */
    SMTPD_CHECK_PUSH(saved_recipient, state->recipient, recipient);

#define SMTPD_CHECK_RCPT_RETURN(x) { \
	SMTPD_CHECK_POP(state->recipient, saved_recipient); \
	return (x); \
    }

    /*
     * The "check_recipient_maps" restriction is relevant only when
     * responding to RCPT TO. It's effectively disabled with DATA (recipient
     * context is explicitly turned off) and not applicable with undelayed
     * client/helo/sender restrictions (no recipient info) or with ETRN
     * (command not allowed in the middle of an ongoing MAIL transaction).
     */
    state->rcptmap_checked = 0;

    /*
     * Apply delayed restrictions.
     */
    if (var_smtpd_delay_reject)
	if ((err = smtpd_check_client(state)) != 0
	    || (err = smtpd_check_helo(state, state->helo_name)) != 0
	    || (err = smtpd_check_mail(state, state->sender)) != 0)
	    SMTPD_CHECK_RCPT_RETURN(err);

    /*
     * Restore the defer_if_permit flag to its value before RCPT TO, and do
     * not set the flag when it was already raised by a previous protocol
     * stage.
     */
    state->defer_if_permit.active = state->defer_if_permit_sender;

    /*
     * Apply restrictions in the order as specified.
     */
    SMTPD_CHECK_RESET();
    status = setjmp(smtpd_check_buf);
    if (status == 0 && rcpt_restrctions->argc)
	    status = generic_checks(state, rcpt_restrctions,
			  recipient, SMTPD_NAME_RECIPIENT, CHECK_RECIP_ACL);

    /*
     * Force permission into deferral when some earlier temporary error may
     * have prevented us from rejecting mail, and report the earlier problem.
     */
    if (status != SMTPD_CHECK_REJECT && state->defer_if_permit.active)
	status = smtpd_check_reject(state, state->defer_if_permit.class,
				  "%s", STR(state->defer_if_permit.reason));

    SMTPD_CHECK_RCPT_RETURN(status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);
}

/* smtpd_check_etrn - validate ETRN request */

char   *smtpd_check_etrn(SMTPD_STATE *state, char *domain)
{
    int     status;
    char   *saved_etrn_name;
    char   *err;

    /*
     * Initialize.
     */
    if (domain == 0)
	return (0);

    /*
     * Minor kluge so that we can delegate work to the generic routine and so
     * that we can syslog the recipient with the reject messages.
     */
    SMTPD_CHECK_PUSH(saved_etrn_name, state->etrn_name, domain);

#define SMTPD_CHECK_ETRN_RETURN(x) { \
	SMTPD_CHECK_POP(state->etrn_name, saved_etrn_name); \
	return (x); \
    }

    /*
     * Apply delayed restrictions.
     */
    if (var_smtpd_delay_reject)
	if ((err = smtpd_check_client(state)) != 0
	    || (err = smtpd_check_helo(state, state->helo_name)) != 0)
	    SMTPD_CHECK_ETRN_RETURN(err);

    /*
     * Restore the defer_if_permit flag to its value before ETRN, and do not
     * set the flag when it was already raised by a previous protocol stage.
     * The client may skip the helo/ehlo.
     */
    state->defer_if_permit.active = state->defer_if_permit_client
	| state->defer_if_permit_helo;

    /*
     * Apply restrictions in the order as specified.
     */
    SMTPD_CHECK_RESET();
    status = setjmp(smtpd_check_buf);
    if (status == 0 && etrn_restrctions->argc)
	status = generic_checks(state, etrn_restrctions, domain,
				SMTPD_NAME_ETRN, CHECK_ETRN_ACL);

    /*
     * Force permission into deferral when some earlier temporary error may
     * have prevented us from rejecting mail, and report the earlier problem.
     */
    if (status != SMTPD_CHECK_REJECT && state->defer_if_permit.active)
	status = smtpd_check_reject(state, state->defer_if_permit.class,
				  "%s", STR(state->defer_if_permit.reason));

    SMTPD_CHECK_ETRN_RETURN(status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);
}

/* smtpd_check_rcptmap - permit if recipient address matches lookup table */

char   *smtpd_check_rcptmap(SMTPD_STATE *state, char *recipient)
{
    char   *myname = "smtpd_check_rcptmap";
    int     status;

    if (msg_verbose)
	msg_info("%s: %s", myname, recipient);

    /*
     * Return here in case of serious trouble.
     */
    if ((status = setjmp(smtpd_check_buf)) == 0)
	status = check_rcpt_maps(state, recipient);

    return (status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);
}

/* check_rcpt_maps - generic_checks() interface for recipient table check */

static int check_rcpt_maps(SMTPD_STATE *state, const char *recipient)
{
    const RESOLVE_REPLY *reply;

    /*
     * Duplicate suppression. There's an implicit check_recipient_maps
     * restriction at the end of all recipient restrictions.
     */
    if (state->rcptmap_checked == 1)
	return (0);
    state->rcptmap_checked = 1;

    /*
     * Resolve the address.
     */
    reply = (const RESOLVE_REPLY *)
	ctable_locate(smtpd_resolve_cache, recipient);
    if (reply->flags & RESOLVE_FLAG_FAIL)
	reject_dict_retry(state, recipient);

    /*
     * Make complex expressions more readable?
     */
#define MATCH(map, rcpt) \
    check_mail_addr_find(state, recipient, map, rcpt, (char **) 0)

#define NOMATCH(map, rcpt) (MATCH(map, rcpt) == 0)

#define NOMATCHV8(map, rcpt) \
    (checkv8_maps_find(state, recipient, map, rcpt) == 0)

    /*
     * XXX We assume the recipient address is OK if it matches a canonical
     * map or virtual alias map. Eventually, the address resolver should give
     * us the final resolved recipient address, and the SMTP server should
     * write the final resolved recipient address to the output record
     * stream. See also the next comment block on recipients in virtual alias
     * domains.
     */
    if (MATCH(rcpt_canon_maps, CONST_STR(reply->recipient))
	|| MATCH(canonical_maps, CONST_STR(reply->recipient))
	|| MATCH(virt_alias_maps, CONST_STR(reply->recipient)))
	return (0);

    /*
     * At this point, anything that resolves to the error mailer is known to
     * be undeliverable.
     * 
     * XXX Until the address resolver does final address resolution, known and
     * unknown recipients in virtual alias domains will both resolve to
     * "error:user unknown".
     */
    if (strcmp(STR(reply->transport), MAIL_SERVICE_ERROR) == 0)
	return (smtpd_check_reject(state, MAIL_ERROR_BOUNCE,
				   "%d <%s>: %s",
				   (reply->flags & RESOLVE_CLASS_ALIAS) ?
				   var_virt_alias_code : 550,
				   recipient, STR(reply->nexthop)));

    /*
     * Reject mail to unknown addresses in local domains (domains that match
     * $mydestination or $inet_interfaces).
     * 
     * XXX Use the less expensive maps_find() (case is already folded) instead
     * of the baroque mail_addr_find(). But then we have to strip the domain
     * and deal with address extensions ourselves.
     * 
     * XXX But that would break sites that use the virtual delivery agent for
     * local delivery, because the virtual delivery agent requires
     * user@domain style addresses in its user database.
     */
#define MATCH_LEFT(l, r, n) (strncasecmp((l), (r), (n)) == 0 && (r)[n] == '@')

    if ((reply->flags & RESOLVE_CLASS_LOCAL)
	&& *var_local_rcpt_maps
	/* Generated by bounce, absorbed by qmgr. */
	&& !MATCH_LEFT(var_double_bounce_sender, CONST_STR(reply->recipient),
		       strlen(var_double_bounce_sender))
	/* Absorbed by qmgr. */
	&& !MATCH_LEFT(MAIL_ADDR_POSTMASTER, CONST_STR(reply->recipient),
		       strlen(MAIL_ADDR_POSTMASTER))
	/* Generated by bounce. */
	&& !MATCH_LEFT(MAIL_ADDR_MAIL_DAEMON, CONST_STR(reply->recipient),
		       strlen(MAIL_ADDR_MAIL_DAEMON))
	&& NOMATCH(local_rcpt_maps, CONST_STR(reply->recipient)))
	return (smtpd_check_reject(state, MAIL_ERROR_BOUNCE,
				   "%d <%s>: User unknown%s",
				   var_local_rcpt_code, recipient,
				   var_show_unk_rcpt_table ?
				   " in local recipient table" : ""));

    /*
     * Reject mail to unknown addresses in virtual mailbox domains.
     */
    if ((reply->flags & RESOLVE_CLASS_VIRTUAL)
	&& *var_virt_mailbox_maps
	&& NOMATCHV8(virt_mailbox_maps, CONST_STR(reply->recipient)))
	return (smtpd_check_reject(state, MAIL_ERROR_BOUNCE,
				   "%d <%s>: User unknown%s",
				   var_virt_mailbox_code, recipient,
				   var_show_unk_rcpt_table ?
				   " in virtual mailbox table" : ""));

    /*
     * Reject mail to unknown addresses in relay domains.
     */
    if ((reply->flags & RESOLVE_CLASS_RELAY)
	&& *var_relay_rcpt_maps
	&& NOMATCH(relay_rcpt_maps, CONST_STR(reply->recipient)))
	return (smtpd_check_reject(state, MAIL_ERROR_BOUNCE,
				   "%d <%s>: User unknown%s",
				   var_relay_rcpt_code, recipient,
				   var_show_unk_rcpt_table ?
				   " in relay recipient table" : ""));

    /*
     * Accept all other addresses - including addresses that passed the above
     * tests because of some table lookup problem.
     */
    return (0);
}

/* smtpd_check_size - check optional SIZE parameter value */

char   *smtpd_check_size(SMTPD_STATE *state, off_t size)
{
    char   *myname = "smtpd_check_size";
    struct fsspace fsbuf;
    int     status;

    /*
     * Return here in case of serious trouble.
     */
    SMTPD_CHECK_RESET();
    if ((status = setjmp(smtpd_check_buf)) != 0)
	return (status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);

    /*
     * Avoid overflow/underflow when comparing message size against available
     * space.
     */
#define BLOCKS(x)	((x) / fsbuf.block_size)

    if (var_message_limit > 0 && size > var_message_limit) {
	(void) smtpd_check_reject(state, MAIL_ERROR_POLICY,
				  "552 Message size exceeds fixed limit");
	return (STR(error_text));
    }
    fsspace(".", &fsbuf);
    if (msg_verbose)
	msg_info("%s: blocks %lu avail %lu min_free %lu size %lu",
		 myname,
		 (unsigned long) fsbuf.block_size,
		 (unsigned long) fsbuf.block_free,
		 (unsigned long) var_queue_minfree,
		 (unsigned long) size);
    if (BLOCKS(var_queue_minfree) >= fsbuf.block_free
	|| BLOCKS(size) >= fsbuf.block_free - BLOCKS(var_queue_minfree)
	|| BLOCKS(size) >= fsbuf.block_free / 2) {
	(void) smtpd_check_reject(state, MAIL_ERROR_RESOURCE,
				  "452 Insufficient system storage");
	return (STR(error_text));
    }
    return (0);
}

/* smtpd_check_data - check DATA command */

char   *smtpd_check_data(SMTPD_STATE *state)
{
    int     status;
    char   *saved_recipient;

    /*
     * Minor kluge so that we can delegate work to the generic routine. We
     * provide no recipient information in the case of multiple recipients,
     * This restriction applies to all recipients alike, and logging only one
     * of them would be misleading.
     */
    if (state->rcpt_count > 1) {
	saved_recipient = state->recipient;
	state->recipient = 0;
    }

    /*
     * Reset the defer_if_permit flag. This is necessary when some recipients
     * were accepted but the last one was rejected.
     */
    state->defer_if_permit.active = 0;

    /*
     * Apply restrictions in the order as specified.
     * 
     * XXX We cannot specify a default target for a bare access map.
     */
    SMTPD_CHECK_RESET();
    status = setjmp(smtpd_check_buf);
    if (status == 0 && data_restrctions->argc)
	status = generic_checks(state, data_restrctions,
				"DATA", SMTPD_NAME_DATA, NO_DEF_ACL);

    /*
     * Force permission into deferral when some earlier temporary error may
     * have prevented us from rejecting mail, and report the earlier problem.
     */
    if (status != SMTPD_CHECK_REJECT && state->defer_if_permit.active)
	status = smtpd_check_reject(state, state->defer_if_permit.class,
				  "%s", STR(state->defer_if_permit.reason));

    if (state->rcpt_count > 1)
	state->recipient = saved_recipient;

    return (status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);
}

#ifdef TEST

 /*
  * Test program to try out all these restrictions without having to go live.
  * This is not entirely stand-alone, as it requires access to the Postfix
  * rewrite/resolve service. This is just for testing code, not for debugging
  * configuration files.
  */
#include <stdlib.h>

#include <msg_vstream.h>
#include <vstring_vstream.h>

#include <mail_conf.h>

#include <smtpd_chat.h>

 /*
  * Dummies. These are never set.
  */
char   *var_client_checks = "";
char   *var_helo_checks = "";
char   *var_mail_checks = "";
char   *var_rcpt_checks = "";
char   *var_etrn_checks = "";
char   *var_data_checks = "";
char   *var_relay_domains = "";
char   *var_mynetworks = "";
char   *var_notify_classes = "";

 /*
  * String-valued configuration parameters.
  */
char   *var_maps_rbl_domains;
char   *var_myorigin;
char   *var_mydest;
char   *var_inet_interfaces;
char   *var_rcpt_delim;
char   *var_rest_classes;
char   *var_alias_maps;
char   *var_rcpt_canon_maps;
char   *var_canonical_maps;
char   *var_virt_alias_maps;
char   *var_virt_alias_doms;
char   *var_virt_mailbox_maps;
char   *var_virt_mailbox_doms;
char   *var_local_rcpt_maps;
char   *var_perm_mx_networks;
char   *var_par_dom_match;
char   *var_smtpd_null_key;
char   *var_smtpd_snd_auth_maps;
char   *var_double_bounce_sender;
char   *var_rbl_reply_maps;
char   *var_smtpd_exp_filter;
char   *var_def_rbl_reply;
char   *var_relay_rcpt_maps;

typedef struct {
    char   *name;
    char   *defval;
    char  **target;
} STRING_TABLE;

#undef DEF_LOCAL_RCPT_MAPS
#define DEF_LOCAL_RCPT_MAPS		""

#undef DEF_VIRT_ALIAS_MAPS
#define DEF_VIRT_ALIAS_MAPS	""

static STRING_TABLE string_table[] = {
    VAR_MAPS_RBL_DOMAINS, DEF_MAPS_RBL_DOMAINS, &var_maps_rbl_domains,
    VAR_MYORIGIN, DEF_MYORIGIN, &var_myorigin,
    VAR_MYDEST, DEF_MYDEST, &var_mydest,
    VAR_INET_INTERFACES, DEF_INET_INTERFACES, &var_inet_interfaces,
    VAR_RCPT_DELIM, DEF_RCPT_DELIM, &var_rcpt_delim,
    VAR_REST_CLASSES, DEF_REST_CLASSES, &var_rest_classes,
    VAR_ALIAS_MAPS, DEF_ALIAS_MAPS, &var_alias_maps,
    VAR_RCPT_CANON_MAPS, DEF_RCPT_CANON_MAPS, &var_rcpt_canon_maps,
    VAR_CANONICAL_MAPS, DEF_CANONICAL_MAPS, &var_canonical_maps,
    VAR_VIRT_ALIAS_MAPS, DEF_VIRT_ALIAS_MAPS, &var_virt_alias_maps,
    VAR_VIRT_ALIAS_DOMS, DEF_VIRT_ALIAS_DOMS, &var_virt_alias_doms,
    VAR_VIRT_MAILBOX_MAPS, DEF_VIRT_MAILBOX_MAPS, &var_virt_mailbox_maps,
    VAR_VIRT_MAILBOX_DOMS, DEF_VIRT_MAILBOX_DOMS, &var_virt_mailbox_doms,
    VAR_LOCAL_RCPT_MAPS, DEF_LOCAL_RCPT_MAPS, &var_local_rcpt_maps,
    VAR_PERM_MX_NETWORKS, DEF_PERM_MX_NETWORKS, &var_perm_mx_networks,
    VAR_PAR_DOM_MATCH, DEF_PAR_DOM_MATCH, &var_par_dom_match,
    VAR_SMTPD_SND_AUTH_MAPS, DEF_SMTPD_SND_AUTH_MAPS, &var_smtpd_snd_auth_maps,
    VAR_SMTPD_NULL_KEY, DEF_SMTPD_NULL_KEY, &var_smtpd_null_key,
    VAR_DOUBLE_BOUNCE, DEF_DOUBLE_BOUNCE, &var_double_bounce_sender,
    VAR_RBL_REPLY_MAPS, DEF_RBL_REPLY_MAPS, &var_rbl_reply_maps,
    VAR_SMTPD_EXP_FILTER, DEF_SMTPD_EXP_FILTER, &var_smtpd_exp_filter,
    VAR_DEF_RBL_REPLY, DEF_DEF_RBL_REPLY, &var_def_rbl_reply,
    VAR_RELAY_RCPT_MAPS, DEF_RELAY_RCPT_MAPS, &var_relay_rcpt_maps,
    0,
};

/* string_init - initialize string parameters */

static void string_init(void)
{
    STRING_TABLE *sp;

    for (sp = string_table; sp->name; sp++)
	sp->target[0] = mystrdup(sp->defval);
}

/* string_update - update string parameter */

static int string_update(char **argv)
{
    STRING_TABLE *sp;

    for (sp = string_table; sp->name; sp++) {
	if (strcasecmp(argv[0], sp->name) == 0) {
	    myfree(sp->target[0]);
	    sp->target[0] = mystrdup(argv[1]);
	    return (1);
	}
    }
    return (0);
}

 /*
  * Integer parameters.
  */
int     var_queue_minfree;		/* XXX use off_t */
typedef struct {
    char   *name;
    int     defval;
    int    *target;
} INT_TABLE;

int     var_unk_client_code;
int     var_bad_name_code;
int     var_unk_name_code;
int     var_unk_addr_code;
int     var_relay_code;
int     var_maps_rbl_code;
int     var_access_map_code;
int     var_reject_code;
int     var_defer_code;
int     var_non_fqdn_code;
int     var_smtpd_delay_reject;
int     var_allow_untrust_route;
int     var_local_rcpt_code;
int     var_relay_rcpt_code;
int     var_virt_mailbox_code;
int     var_virt_alias_code;

static INT_TABLE int_table[] = {
    "msg_verbose", 0, &msg_verbose,
    VAR_UNK_CLIENT_CODE, DEF_UNK_CLIENT_CODE, &var_unk_client_code,
    VAR_BAD_NAME_CODE, DEF_BAD_NAME_CODE, &var_bad_name_code,
    VAR_UNK_NAME_CODE, DEF_UNK_NAME_CODE, &var_unk_name_code,
    VAR_UNK_ADDR_CODE, DEF_UNK_ADDR_CODE, &var_unk_addr_code,
    VAR_RELAY_CODE, DEF_RELAY_CODE, &var_relay_code,
    VAR_MAPS_RBL_CODE, DEF_MAPS_RBL_CODE, &var_maps_rbl_code,
    VAR_ACCESS_MAP_CODE, DEF_ACCESS_MAP_CODE, &var_access_map_code,
    VAR_REJECT_CODE, DEF_REJECT_CODE, &var_reject_code,
    VAR_DEFER_CODE, DEF_DEFER_CODE, &var_defer_code,
    VAR_NON_FQDN_CODE, DEF_NON_FQDN_CODE, &var_non_fqdn_code,
    VAR_SMTPD_DELAY_REJECT, DEF_SMTPD_DELAY_REJECT, &var_smtpd_delay_reject,
    VAR_ALLOW_UNTRUST_ROUTE, DEF_ALLOW_UNTRUST_ROUTE, &var_allow_untrust_route,
    VAR_LOCAL_RCPT_CODE, DEF_LOCAL_RCPT_CODE, &var_local_rcpt_code,
    VAR_RELAY_RCPT_CODE, DEF_RELAY_RCPT_CODE, &var_relay_rcpt_code,
    VAR_VIRT_ALIAS_CODE, DEF_VIRT_ALIAS_CODE, &var_virt_alias_code,
    VAR_VIRT_MAILBOX_CODE, DEF_VIRT_MAILBOX_CODE, &var_virt_mailbox_code,
    0,
};

/* int_init - initialize int parameters */

static void int_init(void)
{
    INT_TABLE *sp;

    for (sp = int_table; sp->name; sp++)
	sp->target[0] = sp->defval;
}

/* int_update - update int parameter */

static int int_update(char **argv)
{
    INT_TABLE *ip;

    for (ip = int_table; ip->name; ip++) {
	if (strcasecmp(argv[0], ip->name) == 0) {
	    if (!ISDIGIT(*argv[1]))
		msg_fatal("bad number: %s %s", ip->name, argv[1]);
	    ip->target[0] = atoi(argv[1]);
	    return (1);
	}
    }
    return (0);
}

 /*
  * Restrictions.
  */
typedef struct {
    char   *name;
    ARGV  **target;
} REST_TABLE;

static REST_TABLE rest_table[] = {
    "client_restrictions", &client_restrctions,
    "helo_restrictions", &helo_restrctions,
    "sender_restrictions", &mail_restrctions,
    "recipient_restrictions", &rcpt_restrctions,
    "etrn_restrictions", &etrn_restrctions,
    0,
};

/* rest_update - update restriction */

static int rest_update(char **argv)
{
    REST_TABLE *rp;

    for (rp = rest_table; rp->name; rp++) {
	if (strcasecmp(rp->name, argv[0]) == 0) {
	    argv_free(rp->target[0]);
	    rp->target[0] = smtpd_check_parse(argv[1]);
	    return (1);
	}
    }
    return (0);
}

/* rest_class - (re)define a restriction class */

static void rest_class(char *class)
{
    char   *cp = class;
    char   *name;
    HTABLE_INFO *entry;

    if (smtpd_rest_classes == 0)
	smtpd_rest_classes = htable_create(1);

    if ((name = mystrtok(&cp, " \t\r\n,")) == 0)
	msg_panic("rest_class: null class name");
    if ((entry = htable_locate(smtpd_rest_classes, name)) != 0)
	argv_free((ARGV *) entry->value);
    else
	entry = htable_enter(smtpd_rest_classes, name, (char *) 0);
    entry->value = (char *) smtpd_check_parse(cp);
}

/* resolve_clnt_init - initialize reply */

void    resolve_clnt_init(RESOLVE_REPLY *reply)
{
    reply->flags = 0;
    reply->transport = vstring_alloc(100);
    reply->nexthop = vstring_alloc(100);
    reply->recipient = vstring_alloc(100);
}

void    resolve_clnt_free(RESOLVE_REPLY *reply)
{
    vstring_free(reply->transport);
    vstring_free(reply->nexthop);
    vstring_free(reply->recipient);
}

bool    var_smtpd_sasl_enable = 0;

#ifdef USE_SASL_AUTH

/* smtpd_sasl_connect - stub */

void    smtpd_sasl_connect(SMTPD_STATE *state)
{
    msg_panic("smtpd_sasl_connect was called");
}

/* smtpd_sasl_disconnect - stub */

void    smtpd_sasl_disconnect(SMTPD_STATE *state)
{
    msg_panic("smtpd_sasl_disconnect was called");
}

/* permit_sasl_auth - stub */

int     permit_sasl_auth(SMTPD_STATE *state, int ifyes, int ifnot)
{
    return (ifnot);
}

#endif

/* canon_addr_internal - stub */

VSTRING *canon_addr_internal(VSTRING *result, const char *addr)
{
    if (addr == STR(result))
	msg_panic("canon_addr_internal: result clobbers input");
    if (*addr && strchr(addr, '@') == 0)
	msg_fatal("%s: address rewriting is disabled", addr);
    vstring_strcpy(result, addr);
}

/* resolve_clnt_query - stub */

void    resolve_clnt_query(const char *addr, RESOLVE_REPLY *reply)
{
    const char *domain;

    if (addr == CONST_STR(reply->recipient))
	msg_panic("resolve_clnt_query: result clobbers input");
    if (strchr(addr, '%'))
	msg_fatal("%s: address rewriting is disabled", addr);
    if ((domain = strrchr(addr, '@')) == 0)
	msg_fatal("%s: unqualified address", addr);
    domain += 1;
    if (resolve_local(domain)) {
	reply->flags = RESOLVE_CLASS_LOCAL;
	vstring_strcpy(reply->transport, MAIL_SERVICE_LOCAL);
	vstring_strcpy(reply->nexthop, domain);
    } else if (string_list_match(virt_alias_doms, domain)) {
	reply->flags = RESOLVE_CLASS_ALIAS;
	vstring_strcpy(reply->transport, MAIL_SERVICE_ERROR);
	vstring_strcpy(reply->nexthop, "user unknown");
    } else if (string_list_match(virt_mailbox_doms, domain)) {
	reply->flags = RESOLVE_CLASS_VIRTUAL;
	vstring_strcpy(reply->transport, MAIL_SERVICE_VIRTUAL);
	vstring_strcpy(reply->nexthop, domain);
    } else if (domain_list_match(relay_domains, domain)) {
	reply->flags = RESOLVE_CLASS_RELAY;
	vstring_strcpy(reply->transport, MAIL_SERVICE_RELAY);
	vstring_strcpy(reply->nexthop, domain);
    } else {
	reply->flags = RESOLVE_CLASS_DEFAULT;
	vstring_strcpy(reply->transport, MAIL_SERVICE_SMTP);
	vstring_strcpy(reply->nexthop, domain);
    }
    vstring_strcpy(reply->recipient, addr);
}

/* smtpd_chat_reset - stub */

void    smtpd_chat_reset(SMTPD_STATE *unused_state)
{
}

/* usage - scream and terminate */

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s", myname);
}

int     main(int argc, char **argv)
{
    VSTRING *buf = vstring_alloc(100);
    SMTPD_STATE state;
    ARGV   *args;
    char   *bp;
    char   *resp;
    char   *addr;

    /*
     * Initialization. Use dummies for client information.
     */
    msg_vstream_init(argv[0], VSTREAM_ERR);
    if (argc != 1)
	usage(argv[0]);
    string_init();
    int_init();
    smtpd_check_init();
    smtpd_state_init(&state, VSTREAM_IN);
    state.queue_id = "<queue id>";

    /*
     * Main loop: update config parameters or test the client, helo, sender
     * and recipient restrictions.
     */
    while (vstring_fgets_nonl(buf, VSTREAM_IN) != 0) {

	/*
	 * Tokenize the command. Note, the comma is not a separator, so that
	 * restriction lists can be entered as comma-separated lists.
	 */
	bp = STR(buf);
	if (!isatty(0)) {
	    vstream_printf(">>> %s\n", bp);
	    vstream_fflush(VSTREAM_OUT);
	}
	if (*bp == '#')
	    continue;
	if (*bp == '!') {
	    vstream_printf("exit %d\n", system(bp + 1));
	    continue;
	}
	args = argv_split(bp, " \t\r\n");

	/*
	 * Recognize the command.
	 */
	resp = "bad command";
	switch (args->argc) {

	    /*
	     * Special case: client identity.
	     */
	case 4:
	case 3:
#define UPDATE_STRING(ptr,val) { if (ptr) myfree(ptr); ptr = mystrdup(val); }

	    if (strcasecmp(args->argv[0], "client") == 0) {
		state.where = "CONNECT";
		UPDATE_STRING(state.name, args->argv[1]);
		UPDATE_STRING(state.addr, args->argv[2]);
		if (args->argc == 4)
		    state.peer_code = atoi(args->argv[3]);
		else
		    state.peer_code = 2;
		if (state.namaddr)
		    myfree(state.namaddr);
		state.namaddr = concatenate(state.name, "[", state.addr,
					    "]", (char *) 0);
		resp = smtpd_check_client(&state);
	    }
	    break;

	    /*
	     * Try config settings.
	     */
#define UPDATE_MAPS(ptr, var, val, lock) \
	{ if (ptr) maps_free(ptr); ptr = maps_create(var, val, lock); }

#define UPDATE_LIST(ptr, val) \
	{ if (ptr) string_list_free(ptr); \
	  ptr = string_list_init(MATCH_FLAG_NONE, val); }

	case 2:
	    if (strcasecmp(args->argv[0], VAR_VIRT_ALIAS_MAPS) == 0) {
		UPDATE_STRING(var_virt_alias_maps, args->argv[1]);
		UPDATE_MAPS(virt_alias_maps, VAR_VIRT_ALIAS_MAPS,
			    var_virt_alias_maps, DICT_FLAG_LOCK);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_VIRT_ALIAS_DOMS) == 0) {
		UPDATE_STRING(var_virt_alias_doms, args->argv[1]);
		UPDATE_LIST(virt_alias_doms, var_virt_alias_doms);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_VIRT_MAILBOX_MAPS) == 0) {
		UPDATE_STRING(var_virt_mailbox_maps, args->argv[1]);
		UPDATE_MAPS(virt_mailbox_maps, VAR_VIRT_MAILBOX_MAPS,
			    var_virt_mailbox_maps, DICT_FLAG_LOCK);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_VIRT_MAILBOX_DOMS) == 0) {
		UPDATE_STRING(var_virt_mailbox_doms, args->argv[1]);
		UPDATE_LIST(virt_mailbox_doms, var_virt_mailbox_doms);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], "local_recipient_maps") == 0) {
		UPDATE_STRING(var_local_rcpt_maps, args->argv[1]);
		UPDATE_MAPS(local_rcpt_maps, VAR_LOCAL_RCPT_MAPS,
			    var_local_rcpt_maps, DICT_FLAG_LOCK);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], "relay_recipient_maps") == 0) {
		UPDATE_STRING(var_relay_rcpt_maps, args->argv[1]);
		UPDATE_MAPS(relay_rcpt_maps, VAR_RELAY_RCPT_MAPS,
			    var_relay_rcpt_maps, DICT_FLAG_LOCK);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], "canonical_maps") == 0) {
		UPDATE_STRING(var_canonical_maps, args->argv[1]);
		UPDATE_MAPS(canonical_maps, VAR_CANONICAL_MAPS,
			    var_canonical_maps, DICT_FLAG_LOCK);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], "rbl_reply_maps") == 0) {
		UPDATE_STRING(var_rbl_reply_maps, args->argv[1]);
		UPDATE_MAPS(rbl_reply_maps, VAR_RBL_REPLY_MAPS,
			    var_rbl_reply_maps, DICT_FLAG_LOCK);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], "mynetworks") == 0) {
		namadr_list_free(mynetworks);
		mynetworks =
		    namadr_list_init(match_parent_style(VAR_MYNETWORKS),
				     args->argv[1]);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], "relay_domains") == 0) {
		domain_list_free(relay_domains);
		relay_domains =
		    domain_list_init(match_parent_style(VAR_RELAY_DOMAINS),
				     args->argv[1]);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], "restriction_class") == 0) {
		rest_class(args->argv[1]);
		resp = 0;
		break;
	    }
	    if (int_update(args->argv)
		|| string_update(args->argv)
		|| rest_update(args->argv)) {
		resp = 0;
		break;
	    }

	    /*
	     * Try restrictions.
	     */
#define TRIM_ADDR(src, res) { \
	    if (*(res = src) == '<') { \
		res += strlen(res) - 1; \
		if (*res == '>') \
		    *res = 0; \
		res = src + 1; \
	    } \
	}

	    if (strcasecmp(args->argv[0], "helo") == 0) {
		state.where = "HELO";
		resp = smtpd_check_helo(&state, args->argv[1]);
		UPDATE_STRING(state.helo_name, args->argv[1]);
	    } else if (strcasecmp(args->argv[0], "mail") == 0) {
		state.where = "MAIL";
		TRIM_ADDR(args->argv[1], addr);
		UPDATE_STRING(state.sender, addr);
		resp = smtpd_check_mail(&state, addr);
	    } else if (strcasecmp(args->argv[0], "rcpt") == 0) {
		state.where = "RCPT";
		TRIM_ADDR(args->argv[1], addr);
		(resp = smtpd_check_rcpt(&state, addr))
		    || (resp = smtpd_check_rcptmap(&state, addr));
	    }
	    break;

	    /*
	     * Show commands.
	     */
	default:
	    resp = "Commands...\n\
		client <name> <address> [<code>]\n\
		helo <hostname>\n\
		sender <address>\n\
		recipient <address>\n\
		msg_verbose <level>\n\
		client_restrictions <restrictions>\n\
		helo_restrictions <restrictions>\n\
		sender_restrictions <restrictions>\n\
		recipient_restrictions <restrictions>\n\
		restriction_class name,<restrictions>\n\
		\n\
		Note: no address rewriting \n";
	    break;
	}
	vstream_printf("%s\n", resp ? resp : "OK");
	vstream_fflush(VSTREAM_OUT);
	argv_free(args);
    }
    vstring_free(buf);
    smtpd_state_reset(&state);
#define FREE_STRING(s) { if (s) myfree(s); }
    FREE_STRING(state.helo_name);
    FREE_STRING(state.sender);
    exit(0);
}

#endif
