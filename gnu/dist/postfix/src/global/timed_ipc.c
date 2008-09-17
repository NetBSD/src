/*++
/* NAME
/*	timed_ipc 3
/* SUMMARY
/*	enforce IPC timeout on stream
/* SYNOPSIS
/*	#include <time_ipc.h>
/*
/*	void	timed_ipc_setup(stream)
/*	VSTREAM	*stream;
/* DESCRIPTION
/*	timed_ipc() enforces on the specified stream the timeout as
/*	specified via the \fIipc_timeout\fR configuration parameter:
/*	a read or write operation fails if it does not succeed within
/*	\fIipc_timeout\fR seconds. This deadline exists as a safety
/*	measure for communication between mail subsystem programs,
/*	and should never be exceeded.
/* DIAGNOSTICS
/*	Panic: sanity check failed. Fatal error: deadline exceeded.
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

#include "mail_params.h"
#include "timed_ipc.h"

/* timed_ipc_setup - enable ipc with timeout */

void    timed_ipc_setup(VSTREAM *stream)
{
    if (var_ipc_timeout <= 0)
	msg_panic("timed_ipc_setup: bad ipc_timeout %d", var_ipc_timeout);

    vstream_control(stream,
		    VSTREAM_CTL_TIMEOUT, var_ipc_timeout,
		    VSTREAM_CTL_END);
}
