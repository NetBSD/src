/*++
/* NAME
/*	smtpd 8
/* SUMMARY
/*	Postfix SMTP server
/* SYNOPSIS
/*	\fBsmtpd\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The SMTP server accepts network connection requests
/*	and performs zero or more SMTP transactions per connection.
/*	Each received message is piped through the \fBcleanup\fR(8)
/*	daemon, and is placed into the \fBincoming\fR queue as one
/*	single queue file.  For this mode of operation, the program
/*	expects to be run from the \fBmaster\fR(8) process manager.
/*
/*	Alternatively, the SMTP server takes an established
/*	connection on standard input and deposits messages directly
/*	into the \fBmaildrop\fR queue. In this so-called stand-alone
/*	mode, the SMTP server can accept mail even while the mail
/*	system is not running.
/*
/*	The SMTP server implements a variety of policies for connection
/*	requests, and for parameters given to \fBHELO, MAIL FROM, VRFY\fR
/*	and \fBRCPT TO\fR commands. They are detailed below and in the
/*	\fBmain.cf\fR configuration file.
/* SECURITY
/* .ad
/* .fi
/*	The SMTP server is moderately security-sensitive. It talks to SMTP
/*	clients and to DNS servers on the network. The SMTP server can be
/*	run chrooted at fixed low privilege.
/* STANDARDS
/*	RFC 821 (SMTP protocol)
/*	RFC 1123 (Host requirements)
/*	RFC 1651 (SMTP service extensions)
/*	RFC 1652 (8bit-MIME transport)
/*	RFC 1854 (SMTP Pipelining)
/*	RFC 1870 (Message Size Declaration)
/*	RFC 1985 (ETRN command) (partial)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces, protocol problems,
/*	policy violations, and of other trouble.
/* BUGS
/*	RFC 1985 is implemented by forcing delivery of all deferred mail.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values. Use the \fBpostfix reload\fR command after
/*	a configuration change.
/* .SH "Compatibility controls"
/* .ad
/* .fi
/* .IP \fBstrict_rfc821_envelopes\fR
/*	Disallow non-RFC 821 style addresses in envelopes. For example,
/*	allow RFC822-style address forms with comments, like Sendmail does.
/* .SH Miscellaneous
/* .ad
/* .fi
/* .IP \fBalways_bcc\fR
/*	Address to send a copy of each message that enters the system.
/* .IP \fBcommand_directory\fR
/*	Location of Postfix support commands (default:
/*	\fB$program_directory\fR).
/* .IP \fBdebug_peer_level\fR
/*	Increment in verbose logging level when a remote host matches a
/*	pattern in the \fBdebug_peer_list\fR parameter.
/* .IP \fBdebug_peer_list\fR
/*	List of domain or network patterns. When a remote host matches
/*	a pattern, increase the verbose logging level by the amount
/*	specified in the \fBdebug_peer_level\fR parameter.
/* .IP \fBerror_notice_recipient\fR
/*	Recipient of protocol/policy/resource/software error notices.
/* .IP \fBhopcount_limit\fR
/*	Limit the number of \fBReceived:\fR message headers.
/* .IP \fBnotify_classes\fR
/*	List of error classes. Of special interest are:
/* .IP \fBlocal_recipient_maps\fR
/*	List of maps with user names that are local to \fB$myorigin\fR
/*	or \fB$inet_interfaces\fR. If this parameter is defined,
/*	then the SMTP server rejects mail for unknown local users.
/* .RS
/* .IP \fBpolicy\fR
/*	When a client violates any policy, mail a transcript of the
/*	entire SMTP session to the postmaster.
/* .IP \fBprotocol\fR
/*	When a client violates the SMTP protocol or issues an unimplemented
/*	command, mail a transcript of the entire SMTP session to the
/*	postmaster.
/* .RE
/* .IP \fBsmtpd_banner\fR
/*	Text that follows the \fB220\fR status code in the SMTP greeting banner.
/* .IP \fBsmtpd_recipient_limit\fR
/*	Restrict the number of recipients that the SMTP server accepts
/*	per message delivery.
/* .IP \fBsmtpd_timeout\fR
/*	Limit the time to send a server response and to receive a client
/*	request.
/* .SH "Resource controls"
/* .ad
/* .fi
/* .IP \fBline_length_limit\fR
/*	Limit the amount of memory in bytes used for the handling of
/*	partial input lines.
/* .IP \fBmessage_size_limit\fR
/*	Limit the total size in bytes of a message, including on-disk
/*	storage for envelope information.
/* .IP \fBqueue_minfree\fR
/*	Minimal amount of free space in bytes in the queue file system
/*	for the SMTP server to accept any mail at all.
/* .SH Tarpitting
/* .ad
/* .fi
/* .IP \fBsmtpd_error_sleep_time\fR
/*	Time to wait in seconds before sending a 4xx or 5xx server error
/*	response.
/* .IP \fBsmtpd_soft_error_limit\fR
/*	When an SMTP client has made this number of errors, wait
/*	\fIerror_count\fR seconds before responding to any client request.
/* .IP \fBsmtpd_hard_error_limit\fR
/*	Disconnect after a client has made this number of errors.
/* .IP \fBsmtpd_junk_command_limit\fR
/*	Limit the number of times a client can issue a junk command
/*	such as NOOP, VRFY, ETRN or RSET in one SMTP session before
/*	it is penalized with tarpit delays.
/* .SH "UCE control restrictions"
/* .ad
/* .fi
/* .IP \fBsmtpd_client_restrictions\fR
/*	Restrict what clients may connect to this mail system.
/* .IP \fBsmtpd_helo_required\fR
/*	Require that clients introduce themselves at the beginning
/*	of an SMTP session.
/* .IP \fBsmtpd_helo_restrictions\fR
/*	Restrict what client hostnames are allowed in \fBHELO\fR and
/*	\fBEHLO\fR commands.
/* .IP \fBsmtpd_sender_restrictions\fR
/*	Restrict what sender addresses are allowed in \fBMAIL FROM\fR commands.
/* .IP \fBsmtpd_recipient_restrictions\fR
/*	Restrict what recipient addresses are allowed in \fBRCPT TO\fR commands.
/* .IP \fBsmtpd_etrn_restrictions\fR
/*	Restrict what domain names can be used in \fBETRN\fR commands,
/*	and what clients may issue \fBETRN\fR commands.
/* .IP \fBallow_untrusted_routing\fR
/*	Allow untrusted clients to specify addresses with sender-specified
/*	routing.  Enabling this opens up nasty relay loopholes involving
/*	trusted backup MX hosts.
/* .IP \fBrestriction_classes\fR
/*	Declares the name of zero or more parameters that contain a
/*	list of UCE restrictions. The names of these parameters can
/*	then be used instead of the restriction lists that they represent.
/* .IP \fBmaps_rbl_domains\fR
/*	List of DNS domains that publish the addresses of blacklisted
/*	hosts.
/* .IP \fBrelay_domains\fR
/*	Restrict what domains or networks this mail system will relay
/*	mail from or to.
/* .SH "UCE control responses"
/* .ad
/* .fi
/* .IP \fBaccess_map_reject_code\fR
/*	Server response when a client violates an access database restriction.
/* .IP \fBinvalid_hostname_reject_code\fR
/*	Server response when a client violates the \fBreject_invalid_hostname\fR
/*	restriction.
/* .IP \fBmaps_rbl_reject_code\fR
/*	Server response when a client violates the \fBmaps_rbl_domains\fR
/*	restriction.
/* .IP \fBreject_code\fR
/*	Response code when the client matches a \fBreject\fR restriction.
/* .IP \fBrelay_domains_reject_code\fR
/*	Server response when a client attempts to violate the mail relay
/*	policy.
/* .IP \fBunknown_address_reject_code\fR
/*	Server response when a client violates the \fBreject_unknown_address\fR
/*	restriction.
/* .IP \fBunknown_client_reject_code\fR
/*	Server response when a client without address to name mapping
/*	violates the \fBreject_unknown_clients\fR restriction.
/* .IP \fBunknown_hostname_reject_code\fR
/*	Server response when a client violates the \fBreject_unknown_hostname\fR
/*	restriction.
/* SEE ALSO
/*	cleanup(8) message canonicalization
/*	master(8) process manager
/*	syslogd(8) system logging
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
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>			/* remove() */
#include <setjmp.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>
#include <events.h>
#include <smtp_stream.h>
#include <valid_hostname.h>
#include <dict.h>
#include <watchdog.h>

