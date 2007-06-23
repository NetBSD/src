/*	$NetBSD: sync_subr.c,v 1.28.6.3 2007/06/23 18:06:04 ad Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sync_subr.c,v 1.28.6.3 2007/06/23 18:06:04 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
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

static void	vn_syncer_add1(struct vnode *, int);

/*
 * Defines and variables for the syncer process.
 */
int syncer_maxdelay = SYNCER_MAXDELAY;	/* maximum delay time */
time_t syncdelay = 30;			/* max time to delay syncing data */
time_t filedelay = 30;			/* time to delay syncing files */
time_t dirdelay  = 15;			/* time to delay syncing directories */
time_t metadelay = 10;			/* time to delay syncing metadata */

kmutex_t syncer_mutex;			/* used to freeze syncer, long term */
static kmutex_t syncer_data_lock;	/* short term lock on data structures */

static int rushjob;			/* number of slots to run ASAP */
static kcondvar_t syncer_cv;		/* cv for rushjob */
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
		TAILQ_INIT(&syncer_workitem_pending[i]);

	mutex_init(&syncer_mutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&syncer_data_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&syncer_cv, "syncer");
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
static void
vn_syncer_add1(vp, delayx)
	struct vnode *vp;
	int delayx;
{
	struct synclist *slp;

	KASSERT(mutex_owned(&syncer_data_lock));

	if (vp->v_iflag & VI_ONWORKLST) {
		slp = &syncer_workitem_pending[vp->v_synclist_slot];
		TAILQ_REMOVE(slp, vp, v_synclist);
	} else {
		/*
		 * We must not modify v_iflag if the vnode
		 * is already on a synclist: sched_sync()
		 * calls this routine while holding only
		 * syncer_data_lock in order to adjust the
		 * position of the vnode.  syncer_data_lock
		 * does not protect v_iflag.
		 */
		KASSERT(mutex_owned(&vp->v_interlock));
		vp->v_iflag |= VI_ONWORKLST;
	}

	if (delayx > syncer_maxdelay - 2)
		delayx = syncer_maxdelay - 2;
	vp->v_synclist_slot = (syncer_delayno + delayx) % syncer_last;

	slp = &syncer_workitem_pending[vp->v_synclist_slot];
	TAILQ_INSERT_TAIL(slp, vp, v_synclist);
}

void
vn_syncer_add_to_worklist(vp, delayx)
	struct vnode *vp;
	int delayx;
{

	KASSERT(mutex_owned(&vp->v_interlock));

	mutex_enter(&syncer_data_lock);
	vn_syncer_add1(vp, delayx);
	mutex_exit(&syncer_data_lock);
}

/*
 * Remove an item from the syncer work queue.
 */
void
vn_syncer_remove_from_worklist(vp)
	struct vnode *vp;
{
	struct synclist *slp;

	KASSERT(mutex_owned(&vp->v_interlock));

	mutex_enter(&syncer_data_lock);

	if (vp->v_iflag & VI_ONWORKLST) {
		vp->v_iflag &= ~VI_ONWORKLST;
		slp = &syncer_workitem_pending[vp->v_synclist_slot];
		TAILQ_REMOVE(slp, vp, v_synclist);
	}

	mutex_exit(&syncer_data_lock);
}

/*
 * System filesystem synchronizer daemon.
 */
void
sched_sync(void *v)
{
	struct synclist *slp;
	struct vnode *vp;
	long starttime;

	updateproc = curlwp;

	for (;;) {
		mutex_enter(&syncer_mutex);
		mutex_enter(&syncer_data_lock);

		starttime = time_second;

		/*
		 * Push files whose dirty time has expired. Be careful
		 * of interrupt race on slp queue.
		 */
		slp = &syncer_workitem_pending[syncer_delayno];
		syncer_delayno += 1;
		if (syncer_delayno >= syncer_last)
			syncer_delayno = 0;

		while ((vp = TAILQ_FIRST(slp)) != NULL) {
			/* We are locking in the wrong direction. */
			if (mutex_tryenter(&vp->v_interlock)) {
				mutex_exit(&syncer_data_lock);
				if (vn_lock(vp, LK_EXCLUSIVE | LK_NOWAIT |
				    LK_INTERLOCK) == 0) {
					(void) VOP_FSYNC(vp, curlwp->l_cred,
					    FSYNC_LAZY, 0, 0, curlwp);
					VOP_UNLOCK(vp, 0);
				}
				mutex_enter(&syncer_data_lock);
			}

			/* XXXAD The vnode may have been recycled. */
			if (TAILQ_FIRST(slp) == vp) {
				/*
				 * Put us back on the worklist.  The worklist
				 * routine will remove us from our current
				 * position and then add us back in at a later
				 * position.
				 */
				vn_syncer_add1(vp, syncdelay);
			}
		}

		mutex_exit(&syncer_data_lock);

		/*
		 * Do soft update processing.
		 */
		if (bioops.io_sync)
			(*bioops.io_sync)(NULL);

		mutex_exit(&syncer_mutex);

		mutex_enter(&syncer_data_lock);
		if (rushjob > 0) {
			/*
			 * The variable rushjob allows the kernel to speed
			 * up the processing of the filesystem syncer
			 * process. A rushjob value of N tells the
			 * filesystem syncer to process the next N seconds
			 * worth of work on its queue ASAP. Currently
			 * rushjob is used by the soft update code to
			 * speed up the filesystem syncer process when the
			 * incore state is getting so far ahead of the
			 * disk that the kernel memory pool is being
			 * threatened with exhaustion.
			 */
			rushjob--;
		} else {
			/*
			 * If it has taken us less than a second to
			 * process the current work, then wait. Otherwise
			 * start right over again. We can still lose time
			 * if any single round takes more than two
			 * seconds, but it does not really matter as we
			 * are just trying to generally pace the
			 * filesystem activity.
			 */
			if (time_second == starttime)
				cv_timedwait(&syncer_cv, &syncer_data_lock, hz);
		}
		mutex_exit(&syncer_data_lock);
	}
}

/*
 * Request the syncer daemon to speed up its work.
 * We never push it to speed up more than half of its
 * normal turn time, otherwise it could take over the CPU.
 */
int
speedup_syncer()
{

	mutex_enter(&syncer_data_lock);
	if (rushjob >= syncdelay / 2) {
		mutex_exit(&syncer_data_lock);
		return (0);
	}
	rushjob++;
	cv_signal(&syncer_cv);
	stat_rush_requests += 1;
	mutex_exit(&syncer_data_lock);

	return (1);
}

SYSCTL_SETUP(sysctl_vfs_syncfs_setup, "sysctl vfs.sync subtree setup")
{
	const struct sysctlnode *rnode, *cnode;
        
	sysctl_createv(clog, 0, NULL, &rnode,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "vfs", NULL,
			NULL, 0, NULL, 0,
			CTL_VFS, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "sync",
			SYSCTL_DESCR("syncer options"),
			NULL, 0, NULL, 0,
			CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &cnode,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "delay",
			SYSCTL_DESCR("max time to delay syncing data"),
			NULL, 0, &syncdelay, 0,
			CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &cnode,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "filedelay",
			SYSCTL_DESCR("time to delay syncing files"),
			NULL, 0, &filedelay, 0,
			CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &cnode,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "dirdelay",
			SYSCTL_DESCR("time to delay syncing directories"),
			NULL, 0, &dirdelay, 0,
			CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &cnode,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "metadelay",
			SYSCTL_DESCR("time to delay syncing metadata"),
			NULL, 0, &metadelay, 0,
			CTL_CREATE, CTL_EOL);
}
