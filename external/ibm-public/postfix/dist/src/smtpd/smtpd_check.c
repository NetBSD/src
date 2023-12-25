/*	$NetBSD: smtpd_check.c,v 1.5.2.1 2023/12/25 12:43:35 martin Exp $	*/

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
/*	int	smtpd_check_addr(sender, address, smtputf8)
/*	const char *sender;
/*	const char *address;
/*	int	smtputf8;
/*
/*	char	*smtpd_check_rewrite(state)
/*	SMTPD_STATE *state;
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
/*	char	*smtpd_check_etrn(state, destination)
/*	SMTPD_STATE *state;
/*	char	*destination;
/*
/*	char	*smtpd_check_data(state)
/*	SMTPD_STATE *state;
/*
/*	char	*smtpd_check_eod(state)
/*	SMTPD_STATE *state;
/*
/*	char	*smtpd_check_size(state, size)
/*	SMTPD_STATE *state;
/*	off_t	size;
/*
/*	char	*smtpd_check_queue(state)
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
/*	smtpd_check_addr() sanity checks an email address and returns
/*	non-zero in case of badness. The sender argument provides sender
/*	context for address resolution and caching, or a null pointer
/*	if information is unavailable.
/*
/*	smtpd_check_rewrite() should be called before opening a queue
/*	file or proxy connection, in order to establish the proper
/*	header address rewriting context.
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
/* .IP local_recipient_maps
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
/*	message is rejected when
/*	the message size exceeds the non-zero bound specified with the
/*	\fImessage_size_limit\fR configuration parameter. This is a
/*	permanent error.
/*
/*	smtpd_check_queue() checks the available queue file system
/*	space.  The message is rejected when:
/* .IP \(bu
/*	The available queue file system space is less than the amount
/*	specified with the \fImin_queue_free\fR configuration parameter.
/*	This is a temporary error.
/* .IP \(bu
/*	The available queue file system space is less than twice the
/*	message size limit. This is a temporary error.
/* .PP
/*	smtpd_check_data() enforces generic restrictions after the
/*	client has sent the DATA command.
/*
/*	smtpd_check_eod() enforces generic restrictions after the
/*	client has sent the END-OF-DATA command.
/*
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*
/*	TLS support originally by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
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
#include <errno.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
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
#include <attr_clnt.h>
#include <myaddrinfo.h>
#include <inet_proto.h>
#include <ip_match.h>
#include <valid_utf8_hostname.h>
#include <midna_domain.h>
#include <mynetworks.h>
#include <name_code.h>

/* DNS library. */

#include <dns.h>

/* Global library. */

#include <string_list.h>
#include <namadr_list.h>
#include <domain_list.h>
#include <mail_params.h>
#include <resolve_clnt.h>
#include <mail_error.h>
#include <resolve_local.h>
#include <own_inet_addr.h>
#include <mail_conf.h>
#include <maps.h>
#include <mail_addr_find.h>
#include <match_parent_style.h>
#include <strip_addr.h>
#include <cleanup_user.h>
#include <record.h>
#include <rec_type.h>
#include <mail_proto.h>
#include <mail_addr.h>
#include <verify_clnt.h>
#include <input_transp.h>
#include <is_header.h>
#include <valid_mailhost_addr.h>
#include <dsn_util.h>
#include <conv_time.h>
#include <xtext.h>
#include <smtp_stream.h>
#include <attr_override.h>
#include <map_search.h>
#include <info_log_addr_form.h>
#include <mail_version.h>

/* Application-specific. */

#include "smtpd.h"
#include "smtpd_sasl_glue.h"
#include "smtpd_check.h"
#include "smtpd_dsn_fix.h"
#include "smtpd_resolve.h"
#include "smtpd_expand.h"

 /*
  * Eject seat in case of parsing problems.
  */
static jmp_buf smtpd_check_buf;

 /*
  * Results of restrictions. Errors are negative; see dict.h.
  */
#define SMTPD_CHECK_DUNNO	0	/* indifferent */
#define SMTPD_CHECK_OK		1	/* explicitly permit */
#define SMTPD_CHECK_REJECT	2	/* explicitly reject */

 /*
  * Intermediate results. These are static to avoid unnecessary stress on the
  * memory manager routines.
  */
static VSTRING *error_text;
static CTABLE *smtpd_rbl_cache;
static CTABLE *smtpd_rbl_byte_cache;

 /*
  * Pre-opened SMTP recipient maps so we can reject mail for unknown users.
  * XXX This does not belong here and will eventually become part of the
  * trivial-rewrite resolver.
  */
static MAPS *local_rcpt_maps;
static MAPS *send_canon_maps;
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
static NAMADR_LIST *mynetworks_curr;
static NAMADR_LIST *mynetworks_new;
static NAMADR_LIST *perm_mx_networks;

#ifdef USE_TLS
static MAPS *relay_ccerts;

#endif

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
static ARGV *relay_restrctions;
static ARGV *fake_relay_restrctions;
static ARGV *rcpt_restrctions;
static ARGV *etrn_restrctions;
static ARGV *data_restrctions;
static ARGV *eod_restrictions;

static HTABLE *smtpd_rest_classes;
static HTABLE *policy_clnt_table;
static HTABLE *map_command_table;

static ARGV *local_rewrite_clients;

 /*
  * The routine that recursively applies restrictions.
  */
static int generic_checks(SMTPD_STATE *, ARGV *, const char *, const char *, const char *);

 /*
  * Recipient table check.
  */
static int check_sender_rcpt_maps(SMTPD_STATE *, const char *);
static int check_recipient_rcpt_maps(SMTPD_STATE *, const char *);
static int check_rcpt_maps(SMTPD_STATE *, const char *, const char *,
			           const char *);

 /*
  * Tempfail actions;
  */
static int unk_name_tf_act;
static int unk_addr_tf_act;
static int unv_rcpt_tf_act;
static int unv_from_tf_act;

 /*
  * Optional permit logging.
  */
static STRING_LIST *smtpd_acl_perm_log;

 /*
  * YASLM.
  */
#define STR	vstring_str
#define CONST_STR(x)	((const char *) vstring_str(x))
#define UPDATE_STRING(ptr,val) { if (ptr) myfree(ptr); ptr = mystrdup(val); }

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
  * reject_unknown_client, hostname-based allow-list, reject
  * 
  * XXX With warn_if_reject, don't raise the defer_if_permit flag when a
  * reject-style restriction fails. Instead, log the warning for the
  * resulting defer message.
  * 
  * XXX With warn_if_reject, do raise the defer_if_reject flag when a
  * permit-style restriction fails. Otherwise, we could reject legitimate
  * mail.
  */
static int PRINTFLIKE(5, 6) defer_if(SMTPD_DEFER *, int, int, const char *, const char *,...);
static int PRINTFLIKE(5, 6) smtpd_check_reject(SMTPD_STATE *, int, int, const char *, const char *,...);

#define DEFER_IF_REJECT2(state, class, code, dsn, fmt, a1, a2) \
    defer_if(&(state)->defer_if_reject, (class), (code), (dsn), (fmt), (a1), (a2))
#define DEFER_IF_REJECT3(state, class, code, dsn, fmt, a1, a2, a3) \
    defer_if(&(state)->defer_if_reject, (class), (code), (dsn), (fmt), (a1), (a2), (a3))
#define DEFER_IF_REJECT4(state, class, code, dsn, fmt, a1, a2, a3, a4) \
    defer_if(&(state)->defer_if_reject, (class), (code), (dsn), (fmt), (a1), (a2), (a3), (a4))

 /*
  * The following choose between DEFER_IF_PERMIT (only if warn_if_reject is
  * turned off) and plain DEFER. See tempfail_actions[] below for the mapping
  * from names to numeric action code.
  */
#define DEFER_ALL_ACT		0
#define DEFER_IF_PERMIT_ACT	1

#define DEFER_IF_PERMIT2(type, state, class, code, dsn, fmt, a1, a2) \
    (((state)->warn_if_reject == 0 && (type) != 0) ? \
	defer_if(&(state)->defer_if_permit, (class), (code), (dsn), (fmt), (a1), (a2)) \
    : \
	smtpd_check_reject((state), (class), (code), (dsn), (fmt), (a1), (a2)))
#define DEFER_IF_PERMIT3(type, state, class, code, dsn, fmt, a1, a2, a3) \
    (((state)->warn_if_reject == 0 && (type) != 0) ? \
	defer_if(&(state)->defer_if_permit, (class), (code), (dsn), (fmt), (a1), (a2), (a3)) \
    : \
	smtpd_check_reject((state), (class), (code), (dsn), (fmt), (a1), (a2), (a3)))
#define DEFER_IF_PERMIT4(type, state, class, code, dsn, fmt, a1, a2, a3, a4) \
    (((state)->warn_if_reject == 0 && (type) != 0) ? \
	defer_if(&(state)->defer_if_permit, (class), (code), (dsn), (fmt), (a1), (a2), (a3), (a4)) \
    : \
	smtpd_check_reject((state), (class), (code), (dsn), (fmt), (a1), (a2), (a3), (a4)))

 /*
  * Cached RBL lookup state.
  */
typedef struct {
    char   *txt;			/* TXT content or NULL */
    DNS_RR *a;				/* A records */
} SMTPD_RBL_STATE;

static void *rbl_pagein(const char *, void *);
static void rbl_pageout(void *, void *);
static void *rbl_byte_pagein(const char *, void *);
static void rbl_byte_pageout(void *, void *);

 /*
  * Context for RBL $name expansion.
  */
typedef struct {
    SMTPD_STATE *state;			/* general state */
    char   *domain;			/* query domain */
    const char *what;			/* rejected value */
    const char *class;			/* name of rejected value */
    const char *txt;			/* randomly selected trimmed TXT rr */
} SMTPD_RBL_EXPAND_CONTEXT;

 /*
  * Multiplication factor for free space check. Free space must be at least
  * smtpd_space_multf * message_size_limit.
  */
double  smtpd_space_multf = 1.5;

 /*
  * SMTPD policy client. Most attributes are ATTR_CLNT attributes.
  */
typedef struct {
    ATTR_CLNT *client;			/* client handle */
    char   *def_action;			/* default action */
    char   *policy_context;		/* context of policy request */
} SMTPD_POLICY_CLNT;

 /*
  * Table-driven parsing of main.cf parameter overrides for specific policy
  * clients. We derive the override names from the corresponding main.cf
  * parameter names by skipping the redundant "smtpd_policy_service_" prefix.
  */
static ATTR_OVER_TIME time_table[] = {
    21 + (const char *) VAR_SMTPD_POLICY_TMOUT, DEF_SMTPD_POLICY_TMOUT, 0, 1, 0,
    21 + (const char *) VAR_SMTPD_POLICY_IDLE, DEF_SMTPD_POLICY_IDLE, 0, 1, 0,
    21 + (const char *) VAR_SMTPD_POLICY_TTL, DEF_SMTPD_POLICY_TTL, 0, 1, 0,
    21 + (const char *) VAR_SMTPD_POLICY_TRY_DELAY, DEF_SMTPD_POLICY_TRY_DELAY, 0, 1, 0,
    0,
};
static ATTR_OVER_INT int_table[] = {
    21 + (const char *) VAR_SMTPD_POLICY_REQ_LIMIT, 0, 0, 0,
    21 + (const char *) VAR_SMTPD_POLICY_TRY_LIMIT, 0, 1, 0,
    0,
};
static ATTR_OVER_STR str_table[] = {
    21 + (const char *) VAR_SMTPD_POLICY_DEF_ACTION, 0, 1, 0,
    21 + (const char *) VAR_SMTPD_POLICY_CONTEXT, 0, 1, 0,
    0,
};

#define link_override_table_to_variable(table, var) \
	do { table[var##_offset].target = &var; } while (0)

#define smtpd_policy_tmout_offset	0
#define smtpd_policy_idle_offset	1
#define smtpd_policy_ttl_offset		2
#define smtpd_policy_try_delay_offset	3

#define smtpd_policy_req_limit_offset	0
#define smtpd_policy_try_limit_offset	1

#define smtpd_policy_def_action_offset	0
#define smtpd_policy_context_offset	1

 /*
  * Search order names must be distinct, non-empty, and non-null.
  */
#define SMTPD_ACL_SEARCH_NAME_CERT_FPRINT	"cert_fingerprint"
#define SMTPD_ACL_SEARCH_NAME_PKEY_FPRINT	"pubkey_fingerprint"
#define SMTPD_ACL_SEARCH_NAME_CERT_ISSUER_CN	"issuer_cn"
#define SMTPD_ACL_SEARCH_NAME_CERT_SUBJECT_CN	"subject_cn"

 /*
  * Search order tokens must be distinct, and 1..126 inclusive, so that they
  * can be stored in a character string without concerns about signed versus
  * unsigned. Code 127 is reserved by map_search(3).
  */
#define SMTPD_ACL_SEARCH_CODE_CERT_FPRINT	1
#define SMTPD_ACL_SEARCH_CODE_PKEY_FPRINT	2
#define SMTPD_ACL_SEARCH_CODE_CERT_ISSUER_CN	3
#define SMTPD_ACL_SEARCH_CODE_CERT_SUBJECT_CN	4

 /*
  * Mapping from search-list names and to search-list codes.
  */
static const NAME_CODE search_actions[] = {
    SMTPD_ACL_SEARCH_NAME_CERT_FPRINT, SMTPD_ACL_SEARCH_CODE_CERT_FPRINT,
    SMTPD_ACL_SEARCH_NAME_PKEY_FPRINT, SMTPD_ACL_SEARCH_CODE_PKEY_FPRINT,
    SMTPD_ACL_SEARCH_NAME_CERT_ISSUER_CN, SMTPD_ACL_SEARCH_CODE_CERT_ISSUER_CN,
    SMTPD_ACL_SEARCH_NAME_CERT_SUBJECT_CN, SMTPD_ACL_SEARCH_CODE_CERT_SUBJECT_CN,
    0, MAP_SEARCH_CODE_UNKNOWN,
};

/* policy_client_register - register policy service endpoint */

static void policy_client_register(const char *name)
{
    static const char myname[] = "policy_client_register";
    SMTPD_POLICY_CLNT *policy_client;
    char   *saved_name = 0;
    const char *policy_name = 0;
    char   *cp;
    const char *sep = CHARS_COMMA_SP;
    const char *parens = CHARS_BRACE;
    char   *err;

    if (policy_clnt_table == 0)
	policy_clnt_table = htable_create(1);

    if (htable_find(policy_clnt_table, name) == 0) {

	/*
	 * Allow per-service overrides for main.cf global settings.
	 */
	int     smtpd_policy_tmout = var_smtpd_policy_tmout;
	int     smtpd_policy_idle = var_smtpd_policy_idle;
	int     smtpd_policy_ttl = var_smtpd_policy_ttl;
	int     smtpd_policy_try_delay = var_smtpd_policy_try_delay;
	int     smtpd_policy_req_limit = var_smtpd_policy_req_limit;
	int     smtpd_policy_try_limit = var_smtpd_policy_try_limit;
	const char *smtpd_policy_def_action = var_smtpd_policy_def_action;
	const char *smtpd_policy_context = var_smtpd_policy_context;

	link_override_table_to_variable(time_table, smtpd_policy_tmout);
	link_override_table_to_variable(time_table, smtpd_policy_idle);
	link_override_table_to_variable(time_table, smtpd_policy_ttl);
	link_override_table_to_variable(time_table, smtpd_policy_try_delay);
	link_override_table_to_variable(int_table, smtpd_policy_req_limit);
	link_override_table_to_variable(int_table, smtpd_policy_try_limit);
	link_override_table_to_variable(str_table, smtpd_policy_def_action);
	link_override_table_to_variable(str_table, smtpd_policy_context);

	if (*name == parens[0]) {
	    cp = saved_name = mystrdup(name);
	    if ((err = extpar(&cp, parens, EXTPAR_FLAG_NONE)) != 0)
		msg_fatal("policy service syntax error: %s", cp);
	    if ((policy_name = mystrtok(&cp, sep)) == 0)
		msg_fatal("empty policy service: \"%s\"", name);
	    attr_override(cp, sep, parens,
			  CA_ATTR_OVER_TIME_TABLE(time_table),
			  CA_ATTR_OVER_INT_TABLE(int_table),
			  CA_ATTR_OVER_STR_TABLE(str_table),
			  CA_ATTR_OVER_END);
	} else {
	    policy_name = name;
	}
	if (msg_verbose)
	    msg_info("%s: name=\"%s\" default_action=\"%s\" max_idle=%d "
		     "max_ttl=%d request_limit=%d retry_delay=%d "
		     "timeout=%d try_limit=%d policy_context=\"%s\"",
		     myname, policy_name, smtpd_policy_def_action,
		     smtpd_policy_idle, smtpd_policy_ttl,
		     smtpd_policy_req_limit, smtpd_policy_try_delay,
		     smtpd_policy_tmout, smtpd_policy_try_limit,
		     smtpd_policy_context);

	/*
	 * Create the client.
	 */
	policy_client = (SMTPD_POLICY_CLNT *) mymalloc(sizeof(*policy_client));
	policy_client->client = attr_clnt_create(policy_name,
						 smtpd_policy_tmout,
						 smtpd_policy_idle,
						 smtpd_policy_ttl);

	attr_clnt_control(policy_client->client,
			  ATTR_CLNT_CTL_REQ_LIMIT, smtpd_policy_req_limit,
			  ATTR_CLNT_CTL_TRY_LIMIT, smtpd_policy_try_limit,
			  ATTR_CLNT_CTL_TRY_DELAY, smtpd_policy_try_delay,
			  ATTR_CLNT_CTL_END);
	policy_client->def_action = mystrdup(smtpd_policy_def_action);
	policy_client->policy_context = mystrdup(smtpd_policy_context);
	htable_enter(policy_clnt_table, name, (void *) policy_client);
	if (saved_name)
	    myfree(saved_name);
    }
}

/* command_map_register - register access table for maps lookup */

static void command_map_register(const char *name)
{
    MAPS   *maps;

    if (map_command_table == 0)
	map_command_table = htable_create(1);

    if (htable_find(map_command_table, name) == 0) {
	maps = maps_create(name, name, DICT_FLAG_LOCK
			   | DICT_FLAG_FOLD_FIX
			   | DICT_FLAG_UTF8_REQUEST);
	(void) htable_enter(map_command_table, name, (void *) maps);
    }
}

/* smtpd_check_parse - pre-parse restrictions */

static ARGV *smtpd_check_parse(int flags, const char *checks)
{
    char   *saved_checks = mystrdup(checks);
    ARGV   *argv = argv_alloc(1);
    char   *bp = saved_checks;
    char   *name;
    char   *last = 0;
    const MAP_SEARCH *map_search;

    /*
     * Pre-parse the restriction list, and open any dictionaries that we
     * encounter. Dictionaries must be opened before entering the chroot
     * jail.
     */
#define SMTPD_CHECK_PARSE_POLICY	(1<<0)
#define SMTPD_CHECK_PARSE_MAPS		(1<<1)
#define SMTPD_CHECK_PARSE_ALL		(~0)

    while ((name = mystrtokq(&bp, CHARS_COMMA_SP, CHARS_BRACE)) != 0) {
	argv_add(argv, name, (char *) 0);
	if ((flags & SMTPD_CHECK_PARSE_POLICY)
	    && last && strcasecmp(last, CHECK_POLICY_SERVICE) == 0) {
	    policy_client_register(name);
	} else if ((flags & SMTPD_CHECK_PARSE_MAPS)
		   && (*name == *CHARS_BRACE || strchr(name, ':') != 0)) {
	    if ((map_search = map_search_create(name)) != 0)
		command_map_register(map_search->map_type_name);
	}
	last = name;
    }
    argv_terminate(argv);

    /*
     * Cleanup.
     */
    myfree(saved_checks);
    return (argv);
}

#ifndef TEST

/* has_required - make sure required restriction is present */

static int has_required(ARGV *restrictions, const char **required)
{
    char  **rest;
    const char **reqd;
    ARGV   *expansion;

    /*
     * Recursively check list membership.
     */
    for (rest = restrictions->argv; *rest; rest++) {
	if (strcasecmp(*rest, WARN_IF_REJECT) == 0 && rest[1] != 0) {
	    rest += 1;
	    continue;
	}
	if (strcasecmp(*rest, PERMIT_ALL) == 0) {
	    if (rest[1] != 0)
		msg_warn("restriction `%s' after `%s' is ignored",
			 rest[1], rest[0]);
	    return (0);
	}
	for (reqd = required; *reqd; reqd++)
	    if (strcasecmp(*rest, *reqd) == 0)
		return (1);
	/* XXX This lookup operation should not be case-sensitive. */
	if ((expansion = (ARGV *) htable_find(smtpd_rest_classes, *rest)) != 0)
	    if (has_required(expansion, required))
		return (1);
    }
    return (0);
}

/* fail_required - handle failure to use required restriction */

static void fail_required(const char *name, const char **required)
{
    const char *myname = "fail_required";
    const char **reqd;
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
    msg_fatal("in parameter %s, specify at least one working instance of: %s",
	      name, STR(example));
}

#endif

/* smtpd_check_init - initialize once during process lifetime */

void    smtpd_check_init(void)
{
    char   *saved_classes;
    const char *name;
    const char *value;
    char   *cp;

#ifndef TEST
    static const char *rcpt_required[] = {
	REJECT_UNAUTH_DEST,
	DEFER_UNAUTH_DEST,
	REJECT_ALL,
	DEFER_ALL,
	DEFER_IF_PERMIT,
	CHECK_RELAY_DOMAINS,
	0,
    };

#endif
    static NAME_CODE tempfail_actions[] = {
	DEFER_ALL, DEFER_ALL_ACT,
	DEFER_IF_PERMIT, DEFER_IF_PERMIT_ACT,
	0, -1,
    };

    /*
     * Pre-open access control lists before going to jail.
     */
    mynetworks_curr =
	namadr_list_init(VAR_MYNETWORKS, MATCH_FLAG_RETURN
		      | match_parent_style(VAR_MYNETWORKS), var_mynetworks);
    mynetworks_new =
	namadr_list_init(VAR_MYNETWORKS, MATCH_FLAG_RETURN
		   | match_parent_style(VAR_MYNETWORKS), mynetworks_host());
    relay_domains =
	domain_list_init(VAR_RELAY_DOMAINS,
			 match_parent_style(VAR_RELAY_DOMAINS),
			 var_relay_domains);
    perm_mx_networks =
	namadr_list_init(VAR_PERM_MX_NETWORKS, MATCH_FLAG_RETURN
			 | match_parent_style(VAR_PERM_MX_NETWORKS),
			 var_perm_mx_networks);
#ifdef USE_TLS
    relay_ccerts = maps_create(VAR_RELAY_CCERTS, var_smtpd_relay_ccerts,
			       DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
#endif

    /*
     * Pre-parse and pre-open the recipient maps.
     */
    local_rcpt_maps = maps_create(VAR_LOCAL_RCPT_MAPS, var_local_rcpt_maps,
				  DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
				  | DICT_FLAG_UTF8_REQUEST);
    send_canon_maps = maps_create(VAR_SEND_CANON_MAPS, var_send_canon_maps,
				  DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
				  | DICT_FLAG_UTF8_REQUEST);
    rcpt_canon_maps = maps_create(VAR_RCPT_CANON_MAPS, var_rcpt_canon_maps,
				  DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
				  | DICT_FLAG_UTF8_REQUEST);
    canonical_maps = maps_create(VAR_CANONICAL_MAPS, var_canonical_maps,
				 DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
				 | DICT_FLAG_UTF8_REQUEST);
    virt_alias_maps = maps_create(VAR_VIRT_ALIAS_MAPS, var_virt_alias_maps,
				  DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
				  | DICT_FLAG_UTF8_REQUEST);
    virt_mailbox_maps = maps_create(VAR_VIRT_MAILBOX_MAPS,
				    var_virt_mailbox_maps,
				    DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
				    | DICT_FLAG_UTF8_REQUEST);
    relay_rcpt_maps = maps_create(VAR_RELAY_RCPT_MAPS, var_relay_rcpt_maps,
				  DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
				  | DICT_FLAG_UTF8_REQUEST);

#ifdef TEST
    virt_alias_doms = string_list_init(VAR_VIRT_ALIAS_DOMS, MATCH_FLAG_NONE,
				       var_virt_alias_doms);
    virt_mailbox_doms = string_list_init(VAR_VIRT_MAILBOX_DOMS, MATCH_FLAG_NONE,
					 var_virt_mailbox_doms);
#endif

    access_parent_style = match_parent_style(SMTPD_ACCESS_MAPS);

    /*
     * Templates for RBL rejection replies.
     */
    rbl_reply_maps = maps_create(VAR_RBL_REPLY_MAPS, var_rbl_reply_maps,
				 DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
				 | DICT_FLAG_UTF8_REQUEST);

    /*
     * Sender to login name mapping.
     */
    smtpd_sender_login_maps = maps_create(VAR_SMTPD_SND_AUTH_MAPS,
					  var_smtpd_snd_auth_maps,
					  DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
					  | DICT_FLAG_UTF8_REQUEST);

    /*
     * error_text is used for returning error responses.
     */
    error_text = vstring_alloc(10);

    /*
     * Initialize the resolved address cache. Note: the cache persists across
     * SMTP sessions so we cannot make it dependent on session state.
     */
    smtpd_resolve_init(100);

    /*
     * Initialize the RBL lookup cache. Note: the cache persists across SMTP
     * sessions so we cannot make it dependent on session state.
     */
    smtpd_rbl_cache = ctable_create(100, rbl_pagein, rbl_pageout, (void *) 0);
    smtpd_rbl_byte_cache = ctable_create(1000, rbl_byte_pagein,
					 rbl_byte_pageout, (void *) 0);

    /*
     * Initialize access map search list support before parsing restriction
     * lists.
     */
    map_search_init(search_actions);

    /*
     * Pre-parse the restriction lists. At the same time, pre-open tables
     * before going to jail.
     */
    client_restrctions = smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
					   var_client_checks);
    helo_restrctions = smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
					 var_helo_checks);
    mail_restrctions = smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
					 var_mail_checks);
    relay_restrctions = smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
					  var_relay_checks);
    if (warn_compat_break_relay_restrictions)
	fake_relay_restrctions = smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
						   FAKE_RELAY_CHECKS);
    rcpt_restrctions = smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
					 var_rcpt_checks);
    etrn_restrctions = smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
					 var_etrn_checks);
    data_restrctions = smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
					 var_data_checks);
    eod_restrictions = smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
					 var_eod_checks);

    /*
     * Parse the pre-defined restriction classes.
     */
    smtpd_rest_classes = htable_create(1);
    if (*var_rest_classes) {
	cp = saved_classes = mystrdup(var_rest_classes);
	while ((name = mystrtok(&cp, CHARS_COMMA_SP)) != 0) {
	    if ((value = mail_conf_lookup_eval(name)) == 0 || *value == 0)
		msg_fatal("restriction class `%s' needs a definition", name);
	    /* XXX This store operation should not be case-sensitive. */
	    htable_enter(smtpd_rest_classes, name,
			 (void *) smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
						    value));
	}
	myfree(saved_classes);
    }

    /*
     * This is the place to specify definitions for complex restrictions such
     * as check_relay_domains in terms of more elementary restrictions.
     */
