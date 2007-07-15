/*	$NetBSD: subr_devsw.c,v 1.10.8.3 2007/07/15 15:52:55 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2007 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: subr_devsw.c,v 1.10.8.3 2007/07/15 15:52:55 ad Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/poll.h>
#include <sys/tty.h>
#include <sys/buf.h>

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

static int bdevsw_attach(const char *, const struct bdevsw *, int *);
static int cdevsw_attach(const char *, const struct cdevsw *, int *);
static void devsw_detach_locked(const struct bdevsw *, const struct cdevsw *);

static kmutex_t devsw_lock;

void
devsw_init(void)
{

	KASSERT(sys_bdevsws < MAXDEVSW - 1);
	KASSERT(sys_cdevsws < MAXDEVSW - 1);

	mutex_init(&devsw_lock, MUTEX_DEFAULT, IPL_NONE);
}

int
devsw_attach(const char *devname, const struct bdevsw *bdev, int *bmajor,
	     const struct cdevsw *cdev, int *cmajor)
{
	struct devsw_conv *conv;
	char *name;
	int error, i;

	if (devname == NULL || cdev == NULL)
		return (EINVAL);

	mutex_enter(&devsw_lock);

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

		if (bdev != NULL)
			bdevsw[*bmajor] = bdev;
		cdevsw[*cmajor] = cdev;

		mutex_exit(&devsw_lock);
		return (0);
	}

	error = bdevsw_attach(devname, bdev, bmajor);
	if (error != 0) 
		goto fail;
	error = cdevsw_attach(devname, cdev, cmajor);
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
		int old, new;

		old = max_devsw_convs;
		new = old + 1;

		newptr = kmem_zalloc(new * DEVSWCONV_SIZE, KM_NOSLEEP);
		if (newptr == NULL) {
			devsw_detach_locked(bdev, cdev);
			error = ENOMEM;
			goto fail;
		}
		newptr[old].d_name = NULL;
		newptr[old].d_bmajor = -1;
		newptr[old].d_cmajor = -1;
		memcpy(newptr, devsw_conv, old * DEVSWCONV_SIZE);
		if (devsw_conv != devsw_conv0)
			kmem_free(devsw_conv, old * DEVSWCONV_SIZE);
		devsw_conv = newptr;
		max_devsw_convs = new;
	}

	i = strlen(devname) + 1;
	name = kmem_alloc(i, KM_NOSLEEP);
	if (name == NULL) {
		devsw_detach_locked(bdev, cdev);
		goto fail;
	}
	strlcpy(name, devname, i);

	devsw_conv[i].d_name = name;
	devsw_conv[i].d_bmajor = *bmajor;
	devsw_conv[i].d_cmajor = *cmajor;

	mutex_exit(&devsw_lock);
	return (0);
 fail:
	mutex_exit(&devsw_lock);
	return (error);
}

static int
bdevsw_attach(const char *devname, const struct bdevsw *devsw, int *devmajor)
{
	const struct bdevsw **newptr;
	int bmajor, i;

	KASSERT(mutex_owned(&devsw_lock));

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
		printf("bdevsw_attach: block majors exhausted");
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

	bdevsw[*devmajor] = devsw;

	return (0);
}

static int
cdevsw_attach(const char *devname, const struct cdevsw *devsw, int *devmajor)
{
	const struct cdevsw **newptr;
	int cmajor, i;

	KASSERT(mutex_owned(&devsw_lock));

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
		printf("cdevsw_attach: character majors exhausted");
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

	cdevsw[*devmajor] = devsw;

	return (0);
}

static void
devsw_detach_locked(const struct bdevsw *bdev, const struct cdevsw *cdev)
{
	int i;

	KASSERT(mutex_owned(&devsw_lock));

	if (bdev != NULL) {
		for (i = 0 ; i < max_bdevsws ; i++) {
			if (bdevsw[i] != bdev)
				continue;
			bdevsw[i] = NULL;
			break;
		}
	}
	if (cdev != NULL) {
		for (i = 0 ; i < max_cdevsws ; i++) {
			if (cdevsw[i] != cdev)
				continue;
			cdevsw[i] = NULL;
			break;
		}
	}
}

void
devsw_detach(const struct bdevsw *bdev, const struct cdevsw *cdev)
{

	mutex_enter(&devsw_lock);
	devsw_detach_locked(bdev, cdev);
	mutex_exit(&devsw_lock);
}

/*
 * Look up a block device by number.
 *
 * => Caller must ensure that the device is attached.
 */
const struct bdevsw *
bdevsw_lookup(dev_t dev)
{
	int bmajor;

	if (dev == NODEV)
		return (NULL);
	bmajor = major(dev);
	if (bmajor < 0 || bmajor >= max_bdevsws)
		return (NULL);

	return (bdevsw[bmajor]);
}

/*
 * Look up a character device by number.
 *
 * => Caller must ensure that the device is attached.
 */
