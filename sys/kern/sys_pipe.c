/*	$NetBSD: sys_pipe.c,v 1.165 2023/10/13 19:07:08 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sys_pipe.c,v 1.165 2023/10/13 19:07:08 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/filio.h>
#include <sys/kernel.h>
#include <sys/ttycom.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>
#include <sys/atomic.h>
#include <sys/pipe.h>

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

static bool	pipebusy(struct pipe *);
static bool	pipeunbusy(struct pipe *);
static void	pipeselwakeup(struct pipe *, int, int);
static int	pipe_ctor(void *, void *, int);
static void	pipe_dtor(void *, void *);

static pool_cache_t	pipe_cache __read_mostly;

void
pipe_init(void)
{

	pipe_cache = pool_cache_init(sizeof(struct pipe), COHERENCY_UNIT, 0, 0,
	    "pipe", NULL, IPL_NONE, pipe_ctor, pipe_dtor, NULL);
	KASSERT(pipe_cache != NULL);
}

static int
pipe_ctor(void *arg, void *obj, int flags)
{
	struct pipe *pipe = obj;

	memset(pipe, 0, sizeof(struct pipe));
	pipe->pipe_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	cv_init(&pipe->pipe_read, "piperd");
	cv_init(&pipe->pipe_write, "pipewr");
	cv_init(&pipe->pipe_busy, "pipebusy");
	selinit(&pipe->pipe_rdsel);
	selinit(&pipe->pipe_wrsel);
	pipe->pipe_kmem = uvm_km_alloc(kernel_map, PIPE_SIZE, 0,
	    UVM_KMF_PAGEABLE | UVM_KMF_WAITVA);
	pipe->pipe_state = PIPE_SIGNALR | PIPE_RDOPEN | PIPE_WROPEN;
	pipe->pipe_buffer.buffer = (void *)pipe->pipe_kmem;
	pipe->pipe_buffer.size = PIPE_SIZE;
	KASSERT(pipe->pipe_kmem != 0);
	atomic_add_int(&amountpipekva, PIPE_SIZE);

	return 0;
}

static void
pipe_dtor(void *arg, void *obj)
{
	struct pipe *pipe = obj;

	cv_destroy(&pipe->pipe_read);
	cv_destroy(&pipe->pipe_write);
	cv_destroy(&pipe->pipe_busy);
	seldestroy(&pipe->pipe_rdsel);
	seldestroy(&pipe->pipe_wrsel);
	mutex_obj_free(pipe->pipe_lock);
	uvm_km_free(kernel_map, pipe->pipe_kmem, PIPE_SIZE, UVM_KMF_PAGEABLE);
	atomic_add_int(&amountpipekva, -PIPE_SIZE);
}

/*
 * The pipe system call for the DTYPE_PIPE type of pipes
 */
int
pipe1(struct lwp *l, int *fildes, int flags)
{
	struct pipe *pipe;
	file_t *rf, *wf;
	int fd, error;
	proc_t *p;

	if (flags & ~(O_CLOEXEC|O_NONBLOCK|O_NOSIGPIPE))
		return EINVAL;
	p = curproc;

	pipe = pool_cache_get(pipe_cache, PR_WAITOK);
	getnanotime(&pipe->pipe_atime);
	pipe->pipe_mtime = pipe->pipe_atime;
	pipe->pipe_btime = pipe->pipe_atime;

	error = fd_allocfile(&rf, &fd);
	if (error) {
		pool_cache_put(pipe_cache, pipe);
		return error;
	}
	fildes[0] = fd;

	error = fd_allocfile(&wf, &fd);
	if (error) {
		fd_abort(p, rf, fildes[0]);
		pool_cache_put(pipe_cache, pipe);
		return error;
	}
	fildes[1] = fd;

	rf->f_flag = FREAD | flags;
	rf->f_type = DTYPE_PIPE;
	rf->f_pipe = pipe;
	rf->f_ops = &pipeops;
	fd_set_exclose(l, fildes[0], (flags & O_CLOEXEC) != 0);

	wf->f_flag = FWRITE | flags;
	wf->f_type = DTYPE_PIPE;
	wf->f_pipe = pipe;
	wf->f_ops = &pipeops;
	fd_set_exclose(l, fildes[1], (flags & O_CLOEXEC) != 0);

	fd_affix(p, rf, fildes[0]);
	fd_affix(p, wf, fildes[1]);
	return 0;
}

