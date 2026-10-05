/*	$NetBSD: sys_pipe.c,v 1.182 2026/10/05 21:01:22 riastradh Exp $	*/

/*-
 * Copyright (c) 2003, 2007, 2008, 2009, 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg, and by Andrew Doran.
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
 * Copyright (c) 1996 John S. Dyson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Absolutely no warranty of function or purpose is made by the author
 *    John S. Dyson.
 * 4. Modifications may be freely made to this file if the above conditions
 *    are met.
 */

/*
 * This file contains a high-performance replacement for the socket-based
 * pipes scheme originally used.  It does not support all features of
 * sockets, but does do everything that pipes normally do.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_pipe.c,v 1.182 2026/10/05 21:01:22 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/filio.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/pipe.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/sdt.h>
#include <sys/select.h>
#include <sys/signalvar.h>
#include <sys/stat.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/ttycom.h>
#include <sys/uio.h>
#include <sys/vnode.h>

static int	pipe_read(file_t *, off_t *, struct uio *, kauth_cred_t, int);
static int	pipe_write(file_t *, off_t *, struct uio *, kauth_cred_t, int);
static int	pipe_close(file_t *);
static int	pipe_poll(file_t *, int);
static int	pipe_kqfilter(file_t *, struct knote *);
static int	pipe_stat(file_t *, struct stat *);
static int	pipe_ioctl(file_t *, u_long, void *);
static void	pipe_restart(file_t *);
static int	pipe_fpathconf(file_t *, int, register_t *);
static int	pipe_posix_fadvise(file_t *, off_t, off_t, int);

static const struct fileops pipeops = {
	.fo_name = "pipe",
	.fo_read = pipe_read,
	.fo_write = pipe_write,
	.fo_ioctl = pipe_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = pipe_poll,
	.fo_stat = pipe_stat,
	.fo_close = pipe_close,
	.fo_kqfilter = pipe_kqfilter,
	.fo_restart = pipe_restart,
	.fo_fpathconf = pipe_fpathconf,
	.fo_posix_fadvise = pipe_posix_fadvise,
};

/*
 * Default pipe buffer size(s), this can be kind-of large now because pipe
 * space is pageable.  The pipe code will try to maintain locality of
 * reference for performance reasons, so small amounts of outstanding I/O
 * will not wipe the cache.
 */
#define	MINPIPESIZE	(PIPE_SIZE / 3)
#define	MAXPIPESIZE	(2 * PIPE_SIZE / 3)

/*
 * Limit the number of "big" pipes
 */
#define	LIMITBIGPIPES	32
static u_int	maxbigpipes __read_mostly = LIMITBIGPIPES;
static u_int	nbigpipe = 0;

/*
 * Amount of KVA consumed by pipe buffers.
 */
static u_int	amountpipekva = 0;

static void	pipeclose(struct file *, struct pipe *);
static void	pipefree(struct pipe *);
static void	pipe_free_kmem(struct pipe *);
static int	pipe_create(struct pipe **, pool_cache_t, struct timespec *);
static int	pipelock(struct pipe *, bool);
static inline void pipeunlock(struct pipe *);
static void	pipeselwakeup(struct pipe *, int);
static int	pipespace(struct pipe *, int);
static int	pipe_ctor(void *, void *, int);
static void	pipe_dtor(void *, void *);

static pool_cache_t	pipe_wr_cache;
static pool_cache_t	pipe_rd_cache;

void
pipe_init(void)
{

	/* Writer side is not automatically allocated KVA. */
	pipe_wr_cache = pool_cache_init(sizeof(struct pipe), 0, 0, 0, "pipewr",
	    NULL, IPL_NONE, pipe_ctor, pipe_dtor, NULL);
	KASSERT(pipe_wr_cache != NULL);

	/* Reader side gets preallocated KVA. */
	pipe_rd_cache = pool_cache_init(sizeof(struct pipe), 0, 0, 0, "piperd",
	    NULL, IPL_NONE, pipe_ctor, pipe_dtor, (void *)1);
	KASSERT(pipe_rd_cache != NULL);
}

static int
pipe_ctor(void *arg, void *obj, int flags)
{
	struct pipe *pipe;
	vaddr_t va;

	pipe = obj;

	memset(pipe, 0, sizeof(struct pipe));
	if (arg != NULL) {
		/* Preallocate space. */
		va = uvm_km_alloc(kernel_map, PIPE_SIZE, 0,
		    UVM_KMF_PAGEABLE | UVM_KMF_WAITVA);
		KASSERT(va != 0);
		pipe->pipe_kmem = va;
		atomic_add_int(&amountpipekva, PIPE_SIZE);
	}
	cv_init(&pipe->pipe_rcv, "pipe_rd");
	cv_init(&pipe->pipe_wcv, "pipe_wr");
	cv_init(&pipe->pipe_draincv, "pipe_drn");
	cv_init(&pipe->pipe_lkcv, "pipe_lk");
	selinit(&pipe->pipe_sel);
	pipe->pipe_state = PIPE_SIGNALR;

	return 0;
}

