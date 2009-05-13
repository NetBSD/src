/*	$NetBSD: kern_descrip.c,v 1.185.2.1 2009/05/13 17:21:56 jym Exp $	*/

/*-
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_descrip.c	8.8 (Berkeley) 2/14/95
 */

/*
 * File descriptor management.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_descrip.c,v 1.185.2.1 2009/05/13 17:21:56 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/pool.h>
#include <sys/unistd.h>
#include <sys/resourcevar.h>
#include <sys/conf.h>
#include <sys/event.h>
#include <sys/kauth.h>
#include <sys/atomic.h>
#include <sys/syscallargs.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/vnode.h>

static int	file_ctor(void *, void *, int);
static void	file_dtor(void *, void *);
static int	fdfile_ctor(void *, void *, int);
static void	fdfile_dtor(void *, void *);
static int	filedesc_ctor(void *, void *, int);
static void	filedesc_dtor(void *, void *);
static int	filedescopen(dev_t, int, int, lwp_t *);

kmutex_t	filelist_lock;	/* lock on filehead */
struct filelist	filehead;	/* head of list of open files */
u_int		nfiles;		/* actual number of open files */

static pool_cache_t filedesc_cache;
static pool_cache_t file_cache;
static pool_cache_t fdfile_cache;

const struct cdevsw filedesc_cdevsw = {
	filedescopen, noclose, noread, nowrite, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER | D_MPSAFE,
};

/* For ease of reading. */
__strong_alias(fd_putvnode,fd_putfile)
__strong_alias(fd_putsock,fd_putfile)

/*
 * Initialize the descriptor system.
 */
void
fd_sys_init(void)
{

	mutex_init(&filelist_lock, MUTEX_DEFAULT, IPL_NONE);

	file_cache = pool_cache_init(sizeof(file_t), coherency_unit, 0,
	    0, "file", NULL, IPL_NONE, file_ctor, file_dtor, NULL);
	KASSERT(file_cache != NULL);

	fdfile_cache = pool_cache_init(sizeof(fdfile_t), coherency_unit, 0,
	    PR_LARGECACHE, "fdfile", NULL, IPL_NONE, fdfile_ctor, fdfile_dtor,
	    NULL);
	KASSERT(fdfile_cache != NULL);

	filedesc_cache = pool_cache_init(sizeof(filedesc_t), coherency_unit,
	    0, 0, "filedesc", NULL, IPL_NONE, filedesc_ctor, filedesc_dtor,
	    NULL);
	KASSERT(filedesc_cache != NULL);
}

static int
fd_next_zero(filedesc_t *fdp, uint32_t *bitmap, int want, u_int bits)
{
	int i, off, maxoff;
	uint32_t sub;

	KASSERT(mutex_owned(&fdp->fd_lock));

	if (want > bits)
		return -1;

	off = want >> NDENTRYSHIFT;
	i = want & NDENTRYMASK;
	if (i) {
		sub = bitmap[off] | ((u_int)~0 >> (NDENTRIES - i));
		if (sub != ~0)
			goto found;
		off++;
	}

	maxoff = NDLOSLOTS(bits);
	while (off < maxoff) {
		if ((sub = bitmap[off]) != ~0)
			goto found;
		off++;
	}

	return (-1);

 found:
	return (off << NDENTRYSHIFT) + ffs(~sub) - 1;
}

static int
fd_last_set(filedesc_t *fd, int last)
{
	int off, i;
	fdfile_t **ofiles = fd->fd_ofiles;
	uint32_t *bitmap = fd->fd_lomap;

	KASSERT(mutex_owned(&fd->fd_lock));

	off = (last - 1) >> NDENTRYSHIFT;

	while (off >= 0 && !bitmap[off])
		off--;

	if (off < 0)
		return (-1);

	i = ((off + 1) << NDENTRYSHIFT) - 1;
	if (i >= last)
		i = last - 1;

	/* XXX should use bitmap */
	/* XXXAD does not work for fd_copy() */
	while (i > 0 && (ofiles[i] == NULL || !ofiles[i]->ff_allocated))
		i--;

	return (i);
}

void
fd_used(filedesc_t *fdp, unsigned fd)
{
	u_int off = fd >> NDENTRYSHIFT;
	fdfile_t *ff;

	ff = fdp->fd_ofiles[fd];

	KASSERT(mutex_owned(&fdp->fd_lock));
	KASSERT((fdp->fd_lomap[off] & (1 << (fd & NDENTRYMASK))) == 0);
	KASSERT(ff != NULL);
	KASSERT(ff->ff_file == NULL);
   	KASSERT(!ff->ff_allocated);

   	ff->ff_allocated = 1;
	fdp->fd_lomap[off] |= 1 << (fd & NDENTRYMASK);
	if (fdp->fd_lomap[off] == ~0) {
		KASSERT((fdp->fd_himap[off >> NDENTRYSHIFT] &
		    (1 << (off & NDENTRYMASK))) == 0);
		fdp->fd_himap[off >> NDENTRYSHIFT] |= 1 << (off & NDENTRYMASK);
	}

	if ((int)fd > fdp->fd_lastfile) {
		fdp->fd_lastfile = fd;
	}

	if (fd >= NDFDFILE) {
		fdp->fd_nused++;
	} else {
		KASSERT(ff == (fdfile_t *)fdp->fd_dfdfile[fd]);
	}
}

void
fd_unused(filedesc_t *fdp, unsigned fd)
{
	u_int off = fd >> NDENTRYSHIFT;
	fdfile_t *ff;

	ff = fdp->fd_ofiles[fd];

	/*
	 * Don't assert the lock is held here, as we may be copying
	 * the table during exec() and it is not needed there.
	 * procfs and sysctl are locked out by proc::p_reflock.
	 *
	 * KASSERT(mutex_owned(&fdp->fd_lock));
	 */
	KASSERT(ff != NULL);
	KASSERT(ff->ff_file == NULL);
   	KASSERT(ff->ff_allocated);

	if (fd < fdp->fd_freefile) {
		fdp->fd_freefile = fd;
	}

	if (fdp->fd_lomap[off] == ~0) {
		KASSERT((fdp->fd_himap[off >> NDENTRYSHIFT] &
		    (1 << (off & NDENTRYMASK))) != 0);
		fdp->fd_himap[off >> NDENTRYSHIFT] &=
		    ~(1 << (off & NDENTRYMASK));
	}
	KASSERT((fdp->fd_lomap[off] & (1 << (fd & NDENTRYMASK))) != 0);
	fdp->fd_lomap[off] &= ~(1 << (fd & NDENTRYMASK));
	ff->ff_allocated = 0;

	KASSERT(fd <= fdp->fd_lastfile);
	if (fd == fdp->fd_lastfile) {
		fdp->fd_lastfile = fd_last_set(fdp, fd);
	}

	if (fd >= NDFDFILE) {
		KASSERT(fdp->fd_nused > 0);
		fdp->fd_nused--;
	} else {
		KASSERT(ff == (fdfile_t *)fdp->fd_dfdfile[fd]);
	}
}