/*
 * Busy a pipe for I/O, blocking other access.  Called with pipe lock held.
 * NB: curlwp may already hold the pipe busy.
 */
static bool
pipebusy(struct pipe *pipe)
{
	struct lwp *l = curlwp;
	bool blocked = false;

	KASSERT(mutex_owned(pipe->pipe_lock));

	if (pipe->pipe_owner != l) {
		while (__predict_false(pipe->pipe_owner != NULL)) {
			cv_wait(&pipe->pipe_busy, pipe->pipe_lock);
			blocked = true;
		}
		pipe->pipe_owner = l;
	}

	return blocked;
}

/*
 * Unbusy a pipe for I/O, if held busy by curlwp.
 */
static bool
pipeunbusy(struct pipe *pipe)
{

	KASSERT(mutex_owned(pipe->pipe_lock));

	if (pipe->pipe_owner == curlwp) {
		pipe->pipe_owner = NULL;
		return true;
	} else
		return false;
}

/*
 * Select/poll wakeup.  This also sends SIGIO to peer.
 */
static void
pipeselwakeup(struct pipe *pipe, int side, int code)
{
	struct selinfo *selp;
	int band, flag;
	pid_t pgid;

	KASSERT(mutex_owned(pipe->pipe_lock));

	if (side == FREAD) {
		selp = &pipe->pipe_rdsel;
		pgid = pipe->pipe_rdpgid;
		flag = PIPE_RDASYNC;
	} else {
		selp = &pipe->pipe_wrsel;
		pgid = pipe->pipe_wrpgid;
		flag = PIPE_WRASYNC;
	}

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

	selnotify(selp, band, NOTE_SUBMIT);

	if (pgid != 0 && (pipe->pipe_state & flag) != 0)
		fownsignal(pgid, SIGIO, code, band, pipe);
}

static int
pipe_read(file_t *fp, off_t *offset, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct pipe *pipe = fp->f_pipe;
	struct pipebuf *bp = &pipe->pipe_buffer;
	size_t size, cnt, ocnt, nread = 0;
	kmutex_t *lock = pipe->pipe_lock;
	int error = 0;
	bool unbusy;

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
		    (atomic_load_relaxed(&pipe->pipe_state) & PIPE_EOF) == 0)
			return EAGAIN;
	}

	mutex_enter(lock);
	ocnt = bp->cnt;

	while (uio->uio_resid) {
		/*
		 * Normal pipe buffer receive.
		 */
		if (bp->cnt > 0) {
			/* If pipebusy() blocked then re-validate. */
			if (pipebusy(pipe))
				continue;
			size = bp->size - bp->out;
			if (size > bp->cnt)
				size = bp->cnt;
			if (size > uio->uio_resid)
				size = uio->uio_resid;

			KASSERT(pipe->pipe_owner == curlwp);
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
		 */
		if ((pipe->pipe_state & PIPE_EOF) != 0)
			break;

		/*
		 * Don't block on non-blocking I/O.
		 */
		if ((fp->f_flag & FNONBLOCK) != 0) {
			error = EAGAIN;
			break;
		}

		/*
		 * Awaken the other side (including select/poll/kqueue)
		 * then sleep ASAP to minimise contention.
		 */
		pipeselwakeup(pipe, FWRITE, POLL_OUT);
		if (pipeunbusy(pipe))
			cv_signal(&pipe->pipe_busy);
		cv_broadcast(&pipe->pipe_write);
		if ((error = cv_wait_sig(&pipe->pipe_read, lock)) != 0)
			break;
	}

	/*
	 * Update timestamp and drop the long term lock (if held).
	 */
	if (error == 0)
		getnanotime(&pipe->pipe_atime);
	unbusy = pipeunbusy(pipe);

	/*
	 * If anything was read off the buffer, signal to the writer it's
	 * possible to write more data. Also send signal if we are here for the
	 * first time after last write.
	 */
	cnt = bp->cnt;
	if (bp->size - cnt >= PIPE_BUF
	    && (ocnt != cnt || (pipe->pipe_state & PIPE_SIGNALR) != 0)) {
		pipe->pipe_state &= ~PIPE_SIGNALR;
		pipeselwakeup(pipe, FWRITE, POLL_OUT);
	}

	/*
	 * Release the mutex and only then wake the other side, to minimise
	 * contention.
	 */
	mutex_exit(lock);
	if (unbusy)
		cv_signal(&pipe->pipe_busy);
	if (cnt < MINPIPESIZE)
		cv_broadcast(&pipe->pipe_write);

	return error;
}