static void
pipe_dtor(void *arg, void *obj)
{
	struct pipe *pipe;

	pipe = obj;

	cv_destroy(&pipe->pipe_rcv);
	cv_destroy(&pipe->pipe_wcv);
	cv_destroy(&pipe->pipe_draincv);
	cv_destroy(&pipe->pipe_lkcv);
	seldestroy(&pipe->pipe_sel);
	if (pipe->pipe_kmem != 0) {
		uvm_km_free(kernel_map, pipe->pipe_kmem, PIPE_SIZE,
		    UVM_KMF_PAGEABLE);
		atomic_add_int(&amountpipekva, -PIPE_SIZE);
	}
}

/*
 * The pipe system call for the DTYPE_PIPE type of pipes
 */
int
pipe1(struct lwp *l, int *fildes, int flags)
{
	struct pipe *rpipe, *wpipe;
	struct timespec nt;
	file_t *rf, *wf;
	int fd, error;
	proc_t *p;

	if (flags & ~(O_CLOEXEC|O_CLOFORK|O_NONBLOCK|O_NOSIGPIPE))
		return SET_ERROR(EINVAL);
	p = curproc;
	rpipe = wpipe = NULL;
	getnanotime(&nt);
	if ((error = pipe_create(&rpipe, pipe_rd_cache, &nt)) ||
	    (error = pipe_create(&wpipe, pipe_wr_cache, &nt))) {
		goto free2;
	}
	rpipe->pipe_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	wpipe->pipe_lock = rpipe->pipe_lock;
	mutex_obj_hold(wpipe->pipe_lock);

	error = fd_allocfile(&rf, &fd);
	if (error)
		goto free2;
	fildes[0] = fd;

	error = fd_allocfile(&wf, &fd);
	if (error)
		goto free3;
	fildes[1] = fd;

	rf->f_flag = FREAD | flags;
	rf->f_type = DTYPE_PIPE;
	rf->f_pipe = rpipe;
	rf->f_ops = &pipeops;
	fd_set_exclose(l, fildes[0], (flags & O_CLOEXEC) != 0);
	fd_set_foclose(l, fildes[0], (flags & O_CLOFORK) != 0);

	wf->f_flag = FWRITE | flags;
	wf->f_type = DTYPE_PIPE;
	wf->f_pipe = wpipe;
	wf->f_ops = &pipeops;
	fd_set_exclose(l, fildes[1], (flags & O_CLOEXEC) != 0);
	fd_set_foclose(l, fildes[1], (flags & O_CLOFORK) != 0);

	rpipe->pipe_peer = wpipe;
	wpipe->pipe_peer = rpipe;

	fd_affix(p, rf, fildes[0]);
	fd_affix(p, wf, fildes[1]);
	return 0;
free3:
	fd_abort(p, rf, fildes[0]);
free2:
	if (wpipe)
		pipefree(wpipe);
	if (rpipe)
		pipefree(rpipe);

	return error;
}

/*
 * Allocate kva for pipe circular buffer, the space is pageable
 * This routine will 'realloc' the size of a pipe safely, if it fails
 * it will retain the old buffer.
 * If it fails it will return ENOMEM.
 */
static int
pipespace(struct pipe *pipe, int size)
{
	void *buffer;

	/*
	 * Allocate pageable virtual address space.  Physical memory is
	 * allocated on demand.
	 */
	if (size == PIPE_SIZE && pipe->pipe_kmem != 0) {
		buffer = (void *)pipe->pipe_kmem;
	} else {
		buffer = (void *)uvm_km_alloc(kernel_map, round_page(size),
		    0, UVM_KMF_PAGEABLE);
		if (buffer == NULL)
			return SET_ERROR(ENOMEM);
		atomic_add_int(&amountpipekva, size);
	}

	/* free old resources if we're resizing */
	pipe_free_kmem(pipe);
	pipe->pipe_buffer.buffer = buffer;
	pipe->pipe_buffer.size = size;
	pipe->pipe_buffer.in = 0;
	pipe->pipe_buffer.out = 0;
	pipe->pipe_buffer.cnt = 0;
	return 0;
}

/*
 * Initialize and allocate VM and memory for pipe.
 */
static int
pipe_create(struct pipe **pipep, pool_cache_t cache, struct timespec *nt)
{
	struct pipe *pipe;
	int error;

	pipe = pool_cache_get(cache, PR_WAITOK);
	KASSERT(pipe != NULL);
	*pipep = pipe;
	error = 0;
	pipe->pipe_atime = pipe->pipe_mtime = pipe->pipe_btime = *nt;
	pipe->pipe_lock = NULL;
	if (cache == pipe_rd_cache) {
		error = pipespace(pipe, PIPE_SIZE);
	} else {
		pipe->pipe_buffer.buffer = NULL;
		pipe->pipe_buffer.size = 0;
		pipe->pipe_buffer.in = 0;
		pipe->pipe_buffer.out = 0;
		pipe->pipe_buffer.cnt = 0;
	}
	return error;
}

