/* 	$NetBSD: devfsctl.c,v 1.1.2.7 2008/04/14 16:23:56 mjf Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: devfsctl.c,v 1.1.2.7 2008/04/14 16:23:56 mjf Exp $");

#if defined(DEBUG) && !defined(DEVFSCTLDEBUG)
#define	DEVFSCTLDEBUG
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/queue.h>
#include <sys/conf.h>
#include <sys/namei.h>
#include <sys/dirent.h>

#include <dev/devfsctl/devfsctlvar.h>
#include <dev/devfsctl/devfsctl.h>
#include <fs/devfs/devfs_comm.h>

#include <machine/stdarg.h>

#ifdef DEVFSCTLDEBUG
#define DPRINTF(f, x)		do { if (devfsctldebug & (f)) printf x; } while (0)


#ifdef DEVFSCTLDEBUG_VALUE
int	devfsctldebug = DEVFSCTLDEBUG_VALUE;
#else
int	devfsctldebug = 0;
#endif /* DEVFSCTLDEBUG_VALUE */

#else
#define	DPRINTF(f, x)		/**/
#endif /* DEVFSCTLDEBUG */

/*
 * A brief note on the locking protocol: it's very simple; we
 * assert an exclusive lock any time thread context enters the
 * DEVFSCTL module.  This is both the DEVFSCTL thread itself, as well as
 * user context.
 */
#define	DEVFSCTL_LOCK(devfsctlsc)					\
	(void) mutex_enter(devfsctlsc)
#define	DEVFSCTL_UNLOCK(devfsctlsc)					\
	(void) mutex_exit(devfsctlsc)

static int	devfsctl_record_event(struct devfsctl_msg *);
static void	devfsctl_periodic_device_check(void);
static void	devfsctl_thread(void *);
static int	devfsctl_mount_change(const char *, int32_t , enum dmsgtype, int);

void devfsctl_init(void);
void devfsctlattach(int);

dev_type_open(devfsctlopen);
dev_type_close(devfsctlclose);
dev_type_ioctl(devfsctlioctl);
dev_type_poll(devfsctlpoll);
dev_type_kqfilter(devfsctlkqfilter);

const struct cdevsw devfsctl_cdevsw = {
	devfsctlopen, devfsctlclose, noread, nowrite, devfsctlioctl,
	nostop, notty, devfsctlpoll, nommap, devfsctlkqfilter, D_OTHER,
};

/* variables used during operation (XXX cgd) */
static 	int devfsctl_open = 0;
static	struct devfsctl_softc *sc;
static 	lwp_t *devfsctl_thread_handle;
static 	kmutex_t devfsctl_lock;
static 	SIMPLEQ_HEAD(,devfsctl_event_entry) event_list = 
    SIMPLEQ_HEAD_INITIALIZER(event_list);
static 	SIMPLEQ_HEAD(,devfsctl_event_entry) mount_event_list = 
    SIMPLEQ_HEAD_INITIALIZER(mount_event_list);
static	int devfsctl_initialised = 0;

/*
 * A new devfs mount has been created, so kick devfsd(8). What devfsd 
 * will actually do is figure out appropriate attributes for device nodes
 * that need to populate this mount and then pass them through devfsctl to the
 * mount.
 *
 * 'path' is the absolute pathname this devfs is mounted on.
 * 'cookie' uniquely identifies the mount point.
 */
int
devfsctl_mount_msg(const char *path, int32_t cookie, int visibility)
{

	return devfsctl_mount_change(path, cookie, DEVFSCTL_NEW_MOUNT,
	    visibility);
}

/*
 * A devfs mount point has been destroyed.
 */
int
devfsctl_unmount_msg(int32_t cookie, int visibility)
{

	return devfsctl_mount_change(NULL, cookie, DEVFSCTL_UNMOUNT,
	    visibility);
}

/*
 * If this function is called because a devfs has been mounted
 * then the 'path' argument must be a valid path, specifically,
 * it must be the mount point pathname of the devfs being mounted.
 */
