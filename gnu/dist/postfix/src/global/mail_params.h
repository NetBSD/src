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
#define DEF_NOTIFY_CLASSES	"resource,software"
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
#define DEF_SGID_GROUP		"postdrop"
extern char *var_sgid_group;
extern gid_t var_sgid_gid;

#define VAR_DEFAULT_PRIVS	"default_privs"
#define DEF_DEFAULT_PRIVS	"nobody"
extern char *var_default_privs;
extern uid_t var_default_uid;
extern gid_t var_default_gid;

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
#define DEF_MYDEST		"$myhostname, localhost.$mydomain"
extern char *var_mydest;

 /*
  * These are by default taken from the name service.
  */
#define VAR_MYHOSTNAME		"myhostname"	/* my hostname (fqdn) */
extern char *var_myhostname;

#define VAR_MYDOMAIN		"mydomain"	/* my domain name */
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
#define DEF_INET_INTERFACES	"all"
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

#define VAR_FALLBACK_RELAY	"fallback_relay"
#define DEF_FALLBACK_RELAY	""
extern char *var_fallback_relay;

#define VAR_DISABLE_DNS		"disable_dns_lookups"
#define DEF_DISABLE_DNS		0
extern bool var_disable_dns;

 /*
  * Location of the mail queue directory tree.
  */
#define VAR_QUEUE_DIR	"queue_directory"
#ifndef DEF_QUEUE_DIR
#define DEF_QUEUE_DIR	"/var/spool/postfix"
#endif
extern char *var_queue_dir;

 /*
  * Location of daemon programs.
  */
#define VAR_PROGRAM_DIR		"program_directory"
#ifndef DEF_PROGRAM_DIR
#define DEF_PROGRAM_DIR		"/usr/libexec/postfix"
#endif

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
  * Standards violation: send "250 AUTH=list" in order to accomodate broken
  * Microsoft clients.
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
#define DEF_ALLOW_COMMANDS	"alias,forward"
extern char *var_allow_commands;

#define VAR_COMMAND_MAXTIME	"command_time_limit"
#define DEF_COMMAND_MAXTIME	"1000s"
extern int var_command_maxtime;

#define VAR_ALLOW_FILES		"allow_mail_to_files"
#define DEF_ALLOW_FILES		"alias,forward"
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

#define VAR_FALLBACK_TRANSP	"fallback_transport"
#define DEF_FALLBACK_TRANSP	""
extern char *var_fallback_transport;

 /*
  * Local delivery: path to per-user forwarding file.
  */
#define VAR_FORWARD_PATH	"forward_path"
#define DEF_FORWARD_PATH	"$home/.forward${recipient_delimiter}${extension},$home/.forward"
extern char *var_forward_path;

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
#define DEF_QUEUE_RUN_DELAY     "1000s"

#define VAR_MIN_BACKOFF_TIME	"minimal_backoff_time"
#define DEF_MIN_BACKOFF_TIME    "1000s"
extern int var_min_backoff_time;

#define VAR_MAX_BACKOFF_TIME	"maximal_backoff_time"
#define DEF_MAX_BACKOFF_TIME    "4000s"
extern int var_max_backoff_time;

#define VAR_MAX_QUEUE_TIME	"maximal_queue_lifetime"
#define DEF_MAX_QUEUE_TIME	"5d"
extern int var_max_queue_time;

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
#define DEF_XPORT_RCPT_LIMIT	10000
extern int var_xport_rcpt_limit;

#define VAR_STACK_RCPT_LIMIT	"default_extra_recipient_limit"
#define _STACK_RCPT_LIMIT	"_extra_recipient_limit"
#define DEF_STACK_RCPT_LIMIT	1000
extern int var_stack_rcpt_limit;

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
  * Any subsystem: default amount of time a mail subsystem keeps an internal
  * IPC connection before closing it.
  */
#define VAR_IPC_IDLE		"ipc_idle"
#define DEF_IPC_IDLE		"100s"
extern int var_ipc_idle_limit;

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
#define DEF_HASH_QUEUE_NAMES	"incoming,active,deferred,bounce,defer,flush,hold"
extern char *var_hash_queue_names;