/*
 * Lock a pipe for I/O, blocking other access
 * Called with pipe spin lock held.
 */
static int
pipelock(struct pipe *pipe, bool catch_p)
{
	int error;

	KASSERT(mutex_owned(pipe->pipe_lock));

	while (pipe->pipe_state & PIPE_LOCKFL) {
		if (catch_p) {
			error = cv_wait_sig(&pipe->pipe_lkcv, pipe->pipe_lock);
			if (error != 0) {
				return error;
			}
		} else
			cv_wait(&pipe->pipe_lkcv, pipe->pipe_lock);
	}

	pipe->pipe_state |= PIPE_LOCKFL;

	return 0;
}

/*
 * unlock a pipe I/O lock
 */
static inline void
pipeunlock(struct pipe *pipe)
{

	KASSERT(pipe->pipe_state & PIPE_LOCKFL);

	pipe->pipe_state &= ~PIPE_LOCKFL;
	cv_signal(&pipe->pipe_lkcv);
}

/*
 * pipeselwakeup(pipe, code)
 *
 *	Activity has happened on pipe's peer oncausing I/O to be
 *	available on pipe, so:
 *
 *	1. Wake any threads waiting in select/poll on pipe.
 *
 *	2. Deliver SIGIO to any process (group) configured to receive
 *	   notifications about I/O on pipe.
 *
 *	`code' is a siginfo_t si_code value in the POLL_* namespace for
 *	the type of notification the waiters will receive, and it
 *	should match the direction of the pipe -- POLL_OUT/POLL_ERR
 *	with the writer side, POLL_IN/POLL_HUP with the reader side.
 */
static void
pipeselwakeup(struct pipe *pipe, int code)
{
	int band;

	KASSERT(mutex_owned(pipe->pipe_lock));

	switch (code) {
	case POLL_IN:
		band = POLLIN|POLLRDNORM;
		break;
	case POLL_OUT:
		band = POLLOUT|POLLWRNORM;
		break;
	case POLL_HUP:
		band = POLLHUP;
		break;
	case POLL_ERR:
		band = POLLERR;
		break;
	default:
		band = 0;
#ifdef DIAGNOSTIC
		printf("bad siginfo code %d in pipe notification.\n", code);
#endif
		break;
	}

	selnotify(&pipe->pipe_sel, band, NOTE_SUBMIT);

	if ((pipe->pipe_state & PIPE_ASYNC) == 0)
		return;

	fownsignal(pipe->pipe_pgid, SIGIO, code, band, pipe);
}

