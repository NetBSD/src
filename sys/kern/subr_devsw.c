/*	$NetBSD: subr_devsw.c,v 1.34.2.6 2016/07/17 21:39:17 pgoyette Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by MAEKAWA Masahide <gehenna@NetBSD.org>, and by Andrew Doran.
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
 * Overview
 *
 *	subr_devsw.c: registers device drivers by name and by major
 *	number, and provides wrapper methods for performing I/O and
 *	other tasks on device drivers, keying on the device number
 *	(dev_t).
 *
 *	When the system is built, the config(8) command generates
 *	static tables of device drivers built into the kernel image
 *	along with their associated methods.  These are recorded in
 *	the cdevsw0 and bdevsw0 tables.  Drivers can also be added to
 *	and removed from the system dynamically.
 *
 * Allocation
 *
 *	When the system initially boots only the statically allocated
 *	indexes (bdevsw0, cdevsw0) are used.  If these overflow due to
 *	allocation, we allocate a fixed block of memory to hold the new,
 *	expanded index.  This "fork" of the table is only ever performed
 *	once in order to guarantee that other threads may safely access
 *	the device tables:
 *
 *	o Once a thread has a "reference" to the table via an earlier
 *	  open() call, we know that the entry in the table must exist
 *	  and so it is safe to access it.
 *
 *	o Regardless of whether other threads see the old or new
 *	  pointers, they will point to a correct device switch
 *	  structure for the operation being performed.
 *
 *	XXX Currently, the wrapper methods such as cdev_read() verify
 *	that a device driver does in fact exist before calling the
 *	associated driver method.  This should be changed so that
 *	once the device is has been referenced by a vnode (opened),
 *	calling	the other methods should be valid until that reference
 *	is dropped.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_devsw.c,v 1.34.2.6 2016/07/17 21:39:17 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_dtrace.h"
#endif

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/poll.h>
#include <sys/tty.h>
#include <sys/cpu.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/sdt.h>
#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/localcount.h>
#include <sys/pserialize.h>

#ifdef DEVSW_DEBUG
#define	DPRINTF(x)	printf x
#else /* DEVSW_DEBUG */
#define	DPRINTF(x)
#endif /* DEVSW_DEBUG */

#define	MAXDEVSW	512	/* the maximum of major device number */
#define	BDEVSW_SIZE	(sizeof(struct bdevsw *))
#define	CDEVSW_SIZE	(sizeof(struct cdevsw *))
#define	DEVSWCONV_SIZE	(sizeof(struct devsw_conv))

extern const struct bdevsw **bdevsw, *bdevsw0[];
extern const struct cdevsw **cdevsw, *cdevsw0[];
extern struct devsw_conv *devsw_conv, devsw_conv0[];
extern const int sys_bdevsws, sys_cdevsws;
extern int max_bdevsws, max_cdevsws, max_devsw_convs;

static int bdevsw_attach(const struct bdevsw *, devmajor_t *);
static int cdevsw_attach(const struct cdevsw *, devmajor_t *);
static void devsw_detach_locked(const struct bdevsw *, const struct cdevsw *);

kmutex_t	device_lock;
kcondvar_t	device_cv;
pserialize_t	device_psz = NULL;

void (*biodone_vfs)(buf_t *) = (void *)nullop;

void
devsw_init(void)
{

	KASSERT(sys_bdevsws < MAXDEVSW - 1);
	KASSERT(sys_cdevsws < MAXDEVSW - 1);
	mutex_init(&device_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&device_cv, "devsw");
}

void
devsw_detach_init(void)
{

	device_psz = pserialize_create();
}

