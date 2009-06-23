/*	$NetBSD: cleanup_init.c,v 1.1.1.1 2009/06/23 10:08:43 tron Exp $	*/

/*++
/* NAME
/*	cleanup_init 3
/* SUMMARY
/*	cleanup callable interface, initializations
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	CONFIG_INT_TABLE cleanup_int_table[];
/*
/*	CONFIG_BOOL_TABLE cleanup_bool_table[];
/*
/*	CONFIG_STR_TABLE cleanup_str_table[];
/*
/*	CONFIG_TIME_TABLE cleanup_time_table[];
/*
/*	void	cleanup_pre_jail(service_name, argv)
/*	char	*service_name;
/*	char	**argv;
/*
/*	void	cleanup_post_jail(service_name, argv)
/*	char	*service_name;
/*	char	**argv;
/*
/*	char	*cleanup_path;
/*	VSTRING	*cleanup_trace_path;
/*
/*	void	cleanup_all()
/*
/*	void	cleanup_sig(sigval)
/*	int	sigval;
/* DESCRIPTION
/*	This module implements a callable interface to the cleanup service
/*	for one-time initializations that must be done before any message
/*	processing can take place.
/*
/*	cleanup_{int,str,time}_table[] specify configuration
/*	parameters that must be initialized before calling any functions
/*	in this module. These tables satisfy the interface as specified in
/*	single_service(3).
/*
/*	cleanup_pre_jail() and cleanup_post_jail() perform mandatory
/*	initializations before and after the process enters the optional
/*	chroot jail. These functions satisfy the interface as specified
/*	in single_service(3).
/*
/*	cleanup_path is either a null pointer or it is the name of a queue
/*	file that currently is being written. This information is used
/*	by cleanup_all() to remove incomplete files after a fatal error,
/*	or by cleanup_sig() after arrival of a SIGTERM signal.
/*
/*	cleanup_trace_path is either a null pointer or the pathname of a
/*	trace logfile with DSN SUCCESS notifications. This information is
/*	used to remove a trace file when the mail transaction is canceled.
/*
/*	cleanup_all() must be called in case of fatal error, in order
/*	to remove an incomplete queue file.
/*
/*	cleanup_sig() must be called in case of SIGTERM, in order
/*	to remove an incomplete queue file.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* SEE ALSO
/*	cleanup_api(3) cleanup callable interface, message processing
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
#include <signal.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <iostuff.h>
#include <name_mask.h>
#include <stringops.h>

/* Global library. */

#include <mail_addr.h>
#include <mail_params.h>
#include <mail_version.h>		/* milter_macro_v */
#include <ext_prop.h>
#include <flush_clnt.h>

/* Application-specific. */

#include "cleanup.h"

 /*
  * Global state: any queue files that we have open, so that the error
  * handler can clean up in case of trouble.
  */
char   *cleanup_path;			/* queue file name */

 /*
  * Another piece of global state: pathnames of partial bounce or trace
  * logfiles that need to be cleaned up when the cleanup request is aborted.
  */
VSTRING *cleanup_trace_path;

 /*
  * Tunable parameters.
  */