/*
 * Custom version of fd_unused() for fd_copy(), where the descriptor
 * table is not yet fully initialized.
 */
static inline void
fd_zap(filedesc_t *fdp, unsigned fd)
{
	u_int off = fd >> NDENTRYSHIFT;

	if (fd < fdp->fd_freefile) {
		fdp->fd_freefile = fd;
	}

	if (fdp->fd_lomap[off] == ~0) {
		KASSERT((fdp->fd_himap[off >> NDENTRYSHIFT] &
		    (1 << (off & NDENTRYMASK))) != 0);
		fdp->fd_himap[off >> NDENTRYSHIFT] &=
		    ~(1 << (off & NDENTRYMASK));
	}
	KASSERT((fdp->fd_lomap[off] & (1 << (fd & NDENTRYMASK))) != 0);
	fdp->fd_lomap[off] &= ~(1 << (fd & NDENTRYMASK));
}

bool
fd_isused(filedesc_t *fdp, unsigned fd)
{
	u_int off = fd >> NDENTRYSHIFT;

	KASSERT(fd < fdp->fd_nfiles);

	return (fdp->fd_lomap[off] & (1 << (fd & NDENTRYMASK))) != 0;
}

/*
 * Look up the file structure corresponding to a file descriptor
 * and return the file, holding a reference on the descriptor.
 */
inline file_t *
fd_getfile(unsigned fd)
{
	filedesc_t *fdp;
	fdfile_t *ff;
	file_t *fp;

	fdp = curlwp->l_fd;

	/*
	 * Look up the fdfile structure representing this descriptor.
	 * Ensure that we see fd_nfiles before fd_ofiles since we
	 * are doing this unlocked.  See fd_tryexpand().
	 */
	if (__predict_false(fd >= fdp->fd_nfiles)) {
		return NULL;
	}
	membar_consumer();
	ff = fdp->fd_ofiles[fd];
	KASSERT(fd >= NDFDFILE || ff == (fdfile_t *)fdp->fd_dfdfile[fd]);
	if (__predict_false(ff == NULL)) {
		return NULL;
	}

	/*
	 * Now get a reference to the descriptor.   Issue a memory
	 * barrier to ensure that we acquire the file pointer _after_
	 * adding a reference.  If no memory barrier, we could fetch
	 * a stale pointer.
	 */
	atomic_inc_uint(&ff->ff_refcnt);
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_enter();
#endif

	/*
	 * If the file is not open or is being closed then put the
	 * reference back.
	 */
	fp = ff->ff_file;
	if (__predict_true(fp != NULL)) {
		return fp;
	}
	fd_putfile(fd);
	return NULL;
}

/*
 * Release a reference to a file descriptor acquired with fd_getfile().
 */
void
fd_putfile(unsigned fd)
{
	filedesc_t *fdp;
	fdfile_t *ff;
	u_int u, v;

	fdp = curlwp->l_fd;
	ff = fdp->fd_ofiles[fd];

	KASSERT(fd < fdp->fd_nfiles);
	KASSERT(ff != NULL);
	KASSERT((ff->ff_refcnt & FR_MASK) > 0);
	KASSERT(fd >= NDFDFILE || ff == (fdfile_t *)fdp->fd_dfdfile[fd]);

	/*
	 * Ensure that any use of the file is complete and globally
	 * visible before dropping the final reference.  If no membar,
	 * the current CPU could still access memory associated with
	 * the file after it has been freed or recycled by another
	 * CPU.
	 */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_exit();
#endif

	/*
	 * Be optimistic and start out with the assumption that no other
	 * threads are trying to close the descriptor.  If the CAS fails,
	 * we lost a race and/or it's being closed.
	 */
	for (u = ff->ff_refcnt & FR_MASK;; u = v) {
		v = atomic_cas_uint(&ff->ff_refcnt, u, u - 1);
		if (__predict_true(u == v)) {
			return;
		}
		if (__predict_false((v & FR_CLOSING) != 0)) {
			break;
		}
	}

	/* Another thread is waiting to close the file: join it. */
	(void)fd_close(fd);
}

/*
 * Convenience wrapper around fd_getfile() that returns reference
 * to a vnode.
 */
int
fd_getvnode(unsigned fd, file_t **fpp)
{
	vnode_t *vp;
	file_t *fp;

	fp = fd_getfile(fd);
	if (__predict_false(fp == NULL)) {
		return EBADF;
	}
	if (__predict_false(fp->f_type != DTYPE_VNODE)) {
		fd_putfile(fd);
		return EINVAL;
	}
	vp = fp->f_data;
	if (__predict_false(vp->v_type == VBAD)) {
		/* XXX Is this case really necessary? */
		fd_putfile(fd);
		return EBADF;
	}
	*fpp = fp;
	return 0;
}

/*
 * Convenience wrapper around fd_getfile() that returns reference
 * to a socket.
 */
int
fd_getsock(unsigned fd, struct socket **sop)
{
	file_t *fp;

	fp = fd_getfile(fd);
	if (__predict_false(fp == NULL)) {
		return EBADF;
	}
	if (__predict_false(fp->f_type != DTYPE_SOCKET)) {
		fd_putfile(fd);
		return ENOTSOCK;
	}
	*sop = fp->f_data;
	return 0;
}

/*
 * Look up the file structure corresponding to a file descriptor
 * and return it with a reference held on the file, not the
 * descriptor.
 *
 * This is heavyweight and only used when accessing descriptors
 * from a foreign process.  The caller must ensure that `p' does
 * not exit or fork across this call.
 *
 * To release the file (not descriptor) reference, use closef().
 */
