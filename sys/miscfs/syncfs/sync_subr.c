/*	$NetBSD: sync_subr.c,v 1.48.2.1 2014/08/20 00:04:31 tls Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

/*
 * The filesystem synchronizer mechanism - syncer.
 *
 * It is useful to delay writes of file data and filesystem metadata for
 * a certain amount of time so that quickly created and deleted files need
 * not waste disk bandwidth being created and removed.  To implement this,
 * vnodes are appended to a "workitem" queue.
 *
 * Most pending metadata should not wait for more than ten seconds.  Thus,
 * mounted on block devices are delayed only about a half the time that file
 * data is delayed.  Similarly, directory updates are more critical, so are
 * only delayed about a third the time that file data is delayed.
 *
 * There are SYNCER_MAXDELAY queues that are processed in a round-robin
 * manner at a rate of one each second (driven off the filesystem syner
 * thread). The syncer_delayno variable indicates the next queue that is
 * to be processed.  Items that need to be processed soon are placed in
 * this queue:
 *
 *	syncer_workitem_pending[syncer_delayno]
 *
 * A delay of e.g. fifteen seconds is done by placing the request fifteen
 * entries later in the queue:
 *
 *	syncer_workitem_pending[(syncer_delayno + 15) & syncer_mask]
 *
 * Flag VI_ONWORKLST indicates that vnode is added into the queue.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sync_subr.c,v 1.48.2.1 2014/08/20 00:04:31 tls Exp $");

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
#include <sys/kmem.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

typedef TAILQ_HEAD(synclist, vnode) synclist_t;

static void	vn_syncer_add1(struct vnode *, int);
static void	sysctl_vfs_syncfs_setup(struct sysctllog **);

/*
 * Defines and variables for the syncer process.
 */
int syncer_maxdelay = SYNCER_MAXDELAY;	/* maximum delay time */
time_t syncdelay = 30;			/* max time to delay syncing data */
time_t filedelay = 30;			/* time to delay syncing files */
time_t dirdelay  = 15;			/* time to delay syncing directories */
time_t metadelay = 10;			/* time to delay syncing metadata */
time_t lockdelay = 1;			/* time to delay if locking fails */

kmutex_t		syncer_mutex;	/* used to freeze syncer, long term */
static kmutex_t		syncer_data_lock; /* short term lock on data structs */

static int		syncer_delayno = 0;
static long		syncer_last;
static synclist_t *	syncer_workitem_pending;

void
vn_initialize_syncerd(void)
{
	int i;

	syncer_last = SYNCER_MAXDELAY + 2;

	sysctl_vfs_syncfs_setup(NULL);

	syncer_workitem_pending =
	    kmem_alloc(syncer_last * sizeof (struct synclist), KM_SLEEP);

	for (i = 0; i < syncer_last; i++)
		TAILQ_INIT(&syncer_workitem_pending[i]);

	mutex_init(&syncer_mutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&syncer_data_lock, MUTEX_DEFAULT, IPL_NONE);
}

/*
 * Add an item to the syncer work queue.
 */
static void
vn_syncer_add1(struct vnode *vp, int delayx)
{
	synclist_t *slp;

	KASSERT(mutex_owned(&syncer_data_lock));

	if (vp->v_iflag & VI_ONWORKLST) {
		/*
		 * Remove in order to adjust the position of the vnode.
		 * Note: called from sched_sync(), which will not hold
		 * interlock, therefore we cannot modify v_iflag here.
		 */
		slp = &syncer_workitem_pending[vp->v_synclist_slot];
		TAILQ_REMOVE(slp, vp, v_synclist);
	} else {
		KASSERT(mutex_owned(vp->v_interlock));
		vp->v_iflag |= VI_ONWORKLST;
	}

	if (delayx > syncer_maxdelay - 2)
		delayx = syncer_maxdelay - 2;
	vp->v_synclist_slot = (syncer_delayno + delayx) % syncer_last;

	slp = &syncer_workitem_pending[vp->v_synclist_slot];
	TAILQ_INSERT_TAIL(slp, vp, v_synclist);
}