static int
pipe_read(file_t *fp, off_t *offset, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct pipe *rpipe = fp->f_pipe;
	struct pipe *wpipe;
	struct pipebuf *bp = &rpipe->pipe_buffer;
	kmutex_t *lock = rpipe->pipe_lock;
	int error;
	size_t nread = 0;
	size_t size;
	size_t ocnt;
	unsigned int wakeup_state = 0;

	/*
	 * Try to avoid locking the pipe if we have nothing to do.
	 *
	 * There are programs which share one pipe amongst multiple processes
	 * and perform non-blocking reads in parallel, even if the pipe is
	 * empty.  This in particular is the case with BSD make, which when
	 * spawned with a high -j number can find itself with over half of the
	 * calls failing to find anything.
	 */
	if ((fp->f_flag & FNONBLOCK) != 0) {
		if (__predict_false(uio->uio_resid == 0))
			return 0;
		if (atomic_load_relaxed(&bp->cnt) == 0 &&
		    (atomic_load_relaxed(&rpipe->pipe_state) & PIPE_EOF) == 0)
			return SET_ERROR(EAGAIN);
	}

	mutex_enter(lock);
	++rpipe->pipe_busy;
	ocnt = bp->cnt;

again:
	error = pipelock(rpipe, true);
	if (error)
		goto unlocked_error;

	while (uio->uio_resid) {
		/*
		 * Normal pipe buffer receive.
		 */
		if (bp->cnt > 0) {
			size = bp->size - bp->out;
			if (size > bp->cnt)
				size = bp->cnt;
			if (size > uio->uio_resid)
				size = uio->uio_resid;

			mutex_exit(lock);
			error = uiomove((char *)bp->buffer + bp->out, size, uio);
			mutex_enter(lock);
			if (error)
				break;

			bp->out += size;
			if (bp->out >= bp->size)
				bp->out = 0;

			bp->cnt -= size;

			/*
			 * If there is no more to read in the pipe, reset
			 * its pointers to the beginning.  This improves
			 * cache hit stats.
			 */
			if (bp->cnt == 0) {
				bp->in = 0;
				bp->out = 0;
			}
			nread += size;
			continue;
		}

		/*
		 * Break if some data was read.
		 */
		if (nread > 0)
			break;

		/*
		 * Detect EOF condition.
		 * Read returns 0 on EOF, no need to set error.
		 *
		 * XXX Why rpipe->pipe_state and not wpipe->pipe_state?
		 * XXX Distinguish reader-closed from writer-closed?
		 */
		if (rpipe->pipe_state & PIPE_EOF)
			break;

		/*
		 * Don't block on non-blocking I/O.
		 */
		if (fp->f_flag & FNONBLOCK) {
			error = SET_ERROR(EAGAIN);
			break;
		}

		/*
		 * Unlock the pipe buffer for our remaining processing.
		 * We will either break out with an error or we will
		 * sleep and relock to loop.
		 */
		pipeunlock(rpipe);

		/*
		 * If the "write-side" is blocked, wake it up now.
		 */
		wpipe = rpipe->pipe_peer;
		if (wpipe != NULL) {
			pipeselwakeup(wpipe, POLL_OUT);
			cv_broadcast(&wpipe->pipe_wcv);
		}

		if (wakeup_state & PIPE_RESTART) {
			error = SET_ERROR(ERESTART);
			goto unlocked_error;
		}

		/* Now wait until the pipe is filled */
		error = cv_wait_sig(&rpipe->pipe_rcv, lock);
		if (error != 0)
			goto unlocked_error;
		wakeup_state = rpipe->pipe_state;
		goto again;
	}

	if (error == 0)
		getnanotime(&rpipe->pipe_atime);
	pipeunlock(rpipe);

unlocked_error:
	--rpipe->pipe_busy;
	if (rpipe->pipe_busy == 0) {
		cv_broadcast(&rpipe->pipe_draincv);
	}
	if (bp->cnt < MINPIPESIZE) {
		if ((wpipe = rpipe->pipe_peer) != NULL)
			cv_broadcast(&wpipe->pipe_wcv);
	}

	/*
	 * If anything was read off the buffer, signal to the writer it's
	 * possible to write more data. Also send signal if we are here for the
	 * first time after last write.
	 */
	if ((bp->size - bp->cnt) >= PIPE_BUF
	    && (ocnt != bp->cnt || (rpipe->pipe_state & PIPE_SIGNALR))) {
		if ((wpipe = rpipe->pipe_peer) != NULL)
			pipeselwakeup(wpipe, POLL_OUT);
		rpipe->pipe_state &= ~PIPE_SIGNALR;
	}

	mutex_exit(lock);
	return error;
}