file_t *
fd_getfile2(proc_t *p, unsigned fd)
{
	filedesc_t *fdp;
	fdfile_t *ff;
	file_t *fp;

	fdp = p->p_fd;
	mutex_enter(&fdp->fd_lock);
	if (fd > fdp->fd_nfiles) {
		mutex_exit(&fdp->fd_lock);
		return NULL;
	}
	if ((ff = fdp->fd_ofiles[fd]) == NULL) {
		mutex_exit(&fdp->fd_lock);
		return NULL;
	}
	mutex_enter(&ff->ff_lock);
	if ((fp = ff->ff_file) == NULL) {
		mutex_exit(&ff->ff_lock);
		mutex_exit(&fdp->fd_lock);
		return NULL;
	}
	mutex_enter(&fp->f_lock);
	fp->f_count++;
	mutex_exit(&fp->f_lock);
	mutex_exit(&ff->ff_lock);
	mutex_exit(&fdp->fd_lock);

	return fp;
}

/*
 * Internal form of close.  Must be called with a reference to the
 * descriptor, and will drop the reference.  When all descriptor
 * references are dropped, releases the descriptor slot and a single
 * reference to the file structure.
 */
int
fd_close(unsigned fd)
{
	struct flock lf;
	filedesc_t *fdp;
	fdfile_t *ff;
	file_t *fp;
	proc_t *p;
	lwp_t *l;

	l = curlwp;
	p = l->l_proc;
	fdp = l->l_fd;
	ff = fdp->fd_ofiles[fd];

	KASSERT(fd >= NDFDFILE || ff == (fdfile_t *)fdp->fd_dfdfile[fd]);

	mutex_enter(&ff->ff_lock);
	KASSERT((ff->ff_refcnt & FR_MASK) > 0);
	if (ff->ff_file == NULL) {
		/*
		 * Another user of the file is already closing, and is
		 * waiting for other users of the file to drain.  Release
		 * our reference, and wake up the closer.
		 */
		atomic_dec_uint(&ff->ff_refcnt);
		cv_broadcast(&ff->ff_closing);
		mutex_exit(&ff->ff_lock);

		/*
		 * An application error, so pretend that the descriptor
		 * was already closed.  We can't safely wait for it to
		 * be closed without potentially deadlocking.
		 */
		return (EBADF);
	}
	KASSERT((ff->ff_refcnt & FR_CLOSING) == 0);

	/*
	 * There may be multiple users of this file within the process.
	 * Notify existing and new users that the file is closing.  This
	 * will prevent them from adding additional uses to this file
	 * while we are closing it.
	 */
	fp = ff->ff_file;
	ff->ff_file = NULL;
	ff->ff_exclose = false;

	/*
	 * We expect the caller to hold a descriptor reference - drop it.
	 * The reference count may increase beyond zero at this point due
	 * to an erroneous descriptor reference by an application, but
	 * fd_getfile() will notice that the file is being closed and drop
	 * the reference again.
	 */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_producer();
#endif
	if (__predict_false(atomic_dec_uint_nv(&ff->ff_refcnt) != 0)) {
		/*
		 * Wait for other references to drain.  This is typically
		 * an application error - the descriptor is being closed
		 * while still in use.
		 *
		 */
		atomic_or_uint(&ff->ff_refcnt, FR_CLOSING);

		/*
		 * Remove any knotes attached to the file.  A knote
		 * attached to the descriptor can hold references on it.
		 */
		mutex_exit(&ff->ff_lock);
		if (!SLIST_EMPTY(&ff->ff_knlist)) {
			knote_fdclose(fd);
		}

		/* Try to drain out descriptor references. */
		(*fp->f_ops->fo_drain)(fp);
		mutex_enter(&ff->ff_lock);

		/*
		 * We need to see the count drop to zero at least once,
		 * in order to ensure that all pre-existing references
		 * have been drained.  New references past this point are
		 * of no interest.
		 */
		while ((ff->ff_refcnt & FR_MASK) != 0) {
			cv_wait(&ff->ff_closing, &ff->ff_lock);
		}
		atomic_and_uint(&ff->ff_refcnt, ~FR_CLOSING);
	} else {
		/* If no references, there must be no knotes. */
		KASSERT(SLIST_EMPTY(&ff->ff_knlist));
	}
	mutex_exit(&ff->ff_lock);

	/*
	 * POSIX record locking dictates that any close releases ALL
	 * locks owned by this process.  This is handled by setting
	 * a flag in the unlock to free ONLY locks obeying POSIX
	 * semantics, and not to free BSD-style file locks.
	 * If the descriptor was in a message, POSIX-style locks
	 * aren't passed with the descriptor.
	 */
	if ((p->p_flag & PK_ADVLOCK) != 0 && fp->f_type == DTYPE_VNODE) {
		lf.l_whence = SEEK_SET;
		lf.l_start = 0;
		lf.l_len = 0;
		lf.l_type = F_UNLCK;
		(void)VOP_ADVLOCK(fp->f_data, p, F_UNLCK, &lf, F_POSIX);
	}


	/* Free descriptor slot. */
	mutex_enter(&fdp->fd_lock);
	fd_unused(fdp, fd);
	mutex_exit(&fdp->fd_lock);

	/* Now drop reference to the file itself. */
	return closef(fp);
}

/*
 * Duplicate a file descriptor.
 */
int
fd_dup(file_t *fp, int minfd, int *newp, bool exclose)
{
	proc_t *p;
	int error;

	p = curproc;

	while ((error = fd_alloc(p, minfd, newp)) != 0) {
		if (error != ENOSPC) {
			return error;
		}
		fd_tryexpand(p);
	}

	curlwp->l_fd->fd_ofiles[*newp]->ff_exclose = exclose;
	fd_affix(p, fp, *newp);
	return 0;
}

/*
 * dup2 operation.
 */
int
fd_dup2(file_t *fp, unsigned new)
{
	filedesc_t *fdp;
	fdfile_t *ff;

	fdp = curlwp->l_fd;

	/*
	 * Ensure there are enough slots in the descriptor table,
	 * and allocate an fdfile_t up front in case we need it.
	 */
	while (new >= fdp->fd_nfiles) {
		fd_tryexpand(curproc);
	}
	ff = pool_cache_get(fdfile_cache, PR_WAITOK);

	/*
	 * If there is already a file open, close it.  If the file is
	 * half open, wait for it to be constructed before closing it.
	 * XXX Potential for deadlock here?
	 */
	mutex_enter(&fdp->fd_lock);
	while (fd_isused(fdp, new)) {
		mutex_exit(&fdp->fd_lock);
		if (fd_getfile(new) != NULL) {
			(void)fd_close(new);
		} else {
			/* XXX Crummy, but unlikely to happen. */
			kpause("dup2", false, 1, NULL);
		}
		mutex_enter(&fdp->fd_lock);
	}
	if (fdp->fd_ofiles[new] == NULL) {
		KASSERT(new >= NDFDFILE);
		fdp->fd_ofiles[new] = ff;
		ff = NULL;
	}		
	fd_used(fdp, new);
	mutex_exit(&fdp->fd_lock);

	/* Slot is now allocated.  Insert copy of the file. */
	fd_affix(curproc, fp, new);
	if (ff != NULL) {
		pool_cache_put(fdfile_cache, ff);
	}
	return 0;
}

