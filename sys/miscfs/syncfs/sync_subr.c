/*	$NetBSD: sync_subr.c,v 1.1.2.1 1999/10/19 12:50:15 fvdl Exp $	*/

/*
 * Copyright 1997 Marshall Kirk McKusick. All Rights Reserved.
 *
 * This code is derived from work done by Greg Ganger at the
 * University of Michigan.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. None of the names of McKusick, Ganger, or the University of Michigan
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MARSHALL KIRK MCKUSICK ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL MARSHALL KIRK MCKUSICK BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/malloc.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

/* 
 * Defines and variables for the syncer process. 
 */
int syncer_maxdelay = SYNCER_MAXDELAY;	/* maximum delay time */
time_t syncdelay = 30;			/* time to delay syncing vnodes */ 
int rushjob;				/* number of slots to run ASAP */ 
 
static int syncer_delayno = 0;
static long syncer_last;
static struct synclist *syncer_workitem_pending;

void
vn_initialize_syncerd()
{
	int i;

	syncer_last = SYNCER_MAXDELAY + 2;

	syncer_workitem_pending = malloc(syncer_last * sizeof (struct synclist),
	    M_VNODE, M_WAITOK);

	for (i = 0; i < syncer_last; i++)
		LIST_INIT(&syncer_workitem_pending[i]);
}

/*
 * The workitem queue.
 * 
 * It is useful to delay writes of file data and filesystem metadata
 * for tens of seconds so that quickly created and deleted files need
 * not waste disk bandwidth being created and removed. To realize this,
 * we append vnodes to a "workitem" queue. When running with a soft
 * updates implementation, most pending metadata dependencies should
 * not wait for more than a few seconds. Thus, mounted on block devices
 * are delayed only about a half the time that file data is delayed.
 * Similarly, directory updates are more critical, so are only delayed
 * about a third the time that file data is delayed. Thus, there are
 * SYNCER_MAXDELAY queues that are processed round-robin at a rate of
 * one each second (driven off the filesystem syner process). The
 * syncer_delayno variable indicates the next queue that is to be processed.
 * Items that need to be processed soon are placed in this queue:
 *
 *	syncer_workitem_pending[syncer_delayno]
 *
 * A delay of fifteen seconds is done by placing the request fifteen
 * entries later in the queue:
 *
 *	syncer_workitem_pending[(syncer_delayno + 15) & syncer_mask]
 *
 */

/*
 * Add an item to the syncer work queue.
 */
void
vn_syncer_add_to_worklist(vp, delay)
	struct vnode *vp;
	int delay;
{
	int s, slot;

	s = splbio();
	if (delay > syncer_maxdelay - 2)
		delay = syncer_maxdelay - 2;
	slot = (syncer_delayno + delay) % syncer_last;
	LIST_INSERT_HEAD(&syncer_workitem_pending[slot], vp, v_synclist);
	splx(s);
}

/*
 * System filesystem synchronizer daemon.
 */
void 
sched_sync(v)
	void *v;
{
	struct synclist *slp;
	struct vnode *vp;
	long starttime;
	int s;

	for (;;) {
		starttime = time.tv_sec;

		/*
		 * Push files whose dirty time has expired.
		 */
		s = splbio();
		slp = &syncer_workitem_pending[syncer_delayno];
		syncer_delayno += 1;
		if (syncer_delayno >= syncer_last)
			syncer_delayno = 0;
		splx(s);
		while ((vp = LIST_FIRST(slp)) != NULL) {
			vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
			(void) VOP_FSYNC(vp, curproc->p_ucred, FSYNC_LAZY,
			    curproc);
			VOP_UNLOCK(vp, 0);
			if (LIST_FIRST(slp) == vp) {
				if (LIST_FIRST(&vp->v_dirtyblkhd) == NULL &&
				    vp->v_type != VBLK)
					panic("sched_sync: fsync failed");
				/*
				 * Move ourselves to the back of the sync list.
				 */
				LIST_REMOVE(vp, v_synclist);
				vn_syncer_add_to_worklist(vp, syncdelay);
			}
		}

		/*
		 * Do soft update processing.
		 */
		if (bioops.io_sync)
			(*bioops.io_sync)(NULL);

		/*
		 * The variable rushjob allows the kernel to speed up the
		 * processing of the filesystem syncer process. A rushjob
		 * value of N tells the filesystem syncer to process the next
		 * N seconds worth of work on its queue ASAP. Currently rushjob
		 * is used by the soft update code to speed up the filesystem
		 * syncer process when the incore state is getting so far
		 * ahead of the disk that the kernel memory pool is being
		 * threatened with exhaustion.
		 */
		if (rushjob > 0) {
			rushjob--;
			continue;
		}

		/*
		 * If it has taken us less than a second to process the
		 * current work, then wait. Otherwise start right over
		 * again. We can still lose time if any single round
		 * takes more than two seconds, but it does not really
		 * matter as we are just trying to generally pace the
		 * filesystem activity.
		 */
		if (time.tv_sec == starttime)
			tsleep(&lbolt, PPAUSE, "syncer", 0);
	}
}