#if 0
    htable_enter(smtpd_rest_classes, "check_relay_domains",
		 smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
			      "permit_mydomain reject_unauth_destination"));
#endif
    htable_enter(smtpd_rest_classes, REJECT_SENDER_LOGIN_MISMATCH,
		 (void *) smtpd_check_parse(SMTPD_CHECK_PARSE_ALL,
					    REJECT_AUTH_SENDER_LOGIN_MISMATCH
				  " " REJECT_UNAUTH_SENDER_LOGIN_MISMATCH));

    /*
     * People screw up the relay restrictions too often. Require that they
     * list at least one restriction that rejects mail by default. We allow
     * relay restrictions to be empty for sites that require backwards
     * compatibility.
     */
#ifndef TEST
    if (!has_required(rcpt_restrctions, rcpt_required)
	&& !has_required(relay_restrctions, rcpt_required))
	fail_required(VAR_RELAY_CHECKS " or " VAR_RCPT_CHECKS, rcpt_required);
#endif

    /*
     * Local rewrite policy.
     */
    local_rewrite_clients = smtpd_check_parse(SMTPD_CHECK_PARSE_MAPS,
					      var_local_rwr_clients);

    /*
     * Tempfail_actions.
     * 
     * XXX This name-to-number mapping should be encapsulated in a separate
     * mail_conf_name_code.c module.
     */
    if ((unk_name_tf_act = name_code(tempfail_actions, NAME_CODE_FLAG_NONE,
				     var_unk_name_tf_act)) < 0)
	msg_fatal("bad configuration: %s = %s",
		  VAR_UNK_NAME_TF_ACT, var_unk_name_tf_act);
    if ((unk_addr_tf_act = name_code(tempfail_actions, NAME_CODE_FLAG_NONE,
				     var_unk_addr_tf_act)) < 0)
	msg_fatal("bad configuration: %s = %s",
		  VAR_UNK_ADDR_TF_ACT, var_unk_addr_tf_act);
    if ((unv_rcpt_tf_act = name_code(tempfail_actions, NAME_CODE_FLAG_NONE,
				     var_unv_rcpt_tf_act)) < 0)
	msg_fatal("bad configuration: %s = %s",
		  VAR_UNV_RCPT_TF_ACT, var_unv_rcpt_tf_act);
    if ((unv_from_tf_act = name_code(tempfail_actions, NAME_CODE_FLAG_NONE,
				     var_unv_from_tf_act)) < 0)
	msg_fatal("bad configuration: %s = %s",
		  VAR_UNV_FROM_TF_ACT, var_unv_from_tf_act);
    if (msg_verbose) {
	msg_info("%s = %s", VAR_UNK_NAME_TF_ACT, tempfail_actions[unk_name_tf_act].name);
	msg_info("%s = %s", VAR_UNK_ADDR_TF_ACT, tempfail_actions[unk_addr_tf_act].name);
	msg_info("%s = %s", VAR_UNV_RCPT_TF_ACT, tempfail_actions[unv_rcpt_tf_act].name);
	msg_info("%s = %s", VAR_UNV_FROM_TF_ACT, tempfail_actions[unv_from_tf_act].name);
    }

    /*
     * Optional permit logging.
     */
    smtpd_acl_perm_log = string_list_init(VAR_SMTPD_ACL_PERM_LOG,
					  MATCH_FLAG_RETURN,
					  var_smtpd_acl_perm_log);
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
	vstring_sprintf_append(buf, " from=<%s>",
			       info_log_addr_form_sender(state->sender));
    if (state->recipient)
	vstring_sprintf_append(buf, " to=<%s>",
			    info_log_addr_form_recipient(state->recipient));
    if (state->protocol)
	vstring_sprintf_append(buf, " proto=%s", state->protocol);
    if (state->helo_name)
	vstring_sprintf_append(buf, " helo=<%s>", state->helo_name);
    msg_info("%s", STR(buf));
    vstring_free(buf);
}

/* smtpd_acl_permit - permit request with optional logging */

static int PRINTFLIKE(5, 6) smtpd_acl_permit(SMTPD_STATE *state,
					             const char *action,
					             const char *reply_class,
					             const char *reply_name,
					             const char *format,...)
{
    const char myname[] = "smtpd_acl_permit";
    va_list ap;
    const char *whatsup;

#ifdef notdef
#define NO_PRINT_ARGS ""
#else
#define NO_PRINT_ARGS "%s", ""
#endif

    /*
     * First, find out if (and how) this permit action should be logged.
     */
    if (msg_verbose)
	msg_info("%s: checking %s settings", myname, VAR_SMTPD_ACL_PERM_LOG);

    if (state->defer_if_permit.active) {
	/* This action is overruled. Do not log. */
	whatsup = 0;
    } else if (string_list_match(smtpd_acl_perm_log, action) != 0) {
	/* This is not a test. Logging is enabled. */
	whatsup = "permit";
    } else {
	/* This is not a test. Logging is disabled. */
	whatsup = 0;
    }
    if (whatsup != 0) {
	vstring_sprintf(error_text, "action=%s for %s=%s",
			action, reply_class, reply_name);
	if (format && *format) {
	    vstring_strcat(error_text, " ");
	    va_start(ap, format);
	    vstring_vsprintf_append(error_text, format, ap);
	    va_end(ap);
	}
	log_whatsup(state, whatsup, STR(error_text));
    } else {
	if (msg_verbose)
	    msg_info("%s: %s: no match", myname, VAR_SMTPD_ACL_PERM_LOG);
    }
    return (SMTPD_CHECK_OK);
}

/* smtpd_check_reject - do the boring things that must be done */

static int smtpd_check_reject(SMTPD_STATE *state, int error_class,
			              int code, const char *dsn,
			              const char *format,...)
{
    va_list ap;
    int     warn_if_reject;
    const char *whatsup;

    /*
     * Do not reject mail if we were asked to warn only. However,
     * configuration/software/data errors cannot be converted into warnings.
     */
    if (state->warn_if_reject && error_class != MAIL_ERROR_SOFTWARE
	&& error_class != MAIL_ERROR_RESOURCE
	&& error_class != MAIL_ERROR_DATA) {
	warn_if_reject = 1;
	whatsup = "reject_warning";
    } else {
	warn_if_reject = 0;
	whatsup = "reject";
    }

    /*
     * Update the error class mask, and format the response. XXX What about
     * multi-line responses? For now we cheat and send whitespace.
     * 
     * Format the response before complaining about configuration errors, so
     * that we can show the error in context.
     */
    state->error_mask |= error_class;
    vstring_sprintf(error_text, "%d %s ", code, dsn);
    va_start(ap, format);
    vstring_vsprintf_append(error_text, format, ap);
    va_end(ap);

    /*
     * Validate the response, that is, the response must begin with a
     * three-digit status code, and the first digit must be 4 or 5. If the
     * response is bad, log a warning and send a generic response instead.
     */
    if (code < 400 || code > 599) {
	msg_warn("SMTP reply code configuration error: %s", STR(error_text));
	vstring_strcpy(error_text, "450 4.7.1 Service unavailable");
    }
    if (!dsn_valid(STR(error_text) + 4)) {
	msg_warn("DSN detail code configuration error: %s", STR(error_text));
	vstring_strcpy(error_text, "450 4.7.1 Service unavailable");
    }

    /*
     * Ensure RFC compliance. We could do this inside smtpd_chat_reply() and
     * switch to multi-line for long replies.
     */
    vstring_truncate(error_text, 510);
    printable(STR(error_text), ' ');

    /*
     * Force this rejection into deferral because of some earlier temporary
     * error that may have prevented us from accepting mail, and report the
     * earlier problem instead.
     */
    if (!warn_if_reject && state->defer_if_reject.active && STR(error_text)[0] == '5') {
	state->warn_if_reject = state->defer_if_reject.active = 0;
	return (smtpd_check_reject(state, state->defer_if_reject.class,
				   state->defer_if_reject.code,
				   STR(state->defer_if_reject.dsn),
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
     * In any case, enforce consistency between the SMTP code and DSN code.
     * SMTP has the higher precedence since it came here first.
     */
    STR(error_text)[4] = STR(error_text)[0];

    /*
     * Log what is happening. When the sysadmin discards policy violation
     * postmaster notices, this may be the only trace left that service was
     * rejected. Print the request, client name/address, and response.
     */
    log_whatsup(state, whatsup, STR(error_text));

    return (warn_if_reject ? 0 : SMTPD_CHECK_REJECT);
}

/* defer_if - prepare to change our mind */

static int defer_if(SMTPD_DEFER *defer, int error_class,
		            int code, const char *dsn,
		            const char *fmt,...)
{
    va_list ap;

    /*
     * Keep the first reason for this type of deferral, to minimize
     * confusion.
     */
    if (defer->active == 0) {
	defer->active = 1;
	defer->class = error_class;
	defer->code = code;
	if (defer->dsn == 0)
	    defer->dsn = vstring_alloc(10);
	vstring_strcpy(defer->dsn, dsn);
	if (defer->reason == 0)
	    defer->reason = vstring_alloc(10);
	va_start(ap, fmt);
	vstring_vsprintf(defer->reason, fmt, ap);
	va_end(ap);
    }
    return (SMTPD_CHECK_DUNNO);
}

/* reject_dict_retry - reject with temporary failure if dict lookup fails */

static NORETURN reject_dict_retry(SMTPD_STATE *state, const char *reply_name)
{
    longjmp(smtpd_check_buf, smtpd_check_reject(state, MAIL_ERROR_DATA,
						451, "4.3.0",
					   "<%s>: Temporary lookup failure",
						reply_name));
}

/* reject_server_error - reject with temporary failure after non-dict error */

static NORETURN reject_server_error(SMTPD_STATE *state)
{
    longjmp(smtpd_check_buf, smtpd_check_reject(state, MAIL_ERROR_SOFTWARE,
						451, "4.3.5",
					     "Server configuration error"));
}

/* check_mail_addr_find - reject with temporary failure if dict lookup fails */

static const char *check_mail_addr_find(SMTPD_STATE *state,
					        const char *reply_name,
					        MAPS *maps, const char *key,
					        char **ext)
{
    const char *result;

    if ((result = mail_addr_find(maps, key, ext)) != 0 || maps->error == 0)
	return (result);
    if (maps->error == DICT_ERR_RETRY)
	/* Warning is already logged. */
	reject_dict_retry(state, reply_name);
    else
	reject_server_error(state);
}

/* reject_unknown_reverse_name - fail if reverse client hostname is unknown */

static int reject_unknown_reverse_name(SMTPD_STATE *state)
{
    const char *myname = "reject_unknown_reverse_name";

    if (msg_verbose)
	msg_info("%s: %s", myname, state->reverse_name);

    if (state->reverse_name_status != SMTPD_PEER_CODE_OK)
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
			state->reverse_name_status == SMTPD_PEER_CODE_PERM ?
				   var_unk_client_code : 450, "4.7.1",
	    "Client host rejected: cannot find your reverse hostname, [%s]",
				   state->addr));
    return (SMTPD_CHECK_DUNNO);
}

/* reject_unknown_client - fail if client hostname is unknown */

static int reject_unknown_client(SMTPD_STATE *state)
{
    const char *myname = "reject_unknown_client";

    if (msg_verbose)
	msg_info("%s: %s %s", myname, state->name, state->addr);

    /* RFC 7372: Email Authentication Status Codes. */
    if (state->name_status != SMTPD_PEER_CODE_OK)
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				state->name_status >= SMTPD_PEER_CODE_PERM ?
				   var_unk_client_code : 450, "4.7.25",
		    "Client host rejected: cannot find your hostname, [%s]",
				   state->addr));
    return (SMTPD_CHECK_DUNNO);
}

/* reject_plaintext_session - fail if session is not encrypted */

static int reject_plaintext_session(SMTPD_STATE *state)
{
    const char *myname = "reject_plaintext_session";

    if (msg_verbose)
	msg_info("%s: %s %s", myname, state->name, state->addr);

#ifdef USE_TLS
    if (state->tls_context == 0)
#endif
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				   var_plaintext_code, "4.7.1",
				   "Session encryption is required"));
    return (SMTPD_CHECK_DUNNO);
}

/* permit_inet_interfaces - succeed if client my own address */

static int permit_inet_interfaces(SMTPD_STATE *state)
{
    const char *myname = "permit_inet_interfaces";

    if (msg_verbose)
	msg_info("%s: %s %s", myname, state->name, state->addr);

    if (own_inet_addr((struct sockaddr *) &(state->sockaddr)))
	/* Permit logging in generic_checks() only. */
	return (SMTPD_CHECK_OK);
    return (SMTPD_CHECK_DUNNO);
}

/* permit_mynetworks - succeed if client is in a trusted network */

static int permit_mynetworks(SMTPD_STATE *state)
{
    const char *myname = "permit_mynetworks";

    if (msg_verbose)
	msg_info("%s: %s %s", myname, state->name, state->addr);

    if (namadr_list_match(mynetworks_curr, state->name, state->addr)) {
	if (warn_compat_break_mynetworks_style
	    && !namadr_list_match(mynetworks_new, state->name, state->addr))
	    msg_info("using backwards-compatible default setting "
		     VAR_MYNETWORKS_STYLE "=%s to permit request from "
		     "client \"%s\"", var_mynetworks_style, state->namaddr);
	/* Permit logging in generic_checks() only. */
	return (SMTPD_CHECK_OK);
    } else if (mynetworks_curr->error == 0)
	return (SMTPD_CHECK_DUNNO);
    else
	return (mynetworks_curr->error);
}

/* dup_if_truncate - save hostname and truncate if it ends in dot */

static char *dup_if_truncate(char *name)
{
    ssize_t len;
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
    const char *myname = "reject_invalid_hostaddr";
    ssize_t len;
    char   *test_addr;
    int     stat;

    if (msg_verbose)
	msg_info("%s: %s", myname, addr);

    if (addr[0] == '[' && (len = strlen(addr)) > 2 && addr[len - 1] == ']') {
	test_addr = mystrndup(addr + 1, len - 2);
    } else
	test_addr = addr;

    /*
     * Validate the address.
     */
    if (!valid_mailhost_addr(test_addr, DONT_GRIPE))
	stat = smtpd_check_reject(state, MAIL_ERROR_POLICY,
				  var_bad_name_code, "5.5.2",
				  "<%s>: %s rejected: invalid ip address",
				  reply_name, reply_class);
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
    const char *myname = "reject_invalid_hostname";
    char   *test_name;
    int     stat;

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

    /*
     * Truncate hostnames ending in dot but not dot-dot.
     */
    test_name = dup_if_truncate(name);

    /*
     * Validate the HELO/EHLO hostname. Fix 20140706: EAI not allowed here.
     */
    if (!valid_hostname(test_name, DONT_GRIPE)
	&& !valid_hostaddr(test_name, DONT_GRIPE))	/* XXX back compat */
	stat = smtpd_check_reject(state, MAIL_ERROR_POLICY,
				  var_bad_name_code, "5.5.2",
				  "<%s>: %s rejected: Invalid name",
				  reply_name, reply_class);
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
    const char *myname = "reject_non_fqdn_hostname";
    char   *test_name;
    int     stat;

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

    /*
     * Truncate hostnames ending in dot but not dot-dot.
     */
    test_name = dup_if_truncate(name);

    /*
     * Validate the hostname. For backwards compatibility, permit non-ASCII
     * names only when the client requested SMTPUTF8 support.
     */
    if (valid_utf8_hostname(state->flags & SMTPD_FLAG_SMTPUTF8,
		 test_name, DONT_GRIPE) == 0 || strchr(test_name, '.') == 0)
	stat = smtpd_check_reject(state, MAIL_ERROR_POLICY,
				  var_non_fqdn_code, "5.5.2",
			 "<%s>: %s rejected: need fully-qualified hostname",
				  reply_name, reply_class);
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
    const char *myname = "reject_unknown_hostname";
    int     dns_status;
    DNS_RR *dummy;

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

#ifdef T_AAAA
#define RR_ADDR_TYPES	T_A, T_AAAA
#else
#define RR_ADDR_TYPES	T_A
#endif

    dns_status = dns_lookup_l(name, 0, &dummy, (VSTRING *) 0,
			      (VSTRING *) 0, DNS_REQ_FLAG_STOP_OK,
			      RR_ADDR_TYPES, T_MX, 0);
    if (dummy)
	dns_rr_free(dummy);
    /* Allow MTA names to have nullMX records. */
    if (dns_status != DNS_OK && dns_status != DNS_NULLMX) {
	if (dns_status == DNS_POLICY) {
	    msg_warn("%s: address or MX lookup error: %s",
		     name, "DNS reply filter drops all results");
	    return (SMTPD_CHECK_DUNNO);
	}
	if (dns_status != DNS_RETRY)
	    return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				       var_unk_name_code, "4.7.1",
				       "<%s>: %s rejected: %s",
				       reply_name, reply_class,
				       dns_status == DNS_INVAL ?
				       "Malformed DNS server reply" :
				       "Host not found"));
	else
	    return (DEFER_IF_PERMIT2(unk_name_tf_act, state, MAIL_ERROR_POLICY,
				     450, "4.7.1",
				     "<%s>: %s rejected: Host not found",
				     reply_name, reply_class));
    }
    return (SMTPD_CHECK_DUNNO);
}

/* reject_unknown_mailhost - fail if name has no A, AAAA or MX record */

static int reject_unknown_mailhost(SMTPD_STATE *state, const char *name,
		            const char *reply_name, const char *reply_class)
{
    const char *myname = "reject_unknown_mailhost";
    int     dns_status;
    DNS_RR *dummy;
    const char *aname;

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

    /*
     * Fix 20140924: convert domain to ASCII.
     */
#ifndef NO_EAI
    if (!allascii(name) && (aname = midna_domain_to_ascii(name)) != 0) {
	if (msg_verbose)
	    msg_info("%s asciified to %s", name, aname);
	name = aname;
    }
#endif

#define MAILHOST_LOOKUP_FLAGS \
    (DNS_REQ_FLAG_STOP_OK | DNS_REQ_FLAG_STOP_INVAL | \
	DNS_REQ_FLAG_STOP_NULLMX | DNS_REQ_FLAG_STOP_MX_POLICY)

    dns_status = dns_lookup_l(name, 0, &dummy, (VSTRING *) 0,
			      (VSTRING *) 0, MAILHOST_LOOKUP_FLAGS,
			      T_MX, RR_ADDR_TYPES, 0);
    if (dummy)
	dns_rr_free(dummy);
    if (dns_status != DNS_OK) {			/* incl. DNS_INVAL */
	if (dns_status == DNS_POLICY) {
	    msg_warn("%s: MX or address lookup error: %s",
		     name, "DNS reply filter drops all results");
	    return (SMTPD_CHECK_DUNNO);
	}
	if (dns_status == DNS_NULLMX)
	    return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
			       strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
				       550 : 556,
			       strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
				       "4.7.27" : "4.1.10",
				       "<%s>: %s rejected: Domain %s "
				       "does not accept mail (nullMX)",
				       reply_name, reply_class, name));
	if (dns_status != DNS_RETRY)
	    return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				       var_unk_addr_code,
			       strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
				       "4.1.8" : "4.1.2",
				       "<%s>: %s rejected: %s",
				       reply_name, reply_class,
				       dns_status == DNS_INVAL ?
				       "Malformed DNS server reply" :
				       "Domain not found"));
	else
	    return (DEFER_IF_PERMIT2(unk_addr_tf_act, state, MAIL_ERROR_POLICY,
			  450, strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
				     "4.1.8" : "4.1.2",
				     "<%s>: %s rejected: Domain not found",
				     reply_name, reply_class));
    }
    return (SMTPD_CHECK_DUNNO);
}

static int permit_auth_destination(SMTPD_STATE *state, char *recipient);

/* permit_tls_clientcerts - OK/DUNNO for message relaying, or set dict_errno */

static int permit_tls_clientcerts(SMTPD_STATE *state, int permit_all_certs)
{
#ifdef USE_TLS
    const char *found = 0;

    if (!state->tls_context)
	return SMTPD_CHECK_DUNNO;

    if (TLS_CERT_IS_TRUSTED(state->tls_context) && permit_all_certs) {
	if (msg_verbose)
	    msg_info("Relaying allowed for all verified client certificates");
	/* Permit logging in generic_checks() only. */
	return (SMTPD_CHECK_OK);
    }

    /*
     * When directly checking the fingerprint, it is OK if the issuing CA is
     * not trusted.
     */
    if (TLS_CERT_IS_PRESENT(state->tls_context)) {
	int     i;
	char   *prints[2];

	if (warn_compat_break_smtpd_tls_fpt_dgst)
	    msg_info("using backwards-compatible default setting "
		     VAR_SMTPD_TLS_FPT_DGST "=md5 to compute certificate "
		     "fingerprints");

	prints[0] = state->tls_context->peer_cert_fprint;
	prints[1] = state->tls_context->peer_pkey_fprint;

	/* After lookup error, leave relay_ccerts->error at non-zero value. */
	for (i = 0; i < 2; ++i) {
	    found = maps_find(relay_ccerts, prints[i], DICT_FLAG_NONE);
	    if (found != 0) {
		if (msg_verbose)
		    msg_info("Relaying allowed for certified client: %s", found);
		/* Permit logging in generic_checks() only. */
		return (SMTPD_CHECK_OK);
	    } else if (relay_ccerts->error != 0) {
		msg_warn("relay_clientcerts: lookup error for fingerprint '%s', "
			 "pkey fingerprint %s", prints[0], prints[1]);
		return (relay_ccerts->error);
	    }
	}
	if (msg_verbose)
	    msg_info("relay_clientcerts: No match for fingerprint '%s', "
		     "pkey fingerprint %s", prints[0], prints[1]);
    } else if (!var_smtpd_tls_ask_ccert) {
	msg_warn("%s is requested, but \"%s = no\"", permit_all_certs ?
		 PERMIT_TLS_ALL_CLIENTCERTS : PERMIT_TLS_CLIENTCERTS,
		 VAR_SMTPD_TLS_ACERT);
    }
#endif
    return (SMTPD_CHECK_DUNNO);
}

/* check_relay_domains - OK/FAIL for message relaying */