void
vn_syncer_add_to_worklist(struct vnode *vp, int delayx)
{

	KASSERT(mutex_owned(vp->v_interlock));

	mutex_enter(&syncer_data_lock);
	vn_syncer_add1(vp, delayx);
	mutex_exit(&syncer_data_lock);
}

/*
 * Remove an item from the syncer work queue.
 */
void
vn_syncer_remove_from_worklist(struct vnode *vp)
{
	synclist_t *slp;

	KASSERT(mutex_owned(vp->v_interlock));

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
sched_sync(void *arg)
{
	synclist_t *slp;
	struct vnode *vp;
	time_t starttime;
	bool synced;

	for (;;) {
		mutex_enter(&syncer_mutex);
		mutex_enter(&syncer_data_lock);

		starttime = time_second;

		/*
		 * Push files whose dirty time has expired.
		 */
		slp = &syncer_workitem_pending[syncer_delayno];
		syncer_delayno += 1;
		if (syncer_delayno >= syncer_last)
			syncer_delayno = 0;

		while ((vp = TAILQ_FIRST(slp)) != NULL) {
			/* We are locking in the wrong direction. */
			synced = false;
			if (mutex_tryenter(vp->v_interlock)) {
				mutex_exit(&syncer_data_lock);
				if (vget(vp, LK_EXCLUSIVE | LK_NOWAIT) == 0) {
					synced = true;
					(void) VOP_FSYNC(vp, curlwp->l_cred,
					    FSYNC_LAZY, 0, 0);
					vput(vp);
				}
				mutex_enter(&syncer_data_lock);
			}

			/*
			 * XXX The vnode may have been recycled, in which
			 * case it may have a new identity.
			 */
			if (TAILQ_FIRST(slp) == vp) {
				/*
				 * Put us back on the worklist.  The worklist
				 * routine will remove us from our current
				 * position and then add us back in at a later
				 * position.
				 *
				 * Try again sooner rather than later if
				 * we were unable to lock the vnode.  Lock
				 * failure should not prevent us from doing
				 * the sync "soon".
				 *
				 * If we locked it yet arrive here, it's
				 * likely that lazy sync is in progress and
				 * so the vnode still has dirty metadata. 
				 * syncdelay is mainly to get this vnode out
				 * of the way so we do not consider it again
				 * "soon" in this loop, so the delay time is
				 * not critical as long as it is not "soon". 
				 * While write-back strategy is the file
				 * system's domain, we expect write-back to
				 * occur no later than syncdelay seconds
				 * into the future.
				 */
				vn_syncer_add1(vp,
				    synced ? syncdelay : lockdelay);
			}
		}
		mutex_exit(&syncer_mutex);

		/*
		 * If it has taken us less than a second to process the
		 * current work, then wait.  Otherwise start right over
		 * again.  We can still lose time if any single round
		 * takes more than two seconds, but it does not really
		 * matter as we are just trying to generally pace the
		 * filesystem activity.
		 */
		if (time_second == starttime) {
			kpause("syncer", false, hz, &syncer_data_lock);
		}
		mutex_exit(&syncer_data_lock);
	}
}

static void
sysctl_vfs_syncfs_setup(struct sysctllog **clog)
{
	const struct sysctlnode *rnode, *cnode;

	sysctl_createv(clog, 0, NULL, &rnode,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "sync",
			SYSCTL_DESCR("syncer options"),
			NULL, 0, NULL, 0,
			CTL_VFS, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &cnode,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_QUAD, "delay",
			SYSCTL_DESCR("max time to delay syncing data"),
			NULL, 0, &syncdelay, 0,
			CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &cnode,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_QUAD, "filedelay",
			SYSCTL_DESCR("time to delay syncing files"),
			NULL, 0, &filedelay, 0,
			CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &cnode,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_QUAD, "dirdelay",
			SYSCTL_DESCR("time to delay syncing directories"),
			NULL, 0, &dirdelay, 0,
			CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &cnode,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_QUAD, "metadelay",
			SYSCTL_DESCR("time to delay syncing metadata"),
			NULL, 0, &metadelay, 0,
			CTL_CREATE, CTL_EOL);
}
