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
#define DEF_LOCAL_TRANSPORT	"local"
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

 /*
  * Masquerading (i.e. subdomain stripping).
  */
#define VAR_MASQ_DOMAINS	"masquerade_domains"
#define DEF_MASQ_DOMAINS	""
extern char *var_masq_domains;

#define VAR_MASQ_EXCEPTIONS	"masquerade_exceptions"
#define DEF_MASQ_EXCEPTIONS	""
extern char *var_masq_exceptions;

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
#define DEF_DAEMON_DIR		"$program_directory"
#endif
extern char *var_daemon_dir;

#define VAR_COMMAND_DIR		"command_directory"
#ifndef DEF_COMMAND_DIR
#define DEF_COMMAND_DIR		"$program_directory"
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

 /*
  * Preferred type of indexed files. The DEF_DB_TYPE macro value is system
  * dependent. It is defined in <sys_defs.h>.
  */
#define VAR_DB_TYPE		"default_database_type"
extern char *var_db_type;

 /*
  * Logging. Changing facility at run-time does not do much good, because
  * something may have to be logged before parameters are read from file.
  */
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
  * Standards violation: disable VRFY.
  */
#define VAR_DISABLE_VRFY_CMD	"disable_vrfy_command"
#define DEF_DISABLE_VRFY_CMD	0
extern bool var_disable_vrfy_cmd;

 /*
  * trivial rewrite/resolve service: mapping tables.
  */
#define VAR_VIRTUAL_MAPS	"virtual_maps"
#define DEF_VIRTUAL_MAPS	""
extern char *var_virtual_maps;

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
#define DEF_COMMAND_MAXTIME	1000
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

#define VAR_EXP_OWN_ALIAS		"expand_owner_alias"
#define DEF_EXP_OWN_ALIAS		0
extern bool var_exp_own_alias;

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
#define DEF_QUEUE_RUN_DELAY     1000

#define VAR_MIN_BACKOFF_TIME	"minimal_backoff_time"
#define DEF_MIN_BACKOFF_TIME    1000
extern int var_min_backoff_time;

#define VAR_MAX_BACKOFF_TIME	"maximal_backoff_time"
#define DEF_MAX_BACKOFF_TIME    4000
extern int var_max_backoff_time;

#define VAR_MAX_QUEUE_TIME	"maximal_queue_lifetime"
#define DEF_MAX_QUEUE_TIME	5
extern int var_max_queue_time;

#define VAR_DELAY_WARN_TIME	"delay_warning_time"
#define DEF_DELAY_WARN_TIME	0
extern int var_delay_warn_time;

#define VAR_QMGR_ACT_LIMIT	"qmgr_message_active_limit"
#define DEF_QMGR_ACT_LIMIT	1000
extern int var_qmgr_active_limit;

#define VAR_QMGR_RCPT_LIMIT	"qmgr_message_recipient_limit"
#define DEF_QMGR_RCPT_LIMIT	10000
extern int var_qmgr_rcpt_limit;

#define VAR_QMGR_FUDGE		"qmgr_fudge_factor"
#define DEF_QMGR_FUDGE		100
extern int var_qmgr_fudge;

#define VAR_QMGR_HOG		"qmgr_site_hog_factor"
#define DEF_QMGR_HOG		90
extern int var_qmgr_hog;

 /*
  * Queue manager: default destination concurrency levels.
  */
#define VAR_INIT_DEST_CON	"initial_destination_concurrency"
#define DEF_INIT_DEST_CON	5
extern int var_init_dest_concurrency;

#define VAR_DEST_CON_LIMIT	"default_destination_concurrency_limit"
#define _DEST_CON_LIMIT		"_destination_concurrency_limit"
#define DEF_DEST_CON_LIMIT	10
extern int var_dest_con_limit;

 /*
  * Queue manager: default number of recipients per transaction.
  */