int     var_hopcount_limit;		/* max mailer hop count */
char   *var_canonical_maps;		/* common canonical maps */
char   *var_send_canon_maps;		/* sender canonical maps */
char   *var_rcpt_canon_maps;		/* recipient canonical maps */
char   *var_canon_classes;		/* what to canonicalize */
char   *var_send_canon_classes;		/* what sender to canonicalize */
char   *var_rcpt_canon_classes;		/* what recipient to canonicalize */
char   *var_virt_alias_maps;		/* virtual alias maps */
char   *var_masq_domains;		/* masquerade domains */
char   *var_masq_exceptions;		/* users not masqueraded */
char   *var_header_checks;		/* primary header checks */
char   *var_mimehdr_checks;		/* mime header checks */
char   *var_nesthdr_checks;		/* nested header checks */
char   *var_body_checks;		/* any body checks */
int     var_dup_filter_limit;		/* recipient dup filter */
bool    var_enable_orcpt;		/* Include orcpt in dup filter? */
char   *var_empty_addr;			/* destination of bounced bounces */
int     var_delay_warn_time;		/* delay that triggers warning */
char   *var_prop_extension;		/* propagate unmatched extension */
char   *var_always_bcc;			/* big brother */
char   *var_rcpt_witheld;		/* recipients not disclosed */
char   *var_masq_classes;		/* what to masquerade */
int     var_qattr_count_limit;		/* named attribute limit */
int     var_virt_recur_limit;		/* maximum virtual alias recursion */
int     var_virt_expan_limit;		/* maximum virtual alias expansion */
int     var_body_check_len;		/* when to stop body scan */
char   *var_send_bcc_maps;		/* sender auto-bcc maps */
char   *var_rcpt_bcc_maps;		/* recipient auto-bcc maps */
char   *var_remote_rwr_domain;		/* header-only surrogate */
char   *var_msg_reject_chars;		/* reject these characters */
char   *var_msg_strip_chars;		/* strip these characters */
int     var_verp_bounce_off;		/* don't verp the bounces */
int     var_milt_conn_time;		/* milter connect/handshake timeout */
int     var_milt_cmd_time;		/* milter command timeout */
int     var_milt_msg_time;		/* milter content timeout */
char   *var_milt_protocol;		/* Sendmail 8 milter protocol */
char   *var_milt_def_action;		/* default milter action */
char   *var_milt_daemon_name;		/* {daemon_name} macro value */
char   *var_milt_v;			/* {v} macro value */
char   *var_milt_conn_macros;		/* connect macros */
char   *var_milt_helo_macros;		/* HELO macros */
char   *var_milt_mail_macros;		/* MAIL FROM macros */
char   *var_milt_rcpt_macros;		/* RCPT TO macros */
char   *var_milt_data_macros;		/* DATA macros */
char   *var_milt_eoh_macros;		/* end-of-header macros */
char   *var_milt_eod_macros;		/* end-of-data macros */
char   *var_milt_unk_macros;		/* unknown command macros */
char   *var_cleanup_milters;		/* non-SMTP mail */
int     var_auto_8bit_enc_hdr;		/* auto-detect 8bit encoding header */
int     var_always_add_hdrs;		/* always add missing headers */

CONFIG_INT_TABLE cleanup_int_table[] = {
    VAR_HOPCOUNT_LIMIT, DEF_HOPCOUNT_LIMIT, &var_hopcount_limit, 1, 0,
    VAR_DUP_FILTER_LIMIT, DEF_DUP_FILTER_LIMIT, &var_dup_filter_limit, 0, 0,
    VAR_QATTR_COUNT_LIMIT, DEF_QATTR_COUNT_LIMIT, &var_qattr_count_limit, 1, 0,
    VAR_VIRT_RECUR_LIMIT, DEF_VIRT_RECUR_LIMIT, &var_virt_recur_limit, 1, 0,
    VAR_VIRT_EXPAN_LIMIT, DEF_VIRT_EXPAN_LIMIT, &var_virt_expan_limit, 1, 0,
    VAR_BODY_CHECK_LEN, DEF_BODY_CHECK_LEN, &var_body_check_len, 0, 0,
    0,
};

CONFIG_BOOL_TABLE cleanup_bool_table[] = {
    VAR_ENABLE_ORCPT, DEF_ENABLE_ORCPT, &var_enable_orcpt,
    VAR_VERP_BOUNCE_OFF, DEF_VERP_BOUNCE_OFF, &var_verp_bounce_off,
    VAR_AUTO_8BIT_ENC_HDR, DEF_AUTO_8BIT_ENC_HDR, &var_auto_8bit_enc_hdr,
    VAR_ALWAYS_ADD_HDRS, DEF_ALWAYS_ADD_HDRS, &var_always_add_hdrs,
    0,
};

CONFIG_TIME_TABLE cleanup_time_table[] = {
    VAR_DELAY_WARN_TIME, DEF_DELAY_WARN_TIME, &var_delay_warn_time, 0, 0,
    VAR_MILT_CONN_TIME, DEF_MILT_CONN_TIME, &var_milt_conn_time, 1, 0,
    VAR_MILT_CMD_TIME, DEF_MILT_CMD_TIME, &var_milt_cmd_time, 1, 0,
    VAR_MILT_MSG_TIME, DEF_MILT_MSG_TIME, &var_milt_msg_time, 1, 0,
    0,
};