int
devsw_attach(const char *devname,
	     const struct bdevsw *bdev, devmajor_t *bmajor,
	     const struct cdevsw *cdev, devmajor_t *cmajor)
{
	struct devsw_conv *conv;
	char *name;
	int error, i;
	size_t len;

	if (devname == NULL || cdev == NULL)
		return (EINVAL);

	mutex_enter(&device_lock);

	if (bdev != NULL) {
		KASSERTMSG(bdev->d_localcount != NULL,
		    "%s: bdev %s has no d_localcount", __func__, devname);
		KASSERTMSG(bdev->d_localcount != cdev->d_localcount,
		    "%s: bdev and cdev for %s have same d_localcount",
		    __func__, devname);
	}
	if (cdev != NULL)
		KASSERTMGS(cdev->d_localcount != NULL,
		    "%s: cdev %s has no d_localcount", __func__, devname);

	for (i = 0 ; i < max_devsw_convs ; i++) {
		conv = &devsw_conv[i];
		if (conv->d_name == NULL || strcmp(devname, conv->d_name) != 0)
			continue;

		if (*bmajor < 0)
			*bmajor = conv->d_bmajor;
		if (*cmajor < 0)
			*cmajor = conv->d_cmajor;

		if (*bmajor != conv->d_bmajor || *cmajor != conv->d_cmajor) {
			error = EINVAL;
			goto fail;
		}
		if ((*bmajor >= 0 && bdev == NULL) || *cmajor < 0) {
			error = EINVAL;
			goto fail;
		}

		if ((*bmajor >= 0 && bdevsw[*bmajor] != NULL) ||
		    cdevsw[*cmajor] != NULL) {
			error = EEXIST;
			goto fail;
		}

		/* use membar_producer() to ensure visibility of the xdevsw */
		if (bdev != NULL) {
			localcount_init(bdev->d_localcount);
			membar_producer();
			bdevsw[*bmajor] = bdev;
		}
		localcount_init(cdev->d_localcount);
		membar_producer();
		cdevsw[*cmajor] = cdev;

		mutex_exit(&device_lock);
		return (0);
	}

	error = bdevsw_attach(bdev, bmajor);
	if (error != 0) 
		goto fail;
	error = cdevsw_attach(cdev, cmajor);
	if (error != 0) {
		devsw_detach_locked(bdev, NULL);
		goto fail;
	}

	for (i = 0 ; i < max_devsw_convs ; i++) {
		if (devsw_conv[i].d_name == NULL)
			break;
	}
	if (i == max_devsw_convs) {
		struct devsw_conv *newptr;
		int old_convs, new_convs;

		old_convs = max_devsw_convs;
		new_convs = old_convs + 1;

		newptr = kmem_zalloc(new_convs * DEVSWCONV_SIZE, KM_NOSLEEP);
		if (newptr == NULL) {
			devsw_detach_locked(bdev, cdev);
			error = ENOMEM;
			goto fail;
		}
		newptr[old_convs].d_name = NULL;
		newptr[old_convs].d_bmajor = -1;
		newptr[old_convs].d_cmajor = -1;
		memcpy(newptr, devsw_conv, old_convs * DEVSWCONV_SIZE);
		if (devsw_conv != devsw_conv0)
			kmem_free(devsw_conv, old_convs * DEVSWCONV_SIZE);
		devsw_conv = newptr;
		max_devsw_convs = new_convs;
	}

	len = strlen(devname) + 1;
	name = kmem_alloc(len, KM_NOSLEEP);
	if (name == NULL) {
		devsw_detach_locked(bdev, cdev);
		error = ENOMEM;
		goto fail;
	}
	strlcpy(name, devname, len);

	devsw_conv[i].d_name = name;
	devsw_conv[i].d_bmajor = *bmajor;
	devsw_conv[i].d_cmajor = *cmajor;

	mutex_exit(&device_lock);
	return (0);
 fail:
	mutex_exit(&device_lock);
	return (error);
}

static int
bdevsw_attach(const struct bdevsw *devsw, devmajor_t *devmajor)
{
	const struct bdevsw **newptr;
	devmajor_t bmajor;
	int i;

	KASSERT(mutex_owned(&device_lock));

	if (devsw == NULL)
		return (0);

	if (*devmajor < 0) {
		for (bmajor = sys_bdevsws ; bmajor < max_bdevsws ; bmajor++) {
			if (bdevsw[bmajor] != NULL)
				continue;
			for (i = 0 ; i < max_devsw_convs ; i++) {
				if (devsw_conv[i].d_bmajor == bmajor)
					break;
			}
			if (i != max_devsw_convs)
				continue;
			break;
		}
		*devmajor = bmajor;
	}

	if (*devmajor >= MAXDEVSW) {
		printf("%s: block majors exhausted", __func__);
		return (ENOMEM);
	}

	if (*devmajor >= max_bdevsws) {
		KASSERT(bdevsw == bdevsw0);
		newptr = kmem_zalloc(MAXDEVSW * BDEVSW_SIZE, KM_NOSLEEP);
		if (newptr == NULL)
			return (ENOMEM);
		memcpy(newptr, bdevsw, max_bdevsws * BDEVSW_SIZE);
		bdevsw = newptr;
		max_bdevsws = MAXDEVSW;
	}

	if (bdevsw[*devmajor] != NULL)
		return (EEXIST);

	/* ensure visibility of the bdevsw */
	membar_producer();

	bdevsw[*devmajor] = devsw;
	KASSERTMSG(devsw->d_localcount != NULL, "%s: bdev for major %d has "
	    "no localcount", __func__, *devmajor);
	localcount_init(devsw->d_localcount);

	return (0);
}

