/*	$NetBSD: sys_pipe.c,v 1.4.2.11 2002/04/01 07:47:57 nathanw Exp $	*/

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
 *
 * $FreeBSD: src/sys/kern/sys_pipe.c,v 1.95 2002/03/09 22:06:31 alfred Exp $
 */

/*
 * This file contains a high-performance replacement for the socket-based
 * pipes scheme originally used in FreeBSD/4.4Lite.  It does not support
 * all features of sockets, but does do everything that pipes normally
 * do.
 *
 * Adaption for NetBSD UVM, including uvm_loan() based direct write, was
 * written by Jaromir Dolecek.
 */

/*
 * This code has two modes of operation, a small write mode and a large
 * write mode.  The small write mode acts like conventional pipes with
 * a kernel buffer.  If the buffer is less than PIPE_MINDIRECT, then the
 * "normal" pipe buffering is done.  If the buffer is between PIPE_MINDIRECT
 * and PIPE_SIZE in size, it is fully mapped into the kernel (on FreeBSD,
 * those pages are also wired), and the receiving process can copy it directly
 * from the pages in the sending process.
 *
 * If the sending process receives a signal, it is possible that it will
 * go away, and certainly its address space can change, because control
 * is returned back to the user-mode side.  In that case, the pipe code
 * arranges to copy the buffer supplied by the user process on FreeBSD, to
 * a pageable kernel buffer, and the receiving process will grab the data
 * from the pageable kernel buffer.  Since signals don't happen all that often,
 * the copy operation is normally eliminated.
 * For NetBSD, the pages are mapped read-only, COW for kernel by uvm_loan(),
 * so no explicit handling need to be done, all is handled by standard VM
 * facilities.
 *
 * The constant PIPE_MINDIRECT is chosen to make sure that buffering will
 * happen for small transfers so that the system will not spend all of
 * its time context switching.  PIPE_SIZE is constrained by the
 * amount of kernel virtual memory.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_pipe.c,v 1.4.2.11 2002/04/01 07:47:57 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/filio.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/ttycom.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/lock.h>
#ifdef __FreeBSD__
#include <sys/mutex.h>
#endif
#ifdef __NetBSD__
#include <sys/select.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <uvm/uvm.h>
#include <sys/sysctl.h>
#include <sys/kernel.h>
#endif /* NetBSD, FreeBSD */

#include <sys/pipe.h>

#ifdef __NetBSD__
/*
 * Avoid microtime(9), it's slow. We don't guard the read from time(9)
 * with splclock(9) since we don't actually need to be THAT sure the access
 * is atomic.
 */
#define vfs_timestamp(tv)	(*(tv) = time)
#endif

/*
 * Use this define if you want to disable *fancy* VM things.  Expect an
 * approx 30% decrease in transfer rate.  This could be useful for
 * OpenBSD.
 */
/* #define PIPE_NODIRECT */

/*
 * interfaces to the outside world
 */
#ifdef __FreeBSD__
static int pipe_read(struct file *fp, struct uio *uio, 
		struct ucred *cred, int flags, struct thread *td);
static int pipe_write(struct file *fp, struct uio *uio, 
		struct ucred *cred, int flags, struct thread *td);
static int pipe_close(struct file *fp, struct thread *td);
static int pipe_poll(struct file *fp, int events, struct ucred *cred,
		struct thread *td);
static int pipe_kqfilter(struct file *fp, struct knote *kn);
static int pipe_stat(struct file *fp, struct stat *sb, struct thread *td);
static int pipe_ioctl(struct file *fp, u_long cmd, caddr_t data, struct thread *td);

static struct fileops pipeops = {
	pipe_read, pipe_write, pipe_ioctl, pipe_poll, pipe_kqfilter,
	pipe_stat, pipe_close
};

static void	filt_pipedetach(struct knote *kn);
static int	filt_piperead(struct knote *kn, long hint);
static int	filt_pipewrite(struct knote *kn, long hint);

static struct filterops pipe_rfiltops =
	{ 1, NULL, filt_pipedetach, filt_piperead };
static struct filterops pipe_wfiltops =
	{ 1, NULL, filt_pipedetach, filt_pipewrite };

#define PIPE_GET_GIANT(pipe)							\
	do {								\
		PIPE_UNLOCK(wpipe);					\
		mtx_lock(&Giant);					\
	} while (0)

#define PIPE_DROP_GIANT(pipe)						\
	do {								\
		mtx_unlock(&Giant);					\
		PIPE_LOCK(wpipe);					\
	} while (0)

#endif /* FreeBSD */

#ifdef __NetBSD__
static int pipe_read(struct file *fp, off_t *offset, struct uio *uio, 
		struct ucred *cred, int flags);
static int pipe_write(struct file *fp, off_t *offset, struct uio *uio, 
		struct ucred *cred, int flags);
static int pipe_close(struct file *fp, struct proc *p);
static int pipe_poll(struct file *fp, int events, struct proc *p);
static int pipe_fcntl(struct file *fp, u_int com, caddr_t data,
		struct proc *p);
static int pipe_stat(struct file *fp, struct stat *sb, struct proc *p);
static int pipe_ioctl(struct file *fp, u_long cmd, caddr_t data, struct proc *p);

static struct fileops pipeops =
    { pipe_read, pipe_write, pipe_ioctl, pipe_fcntl, pipe_poll,
      pipe_stat, pipe_close };

/* XXXSMP perhaps use spinlocks & KERNEL_PROC_(UN)LOCK() ? just clear now */
#define PIPE_GET_GIANT(pipe)
#define PIPE_DROP_GIANT(pipe)
#define GIANT_REQUIRED

#endif /* NetBSD */

/*
 * Default pipe buffer size(s), this can be kind-of large now because pipe
 * space is pageable.  The pipe code will try to maintain locality of
 * reference for performance reasons, so small amounts of outstanding I/O
 * will not wipe the cache.
 */
#define MINPIPESIZE (PIPE_SIZE/3)
#define MAXPIPESIZE (2*PIPE_SIZE/3)

/*
 * Maximum amount of kva for pipes -- this is kind-of a soft limit, but
 * is there so that on large systems, we don't exhaust it.
 */
#define MAXPIPEKVA (8*1024*1024)
static int maxpipekva = MAXPIPEKVA;

/*
 * Limit for direct transfers, we cannot, of course limit
 * the amount of kva for pipes in general though.
 */
#define LIMITPIPEKVA (16*1024*1024)
static int limitpipekva = LIMITPIPEKVA;

/*
 * Limit the number of "big" pipes
 */
#define LIMITBIGPIPES  32
static int maxbigpipes = LIMITBIGPIPES;
static int nbigpipe = 0;

/*
 * Amount of KVA consumed by pipe buffers.
 */
static int amountpipekva = 0;

static void pipeclose(struct pipe *cpipe);
static void pipe_free_kmem(struct pipe *cpipe);
static int pipe_create(struct pipe **cpipep, int allockva);
static __inline int pipelock(struct pipe *cpipe, int catch);
static __inline void pipeunlock(struct pipe *cpipe);
static __inline void pipeselwakeup(struct pipe *cpipe, struct pipe *sigp);
#ifndef PIPE_NODIRECT
static int pipe_direct_write(struct pipe *wpipe, struct uio *uio);
#endif
static int pipespace(struct pipe *cpipe, int size);

#ifdef __NetBSD__
#ifndef PIPE_NODIRECT
static int pipe_loan_alloc(struct pipe *, int);
static void pipe_loan_free(struct pipe *);
#endif /* PIPE_NODIRECT */

static struct pool pipe_pool;
#endif /* NetBSD */

#ifdef __FreeBSD__
static vm_zone_t pipe_zone;

static void pipeinit(void *dummy __unused);
#ifndef PIPE_NODIRECT
static int pipe_build_write_buffer(struct pipe *wpipe, struct uio *uio);
static void pipe_destroy_write_buffer(struct pipe *wpipe);
static void pipe_clone_write_buffer(struct pipe *wpipe);
#endif

SYSINIT(vfs, SI_SUB_VFS, SI_ORDER_ANY, pipeinit, NULL);

