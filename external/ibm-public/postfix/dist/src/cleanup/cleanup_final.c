/*	$NetBSD: cleanup_final.c,v 1.1.1.1 2009/06/23 10:08:43 tron Exp $	*/

/*++
/* NAME
/*	cleanup_final 3
/* SUMMARY
/*	finalize queue file
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_final(state)
/*	CLEANUP_STATE *state;
/* DESCRIPTION
/*	cleanup_final() performs final queue file content (not
/*	attribute) updates so that the file is ready to be closed.
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
#include <errno.h>

/* Utility library. */

#include <msg.h>

/* Global library. */

#include <cleanup_user.h>
#include <rec_type.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_final - final queue file content updates */

void    cleanup_final(CLEANUP_STATE *state)
{
    const char *myname = "cleanup_final";

    /*
     * vstream_fseek() would flush the buffer anyway, but the code just reads
     * better if we flush first, because it makes seek error handling more
     * straightforward.
     */
    if (vstream_fflush(state->dst)) {
	if (errno == EFBIG) {
	    msg_warn("%s: queue file size limit exceeded", state->queue_id);
	    state->errs |= CLEANUP_STAT_SIZE;
	} else {
	    msg_warn("%s: write queue file: %m", state->queue_id);
	    state->errs |= CLEANUP_STAT_WRITE;
	}
	return;
    }

    /*
     * Update the preliminary message size and count fields with the actual
     * values.
     */
    if (vstream_fseek(state->dst, 0L, SEEK_SET) < 0)
	msg_fatal("%s: vstream_fseek %s: %m", myname, cleanup_path);
    cleanup_out_format(state, REC_TYPE_SIZE, REC_TYPE_SIZE_FORMAT,
	    (REC_TYPE_SIZE_CAST1) (state->xtra_offset - state->data_offset),
		       (REC_TYPE_SIZE_CAST2) state->data_offset,
		       (REC_TYPE_SIZE_CAST3) state->rcpt_count,
		       (REC_TYPE_SIZE_CAST4) state->qmgr_opts,
		       (REC_TYPE_SIZE_CAST5) state->cont_length);
}
