/*	$NetBSD: mail_params.h,v 1.14 2007/05/19 17:49:48 heas Exp $	*/

#ifndef _MAIL_PARAMS_H_INCLUDED_
#define _MAIL_PARAMS_H_INCLUDED_

/*++
/* NAME
/*	mail_params 3h
/* SUMMARY
/*	globally configurable parameters
/* SYNOPSIS
/*	#include <mail_params.h>
/* DESCRIPTION
/* .nf

 /*
  * This is to make it easier to auto-generate tables.
  */
typedef int bool;

 /*
  * Name used when this mail system announces itself.
  */
#define VAR_MAIL_NAME		"mail_name"
#define DEF_MAIL_NAME		"Postfix"
extern char *var_mail_name;

 /*
  * You want to be helped or not.
  */
#define VAR_HELPFUL_WARNINGS	"helpful_warnings"
#define DEF_HELPFUL_WARNINGS	1
extern bool var_helpful_warnings;

 /*
  * You want to be helped or not.
  */
#define VAR_SHOW_UNK_RCPT_TABLE	"show_user_unknown_table_name"
#define DEF_SHOW_UNK_RCPT_TABLE	1
extern bool var_show_unk_rcpt_table;

 /*
  * What problem classes should be reported to the postmaster via email.
  * Default is bad problems only. See mail_error(3). Even when mail notices
  * are disabled, problems are still logged to the syslog daemon.
  */
#define VAR_NOTIFY_CLASSES	"notify_classes"
#define DEF_NOTIFY_CLASSES	"resource, software"
extern char *var_notify_classes;

 /*
  * What do I turn <> into? Sendmail defaults to mailer-daemon.
  */
#define VAR_EMPTY_ADDR         "empty_address_recipient"
#define DEF_EMPTY_ADDR         MAIL_ADDR_MAIL_DAEMON
extern char *var_empty_addr;

 /*
  * Privileges used by the mail system: the owner of files and commands, and
  * the rights to be used when running external commands.
  */
#define VAR_MAIL_OWNER		"mail_owner"
#define DEF_MAIL_OWNER		"postfix"
extern char *var_mail_owner;
extern uid_t var_owner_uid;
extern gid_t var_owner_gid;

#define VAR_SGID_GROUP		"setgid_group"
#define DEF_SGID_GROUP		"maildrop"
extern char *var_sgid_group;
extern gid_t var_sgid_gid;

#define VAR_DEFAULT_PRIVS	"default_privs"
#define DEF_DEFAULT_PRIVS	"nobody"
extern char *var_default_privs;
extern uid_t var_default_uid;
extern gid_t var_default_gid;

 /*
  * Access control for local privileged operations:
  */
#define STATIC_ANYONE_ACL	"static:anyone"

#define VAR_FLUSH_ACL		"authorized_flush_users"
#define DEF_FLUSH_ACL		STATIC_ANYONE_ACL
extern char *var_flush_acl;

#define VAR_SHOWQ_ACL		"authorized_mailq_users"
#define DEF_SHOWQ_ACL		STATIC_ANYONE_ACL
extern char *var_showq_acl;

#define VAR_SUBMIT_ACL		"authorized_submit_users"
#define DEF_SUBMIT_ACL		STATIC_ANYONE_ACL
extern char *var_submit_acl;

 /*
  * What goes on the right-hand side of addresses of mail sent from this
  * machine.
  */
#define VAR_MYORIGIN		"myorigin"
#define DEF_MYORIGIN		"$myhostname"
extern char *var_myorigin;

 /*
  * What domains I will receive mail for. Not to be confused with transit
  * mail to other destinations.
  */
#define VAR_MYDEST		"mydestination"
#define DEF_MYDEST		"$myhostname, localhost.$mydomain, localhost"
extern char *var_mydest;

 /*
  * These are by default taken from the name service.
  */
#define VAR_MYHOSTNAME		"myhostname"	/* my hostname (fqdn) */
extern char *var_myhostname;

#define VAR_MYDOMAIN		"mydomain"	/* my domain name */
#define DEF_MYDOMAIN		"localdomain"
extern char *var_mydomain;

 /*
  * The default local delivery transport.
  */
#define VAR_LOCAL_TRANSPORT	"local_transport"
#define DEF_LOCAL_TRANSPORT	MAIL_SERVICE_LOCAL ":$myhostname"
extern char *var_local_transport;

 /*
  * Where to send postmaster copies of bounced mail, and other notices.
  */
#define VAR_BOUNCE_RCPT		"bounce_notice_recipient"
#define DEF_BOUNCE_RCPT		"postmaster"
extern char *var_bounce_rcpt;

#define VAR_2BOUNCE_RCPT	"2bounce_notice_recipient"
#define DEF_2BOUNCE_RCPT	"postmaster"
extern char *var_2bounce_rcpt;

#define VAR_DELAY_RCPT		"delay_notice_recipient"
#define DEF_DELAY_RCPT		"postmaster"
extern char *var_delay_rcpt;

#define VAR_ERROR_RCPT		"error_notice_recipient"
#define DEF_ERROR_RCPT		"postmaster"
extern char *var_error_rcpt;

 /*
  * Virtual host support. Default is to listen on all machine interfaces.
  */
#define VAR_INET_INTERFACES	"inet_interfaces"	/* listen addresses */
#define INET_INTERFACES_ALL	"all"
#define INET_INTERFACES_LOCAL	"loopback-only"
#define DEF_INET_INTERFACES	INET_INTERFACES_ALL
extern char *var_inet_interfaces;

#define VAR_PROXY_INTERFACES	"proxy_interfaces"	/* proxies, NATs */
#define DEF_PROXY_INTERFACES	""
extern char *var_proxy_interfaces;

 /*
  * Masquerading (i.e. subdomain stripping).
  */
#define VAR_MASQ_DOMAINS	"masquerade_domains"
#define DEF_MASQ_DOMAINS	""
extern char *var_masq_domains;

#define VAR_MASQ_EXCEPTIONS	"masquerade_exceptions"
#define DEF_MASQ_EXCEPTIONS	""
extern char *var_masq_exceptions;

#define MASQ_CLASS_ENV_FROM	"envelope_sender"
#define MASQ_CLASS_ENV_RCPT	"envelope_recipient"
#define MASQ_CLASS_HDR_FROM	"header_sender"
#define MASQ_CLASS_HDR_RCPT	"header_recipient"

#define VAR_MASQ_CLASSES	"masquerade_classes"
#define DEF_MASQ_CLASSES	MASQ_CLASS_ENV_FROM ", " \
				MASQ_CLASS_HDR_FROM ", " \
				MASQ_CLASS_HDR_RCPT
extern char *var_masq_classes;

 /*
  * Intranet versus internet.
  */
#define VAR_RELAYHOST		"relayhost"
#define DEF_RELAYHOST		""
extern char *var_relayhost;

#define VAR_SND_RELAY_MAPS	"sender_dependent_relayhost_maps"
#define DEF_SND_RELAY_MAPS	""
extern char *var_snd_relay_maps;

#define VAR_SMTP_FALLBACK	"smtp_fallback_relay"
#define DEF_SMTP_FALLBACK	"$fallback_relay"
#define VAR_LMTP_FALLBACK	"smtp_fallback_relay"
#define DEF_FALLBACK_RELAY	""
extern char *var_fallback_relay;

#define VAR_DISABLE_DNS		"disable_dns_lookups"
#define DEF_DISABLE_DNS		0
extern bool var_disable_dns;

#define SMTP_HOST_LOOKUP_DNS	"dns"
#define SMTP_HOST_LOOKUP_NATIVE	"native"

#define VAR_SMTP_HOST_LOOKUP	"smtp_host_lookup"
#define DEF_SMTP_HOST_LOOKUP	SMTP_HOST_LOOKUP_DNS
#define VAR_LMTP_HOST_LOOKUP	"lmtp_host_lookup"
#define DEF_LMTP_HOST_LOOKUP	SMTP_HOST_LOOKUP_DNS
extern int var_smtp_dns_lookup;

#define VAR_SMTP_MXADDR_LIMIT	"smtp_mx_address_limit"
#define DEF_SMTP_MXADDR_LIMIT	5
#define VAR_LMTP_MXADDR_LIMIT	"lmtp_mx_address_limit"
#define DEF_LMTP_MXADDR_LIMIT	5
extern int var_smtp_mxaddr_limit;

#define VAR_SMTP_MXSESS_LIMIT	"smtp_mx_session_limit"
#define DEF_SMTP_MXSESS_LIMIT	2
#define VAR_LMTP_MXSESS_LIMIT	"lmtp_mx_session_limit"
#define DEF_LMTP_MXSESS_LIMIT	2
extern int var_smtp_mxsess_limit;

 /*
  * Location of the mail queue directory tree.
  */
#define VAR_QUEUE_DIR	"queue_directory"
#ifndef DEF_QUEUE_DIR
#define DEF_QUEUE_DIR	"/var/spool/postfix"
#endif
extern char *var_queue_dir;

 /*
  * Location of command and daemon programs.
  */
#define VAR_DAEMON_DIR		"daemon_directory"
#ifndef DEF_DAEMON_DIR
#define DEF_DAEMON_DIR		"/usr/libexec/postfix"
#endif
extern char *var_daemon_dir;

#define VAR_COMMAND_DIR		"command_directory"
#ifndef DEF_COMMAND_DIR
#define DEF_COMMAND_DIR		"/usr/sbin"
#endif
extern char *var_command_dir;

 /*
  * Location of PID files.
  */
#define VAR_PID_DIR		"process_id_directory"
#ifndef DEF_PID_DIR
#define DEF_PID_DIR		"pid"
#endif
extern char *var_pid_dir;

 /*
  * Program startup time.
  */
extern time_t var_starttime;

 /*
  * Location of configuration files.
  */
#define VAR_CONFIG_DIR		"config_directory"
#ifndef DEF_CONFIG_DIR
#define DEF_CONFIG_DIR		"/etc/postfix"
#endif
extern char *var_config_dir;

#define VAR_CONFIG_DIRS		"alternate_config_directories"
#define DEF_CONFIG_DIRS		""
extern char *var_config_dirs;

#define MAIN_CONF_FILE		"main.cf"
#define MASTER_CONF_FILE	"master.cf"

 /*
  * Preferred type of indexed files. The DEF_DB_TYPE macro value is system
  * dependent. It is defined in <sys_defs.h>.
  */
#define VAR_DB_TYPE		"default_database_type"
extern char *var_db_type;

 /*
  * What syslog facility to use. Unfortunately, something may have to be
  * logged before parameters are read from the main.cf file. This logging
  * will go the LOG_FACILITY facility specified below.
  */
#define VAR_SYSLOG_FACILITY	"syslog_facility"
extern char *var_syslog_facility;

#ifndef DEF_SYSLOG_FACILITY
#define DEF_SYSLOG_FACILITY	"mail"
#endif

#ifndef LOG_FACILITY
#define LOG_FACILITY	LOG_MAIL
#endif

 /*
  * Big brother: who receives a blank-carbon copy of all mail that enters
  * this mail system.
  */
#define VAR_ALWAYS_BCC		"always_bcc"
#define DEF_ALWAYS_BCC		""
extern char *var_always_bcc;

 /*
  * What to put in the To: header when no recipients were disclosed.
  * 
  * XXX 2822: When no recipient headers remain, a system should insert a Bcc:
  * header without additional information. That is not so great given that
  * MTAs routinely strip Bcc: headers from message headers.
  */
#define VAR_RCPT_WITHELD	"undisclosed_recipients_header"
#define DEF_RCPT_WITHELD	"To: undisclosed-recipients:;"
extern char *var_rcpt_witheld;

 /*
  * Standards violation: allow/permit RFC 822-style addresses in SMTP
  * commands.
  */
#define VAR_STRICT_RFC821_ENV	"strict_rfc821_envelopes"
#define DEF_STRICT_RFC821_ENV	0
extern bool var_strict_rfc821_env;

 /*
  * Standards violation: send "250 AUTH=list" in order to accomodate clients
  * that implement an old version of the protocol.
  */
#define VAR_BROKEN_AUTH_CLNTS	"broken_sasl_auth_clients"
#define DEF_BROKEN_AUTH_CLNTS	0
extern bool var_broken_auth_clients;

 /*
  * Standards violation: disable VRFY.
  */
#define VAR_DISABLE_VRFY_CMD	"disable_vrfy_command"
#define DEF_DISABLE_VRFY_CMD	0
extern bool var_disable_vrfy_cmd;

 /*
  * trivial rewrite/resolve service: mapping tables.
  */
#define VAR_VIRT_ALIAS_MAPS	"virtual_alias_maps"
#define DEF_VIRT_ALIAS_MAPS	"$virtual_maps"	/* Compatibility! */
extern char *var_virt_alias_maps;

#define VAR_VIRT_ALIAS_DOMS	"virtual_alias_domains"
#define DEF_VIRT_ALIAS_DOMS	"$virtual_alias_maps"
extern char *var_virt_alias_doms;

#define VAR_VIRT_ALIAS_CODE	"unknown_virtual_alias_reject_code"
#define DEF_VIRT_ALIAS_CODE	550
extern int var_virt_alias_code;

#define VAR_CANONICAL_MAPS	"canonical_maps"
#define DEF_CANONICAL_MAPS	""
extern char *var_canonical_maps;

#define VAR_SEND_CANON_MAPS	"sender_canonical_maps"
#define DEF_SEND_CANON_MAPS	""
extern char *var_send_canon_maps;

#define VAR_RCPT_CANON_MAPS	"recipient_canonical_maps"
#define DEF_RCPT_CANON_MAPS	""
extern char *var_rcpt_canon_maps;

#define CANON_CLASS_ENV_FROM	"envelope_sender"
#define CANON_CLASS_ENV_RCPT	"envelope_recipient"
#define CANON_CLASS_HDR_FROM	"header_sender"
#define CANON_CLASS_HDR_RCPT	"header_recipient"

#define VAR_CANON_CLASSES	"canonical_classes"
#define DEF_CANON_CLASSES	CANON_CLASS_ENV_FROM ", " \
				CANON_CLASS_ENV_RCPT ", " \
				CANON_CLASS_HDR_FROM ", " \
				CANON_CLASS_HDR_RCPT
extern char *var_canon_classes;

#define VAR_SEND_CANON_CLASSES	"sender_canonical_classes"
#define DEF_SEND_CANON_CLASSES	CANON_CLASS_ENV_FROM ", " \
				CANON_CLASS_HDR_FROM
extern char *var_send_canon_classes;

#define VAR_RCPT_CANON_CLASSES	"recipient_canonical_classes"
#define DEF_RCPT_CANON_CLASSES	CANON_CLASS_ENV_RCPT ", " \
				CANON_CLASS_HDR_RCPT
extern char *var_rcpt_canon_classes;

#define VAR_SEND_BCC_MAPS	"sender_bcc_maps"
#define DEF_SEND_BCC_MAPS	""
extern char *var_send_bcc_maps;

#define VAR_RCPT_BCC_MAPS	"recipient_bcc_maps"
#define DEF_RCPT_BCC_MAPS	""
extern char *var_rcpt_bcc_maps;

#define VAR_TRANSPORT_MAPS	"transport_maps"
#define DEF_TRANSPORT_MAPS	""
extern char *var_transport_maps;

#define VAR_DEF_TRANSPORT	"default_transport"
#define DEF_DEF_TRANSPORT	MAIL_SERVICE_SMTP
extern char *var_def_transport;

 /*
  * trivial rewrite/resolve service: rewriting controls.
  */