/*
 * Drop reference to a file structure.
 */
int
closef(file_t *fp)
{
	struct flock lf;
	int error;

	/*
	 * Drop reference.  If referenced elsewhere it's still open
	 * and we have nothing more to do.
	 */
	mutex_enter(&fp->f_lock);
	KASSERT(fp->f_count > 0);
	if (--fp->f_count > 0) {
		mutex_exit(&fp->f_lock);
		return 0;
	}
	KASSERT(fp->f_count == 0);
	mutex_exit(&fp->f_lock);

	/* We held the last reference - release locks, close and free. */
        if ((fp->f_flag & FHASLOCK) && fp->f_type == DTYPE_VNODE) {
        	lf.l_whence = SEEK_SET;
		lf.l_start = 0;
		lf.l_len = 0;
		lf.l_type = F_UNLCK;
		(void)VOP_ADVLOCK(fp->f_data, fp, F_UNLCK, &lf, F_FLOCK);
	}
	if (fp->f_ops != NULL) {
		error = (*fp->f_ops->fo_close)(fp);
	} else {
		error = 0;
	}
	ffree(fp);

	return error;
}

/*
 * Allocate a file descriptor for the process.
 */
int
fd_alloc(proc_t *p, int want, int *result)
{
	filedesc_t *fdp;
	int i, lim, last, error;
	u_int off, new;
	fdfile_t *ff;

	KASSERT(p == curproc || p == &proc0);

	fdp = p->p_fd;
	ff = pool_cache_get(fdfile_cache, PR_WAITOK);
	KASSERT(ff->ff_refcnt == 0);
	KASSERT(ff->ff_file == NULL);

	/*
	 * Search for a free descriptor starting at the higher
	 * of want or fd_freefile.
	 */
	mutex_enter(&fdp->fd_lock);
	KASSERT(fdp->fd_ofiles[0] == (fdfile_t *)fdp->fd_dfdfile[0]);
	lim = min((int)p->p_rlimit[RLIMIT_NOFILE].rlim_cur, maxfiles);
	last = min(fdp->fd_nfiles, lim);
	for (;;) {
		if ((i = want) < fdp->fd_freefile)
			i = fdp->fd_freefile;
		off = i >> NDENTRYSHIFT;
		new = fd_next_zero(fdp, fdp->fd_himap, off,
		    (last + NDENTRIES - 1) >> NDENTRYSHIFT);
		if (new == -1)
			break;
		i = fd_next_zero(fdp, &fdp->fd_lomap[new],
		    new > off ? 0 : i & NDENTRYMASK, NDENTRIES);
		if (i == -1) {
			/*
			 * Free file descriptor in this block was
			 * below want, try again with higher want.
			 */
			want = (new + 1) << NDENTRYSHIFT;
			continue;
		}
		i += (new << NDENTRYSHIFT);
		if (i >= last) {
			break;
		}
		if (fdp->fd_ofiles[i] == NULL) {
			KASSERT(i >= NDFDFILE);
			fdp->fd_ofiles[i] = ff;
		} else {
		   	pool_cache_put(fdfile_cache, ff);
		}
		KASSERT(fdp->fd_ofiles[i]->ff_file == NULL);
		fd_used(fdp, i);
		if (want <= fdp->fd_freefile) {
			fdp->fd_freefile = i;
		}
		*result = i;
		mutex_exit(&fdp->fd_lock);
		KASSERT(i >= NDFDFILE ||
		    fdp->fd_ofiles[i] == (fdfile_t *)fdp->fd_dfdfile[i]);
		return 0;
	}

	/* No space in current array.  Let the caller expand and retry. */
	error = (fdp->fd_nfiles >= lim) ? EMFILE : ENOSPC;
	mutex_exit(&fdp->fd_lock);
	pool_cache_put(fdfile_cache, ff);
	return error;
}

/*
 * Allocate memory for the open files array.
 */
static fdfile_t **
fd_ofile_alloc(int n)
{
	uintptr_t *ptr, sz;

	KASSERT(n > NDFILE);

	sz = (n + 2) * sizeof(uintptr_t);
	ptr = kmem_alloc((size_t)sz, KM_SLEEP);
	ptr[1] = sz;

	return (fdfile_t **)(ptr + 2);
}

/*
 * Free an open files array.
 */
static void
fd_ofile_free(int n, fdfile_t **of)
{
	uintptr_t *ptr, sz;

	KASSERT(n > NDFILE);

	sz = (n + 2) * sizeof(uintptr_t);
	ptr = (uintptr_t *)of - 2;
	KASSERT(ptr[1] == sz);
	kmem_free(ptr, sz);
}

/*
 * Allocate descriptor bitmap.
 */
static void
fd_map_alloc(int n, uint32_t **lo, uint32_t **hi)
{
	uint8_t *ptr;
	size_t szlo, szhi;

	KASSERT(n > NDENTRIES);

	szlo = NDLOSLOTS(n) * sizeof(uint32_t);
	szhi = NDHISLOTS(n) * sizeof(uint32_t);
	ptr = kmem_alloc(szlo + szhi, KM_SLEEP);
	*lo = (uint32_t *)ptr;
	*hi = (uint32_t *)(ptr + szlo);
}

/*
 * Free descriptor bitmap.
 */
static void
fd_map_free(int n, uint32_t *lo, uint32_t *hi)
{
	size_t szlo, szhi;

	KASSERT(n > NDENTRIES);

	szlo = NDLOSLOTS(n) * sizeof(uint32_t);
	szhi = NDHISLOTS(n) * sizeof(uint32_t);
	KASSERT(hi == (uint32_t *)((uint8_t *)lo + szlo));
	kmem_free(lo, szlo + szhi);
}

/*
 * Expand a process' descriptor table.
 */
