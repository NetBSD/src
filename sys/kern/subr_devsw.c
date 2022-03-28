/*	$NetBSD: subr_devsw.c,v 1.44 2022/03/28 12:39:10 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: subr_devsw.c,v 1.44 2022/03/28 12:39:10 riastradh Exp $");

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
#include <sys/localcount.h>
#include <sys/pserialize.h>
#include <sys/xcall.h>
#include <sys/device.h>

#ifdef DEVSW_DEBUG
#define	DPRINTF(x)	printf x
#else /* DEVSW_DEBUG */
#define	DPRINTF(x)
#endif /* DEVSW_DEBUG */

#define	MAXDEVSW	512	/* the maximum of major device number */
#define	BDEVSW_SIZE	(sizeof(struct bdevsw *))
#define	CDEVSW_SIZE	(sizeof(struct cdevsw *))
#define	DEVSWCONV_SIZE	(sizeof(struct devsw_conv))

struct devswref {
	struct localcount	*dr_lc;
};

/* XXX bdevsw, cdevsw, max_bdevsws, and max_cdevsws should be volatile */
extern const struct bdevsw **bdevsw, *bdevsw0[];
extern const struct cdevsw **cdevsw, *cdevsw0[];
extern struct devsw_conv *devsw_conv, devsw_conv0[];
extern const int sys_bdevsws, sys_cdevsws;
extern int max_bdevsws, max_cdevsws, max_devsw_convs;

static struct devswref *cdevswref;
static struct devswref *bdevswref;
static kcondvar_t devsw_cv;

static int bdevsw_attach(const struct bdevsw *, devmajor_t *);
static int cdevsw_attach(const struct cdevsw *, devmajor_t *);
static void devsw_detach_locked(const struct bdevsw *, const struct cdevsw *);

kmutex_t device_lock;

void (*biodone_vfs)(buf_t *) = (void *)nullop;

void
devsw_init(void)
{

	KASSERT(sys_bdevsws < MAXDEVSW - 1);
	KASSERT(sys_cdevsws < MAXDEVSW - 1);
	mutex_init(&device_lock, MUTEX_DEFAULT, IPL_NONE);

	cv_init(&devsw_cv, "devsw");
}

int
devsw_attach(const char *devname,
	     const struct bdevsw *bdev, devmajor_t *bmajor,
	     const struct cdevsw *cdev, devmajor_t *cmajor)
{
	struct devsw_conv *conv;
	char *name;
	int error, i;

	if (devname == NULL || cdev == NULL)
		return (EINVAL);

	mutex_enter(&device_lock);

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
		break;
	}

	/*
	 * XXX This should allocate what it needs up front so we never
	 * need to flail around trying to unwind.
	 */
	error = bdevsw_attach(bdev, bmajor);
	if (error != 0) 
		goto fail;
	error = cdevsw_attach(cdev, cmajor);
	if (error != 0) {
		devsw_detach_locked(bdev, NULL);
		goto fail;
	}

	/*
	 * If we already found a conv, we're done.  Otherwise, find an
	 * empty slot or extend the table.
	 */
	if (i == max_devsw_convs)
		goto fail;

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

	name = kmem_strdupsize(devname, NULL, KM_NOSLEEP);
	if (name == NULL) {
		devsw_detach_locked(bdev, cdev);
		error = ENOMEM;
		goto fail;
	}

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
	const struct bdevsw **newbdevsw = NULL;
	struct devswref *newbdevswref = NULL;
	struct localcount *lc;
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

	if (bdevswref == NULL) {
		newbdevswref = kmem_zalloc(MAXDEVSW * sizeof(newbdevswref[0]),
		    KM_NOSLEEP);
		if (newbdevswref == NULL)
			return ENOMEM;
		atomic_store_release(&bdevswref, newbdevswref);
	}

	if (*devmajor >= max_bdevsws) {
		KASSERT(bdevsw == bdevsw0);
		newbdevsw = kmem_zalloc(MAXDEVSW * sizeof(newbdevsw[0]),
		    KM_NOSLEEP);
		if (newbdevsw == NULL)
			return ENOMEM;
		memcpy(newbdevsw, bdevsw, max_bdevsws * sizeof(bdevsw[0]));
		atomic_store_release(&bdevsw, newbdevsw);
		atomic_store_release(&max_bdevsws, MAXDEVSW);
	}

	if (bdevsw[*devmajor] != NULL)
		return (EEXIST);

	KASSERT(bdevswref[*devmajor].dr_lc == NULL);
	lc = kmem_zalloc(sizeof(*lc), KM_SLEEP);
	localcount_init(lc);
	bdevswref[*devmajor].dr_lc = lc;

	atomic_store_release(&bdevsw[*devmajor], devsw);

	return (0);
}