CONFIG_STR_TABLE cleanup_str_table[] = {
    VAR_CANONICAL_MAPS, DEF_CANONICAL_MAPS, &var_canonical_maps, 0, 0,
    VAR_SEND_CANON_MAPS, DEF_SEND_CANON_MAPS, &var_send_canon_maps, 0, 0,
    VAR_RCPT_CANON_MAPS, DEF_RCPT_CANON_MAPS, &var_rcpt_canon_maps, 0, 0,
    VAR_CANON_CLASSES, DEF_CANON_CLASSES, &var_canon_classes, 1, 0,
    VAR_SEND_CANON_CLASSES, DEF_SEND_CANON_CLASSES, &var_send_canon_classes, 1, 0,
    VAR_RCPT_CANON_CLASSES, DEF_RCPT_CANON_CLASSES, &var_rcpt_canon_classes, 1, 0,
    VAR_VIRT_ALIAS_MAPS, DEF_VIRT_ALIAS_MAPS, &var_virt_alias_maps, 0, 0,
    VAR_MASQ_DOMAINS, DEF_MASQ_DOMAINS, &var_masq_domains, 0, 0,
    VAR_EMPTY_ADDR, DEF_EMPTY_ADDR, &var_empty_addr, 1, 0,
    VAR_MASQ_EXCEPTIONS, DEF_MASQ_EXCEPTIONS, &var_masq_exceptions, 0, 0,
    VAR_HEADER_CHECKS, DEF_HEADER_CHECKS, &var_header_checks, 0, 0,
    VAR_MIMEHDR_CHECKS, DEF_MIMEHDR_CHECKS, &var_mimehdr_checks, 0, 0,
    VAR_NESTHDR_CHECKS, DEF_NESTHDR_CHECKS, &var_nesthdr_checks, 0, 0,
    VAR_BODY_CHECKS, DEF_BODY_CHECKS, &var_body_checks, 0, 0,
    VAR_PROP_EXTENSION, DEF_PROP_EXTENSION, &var_prop_extension, 0, 0,
    VAR_ALWAYS_BCC, DEF_ALWAYS_BCC, &var_always_bcc, 0, 0,
    VAR_RCPT_WITHELD, DEF_RCPT_WITHELD, &var_rcpt_witheld, 0, 0,
    VAR_MASQ_CLASSES, DEF_MASQ_CLASSES, &var_masq_classes, 0, 0,
    VAR_SEND_BCC_MAPS, DEF_SEND_BCC_MAPS, &var_send_bcc_maps, 0, 0,
    VAR_RCPT_BCC_MAPS, DEF_RCPT_BCC_MAPS, &var_rcpt_bcc_maps, 0, 0,
    VAR_REM_RWR_DOMAIN, DEF_REM_RWR_DOMAIN, &var_remote_rwr_domain, 0, 0,
    VAR_MSG_REJECT_CHARS, DEF_MSG_REJECT_CHARS, &var_msg_reject_chars, 0, 0,
    VAR_MSG_STRIP_CHARS, DEF_MSG_STRIP_CHARS, &var_msg_strip_chars, 0, 0,
    VAR_MILT_PROTOCOL, DEF_MILT_PROTOCOL, &var_milt_protocol, 1, 0,
    VAR_MILT_DEF_ACTION, DEF_MILT_DEF_ACTION, &var_milt_def_action, 1, 0,
    VAR_MILT_DAEMON_NAME, DEF_MILT_DAEMON_NAME, &var_milt_daemon_name, 1, 0,
    VAR_MILT_V, DEF_MILT_V, &var_milt_v, 1, 0,
    VAR_MILT_CONN_MACROS, DEF_MILT_CONN_MACROS, &var_milt_conn_macros, 0, 0,
    VAR_MILT_HELO_MACROS, DEF_MILT_HELO_MACROS, &var_milt_helo_macros, 0, 0,
    VAR_MILT_MAIL_MACROS, DEF_MILT_MAIL_MACROS, &var_milt_mail_macros, 0, 0,
    VAR_MILT_RCPT_MACROS, DEF_MILT_RCPT_MACROS, &var_milt_rcpt_macros, 0, 0,
    VAR_MILT_DATA_MACROS, DEF_MILT_DATA_MACROS, &var_milt_data_macros, 0, 0,
    VAR_MILT_EOH_MACROS, DEF_MILT_EOH_MACROS, &var_milt_eoh_macros, 0, 0,
    VAR_MILT_EOD_MACROS, DEF_MILT_EOD_MACROS, &var_milt_eod_macros, 0, 0,
    VAR_MILT_UNK_MACROS, DEF_MILT_UNK_MACROS, &var_milt_unk_macros, 0, 0,
    VAR_CLEANUP_MILTERS, DEF_CLEANUP_MILTERS, &var_cleanup_milters, 0, 0,
    0,
};

 /*
  * Mappings.
  */