void
fd_tryexpand(proc_t *p)
{
	filedesc_t *fdp;
	int i, numfiles, oldnfiles;
	fdfile_t **newofile;
	uint32_t *newhimap, *newlomap;

	KASSERT(p == curproc || p == &proc0);

	fdp = p->p_fd;
	newhimap = NULL;
	newlomap = NULL;
	oldnfiles = fdp->fd_nfiles;

	if (oldnfiles < NDEXTENT)
		numfiles = NDEXTENT;
	else
		numfiles = 2 * oldnfiles;

	newofile = fd_ofile_alloc(numfiles);
	if (NDHISLOTS(numfiles) > NDHISLOTS(oldnfiles)) {
		fd_map_alloc(numfiles, &newlomap, &newhimap);
	}

	mutex_enter(&fdp->fd_lock);
	KASSERT(fdp->fd_ofiles[0] == (fdfile_t *)fdp->fd_dfdfile[0]);
	if (fdp->fd_nfiles != oldnfiles) {
		/* fdp changed; caller must retry */
		mutex_exit(&fdp->fd_lock);
		fd_ofile_free(numfiles, newofile);
		if (NDHISLOTS(numfiles) > NDHISLOTS(oldnfiles)) {
			fd_map_free(numfiles, newlomap, newhimap);
		}
		return;
	}

	/* Copy the existing ofile array and zero the new portion. */
	i = sizeof(fdfile_t *) * fdp->fd_nfiles;
	memcpy(newofile, fdp->fd_ofiles, i);
	memset((uint8_t *)newofile + i, 0, numfiles * sizeof(fdfile_t *) - i);

	/*
	 * Link old ofiles array into list to be discarded.  We defer
	 * freeing until process exit if the descriptor table is visble
	 * to other threads.
	 */
	if (oldnfiles > NDFILE) {
		if ((fdp->fd_refcnt | p->p_nlwps) > 1) {
			fdp->fd_ofiles[-2] = (void *)fdp->fd_discard;
			fdp->fd_discard = fdp->fd_ofiles - 2;
		} else {
			fd_ofile_free(oldnfiles, fdp->fd_ofiles);
		}
	}

	if (NDHISLOTS(numfiles) > NDHISLOTS(oldnfiles)) {
		i = NDHISLOTS(oldnfiles) * sizeof(uint32_t);
		memcpy(newhimap, fdp->fd_himap, i);
		memset((uint8_t *)newhimap + i, 0,
		    NDHISLOTS(numfiles) * sizeof(uint32_t) - i);

		i = NDLOSLOTS(oldnfiles) * sizeof(uint32_t);
		memcpy(newlomap, fdp->fd_lomap, i);
		memset((uint8_t *)newlomap + i, 0,
		    NDLOSLOTS(numfiles) * sizeof(uint32_t) - i);

		if (NDHISLOTS(oldnfiles) > NDHISLOTS(NDFILE)) {
			fd_map_free(oldnfiles, fdp->fd_lomap, fdp->fd_himap);
		}
		fdp->fd_himap = newhimap;
		fdp->fd_lomap = newlomap;
	}

	/*
	 * All other modifications must become globally visible before
	 * the change to fd_nfiles.  See fd_getfile().
	 */
	fdp->fd_ofiles = newofile;
	membar_producer();
	fdp->fd_nfiles = numfiles;
	mutex_exit(&fdp->fd_lock);

	KASSERT(fdp->fd_ofiles[0] == (fdfile_t *)fdp->fd_dfdfile[0]);
}

/*
 * Create a new open file structure and allocate a file descriptor
 * for the current process.
 */
int
fd_allocfile(file_t **resultfp, int *resultfd)
{
	file_t *fp;
	proc_t *p;
	int error;

	p = curproc;

	while ((error = fd_alloc(p, 0, resultfd)) != 0) {
		if (error != ENOSPC) {
			return error;
		}
		fd_tryexpand(p);
	}

	fp = pool_cache_get(file_cache, PR_WAITOK);
	KASSERT(fp->f_count == 0);
	KASSERT(fp->f_msgcount == 0);
	KASSERT(fp->f_unpcount == 0);
	fp->f_cred = kauth_cred_get();
	kauth_cred_hold(fp->f_cred);

	if (__predict_false(atomic_inc_uint_nv(&nfiles) >= maxfiles)) {
		fd_abort(p, fp, *resultfd);
		tablefull("file", "increase kern.maxfiles or MAXFILES");
		return ENFILE;
	}

	/*
	 * Don't allow recycled files to be scanned.
	 */
	if ((fp->f_flag & FSCAN) != 0) {
		mutex_enter(&fp->f_lock);
		atomic_and_uint(&fp->f_flag, ~FSCAN);
		mutex_exit(&fp->f_lock);
	}

	fp->f_advice = 0;
	fp->f_msgcount = 0;
	fp->f_offset = 0;
	*resultfp = fp;

	return 0;
}

/*
 * Successful creation of a new descriptor: make visible to the process.
 */
void
fd_affix(proc_t *p, file_t *fp, unsigned fd)
{
	fdfile_t *ff;
	filedesc_t *fdp;

	KASSERT(p == curproc || p == &proc0);

	/* Add a reference to the file structure. */
	mutex_enter(&fp->f_lock);
	fp->f_count++;
	mutex_exit(&fp->f_lock);

	/*
	 * Insert the new file into the descriptor slot.
	 *
	 * The memory barriers provided by lock activity in this routine
	 * ensure that any updates to the file structure become globally
	 * visible before the file becomes visible to other LWPs in the
	 * current process.
	 */
	fdp = p->p_fd;
	ff = fdp->fd_ofiles[fd];

	KASSERT(ff != NULL);
	KASSERT(ff->ff_file == NULL);
	KASSERT(ff->ff_allocated);
	KASSERT(fd_isused(fdp, fd));
	KASSERT(fd >= NDFDFILE ||
	    fdp->fd_ofiles[fd] == (fdfile_t *)fdp->fd_dfdfile[fd]);

	/* No need to lock in order to make file initially visible. */
	ff->ff_file = fp;
}

/*
 * Abort creation of a new descriptor: free descriptor slot and file.
 */
void
fd_abort(proc_t *p, file_t *fp, unsigned fd)
{
	filedesc_t *fdp;
	fdfile_t *ff;

	KASSERT(p == curproc || p == &proc0);

	fdp = p->p_fd;
	ff = fdp->fd_ofiles[fd];

	KASSERT(fd >= NDFDFILE ||
	    fdp->fd_ofiles[fd] == (fdfile_t *)fdp->fd_dfdfile[fd]);

	mutex_enter(&fdp->fd_lock);
	KASSERT(fd_isused(fdp, fd));
	fd_unused(fdp, fd);
	mutex_exit(&fdp->fd_lock);

	if (fp != NULL) {
		ffree(fp);
	}
}

