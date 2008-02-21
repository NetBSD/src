/* 	$NetBSD: dctl.c,v 1.1.6.1 2008/02/21 20:44:55 mjf Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
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

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl and Christopher G. Demetriou.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * from: sys/arch/i386/i386/apm.c,v 1.49 2000/05/08
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dctl.c,v 1.1.6.1 2008/02/21 20:44:55 mjf Exp $");

#if defined(DEBUG) && !defined(DCTLDEBUG)
#define	DCTLDEBUG
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/queue.h>
#include <sys/conf.h>

#include <dev/dctl/dctlvar.h>
#include <dev/dctl/dctl.h>
#include <fs/devfs/devfs_comm.h>

#include <machine/stdarg.h>

#ifdef DCTLDEBUG
#define DPRINTF(f, x)		do { if (dctldebug & (f)) printf x; } while (0)


#ifdef DCTLDEBUG_VALUE
int	dctldebug = DCTLDEBUG_VALUE;
#else
int	dctldebug = 0;
#endif /* DCTLDEBUG_VALUE */

#else
#define	DPRINTF(f, x)		/**/
#endif /* DCTLDEBUG */

/*
 * A brief note on the locking protocol: it's very simple; we
 * assert an exclusive lock any time thread context enters the
 * DCTL module.  This is both the DCTL thread itself, as well as
 * user context.
 */
#define	DCTL_LOCK(dctlsc)						\
	(void) mutex_enter(dctlsc)
#define	DCTL_UNLOCK(dctlsc)						\
	(void) mutex_exit(dctlsc)

static int	dctl_record_event(struct dctl_msg *);
static void	dctl_periodic_device_check(void);
static void	dctl_thread(void *);
static int	dctl_mount_change(const char *, int32_t , enum dmsgtype);

void dctl_init(void);
void dctlattach(int);

dev_type_open(dctlopen);
dev_type_close(dctlclose);
dev_type_ioctl(dctlioctl);
dev_type_poll(dctlpoll);
dev_type_kqfilter(dctlkqfilter);

const struct cdevsw dctl_cdevsw = {
	dctlopen, dctlclose, noread, nowrite, dctlioctl,
	nostop, notty, dctlpoll, nommap, dctlkqfilter, D_OTHER,
};

/* variables used during operation (XXX cgd) */
static 	int dctl_open = 0;
static	struct dctl_softc *sc;
static 	lwp_t *dctl_thread_handle;
static 	kmutex_t dctl_lock;
static 	SIMPLEQ_HEAD(,dctl_event_entry) event_list = 
    SIMPLEQ_HEAD_INITIALIZER(event_list);
static	int dctl_initialised = 0;
static	struct devicelist ourdevs = TAILQ_HEAD_INITIALIZER(ourdevs);

/*
 * A device has been removed from the machine.
 */
void
dctl_device_gone(device_t dev)
{
	struct dctl_msg *msg;
	msg = kmem_zalloc(sizeof(*msg), KM_SLEEP);
	if (msg == NULL) {
		printf("couldn't alloc memory for message\n");
		return;
	}

	msg->d_type = DCTL_REMOVED_DEVICE;

	/* cookie */
	msg->d_dc = (intptr_t)dev;
	strlcpy(msg->d_kdev.k_name, device_xname(dev), 
	    sizeof(msg->d_kdev));

	dctl_record_event(msg);
	return;
}

/*
 * A new devfs mount has been created, so kick devfsd(8). What devfsd 
 * will actually do is figure out appropriate attributes for device nodes
 * that need to populate this mount and then pass them through dctl to the
 * mount.
 *
 * 'path' is the absolute pathname this devfs is mounted on.
 * 'cookie' uniquely identifies the mount point.
 */
int
dctl_mount_msg(const char *path, int32_t cookie)
{
	return dctl_mount_change(path, cookie, DCTL_NEW_MOUNT);
}

/*
 * A devfs mount point has been destroyed.
 */
int
dctl_unmount_msg(int32_t cookie)
{
	return dctl_mount_change(NULL, cookie, DCTL_UNMOUNT);
}

/*
 * If this function is called because a devfs has been mounted
 * then the 'path' argument must be a valid path, specifically,
 * it must be the mount point pathname of the devfs being mounted.
 */
static int
dctl_mount_change(const char *path, int32_t cookie, enum dmsgtype type)
{
	int error;
	struct dctl_msg *msg;

	if ((msg = kmem_zalloc(sizeof(*msg), KM_SLEEP)) == NULL) {
		printf("could not alloc memory for dctl message\n");
		return -1;
	}

	msg->d_type = type;
	msg->d_mc = cookie;

	if (type != DCTL_UNMOUNT) {
		KASSERT(path != NULL);
		strlcpy(msg->d_mp.m_pathname, 
		    path, sizeof(msg->d_mp.m_pathname));
		    
	}

	DCTL_LOCK(&dctl_lock);
	if ((error = dctl_record_event(msg)) != 0)
		kmem_free(msg, sizeof(*msg));
	DCTL_UNLOCK(&dctl_lock);
	return error;
}