/* Global library. */

#include <mail_params.h>
#include <record.h>
#include <rec_type.h>
#include <mail_proto.h>
#include <cleanup_user.h>
#include <mail_date.h>
#include <mail_conf.h>
#include <off_cvt.h>
#include <debug_peer.h>
#include <mail_error.h>
#include <mail_flush.h>
#include <mail_stream.h>
#include <mail_queue.h>
#include <tok822.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

/* Application-specific */

#include "smtpd_token.h"
#include "smtpd.h"
#include "smtpd_check.h"
#include "smtpd_chat.h"

 /*
  * Tunable parameters. Make sure that there is some bound on the length of
  * an SMTP command, so that the mail system stays in control even when a
  * malicious client sends commands of unreasonable length (qmail-dos-1).
  * Make sure there is some bound on the number of recipients, so that the
  * mail system stays in control even when a malicious client sends an
  * unreasonable number of recipients (qmail-dos-2).
  */
int     var_smtpd_rcpt_limit;
int     var_smtpd_tmout;
char   *var_relay_domains;
int     var_smtpd_soft_erlim;
int     var_smtpd_hard_erlim;
int     var_queue_minfree;		/* XXX use off_t */
char   *var_smtpd_banner;
char   *var_debug_peer_list;
int     var_debug_peer_level;
char   *var_notify_classes;
char   *var_client_checks;
char   *var_helo_checks;
char   *var_mail_checks;
char   *var_rcpt_checks;
char   *var_etrn_checks;
int     var_unk_client_code;
int     var_bad_name_code;
int     var_unk_name_code;
int     var_unk_addr_code;
int     var_relay_code;
int     var_maps_rbl_code;
int     var_access_map_code;
char   *var_maps_rbl_domains;
int     var_helo_required;
int     var_reject_code;
int     var_smtpd_err_sleep;
int     var_non_fqdn_code;
char   *var_always_bcc;
char   *var_error_rcpt;
int     var_smtpd_delay_reject;
char   *var_rest_classes;
int     var_strict_rfc821_env;
bool    var_disable_vrfy_cmd;
char   *var_canonical_maps;
char   *var_rcpt_canon_maps;
char   *var_virtual_maps;
char   *var_relocated_maps;
char   *var_alias_maps;
char   *var_local_rcpt_maps;
bool    var_allow_untrust_route;
int     var_smtpd_junk_cmd_limit;

 /*
  * Global state, for stand-alone mode queue file cleanup. When this is
  * non-null at cleanup time, the named file is removed.
  */
char   *smtpd_path;

 /*
  * Silly little macros.
  */
#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* collapse_args - put arguments together again */

static void collapse_args(int argc, SMTPD_TOKEN *argv)
{
    int     i;

    for (i = 1; i < argc; i++) {
	vstring_strcat(argv[0].vstrval, " ");
	vstring_strcat(argv[0].vstrval, argv[i].strval);
    }
    argv[0].strval = STR(argv[0].vstrval);
}

/* helo_cmd - process HELO command */

