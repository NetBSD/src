/*	$NetBSD: mail_flush.c,v 1.1.1.1 2009/06/23 10:08:46 tron Exp $	*/

/*++
/* NAME
/*	mail_flush 3
/* SUMMARY
/*	flush backed up mail
/* SYNOPSIS
/*	#include <mail_flush.h>
/*
/*	int	mail_flush_deferred()
/*
/*	int	mail_flush_maildrop()
/* DESCRIPTION
/*	This module triggers delivery of backed up mail.
/*
/*	mail_flush_deferred() triggers delivery of all deferred
/*	or incoming mail. This function tickles the queue manager.
/*
/*	mail_flush_maildrop() triggers delivery of all mail in
/*	the maildrop directory. This function tickles the pickup
/*	service.
/* DIAGNOSTICS
/*	The result is 0 in case of success, -1 in case of failure.
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

#include "sys_defs.h"

/* Utility library. */

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <mail_flush.h>

/* mail_flush_deferred - flush deferred/incoming queue */

int     mail_flush_deferred(void)
{
    static char qmgr_trigger[] = {
	QMGR_REQ_FLUSH_DEAD,		/* all hosts, all transports */
	QMGR_REQ_SCAN_ALL,		/* all time stamps */
	QMGR_REQ_SCAN_DEFERRED,		/* scan deferred queue */
	QMGR_REQ_SCAN_INCOMING,		/* scan incoming queue */
    };

    /*
     * Trigger the flush queue service.
     */
    return (mail_trigger(MAIL_CLASS_PUBLIC, var_queue_service,
			 qmgr_trigger, sizeof(qmgr_trigger)));
}

/* mail_flush_maildrop - flush maildrop queue */

int     mail_flush_maildrop(void)
{
    static char wakeup[] = {TRIGGER_REQ_WAKEUP};

    /*
     * Trigger the pickup service.
     */
    return (mail_trigger(MAIL_CLASS_PUBLIC, var_pickup_service,
			 wakeup, sizeof(wakeup)));
}