/*
 * The attributes of some device special node have been
 * modified, so enque a message to let devfsd know.
 *
 * NOTE: We treat updates to filenames, i.e. performing a mv(1),
 * separately from other updates such as chmod(1), etc. If the
 * filename is not being updated, nfilename must be NULL.
 */
int
dctl_attr_msg(int32_t mount_cookie, intptr_t dev_cookie, mode_t nmode, 
	uid_t nuid, gid_t ngid, int nflags, char *nfilename)
{
	int error;
	struct dctl_msg *msg;

	if ((msg = kmem_zalloc(sizeof(*msg), KM_SLEEP)) == NULL) {
		printf("could not alloc memory for dctl message\n");
		return -1;
	}

	/* Cookies to uniquely identify this device node */
	msg->d_sn.sc_dev = dev_cookie;
	msg->d_sn.sc_mount = mount_cookie;

	if (nfilename == NULL)
		msg->d_type = DCTL_UPDATE_NODE_ATTR;
	else
		msg->d_type = DCTL_UPDATE_NODE_NAME;

	msg->d_attr.d_mode = nmode;
	msg->d_attr.d_uid = nuid;
	msg->d_attr.d_gid = ngid;
	msg->d_attr.d_flags = nflags;

	/*
	 * We pass back the parent directory and path component.
	 * NOTE: We currently don't allow creation of directories
	 *	 so the parent will always be "2".
	 */
	strlcpy(msg->d_attr.d_component.out_pthcmp.d_pthcmp, nfilename, 
	     sizeof(msg->d_attr.d_component.out_pthcmp.d_pthcmp));

	DCTL_LOCK(&dctl_lock);
	if ((error = dctl_record_event(msg)) != 0)
		kmem_free(msg, sizeof(*msg));
	DCTL_UNLOCK(&dctl_lock);
	return error;
}

/*
 * Given attributes for a new device node ask the devfs identifed
 * by 'dcookie' to create it for us.
 */
static int
dctl_push_node(struct dctl_node_cookie nc, struct dctl_specnode_attr *na)
{
	int error;
	int32_t mcookie = nc.sc_mount;
	int32_t dcookie = nc.sc_dev;
	const char *path = na->d_component.in_pthcmp.d_filename;

	error = devfs_create_node(mcookie, path, dcookie, na->d_uid, 
	    na->d_gid, na->d_mode, na->d_flags);
	    
	return error;
}

/*
 * Record a new event to be sent to userland. 
 *
 * A new event occurs when,
 *	- A new device has been discovered/removed
 *	- The attributes of a device special file have changed
 *	- A new devfs mount point has been created/destroyed
 *
 * 'msg' will tell us what sort of event has occurred.
 *
 * XXX: This must always be called with DCTL_LOCK held.
 */
static int
dctl_record_event(struct dctl_msg *msg)
{
	struct dctl_event_entry *de;

	if ((de = kmem_zalloc(sizeof(*de), KM_SLEEP)) == NULL) {
		printf("unable to allocate memory for dev_event\n");
		return -1;
	}

	de->de_msg = msg;
	SIMPLEQ_INSERT_TAIL(&event_list, de, de_entries);
	
	sc->sc_event_count++;
	selnotify(&sc->sc_rsel, 0);
	return 0;
}

/*
 * XXX: We keep checking to see if any new devices that we don't know
 * about have been found. We don't currently detect whether devices have
 * been removed.
 */
static void
dctl_periodic_device_check(void)
{
	struct device *dev;
	struct dctl_msg *msg;

	/* Check for new devices */
	TAILQ_FOREACH(dev, &alldevs, dv_list) {
		if (dev->dv_dlist.tqe_prev == NULL) {

			/* Found a new device so create a new msg */
			msg = kmem_zalloc(sizeof(*msg), KM_SLEEP);
			if (msg == NULL) {
				printf("couldn't alloc memory for message\n");
				continue;
			}

			msg->d_type = DCTL_NEW_DEVICE;

			/* cookie */
			msg->d_dc = (intptr_t)dev;
			strlcpy(msg->d_kdev.k_name, device_xname(dev), 
			    sizeof(msg->d_kdev));

			TAILQ_INSERT_TAIL(&ourdevs, dev, dv_dlist);
			if (dctl_record_event(msg) == -1) {
				printf("could not record new device event\n");
				kmem_free(msg, sizeof(*msg));
				return;
			}
		}
	}
}