static int
cdevsw_attach(const struct cdevsw *devsw, devmajor_t *devmajor)
{
	const struct cdevsw **newcdevsw = NULL;
	struct devswref *newcdevswref = NULL;
	struct localcount *lc;
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

	if (cdevswref == NULL) {
		newcdevswref = kmem_zalloc(MAXDEVSW * sizeof(newcdevswref[0]),
		    KM_NOSLEEP);
		if (newcdevswref == NULL)
			return ENOMEM;
		atomic_store_release(&cdevswref, newcdevswref);
	}

	if (*devmajor >= max_cdevsws) {
		KASSERT(cdevsw == cdevsw0);
		newcdevsw = kmem_zalloc(MAXDEVSW * sizeof(newcdevsw[0]),
		    KM_NOSLEEP);
		if (newcdevsw == NULL)
			return ENOMEM;
		memcpy(newcdevsw, cdevsw, max_cdevsws * sizeof(cdevsw[0]));
		atomic_store_release(&cdevsw, newcdevsw);
		atomic_store_release(&max_cdevsws, MAXDEVSW);
	}

	if (cdevsw[*devmajor] != NULL)
		return (EEXIST);

	KASSERT(cdevswref[*devmajor].dr_lc == NULL);
	lc = kmem_zalloc(sizeof(*lc), KM_SLEEP);
	localcount_init(lc);
	cdevswref[*devmajor].dr_lc = lc;

	atomic_store_release(&cdevsw[*devmajor], devsw);

	return (0);
}

static void
devsw_detach_locked(const struct bdevsw *bdev, const struct cdevsw *cdev)
{
	int bi, ci = -1/*XXXGCC*/;

	KASSERT(mutex_owned(&device_lock));

	/* Prevent new references.  */
	if (bdev != NULL) {
		for (bi = 0; bi < max_bdevsws; bi++) {
			if (bdevsw[bi] != bdev)
				continue;
			atomic_store_relaxed(&bdevsw[bi], NULL);
			break;
		}
		KASSERT(bi < max_bdevsws);
	}
	if (cdev != NULL) {
		for (ci = 0; ci < max_cdevsws; ci++) {
			if (cdevsw[ci] != cdev)
				continue;
			atomic_store_relaxed(&cdevsw[ci], NULL);
			break;
		}
		KASSERT(ci < max_cdevsws);
	}

	if (bdev == NULL && cdev == NULL) /* XXX possible? */
		return;

	/*
	 * Wait for all bdevsw_lookup_acquire, cdevsw_lookup_acquire
	 * calls to notice that the devsw is gone.
	 *
	 * XXX Despite the use of the pserialize_read_enter/exit API
	 * elsewhere in this file, we use xc_barrier here instead of
	 * pserialize_perform -- because devsw_init is too early for
	 * pserialize_create.  Either pserialize_create should be made
	 * to work earlier, or it should be nixed altogether.  Until
	 * that is fixed, xc_barrier will serve the same purpose.
	 */
	xc_barrier(0);

	/*
	 * Wait for all references to drain.  It is the caller's
	 * responsibility to ensure that at this point, there are no
	 * extant open instances and all new d_open calls will fail.
	 *
	 * Note that localcount_drain may release and reacquire
	 * device_lock.
	 */
	if (bdev != NULL) {
		localcount_drain(bdevswref[bi].dr_lc,
		    &devsw_cv, &device_lock);
		localcount_fini(bdevswref[bi].dr_lc);
		kmem_free(bdevswref[bi].dr_lc, sizeof(*bdevswref[bi].dr_lc));
		bdevswref[bi].dr_lc = NULL;
	}
	if (cdev != NULL) {
		localcount_drain(cdevswref[ci].dr_lc,
		    &devsw_cv, &device_lock);
		localcount_fini(cdevswref[ci].dr_lc);
		kmem_free(cdevswref[ci].dr_lc, sizeof(*cdevswref[ci].dr_lc));
		cdevswref[ci].dr_lc = NULL;
	}
}

void
devsw_detach(const struct bdevsw *bdev, const struct cdevsw *cdev)
{

	mutex_enter(&device_lock);
	devsw_detach_locked(bdev, cdev);
	mutex_exit(&device_lock);
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
	if (bmajor < 0 || bmajor >= atomic_load_relaxed(&max_bdevsws))
		return (NULL);

	return atomic_load_consume(&bdevsw)[bmajor];
}

