/*++
/* NAME
/*	cleanup_init 3
/* SUMMARY
/*	cleanup callable interface, initializations
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	CONFIG_BOOL_TABLE cleanup_bool_table[];
/*
/*	CONFIG_INT_TABLE cleanup_int_table[];
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
/*
/*	void	cleanup_all()
/* DESCRIPTION
/*	This module implements a callable interface to the cleanup service
/*	for one-time initializations that must be done before any message
/*	processing can take place.
/*
/*	cleanup_{bool,int,str,time}_table[] specify configuration
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
/*	by cleanup_all() to remove incomplete files after a fatal error.
/*
/*	cleanup_all() must be called in case of fatal error, in order
/*	to remove an incomplete queue file. Normally, as part of process
/*	initialization, one registers a msg_cleanup() handler and a signal()
/*	handler that both call cleanup_all() before terminating the process.
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

/* Utility library. */

#include <msg.h>
#include <iostuff.h>
#include <name_mask.h>

/* Global library. */

#include <mail_addr.h>
#include <mail_params.h>
#include <ext_prop.h>

/* Application-specific. */

#include "cleanup.h"

 /*
  * Global state: any queue files that we have open, so that the error
  * handler can clean up in case of trouble.
  */
char   *cleanup_path;			/* queue file name */

 /*
  * Tunable parameters.
  */
int     var_hopcount_limit;		/* max mailer hop count */
int     var_header_limit;		/* max header length */
char   *var_canonical_maps;		/* common canonical maps */
char   *var_send_canon_maps;		/* sender canonical maps */
char   *var_rcpt_canon_maps;		/* recipient canonical maps */
char   *var_virtual_maps;		/* virtual maps */
char   *var_masq_domains;		/* masquerade domains */
char   *var_masq_exceptions;		/* users not masqueraded */
char   *var_header_checks;		/* any header checks */
char   *var_body_checks;		/* any body checks */
int     var_dup_filter_limit;		/* recipient dup filter */
char   *var_empty_addr;			/* destination of bounced bounces */
int     var_delay_warn_time;		/* delay that triggers warning */
char   *var_prop_extension;		/* propagate unmatched extension */
char   *var_always_bcc;			/* big brother */
int     var_extra_rcpt_limit;		/* recipient extract limit */
char   *var_rcpt_witheld;		/* recipients not disclosed */
bool    var_canon_env_rcpt;		/* canonicalize envelope recipient */
char   *var_masq_classes;		/* what to masquerade */

CONFIG_INT_TABLE cleanup_int_table[] = {
    VAR_HOPCOUNT_LIMIT, DEF_HOPCOUNT_LIMIT, &var_hopcount_limit, 1, 0,
    VAR_HEADER_LIMIT, DEF_HEADER_LIMIT, &var_header_limit, 1, 0,
    VAR_DUP_FILTER_LIMIT, DEF_DUP_FILTER_LIMIT, &var_dup_filter_limit, 0, 0,
    VAR_EXTRA_RCPT_LIMIT, DEF_EXTRA_RCPT_LIMIT, &var_extra_rcpt_limit, 0, 0,
    0,
};

CONFIG_TIME_TABLE cleanup_time_table[] = {
    VAR_DELAY_WARN_TIME, DEF_DELAY_WARN_TIME, &var_delay_warn_time, 0, 0,
    0,
};