static int helo_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err;

    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: HELO hostname");
	return (-1);
    }
    if (state->helo_name != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 Duplicate HELO/EHLO");
	return (-1);
    }
    if (argc > 2)
	collapse_args(argc - 1, argv + 1);
    if (SMTPD_STAND_ALONE(state) == 0
	&& var_smtpd_delay_reject == 0
	&& (err = smtpd_check_helo(state, argv[1].strval)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    state->helo_name = mystrdup(printable(argv[1].strval, '?'));
    state->protocol = "SMTP";
    smtpd_chat_reply(state, "250 %s", var_myhostname);
    return (0);
}

/* ehlo_cmd - process EHLO command */

static int ehlo_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err;

    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: EHLO hostname");
	return (-1);
    }
    if (state->helo_name != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 Error: duplicate HELO/EHLO");
	return (-1);
    }
    if (argc > 2)
	collapse_args(argc - 1, argv + 1);
    if (SMTPD_STAND_ALONE(state) == 0
	&& var_smtpd_delay_reject == 0
	&& (err = smtpd_check_helo(state, argv[1].strval)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    state->helo_name = mystrdup(printable(argv[1].strval, '?'));
    state->protocol = "ESMTP";
    smtpd_chat_reply(state, "250-%s", var_myhostname);
    smtpd_chat_reply(state, "250-PIPELINING");
    if (var_message_limit)
	smtpd_chat_reply(state, "250-SIZE %lu",
			 (unsigned long) var_message_limit);	/* XXX */
    else
	smtpd_chat_reply(state, "250-SIZE");
    smtpd_chat_reply(state, "250-ETRN");
    smtpd_chat_reply(state, "250 8BITMIME");
    return (0);
}

/* helo_reset - reset HELO/EHLO command stuff */

static void helo_reset(SMTPD_STATE *state)
{
    if (state->helo_name)
	myfree(state->helo_name);
    state->helo_name = 0;
}

/* mail_open_stream - open mail destination */

static void mail_open_stream(SMTPD_STATE *state)
{
    char   *postdrop_command;

    /*
     * If running from the master or from inetd, connect to the cleanup
     * service.
     */
    if (SMTPD_STAND_ALONE(state) == 0) {
	state->dest = mail_stream_service(MAIL_CLASS_PRIVATE,
					  MAIL_SERVICE_CLEANUP);
	if (state->dest == 0
	 || mail_print(state->dest->stream, "%d", CLEANUP_FLAG_FILTER) != 0)
	    msg_fatal("unable to connect to the %s %s service",
		      MAIL_CLASS_PRIVATE, MAIL_SERVICE_CLEANUP);
    }

    /*
     * Otherwise, if the maildrop is writable, create a maildrop file.
     * Arrange for pickup service notification. Make a copy of the pathname
     * so that the file can be deleted in case of a fatal run-time error.
     */
    else if (access(MAIL_QUEUE_MAILDROP, W_OK) == 0) {
	state->dest = mail_stream_file(MAIL_QUEUE_MAILDROP,
				    MAIL_CLASS_PUBLIC, MAIL_SERVICE_PICKUP);
	smtpd_path = mystrdup(VSTREAM_PATH(state->dest->stream));
    }

    /*
     * Otherwise, pipe the message through the privileged postdrop helper.
     * XXX Make postdrop a manifest constant.
     */
    else {
	postdrop_command = concatenate(var_command_dir, "/postdrop",
			      msg_verbose ? " -v" : (char *) 0, (char *) 0);
	state->dest = mail_stream_command(postdrop_command);
	if (state->dest == 0)
	    msg_fatal("unable to execute %s", postdrop_command);
	myfree(postdrop_command);
    }
    state->cleanup = state->dest->stream;
    state->queue_id = mystrdup(state->dest->id);
}

/* extract_addr - extract address from rubble */

static char *extract_addr(SMTPD_STATE *state, SMTPD_TOKEN *arg,
			          int allow_empty_addr, int strict_rfc821)
{
    char   *myname = "extract_addr";
    TOK822 *tree;
    TOK822 *tp;
    TOK822 *addr = 0;
    int     naddr;
    int     non_addr;
    char   *err = 0;

    /*
     * Special case.
     */
#define PERMIT_EMPTY_ADDR	1
#define REJECT_EMPTY_ADDR	0

    /*
     * Some mailers send RFC822-style address forms (with comments and such)
     * in SMTP envelopes. We cannot blame users for this: the blame is with
     * programmers violating the RFC, and with sendmail for being permissive.
     * 
     * XXX The SMTP command tokenizer must leave the address in externalized
     * (quoted) form, so that the address parser can correctly extract the
     * address from surrounding junk.
     * 
     * XXX We have only one address parser, written according to the rules of
     * RFC 822. That standard differs subtly from RFC 821.
     */
    if (msg_verbose)
	msg_info("%s: input: %s", myname, STR(arg->vstrval));
    tree = tok822_parse(STR(arg->vstrval));

    /*
     * Find trouble.
     */
    for (naddr = non_addr = 0, tp = tree; tp != 0; tp = tp->next) {
	if (tp->type == TOK822_ADDR) {
	    addr = tp;
	    naddr += 1;				/* count address forms */
	} else if (tp->type == '<' || tp->type == '>') {
	     /* void */ ;			/* ignore brackets */
	} else {
	    non_addr += 1;			/* count non-address forms */
	}
    }

    /*
     * Report trouble. Log a warning only if we are going to sleep+reject so
     * that attackers can't flood our logfiles.
     */
    if ((naddr < 1 && !allow_empty_addr)
	|| naddr > 1
	|| (strict_rfc821 && (non_addr || *STR(arg->vstrval) != '<'))) {
	msg_warn("Illegal address syntax from %s in %s command: %s",
		 state->namaddr, state->where, STR(arg->vstrval));
	err = "501 Bad address syntax";
    }

    /*
     * Overwrite the input with the extracted address. This seems bad design,
     * but we really are not going to use the original data anymore. What we
     * start with is quoted (external) form, and what we need is unquoted
     * (internal form).
     */
    if (addr)
	tok822_internalize(arg->vstrval, addr->head, TOK822_STR_DEFL);
    else
	vstring_strcpy(arg->vstrval, "");
    arg->strval = STR(arg->vstrval);

    /*
     * Cleanup.
     */
    tok822_free_tree(tree);
    if (msg_verbose)
	msg_info("%s: result: %s", myname, STR(arg->vstrval));
    return (err);
}

/* mail_cmd - process MAIL command */

static int mail_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err;
    int     narg;
    off_t   size = 0;
    char   *arg;

    /*
     * Sanity checks. XXX Ignore bad SIZE= values until we can reliably and
     * portably detect overflows while converting from string to off_t.
     */
    if (var_helo_required && state->helo_name == 0) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "503 Error: send HELO/EHLO first");
	return (-1);
    }
    if (state->cleanup != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 Error: nested MAIL command");
	return (-1);
    }
    if (argc < 3
	|| strcasecmp(argv[1].strval, "from:") != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: MAIL FROM: <address>");
	return (-1);
    }
    if (argv[2].tokval == SMTPD_TOK_ERROR) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Bad address syntax");
	return (-1);
    }
    if ((err = extract_addr(state, argv + 2, PERMIT_EMPTY_ADDR, var_strict_rfc821_env)) != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    for (narg = 3; narg < argc; narg++) {
	arg = argv[narg].strval;
	if (strcasecmp(arg, "BODY=8BITMIME") == 0
	    || strcasecmp(arg, "BODY=7BIT") == 0) {
	     /* void */ ;
	} else if (strncasecmp(arg, "SIZE=", 5) == 0) {
	    if ((size = off_cvt_string(arg + 5)) < 0)
		size = 0;
	} else {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "555 Unsupported option: %s", arg);
	    return (-1);
	}
    }
    state->time = time((time_t *) 0);
    if (SMTPD_STAND_ALONE(state) == 0
	&& var_smtpd_delay_reject == 0
	&& (err = smtpd_check_mail(state, argv[2].strval)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    if ((err = smtpd_check_size(state, size)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }

    /*
     * Open queue file or IPC stream.
     */
    mail_open_stream(state);
    msg_info("%s: client=%s[%s]", state->queue_id, state->name, state->addr);

    /*
     * Record the time of arrival and the sender envelope address.
     */
    rec_fprintf(state->cleanup, REC_TYPE_TIME, "%ld",
		(long) time((time_t *) 0));
    rec_fputs(state->cleanup, REC_TYPE_FROM, argv[2].strval);
    state->sender = mystrdup(argv[2].strval);
    smtpd_chat_reply(state, "250 Ok");
    return (0);
}

/* mail_reset - reset MAIL command stuff */

static void mail_reset(SMTPD_STATE *state)
{

    /*
     * Unceremoniously close the pipe to the cleanup service. The cleanup
     * service will delete the queue file when it detects a premature
     * end-of-file condition on input.
     */
    if (state->cleanup != 0) {
	mail_stream_cleanup(state->dest);
	state->dest = 0;
	state->cleanup = 0;
    }
    state->err = 0;
    if (state->queue_id != 0) {
	myfree(state->queue_id);
	state->queue_id = 0;
    }
    if (smtpd_path) {
	if (remove(smtpd_path))
	    msg_warn("remove %s: %m", smtpd_path);
	else if (msg_verbose)
	    msg_info("remove %s", smtpd_path);
	myfree(smtpd_path);
	smtpd_path = 0;
    }
    if (state->sender) {
	myfree(state->sender);
	state->sender = 0;
    }
}

/* rcpt_cmd - process RCPT TO command */

static int rcpt_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err;
    int     narg;
    char   *arg;

    /*
     * Sanity checks.
     */
    if (state->cleanup == 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 Error: need MAIL command");
	return (-1);
    }
    if (argc < 3
	|| strcasecmp(argv[1].strval, "to:") != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: RCPT TO: <address>");
	return (-1);
    }
    if (argv[2].tokval == SMTPD_TOK_ERROR) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Bad address syntax");
	return (-1);
    }
    if ((err = extract_addr(state, argv + 2, REJECT_EMPTY_ADDR, var_strict_rfc821_env)) != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    for (narg = 3; narg < argc; narg++) {
	arg = argv[narg].strval;
	if (1) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "555 Unsupported option: %s", arg);
	    return (-1);
	}
    }
    if (var_smtpd_rcpt_limit && state->rcpt_count >= var_smtpd_rcpt_limit) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "452 Error: too many recipients");
	return (-1);
    }
    if (SMTPD_STAND_ALONE(state) == 0) {
	if ((err = smtpd_check_rcpt(state, argv[2].strval)) != 0) {
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
	if ((err = smtpd_check_rcptmap(state, argv[2].strval)) != 0) {
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
    }

    /*
     * Store the recipient. Remember the first one.
     */
    state->rcpt_count++;
    if (state->recipient == 0)
	state->recipient = mystrdup(argv[2].strval);
    rec_fputs(state->cleanup, REC_TYPE_RCPT, argv[2].strval);
    smtpd_chat_reply(state, "250 Ok");
    return (0);
}

/* rcpt_reset - reset RCPT stuff */

static void rcpt_reset(SMTPD_STATE *state)
{
    if (state->recipient) {
	myfree(state->recipient);
	state->recipient = 0;
    }
    state->rcpt_count = 0;
}

/* data_cmd - process DATA command */

static int data_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *unused_argv)
{
    char   *start;
    int     len;
    int     curr_rec_type;
    int     prev_rec_type;
    int     first = 1;

    /*
     * Sanity checks. With ESMTP command pipelining the client can send DATA
     * before all recipients are rejected, so don't report that as a protocol
     * error.
     */
    if (state->rcpt_count == 0) {
	if (state->cleanup == 0)
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 Error: need RCPT command");
	return (-1);
    }
    if (argc != 1) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: DATA");
	return (-1);
    }

    /*
     * Terminate the message envelope segment. Start the message content
     * segment, and prepend our own Received: header. If there is only one
     * recipient, list the recipient address.
     */
    if (*var_always_bcc)
	rec_fputs(state->cleanup, REC_TYPE_RCPT, var_always_bcc);
    rec_fputs(state->cleanup, REC_TYPE_MESG, "");
    rec_fprintf(state->cleanup, REC_TYPE_NORM,
		"Received: from %s (%s [%s])",
		state->helo_name ? state->helo_name : state->name,
		state->name, state->addr);
    if (state->rcpt_count == 1 && state->recipient) {
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		    "\tby %s (%s) with %s id %s",
		    var_myhostname, var_mail_name,
		    state->protocol, state->queue_id);
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		"\tfor <%s>; %s", state->recipient, mail_date(state->time));
    } else {
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		    "\tby %s (%s) with %s",
		    var_myhostname, var_mail_name, state->protocol);
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		    "\tid %s; %s", state->queue_id, mail_date(state->time));
    }
    smtpd_chat_reply(state, "354 End data with <CR><LF>.<CR><LF>");

    /*
     * Copy the message content. If the cleanup process has a problem, keep
     * reading until the remote stops sending, then complain. Read typed
     * records from the SMTP stream so we can handle data that spans buffers.
     * 
     * XXX Force an empty record when the queue file content begins with
     * whitespace, so that it won't be considered as being part of our own
     * Received: header. What an ugly Kluge.
     */
    if (vstream_fflush(state->cleanup))
	state->err = CLEANUP_STAT_WRITE;

    for (prev_rec_type = 0; /* void */ ; prev_rec_type = curr_rec_type) {
	if (smtp_get(state->buffer, state->client, var_line_limit) == '\n')
	    curr_rec_type = REC_TYPE_NORM;
	else
	    curr_rec_type = REC_TYPE_CONT;
	start = vstring_str(state->buffer);
	len = VSTRING_LEN(state->buffer);
	if (first) {
	    first = 0;
	    if (len > 0 && ISSPACE(start[0]))
		rec_put(state->cleanup, REC_TYPE_NORM, "", 0);
	}
	if (prev_rec_type != REC_TYPE_CONT
	    && *start == '.' && (++start, --len) == 0)
	    break;
	if (state->err == CLEANUP_STAT_OK
	    && rec_put(state->cleanup, curr_rec_type, start, len) < 0)
	    state->err = CLEANUP_STAT_WRITE;
    }

    /*
     * Send the end-of-segment markers.
     */
    if (state->err == CLEANUP_STAT_OK)
	if (rec_fputs(state->cleanup, REC_TYPE_XTRA, "") < 0
	    || rec_fputs(state->cleanup, REC_TYPE_END, "") < 0
	    || vstream_fflush(state->cleanup))
	    state->err = CLEANUP_STAT_WRITE;

    /*
     * Finish the queue file or finish the cleanup conversation.
     */
    if (state->err == 0)
	state->err |= mail_stream_finish(state->dest);
    else
	mail_stream_cleanup(state->dest);
    state->dest = 0;
    state->cleanup = 0;

    /*
     * Delete the queue file or disable delete on fatal error or interrupt.
     */
    if (smtpd_path) {
	if (state->err != 0) {
	    if (remove(smtpd_path))
		msg_warn("remove %s: %m", smtpd_path);
	    else if (msg_verbose)
		msg_info("remove %s", smtpd_path);
	}
	myfree(smtpd_path);
	smtpd_path = 0;
    }

    /*
     * Handle any errors. One message may suffer from multiple errors, so
     * complain only about the most severe error. Forgive any previous client
     * errors when a message was received successfully.
     */
    if (state->err == CLEANUP_STAT_OK) {
	state->error_count = 0;
	state->error_mask = 0;
	state->junk_cmds = 0;
	smtpd_chat_reply(state, "250 Ok: queued as %s", state->queue_id);
    } else if ((state->err & CLEANUP_STAT_BAD) != 0) {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	smtpd_chat_reply(state, "451 Error: internal error %d", state->err);
    } else if ((state->err & CLEANUP_STAT_SIZE) != 0) {
	state->error_mask |= MAIL_ERROR_BOUNCE;
	smtpd_chat_reply(state, "552 Error: message too large");
    } else if ((state->err & CLEANUP_STAT_HOPS) != 0) {
	state->error_mask |= MAIL_ERROR_BOUNCE;
	smtpd_chat_reply(state, "554 Error: too many hops");
    } else if ((state->err & CLEANUP_STAT_CONT) != 0) {
	state->error_mask |= MAIL_ERROR_BOUNCE;
	smtpd_chat_reply(state, "552 Error: content rejected");
    } else if ((state->err & CLEANUP_STAT_WRITE) != 0) {
	state->error_mask |= MAIL_ERROR_RESOURCE;
	smtpd_chat_reply(state, "451 Error: queue file write error");
    } else if ((state->err & CLEANUP_STAT_RCPT) != 0) {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	smtpd_chat_reply(state, "451 Error: internal error %d", state->err);
    } else {
	msg_panic("data_cmd: unknown status %d", state->err);
    }

    /*
     * Disconnect after transmission must not be treated as "lost connection
     * after DATA".
     */
    state->where = SMTPD_AFTER_DOT;

    /*
     * Cleanup. The client may send another MAIL command.
     */
    mail_reset(state);
    rcpt_reset(state);
    return (state->err);
}