static void
pipeinit(void *dummy __unused)
{

	pipe_zone = zinit("PIPE", sizeof(struct pipe), 0, 0, 4);
}
#endif /* FreeBSD */

/*
 * The pipe system call for the DTYPE_PIPE type of pipes
 */

/* ARGSUSED */
#ifdef __FreeBSD__
int
pipe(td, uap)
	struct thread *td;
	struct pipe_args /* {
		int	dummy;
	} */ *uap;
#elif defined(__NetBSD__)
int
sys_pipe(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
#endif
{
	struct file *rf, *wf;
	struct pipe *rpipe, *wpipe;
	int fd, error;
	struct proc *p;
#ifdef __FreeBSD__
	struct mtx *pmtx;

	KASSERT(pipe_zone != NULL, ("pipe_zone not initialized"));

	pmtx = malloc(sizeof(*pmtx), M_TEMP, M_WAITOK | M_ZERO);
	
	rpipe = wpipe = NULL;
	if (pipe_create(&rpipe, 1) || pipe_create(&wpipe, 1)) {
		pipeclose(rpipe); 
		pipeclose(wpipe); 
		free(pmtx, M_TEMP);
		return (ENFILE);
	}

	error = falloc(td, &rf, &fd);
	if (error) {
		pipeclose(rpipe);
		pipeclose(wpipe);
		free(pmtx, M_TEMP);
		return (error);
	}
	fhold(rf);
	td->td_retval[0] = fd;

	/*
	 * Warning: once we've gotten past allocation of the fd for the
	 * read-side, we can only drop the read side via fdrop() in order
	 * to avoid races against processes which manage to dup() the read
	 * side while we are blocked trying to allocate the write side.
	 */
	FILE_LOCK(rf);
	rf->f_flag = FREAD | FWRITE;
	rf->f_type = DTYPE_PIPE;
	rf->f_data = (caddr_t)rpipe;
	rf->f_ops = &pipeops;
	FILE_UNLOCK(rf);
	error = falloc(td, &wf, &fd);
	if (error) {
		struct filedesc *fdp = td->td_proc->p_fd;
		FILEDESC_LOCK(fdp);
		if (fdp->fd_ofiles[td->td_retval[0]] == rf) {
			fdp->fd_ofiles[td->td_retval[0]] = NULL;
			FILEDESC_UNLOCK(fdp);
			fdrop(rf, td);
		} else
			FILEDESC_UNLOCK(fdp);
		fdrop(rf, td);
		/* rpipe has been closed by fdrop(). */
		pipeclose(wpipe);
		free(pmtx, M_TEMP);
		return (error);
	}
	FILE_LOCK(wf);
	wf->f_flag = FREAD | FWRITE;
	wf->f_type = DTYPE_PIPE;
	wf->f_data = (caddr_t)wpipe;
	wf->f_ops = &pipeops;
	p->p_retval[1] = fd;
	rpipe->pipe_peer = wpipe;
	wpipe->pipe_peer = rpipe;
	mtx_init(pmtx, "pipe mutex", MTX_DEF);
	rpipe->pipe_mtxp = wpipe->pipe_mtxp = pmtx;
	fdrop(rf, td);
#endif /* FreeBSD */

#ifdef __NetBSD__
	p = l->l_proc;
	rpipe = wpipe = NULL;
	if (pipe_create(&rpipe, 1) || pipe_create(&wpipe, 0)) {
		pipeclose(rpipe);
		pipeclose(wpipe);
		return (ENFILE);
	}

	/*
	 * Note: the file structure returned from falloc() is marked
	 * as 'larval' initially. Unless we mark it as 'mature' by
	 * FILE_SET_MATURE(), any attempt to do anything with it would
	 * return EBADF, including e.g. dup(2) or close(2). This avoids
	 * file descriptor races if we block in the second falloc().
	 */

	error = falloc(p, &rf, &fd);
	if (error)
		goto free2;
	retval[0] = fd;
	rf->f_flag = FREAD;
	rf->f_type = DTYPE_PIPE;
	rf->f_data = (caddr_t)rpipe;
	rf->f_ops = &pipeops;

	error = falloc(p, &wf, &fd);
	if (error)
		goto free3;
	retval[1] = fd;
	wf->f_flag = FWRITE;
	wf->f_type = DTYPE_PIPE;
	wf->f_data = (caddr_t)wpipe;
	wf->f_ops = &pipeops;

	rpipe->pipe_peer = wpipe;
	wpipe->pipe_peer = rpipe;

	FILE_SET_MATURE(rf);
	FILE_SET_MATURE(wf);
	FILE_UNUSE(rf, p);
	FILE_UNUSE(wf, p);
	return (0);
free3:
	FILE_UNUSE(rf, p);
	ffree(rf);
	fdremove(p->p_fd, retval[0]);
free2:
	pipeclose(wpipe);
	pipeclose(rpipe);
#endif /* NetBSD */

	return (error);
}

/*
 * Allocate kva for pipe circular buffer, the space is pageable
 * This routine will 'realloc' the size of a pipe safely, if it fails
 * it will retain the old buffer.
 * If it fails it will return ENOMEM.
 */
static int
pipespace(cpipe, size)
	struct pipe *cpipe;
	int size;
{
	caddr_t buffer;
#ifdef __FreeBSD__
	struct vm_object *object;
	int npages, error;

	GIANT_REQUIRED;
	KASSERT(cpipe->pipe_mtxp == NULL || !mtx_owned(PIPE_MTX(cpipe)),
	       ("pipespace: pipe mutex locked"));

	npages = round_page(size)/PAGE_SIZE;
	/*
	 * Create an object, I don't like the idea of paging to/from
	 * kernel_object.
	 */
	object = vm_object_allocate(OBJT_DEFAULT, npages);
	buffer = (caddr_t) vm_map_min(kernel_map);

	/*
	 * Insert the object into the kernel map, and allocate kva for it.
	 * The map entry is, by default, pageable.
	 */
	error = vm_map_find(kernel_map, object, 0,
		(vm_offset_t *) &buffer, size, 1,
		VM_PROT_ALL, VM_PROT_ALL, 0);

	if (error != KERN_SUCCESS) {
		vm_object_deallocate(object);
		return (ENOMEM);
	}
#endif /* FreeBSD */

#ifdef __NetBSD__
	/*
	 * Allocate pageable virtual address space. Physical memory is allocated
	 * on demand.
	 */
	buffer = (caddr_t) uvm_km_valloc(kernel_map, round_page(size));
	if (buffer == NULL)
		return (ENOMEM);
#endif /* NetBSD */

	/* free old resources if we're resizing */
	pipe_free_kmem(cpipe);
#ifdef __FreeBSD__
	cpipe->pipe_buffer.object = object;
#endif
	cpipe->pipe_buffer.buffer = buffer;
	cpipe->pipe_buffer.size = size;
	cpipe->pipe_buffer.in = 0;
	cpipe->pipe_buffer.out = 0;
	cpipe->pipe_buffer.cnt = 0;
	amountpipekva += cpipe->pipe_buffer.size;
	return (0);
}

/*
 * initialize and allocate VM and memory for pipe
 */
static int
pipe_create(cpipep, allockva)
	struct pipe **cpipep;
	int allockva;
{
	struct pipe *cpipe;
	int error;

#ifdef __FreeBSD__
	*cpipep = zalloc(pipe_zone);
#endif
#ifdef __NetBSD__
	*cpipep = pool_get(&pipe_pool, M_WAITOK);
#endif
	if (*cpipep == NULL)
		return (ENOMEM);

	cpipe = *cpipep;

	/* Initialize */ 
	memset(cpipe, 0, sizeof(*cpipe));
	cpipe->pipe_state = PIPE_SIGNALR;

#ifdef __FreeBSD__
	cpipe->pipe_mtxp = NULL;	/* avoid pipespace assertion */
#endif
	if (allockva && (error = pipespace(cpipe, PIPE_SIZE)))
		return (error);

	vfs_timestamp(&cpipe->pipe_ctime);
	cpipe->pipe_atime = cpipe->pipe_ctime;
	cpipe->pipe_mtime = cpipe->pipe_ctime;
#ifdef __NetBSD__
	cpipe->pipe_pgid = NO_PID;
	lockinit(&cpipe->pipe_lock, PRIBIO | PCATCH, "pipelk", 0, 0);
#endif

	return (0);
}


/*
 * lock a pipe for I/O, blocking other access
 */
static __inline int
pipelock(cpipe, catch)
	struct pipe *cpipe;
	int catch;
{
	int error;

#ifdef __FreeBSD__
	PIPE_LOCK_ASSERT(cpipe, MA_OWNED);
	while (cpipe->pipe_state & PIPE_LOCKFL) {
		cpipe->pipe_state |= PIPE_LWANT;
		error = msleep(cpipe, PIPE_MTX(cpipe),
		    catch ? (PRIBIO | PCATCH) : PRIBIO,
		    "pipelk", 0);
		if (error != 0)
			return (error);
	}
	cpipe->pipe_state |= PIPE_LOCKFL;
	return (0);
#endif

#ifdef __NetBSD__
	do {
		error = lockmgr(&cpipe->pipe_lock, LK_EXCLUSIVE, NULL);
	} while (!catch && (error == EINTR || error == ERESTART));
	return (error);
#endif
}

/*
 * unlock a pipe I/O lock
 */
static __inline void
pipeunlock(cpipe)
	struct pipe *cpipe;
{

#ifdef __FreeBSD__
	PIPE_LOCK_ASSERT(cpipe, MA_OWNED);
	cpipe->pipe_state &= ~PIPE_LOCKFL;
	if (cpipe->pipe_state & PIPE_LWANT) {
		cpipe->pipe_state &= ~PIPE_LWANT;
		wakeup(cpipe);
	}
#endif

#ifdef __NetBSD__
	lockmgr(&cpipe->pipe_lock, LK_RELEASE, NULL);
#endif
}

/*
 * Select/poll wakup. This also sends SIGIO to peer connected to
 * 'sigpipe' side of pipe.
 */
static __inline void
pipeselwakeup(selp, sigp)
	struct pipe *selp, *sigp;
{
	if (selp->pipe_state & PIPE_SEL) {
		selp->pipe_state &= ~PIPE_SEL;
		selwakeup(&selp->pipe_sel);
	}
#ifdef __FreeBSD__
	if (sigp && (sigp->pipe_state & PIPE_ASYNC) && sigp->pipe_sigio)
		pgsigio(sigp->pipe_sigio, SIGIO, 0);
	KNOTE(&selp->pipe_sel.si_note, 0);
#endif

#ifdef __NetBSD__
	if (sigp && (sigp->pipe_state & PIPE_ASYNC)
	    && sigp->pipe_pgid != NO_PID){
		struct proc *p;

		if (sigp->pipe_pgid < 0)
			gsignal(-sigp->pipe_pgid, SIGIO);
		else if (sigp->pipe_pgid > 0 && (p = pfind(sigp->pipe_pgid)) != 0)
			psignal(p, SIGIO);
	}
#endif /* NetBSD */
}

/* ARGSUSED */
#ifdef __FreeBSD__
static int
pipe_read(fp, uio, cred, flags, td)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
	struct thread *td;
	int flags;
	struct proc *p;
#elif defined(__NetBSD__)
static int
pipe_read(fp, offset, uio, cred, flags)
	struct file *fp;
	off_t *offset;
	struct uio *uio;
	struct ucred *cred;
	int flags;
#endif
{
	struct pipe *rpipe = (struct pipe *) fp->f_data;
	int error;
	size_t nread = 0;
	size_t size;
	size_t ocnt;

	PIPE_LOCK(rpipe);
	++rpipe->pipe_busy;
	error = pipelock(rpipe, 1);
	if (error)
		goto unlocked_error;

	ocnt = rpipe->pipe_buffer.cnt;

	while (uio->uio_resid) {
		/*
		 * normal pipe buffer receive
		 */
		if (rpipe->pipe_buffer.cnt > 0) {
			size = rpipe->pipe_buffer.size - rpipe->pipe_buffer.out;
			if (size > rpipe->pipe_buffer.cnt)
				size = rpipe->pipe_buffer.cnt;
			if (size > uio->uio_resid)
				size = uio->uio_resid;

			PIPE_UNLOCK(rpipe);
			error = uiomove(&rpipe->pipe_buffer.buffer[rpipe->pipe_buffer.out],
					size, uio);
			PIPE_LOCK(rpipe);
			if (error)
				break;

			rpipe->pipe_buffer.out += size;
			if (rpipe->pipe_buffer.out >= rpipe->pipe_buffer.size)
				rpipe->pipe_buffer.out = 0;

			rpipe->pipe_buffer.cnt -= size;

			/*
			 * If there is no more to read in the pipe, reset
			 * its pointers to the beginning.  This improves
			 * cache hit stats.
			 */
			if (rpipe->pipe_buffer.cnt == 0) {
				rpipe->pipe_buffer.in = 0;
				rpipe->pipe_buffer.out = 0;
			}
			nread += size;
#ifndef PIPE_NODIRECT
		/*
		 * Direct copy, bypassing a kernel buffer.
		 */
		} else if ((size = rpipe->pipe_map.cnt) &&
			   (rpipe->pipe_state & PIPE_DIRECTW)) {
			caddr_t	va;
			if (size > uio->uio_resid)
				size = uio->uio_resid;

			va = (caddr_t) rpipe->pipe_map.kva +
			    rpipe->pipe_map.pos;
			PIPE_UNLOCK(rpipe);
			error = uiomove(va, size, uio);
			PIPE_LOCK(rpipe);
			if (error)
				break;
			nread += size;
			rpipe->pipe_map.pos += size;
			rpipe->pipe_map.cnt -= size;
			if (rpipe->pipe_map.cnt == 0) {
				rpipe->pipe_state &= ~PIPE_DIRECTW;
				wakeup(rpipe);
			}
#endif
		} else {
			/*
			 * detect EOF condition
			 * read returns 0 on EOF, no need to set error
			 */
			if (rpipe->pipe_state & PIPE_EOF)
				break;

			/*
			 * If the "write-side" has been blocked, wake it up now.
			 */
			if (rpipe->pipe_state & PIPE_WANTW) {
				rpipe->pipe_state &= ~PIPE_WANTW;
				wakeup(rpipe);
			}

			/*
			 * Break if some data was read.
			 */
			if (nread > 0)
				break;

			/*
			 * don't block on non-blocking I/O
			 */
			if (fp->f_flag & FNONBLOCK) {
				error = EAGAIN;
				break;
			}

			/*
			 * Unlock the pipe buffer for our remaining processing.
			 * We will either break out with an error or we will
			 * sleep and relock to loop.
			 */
			pipeunlock(rpipe);

			/*
			 * We want to read more, wake up select/poll.
			 */
			pipeselwakeup(rpipe, rpipe->pipe_peer);

			rpipe->pipe_state |= PIPE_WANTR;
#ifdef __FreeBSD__
			error = msleep(rpipe, PIPE_MTX(rpipe), PRIBIO | PCATCH,
				    "piperd", 0);
#else
			error = tsleep(rpipe, PRIBIO | PCATCH, "piperd", 0);
#endif
			if (error != 0 || (error = pipelock(rpipe, 1)))
				goto unlocked_error;
		}
	}
	pipeunlock(rpipe);

	/* XXX: should probably do this before getting any locks. */
	if (error == 0)
		vfs_timestamp(&rpipe->pipe_atime);
unlocked_error:
	--rpipe->pipe_busy;

	/*
	 * PIPE_WANTCLOSE processing only makes sense if pipe_busy is 0.
	 */
	if ((rpipe->pipe_busy == 0) && (rpipe->pipe_state & PIPE_WANTCLOSE)) {
		rpipe->pipe_state &= ~(PIPE_WANTCLOSE|PIPE_WANTW);
		wakeup(rpipe);
	} else if (rpipe->pipe_buffer.cnt < MINPIPESIZE) {
		/*
		 * Handle write blocking hysteresis.
		 */
		if (rpipe->pipe_state & PIPE_WANTW) {
			rpipe->pipe_state &= ~PIPE_WANTW;
			wakeup(rpipe);
		}
	}

	/*
	 * If anything was read off the buffer, signal to the writer it's
	 * possible to write more data. Also send signal if we are here for the
	 * first time after last write.
	 */
	if ((rpipe->pipe_buffer.size - rpipe->pipe_buffer.cnt) >= PIPE_BUF
	    && (ocnt != rpipe->pipe_buffer.cnt || (rpipe->pipe_state & PIPE_SIGNALR))) {
		pipeselwakeup(rpipe, rpipe->pipe_peer);
		rpipe->pipe_state &= ~PIPE_SIGNALR;
	}

	PIPE_UNLOCK(rpipe);
	return (error);
}

#ifdef __FreeBSD__
#ifndef PIPE_NODIRECT
/*
 * Map the sending processes' buffer into kernel space and wire it.
 * This is similar to a physical write operation.
 */
static int
pipe_build_write_buffer(wpipe, uio)
	struct pipe *wpipe;
	struct uio *uio;
{
	size_t size;
	int i;
	vm_offset_t addr, endaddr, paddr;

	GIANT_REQUIRED;
	PIPE_LOCK_ASSERT(wpipe, MA_NOTOWNED);

	size = uio->uio_iov->iov_len;
	if (size > wpipe->pipe_buffer.size)
		size = wpipe->pipe_buffer.size;

	endaddr = round_page((vm_offset_t)uio->uio_iov->iov_base + size);
	addr = trunc_page((vm_offset_t)uio->uio_iov->iov_base);
	for (i = 0; addr < endaddr; addr += PAGE_SIZE, i++) {
		vm_page_t m;

		if (vm_fault_quick((caddr_t)addr, VM_PROT_READ) < 0 ||
		    (paddr = pmap_kextract(addr)) == 0) {
			int j;

			for (j = 0; j < i; j++)
				vm_page_unwire(wpipe->pipe_map.ms[j], 1);
			return (EFAULT);
		}

		m = PHYS_TO_VM_PAGE(paddr);
		vm_page_wire(m);
		wpipe->pipe_map.ms[i] = m;
	}

/*
 * set up the control block
 */
	wpipe->pipe_map.npages = i;
	wpipe->pipe_map.pos =
	    ((vm_offset_t) uio->uio_iov->iov_base) & PAGE_MASK;
	wpipe->pipe_map.cnt = size;

/*
 * and map the buffer
 */
	if (wpipe->pipe_map.kva == 0) {
		/*
		 * We need to allocate space for an extra page because the
		 * address range might (will) span pages at times.
		 */
		wpipe->pipe_map.kva = kmem_alloc_pageable(kernel_map,
			wpipe->pipe_buffer.size + PAGE_SIZE);
		amountpipekva += wpipe->pipe_buffer.size + PAGE_SIZE;
	}
	pmap_qenter(wpipe->pipe_map.kva, wpipe->pipe_map.ms,
		wpipe->pipe_map.npages);

/*
 * and update the uio data
 */

	uio->uio_iov->iov_len -= size;
	uio->uio_iov->iov_base += size;
	if (uio->uio_iov->iov_len == 0)
		uio->uio_iov++;
	uio->uio_resid -= size;
	uio->uio_offset += size;
	return (0);
}

/*
 * unmap and unwire the process buffer
 */
static void
pipe_destroy_write_buffer(wpipe)
	struct pipe *wpipe;
{
	int i;

	GIANT_REQUIRED;
	PIPE_LOCK_ASSERT(wpipe, MA_NOTOWNED);

	if (wpipe->pipe_map.kva) {
		pmap_qremove(wpipe->pipe_map.kva, wpipe->pipe_map.npages);

		if (amountpipekva > maxpipekva) {
			vm_offset_t kva = wpipe->pipe_map.kva;
			wpipe->pipe_map.kva = 0;
			kmem_free(kernel_map, kva,
				wpipe->pipe_buffer.size + PAGE_SIZE);
			amountpipekva -= wpipe->pipe_buffer.size + PAGE_SIZE;
		}
	}
	for (i = 0; i < wpipe->pipe_map.npages; i++)
		vm_page_unwire(wpipe->pipe_map.ms[i], 1);
	wpipe->pipe_map.npages = 0;
}

/*
 * In the case of a signal, the writing process might go away.  This
 * code copies the data into the circular buffer so that the source
 * pages can be freed without loss of data.
 */
static void
pipe_clone_write_buffer(wpipe)
	struct pipe *wpipe;
{
	int size;
	int pos;

	PIPE_LOCK_ASSERT(wpipe, MA_OWNED);
	size = wpipe->pipe_map.cnt;
	pos = wpipe->pipe_map.pos;
	memcpy((caddr_t) wpipe->pipe_buffer.buffer,
	    (caddr_t) wpipe->pipe_map.kva + pos, size);

	wpipe->pipe_buffer.in = size;
	wpipe->pipe_buffer.out = 0;
	wpipe->pipe_buffer.cnt = size;
	wpipe->pipe_state &= ~PIPE_DIRECTW;

	PIPE_GET_GIANT(wpipe);
	pipe_destroy_write_buffer(wpipe);
	PIPE_DROP_GIANT(wpipe);
}

/*
 * This implements the pipe buffer write mechanism.  Note that only
 * a direct write OR a normal pipe write can be pending at any given time.
 * If there are any characters in the pipe buffer, the direct write will
 * be deferred until the receiving process grabs all of the bytes from
 * the pipe buffer.  Then the direct mapping write is set-up.
 */
static int
pipe_direct_write(wpipe, uio)
	struct pipe *wpipe;
	struct uio *uio;
{
	int error;

retry:
	PIPE_LOCK_ASSERT(wpipe, MA_OWNED);
	while (wpipe->pipe_state & PIPE_DIRECTW) {
		if (wpipe->pipe_state & PIPE_WANTR) {
			wpipe->pipe_state &= ~PIPE_WANTR;
			wakeup(wpipe);
		}
		wpipe->pipe_state |= PIPE_WANTW;
		error = msleep(wpipe, PIPE_MTX(wpipe),
		    PRIBIO | PCATCH, "pipdww", 0);
		if (error)
			goto error1;
		if (wpipe->pipe_state & PIPE_EOF) {
			error = EPIPE;
			goto error1;
		}
	}
	wpipe->pipe_map.cnt = 0;	/* transfer not ready yet */
	if (wpipe->pipe_buffer.cnt > 0) {
		if (wpipe->pipe_state & PIPE_WANTR) {
			wpipe->pipe_state &= ~PIPE_WANTR;
			wakeup(wpipe);
		}

		wpipe->pipe_state |= PIPE_WANTW;
		error = msleep(wpipe, PIPE_MTX(wpipe),
		    PRIBIO | PCATCH, "pipdwc", 0);
		if (error)
			goto error1;
		if (wpipe->pipe_state & PIPE_EOF) {
			error = EPIPE;
			goto error1;
		}
		goto retry;
	}

	wpipe->pipe_state |= PIPE_DIRECTW;

	PIPE_GET_GIANT(wpipe);
	error = pipe_build_write_buffer(wpipe, uio);
	PIPE_DROP_GIANT(wpipe);
	if (error) {
		wpipe->pipe_state &= ~PIPE_DIRECTW;
		goto error1;
	}

	error = 0;
	while (!error && (wpipe->pipe_state & PIPE_DIRECTW)) {
		if (wpipe->pipe_state & PIPE_EOF) {
			pipelock(wpipe, 0);
			PIPE_GET_GIANT(wpipe);
			pipe_destroy_write_buffer(wpipe);
			PIPE_DROP_GIANT(wpipe);
			pipeunlock(wpipe);
			pipeselwakeup(wpipe, wpipe);
			error = EPIPE;
			goto error1;
		}
		if (wpipe->pipe_state & PIPE_WANTR) {
			wpipe->pipe_state &= ~PIPE_WANTR;
			wakeup(wpipe);
		}
		pipeselwakeup(wpipe, wpipe);
		error = msleep(wpipe, PIPE_MTX(wpipe), PRIBIO | PCATCH,
		    "pipdwt", 0);
	}

	pipelock(wpipe,0);
	if (wpipe->pipe_state & PIPE_DIRECTW) {
		/*
		 * this bit of trickery substitutes a kernel buffer for
		 * the process that might be going away.
		 */
		pipe_clone_write_buffer(wpipe);
	} else {
		PIPE_GET_GIANT(wpipe);
		pipe_destroy_write_buffer(wpipe);
		PIPE_DROP_GIANT(wpipe);
	}
	pipeunlock(wpipe);
	return (error);

error1:
	wakeup(wpipe);
	return (error);
}
#endif /* !PIPE_NODIRECT */
#endif /* FreeBSD */

#ifdef __NetBSD__
#ifndef PIPE_NODIRECT
/*
 * Allocate structure for loan transfer.
 */
static int
pipe_loan_alloc(wpipe, npages)
	struct pipe *wpipe;
	int npages;
{
	vsize_t len;

	len = (vsize_t)npages << PAGE_SHIFT;
	wpipe->pipe_map.kva = uvm_km_valloc_wait(kernel_map, len);
	if (wpipe->pipe_map.kva == 0)
		return (ENOMEM);

	amountpipekva += len;
	wpipe->pipe_map.npages = npages;
	wpipe->pipe_map.pgs = malloc(npages * sizeof(struct vm_page *), M_PIPE,
	    M_WAITOK);
	return (0);
}

/*
 * Free resources allocated for loan transfer.
 */
static void
pipe_loan_free(wpipe)
	struct pipe *wpipe;
{
	vsize_t len;

	len = (vsize_t)wpipe->pipe_map.npages << PAGE_SHIFT;
	uvm_km_free(kernel_map, wpipe->pipe_map.kva, len);
	wpipe->pipe_map.kva = 0;
	amountpipekva -= len;
	free(wpipe->pipe_map.pgs, M_PIPE);
	wpipe->pipe_map.pgs = NULL;
}

/*
 * NetBSD direct write, using uvm_loan() mechanism.
 * This implements the pipe buffer write mechanism.  Note that only
 * a direct write OR a normal pipe write can be pending at any given time.
 * If there are any characters in the pipe buffer, the direct write will
 * be deferred until the receiving process grabs all of the bytes from
 * the pipe buffer.  Then the direct mapping write is set-up.
 */
static int
pipe_direct_write(wpipe, uio)
	struct pipe *wpipe;
	struct uio *uio;
{
	int error, npages, j;
	struct vm_page **pgs;
	vaddr_t bbase, kva, base, bend;
	vsize_t blen, bcnt;
	voff_t bpos;

retry:
	while (wpipe->pipe_state & PIPE_DIRECTW) {
		if (wpipe->pipe_state & PIPE_WANTR) {
			wpipe->pipe_state &= ~PIPE_WANTR;
			wakeup(wpipe);
		}
		wpipe->pipe_state |= PIPE_WANTW;
		error = tsleep(wpipe, PRIBIO | PCATCH, "pipdww", 0);
		if (error)
			goto error;
		if (wpipe->pipe_state & PIPE_EOF) {
			error = EPIPE;
			goto error;
		}
	}
	wpipe->pipe_map.cnt = 0;	/* transfer not ready yet */
	if (wpipe->pipe_buffer.cnt > 0) {
		if (wpipe->pipe_state & PIPE_WANTR) {
			wpipe->pipe_state &= ~PIPE_WANTR;
			wakeup(wpipe);
		}

		wpipe->pipe_state |= PIPE_WANTW;
		error = tsleep(wpipe, PRIBIO | PCATCH, "pipdwc", 0);
		if (error)
			goto error;
		if (wpipe->pipe_state & PIPE_EOF) {
			error = EPIPE;
			goto error;
		}
		goto retry;
	}

	/*
	 * Handle first PIPE_CHUNK_SIZE bytes of buffer. Deal with buffers
	 * not aligned to PAGE_SIZE.
	 */
	bbase = (vaddr_t)uio->uio_iov->iov_base;
	base = trunc_page(bbase);
	bend = round_page(bbase + uio->uio_iov->iov_len);
	blen = bend - base;
	bpos = bbase - base;

	if (blen > PIPE_DIRECT_CHUNK) {
		blen = PIPE_DIRECT_CHUNK;
		bend = base + blen;
		bcnt = PIPE_DIRECT_CHUNK - bpos;
	} else {
		bcnt = uio->uio_iov->iov_len;
	}
	npages = blen >> PAGE_SHIFT;

	wpipe->pipe_map.pos = bpos;
	wpipe->pipe_map.cnt = bcnt;

	/*
	 * Free the old kva if we need more pages than we have
	 * allocated.
	 */
	if (wpipe->pipe_map.kva && npages > wpipe->pipe_map.npages)
		pipe_loan_free(wpipe);

	/* Allocate new kva. */
	if (wpipe->pipe_map.kva == 0) {
		error = pipe_loan_alloc(wpipe, npages);
		if (error) {
			goto error;
		}
	}

	/* Loan the write buffer memory from writer process */
	pgs = wpipe->pipe_map.pgs;
	error = uvm_loan(&uio->uio_procp->p_vmspace->vm_map, base, blen,
	    pgs, UVM_LOAN_TOPAGE);
	if (error) {
		pgs = NULL;
		goto cleanup;
	}

	/* Enter the loaned pages to kva */
	kva = wpipe->pipe_map.kva;
	for (j = 0; j < npages; j++, kva += PAGE_SIZE) {
		pmap_kenter_pa(kva, VM_PAGE_TO_PHYS(pgs[j]), VM_PROT_READ);
	}
	pmap_update(pmap_kernel());

	wpipe->pipe_state |= PIPE_DIRECTW;
	while (!error && (wpipe->pipe_state & PIPE_DIRECTW)) {
		if (wpipe->pipe_state & PIPE_EOF) {
			error = EPIPE;
			break;
		}
		if (wpipe->pipe_state & PIPE_WANTR) {
			wpipe->pipe_state &= ~PIPE_WANTR;
			wakeup(wpipe);
		}
		pipeselwakeup(wpipe, wpipe);
		error = tsleep(wpipe, PRIBIO | PCATCH, "pipdwt", 0);
	}

	if (error)
		wpipe->pipe_state &= ~PIPE_DIRECTW;

cleanup:
	pipelock(wpipe, 0);
	if (pgs != NULL) {
		pmap_kremove(wpipe->pipe_map.kva, blen);
		uvm_unloan(pgs, npages, UVM_LOAN_TOPAGE);
	}
	if (error || amountpipekva > maxpipekva)
		pipe_loan_free(wpipe);
	pipeunlock(wpipe);

	if (error) {
		pipeselwakeup(wpipe, wpipe);

		/*
		 * If nothing was read from what we offered, return error
		 * straight on. Otherwise update uio resid first. Caller
		 * will deal with the error condition, returning short
		 * write, error, or restarting the write(2) as appropriate.
		 */
		if (wpipe->pipe_map.cnt == bcnt) {
error:
			wakeup(wpipe);
			return (error);
		}

		bcnt -= wpipe->pipe_map.cnt;
	}

	uio->uio_resid -= bcnt;
	/* uio_offset not updated, not set/used for write(2) */
	uio->uio_iov->iov_base = (char *)uio->uio_iov->iov_base + bcnt;
	uio->uio_iov->iov_len -= bcnt;
	if (uio->uio_iov->iov_len == 0) {
		uio->uio_iov++;
		uio->uio_iovcnt--;
	}

	return (error);
}
#endif /* !PIPE_NODIRECT */
#endif /* NetBSD */

#ifdef __FreeBSD__
static int
pipe_write(fp, uio, cred, flags, td)
	struct file *fp;
	off_t *offset;
	struct uio *uio;
	struct ucred *cred;
	int flags;
	struct thread *td;
#elif defined(__NetBSD__)
static int
pipe_write(fp, offset, uio, cred, flags)
	struct file *fp;
	off_t *offset;
	struct uio *uio;
	struct ucred *cred;
	int flags;
#endif
{
	int error = 0;
	struct pipe *wpipe, *rpipe;

	rpipe = (struct pipe *) fp->f_data;
	wpipe = rpipe->pipe_peer;

	PIPE_LOCK(rpipe);
	/*
	 * detect loss of pipe read side, issue SIGPIPE if lost.
	 */
	if ((wpipe == NULL) || (wpipe->pipe_state & PIPE_EOF)) {
		PIPE_UNLOCK(rpipe);
		return (EPIPE);
	}

	++wpipe->pipe_busy;

	/*
	 * If it is advantageous to resize the pipe buffer, do
	 * so.
	 */
	if ((uio->uio_resid > PIPE_SIZE) &&
		(nbigpipe < maxbigpipes) &&
#ifndef PIPE_NODIRECT
		(wpipe->pipe_state & PIPE_DIRECTW) == 0 &&
#endif
		(wpipe->pipe_buffer.size <= PIPE_SIZE) &&
		(wpipe->pipe_buffer.cnt == 0)) {

		if ((error = pipelock(wpipe,1)) == 0) {
			PIPE_GET_GIANT(rpipe);
			if (pipespace(wpipe, BIG_PIPE_SIZE) == 0)
				nbigpipe++;
			PIPE_DROP_GIANT(rpipe);
			pipeunlock(wpipe);
		} else {
			/*
			 * If an error occurred, unbusy and return, waking up
			 * any waiting readers.
			 */ 
			--wpipe->pipe_busy;
			if (wpipe->pipe_busy == 0
			    && (wpipe->pipe_state & PIPE_WANTCLOSE)) {
				wpipe->pipe_state &=
				    ~(PIPE_WANTCLOSE | PIPE_WANTR);
				wakeup(wpipe);
			}

			return (error);
		}
	}

#ifdef __FreeBSD__
	/*
	 * If an early error occured unbusy and return, waking up any pending
	 * readers.
	 */
	if (error) {
		--wpipe->pipe_busy;
		if ((wpipe->pipe_busy == 0) && 
		    (wpipe->pipe_state & PIPE_WANT)) {
			wpipe->pipe_state &= ~(PIPE_WANT | PIPE_WANTR);
			wakeup(wpipe);
		}
		PIPE_UNLOCK(rpipe);
		return(error);
	}
		
	KASSERT(wpipe->pipe_buffer.buffer != NULL, ("pipe buffer gone"));
#endif

	while (uio->uio_resid) {
		int space;

#ifndef PIPE_NODIRECT
		/*
		 * If the transfer is large, we can gain performance if
		 * we do process-to-process copies directly.
		 * If the write is non-blocking, we don't use the
		 * direct write mechanism.
		 *
		 * The direct write mechanism will detect the reader going
		 * away on us.
		 */
		if ((uio->uio_iov->iov_len >= PIPE_MINDIRECT) &&
		    (fp->f_flag & FNONBLOCK) == 0 &&
		    (wpipe->pipe_map.kva || (amountpipekva < limitpipekva))) {
			error = pipe_direct_write(wpipe, uio);

			/*
			 * Break out if error occured, unless it's ENOMEM.
			 * ENOMEM means we failed to allocate some resources
			 * for direct write, so we just fallback to ordinary
			 * write. If the direct write was successful,
			 * process rest of data via ordinary write.
			 */
			if (!error)
				continue;

			if (error != ENOMEM)
				break;
		}
#endif /* PIPE_NODIRECT */

		/*
		 * Pipe buffered writes cannot be coincidental with
		 * direct writes.  We wait until the currently executing
		 * direct write is completed before we start filling the
		 * pipe buffer.  We break out if a signal occurs or the
		 * reader goes away.
		 */
	retrywrite:
		while (wpipe->pipe_state & PIPE_DIRECTW) {
			if (wpipe->pipe_state & PIPE_WANTR) {
				wpipe->pipe_state &= ~PIPE_WANTR;
				wakeup(wpipe);
			}
#ifdef __FreeBSD__
			error = msleep(wpipe, PIPE_MTX(rpipe), PRIBIO | PCATCH,
			    "pipbww", 0);
#else
			error = tsleep(wpipe, PRIBIO | PCATCH, "pipbww", 0);
#endif
			if (wpipe->pipe_state & PIPE_EOF)
				break;
			if (error)
				break;
		}
		if (wpipe->pipe_state & PIPE_EOF) {
			error = EPIPE;
			break;
		}

		space = wpipe->pipe_buffer.size - wpipe->pipe_buffer.cnt;

		/* Writes of size <= PIPE_BUF must be atomic. */
		if ((space < uio->uio_resid) && (uio->uio_resid <= PIPE_BUF))
			space = 0;

		if (space > 0) {
			int size;	/* Transfer size */
			int segsize;	/* first segment to transfer */

			if ((error = pipelock(wpipe,1)) != 0)
				break;

			/*
			 * It is possible for a direct write to
			 * slip in on us... handle it here...
			 */
			if (wpipe->pipe_state & PIPE_DIRECTW) {
				pipeunlock(wpipe);
				goto retrywrite;
			}
			/* 
			 * If a process blocked in uiomove, our
			 * value for space might be bad.
			 *
			 * XXX will we be ok if the reader has gone
			 * away here?
			 */
			if (space > wpipe->pipe_buffer.size - 
				    wpipe->pipe_buffer.cnt) {
				pipeunlock(wpipe);
				goto retrywrite;
			}

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
			segsize = wpipe->pipe_buffer.size - 
				wpipe->pipe_buffer.in;
			if (segsize > size)
				segsize = size;

			/* Transfer first segment */

			PIPE_UNLOCK(rpipe);
			error = uiomove(&wpipe->pipe_buffer.buffer[wpipe->pipe_buffer.in], 
						segsize, uio);
			PIPE_LOCK(rpipe);

			if (error == 0 && segsize < size) {
				/* 
				 * Transfer remaining part now, to
				 * support atomic writes.  Wraparound
				 * happened.
				 */
#ifdef DEBUG
				if (wpipe->pipe_buffer.in + segsize != 
				    wpipe->pipe_buffer.size)
					panic("Expected pipe buffer wraparound disappeared");
#endif

				PIPE_UNLOCK(rpipe);
				error = uiomove(&wpipe->pipe_buffer.buffer[0],
						size - segsize, uio);
				PIPE_LOCK(rpipe);
			}
			if (error == 0) {
				wpipe->pipe_buffer.in += size;
				if (wpipe->pipe_buffer.in >=
				    wpipe->pipe_buffer.size) {
#ifdef DEBUG
					if (wpipe->pipe_buffer.in != size - segsize + wpipe->pipe_buffer.size)
						panic("Expected wraparound bad");
#endif
					wpipe->pipe_buffer.in = size - segsize;
				}

				wpipe->pipe_buffer.cnt += size;
#ifdef DEBUG
				if (wpipe->pipe_buffer.cnt > wpipe->pipe_buffer.size)
					panic("Pipe buffer overflow");
#endif
			}
			pipeunlock(wpipe);
			if (error)
				break;
		} else {
			/*
			 * If the "read-side" has been blocked, wake it up now.
			 */
			if (wpipe->pipe_state & PIPE_WANTR) {
				wpipe->pipe_state &= ~PIPE_WANTR;
				wakeup(wpipe);
			}

			/*
			 * don't block on non-blocking I/O
			 */
			if (fp->f_flag & FNONBLOCK) {
				error = EAGAIN;
				break;
			}

			/*
			 * We have no more space and have something to offer,
			 * wake up select/poll.
			 */
			pipeselwakeup(wpipe, wpipe);

			wpipe->pipe_state |= PIPE_WANTW;
#ifdef __FreeBSD__
			error = msleep(wpipe, PIPE_MTX(rpipe),
			    PRIBIO | PCATCH, "pipewr", 0);
#else
			error = tsleep(wpipe, PRIBIO | PCATCH, "pipewr", 0);
#endif
			if (error != 0)
				break;
			/*
			 * If read side wants to go away, we just issue a signal
			 * to ourselves.
			 */
			if (wpipe->pipe_state & PIPE_EOF) {
				error = EPIPE;
				break;
			}
		}
	}

	--wpipe->pipe_busy;
	if ((wpipe->pipe_busy == 0) && (wpipe->pipe_state & PIPE_WANTCLOSE)) {
		wpipe->pipe_state &= ~(PIPE_WANTCLOSE | PIPE_WANTR);
		wakeup(wpipe);
	} else if (wpipe->pipe_buffer.cnt > 0) {
		/*
		 * If we have put any characters in the buffer, we wake up
		 * the reader.
		 */
		if (wpipe->pipe_state & PIPE_WANTR) {
			wpipe->pipe_state &= ~PIPE_WANTR;
			wakeup(wpipe);
		}
	}

	/*
	 * Don't return EPIPE if I/O was successful
	 */
	if ((error == EPIPE) && (wpipe->pipe_buffer.cnt == 0)
	    && (uio->uio_resid == 0))
		error = 0;

	if (error == 0)
		vfs_timestamp(&wpipe->pipe_mtime);

	/*
	 * We have something to offer, wake up select/poll.
	 * wpipe->pipe_map.cnt is always 0 in this point (direct write
	 * is only done synchronously), so check only wpipe->pipe_buffer.cnt
	 */
	if (wpipe->pipe_buffer.cnt)
		pipeselwakeup(wpipe, wpipe);

	/*
	 * Arrange for next read(2) to do a signal.
	 */
	wpipe->pipe_state |= PIPE_SIGNALR;

	PIPE_UNLOCK(rpipe);
	return (error);
}

/*
 * we implement a very minimal set of ioctls for compatibility with sockets.
 */
int
#ifdef __FreeBSD__
pipe_ioctl(fp, cmd, data, td)
	struct file *fp;
	u_long cmd;
	caddr_t data;
	struct thread *td;
#else
pipe_ioctl(fp, cmd, data, p)
	struct file *fp;
	u_long cmd;
	caddr_t data;
	struct proc *p;
#endif
{
	struct pipe *mpipe = (struct pipe *)fp->f_data;

	switch (cmd) {

	case FIONBIO:
		return (0);

	case FIOASYNC:
		PIPE_LOCK(mpipe);
		if (*(int *)data) {
			mpipe->pipe_state |= PIPE_ASYNC;
		} else {
			mpipe->pipe_state &= ~PIPE_ASYNC;
		}
		PIPE_UNLOCK(mpipe);
		return (0);

	case FIONREAD:
		PIPE_LOCK(mpipe);
#ifndef PIPE_NODIRECT
		if (mpipe->pipe_state & PIPE_DIRECTW)
			*(int *)data = mpipe->pipe_map.cnt;
		else
#endif
			*(int *)data = mpipe->pipe_buffer.cnt;
		PIPE_UNLOCK(mpipe);
		return (0);

#ifdef __FreeBSD__
	case FIOSETOWN:
		return (fsetown(*(int *)data, &mpipe->pipe_sigio));

	case FIOGETOWN:
		*(int *)data = fgetown(mpipe->pipe_sigio);
		return (0);

	/* This is deprecated, FIOSETOWN should be used instead. */
	case TIOCSPGRP:
		return (fsetown(-(*(int *)data), &mpipe->pipe_sigio));

	/* This is deprecated, FIOGETOWN should be used instead. */
	case TIOCGPGRP:
		*(int *)data = -fgetown(mpipe->pipe_sigio);
		return (0);
#endif /* FreeBSD */
#ifdef __NetBSD__
	case TIOCSPGRP:
		mpipe->pipe_pgid = *(int *)data;
		return (0);

	case TIOCGPGRP:
		*(int *)data = mpipe->pipe_pgid;
		return (0);
#endif /* NetBSD */

	}
	return (EPASSTHROUGH);
}

int
#ifdef __FreeBSD__
pipe_poll(fp, events, cred, td)
	struct file *fp;
	int events;
	struct ucred *cred;
	struct thread *td;
#elif defined(__NetBSD__)
pipe_poll(fp, events, td)
	struct file *fp;
	int events;
	struct proc *td;
#endif
{
	struct pipe *rpipe = (struct pipe *)fp->f_data;
	struct pipe *wpipe;
	int revents = 0;

	wpipe = rpipe->pipe_peer;
	PIPE_LOCK(rpipe);
	if (events & (POLLIN | POLLRDNORM))
		if ((rpipe->pipe_buffer.cnt > 0) ||
#ifndef PIPE_NODIRECT
		    (rpipe->pipe_state & PIPE_DIRECTW) ||
#endif
		    (rpipe->pipe_state & PIPE_EOF))
			revents |= events & (POLLIN | POLLRDNORM);

	if (events & (POLLOUT | POLLWRNORM))
		if (wpipe == NULL || (wpipe->pipe_state & PIPE_EOF)
		    || (
#ifndef PIPE_NODIRECT
		     ((wpipe->pipe_state & PIPE_DIRECTW) == 0) &&
#endif
		     (wpipe->pipe_buffer.size - wpipe->pipe_buffer.cnt) >= PIPE_BUF))
			revents |= events & (POLLOUT | POLLWRNORM);

	if ((rpipe->pipe_state & PIPE_EOF) ||
	    (wpipe == NULL) ||
	    (wpipe->pipe_state & PIPE_EOF))
		revents |= POLLHUP;

	if (revents == 0) {
		if (events & (POLLIN | POLLRDNORM)) {
			selrecord(td, &rpipe->pipe_sel);
			rpipe->pipe_state |= PIPE_SEL;
		}

		if (events & (POLLOUT | POLLWRNORM)) {
			selrecord(td, &wpipe->pipe_sel);
			wpipe->pipe_state |= PIPE_SEL;
		}
	}
	PIPE_UNLOCK(rpipe);

	return (revents);
}

static int
#ifdef __FreeBSD__
pipe_stat(fp, ub, td)
	struct file *fp;
	struct stat *ub;
	struct thread *td;
#else
pipe_stat(fp, ub, td)
	struct file *fp;
	struct stat *ub;
	struct proc *td;
#endif
{
	struct pipe *pipe = (struct pipe *)fp->f_data;

	memset((caddr_t)ub, 0, sizeof(*ub));
	ub->st_mode = S_IFIFO;
	ub->st_blksize = pipe->pipe_buffer.size;
	ub->st_size = pipe->pipe_buffer.cnt;
	ub->st_blocks = (ub->st_size) ? 1 : 0;
#ifdef __FreeBSD__
	ub->st_atimespec = pipe->pipe_atime;
	ub->st_mtimespec = pipe->pipe_mtime;
	ub->st_ctimespec = pipe->pipe_ctime;
#endif /* FreeBSD */
#ifdef __NetBSD__
	TIMEVAL_TO_TIMESPEC(&pipe->pipe_atime, &ub->st_atimespec)
	TIMEVAL_TO_TIMESPEC(&pipe->pipe_mtime, &ub->st_mtimespec);
	TIMEVAL_TO_TIMESPEC(&pipe->pipe_ctime, &ub->st_ctimespec);
#endif /* NetBSD */
	ub->st_uid = fp->f_cred->cr_uid;
	ub->st_gid = fp->f_cred->cr_gid;
	/*
	 * Left as 0: st_dev, st_ino, st_nlink, st_rdev, st_flags, st_gen.
	 * XXX (st_dev, st_ino) should be unique.
	 */
	return (0);
}

/* ARGSUSED */
static int
#ifdef __FreeBSD__
pipe_close(fp, td)
	struct file *fp;
	struct thread *td;
#else
pipe_close(fp, td)
	struct file *fp;
	struct proc *td;
#endif
{
	struct pipe *cpipe = (struct pipe *)fp->f_data;

#ifdef __FreeBSD__
	fp->f_ops = &badfileops;
	funsetown(cpipe->pipe_sigio);
#endif
	fp->f_data = NULL;
	pipeclose(cpipe);
	return (0);
}

static void
pipe_free_kmem(cpipe)
	struct pipe *cpipe;
{

#ifdef __FreeBSD__

	GIANT_REQUIRED;
	KASSERT(cpipe->pipe_mtxp == NULL || !mtx_owned(PIPE_MTX(cpipe)),
	       ("pipespace: pipe mutex locked"));
#endif

	if (cpipe->pipe_buffer.buffer != NULL) {
		if (cpipe->pipe_buffer.size > PIPE_SIZE)
			--nbigpipe;
		amountpipekva -= cpipe->pipe_buffer.size;
#ifdef __FreeBSD__
		kmem_free(kernel_map,
			(vm_offset_t)cpipe->pipe_buffer.buffer,
			cpipe->pipe_buffer.size);
#elif defined(__NetBSD__)
		uvm_km_free(kernel_map,
			(vaddr_t)cpipe->pipe_buffer.buffer,
			cpipe->pipe_buffer.size);
#endif /* NetBSD */
		cpipe->pipe_buffer.buffer = NULL;
	}
#ifndef PIPE_NODIRECT
	if (cpipe->pipe_map.kva != 0) {
#ifdef __FreeBSD__
		amountpipekva -= cpipe->pipe_buffer.size + PAGE_SIZE;
		kmem_free(kernel_map,
			cpipe->pipe_map.kva,
			cpipe->pipe_buffer.size + PAGE_SIZE);
#elif defined(__NetBSD__)
		pipe_loan_free(cpipe);
#endif /* NetBSD */
		cpipe->pipe_map.cnt = 0;
		cpipe->pipe_map.kva = 0;
		cpipe->pipe_map.pos = 0;
		cpipe->pipe_map.npages = 0;
	}
#endif /* !PIPE_NODIRECT */
}

/*
 * shutdown the pipe
 */
static void
pipeclose(cpipe)
	struct pipe *cpipe;
{
	struct pipe *ppipe;
#ifdef __FreeBSD__
	int hadpeer = 0;
#endif

	if (cpipe == NULL)
		return;

	/* partially created pipes won't have a valid mutex. */
	if (PIPE_MTX(cpipe) != NULL)
		PIPE_LOCK(cpipe);
		
	pipeselwakeup(cpipe, cpipe);

	/*
	 * If the other side is blocked, wake it up saying that
	 * we want to close it down.
	 */
	while (cpipe->pipe_busy) {
		wakeup(cpipe);
		cpipe->pipe_state |= PIPE_WANTCLOSE | PIPE_EOF;
#ifdef __FreeBSD__
		msleep(cpipe, PIPE_MTX(cpipe), PRIBIO, "pipecl", 0);
#else
		tsleep(cpipe, PRIBIO, "pipecl", 0);
#endif
	}

	/*
	 * Disconnect from peer
	 */
	if ((ppipe = cpipe->pipe_peer) != NULL) {
#ifdef __FreeBSD__
		hadpeer++;
#endif
		pipeselwakeup(ppipe, ppipe);

		ppipe->pipe_state |= PIPE_EOF;
		wakeup(ppipe);
#ifdef __FreeBSD__
		KNOTE(&ppipe->pipe_sel.si_note, 0);
#endif
		ppipe->pipe_peer = NULL;
	}
	/*
	 * free resources
	 */
#ifdef __FreeBSD__
	if (PIPE_MTX(cpipe) != NULL) {
		PIPE_UNLOCK(cpipe);
		if (!hadpeer) {
			mtx_destroy(PIPE_MTX(cpipe));
			free(PIPE_MTX(cpipe), M_TEMP);
		}
	}
	mtx_lock(&Giant);
	pipe_free_kmem(cpipe);
	zfree(pipe_zone, cpipe);
	mtx_unlock(&Giant);
#endif

#ifdef __NetBSD__
	if (PIPE_MTX(cpipe) != NULL)
		PIPE_UNLOCK(cpipe);

	pipe_free_kmem(cpipe);
	(void) lockmgr(&cpipe->pipe_lock, LK_DRAIN, NULL);
	pool_put(&pipe_pool, cpipe);
#endif
}

#ifdef __FreeBSD__
/*ARGSUSED*/
static int
pipe_kqfilter(struct file *fp, struct knote *kn)
{
	struct pipe *cpipe;

	cpipe = (struct pipe *)kn->kn_fp->f_data;
	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &pipe_rfiltops;
		break;
	case EVFILT_WRITE:
		kn->kn_fop = &pipe_wfiltops;
		cpipe = cpipe->pipe_peer;
		break;
	default:
		return (1);
	}
	kn->kn_hook = (caddr_t)cpipe;

	PIPE_LOCK(cpipe);
	SLIST_INSERT_HEAD(&cpipe->pipe_sel.si_note, kn, kn_selnext);
	PIPE_UNLOCK(cpipe);
	return (0);
}