MAPS   *cleanup_comm_canon_maps;
MAPS   *cleanup_send_canon_maps;
MAPS   *cleanup_rcpt_canon_maps;
int     cleanup_comm_canon_flags;
int     cleanup_send_canon_flags;
int     cleanup_rcpt_canon_flags;
MAPS   *cleanup_header_checks;
MAPS   *cleanup_mimehdr_checks;
MAPS   *cleanup_nesthdr_checks;
MAPS   *cleanup_body_checks;
MAPS   *cleanup_virt_alias_maps;
ARGV   *cleanup_masq_domains;
STRING_LIST *cleanup_masq_exceptions;
int     cleanup_masq_flags;
MAPS   *cleanup_send_bcc_maps;
MAPS   *cleanup_rcpt_bcc_maps;

 /*
  * Character filters.
  */
VSTRING *cleanup_reject_chars;
VSTRING *cleanup_strip_chars;

 /*
  * Address extension propagation restrictions.
  */
int     cleanup_ext_prop_mask;

 /*
  * Milter support.
  */
MILTERS *cleanup_milters;

/* cleanup_all - callback for the runtime error handler */

void    cleanup_all(void)
{
    cleanup_sig(0);
}

/* cleanup_sig - callback for the SIGTERM handler */

void    cleanup_sig(int sig)
{

    /*
     * msg_fatal() is safe against calling itself recursively, but signals
     * need extra safety.
     * 
     * XXX While running as a signal handler, can't ask the memory manager to
     * release VSTRING storage.
     */
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN) {
	if (cleanup_trace_path) {
	    (void) REMOVE(vstring_str(cleanup_trace_path));
	    cleanup_trace_path = 0;
	}
	if (cleanup_path) {
	    (void) REMOVE(cleanup_path);
	    cleanup_path = 0;
	}
	if (sig)
	    _exit(sig);
    }
}

/* cleanup_pre_jail - initialize before entering the chroot jail */