/* rset_cmd - process RSET */

static int rset_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *unused_argv)
{

    /*
     * Sanity checks.
     */
    if (argc != 1) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: RSET");
	return (-1);
    }

    /*
     * Restore state to right after HELO/EHLO command.
     */
    mail_reset(state);
    rcpt_reset(state);
    smtpd_chat_reply(state, "250 Ok");
    return (0);
}

/* noop_cmd - process NOOP */

static int noop_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *unused_argv)
{

    /*
     * Sanity checks.
     */
    if (argc != 1) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: NOOP");
	return (-1);
    }
    smtpd_chat_reply(state, "250 Ok");
    return (0);
}

/* vrfy_cmd - process VRFY */

static int vrfy_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err = 0;

    /*
     * The SMTP standard (RFC 821) disallows unquoted special characters in
     * the VRFY argument. Common practice violates the standard, however.
     * Postfix accomodates common practice where it violates the standard.
     * 
     * XXX Impedance mismatch! The SMTP command tokenizer preserves quoting,
     * whereas the recipient restrictions checks expect unquoted (internal)
     * address forms. Therefore we must parse out the address, or we must
     * stop doing recipient restriction checks and lose the opportunity to
     * say "user unknown" at the SMTP port.
     */
#define SLOPPY	0

    if (var_disable_vrfy_cmd) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "502 VRFY command is disabled");
	return (-1);
    }
    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: VRFY address");
	return (-1);
    }
    if (argc > 2)
	collapse_args(argc - 1, argv + 1);
    if ((err = extract_addr(state, argv + 1, REJECT_EMPTY_ADDR, SLOPPY)) != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    if (SMTPD_STAND_ALONE(state) == 0
	&& (err = smtpd_check_rcptmap(state, argv[1].strval)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    smtpd_chat_reply(state, "252 <%s>", argv[1].strval);
    return (0);
}

/* etrn_cmd - process ETRN command */

static int etrn_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err;

    /*
     * Sanity checks.
     */
    if (var_helo_required && state->helo_name == 0) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "503 Error: send HELO/EHLO first");
	return (-1);
    }
    if (state->cleanup != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 Error: MAIL transaction in progress");
	return (-1);
    }
    if (argc != 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "500 Syntax: ETRN domain");
	return (-1);
    }
    if (!ISALNUM(argv[1].strval[0]))
	argv[1].strval++;
    if (!valid_hostname(argv[1].strval)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Error: invalid parameter syntax");
	return (-1);
    }

    /*
     * XXX The implementation borrows heavily from the code that implements
     * UCE restrictions. These typically return 450 or 550 when a request is
     * rejected. RFC 1985 requires that 459 be sent when the server refuses
     * to perform the request.
     */
    if (SMTPD_STAND_ALONE(state) == 0
	&& (err = smtpd_check_etrn(state, argv[1].strval)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }

    /*
     * XXX The preliminary implementation causes a full deferred queue scan.
     */
    if (mail_flush_site(argv[1].strval) < 0)
	smtpd_chat_reply(state, "458 Unable to queue messages");
    else
	smtpd_chat_reply(state, "250 Queuing started");
    return (0);
}