static void
filt_pipedetach(struct knote *kn)
{
	struct pipe *cpipe = (struct pipe *)kn->kn_fp->f_data;

	PIPE_LOCK(cpipe);
	SLIST_REMOVE(&cpipe->pipe_sel.si_note, kn, knote, kn_selnext);
	PIPE_UNLOCK(cpipe);
}

/*ARGSUSED*/
static int
filt_piperead(struct knote *kn, long hint)
{
	struct pipe *rpipe = (struct pipe *)kn->kn_fp->f_data;
	struct pipe *wpipe = rpipe->pipe_peer;

	PIPE_LOCK(rpipe);
	kn->kn_data = rpipe->pipe_buffer.cnt;
	if ((kn->kn_data == 0) && (rpipe->pipe_state & PIPE_DIRECTW))
		kn->kn_data = rpipe->pipe_map.cnt;

	if ((rpipe->pipe_state & PIPE_EOF) ||
	    (wpipe == NULL) || (wpipe->pipe_state & PIPE_EOF)) {
		kn->kn_flags |= EV_EOF;
		PIPE_UNLOCK(rpipe);
		return (1);
	}
	PIPE_UNLOCK(rpipe);
	return (kn->kn_data > 0);
}

/*ARGSUSED*/
static int
filt_pipewrite(struct knote *kn, long hint)
{
	struct pipe *rpipe = (struct pipe *)kn->kn_fp->f_data;
	struct pipe *wpipe = rpipe->pipe_peer;

	PIPE_LOCK(rpipe);
	if ((wpipe == NULL) || (wpipe->pipe_state & PIPE_EOF)) {
		kn->kn_data = 0;
		kn->kn_flags |= EV_EOF; 
		PIPE_UNLOCK(rpipe);
		return (1);
	}
	kn->kn_data = wpipe->pipe_buffer.size - wpipe->pipe_buffer.cnt;
	if (wpipe->pipe_state & PIPE_DIRECTW)
		kn->kn_data = 0;

	PIPE_UNLOCK(rpipe);
	return (kn->kn_data >= PIPE_BUF);
}
#endif /* FreeBSD */

#ifdef __NetBSD__
static int
pipe_fcntl(fp, cmd, data, p)
	struct file *fp;
	u_int cmd;
	caddr_t data;
	struct proc *p;
{
	if (cmd == F_SETFL)
		return (0);
	else
		return (EOPNOTSUPP);
}

/*
 * Handle pipe sysctls.
 */
int
sysctl_dopipe(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case KERN_PIPE_MAXKVASZ:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &maxpipekva));
	case KERN_PIPE_LIMITKVA:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &limitpipekva));
	case KERN_PIPE_MAXBIGPIPES:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &maxbigpipes));
	case KERN_PIPE_NBIGPIPES:
		return (sysctl_rdint(oldp, oldlenp, newp, nbigpipe));
	case KERN_PIPE_KVASIZE:
		return (sysctl_rdint(oldp, oldlenp, newp, amountpipekva));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * Initialize pipe structs.
 */
void
pipe_init(void)
{
	pool_init(&pipe_pool, sizeof(struct pipe), 0, 0, 0, "pipepl", NULL);
}

#endif /* __NetBSD __ */
