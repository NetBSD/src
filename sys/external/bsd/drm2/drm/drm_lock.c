/*	$NetBSD: drm_lock.c,v 1.3.4.3 2017/12/03 11:37:58 jdolecek Exp $	*/

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
 * DRM lock.  Each drm master has a heavy-weight lock to provide mutual
 * exclusion for access to the hardware.  The lock can be held by the
 * kernel or by a drm file; the kernel takes access only for unusual
 * purposes, with drm_idlelock_take, mainly for idling the GPU when
 * closing down.
 *
 * The physical memory storing the lock state is shared between
 * userland and kernel: the pointer at dev->master->lock->hw_lock is
 * mapped into both userland and kernel address spaces.  This way,
 * userland can try to take the hardware lock without a system call,
 * although if it fails then it will use the DRM_LOCK ioctl to block
 * atomically until the lock is available.  All this means that the
 * kernel must use atomic_ops to manage the lock state.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_lock.c,v 1.3.4.3 2017/12/03 11:37:58 jdolecek Exp $");

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
	int error;

	/* Sanitize the drm global mutex bollocks until we get rid of it.  */
	KASSERT(mutex_is_locked(&drm_global_mutex));
	mutex_unlock(&drm_global_mutex);

	/* Refuse to lock on behalf of the kernel.  */
	if (lock_request->context == DRM_KERNEL_CONTEXT) {
		error = -EINVAL;
		goto out0;
	}

	/* Refuse to set the magic bits.  */
	if (lock_request->context !=
	    _DRM_LOCKING_CONTEXT(lock_request->context)) {
		error = -EINVAL;
		goto out0;
	}

	/* Count it in the file and device statistics (XXX why here?).  */
	file->lock_count++;

	/* Wait until the hardware lock is gone or we can acquire it.   */
	spin_lock(&master->lock.spinlock);

	if (master->lock.user_waiters == UINT32_MAX) {
		error = -EBUSY;
		goto out1;
	}

	master->lock.user_waiters++;
	DRM_SPIN_WAIT_UNTIL(error, &master->lock.lock_queue,
	    &master->lock.spinlock,
	    ((master->lock.hw_lock == NULL) ||
		drm_lock_acquire(&master->lock, lock_request->context)));
	KASSERT(0 < master->lock.user_waiters);
	master->lock.user_waiters--;
	if (error)
		goto out1;

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
		goto out1;
	}

	/* Mark the lock as owned by file.  */
	master->lock.file_priv = file;
	master->lock.lock_time = jiffies; /* XXX Unused?  */

	/* Block signals while the lock is held.  */
	error = drm_lock_block_signals(dev, lock_request, file);
	if (error)
		goto fail2;

	/* Enter the DMA quiescent state if requested and available.  */
	/* XXX Drop the spin lock first...  */
	if (ISSET(lock_request->flags, _DRM_LOCK_QUIESCENT) &&
	    (dev->driver->dma_quiescent != NULL)) {
		error = (*dev->driver->dma_quiescent)(dev);
		if (error)
			goto fail3;
	}

	/* Success!  */
	error = 0;
	goto out1;

fail3:	drm_lock_unblock_signals(dev, lock_request, file);
fail2:	drm_lock_release(&master->lock, lock_request->context);
	master->lock.file_priv = NULL;
out1:	spin_unlock(&master->lock.spinlock);
out0:	mutex_lock(&drm_global_mutex);
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

	/* Sanitize the drm global mutex bollocks until we get rid of it.  */
	KASSERT(mutex_is_locked(&drm_global_mutex));
	mutex_unlock(&drm_global_mutex);

	/* Refuse to unlock on behalf of the kernel.  */
	if (lock_request->context == DRM_KERNEL_CONTEXT) {
		error = -EINVAL;
		goto out0;
	}

	/* Lock the internal spin lock to make changes.  */
	spin_lock(&master->lock.spinlock);

	/* Make sure it's actually locked.  */
	if (!_DRM_LOCK_IS_HELD(master->lock.hw_lock->lock)) {
		error = -EINVAL;	/* XXX Right error?  */
		goto out1;
	}

	/* Make sure it's locked in the right context.  */
	if (_DRM_LOCKING_CONTEXT(master->lock.hw_lock->lock) !=
	    lock_request->context) {
		error = -EACCES;	/* XXX Right error?  */
		goto out1;
	}

	/* Make sure it's locked by us.  */
	if (master->lock.file_priv != file) {
		error = -EACCES;	/* XXX Right error?  */
		goto out1;
	}

	/* Actually release the lock.  */
	drm_lock_release(&master->lock, lock_request->context);

	/* Clear the lock's file pointer, just in case.  */
	master->lock.file_priv = NULL;

	/* Unblock the signals we blocked in drm_lock.  */
	drm_lock_unblock_signals(dev, lock_request, file);

	/* Success!  */
	error = 0;