static const struct bdevsw *
bdevsw_lookup_acquire(dev_t dev, struct localcount **lcp)
{
	devmajor_t bmajor;
	const struct bdevsw *bdev = NULL, *const *curbdevsw;
	struct devswref *curbdevswref;
	int s;

	if (dev == NODEV)
		return NULL;
	bmajor = major(dev);
	if (bmajor < 0)
		return NULL;

	s = pserialize_read_enter();

	/*
	 * max_bdevsws never goes down, so it is safe to rely on this
	 * condition without any locking for the array access below.
	 * Test sys_bdevsws first so we can avoid the memory barrier in
	 * that case.
	 */
	if (bmajor >= sys_bdevsws &&
	    bmajor >= atomic_load_acquire(&max_bdevsws))
		goto out;
	curbdevsw = atomic_load_consume(&bdevsw);
	if ((bdev = atomic_load_consume(&curbdevsw[bmajor])) == NULL)
		goto out;

	curbdevswref = atomic_load_consume(&bdevswref);
	if (curbdevswref == NULL) {
		*lcp = NULL;
	} else if ((*lcp = curbdevswref[bmajor].dr_lc) != NULL) {
		localcount_acquire(*lcp);
	}
out:
	pserialize_read_exit(s);
	return bdev;
}

static void
bdevsw_release(const struct bdevsw *bdev, struct localcount *lc)
{

	if (lc == NULL)
		return;
	localcount_release(lc, &devsw_cv, &device_lock);
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
	if (cmajor < 0 || cmajor >= atomic_load_relaxed(&max_cdevsws))
		return (NULL);

	return atomic_load_consume(&cdevsw)[cmajor];
}

static const struct cdevsw *
cdevsw_lookup_acquire(dev_t dev, struct localcount **lcp)
{
	devmajor_t cmajor;
	const struct cdevsw *cdev = NULL, *const *curcdevsw;
	struct devswref *curcdevswref;
	int s;

	if (dev == NODEV)
		return NULL;
	cmajor = major(dev);
	if (cmajor < 0)
		return NULL;

	s = pserialize_read_enter();

	/*
	 * max_cdevsws never goes down, so it is safe to rely on this
	 * condition without any locking for the array access below.
	 * Test sys_cdevsws first so we can avoid the memory barrier in
	 * that case.
	 */
	if (cmajor >= sys_cdevsws &&
	    cmajor >= atomic_load_acquire(&max_cdevsws))
		goto out;
	curcdevsw = atomic_load_consume(&cdevsw);
	if ((cdev = atomic_load_consume(&curcdevsw[cmajor])) == NULL)
		goto out;

	curcdevswref = atomic_load_consume(&cdevswref);
	if (curcdevswref == NULL) {
		*lcp = NULL;
	} else if ((*lcp = curcdevswref[cmajor].dr_lc) != NULL) {
		localcount_acquire(*lcp);
	}
out:
	pserialize_read_exit(s);
	return cdev;
}