#define VAR_DEST_RCPT_LIMIT	"default_destination_recipient_limit"
#define _DEST_RCPT_LIMIT	"_destination_recipient_limit"
#define DEF_DEST_RCPT_LIMIT	50
extern int var_dest_rcpt_limit;

#define VAR_LOCAL_RCPT_LIMIT	"local" _DEST_RCPT_LIMIT	/* XXX */
#define DEF_LOCAL_RCPT_LIMIT	1	/* XXX */

 /*
  * Queue manager: default delay before retrying a dead transport.
  */
#define VAR_XPORT_RETRY_TIME	"transport_retry_time"
#define DEF_XPORT_RETRY_TIME	60
extern int var_transport_retry_time;

 /*
  * Queue manager: what transports to defer delivery to.
  */
#define VAR_DEFER_XPORTS	"defer_transports"
#define DEF_DEFER_XPORTS	""
extern char *var_defer_xports;

 /*
  * Master: default process count limit per mail subsystem.
  */
#define VAR_PROC_LIMIT		"default_process_limit"
#define DEF_PROC_LIMIT		50
extern int var_proc_limit;

 /*
  * Master: default time to wait after service is throttled.
  */
#define VAR_THROTTLE_TIME	"service_throttle_time"
#define DEF_THROTTLE_TIME	60
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
#define DEF_MAX_IDLE		100
extern int var_idle_limit;

 /*
  * Any subsystem: default amount of time a mail subsystem keeps an internal
  * IPC connection before closing it.
  */
#define VAR_IPC_IDLE		"ipc_idle"
#define DEF_IPC_IDLE		100
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
#define DEF_HASH_QUEUE_NAMES	"defer"
extern char *var_hash_queue_names;

#define VAR_HASH_QUEUE_DEPTH	"hash_queue_depth"
#define DEF_HASH_QUEUE_DEPTH	2
extern int var_hash_queue_depth;

 /*
  * SMTP client. Timeouts inspired by RFC 1123. The SMTP recipient limit
  * determines how many recipient addresses the SMTP client sends along with
  * each message. Unfortunately, some mailers misbehave and disconnect (smap)
  * when given more recipients than they are willing to handle.
  */
#define VAR_BESTMX_TRANSP	"best_mx_transport"
#define DEF_BESTMX_TRANSP	""
extern char *var_bestmx_transp;

#define VAR_SMTP_CONN_TMOUT	"smtp_connect_timeout"
#define DEF_SMTP_CONN_TMOUT	0
extern int var_smtp_conn_tmout;

#define VAR_SMTP_HELO_TMOUT	"smtp_helo_timeout"
#define DEF_SMTP_HELO_TMOUT	300
extern int var_smtp_helo_tmout;

#define VAR_SMTP_MAIL_TMOUT	"smtp_mail_timeout"
#define DEF_SMTP_MAIL_TMOUT	300
extern int var_smtp_mail_tmout;

#define VAR_SMTP_RCPT_TMOUT	"smtp_rcpt_timeout"
#define DEF_SMTP_RCPT_TMOUT	300
extern int var_smtp_rcpt_tmout;

#define VAR_SMTP_DATA0_TMOUT	"smtp_data_init_timeout"
#define DEF_SMTP_DATA0_TMOUT	120
extern int var_smtp_data0_tmout;

#define VAR_SMTP_DATA1_TMOUT	"smtp_data_xfer_timeout"
#define DEF_SMTP_DATA1_TMOUT	180
extern int var_smtp_data1_tmout;

#define VAR_SMTP_DATA2_TMOUT	"smtp_data_done_timeout"
#define DEF_SMTP_DATA2_TMOUT	600
extern int var_smtp_data2_tmout;

#define VAR_SMTP_QUIT_TMOUT	"smtp_quit_timeout"
#define DEF_SMTP_QUIT_TMOUT	300
extern int var_smtp_quit_tmout;