static int
devfsctl_mount_change(const char *path, int32_t cookie, enum dmsgtype type,
		      int visibility)
{
	int error;
	struct devfsctl_msg *msg;

	if ((msg = kmem_zalloc(sizeof(*msg), KM_SLEEP)) == NULL) {
		printf("could not alloc memory for devfsctl message\n");
		return -1;
	}

	msg->d_type = type;
	msg->d_mc = cookie;

	if (type != DEVFSCTL_UNMOUNT) {
		KASSERT(path != NULL);
		strlcpy(msg->d_mp.m_pathname, 
		    path, sizeof(msg->d_mp.m_pathname));
		    
	}

	msg->d_mp.m_visibility = visibility;

	DEVFSCTL_LOCK(&devfsctl_lock);
	if ((error = devfsctl_record_event(msg)) != 0)
		kmem_free(msg, sizeof(*msg));
	DEVFSCTL_UNLOCK(&devfsctl_lock);
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
devfsctl_attr_msg(int32_t mount_cookie, dev_t dev_cookie, mode_t nmode, 
	uid_t nuid, gid_t ngid, int nflags, char *nfilename)
{
	int error;
	struct devfsctl_msg *msg;

	if ((msg = kmem_zalloc(sizeof(*msg), KM_SLEEP)) == NULL) {
		printf("could not alloc memory for devfsctl message\n");
		return -1;
	}

	/* Cookies to uniquely identify this device node */
	msg->d_sn.sc_dev = dev_cookie;
	msg->d_sn.sc_mount = mount_cookie;

	if (nfilename == NULL)
		msg->d_type = DEVFSCTL_UPDATE_NODE_ATTR;
	else
		msg->d_type = DEVFSCTL_UPDATE_NODE_NAME;

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

	DEVFSCTL_LOCK(&devfsctl_lock);
	if ((error = devfsctl_record_event(msg)) != 0)
		kmem_free(msg, sizeof(*msg));
	DEVFSCTL_UNLOCK(&devfsctl_lock);
	return error;
}

/*
 * Given attributes for a new device node ask the devfs identifed
 * by 'dcookie' to create it for us.
 */
static int
devfsctl_push_node(struct devfsctl_node_cookie nc,
		   struct devfsctl_specnode_attr *na)
{
	int error;
	int32_t mcookie = nc.sc_mount;
	dev_t dcookie = nc.sc_dev;
	const char *path = na->d_component.in_pthcmp.d_filename;
	struct device_name *dn;

	/* Lookup the device_name for this device */
	dn = device_lookup_info(dcookie, na->d_char);
	if (dn == NULL)
		return EINVAL;

	/* If this device is in the process of being removed, just return */
	if (dn->d_gone == true)
		return 0;

	error = devfs_create_node(mcookie, path, dcookie, na->d_uid, 
	    na->d_gid, na->d_mode, na->d_flags, na->d_char);
	    
	/* 
	 * Someone is waiting to found out if we succeeded in
	 * creating a device file.
	 */
	if (cv_is_valid(&dn->d_cv) == true) {
		mutex_enter(&dn->d_cvmutex);
		dn->d_busy = false;
		dn->d_retval = error;
		cv_signal(&dn->d_cv);
		mutex_exit(&dn->d_cvmutex);
	}

	return error;
}

/*
 * Given attributes for a new device node ask the devfs identifed
 * by 'dcookie' to delete it.
 */
static int
devfsctl_delete_node(struct devfsctl_node_cookie nc,
		     struct devfsctl_specnode_attr *na)
{
	int error;
	int32_t mcookie = nc.sc_mount;
	dev_t dcookie = nc.sc_dev;
	const char *path = na->d_component.in_pthcmp.d_filename;

	error = devfs_remove_node(mcookie, path, dcookie, na->d_flags);
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
 * XXX: This must always be called with DEVFSCTL_LOCK held.
 */
static int
devfsctl_record_event(struct devfsctl_msg *msg)
{
	struct devfsctl_event_entry *de;

	KASSERT(mutex_owned(&devfsctl_lock));

	if ((de = kmem_zalloc(sizeof(*de), KM_SLEEP)) == NULL) {
		printf("unable to allocate memory for dev_event\n");
		return -1;
	}

	de->de_msg = msg;
	de->de_on_mount = 0;
	SIMPLEQ_INSERT_TAIL(&event_list, de, de_entries);
	
	sc->sc_event_count++;
	selnotify(&sc->sc_rsel, 0, NOTE_SUBMIT);
	return 0;
}

/*
 * XXX: We keep checking to see if any new devices that we don't know
 * about have been found. We don't currently detect whether devices have
 * been removed.
 */
static void
devfsctl_periodic_device_check(void)
{
	struct device_name *dn;
	struct devfsctl_msg *msg;

	/* Check for new and removed devices */
	mutex_enter(&dname_lock);
	TAILQ_FOREACH(dn, &device_names, d_next) {
		if (dn->d_found == false) {

			/* Found a new device so create a new msg */
			msg = kmem_zalloc(sizeof(*msg), KM_SLEEP);
			if (msg == NULL) {
				printf("couldn't alloc memory for message\n");
				continue;
			}

			msg->d_type = DEVFSCTL_NEW_DEVICE;

			/* cookie */
			msg->d_dc = dn->d_dev;
			strlcpy(msg->d_kdev.k_name, dn->d_name, 
			    sizeof(msg->d_kdev));

			/* type of device */
			msg->d_kdev.k_type = dn->d_type;

			/* block or char */
			msg->d_kdev.k_char = dn->d_char;

			dn->d_found = true;
			if (devfsctl_record_event(msg) == -1) {
				printf("could not record new device event\n");
				kmem_free(msg, sizeof(*msg));
				return;
			}
		}

		if (dn->d_gone == true) {
			msg = kmem_zalloc(sizeof(*msg), KM_SLEEP);
			if (msg == NULL) {
				printf("couldn't alloc memory for message\n");
				continue;
			}

			msg->d_type = DEVFSCTL_REMOVED_DEVICE;

			/* cookie */
			msg->d_dc = dn->d_dev;
			strlcpy(msg->d_kdev.k_name, dn->d_name, 
			    sizeof(msg->d_kdev));

			TAILQ_REMOVE(&device_names, dn, d_next);

			if (devfsctl_record_event(msg) == -1) {
				printf("could not record new device event\n");
				kmem_free(msg, sizeof(*msg));
				return;
			}
			kmem_free(dn->d_name, MAXNAMLEN);
			kmem_free(dn, sizeof(*dn));
		}
	}
	mutex_exit(&dname_lock);
}

void
devfsctlattach(int n)
{

	if (devfsctl_initialised)
		return;
	devfsctl_initialised = 1;

	if ((sc = kmem_zalloc(sizeof(*sc), KM_SLEEP)) == NULL) {
		printf("unable to allocate mem, devfsctl dead\n");
		return;
	}
	TAILQ_INIT(&sc->sc_devlist);

	mutex_init(&devfsctl_lock, MUTEX_DEFAULT, IPL_NONE);
	SIMPLEQ_INIT(&event_list);

	if (kthread_create(PRI_NONE, 0, NULL, devfsctl_thread, NULL,
	    &devfsctl_thread_handle, "devfsctl") != 0) {
		/*
		 * We were unable to create the DEVFSCTL thread; bail out.
		 */
		printf("unable to create thread, "
		    "kernel DEVFSCTL support disabled\n");
	}
}

void
devfsctl_thread(void *arg)
{

	/*
	 * Loop forever, doing a periodic check for DEVFSCTL events.
	 */
	DEVFSCTL_LOCK(&devfsctl_lock); 
	for (;;) {
		devfsctl_periodic_device_check();
		kpause("devfsev", false, (8 * hz) / 7, &devfsctl_lock);
	}
}

int
devfsctlopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int error = 0;
	struct devfsctl_event_entry *ee;

	DEVFSCTL_LOCK(&devfsctl_lock); 
	if (devfsctl_open != 0)
		error = EBUSY;
	else 
		devfsctl_open = 1;

	/* Copy all mounts onto the main event list. */
	SIMPLEQ_FOREACH(ee, &mount_event_list, dm_entries) {
		SIMPLEQ_INSERT_TAIL(&event_list, ee, de_entries);
		sc->sc_event_count++;
	}
	DEVFSCTL_UNLOCK(&devfsctl_lock);

	if (!SIMPLEQ_EMPTY(&mount_event_list))
		selnotify(&sc->sc_rsel, 0, 0);

	return error;
}

int
devfsctlclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct device_name *dn;

	DEVFSCTL_LOCK(&devfsctl_lock); 

	mutex_enter(&dname_lock);
	TAILQ_FOREACH(dn, &device_names, d_next)
		dn->d_found = false;
	mutex_exit(&dname_lock);

	devfsctl_open = 0;
	sc->sc_event_count = 0;

	DEVFSCTL_UNLOCK(&devfsctl_lock);
	return 0;
}

