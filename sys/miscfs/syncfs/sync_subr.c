/*	$NetBSD: sync_subr.c,v 1.8.2.2 2001/09/21 22:36:40 nathanw Exp $	*/

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
#include <sys/lwp.h>
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
time_t syncdelay = 30;			/* max time to delay syncing data */ 
time_t filedelay = 30;			/* time to delay syncing files */
time_t dirdelay  = 15;			/* time to delay syncing directories */
time_t metadelay = 10;			/* time to delay syncing metadata */

struct lock syncer_lock;		/* used to freeze syncer */

static int rushjob;			/* number of slots to run ASAP */ 
static int stat_rush_requests;		/* number of times I/O speeded up */
 
static int syncer_delayno = 0;
static long syncer_last;
static struct synclist *syncer_workitem_pending;
struct lwp *updateproc = NULL;

void
vn_initialize_syncerd()
{
	int i;

	syncer_last = SYNCER_MAXDELAY + 2;

	syncer_workitem_pending = malloc(syncer_last * sizeof (struct synclist),
	    M_VNODE, M_WAITOK);

	for (i = 0; i < syncer_last; i++)
		LIST_INIT(&syncer_workitem_pending[i]);

	lockinit(&syncer_lock, PVFS, "synclk", 0, 0);
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

	if (vp->v_flag & VONWORKLST) {
		LIST_REMOVE(vp, v_synclist);
	}

	if (delay > syncer_maxdelay - 2)
		delay = syncer_maxdelay - 2;
	slot = (syncer_delayno + delay) % syncer_last;

	LIST_INSERT_HEAD(&syncer_workitem_pending[slot], vp, v_synclist);
	vp->v_flag |= VONWORKLST;
	splx(s);
}

/*
 * Remove an item fromthe syncer work queue.
 */
void
vn_syncer_remove_from_worklist(vp)
	struct vnode *vp;
{
	int s;

	s = splbio();

	if (vp->v_flag & VONWORKLST) {
		LIST_REMOVE(vp, v_synclist);
	}

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

	updateproc = curproc;
	
	for (;;) {
		starttime = time.tv_sec;

		/*
		 * Push files whose dirty time has expired. Be careful
		 * of interrupt race on slp queue.
		 */
		s = splbio();
		slp = &syncer_workitem_pending[syncer_delayno];
		syncer_delayno += 1;
		if (syncer_delayno >= syncer_last)
			syncer_delayno = 0;
		splx(s);

		lockmgr(&syncer_lock, LK_EXCLUSIVE, NULL);

		while ((vp = LIST_FIRST(slp)) != NULL) {
			if (vn_lock(vp, LK_EXCLUSIVE | LK_NOWAIT) == 0) {
				(void) VOP_FSYNC(vp, curproc->l_proc->p_ucred,
				    FSYNC_LAZY, 0, 0, curproc);
				VOP_UNLOCK(vp, 0);
			}
			s = splbio();
			if (LIST_FIRST(slp) == vp) {

				/*
				 * Put us back on the worklist.  The worklist
				 * routine will remove us from our current
				 * position and then add us back in at a later
				 * position.
				 */

				vn_syncer_add_to_worklist(vp, syncdelay);
			}
			splx(s);
		}

		/*
		 * Do soft update processing.
		 */
		if (bioops.io_sync)
			(*bioops.io_sync)(NULL);

		lockmgr(&syncer_lock, LK_RELEASE, NULL);

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
			tsleep(&rushjob, PPAUSE, "syncer", hz);
	}
}

/*
 * Request the syncer daemon to speed up its work.
 * We never push it to speed up more than half of its
 * normal turn time, otherwise it could take over the cpu.
 */
int
speedup_syncer()
{
	if (rushjob >= syncdelay / 2) {
		return (0);
	}

	rushjob++;
	wakeup(&rushjob);
	stat_rush_requests += 1;
	return (1);
}