static int
pipe_write(file_t *fp, off_t *offset, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct pipe *pipe = fp->f_pipe;
	struct pipebuf *bp = &pipe->pipe_buffer;
	kmutex_t *lock = pipe->pipe_lock;
	size_t cnt, space, orig_resid = uio->uio_resid;
	bool unbusy;
	int error;

	/*
	 * If it is advantageous to resize the pipe buffer, do so.
	 */
	mutex_enter(lock);
	if (uio->uio_resid > PIPE_SIZE &&
	    (pipe->pipe_state & PIPE_RESIZED) == 0 &&
	    nbigpipe < maxbigpipes && bp->cnt == 0) {
	    	size_t size = round_page(BIG_PIPE_SIZE);
		void *buffer = (void *)uvm_km_alloc(kernel_map, size,
		    0, UVM_KMF_PAGEABLE);
		if (buffer != NULL) {
			atomic_add_int(&amountpipekva, size);
			atomic_inc_uint(&nbigpipe);
			pipe->pipe_buffer.buffer = buffer;
			pipe->pipe_buffer.size = size;
			pipe->pipe_buffer.in = 0;
			pipe->pipe_buffer.out = 0;
			pipe->pipe_buffer.cnt = 0;
		}
		pipe->pipe_state |= PIPE_RESIZED;
	}

	while (uio->uio_resid > 0) {
		/*
		 * If read side has gone away, we just issue a signal to
		 * ourselves.
		 */
		if ((pipe->pipe_state & PIPE_EOF) != 0) {
			error = EPIPE;
			break;
		}

		/* Writes of size <= PIPE_BUF must be atomic. */
		space = bp->size - bp->cnt;
		if (space < uio->uio_resid && uio->uio_resid <= PIPE_BUF)
			space = 0;

		if (space > 0) {
			int size;	/* Transfer size */
			int segsize;	/* first segment to transfer */

			/* If pipebusy() blocked then re-validate. */
			if (pipebusy(pipe))
				continue;

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
			KASSERT(pipe->pipe_owner == curlwp);
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
			continue;
		}

		/*
		 * Don't block on non-blocking I/O.
		 */
		if ((fp->f_flag & FNONBLOCK) != 0) {
			error = EAGAIN;
			break;
		}

		/*
		 * Awaken the other side (including select/poll/kqueue) then
		 * sleep ASAP to minimise contention.
		 */
		pipeselwakeup(pipe, FREAD, POLL_IN);
		if (pipeunbusy(pipe))
			cv_signal(&pipe->pipe_busy);
		cv_broadcast(&pipe->pipe_read);
		if ((error = cv_wait_sig(&pipe->pipe_write, lock)) != 0)
			break;
	}

	/*
	 * Don't return EPIPE if I/O was successful
	 */
	if (error == EPIPE && uio->uio_resid != orig_resid)
		error = 0;

	/*
	 * Update timestamp and drop the long term lock (if held).
	 */
	if (error == 0)
		getnanotime(&pipe->pipe_mtime);
	unbusy = pipeunbusy(pipe);

	/*
	 * Arrange for next read(2) to do a signal.
	 */
	pipe->pipe_state |= PIPE_SIGNALR;

	/*
	 * We have something to offer, wake up select/poll.
	 */
	if ((cnt = bp->cnt) > 0)
		pipeselwakeup(pipe, FREAD, POLL_IN);

	/*
	 * Release the mutex then wake other side, to minimise contention.
	 */
	mutex_exit(lock);
	if (unbusy)
		cv_signal(&pipe->pipe_busy);
	if (cnt > 0)
		cv_broadcast(&pipe->pipe_read);

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
	int flag;

	switch (cmd) {
	case FIONBIO:
		return 0;

	case FIOASYNC:
		flag = (fp->f_flag & FREAD) != 0 ? PIPE_RDASYNC : PIPE_WRASYNC;
		mutex_enter(lock);
		if (*(int *)data)
			pipe->pipe_state |= flag;
		else
			pipe->pipe_state &= ~flag;
		mutex_exit(lock);
		return 0;

	case FIONREAD:
		if ((fp->f_flag & FREAD) != 0)
			*(int *)data =
			    atomic_load_relaxed(&pipe->pipe_buffer.cnt);
		else
			*(int *)data = 0;
		return 0;

	case FIONWRITE:
		if ((fp->f_flag & FWRITE) != 0)
			*(int *)data =
			    atomic_load_relaxed(&pipe->pipe_buffer.cnt);
		else
			*(int *)data = 0;
		return (0);

	case FIONSPACE:
		if ((fp->f_flag & FWRITE) != 0) {
			mutex_enter(lock);
			*(int *)data = pipe->pipe_buffer.size -
			    pipe->pipe_buffer.cnt;
			mutex_exit(lock);
		} else
			*(int *)data = 0;
		return (0);

	case TIOCSPGRP:
	case FIOSETOWN:
		return fsetown((fp->f_flag & FREAD) != 0 ?
		    &pipe->pipe_rdpgid : &pipe->pipe_wrpgid, cmd, data);

	case TIOCGPGRP:
	case FIOGETOWN:
		return fgetown((fp->f_flag & FREAD) != 0 ?
		    pipe->pipe_rdpgid : pipe->pipe_wrpgid, cmd, data);

	default:
		return EPASSTHROUGH;
	}
}