static int
pipe_write(file_t *fp, off_t *offset, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct pipe *wpipe, *rpipe;
	struct pipebuf *bp;
	kmutex_t *lock;
	int error;
	unsigned int wakeup_state = 0;

	/* We want to write to our peer */
	wpipe = fp->f_pipe;
	lock = wpipe->pipe_lock;
	error = 0;

	mutex_enter(lock);
	rpipe = wpipe->pipe_peer;

	/*
	 * Detect loss of pipe read side, issue SIGPIPE if lost.
	 */
	if (rpipe == NULL || (rpipe->pipe_state & PIPE_EOF) != 0) {
		mutex_exit(lock);
		return SET_ERROR(EPIPE);
	}
	++wpipe->pipe_busy;

	/* Acquire the long-term pipe lock */
	if ((error = pipelock(rpipe, true)) != 0) {
		--wpipe->pipe_busy;
		if (wpipe->pipe_busy == 0) {
			cv_broadcast(&wpipe->pipe_draincv);
		}
		mutex_exit(lock);
		return error;
	}

	bp = &rpipe->pipe_buffer;

	/*
	 * If it is advantageous to resize the pipe buffer, do so.
	 */
	if ((uio->uio_resid > PIPE_SIZE) &&
	    (nbigpipe < maxbigpipes) &&
	    (bp->size <= PIPE_SIZE) && (bp->cnt == 0)) {

		if (pipespace(rpipe, BIG_PIPE_SIZE) == 0)
			atomic_inc_uint(&nbigpipe);
	}

	while (uio->uio_resid) {
		size_t space;

		space = bp->size - bp->cnt;

		/* Writes of size <= PIPE_BUF must be atomic. */
		if ((space < uio->uio_resid) && (uio->uio_resid <= PIPE_BUF))
			space = 0;

		if (space > 0) {
			int size;	/* Transfer size */
			int segsize;	/* first segment to transfer */

			/*
			 * Transfer size is minimum of uio transfer
			 * and free space in pipe buffer.
			 */
			if (space > uio->uio_resid)
				size = uio->uio_resid;
			else
				size = space;
			/*
			 * First segment to transfer is minimum of
			 * transfer size and contiguous space in
			 * pipe buffer.  If first segment to transfer
			 * is less than the transfer size, we've got
			 * a wraparound in the buffer.
			 */
			segsize = bp->size - bp->in;
			if (segsize > size)
				segsize = size;

			/* Transfer first segment */
			mutex_exit(lock);
			error = uiomove((char *)bp->buffer + bp->in, segsize,
			    uio);

			if (error == 0 && segsize < size) {
				/*
				 * Transfer remaining part now, to
				 * support atomic writes.  Wraparound
				 * happened.
				 */
				KASSERT(bp->in + segsize == bp->size);
				error = uiomove(bp->buffer,
				    size - segsize, uio);
			}
			mutex_enter(lock);
			if (error)
				break;

			bp->in += size;
			if (bp->in >= bp->size) {
				KASSERT(bp->in == size - segsize + bp->size);
				bp->in = size - segsize;
			}

			bp->cnt += size;
			KASSERT(bp->cnt <= bp->size);
			wakeup_state = 0;
		} else {
			/*
			 * If the "read-side" has been blocked, wake it up now.
			 */
			cv_broadcast(&rpipe->pipe_rcv);

			/*
			 * Don't block on non-blocking I/O.
			 */
			if (fp->f_flag & FNONBLOCK) {
				error = SET_ERROR(EAGAIN);
				break;
			}

			/*
			 * We have no more space and have something to offer,
			 * wake up select/poll.
			 */
			if (bp->cnt)
				pipeselwakeup(rpipe, POLL_IN);

			if (wakeup_state & PIPE_RESTART) {
				error = SET_ERROR(ERESTART);
				break;
			}

			/*
			 * If read side wants to go away, we just issue a signal
			 * to ourselves.
			 *
			 * XXX Shouldn't this happen before we uiomove anything?
			 *
			 * XXX Why rpipe->pipe_state and not wpipe->pipe_state?
			 * XXX Distinguish reader-closed from writer-closed?
			 */
			if (rpipe->pipe_state & PIPE_EOF) {
				error = SET_ERROR(EPIPE);
				break;
			}

			pipeunlock(rpipe);
			error = cv_wait_sig(&wpipe->pipe_wcv, lock);
			(void)pipelock(rpipe, false);
			if (error != 0)
				break;
			wakeup_state = wpipe->pipe_state;
		}
	}

	--wpipe->pipe_busy;
	if (wpipe->pipe_busy == 0) {
		cv_broadcast(&wpipe->pipe_draincv);
	}
	if (bp->cnt > 0) {
		cv_broadcast(&rpipe->pipe_rcv);
	}

	/*
	 * Don't return EPIPE if I/O was successful
	 *
	 * XXX Shouldn't we avoid returning _any_ error if we
	 * transmitted _any_ positive number of bytes?  Or does that
	 * happen downstream of here, and if so, why do we need to do
	 * that here?
	 */
	if (error == EPIPE && bp->cnt == 0 && uio->uio_resid == 0)
		error = 0;

	if (error == 0)
		getnanotime(&rpipe->pipe_mtime);

	/*
	 * We have something to offer, wake up select/poll.
	 */
	if (bp->cnt)
		pipeselwakeup(rpipe, POLL_IN);

	/*
	 * Arrange for next read(2) to do a signal.
	 */
	rpipe->pipe_state |= PIPE_SIGNALR;

	pipeunlock(rpipe);
	mutex_exit(lock);
	return error;
}

/*
 * We implement a very minimal set of ioctls for compatibility with sockets.
 */
int
pipe_ioctl(file_t *fp, u_long cmd, void *data)
{
	struct pipe *pipe = fp->f_pipe;
	kmutex_t *lock = pipe->pipe_lock;

	switch (cmd) {

	case FIONBIO:
		return 0;

	case FIOASYNC:
		mutex_enter(lock);
		if (*(int *)data) {
			pipe->pipe_state |= PIPE_ASYNC;
		} else {
			pipe->pipe_state &= ~PIPE_ASYNC;
		}
		mutex_exit(lock);
		return 0;

	case FIONREAD:
		mutex_enter(lock);
		*(int *)data = pipe->pipe_buffer.cnt;
		mutex_exit(lock);
		return 0;

	case FIONWRITE:
		/* Look at other side */
		mutex_enter(lock);
		pipe = pipe->pipe_peer;
		if (pipe == NULL)
			*(int *)data = 0;
		else
			*(int *)data = pipe->pipe_buffer.cnt;
		mutex_exit(lock);
		return 0;

	case FIONSPACE:
		/* Look at other side */
		mutex_enter(lock);
		pipe = pipe->pipe_peer;
		if (pipe == NULL)
			*(int *)data = 0;
		else
			*(int *)data = pipe->pipe_buffer.size -
			    pipe->pipe_buffer.cnt;
		mutex_exit(lock);
		return 0;

	case TIOCSPGRP:
	case FIOSETOWN:
		return fsetown(&pipe->pipe_pgid, cmd, data);

	case TIOCGPGRP:
	case FIOGETOWN:
		return fgetown(pipe->pipe_pgid, cmd, data);

	}
	return EPASSTHROUGH;
}