static int check_relay_domains(SMTPD_STATE *state, char *recipient,
			               char *reply_name, char *reply_class)
{
    const char *myname = "check_relay_domains";

#if 1
    static int once;

    if (once == 0) {
	once = 1;
	msg_warn("support for restriction \"%s\" will be removed from %s; "
		 "use \"%s\" instead",
		 CHECK_RELAY_DOMAINS, var_mail_name, REJECT_UNAUTH_DEST);
    }
#endif

    if (msg_verbose)
	msg_info("%s: %s", myname, recipient);

    /*
     * Permit if the client matches the relay_domains list.
     */
    if (domain_list_match(relay_domains, state->name)) {
	if (warn_compat_break_relay_domains)
	    msg_info("using backwards-compatible default setting "
		     VAR_RELAY_DOMAINS "=$mydestination to permit "
		     "request from client \"%s\"", state->name);
	return (SMTPD_CHECK_OK);
    }

    /*
     * Permit authorized destinations.
     */
    if (permit_auth_destination(state, recipient) == SMTPD_CHECK_OK)
	return (SMTPD_CHECK_OK);

    /*
     * Deny relaying between sites that both are not in relay_domains.
     */
    return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
			       var_relay_code, "5.7.1",
			       "<%s>: %s rejected: Relay access denied",
			       reply_name, reply_class));
}

/* permit_auth_destination - OK for message relaying */

static int permit_auth_destination(SMTPD_STATE *state, char *recipient)
{
    const char *myname = "permit_auth_destination";
    const RESOLVE_REPLY *reply;
    const char *domain;

    if (msg_verbose)
	msg_info("%s: %s", myname, recipient);

    /*
     * Resolve the address.
     */
    reply = smtpd_resolve_addr(state->sender, recipient);
    if (reply->flags & RESOLVE_FLAG_FAIL)
	reject_dict_retry(state, recipient);

    /*
     * Handle special case that is not supposed to happen.
     */
    if ((domain = strrchr(CONST_STR(reply->recipient), '@')) == 0)
	return (SMTPD_CHECK_OK);
    domain += 1;

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
    if (reply->flags & RESOLVE_CLASS_RELAY) {
	if (warn_compat_break_relay_domains)
	    msg_info("using backwards-compatible default setting "
		     VAR_RELAY_DOMAINS "=$mydestination to accept mail "
		     "for domain \"%s\"", domain);
	return (SMTPD_CHECK_OK);
    }

    /*
     * Skip when not matched
     */
    return (SMTPD_CHECK_DUNNO);
}

/* reject_unauth_destination - FAIL for message relaying */

static int reject_unauth_destination(SMTPD_STATE *state, char *recipient,
			              int reply_code, const char *reply_dsn)
{
    const char *myname = "reject_unauth_destination";

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
			       reply_code, reply_dsn,
			       "<%s>: Relay access denied",
			       recipient));
}

/* reject_unauth_pipelining - reject improper use of SMTP command pipelining */

static int reject_unauth_pipelining(SMTPD_STATE *state,
		            const char *reply_name, const char *reply_class)
{
    const char *myname = "reject_unauth_pipelining";

    if (msg_verbose)
	msg_info("%s: %s", myname, state->where);

    if (state->flags & SMTPD_FLAG_ILL_PIPELINING)
	return (smtpd_check_reject(state, MAIL_ERROR_PROTOCOL,
				   503, "5.5.0",
	       "<%s>: %s rejected: Improper use of SMTP command pipelining",
				   reply_name, reply_class));

    return (SMTPD_CHECK_DUNNO);
}

/* all_auth_mx_addr - match host addresses against permit_mx_backup_networks */

