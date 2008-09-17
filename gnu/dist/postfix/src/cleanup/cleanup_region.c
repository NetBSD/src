/*++
/* NAME
/*	cleanup_region 3
/* SUMMARY
/*	queue file region manager
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_region_init(state)
/*	CLEANUP_STATE *state;
/*
/*	CLEANUP_REGION *cleanup_region_open(state, space_needed)
/*	CLEANUP_STATE *state;
/*	ssize_t	space_needed;
/*
/*	int	cleanup_region_close(state, rp)
/*	CLEANUP_STATE *state;
/*	CLEANUP_REGION *rp;
/*
/*	CLEANUP_REGION *cleanup_region_return(state, rp)
/*	CLEANUP_STATE *state;
/*	CLEANUP_REGION *rp;
/*
/*	void	cleanup_region_done(state)
/*	CLEANUP_STATE *state;
/* DESCRIPTION
/*	This module maintains queue file regions. Regions are created
/*	on-the-fly and can be reused multiple times. Each region
/*	structure consists of a file offset, a length (0 for an
/*	open-ended region at the end of the file), a write offset
/*	(maintained by the caller), and list linkage. Region
/*	boundaries are not enforced by this module. It is up to the
/*	caller to ensure that they stay within bounds.
/*
/*	cleanup_region_init() performs mandatory initialization and
/*	overlays an initial region structure over an already existing
/*	queue file. This function must not be called before the
/*	queue file is complete.
/*
/*	cleanup_region_open() opens an existing region or creates
/*	a new region that can accomodate at least the specified
/*	amount of space. A new region is an open-ended region at
/*	the end of the file; it must be closed (see next) before
/*	unrelated data can be appended to the same file.
/*
/*	cleanup_region_close() indicates that a region will not be
/*	updated further. With an open-ended region, the region's
/*	end is frozen just before the caller-maintained write offset.
/*	With a close-ended region, unused space (beginning at the
/*	caller-maintained write offset) may be returned to the free
/*	pool.
/*
/*	cleanup_region_return() returns a list of regions to the
/*	free pool, and returns a null pointer. To avoid fragmentation,
/*	adjacent free regions may be coalesced together.
/*
/*	cleanup_region_done() destroys all in-memory information
/*	that was allocated for administering queue file regions.
/*
/*	Arguments:
/* .IP state
/*	Queue file and message processing state. This state is
/*	updated as records are processed and as errors happen.
/* .IP space_needed
/*	The minimum region size needed.
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
#include <sys/stat.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>

/* Application-specific. */

#include <cleanup.h>

/* cleanup_region_alloc - create queue file region */

static CLEANUP_REGION *cleanup_region_alloc(off_t start, off_t len)
{
    CLEANUP_REGION *rp;

    rp = (CLEANUP_REGION *) mymalloc(sizeof(*rp));
    rp->write_offs = rp->start = start;
    rp->len = len;
    rp->next = 0;

    return (rp);
}

/* cleanup_region_free - destroy region list */

static CLEANUP_REGION *cleanup_region_free(CLEANUP_REGION *regions)
{
    CLEANUP_REGION *rp;
    CLEANUP_REGION *next;

    for (rp = regions; rp != 0; rp = next) {
	next = rp->next;
	myfree((char *) rp);
    }
    return (0);
}

/* cleanup_region_init - create initial region overlay */

void    cleanup_region_init(CLEANUP_STATE *state)
{
    const char *myname = "cleanup_region_init";

    /*
     * Sanity check.
     */
    if (state->free_regions != 0 || state->body_regions != 0)
	msg_panic("%s: repeated call", myname);

    /*
     * Craft the first regions on the fly, from circumstantial evidence.
     */
    state->body_regions =
	cleanup_region_alloc(state->append_hdr_pt_target,
			  state->xtra_offset - state->append_hdr_pt_target);
    if (msg_verbose)
	msg_info("%s: body start %ld len %ld",
	      myname, (long) state->body_regions->start, (long) state->body_regions->len);
}

/* cleanup_region_open - open existing region or create new region */

CLEANUP_REGION *cleanup_region_open(CLEANUP_STATE *state, ssize_t len)
{
    const char *myname = "cleanup_region_open";
    CLEANUP_REGION **rpp;
    CLEANUP_REGION *rp;
    struct stat st;

    /*
     * Find the first region that is large enough, or create a new region.
     */
    for (rpp = &state->free_regions; /* see below */ ; rpp = &(rp->next)) {

	/*
	 * Create an open-ended region at the end of the queue file. We
	 * freeze the region size after we stop writing to it. XXX Assume
	 * that fstat() returns a file size that is never less than the file
	 * append offset. It is not a problem if fstat() returns a larger
	 * result; we would just waste some space.
	 */
	if ((rp = *rpp) == 0) {
	    if (fstat(vstream_fileno(state->dst), &st) < 0)
		msg_fatal("%s: fstat file %s: %m", myname, cleanup_path);
	    rp = cleanup_region_alloc(st.st_size, 0);
	    break;
	}

	/*
	 * Reuse an existing region.
	 */
	if (rp->len >= len) {
	    (*rpp) = rp->next;
	    rp->next = 0;
	    rp->write_offs = rp->start;
	    break;
	}

	/*
	 * Skip a too small region.
	 */
	if (msg_verbose)
	    msg_info("%s: skip start %ld len %ld < %ld",
		     myname, (long) rp->start, (long) rp->len, (long) len);
    }
    if (msg_verbose)
	msg_info("%s: done start %ld len %ld",
		 myname, (long) rp->start, (long) rp->len);
    return (rp);
}

/* cleanup_region_close - freeze queue file region size */

void    cleanup_region_close(CLEANUP_STATE *unused_state, CLEANUP_REGION *rp)
{
    const char *myname = "cleanup_region_close";

    /*
     * If this region is still open ended, freeze the size. If this region is
     * closed, some future version of this routine may shrink the size and
     * return the unused portion to the free pool.
     */
    if (rp->len == 0)
	rp->len = rp->write_offs - rp->start;
    if (msg_verbose)
	msg_info("%s: freeze start %ld len %ld",
		 myname, (long) rp->start, (long) rp->len);
}

/* cleanup_region_return - return region list to free pool */

CLEANUP_REGION *cleanup_region_return(CLEANUP_STATE *state, CLEANUP_REGION *rp)
{
    CLEANUP_REGION **rpp;

    for (rpp = &state->free_regions; (*rpp) != 0; rpp = &(*rpp)->next)
	 /* void */ ;
    *rpp = rp;
    return (0);
}

/* cleanup_region_done - destroy region metadata */

void    cleanup_region_done(CLEANUP_STATE *state)
{
    if (state->free_regions != 0)
	state->free_regions = cleanup_region_free(state->free_regions);
    if (state->body_regions != 0)
	state->body_regions = cleanup_region_free(state->body_regions);
}