#define VAR_SWAP_BANGPATH	"swap_bangpath"
#define DEF_SWAP_BANGPATH	1
extern bool var_swap_bangpath;

#define VAR_APP_AT_MYORIGIN	"append_at_myorigin"
#define DEF_APP_AT_MYORIGIN	1
extern bool var_append_at_myorigin;

#define VAR_APP_DOT_MYDOMAIN	"append_dot_mydomain"
#define DEF_APP_DOT_MYDOMAIN	1
extern bool var_append_dot_mydomain;

#define VAR_PERCENT_HACK	"allow_percent_hack"
#define DEF_PERCENT_HACK	1
extern bool var_percent_hack;

 /*
  * Local delivery: alias databases.
  */
#define VAR_ALIAS_MAPS		"alias_maps"
#ifdef HAS_NIS
#define DEF_ALIAS_MAPS		ALIAS_DB_MAP ", nis:mail.aliases"
#else
#define DEF_ALIAS_MAPS		ALIAS_DB_MAP
#endif
extern char *var_alias_maps;

 /*
  * Local delivery: to BIFF or not to BIFF.
  */
#define VAR_BIFF		"biff"
#define DEF_BIFF		1
extern bool var_biff;

 /*
  * Local delivery: mail to files/commands.
  */
#define VAR_ALLOW_COMMANDS	"allow_mail_to_commands"
#define DEF_ALLOW_COMMANDS	"alias, forward"
extern char *var_allow_commands;

#define VAR_COMMAND_MAXTIME	"command_time_limit"
#define DEF_COMMAND_MAXTIME	"1000s"
extern int var_command_maxtime;

#define VAR_ALLOW_FILES		"allow_mail_to_files"
#define DEF_ALLOW_FILES		"alias, forward"
extern char *var_allow_files;

#define VAR_LOCAL_CMD_SHELL	"local_command_shell"
#define DEF_LOCAL_CMD_SHELL	""
extern char *var_local_cmd_shell;

#define VAR_ALIAS_DB_MAP	"alias_database"
#define DEF_ALIAS_DB_MAP	ALIAS_DB_MAP	/* sys_defs.h */
extern char *var_alias_db_map;

#define VAR_LUSER_RELAY		"luser_relay"
#define DEF_LUSER_RELAY		""
extern char *var_luser_relay;

 /*
  * Local delivery: mailbox delivery.
  */
#define VAR_MAIL_SPOOL_DIR	"mail_spool_directory"
#define DEF_MAIL_SPOOL_DIR	_PATH_MAILDIR
extern char *var_mail_spool_dir;

#define VAR_HOME_MAILBOX	"home_mailbox"
#define DEF_HOME_MAILBOX	""
extern char *var_home_mailbox;

#define VAR_MAILBOX_COMMAND	"mailbox_command"
#define DEF_MAILBOX_COMMAND	""
extern char *var_mailbox_command;

#define VAR_MAILBOX_CMD_MAPS	"mailbox_command_maps"
#define DEF_MAILBOX_CMD_MAPS	""
extern char *var_mailbox_cmd_maps;

#define VAR_MAILBOX_TRANSP	"mailbox_transport"
#define DEF_MAILBOX_TRANSP	""
extern char *var_mailbox_transport;

#define VAR_MBOX_TRANSP_MAPS	"mailbox_transport_maps"
#define DEF_MBOX_TRANSP_MAPS	""
extern char *var_mbox_transp_maps;

#define VAR_FALLBACK_TRANSP	"fallback_transport"
#define DEF_FALLBACK_TRANSP	""
extern char *var_fallback_transport;

#define VAR_FBCK_TRANSP_MAPS	"fallback_transport_maps"
#define DEF_FBCK_TRANSP_MAPS	""
extern char *var_fbck_transp_maps;

 /*
  * Local delivery: path to per-user forwarding file.
  */
#define VAR_FORWARD_PATH	"forward_path"
#define DEF_FORWARD_PATH	"$home/.forward${recipient_delimiter}${extension}, $home/.forward"
extern char *var_forward_path;

 /*
  * Local delivery: external command execution directory.
  */
#define VAR_EXEC_DIRECTORY	"command_execution_directory"
#define DEF_EXEC_DIRECTORY	""
extern char *var_exec_directory;

#define VAR_EXEC_EXP_FILTER	"execution_directory_expansion_filter"
#define DEF_EXEC_EXP_FILTER	"1234567890!@%-_=+:,./\
abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ"
extern char *var_exec_exp_filter;

 /*
  * Mailbox locking. DEF_MAILBOX_LOCK is defined in sys_defs.h.
  */
#define VAR_MAILBOX_LOCK	"mailbox_delivery_lock"
extern char *var_mailbox_lock;

 /*
  * Mailbox size limit. This used to be enforced as a side effect of the way
  * the message size limit is implemented, but that is not clean.
  */
#define VAR_MAILBOX_LIMIT	"mailbox_size_limit"
#define DEF_MAILBOX_LIMIT	(DEF_MESSAGE_LIMIT * 5)
extern int var_mailbox_limit;

 /*
  * Miscellaneous.
  */
#define VAR_PROP_EXTENSION	"propagate_unmatched_extensions"
#define DEF_PROP_EXTENSION	"canonical, virtual"
extern char *var_prop_extension;

#define VAR_RCPT_DELIM		"recipient_delimiter"
#define DEF_RCPT_DELIM		""
extern char *var_rcpt_delim;

#define VAR_CMD_EXP_FILTER	"command_expansion_filter"
#define DEF_CMD_EXP_FILTER	"1234567890!@%-_=+:,./\
abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ"
extern char *var_cmd_exp_filter;

#define VAR_FWD_EXP_FILTER	"forward_expansion_filter"
#define DEF_FWD_EXP_FILTER	"1234567890!@%-_=+:,./\
abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ"
extern char *var_fwd_exp_filter;

#define VAR_DELIVER_HDR		"prepend_delivered_header"
#define DEF_DELIVER_HDR		"command, file, forward"
extern char *var_deliver_hdr;

 /*
  * Cleanup: enable support for X-Original-To message headers, which are
  * needed for multi-recipient mailboxes. When this is turned on, perform
  * duplicate elimination on (original rcpt, rewritten rcpt) pairs, and
  * generating non-empty original recipient records in the queue file.
  */
#define VAR_ENABLE_ORCPT	"enable_original_recipient"
#define DEF_ENABLE_ORCPT	1
extern bool var_enable_orcpt;

#define VAR_EXP_OWN_ALIAS	"expand_owner_alias"
#define DEF_EXP_OWN_ALIAS	0
extern bool var_exp_own_alias;

#define VAR_STAT_HOME_DIR	"require_home_directory"
#define DEF_STAT_HOME_DIR	0
extern bool var_stat_home_dir;

 /*
  * Queue manager: maximal size of the duplicate expansion filter. By
  * default, we do graceful degradation with huge mailing lists.
  */
#define VAR_DUP_FILTER_LIMIT	"duplicate_filter_limit"
#define DEF_DUP_FILTER_LIMIT	1000
extern int var_dup_filter_limit;

#define VAR_TLS_RAND_EXCH_NAME	"tls_random_exchange_name"
#define DEF_TLS_RAND_EXCH_NAME	"${config_directory}/prng_exch"
extern char *var_tls_rand_exch_name;

#define VAR_TLS_RAND_SOURCE	"tls_random_source"
#ifdef HAS_DEV_URANDOM
#define DEF_TLS_RAND_SOURCE	"dev:/dev/urandom"
#else
#define DEF_TLS_RAND_SOURCE	""
#endif
extern char *var_tls_rand_source;

#define VAR_TLS_RAND_BYTES	"tls_random_bytes"
#define DEF_TLS_RAND_BYTES	32
extern int var_tls_rand_bytes;

#define VAR_TLS_DAEMON_RAND_BYTES	"tls_daemon_random_bytes"
#define DEF_TLS_DAEMON_RAND_BYTES	32
extern int var_tls_daemon_rand_bytes;

#define VAR_TLS_RESEED_PERIOD	"tls_random_reseed_period"
#define DEF_TLS_RESEED_PERIOD	"3600s"
extern int var_tls_reseed_period;

#define VAR_TLS_PRNG_UPD_PERIOD	"tls_random_prng_update_period"
#define DEF_TLS_PRNG_UPD_PERIOD "3600s"
extern int var_tls_prng_upd_period;

 /*
  * Queue manager: relocated databases.
  */
#define VAR_RELOCATED_MAPS		"relocated_maps"
#define DEF_RELOCATED_MAPS		""
extern char *var_relocated_maps;

 /*
  * Queue manager: after each failed attempt the backoff time (how long we
  * won't try this host in seconds) is doubled until it reaches the maximum.
  * MAX_QUEUE_TIME limits the amount of time a message may spend in the mail
  * queue before it is sent back.
  */
#define VAR_QUEUE_RUN_DELAY	"queue_run_delay"
#define DEF_QUEUE_RUN_DELAY     "300s"

#define VAR_MIN_BACKOFF_TIME	"minimal_backoff_time"
#define DEF_MIN_BACKOFF_TIME    DEF_QUEUE_RUN_DELAY
extern int var_min_backoff_time;

#define VAR_MAX_BACKOFF_TIME	"maximal_backoff_time"
#define DEF_MAX_BACKOFF_TIME    "4000s"
extern int var_max_backoff_time;

#define VAR_MAX_QUEUE_TIME	"maximal_queue_lifetime"
#define DEF_MAX_QUEUE_TIME	"5d"
extern int var_max_queue_time;

 /*
  * XXX The default can't be $maximal_queue_lifetime, because that panics
  * when a non-default maximal_queue_lifetime setting contains no time unit.
  */
#define VAR_DSN_QUEUE_TIME	"bounce_queue_lifetime"
#define DEF_DSN_QUEUE_TIME	"5d"
extern int var_dsn_queue_time;

#define VAR_DELAY_WARN_TIME	"delay_warning_time"
#define DEF_DELAY_WARN_TIME	"0h"
extern int var_delay_warn_time;

 /*
  * Queue manager: various in-core message and recipient limits.
  */
#define VAR_QMGR_ACT_LIMIT	"qmgr_message_active_limit"
#define DEF_QMGR_ACT_LIMIT	20000
extern int var_qmgr_active_limit;

#define VAR_QMGR_RCPT_LIMIT	"qmgr_message_recipient_limit"
#define DEF_QMGR_RCPT_LIMIT	20000
extern int var_qmgr_rcpt_limit;

#define VAR_QMGR_MSG_RCPT_LIMIT	"qmgr_message_recipient_minimum"
#define DEF_QMGR_MSG_RCPT_LIMIT	10
extern int var_qmgr_msg_rcpt_limit;

#define VAR_XPORT_RCPT_LIMIT	"default_recipient_limit"
#define _XPORT_RCPT_LIMIT	"_recipient_limit"
#define DEF_XPORT_RCPT_LIMIT	20000
extern int var_xport_rcpt_limit;

#define VAR_STACK_RCPT_LIMIT	"default_extra_recipient_limit"
#define _STACK_RCPT_LIMIT	"_extra_recipient_limit"
#define DEF_STACK_RCPT_LIMIT	1000
extern int var_stack_rcpt_limit;

#define VAR_XPORT_REFILL_LIMIT	"default_recipient_refill_limit"
#define _XPORT_REFILL_LIMIT	"_recipient_refill_limit"
#define DEF_XPORT_REFILL_LIMIT	100
extern int var_xport_refill_limit;

#define VAR_XPORT_REFILL_DELAY	"default_recipient_refill_delay"
#define _XPORT_REFILL_DELAY	"_recipient_refill_delay"
#define DEF_XPORT_REFILL_DELAY	"5s"
extern int var_xport_refill_delay;

 /*
  * Queue manager: default job scheduler parameters.
  */
#define VAR_DELIVERY_SLOT_COST	"default_delivery_slot_cost"
#define _DELIVERY_SLOT_COST	"_delivery_slot_cost"
#define DEF_DELIVERY_SLOT_COST	5
extern int var_delivery_slot_cost;

#define VAR_DELIVERY_SLOT_LOAN	"default_delivery_slot_loan"
#define _DELIVERY_SLOT_LOAN	"_delivery_slot_loan"
#define DEF_DELIVERY_SLOT_LOAN	3
extern int var_delivery_slot_loan;

#define VAR_DELIVERY_SLOT_DISCOUNT	"default_delivery_slot_discount"
#define _DELIVERY_SLOT_DISCOUNT	"_delivery_slot_discount"
#define DEF_DELIVERY_SLOT_DISCOUNT	50
extern int var_delivery_slot_discount;

#define VAR_MIN_DELIVERY_SLOTS	"default_minimum_delivery_slots"
#define _MIN_DELIVERY_SLOTS	"_minimum_delivery_slots"
#define DEF_MIN_DELIVERY_SLOTS	3
extern int var_min_delivery_slots;

#define VAR_QMGR_FUDGE		"qmgr_fudge_factor"
#define DEF_QMGR_FUDGE		100
extern int var_qmgr_fudge;

 /*
  * Queue manager: default destination concurrency levels.
  */
#define VAR_INIT_DEST_CON	"initial_destination_concurrency"
#define DEF_INIT_DEST_CON	5
extern int var_init_dest_concurrency;

#define VAR_DEST_CON_LIMIT	"default_destination_concurrency_limit"
#define _DEST_CON_LIMIT		"_destination_concurrency_limit"
#define DEF_DEST_CON_LIMIT	20
extern int var_dest_con_limit;

#define VAR_LOCAL_CON_LIMIT	"local" _DEST_CON_LIMIT
#define DEF_LOCAL_CON_LIMIT	2
extern int var_local_con_lim;

 /*
  * Queue manager: default number of recipients per transaction.
  */
#define VAR_DEST_RCPT_LIMIT	"default_destination_recipient_limit"
#define _DEST_RCPT_LIMIT	"_destination_recipient_limit"
#define DEF_DEST_RCPT_LIMIT	50
extern int var_dest_rcpt_limit;

#define VAR_LOCAL_RCPT_LIMIT	"local" _DEST_RCPT_LIMIT	/* XXX */
#define DEF_LOCAL_RCPT_LIMIT	1	/* XXX */
extern int var_local_rcpt_lim;

 /*
  * Queue manager: default delay before retrying a dead transport.
  */
#define VAR_XPORT_RETRY_TIME	"transport_retry_time"
#define DEF_XPORT_RETRY_TIME	"60s"
extern int var_transport_retry_time;

 /*
  * Queue manager: what transports to defer delivery to.
  */
#define VAR_DEFER_XPORTS	"defer_transports"
#define DEF_DEFER_XPORTS	""
extern char *var_defer_xports;

 /*
  * Queue manager: how often to warn that a destination is clogging the
  * active queue.
  */
#define VAR_QMGR_CLOG_WARN_TIME	"qmgr_clog_warn_time"
#define DEF_QMGR_CLOG_WARN_TIME	"300s"
extern int var_qmgr_clog_warn_time;

 /*
  * Master: default process count limit per mail subsystem.
  */
#define VAR_PROC_LIMIT		"default_process_limit"
#define DEF_PROC_LIMIT		100
extern int var_proc_limit;

 /*
  * Master: default time to wait after service is throttled.
  */