int
devfsctlioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error;
	struct devfsctl_msg *msg;
	struct devfsctl_event_entry *ee, *ef;
	struct devfsctl_ioctl_data *idata;

	DEVFSCTL_LOCK(&devfsctl_lock);
	switch (cmd) {
	case DEVFSCTL_IOC_NEXTEVENT:
		if (!sc->sc_event_count)
			error = EAGAIN;
		else {
			msg = (struct devfsctl_msg *)data;
			if (msg == NULL) {
				error = EINVAL;
				break;
			}

			ee = SIMPLEQ_FIRST(&event_list);
			*msg = *ee->de_msg;
			SIMPLEQ_REMOVE_HEAD(&event_list, de_entries);

			/* move to the mount list */
			if (ee->de_msg->d_type == DEVFSCTL_NEW_MOUNT) {
				/* is it already there? */
				if (ee->de_on_mount == 0) {
					SIMPLEQ_INSERT_TAIL(&mount_event_list,
					    ee, dm_entries);
					ee->de_on_mount = 1;
				}
				sc->sc_event_count--;
				error = 0;
			} else if (ee->de_msg->d_type == DEVFSCTL_UNMOUNT) {
				/* 
				 * Search the list of mount events and remove
				 * this mount.
				 */
				SIMPLEQ_FOREACH(ef, &mount_event_list,
				    dm_entries) {
					if (ef->de_msg->d_dc ==
					    ee->de_msg->d_dc)
						break;
				}

				if (ef == NULL)
					panic("devfs umount without mount\n");

				SIMPLEQ_REMOVE(&mount_event_list, ef, 
				    devfsctl_event_entry, dm_entries);

				kmem_free(ef->de_msg, sizeof(*ef->de_msg));
				kmem_free(ef, sizeof(*ef));

				kmem_free(ee->de_msg, sizeof(*ee->de_msg));
				kmem_free(ee, sizeof(*ee));
				sc->sc_event_count--;
				error = 0;

			} else {
				kmem_free(ee->de_msg, sizeof(*ee->de_msg));
				kmem_free(ee, sizeof(*ee));
				sc->sc_event_count--;
				error = 0;
			}
		}
		break;
	case DEVFSCTL_IOC_CREATENODE:
		msg = (struct devfsctl_msg *)data;
		if (msg == NULL) {
			error = EINVAL;
			break;
		}

		/* 
		 * Create a device special file for this device using
		 * the attributes passed to us from userland.
		 */
		error = devfsctl_push_node(msg->d_sn, &msg->d_attr);
		break;
	case DEVFSCTL_IOC_DELETENODE:
		msg = (struct devfsctl_msg *)data;
		if (msg == NULL) {
			error = EINVAL;
			break;
		}
		error = devfsctl_delete_node(msg->d_sn, &msg->d_attr);
		break;
	case DEVFSCTL_IOC_INNERIOCTL:
		/* 
		 * Because devfsd(8) needs to query devices through ioctls
		 * _before_ their nodes have been pushed into devfs file
		 * systems we provide a way to perform the ioctl without
		 * the need for an open file descriptor in userland.
		 */
		error = EINVAL;

		idata = (struct devfsctl_ioctl_data *)data;

		/* Handle disk devices */
		if (idata->d_type == DEV_DISK) {
			int partno;			/* partition number */
			struct disklabel dlabel;
			struct partition part;

			/* Try to get wedge info, if supported */
			error = bdev_ioctl(idata->d_dev, DIOCGWEDGEINFO,
			    &idata->i_args.di.di_winfo, 0, curlwp);

			if (error && (error != ENOTTY))
				break;

			partno = DISKPART(idata->d_dev);
			part = dlabel.d_partitions[partno];

			if (part.p_fstype == FS_BSDFFS)
				idata->i_args.di.di_fstype = "BSDFFS";
			else if (part.p_fstype == FS_SWAP)
				idata->i_args.di.di_fstype = "SWAP";
			else if (part.p_fstype == FS_MSDOS)
				idata->i_args.di.di_fstype = "MSDOS";
			else if (part.p_fstype == FS_BSDLFS)
				idata->i_args.di.di_fstype = "LFS";
			else if (part.p_fstype == FS_ISO9660)
				idata->i_args.di.di_fstype = "ISO9660";
			else if (part.p_fstype == FS_ADOS)
				idata->i_args.di.di_fstype = "ADOS";
			else if (part.p_fstype == FS_HFS)
				idata->i_args.di.di_fstype = "HFS";
			else if (part.p_fstype == FS_FILECORE)
				idata->i_args.di.di_fstype = "FILECORE";
			else if (part.p_fstype == FS_EX2FS)
				idata->i_args.di.di_fstype = "EXT2FS";
			else if (part.p_fstype == FS_NTFS)
				idata->i_args.di.di_fstype = "NTFS";
			else if (part.p_fstype == FS_RAID)
				idata->i_args.di.di_fstype = "RAID";
			else if (part.p_fstype == FS_CCD)
				idata->i_args.di.di_fstype = "CCD";
			else if (part.p_fstype == FS_APPLEUFS)
				idata->i_args.di.di_fstype = "APPLEUFS";
		}	
		break;
	default:
		error = ENOTTY;
		break;
	}
	DEVFSCTL_UNLOCK(&devfsctl_lock);

	return error;
}