void    cleanup_pre_jail(char *unused_name, char **unused_argv)
{
    static const NAME_MASK send_canon_class_table[] = {
	CANON_CLASS_ENV_FROM, CLEANUP_CANON_FLAG_ENV_FROM,
	CANON_CLASS_HDR_FROM, CLEANUP_CANON_FLAG_HDR_FROM,
	0,
    };
    static const NAME_MASK rcpt_canon_class_table[] = {
	CANON_CLASS_ENV_RCPT, CLEANUP_CANON_FLAG_ENV_RCPT,
	CANON_CLASS_HDR_RCPT, CLEANUP_CANON_FLAG_HDR_RCPT,
	0,
    };
    static const NAME_MASK canon_class_table[] = {
	CANON_CLASS_ENV_FROM, CLEANUP_CANON_FLAG_ENV_FROM,
	CANON_CLASS_ENV_RCPT, CLEANUP_CANON_FLAG_ENV_RCPT,
	CANON_CLASS_HDR_FROM, CLEANUP_CANON_FLAG_HDR_FROM,
	CANON_CLASS_HDR_RCPT, CLEANUP_CANON_FLAG_HDR_RCPT,
	0,
    };
    static const NAME_MASK masq_class_table[] = {
	MASQ_CLASS_ENV_FROM, CLEANUP_MASQ_FLAG_ENV_FROM,
	MASQ_CLASS_ENV_RCPT, CLEANUP_MASQ_FLAG_ENV_RCPT,
	MASQ_CLASS_HDR_FROM, CLEANUP_MASQ_FLAG_HDR_FROM,
	MASQ_CLASS_HDR_RCPT, CLEANUP_MASQ_FLAG_HDR_RCPT,
	0,
    };

    if (*var_canonical_maps)
	cleanup_comm_canon_maps =
	    maps_create(VAR_CANONICAL_MAPS, var_canonical_maps,
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
    if (*var_send_canon_maps)
	cleanup_send_canon_maps =
	    maps_create(VAR_SEND_CANON_MAPS, var_send_canon_maps,
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
    if (*var_rcpt_canon_maps)
	cleanup_rcpt_canon_maps =
	    maps_create(VAR_RCPT_CANON_MAPS, var_rcpt_canon_maps,
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
    if (*var_virt_alias_maps)
	cleanup_virt_alias_maps = maps_create(VAR_VIRT_ALIAS_MAPS,
					      var_virt_alias_maps,
					      DICT_FLAG_LOCK
					      | DICT_FLAG_FOLD_FIX);
    if (*var_canon_classes)
	cleanup_comm_canon_flags =
	    name_mask(VAR_CANON_CLASSES, canon_class_table,
		      var_canon_classes);
    if (*var_send_canon_classes)
	cleanup_send_canon_flags =
	    name_mask(VAR_CANON_CLASSES, send_canon_class_table,
		      var_send_canon_classes);
    if (*var_rcpt_canon_classes)
	cleanup_rcpt_canon_flags =
	    name_mask(VAR_CANON_CLASSES, rcpt_canon_class_table,
		      var_rcpt_canon_classes);
    if (*var_masq_domains)
	cleanup_masq_domains = argv_split(var_masq_domains, " ,\t\r\n");
    if (*var_header_checks)
	cleanup_header_checks =
	    maps_create(VAR_HEADER_CHECKS, var_header_checks, DICT_FLAG_LOCK);
    if (*var_mimehdr_checks)
	cleanup_mimehdr_checks =
	    maps_create(VAR_MIMEHDR_CHECKS, var_mimehdr_checks, DICT_FLAG_LOCK);
    if (*var_nesthdr_checks)
	cleanup_nesthdr_checks =
	    maps_create(VAR_NESTHDR_CHECKS, var_nesthdr_checks, DICT_FLAG_LOCK);
    if (*var_body_checks)
	cleanup_body_checks =
	    maps_create(VAR_BODY_CHECKS, var_body_checks, DICT_FLAG_LOCK);
    if (*var_masq_exceptions)
	cleanup_masq_exceptions =
	    string_list_init(MATCH_FLAG_NONE, var_masq_exceptions);
    if (*var_masq_classes)
	cleanup_masq_flags = name_mask(VAR_MASQ_CLASSES, masq_class_table,
				       var_masq_classes);
    if (*var_send_bcc_maps)
	cleanup_send_bcc_maps =
	    maps_create(VAR_SEND_BCC_MAPS, var_send_bcc_maps,
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
    if (*var_rcpt_bcc_maps)
	cleanup_rcpt_bcc_maps =
	    maps_create(VAR_RCPT_BCC_MAPS, var_rcpt_bcc_maps,
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
    if (*var_cleanup_milters)
	cleanup_milters = milter_create(var_cleanup_milters,
					var_milt_conn_time,
					var_milt_cmd_time,
					var_milt_msg_time,
					var_milt_protocol,
					var_milt_def_action,
					var_milt_conn_macros,
					var_milt_helo_macros,
					var_milt_mail_macros,
					var_milt_rcpt_macros,
					var_milt_data_macros,
					var_milt_eoh_macros,
					var_milt_eod_macros,
					var_milt_unk_macros);

    flush_init();
}

/* cleanup_post_jail - initialize after entering the chroot jail */

void    cleanup_post_jail(char *unused_name, char **unused_argv)
{

    /*
     * Optionally set the file size resource limit. XXX This limits the
     * message content to somewhat less than requested, because the total
     * queue file size also includes envelope information. Unless people set
     * really low limit, the difference is going to matter only when a queue
     * file has lots of recipients.
     */
    if (var_message_limit > 0)
	set_file_limit((off_t) var_message_limit);

    /*
     * Control how unmatched extensions are propagated.
     */
    cleanup_ext_prop_mask =
	ext_prop_mask(VAR_PROP_EXTENSION, var_prop_extension);

    /*
     * Setup the filters for characters that should be rejected, and for
     * characters that should be removed.
     */
    if (*var_msg_reject_chars) {
	cleanup_reject_chars = vstring_alloc(strlen(var_msg_reject_chars));
	unescape(cleanup_reject_chars, var_msg_reject_chars);
    }
    if (*var_msg_strip_chars) {
	cleanup_strip_chars = vstring_alloc(strlen(var_msg_strip_chars));
	unescape(cleanup_strip_chars, var_msg_strip_chars);
    }
}