#define VAR_THROTTLE_TIME	"service_throttle_time"
#define DEF_THROTTLE_TIME	"60s"
extern int var_throttle_time;

 /*
  * Any subsystem: default maximum number of clients serviced before a mail
  * subsystem terminates (except queue manager).
  */
#define VAR_MAX_USE		"max_use"
#define DEF_MAX_USE		100
extern int var_use_limit;

 /*
  * Any subsystem: default amount of time a mail subsystem waits for a client
  * connection (except queue manager).
  */
#define VAR_MAX_IDLE		"max_idle"
#define DEF_MAX_IDLE		"100s"
extern int var_idle_limit;

 /*
  * Any subsystem: default amount of time a mail subsystem waits for
  * application events to drain.
  */
#define VAR_EVENT_DRAIN		"application_event_drain_time"
#define DEF_EVENT_DRAIN		"100s"
extern int var_event_drain;

 /*
  * Any subsystem: default amount of time a mail subsystem keeps an internal
  * IPC connection before closing it because it is idle for too much time.
  */
#define VAR_IPC_IDLE		"ipc_idle"
#define DEF_IPC_IDLE		"5s"
extern int var_ipc_idle_limit;

 /*
  * Any subsystem: default amount of time a mail subsystem keeps an internal
  * IPC connection before closing it because the connection has existed for
  * too much time.
  */
#define VAR_IPC_TTL		"ipc_ttl"
#define DEF_IPC_TTL		"1000s"
extern int var_ipc_ttl_limit;

 /*
  * Any front-end subsystem: avoid running out of memory when someone sends
  * infinitely-long requests or replies.
  */
#define VAR_LINE_LIMIT		"line_length_limit"
#define DEF_LINE_LIMIT		2048
extern int var_line_limit;

 /*
  * Specify what SMTP peers need verbose logging.
  */
#define VAR_DEBUG_PEER_LIST	"debug_peer_list"
#define DEF_DEBUG_PEER_LIST	""
extern char *var_debug_peer_list;

#define VAR_DEBUG_PEER_LEVEL	"debug_peer_level"
#define DEF_DEBUG_PEER_LEVEL	2
extern int var_debug_peer_level;

 /*
  * Queue management: what queues are hashed behind a forest of
  * subdirectories, and how deep the forest is.
  */
#define VAR_HASH_QUEUE_NAMES	"hash_queue_names"
#define DEF_HASH_QUEUE_NAMES	"deferred, defer"
extern char *var_hash_queue_names;

#define VAR_HASH_QUEUE_DEPTH	"hash_queue_depth"
#define DEF_HASH_QUEUE_DEPTH	1
extern int var_hash_queue_depth;

 /*
  * Multi-protocol support.
  */
#define INET_PROTO_NAME_IPV4	"ipv4"
#define INET_PROTO_NAME_IPV6	"ipv6"
#define INET_PROTO_NAME_ALL	"all"
#define VAR_INET_PROTOCOLS	"inet_protocols"
extern char *var_inet_protocols;

 /*
  * SMTP client. Timeouts inspired by RFC 1123. The SMTP recipient limit
  * determines how many recipient addresses the SMTP client sends along with
  * each message. Unfortunately, some mailers misbehave and disconnect (smap)
  * when given more recipients than they are willing to handle.
  * 
  * XXX 2821: A mail system is supposed to use EHLO instead of HELO, and to fall
  * back to HELO if EHLO is not supported.
  */
#define VAR_BESTMX_TRANSP	"best_mx_transport"
#define DEF_BESTMX_TRANSP	""
extern char *var_bestmx_transp;

#define VAR_SMTP_CACHE_CONNT	"smtp_connection_cache_time_limit"
#define DEF_SMTP_CACHE_CONNT	"2s"
#define VAR_LMTP_CACHE_CONNT	"lmtp_connection_cache_time_limit"
#define DEF_LMTP_CACHE_CONNT	"2s"
extern int var_smtp_cache_conn;

#define VAR_SMTP_REUSE_TIME	"smtp_connection_reuse_time_limit"
#define DEF_SMTP_REUSE_TIME	"300s"
#define VAR_LMTP_REUSE_TIME	"lmtp_connection_reuse_time_limit"
#define DEF_LMTP_REUSE_TIME	"300s"
extern int var_smtp_reuse_time;

#define VAR_SMTP_CACHE_DEST	"smtp_connection_cache_destinations"
#define DEF_SMTP_CACHE_DEST	""
#define VAR_LMTP_CACHE_DEST	"lmtp_connection_cache_destinations"
#define DEF_LMTP_CACHE_DEST	""
extern char *var_smtp_cache_dest;

#define VAR_SMTP_CACHE_DEMAND	"smtp_connection_cache_on_demand"
#ifndef DEF_SMTP_CACHE_DEMAND
#define DEF_SMTP_CACHE_DEMAND	1
#endif
#define VAR_LMTP_CACHE_DEMAND	"lmtp_connection_cache_on_demand"
#ifndef DEF_LMTP_CACHE_DEMAND
#define DEF_LMTP_CACHE_DEMAND	1
#endif
extern bool var_smtp_cache_demand;

#define VAR_SMTP_CONN_TMOUT	"smtp_connect_timeout"
#define DEF_SMTP_CONN_TMOUT	"30s"
extern int var_smtp_conn_tmout;

#define VAR_SMTP_HELO_TMOUT	"smtp_helo_timeout"
#define DEF_SMTP_HELO_TMOUT	"300s"
#define VAR_LMTP_HELO_TMOUT	"lmtp_lhlo_timeout"
#define DEF_LMTP_HELO_TMOUT	"300s"
extern int var_smtp_helo_tmout;

#define VAR_SMTP_XFWD_TMOUT	"smtp_xforward_timeout"
#define DEF_SMTP_XFWD_TMOUT	"300s"
extern int var_smtp_xfwd_tmout;

#define VAR_SMTP_STARTTLS_TMOUT	"smtp_starttls_timeout"
#define DEF_SMTP_STARTTLS_TMOUT	"300s"
#define VAR_LMTP_STARTTLS_TMOUT	"lmtp_starttls_timeout"
#define DEF_LMTP_STARTTLS_TMOUT	"300s"
extern int var_smtp_starttls_tmout;

#define VAR_SMTP_MAIL_TMOUT	"smtp_mail_timeout"
#define DEF_SMTP_MAIL_TMOUT	"300s"
extern int var_smtp_mail_tmout;

#define VAR_SMTP_RCPT_TMOUT	"smtp_rcpt_timeout"
#define DEF_SMTP_RCPT_TMOUT	"300s"
extern int var_smtp_rcpt_tmout;

#define VAR_SMTP_DATA0_TMOUT	"smtp_data_init_timeout"
#define DEF_SMTP_DATA0_TMOUT	"120s"
extern int var_smtp_data0_tmout;

#define VAR_SMTP_DATA1_TMOUT	"smtp_data_xfer_timeout"
#define DEF_SMTP_DATA1_TMOUT	"180s"
extern int var_smtp_data1_tmout;

#define VAR_SMTP_DATA2_TMOUT	"smtp_data_done_timeout"
#define DEF_SMTP_DATA2_TMOUT	"600s"
extern int var_smtp_data2_tmout;

#define VAR_SMTP_RSET_TMOUT	"smtp_rset_timeout"
#define DEF_SMTP_RSET_TMOUT	"20s"
extern int var_smtp_rset_tmout;

#define VAR_SMTP_QUIT_TMOUT	"smtp_quit_timeout"
#define DEF_SMTP_QUIT_TMOUT	"300s"
extern int var_smtp_quit_tmout;

#define VAR_SMTP_QUOTE_821_ENV	"smtp_quote_rfc821_envelope"
#define DEF_SMTP_QUOTE_821_ENV	1
#define VAR_LMTP_QUOTE_821_ENV	"lmtp_quote_rfc821_envelope"
#define DEF_LMTP_QUOTE_821_ENV	1
extern int var_smtp_quote_821_env;

#define VAR_SMTP_SKIP_5XX	"smtp_skip_5xx_greeting"
#define DEF_SMTP_SKIP_5XX	1
#define VAR_LMTP_SKIP_5XX	"lmtp_skip_5xx_greeting"
#define DEF_LMTP_SKIP_5XX	1
extern bool var_smtp_skip_5xx_greeting;

#define VAR_IGN_MX_LOOKUP_ERR	"ignore_mx_lookup_error"
#define DEF_IGN_MX_LOOKUP_ERR	0
extern bool var_ign_mx_lookup_err;

#define VAR_SKIP_QUIT_RESP	"smtp_skip_quit_response"
#define DEF_SKIP_QUIT_RESP	1
extern bool var_skip_quit_resp;

#define VAR_SMTP_ALWAYS_EHLO	"smtp_always_send_ehlo"
#ifdef RFC821_SYNTAX
#define DEF_SMTP_ALWAYS_EHLO	0
#else
#define DEF_SMTP_ALWAYS_EHLO	1
#endif
extern bool var_smtp_always_ehlo;

#define VAR_SMTP_NEVER_EHLO	"smtp_never_send_ehlo"
#define DEF_SMTP_NEVER_EHLO	0
extern bool var_smtp_never_ehlo;

#define VAR_SMTP_BIND_ADDR	"smtp_bind_address"
#define DEF_SMTP_BIND_ADDR	""
#define VAR_LMTP_BIND_ADDR	"lmtp_bind_address"
#define DEF_LMTP_BIND_ADDR	""
extern char *var_smtp_bind_addr;

#define VAR_SMTP_BIND_ADDR6	"smtp_bind_address6"
#define DEF_SMTP_BIND_ADDR6	""
#define VAR_LMTP_BIND_ADDR6	"lmtp_bind_address6"
#define DEF_LMTP_BIND_ADDR6	""
extern char *var_smtp_bind_addr6;

#define VAR_SMTP_HELO_NAME	"smtp_helo_name"
#define DEF_SMTP_HELO_NAME	"$myhostname"
#define VAR_LMTP_HELO_NAME	"lmtp_lhlo_name"
#define DEF_LMTP_HELO_NAME	"$myhostname"
extern char *var_smtp_helo_name;

#define VAR_SMTP_RAND_ADDR	"smtp_randomize_addresses"
#define DEF_SMTP_RAND_ADDR	1
#define VAR_LMTP_RAND_ADDR	"lmtp_randomize_addresses"
#define DEF_LMTP_RAND_ADDR	1
extern bool var_smtp_rand_addr;

#define VAR_SMTP_LINE_LIMIT	"smtp_line_length_limit"
#define DEF_SMTP_LINE_LIMIT	990
#define VAR_LMTP_LINE_LIMIT	"lmtp_line_length_limit"
#define DEF_LMTP_LINE_LIMIT	990
extern int var_smtp_line_limit;

#define VAR_SMTP_PIX_THRESH	"smtp_pix_workaround_threshold_time"
#define DEF_SMTP_PIX_THRESH	"500s"
#define VAR_LMTP_PIX_THRESH	"lmtp_pix_workaround_threshold_time"
#define DEF_LMTP_PIX_THRESH	"500s"
extern int var_smtp_pix_thresh;

#define VAR_SMTP_PIX_DELAY	"smtp_pix_workaround_delay_time"
#define DEF_SMTP_PIX_DELAY	"10s"
#define VAR_LMTP_PIX_DELAY	"lmtp_pix_workaround_delay_time"
#define DEF_LMTP_PIX_DELAY	"10s"
extern int var_smtp_pix_delay;

 /*
  * Courageous people may want to turn off PIX bug workarounds.
  */
#define	PIX_BUG_DISABLE_ESMTP		"disable_esmtp"
#define	PIX_BUG_DELAY_DOTCRLF		"delay_dotcrlf"
#define VAR_SMTP_PIX_BUG_WORDS		"smtp_pix_workarounds"
#define DEF_SMTP_PIX_BUG_WORDS		PIX_BUG_DISABLE_ESMTP "," \
					PIX_BUG_DELAY_DOTCRLF
#define VAR_LMTP_PIX_BUG_WORDS		"lmtp_pix_workarounds"
#define DEF_LMTP_PIX_BUG_WORDS		DEF_SMTP_PIX_BUG_WORDS
extern char *var_smtp_pix_bug_words;

#define VAR_SMTP_PIX_BUG_MAPS		"smtp_pix_workaround_maps"
#define DEF_SMTP_PIX_BUG_MAPS		""
#define VAR_LMTP_PIX_BUG_MAPS		"lmtp_pix_workaround_maps"
#define DEF_LMTP_PIX_BUG_MAPS		""
extern char *var_smtp_pix_bug_maps;

#define VAR_SMTP_DEFER_MXADDR	"smtp_defer_if_no_mx_address_found"
#define DEF_SMTP_DEFER_MXADDR	0
#define VAR_LMTP_DEFER_MXADDR	"lmtp_defer_if_no_mx_address_found"
#define DEF_LMTP_DEFER_MXADDR	0
extern bool var_smtp_defer_mxaddr;

#define VAR_SMTP_SEND_XFORWARD	"smtp_send_xforward_command"
#define DEF_SMTP_SEND_XFORWARD	0
extern bool var_smtp_send_xforward;

#define VAR_SMTP_GENERIC_MAPS	"smtp_generic_maps"
#define DEF_SMTP_GENERIC_MAPS	""
#define VAR_LMTP_GENERIC_MAPS	"lmtp_generic_maps"
#define DEF_LMTP_GENERIC_MAPS	""
extern char *var_smtp_generic_maps;

 /*
  * SMTP server. The soft error limit determines how many errors an SMTP
  * client may make before we start to slow down; the hard error limit
  * determines after how many client errors we disconnect.
  */
#define VAR_SMTPD_BANNER	"smtpd_banner"
#define DEF_SMTPD_BANNER	"$myhostname ESMTP $mail_name"
extern char *var_smtpd_banner;

#define VAR_SMTPD_TMOUT		"smtpd_timeout"
#define DEF_SMTPD_TMOUT		"300s"
extern int var_smtpd_tmout;

#define VAR_SMTPD_STARTTLS_TMOUT "smtpd_starttls_timeout"
#define DEF_SMTPD_STARTTLS_TMOUT "300s"
extern int var_smtpd_starttls_tmout;

#define VAR_SMTPD_RCPT_LIMIT	"smtpd_recipient_limit"
#define DEF_SMTPD_RCPT_LIMIT	1000
extern int var_smtpd_rcpt_limit;

#define VAR_SMTPD_SOFT_ERLIM	"smtpd_soft_error_limit"
#define DEF_SMTPD_SOFT_ERLIM	10
extern int var_smtpd_soft_erlim;

#define VAR_SMTPD_HARD_ERLIM	"smtpd_hard_error_limit"
#define DEF_SMTPD_HARD_ERLIM	20
extern int var_smtpd_hard_erlim;

#define VAR_SMTPD_ERR_SLEEP	"smtpd_error_sleep_time"
#define DEF_SMTPD_ERR_SLEEP	"1s"
extern int var_smtpd_err_sleep;

#define VAR_SMTPD_JUNK_CMD	"smtpd_junk_command_limit"
#define DEF_SMTPD_JUNK_CMD	100
extern int var_smtpd_junk_cmd_limit;

#define VAR_SMTPD_RCPT_OVERLIM	"smtpd_recipient_overshoot_limit"
#define DEF_SMTPD_RCPT_OVERLIM	1000
extern int var_smtpd_rcpt_overlim;

#define VAR_SMTPD_HIST_THRSH	"smtpd_history_flush_threshold"
#define DEF_SMTPD_HIST_THRSH	100
extern int var_smtpd_hist_thrsh;