int
devfsctlpoll(dev_t dev, int events, struct lwp *l)
{
	int revents = 0;

	DEVFSCTL_LOCK(&devfsctl_lock);
	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_event_count)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(l, &sc->sc_rsel);
	}
	DEVFSCTL_UNLOCK(&devfsctl_lock);

	return (revents);
}

static void
filt_devfsctlrdetach(struct knote *kn)
{
	struct devfsctl_softc *sd = kn->kn_hook;

	DEVFSCTL_LOCK(&devfsctl_lock);
	SLIST_REMOVE(&sd->sc_rsel.sel_klist, kn, knote, kn_selnext);
	DEVFSCTL_UNLOCK(&devfsctl_lock);
}

static int
filt_devfsctlread(struct knote *kn, long hint)
{
	struct devfsctl_softc *sd = kn->kn_hook;
	int rv;

	if (hint != NOTE_SUBMIT)
		DEVFSCTL_LOCK(&devfsctl_lock);
	kn->kn_data = sd->sc_event_count;
	rv = (kn->kn_data > 0);
	if (hint != NOTE_SUBMIT)
		DEVFSCTL_UNLOCK(&devfsctl_lock);

	return rv;
}

static const struct filterops devfsctlread_filtops =
	{ 1, NULL, filt_devfsctlrdetach, filt_devfsctlread };

int
devfsctlkqfilter(dev_t dev, struct knote *kn)
{
	struct klist *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rsel.sel_klist;
		kn->kn_fop = &devfsctlread_filtops;
		break;

	default:
		return (EINVAL);
	}

	kn->kn_hook = sc;

	DEVFSCTL_LOCK(&devfsctl_lock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	DEVFSCTL_UNLOCK(&devfsctl_lock);

	return (0);
}