/* quit_cmd - process QUIT command */

static int quit_cmd(SMTPD_STATE *state, int unused_argc, SMTPD_TOKEN *unused_argv)
{

    /*
     * Don't bother checking the syntax.
     */
    smtpd_chat_reply(state, "221 Bye");
    return (0);
}

 /*
  * The table of all SMTP commands that we know.
  */
typedef struct SMTPD_CMD {
    char   *name;
    int     (*action) (SMTPD_STATE *, int, SMTPD_TOKEN *);
    int     flags;
} SMTPD_CMD;

#define SMTPD_CMD_FLAG_LIMIT	(1<<0)	/* limit usage */

static SMTPD_CMD smtpd_cmd_table[] = {
    "HELO", helo_cmd, 0,
    "EHLO", ehlo_cmd, 0,
    "MAIL", mail_cmd, 0,
    "RCPT", rcpt_cmd, 0,
    "DATA", data_cmd, 0,
    "RSET", rset_cmd, SMTPD_CMD_FLAG_LIMIT,
    "NOOP", noop_cmd, SMTPD_CMD_FLAG_LIMIT,
    "VRFY", vrfy_cmd, SMTPD_CMD_FLAG_LIMIT,
    "ETRN", etrn_cmd, SMTPD_CMD_FLAG_LIMIT,
    "QUIT", quit_cmd, 0,
    0,
};