int
pipe_poll(file_t *fp, int events)
{
	struct pipe *pipe = fp->f_pipe;
	struct pipe *ppipe;
	int revents = 0;

	mutex_enter(pipe->pipe_lock);
	ppipe = pipe->pipe_peer;

	if (fp->f_flag & FREAD) {
		struct pipe *rpipe = pipe;

		/*
		 * If the writer has been closed, then we can always
		 * read (possibly returning EOF) without blocking, so
		 * set POLLIN|POLLRDNORM if requested, and set POLLHUP
		 * unsolicited to notify reader of the fact.
		 *
		 * Otherwise, we can only read without blocking if
		 * there are bytes in the buffer.
		 */
		if (rpipe->pipe_state & PIPE_EOF) {
			revents |= events & (POLLIN | POLLRDNORM);
			revents |= POLLHUP;
		} else if (rpipe->pipe_buffer.cnt > 0) {
			revents |= events & (POLLIN | POLLRDNORM);
		}
	} else if (fp->f_flag & FWRITE) {
		struct pipe *wpipe = pipe;
		struct pipe *rpipe = ppipe;

		/*
		 * If the reader has been closed, then any writes will
		 * immediately fail with EPIPE, so report
		 * POLLOUT|POLLWRNORM if requested and POLLERR
		 * unsolicited.
		 *
		 * Otherwise, we can only write without blocking if
		 * there are at least PIPE_BUF bytes free in the
		 * buffer.
		 */
		if (rpipe == NULL || (wpipe->pipe_state & PIPE_EOF) != 0) {
			revents |= events & (POLLOUT | POLLWRNORM);
			revents |= POLLERR;
		} else if (rpipe->pipe_buffer.size - rpipe->pipe_buffer.cnt >=
		    PIPE_BUF) {
			revents |= events & (POLLOUT | POLLWRNORM);
		}
	} else {
		panic("file %p pipe %p invalid direction flag 0x%x",
		    fp, pipe, fp->f_flag);
	}

	if (revents == 0)
		selrecord(curlwp, &pipe->pipe_sel);
	mutex_exit(pipe->pipe_lock);

	return revents;
}

static int
pipe_stat(file_t *fp, struct stat *ub)
{
	struct pipe *pipe = fp->f_pipe;

	mutex_enter(pipe->pipe_lock);
	memset(ub, 0, sizeof(*ub));
	ub->st_mode = S_IFIFO | S_IRUSR | S_IWUSR;
	ub->st_blksize = pipe->pipe_buffer.size;
	if (ub->st_blksize == 0 && pipe->pipe_peer)
		ub->st_blksize = pipe->pipe_peer->pipe_buffer.size;
	ub->st_size = pipe->pipe_buffer.cnt;
	ub->st_blocks = (ub->st_size) ? 1 : 0;
	ub->st_atimespec = pipe->pipe_atime;
	ub->st_mtimespec = pipe->pipe_mtime;
	ub->st_ctimespec = ub->st_birthtimespec = pipe->pipe_btime;
	ub->st_uid = kauth_cred_geteuid(fp->f_cred);
	ub->st_gid = kauth_cred_getegid(fp->f_cred);

	/*
	 * Left as 0: st_dev, st_ino, st_nlink, st_rdev, st_flags, st_gen.
	 * XXX (st_dev, st_ino) should be unique.
	 */
	mutex_exit(pipe->pipe_lock);
	return 0;
}

static int
pipe_close(file_t *fp)
{
	struct pipe *pipe = fp->f_pipe;

	fp->f_pipe = NULL;
	pipeclose(fp, pipe);
	return 0;
}

static void
pipe_restart(file_t *fp)
{
	struct pipe *pipe = fp->f_pipe;

	/*
	 * Unblock blocked reads/writes in order to allow close() to complete.
	 * System calls return ERESTART so that the fd is revalidated.
	 * (Partial writes return the transfer length.)
	 */
	mutex_enter(pipe->pipe_lock);
	pipe->pipe_state |= PIPE_RESTART;
	/*
	 * At most one of these is in use at any time, depending on
	 * whether fp->f_flag has FREAD or FWRITE set, but there's no
	 * harm in waking both here.
	 */
	cv_broadcast(&pipe->pipe_rcv);
	cv_broadcast(&pipe->pipe_wcv);
	mutex_exit(pipe->pipe_lock);
}

static int
pipe_fpathconf(struct file *fp, int name, register_t *retval)
{

	switch (name) {
	case _PC_PIPE_BUF:
		*retval = PIPE_BUF;
		return 0;
	default:
		return SET_ERROR(EINVAL);
	}
}

