/*++
/* NAME
/*	qmgr_bounce
/* SUMMARY
/*	deal with mail that will not be delivered
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	QMGR_QUEUE *qmgr_bounce_recipient(message, recipient, format, ...)
/*	QMGR_MESSAGE *message;
/*	QMGR_RCPT *recipient;
/*	const char *format;
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
/* .IP format
/*	Free-format text that describes why delivery will not happen.
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
#include <stdarg.h>

/* Utility library. */

/* Global library. */

#include <bounce.h>
#include <deliver_completed.h>

/* Application-specific. */

#include "qmgr.h"

/* qmgr_bounce_recipient - bounce one message recipient */

void    qmgr_bounce_recipient(QMGR_MESSAGE *message, QMGR_RCPT *recipient,
			              const char *format,...)
{
    va_list ap;
    int     status;

    va_start(ap, format);
    status = vbounce_append(BOUNCE_FLAG_KEEP, message->queue_id,
			    recipient->address, "none",
			    message->arrival_time, format, ap);
    va_end(ap);

    if (status == 0)
	deliver_completed(message->fp, recipient->offset);
    else
	message->flags |= status;
}