#define VAR_SMTPD_NOOP_CMDS	"smtpd_noop_commands"
#define DEF_SMTPD_NOOP_CMDS	""
extern char *var_smtpd_noop_cmds;

#define VAR_SMTPD_FORBID_CMDS	"smtpd_forbidden_commands"
#define DEF_SMTPD_FORBID_CMDS	"CONNECT GET POST"
extern char *var_smtpd_forbid_cmds;

#define VAR_SMTPD_TLS_WRAPPER	"smtpd_tls_wrappermode"
#define DEF_SMTPD_TLS_WRAPPER	0
extern bool var_smtpd_tls_wrappermode;

#define VAR_SMTPD_TLS_LEVEL	"smtpd_tls_security_level"
#define DEF_SMTPD_TLS_LEVEL	""
extern char *var_smtpd_tls_level;

#define VAR_SMTPD_USE_TLS	"smtpd_use_tls"
#define DEF_SMTPD_USE_TLS	0
extern bool var_smtpd_use_tls;

#define VAR_SMTPD_ENFORCE_TLS	"smtpd_enforce_tls"
#define DEF_SMTPD_ENFORCE_TLS	0
extern bool var_smtpd_enforce_tls;

#define VAR_SMTPD_TLS_AUTH_ONLY	"smtpd_tls_auth_only"
#define DEF_SMTPD_TLS_AUTH_ONLY 0
extern bool var_smtpd_tls_auth_only;

#define VAR_SMTPD_TLS_ACERT	"smtpd_tls_ask_ccert"
#define DEF_SMTPD_TLS_ACERT	0
extern bool var_smtpd_tls_ask_ccert;

#define VAR_SMTPD_TLS_RCERT	"smtpd_tls_req_ccert"
#define DEF_SMTPD_TLS_RCERT	0
extern bool var_smtpd_tls_req_ccert;

#define VAR_SMTPD_TLS_CCERT_VD	"smtpd_tls_ccert_verifydepth"
#define DEF_SMTPD_TLS_CCERT_VD	5
extern int var_smtpd_tls_ccert_vd;

#define VAR_SMTPD_TLS_CERT_FILE	"smtpd_tls_cert_file"
#define DEF_SMTPD_TLS_CERT_FILE	""
extern char *var_smtpd_tls_cert_file;

#define VAR_SMTPD_TLS_KEY_FILE	"smtpd_tls_key_file"
#define DEF_SMTPD_TLS_KEY_FILE	"$smtpd_tls_cert_file"
extern char *var_smtpd_tls_key_file;

#define VAR_SMTPD_TLS_DCERT_FILE "smtpd_tls_dcert_file"
#define DEF_SMTPD_TLS_DCERT_FILE ""
extern char *var_smtpd_tls_dcert_file;

#define VAR_SMTPD_TLS_DKEY_FILE	"smtpd_tls_dkey_file"
#define DEF_SMTPD_TLS_DKEY_FILE	"$smtpd_tls_dcert_file"
extern char *var_smtpd_tls_dkey_file;

#define VAR_SMTPD_TLS_CA_FILE	"smtpd_tls_CAfile"
#define DEF_SMTPD_TLS_CA_FILE	""
extern char *var_smtpd_tls_CAfile;

#define VAR_SMTPD_TLS_CA_PATH	"smtpd_tls_CApath"
#define DEF_SMTPD_TLS_CA_PATH	""
extern char *var_smtpd_tls_CApath;

#define VAR_SMTPD_TLS_MAND_PROTO	"smtpd_tls_mandatory_protocols"
#define DEF_SMTPD_TLS_MAND_PROTO	"SSLv3, TLSv1"
extern char *var_smtpd_tls_mand_proto;

#define VAR_SMTPD_TLS_MAND_CIPH	"smtpd_tls_mandatory_ciphers"
#define DEF_SMTPD_TLS_MAND_CIPH	"medium"
extern char *var_smtpd_tls_mand_ciph;

#define VAR_SMTPD_TLS_EXCL_CIPH  "smtpd_tls_exclude_ciphers"
#define DEF_SMTPD_TLS_EXCL_CIPH  ""
extern char *var_smtpd_tls_excl_ciph;

#define VAR_SMTPD_TLS_MAND_EXCL  "smtpd_tls_mandatory_exclude_ciphers"
#define DEF_SMTPD_TLS_MAND_EXCL  ""
extern char *var_smtpd_tls_mand_excl;

#define VAR_SMTPD_TLS_512_FILE	"smtpd_tls_dh512_param_file"
#define DEF_SMTPD_TLS_512_FILE	""
extern char *var_smtpd_tls_dh512_param_file;

#define VAR_SMTPD_TLS_1024_FILE	"smtpd_tls_dh1024_param_file"
#define DEF_SMTPD_TLS_1024_FILE	""
extern char *var_smtpd_tls_dh1024_param_file;

#define VAR_SMTPD_TLS_LOGLEVEL	"smtpd_tls_loglevel"
#define DEF_SMTPD_TLS_LOGLEVEL	0
extern int var_smtpd_tls_loglevel;

#define VAR_SMTPD_TLS_RECHEAD	"smtpd_tls_received_header"
#define DEF_SMTPD_TLS_RECHEAD	0
extern bool var_smtpd_tls_received_header;

#define VAR_SMTPD_TLS_SCACHE_DB	"smtpd_tls_session_cache_database"
#define DEF_SMTPD_TLS_SCACHE_DB	""
extern char *var_smtpd_tls_scache_db;

#define VAR_SMTPD_TLS_SCACHTIME	"smtpd_tls_session_cache_timeout"
#define DEF_SMTPD_TLS_SCACHTIME	"3600s"
extern int var_smtpd_tls_scache_timeout;

#define VAR_SMTPD_TLS_SET_SESSID	"smtpd_tls_always_issue_session_ids"
#define DEF_SMTPD_TLS_SET_SESSID	1
extern bool var_smtpd_tls_set_sessid;

#define VAR_SMTPD_DELAY_OPEN	"smtpd_delay_open_until_valid_rcpt"
#define DEF_SMTPD_DELAY_OPEN	1
extern bool var_smtpd_delay_open;

#define VAR_SMTP_TLS_PER_SITE	"smtp_tls_per_site"
#define DEF_SMTP_TLS_PER_SITE	""
#define VAR_LMTP_TLS_PER_SITE	"lmtp_tls_per_site"
#define DEF_LMTP_TLS_PER_SITE	""
extern char *var_smtp_tls_per_site;

#define VAR_SMTP_USE_TLS	"smtp_use_tls"
#define DEF_SMTP_USE_TLS	0
#define VAR_LMTP_USE_TLS	"lmtp_use_tls"
#define DEF_LMTP_USE_TLS	0
extern bool var_smtp_use_tls;

#define VAR_SMTP_ENFORCE_TLS	"smtp_enforce_tls"
#define DEF_SMTP_ENFORCE_TLS	0
#define VAR_LMTP_ENFORCE_TLS	"lmtp_enforce_tls"
#define DEF_LMTP_ENFORCE_TLS	0
extern bool var_smtp_enforce_tls;

#define VAR_SMTP_TLS_ENFORCE_PN	"smtp_tls_enforce_peername"
#define DEF_SMTP_TLS_ENFORCE_PN	1
#define VAR_LMTP_TLS_ENFORCE_PN	"lmtp_tls_enforce_peername"
#define DEF_LMTP_TLS_ENFORCE_PN	1
extern bool var_smtp_tls_enforce_peername;

#define VAR_SMTP_TLS_LEVEL	"smtp_tls_security_level"
#define DEF_SMTP_TLS_LEVEL	""
#define VAR_LMTP_TLS_LEVEL	"lmtp_tls_security_level"
#define DEF_LMTP_TLS_LEVEL	""
extern char *var_smtp_tls_level;

#define VAR_SMTP_TLS_SCERT_VD	"smtp_tls_scert_verifydepth"
#define DEF_SMTP_TLS_SCERT_VD	5
#define VAR_LMTP_TLS_SCERT_VD	"lmtp_tls_scert_verifydepth"
#define DEF_LMTP_TLS_SCERT_VD	5
extern int var_smtp_tls_scert_vd;

#define VAR_SMTP_TLS_CERT_FILE	"smtp_tls_cert_file"
#define DEF_SMTP_TLS_CERT_FILE	""
#define VAR_LMTP_TLS_CERT_FILE	"lmtp_tls_cert_file"
#define DEF_LMTP_TLS_CERT_FILE	""
extern char *var_smtp_tls_cert_file;

#define VAR_SMTP_TLS_KEY_FILE	"smtp_tls_key_file"
#define DEF_SMTP_TLS_KEY_FILE	"$smtp_tls_cert_file"
#define VAR_LMTP_TLS_KEY_FILE	"lmtp_tls_key_file"
#define DEF_LMTP_TLS_KEY_FILE	"$lmtp_tls_cert_file"
extern char *var_smtp_tls_key_file;

#define VAR_SMTP_TLS_DCERT_FILE "smtp_tls_dcert_file"
#define DEF_SMTP_TLS_DCERT_FILE ""
#define VAR_LMTP_TLS_DCERT_FILE "lmtp_tls_dcert_file"
#define DEF_LMTP_TLS_DCERT_FILE ""
extern char *var_smtp_tls_dcert_file;

#define VAR_SMTP_TLS_DKEY_FILE	"smtp_tls_dkey_file"
#define DEF_SMTP_TLS_DKEY_FILE	"$smtp_tls_dcert_file"
#define VAR_LMTP_TLS_DKEY_FILE	"lmtp_tls_dkey_file"
#define DEF_LMTP_TLS_DKEY_FILE	"$lmtp_tls_dcert_file"
extern char *var_smtp_tls_dkey_file;

#define VAR_SMTP_TLS_CA_FILE	"smtp_tls_CAfile"
#define DEF_SMTP_TLS_CA_FILE	""
#define VAR_LMTP_TLS_CA_FILE	"lmtp_tls_CAfile"
#define DEF_LMTP_TLS_CA_FILE	""
extern char *var_smtp_tls_CAfile;

#define VAR_SMTP_TLS_CA_PATH	"smtp_tls_CApath"
#define DEF_SMTP_TLS_CA_PATH	""
#define VAR_LMTP_TLS_CA_PATH	"lmtp_tls_CApath"
#define DEF_LMTP_TLS_CA_PATH	""
extern char *var_smtp_tls_CApath;

#define VAR_SMTP_TLS_MAND_CIPH	"smtp_tls_mandatory_ciphers"
#define DEF_SMTP_TLS_MAND_CIPH	"medium"
#define VAR_LMTP_TLS_MAND_CIPH	"lmtp_tls_mandatory_ciphers"
#define DEF_LMTP_TLS_MAND_CIPH	"medium"
extern char *var_smtp_tls_mand_ciph;

#define VAR_SMTP_TLS_EXCL_CIPH  "smtp_tls_exclude_ciphers"
#define DEF_SMTP_TLS_EXCL_CIPH  ""
#define VAR_LMTP_TLS_EXCL_CIPH  "lmtp_tls_exclude_ciphers"
#define DEF_LMTP_TLS_EXCL_CIPH  ""
extern char *var_smtp_tls_excl_ciph;

#define VAR_SMTP_TLS_MAND_EXCL  "smtp_tls_mandatory_exclude_ciphers"
#define DEF_SMTP_TLS_MAND_EXCL  ""
#define VAR_LMTP_TLS_MAND_EXCL  "lmtp_tls_mandatory_exclude_ciphers"
#define DEF_LMTP_TLS_MAND_EXCL  ""
extern char *var_smtp_tls_mand_excl;

#define VAR_SMTP_TLS_LOGLEVEL	"smtp_tls_loglevel"
#define DEF_SMTP_TLS_LOGLEVEL	0
#define VAR_LMTP_TLS_LOGLEVEL	"lmtp_tls_loglevel"
#define DEF_LMTP_TLS_LOGLEVEL	0
extern int var_smtp_tls_loglevel;	/* In smtp(8) and tlsmgr(8) */
extern int var_lmtp_tls_loglevel;	/* In tlsmgr(8) */

#define VAR_SMTP_TLS_NOTEOFFER	"smtp_tls_note_starttls_offer"
#define DEF_SMTP_TLS_NOTEOFFER	0
#define VAR_LMTP_TLS_NOTEOFFER	"lmtp_tls_note_starttls_offer"
#define DEF_LMTP_TLS_NOTEOFFER	0
extern bool var_smtp_tls_note_starttls_offer;

#define VAR_SMTP_TLS_SCACHE_DB	"smtp_tls_session_cache_database"
#define DEF_SMTP_TLS_SCACHE_DB	""
#define VAR_LMTP_TLS_SCACHE_DB	"lmtp_tls_session_cache_database"
#define DEF_LMTP_TLS_SCACHE_DB	""
extern char *var_smtp_tls_scache_db;
extern char *var_lmtp_tls_scache_db;

#define VAR_SMTP_TLS_SCACHTIME	"smtp_tls_session_cache_timeout"
#define DEF_SMTP_TLS_SCACHTIME	"3600s"
#define VAR_LMTP_TLS_SCACHTIME	"lmtp_tls_session_cache_timeout"
#define DEF_LMTP_TLS_SCACHTIME	"3600s"
extern int var_smtp_tls_scache_timeout;
extern int var_lmtp_tls_scache_timeout;

#define VAR_SMTP_TLS_POLICY	"smtp_tls_policy_maps"
#define DEF_SMTP_TLS_POLICY	""
#define VAR_LMTP_TLS_POLICY	"lmtp_tls_policy_maps"
#define DEF_LMTP_TLS_POLICY	""
extern char *var_smtp_tls_policy;

#define VAR_SMTP_TLS_MAND_PROTO	"smtp_tls_mandatory_protocols"
#define DEF_SMTP_TLS_MAND_PROTO	"SSLv3, TLSv1"
#define VAR_LMTP_TLS_MAND_PROTO	"lmtp_tls_mandatory_protocols"
#define DEF_LMTP_TLS_MAND_PROTO	"SSLv3, TLSv1"
extern char *var_smtp_tls_mand_proto;

#define VAR_SMTP_TLS_VFY_CMATCH	"smtp_tls_verify_cert_match"
#define DEF_SMTP_TLS_VFY_CMATCH	"hostname"
#define VAR_LMTP_TLS_VFY_CMATCH	"lmtp_tls_verify_cert_match"
#define DEF_LMTP_TLS_VFY_CMATCH	"hostname"
extern char *var_smtp_tls_vfy_cmatch;

 /*
  * There are no MX lookups for LMTP, so verify == secure
  */
#define VAR_SMTP_TLS_SEC_CMATCH	"smtp_tls_secure_cert_match"
#define DEF_SMTP_TLS_SEC_CMATCH	"nexthop, dot-nexthop"
#define VAR_LMTP_TLS_SEC_CMATCH	"lmtp_tls_secure_cert_match"
#define DEF_LMTP_TLS_SEC_CMATCH	"nexthop"
extern char *var_smtp_tls_sec_cmatch;


 /*
  * SASL authentication support, SMTP server side.
  */
#define VAR_SMTPD_SASL_ENABLE	"smtpd_sasl_auth_enable"
#define DEF_SMTPD_SASL_ENABLE	0
extern bool var_smtpd_sasl_enable;

#define VAR_SMTPD_SASL_AUTH_HDR	"smtpd_sasl_authenticated_header"
#define DEF_SMTPD_SASL_AUTH_HDR	0
extern bool var_smtpd_sasl_auth_hdr;