/* smtpd_proto - talk the SMTP protocol */

static void smtpd_proto(SMTPD_STATE *state)
{
    int     argc;
    SMTPD_TOKEN *argv;
    SMTPD_CMD *cmdp;

    /*
     * Print a greeting banner and run the state machine. Read SMTP commands
     * one line at a time. According to the standard, a sender or recipient
     * address could contain an escaped newline. I think this is perverse,
     * and anyone depending on this is really asking for trouble.
     * 
     * In case of mail protocol trouble, the program jumps back to this place,
     * so that it can perform the necessary cleanup before talking to the
     * next client. The setjmp/longjmp primitives are like a sharp tool: use
     * with care. I would certainly recommend against the use of
     * setjmp/longjmp in programs that change privilege levels.
     * 
     * In case of file system trouble the program terminates after logging the
     * error and after informing the client. In all other cases (out of
     * memory, panic) the error is logged, and the msg_cleanup() exit handler
     * cleans up, but no attempt is made to inform the client of the nature
     * of the problem.
     */
    smtp_timeout_setup(state->client, var_smtpd_tmout);

    switch (setjmp(smtp_timeout_buf)) {

    default:
	msg_panic("smtpd_proto: unknown error reading from %s[%s]",
		  state->name, state->addr);
	break;

    case SMTP_ERR_TIME:
	state->reason = "timeout";
	smtpd_chat_reply(state, "421 Error: timeout exceeded");
	break;

    case SMTP_ERR_EOF:
	state->reason = "lost connection";
	break;

    case 0:
	for (;;) {
	    if (state->error_count > var_smtpd_hard_erlim) {
		state->reason = "too many errors";
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "421 Error: too many errors");
		break;
	    }
	    watchdog_pat();
	    smtpd_chat_query(state);
	    if ((argc = smtpd_token(vstring_str(state->buffer), &argv)) == 0) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "500 Error: bad syntax");
		state->error_count++;
		continue;
	    }
	    for (cmdp = smtpd_cmd_table; cmdp->name != 0; cmdp++)
		if (strcasecmp(argv[0].strval, cmdp->name) == 0)
		    break;
	    if (cmdp->name == 0) {
		smtpd_chat_reply(state, "502 Error: command not implemented");
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		state->error_count++;
		continue;
	    }
	    if (state->access_denied && cmdp->action != quit_cmd) {
		smtpd_chat_reply(state, "%s", state->access_denied);
		state->error_count++;
		continue;
	    }
	    state->where = cmdp->name;
	    if (cmdp->action(state, argc, argv) != 0)
		state->error_count++;
	    if ((cmdp->flags & SMTPD_CMD_FLAG_LIMIT)
		&& state->junk_cmds++ > var_smtpd_junk_cmd_limit)
		state->error_count++;

	    if (cmdp->action == quit_cmd)
		break;
	}
	break;
    }

    /*
     * Log abnormal session termination, in case postmaster notification has
     * been turned off. In the log, indicate the last recognized state before
     * things went wrong. Don't complain about clients that go away without
     * sending QUIT.
     */
    if (state->reason && state->where && strcmp(state->where, SMTPD_AFTER_DOT))
	msg_info("%s after %s from %s[%s]",
		 state->reason, state->where, state->name, state->addr);

    /*
     * Notify the postmaster if there were errors but no message was
     * collected. This usually indicates a client configuration problem, or
     * that someone is trying nasty things. Either is significant enough to
     * bother the postmaster. XXX Can't report problems when running in
     * stand-alone mode: postmaster notices require availability of the
     * cleanup service.
     */
    if (state->history != 0 && state->client != VSTREAM_IN
	&& (state->error_mask & state->notify_mask))
	smtpd_chat_notify(state);

    /*
     * Cleanup whatever information the client gave us during the SMTP
     * dialog.
     */
    helo_reset(state);
    mail_reset(state);
    rcpt_reset(state);
    smtpd_chat_reset(state);
}