int
pipe_poll(file_t *fp, int events)
{
	struct pipe *pipe = fp->f_pipe;
	kmutex_t *lock = pipe->pipe_lock;
	int revents = 0;

	/* Unlocked fast path for make(1). */
	if ((fp->f_flag & FREAD) != 0 &&
	    atomic_load_relaxed(&pipe->pipe_buffer.cnt) != 0 &&
	    (atomic_load_relaxed(&pipe->pipe_state) & PIPE_EOF) == 0 &&
	    (events & (POLLIN | POLLRDNORM)) != 0 &&
	    (events & (POLLOUT | POLLWRNORM)) == 0)
		return events & (POLLIN | POLLRDNORM);

	mutex_enter(lock);

	if ((fp->f_flag & FREAD) != 0) {
		if ((events & (POLLIN | POLLRDNORM)) != 0) {
			if (pipe->pipe_buffer.cnt > 0 ||
			    (pipe->pipe_state & PIPE_EOF) != 0)
				revents |= events & (POLLIN | POLLRDNORM);
			selrecord(curlwp, &pipe->pipe_rdsel);
		}
	} else if ((events & (POLLOUT | POLLWRNORM)) != 0) {
		KASSERT((fp->f_flag & FWRITE) != 0);
		size_t space = pipe->pipe_buffer.size - pipe->pipe_buffer.cnt;
		if ((pipe->pipe_state & PIPE_EOF) != 0)
			revents |= events & (POLLOUT | POLLWRNORM);
		if ((pipe->pipe_state & PIPE_EOF) || space >= PIPE_BUF)
			revents |= events & (POLLOUT | POLLWRNORM);
		selrecord(curlwp, &pipe->pipe_wrsel);
	}

	if ((pipe->pipe_state & PIPE_EOF) != 0)
		revents |= POLLHUP;

	mutex_exit(lock);

	return revents;
}

