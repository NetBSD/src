/*++
/* NAME
/*	deliver_completed 3
/* SUMMARY
/*	recipient delivery completion
/* SYNOPSIS
/*	#include <deliver_completed.h>
/*
/*	void	deliver_completed(stream, offset)
/*	VSTREAM	*stream;
/*	long	offset;
/* DESCRIPTION
/*	deliver_completed() crosses off the specified recipient from
/*	an open queue file. A -1 offset means ignore the request -
/*	this is used for delivery requests that are passed on from
/*	one delivery agent to another.
/* DIAGNOSTICS
/*	Fatal error: unable to update the queue file.
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
#include <vstream.h>

/* Global library. */

#include "record.h"
#include "rec_type.h"
#include "deliver_completed.h"

/* deliver_completed - handle per-recipient delivery completion */

void    deliver_completed(VSTREAM *stream, long offset)
{
    char   *myname = "deliver_completed";

    if (offset == -1)
	return;

    if (offset <= 0)
	msg_panic("%s: bad offset %ld", myname, offset);

    if (rec_put_type(stream, REC_TYPE_DONE, offset) < 0
	|| vstream_fflush(stream))
	msg_fatal("update queue file %s: %m", VSTREAM_PATH(stream));
}
