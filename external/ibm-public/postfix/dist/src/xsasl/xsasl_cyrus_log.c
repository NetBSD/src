/*	$NetBSD: xsasl_cyrus_log.c,v 1.1.1.1 2009/06/23 10:09:02 tron Exp $	*/

/*++
/* NAME
/*	xsasl_cyrus_log 3
/* SUMMARY
/*	Cyrus SASL logging call-back routine
/* SYNOPSIS
/*	#include <xsasl_cyrus_common.h>
/*
/*	int	xsasl_cyrus_log(context, priority, text)
/*	void	*context;
/*	int	priority;
/*	const char *text;
/* DESCRIPTION
/*	xsasl_cyrus_log() logs a Cyrus message.
/* DIAGNOSTICS:
/*	Fatal: out of memory.
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

/* Application-specific */

#include <xsasl_cyrus_common.h>

#if defined(USE_SASL_AUTH) && defined(USE_CYRUS_SASL)

#include <sasl.h>
#include <saslutil.h>

/* xsasl_cyrus_log - logging callback */

int     xsasl_cyrus_log(void *unused_context, int priority,
			        const char *message)
{
    switch (priority) {
	case SASL_LOG_ERR:		/* unusual errors */
#ifdef SASL_LOG_WARN			/* non-fatal warnings (Cyrus-SASL v2) */
	case SASL_LOG_WARN:
#endif
#ifdef SASL_LOG_WARNING			/* non-fatal warnings (Cyrus-SASL v1) */
	case SASL_LOG_WARNING:
#endif
	msg_warn("SASL authentication problem: %s", message);
	break;
#ifdef SASL_LOG_INFO
    case SASL_LOG_INFO:			/* other info (Cyrus-SASL v1) */
	if (msg_verbose)
	    msg_info("SASL authentication info: %s", message);
	break;
#endif
#ifdef SASL_LOG_NOTE
    case SASL_LOG_NOTE:			/* other info (Cyrus-SASL v2) */
	if (msg_verbose)
	    msg_info("SASL authentication info: %s", message);
	break;
#endif
#ifdef SASL_LOG_FAIL
    case SASL_LOG_FAIL:			/* authentication failures
						 * (Cyrus-SASL v2) */
	msg_warn("SASL authentication failure: %s", message);
	break;
#endif
#ifdef SASL_LOG_DEBUG
    case SASL_LOG_DEBUG:			/* more verbose than LOG_NOTE
						 * (Cyrus-SASL v2) */
	if (msg_verbose > 1)
	    msg_info("SASL authentication debug: %s", message);
	break;
#endif
#ifdef SASL_LOG_TRACE
    case SASL_LOG_TRACE:			/* traces of internal
						 * protocols (Cyrus-SASL v2) */
	if (msg_verbose > 1)
	    msg_info("SASL authentication trace: %s", message);
	break;
#endif
#ifdef SASL_LOG_PASS
    case SASL_LOG_PASS:			/* traces of internal
						 * protocols, including
						 * passwords (Cyrus-SASL v2) */
	if (msg_verbose > 1)
	    msg_info("SASL authentication pass: %s", message);
	break;
#endif
    }
    return (SASL_OK);
}

#endif