static int
cdevsw_attach(const struct cdevsw *devsw, devmajor_t *devmajor)
{
	const struct cdevsw **newptr;
	devmajor_t cmajor;
	int i;

	KASSERT(mutex_owned(&device_lock));

	if (*devmajor < 0) {
		for (cmajor = sys_cdevsws ; cmajor < max_cdevsws ; cmajor++) {
			if (cdevsw[cmajor] != NULL)
				continue;
			for (i = 0 ; i < max_devsw_convs ; i++) {
				if (devsw_conv[i].d_cmajor == cmajor)
					break;
			}
			if (i != max_devsw_convs)
				continue;
			break;
		}
		*devmajor = cmajor;
	}

	if (*devmajor >= MAXDEVSW) {
		printf("%s: character majors exhausted", __func__);
		return (ENOMEM);
	}

	if (*devmajor >= max_cdevsws) {
		KASSERT(cdevsw == cdevsw0);
		newptr = kmem_zalloc(MAXDEVSW * CDEVSW_SIZE, KM_NOSLEEP);
		if (newptr == NULL)
			return (ENOMEM);
		memcpy(newptr, cdevsw, max_cdevsws * CDEVSW_SIZE);
		cdevsw = newptr;
		max_cdevsws = MAXDEVSW;
	}

	if (cdevsw[*devmajor] != NULL)
		return (EEXIST);

	/* ensure visibility of the bdevsw */
	membar_producer();

	cdevsw[*devmajor] = devsw;
	KASSERTMSG(devsw->d_localcount != NULL, "%s: cdev for major %d has "
	    "no localcount", __func__, *devmajor);
	localcount_init(devsw->d_localcount);

	return (0);
}

/*
 * First, look up both bdev and cdev indices, and remove the
 * {b,c]devsw[] entries so no new references can be taken.  Then
 * drain any existing references.
 */

static void
devsw_detach_locked(const struct bdevsw *bdev, const struct cdevsw *cdev)
{
	int i, j;

	KASSERT(mutex_owned(&device_lock));

	i = max_bdevsws;
	if (bdev != NULL) {
		for (i = 0 ; i < max_bdevsws ; i++) {
			if (bdevsw[i] != bdev)
				continue;

			KASSERTMSG(bdev->d_localcount != NULL,
			    "%s: no bdev localcount for major %d", __func__, i);
			break;
		}
	}
	j = max_cdevsws;
	if (cdev != NULL) {
		for (j = 0 ; j < max_cdevsws ; j++) {
			if (cdevsw[j] != cdev)
				continue;

			KASSERTMSG(cdev->d_localcount != NULL,
			    "%s: no cdev localcount for major %d", __func__, j);
			break;
		}
	}
	if (i < max_bdevsws)
		bdevsw[i] = NULL;
	if (j < max_cdevsws )
		cdevsw[j] = NULL;

	/* Wait for all current readers to finish with the devsw */
	pserialize_perform(device_psz);

	/*
	 * No new readers can reach the bdev and cdev via the
	 * {b,c}devsw[] arrays.  Wait for existing references to
	 * drain, and then destroy.
	 */

	if (i < max_bdevsws && bdev->d_localcount != NULL) {
		localcount_drain(bdev->d_localcount, &device_cv, &device_lock);
		localcount_fini(bdev->d_localcount);
	}
	if (j < max_cdevsws && cdev->d_localcount != NULL ) {
		localcount_drain(cdev->d_localcount, &device_cv, &device_lock);
		localcount_fini(cdev->d_localcount);
	}
}

int
devsw_detach(const struct bdevsw *bdev, const struct cdevsw *cdev)
{

	mutex_enter(&device_lock);
	devsw_detach_locked(bdev, cdev);
	mutex_exit(&device_lock);
	return 0;
}

/*
 * Look up a block device by number.
 *
 * => Caller must ensure that the device is attached.
 */