void
dctlattach(int n)
{
	if (dctl_initialised)
		return;
	dctl_initialised = 1;

	if ((sc = kmem_zalloc(sizeof(*sc), KM_SLEEP)) == NULL) {
		printf("unable to allocate mem, dctl dead\n");
		return;
	}
	TAILQ_INIT(&sc->sc_devlist);

	mutex_init(&dctl_lock, MUTEX_DEFAULT, IPL_NONE);
	SIMPLEQ_INIT(&event_list);

	if (kthread_create(PRI_NONE, 0, NULL, dctl_thread, NULL,
	    &dctl_thread_handle, "dctl_thread") != 0) {
		/*
		 * We were unable to create the DCTL thread; bail out.
		 */
		printf("unable to create thread, "
		    "kernel DCTL support disabled\n");
	}
}

void
dctl_thread(void *arg)
{
	/*
	 * Loop forever, doing a periodic check for DCTL events.
	 */
	for (;;) {
		DCTL_LOCK(&dctl_lock); 
		dctl_periodic_device_check();
		DCTL_UNLOCK(&dctl_lock);
		(void) tsleep(sc, PWAIT, "dctlev",  (8 * hz) / 7);
	}
}

int
dctlopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int error = 0;

	DCTL_LOCK(&dctl_lock); 
	if (dctl_open != 0)
		error = EBUSY;
	else 
		dctl_open = 1;
	DCTL_UNLOCK(&dctl_lock);

	return error;
}

int
dctlclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	DCTL_LOCK(&dctl_lock); 
	struct device *odev;

	while ((odev = TAILQ_FIRST(&ourdevs)) != NULL) {
		TAILQ_REMOVE(&ourdevs, odev, dv_dlist);
		odev->dv_dlist.tqe_prev = NULL;
	}

	dctl_open = 0;
	sc->sc_event_count = 0;

	DCTL_UNLOCK(&dctl_lock);
	return 0;
}

int
dctlioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error;
	struct dctl_msg *msg;
	struct dctl_event_entry *ee;

	DCTL_LOCK(&dctl_lock);
	switch (cmd) {
	case DCTL_IOC_NEXTEVENT:
		if (!sc->sc_event_count)
			error = EAGAIN;
		else {
			msg = (struct dctl_msg *)data;
			if (msg == NULL) {
				error = EINVAL;
				break;
			}

			ee = SIMPLEQ_FIRST(&event_list);
			*msg = *ee->de_msg;
			SIMPLEQ_REMOVE_HEAD(&event_list, de_entries);
			kmem_free(ee->de_msg, sizeof(*ee->de_msg));
			kmem_free(ee, sizeof(*ee));
			sc->sc_event_count--;
			error = 0;
		}
		break;
	case DCTL_IOC_CREATENODE:
		msg = (struct dctl_msg *)data;
		if (msg == NULL) {
			printf("argument is NULL\n");
			error = EINVAL;
			break;
		}

		/* 
		 * Create a device special file for this device using
		 * the attributes passed to us from userland.
		 */
		error = dctl_push_node(msg->d_sn, &msg->d_attr);
		break;
	default:
		error = ENOTTY;
	}
	DCTL_UNLOCK(&dctl_lock);

	return error;
}

int
dctlpoll(dev_t dev, int events, struct lwp *l)
{
	int revents = 0;

	DCTL_LOCK(&dctl_lock);
	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_event_count)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(l, &sc->sc_rsel);
	}
	DCTL_UNLOCK(&dctl_lock);

	return (revents);
}

static void
filt_dctlrdetach(struct knote *kn)
{
	struct dctl_softc *sd = kn->kn_hook;

	DCTL_LOCK(&dctl_lock);
	SLIST_REMOVE(&sd->sc_rsel.sel_klist, kn, knote, kn_selnext);
	DCTL_UNLOCK(&dctl_lock);
}

static int
filt_dctlread(struct knote *kn, long hint)
{
	struct dctl_softc *sd = kn->kn_hook;

	kn->kn_data = sd->sc_event_count;
	return (kn->kn_data > 0);
}

static const struct filterops dctlread_filtops =
	{ 1, NULL, filt_dctlrdetach, filt_dctlread };

int
dctlkqfilter(dev_t dev, struct knote *kn)
{
	struct klist *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rsel.sel_klist;
		kn->kn_fop = &dctlread_filtops;
		break;

	default:
		return (EINVAL);
	}

	kn->kn_hook = sc;

	DCTL_LOCK(&dctl_lock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	DCTL_UNLOCK(&dctl_lock);

	return (0);
}