static int all_auth_mx_addr(SMTPD_STATE *state, char *host,
		            const char *reply_name, const char *reply_class)
{
    const char *myname = "all_auth_mx_addr";
    MAI_HOSTADDR_STR hostaddr;
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
    dns_status = dns_lookup_v(host, 0, &addr_list, (VSTRING *) 0, (VSTRING *) 0,
		      DNS_REQ_FLAG_NONE, inet_proto_info()->dns_atype_list);
    /* DNS_NULLMX is not applicable here. */
    if (dns_status != DNS_OK) {			/* incl. DNS_INVAL */
	DEFER_IF_REJECT4(state, MAIL_ERROR_POLICY,
			 450, "4.4.4",
			 "<%s>: %s rejected: Unable to look up host "
			 "%s as mail exchanger: %s",
			 reply_name, reply_class, host,
			 dns_status == DNS_POLICY ?
			 "DNS reply filter policy" :
			 dns_strerror(dns_get_h_errno()));
	return (NOPE);
    }
    for (rr = addr_list; rr != 0; rr = rr->next) {
	if (dns_rr_to_pa(rr, &hostaddr) == 0) {
	    msg_warn("%s: skipping record type %s for host %s: %m",
		     myname, dns_strtype(rr->type), host);
	    continue;
	}
	if (msg_verbose)
	    msg_info("%s: checking: %s", myname, hostaddr.buf);

	if (!namadr_list_match(perm_mx_networks, host, hostaddr.buf)) {
	    if (perm_mx_networks->error == 0) {

		/*
		 * Reject: at least one IP address is not listed in
		 * permit_mx_backup_networks.
		 */
		if (msg_verbose)
		    msg_info("%s: address %s for %s does not match %s",
			  myname, hostaddr.buf, host, VAR_PERM_MX_NETWORKS);
	    } else {
		msg_warn("%s: %s lookup error for address %s for %s",
			 myname, VAR_PERM_MX_NETWORKS, hostaddr.buf, host);
		DEFER_IF_REJECT3(state, MAIL_ERROR_POLICY,
				 450, "4.4.4",
				 "<%s>: %s rejected: Unable to verify host %s as mail exchanger",
				 reply_name, reply_class, host);
	    }
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
    const char *myname = "has_my_addr";
    struct addrinfo *res;
    struct addrinfo *res0;
    int     aierr;
    MAI_HOSTADDR_STR hostaddr;
    const INET_PROTO_INFO *proto_info = inet_proto_info();

    if (msg_verbose)
	msg_info("%s: host %s", myname, host);

    /*
     * If we can't lookup the host, defer rather than reject.
     */
#define YUP	1
#define NOPE	0

    aierr = hostname_to_sockaddr(host, (char *) 0, 0, &res0);
    if (aierr) {
	DEFER_IF_REJECT4(state, MAIL_ERROR_POLICY,
			 450, "4.4.4",
	  "<%s>: %s rejected: Unable to look up mail exchanger host %s: %s",
			 reply_name, reply_class, host, MAI_STRERROR(aierr));
	return (NOPE);
    }
#define HAS_MY_ADDR_RETURN(x) { freeaddrinfo(res0); return (x); }

    for (res = res0; res != 0; res = res->ai_next) {
	if (strchr((char *) proto_info->sa_family_list, res->ai_family) == 0) {
	    if (msg_verbose)
		msg_info("skipping address family %d for host %s",
			 res->ai_family, host);
	    continue;
	}
	if (msg_verbose) {
	    SOCKADDR_TO_HOSTADDR(res->ai_addr, res->ai_addrlen,
				 &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
	    msg_info("%s: addr %s", myname, hostaddr.buf);
	}
	if (own_inet_addr(res->ai_addr))
	    HAS_MY_ADDR_RETURN(YUP);
	if (proxy_inet_addr(res->ai_addr))
	    HAS_MY_ADDR_RETURN(YUP);
    }
    if (msg_verbose)
	msg_info("%s: host %s: no match", myname, host);

    HAS_MY_ADDR_RETURN(NOPE);
}

/* i_am_mx - is this machine listed as MX relay */

static int i_am_mx(SMTPD_STATE *state, DNS_RR *mx_list,
		           const char *reply_name, const char *reply_class)
{
    const char *myname = "i_am_mx";
    DNS_RR *mx;

    /*
     * Compare hostnames first. Only if no name match is found, go through
     * the trouble of host address lookups.
     */
    for (mx = mx_list; mx != 0; mx = mx->next) {
	if (msg_verbose)
	    msg_info("%s: resolve hostname: %s", myname, (char *) mx->data);
	if (resolve_local((char *) mx->data) > 0)
	    return (YUP);
	/* if no match or error, match interface addresses instead. */
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
    const char *myname = "permit_mx_primary";
    DNS_RR *mx;

    if (msg_verbose)
	msg_info("%s", myname);

    /*
     * See if each best MX host has all IP addresses in
     * permit_mx_backup_networks.
     */
    for (mx = mx_list; mx != 0; mx = mx->next) {
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
    const char *myname = "permit_mx_backup";
    const RESOLVE_REPLY *reply;
    const char *domain;
    const char *adomain;
    DNS_RR *mx_list;
    DNS_RR *middle;
    DNS_RR *rest;
    int     dns_status;

    if (msg_verbose)
	msg_info("%s: %s", myname, recipient);

    /*
     * Resolve the address.
     */
    reply = smtpd_resolve_addr(state->sender, recipient);
    if (reply->flags & RESOLVE_FLAG_FAIL)
	reject_dict_retry(state, recipient);

    /*
     * For backwards compatibility, emulate permit_auth_destination. However,
     * old permit_mx_backup implementations allow source routing with local
     * address class.
     */
    if ((domain = strrchr(CONST_STR(reply->recipient), '@')) == 0)
	return (SMTPD_CHECK_OK);
    domain += 1;
#if 0
    if (reply->flags & RESOLVE_CLASS_LOCAL)
	return (SMTPD_CHECK_OK);
#endif
    if (var_allow_untrust_route == 0 && (reply->flags & RESOLVE_FLAG_ROUTED))
	return (SMTPD_CHECK_DUNNO);
    if (reply->flags & RESOLVE_CLASS_FINAL)
	return (SMTPD_CHECK_OK);
    if (reply->flags & RESOLVE_CLASS_RELAY) {
	if (warn_compat_break_relay_domains)
	    msg_info("using backwards-compatible default setting "
		     VAR_RELAY_DOMAINS "=$mydestination to accept mail "
		     "for domain \"%s\"", domain);
	return (SMTPD_CHECK_OK);
    }
    if (msg_verbose)
	msg_info("%s: not local: %s", myname, recipient);

    /*
     * Skip numerical forms that didn't match the local system.
     */
    if (domain[0] == '[' && domain[strlen(domain) - 1] == ']')
	return (SMTPD_CHECK_DUNNO);

    /*
     * Fix 20140924: convert domain to ASCII.
     */
#ifndef NO_EAI
    if (!allascii(domain) && (adomain = midna_domain_to_ascii(domain)) != 0) {
	if (msg_verbose)
	    msg_info("%s asciified to %s", domain, adomain);
	domain = adomain;
    }
#endif

    /*
     * Look up the list of MX host names for this domain. If no MX host is
     * found, perhaps it is a CNAME for the local machine. Clients aren't
     * supposed to send CNAMEs in SMTP commands, but it happens anyway. If we
     * can't look up the destination, play safe and turn reject into defer.
     */
    dns_status = dns_lookup(domain, T_MX, 0, &mx_list,
			    (VSTRING *) 0, (VSTRING *) 0);
#if 0
    if (dns_status == DNS_NOTFOUND)
	return (has_my_addr(state, domain, reply_name, reply_class) ?
		SMTPD_CHECK_OK : SMTPD_CHECK_DUNNO);
#endif
    if (dns_status != DNS_OK) {			/* incl. DNS_INVAL */
	/* We don't special-case DNS_NULLMX. */
	if (dns_status == DNS_RETRY || dns_status == DNS_POLICY)
	    DEFER_IF_REJECT3(state, MAIL_ERROR_POLICY,
			     450, "4.4.4",
			     "<%s>: %s rejected: Unable to look up mail "
			     "exchanger information: %s",
			     reply_name, reply_class,
			     dns_status == DNS_POLICY ?
			     "DNS reply filter policy" :
			     dns_strerror(dns_get_h_errno()));
	return (SMTPD_CHECK_DUNNO);
    }

    /*
     * Separate MX list into primaries and backups.
     */
    mx_list = dns_rr_sort(mx_list, dns_rr_compare_pref_any);
    for (middle = mx_list; /* see below */ ; middle = rest) {
	rest = middle->next;
	if (rest == 0)
	    break;
	if (rest->pref != mx_list->pref) {
	    middle->next = 0;
	    break;
	}
    }
    /* postcondition: middle->next = 0, rest may be 0. */

#define PERMIT_MX_BACKUP_RETURN(x) do { \
	middle->next = rest; \
	dns_rr_free(mx_list); \
	return (x); \
   } while (0)

    /*
     * First, see if we match any of the primary MX servers.
     */
    if (i_am_mx(state, mx_list, reply_name, reply_class))
	PERMIT_MX_BACKUP_RETURN(SMTPD_CHECK_DUNNO);

    /*
     * Then, see if we match any of the backup MX servers.
     */
    if (rest == 0 || !i_am_mx(state, rest, reply_name, reply_class))
	PERMIT_MX_BACKUP_RETURN(SMTPD_CHECK_DUNNO);

    /*
     * Optionally, see if the primary MX hosts are in a restricted list of
     * networks.
     */
    if (*var_perm_mx_networks
	&& !permit_mx_primary(state, mx_list, reply_name, reply_class))
	PERMIT_MX_BACKUP_RETURN(SMTPD_CHECK_DUNNO);

    /*
     * The destination passed all requirements.
     */
    PERMIT_MX_BACKUP_RETURN(SMTPD_CHECK_OK);
}

/* reject_non_fqdn_address - fail if address is not in fqdn form */

static int reject_non_fqdn_address(SMTPD_STATE *state, char *addr,
				        char *reply_name, char *reply_class)
{
    const char *myname = "reject_non_fqdn_address";
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
     * Validate the domain. For backwards compatibility, permit non-ASCII
     * names only when the client requested SMTPUTF8 support.
     */
    if (!*test_dom || !valid_utf8_hostname(state->flags & SMTPD_FLAG_SMTPUTF8,
			    test_dom, DONT_GRIPE) || !strchr(test_dom, '.'))
	stat = smtpd_check_reject(state, MAIL_ERROR_POLICY,
				  var_non_fqdn_code, "4.5.2",
			  "<%s>: %s rejected: need fully-qualified address",
				  reply_name, reply_class);
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
    const char *myname = "reject_unknown_address";
    const RESOLVE_REPLY *reply;
    const char *domain;

    if (msg_verbose)
	msg_info("%s: %s", myname, addr);

    /*
     * Resolve the address.
     */
    reply = smtpd_resolve_addr(strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
			       state->recipient : state->sender, addr);
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

/* reject_unverified_address - fail if address bounces */

static int reject_unverified_address(SMTPD_STATE *state, const char *addr,
		            const char *reply_name, const char *reply_class,
			             int unv_addr_dcode, int unv_addr_rcode,
				             int unv_addr_tf_act,
				             const char *alt_reply)
{
    const char *myname = "reject_unverified_address";
    VSTRING *why = vstring_alloc(10);
    int     rqst_status = SMTPD_CHECK_DUNNO;
    int     rcpt_status;
    int     verify_status;
    int     count;
    int     reject_code = 0;

    if (msg_verbose)
	msg_info("%s: %s", myname, addr);

    /*
     * Verify the address. Don't waste too much of their or our time.
     */
    for (count = 0; /* see below */ ; /* see below */ ) {
	verify_status = verify_clnt_query(addr, &rcpt_status, why);
	if (verify_status != VRFY_STAT_OK || rcpt_status != DEL_RCPT_STAT_TODO)
	    break;
	if (++count >= var_verify_poll_count)
	    break;
	sleep(var_verify_poll_delay);
    }
    if (verify_status != VRFY_STAT_OK) {
	msg_warn("%s service failure", var_verify_service);
	rqst_status =
	    DEFER_IF_PERMIT2(unv_addr_tf_act, state, MAIL_ERROR_POLICY,
			  450, strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
			     SND_DSN : "4.1.1",
			  "<%s>: %s rejected: address verification problem",
			     reply_name, reply_class);
    } else {
	switch (rcpt_status) {
	default:
	    msg_warn("unknown address verification status %d", rcpt_status);
	    break;
	case DEL_RCPT_STAT_TODO:
	case DEL_RCPT_STAT_DEFER:
	    reject_code = unv_addr_dcode;
	    break;
	case DEL_RCPT_STAT_OK:
	    break;
	case DEL_RCPT_STAT_BOUNCE:
	    reject_code = unv_addr_rcode;
	    break;
	}
	if (reject_code >= 400 && *alt_reply)
	    vstring_strcpy(why, alt_reply);
	switch (reject_code / 100) {
	case 2:
	    break;
	case 4:
	    rqst_status =
		DEFER_IF_PERMIT3(unv_addr_tf_act, state, MAIL_ERROR_POLICY,
				 reject_code,
			       strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
				 SND_DSN : "4.1.1",
			    "<%s>: %s rejected: unverified address: %.250s",
				 reply_name, reply_class, STR(why));
	    break;
	default:
	    if (reject_code != 0)
		rqst_status =
		    smtpd_check_reject(state, MAIL_ERROR_POLICY,
				       reject_code,
			       strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
				       SND_DSN : "4.1.1",
			     "<%s>: %s rejected: undeliverable address: %s",
				       reply_name, reply_class, STR(why));
	    break;
	}
    }
    vstring_free(why);
    return (rqst_status);
}

/* can_delegate_action - can we delegate this to the cleanup server */

#ifndef TEST

static int not_in_client_helo(SMTPD_STATE *, const char *, const char *, const char *);

static int can_delegate_action(SMTPD_STATE *state, const char *table,
			        const char *action, const char *reply_class)
{

    /*
     * If we're not using the cleanup server, then there is no way that we
     * can support actions such as FILTER or HOLD that are delegated to the
     * cleanup server.
     */
    if (USE_SMTPD_PROXY(state)) {
	msg_warn("access table %s: with %s specified, action %s is unavailable",
		 table, VAR_SMTPD_PROXY_FILT, action);
	return (0);
    }

    /*
     * ETRN does not receive mail so we can't store queue file records.
     */
    if (strcmp(state->where, SMTPD_CMD_ETRN) == 0) {
	msg_warn("access table %s: action %s is unavailable in %s",
		 table, action, VAR_ETRN_CHECKS);
	return (0);
    }
    return (not_in_client_helo(state, table, action, reply_class));
}

/* not_in_client_helo - not in client or helo restriction context */

static int not_in_client_helo(SMTPD_STATE *state, const char *table,
			              const char *action,
			              const char *unused_reply_class)
{

    /*
     * If delay_reject=no, then client and helo restrictions take effect
     * immediately, outside any particular mail transaction context. For
     * example, rejecting HELO does not affect subsequent mail deliveries.
     * Thus, if delay_reject=no, client and helo actions such as FILTER or
     * HOLD also should not affect subsequent mail deliveries. Hmm...
     * 
     * XXX If the MAIL FROM command is rejected then we have to reset access map
     * side effects such as FILTER.
     */
    if (state->sender == 0) {
	msg_warn("access table %s: with %s=%s, "
		 "action %s is always skipped in %s or %s restrictions",
		 table, VAR_SMTPD_DELAY_REJECT, CONFIG_BOOL_NO,
		 action, SMTPD_NAME_CLIENT, SMTPD_NAME_HELO);
	/* XXX What about ETRN? */
	return (0);
    }
    return (1);
}

#endif

/* check_table_result - translate table lookup result into pass/reject */

static int check_table_result(SMTPD_STATE *state, const char *table,
			              const char *value, const char *datum,
			              const char *reply_name,
			              const char *reply_class,
			              const char *def_acl)
{
    const char *myname = "check_table_result";
    int     code;
    ARGV   *restrictions;
    jmp_buf savebuf;
    int     status;
    const char *cmd_text;
    int     cmd_len;
    static char def_dsn[] = "5.7.1";
    DSN_SPLIT dp;
    static VSTRING *buf;

#ifdef DELAY_ACTION
    int     defer_delay;

#endif

    if (buf == 0)
	buf = vstring_alloc(10);

    /*
     * Parse into command and text. Do not change the input.
     */
    cmd_text = value + strcspn(value, " \t");
    cmd_len = cmd_text - value;
    vstring_strncpy(buf, value, cmd_len);
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
	dsn_split(&dp, "5.7.1", cmd_text);
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				   var_map_reject_code,
				   smtpd_dsn_fix(DSN_STATUS(dp.dsn),
						 reply_class),
				   "<%s>: %s rejected: %s",
				   reply_name, reply_class,
				   *dp.text ? dp.text : "Access denied"));
    }

    /*
     * DEFER means "try again". Use optional text or generate a generic error
     * response.
     */
    if (STREQUAL(value, "DEFER", cmd_len)) {
	dsn_split(&dp, "4.7.1", cmd_text);
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				   var_map_defer_code,
				   smtpd_dsn_fix(DSN_STATUS(dp.dsn),
						 reply_class),
				   "<%s>: %s rejected: %s",
				   reply_name, reply_class,
				   *dp.text ? dp.text : "Access denied"));
    }
#ifndef SHUT_RDWR
#define SHUT_RDWR   2
#endif

    /*
     * HANGUP. Text is optional. Drop the connection without sending any
     * reply.
     * 
     * Note: this is an unsupported test feature. No attempt is made to maintain
     * compatibility between successive versions.
     */
    if (STREQUAL(value, "HANGUP", cmd_len)) {
	shutdown(vstream_fileno(state->client), SHUT_RDWR);
	log_whatsup(state, "hangup", cmd_text);
	vstream_longjmp(state->client, SMTP_ERR_QUIET);
    }

    /*
     * INFO. Text is optional.
     */
    if (STREQUAL(value, "INFO", cmd_len)) {
	log_whatsup(state, "info", cmd_text);
	return (SMTPD_CHECK_DUNNO);
    }

    /*
     * WARN. Text is optional.
     */
    if (STREQUAL(value, "WARN", cmd_len)) {
	log_whatsup(state, "warn", cmd_text);
	return (SMTPD_CHECK_DUNNO);
    }

    /*
     * FILTER means deliver to content filter. But we may still change our
     * mind, and reject/discard the message for other reasons.
     */
    if (STREQUAL(value, "FILTER", cmd_len)) {
#ifndef TEST
	if (can_delegate_action(state, table, "FILTER", reply_class) == 0)
	    return (SMTPD_CHECK_DUNNO);
#endif
	if (*cmd_text == 0) {
	    msg_warn("access table %s entry \"%s\" has FILTER entry without value",
		     table, datum);
	    return (SMTPD_CHECK_DUNNO);
	} else if (strchr(cmd_text, ':') == 0) {
	    msg_warn("access table %s entry \"%s\" requires transport:destination",
		     table, datum);
	    return (SMTPD_CHECK_DUNNO);
	} else {
	    vstring_sprintf(error_text, "<%s>: %s triggers FILTER %s",
			    reply_name, reply_class, cmd_text);
	    log_whatsup(state, "filter", STR(error_text));
#ifndef TEST
	    UPDATE_STRING(state->saved_filter, cmd_text);
#endif
	    return (SMTPD_CHECK_DUNNO);
	}
    }

    /*
     * HOLD means deliver later. But we may still change our mind, and
     * reject/discard the message for other reasons.
     */
    if (STREQUAL(value, "HOLD", cmd_len)) {
#ifndef TEST
	if (can_delegate_action(state, table, "HOLD", reply_class) == 0
	    || (state->saved_flags & CLEANUP_FLAG_HOLD))
	    return (SMTPD_CHECK_DUNNO);
#endif
	vstring_sprintf(error_text, "<%s>: %s %s", reply_name, reply_class,
			*cmd_text ? cmd_text : "triggers HOLD action");
	log_whatsup(state, "hold", STR(error_text));
#ifndef TEST
	state->saved_flags |= CLEANUP_FLAG_HOLD;
#endif
	return (SMTPD_CHECK_DUNNO);
    }

    /*
     * DELAY means deliver later. But we may still change our mind, and
     * reject/discard the message for other reasons.
     * 
     * This feature is deleted because it has too many problems. 1) It does not
     * work on some remote file systems; 2) mail will be delivered anyway
     * with "sendmail -q" etc.; 3) while the mail is queued it bogs down the
     * deferred queue scan with huge amounts of useless disk I/O operations.
     */
#ifdef DELAY_ACTION
    if (STREQUAL(value, "DELAY", cmd_len)) {
#ifndef TEST
	if (can_delegate_action(state, table, "DELAY", reply_class) == 0)
	    return (SMTPD_CHECK_DUNNO);
#endif
	if (*cmd_text == 0) {
	    msg_warn("access table %s entry \"%s\" has DELAY entry without value",
		     table, datum);
	    return (SMTPD_CHECK_DUNNO);
	}
	if (conv_time(cmd_text, &defer_delay, 's') == 0) {
	    msg_warn("access table %s entry \"%s\" has invalid DELAY argument \"%s\"",
		     table, datum, cmd_text);
	    return (SMTPD_CHECK_DUNNO);
	}
	vstring_sprintf(error_text, "<%s>: %s %s", reply_name, reply_class,
			*cmd_text ? cmd_text : "triggers DELAY action");
	log_whatsup(state, "delay", STR(error_text));
#ifndef TEST
	state->saved_delay = defer_delay;
#endif
	return (SMTPD_CHECK_DUNNO);
    }
#endif

    /*
     * DISCARD means silently discard and claim successful delivery.
     */
    if (STREQUAL(value, "DISCARD", cmd_len)) {
#ifndef TEST
	if (can_delegate_action(state, table, "DISCARD", reply_class) == 0)
	    return (SMTPD_CHECK_DUNNO);
#endif
	vstring_sprintf(error_text, "<%s>: %s %s", reply_name, reply_class,
			*cmd_text ? cmd_text : "triggers DISCARD action");
	log_whatsup(state, "discard", STR(error_text));
#ifndef TEST
	state->saved_flags |= CLEANUP_FLAG_DISCARD;
	state->discard = 1;
#endif
	return (smtpd_acl_permit(state, STR(buf), reply_class, reply_name,
				 "from %s", table));
    }

    /*
     * REDIRECT means deliver to designated recipient. But we may still
     * change our mind, and reject/discard the message for other reasons.
     */
    if (STREQUAL(value, "REDIRECT", cmd_len)) {
#ifndef TEST
	if (can_delegate_action(state, table, "REDIRECT", reply_class) == 0)
	    return (SMTPD_CHECK_DUNNO);
#endif
	if (strchr(cmd_text, '@') == 0) {
	    msg_warn("access table %s entry \"%s\" requires user@domain target",
		     table, datum);
	    return (SMTPD_CHECK_DUNNO);
	} else {
	    vstring_sprintf(error_text, "<%s>: %s triggers REDIRECT %s",
			    reply_name, reply_class, cmd_text);
	    log_whatsup(state, "redirect", STR(error_text));
#ifndef TEST
	    UPDATE_STRING(state->saved_redirect, cmd_text);
#endif
	    return (SMTPD_CHECK_DUNNO);
	}
    }

    /*
     * BCC means deliver to designated recipient. But we may still change our
     * mind, and reject/discard the message for other reasons.
     */
    if (STREQUAL(value, "BCC", cmd_len)) {
#ifndef TEST
	if (can_delegate_action(state, table, "BCC", reply_class) == 0)
	    return (SMTPD_CHECK_DUNNO);
#endif
	if (strchr(cmd_text, '@') == 0) {
	    msg_warn("access table %s entry \"%s\" requires user@domain target",
		     table, datum);
	    return (SMTPD_CHECK_DUNNO);
	} else {
	    vstring_sprintf(error_text, "<%s>: %s triggers BCC %s",
			    reply_name, reply_class, cmd_text);
	    log_whatsup(state, "bcc", STR(error_text));
#ifndef TEST
	    if (state->saved_bcc == 0)
		state->saved_bcc = argv_alloc(1);
	    argv_add(state->saved_bcc, cmd_text, (char *) 0);
#endif
	    return (SMTPD_CHECK_DUNNO);
	}
    }

    /*
     * DEFER_IF_PERMIT changes "permit" into "maybe". Use optional text or
     * generate a generic error response.
     */
    if (STREQUAL(value, DEFER_IF_PERMIT, cmd_len)) {
	dsn_split(&dp, "4.7.1", cmd_text);
	return (DEFER_IF_PERMIT3(DEFER_IF_PERMIT_ACT, state, MAIL_ERROR_POLICY,
				 var_map_defer_code,
			     smtpd_dsn_fix(DSN_STATUS(dp.dsn), reply_class),
				 "<%s>: %s rejected: %s",
				 reply_name, reply_class,
			       *dp.text ? dp.text : "Service unavailable"));
    }

    /*
     * DEFER_IF_REJECT changes "reject" into "maybe". Use optional text or
     * generate a generic error response.
     */
    if (STREQUAL(value, DEFER_IF_REJECT, cmd_len)) {
	dsn_split(&dp, "4.7.1", cmd_text);
	DEFER_IF_REJECT3(state, MAIL_ERROR_POLICY,
			 var_map_defer_code,
			 smtpd_dsn_fix(DSN_STATUS(dp.dsn), reply_class),
			 "<%s>: %s rejected: %s",
			 reply_name, reply_class,
			 *dp.text ? dp.text : "Service unavailable");
	return (SMTPD_CHECK_DUNNO);
    }

    /*
     * PREPEND prepends the specified message header text.
     */
    if (STREQUAL(value, "PREPEND", cmd_len)) {
#ifndef TEST
	/* XXX what about ETRN. */
	if (not_in_client_helo(state, table, "PREPEND", reply_class) == 0)
	    return (SMTPD_CHECK_DUNNO);
#endif
	if (strcmp(state->where, SMTPD_AFTER_EOM) == 0) {
	    msg_warn("access table %s: action PREPEND must be used before %s",
		     table, VAR_EOD_CHECKS);
	    return (SMTPD_CHECK_DUNNO);
	}
	if (*cmd_text == 0 || is_header(cmd_text) == 0) {
	    msg_warn("access table %s entry \"%s\" requires header: text",
		     table, datum);
	    return (SMTPD_CHECK_DUNNO);
	} else {
	    if (state->prepend == 0)
		state->prepend = argv_alloc(1);
	    argv_add(state->prepend, cmd_text, (char *) 0);
	    return (SMTPD_CHECK_DUNNO);
	}
    }

    /*
     * All-numeric result probably means OK - some out-of-band authentication
     * mechanism uses this as time stamp.
     */
    if (alldig(value))
	return (smtpd_acl_permit(state, STR(buf), reply_class, reply_name,
				 "from %s", table));

    /*
     * 4xx or 5xx means NO as well. smtpd_check_reject() will validate the
     * response status code.
     * 
     * If the caller specifies an RFC 3463 enhanced status code, put it
     * immediately after the SMTP status code as described in RFC 2034.
     */
    if (cmd_len == 3 && *cmd_text
	&& (value[0] == '4' || value[0] == '5')
	&& ISDIGIT(value[1]) && ISDIGIT(value[2])) {
	code = atoi(value);
	def_dsn[0] = value[0];
	dsn_split(&dp, def_dsn, cmd_text);
	return (smtpd_check_reject(state, MAIL_ERROR_POLICY,
				   code,
				   smtpd_dsn_fix(DSN_STATUS(dp.dsn),
						 reply_class),
				   "<%s>: %s rejected: %s",
				   reply_name, reply_class,
				   *dp.text ? dp.text : "Access denied"));
    }

    /*
     * OK or RELAY means YES. Ignore trailing text.
     */
    if (STREQUAL(value, "OK", cmd_len) || STREQUAL(value, "RELAY", cmd_len))
	return (smtpd_acl_permit(state, STR(buf), reply_class, reply_name,
				 "from %s", table));

    /*
     * Unfortunately, maps must be declared ahead of time so they can be
     * opened before we go to jail. We could insist that the RHS can only
     * contain a pre-defined restriction class name, but that would be too
     * restrictive. Instead we warn if an access table references any map.
     * 
     * XXX Don't use passwd files or address rewriting maps as access tables.
     */
    if (strchr(value, ':') != 0) {
	msg_warn("access table %s has entry with lookup table: %s",
		 table, value);
	msg_warn("do not specify lookup tables inside SMTPD access maps");
	msg_warn("define a restriction class and specify its name instead.");
	reject_server_error(state);
    }

    /*
     * Don't get carried away with recursion.
     */
    if (state->recursion > 100) {
	msg_warn("access table %s entry %s causes unreasonable recursion",
		 table, value);
	reject_server_error(state);
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

    restrictions = argv_splitq(value, CHARS_COMMA_SP, CHARS_BRACE);
    memcpy(ADDROF(savebuf), ADDROF(smtpd_check_buf), sizeof(savebuf));
    status = setjmp(smtpd_check_buf);
    if (status != 0) {
	argv_free(restrictions);
	memcpy(ADDROF(smtpd_check_buf), ADDROF(savebuf),
	       sizeof(smtpd_check_buf));
	longjmp(smtpd_check_buf, status);
    }
    if (restrictions->argc == 0) {
	msg_warn("access table %s entry %s has empty value",
		 table, value);
	status = SMTPD_CHECK_OK;
    } else {
	status = generic_checks(state, restrictions, reply_name,
				reply_class, def_acl);
    }
    argv_free(restrictions);
    memcpy(ADDROF(smtpd_check_buf), ADDROF(savebuf), sizeof(smtpd_check_buf));
    return (status);
}

/* check_access - table lookup without substring magic */

static int check_access(SMTPD_STATE *state, const char *table, const char *name,
		              int flags, int *found, const char *reply_name,
			        const char *reply_class, const char *def_acl)
{
    const char *myname = "check_access";
    const char *value;
    MAPS   *maps;

#define CHK_ACCESS_RETURN(x,y) \
	{ *found = y; return(x); }
#define FULL	0
#define PARTIAL	DICT_FLAG_FIXED
#define FOUND	1
#define MISSED	0

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

    if ((maps = (MAPS *) htable_find(map_command_table, table)) == 0) {
	msg_warn("%s: unexpected dictionary: %s", myname, table);
	value = "451 4.3.5 Server configuration error";
	CHK_ACCESS_RETURN(check_table_result(state, table, value, name,
					     reply_name, reply_class,
					     def_acl), FOUND);
    }
    if ((value = maps_find(maps, name, flags)) != 0)
	CHK_ACCESS_RETURN(check_table_result(state, table, value, name,
					     reply_name, reply_class,
					     def_acl), FOUND);
    if (maps->error != 0) {
	/* Warning is already logged. */
	value = "451 4.3.5 Server configuration error";
	CHK_ACCESS_RETURN(check_table_result(state, table, value, name,
					     reply_name, reply_class,
					     def_acl), FOUND);
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
    const char *myname = "check_domain_access";
    const char *name;
    const char *next;
    const char *value;
    MAPS   *maps;
    int     maybe_numerical = 1;

    if (msg_verbose)
	msg_info("%s: %s", myname, domain);

    /*
     * Try the name and its parent domains. Including top-level domains.
     * 
     * Helo names can end in ".". The test below avoids lookups of the empty
     * key, because Berkeley DB cannot deal with it. [Victor Duchovni, Morgan
     * Stanley].
     * 
     * TODO(wietse) move to mail_domain_find library module.
     */
#define CHK_DOMAIN_RETURN(x,y) { *found = y; return(x); }

    if ((maps = (MAPS *) htable_find(map_command_table, table)) == 0) {
	msg_warn("%s: unexpected dictionary: %s", myname, table);
	value = "451 4.3.5 Server configuration error";
	CHK_DOMAIN_RETURN(check_table_result(state, table, value,
					     domain, reply_name, reply_class,
					     def_acl), FOUND);
    }
    for (name = domain; *name != 0; name = next) {
	if ((value = maps_find(maps, name, flags)) != 0)
	    CHK_DOMAIN_RETURN(check_table_result(state, table, value,
					    domain, reply_name, reply_class,
						 def_acl), FOUND);
	if (maps->error != 0) {
	    /* Warning is already logged. */
	    value = "451 4.3.5 Server configuration error";
	    CHK_DOMAIN_RETURN(check_table_result(state, table, value,
					    domain, reply_name, reply_class,
						 def_acl), FOUND);
	}
	/* Don't apply subdomain magic to numerical hostnames. */
	if (maybe_numerical
	    && (maybe_numerical = valid_hostaddr(domain, DONT_GRIPE)) != 0)
	    break;
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
    const char *myname = "check_addr_access";
    char   *addr;
    const char *value;
    MAPS   *maps;
    int     delim;

    if (msg_verbose)
	msg_info("%s: %s", myname, address);

    /*
     * Try the address and its parent networks.
     * 
     * TODO(wietse) move to mail_ipaddr_find library module.
     */
#define CHK_ADDR_RETURN(x,y) { *found = y; return(x); }

    addr = STR(vstring_strcpy(error_text, address));
#ifdef HAS_IPV6
    if (strchr(addr, ':') != 0)
	delim = ':';
    else
#endif
	delim = '.';

    if ((maps = (MAPS *) htable_find(map_command_table, table)) == 0) {
	msg_warn("%s: unexpected dictionary: %s", myname, table);
	value = "451 4.3.5 Server configuration error";
	CHK_ADDR_RETURN(check_table_result(state, table, value, address,
					   reply_name, reply_class,
					   def_acl), FOUND);
    }
    do {
	if ((value = maps_find(maps, addr, flags)) != 0)
	    CHK_ADDR_RETURN(check_table_result(state, table, value, address,
					       reply_name, reply_class,
					       def_acl), FOUND);
	if (maps->error != 0) {
	    /* Warning is already logged. */
	    value = "451 4.3.5 Server configuration error";
	    CHK_ADDR_RETURN(check_table_result(state, table, value, address,
					       reply_name, reply_class,
					       def_acl), FOUND);
	}
	flags = PARTIAL;
    } while (split_at_right(addr, delim));

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
    const char *myname = "check_namadr_access";
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

/* check_server_access - access control by server host name or address */

static int check_server_access(SMTPD_STATE *state, const char *table,
			               const char *name,
			               int type,
			               const char *reply_name,
			               const char *reply_class,
			               const char *def_acl)
{
    const char *myname = "check_server_access";
    const char *domain;
    const char *adomain;
    int     dns_status;
    DNS_RR *server_list;
    DNS_RR *server;
    int     found = 0;
    MAI_HOSTADDR_STR addr_string;
    int     aierr;
    struct addrinfo *res0;
    struct addrinfo *res;
    int     status;
    const INET_PROTO_INFO *proto_info;

    /*
     * Sanity check.
     */
    if (type != T_MX && type != T_NS && type != T_A
#ifdef HAS_IPV6
	&& type != T_AAAA
#endif
	)
	msg_panic("%s: unexpected resource type \"%s\" in request",
		  myname, dns_strtype(type));

    if (msg_verbose)
	msg_info("%s: %s %s", myname, dns_strtype(type), name);

    /*
     * Skip over local-part.
     */
    if ((domain = strrchr(name, '@')) != 0)
	domain += 1;
    else
	domain = name;

    /*
     * Treat an address literal as its own MX server, just like we treat a
     * name without MX record as its own MX server. There is, however, no
     * applicable NS server equivalent.
     */
    if (*domain == '[') {
	char   *saved_addr;
	const char *bare_addr;
	ssize_t len;

	if (type != T_A && type != T_MX)
	    return (SMTPD_CHECK_DUNNO);
	len = strlen(domain);
	if (domain[len - 1] != ']')
	    return (SMTPD_CHECK_DUNNO);
	/* Memory leak alert: no early returns after this point. */
	saved_addr = mystrndup(domain + 1, len - 2);
	if ((bare_addr = valid_mailhost_addr(saved_addr, DONT_GRIPE)) == 0)
	    status = SMTPD_CHECK_DUNNO;
	else
	    status = check_addr_access(state, table, bare_addr, FULL,
				       &found, reply_name, reply_class,
				       def_acl);
	myfree(saved_addr);
	return (status);
    }

    /*
     * Fix 20140924: convert domain to ASCII.
     */
#ifndef NO_EAI
    if (!allascii(domain) && (adomain = midna_domain_to_ascii(domain)) != 0) {
	if (msg_verbose)
	    msg_info("%s asciified to %s", domain, adomain);
	domain = adomain;
    }
#endif

    /*
     * If the request is type A or AAAA, fabricate an MX record that points
     * to the domain name itself, and skip name-based access control.
     * 
     * If the domain name does not exist then we apply no restriction.
     * 
     * If the domain name exists but no MX record exists, fabricate an MX record
     * that points to the domain name itself.
     * 
     * If the domain name exists but no NS record exists, look up parent domain
     * NS records.
     * 
     * XXX 20150707 Work around broken DNS servers that reply with NXDOMAIN
     * instead of "no data".
     */
    if (type == T_A
#ifdef HAS_IPV6
	|| type == T_AAAA
#endif
	) {
	server_list = dns_rr_create_nopref(domain, domain, T_MX, C_IN, 0,
					   domain, strlen(domain) + 1);
    } else {
	dns_status = dns_lookup(domain, type, 0, &server_list,
				(VSTRING *) 0, (VSTRING *) 0);
	if (dns_status == DNS_NULLMX)
	    return (SMTPD_CHECK_DUNNO);
	if (dns_status == DNS_NOTFOUND /* Not: h_errno == NO_DATA */ ) {
	    if (type == T_MX) {
		server_list = dns_rr_create_nopref(domain, domain, type, C_IN,
					     0, domain, strlen(domain) + 1);
		dns_status = DNS_OK;
	    } else if (type == T_NS /* && h_errno == NO_DATA */ ) {
		while ((domain = strchr(domain, '.')) != 0 && domain[1]) {
		    domain += 1;
		    dns_status = dns_lookup(domain, type, 0, &server_list,
					    (VSTRING *) 0, (VSTRING *) 0);
		    if (dns_status != DNS_NOTFOUND /* || h_errno != NO_DATA */ )
			break;
		}
	    }
	}
	if (dns_status != DNS_OK) {
	    msg_warn("Unable to look up %s host for %s: %s", dns_strtype(type),
		     domain && domain[1] ? domain : name,
		     dns_status == DNS_POLICY ?
		     "DNS reply filter policy" :
		     dns_strerror(dns_get_h_errno()));
	    return (SMTPD_CHECK_DUNNO);
	}
    }

    /*
     * No bare returns after this point or we have a memory leak.
     */
#define CHECK_SERVER_RETURN(x) { dns_rr_free(server_list); return(x); }

    /*
     * Check the hostnames first, then the addresses.
     */
    proto_info = inet_proto_info();
    for (server = server_list; server != 0; server = server->next) {
	if (msg_verbose)
	    msg_info("%s: %s hostname check: %s",
		     myname, dns_strtype(type), (char *) server->data);
	if (valid_hostaddr((char *) server->data, DONT_GRIPE)) {
	    if ((status = check_addr_access(state, table, (char *) server->data,
				      FULL, &found, reply_name, reply_class,
					    def_acl)) != 0 || found)
		CHECK_SERVER_RETURN(status);
	    continue;
	}
	if (type != T_A && type != T_AAAA
	    && ((status = check_domain_access(state, table, (char *) server->data,
				      FULL, &found, reply_name, reply_class,
					      def_acl)) != 0 || found))
	    CHECK_SERVER_RETURN(status);
	if ((aierr = hostname_to_sockaddr((char *) server->data,
					  (char *) 0, 0, &res0)) != 0) {
	    if (type != T_A && type != T_AAAA)
		msg_warn("Unable to look up %s host %s for %s %s: %s",
			 dns_strtype(type), (char *) server->data,
			 reply_class, reply_name, MAI_STRERROR(aierr));
	    continue;
	}
	/* Now we must also free the addrinfo result. */
	if (msg_verbose)
	    msg_info("%s: %s host address check: %s",
		     myname, dns_strtype(type), (char *) server->data);
	for (res = res0; res != 0; res = res->ai_next) {
	    if (strchr((char *) proto_info->sa_family_list, res->ai_family) == 0) {
		if (msg_verbose)
		    msg_info("skipping address family %d for host %s",
			     res->ai_family, server->data);
		continue;
	    }
	    SOCKADDR_TO_HOSTADDR(res->ai_addr, res->ai_addrlen,
				 &addr_string, (MAI_SERVPORT_STR *) 0, 0);
	    status = check_addr_access(state, table, addr_string.buf, FULL,
				       &found, reply_name, reply_class,
				       def_acl);
	    if (status != 0 || found) {
		freeaddrinfo(res0);		/* 200412 */
		CHECK_SERVER_RETURN(status);
	    }
	}
	freeaddrinfo(res0);			/* 200412 */
    }
    CHECK_SERVER_RETURN(SMTPD_CHECK_DUNNO);
}

/* check_ccert_access - access for TLS clients by certificate fingerprint */

static int check_ccert_access(SMTPD_STATE *state, const char *acl_spec,
			              const char *def_acl)
{
    int     result = SMTPD_CHECK_DUNNO;

#ifdef USE_TLS
    const char *myname = "check_ccert_access";
    int     found;
    const MAP_SEARCH *acl;
    const char default_search[] = {
	SMTPD_ACL_SEARCH_CODE_CERT_FPRINT,
	SMTPD_ACL_SEARCH_CODE_PKEY_FPRINT,
	0,
    };
    const char *search_order;

    /*
     * Look up the acl search list. If there is no ACL then we don't have a
     * table to check.
     */
    if ((acl = map_search_lookup(acl_spec)) == 0) {
	msg_warn("See earlier parsing error messages for '%s", acl_spec);
	return (smtpd_check_reject(state, MAIL_ERROR_SOFTWARE, 451, "4.3.5",
				   "Server configuration error"));
    }
    if ((search_order = acl->search_order) == 0)
	search_order = default_search;
    if (msg_verbose)
	msg_info("%s: search_order length=%ld",
		 myname, (long) strlen(search_order));

    /*
     * When directly checking the fingerprint, it is OK if the issuing CA is
     * not trusted.
     */
    if (TLS_CERT_IS_PRESENT(state->tls_context)) {
	const char *action;
	const char *match_this;
	const char *known_action;

	for (action = search_order; *action; action++) {
	    switch (*action) {
	    case SMTPD_ACL_SEARCH_CODE_CERT_FPRINT:
		match_this = state->tls_context->peer_cert_fprint;
		if (warn_compat_break_smtpd_tls_fpt_dgst)
		    msg_info("using backwards-compatible default setting "
			     VAR_SMTPD_TLS_FPT_DGST "=md5 to compute "
			     "certificate fingerprints");
		break;
	    case SMTPD_ACL_SEARCH_CODE_PKEY_FPRINT:
		match_this = state->tls_context->peer_pkey_fprint;
		if (warn_compat_break_smtpd_tls_fpt_dgst)
		    msg_info("using backwards-compatible default setting "
			     VAR_SMTPD_TLS_FPT_DGST "=md5 to compute "
			     "certificate fingerprints");
		break;
	    default:
		known_action = str_name_code(search_actions, *action);
		if (known_action == 0)
		    msg_panic("%s: unknown action #%d in '%s'",
			      myname, *action, acl_spec);
		msg_warn("%s: unexpected action '%s' in '%s'",
			 myname, known_action, acl_spec);
		return (smtpd_check_reject(state, MAIL_ERROR_SOFTWARE,
					   451, "4.3.5",
					   "Server configuration error"));
	    }
	    if (msg_verbose)
		msg_info("%s: look up %s %s",
			 myname, str_name_code(search_actions, *action),
			 match_this);

	    /*
	     * Log the peer CommonName when access is denied. Non-printable
	     * characters will be neutered by smtpd_check_reject(). The SMTP
	     * client name and address are always syslogged as part of a
	     * "reject" event. XXX Should log the thing that is rejected
	     * (fingerprint etc.) or would that give away too much?
	     */
	    result = check_access(state, acl->map_type_name, match_this,
				  DICT_FLAG_NONE, &found,
				  state->tls_context->peer_CN,
				  SMTPD_NAME_CCERT, def_acl);
	    if (result != SMTPD_CHECK_DUNNO)
		break;
	}
    } else if (!var_smtpd_tls_ask_ccert) {
	msg_warn("%s is requested, but \"%s = no\"",
		 CHECK_CCERT_ACL, VAR_SMTPD_TLS_ACERT);
    } else {
	if (msg_verbose)
	    msg_info("%s: no client certificate", myname);
    }
#endif
    return (result);
}

/* check_sasl_access - access by SASL user name */

#ifdef USE_SASL_AUTH

static int check_sasl_access(SMTPD_STATE *state, const char *table,
			             const char *def_acl)
{
    int     result;
    int     unused_found;
    char   *sane_username = printable(mystrdup(state->sasl_username), '_');

    result = check_access(state, table, state->sasl_username,
			  DICT_FLAG_NONE, &unused_found, sane_username,
			  SMTPD_NAME_SASL_USER, def_acl);
    myfree(sane_username);
    return (result);
}

#endif

/* check_mail_access - OK/FAIL based on mail address lookup */

static int check_mail_access(SMTPD_STATE *state, const char *table,
			             const char *addr, int *found,
			             const char *reply_name,
			             const char *reply_class,
			             const char *def_acl)
{
    const char *myname = "check_mail_access";
    const RESOLVE_REPLY *reply;
    const char *value;
    int     lookup_strategy;
    int     status;
    MAPS   *maps;

    if (msg_verbose)
	msg_info("%s: %s", myname, addr);

    /*
     * Resolve the address.
     */
    reply = smtpd_resolve_addr(strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
			       state->recipient : state->sender, addr);
    if (reply->flags & RESOLVE_FLAG_FAIL)
	reject_dict_retry(state, addr);

    /*
     * Garbage in, garbage out. Every address from rewrite_clnt_internal()
     * and from resolve_clnt_query() must be fully qualified.
     */
    if (strrchr(CONST_STR(reply->recipient), '@') == 0) {
	msg_warn("%s: no @domain in address: %s", myname,
		 CONST_STR(reply->recipient));
	return (0);
    }

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
    lookup_strategy = MA_FIND_FULL | MA_FIND_NOEXT | MA_FIND_DOMAIN
	| MA_FIND_LOCALPART_AT
	| (access_parent_style == MATCH_FLAG_PARENT ?
	   MA_FIND_PDMS : MA_FIND_PDDMDS);

    if ((maps = (MAPS *) htable_find(map_command_table, table)) == 0) {
	msg_warn("%s: unexpected dictionary: %s", myname, table);
	value = "451 4.3.5 Server configuration error";
	return (check_table_result(state, table, value,
				   CONST_STR(reply->recipient),
				   reply_name, reply_class,
				   def_acl));
    }
    if ((value = mail_addr_find_strategy(maps, CONST_STR(reply->recipient),
				      (char **) 0, lookup_strategy)) != 0) {
	*found = 1;
	status = check_table_result(state, table, value,
				    CONST_STR(reply->recipient),
				    reply_name, reply_class, def_acl);
	return (status == SMTPD_CHECK_OK && SUSPICIOUS(reply, reply_class) ?
		SMTPD_CHECK_DUNNO : status);
    } else if (maps->error != 0) {
	/* Warning is already logged. */
	value = "451 4.3.5 Server configuration error";
	return (check_table_result(state, table, value,
				   CONST_STR(reply->recipient),
				   reply_name, reply_class,
				   def_acl));
    }

    /*
     * Undecided when no match found.
     */
    return (SMTPD_CHECK_DUNNO);
}

/* Support for different DNSXL lookup results. */

static SMTPD_RBL_STATE dnsxl_stat_soft[1];

#define SMTPD_DNSXL_STAT_SOFT(dnsxl_res) ((dnsxl_res) == dnsxl_stat_soft)
#define SMTPD_DNXSL_STAT_HARD(dnsxl_res) ((dnsxl_res) == 0)
#define SMTPD_DNSXL_STAT_OK(dnsxl_res) \
	!(SMTPD_DNXSL_STAT_HARD(dnsxl_res) || SMTPD_DNSXL_STAT_SOFT(dnsxl_res))

/* rbl_pagein - look up an RBL lookup result */

static void *rbl_pagein(const char *query, void *unused_context)
{
    DNS_RR *txt_list;
    VSTRING *why;
    int     dns_status;
    SMTPD_RBL_STATE *rbl = 0;
    DNS_RR *addr_list;
    DNS_RR *rr;
    DNS_RR *next;
    VSTRING *buf;
    int     space_left;

    /*
     * Do the query. If the DNS lookup produces no definitive reply, give the
     * requestor the benefit of the doubt. We can't block all email simply
     * because an RBL server is unavailable.
     * 
     * Don't do this for AAAA records. Yet.
     */
    why = vstring_alloc(10);
    dns_status = dns_lookup(query, T_A, 0, &addr_list, (VSTRING *) 0, why);
    if (dns_status != DNS_OK && dns_status != DNS_NOTFOUND) {
	msg_warn("%s: RBL lookup error: %s", query, STR(why));
	rbl = dnsxl_stat_soft;
    }
    vstring_free(why);
    if (dns_status != DNS_OK)
	return ((void *) rbl);

    /*
     * Save the result. Yes, we cache negative results as well as positive
     * results. Concatenate multiple TXT records, up to some limit.
     */
#define RBL_TXT_LIMIT	500

    rbl = (SMTPD_RBL_STATE *) mymalloc(sizeof(*rbl));
    dns_status = dns_lookup(query, T_TXT, 0, &txt_list,
			    (VSTRING *) 0, (VSTRING *) 0);
    if (dns_status == DNS_OK) {
	buf = vstring_alloc(1);
	space_left = RBL_TXT_LIMIT;
	for (rr = txt_list; rr != 0 && space_left > 0; rr = next) {
	    vstring_strncat(buf, rr->data, (int) rr->data_len > space_left ?
			    space_left : rr->data_len);
	    space_left = RBL_TXT_LIMIT - VSTRING_LEN(buf);
	    next = rr->next;
	    if (next && space_left > 3) {
		vstring_strcat(buf, " / ");
		space_left -= 3;
	    }
	}
	rbl->txt = vstring_export(buf);
	dns_rr_free(txt_list);
    } else {
	if (dns_status == DNS_POLICY)
	    msg_warn("%s: TXT lookup error: %s",
		     query, "DNS reply filter drops all results");
	rbl->txt = 0;
    }
    rbl->a = addr_list;
    return ((void *) rbl);
}

/* rbl_pageout - discard an RBL lookup result */

static void rbl_pageout(void *data, void *unused_context)
{
    SMTPD_RBL_STATE *rbl = (SMTPD_RBL_STATE *) data;

    if (SMTPD_DNSXL_STAT_OK(rbl)) {
	if (rbl->txt)
	    myfree(rbl->txt);
	if (rbl->a)
	    dns_rr_free(rbl->a);
	myfree((void *) rbl);
    }
}

/* rbl_byte_pagein - parse RBL reply pattern, save byte codes */

static void *rbl_byte_pagein(const char *query, void *unused_context)
{
    VSTRING *byte_codes = vstring_alloc(100);
    char   *saved_query = mystrdup(query);
    char   *saved_byte_codes;
    char   *err;

    if ((err = ip_match_parse(byte_codes, saved_query)) != 0)
	msg_fatal("RBL reply error: %s", err);
    saved_byte_codes = ip_match_save(byte_codes);
    myfree(saved_query);
    vstring_free(byte_codes);
    return (saved_byte_codes);
}

/* rbl_byte_pageout - discard parsed RBL reply byte codes */

static void rbl_byte_pageout(void *data, void *unused_context)
{
    myfree(data);
}

/* rbl_expand_lookup - RBL specific $name expansion */

static const char *rbl_expand_lookup(const char *name, int mode,
				             void *context)
{
    SMTPD_RBL_EXPAND_CONTEXT *rbl_exp = (SMTPD_RBL_EXPAND_CONTEXT *) context;
    SMTPD_STATE *state = rbl_exp->state;

#define STREQ(x,y) (*(x) == *(y) && strcmp((x), (y)) == 0)

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
	return (rbl_exp->txt);
    } else if (STREQ(name, MAIL_ATTR_RBL_TXT)) {/* LaMont compat */
	return (rbl_exp->txt);
    } else if (STREQ(name, MAIL_ATTR_RBL_WHAT)) {
	return (rbl_exp->what);
    } else if (STREQ(name, MAIL_ATTR_RBL_CLASS)) {
	return (rbl_exp->class);
    } else {
	return (smtpd_expand_lookup(name, mode, (void *) state));
    }
}

/* rbl_reject_reply - format reply after RBL reject */

static int rbl_reject_reply(SMTPD_STATE *state, const SMTPD_RBL_STATE *rbl,
			            const char *rbl_domain,
			            const char *what,
			            const char *reply_class)
{
    const char *myname = "rbl_reject_reply";
    VSTRING *why = 0;
    const char *template = 0;
    SMTPD_RBL_EXPAND_CONTEXT rbl_exp;
    int     result;
    DSN_SPLIT dp;
    int     code;

    /*
     * Use the server-specific reply template or use the default one.
     */
    if (*var_rbl_reply_maps) {
	template = maps_find(rbl_reply_maps, rbl_domain, DICT_FLAG_NONE);
	if (rbl_reply_maps->error)
	    reject_server_error(state);
    }
    why = vstring_alloc(100);
    rbl_exp.state = state;
    rbl_exp.domain = mystrdup(rbl_domain);
    (void) split_at(rbl_exp.domain, '=');
    rbl_exp.what = what;
    rbl_exp.class = reply_class;
    rbl_exp.txt = (rbl->txt == 0 ? "" : rbl->txt);

    for (;;) {
	if (template == 0)
	    template = var_def_rbl_reply;
	if (mac_expand(why, template, MAC_EXP_FLAG_NONE,
		       STR(smtpd_expand_filter), rbl_expand_lookup,
		       (void *) &rbl_exp) == 0)
	    break;
	if (template == var_def_rbl_reply)
	    msg_fatal("%s: bad default rbl reply template: %s",
		      myname, var_def_rbl_reply);
	msg_warn("%s: bad rbl reply template for domain %s: %s",
		 myname, rbl_domain, template);
	template = 0;				/* pretend not found */
    }

    /*
     * XXX Impedance mis-match.
     * 
     * Validate the response, that is, the response must begin with a
     * three-digit status code, and the first digit must be 4 or 5. If the
     * response is bad, log a warning and send a generic response instead.
     */
    if ((STR(why)[0] != '4' && STR(why)[0] != '5')
	|| !ISDIGIT(STR(why)[1]) || !ISDIGIT(STR(why)[2])
	|| STR(why)[3] != ' ') {
	msg_warn("rbl response code configuration error: %s", STR(why));
	result = smtpd_check_reject(state, MAIL_ERROR_POLICY,
				    450, "4.7.1", "Service unavailable");
    } else {
	code = atoi(STR(why));
	dsn_split(&dp, "4.7.1", STR(why) + 4);
	result = smtpd_check_reject(state, MAIL_ERROR_POLICY,
				    code,
				    smtpd_dsn_fix(DSN_STATUS(dp.dsn),
						  reply_class),
				    "%s", *dp.text ?
				    dp.text : "Service unavailable");
    }

    /*
     * Clean up.
     */
    myfree(rbl_exp.domain);
    vstring_free(why);

    return (result);
}

/* rbl_match_addr - match address list */

static int rbl_match_addr(SMTPD_RBL_STATE *rbl, const char *byte_codes)
{
    const char *myname = "rbl_match_addr";
    DNS_RR *rr;

    for (rr = rbl->a; rr != 0; rr = rr->next) {
	if (rr->type == T_A) {
	    if (ip_match_execute(byte_codes, rr->data))
		return (1);
	} else {
	    msg_warn("%s: skipping record type %s for query %s",
		     myname, dns_strtype(rr->type), rr->qname);
	}
    }
    return (0);
}

/* find_dnsxl_addr - look up address in DNSXL */

static const SMTPD_RBL_STATE *find_dnsxl_addr(SMTPD_STATE *state,
					              const char *rbl_domain,
					              const char *addr)
{
    const char *myname = "find_dnsxl_addr";
    ARGV   *octets;
    VSTRING *query;
    int     i;
    SMTPD_RBL_STATE *rbl;
    const char *reply_addr;
    const char *byte_codes;
    struct addrinfo *res;
    unsigned char *ipv6_addr;

    query = vstring_alloc(100);

    /*
     * Reverse the client IPV6 address, represented as 32 hexadecimal
     * nibbles. We use the binary address to avoid tricky code. Asking for an
     * AAAA record makes no sense here. Just like with IPv4 we use the lookup
     * result as a bit mask, not as an IP address.
     */
#ifdef HAS_IPV6
    if (valid_ipv6_hostaddr(addr, DONT_GRIPE)) {
	if (hostaddr_to_sockaddr(addr, (char *) 0, 0, &res) != 0
	    || res->ai_family != PF_INET6)
	    msg_fatal("%s: unable to convert address %s", myname, addr);
	ipv6_addr = (unsigned char *) &SOCK_ADDR_IN6_ADDR(res->ai_addr);
	for (i = sizeof(SOCK_ADDR_IN6_ADDR(res->ai_addr)) - 1; i >= 0; i--)
	    vstring_sprintf_append(query, "%x.%x.",
				   ipv6_addr[i] & 0xf, ipv6_addr[i] >> 4);
	freeaddrinfo(res);
    } else
#endif

	/*
	 * Reverse the client IPV4 address, represented as four decimal octet
	 * values. We use the textual address for convenience.
	 */
    {
	octets = argv_split(addr, ".");
	for (i = octets->argc - 1; i >= 0; i--) {
	    vstring_strcat(query, octets->argv[i]);
	    vstring_strcat(query, ".");
	}
	argv_free(octets);
    }

    /*
     * Tack on the RBL domain name and query the DNS for an A record.
     */
    vstring_strcat(query, rbl_domain);
    reply_addr = split_at(STR(query), '=');
    rbl = (SMTPD_RBL_STATE *) ctable_locate(smtpd_rbl_cache, STR(query));
    if (reply_addr != 0)
	byte_codes = ctable_locate(smtpd_rbl_byte_cache, reply_addr);

    /*
     * If the record exists, match the result address.
     */
    if (SMTPD_DNSXL_STAT_OK(rbl) && reply_addr != 0
	&& !rbl_match_addr(rbl, byte_codes))
	rbl = 0;
    vstring_free(query);
    return (rbl);
}

/* reject_rbl_addr - reject address in DNS deny list */

static int reject_rbl_addr(SMTPD_STATE *state, const char *rbl_domain,
			           const char *addr, const char *reply_class)
{
    const char *myname = "reject_rbl_addr";
    const SMTPD_RBL_STATE *rbl;

    if (msg_verbose)
	msg_info("%s: %s %s", myname, reply_class, addr);

    rbl = find_dnsxl_addr(state, rbl_domain, addr);
    if (!SMTPD_DNSXL_STAT_OK(rbl)) {
	return (SMTPD_CHECK_DUNNO);
    } else {
	return (rbl_reject_reply(state, rbl, rbl_domain, addr, reply_class));
    }
}

/* permit_dnswl_addr - permit address in DNSWL */

static int permit_dnswl_addr(SMTPD_STATE *state, const char *dnswl_domain,
			          const char *addr, const char *reply_class)
{
    const char *myname = "permit_dnswl_addr";
    const SMTPD_RBL_STATE *dnswl_result;

    if (msg_verbose)
	msg_info("%s: %s", myname, addr);

    /* Safety: don't allowlist unauthorized recipients. */
    if (strcmp(state->where, SMTPD_CMD_RCPT) == 0 && state->recipient != 0
      && permit_auth_destination(state, state->recipient) != SMTPD_CHECK_OK)
	return (SMTPD_CHECK_DUNNO);

    dnswl_result = find_dnsxl_addr(state, dnswl_domain, addr);
    if (SMTPD_DNXSL_STAT_HARD(dnswl_result)) {
	return (SMTPD_CHECK_DUNNO);
    } else if (SMTPD_DNSXL_STAT_SOFT(dnswl_result)) {
	/* XXX: Make configurable as dnswl_tempfail_action. */
	DEFER_IF_REJECT3(state, MAIL_ERROR_POLICY,
			 450, "4.7.1",
			 "<%s>: %s rejected: %s",
			 addr, reply_class,
			 "Service unavailable");
	return (SMTPD_CHECK_DUNNO);
    } else if (SMTPD_DNSXL_STAT_OK(dnswl_result)) {
	return (SMTPD_CHECK_OK);
    } else {
	/* Future proofing, in case find_dnsxl_addr() result is changed. */
	msg_panic("%s: find_dnsxl_addr API failure", myname);
    }
}

/* find_dnsxl_domain - reject if domain in DNS deny list */

static const SMTPD_RBL_STATE *find_dnsxl_domain(SMTPD_STATE *state,
			           const char *rbl_domain, const char *what)
{
    VSTRING *query;
    SMTPD_RBL_STATE *rbl;
    const char *domain;
    const char *reply_addr;
    const char *byte_codes;
    const char *suffix;
    const char *adomain;

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

    /*
     * XXX Some Spamhaus RHSBL rejects lookups with "No IP queries" even if
     * the name has an alphanumerical prefix. We play safe, and skip both
     * RHSBL and RHSWL queries for names ending in a numerical suffix.
     */
    if (domain[0] == 0)
	return (SMTPD_CHECK_DUNNO);
    suffix = strrchr(domain, '.');
    if (alldig(suffix == 0 ? domain : suffix + 1))
	return (SMTPD_CHECK_DUNNO);

    /*
     * Fix 20140706: convert domain to ASCII.
     */
#ifndef NO_EAI
    if (!allascii(domain) && (adomain = midna_domain_to_ascii(domain)) != 0) {
	if (msg_verbose)
	    msg_info("%s asciified to %s", domain, adomain);
	domain = adomain;
    }
#endif
    if (domain[0] == 0 || valid_hostname(domain, DONT_GRIPE) == 0)
	return (SMTPD_CHECK_DUNNO);

    query = vstring_alloc(100);
    vstring_sprintf(query, "%s.%s", domain, rbl_domain);
    reply_addr = split_at(STR(query), '=');
    rbl = (SMTPD_RBL_STATE *) ctable_locate(smtpd_rbl_cache, STR(query));
    if (reply_addr != 0)
	byte_codes = ctable_locate(smtpd_rbl_byte_cache, reply_addr);

    /*
     * If the record exists, match the result address.
     */
    if (SMTPD_DNSXL_STAT_OK(rbl) && reply_addr != 0
	&& !rbl_match_addr(rbl, byte_codes))
	rbl = 0;
    vstring_free(query);
    return (rbl);
}

/* reject_rbl_domain - reject if domain in DNS deny list */

static int reject_rbl_domain(SMTPD_STATE *state, const char *rbl_domain,
			          const char *what, const char *reply_class)
{
    const char *myname = "reject_rbl_domain";
    const SMTPD_RBL_STATE *rbl;

    if (msg_verbose)
	msg_info("%s: %s %s", myname, rbl_domain, what);

    rbl = find_dnsxl_domain(state, rbl_domain, what);
    if (!SMTPD_DNSXL_STAT_OK(rbl)) {
	return (SMTPD_CHECK_DUNNO);
    } else {
	return (rbl_reject_reply(state, rbl, rbl_domain, what, reply_class));
    }
}

/* permit_dnswl_domain - permit domain in DNSWL */

static int permit_dnswl_domain(SMTPD_STATE *state, const char *dnswl_domain,
			          const char *what, const char *reply_class)
{
    const char *myname = "permit_dnswl_domain";
    const SMTPD_RBL_STATE *dnswl_result;

    if (msg_verbose)
	msg_info("%s: %s", myname, what);

    /* Safety: don't allowlist unauthorized recipients. */
    if (strcmp(state->where, SMTPD_CMD_RCPT) == 0 && state->recipient != 0
      && permit_auth_destination(state, state->recipient) != SMTPD_CHECK_OK)
	return (SMTPD_CHECK_DUNNO);

    dnswl_result = find_dnsxl_domain(state, dnswl_domain, what);
    if (SMTPD_DNXSL_STAT_HARD(dnswl_result)) {
	return (SMTPD_CHECK_DUNNO);
    } else if (SMTPD_DNSXL_STAT_SOFT(dnswl_result)) {
	/* XXX: Make configurable as rhswl_tempfail_action. */
	DEFER_IF_REJECT3(state, MAIL_ERROR_POLICY,
			 450, "4.7.1",
			 "<%s>: %s rejected: %s",
			 what, reply_class,
			 "Service unavailable");
	return (SMTPD_CHECK_DUNNO);
    } else if (SMTPD_DNSXL_STAT_OK(dnswl_result)) {
	return (SMTPD_CHECK_OK);
    } else {
	/* Future proofing, in case find_dnsxl_addr() result is changed. */
	msg_panic("%s: find_dnsxl_addr API failure", myname);
    }
}

/* reject_maps_rbl - reject if client address in DNS deny list */

static int reject_maps_rbl(SMTPD_STATE *state)
{
    const char *myname = "reject_maps_rbl";
    char   *saved_domains = mystrdup(var_maps_rbl_domains);
    char   *bp = saved_domains;
    char   *rbl_domain;
    int     result = SMTPD_CHECK_DUNNO;
    static int warned;

    if (msg_verbose)
	msg_info("%s: %s", myname, state->addr);

    if (warned == 0) {
	warned++;
	msg_warn("support for restriction \"%s\" will be removed from %s; "
		 "use \"%s domain-name\" instead",
		 REJECT_MAPS_RBL, var_mail_name, REJECT_RBL_CLIENT);
    }
    while ((rbl_domain = mystrtok(&bp, CHARS_COMMA_SP)) != 0) {
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

#ifdef USE_SASL_AUTH

/* reject_auth_sender_login_mismatch - logged in client must own sender address */

static int reject_auth_sender_login_mismatch(SMTPD_STATE *state, const char *sender, int allow_unknown_sender)
{
    const RESOLVE_REPLY *reply;
    const char *owners;
    char   *saved_owners;
    char   *cp;
    char   *name;
    int     found = 0;

#define ALLOW_UNKNOWN_SENDER	1
#define FORBID_UNKNOWN_SENDER	0

    /*
     * Reject if the client is logged in and does not own the sender address.
     */
    if (smtpd_sender_login_maps && state->sasl_username) {
	reply = smtpd_resolve_addr(state->recipient, sender);
	if (reply->flags & RESOLVE_FLAG_FAIL)
	    reject_dict_retry(state, sender);
	if ((owners = check_mail_addr_find(state, sender, smtpd_sender_login_maps,
				STR(reply->recipient), (char **) 0)) != 0) {
	    cp = saved_owners = mystrdup(owners);
	    while ((name = mystrtok(&cp, CHARS_COMMA_SP)) != 0) {
		if (strcasecmp_utf8(state->sasl_username, name) == 0) {
		    found = 1;
		    break;
		}
	    }
	    myfree(saved_owners);
	} else if (allow_unknown_sender)
	    return (SMTPD_CHECK_DUNNO);
	if (!found)
	    return (smtpd_check_reject(state, MAIL_ERROR_POLICY, 553, "5.7.1",
		      "<%s>: Sender address rejected: not owned by user %s",
				       sender, state->sasl_username));
    }
    return (SMTPD_CHECK_DUNNO);
}

/* reject_unauth_sender_login_mismatch - sender requires client is logged in */

static int reject_unauth_sender_login_mismatch(SMTPD_STATE *state, const char *sender)
{
    const RESOLVE_REPLY *reply;

    /*
     * Reject if the client is not logged in and the sender address has an
     * owner.
     */
    if (smtpd_sender_login_maps && !state->sasl_username) {
	reply = smtpd_resolve_addr(state->recipient, sender);
	if (reply->flags & RESOLVE_FLAG_FAIL)
	    reject_dict_retry(state, sender);
	if (check_mail_addr_find(state, sender, smtpd_sender_login_maps,
				 STR(reply->recipient), (char **) 0) != 0)
	    return (smtpd_check_reject(state, MAIL_ERROR_POLICY, 553, "5.7.1",
		   "<%s>: Sender address rejected: not logged in", sender));
    }
    return (SMTPD_CHECK_DUNNO);
}

#endif

/* valid_utf8_action - validate UTF-8 policy server response */

static int valid_utf8_action(const char *server, const char *action)
{
    int     retval;

    if ((retval = valid_utf8_string(action, strlen(action))) == 0)
	msg_warn("malformed UTF-8 in policy server %s response: \"%s\"",
		 server, action);
    return (retval);
}

/* check_policy_service - check delegated policy service */

static int check_policy_service(SMTPD_STATE *state, const char *server,
		            const char *reply_name, const char *reply_class,
				        const char *def_acl)
{
    static int warned = 0;
    static VSTRING *action = 0;
    SMTPD_POLICY_CLNT *policy_clnt;

#ifdef USE_TLS
    VSTRING *subject_buf;
    VSTRING *issuer_buf;
    const char *subject;
    const char *issuer;

#endif
    int     ret;

    /*
     * Sanity check.
     */
    if (!policy_clnt_table
	|| (policy_clnt = (SMTPD_POLICY_CLNT *)
	    htable_find(policy_clnt_table, server)) == 0)
	msg_panic("check_policy_service: no client endpoint for server %s",
		  server);

    /*
     * Initialize.
     */
    if (action == 0)
	action = vstring_alloc(10);

#ifdef USE_TLS
#define ENCODE_CN(coded_CN, coded_CN_buf, CN) do { \
	if (!TLS_CERT_IS_TRUSTED(state->tls_context) || *(CN) == 0) { \
	    coded_CN_buf = 0; \
	    coded_CN = ""; \
	} else { \
	    coded_CN_buf = vstring_alloc(strlen(CN) + 1); \
	    xtext_quote(coded_CN_buf, CN, ""); \
	    coded_CN = STR(coded_CN_buf); \
	} \
    } while (0);

    ENCODE_CN(subject, subject_buf, state->tls_context->peer_CN);
    ENCODE_CN(issuer, issuer_buf, state->tls_context->issuer_CN);

    /*
     * XXX: Too noisy to warn for each policy lookup, especially because we
     * don't even know whether the policy server will use the fingerprint. So
     * warn at most once per process, though on only lightly loaded servers,
     * it might come close to one warning per inbound message.
     */
    if (!warned
	&& warn_compat_break_smtpd_tls_fpt_dgst
	&& state->tls_context
	&& state->tls_context->peer_cert_fprint
	&& *state->tls_context->peer_cert_fprint) {
	warned = 1;
	msg_info("using backwards-compatible default setting "
		 VAR_SMTPD_TLS_FPT_DGST "=md5 to compute certificate "
		 "fingerprints");
    }
#endif

    if (attr_clnt_request(policy_clnt->client,
			  ATTR_FLAG_NONE,	/* Query attributes. */
			SEND_ATTR_STR(MAIL_ATTR_REQ, "smtpd_access_policy"),
			  SEND_ATTR_STR(MAIL_ATTR_PROTO_STATE,
					STREQ(state->where, SMTPD_CMD_BDAT) ?
					SMTPD_CMD_DATA : state->where),
		   SEND_ATTR_STR(MAIL_ATTR_ACT_PROTO_NAME, state->protocol),
		      SEND_ATTR_STR(MAIL_ATTR_ACT_CLIENT_ADDR, state->addr),
		      SEND_ATTR_STR(MAIL_ATTR_ACT_CLIENT_NAME, state->name),
		      SEND_ATTR_STR(MAIL_ATTR_ACT_CLIENT_PORT, state->port),
			  SEND_ATTR_STR(MAIL_ATTR_ACT_REVERSE_CLIENT_NAME,
					state->reverse_name),
			  SEND_ATTR_STR(MAIL_ATTR_ACT_SERVER_ADDR,
					state->dest_addr),
			  SEND_ATTR_STR(MAIL_ATTR_ACT_SERVER_PORT,
					state->dest_port),
			  SEND_ATTR_STR(MAIL_ATTR_ACT_HELO_NAME,
				  state->helo_name ? state->helo_name : ""),
			  SEND_ATTR_STR(MAIL_ATTR_SENDER,
					state->sender ? state->sender : ""),
			  SEND_ATTR_STR(MAIL_ATTR_RECIP,
				  state->recipient ? state->recipient : ""),
			  SEND_ATTR_INT(MAIL_ATTR_RCPT_COUNT,
			 ((strcasecmp(state->where, SMTPD_CMD_DATA) == 0) ||
			  (strcasecmp(state->where, SMTPD_CMD_BDAT) == 0) ||
			  (strcasecmp(state->where, SMTPD_AFTER_EOM) == 0)) ?
					state->rcpt_count : 0),
			  SEND_ATTR_STR(MAIL_ATTR_QUEUEID,
				    state->queue_id ? state->queue_id : ""),
			  SEND_ATTR_STR(MAIL_ATTR_INSTANCE,
					STR(state->instance)),
			  SEND_ATTR_LONG(MAIL_ATTR_SIZE,
				      (unsigned long) (state->act_size > 0 ?
					state->act_size : state->msg_size)),
			  SEND_ATTR_STR(MAIL_ATTR_ETRN_DOMAIN,
				  state->etrn_name ? state->etrn_name : ""),
			  SEND_ATTR_STR(MAIL_ATTR_STRESS, var_stress),
#ifdef USE_SASL_AUTH
			  SEND_ATTR_STR(MAIL_ATTR_SASL_METHOD,
			      state->sasl_method ? state->sasl_method : ""),
			  SEND_ATTR_STR(MAIL_ATTR_SASL_USERNAME,
			  state->sasl_username ? state->sasl_username : ""),
			  SEND_ATTR_STR(MAIL_ATTR_SASL_SENDER,
			      state->sasl_sender ? state->sasl_sender : ""),
#endif
#ifdef USE_TLS
#define IF_ENCRYPTED(x, y) ((state->tls_context && ((x) != 0)) ? (x) : (y))
			  SEND_ATTR_STR(MAIL_ATTR_CCERT_SUBJECT, subject),
			  SEND_ATTR_STR(MAIL_ATTR_CCERT_ISSUER, issuer),

    /*
     * When directly checking the fingerprint, it is OK if the issuing CA is
     * not trusted.
     */
			  SEND_ATTR_STR(MAIL_ATTR_CCERT_CERT_FPRINT,
		    IF_ENCRYPTED(state->tls_context->peer_cert_fprint, "")),
			  SEND_ATTR_STR(MAIL_ATTR_CCERT_PKEY_FPRINT,
		    IF_ENCRYPTED(state->tls_context->peer_pkey_fprint, "")),
			  SEND_ATTR_STR(MAIL_ATTR_CRYPTO_PROTOCOL,
			    IF_ENCRYPTED(state->tls_context->protocol, "")),
			  SEND_ATTR_STR(MAIL_ATTR_CRYPTO_CIPHER,
			 IF_ENCRYPTED(state->tls_context->cipher_name, "")),
			  SEND_ATTR_INT(MAIL_ATTR_CRYPTO_KEYSIZE,
		       IF_ENCRYPTED(state->tls_context->cipher_usebits, 0)),
#endif
			  SEND_ATTR_STR(MAIL_ATTR_POL_CONTEXT,
					policy_clnt->policy_context),
			  SEND_ATTR_STR(MAIL_ATTR_COMPAT_LEVEL,
					var_compatibility_level),
			  SEND_ATTR_STR(MAIL_ATTR_MAIL_VERSION,
					var_mail_version),
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes. */
			  RECV_ATTR_STR(MAIL_ATTR_ACTION, action),
			  ATTR_TYPE_END) != 1
	|| (var_smtputf8_enable && valid_utf8_action(server, STR(action)) == 0)) {
	NOCLOBBER static int nesting_level = 0;
	jmp_buf savebuf;
	int     status;

	/*
	 * Safety to prevent recursive execution of the default action.
	 */
	nesting_level += 1;
	memcpy(ADDROF(savebuf), ADDROF(smtpd_check_buf), sizeof(savebuf));
	status = setjmp(smtpd_check_buf);
	if (status != 0) {
	    nesting_level -= 1;
	    memcpy(ADDROF(smtpd_check_buf), ADDROF(savebuf),
		   sizeof(smtpd_check_buf));
	    longjmp(smtpd_check_buf, status);
	}
	ret = check_table_result(state, server, nesting_level == 1 ?
				 policy_clnt->def_action :
				 DEF_SMTPD_POLICY_DEF_ACTION,
				 "policy query", reply_name,
				 reply_class, def_acl);
	nesting_level -= 1;
	memcpy(ADDROF(smtpd_check_buf), ADDROF(savebuf),
	       sizeof(smtpd_check_buf));
    } else {

	/*
	 * XXX This produces bogus error messages when the reply is
	 * malformed.
	 */
	ret = check_table_result(state, server, STR(action),
				 "policy query", reply_name,
				 reply_class, def_acl);
    }
#ifdef USE_TLS
    if (subject_buf)
	vstring_free(subject_buf);
    if (issuer_buf)
	vstring_free(issuer_buf);
#endif
    return (ret);
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
	msg_warn("restriction %s: bad argument \"%s\": need maptype:mapname",
		 command, **argp);
	reject_server_error(state);
    } else {
	return (1);
    }
}

/* forbid_allowlist - disallow allowlisting */

static void forbid_allowlist(SMTPD_STATE *state, const char *name,
			             int status, const char *target)
{
    if (state->discard == 0 && status == SMTPD_CHECK_OK) {
	msg_warn("restriction %s returns OK for %s", name, target);
	msg_warn("this is not allowed for security reasons");
	msg_warn("use DUNNO instead of OK if you want to make an exception");
	reject_server_error(state);
    }
}

/* generic_checks - generic restrictions */

static int generic_checks(SMTPD_STATE *state, ARGV *restrictions,
			          const char *reply_name,
			          const char *reply_class,
			          const char *def_acl)
{
    const char *myname = "generic_checks";
    char  **cpp;
    const char *name;
    int     status = 0;
    ARGV   *list;
    int     found;
    int     saved_recursion = state->recursion++;

    if (msg_verbose)
	msg_info(">>> START %s RESTRICTIONS <<<", reply_class);

    for (cpp = restrictions->argv; (name = *cpp) != 0; cpp++) {

	if (state->discard != 0)
	    break;

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
		msg_warn("specify one of (%s, %s, %s, %s, %s, %s) before %s restriction \"%s\"",
			 CHECK_CLIENT_ACL, CHECK_REVERSE_CLIENT_ACL, CHECK_HELO_ACL, CHECK_SENDER_ACL,
			 CHECK_RECIP_ACL, CHECK_ETRN_ACL, reply_class, name);
		reject_server_error(state);
	    }
	    name = def_acl;
	    cpp -= 1;
	}

	/*
	 * Generic restrictions.
	 */
	if (strcasecmp(name, PERMIT_ALL) == 0) {
	    status = smtpd_acl_permit(state, name, reply_class,
				      reply_name, NO_PRINT_ARGS);
	    if (status == SMTPD_CHECK_OK && cpp[1] != 0)
		msg_warn("restriction `%s' after `%s' is ignored",
			 cpp[1], PERMIT_ALL);
	} else if (strcasecmp(name, DEFER_ALL) == 0) {
	    status = smtpd_check_reject(state, MAIL_ERROR_POLICY,
					var_defer_code, "4.3.2",
					"<%s>: %s rejected: Try again later",
					reply_name, reply_class);
	    if (cpp[1] != 0 && state->warn_if_reject == 0)
		msg_warn("restriction `%s' after `%s' is ignored",
			 cpp[1], DEFER_ALL);
	} else if (strcasecmp(name, REJECT_ALL) == 0) {
	    status = smtpd_check_reject(state, MAIL_ERROR_POLICY,
					var_reject_code, "5.7.1",
					"<%s>: %s rejected: Access denied",
					reply_name, reply_class);
	    if (cpp[1] != 0 && state->warn_if_reject == 0)
		msg_warn("restriction `%s' after `%s' is ignored",
			 cpp[1], REJECT_ALL);
	} else if (strcasecmp(name, REJECT_UNAUTH_PIPE) == 0) {
	    status = reject_unauth_pipelining(state, reply_name, reply_class);
	} else if (strcasecmp(name, CHECK_POLICY_SERVICE) == 0) {
	    if (cpp[1] == 0 || strchr(cpp[1], ':') == 0) {
		msg_warn("restriction %s must be followed by transport:server",
			 CHECK_POLICY_SERVICE);
		reject_server_error(state);
	    } else
		status = check_policy_service(state, *++cpp, reply_name,
					      reply_class, def_acl);
	} else if (strcasecmp(name, DEFER_IF_PERMIT) == 0) {
	    status = DEFER_IF_PERMIT2(DEFER_IF_PERMIT_ACT,
				      state, MAIL_ERROR_POLICY,
				      450, "4.7.0",
			     "<%s>: %s rejected: defer_if_permit requested",
				      reply_name, reply_class);
	} else if (strcasecmp(name, DEFER_IF_REJECT) == 0) {
	    DEFER_IF_REJECT2(state, MAIL_ERROR_POLICY,
			     450, "4.7.0",
			     "<%s>: %s rejected: defer_if_reject requested",
			     reply_name, reply_class);
	} else if (strcasecmp(name, SLEEP) == 0) {
	    if (cpp[1] == 0 || alldig(cpp[1]) == 0) {
		msg_warn("restriction %s must be followed by number", SLEEP);
		reject_server_error(state);
	    } else
		sleep(atoi(*++cpp));
	} else if (strcasecmp(name, REJECT_PLAINTEXT_SESSION) == 0) {
	    status = reject_plaintext_session(state);
	}

	/*
	 * Client name/address restrictions.
	 */
	else if (strcasecmp(name, REJECT_UNKNOWN_CLIENT_HOSTNAME) == 0
		 || strcasecmp(name, REJECT_UNKNOWN_CLIENT) == 0) {
	    status = reject_unknown_client(state);
	} else if (strcasecmp(name, REJECT_UNKNOWN_REVERSE_HOSTNAME) == 0) {
	    status = reject_unknown_reverse_name(state);
	} else if (strcasecmp(name, PERMIT_INET_INTERFACES) == 0) {
	    status = permit_inet_interfaces(state);
	    if (status == SMTPD_CHECK_OK)
		status = smtpd_acl_permit(state, name, SMTPD_NAME_CLIENT,
					  state->namaddr, NO_PRINT_ARGS);
	} else if (strcasecmp(name, PERMIT_MYNETWORKS) == 0) {
	    status = permit_mynetworks(state);
	    if (status == SMTPD_CHECK_OK)
		status = smtpd_acl_permit(state, name, SMTPD_NAME_CLIENT,
					  state->namaddr, NO_PRINT_ARGS);
	} else if (is_map_command(state, name, CHECK_CLIENT_ACL, &cpp)) {
	    status = check_namadr_access(state, *cpp, state->name, state->addr,
					 FULL, &found, state->namaddr,
					 SMTPD_NAME_CLIENT, def_acl);
	} else if (is_map_command(state, name, CHECK_REVERSE_CLIENT_ACL, &cpp)) {
	    status = check_namadr_access(state, *cpp, state->reverse_name, state->addr,
					 FULL, &found, state->reverse_name,
					 SMTPD_NAME_REV_CLIENT, def_acl);
	    forbid_allowlist(state, name, status, state->reverse_name);
	} else if (strcasecmp(name, REJECT_MAPS_RBL) == 0) {
	    status = reject_maps_rbl(state);
	} else if (strcasecmp(name, REJECT_RBL_CLIENT) == 0
		   || strcasecmp(name, REJECT_RBL) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument", name);
	    else
		status = reject_rbl_addr(state, *(cpp += 1), state->addr,
					 SMTPD_NAME_CLIENT);
	} else if (strcasecmp(name, PERMIT_DNSWL_CLIENT) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument", name);
	    else {
		status = permit_dnswl_addr(state, *(cpp += 1), state->addr,
					   SMTPD_NAME_CLIENT);
		if (status == SMTPD_CHECK_OK)
		    status = smtpd_acl_permit(state, name, SMTPD_NAME_CLIENT,
					      state->namaddr, NO_PRINT_ARGS);
	    }
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
	} else if (strcasecmp(name, PERMIT_RHSWL_CLIENT) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument",
			 name);
	    else {
		cpp += 1;
		if (strcasecmp(state->name, "unknown") != 0) {
		    status = permit_dnswl_domain(state, *cpp, state->name,
						 SMTPD_NAME_CLIENT);
		    if (status == SMTPD_CHECK_OK)
			status = smtpd_acl_permit(state, name,
			  SMTPD_NAME_CLIENT, state->namaddr, NO_PRINT_ARGS);
		}
	    }
	} else if (strcasecmp(name, REJECT_RHSBL_REVERSE_CLIENT) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument",
			 name);
	    else {
		cpp += 1;
		if (strcasecmp(state->reverse_name, "unknown") != 0)
		    status = reject_rbl_domain(state, *cpp, state->reverse_name,
					       SMTPD_NAME_REV_CLIENT);
	    }
	} else if (is_map_command(state, name, CHECK_CCERT_ACL, &cpp)) {
	    status = check_ccert_access(state, *cpp, def_acl);
	} else if (is_map_command(state, name, CHECK_SASL_ACL, &cpp)) {
#ifdef USE_SASL_AUTH
	    if (var_smtpd_sasl_enable) {
		if (state->sasl_username && state->sasl_username[0])
		    status = check_sasl_access(state, *cpp, def_acl);
	    } else
#endif
		msg_warn("restriction `%s' ignored: no SASL support", name);
	} else if (is_map_command(state, name, CHECK_CLIENT_NS_ACL, &cpp)) {
	    if (strcasecmp(state->name, "unknown") != 0) {
		status = check_server_access(state, *cpp, state->name,
					     T_NS, state->namaddr,
					     SMTPD_NAME_CLIENT, def_acl);
		forbid_allowlist(state, name, status, state->name);
	    }
	} else if (is_map_command(state, name, CHECK_CLIENT_MX_ACL, &cpp)) {
	    if (strcasecmp(state->name, "unknown") != 0) {
		status = check_server_access(state, *cpp, state->name,
					     T_MX, state->namaddr,
					     SMTPD_NAME_CLIENT, def_acl);
		forbid_allowlist(state, name, status, state->name);
	    }
	} else if (is_map_command(state, name, CHECK_CLIENT_A_ACL, &cpp)) {
	    if (strcasecmp(state->name, "unknown") != 0) {
		status = check_server_access(state, *cpp, state->name,
					     T_A, state->namaddr,
					     SMTPD_NAME_CLIENT, def_acl);
		forbid_allowlist(state, name, status, state->name);
	    }
	} else if (is_map_command(state, name, CHECK_REVERSE_CLIENT_NS_ACL, &cpp)) {
	    if (strcasecmp(state->reverse_name, "unknown") != 0) {
		status = check_server_access(state, *cpp, state->reverse_name,
					     T_NS, state->reverse_name,
					     SMTPD_NAME_REV_CLIENT, def_acl);
		forbid_allowlist(state, name, status, state->reverse_name);
	    }
	} else if (is_map_command(state, name, CHECK_REVERSE_CLIENT_MX_ACL, &cpp)) {
	    if (strcasecmp(state->reverse_name, "unknown") != 0) {
		status = check_server_access(state, *cpp, state->reverse_name,
					     T_MX, state->reverse_name,
					     SMTPD_NAME_REV_CLIENT, def_acl);
		forbid_allowlist(state, name, status, state->reverse_name);
	    }
	} else if (is_map_command(state, name, CHECK_REVERSE_CLIENT_A_ACL, &cpp)) {
	    if (strcasecmp(state->reverse_name, "unknown") != 0) {
		status = check_server_access(state, *cpp, state->reverse_name,
					     T_A, state->reverse_name,
					     SMTPD_NAME_REV_CLIENT, def_acl);
		forbid_allowlist(state, name, status, state->reverse_name);
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
	} else if (strcasecmp(name, REJECT_INVALID_HELO_HOSTNAME) == 0
		   || strcasecmp(name, REJECT_INVALID_HOSTNAME) == 0) {
	    if (state->helo_name) {
		if (*state->helo_name != '[')
		    status = reject_invalid_hostname(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
		else
		    status = reject_invalid_hostaddr(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
	    }
	} else if (strcasecmp(name, REJECT_UNKNOWN_HELO_HOSTNAME) == 0
		   || strcasecmp(name, REJECT_UNKNOWN_HOSTNAME) == 0) {
	    if (state->helo_name) {
		if (*state->helo_name != '[')
		    status = reject_unknown_hostname(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
		else
		    status = reject_invalid_hostaddr(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
	    }
	} else if (strcasecmp(name, PERMIT_NAKED_IP_ADDR) == 0) {
	    msg_warn("restriction %s is deprecated. Use %s or %s instead",
		 PERMIT_NAKED_IP_ADDR, PERMIT_MYNETWORKS, PERMIT_SASL_AUTH);
	    if (state->helo_name) {
		if (state->helo_name[strspn(state->helo_name, "0123456789.:")] == 0
		&& (status = reject_invalid_hostaddr(state, state->helo_name,
				   state->helo_name, SMTPD_NAME_HELO)) == 0)
		    status = smtpd_acl_permit(state, name, SMTPD_NAME_HELO,
					   state->helo_name, NO_PRINT_ARGS);
	    }
	} else if (is_map_command(state, name, CHECK_HELO_NS_ACL, &cpp)) {
	    if (state->helo_name) {
		status = check_server_access(state, *cpp, state->helo_name,
					     T_NS, state->helo_name,
					     SMTPD_NAME_HELO, def_acl);
		forbid_allowlist(state, name, status, state->helo_name);
	    }
	} else if (is_map_command(state, name, CHECK_HELO_MX_ACL, &cpp)) {
	    if (state->helo_name) {
		status = check_server_access(state, *cpp, state->helo_name,
					     T_MX, state->helo_name,
					     SMTPD_NAME_HELO, def_acl);
		forbid_allowlist(state, name, status, state->helo_name);
	    }
	} else if (is_map_command(state, name, CHECK_HELO_A_ACL, &cpp)) {
	    if (state->helo_name) {
		status = check_server_access(state, *cpp, state->helo_name,
					     T_A, state->helo_name,
					     SMTPD_NAME_HELO, def_acl);
		forbid_allowlist(state, name, status, state->helo_name);
	    }
	} else if (strcasecmp(name, REJECT_NON_FQDN_HELO_HOSTNAME) == 0
		   || strcasecmp(name, REJECT_NON_FQDN_HOSTNAME) == 0) {
	    if (state->helo_name) {
		if (*state->helo_name != '[')
		    status = reject_non_fqdn_hostname(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
		else
		    status = reject_invalid_hostaddr(state, state->helo_name,
					 state->helo_name, SMTPD_NAME_HELO);
	    }
	} else if (strcasecmp(name, REJECT_RHSBL_HELO) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument",
			 name);
	    else {
		cpp += 1;
		if (state->helo_name)
		    status = reject_rbl_domain(state, *cpp, state->helo_name,
					       SMTPD_NAME_HELO);
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
	} else if (strcasecmp(name, REJECT_UNVERIFIED_SENDER) == 0) {
	    if (state->sender && *state->sender)
		status = reject_unverified_address(state, state->sender,
					   state->sender, SMTPD_NAME_SENDER,
				     var_unv_from_dcode, var_unv_from_rcode,
						   unv_from_tf_act,
						   var_unv_from_why);
	} else if (strcasecmp(name, REJECT_NON_FQDN_SENDER) == 0) {
	    if (state->sender && *state->sender)
		status = reject_non_fqdn_address(state, state->sender,
					  state->sender, SMTPD_NAME_SENDER);
	} else if (strcasecmp(name, REJECT_AUTH_SENDER_LOGIN_MISMATCH) == 0) {
#ifdef USE_SASL_AUTH
	    if (var_smtpd_sasl_enable) {
		if (state->sender && *state->sender)
		    status = reject_auth_sender_login_mismatch(state,
				      state->sender, FORBID_UNKNOWN_SENDER);
	    } else
#endif
		msg_warn("restriction `%s' ignored: no SASL support", name);
	} else if (strcasecmp(name, REJECT_KNOWN_SENDER_LOGIN_MISMATCH) == 0) {
#ifdef USE_SASL_AUTH
	    if (var_smtpd_sasl_enable) {
		if (state->sender && *state->sender) {
		    if (state->sasl_username)
			status = reject_auth_sender_login_mismatch(state,
				       state->sender, ALLOW_UNKNOWN_SENDER);
		    else
			status = reject_unauth_sender_login_mismatch(state, state->sender);
		}
	    } else
#endif
		msg_warn("restriction `%s' ignored: no SASL support", name);
	} else if (strcasecmp(name, REJECT_UNAUTH_SENDER_LOGIN_MISMATCH) == 0) {
#ifdef USE_SASL_AUTH
	    if (var_smtpd_sasl_enable) {
		if (state->sender && *state->sender)
		    status = reject_unauth_sender_login_mismatch(state, state->sender);
	    } else
#endif
		msg_warn("restriction `%s' ignored: no SASL support", name);
	} else if (is_map_command(state, name, CHECK_SENDER_NS_ACL, &cpp)) {
	    if (state->sender && *state->sender) {
		status = check_server_access(state, *cpp, state->sender,
					     T_NS, state->sender,
					     SMTPD_NAME_SENDER, def_acl);
		forbid_allowlist(state, name, status, state->sender);
	    }
	} else if (is_map_command(state, name, CHECK_SENDER_MX_ACL, &cpp)) {
	    if (state->sender && *state->sender) {
		status = check_server_access(state, *cpp, state->sender,
					     T_MX, state->sender,
					     SMTPD_NAME_SENDER, def_acl);
		forbid_allowlist(state, name, status, state->sender);
	    }
	} else if (is_map_command(state, name, CHECK_SENDER_A_ACL, &cpp)) {
	    if (state->sender && *state->sender) {
		status = check_server_access(state, *cpp, state->sender,
					     T_A, state->sender,
					     SMTPD_NAME_SENDER, def_acl);
		forbid_allowlist(state, name, status, state->sender);
	    }
	} else if (strcasecmp(name, REJECT_RHSBL_SENDER) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument", name);
	    else {
		cpp += 1;
		if (state->sender && *state->sender)
		    status = reject_rbl_domain(state, *cpp, state->sender,
					       SMTPD_NAME_SENDER);
	    }
	} else if (strcasecmp(name, REJECT_UNLISTED_SENDER) == 0) {
	    if (state->sender && *state->sender)
		status = check_sender_rcpt_maps(state, state->sender);
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
	    if (state->recipient) {
		status = permit_mx_backup(state, state->recipient,
				    state->recipient, SMTPD_NAME_RECIPIENT);
		if (status == SMTPD_CHECK_OK)
		    status = smtpd_acl_permit(state, name, SMTPD_NAME_RECIPIENT,
					   state->recipient, NO_PRINT_ARGS);
	    }
	} else if (strcasecmp(name, PERMIT_AUTH_DEST) == 0) {
	    if (state->recipient) {
		status = permit_auth_destination(state, state->recipient);
		if (status == SMTPD_CHECK_OK)
		    status = smtpd_acl_permit(state, name, SMTPD_NAME_RECIPIENT,
					   state->recipient, NO_PRINT_ARGS);
	    }
	} else if (strcasecmp(name, REJECT_UNAUTH_DEST) == 0) {
	    if (state->recipient)
		status = reject_unauth_destination(state, state->recipient,
						   var_relay_code, "5.7.1");
	} else if (strcasecmp(name, DEFER_UNAUTH_DEST) == 0) {
	    if (state->recipient)
		status = reject_unauth_destination(state, state->recipient,
					     var_relay_code - 100, "4.7.1");
	} else if (strcasecmp(name, CHECK_RELAY_DOMAINS) == 0) {
	    if (state->recipient)
		status = check_relay_domains(state, state->recipient,
				    state->recipient, SMTPD_NAME_RECIPIENT);
	    if (status == SMTPD_CHECK_OK)
		status = smtpd_acl_permit(state, name, SMTPD_NAME_RECIPIENT,
					  state->recipient, NO_PRINT_ARGS);
	    if (cpp[1] != 0 && state->warn_if_reject == 0)
		msg_warn("restriction `%s' after `%s' is ignored",
			 cpp[1], CHECK_RELAY_DOMAINS);
	} else if (strcasecmp(name, PERMIT_SASL_AUTH) == 0) {
#ifdef USE_SASL_AUTH
	    if (smtpd_sasl_is_active(state)) {
		status = permit_sasl_auth(state,
					  SMTPD_CHECK_OK, SMTPD_CHECK_DUNNO);
		if (status == SMTPD_CHECK_OK)
		    status = smtpd_acl_permit(state, name, SMTPD_NAME_CLIENT,
					      state->namaddr, NO_PRINT_ARGS);
	    }
#endif
	} else if (strcasecmp(name, PERMIT_TLS_ALL_CLIENTCERTS) == 0) {
	    status = permit_tls_clientcerts(state, 1);
	    if (status == SMTPD_CHECK_OK)
		status = smtpd_acl_permit(state, name, SMTPD_NAME_CLIENT,
					  state->namaddr, NO_PRINT_ARGS);
	} else if (strcasecmp(name, PERMIT_TLS_CLIENTCERTS) == 0) {
	    status = permit_tls_clientcerts(state, 0);
	    if (status == SMTPD_CHECK_OK)
		status = smtpd_acl_permit(state, name, SMTPD_NAME_CLIENT,
					  state->namaddr, NO_PRINT_ARGS);
	} else if (strcasecmp(name, REJECT_UNKNOWN_RCPTDOM) == 0) {
	    if (state->recipient)
		status = reject_unknown_address(state, state->recipient,
				    state->recipient, SMTPD_NAME_RECIPIENT);
	} else if (strcasecmp(name, REJECT_NON_FQDN_RCPT) == 0) {
	    if (state->recipient)
		status = reject_non_fqdn_address(state, state->recipient,
				    state->recipient, SMTPD_NAME_RECIPIENT);
	} else if (is_map_command(state, name, CHECK_RECIP_NS_ACL, &cpp)) {
	    if (state->recipient && *state->recipient) {
		status = check_server_access(state, *cpp, state->recipient,
					     T_NS, state->recipient,
					     SMTPD_NAME_RECIPIENT, def_acl);
		forbid_allowlist(state, name, status, state->recipient);
	    }
	} else if (is_map_command(state, name, CHECK_RECIP_MX_ACL, &cpp)) {
	    if (state->recipient && *state->recipient) {
		status = check_server_access(state, *cpp, state->recipient,
					     T_MX, state->recipient,
					     SMTPD_NAME_RECIPIENT, def_acl);
		forbid_allowlist(state, name, status, state->recipient);
	    }
	} else if (is_map_command(state, name, CHECK_RECIP_A_ACL, &cpp)) {
	    if (state->recipient && *state->recipient) {
		status = check_server_access(state, *cpp, state->recipient,
					     T_A, state->recipient,
					     SMTPD_NAME_RECIPIENT, def_acl);
		forbid_allowlist(state, name, status, state->recipient);
	    }
	} else if (strcasecmp(name, REJECT_RHSBL_RECIPIENT) == 0) {
	    if (cpp[1] == 0)
		msg_warn("restriction %s requires domain name argument", name);
	    else {
		cpp += 1;
		if (state->recipient)
		    status = reject_rbl_domain(state, *cpp, state->recipient,
					       SMTPD_NAME_RECIPIENT);
	    }
	} else if (strcasecmp(name, CHECK_RCPT_MAPS) == 0
		   || strcasecmp(name, REJECT_UNLISTED_RCPT) == 0) {
	    if (state->recipient && *state->recipient)
		status = check_recipient_rcpt_maps(state, state->recipient);
	} else if (strcasecmp(name, REJECT_MUL_RCPT_BOUNCE) == 0) {
	    if (state->sender && *state->sender == 0 && state->rcpt_count
		> (strcmp(state->where, SMTPD_CMD_RCPT) != 0))
		status = smtpd_check_reject(state, MAIL_ERROR_POLICY,
					    var_mul_rcpt_code, "5.5.3",
				"<%s>: %s rejected: Multi-recipient bounce",
					    reply_name, reply_class);
	} else if (strcasecmp(name, REJECT_UNVERIFIED_RECIP) == 0) {
	    if (state->recipient && *state->recipient)
		status = reject_unverified_address(state, state->recipient,
				     state->recipient, SMTPD_NAME_RECIPIENT,
				     var_unv_rcpt_dcode, var_unv_rcpt_rcode,
						   unv_rcpt_tf_act,
						   var_unv_rcpt_why);
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
	    reject_server_error(state);
	}
	if (msg_verbose)
	    msg_info("%s: name=%s status=%d", myname, name, status);

	if (status < 0) {
	    if (status == DICT_ERR_RETRY)
		reject_dict_retry(state, reply_name);
	    else
		reject_server_error(state);
	}
	if (state->warn_if_reject >= state->recursion)
	    state->warn_if_reject = 0;

	if (status != 0)
	    break;

	if (state->defer_if_permit.active && state->defer_if_reject.active)
	    break;
    }
    if (msg_verbose)
	msg_info(">>> END %s RESTRICTIONS <<<", reply_class);

    state->recursion = saved_recursion;

    /* In case the list terminated with one or more warn_if_mumble. */
    if (state->warn_if_reject >= state->recursion)
	state->warn_if_reject = 0;

    return (status);
}

/* smtpd_check_addr - address sanity check */

int     smtpd_check_addr(const char *sender, const char *addr, int smtputf8)
{
    const RESOLVE_REPLY *resolve_reply;
    const char *myname = "smtpd_check_addr";
    const char *domain;

    if (msg_verbose)
	msg_info("%s: addr=%s", myname, addr);

    /*
     * Catch syntax errors early on if we can, but be prepared to re-compute
     * the result later when the cache fills up with lots of recipients, at
     * which time errors can still happen.
     */
    if (addr == 0 || *addr == 0)
	return (0);
    resolve_reply = smtpd_resolve_addr(sender, addr);
    if (resolve_reply->flags & RESOLVE_FLAG_ERROR)
	return (-1);

    /*
     * Backwards compatibility: if the client does not request SMTPUTF8
     * support, then behave like Postfix < 3.0 trivial-rewrite, and don't
     * allow non-ASCII email domains. Historically, Postfix does not reject
     * UTF8 etc. in the address localpart.
     */
    if (smtputf8 == 0
	&& (domain = strrchr(STR(resolve_reply->recipient), '@')) != 0
	&& *(domain += 1) != 0 && !allascii(domain))
	return (-1);

    return (0);
}

/* smtpd_check_rewrite - choose address qualification context */

char   *smtpd_check_rewrite(SMTPD_STATE *state)
{
    const char *myname = "smtpd_check_rewrite";
    int     status;
    char  **cpp;
    MAPS   *maps;
    char   *name;

    /*
     * We don't use generic_checks() because it produces results that aren't
     * applicable such as DEFER or REJECT.
     */
    for (cpp = local_rewrite_clients->argv; *cpp != 0; cpp++) {
	if (msg_verbose)
	    msg_info("%s: trying: %s", myname, *cpp);
	status = SMTPD_CHECK_DUNNO;
	if (strchr(name = *cpp, ':') != 0) {
	    name = CHECK_ADDR_MAP;
	    cpp -= 1;
	}
	if (strcasecmp(name, PERMIT_INET_INTERFACES) == 0) {
	    status = permit_inet_interfaces(state);
	} else if (strcasecmp(name, PERMIT_MYNETWORKS) == 0) {
	    status = permit_mynetworks(state);
	} else if (is_map_command(state, name, CHECK_ADDR_MAP, &cpp)) {
	    if ((maps = (MAPS *) htable_find(map_command_table, *cpp)) == 0)
		msg_panic("%s: dictionary not found: %s", myname, *cpp);
	    if (maps_find(maps, state->addr, 0) != 0)
		status = SMTPD_CHECK_OK;
	    else if (maps->error != 0) {
		/* Warning is already logged. */
		status = maps->error;
	    }
	} else if (strcasecmp(name, PERMIT_SASL_AUTH) == 0) {
#ifdef USE_SASL_AUTH
	    if (smtpd_sasl_is_active(state))
		status = permit_sasl_auth(state, SMTPD_CHECK_OK,
					  SMTPD_CHECK_DUNNO);
#endif
	} else if (strcasecmp(name, PERMIT_TLS_ALL_CLIENTCERTS) == 0) {
	    status = permit_tls_clientcerts(state, 1);
	} else if (strcasecmp(name, PERMIT_TLS_CLIENTCERTS) == 0) {
	    status = permit_tls_clientcerts(state, 0);
	} else {
	    msg_warn("parameter %s: invalid request: %s",
		     VAR_LOC_RWR_CLIENTS, name);
	    continue;
	}
	if (status < 0) {
	    if (status == DICT_ERR_RETRY) {
		state->error_mask |= MAIL_ERROR_RESOURCE;
		log_whatsup(state, "reject",
			    "451 4.3.0 Temporary lookup error");
		return ("451 4.3.0 Temporary lookup error");
	    } else {
		state->error_mask |= MAIL_ERROR_SOFTWARE;
		log_whatsup(state, "reject",
			    "451 4.3.5 Server configuration error");
		return ("451 4.3.5 Server configuration error");
	    }
	}
	if (status == SMTPD_CHECK_OK) {
	    state->rewrite_context = MAIL_ATTR_RWR_LOCAL;
	    return (0);
	}
    }
    state->rewrite_context = MAIL_ATTR_RWR_REMOTE;
    return (0);
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
    state->sender_rcptmap_checked = 0;

    /*
     * Apply restrictions in the order as specified.
     */
    SMTPD_CHECK_RESET();
    status = setjmp(smtpd_check_buf);
    if (status == 0 && mail_restrctions->argc)
	status = generic_checks(state, mail_restrctions, sender,
				SMTPD_NAME_SENDER, CHECK_SENDER_ACL);
    state->defer_if_permit_sender = state->defer_if_permit.active;

    /*
     * If the "reject_unlisted_sender" restriction still needs to be applied,
     * validate the sender here.
     */
    if (var_smtpd_rej_unl_from
	&& status != SMTPD_CHECK_REJECT && state->sender_rcptmap_checked == 0
	&& state->discard == 0 && *sender)
	status = check_sender_rcpt_maps(state, sender);

    SMTPD_CHECK_MAIL_RETURN(status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);
}

/* smtpd_check_rcpt - validate recipient address, driver */

char   *smtpd_check_rcpt(SMTPD_STATE *state, char *recipient)
{
    int     status;
    char   *saved_recipient;
    char   *err;
    ARGV   *restrctions[2];
    int     n;
    int     rcpt_index;
    int     relay_index;

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
     * responding to RCPT TO or VRFY.
     */
    state->recipient_rcptmap_checked = 0;

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
     * Apply restrictions in the order as specified. We allow relay
     * restrictions to be empty, for sites that require backwards
     * compatibility.
     * 
     * If compatibility_level < 1 and smtpd_relay_restrictions is left at its
     * default value, find out if the new smtpd_relay_restrictions default
     * value would block the request, without logging REJECT messages.
     * Approach: evaluate fake relay restrictions (permit_mynetworks,
     * permit_sasl_authenticated, permit_auth_destination) and log a warning
     * if the result is DUNNO instead of OK, i.e. a reject_unauth_destination
     * at the end would have blocked the request.
     * 
     * If warn_compat_break_relay_restrictions is true, always evaluate
     * smtpd_relay_restrictions last (rcpt_index == 0). The backwards
     * compatibility warning says that it avoids blocking a recipient (with
     * "Relay access denied"); that is not useful information when moments
     * later, smtpd_recipient_restrictions blocks the recipient anyway (with
     * 'Relay access denied' or some other cause).
     */
    SMTPD_CHECK_RESET();
    rcpt_index = (var_relay_before_rcpt_checks
		  && !warn_compat_break_relay_restrictions);
    relay_index = !rcpt_index;

    restrctions[rcpt_index] = rcpt_restrctions;
    restrctions[relay_index] = warn_compat_break_relay_restrictions ?
	fake_relay_restrctions : relay_restrctions;
    for (n = 0; n < 2; n++) {
	status = setjmp(smtpd_check_buf);
	if (status == 0 && restrctions[n]->argc)
	    status = generic_checks(state, restrctions[n],
			  recipient, SMTPD_NAME_RECIPIENT, CHECK_RECIP_ACL);
	if (n == relay_index && warn_compat_break_relay_restrictions
	    && status == SMTPD_CHECK_DUNNO) {
	    msg_info("using backwards-compatible default setting \""
		     VAR_RELAY_CHECKS " = (empty)\" to avoid \"Relay "
		     "access denied\" error for recipient \"%s\" from "
		     "client \"%s\"", state->recipient, state->namaddr);
	}
	if (status == SMTPD_CHECK_REJECT)
	    break;
    }
    if (status == SMTPD_CHECK_REJECT
	&& warn_compat_relay_before_rcpt_checks && n == 0)
	msg_info("using backwards-compatible default setting "
		 VAR_RELAY_BEFORE_RCPT_CHECKS "=no to reject "
		 "recipient \"%s\" from client \"%s\"",
		 state->recipient, state->namaddr);

    /*
     * Force permission into deferral when some earlier temporary error may
     * have prevented us from rejecting mail, and report the earlier problem.
     */
    if (status != SMTPD_CHECK_REJECT && state->defer_if_permit.active)
	status = smtpd_check_reject(state, state->defer_if_permit.class,
				    state->defer_if_permit.code,
				    STR(state->defer_if_permit.dsn),
				  "%s", STR(state->defer_if_permit.reason));

    /*
     * If the "reject_unlisted_recipient" restriction still needs to be
     * applied, validate the recipient here.
     */
    if (var_smtpd_rej_unl_rcpt
	&& status != SMTPD_CHECK_REJECT
	&& state->recipient_rcptmap_checked == 0
	&& state->discard == 0)
	status = check_recipient_rcpt_maps(state, recipient);

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
				    state->defer_if_permit.code,
				    STR(state->defer_if_permit.dsn),
				  "%s", STR(state->defer_if_permit.reason));

    SMTPD_CHECK_ETRN_RETURN(status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);
}

/* check_recipient_rcpt_maps - generic_checks() recipient table check */

static int check_recipient_rcpt_maps(SMTPD_STATE *state, const char *recipient)
{

    /*
     * Duplicate suppression. There's an implicit check_recipient_maps
     * restriction at the end of all recipient restrictions.
     */
    if (smtpd_input_transp_mask & INPUT_TRANSP_UNKNOWN_RCPT)
	return (0);
    if (state->recipient_rcptmap_checked == 1)
	return (0);
    if (state->warn_if_reject == 0)
	/* We really validate the recipient address. */
	state->recipient_rcptmap_checked = 1;
    return (check_rcpt_maps(state, state->sender, recipient,
			    SMTPD_NAME_RECIPIENT));
}

/* check_sender_rcpt_maps - generic_checks() sender table check */

static int check_sender_rcpt_maps(SMTPD_STATE *state, const char *sender)
{

    /*
     * Duplicate suppression. There's an implicit check_sender_maps
     * restriction at the end of all sender restrictions.
     */
    if (smtpd_input_transp_mask & INPUT_TRANSP_UNKNOWN_RCPT)
	return (0);
    if (state->sender_rcptmap_checked == 1)
	return (0);
    if (state->warn_if_reject == 0)
	/* We really validate the sender address. */
	state->sender_rcptmap_checked = 1;
    return (check_rcpt_maps(state, state->recipient, sender,
			    SMTPD_NAME_SENDER));
}

/* check_rcpt_maps - generic_checks() interface for recipient table check */

static int check_rcpt_maps(SMTPD_STATE *state, const char *sender,
			           const char *recipient,
			           const char *reply_class)
{
    const RESOLVE_REPLY *reply;
    DSN_SPLIT dp;

    if (msg_verbose)
	msg_info(">>> CHECKING %s VALIDATION MAPS <<<", reply_class);

    /*
     * Resolve the address.
     */
    reply = smtpd_resolve_addr(sender, recipient);
    if (reply->flags & RESOLVE_FLAG_FAIL)
	reject_dict_retry(state, recipient);

    /*
     * Make complex expressions more readable?
     */
#define MATCH(map, rcpt) \
    check_mail_addr_find(state, recipient, map, rcpt, (char **) 0)

#define NOMATCH(map, rcpt) (MATCH(map, rcpt) == 0)

    /*
     * XXX We assume the recipient address is OK if it matches a canonical
     * map or virtual alias map. Eventually, the address resolver should give
     * us the final resolved recipient address, and the SMTP server should
     * write the final resolved recipient address to the output record
     * stream. See also the next comment block on recipients in virtual alias
     * domains.
     */
    if (MATCH(rcpt_canon_maps, CONST_STR(reply->recipient))
	|| (strcmp(reply_class, SMTPD_NAME_SENDER) == 0
	    && MATCH(send_canon_maps, CONST_STR(reply->recipient)))
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
    if (strcmp(STR(reply->transport), MAIL_SERVICE_ERROR) == 0) {
	dsn_split(&dp, strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
		  "5.1.0" : "5.1.1", STR(reply->nexthop));
	return (smtpd_check_reject(state, MAIL_ERROR_BOUNCE,
				   (reply->flags & RESOLVE_CLASS_ALIAS) ?
				   var_virt_alias_code : 550,
				   smtpd_dsn_fix(DSN_STATUS(dp.dsn),
						 reply_class),
				   "<%s>: %s rejected: %s",
				   recipient, reply_class,
				   dp.text));
    }
    if (strcmp(STR(reply->transport), MAIL_SERVICE_RETRY) == 0) {
	dsn_split(&dp, strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
		  "4.1.0" : "4.1.1", STR(reply->nexthop));
	return (smtpd_check_reject(state, MAIL_ERROR_BOUNCE, 450,
				   smtpd_dsn_fix(DSN_STATUS(dp.dsn),
						 reply_class),
				   "<%s>: %s rejected: %s",
				   recipient, reply_class,
				   dp.text));
    }

    /*
     * Search the recipient lookup tables of the respective address class.
     * 
     * XXX Use the less expensive maps_find() (built-in case folding) instead of
     * the baroque mail_addr_find(). But then we have to strip the domain and
     * deal with address extensions ourselves.
     * 
     * XXX But that would break sites that use the virtual delivery agent for
     * local delivery, because the virtual delivery agent requires
     * user@domain style addresses in its user database.
     */
#define MATCH_LEFT(l, r, n) \
	(strncasecmp_utf8((l), (r), (n)) == 0 && (r)[n] == '@')

    switch (reply->flags & RESOLVE_CLASS_MASK) {

	/*
	 * Reject mail to unknown addresses in local domains (domains that
	 * match $mydestination or ${proxy,inet}_interfaces).
	 */
    case RESOLVE_CLASS_LOCAL:
	if (*var_local_rcpt_maps
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
				       var_local_rcpt_code,
			       strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
				       "5.1.0" : "5.1.1",
				       "<%s>: %s rejected: User unknown%s",
				       recipient, reply_class,
				       var_show_unk_rcpt_table ?
				       " in local recipient table" : ""));
	break;

	/*
	 * Reject mail to unknown addresses in virtual mailbox domains.
	 */
    case RESOLVE_CLASS_VIRTUAL:
	if (*var_virt_mailbox_maps
	    && NOMATCH(virt_mailbox_maps, CONST_STR(reply->recipient)))
	    return (smtpd_check_reject(state, MAIL_ERROR_BOUNCE,
				       var_virt_mailbox_code,
			       strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
				       "5.1.0" : "5.1.1",
				       "<%s>: %s rejected: User unknown%s",
				       recipient, reply_class,
				       var_show_unk_rcpt_table ?
				       " in virtual mailbox table" : ""));
	break;

	/*
	 * Reject mail to unknown addresses in relay domains.
	 */
    case RESOLVE_CLASS_RELAY:
	if (*var_relay_rcpt_maps
	    && NOMATCH(relay_rcpt_maps, CONST_STR(reply->recipient)))
	    return (smtpd_check_reject(state, MAIL_ERROR_BOUNCE,
				       var_relay_rcpt_code,
			       strcmp(reply_class, SMTPD_NAME_SENDER) == 0 ?
				       "5.1.0" : "5.1.1",
				       "<%s>: %s rejected: User unknown%s",
				       recipient, reply_class,
				       var_show_unk_rcpt_table ?
				       " in relay recipient table" : ""));
	if (warn_compat_break_relay_domains)
	    msg_info("using backwards-compatible default setting "
		     VAR_RELAY_DOMAINS "=$mydestination to accept mail "
		     "for address \"%s\"", recipient);
	break;
    }

    /*
     * Accept all other addresses - including addresses that passed the above
     * tests because of some table lookup problem.
     */
    return (0);
}

/* smtpd_check_size - check optional SIZE parameter value */

char   *smtpd_check_size(SMTPD_STATE *state, off_t size)
{
    int     status;

    /*
     * Return here in case of serious trouble.
     */
    SMTPD_CHECK_RESET();
    if ((status = setjmp(smtpd_check_buf)) != 0)
	return (status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);

    /*
     * Check against file size limit.
     */
    if (ENFORCING_SIZE_LIMIT(var_message_limit) && size > var_message_limit) {
	(void) smtpd_check_reject(state, MAIL_ERROR_POLICY,
				  552, "5.3.4",
				  "Message size exceeds fixed limit");
	return (STR(error_text));
    }
    return (0);
}

/* smtpd_check_queue - check queue space */

char   *smtpd_check_queue(SMTPD_STATE *state)
{
    const char *myname = "smtpd_check_queue";
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

    fsspace(".", &fsbuf);
    if (msg_verbose)
	msg_info("%s: blocks %lu avail %lu min_free %lu msg_size_limit %lu",
		 myname,
		 (unsigned long) fsbuf.block_size,
		 (unsigned long) fsbuf.block_free,
		 (unsigned long) var_queue_minfree,
		 (unsigned long) var_message_limit);
    if (BLOCKS(var_queue_minfree) >= fsbuf.block_free
     || BLOCKS(var_message_limit) >= fsbuf.block_free / smtpd_space_multf) {
	(void) smtpd_check_reject(state, MAIL_ERROR_RESOURCE,
				  452, "4.3.1",
				  "Insufficient system storage");
	msg_warn("not enough free space in mail queue: %lu bytes < "
		 "%g*message size limit",
		 (unsigned long) fsbuf.block_free * fsbuf.block_size,
		 smtpd_space_multf);
	return (STR(error_text));
    }
    return (0);
}

/* smtpd_check_data - check DATA command */

char   *smtpd_check_data(SMTPD_STATE *state)
{
    int     status;
    char   *NOCLOBBER saved_recipient;

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
				SMTPD_CMD_DATA, SMTPD_NAME_DATA, NO_DEF_ACL);

    /*
     * Force permission into deferral when some earlier temporary error may
     * have prevented us from rejecting mail, and report the earlier problem.
     */
    if (status != SMTPD_CHECK_REJECT && state->defer_if_permit.active)
	status = smtpd_check_reject(state, state->defer_if_permit.class,
				    state->defer_if_permit.code,
				    STR(state->defer_if_permit.dsn),
				  "%s", STR(state->defer_if_permit.reason));

    if (state->rcpt_count > 1)
	state->recipient = saved_recipient;

    return (status == SMTPD_CHECK_REJECT ? STR(error_text) : 0);
}

/* smtpd_check_eod - check end-of-data command */

char   *smtpd_check_eod(SMTPD_STATE *state)
{
    int     status;
    char   *NOCLOBBER saved_recipient;

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
    if (status == 0 && eod_restrictions->argc)
	status = generic_checks(state, eod_restrictions,
				SMTPD_CMD_EOD, SMTPD_NAME_EOD, NO_DEF_ACL);

    /*
     * Force permission into deferral when some earlier temporary error may
     * have prevented us from rejecting mail, and report the earlier problem.
     */
    if (status != SMTPD_CHECK_REJECT && state->defer_if_permit.active)
	status = smtpd_check_reject(state, state->defer_if_permit.class,
				    state->defer_if_permit.code,
				    STR(state->defer_if_permit.dsn),
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
#include <rewrite_clnt.h>
#include <dns.h>

#include <smtpd_chat.h>

int     smtpd_input_transp_mask;

 /*
  * Dummies. These are never set.
  */
char   *var_client_checks = "";
char   *var_helo_checks = "";
char   *var_mail_checks = "";
char   *var_relay_checks = "";
char   *var_rcpt_checks = "";
char   *var_etrn_checks = "";
char   *var_data_checks = "";
char   *var_eod_checks = "";
char   *var_smtpd_uproxy_proto = "";
int     var_smtpd_uproxy_tmout = 0;

#ifdef USE_TLS
char   *var_relay_ccerts = "";

#endif
char   *var_notify_classes = "";
char   *var_smtpd_policy_def_action = "";
char   *var_smtpd_policy_context = "";

 /*
  * String-valued configuration parameters.
  */
char   *var_maps_rbl_domains;
char   *var_rest_classes;
char   *var_alias_maps;
char   *var_send_canon_maps;
char   *var_rcpt_canon_maps;
char   *var_canonical_maps;
char   *var_virt_alias_maps;
char   *var_virt_alias_doms;
char   *var_virt_mailbox_maps;
char   *var_virt_mailbox_doms;
char   *var_local_rcpt_maps;
char   *var_perm_mx_networks;
char   *var_smtpd_null_key;
char   *var_smtpd_snd_auth_maps;
char   *var_rbl_reply_maps;
char   *var_smtpd_exp_filter;
char   *var_def_rbl_reply;
char   *var_relay_rcpt_maps;
char   *var_verify_sender;
char   *var_smtpd_sasl_opts;
char   *var_local_rwr_clients;
char   *var_smtpd_relay_ccerts;
char   *var_unv_from_why;
char   *var_unv_rcpt_why;
char   *var_stress;
char   *var_unk_name_tf_act;
char   *var_unk_addr_tf_act;
char   *var_unv_rcpt_tf_act;
char   *var_unv_from_tf_act;
char   *var_smtpd_acl_perm_log;

typedef struct {
    char   *name;
    char   *defval;
    char  **target;
} STRING_TABLE;

#undef DEF_VIRT_ALIAS_MAPS
#define DEF_VIRT_ALIAS_MAPS	""

#undef DEF_LOCAL_RCPT_MAPS
#define DEF_LOCAL_RCPT_MAPS	""

static const STRING_TABLE string_table[] = {
    VAR_MAPS_RBL_DOMAINS, DEF_MAPS_RBL_DOMAINS, &var_maps_rbl_domains,
    VAR_MYORIGIN, DEF_MYORIGIN, &var_myorigin,
    VAR_MYDEST, DEF_MYDEST, &var_mydest,
    VAR_INET_INTERFACES, DEF_INET_INTERFACES, &var_inet_interfaces,
    VAR_PROXY_INTERFACES, DEF_PROXY_INTERFACES, &var_proxy_interfaces,
    VAR_RCPT_DELIM, DEF_RCPT_DELIM, &var_rcpt_delim,
    VAR_REST_CLASSES, DEF_REST_CLASSES, &var_rest_classes,
    VAR_ALIAS_MAPS, DEF_ALIAS_MAPS, &var_alias_maps,
    VAR_SEND_CANON_MAPS, DEF_SEND_CANON_MAPS, &var_send_canon_maps,
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
    VAR_VERIFY_SENDER, DEF_VERIFY_SENDER, &var_verify_sender,
    VAR_MAIL_NAME, DEF_MAIL_NAME, &var_mail_name,
    VAR_SMTPD_SASL_OPTS, DEF_SMTPD_SASL_OPTS, &var_smtpd_sasl_opts,
    VAR_LOC_RWR_CLIENTS, DEF_LOC_RWR_CLIENTS, &var_local_rwr_clients,
    VAR_RELAY_CCERTS, DEF_RELAY_CCERTS, &var_smtpd_relay_ccerts,
    VAR_UNV_FROM_WHY, DEF_UNV_FROM_WHY, &var_unv_from_why,
    VAR_UNV_RCPT_WHY, DEF_UNV_RCPT_WHY, &var_unv_rcpt_why,
    VAR_STRESS, DEF_STRESS, &var_stress,
    /* XXX Can't use ``$name'' type default values below. */
    VAR_UNK_NAME_TF_ACT, DEF_REJECT_TMPF_ACT, &var_unk_name_tf_act,
    VAR_UNK_ADDR_TF_ACT, DEF_REJECT_TMPF_ACT, &var_unk_addr_tf_act,
    VAR_UNV_RCPT_TF_ACT, DEF_REJECT_TMPF_ACT, &var_unv_rcpt_tf_act,
    VAR_UNV_FROM_TF_ACT, DEF_REJECT_TMPF_ACT, &var_unv_from_tf_act,
    /* XXX Can't use ``$name'' type default values above. */
    VAR_SMTPD_ACL_PERM_LOG, DEF_SMTPD_ACL_PERM_LOG, &var_smtpd_acl_perm_log,
    VAR_SMTPD_DNS_RE_FILTER, DEF_SMTPD_DNS_RE_FILTER, &var_smtpd_dns_re_filter,
    VAR_INFO_LOG_ADDR_FORM, DEF_INFO_LOG_ADDR_FORM, &var_info_log_addr_form,
    /* XXX No static initialization with "", because owned by a library. */
    VAR_MYNETWORKS, "", &var_mynetworks,
    VAR_RELAY_DOMAINS, "", &var_relay_domains,
    0,
};

/* string_init - initialize string parameters */

static void string_init(void)
{
    const STRING_TABLE *sp;

    for (sp = string_table; sp->name; sp++)
	sp->target[0] = mystrdup(sp->defval);
}

/* string_update - update string parameter */

static int string_update(char **argv)
{
    const STRING_TABLE *sp;

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
long    var_queue_minfree;		/* XXX use off_t */
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
int     var_map_reject_code;
int     var_map_defer_code;
int     var_reject_code;
int     var_defer_code;
int     var_non_fqdn_code;
int     var_smtpd_delay_reject;
int     var_allow_untrust_route;
int     var_mul_rcpt_code;
int     var_unv_from_rcode;
int     var_unv_from_dcode;
int     var_unv_rcpt_rcode;
int     var_unv_rcpt_dcode;
int     var_local_rcpt_code;
int     var_relay_rcpt_code;
int     var_virt_mailbox_code;
int     var_virt_alias_code;
int     var_show_unk_rcpt_table;
int     var_verify_poll_count;
int     var_verify_poll_delay;
int     var_smtpd_policy_tmout;
int     var_smtpd_policy_idle;
int     var_smtpd_policy_ttl;
int     var_smtpd_policy_req_limit;
int     var_smtpd_policy_try_limit;
int     var_smtpd_policy_try_delay;
int     var_smtpd_rej_unl_from;
int     var_smtpd_rej_unl_rcpt;
int     var_plaintext_code;
bool    var_smtpd_peername_lookup;
bool    var_smtpd_client_port_log;
char   *var_smtpd_dns_re_filter;
bool    var_smtpd_tls_ask_ccert;
int     var_smtpd_cipv4_prefix;
int     var_smtpd_cipv6_prefix;

#define int_table test_int_table

static const INT_TABLE int_table[] = {
    "msg_verbose", 0, &msg_verbose,
    VAR_UNK_CLIENT_CODE, DEF_UNK_CLIENT_CODE, &var_unk_client_code,
    VAR_BAD_NAME_CODE, DEF_BAD_NAME_CODE, &var_bad_name_code,
    VAR_UNK_NAME_CODE, DEF_UNK_NAME_CODE, &var_unk_name_code,
    VAR_UNK_ADDR_CODE, DEF_UNK_ADDR_CODE, &var_unk_addr_code,
    VAR_RELAY_CODE, DEF_RELAY_CODE, &var_relay_code,
    VAR_MAPS_RBL_CODE, DEF_MAPS_RBL_CODE, &var_maps_rbl_code,
    VAR_MAP_REJECT_CODE, DEF_MAP_REJECT_CODE, &var_map_reject_code,
    VAR_MAP_DEFER_CODE, DEF_MAP_DEFER_CODE, &var_map_defer_code,
    VAR_REJECT_CODE, DEF_REJECT_CODE, &var_reject_code,
    VAR_DEFER_CODE, DEF_DEFER_CODE, &var_defer_code,
    VAR_NON_FQDN_CODE, DEF_NON_FQDN_CODE, &var_non_fqdn_code,
    VAR_SMTPD_DELAY_REJECT, DEF_SMTPD_DELAY_REJECT, &var_smtpd_delay_reject,
    VAR_ALLOW_UNTRUST_ROUTE, DEF_ALLOW_UNTRUST_ROUTE, &var_allow_untrust_route,
    VAR_MUL_RCPT_CODE, DEF_MUL_RCPT_CODE, &var_mul_rcpt_code,
    VAR_UNV_FROM_RCODE, DEF_UNV_FROM_RCODE, &var_unv_from_rcode,
    VAR_UNV_FROM_DCODE, DEF_UNV_FROM_DCODE, &var_unv_from_dcode,
    VAR_UNV_RCPT_RCODE, DEF_UNV_RCPT_RCODE, &var_unv_rcpt_rcode,
    VAR_UNV_RCPT_DCODE, DEF_UNV_RCPT_DCODE, &var_unv_rcpt_dcode,
    VAR_LOCAL_RCPT_CODE, DEF_LOCAL_RCPT_CODE, &var_local_rcpt_code,
    VAR_RELAY_RCPT_CODE, DEF_RELAY_RCPT_CODE, &var_relay_rcpt_code,
    VAR_VIRT_ALIAS_CODE, DEF_VIRT_ALIAS_CODE, &var_virt_alias_code,
    VAR_VIRT_MAILBOX_CODE, DEF_VIRT_MAILBOX_CODE, &var_virt_mailbox_code,
    VAR_SHOW_UNK_RCPT_TABLE, DEF_SHOW_UNK_RCPT_TABLE, &var_show_unk_rcpt_table,
    VAR_VERIFY_POLL_COUNT, 3, &var_verify_poll_count,
    VAR_SMTPD_REJ_UNL_FROM, DEF_SMTPD_REJ_UNL_FROM, &var_smtpd_rej_unl_from,
    VAR_SMTPD_REJ_UNL_RCPT, DEF_SMTPD_REJ_UNL_RCPT, &var_smtpd_rej_unl_rcpt,
    VAR_PLAINTEXT_CODE, DEF_PLAINTEXT_CODE, &var_plaintext_code,
    VAR_SMTPD_PEERNAME_LOOKUP, DEF_SMTPD_PEERNAME_LOOKUP, &var_smtpd_peername_lookup,
    VAR_SMTPD_CLIENT_PORT_LOG, DEF_SMTPD_CLIENT_PORT_LOG, &var_smtpd_client_port_log,
    VAR_SMTPD_TLS_ACERT, DEF_SMTPD_TLS_ACERT, &var_smtpd_tls_ask_ccert,
    VAR_SMTPD_CIPV4_PREFIX, DEF_SMTPD_CIPV4_PREFIX, &var_smtpd_cipv4_prefix,
    VAR_SMTPD_CIPV6_PREFIX, DEF_SMTPD_CIPV6_PREFIX, &var_smtpd_cipv6_prefix,
    0,
};

/* int_init - initialize int parameters */

static void int_init(void)
{
    const INT_TABLE *sp;

    for (sp = int_table; sp->name; sp++)
	sp->target[0] = sp->defval;
}

/* int_update - update int parameter */

static int int_update(char **argv)
{
    const INT_TABLE *ip;

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
  * Boolean parameters.
  */
bool    var_relay_before_rcpt_checks;

 /*
  * Restrictions.
  */
typedef struct {
    char   *name;
    ARGV  **target;
} REST_TABLE;

static const REST_TABLE rest_table[] = {
    "client_restrictions", &client_restrctions,
    "helo_restrictions", &helo_restrctions,
    "sender_restrictions", &mail_restrctions,
    "relay_restrictions", &relay_restrctions,
    "recipient_restrictions", &rcpt_restrctions,
    "etrn_restrictions", &etrn_restrctions,
    0,
};

/* rest_update - update restriction */

static int rest_update(char **argv)
{
    const REST_TABLE *rp;

    for (rp = rest_table; rp->name; rp++) {
	if (strcasecmp(rp->name, argv[0]) == 0) {
	    argv_free(rp->target[0]);
	    rp->target[0] = smtpd_check_parse(SMTPD_CHECK_PARSE_ALL, argv[1]);
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

    if ((name = mystrtok(&cp, CHARS_COMMA_SP)) == 0)
	msg_panic("rest_class: null class name");
    if ((entry = htable_locate(smtpd_rest_classes, name)) != 0)
	argv_free((ARGV *) entry->value);
    else
	entry = htable_enter(smtpd_rest_classes, name, (void *) 0);
    entry->value = (void *) smtpd_check_parse(SMTPD_CHECK_PARSE_ALL, cp);
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

/* smtpd_sasl_activate - stub */

void    smtpd_sasl_activate(SMTPD_STATE *state, const char *opts_name,
			            const char *opts_var)
{
    msg_panic("smtpd_sasl_activate was called");
}

/* smtpd_sasl_deactivate - stub */

void    smtpd_sasl_deactivate(SMTPD_STATE *state)
{
    msg_panic("smtpd_sasl_deactivate was called");
}

/* permit_sasl_auth - stub */

int     permit_sasl_auth(SMTPD_STATE *state, int ifyes, int ifnot)
{
    return (ifnot);
}

/* smtpd_sasl_state_init - the real deal */

void    smtpd_sasl_state_init(SMTPD_STATE *state)
{
    state->sasl_username = 0;
    state->sasl_method = 0;
    state->sasl_sender = 0;
}

#endif

/* verify_clnt_query - stub */

int     verify_clnt_query(const char *addr, int *addr_status, VSTRING *why)
{
    *addr_status = DEL_RCPT_STAT_OK;
    return (VRFY_STAT_OK);
}

/* rewrite_clnt_internal - stub */

VSTRING *rewrite_clnt_internal(const char *context, const char *addr,
			               VSTRING *result)
{
    if (addr == STR(result))
	msg_panic("rewrite_clnt_internal: result clobbers input");
    if (*addr && strchr(addr, '@') == 0)
	msg_fatal("%s: address rewriting is disabled", addr);
    vstring_strcpy(result, addr);
    return (result);
}

/* resolve_clnt_query - stub */

void    resolve_clnt(const char *class, const char *unused_sender, const char *addr,
		             RESOLVE_REPLY *reply)
{
    const char *domain;
    int     rc;

    if (addr == CONST_STR(reply->recipient))
	msg_panic("resolve_clnt_query: result clobbers input");
    if (strchr(addr, '%'))
	msg_fatal("%s: address rewriting is disabled", addr);
    if ((domain = strrchr(addr, '@')) == 0)
	msg_fatal("%s: unqualified address", addr);
    domain += 1;
    if ((rc = resolve_local(domain)) > 0) {
	reply->flags = RESOLVE_CLASS_LOCAL;
	vstring_strcpy(reply->transport, MAIL_SERVICE_LOCAL);
	vstring_strcpy(reply->nexthop, domain);
    } else if (rc < 0) {
	reply->flags = RESOLVE_FLAG_FAIL;
    } else if (string_list_match(virt_alias_doms, domain)) {
	reply->flags = RESOLVE_CLASS_ALIAS;
	vstring_strcpy(reply->transport, MAIL_SERVICE_ERROR);
	vstring_strcpy(reply->nexthop, "user unknown");
    } else if (virt_alias_doms->error) {
	reply->flags = RESOLVE_FLAG_FAIL;
    } else if (string_list_match(virt_mailbox_doms, domain)) {
	reply->flags = RESOLVE_CLASS_VIRTUAL;
	vstring_strcpy(reply->transport, MAIL_SERVICE_VIRTUAL);
	vstring_strcpy(reply->nexthop, domain);
    } else if (virt_mailbox_doms->error) {
	reply->flags = RESOLVE_FLAG_FAIL;
    } else if (domain_list_match(relay_domains, domain)) {
	reply->flags = RESOLVE_CLASS_RELAY;
	vstring_strcpy(reply->transport, MAIL_SERVICE_RELAY);
	vstring_strcpy(reply->nexthop, domain);
    } else if (relay_domains->error) {
	reply->flags = RESOLVE_FLAG_FAIL;
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
    smtpd_expand_init();
    (void) inet_proto_init(argv[0], INET_PROTO_NAME_IPV4);
    smtpd_state_init(&state, VSTREAM_IN, "smtpd");
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
	args = argv_splitq(bp, CHARS_SPACE, CHARS_BRACE);

	/*
	 * Recognize the command.
	 */
	resp = "bad command";
	switch (args->argc) {

	    /*
	     * Emtpy line.
	     */
	case 0:
	    argv_free(args);
	    continue;

	    /*
	     * Special case: rewrite context.
	     */
	case 1:
	    if (strcasecmp(args->argv[0], "rewrite") == 0) {
		resp = smtpd_check_rewrite(&state);
		break;
	    }

	    /*
	     * Other parameter-less commands.
	     */
	    if (strcasecmp(args->argv[0], "flush_dnsxl_cache") == 0) {
		if (smtpd_rbl_cache) {
		    ctable_free(smtpd_rbl_cache);
		    ctable_free(smtpd_rbl_byte_cache);
		}
		smtpd_rbl_cache = ctable_create(100, rbl_pagein,
						rbl_pageout, (void *) 0);
		smtpd_rbl_byte_cache = ctable_create(1000, rbl_byte_pagein,
					      rbl_byte_pageout, (void *) 0);
		resp = 0;
		break;
	    }

	    /*
	     * Special case: client identity.
	     */
	case 4:
	case 3:
	    if (strcasecmp(args->argv[0], "client") == 0) {
		state.where = SMTPD_AFTER_CONNECT;
		UPDATE_STRING(state.name, args->argv[1]);
		UPDATE_STRING(state.reverse_name, args->argv[1]);
		UPDATE_STRING(state.addr, args->argv[2]);
		if (args->argc == 4)
		    state.name_status =
			state.reverse_name_status =
			atoi(args->argv[3]);
		else if (strcmp(state.name, "unknown") == 0)
		    state.name_status =
			state.reverse_name_status =
			SMTPD_PEER_CODE_TEMP;
		else
		    state.name_status =
			state.reverse_name_status =
			SMTPD_PEER_CODE_OK;
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

#define UPDATE_LIST(ptr, var, val) \
	{ if (ptr) string_list_free(ptr); \
	  ptr = string_list_init(var, MATCH_FLAG_NONE, val); }

	case 2:
	    if (strcasecmp(args->argv[0], VAR_MYDEST) == 0) {
		UPDATE_STRING(var_mydest, args->argv[1]);
		resolve_local_init();
		smtpd_resolve_init(100);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_VIRT_ALIAS_MAPS) == 0) {
		UPDATE_STRING(var_virt_alias_maps, args->argv[1]);
		UPDATE_MAPS(virt_alias_maps, VAR_VIRT_ALIAS_MAPS,
			    var_virt_alias_maps, DICT_FLAG_LOCK
			    | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_VIRT_ALIAS_DOMS) == 0) {
		UPDATE_STRING(var_virt_alias_doms, args->argv[1]);
		UPDATE_LIST(virt_alias_doms, VAR_VIRT_ALIAS_DOMS,
			    var_virt_alias_doms);
		smtpd_resolve_init(100);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_VIRT_MAILBOX_MAPS) == 0) {
		UPDATE_STRING(var_virt_mailbox_maps, args->argv[1]);
		UPDATE_MAPS(virt_mailbox_maps, VAR_VIRT_MAILBOX_MAPS,
			    var_virt_mailbox_maps, DICT_FLAG_LOCK
			    | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_VIRT_MAILBOX_DOMS) == 0) {
		UPDATE_STRING(var_virt_mailbox_doms, args->argv[1]);
		UPDATE_LIST(virt_mailbox_doms, VAR_VIRT_MAILBOX_DOMS,
			    var_virt_mailbox_doms);
		smtpd_resolve_init(100);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_LOCAL_RCPT_MAPS) == 0) {
		UPDATE_STRING(var_local_rcpt_maps, args->argv[1]);
		UPDATE_MAPS(local_rcpt_maps, VAR_LOCAL_RCPT_MAPS,
			    var_local_rcpt_maps, DICT_FLAG_LOCK
			    | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_RELAY_RCPT_MAPS) == 0) {
		UPDATE_STRING(var_relay_rcpt_maps, args->argv[1]);
		UPDATE_MAPS(relay_rcpt_maps, VAR_RELAY_RCPT_MAPS,
			    var_relay_rcpt_maps, DICT_FLAG_LOCK
			    | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_CANONICAL_MAPS) == 0) {
		UPDATE_STRING(var_canonical_maps, args->argv[1]);
		UPDATE_MAPS(canonical_maps, VAR_CANONICAL_MAPS,
			    var_canonical_maps, DICT_FLAG_LOCK
			    | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_SEND_CANON_MAPS) == 0) {
		UPDATE_STRING(var_send_canon_maps, args->argv[1]);
		UPDATE_MAPS(send_canon_maps, VAR_SEND_CANON_MAPS,
			    var_send_canon_maps, DICT_FLAG_LOCK
			    | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_RCPT_CANON_MAPS) == 0) {
		UPDATE_STRING(var_rcpt_canon_maps, args->argv[1]);
		UPDATE_MAPS(rcpt_canon_maps, VAR_RCPT_CANON_MAPS,
			    var_rcpt_canon_maps, DICT_FLAG_LOCK
			    | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_RBL_REPLY_MAPS) == 0) {
		UPDATE_STRING(var_rbl_reply_maps, args->argv[1]);
		UPDATE_MAPS(rbl_reply_maps, VAR_RBL_REPLY_MAPS,
			    var_rbl_reply_maps, DICT_FLAG_LOCK
			    | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_MYNETWORKS) == 0) {
		/* NOT: UPDATE_STRING */
		namadr_list_free(mynetworks_curr);
		mynetworks_curr =
		    namadr_list_init(VAR_MYNETWORKS, MATCH_FLAG_RETURN
				     | match_parent_style(VAR_MYNETWORKS),
				     args->argv[1]);
		smtpd_resolve_init(100);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_RELAY_DOMAINS) == 0) {
		/* NOT: UPDATE_STRING */
		domain_list_free(relay_domains);
		relay_domains =
		    domain_list_init(VAR_RELAY_DOMAINS,
				     match_parent_style(VAR_RELAY_DOMAINS),
				     args->argv[1]);
		smtpd_resolve_init(100);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_PERM_MX_NETWORKS) == 0) {
		UPDATE_STRING(var_perm_mx_networks, args->argv[1]);
		domain_list_free(perm_mx_networks);
		perm_mx_networks =
		    namadr_list_init(VAR_PERM_MX_NETWORKS, MATCH_FLAG_RETURN
				 | match_parent_style(VAR_PERM_MX_NETWORKS),
				     args->argv[1]);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_SMTPD_DNS_RE_FILTER) == 0) {
		/* NOT: UPDATE_STRING */
		dns_rr_filter_compile(VAR_SMTPD_DNS_RE_FILTER, args->argv[1]);
		resp = 0;
		break;
	    }
#ifdef USE_TLS
	    if (strcasecmp(args->argv[0], VAR_RELAY_CCERTS) == 0) {
		UPDATE_STRING(var_smtpd_relay_ccerts, args->argv[1]);
		UPDATE_MAPS(relay_ccerts, VAR_RELAY_CCERTS,
			    var_smtpd_relay_ccerts, DICT_FLAG_LOCK
			    | DICT_FLAG_FOLD_FIX);
		resp = 0;
	    }
#endif
	    if (strcasecmp(args->argv[0], "restriction_class") == 0) {
		rest_class(args->argv[1]);
		resp = 0;
		break;
	    }
	    if (strcasecmp(args->argv[0], VAR_LOC_RWR_CLIENTS) == 0) {
		UPDATE_STRING(var_local_rwr_clients, args->argv[1]);
		argv_free(local_rewrite_clients);
		local_rewrite_clients = smtpd_check_parse(SMTPD_CHECK_PARSE_MAPS,
						     var_local_rwr_clients);
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
		resp = smtpd_check_rcpt(&state, addr);
#ifdef USE_TLS
	    } else if (strcasecmp(args->argv[0], "fingerprint") == 0) {
		if (state.tls_context == 0) {
		    state.tls_context =
			(TLS_SESS_STATE *) mymalloc(sizeof(*state.tls_context));
		    memset((void *) state.tls_context, 0,
			   sizeof(*state.tls_context));
		    state.tls_context->peer_cert_fprint =
			state.tls_context->peer_pkey_fprint = 0;
		}
		state.tls_context->peer_status |= TLS_CERT_FLAG_PRESENT;
		UPDATE_STRING(state.tls_context->peer_cert_fprint,
			      args->argv[1]);
		state.tls_context->peer_pkey_fprint =
		    state.tls_context->peer_cert_fprint;
		resp = "OK";
		break;
#endif
	    }
	    break;

	    /*
	     * Show commands.
	     */
	default:
	    if (strcasecmp(args->argv[0], "check_rewrite") == 0) {
		smtpd_check_rewrite(&state);
		resp = state.rewrite_context;
		break;
	    }
	    resp = "Commands...\n\
		client <name> <address> [<code>]\n\
		helo <hostname>\n\
		sender <address>\n\
		recipient <address>\n\
		check_rewrite\n\
		msg_verbose <level>\n\
		client_restrictions <restrictions>\n\
		helo_restrictions <restrictions>\n\
		sender_restrictions <restrictions>\n\
		recipient_restrictions <restrictions>\n\
		restriction_class name,<restrictions>\n\
		flush_dnsxl_cache\n\
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
#ifdef USE_TLS
    if (state.tls_context) {
	FREE_STRING(state.tls_context->peer_cert_fprint);
	myfree((void *) state.tls_context);
    }
#endif
    exit(0);
}

#endif