const struct bdevsw *
bdevsw_lookup(dev_t dev)
{
	devmajor_t bmajor;

	if (dev == NODEV)
		return (NULL);
	bmajor = major(dev);
	if (bmajor < 0 || bmajor >= max_bdevsws)
		return (NULL);

	return (bdevsw[bmajor]);
}

const struct bdevsw *
bdevsw_lookup_acquire(dev_t dev)
{
	devmajor_t bmajor;
	const struct bdevsw *bdev = NULL;
	int s;

	if (dev == NODEV)
		return (NULL);
	bmajor = major(dev);
	if (bmajor < 0 || bmajor >= max_bdevsws)
		return (NULL);

	/* Start a read transaction to block localcount_drain() */
	s = pserialize_read_enter();

	/* Get the struct bdevsw pointer */
	bdev = bdevsw[bmajor];
	if (bdev == NULL)
		goto out;

	/* Wait for the content of the struct bdevsw to become visible */
	membar_datadep_consumer();

	/* If the devsw is not statically linked, acquire a reference */
	if (bdevsw[bmajor]->d_localcount != NULL)
		localcount_acquire(bdevsw[bmajor]->d_localcount);

out:	pserialize_read_exit(s);

	return bdev;
}

void
bdevsw_release(const struct bdevsw *bd)
{

	KASSERT(bd != NULL);
	if (bd->d_localcount != NULL)
		localcount_release(bd->d_localcount, &device_cv, &device_lock);
}

/*
 * Look up a character device by number.
 *
 * => Caller must ensure that the device is attached.
 */
const struct cdevsw *
cdevsw_lookup(dev_t dev)
{
	devmajor_t cmajor;

	if (dev == NODEV)
		return (NULL);
	cmajor = major(dev);
	if (cmajor < 0 || cmajor >= max_cdevsws)
		return (NULL);

	return (cdevsw[cmajor]);
}

const struct cdevsw *
cdevsw_lookup_acquire(dev_t dev)
{
	devmajor_t cmajor;
	const struct cdevsw *cdev = NULL;
	int s;

	if (dev == NODEV)
		return (NULL);
	cmajor = major(dev);
	if (cmajor < 0 || cmajor >= max_cdevsws)
		return (NULL);

	/* Start a read transaction to block localcount_drain() */
	s = pserialize_read_enter();

	/* Get the struct bdevsw pointer */
	cdev = cdevsw[cmajor];
	if (cdev == NULL)
		goto out;

	/* Wait for the content of the struct cdevsw to become visible */
	membar_datadep_consumer();

	/* If the devsw is not statically linked, acquire a reference */
	if (cdevsw[cmajor]->d_localcount != NULL)
		localcount_acquire(cdevsw[cmajor]->d_localcount);

out:	pserialize_read_exit(s);
	mutex_exit(&device_lock);

	return cdev;
}

void
cdevsw_release(const struct cdevsw *cd)
{

	KASSERT(cd != NULL);
	if (cd->d_localcount != NULL)
		localcount_release(cd->d_localcount, &device_cv, &device_lock);
}

/*
 * Look up a block device by reference to its operations set.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the returned major is still valid when dereferenced.
 */
devmajor_t
bdevsw_lookup_major(const struct bdevsw *bdev)
{
	devmajor_t bmajor;

	for (bmajor = 0 ; bmajor < max_bdevsws ; bmajor++) {
		if (bdevsw[bmajor] == bdev)
			return (bmajor);
	}

	return (NODEVMAJOR);
}

/*
 * Look up a character device by reference to its operations set.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the returned major is still valid when dereferenced.
 */
devmajor_t
cdevsw_lookup_major(const struct cdevsw *cdev)
{
	devmajor_t cmajor;

	for (cmajor = 0 ; cmajor < max_cdevsws ; cmajor++) {
		if (cdevsw[cmajor] == cdev)
			return (cmajor);
	}

	return (NODEVMAJOR);
}

/*
 * Convert from block major number to name.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the name pointer is still valid when dereferenced.
 */
