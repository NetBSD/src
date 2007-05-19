/*	$NetBSD: cleanup_body_edit.c,v 1.1.1.1 2007/05/19 16:28:07 heas Exp $	*/

/*++
/* NAME
/*	cleanup_body_edit 3
/* SUMMARY
/*	edit body content
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	int	cleanup_body_edit_start(state)
/*	CLEANUP_STATE *state;
/*
/*	int	cleanup_body_edit_write(state, type, buf)
/*	CLEANUP_STATE *state;
/*	int	type;
/*	VSTRING	*buf;
/*
/*	int	cleanup_body_edit_finish(state)
/*	CLEANUP_STATE *state;
/*
/*	void	cleanup_body_edit_free(state)
/*	CLEANUP_STATE *state;
/* DESCRIPTION
/*	This module maintains queue file regions with body content.
/*	Regions are created on the fly, and can be reused multiple
/*	times. This module must not be called until the queue file
/*	is complete, and there must be no other read/write access
/*	to the queue file between the cleanup_body_edit_start() and
/*	cleanup_body_edit_finish() calls.
/*
/*	cleanup_body_edit_start() performs initialization and sets
/*	the queue file write pointer to the start of the first body
/*	region.
/*
/*	cleanup_body_edit_write() adds a queue file record to the
/*	queue file. When the current body region fills up, some
/*	unused region is reused, or a new region is created.
/*
/*	cleanup_body_edit_finish() makes some final adjustments
/*	after the last body content record is written.
/*
/*	cleanup_body_edit_free() frees up memory that was allocated
/*	by cleanup_body_edit_start() and cleanup_body_edit_write().
/*
/*	Arguments:
/* .IP state
/*	Queue file and message processing state. This state is updated
/*	as records are processed and as errors happen.
/* .IP type
/*	Record type.
/* .IP buf
/*	Record content.
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
#include <mymalloc.h>
#include <vstream.h>
#include <vstring.h>

/* Global library. */

#include <rec_type.h>
#include <record.h>

/* Application-specific. */

#include <cleanup.h>

#define LEN(s) VSTRING_LEN(s)

static int cleanup_body_edit_ptr_rec_len;

/* cleanup_body_edit_start - rewrite body region pool */

int     cleanup_body_edit_start(CLEANUP_STATE *state)
{
    const char *myname = "cleanup_body_edit_start";
    CLEANUP_REGION *curr_rp;

    /*
     * Calculate the payload size sans body.
     */
    state->cont_length = state->body_offset - state->data_offset;

    /*
     * One-time initialization.
     */
    if (state->body_regions == 0) {
	REC_SPACE_NEED(REC_TYPE_PTR_PAYL_SIZE, cleanup_body_edit_ptr_rec_len);
	cleanup_region_init(state);
    }

    /*
     * Return all body regions to the free pool.
     */
    cleanup_region_return(state, state->body_regions);

    /*
     * Select the first region. XXX This will usally be the original body
     * segment, but we must not count on that. Region assignments may change
     * when header editing also uses queue file regions. XXX We don't really
     * know if the first region will be large enough to hold the first body
     * text record, but this problem is so rare that we will not complicate
     * the code for it. If the first region is too small then we will simply
     * waste it.
     */
    curr_rp = state->curr_body_region = state->body_regions =
	cleanup_region_open(state, cleanup_body_edit_ptr_rec_len);

    /*
     * Link the first body region to the last message header.
     */
    if (vstream_fseek(state->dst, state->append_hdr_pt_offset, SEEK_SET) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	return (-1);
    }
    state->append_hdr_pt_target = curr_rp->start;
    rec_fprintf(state->dst, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		(long) state->append_hdr_pt_target);

    /*
     * Move the file write pointer to the start of the current region.
     */
    if (vstream_ftell(state->dst) != curr_rp->start
	&& vstream_fseek(state->dst, curr_rp->start, SEEK_SET) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	return (-1);
    }
    return (0);
}

/* cleanup_body_edit_write - add record to body region pool */

int     cleanup_body_edit_write(CLEANUP_STATE *state, int rec_type,
				        VSTRING *buf)
{
    const char *myname = "cleanup_body_edit_write";
    CLEANUP_REGION *curr_rp = state->curr_body_region;
    CLEANUP_REGION *next_rp;
    off_t   space_used;
    ssize_t space_needed;
    ssize_t rec_len;

    if (msg_verbose)
	msg_info("%s: where %ld, buflen %ld region start %ld len %ld",
		 myname, (long) curr_rp->write_offs, (long) LEN(buf),
		 (long) curr_rp->start, (long) curr_rp->len);

    /*
     * Switch to the next body region if we filled up the current one (we
     * always append to an open-ended region). Besides space to write the
     * specified record, we need to leave space for a final pointer record
     * that will link this body region to the next region or to the content
     * terminator record.
     */
    if (curr_rp->len > 0) {
	space_used = curr_rp->write_offs - curr_rp->start;
	REC_SPACE_NEED(LEN(buf), rec_len);
	space_needed = rec_len + cleanup_body_edit_ptr_rec_len;
	if (space_needed > curr_rp->len - space_used) {

	    /*
	     * Update the payload size. Connect the filled up body region to
	     * its successor.
	     */
	    state->cont_length += space_used;
	    next_rp = cleanup_region_open(state, space_needed);
	    if (msg_verbose)
		msg_info("%s: link %ld -> %ld", myname,
			 (long) curr_rp->write_offs, (long) next_rp->start);
	    rec_fprintf(state->dst, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
			(long) next_rp->start);
	    curr_rp->write_offs = vstream_ftell(state->dst);
	    cleanup_region_close(state, curr_rp);
	    curr_rp->next = next_rp;

	    /*
	     * Select the new body region.
	     */
	    state->curr_body_region = curr_rp = next_rp;
	    if (vstream_fseek(state->dst, curr_rp->start, SEEK_SET) < 0) {
		msg_warn("%s: seek file %s: %m", myname, cleanup_path);
		return (-1);
	    }
	}
    }

    /*
     * Finally, output the queue file record.
     */
    CLEANUP_OUT_BUF(state, REC_TYPE_NORM, buf);
    curr_rp->write_offs = vstream_ftell(state->dst);

    return (0);
}

/* cleanup_body_edit_finish - wrap up body region pool */

int     cleanup_body_edit_finish(CLEANUP_STATE *state)
{
    CLEANUP_REGION *curr_rp = state->curr_body_region;

    /*
     * Update the payload size.
     */
    state->cont_length += curr_rp->write_offs - curr_rp->start;

    /*
     * Link the last body region to the content terminator record.
     */
    rec_fprintf(state->dst, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		(long) state->xtra_offset);
    curr_rp->write_offs = vstream_ftell(state->dst);
    cleanup_region_close(state, curr_rp);

    return (CLEANUP_OUT_OK(state) ? 0 : -1);
}