const struct cdevsw *
cdevsw_lookup(dev_t dev)
{
	int cmajor;

	if (dev == NODEV)
		return (NULL);
	cmajor = major(dev);
	if (cmajor < 0 || cmajor >= max_cdevsws)
		return (NULL);

	return (cdevsw[cmajor]);
}

/*
 * Look up a block device by reference to its operations set.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the returned major is still valid when dereferenced.
 */
int
bdevsw_lookup_major(const struct bdevsw *bdev)
{
	int bmajor;

	for (bmajor = 0 ; bmajor < max_bdevsws ; bmajor++) {
		if (bdevsw[bmajor] == bdev)
			return (bmajor);
	}

	return (-1);
}

/*
 * Look up a character device by reference to its operations set.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the returned major is still valid when dereferenced.
 */
int
cdevsw_lookup_major(const struct cdevsw *cdev)
{
	int cmajor;

	for (cmajor = 0 ; cmajor < max_cdevsws ; cmajor++) {
		if (cdevsw[cmajor] == cdev)
			return (cmajor);
	}

	return (-1);
}

/*
 * Convert from block major number to name.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the name pointer is still valid when dereferenced.
 */
const char *
devsw_blk2name(int bmajor)
{
	const char *name;
	int cmajor, i;

	name = NULL;
	cmajor = -1;

	mutex_enter(&devsw_lock);
	if (bmajor < 0 || bmajor >= max_bdevsws || bdevsw[bmajor] == NULL) {
		mutex_exit(&devsw_lock);
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
	mutex_exit(&devsw_lock);

	return (name);
}

/*
 * Convert from device name to block major number.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the major number is still valid when dereferenced.
 */
int
devsw_name2blk(const char *name, char *devname, size_t devnamelen)
{
	struct devsw_conv *conv;
	int bmajor, i;

	if (name == NULL)
		return (-1);

	mutex_enter(&devsw_lock);
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
		mutex_exit(&devsw_lock);
		return (bmajor);
	}

	mutex_exit(&devsw_lock);
	return (-1);
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
	int bmajor, cmajor, i;
	dev_t rv;

	cmajor = major(cdev);
	bmajor = -1;
	rv = NODEV;

	mutex_enter(&devsw_lock);
	if (cmajor < 0 || cmajor >= max_cdevsws || cdevsw[cmajor] == NULL) {
		mutex_exit(&devsw_lock);
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
	mutex_exit(&devsw_lock);

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
	int bmajor, cmajor, i;
	dev_t rv;

	bmajor = major(bdev);
	cmajor = -1;
	rv = NODEV;

	mutex_enter(&devsw_lock);
	if (bmajor < 0 || bmajor >= max_bdevsws || bdevsw[bmajor] == NULL) {
		mutex_exit(&devsw_lock);
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
	mutex_exit(&devsw_lock);

	return (rv);
}

/*
 * Device access methods.
 */

#define	DEV_LOCK(d)						\
	if ((d->d_flag & D_MPSAFE) == 0) {			\
		KERNEL_LOCK(1, curlwp);				\
	}

#define	DEV_UNLOCK(d)						\
	if ((d->d_flag & D_MPSAFE) == 0) {			\
		KERNEL_UNLOCK_ONE(curlwp);			\
	}

int
bdev_open(dev_t dev, int flag, int devtype, lwp_t *l)
{
	const struct bdevsw *d;
	int rv;

	/*
	 * For open we need to lock, in order to synchronize
	 * with attach/detach.
	 */
	mutex_enter(&devsw_lock);
	d = bdevsw_lookup(dev);
	mutex_exit(&devsw_lock);
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
	int rv;

	if ((d = bdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_close)(dev, flag, devtype, l);
	DEV_UNLOCK(d);

	return rv;
}

void
bdev_strategy(struct buf *bp)
{
	const struct bdevsw *d;

	if ((d = bdevsw_lookup(bp->b_dev)) == NULL)
		panic("bdev_strategy");

	DEV_LOCK(d);
	(*d->d_strategy)(bp);
	DEV_UNLOCK(d);
}

int
bdev_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	const struct bdevsw *d;
	int rv;

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
cdev_open(dev_t dev, int flag, int devtype, lwp_t *l)
{
	const struct cdevsw *d;
	int rv;

	/*
	 * For open we need to lock, in order to synchronize
	 * with attach/detach.
	 */
	mutex_enter(&devsw_lock);
	d = cdevsw_lookup(dev);
	mutex_exit(&devsw_lock);
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
	int rv;

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
	int rv;

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
	int rv;

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
	int rv;

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
	struct tty * rv;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return NULL;

	DEV_LOCK(d);
	rv = (*d->d_tty)(dev);
	DEV_UNLOCK(d);

	return rv;
}

int
cdev_poll(dev_t dev, int flag, lwp_t *l)
{
	const struct cdevsw *d;
	int rv;

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
	int rv;

	if ((d = cdevsw_lookup(dev)) == NULL)
		return ENXIO;

	DEV_LOCK(d);
	rv = (*d->d_kqfilter)(dev, kn);
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
