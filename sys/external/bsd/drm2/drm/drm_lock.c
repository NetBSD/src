/*	$NetBSD: drm_lock.c,v 1.1.2.1 2013/07/24 02:33:17 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * DRM lock.  Each drm master has a lock to provide mutual exclusion
 * for access to something about the hardware (XXX what?).  The lock
 * can be held by the kernel or by a drm file (XXX not by a process!
 * and multiple processes may share a common drm file).  (XXX What
 * excludes different kthreads?)  The DRM_LOCK ioctl grants the lock to
 * the userland.  The kernel may need to steal this lock in order to
 * close a drm file; I believe drm_global_mutex excludes different
 * kernel threads from doing this simultaneously.
 *
 * XXX This description is incoherent and incomplete.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_lock.c,v 1.1.2.1 2013/07/24 02:33:17 riastradh Exp $");

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <drm/drmP.h>

static bool	drm_lock_acquire(struct drm_lock_data *, int);
static void	drm_lock_release(struct drm_lock_data *, int);
static int	drm_lock_block_signals(struct drm_device *, struct drm_lock *,
		    struct drm_file *);
static void	drm_lock_unblock_signals(struct drm_device *,
		    struct drm_lock *, struct drm_file *);

/*
 * Take the lock on behalf of userland.
 */
int
drm_lock(struct drm_device *dev, void *data, struct drm_file *file)
{
	struct drm_lock *lock_request = data;
	struct drm_master *master = file->master;
	bool acquired;
	int error;

	KASSERT(mutex_is_locked(&drm_global_mutex));

	/* Refuse to lock on behalf of the kernel.  */
	if (lock_request->context == DRM_KERNEL_CONTEXT)
		return -EINVAL;

	/* Count it in the file and device statistics (XXX why here?).  */
	file->lock_count++;
	atomic_inc(&dev->counts[_DRM_STAT_LOCKS]);

	/* Wait until the hardware lock is gone or we can acquire it.   */
	spin_lock(&master->lock.spinlock);
	DRM_SPIN_WAIT_UNTIL(error, &master->lock.lock_queue,
	    &master->lock.spinlock,
	    ((master->lock.hw_lock == NULL) ||
		(acquired = drm_lock_acquire(&master->lock,
		    lock_request->context))));
	if (error)
		goto out;

	/* If the lock is gone, give up.  */
	if (master->lock.hw_lock == NULL) {
#if 0				/* XXX Linux sends SIGTERM, but why?  */
		mutex_enter(proc_lock);
		psignal(curproc, SIGTERM);
		mutex_exit(proc_lock);
		error = -EINTR;
#else
		error = -ENXIO;
#endif
		goto out;
	}

	/* Mark the lock as owned by file.  */
	master->lock.file_priv = file;
#ifndef __NetBSD__
	master->lock.lock_time = jiffies; /* XXX Unused?  */
#endif

	/* Block signals while the lock is held.  */
	error = drm_lock_block_signals(dev, lock_request, file);
	if (error)
		goto fail0;

	/* Enter the DMA quiescent state if requested and available.  */
	if (ISSET(lock_request->flags, _DRM_LOCK_QUIESCENT) &&
	    (dev->driver->dma_quiescent != NULL)) {
		error = (*dev->driver->dma_quiescent)(dev);
		if (error)
			goto fail1;
	}

	/* Success!  */
	error = 0;
	goto out;

fail1:	drm_lock_unblock_signals(dev, lock_request, file);
fail0:	drm_lock_release(&master->lock, lock_request->context);
	master->lock.file_priv = NULL;
out:	spin_unlock(&master->lock.spinlock);
	return error;
}

/*
 * Try to relinquish a lock that userland thinks it holds, per
 * userland's request.  Fail if it doesn't actually hold the lock.
 */
