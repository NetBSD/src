/*++
/* NAME
/*	cleanup_skip 3
/* SUMMARY
/*	skip further input
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_skip(void)
/* DESCRIPTION
/*	cleanup_skip() skips client input. This function is used after
/*	detecting an error. The idea is to drain input from the client
/*	so the client is ready to read the cleanup service status report.
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

#include <cleanup_user.h>
#include <record.h>
#include <rec_type.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_skip - skip further client input */

void    cleanup_skip(void)
{
    int     type;

    if ((cleanup_errs & CLEANUP_STAT_CONT) == 0)
	msg_warn("%s: skipping further client input", cleanup_queue_id);

    /*
     * XXX Rely on the front-end programs to enforce record size limits.
     */
    while ((type = rec_get(cleanup_src, cleanup_inbuf, 0)) > 0
	   && type != REC_TYPE_END)
	 /* void */ ;
}