/* smtpd_service - service one client */

static void smtpd_service(VSTREAM *stream, char *unused_service, char **argv)
{
    SMTPD_STATE state;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This routine runs when a client has connected to our network port, or
     * when the smtp server is run in stand-alone mode (input from pipe).
     * 
     * Look up and sanitize the peer name, then initialize some connection-
     * specific state. When the name service is hosed, hostname lookup will
     * take a while. This is why I always run a local name server on critical
     * machines.
     */
    smtpd_state_init(&state, stream);

    /*
     * See if we need to turn on verbose logging for this client.
     */
    debug_peer_check(state.name, state.addr);

    /*
     * See if we want to talk to this client at all. Then, log the connection
     * event.
     */
    if (var_smtpd_delay_reject == 0
	&& (state.access_denied = smtpd_check_client(&state)) != 0) {
	smtpd_chat_reply(&state, "%s", state.access_denied);
    } else {
	smtpd_chat_reply(&state, "220 %s", var_smtpd_banner);
	msg_info("connect from %s[%s]", state.name, state.addr);
    }

    /*
     * Provide the SMTP service.
     */
    smtpd_proto(&state);

    /*
     * After the client has gone away, clean up whatever we have set up at
     * connection time.
     */
    msg_info("disconnect from %s[%s]", state.name, state.addr);
    smtpd_state_reset(&state);
    debug_peer_restore();
}

/* smtpd_cleanup - stand-alone mode queue file cleanup */

static void smtpd_cleanup(void)
{
    char   *myname = "smtpd_cleanup";

    /*
     * This routine is called by the run-time error handler, right before
     * program exit.
     */
    if (smtpd_path) {
	if (remove(smtpd_path))
	    msg_warn("%s: remove %s: %m", myname, smtpd_path);
	else if (msg_verbose)
	    msg_info("%s: remove %s", myname, smtpd_path);
	smtpd_path = 0;
    }
}

/* smtpd_sig - signal handler */

static void smtpd_sig(int sig)
{
    smtpd_cleanup();
    exit(sig);
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    if (dict_changed()) {
	msg_info("lookup table has changed -- exiting");
	exit(0);
    }
}

/* post_jail_init - post-jail initialization */

static void post_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Set up signal handlers so that we clean up in stand-alone mode.
     */
    signal(SIGHUP, smtpd_sig);
    signal(SIGINT, smtpd_sig);
    signal(SIGQUIT, smtpd_sig);
    signal(SIGTERM, smtpd_sig);
}

/* pre_jail_init - pre-jail initialization */

static void pre_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Initialize blacklist/etc. patterns before entering the chroot jail, in
     * case they specify a filename pattern.
     */
    smtpd_check_init();
    debug_peer_init();
    msg_cleanup(smtpd_cleanup);
}

/* main - the main program */