const char *
devsw_blk2name(devmajor_t bmajor)
{
	const char *name;
	devmajor_t cmajor;
	int i;

	name = NULL;
	cmajor = -1;

	mutex_enter(&device_lock);
	if (bmajor < 0 || bmajor >= max_bdevsws || bdevsw[bmajor] == NULL) {
		mutex_exit(&device_lock);
		return (NULL);
	}
	for (i = 0 ; i < max_devsw_convs; i++) {
		if (devsw_conv[i].d_bmajor == bmajor) {
			cmajor = devsw_conv[i].d_cmajor;
			break;
		}
	}
	if (cmajor >= 0 && cmajor < max_cdevsws && cdevsw[cmajor] != NULL)
		name = devsw_conv[i].d_name;
	mutex_exit(&device_lock);

	return (name);
}

/*
 * Convert char major number to device driver name.
 */
const char *
cdevsw_getname(devmajor_t major)
{
	const char *name;
	int i;

	name = NULL;

	if (major < 0)
		return (NULL);
  
	mutex_enter(&device_lock);
	for (i = 0 ; i < max_devsw_convs; i++) {
		if (devsw_conv[i].d_cmajor == major) {
			name = devsw_conv[i].d_name;
			break;
		}
	}
	mutex_exit(&device_lock);
	return (name);
}

/*
 * Convert block major number to device driver name.
 */
const char *
bdevsw_getname(devmajor_t major)
{
	const char *name;
	int i;

	name = NULL;

	if (major < 0)
		return (NULL);
  
	mutex_enter(&device_lock);
	for (i = 0 ; i < max_devsw_convs; i++) {
		if (devsw_conv[i].d_bmajor == major) {
			name = devsw_conv[i].d_name;
			break;
		}
	}
	mutex_exit(&device_lock);
	return (name);
}

/*
 * Convert from device name to block major number.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the major number is still valid when dereferenced.
 */
devmajor_t
devsw_name2blk(const char *name, char *devname, size_t devnamelen)
{
	struct devsw_conv *conv;
	devmajor_t bmajor;
	int i;

	if (name == NULL)
		return (NODEVMAJOR);

	mutex_enter(&device_lock);
	for (i = 0 ; i < max_devsw_convs ; i++) {
		size_t len;

		conv = &devsw_conv[i];
		if (conv->d_name == NULL)
			continue;
		len = strlen(conv->d_name);
		if (strncmp(conv->d_name, name, len) != 0)
			continue;
		if (*(name +len) && !isdigit(*(name + len)))
			continue;
		bmajor = conv->d_bmajor;
		if (bmajor < 0 || bmajor >= max_bdevsws ||
		    bdevsw[bmajor] == NULL)
			break;
		if (devname != NULL) {
#ifdef DEVSW_DEBUG
			if (strlen(conv->d_name) >= devnamelen)
				printf("devsw_name2blk: too short buffer");
#endif /* DEVSW_DEBUG */
			strncpy(devname, conv->d_name, devnamelen);
			devname[devnamelen - 1] = '\0';
		}
		mutex_exit(&device_lock);
		return (bmajor);
	}

	mutex_exit(&device_lock);
	return (NODEVMAJOR);
}

/*
 * Convert from device name to char major number.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the major number is still valid when dereferenced.
 */
devmajor_t
devsw_name2chr(const char *name, char *devname, size_t devnamelen)
{
	struct devsw_conv *conv;
	devmajor_t cmajor;
	int i;

	if (name == NULL)
		return (NODEVMAJOR);

	mutex_enter(&device_lock);
	for (i = 0 ; i < max_devsw_convs ; i++) {
		size_t len;

		conv = &devsw_conv[i];
		if (conv->d_name == NULL)
			continue;
		len = strlen(conv->d_name);
		if (strncmp(conv->d_name, name, len) != 0)
			continue;
		if (*(name +len) && !isdigit(*(name + len)))
			continue;
		cmajor = conv->d_cmajor;
		if (cmajor < 0 || cmajor >= max_cdevsws ||
		    cdevsw[cmajor] == NULL)
			break;
		if (devname != NULL) {
#ifdef DEVSW_DEBUG
			if (strlen(conv->d_name) >= devnamelen)
				printf("devsw_name2chr: too short buffer");
#endif /* DEVSW_DEBUG */
			strncpy(devname, conv->d_name, devnamelen);
			devname[devnamelen - 1] = '\0';
		}
		mutex_exit(&device_lock);
		return (cmajor);
	}

	mutex_exit(&device_lock);
	return (NODEVMAJOR);
}

/*
 * Convert from character dev_t to block dev_t.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the major number is still valid when dereferenced.
 */