#define VAR_SMTP_SKIP_4XX	"smtp_skip_4xx_greeting"
#define DEF_SMTP_SKIP_4XX	0
extern bool var_smtp_skip_4xx_greeting;

#define VAR_IGN_MX_LOOKUP_ERR	"ignore_mx_lookup_error"
#define DEF_IGN_MX_LOOKUP_ERR	0
extern bool var_ign_mx_lookup_err;

#define VAR_SKIP_QUIT_RESP	"smtp_skip_quit_response"
#define DEF_SKIP_QUIT_RESP	1
extern bool var_skip_quit_resp;

 /*
  * SMTP server. The soft error limit determines how many errors an SMTP
  * client may make before we start to slow down; the hard error limit
  * determines after how many client errors we disconnect.
  */
#define VAR_SMTPD_BANNER	"smtpd_banner"
#define DEF_SMTPD_BANNER	"$myhostname ESMTP $mail_name"
extern char *var_smtpd_banner;

#define VAR_SMTPD_TMOUT		"smtpd_timeout"
#define DEF_SMTPD_TMOUT		300
extern int var_smtpd_tmout;

#define VAR_SMTPD_RCPT_LIMIT	"smtpd_recipient_limit"
#define DEF_SMTPD_RCPT_LIMIT	1000
extern int var_smtpd_rcpt_limit;

#define VAR_SMTPD_SOFT_ERLIM	"smtpd_soft_error_limit"
#define DEF_SMTPD_SOFT_ERLIM	10
extern int var_smtpd_soft_erlim;

#define VAR_SMTPD_HARD_ERLIM	"smtpd_hard_error_limit"
#define DEF_SMTPD_HARD_ERLIM	100
extern int var_smtpd_hard_erlim;

#define VAR_SMTPD_ERR_SLEEP	"smtpd_error_sleep_time"
#define DEF_SMTPD_ERR_SLEEP	5
extern int var_smtpd_err_sleep;

#define VAR_SMTPD_JUNK_CMD	"smtpd_junk_command_limit"
#define DEF_SMTPD_JUNK_CMD	1000
extern int var_smtpd_junk_cmd_limit;

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

#define VAR_BODY_CHECKS		"body_checks"
#define DEF_BODY_CHECKS		""
extern char *var_body_checks;

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
#define DEF_FORK_DELAY		1
extern int var_fork_delay;

 /*
  * When locking a mailbox, how often to try and how long to wait.
  */
#define VAR_FLOCK_TRIES          "deliver_lock_attempts"
#define DEF_FLOCK_TRIES          5
extern int var_flock_tries;

#define VAR_FLOCK_DELAY          "deliver_lock_delay"
#define DEF_FLOCK_DELAY          1
extern int var_flock_delay;

#define VAR_FLOCK_STALE		"stale_lock_time"
#define DEF_FLOCK_STALE		500
extern int var_flock_stale;

#define VAR_MAILTOOL_COMPAT	"sun_mailtool_compatibility"
#define DEF_MAILTOOL_COMPAT	0
extern int var_mailtool_compat;

 /*
  * How long a daemon command may take to receive or deliver a message etc.
  * before we assume it is wegded (should never happen).
  */
#define VAR_DAEMON_TIMEOUT	"daemon_timeout"
#define DEF_DAEMON_TIMEOUT	18000
extern int var_daemon_timeout;

 /*
  * How long an intra-mail command may take before we assume the mail system
  * is in deadlock (should never happen).
  */
#define VAR_IPC_TIMEOUT		"ipc_timeout"
#define DEF_IPC_TIMEOUT		3600
extern int var_ipc_timeout;

 /*
  * Time limit on intra-mail triggers.
  */
#define VAR_TRIGGER_TIMEOUT	"trigger_timeout"
#define DEF_TRIGGER_TIMEOUT	10
extern int var_trigger_timeout;

 /*
  * SMTP server restrictions. What networks I am willing to relay from, what
  * domains I am willing to forward mail from or to, what clients I refuse to
  * talk to, and what domains I never want to see in the sender address.
  */
