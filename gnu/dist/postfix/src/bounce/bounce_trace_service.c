/*++
/* NAME
/*	bounce_trace_service 3
/* SUMMARY
/*	send status report to sender, server side
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	int     bounce_trace_service(flags, queue_name, queue_id, encoding, sender)
/*	int	flags;
/*	char	*queue_name;
/*	char	*queue_id;
/*	char	*encoding;
/*	char	*sender;
/* DESCRIPTION
/*	This module implements the server side of the trace_flush()
/*	(send delivery notice) request. The logfile
/*	is removed after the notice is posted.
/*
/*	A status report includes a prelude with human-readable text,
/*	a DSN-style report, and the email message that was subject of
/*	the status report.
/*
/*	When a status report is sent, the sender address is the empty
/*	address.
/* DIAGNOSTICS
/*	Fatal error: error opening existing file. Warnings: corrupt
/*	message file. A corrupt message is saved to the "corrupt"
/*	queue for further inspection.
/* BUGS
/* SEE ALSO
/*	bounce(3) basic bounce service client interface
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
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstream.h>

/* Global library. */

#include <mail_params.h>
#include <mail_queue.h>
#include <post_mail.h>
#include <mail_addr.h>
#include <mail_error.h>

/* Application-specific. */

#include "bounce_service.h"

#define STR vstring_str

/* bounce_trace_service - send a delivery status notice */

int     bounce_trace_service(int unused_flags, char *service, char *queue_name,
			             char *queue_id, char *encoding,
			             char *recipient)
{
    BOUNCE_INFO *bounce_info;
    int     bounce_status = 1;
    VSTREAM *bounce;

    /*
     * Initialize. Open queue file, bounce log, etc.
     */
    bounce_info = bounce_mail_init(service, queue_name, queue_id,
				   encoding, BOUNCE_MSG_STATUS);

#define NULL_SENDER		MAIL_ADDR_EMPTY	/* special address */
#define NULL_TRACE_FLAGS	0
#define BOUNCE_ALL		0

    /*
     * Send a single bounce with a template message header, some boilerplate
     * text that pretends that we are a polite mail system, the text with
     * per-recipient status, and a copy of the original message.
     */
    if ((bounce = post_mail_fopen_nowait(NULL_SENDER, recipient,
					 CLEANUP_FLAG_MASK_INTERNAL,
					 NULL_TRACE_FLAGS)) != 0) {
	if (bounce_header(bounce, bounce_info, recipient) == 0
	    && bounce_boilerplate(bounce, bounce_info) == 0
	    && bounce_diagnostic_log(bounce, bounce_info) == 0
	    && bounce_header_dsn(bounce, bounce_info) == 0
	    && bounce_diagnostic_dsn(bounce, bounce_info) == 0)
	    bounce_original(bounce, bounce_info, BOUNCE_ALL);
	bounce_status = post_mail_fclose(bounce);
    }

    /*
     * Examine the completion status. Delete the trace log file only when the
     * status notice was posted successfully.
     */
    if (bounce_status == 0 && mail_queue_remove(service, queue_id)
	&& errno != ENOENT)
	msg_fatal("remove %s %s: %m", service, queue_id);

    /*
     * Cleanup.
     */
    bounce_mail_free(bounce_info);

    return (bounce_status);
}