dev_t
devsw_chr2blk(dev_t cdev)
{
	devmajor_t bmajor, cmajor;
	int i;
	dev_t rv;

	cmajor = major(cdev);
	bmajor = NODEVMAJOR;
	rv = NODEV;

	mutex_enter(&device_lock);
	if (cmajor < 0 || cmajor >= max_cdevsws || cdevsw[cmajor] == NULL) {
		mutex_exit(&device_lock);
		return (NODEV);
	}
	for (i = 0 ; i < max_devsw_convs ; i++) {
		if (devsw_conv[i].d_cmajor == cmajor) {
			bmajor = devsw_conv[i].d_bmajor;
			break;
		}
	}
	if (bmajor >= 0 && bmajor < max_bdevsws && bdevsw[bmajor] != NULL)
		rv = makedev(bmajor, minor(cdev));
	mutex_exit(&device_lock);

	return (rv);
}

/*
 * Convert from block dev_t to character dev_t.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the major number is still valid when dereferenced.
 */
dev_t
devsw_blk2chr(dev_t bdev)
{
	devmajor_t bmajor, cmajor;
	int i;
	dev_t rv;

	bmajor = major(bdev);
	cmajor = NODEVMAJOR;
	rv = NODEV;

	mutex_enter(&device_lock);
	if (bmajor < 0 || bmajor >= max_bdevsws || bdevsw[bmajor] == NULL) {
		mutex_exit(&device_lock);
		return (NODEV);
	}
	for (i = 0 ; i < max_devsw_convs ; i++) {
		if (devsw_conv[i].d_bmajor == bmajor) {
			cmajor = devsw_conv[i].d_cmajor;
			break;
		}
	}
	if (cmajor >= 0 && cmajor < max_cdevsws && cdevsw[cmajor] != NULL)
		rv = makedev(cmajor, minor(bdev));
	mutex_exit(&device_lock);

	return (rv);
}

/*
 * Device access methods.
 */

#define	DEV_LOCK(d)						\
	if ((mpflag = (d->d_flag & D_MPSAFE)) == 0) {		\
		KERNEL_LOCK(1, NULL);				\
	}

#define	DEV_UNLOCK(d)						\
	if (mpflag == 0) {					\
		KERNEL_UNLOCK_ONE(NULL);			\
	}

int
bdev_open(dev_t dev, int flag, int devtype, lwp_t *l)
{
	const struct bdevsw *d;
	int rv, mpflag;

	/*
	 * For open we need to lock, in order to synchronize
	 * with attach/detach.
	 */
	mutex_enter(&device_lock);
	d = bdevsw_lookup(dev);
	mutex_exit(&device_lock);
	if (d == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_open)(dev, flag, devtype, l);
	DEV_UNLOCK(d);

	return rv;
}