/*
 * Free a file descriptor.
 */
void
ffree(file_t *fp)
{

	KASSERT(fp->f_count == 0);

	atomic_dec_uint(&nfiles);
	kauth_cred_free(fp->f_cred);
	pool_cache_put(file_cache, fp);
}

static int
file_ctor(void *arg, void *obj, int flags)
{
	file_t *fp = obj;

	memset(fp, 0, sizeof(*fp));
	mutex_init(&fp->f_lock, MUTEX_DEFAULT, IPL_NONE);

	mutex_enter(&filelist_lock);
	LIST_INSERT_HEAD(&filehead, fp, f_list);
	mutex_exit(&filelist_lock);

	return 0;
}

static void
file_dtor(void *arg, void *obj)
{
	file_t *fp = obj;

	mutex_enter(&filelist_lock);
	LIST_REMOVE(fp, f_list);
	mutex_exit(&filelist_lock);

	mutex_destroy(&fp->f_lock);
}

static int
fdfile_ctor(void *arg, void *obj, int flags)
{
	fdfile_t *ff = obj;

	memset(ff, 0, sizeof(*ff));
	mutex_init(&ff->ff_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&ff->ff_closing, "fdclose");

	return 0;
}

static void
fdfile_dtor(void *arg, void *obj)
{
	fdfile_t *ff = obj;

	mutex_destroy(&ff->ff_lock);
	cv_destroy(&ff->ff_closing);
}

file_t *
fgetdummy(void)
{
	file_t *fp;

	fp = kmem_alloc(sizeof(*fp), KM_SLEEP);
	if (fp != NULL) {
		memset(fp, 0, sizeof(*fp));
		mutex_init(&fp->f_lock, MUTEX_DEFAULT, IPL_NONE);
	}
	return fp;
}

void
fputdummy(file_t *fp)
{

	mutex_destroy(&fp->f_lock);
	kmem_free(fp, sizeof(*fp));
}

/*
 * Create an initial filedesc structure.
 */
filedesc_t *
fd_init(filedesc_t *fdp)
{
	unsigned fd;

	if (fdp == NULL) {
		fdp = pool_cache_get(filedesc_cache, PR_WAITOK);
	} else {
		filedesc_ctor(NULL, fdp, PR_WAITOK);
	}

	fdp->fd_refcnt = 1;
	fdp->fd_ofiles = fdp->fd_dfiles;
	fdp->fd_nfiles = NDFILE;
	fdp->fd_himap = fdp->fd_dhimap;
	fdp->fd_lomap = fdp->fd_dlomap;
	KASSERT(fdp->fd_lastfile == -1);
	KASSERT(fdp->fd_lastkqfile == -1);
	KASSERT(fdp->fd_knhash == NULL);

	memset(&fdp->fd_startzero, 0, sizeof(*fdp) -
	    offsetof(filedesc_t, fd_startzero));
	for (fd = 0; fd < NDFDFILE; fd++) {
		fdp->fd_ofiles[fd] = (fdfile_t *)fdp->fd_dfdfile[fd];
	}

	return fdp;
}

/*
 * Initialize a file descriptor table.
 */
static int
filedesc_ctor(void *arg, void *obj, int flag)
{
	filedesc_t *fdp = obj;
	int i;

	memset(fdp, 0, sizeof(*fdp));
	mutex_init(&fdp->fd_lock, MUTEX_DEFAULT, IPL_NONE);
	fdp->fd_lastfile = -1;
	fdp->fd_lastkqfile = -1;

	CTASSERT(sizeof(fdp->fd_dfdfile[0]) >= sizeof(fdfile_t));
	for (i = 0; i < NDFDFILE; i++) {
		fdfile_ctor(NULL, fdp->fd_dfdfile[i], PR_WAITOK);
	}

	return 0;
}

static void
filedesc_dtor(void *arg, void *obj)
{
	filedesc_t *fdp = obj;
	int i;

	for (i = 0; i < NDFDFILE; i++) {
		fdfile_dtor(NULL, fdp->fd_dfdfile[i]);
	}

	mutex_destroy(&fdp->fd_lock);
}

/*
 * Make p2 share p1's filedesc structure.
 */
void
fd_share(struct proc *p2)
{
	filedesc_t *fdp;

	fdp = curlwp->l_fd;
	p2->p_fd = fdp;
	atomic_inc_uint(&fdp->fd_refcnt);
}

/*
 * Copy a filedesc structure.
 */