static void
cdevsw_release(const struct cdevsw *cdev, struct localcount *lc)
{

	if (lc == NULL)
		return;
	localcount_release(lc, &devsw_cv, &device_lock);
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
	const struct bdevsw *const *curbdevsw;
	devmajor_t bmajor, bmax;

	bmax = atomic_load_acquire(&max_bdevsws);
	curbdevsw = atomic_load_consume(&bdevsw);
	for (bmajor = 0; bmajor < bmax; bmajor++) {
		if (atomic_load_relaxed(&curbdevsw[bmajor]) == bdev)
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
	const struct cdevsw *const *curcdevsw;
	devmajor_t cmajor, cmax;

	cmax = atomic_load_acquire(&max_cdevsws);
	curcdevsw = atomic_load_consume(&cdevsw);
	for (cmajor = 0; cmajor < cmax; cmajor++) {
		if (atomic_load_relaxed(&curcdevsw[cmajor]) == cdev)
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
				printf("%s: too short buffer", __func__);
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
				printf("%s: too short buffer", __func__);
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
	struct localcount *lc;
	device_t dv = NULL/*XXXGCC*/;
	int unit, rv, mpflag;

	d = bdevsw_lookup_acquire(dev, &lc);
	if (d == NULL)
		return ENXIO;

	if (d->d_devtounit) {
		/*
		 * If the device node corresponds to an autoconf device
		 * instance, acquire a reference to it so that during
		 * d_open, device_lookup is stable.
		 *
		 * XXX This should also arrange to instantiate cloning
		 * pseudo-devices if appropriate, but that requires
		 * reviewing them all to find and verify a common
		 * pattern.
		 */
		if ((unit = (*d->d_devtounit)(dev)) == -1)
			return ENXIO;
		if ((dv = device_lookup_acquire(d->d_cfdriver, unit)) == NULL)
			return ENXIO;
	}

	DEV_LOCK(d);
	rv = (*d->d_open)(dev, flag, devtype, l);
	DEV_UNLOCK(d);

	if (d->d_devtounit) {
		device_release(dv);
	}

	bdevsw_release(d, lc);

	return rv;
}

int
bdev_cancel(dev_t dev, int flag, int devtype, struct lwp *l)
{
	const struct bdevsw *d;
	int rv, mpflag;

	if ((d = bdevsw_lookup(dev)) == NULL)
		return ENXIO;
	if (d->d_cancel == NULL)
		return ENODEV;

	DEV_LOCK(d);
	rv = (*d->d_cancel)(dev, flag, devtype, l);
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
bdev_flags(dev_t dev)
{
	const struct bdevsw *d;

	if ((d = bdevsw_lookup(dev)) == NULL)
		return 0;
	return d->d_flag & ~D_TYPEMASK;
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

void
bdev_detached(dev_t dev)
{
	const struct bdevsw *d;
	device_t dv;
	int unit;

	if ((d = bdevsw_lookup(dev)) == NULL)
		return;
	if (d->d_devtounit == NULL)
		return;
	if ((unit = (*d->d_devtounit)(dev)) == -1)
		return;
	if ((dv = device_lookup(d->d_cfdriver, unit)) == NULL)
		return;
	config_detach_commit(dv);
}

int
cdev_open(dev_t dev, int flag, int devtype, lwp_t *l)
{
	const struct cdevsw *d;
	struct localcount *lc;
	device_t dv = NULL/*XXXGCC*/;
	int unit, rv, mpflag;

	d = cdevsw_lookup_acquire(dev, &lc);
	if (d == NULL)
		return ENXIO;

	if (d->d_devtounit) {
		/*
		 * If the device node corresponds to an autoconf device
		 * instance, acquire a reference to it so that during
		 * d_open, device_lookup is stable.
		 *
		 * XXX This should also arrange to instantiate cloning
		 * pseudo-devices if appropriate, but that requires
		 * reviewing them all to find and verify a common
		 * pattern.
		 */
		if ((unit = (*d->d_devtounit)(dev)) == -1)
			return ENXIO;
		if ((dv = device_lookup_acquire(d->d_cfdriver, unit)) == NULL)
			return ENXIO;
	}

	DEV_LOCK(d);
	rv = (*d->d_open)(dev, flag, devtype, l);
	DEV_UNLOCK(d);

	if (d->d_devtounit) {
		device_release(dv);
	}

	cdevsw_release(d, lc);

	return rv;
}

int
cdev_cancel(dev_t dev, int flag, int devtype, struct lwp *l)
{
	const struct cdevsw *d;
	int rv, mpflag;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return ENXIO;
	if (d->d_cancel == NULL)
		return ENODEV;

	DEV_LOCK(d);
	rv = (*d->d_cancel)(dev, flag, devtype, l);
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
cdev_flags(dev_t dev)
{
	const struct cdevsw *d;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return 0;
	return d->d_flag & ~D_TYPEMASK;
}

int
cdev_type(dev_t dev)
{
	const struct cdevsw *d;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return D_OTHER;
	return d->d_flag & D_TYPEMASK;
}

void
cdev_detached(dev_t dev)
{
	const struct cdevsw *d;
	device_t dv;
	int unit;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return;
	if (d->d_devtounit == NULL)
		return;
	if ((unit = (*d->d_devtounit)(dev)) == -1)
		return;
	if ((dv = device_lookup(d->d_cfdriver, unit)) == NULL)
		return;
	config_detach_commit(dv);
}

/*
 * nommap(dev, off, prot)
 *
 *	mmap routine that always fails, for non-mmappable devices.
 */
paddr_t
nommap(dev_t dev, off_t off, int prot)
{

	return (paddr_t)-1;
}

/*
 * dev_minor_unit(dev)
 *
 *	Returns minor(dev) as an int.  Intended for use with struct
 *	bdevsw, cdevsw::d_devtounit for drivers whose /dev nodes are
 *	implemented by reference to an autoconf instance with the minor
 *	number.
 */
int
dev_minor_unit(dev_t dev)
{

	return minor(dev);
}