int
bdev_close(dev_t dev, int flag, int devtype, lwp_t *l)
{
	const struct bdevsw *d;
	int rv, mpflag;

	if ((d = bdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_close)(dev, flag, devtype, l);
	DEV_UNLOCK(d);

	return rv;
}

SDT_PROVIDER_DECLARE(io);
SDT_PROBE_DEFINE1(io, kernel, , start, "struct buf *"/*bp*/);

void
bdev_strategy(struct buf *bp)
{
	const struct bdevsw *d;
	int mpflag;

	SDT_PROBE1(io, kernel, , start, bp);

	if ((d = bdevsw_lookup(bp->b_dev)) == NULL) {
		bp->b_error = ENXIO;
		bp->b_resid = bp->b_bcount;
		biodone_vfs(bp); /* biodone() iff vfs present */
		return;
	}

	DEV_LOCK(d);
	(*d->d_strategy)(bp);
	DEV_UNLOCK(d);
}

int
bdev_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	const struct bdevsw *d;
	int rv, mpflag;

	if ((d = bdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_ioctl)(dev, cmd, data, flag, l);
	DEV_UNLOCK(d);

	return rv;
}

int
bdev_dump(dev_t dev, daddr_t addr, void *data, size_t sz)
{
	const struct bdevsw *d;
	int rv;

	/*
	 * Dump can be called without the device open.  Since it can
	 * currently only be called with the system paused (and in a
	 * potentially unstable state), we don't perform any locking.
	 */
	if ((d = bdevsw_lookup(dev)) == NULL)
		return ENXIO;

	/* DEV_LOCK(d); */
	rv = (*d->d_dump)(dev, addr, data, sz);
	/* DEV_UNLOCK(d); */

	return rv;
}

int
bdev_type(dev_t dev)
{
	const struct bdevsw *d;

	if ((d = bdevsw_lookup(dev)) == NULL)
		return D_OTHER;
	return d->d_flag & D_TYPEMASK;
}

int
bdev_size(dev_t dev)
{
	const struct bdevsw *d;
	int rv, mpflag = 0;

	if ((d = bdevsw_lookup(dev)) == NULL ||
	    d->d_psize == NULL)
		return -1;

	/*
	 * Don't to try lock the device if we're dumping.
	 * XXX: is there a better way to test this?
	 */
	if ((boothowto & RB_DUMP) == 0)
		DEV_LOCK(d);
	rv = (*d->d_psize)(dev);
	if ((boothowto & RB_DUMP) == 0)
		DEV_UNLOCK(d);

	return rv;
}

int
bdev_discard(dev_t dev, off_t pos, off_t len)
{
	const struct bdevsw *d;
	int rv, mpflag;

	if ((d = bdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_discard)(dev, pos, len);
	DEV_UNLOCK(d);

	return rv;
}

int
cdev_open(dev_t dev, int flag, int devtype, lwp_t *l)
{
	const struct cdevsw *d;
	int rv, mpflag;

	/*
	 * For open we need to lock, in order to synchronize
	 * with attach/detach.
	 */
	mutex_enter(&device_lock);
	d = cdevsw_lookup(dev);
	mutex_exit(&device_lock);
	if (d == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_open)(dev, flag, devtype, l);
	DEV_UNLOCK(d);

	return rv;
}

int
cdev_close(dev_t dev, int flag, int devtype, lwp_t *l)
{
	const struct cdevsw *d;
	int rv, mpflag;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_close)(dev, flag, devtype, l);
	DEV_UNLOCK(d);

	return rv;
}

int
cdev_read(dev_t dev, struct uio *uio, int flag)
{
	const struct cdevsw *d;
	int rv, mpflag;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_read)(dev, uio, flag);
	DEV_UNLOCK(d);

	return rv;
}

int
cdev_write(dev_t dev, struct uio *uio, int flag)
{
	const struct cdevsw *d;
	int rv, mpflag;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_write)(dev, uio, flag);
	DEV_UNLOCK(d);

	return rv;
}

int
cdev_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	const struct cdevsw *d;
	int rv, mpflag;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_ioctl)(dev, cmd, data, flag, l);
	DEV_UNLOCK(d);

	return rv;
}

void
cdev_stop(struct tty *tp, int flag)
{
	const struct cdevsw *d;
	int mpflag;

	if ((d = cdevsw_lookup(tp->t_dev)) == NULL)
		return;

	DEV_LOCK(d);
	(*d->d_stop)(tp, flag);
	DEV_UNLOCK(d);
}

struct tty *
cdev_tty(dev_t dev)
{
	const struct cdevsw *d;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return NULL;

	/* XXX Check if necessary. */
	if (d->d_tty == NULL)
		return NULL;

	return (*d->d_tty)(dev);
}

int
cdev_poll(dev_t dev, int flag, lwp_t *l)
{
	const struct cdevsw *d;
	int rv, mpflag;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return POLLERR;

	DEV_LOCK(d);
	rv = (*d->d_poll)(dev, flag, l);
	DEV_UNLOCK(d);

	return rv;
}

paddr_t
cdev_mmap(dev_t dev, off_t off, int flag)
{
	const struct cdevsw *d;
	paddr_t rv;
	int mpflag;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return (paddr_t)-1LL;

	DEV_LOCK(d);
	rv = (*d->d_mmap)(dev, off, flag);
	DEV_UNLOCK(d);

	return rv;
}

int
cdev_kqfilter(dev_t dev, struct knote *kn)
{
	const struct cdevsw *d;
	int rv, mpflag;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_kqfilter)(dev, kn);
	DEV_UNLOCK(d);

	return rv;
}

int
cdev_discard(dev_t dev, off_t pos, off_t len)
{
	const struct cdevsw *d;
	int rv, mpflag;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_discard)(dev, pos, len);
	DEV_UNLOCK(d);

	return rv;
}

int
cdev_type(dev_t dev)
{
	const struct cdevsw *d;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return D_OTHER;
	return d->d_flag & D_TYPEMASK;
}