static int
pipe_stat(file_t *fp, struct stat *ub)
{
	struct pipe *pipe = fp->f_pipe;
	kmutex_t *lock = pipe->pipe_lock;

	memset(ub, 0, sizeof(*ub));

	mutex_enter(lock);
	ub->st_mode = S_IFIFO | S_IRUSR | S_IWUSR;
	ub->st_blksize = pipe->pipe_buffer.size;
	ub->st_size = pipe->pipe_buffer.cnt;
	ub->st_blocks = (ub->st_size) ? 1 : 0;
	ub->st_atimespec = pipe->pipe_atime;
	ub->st_mtimespec = pipe->pipe_mtime;
	ub->st_ctimespec = pipe->pipe_btime;
	ub->st_birthtimespec = pipe->pipe_btime;
	ub->st_uid = kauth_cred_geteuid(fp->f_cred);
	ub->st_gid = kauth_cred_getegid(fp->f_cred);
	mutex_exit(lock);

	/*
	 * Left as 0: st_dev, st_ino, st_nlink, st_rdev, st_flags, st_gen.
	 * XXX (st_dev, st_ino) should be unique.
	 */
	return 0;
}

static int
pipe_close(file_t *fp)
{
	struct pipe *pipe = fp->f_pipe;
	kmutex_t *lock = pipe->pipe_lock;
	u_int state;

	KASSERT(cv_is_valid(&pipe->pipe_read));
	KASSERT(cv_is_valid(&pipe->pipe_write));
	KASSERT(cv_is_valid(&pipe->pipe_busy));

	fp->f_pipe = NULL;

	/*
	 * If the other side is blocked, wake it up.
	 *
	 * Any knote objects still left in the list are the one attached by
	 * peer.  Since no one will traverse this list, we just clear it.
	 *
	 * XXX Exposes select/kqueue internals.
	 */
	mutex_enter(lock);
	pipebusy(pipe);
	state = pipe->pipe_state | PIPE_EOF;
	if ((fp->f_flag & FREAD) != 0) {
		KASSERT((state & PIPE_RDOPEN) != 0);
		SLIST_INIT(&pipe->pipe_rdsel.sel_klist);
		pipe->pipe_rdpgid = 0;
		state &= ~(PIPE_RDASYNC | PIPE_RDOPEN);
		pipeselwakeup(pipe, FWRITE, POLL_HUP);
		cv_broadcast(&pipe->pipe_write);
	} else {
		KASSERT((fp->f_flag & FWRITE) != 0);
		KASSERT((state & PIPE_WROPEN) != 0);
		SLIST_INIT(&pipe->pipe_wrsel.sel_klist);
		pipe->pipe_wrpgid = 0;
		state &= ~(PIPE_WRASYNC | PIPE_WROPEN);
		pipeselwakeup(pipe, FREAD, POLL_HUP);
		cv_broadcast(&pipe->pipe_read);
	}
	pipe->pipe_state = state;
	pipeunbusy(pipe);
	cv_signal(&pipe->pipe_busy);
	mutex_exit(lock);

	/*
	 * NB: now that the mutex is released, we cannot touch "pipe" any
	 * more unless we are the last guy out, since nothing else is
	 * keeping the data structure around.  This also means we have to
	 * wake the other side with the mutex held above.
	 */
	if ((state & (PIPE_RDOPEN | PIPE_WROPEN)) != 0)
		return 0;

	/* Both sides are closed, free resources. */
	pipe->pipe_state = PIPE_SIGNALR | PIPE_RDOPEN | PIPE_WROPEN;
	pipe->pipe_buffer.in = 0;
	pipe->pipe_buffer.out = 0;
	pipe->pipe_buffer.cnt = 0;
	if (pipe->pipe_buffer.buffer != (void *)pipe->pipe_kmem) {
		uvm_km_free(kernel_map, (vaddr_t)pipe->pipe_buffer.buffer,
		    pipe->pipe_buffer.size, UVM_KMF_PAGEABLE);
		atomic_add_int(&amountpipekva, -pipe->pipe_buffer.size);
		atomic_dec_uint(&nbigpipe);
		pipe->pipe_buffer.buffer = (void *)pipe->pipe_kmem;
		pipe->pipe_buffer.size = PIPE_SIZE;
	}
	pool_cache_put(pipe_cache, pipe);

	return 0;
}