#define VAR_SMTPD_SASL_OPTS	"smtpd_sasl_security_options"
#define DEF_SMTPD_SASL_OPTS	"noanonymous"
extern char *var_smtpd_sasl_opts;

#define VAR_SMTPD_SASL_PATH	"smtpd_sasl_path"
#define DEF_SMTPD_SASL_PATH	"smtpd"
extern char *var_smtpd_sasl_path;

#define VAR_SMTPD_SASL_TLS_OPTS	"smtpd_sasl_tls_security_options"
#define DEF_SMTPD_SASL_TLS_OPTS	"$" VAR_SMTPD_SASL_OPTS
extern char *var_smtpd_sasl_tls_opts;

#define VAR_SMTPD_SASL_REALM	"smtpd_sasl_local_domain"
#define DEF_SMTPD_SASL_REALM	""
extern char *var_smtpd_sasl_realm;

#define VAR_SMTPD_SASL_EXCEPTIONS_NETWORKS	"smtpd_sasl_exceptions_networks"
#define DEF_SMTPD_SASL_EXCEPTIONS_NETWORKS	""
extern char *var_smtpd_sasl_exceptions_networks;

#ifndef DEF_SERVER_SASL_TYPE
#define DEF_SERVER_SASL_TYPE	"cyrus"
#endif

#define VAR_SMTPD_SASL_TYPE	"smtpd_sasl_type"
#define DEF_SMTPD_SASL_TYPE	DEF_SERVER_SASL_TYPE
extern char *var_smtpd_sasl_type;

#define VAR_SMTPD_SND_AUTH_MAPS	"smtpd_sender_login_maps"
#define DEF_SMTPD_SND_AUTH_MAPS	""
extern char *var_smtpd_snd_auth_maps;

#define REJECT_SENDER_LOGIN_MISMATCH	"reject_sender_login_mismatch"
#define REJECT_AUTH_SENDER_LOGIN_MISMATCH \
				"reject_authenticated_sender_login_mismatch"
#define REJECT_UNAUTH_SENDER_LOGIN_MISMATCH \
				"reject_unauthenticated_sender_login_mismatch"

 /*
  * SASL authentication support, SMTP client side.
  */
#define VAR_SMTP_SASL_ENABLE	"smtp_sasl_auth_enable"
#define DEF_SMTP_SASL_ENABLE	0
extern bool var_smtp_sasl_enable;

#define VAR_SMTP_SASL_PASSWD	"smtp_sasl_password_maps"
#define DEF_SMTP_SASL_PASSWD	""
extern char *var_smtp_sasl_passwd;

#define VAR_SMTP_SASL_OPTS	"smtp_sasl_security_options"
#define DEF_SMTP_SASL_OPTS	"noplaintext, noanonymous"
extern char *var_smtp_sasl_opts;

#define VAR_SMTP_SASL_PATH	"smtp_sasl_path"
#define DEF_SMTP_SASL_PATH	""
extern char *var_smtp_sasl_path;

#define VAR_SMTP_SASL_MECHS	"smtp_sasl_mechanism_filter"
#define DEF_SMTP_SASL_MECHS	""
#define VAR_LMTP_SASL_MECHS	"lmtp_sasl_mechanism_filter"
#define DEF_LMTP_SASL_MECHS	""
extern char *var_smtp_sasl_mechs;

#ifndef DEF_CLIENT_SASL_TYPE
#define DEF_CLIENT_SASL_TYPE	"cyrus"
#endif

#define VAR_SMTP_SASL_TYPE	"smtp_sasl_type"
#define DEF_SMTP_SASL_TYPE	DEF_CLIENT_SASL_TYPE
#define VAR_LMTP_SASL_TYPE	"lmtp_sasl_type"
#define DEF_LMTP_SASL_TYPE	DEF_CLIENT_SASL_TYPE
extern char *var_smtp_sasl_type;

#define VAR_SMTP_SASL_TLS_OPTS	"smtp_sasl_tls_security_options"
#define DEF_SMTP_SASL_TLS_OPTS	"$" VAR_SMTP_SASL_OPTS
#define VAR_LMTP_SASL_TLS_OPTS	"lmtp_sasl_tls_security_options"
#define DEF_LMTP_SASL_TLS_OPTS	"$" VAR_LMTP_SASL_OPTS
extern char *var_smtp_sasl_tls_opts;

#define VAR_SMTP_SASL_TLSV_OPTS	"smtp_sasl_tls_verified_security_options"
#define DEF_SMTP_SASL_TLSV_OPTS	"$" VAR_SMTP_SASL_TLS_OPTS
#define VAR_LMTP_SASL_TLSV_OPTS	"lmtp_sasl_tls_verified_security_options"
#define DEF_LMTP_SASL_TLSV_OPTS	"$" VAR_LMTP_SASL_TLS_OPTS
extern char *var_smtp_sasl_tlsv_opts;

 /*
  * LMTP server. The soft error limit determines how many errors an LMTP
  * client may make before we start to slow down; the hard error limit
  * determines after how many client errors we disconnect.
  */
#define VAR_LMTPD_BANNER	"lmtpd_banner"
#define DEF_LMTPD_BANNER	"$myhostname $mail_name"
extern char *var_lmtpd_banner;

#define VAR_LMTPD_TMOUT		"lmtpd_timeout"
#define DEF_LMTPD_TMOUT		"300s"
extern int var_lmtpd_tmout;

#define VAR_LMTPD_RCPT_LIMIT	"lmtpd_recipient_limit"
#define DEF_LMTPD_RCPT_LIMIT	1000
extern int var_lmtpd_rcpt_limit;

#define VAR_LMTPD_SOFT_ERLIM	"lmtpd_soft_error_limit"
#define DEF_LMTPD_SOFT_ERLIM	10
extern int var_lmtpd_soft_erlim;

#define VAR_LMTPD_HARD_ERLIM	"lmtpd_hard_error_limit"
#define DEF_LMTPD_HARD_ERLIM	100
extern int var_lmtpd_hard_erlim;

#define VAR_LMTPD_ERR_SLEEP	"lmtpd_error_sleep_time"
#define DEF_LMTPD_ERR_SLEEP	"5s"
extern int var_lmtpd_err_sleep;

#define VAR_LMTPD_JUNK_CMD	"lmtpd_junk_command_limit"
#define DEF_LMTPD_JUNK_CMD	1000
extern int var_lmtpd_junk_cmd_limit;

 /*
  * SASL authentication support, LMTP server side.
  */
#define VAR_LMTPD_SASL_ENABLE	"lmtpd_sasl_auth_enable"
#define DEF_LMTPD_SASL_ENABLE	0
extern bool var_lmtpd_sasl_enable;

#define VAR_LMTPD_SASL_OPTS	"lmtpd_sasl_security_options"
#define DEF_LMTPD_SASL_OPTS	"noanonymous"
extern char *var_lmtpd_sasl_opts;

#define VAR_LMTPD_SASL_REALM	"lmtpd_sasl_local_domain"
#define DEF_LMTPD_SASL_REALM	"$myhostname"
extern char *var_lmtpd_sasl_realm;

 /*
  * SASL authentication support, LMTP client side.
  */
#define VAR_LMTP_SASL_ENABLE	"lmtp_sasl_auth_enable"
#define DEF_LMTP_SASL_ENABLE	0
extern bool var_lmtp_sasl_enable;

#define VAR_LMTP_SASL_PASSWD	"lmtp_sasl_password_maps"
#define DEF_LMTP_SASL_PASSWD	""
extern char *var_lmtp_sasl_passwd;

#define VAR_LMTP_SASL_OPTS	"lmtp_sasl_security_options"
#define DEF_LMTP_SASL_OPTS	"noplaintext, noanonymous"
extern char *var_lmtp_sasl_opts;

#define VAR_LMTP_SASL_PATH	"lmtp_sasl_path"
#define DEF_LMTP_SASL_PATH	""
extern char *var_lmtp_sasl_path;

 /*
  * SASL-based relay etc. control.
  */
#define PERMIT_SASL_AUTH	"permit_sasl_authenticated"

 /*
  * LMTP client. Timeouts inspired by RFC 1123. The LMTP recipient limit
  * determines how many recipient addresses the LMTP client sends along with
  * each message. Unfortunately, some mailers misbehave and disconnect (smap)
  * when given more recipients than they are willing to handle.
  */
#define VAR_LMTP_TCP_PORT	"lmtp_tcp_port"
#define DEF_LMTP_TCP_PORT	"24"
extern char *var_lmtp_tcp_port;

#define VAR_LMTP_CACHE_CONN	"lmtp_cache_connection"
#define DEF_LMTP_CACHE_CONN	1
extern bool var_lmtp_cache_conn;

#define VAR_LMTP_SKIP_QUIT_RESP	"lmtp_skip_quit_response"
#define DEF_LMTP_SKIP_QUIT_RESP	0
extern bool var_lmtp_skip_quit_resp;

#define VAR_LMTP_CONN_TMOUT	"lmtp_connect_timeout"
#define DEF_LMTP_CONN_TMOUT	"0s"
extern int var_lmtp_conn_tmout;

#define VAR_LMTP_RSET_TMOUT	"lmtp_rset_timeout"
#define DEF_LMTP_RSET_TMOUT	"20s"
extern int var_lmtp_rset_tmout;

#define VAR_LMTP_LHLO_TMOUT	"lmtp_lhlo_timeout"
#define DEF_LMTP_LHLO_TMOUT	"300s"
extern int var_lmtp_lhlo_tmout;

#define VAR_LMTP_XFWD_TMOUT	"lmtp_xforward_timeout"
#define DEF_LMTP_XFWD_TMOUT	"300s"
extern int var_lmtp_xfwd_tmout;

#define VAR_LMTP_MAIL_TMOUT	"lmtp_mail_timeout"
#define DEF_LMTP_MAIL_TMOUT	"300s"
extern int var_lmtp_mail_tmout;

#define VAR_LMTP_RCPT_TMOUT	"lmtp_rcpt_timeout"
#define DEF_LMTP_RCPT_TMOUT	"300s"
extern int var_lmtp_rcpt_tmout;

#define VAR_LMTP_DATA0_TMOUT	"lmtp_data_init_timeout"
#define DEF_LMTP_DATA0_TMOUT	"120s"
extern int var_lmtp_data0_tmout;

#define VAR_LMTP_DATA1_TMOUT	"lmtp_data_xfer_timeout"
#define DEF_LMTP_DATA1_TMOUT	"180s"
extern int var_lmtp_data1_tmout;

#define VAR_LMTP_DATA2_TMOUT	"lmtp_data_done_timeout"
#define DEF_LMTP_DATA2_TMOUT	"600s"
extern int var_lmtp_data2_tmout;

#define VAR_LMTP_QUIT_TMOUT	"lmtp_quit_timeout"
#define DEF_LMTP_QUIT_TMOUT	"300s"
extern int var_lmtp_quit_tmout;

#define VAR_LMTP_SEND_XFORWARD	"lmtp_send_xforward_command"
#define DEF_LMTP_SEND_XFORWARD	0
extern bool var_lmtp_send_xforward;

 /*
  * Cleanup service. Header info that exceeds $header_size_limit bytes or
  * $header_address_token_limit tokens is discarded.
  */
#define VAR_HOPCOUNT_LIMIT	"hopcount_limit"
#define DEF_HOPCOUNT_LIMIT	50
extern int var_hopcount_limit;

#define VAR_HEADER_LIMIT	"header_size_limit"
#define DEF_HEADER_LIMIT	102400
extern int var_header_limit;

#define VAR_TOKEN_LIMIT		"header_address_token_limit"
#define DEF_TOKEN_LIMIT		10240
extern int var_token_limit;

#define VAR_VIRT_RECUR_LIMIT	"virtual_alias_recursion_limit"
#define DEF_VIRT_RECUR_LIMIT	1000
extern int var_virt_recur_limit;

#define VAR_VIRT_EXPAN_LIMIT	"virtual_alias_expansion_limit"
#define DEF_VIRT_EXPAN_LIMIT	1000
extern int var_virt_expan_limit;

 /*
  * Message/queue size limits.
  */
#define VAR_MESSAGE_LIMIT	"message_size_limit"
#define DEF_MESSAGE_LIMIT	10240000
extern int var_message_limit;

#define VAR_QUEUE_MINFREE	"queue_minfree"
#define DEF_QUEUE_MINFREE	0
extern int var_queue_minfree;

#define VAR_HEADER_CHECKS	"header_checks"
#define DEF_HEADER_CHECKS	""
extern char *var_header_checks;

#define VAR_MIMEHDR_CHECKS	"mime_header_checks"
#define DEF_MIMEHDR_CHECKS	"$header_checks"
extern char *var_mimehdr_checks;

#define VAR_NESTHDR_CHECKS	"nested_header_checks"
#define DEF_NESTHDR_CHECKS	"$header_checks"
extern char *var_nesthdr_checks;

#define VAR_BODY_CHECKS		"body_checks"
#define DEF_BODY_CHECKS		""
extern char *var_body_checks;

#define VAR_BODY_CHECK_LEN	"body_checks_size_limit"
#define DEF_BODY_CHECK_LEN	(50*1024)
extern int var_body_check_len;

 /*
  * Bounce service: truncate bounce message that exceed $bounce_size_limit.
  */
#define VAR_BOUNCE_LIMIT	"bounce_size_limit"
#define DEF_BOUNCE_LIMIT	50000
extern int var_bounce_limit;

 /*
  * Bounce service: reserved sender address for double bounces. The local
  * delivery service discards undeliverable double bounces.
  */
#define VAR_DOUBLE_BOUNCE	"double_bounce_sender"
#define DEF_DOUBLE_BOUNCE	"double-bounce"
extern char *var_double_bounce_sender;

 /*
  * When forking a process, how often to try and how long to wait.
  */
#define VAR_FORK_TRIES		"fork_attempts"
#define DEF_FORK_TRIES		5
extern int var_fork_tries;

#define VAR_FORK_DELAY		"fork_delay"
#define DEF_FORK_DELAY		"1s"
extern int var_fork_delay;

 /*
  * When locking a mailbox, how often to try and how long to wait.
  */
#define VAR_FLOCK_TRIES          "deliver_lock_attempts"
#define DEF_FLOCK_TRIES          20
extern int var_flock_tries;

#define VAR_FLOCK_DELAY          "deliver_lock_delay"
#define DEF_FLOCK_DELAY          "1s"
extern int var_flock_delay;

#define VAR_FLOCK_STALE		"stale_lock_time"
#define DEF_FLOCK_STALE		"500s"
extern int var_flock_stale;

#define VAR_MAILTOOL_COMPAT	"sun_mailtool_compatibility"
#define DEF_MAILTOOL_COMPAT	0
extern int var_mailtool_compat;

 /*
  * How long a daemon command may take to receive or deliver a message etc.
  * before we assume it is wegded (should never happen).
  */
#define VAR_DAEMON_TIMEOUT	"daemon_timeout"
#define DEF_DAEMON_TIMEOUT	"18000s"
extern int var_daemon_timeout;

 /*
  * How long an intra-mail command may take before we assume the mail system
  * is in deadlock (should never happen).
  */
#define VAR_IPC_TIMEOUT		"ipc_timeout"
#define DEF_IPC_TIMEOUT		"3600s"
extern int var_ipc_timeout;

 /*
  * Time limit on intra-mail triggers.
  */