int     main(int argc, char **argv)
{
    static CONFIG_INT_TABLE int_table[] = {
	VAR_SMTPD_RCPT_LIMIT, DEF_SMTPD_RCPT_LIMIT, &var_smtpd_rcpt_limit, 1, 0,
	VAR_SMTPD_TMOUT, DEF_SMTPD_TMOUT, &var_smtpd_tmout, 1, 0,
	VAR_SMTPD_SOFT_ERLIM, DEF_SMTPD_SOFT_ERLIM, &var_smtpd_soft_erlim, 1, 0,
	VAR_SMTPD_HARD_ERLIM, DEF_SMTPD_HARD_ERLIM, &var_smtpd_hard_erlim, 1, 0,
	VAR_QUEUE_MINFREE, DEF_QUEUE_MINFREE, &var_queue_minfree, 0, 0,
	VAR_DEBUG_PEER_LEVEL, DEF_DEBUG_PEER_LEVEL, &var_debug_peer_level, 1, 0,
	VAR_UNK_CLIENT_CODE, DEF_UNK_CLIENT_CODE, &var_unk_client_code, 0, 0,
	VAR_BAD_NAME_CODE, DEF_BAD_NAME_CODE, &var_bad_name_code, 0, 0,
	VAR_UNK_NAME_CODE, DEF_UNK_NAME_CODE, &var_unk_name_code, 0, 0,
	VAR_UNK_ADDR_CODE, DEF_UNK_ADDR_CODE, &var_unk_addr_code, 0, 0,
	VAR_RELAY_CODE, DEF_RELAY_CODE, &var_relay_code, 0, 0,
	VAR_MAPS_RBL_CODE, DEF_MAPS_RBL_CODE, &var_maps_rbl_code, 0, 0,
	VAR_ACCESS_MAP_CODE, DEF_ACCESS_MAP_CODE, &var_access_map_code, 0, 0,
	VAR_REJECT_CODE, DEF_REJECT_CODE, &var_reject_code, 0, 0,
	VAR_SMTPD_ERR_SLEEP, DEF_SMTPD_ERR_SLEEP, &var_smtpd_err_sleep, 0, 0,
	VAR_NON_FQDN_CODE, DEF_NON_FQDN_CODE, &var_non_fqdn_code, 0, 0,
	VAR_SMTPD_JUNK_CMD, DEF_SMTPD_JUNK_CMD, &var_smtpd_junk_cmd_limit, 1, 0,
	0,
    };
    static CONFIG_BOOL_TABLE bool_table[] = {
	VAR_HELO_REQUIRED, DEF_HELO_REQUIRED, &var_helo_required,
	VAR_SMTPD_DELAY_REJECT, DEF_SMTPD_DELAY_REJECT, &var_smtpd_delay_reject,
	VAR_STRICT_RFC821_ENV, DEF_STRICT_RFC821_ENV, &var_strict_rfc821_env,
	VAR_DISABLE_VRFY_CMD, DEF_DISABLE_VRFY_CMD, &var_disable_vrfy_cmd,
	VAR_ALLOW_UNTRUST_ROUTE, DEF_ALLOW_UNTRUST_ROUTE, &var_allow_untrust_route,
	0,
    };
    static CONFIG_STR_TABLE str_table[] = {
	VAR_RELAY_DOMAINS, DEF_RELAY_DOMAINS, &var_relay_domains, 0, 0,
	VAR_SMTPD_BANNER, DEF_SMTPD_BANNER, &var_smtpd_banner, 1, 0,
	VAR_DEBUG_PEER_LIST, DEF_DEBUG_PEER_LIST, &var_debug_peer_list, 0, 0,
	VAR_NOTIFY_CLASSES, DEF_NOTIFY_CLASSES, &var_notify_classes, 0, 0,
	VAR_CLIENT_CHECKS, DEF_CLIENT_CHECKS, &var_client_checks, 0, 0,
	VAR_HELO_CHECKS, DEF_HELO_CHECKS, &var_helo_checks, 0, 0,
	VAR_MAIL_CHECKS, DEF_MAIL_CHECKS, &var_mail_checks, 0, 0,
	VAR_RCPT_CHECKS, DEF_RCPT_CHECKS, &var_rcpt_checks, 0, 0,
	VAR_ETRN_CHECKS, DEF_ETRN_CHECKS, &var_etrn_checks, 0, 0,
	VAR_MAPS_RBL_DOMAINS, DEF_MAPS_RBL_DOMAINS, &var_maps_rbl_domains, 0, 0,
	VAR_ALWAYS_BCC, DEF_ALWAYS_BCC, &var_always_bcc, 0, 0,
	VAR_ERROR_RCPT, DEF_ERROR_RCPT, &var_error_rcpt, 1, 0,
	VAR_REST_CLASSES, DEF_REST_CLASSES, &var_rest_classes, 0, 0,
	VAR_CANONICAL_MAPS, DEF_CANONICAL_MAPS, &var_canonical_maps, 0, 0,
	VAR_RCPT_CANON_MAPS, DEF_RCPT_CANON_MAPS, &var_rcpt_canon_maps, 0, 0,
	VAR_VIRTUAL_MAPS, DEF_VIRTUAL_MAPS, &var_virtual_maps, 0, 0,
	VAR_RELOCATED_MAPS, DEF_RELOCATED_MAPS, &var_relocated_maps, 0, 0,
	VAR_ALIAS_MAPS, DEF_ALIAS_MAPS, &var_alias_maps, 0, 0,
	VAR_LOCAL_RCPT_MAPS, DEF_LOCAL_RCPT_MAPS, &var_local_rcpt_maps, 0, 0,
	0,
    };

    /*
     * Pass control to the single-threaded service skeleton.
     */
    single_server_main(argc, argv, smtpd_service,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_BOOL_TABLE, bool_table,
		       MAIL_SERVER_PRE_INIT, pre_jail_init,
		       MAIL_SERVER_POST_INIT, post_jail_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       0);
}