filedesc_t *
fd_copy(void)
{
	filedesc_t *newfdp, *fdp;
	fdfile_t *ff, *fflist, **ffp, **nffp, *ff2;
	int i, nused, numfiles, lastfile, j, newlast;
	file_t *fp;

	fdp = curproc->p_fd;
	newfdp = pool_cache_get(filedesc_cache, PR_WAITOK);
	newfdp->fd_refcnt = 1;

	KASSERT(newfdp->fd_knhash == NULL);
	KASSERT(newfdp->fd_knhashmask == 0);
	KASSERT(newfdp->fd_discard == NULL);

	for (;;) {
		numfiles = fdp->fd_nfiles;
		lastfile = fdp->fd_lastfile;

		/*
		 * If the number of open files fits in the internal arrays
		 * of the open file structure, use them, otherwise allocate
		 * additional memory for the number of descriptors currently
		 * in use.
		 */
		if (lastfile < NDFILE) {
			i = NDFILE;
			newfdp->fd_ofiles = newfdp->fd_dfiles;
		} else {
			/*
			 * Compute the smallest multiple of NDEXTENT needed
			 * for the file descriptors currently in use,
			 * allowing the table to shrink.
			 */
			i = numfiles;
			while (i >= 2 * NDEXTENT && i > lastfile * 2) {
				i /= 2;
			}
			newfdp->fd_ofiles = fd_ofile_alloc(i);
			KASSERT(i > NDFILE);
		}
		if (NDHISLOTS(i) <= NDHISLOTS(NDFILE)) {
			newfdp->fd_himap = newfdp->fd_dhimap;
			newfdp->fd_lomap = newfdp->fd_dlomap;
		} else {
			fd_map_alloc(i, &newfdp->fd_lomap,
			    &newfdp->fd_himap);
		}

		/*
		 * Allocate and string together fdfile structures.
		 * We abuse fdfile_t::ff_file here, but it will be
		 * cleared before this routine returns.
		 */
		nused = fdp->fd_nused;
		fflist = NULL;
		for (j = nused; j != 0; j--) {
			ff = pool_cache_get(fdfile_cache, PR_WAITOK);
			ff->ff_file = (void *)fflist;
			fflist = ff;
		}

		mutex_enter(&fdp->fd_lock);
		if (numfiles == fdp->fd_nfiles && nused == fdp->fd_nused &&
		    lastfile == fdp->fd_lastfile) {
			break;
		}
		mutex_exit(&fdp->fd_lock);
		if (i > NDFILE) {
			fd_ofile_free(i, newfdp->fd_ofiles);
		}
		if (NDHISLOTS(i) > NDHISLOTS(NDFILE)) {
			fd_map_free(i, newfdp->fd_lomap, newfdp->fd_himap);
		}
		while (fflist != NULL) {
			ff = fflist;
			fflist = (void *)ff->ff_file;
			ff->ff_file = NULL;
			pool_cache_put(fdfile_cache, ff);
		}
	}

	newfdp->fd_nfiles = i;
	newfdp->fd_freefile = fdp->fd_freefile;
	newfdp->fd_exclose = fdp->fd_exclose;

	/*
	 * Clear the entries that will not be copied over.
	 * Avoid calling memset with 0 size.
	 */
	if (lastfile < (i-1)) {
		memset(newfdp->fd_ofiles + lastfile + 1, 0,
		    (i - lastfile - 1) * sizeof(file_t **));
	}
	if (i < NDENTRIES * NDENTRIES) {
		i = NDENTRIES * NDENTRIES; /* size of inlined bitmaps */
	}
	memcpy(newfdp->fd_himap, fdp->fd_himap, NDHISLOTS(i)*sizeof(uint32_t));
	memcpy(newfdp->fd_lomap, fdp->fd_lomap, NDLOSLOTS(i)*sizeof(uint32_t));

	ffp = fdp->fd_ofiles;
	nffp = newfdp->fd_ofiles;
	j = imax(lastfile, (NDFDFILE - 1));
	newlast = -1;
	KASSERT(j < fdp->fd_nfiles);
	for (i = 0; i <= j; i++, ffp++, *nffp++ = ff2) {
		ff = *ffp;
		/* Install built-in fdfiles even if unused here. */
		if (i < NDFDFILE) {
			ff2 = (fdfile_t *)newfdp->fd_dfdfile[i];
		} else {
			ff2 = NULL;
		}
		/* Determine if descriptor is active in parent. */
		if (ff == NULL || !fd_isused(fdp, i)) {
			KASSERT(ff != NULL || i >= NDFDFILE);
			continue;
		}
		mutex_enter(&ff->ff_lock);
		fp = ff->ff_file;
		if (fp == NULL) {
			/* Descriptor is half-open: free slot. */
			fd_zap(newfdp, i);
			mutex_exit(&ff->ff_lock);
			continue;
		}
		if (fp->f_type == DTYPE_KQUEUE) {
			/* kqueue descriptors cannot be copied. */
			fd_zap(newfdp, i);
			mutex_exit(&ff->ff_lock);
			continue;
		}
		/* It's active: add a reference to the file. */
		mutex_enter(&fp->f_lock);
		fp->f_count++;
		mutex_exit(&fp->f_lock);
		/* Consume one fdfile_t to represent it. */
		if (i >= NDFDFILE) {
			ff2 = fflist;
			fflist = (void *)ff2->ff_file;
		}
		ff2->ff_file = fp;
		ff2->ff_exclose = ff->ff_exclose;
		ff2->ff_allocated = true;
		mutex_exit(&ff->ff_lock);
		if (i > newlast) {
			newlast = i;
		}
	}
	mutex_exit(&fdp->fd_lock);

	/* Discard unused fdfile_t structures. */
	while (__predict_false(fflist != NULL)) {
		ff = fflist;
		fflist = (void *)ff->ff_file;
		ff->ff_file = NULL;
		pool_cache_put(fdfile_cache, ff);
		nused--;
	}
	KASSERT(nused >= 0);
	KASSERT(newfdp->fd_ofiles[0] == (fdfile_t *)newfdp->fd_dfdfile[0]);

	newfdp->fd_nused = nused;
	newfdp->fd_lastfile = newlast;

	return (newfdp);
}

/*
 * Release a filedesc structure.
 */
void
fd_free(void)
{
	filedesc_t *fdp;
	fdfile_t *ff;
	file_t *fp;
	int fd, lastfd;
	void **discard;

	fdp = curlwp->l_fd;

	KASSERT(fdp->fd_ofiles[0] == (fdfile_t *)fdp->fd_dfdfile[0]);

	if (atomic_dec_uint_nv(&fdp->fd_refcnt) > 0)
		return;

	/*
	 * Close any files that the process holds open.
	 */
	for (fd = 0, lastfd = fdp->fd_nfiles - 1; fd <= lastfd; fd++) {
		ff = fdp->fd_ofiles[fd];
		KASSERT(fd >= NDFDFILE ||
		    ff == (fdfile_t *)fdp->fd_dfdfile[fd]);
		if ((ff = fdp->fd_ofiles[fd]) == NULL)
			continue;
		if ((fp = ff->ff_file) != NULL) {
			/*
			 * Must use fd_close() here as kqueue holds
			 * long term references to descriptors.
			 */
			ff->ff_refcnt++;
			fd_close(fd);
		}
		KASSERT(ff->ff_refcnt == 0);
		KASSERT(ff->ff_file == NULL);
		KASSERT(!ff->ff_exclose);
		KASSERT(!ff->ff_allocated);
		if (fd >= NDFDFILE) {
			pool_cache_put(fdfile_cache, ff);
		}
	}

	/*
	 * Clean out the descriptor table for the next user and return
	 * to the cache.
	 */
	while ((discard = fdp->fd_discard) != NULL) {
		fdp->fd_discard = discard[0];
		kmem_free(discard, (uintptr_t)discard[1]);
	}
	if (NDHISLOTS(fdp->fd_nfiles) > NDHISLOTS(NDFILE)) {
		KASSERT(fdp->fd_himap != fdp->fd_dhimap);
		KASSERT(fdp->fd_lomap != fdp->fd_dlomap);
		fd_map_free(fdp->fd_nfiles, fdp->fd_lomap, fdp->fd_himap);
	}
	if (fdp->fd_nfiles > NDFILE) {
		KASSERT(fdp->fd_ofiles != fdp->fd_dfiles);
		fd_ofile_free(fdp->fd_nfiles, fdp->fd_ofiles);
	}
	if (fdp->fd_knhash != NULL) {
		hashdone(fdp->fd_knhash, HASH_LIST, fdp->fd_knhashmask);
		fdp->fd_knhash = NULL;
		fdp->fd_knhashmask = 0;
	} else {
		KASSERT(fdp->fd_knhashmask == 0);
	}
	fdp->fd_lastkqfile = -1;
	pool_cache_put(filedesc_cache, fdp);
}