static void
pipe_restart(file_t *fp)
{
	struct pipe *pipe = fp->f_pipe;
	kmutex_t *lock = pipe->pipe_lock;

	/*
	 * Unblock blocked reads/writes in order to allow close() to complete.
	 * System calls return ERESTART so that the fd is revalidated.
	 * (Partial writes return the transfer length.)
	 */
	mutex_enter(lock);
	cv_fdrestart(&pipe->pipe_read);
	cv_fdrestart(&pipe->pipe_write);
	cv_fdrestart(&pipe->pipe_busy);
	mutex_exit(lock);
}

static int
pipe_fpathconf(struct file *fp, int name, register_t *retval)
{

	switch (name) {
	case _PC_PIPE_BUF:
		*retval = PIPE_BUF;
		return 0;
	default:
		return EINVAL;
	}
}

static int
pipe_posix_fadvise(struct file *fp, off_t offset, off_t len, int advice)
{

	return ESPIPE;
}

static void
filt_pipedetach(struct knote *kn)
{
	struct file *fp = kn->kn_obj;
	struct pipe *pipe = fp->f_pipe;
	kmutex_t *lock = pipe->pipe_lock;

	mutex_enter(lock);

	KASSERT(kn->kn_hook == pipe);
	if ((fp->f_flag & FREAD) != 0) {
		if ((pipe->pipe_state & PIPE_RDOPEN) != 0)
			selremove_knote(&pipe->pipe_rdsel, kn);
	} else if ((pipe->pipe_state & PIPE_WROPEN) != 0)
		selremove_knote(&pipe->pipe_wrsel, kn);
	mutex_exit(lock);
}

static int
filt_piperead(struct knote *kn, long hint)
{
	struct file *fp = kn->kn_obj;
	struct pipe *pipe = fp->f_pipe;
	kmutex_t *lock = pipe->pipe_lock;
	int rv;

	if ((hint & NOTE_SUBMIT) == 0) {
		mutex_enter(lock);
	}

	if ((fp->f_flag & FREAD) != 0)
		kn->kn_data = pipe->pipe_buffer.cnt;
	else
		kn->kn_data = 0;

	if ((pipe->pipe_state & PIPE_EOF) != 0) {
		knote_set_eof(kn, 0);
		rv = 1;
	} else {
		rv = kn->kn_data > 0;
	}

	if ((hint & NOTE_SUBMIT) == 0) {
		mutex_exit(lock);
	}
	return rv;
}

static int
filt_pipewrite(struct knote *kn, long hint)
{
	struct file *fp = kn->kn_obj;
	struct pipe *pipe = fp->f_pipe;
	kmutex_t *lock = pipe->pipe_lock;
	int rv;

	if ((hint & NOTE_SUBMIT) == 0) {
		mutex_enter(lock);
	}

	if ((pipe->pipe_state & PIPE_EOF)) {
		kn->kn_data = 0;
		knote_set_eof(kn, 0);
		rv = 1;
	} else if ((fp->f_flag & FWRITE) != 0) {
		kn->kn_data = pipe->pipe_buffer.size - pipe->pipe_buffer.cnt;
		rv = kn->kn_data >= PIPE_BUF;
	} else {
		kn->kn_data = 0;
		rv = 0;
	}

	if ((hint & NOTE_SUBMIT) == 0) {
		mutex_exit(lock);
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
	struct pipe *pipe = ((file_t *)kn->kn_obj)->f_pipe;
	kmutex_t *lock = pipe->pipe_lock;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		if ((fp->f_flag & FREAD) == 0)
			return EINVAL;
		mutex_enter(lock);
		kn->kn_fop = &pipe_rfiltops;
		kn->kn_hook = pipe;
		selrecord_knote(&pipe->pipe_rdsel, kn);
		mutex_exit(lock);
		break;
	case EVFILT_WRITE:
		if ((fp->f_flag & FWRITE) == 0)
			return EINVAL;
		mutex_enter(lock);
		kn->kn_fop = &pipe_wfiltops;
		if ((pipe->pipe_state & PIPE_EOF) != 0) {
			/* Other end of pipe has been closed. */
			mutex_exit(lock);
			return EBADF;
		}
		kn->kn_hook = pipe;
		selrecord_knote(&pipe->pipe_wrsel, kn);
		mutex_exit(lock);
		break;
	default:
		return EINVAL;
	}

	return (0);
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