#define VAR_HASH_QUEUE_DEPTH	"hash_queue_depth"
#define DEF_HASH_QUEUE_DEPTH	1
extern int var_hash_queue_depth;

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

#define VAR_SMTP_CONN_TMOUT	"smtp_connect_timeout"
#define DEF_SMTP_CONN_TMOUT	"30s"
extern int var_smtp_conn_tmout;

#define VAR_SMTP_HELO_TMOUT	"smtp_helo_timeout"
#define DEF_SMTP_HELO_TMOUT	"300s"
extern int var_smtp_helo_tmout;

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

#define VAR_SMTP_QUIT_TMOUT	"smtp_quit_timeout"
#define DEF_SMTP_QUIT_TMOUT	"300s"
extern int var_smtp_quit_tmout;

#define VAR_SMTP_SKIP_4XX	"smtp_skip_4xx_greeting"
#define DEF_SMTP_SKIP_4XX	1
extern bool var_smtp_skip_4xx_greeting;

#define VAR_SMTP_SKIP_5XX	"smtp_skip_5xx_greeting"
#define DEF_SMTP_SKIP_5XX	1
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
extern char *var_smtp_bind_addr;

#define VAR_SMTP_HELO_NAME	"smtp_helo_name"
#define DEF_SMTP_HELO_NAME	"$myhostname"
extern char *var_smtp_helo_name;

#define VAR_SMTP_RAND_ADDR	"smtp_randomize_addresses"
#define DEF_SMTP_RAND_ADDR	1
extern bool var_smtp_rand_addr;

#define VAR_SMTP_LINE_LIMIT	"smtp_line_length_limit"
#define DEF_SMTP_LINE_LIMIT	990
extern int var_smtp_line_limit;

#define VAR_SMTP_PIX_THRESH	"smtp_pix_workaround_threshold_time"
#define DEF_SMTP_PIX_THRESH	"500s"
extern int var_smtp_pix_thresh;

#define VAR_SMTP_PIX_DELAY	"smtp_pix_workaround_delay_time"
#define DEF_SMTP_PIX_DELAY	"10s"
extern int var_smtp_pix_delay;

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

#define VAR_SMTPD_HIST_THRSH	"smtpd_history_flush_threshold"
#define DEF_SMTPD_HIST_THRSH	100
extern int var_smtpd_hist_thrsh;

#define VAR_SMTPD_NOOP_CMDS	"smtpd_noop_commands"
#define DEF_SMTPD_NOOP_CMDS	""
extern char *var_smtpd_noop_cmds;

 /*
  * SASL authentication support, SMTP server side.
  */
#define VAR_SMTPD_SASL_ENABLE	"smtpd_sasl_auth_enable"
#define DEF_SMTPD_SASL_ENABLE	0
extern bool var_smtpd_sasl_enable;

#define VAR_SMTPD_SASL_OPTS	"smtpd_sasl_security_options"
#define DEF_SMTPD_SASL_OPTS	"noanonymous"
extern char *var_smtpd_sasl_opts;

#define VAR_SMTPD_SASL_REALM	"smtpd_sasl_local_domain"
#define DEF_SMTPD_SASL_REALM	""
extern char *var_smtpd_sasl_realm;

#define VAR_SMTPD_SND_AUTH_MAPS	"smtpd_sender_login_maps"
#define DEF_SMTPD_SND_AUTH_MAPS	""
extern char *var_smtpd_snd_auth_maps;

#define REJECT_SENDER_LOGIN_MISMATCH	"reject_sender_login_mismatch"

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
#define DEF_LMTP_TCP_PORT	24
extern int var_lmtp_tcp_port;

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
#define DEF_LMTP_RSET_TMOUT	"300s"
extern int var_lmtp_rset_tmout;

