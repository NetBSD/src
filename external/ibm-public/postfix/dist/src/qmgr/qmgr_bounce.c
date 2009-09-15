/*	$NetBSD: qmgr_bounce.c,v 1.1.1.1.2.2 2009/09/15 06:03:31 snj Exp $	*/

/*++
/* NAME
/*	qmgr_bounce
/* SUMMARY
/*	deal with mail that will not be delivered
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	QMGR_QUEUE *qmgr_bounce_recipient(message, recipient, dsn)
/*	QMGR_MESSAGE *message;
/*	RECIPIENT *recipient;
/*	DSN	*dsn;
/* DESCRIPTION
/*	qmgr_bounce_recipient() produces a bounce log record.
/*	Once the bounce record is written successfully, the recipient
/*	is marked as done. When the bounce record cannot be written,
/*	the message structure is updated to reflect that the mail is
/*	deferred.
/*
/*	Arguments:
/* .IP message
/*	Open queue file with the message being bounced.
/* .IP recipient
/*	The recipient that will not be delivered.
/* .IP dsn
/*	Delivery status information. See dsn(3).
/* DIAGNOSTICS
/*	Panic: consistency check failure. Fatal: out of memory.
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

/* Global library. */

#include <bounce.h>
#include <deliver_completed.h>

/* Application-specific. */

#include "qmgr.h"

/* qmgr_bounce_recipient - bounce one message recipient */

void    qmgr_bounce_recipient(QMGR_MESSAGE *message, RECIPIENT *recipient,
			              DSN *dsn)
{
    MSG_STATS stats;
    int     status;

    status = bounce_append(message->tflags, message->queue_id,
			   QMGR_MSG_STATS(&stats, message), recipient,
			   "none", dsn);

    if (status == 0)
	deliver_completed(message->fp, recipient->offset);
    else
	message->flags |= status;
}