#define VAR_TRIGGER_TIMEOUT	"trigger_timeout"
#define DEF_TRIGGER_TIMEOUT	"10s"
extern int var_trigger_timeout;

 /*
  * SMTP server restrictions. What networks I am willing to relay from, what
  * domains I am willing to forward mail from or to, what clients I refuse to
  * talk to, and what domains I never want to see in the sender address.
  */
#define VAR_MYNETWORKS		"mynetworks"
extern char *var_mynetworks;

#define VAR_MYNETWORKS_STYLE	"mynetworks_style"
#define DEF_MYNETWORKS_STYLE	MYNETWORKS_STYLE_SUBNET
extern char *var_mynetworks_style;

#define	MYNETWORKS_STYLE_CLASS	"class"
#define	MYNETWORKS_STYLE_SUBNET	"subnet"
#define	MYNETWORKS_STYLE_HOST	"host"

#define VAR_RELAY_DOMAINS	"relay_domains"
#define DEF_RELAY_DOMAINS	"$mydestination"
extern char *var_relay_domains;

#define VAR_RELAY_TRANSPORT	"relay_transport"
#define DEF_RELAY_TRANSPORT	MAIL_SERVICE_RELAY
extern char *var_relay_transport;

#define VAR_RELAY_RCPT_MAPS	"relay_recipient_maps"
#define DEF_RELAY_RCPT_MAPS	""
extern char *var_relay_rcpt_maps;

#define VAR_RELAY_RCPT_CODE	"unknown_relay_recipient_reject_code"
#define DEF_RELAY_RCPT_CODE	550
extern int var_relay_rcpt_code;

#define VAR_RELAY_CCERTS	"relay_clientcerts"
#define DEF_RELAY_CCERTS	""
extern char *var_smtpd_relay_ccerts;

#define VAR_CLIENT_CHECKS	"smtpd_client_restrictions"
#define DEF_CLIENT_CHECKS	""
extern char *var_client_checks;

#define VAR_HELO_REQUIRED	"smtpd_helo_required"
#define DEF_HELO_REQUIRED	0
extern bool var_helo_required;

#define VAR_HELO_CHECKS		"smtpd_helo_restrictions"
#define DEF_HELO_CHECKS		""
extern char *var_helo_checks;

#define VAR_MAIL_CHECKS		"smtpd_sender_restrictions"
#define DEF_MAIL_CHECKS		""
extern char *var_mail_checks;

#define VAR_RCPT_CHECKS		"smtpd_recipient_restrictions"
#define DEF_RCPT_CHECKS		PERMIT_MYNETWORKS ", " REJECT_UNAUTH_DEST
extern char *var_rcpt_checks;

#define VAR_ETRN_CHECKS		"smtpd_etrn_restrictions"
#define DEF_ETRN_CHECKS		""
extern char *var_etrn_checks;

#define VAR_DATA_CHECKS		"smtpd_data_restrictions"
#define DEF_DATA_CHECKS		""
extern char *var_data_checks;

#define VAR_EOD_CHECKS		"smtpd_end_of_data_restrictions"
#define DEF_EOD_CHECKS		""
extern char *var_eod_checks;

#define VAR_REST_CLASSES	"smtpd_restriction_classes"
#define DEF_REST_CLASSES	""
extern char *var_rest_classes;

#define VAR_ALLOW_UNTRUST_ROUTE	"allow_untrusted_routing"
#define DEF_ALLOW_UNTRUST_ROUTE	0
extern bool var_allow_untrust_route;

 /*
  * Names of specific restrictions, and the corresponding configuration
  * parameters that control the status codes sent in response to rejected
  * requests.
  */
#define PERMIT_ALL		"permit"
#define REJECT_ALL		"reject"
#define VAR_REJECT_CODE		"reject_code"
#define DEF_REJECT_CODE		554
extern int var_reject_code;

#define DEFER_ALL		"defer"
#define VAR_DEFER_CODE		"defer_code"
#define DEF_DEFER_CODE		450
extern int var_defer_code;

#define DEFER_IF_PERMIT		"defer_if_permit"
#define DEFER_IF_REJECT		"defer_if_reject"

#define SLEEP			"sleep"

#define REJECT_PLAINTEXT_SESSION "reject_plaintext_session"
#define VAR_PLAINTEXT_CODE	"plaintext_reject_code"
#define DEF_PLAINTEXT_CODE	450
extern int var_plaintext_code;

#define REJECT_UNKNOWN_CLIENT	"reject_unknown_client"
#define REJECT_UNKNOWN_CLIENT_HOSTNAME "reject_unknown_client_hostname"
#define REJECT_UNKNOWN_REVERSE_HOSTNAME "reject_unknown_reverse_client_hostname"
#define REJECT_UNKNOWN_FORWARD_HOSTNAME "reject_unknown_forward_client_hostname"
#define VAR_UNK_CLIENT_CODE	"unknown_client_reject_code"
#define DEF_UNK_CLIENT_CODE	450
extern int var_unk_client_code;

#define PERMIT_INET_INTERFACES	"permit_inet_interfaces"

#define PERMIT_MYNETWORKS	"permit_mynetworks"

#define PERMIT_NAKED_IP_ADDR	"permit_naked_ip_address"

#define REJECT_INVALID_HELO_HOSTNAME	"reject_invalid_helo_hostname"
#define REJECT_INVALID_HOSTNAME	"reject_invalid_hostname"
#define VAR_BAD_NAME_CODE	"invalid_hostname_reject_code"
#define DEF_BAD_NAME_CODE	501	/* SYNTAX */
extern int var_bad_name_code;

#define REJECT_UNKNOWN_HELO_HOSTNAME "reject_unknown_helo_hostname"
#define REJECT_UNKNOWN_HOSTNAME	"reject_unknown_hostname"
#define VAR_UNK_NAME_CODE	"unknown_hostname_reject_code"
#define DEF_UNK_NAME_CODE	450
extern int var_unk_name_code;

#define REJECT_NON_FQDN_HELO_HOSTNAME "reject_non_fqdn_helo_hostname"
#define REJECT_NON_FQDN_HOSTNAME "reject_non_fqdn_hostname"
#define REJECT_NON_FQDN_SENDER	"reject_non_fqdn_sender"
#define REJECT_NON_FQDN_RCPT	"reject_non_fqdn_recipient"
#define VAR_NON_FQDN_CODE	"non_fqdn_reject_code"
#define DEF_NON_FQDN_CODE	504	/* POLICY */
extern int var_non_fqdn_code;

#define REJECT_UNKNOWN_SENDDOM	"reject_unknown_sender_domain"
#define REJECT_UNKNOWN_RCPTDOM	"reject_unknown_recipient_domain"
#define REJECT_UNKNOWN_ADDRESS	"reject_unknown_address"
#define REJECT_UNLISTED_SENDER	"reject_unlisted_sender"
#define REJECT_UNLISTED_RCPT	"reject_unlisted_recipient"
#define CHECK_RCPT_MAPS		"check_recipient_maps"

#define VAR_UNK_ADDR_CODE	"unknown_address_reject_code"
#define DEF_UNK_ADDR_CODE	450
extern int var_unk_addr_code;

#define VAR_SMTPD_REJ_UNL_FROM	"smtpd_reject_unlisted_sender"
#define DEF_SMTPD_REJ_UNL_FROM	0
extern bool var_smtpd_rej_unl_from;

#define VAR_SMTPD_REJ_UNL_RCPT	"smtpd_reject_unlisted_recipient"
#define DEF_SMTPD_REJ_UNL_RCPT	1
extern bool var_smtpd_rej_unl_rcpt;

#define REJECT_UNVERIFIED_RECIP "reject_unverified_recipient"
#define VAR_UNV_RCPT_CODE	"unverified_recipient_reject_code"
#define DEF_UNV_RCPT_CODE	450
extern int var_unv_rcpt_code;

#define REJECT_UNVERIFIED_SENDER "reject_unverified_sender"
#define VAR_UNV_FROM_CODE	"unverified_sender_reject_code"
#define DEF_UNV_FROM_CODE	450
extern int var_unv_from_code;

#define REJECT_MUL_RCPT_BOUNCE	"reject_multi_recipient_bounce"
#define VAR_MUL_RCPT_CODE	"multi_recipient_bounce_reject_code"
#define DEF_MUL_RCPT_CODE	550
extern int var_mul_rcpt_code;

#define PERMIT_AUTH_DEST	"permit_auth_destination"
#define REJECT_UNAUTH_DEST	"reject_unauth_destination"
#define CHECK_RELAY_DOMAINS	"check_relay_domains"
#define PERMIT_TLS_CLIENTCERTS	"permit_tls_clientcerts"
#define PERMIT_TLS_ALL_CLIENTCERTS	"permit_tls_all_clientcerts"
#define VAR_RELAY_CODE		"relay_domains_reject_code"
#define DEF_RELAY_CODE		554
extern int var_relay_code;

#define PERMIT_MX_BACKUP	"permit_mx_backup"

#define VAR_PERM_MX_NETWORKS	"permit_mx_backup_networks"
#define DEF_PERM_MX_NETWORKS	""
extern char *var_perm_mx_networks;

#define VAR_ACCESS_MAP_CODE	"access_map_reject_code"
#define DEF_ACCESS_MAP_CODE	554
extern int var_access_map_code;

#define CHECK_CLIENT_ACL	"check_client_access"
#define CHECK_CCERT_ACL		"check_ccert_access"
#define CHECK_HELO_ACL		"check_helo_access"
#define CHECK_SENDER_ACL	"check_sender_access"
#define CHECK_RECIP_ACL		"check_recipient_access"
#define CHECK_ETRN_ACL		"check_etrn_access"

#define CHECK_HELO_MX_ACL	"check_helo_mx_access"
#define CHECK_SENDER_MX_ACL	"check_sender_mx_access"
#define CHECK_RECIP_MX_ACL	"check_recipient_mx_access"
#define CHECK_HELO_NS_ACL	"check_helo_ns_access"
#define CHECK_SENDER_NS_ACL	"check_sender_ns_access"
#define CHECK_RECIP_NS_ACL	"check_recipient_ns_access"

#define WARN_IF_REJECT		"warn_if_reject"

#define REJECT_RBL		"reject_rbl"	/* LaMont compatibility */
#define REJECT_RBL_CLIENT	"reject_rbl_client"
#define REJECT_RHSBL_CLIENT	"reject_rhsbl_client"
#define REJECT_RHSBL_HELO	"reject_rhsbl_helo"
#define REJECT_RHSBL_SENDER	"reject_rhsbl_sender"
#define REJECT_RHSBL_RECIPIENT	"reject_rhsbl_recipient"

#define VAR_RBL_REPLY_MAPS	"rbl_reply_maps"
#define DEF_RBL_REPLY_MAPS	""
extern char *var_rbl_reply_maps;

#define VAR_DEF_RBL_REPLY	"default_rbl_reply"
#define DEF_DEF_RBL_REPLY	"$rbl_code Service unavailable; $rbl_class [$rbl_what] blocked using $rbl_domain${rbl_reason?; $rbl_reason}"
extern char *var_def_rbl_reply;

#define REJECT_MAPS_RBL		"reject_maps_rbl"	/* backwards compat */
#define VAR_MAPS_RBL_CODE	"maps_rbl_reject_code"
#define DEF_MAPS_RBL_CODE	554
extern int var_maps_rbl_code;

#define VAR_MAPS_RBL_DOMAINS	"maps_rbl_domains"	/* backwards compat */
#define DEF_MAPS_RBL_DOMAINS	""
extern char *var_maps_rbl_domains;

#define VAR_SMTPD_DELAY_REJECT	"smtpd_delay_reject"
#define DEF_SMTPD_DELAY_REJECT	1
extern int var_smtpd_delay_reject;

#define REJECT_UNAUTH_PIPE	"reject_unauth_pipelining"

#define VAR_SMTPD_NULL_KEY	"smtpd_null_access_lookup_key"
#define DEF_SMTPD_NULL_KEY	"<>"
extern char *var_smtpd_null_key;

#define VAR_SMTPD_EXP_FILTER	"smtpd_expansion_filter"
#define DEF_SMTPD_EXP_FILTER	"\\t\\40!\"#$%&'()*+,-./0123456789:;<=>?@\
ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\\\]^_`\
abcdefghijklmnopqrstuvwxyz{|}~"
extern char *var_smtpd_exp_filter;

#define VAR_SMTPD_PEERNAME_LOOKUP	"smtpd_peername_lookup"
#define DEF_SMTPD_PEERNAME_LOOKUP	1
extern bool var_smtpd_peername_lookup;

 /*
  * Heuristic to reject unknown local recipients at the SMTP port.
  */
#define VAR_LOCAL_RCPT_MAPS	"local_recipient_maps"
#define DEF_LOCAL_RCPT_MAPS	"proxy:unix:passwd.byname $" VAR_ALIAS_MAPS
extern char *var_local_rcpt_maps;

#define VAR_LOCAL_RCPT_CODE	"unknown_local_recipient_reject_code"
#define DEF_LOCAL_RCPT_CODE	550
extern int var_local_rcpt_code;

 /*
  * List of pre-approved maps that are OK to open with the proxymap service.
  */
#define VAR_PROXY_READ_MAPS	"proxy_read_maps"
#define DEF_PROXY_READ_MAPS	"$" VAR_LOCAL_RCPT_MAPS \
				" $" VAR_MYDEST \
				" $" VAR_VIRT_ALIAS_MAPS \
				" $" VAR_VIRT_ALIAS_DOMS \
				" $" VAR_VIRT_MAILBOX_MAPS \
				" $" VAR_VIRT_MAILBOX_DOMS \
				" $" VAR_RELAY_RCPT_MAPS \
				" $" VAR_RELAY_DOMAINS \
				" $" VAR_CANONICAL_MAPS \
				" $" VAR_SEND_CANON_MAPS \
				" $" VAR_RCPT_CANON_MAPS \
				" $" VAR_RELOCATED_MAPS \
				" $" VAR_TRANSPORT_MAPS \
				" $" VAR_MYNETWORKS
extern char *var_proxy_read_maps;

 /*
  * Other.
  */
#define VAR_PROCNAME		"process_name"
extern char *var_procname;

#define VAR_PID			"process_id"
extern int var_pid;

#define VAR_DEBUG_COMMAND	"debugger_command"

 /*
  * Paranoia: save files instead of deleting them.
  */
#define VAR_DONT_REMOVE		"dont_remove"
#define DEF_DONT_REMOVE		0
extern bool var_dont_remove;

 /*
  * Paranoia: defer messages instead of bouncing them.
  */
#define VAR_SOFT_BOUNCE		"soft_bounce"
#define DEF_SOFT_BOUNCE		0
extern bool var_soft_bounce;

 /*
  * Give special treatment to owner- and -request.
  */
#define VAR_OWNREQ_SPECIAL		"owner_request_special"
#define DEF_OWNREQ_SPECIAL		1
extern bool var_ownreq_special;

 /*
  * Allow/disallow recipient addresses starting with `-'.
  */
#define VAR_ALLOW_MIN_USER		"allow_min_user"
#define DEF_ALLOW_MIN_USER		0
extern bool var_allow_min_user;

extern void mail_params_init(void);

 /*
  * Content inspection and filtering.
  */
#define VAR_FILTER_XPORT		"content_filter"
#define DEF_FILTER_XPORT		""
extern char *var_filter_xport;

 /*
  * Fast flush service support.
  */