#define VAR_LMTP_LHLO_TMOUT	"lmtp_lhlo_timeout"
#define DEF_LMTP_LHLO_TMOUT	"300s"
extern int var_lmtp_lhlo_tmout;

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

 /*
  * Cleanup service. Header info that exceeds $header_size_limit bytes forces
  * the start of the message body.
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

#define VAR_EXTRA_RCPT_LIMIT	"extract_recipient_limit"
#define DEF_EXTRA_RCPT_LIMIT	10240
extern int var_extra_rcpt_limit;

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

#define REJECT_UNKNOWN_CLIENT	"reject_unknown_client"
#define VAR_UNK_CLIENT_CODE	"unknown_client_reject_code"
#define DEF_UNK_CLIENT_CODE	450
extern int var_unk_client_code;

#define PERMIT_MYNETWORKS	"permit_mynetworks"

#define PERMIT_NAKED_IP_ADDR	"permit_naked_ip_address"

#define REJECT_INVALID_HOSTNAME	"reject_invalid_hostname"
#define VAR_BAD_NAME_CODE	"invalid_hostname_reject_code"
#define DEF_BAD_NAME_CODE	501	/* SYNTAX */
extern int var_bad_name_code;

#define REJECT_UNKNOWN_HOSTNAME	"reject_unknown_hostname"
#define VAR_UNK_NAME_CODE	"unknown_hostname_reject_code"
#define DEF_UNK_NAME_CODE	450
extern int var_unk_name_code;

#define REJECT_NON_FQDN_HOSTNAME "reject_non_fqdn_hostname"
#define REJECT_NON_FQDN_SENDER	"reject_non_fqdn_sender"
#define REJECT_NON_FQDN_RCPT	"reject_non_fqdn_recipient"
#define VAR_NON_FQDN_CODE	"non_fqdn_reject_code"
#define DEF_NON_FQDN_CODE	504	/* POLICY */
extern int var_non_fqdn_code;

#define REJECT_UNKNOWN_SENDDOM	"reject_unknown_sender_domain"
#define REJECT_UNKNOWN_RCPTDOM	"reject_unknown_recipient_domain"
#define REJECT_UNKNOWN_ADDRESS	"reject_unknown_address"
#define CHECK_RCPT_MAPS		"check_recipient_maps"
#define VAR_UNK_ADDR_CODE	"unknown_address_reject_code"
#define DEF_UNK_ADDR_CODE	450
extern int var_unk_addr_code;

#define PERMIT_AUTH_DEST	"permit_auth_destination"
#define REJECT_UNAUTH_DEST	"reject_unauth_destination"
#define CHECK_RELAY_DOMAINS	"check_relay_domains"
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
#define CHECK_HELO_ACL		"check_helo_access"
#define CHECK_SENDER_ACL	"check_sender_access"
#define CHECK_RECIP_ACL		"check_recipient_access"
#define CHECK_ETRN_ACL		"check_etrn_access"

#define WARN_IF_REJECT		"warn_if_reject"

#define REJECT_RBL		"reject_rbl"	/* LaMont compatibility */
#define REJECT_RBL_CLIENT	"reject_rbl_client"
#define REJECT_RHSBL_CLIENT	"reject_rhsbl_client"
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

 /*
  * Heuristic to reject unknown local recipients at the SMTP port.
  */
#define VAR_LOCAL_RCPT_MAPS	"local_recipient_maps"
#define DEF_LOCAL_RCPT_MAPS	"proxy:unix:passwd.byname $alias_maps"
extern char *var_local_rcpt_maps;

#define VAR_LOCAL_RCPT_CODE	"unknown_local_recipient_reject_code"
#define DEF_LOCAL_RCPT_CODE	550
extern int var_local_rcpt_code;

 /*
  * List of pre-approved maps that are OK to open with the proxymap service.
  */
#define VAR_PROXY_READ_MAPS     "proxy_read_maps"
#define DEF_PROXY_READ_MAPS     "$" VAR_LOCAL_RCPT_MAPS \
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
#define DEF_IMPORT_ENVIRON		"MAIL_CONFIG MAIL_DEBUG MAIL_LOGTAG TZ XAUTHORITY DISPLAY"
extern char *var_import_environ;

#define VAR_EXPORT_ENVIRON		"export_environment"
#define DEF_EXPORT_ENVIRON		"TZ MAIL_CONFIG"
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
#define DEF_VIRT_MAILBOX_LOCK		"fcntl"
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

#define VAR_VERP_CLIENTS		"authorized_verp_clients"
#define DEF_VERP_CLIENTS		"$mynetworks"
extern char *var_verp_clients;

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