CONFIG_STR_TABLE cleanup_str_table[] = {
    VAR_CANONICAL_MAPS, DEF_CANONICAL_MAPS, &var_canonical_maps, 0, 0,
    VAR_SEND_CANON_MAPS, DEF_SEND_CANON_MAPS, &var_send_canon_maps, 0, 0,
    VAR_RCPT_CANON_MAPS, DEF_RCPT_CANON_MAPS, &var_rcpt_canon_maps, 0, 0,
    VAR_VIRTUAL_MAPS, DEF_VIRTUAL_MAPS, &var_virtual_maps, 0, 0,
    VAR_MASQ_DOMAINS, DEF_MASQ_DOMAINS, &var_masq_domains, 0, 0,
    VAR_EMPTY_ADDR, DEF_EMPTY_ADDR, &var_empty_addr, 1, 0,
    VAR_MASQ_EXCEPTIONS, DEF_MASQ_EXCEPTIONS, &var_masq_exceptions, 0, 0,
    VAR_HEADER_CHECKS, DEF_HEADER_CHECKS, &var_header_checks, 0, 0,
    VAR_BODY_CHECKS, DEF_BODY_CHECKS, &var_body_checks, 0, 0,
    VAR_PROP_EXTENSION, DEF_PROP_EXTENSION, &var_prop_extension, 0, 0,
    VAR_ALWAYS_BCC, DEF_ALWAYS_BCC, &var_always_bcc, 0, 0,
    VAR_RCPT_WITHELD, DEF_RCPT_WITHELD, &var_rcpt_witheld, 1, 0,
    VAR_MASQ_CLASSES, DEF_MASQ_CLASSES, &var_masq_classes, 0, 0,
    0,
};

CONFIG_BOOL_TABLE cleanup_bool_table[] = {
    VAR_CANON_ENV_RCPT, DEF_CANON_ENV_RCPT, &var_canon_env_rcpt,
    0,
};

 /*
  * Mappings.
  */
MAPS   *cleanup_comm_canon_maps;
MAPS   *cleanup_send_canon_maps;
MAPS   *cleanup_rcpt_canon_maps;
MAPS   *cleanup_header_checks;
MAPS   *cleanup_body_checks;
MAPS   *cleanup_virtual_maps;
ARGV   *cleanup_masq_domains;
int     cleanup_masq_flags;

 /*
  * Address extension propagation restrictions.
  */
int     cleanup_ext_prop_mask;

/* cleanup_all - callback for the runtime error handler */

void    cleanup_all(void)
{
    if (cleanup_path && REMOVE(cleanup_path))
	msg_warn("cleanup_all: remove %s: %m", cleanup_path);
}

/* cleanup_pre_jail - initialize before entering the chroot jail */

void    cleanup_pre_jail(char *unused_name, char **unused_argv)
{
    static NAME_MASK masq_class_table[] = {
	MASQ_CLASS_ENV_FROM, CLEANUP_MASQ_FLAG_ENV_FROM,
	MASQ_CLASS_ENV_RCPT, CLEANUP_MASQ_FLAG_ENV_RCPT,
	MASQ_CLASS_HDR_FROM, CLEANUP_MASQ_FLAG_HDR_FROM,
	MASQ_CLASS_HDR_RCPT, CLEANUP_MASQ_FLAG_HDR_RCPT,
	0,
    };

    if (*var_canonical_maps)
	cleanup_comm_canon_maps =
	    maps_create(VAR_CANONICAL_MAPS, var_canonical_maps, DICT_FLAG_LOCK);
    if (*var_send_canon_maps)
	cleanup_send_canon_maps =
	    maps_create(VAR_SEND_CANON_MAPS, var_send_canon_maps,
			DICT_FLAG_LOCK);
    if (*var_rcpt_canon_maps)
	cleanup_rcpt_canon_maps =
	    maps_create(VAR_RCPT_CANON_MAPS, var_rcpt_canon_maps,
			DICT_FLAG_LOCK);
    if (*var_virtual_maps)
	cleanup_virtual_maps = maps_create(VAR_VIRTUAL_MAPS, var_virtual_maps,
					   DICT_FLAG_LOCK);
    if (*var_masq_domains)
	cleanup_masq_domains = argv_split(var_masq_domains, " ,\t\r\n");
    if (*var_header_checks)
	cleanup_header_checks =
	    maps_create(VAR_HEADER_CHECKS, var_header_checks, DICT_FLAG_LOCK);
    if (*var_body_checks)
	cleanup_body_checks =
	    maps_create(VAR_BODY_CHECKS, var_body_checks, DICT_FLAG_LOCK);
    if (*var_masq_classes)
	cleanup_masq_flags = name_mask(VAR_MASQ_CLASSES, masq_class_table,
				       var_masq_classes);
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
    cleanup_ext_prop_mask = ext_prop_mask(var_prop_extension);
}