#define VAR_FFLUSH_DOMAINS		"fast_flush_domains"
#define DEF_FFLUSH_DOMAINS		"$relay_domains"
extern char *var_fflush_domains;

#define VAR_FFLUSH_PURGE		"fast_flush_purge_time"
#define DEF_FFLUSH_PURGE		"7d"
extern int var_fflush_purge;

#define VAR_FFLUSH_REFRESH		"fast_flush_refresh_time"
#define DEF_FFLUSH_REFRESH		"12h"
extern int var_fflush_refresh;

 /*
  * Environmental management - what Postfix imports from the external world,
  * and what Postfix exports to the external world.
  */
#define VAR_IMPORT_ENVIRON		"import_environment"
#define DEF_IMPORT_ENVIRON		"MAIL_CONFIG MAIL_DEBUG MAIL_LOGTAG TZ XAUTHORITY DISPLAY LANG=C"
extern char *var_import_environ;

#define VAR_EXPORT_ENVIRON		"export_environment"
#define DEF_EXPORT_ENVIRON		"TZ MAIL_CONFIG LANG"
extern char *var_export_environ;

 /*
  * Tunables for the "virtual" local delivery agent
  */
#define VAR_VIRT_TRANSPORT		"virtual_transport"
#define DEF_VIRT_TRANSPORT		MAIL_SERVICE_VIRTUAL
extern char *var_virt_transport;

#define VAR_VIRT_MAILBOX_MAPS		"virtual_mailbox_maps"
#define DEF_VIRT_MAILBOX_MAPS		""
extern char *var_virt_mailbox_maps;

#define VAR_VIRT_MAILBOX_DOMS		"virtual_mailbox_domains"
#define DEF_VIRT_MAILBOX_DOMS		"$virtual_mailbox_maps"
extern char *var_virt_mailbox_doms;

#define VAR_VIRT_MAILBOX_CODE		"unknown_virtual_mailbox_reject_code"
#define DEF_VIRT_MAILBOX_CODE		550
extern int var_virt_mailbox_code;

#define VAR_VIRT_UID_MAPS		"virtual_uid_maps"
#define DEF_VIRT_UID_MAPS		""
extern char *var_virt_uid_maps;

#define VAR_VIRT_GID_MAPS		"virtual_gid_maps"
#define DEF_VIRT_GID_MAPS		""
extern char *var_virt_gid_maps;

#define VAR_VIRT_MINUID			"virtual_minimum_uid"
#define DEF_VIRT_MINUID			100
extern int var_virt_minimum_uid;

#define VAR_VIRT_MAILBOX_BASE		"virtual_mailbox_base"
#define DEF_VIRT_MAILBOX_BASE		""
extern char *var_virt_mailbox_base;

#define VAR_VIRT_MAILBOX_LIMIT		"virtual_mailbox_limit"
#define DEF_VIRT_MAILBOX_LIMIT		(5 * DEF_MESSAGE_LIMIT)
extern int var_virt_mailbox_limit;

#define VAR_VIRT_MAILBOX_LOCK		"virtual_mailbox_lock"
#define DEF_VIRT_MAILBOX_LOCK		"fcntl, dotlock"
extern char *var_virt_mailbox_lock;

 /*
  * Distinct logging tag for multiple Postfix instances.
  */
#define VAR_SYSLOG_NAME			"syslog_name"
#define DEF_SYSLOG_NAME			"postfix"
extern char *var_syslog_name;

 /*
  * QMQPD
  */
#define VAR_QMQPD_CLIENTS		"qmqpd_authorized_clients"
#define DEF_QMQPD_CLIENTS		""
extern char *var_qmqpd_clients;

#define VAR_QMTPD_TMOUT			"qmqpd_timeout"
#define DEF_QMTPD_TMOUT			"300s"
extern int var_qmqpd_timeout;

#define VAR_QMTPD_ERR_SLEEP		"qmqpd_error_delay"
#define DEF_QMTPD_ERR_SLEEP		"1s"
extern int var_qmqpd_err_sleep;

 /*
  * VERP, more DJB intellectual cross-pollination. However, we prefer + as
  * the default recipient delimiter.
  */
#define VAR_VERP_DELIMS			"default_verp_delimiters"
#define DEF_VERP_DELIMS			"+="
extern char *var_verp_delims;

#define VAR_VERP_FILTER			"verp_delimiter_filter"
#define DEF_VERP_FILTER			"-=+"
extern char *var_verp_filter;

#define VAR_VERP_BOUNCE_OFF		"disable_verp_bounces"
#define DEF_VERP_BOUNCE_OFF		0
extern bool var_verp_bounce_off;

#define VAR_VERP_CLIENTS		"smtpd_authorized_verp_clients"
#define DEF_VERP_CLIENTS		"$authorized_verp_clients"
extern char *var_verp_clients;

 /*
  * XCLIENT, for rule testing and fetchmail like apps.
  */
#define VAR_XCLIENT_HOSTS		"smtpd_authorized_xclient_hosts"
#define DEF_XCLIENT_HOSTS		""
extern char *var_xclient_hosts;

 /*
  * XFORWARD, for improved post-filter logging.
  */
#define VAR_XFORWARD_HOSTS		"smtpd_authorized_xforward_hosts"
#define DEF_XFORWARD_HOSTS		""
extern char *var_xforward_hosts;

 /*
  * Inbound mail flow control. This allows for a stiffer coupling between
  * receiving mail and sending mail. A sending process produces one token for
  * each message that it takes from the incoming queue; a receiving process
  * consumes one token for each message that it adds to the incoming queue.
  * When no token is available (Postfix receives more mail than it is able to
  * deliver) a receiving process pauses for $in_flow_delay seconds so that
  * the sending processes get a chance to access the disk.
  */
#define VAR_IN_FLOW_DELAY			"in_flow_delay"
#ifdef PIPES_CANT_FIONREAD
#define DEF_IN_FLOW_DELAY			"0s"
#else
#define DEF_IN_FLOW_DELAY			"1s"
#endif
extern int var_in_flow_delay;

 /*
  * Backwards compatibility: foo.com matches itself and names below foo.com.
  */
#define VAR_PAR_DOM_MATCH		"parent_domain_matches_subdomains"
#define DEF_PAR_DOM_MATCH		VAR_DEBUG_PEER_LIST "," \
					VAR_FFLUSH_DOMAINS "," \
					VAR_MYNETWORKS "," \
					VAR_PERM_MX_NETWORKS "," \
					VAR_QMQPD_CLIENTS "," \
					VAR_RELAY_DOMAINS "," \
					SMTPD_ACCESS_MAPS
extern char *var_par_dom_match;

#define SMTPD_ACCESS_MAPS		"smtpd_access_maps"

 /*
  * Run-time fault injection.
  */
#define VAR_FAULT_INJ_CODE		"fault_injection_code"
#define DEF_FAULT_INJ_CODE		0
extern int var_fault_inj_code;

 /*
  * Install/upgrade information.
  */
#define VAR_SENDMAIL_PATH		"sendmail_path"
#ifndef DEF_SENDMAIL_PATH
#define DEF_SENDMAIL_PATH		"/usr/sbin/sendmail"
#endif

#define VAR_MAILQ_PATH			"mailq_path"
#ifndef DEF_MAILQ_PATH
#define DEF_MAILQ_PATH			"/usr/bin/mailq"
#endif

#define VAR_NEWALIAS_PATH		"newaliases_path"
#ifndef DEF_NEWALIAS_PATH
#define DEF_NEWALIAS_PATH		"/usr/bin/newaliases"
#endif

#define VAR_MANPAGE_DIR			"manpage_directory"
#ifndef DEF_MANPAGE_DIR
#define DEF_MANPAGE_DIR			"/usr/local/man"
#endif

#define VAR_SAMPLE_DIR			"sample_directory"
#ifndef DEF_SAMPLE_DIR
#define DEF_SAMPLE_DIR			DEF_CONFIG_DIR
#endif

#define VAR_README_DIR			"readme_directory"
#ifndef DEF_README_DIR
#define DEF_README_DIR			"no"
#endif

#define VAR_HTML_DIR			"html_directory"
#ifndef DEF_HTML_DIR
#define DEF_HTML_DIR			"no"
#endif

 /*
  * Safety: resolve the address with unquoted localpart (default, but
  * technically incorrect), instead of resolving the address with quoted
  * localpart (technically correct, but unsafe). The default prevents mail
  * relay loopholes with "user@domain"@domain when relaying mail to a
  * Sendmail system.
  */
#define VAR_RESOLVE_DEQUOTED		"resolve_dequoted_address"
#define DEF_RESOLVE_DEQUOTED		1
extern bool var_resolve_dequoted;

#define VAR_RESOLVE_NULLDOM		"resolve_null_domain"
#define DEF_RESOLVE_NULLDOM		0
extern bool var_resolve_nulldom;

#define VAR_RESOLVE_NUM_DOM		"resolve_numeric_domain"
#define DEF_RESOLVE_NUM_DOM		0
extern bool var_resolve_num_dom;

 /*
  * Service names. The transport (TCP, FIFO or UNIX-domain) type is frozen
  * because you cannot simply mix them, and accessibility (private/public) is
  * frozen for security reasons. We list only the internal services, not the
  * externally visible SMTP server, or the delivery agents that can already
  * be chosen via transport mappings etc.
  */
#define VAR_BOUNCE_SERVICE		"bounce_service_name"
#define DEF_BOUNCE_SERVICE		MAIL_SERVICE_BOUNCE
extern char *var_bounce_service;

#define VAR_CLEANUP_SERVICE		"cleanup_service_name"
#define DEF_CLEANUP_SERVICE		MAIL_SERVICE_CLEANUP
extern char *var_cleanup_service;

#define VAR_DEFER_SERVICE		"defer_service_name"
#define DEF_DEFER_SERVICE		MAIL_SERVICE_DEFER
extern char *var_defer_service;

#define VAR_PICKUP_SERVICE		"pickup_service_name"
#define DEF_PICKUP_SERVICE		MAIL_SERVICE_PICKUP
extern char *var_pickup_service;

#define VAR_QUEUE_SERVICE		"queue_service_name"
#define DEF_QUEUE_SERVICE		MAIL_SERVICE_QUEUE
extern char *var_queue_service;

 /* XXX resolve does not exist as a separate service */

#define VAR_REWRITE_SERVICE		"rewrite_service_name"
#define DEF_REWRITE_SERVICE		MAIL_SERVICE_REWRITE
extern char *var_rewrite_service;

#define VAR_SHOWQ_SERVICE		"showq_service_name"
#define DEF_SHOWQ_SERVICE		MAIL_SERVICE_SHOWQ
extern char *var_showq_service;

#define VAR_ERROR_SERVICE		"error_service_name"
#define DEF_ERROR_SERVICE		MAIL_SERVICE_ERROR
extern char *var_error_service;

#define VAR_FLUSH_SERVICE		"flush_service_name"
#define DEF_FLUSH_SERVICE		MAIL_SERVICE_FLUSH
extern char *var_flush_service;

 /*
  * Session cache service.
  */
#define VAR_SCACHE_SERVICE		"connection_cache_service_name"
#define DEF_SCACHE_SERVICE		"scache"
extern char *var_scache_service;

#define VAR_SCACHE_PROTO_TMOUT		"connection_cache_protocol_timeout"
#define DEF_SCACHE_PROTO_TMOUT		"5s"
extern int var_scache_proto_tmout;

#define VAR_SCACHE_TTL_LIM		"connection_cache_ttl_limit"
#define DEF_SCACHE_TTL_LIM		"2s"
extern int var_scache_ttl_lim;

#define VAR_SCACHE_STAT_TIME		"connection_cache_status_update_time"
#define DEF_SCACHE_STAT_TIME		"600s"
extern int var_scache_stat_time;

 /*
  * Address verification service.
  */
#define VAR_VERIFY_SERVICE		"address_verify_service_name"
#define DEF_VERIFY_SERVICE		MAIL_SERVICE_VERIFY
extern char *var_verify_service;

#define VAR_VERIFY_MAP			"address_verify_map"
#define DEF_VERIFY_MAP			""
extern char *var_verify_map;

#define VAR_VERIFY_POS_EXP		"address_verify_positive_expire_time"
#define DEF_VERIFY_POS_EXP		"31d"
extern int var_verify_pos_exp;

#define VAR_VERIFY_POS_TRY		"address_verify_positive_refresh_time"
#define DEF_VERIFY_POS_TRY		"7d"
extern int var_verify_pos_try;

#define VAR_VERIFY_NEG_EXP		"address_verify_negative_expire_time"
#define DEF_VERIFY_NEG_EXP		"3d"
extern int var_verify_neg_exp;

#define VAR_VERIFY_NEG_TRY		"address_verify_negative_refresh_time"
#define DEF_VERIFY_NEG_TRY		"3h"
extern int var_verify_neg_try;

#define VAR_VERIFY_NEG_CACHE		"address_verify_negative_cache"
#define DEF_VERIFY_NEG_CACHE		1
extern bool var_verify_neg_cache;

#define VAR_VERIFY_SENDER		"address_verify_sender"
#define DEF_VERIFY_SENDER		"postmaster"
extern char *var_verify_sender;

#define VAR_VERIFY_POLL_COUNT		"address_verify_poll_count"
#define DEF_VERIFY_POLL_COUNT		3
extern int var_verify_poll_count;

#define VAR_VERIFY_POLL_DELAY		"address_verify_poll_delay"
#define DEF_VERIFY_POLL_DELAY		"3s"
extern int var_verify_poll_delay;

#define VAR_VRFY_LOCAL_XPORT		"address_verify_local_transport"
#define DEF_VRFY_LOCAL_XPORT		"$" VAR_LOCAL_TRANSPORT
extern char *var_vrfy_local_xport;

#define VAR_VRFY_VIRT_XPORT		"address_verify_virtual_transport"
#define DEF_VRFY_VIRT_XPORT		"$" VAR_VIRT_TRANSPORT
extern char *var_vrfy_virt_xport;

#define VAR_VRFY_RELAY_XPORT		"address_verify_relay_transport"
#define DEF_VRFY_RELAY_XPORT		"$" VAR_RELAY_TRANSPORT
extern char *var_vrfy_relay_xport;

#define VAR_VRFY_DEF_XPORT		"address_verify_default_transport"
#define DEF_VRFY_DEF_XPORT		"$" VAR_DEF_TRANSPORT
extern char *var_vrfy_def_xport;

#define VAR_VRFY_RELAYHOST		"address_verify_relayhost"
#define DEF_VRFY_RELAYHOST		"$" VAR_RELAYHOST
extern char *var_vrfy_relayhost;

#define VAR_VRFY_RELAY_MAPS		"address_verify_sender_dependent_relayhost_maps"
#define DEF_VRFY_RELAY_MAPS		"$" VAR_SND_RELAY_MAPS
extern char *var_vrfy_relay_maps;

#define VAR_VRFY_XPORT_MAPS		"address_verify_transport_maps"
#define DEF_VRFY_XPORT_MAPS		"$" VAR_TRANSPORT_MAPS
extern char *var_vrfy_xport_maps;

 /*
  * Message delivery trace service.
  */
#define VAR_TRACE_SERVICE		"trace_service_name"
#define DEF_TRACE_SERVICE		MAIL_SERVICE_TRACE
extern char *var_trace_service;

 /*
  * Mailbox/maildir delivery errors that cause delivery to be tried again.
  */