int
drm_unlock(struct drm_device *dev, void *data, struct drm_file *file)
{
	struct drm_lock *lock_request = data;
	struct drm_master *master = file->master;
	int error;

	KASSERT(mutex_is_locked(&drm_global_mutex));

	/* Refuse to unlock on behalf of the kernel.  */
	if (lock_request->context == DRM_KERNEL_CONTEXT)
		return -EINVAL;

	/* Count it in the device statistics.  */
	atomic_inc(&dev->counts[_DRM_STAT_UNLOCKS]);

	/* Lock the internal spin lock to make changes.  */
	spin_lock(&master->lock.spinlock);

	/* Make sure it's actually locked.  */
	if (!_DRM_LOCK_IS_HELD(master->lock.hw_lock->lock)) {
		error = -EINVAL;	/* XXX Right error?  */
		goto out;
	}

	/* Make sure it's locked in the right context.  */
	if (_DRM_LOCKING_CONTEXT(master->lock.hw_lock->lock) !=
	    lock_request->context) {
		error = -EACCES;	/* XXX Right error?  */
		goto out;
	}

	/* Make sure it's locked by us.  */
	if (master->lock.file_priv != file) {
		error = -EACCES;	/* XXX Right error?  */
		goto out;
	}

	/* Actually release the lock.  */
	drm_lock_release(&master->lock, lock_request->context);

	/* Clear the lock's file pointer, just in case.  */
	master->lock.file_priv = NULL;

	/* Unblock the signals we blocked in drm_lock.  */
	drm_lock_unblock_signals(dev, lock_request, file);

	/* Success!  */
	error = 0;

out:	spin_unlock(&master->lock.spinlock);
	return error;
}

/*
 * Take the lock for the kernel's use.
 */
void
drm_idlelock_take(struct drm_lock_data *lock_data __unused)
{
	KASSERT(mutex_is_locked(&drm_global_mutex));
	panic("drm_idlelock_take is not yet implemented"); /* XXX */
}

/*
 * Release the lock from the kernel.
 */
void
drm_idlelock_release(struct drm_lock_data *lock_data __unused)
{
	KASSERT(mutex_is_locked(&drm_global_mutex));
	panic("drm_idlelock_release is not yet implemented"); /* XXX */
}

/*
 * Try to acquire the lock.  Return true if successful, false if not.
 *
 * Lock's spinlock must be held.
 */
static bool
drm_lock_acquire(struct drm_lock_data *lock_data, int context)
{

	KASSERT(spin_is_locked(&lock_data->spinlock));

	/* Test and set.  */
	if (_DRM_LOCK_IS_HELD(lock_data->hw_lock->lock)) {
		return false;
	} else {
		lock_data->hw_lock->lock = (context | _DRM_LOCK_HELD);
		return true;
	}
}

/*
 * Release the lock held in the given context.  Wake any waiters,
 * preferring kernel waiters over userland waiters.
 *
 * Lock's spinlock must be held and lock must be held in this context.
 */
static void
drm_lock_release(struct drm_lock_data *lock_data, int context)
{

	(void)context;		/* ignore */
	KASSERT(spin_is_locked(&lock_data->spinlock));
	KASSERT(_DRM_LOCK_IS_HELD(lock_data->hw_lock->lock));
	KASSERT(_DRM_LOCKING_CONTEXT(lock_data->hw_lock->lock) == context);

	/* Are there any kernel waiters?  */
	if (lock_data->n_kernel_waiters > 0) {
		/* If there's a kernel waiter, grant it ownership.  */
		lock_data->hw_lock->lock = (DRM_KERNEL_CONTEXT |
		    _DRM_LOCK_HELD);
		lock_data->n_kernel_waiters--;
		DRM_SPIN_WAKEUP_ONE(&lock_data->kernel_lock_queue,
		    &lock_data->spinlock);
	} else {
		/* Otherwise, free it up and wake someone else.  */
		lock_data->hw_lock->lock = 0;
		DRM_SPIN_WAKEUP_ONE(&lock_data->lock_queue,
		    &lock_data->spinlock);
	}
}

/*
 * Block signals for a process that holds a drm lock.
 *
 * XXX It's not processes but files that hold drm locks, so blocking
 * signals in a process seems wrong, and it's not clear that blocking
 * signals automatically is remotely sensible anyway.
 */
static int
drm_lock_block_signals(struct drm_device *dev __unused,
    struct drm_lock *lock_request __unused, struct drm_file *file __unused)
{
	return 0;
}

/*
 * Unblock the signals that drm_lock_block_signals blocked.
 */
static void
drm_lock_unblock_signals(struct drm_device *dev __unused,
    struct drm_lock *lock_request __unused, struct drm_file *file __unused)
{
}