/*
 * File Descriptor pseudo-device driver (/dev/fd/).
 *
 * Opening minor device N dup()s the file (if any) connected to file
 * descriptor N belonging to the calling process.  Note that this driver
 * consists of only the ``open()'' routine, because all subsequent
 * references to this file will be direct to the other driver.
 */
static int
filedescopen(dev_t dev, int mode, int type, lwp_t *l)
{

	/*
	 * XXX Kludge: set dupfd to contain the value of the
	 * the file descriptor being sought for duplication. The error
	 * return ensures that the vnode for this device will be released
	 * by vn_open. Open will detect this special error and take the
	 * actions in dupfdopen below. Other callers of vn_open or VOP_OPEN
	 * will simply report the error.
	 */
	l->l_dupfd = minor(dev);	/* XXX */
	return EDUPFD;
}

/*
 * Duplicate the specified descriptor to a free descriptor.
 */
int
fd_dupopen(int old, int *new, int mode, int error)
{
	filedesc_t *fdp;
	fdfile_t *ff;
	file_t *fp;

	if ((fp = fd_getfile(old)) == NULL) {
		return EBADF;
	}
	fdp = curlwp->l_fd;
	ff = fdp->fd_ofiles[old];

	/*
	 * There are two cases of interest here.
	 *
	 * For EDUPFD simply dup (dfd) to file descriptor
	 * (indx) and return.
	 *
	 * For EMOVEFD steal away the file structure from (dfd) and
	 * store it in (indx).  (dfd) is effectively closed by
	 * this operation.
	 *
	 * Any other error code is just returned.
	 */
	switch (error) {
	case EDUPFD:
		/*
		 * Check that the mode the file is being opened for is a
		 * subset of the mode of the existing descriptor.
		 */
		if (((mode & (FREAD|FWRITE)) | fp->f_flag) != fp->f_flag) {
			error = EACCES;
			break;
		}

		/* Copy it. */
		error = fd_dup(fp, 0, new, fdp->fd_ofiles[old]->ff_exclose);
		break;

	case EMOVEFD:
		/* Copy it. */
		error = fd_dup(fp, 0, new, fdp->fd_ofiles[old]->ff_exclose);
		if (error != 0) {
			break;
		}

		/* Steal away the file pointer from 'old'. */
		(void)fd_close(old);
		return 0;
	}

	fd_putfile(old);
	return error;
}

/*
 * Sets descriptor owner. If the owner is a process, 'pgid'
 * is set to positive value, process ID. If the owner is process group,
 * 'pgid' is set to -pg_id.
 */
int
fsetown(pid_t *pgid, u_long cmd, const void *data)
{
	int id = *(const int *)data;
	int error;

	switch (cmd) {
	case TIOCSPGRP:
		if (id < 0)
			return (EINVAL);
		id = -id;
		break;
	default:
		break;
	}

	if (id > 0 && !pfind(id))
		return (ESRCH);
	else if (id < 0 && (error = pgid_in_session(curproc, -id)))
		return (error);

	*pgid = id;
	return (0);
}

/*
 * Return descriptor owner information. If the value is positive,
 * it's process ID. If it's negative, it's process group ID and
 * needs the sign removed before use.
 */
int
fgetown(pid_t pgid, u_long cmd, void *data)
{

	switch (cmd) {
	case TIOCGPGRP:
		*(int *)data = -pgid;
		break;
	default:
		*(int *)data = pgid;
		break;
	}
	return (0);
}

/*
 * Send signal to descriptor owner, either process or process group.
 */
void
fownsignal(pid_t pgid, int signo, int code, int band, void *fdescdata)
{
	ksiginfo_t ksi;

	KASSERT(!cpu_intr_p());

	if (pgid == 0) {
		return;
	}

	KSI_INIT(&ksi);
	ksi.ksi_signo = signo;
	ksi.ksi_code = code;
	ksi.ksi_band = band;

	mutex_enter(proc_lock);
	if (pgid > 0) {
		struct proc *p1;

		p1 = p_find(pgid, PFIND_LOCKED);
		if (p1 != NULL) {
			kpsignal(p1, &ksi, fdescdata);
		}
	} else {
		struct pgrp *pgrp;

		KASSERT(pgid < 0);
		pgrp = pg_find(-pgid, PFIND_LOCKED);
		if (pgrp != NULL) {
			kpgsignal(pgrp, &ksi, fdescdata, 0);
		}
	}
	mutex_exit(proc_lock);
}

int
fd_clone(file_t *fp, unsigned fd, int flag, const struct fileops *fops,
	 void *data)
{

	fp->f_flag = flag;
	fp->f_type = DTYPE_MISC;
	fp->f_ops = fops;
	fp->f_data = data;
	curlwp->l_dupfd = fd;
	fd_affix(curproc, fp, fd);

	return EMOVEFD;
}

int
fnullop_fcntl(file_t *fp, u_int cmd, void *data)
{

	if (cmd == F_SETFL)
		return 0;

	return EOPNOTSUPP;
}

int
fnullop_poll(file_t *fp, int which)
{

	return 0;
}

int
fnullop_kqfilter(file_t *fp, struct knote *kn)
{

	return 0;
}

void
fnullop_drain(file_t *fp)
{

}

int
fbadop_read(file_t *fp, off_t *offset, struct uio *uio,
	    kauth_cred_t cred, int flags)
{

	return EOPNOTSUPP;
}

int
fbadop_write(file_t *fp, off_t *offset, struct uio *uio,
	     kauth_cred_t cred, int flags)
{

	return EOPNOTSUPP;
}

int
fbadop_ioctl(file_t *fp, u_long com, void *data)
{

	return EOPNOTSUPP;
}

int
fbadop_stat(file_t *fp, struct stat *sb)
{

	return EOPNOTSUPP;
}

int
fbadop_close(file_t *fp)
{

	return EOPNOTSUPP;
}