#define VAR_MBX_DEFER_ERRS		"mailbox_defer_errors"
#define DEF_MBX_DEFER_ERRS		"eagain, enospc, estale"
extern char *var_mbx_defer_errs;

#define VAR_MDR_DEFER_ERRS		"maildir_defer_errors"
#define DEF_MDR_DEFER_ERRS		"enospc, estale"
extern char *var_mdr_defer_errs;

 /*
  * Berkeley DB memory pool sizes.
  */
#define	VAR_DB_CREATE_BUF		"berkeley_db_create_buffer_size"
#define DEF_DB_CREATE_BUF		(16 * 1024 *1024)
extern int var_db_create_buf;

#define	VAR_DB_READ_BUF			"berkeley_db_read_buffer_size"
#define DEF_DB_READ_BUF			(128 *1024)
extern int var_db_read_buf;

 /*
  * Named queue file attributes.
  */
#define VAR_QATTR_COUNT_LIMIT		"queue_file_attribute_count_limit"
#define DEF_QATTR_COUNT_LIMIT		100
extern int var_qattr_count_limit;

 /*
  * MIME support.
  */
#define VAR_MIME_MAXDEPTH		"mime_nesting_limit"
#define DEF_MIME_MAXDEPTH		100
extern int var_mime_maxdepth;

#define VAR_MIME_BOUND_LEN		"mime_boundary_length_limit"
#define DEF_MIME_BOUND_LEN		2048
extern int var_mime_bound_len;

#define VAR_DISABLE_MIME_INPUT		"disable_mime_input_processing"
#define DEF_DISABLE_MIME_INPUT		0
extern bool var_disable_mime_input;

#define VAR_DISABLE_MIME_OCONV		"disable_mime_output_conversion"
#define DEF_DISABLE_MIME_OCONV		0
extern bool var_disable_mime_oconv;

#define VAR_STRICT_8BITMIME		"strict_8bitmime"
#define DEF_STRICT_8BITMIME		0
extern bool var_strict_8bitmime;

#define VAR_STRICT_7BIT_HDRS		"strict_7bit_headers"
#define DEF_STRICT_7BIT_HDRS		0
extern bool var_strict_7bit_hdrs;

#define VAR_STRICT_8BIT_BODY		"strict_8bitmime_body"
#define DEF_STRICT_8BIT_BODY		0
extern bool var_strict_8bit_body;

#define VAR_STRICT_ENCODING		"strict_mime_encoding_domain"
#define DEF_STRICT_ENCODING		0
extern bool var_strict_encoding;

 /*
  * Bizarre.
  */
#define VAR_SENDER_ROUTING		"sender_based_routing"
#define DEF_SENDER_ROUTING		0
extern bool var_sender_routing;

#define VAR_XPORT_NULL_KEY	"transport_null_address_lookup_key"
#define DEF_XPORT_NULL_KEY	"<>"
extern char *var_xport_null_key;

 /*
  * Bounce service controls.
  */
#define VAR_OLDLOG_COMPAT		"backwards_bounce_logfile_compatibility"
#define DEF_OLDLOG_COMPAT		1
extern bool var_oldlog_compat;

 /*
  * SMTPD content proxy.
  */
#define VAR_SMTPD_PROXY_FILT		"smtpd_proxy_filter"
#define DEF_SMTPD_PROXY_FILT		""
extern char *var_smtpd_proxy_filt;

#define VAR_SMTPD_PROXY_EHLO		"smtpd_proxy_ehlo"
#define DEF_SMTPD_PROXY_EHLO		"$" VAR_MYHOSTNAME
extern char *var_smtpd_proxy_ehlo;

#define VAR_SMTPD_PROXY_TMOUT		"smtpd_proxy_timeout"
#define DEF_SMTPD_PROXY_TMOUT		"100s"
extern int var_smtpd_proxy_tmout;

 /*
  * Transparency options for mail input interfaces and for the cleanup server
  * behind them. These should turn off stuff we don't want to happen, because
  * the default is to do a lot of things.
  */
#define VAR_INPUT_TRANSP		"receive_override_options"
#define DEF_INPUT_TRANSP		""
extern char *var_smtpd_input_transp;

 /*
  * SMTP server policy delegation.
  */
#define VAR_SMTPD_POLICY_TMOUT		"smtpd_policy_service_timeout"
#define DEF_SMTPD_POLICY_TMOUT		"100s"
extern int var_smtpd_policy_tmout;

#define VAR_SMTPD_POLICY_IDLE		"smtpd_policy_service_max_idle"
#define DEF_SMTPD_POLICY_IDLE		"300s"
extern int var_smtpd_policy_idle;

#define VAR_SMTPD_POLICY_TTL		"smtpd_policy_service_max_ttl"
#define DEF_SMTPD_POLICY_TTL		"1000s"
extern int var_smtpd_policy_ttl;

#define CHECK_POLICY_SERVICE		"check_policy_service"

 /*
  * Client rate control.
  */
#define VAR_SMTPD_CRATE_LIMIT		"smtpd_client_connection_rate_limit"
#define DEF_SMTPD_CRATE_LIMIT		0
extern int var_smtpd_crate_limit;

#define VAR_SMTPD_CCONN_LIMIT		"smtpd_client_connection_count_limit"
#define DEF_SMTPD_CCONN_LIMIT		((DEF_PROC_LIMIT + 1) / 2)
extern int var_smtpd_cconn_limit;

#define VAR_SMTPD_CMAIL_LIMIT		"smtpd_client_message_rate_limit"
#define DEF_SMTPD_CMAIL_LIMIT		0
extern int var_smtpd_cmail_limit;

#define VAR_SMTPD_CRCPT_LIMIT		"smtpd_client_recipient_rate_limit"
#define DEF_SMTPD_CRCPT_LIMIT		0
extern int var_smtpd_crcpt_limit;

#define VAR_SMTPD_CNTLS_LIMIT		"smtpd_client_new_tls_session_rate_limit"
#define DEF_SMTPD_CNTLS_LIMIT		0
extern int var_smtpd_cntls_limit;

#define VAR_SMTPD_HOGGERS		"smtpd_client_event_limit_exceptions"
#define DEF_SMTPD_HOGGERS		"${smtpd_client_connection_limit_exceptions:$" VAR_MYNETWORKS "}"
extern char *var_smtpd_hoggers;

#define VAR_ANVIL_TIME_UNIT		"anvil_rate_time_unit"
#define DEF_ANVIL_TIME_UNIT		"60s"
extern int var_anvil_time_unit;

#define VAR_ANVIL_STAT_TIME		"anvil_status_update_time"
#define DEF_ANVIL_STAT_TIME		"600s"
extern int var_anvil_stat_time;

 /*
  * Temporary stop gap.
  */
#if 0
#include <anvil_clnt.h>

#define VAR_ANVIL_SERVICE		"client_connection_rate_service_name"
#define DEF_ANVIL_SERVICE		"local:" ANVIL_CLASS "/" ANVIL_SERVICE
extern char *var_anvil_service;

#endif

 /*
  * What domain names to assume when no valid domain context exists.
  */
#define VAR_REM_RWR_DOMAIN		"remote_header_rewrite_domain"
#define DEF_REM_RWR_DOMAIN		""
extern char *var_remote_rwr_domain;

#define CHECK_ADDR_MAP			"check_address_map"

#define VAR_LOC_RWR_CLIENTS		"local_header_rewrite_clients"
#define DEF_LOC_RWR_CLIENTS		PERMIT_INET_INTERFACES
extern char *var_local_rwr_clients;

 /*
  * EHLO keyword filter.
  */
#define VAR_SMTPD_EHLO_DIS_WORDS	"smtpd_discard_ehlo_keywords"
#define DEF_SMTPD_EHLO_DIS_WORDS	""
extern char *var_smtpd_ehlo_dis_words;

#define VAR_SMTPD_EHLO_DIS_MAPS		"smtpd_discard_ehlo_keyword_address_maps"
#define DEF_SMTPD_EHLO_DIS_MAPS		""
extern char *var_smtpd_ehlo_dis_maps;

#define VAR_SMTP_EHLO_DIS_WORDS		"smtp_discard_ehlo_keywords"
#define DEF_SMTP_EHLO_DIS_WORDS		""
#define VAR_LMTP_EHLO_DIS_WORDS		"lmtp_discard_lhlo_keywords"
#define DEF_LMTP_EHLO_DIS_WORDS		""
extern char *var_smtp_ehlo_dis_words;

#define VAR_SMTP_EHLO_DIS_MAPS		"smtp_discard_ehlo_keyword_address_maps"
#define DEF_SMTP_EHLO_DIS_MAPS		""
#define VAR_LMTP_EHLO_DIS_MAPS		"lmtp_discard_lhlo_keyword_address_maps"
#define DEF_LMTP_EHLO_DIS_MAPS		""
extern char *var_smtp_ehlo_dis_maps;

 /*
  * gcc workaround for warnings about empty or null format strings.
  */
extern const char null_format_string[1];

 /*
  * Characters to reject or strip.
  */
#define VAR_MSG_REJECT_CHARS		"message_reject_characters"
#define DEF_MSG_REJECT_CHARS		""
extern char *var_msg_reject_chars;

#define VAR_MSG_STRIP_CHARS		"message_strip_characters"
#define DEF_MSG_STRIP_CHARS		""
extern char *var_msg_strip_chars;

 /*
  * Local forwarding complexity controls.
  */
#define VAR_FROZEN_DELIVERED		"frozen_delivered_to"
#define DEF_FROZEN_DELIVERED		1
extern bool var_frozen_delivered;

 /*
  * Delay logging time roundup.
  */
#define VAR_DELAY_MAX_RES		"delay_logging_resolution_limit"
#define MAX_DELAY_MAX_RES		6
#define DEF_DELAY_MAX_RES		2
#define MIN_DELAY_MAX_RES		0
extern int var_delay_max_res;

 /*
  * Bounce message templates.
  */
#define VAR_BOUNCE_TMPL			"bounce_template_file"
#define DEF_BOUNCE_TMPL			""
extern char *var_bounce_tmpl;

 /*
  * Sender-dependent authentication.
  */
#define VAR_SMTP_SENDER_AUTH	"smtp_sender_dependent_authentication"
#define DEF_SMTP_SENDER_AUTH	0
#define VAR_LMTP_SENDER_AUTH	"lmtp_sender_dependent_authentication"
#define DEF_LMTP_SENDER_AUTH	0
extern bool var_smtp_sender_auth;

 /*
  * Allow CNAME lookup result to override the server hostname.
  */
#define VAR_SMTP_CNAME_OVERR		"smtp_cname_overrides_servername"
#define DEF_SMTP_CNAME_OVERR		0
#define VAR_LMTP_CNAME_OVERR		"lmtp_cname_overrides_servername"
#define DEF_LMTP_CNAME_OVERR		0
extern bool var_smtp_cname_overr;

 /*
  * TLS cipherlists
  */
#define VAR_TLS_HIGH_CLIST	"tls_high_cipherlist"
#define DEF_TLS_HIGH_CLIST	"ALL:!EXPORT:!LOW:!MEDIUM:+RC4:@STRENGTH"
extern char *var_tls_high_clist;

#define VAR_TLS_MEDIUM_CLIST	"tls_medium_cipherlist"
#define DEF_TLS_MEDIUM_CLIST	"ALL:!EXPORT:!LOW:+RC4:@STRENGTH"
extern char *var_tls_medium_clist;

#define VAR_TLS_LOW_CLIST	"tls_low_cipherlist"
#define DEF_TLS_LOW_CLIST	"ALL:!EXPORT:+RC4:@STRENGTH"
extern char *var_tls_low_clist;

#define VAR_TLS_EXPORT_CLIST	"tls_export_cipherlist"
#define DEF_TLS_EXPORT_CLIST	"ALL:+RC4:@STRENGTH"
extern char *var_tls_export_clist;

#define VAR_TLS_NULL_CLIST	"tls_null_cipherlist"
#define DEF_TLS_NULL_CLIST	"eNULL:!aNULL"
extern char *var_tls_null_clist;

 /*
  * Sendmail-style mail filter support.
  */
#define VAR_SMTPD_MILTERS		"smtpd_milters"
#define DEF_SMTPD_MILTERS		""
extern char *var_smtpd_milters;

#define VAR_CLEANUP_MILTERS		"non_smtpd_milters"
#define DEF_CLEANUP_MILTERS		""
extern char *var_cleanup_milters;

#define VAR_MILT_DEF_ACTION		"milter_default_action"
#define DEF_MILT_DEF_ACTION		"tempfail"
extern char *var_milt_def_action;

#define VAR_MILT_CONN_MACROS		"milter_connect_macros"
#define DEF_MILT_CONN_MACROS		"j {daemon_name} v"
extern char *var_milt_conn_macros;

#define VAR_MILT_HELO_MACROS		"milter_helo_macros"
#define DEF_MILT_HELO_MACROS		"{tls_version} {cipher} {cipher_bits}" \
					" {cert_subject} {cert_issuer}"
extern char *var_milt_helo_macros;

#define VAR_MILT_MAIL_MACROS		"milter_mail_macros"
#define DEF_MILT_MAIL_MACROS		"i {auth_type} {auth_authen}" \
					" {auth_author} {mail_addr}"
extern char *var_milt_mail_macros;

#define VAR_MILT_RCPT_MACROS		"milter_rcpt_macros"
#define DEF_MILT_RCPT_MACROS		"i {rcpt_addr}"
extern char *var_milt_rcpt_macros;

#define VAR_MILT_DATA_MACROS		"milter_data_macros"
#define DEF_MILT_DATA_MACROS		"i"
extern char *var_milt_data_macros;

#define VAR_MILT_UNK_MACROS		"milter_unknown_command_macros"
#define DEF_MILT_UNK_MACROS		""
extern char *var_milt_unk_macros;

#define VAR_MILT_EOD_MACROS		"milter_end_of_data_macros"
#define DEF_MILT_EOD_MACROS		"i"
extern char *var_milt_eod_macros;

#define VAR_MILT_CONN_TIME		"milter_connect_timeout"
#define DEF_MILT_CONN_TIME		"30s"
extern int var_milt_conn_time;

#define VAR_MILT_CMD_TIME		"milter_command_timeout"
#define DEF_MILT_CMD_TIME		"30s"
extern int var_milt_cmd_time;

#define VAR_MILT_MSG_TIME		"milter_content_timeout"
#define DEF_MILT_MSG_TIME		"300s"
extern int var_milt_msg_time;

#define VAR_MILT_PROTOCOL		"milter_protocol"
#define DEF_MILT_PROTOCOL		"2"
extern char *var_milt_protocol;

#define VAR_MILT_DEF_ACTION		"milter_default_action"
#define DEF_MILT_DEF_ACTION		"tempfail"
extern char *var_milt_def_action;

#define VAR_MILT_DAEMON_NAME		"milter_macro_daemon_name"
#define DEF_MILT_DAEMON_NAME		"$" VAR_MYHOSTNAME
extern char *var_milt_daemon_name;

#define VAR_MILT_V			"milter_macro_v"
#define DEF_MILT_V			"$" VAR_MAIL_NAME " $" VAR_MAIL_VERSION
extern char *var_milt_v;

 /*
  * What internal mail do we inspect/stamp/etc.? This is not yet safe enough
  * to enable world-wide.
  */
#define VAR_INT_FILT_CLASSES		"internal_mail_filter_classes"
#define DEF_INT_FILT_CLASSES		""
extern char *var_int_filt_classes;

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

#endif