static int
pipe_posix_fadvise(struct file *fp, off_t offset, off_t len, int advice)
{

	return SET_ERROR(ESPIPE);
}

static void
pipe_free_kmem(struct pipe *pipe)
{

	if (pipe->pipe_buffer.buffer != NULL) {
		if (pipe->pipe_buffer.size > PIPE_SIZE) {
			atomic_dec_uint(&nbigpipe);
		}
		if (pipe->pipe_buffer.buffer != (void *)pipe->pipe_kmem) {
			uvm_km_free(kernel_map,
			    (vaddr_t)pipe->pipe_buffer.buffer,
			    pipe->pipe_buffer.size, UVM_KMF_PAGEABLE);
			atomic_add_int(&amountpipekva,
			    -pipe->pipe_buffer.size);
		}
		pipe->pipe_buffer.buffer = NULL;
	}
}

/*
 * Shutdown the pipe.
 */
static void
pipeclose(struct file *fp, struct pipe *pipe)
{
	kmutex_t *lock;
	struct pipe *ppipe;

	KASSERT(cv_is_valid(&pipe->pipe_rcv));
	KASSERT(cv_is_valid(&pipe->pipe_wcv));
	KASSERT(cv_is_valid(&pipe->pipe_draincv));
	KASSERT(cv_is_valid(&pipe->pipe_lkcv));

	lock = pipe->pipe_lock;
	KASSERT(lock != NULL);

	mutex_enter(lock);

	/*
	 * fd_close has issued .fo_restart to wake all waiters on this
	 * side of the pipe, blocked new references, and waited for all
	 * references to drain, so it should not be possible for there
	 * to be any waiters remaining.  (Only one of the condvars was
	 * ever in use anyway depending on whether this is the reader
	 * side or the writer side of the pipe.)
	 */
	KASSERT(!cv_has_waiters(&pipe->pipe_rcv));
	KASSERT(!cv_has_waiters(&pipe->pipe_wcv));

	/*
	 * There may, however, be threads waiting in select/poll for
	 * I/O to be ready on this side of the pipe.  Wake them (but
	 * don't send SIGIO as pipeselwakeup does) so they can fail
	 * with EBADF/POLLNVAL.
	 */
	selnotify(&pipe->pipe_sel, 0, NOTE_SUBMIT);

	/*
	 * If the other side is busy, wake it up saying that
	 * we want to close it down, which will prevent peers
	 * from starting new I/O.  Once it is no longer busy,
	 * disconnect it.
	 */
	KASSERT(pipe->pipe_peer != NULL || (pipe->pipe_state & PIPE_EOF) != 0);
	pipe->pipe_state |= PIPE_EOF;
	if ((ppipe = pipe->pipe_peer) != NULL) {
		if (fp->f_flag & FREAD) {
			struct pipe *wpipe = ppipe;

			pipeselwakeup(wpipe, POLL_ERR);
		} else if (fp->f_flag & FWRITE) {
			struct pipe *rpipe = ppipe;

			pipeselwakeup(rpipe, POLL_HUP);
		} else {
			panic("file %p pipe %p invalid direction flag 0x%x",
			    fp, pipe, fp->f_flag);
		}

		ppipe->pipe_state |= PIPE_EOF;
		if (ppipe->pipe_busy) {
			cv_broadcast(&ppipe->pipe_rcv);
			cv_broadcast(&ppipe->pipe_wcv);
			while (ppipe->pipe_busy)
				cv_wait(&ppipe->pipe_draincv, lock);
		}
		ppipe->pipe_peer = NULL;
	}

	/*
	 * Any knote objects still left in the list are
	 * the one attached by peer.  Since no one will
	 * traverse this list, we just clear it.
	 *
	 * XXX Exposes select/kqueue internals.
	 */
	SLIST_INIT(&pipe->pipe_sel.sel_klist);

	KASSERT((pipe->pipe_state & PIPE_LOCKFL) == 0);
	mutex_exit(lock);

	/*
	 * Free resources.
	 */
	pipefree(pipe);
}

static void
pipefree(struct pipe *pipe)
{

	pipe->pipe_pgid = 0;
	pipe->pipe_state = PIPE_SIGNALR;
	pipe->pipe_peer = NULL;
	mutex_obj_free(pipe->pipe_lock);
	pipe->pipe_lock = NULL;
	pipe_free_kmem(pipe);
	if (pipe->pipe_kmem != 0) {
		pool_cache_put(pipe_rd_cache, pipe);
	} else {
		pool_cache_put(pipe_wr_cache, pipe);
	}
}

static void
filt_pipedetach(struct knote *kn)
{
	struct pipe *pipe;
	kmutex_t *lock;

	pipe = ((file_t *)kn->kn_obj)->f_pipe;
	lock = pipe->pipe_lock;

	mutex_enter(lock);
	KASSERT(kn->kn_hook == pipe);
	selremove_knote(&pipe->pipe_sel, kn);
	mutex_exit(lock);
}