out1:	spin_unlock(&master->lock.spinlock);
out0:	mutex_lock(&drm_global_mutex);
	return error;
}

/*
 * Drop the lock.
 *
 * Return value is an artefact of Linux.  Caller must guarantee
 * preconditions; failure is fatal.
 *
 * XXX Should we also unblock signals like drm_unlock does?
 */
int
drm_lock_free(struct drm_lock_data *lock_data, unsigned int context)
{

	spin_lock(&lock_data->spinlock);
	drm_lock_release(lock_data, context);
	spin_unlock(&lock_data->spinlock);

	return 0;
}

/*
 * Try to acquire the lock.  Whether or not we acquire it, guarantee
 * that whoever next releases it relinquishes it to the kernel, not to
 * anyone else.
 */
void
drm_idlelock_take(struct drm_lock_data *lock_data)
{

	spin_lock(&lock_data->spinlock);
	KASSERT(!lock_data->idle_has_lock);
	KASSERT(lock_data->kernel_waiters < UINT32_MAX);
	lock_data->kernel_waiters++;
	/* Try to acquire the lock.  */
	if (drm_lock_acquire(lock_data, DRM_KERNEL_CONTEXT)) {
		lock_data->idle_has_lock = 1;
	} else {
		/*
		 * Recording that there are kernel waiters will prevent
		 * userland from acquiring the lock again when it is
		 * next released.
		 */
	}
	spin_unlock(&lock_data->spinlock);
}

/*
 * Release whatever drm_idlelock_take managed to acquire.
 */
void
drm_idlelock_release(struct drm_lock_data *lock_data)
{

	spin_lock(&lock_data->spinlock);
	KASSERT(0 < lock_data->kernel_waiters);
	if (--lock_data->kernel_waiters == 0) {
		if (lock_data->idle_has_lock) {
			/* We did acquire it.  Release it.  */
			drm_lock_release(lock_data, DRM_KERNEL_CONTEXT);
		}
	}
	spin_unlock(&lock_data->spinlock);
}

/*
 * Does this file hold this drm device's hardware lock?
 *
 * Used to decide whether to release the lock when the file is being
 * closed.
 *
 * XXX I don't think this answers correctly in the case that the
 * userland has taken the lock and it is uncontended.  But I don't
 * think we can know what the correct answer is in that case.
 */
int
drm_i_have_hw_lock(struct drm_device *dev, struct drm_file *file)
{
	struct drm_lock_data *const lock_data = &file->master->lock;
	int answer = 0;

	/* If this file has never locked anything, then no.  */
	if (file->lock_count == 0)
		return 0;

	spin_lock(&lock_data->spinlock);

	/* If there is no lock, then this file doesn't hold it.  */
	if (lock_data->hw_lock == NULL)
		goto out;

	/* If this lock is not held, then this file doesn't hold it.   */
	if (!_DRM_LOCK_IS_HELD(lock_data->hw_lock->lock))
		goto out;

	/*
	 * Otherwise, it boils down to whether this file is the owner
	 * or someone else.
	 *
	 * XXX This is not reliable!  Userland doesn't update this when
	 * it takes the lock...
	 */
	answer = (file == lock_data->file_priv);

out:	spin_unlock(&lock_data->spinlock);
	return answer;
}

/*
 * Try to acquire the lock.  Return true if successful, false if not.
 *
 * This is hairy because it races with userland, and if userland
 * already holds the lock, we must tell it, by marking it
 * _DRM_LOCK_CONT (contended), that it must call ioctl(DRM_UNLOCK) to
 * release the lock so that we can wake waiters.
 *
 * XXX What happens if the process is interrupted?
 */
static bool
drm_lock_acquire(struct drm_lock_data *lock_data, int context)
{
        volatile unsigned int *const lock = &lock_data->hw_lock->lock;
	unsigned int old, new;

	KASSERT(spin_is_locked(&lock_data->spinlock));

	do {
		old = *lock;
		if (!_DRM_LOCK_IS_HELD(old)) {
			new = (context | _DRM_LOCK_HELD);
			if ((0 < lock_data->user_waiters) ||
			    (0 < lock_data->kernel_waiters))
				new |= _DRM_LOCK_CONT;
		} else if (_DRM_LOCKING_CONTEXT(old) != context) {
			new = (old | _DRM_LOCK_CONT);
		} else {
			DRM_ERROR("%d already holds heavyweight lock\n",
			    context);
			return false;
		}
	} while (atomic_cas_uint(lock, old, new) != old);

	return !_DRM_LOCK_IS_HELD(old);
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

	lock_data->hw_lock->lock = 0;
	DRM_SPIN_WAKEUP_ONE(&lock_data->lock_queue, &lock_data->spinlock);
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