#define VAR_MYNETWORKS		"mynetworks"
extern char *var_mynetworks;

#define VAR_RELAY_DOMAINS	"relay_domains"
#define DEF_RELAY_DOMAINS	"$mydestination"
extern char *var_relay_domains;

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
#define DEF_RCPT_CHECKS		PERMIT_MYNETWORKS "," CHECK_RELAY_DOMAINS
extern char *var_rcpt_checks;

#define VAR_ETRN_CHECKS		"smtpd_etrn_restrictions"
#define DEF_ETRN_CHECKS		""
extern char *var_etrn_checks;

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

#define REJECT_UNKNOWN_CLIENT	"reject_unknown_client"
#define VAR_UNK_CLIENT_CODE	"unknown_client_reject_code"
#define DEF_UNK_CLIENT_CODE	450
extern int var_unk_client_code;

#define PERMIT_MYNETWORKS	"permit_mynetworks"

#define PERMIT_NAKED_IP_ADDR	"permit_naked_ip_address"

#define REJECT_INVALID_HOSTNAME	"reject_invalid_hostname"
#define VAR_BAD_NAME_CODE	"invalid_hostname_reject_code"
#define DEF_BAD_NAME_CODE	501
extern int var_bad_name_code;

#define REJECT_UNKNOWN_HOSTNAME	"reject_unknown_hostname"
#define VAR_UNK_NAME_CODE	"unknown_hostname_reject_code"
#define DEF_UNK_NAME_CODE	450
extern int var_unk_name_code;

#define REJECT_NON_FQDN_HOSTNAME "reject_non_fqdn_hostname"
#define REJECT_NON_FQDN_SENDER	"reject_non_fqdn_sender"
#define REJECT_NON_FQDN_RCPT	"reject_non_fqdn_recipient"
#define VAR_NON_FQDN_CODE	"non_fqdn_reject_code"
#define DEF_NON_FQDN_CODE	504
extern int var_non_fqdn_code;

#define REJECT_UNKNOWN_SENDDOM	"reject_unknown_sender_domain"
#define REJECT_UNKNOWN_RCPTDOM	"reject_unknown_recipient_domain"
#define REJECT_UNKNOWN_ADDRESS	"reject_unknown_address"
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

#define VAR_ACCESS_MAP_CODE	"access_map_reject_code"
#define DEF_ACCESS_MAP_CODE	554
extern int var_access_map_code;

#define CHECK_CLIENT_ACL	"check_client_access"
#define CHECK_HELO_ACL		"check_helo_access"
#define CHECK_SENDER_ACL	"check_sender_access"
#define CHECK_RECIP_ACL		"check_recipient_access"
#define CHECK_ETRN_ACL		"check_etrn_access"

#define REJECT_MAPS_RBL		"reject_maps_rbl"
#define VAR_MAPS_RBL_CODE	"maps_rbl_reject_code"
#define DEF_MAPS_RBL_CODE	554
extern int var_maps_rbl_code;

#define VAR_MAPS_RBL_DOMAINS	"maps_rbl_domains"
#define DEF_MAPS_RBL_DOMAINS	"rbl.maps.vix.com"
extern char *var_maps_rbl_domains;

#define VAR_SMTPD_DELAY_REJECT	"smtpd_delay_reject"
#define DEF_SMTPD_DELAY_REJECT	1
extern int var_smtpd_delay_reject;

#define REJECT_UNAUTH_PIPE	"reject_unauth_pipelining"

 /*
  * Heuristic to reject most unknown recipients at the SMTP port.
  */
#define VAR_LOCAL_RCPT_MAPS	"local_recipient_maps"
#define DEF_LOCAL_RCPT_MAPS	""
extern char *var_local_rcpt_maps;

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