static void
filt_pipenodetach(struct knote *kn)
{
	/* not attached, nothing to do */
}

static int
filt_pipewrongend(struct knote *kn, long hint)
{

	/* Never ready! */
	return 0;
}

static int
filt_piperead(struct knote *kn, long hint)
{
	struct pipe *rpipe = ((file_t *)kn->kn_obj)->f_pipe;
	struct pipe *wpipe;
	int rv;

	if ((hint & NOTE_SUBMIT) == 0) {
		mutex_enter(rpipe->pipe_lock);
	} else {
		KASSERT(mutex_owned(rpipe->pipe_lock));
	}
	wpipe = rpipe->pipe_peer;
	kn->kn_data = rpipe->pipe_buffer.cnt;

	if ((rpipe->pipe_state & PIPE_EOF) ||
	    (wpipe == NULL) || (wpipe->pipe_state & PIPE_EOF)) {
		knote_set_eof(kn, 0);
		rv = 1;
	} else {
		rv = kn->kn_data > 0;
	}

	if ((hint & NOTE_SUBMIT) == 0) {
		mutex_exit(rpipe->pipe_lock);
	} else {
		KASSERT(mutex_owned(rpipe->pipe_lock));
	}
	return rv;
}

static int
filt_pipewrite(struct knote *kn, long hint)
{
	struct pipe *wpipe = ((file_t *)kn->kn_obj)->f_pipe;
	struct pipe *rpipe;
	int rv;

	if ((hint & NOTE_SUBMIT) == 0) {
		mutex_enter(wpipe->pipe_lock);
	} else {
		KASSERT(mutex_owned(wpipe->pipe_lock));
	}
	rpipe = wpipe->pipe_peer;

	if ((rpipe == NULL) || (rpipe->pipe_state & PIPE_EOF)) {
		kn->kn_data = 0;
		knote_set_eof(kn, 0);
		rv = 1;
	} else {
		kn->kn_data = rpipe->pipe_buffer.size - rpipe->pipe_buffer.cnt;
		rv = kn->kn_data >= PIPE_BUF;
	}

	if ((hint & NOTE_SUBMIT) == 0) {
		mutex_exit(wpipe->pipe_lock);
	} else {
		KASSERT(mutex_owned(wpipe->pipe_lock));
	}
	return rv;
}

static const struct filterops pipe_rfiltops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = NULL,
	.f_detach = filt_pipedetach,
	.f_event = filt_piperead,
};

static const struct filterops pipe_wfiltops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = NULL,
	.f_detach = filt_pipedetach,
	.f_event = filt_pipewrite,
};

static int
pipe_kqfilter(file_t *fp, struct knote *kn)
{
	struct pipe *pipe;
	kmutex_t *lock;

	pipe = ((file_t *)kn->kn_obj)->f_pipe;
	lock = pipe->pipe_lock;

	mutex_enter(lock);

	switch (kn->kn_filter) {
	case EVFILT_READ:
		if ((fp->f_flag & FREAD) == 0) {
			kn->kn_fop = &pipe_wrongendfiltops;
			mutex_exit(lock);
			return 0;
		}
		kn->kn_fop = &pipe_rfiltops;
		break;
	case EVFILT_WRITE:
		if ((fp->f_flag & FWRITE) == 0) {
			kn->kn_fop = &pipe_wrongendfiltops;
			mutex_exit(lock);
			return 0;
		}
		kn->kn_fop = &pipe_wfiltops;
		break;
	default:
		mutex_exit(lock);
		return SET_ERROR(EINVAL);
	}

	kn->kn_hook = pipe;
	selrecord_knote(&pipe->pipe_sel, kn);
	mutex_exit(lock);

	return 0;
}

/*
 * Handle pipe sysctls.
 */
SYSCTL_SETUP(sysctl_kern_pipe_setup, "sysctl kern.pipe subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "pipe",
		       SYSCTL_DESCR("Pipe settings"),
		       NULL, 0, NULL, 0,
		       CTL_KERN, KERN_PIPE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxbigpipes",
		       SYSCTL_DESCR("Maximum number of \"big\" pipes"),
		       NULL, 0, &maxbigpipes, 0,
		       CTL_KERN, KERN_PIPE, KERN_PIPE_MAXBIGPIPES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "nbigpipes",
		       SYSCTL_DESCR("Number of \"big\" pipes"),
		       NULL, 0, &nbigpipe, 0,
		       CTL_KERN, KERN_PIPE, KERN_PIPE_NBIGPIPES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "kvasize",
		       SYSCTL_DESCR("Amount of kernel memory consumed by pipe "
				    "buffers"),
		       NULL, 0, &amountpipekva, 0,
		       CTL_KERN, KERN_PIPE, KERN_PIPE_KVASIZE, CTL_EOL);
}
