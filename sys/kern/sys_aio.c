/*	$NetBSD: sys_aio.c,v 1.7.2.5 2008/01/21 09:46:22 yamt Exp $	*/

/*
 * Copyright (c) 2007, Mindaugas Rasiukevicius <rmind at NetBSD org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
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
 * TODO:
 *   1. Additional work for VCHR and maybe VBLK devices.
 *   2. Consider making the job-finding O(n) per one file descriptor.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_aio.c,v 1.7.2.5 2008/01/21 09:46:22 yamt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/atomic.h>

#include <uvm/uvm_extern.h>

/*
 * System-wide limits and counter of AIO operations.
 */
static u_int aio_listio_max = AIO_LISTIO_MAX;
static u_int aio_max = AIO_MAX;
static u_int aio_jobs_count;

static struct pool aio_job_pool;
static struct pool aio_lio_pool;

/* Prototypes */
void aio_worker(void *);
static void aio_process(struct aio_job *);
static void aio_sendsig(struct proc *, struct sigevent *);
static int aio_enqueue_job(int, void *, struct lio_req *);

/*
 * Initialize the AIO system.
 */
void
aio_sysinit(void)
{

	pool_init(&aio_job_pool, sizeof(struct aio_job), 0, 0, 0,
	    "aio_jobs_pool", &pool_allocator_nointr, IPL_NONE);
	pool_init(&aio_lio_pool, sizeof(struct lio_req), 0, 0, 0,
	    "aio_lio_pool", &pool_allocator_nointr, IPL_NONE);
}

/*
 * Initialize Asynchronous I/O data structures for the process.
 */
int
aio_init(struct proc *p)
{
	struct aioproc *aio;
	struct lwp *l;
	int error;
	bool inmem;
	vaddr_t uaddr;

	/* Allocate and initialize AIO structure */
	aio = kmem_zalloc(sizeof(struct aioproc), KM_SLEEP);
	if (aio == NULL)
		return EAGAIN;

	/* Initialize queue and their synchronization structures */
	mutex_init(&aio->aio_mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&aio->aio_worker_cv, "aiowork");
	cv_init(&aio->done_cv, "aiodone");
	TAILQ_INIT(&aio->jobs_queue);

	/*
	 * Create an AIO worker thread.
	 * XXX: Currently, AIO thread is not protected against user's actions.
	 */
	inmem = uvm_uarea_alloc(&uaddr);
	if (uaddr == 0) {
		aio_exit(p, aio);
		return EAGAIN;
	}
	error = lwp_create(curlwp, p, uaddr, inmem, 0, NULL, 0, aio_worker,
	    NULL, &l, curlwp->l_class);
	if (error != 0) {
		uvm_uarea_free(uaddr, curcpu());
		aio_exit(p, aio);
		return error;
	}

	/* Recheck if we are really first */
	mutex_enter(&p->p_mutex);
	if (p->p_aio) {
		mutex_exit(&p->p_mutex);
		aio_exit(p, aio);
		lwp_exit(l);
		return 0;
	}
	p->p_aio = aio;
	mutex_exit(&p->p_mutex);

	/* Complete the initialization of thread, and run it */
	mutex_enter(&p->p_smutex);
	aio->aio_worker = l;
	p->p_nrlwps++;
	lwp_lock(l);
	l->l_stat = LSRUN;
	l->l_priority = MAXPRI_USER;
	sched_enqueue(l, false);
	lwp_unlock(l);
	mutex_exit(&p->p_smutex);

	return 0;
}

/*
 * Exit of Asynchronous I/O subsystem of process.
 */
void
aio_exit(struct proc *p, struct aioproc *aio)
{
	struct aio_job *a_job;

	if (aio == NULL)
		return;

	/* Free AIO queue */
	while (!TAILQ_EMPTY(&aio->jobs_queue)) {
		a_job = TAILQ_FIRST(&aio->jobs_queue);
		TAILQ_REMOVE(&aio->jobs_queue, a_job, list);
		pool_put(&aio_job_pool, a_job);
		atomic_dec_uint(&aio_jobs_count);
	}

	/* Destroy and free the entire AIO data structure */
	cv_destroy(&aio->aio_worker_cv);
	cv_destroy(&aio->done_cv);
	mutex_destroy(&aio->aio_mtx);
	kmem_free(aio, sizeof(struct aioproc));
}

/*
 * AIO worker thread and processor.
 */
void
aio_worker(void *arg)
{
	struct proc *p = curlwp->l_proc;
	struct aioproc *aio = p->p_aio;
	struct aio_job *a_job;
	struct lio_req *lio;
	sigset_t oss, nss;
	int error, refcnt;

	/*
	 * Make an empty signal mask, so it
	 * handles only SIGKILL and SIGSTOP.
	 */
	sigfillset(&nss);
	mutex_enter(&p->p_smutex);
	error = sigprocmask1(curlwp, SIG_SETMASK, &nss, &oss);
	mutex_exit(&p->p_smutex);
	KASSERT(error == 0);

	for (;;) {
		/*
		 * Loop for each job in the queue.  If there
		 * are no jobs then sleep.
		 */
		mutex_enter(&aio->aio_mtx);
		while ((a_job = TAILQ_FIRST(&aio->jobs_queue)) == NULL) {
			if (cv_wait_sig(&aio->aio_worker_cv, &aio->aio_mtx)) {
				/*
				 * Thread was interrupted - check for
				 * pending exit or suspend.
				 */
				mutex_exit(&aio->aio_mtx);
				lwp_userret(curlwp);
				mutex_enter(&aio->aio_mtx);
			}
		}

		/* Take the job from the queue */
		aio->curjob = a_job;
		TAILQ_REMOVE(&aio->jobs_queue, a_job, list);

		atomic_dec_uint(&aio_jobs_count);
		aio->jobs_count--;

		mutex_exit(&aio->aio_mtx);

		/* Process an AIO operation */
		aio_process(a_job);

		/* Copy data structure back to the user-space */
		(void)copyout(&a_job->aiocbp, a_job->aiocb_uptr,
		    sizeof(struct aiocb));

		mutex_enter(&aio->aio_mtx);
		aio->curjob = NULL;

		/* Decrease a reference counter, if there is a LIO structure */
		lio = a_job->lio;
		refcnt = (lio != NULL ? --lio->refcnt : -1);

		/* Notify all suspenders */
		cv_broadcast(&aio->done_cv);
		mutex_exit(&aio->aio_mtx);

		/* Send a signal, if any */
		aio_sendsig(p, &a_job->aiocbp.aio_sigevent);

		/* Destroy the LIO structure */
		if (refcnt == 0) {
			aio_sendsig(p, &lio->sig);
			pool_put(&aio_lio_pool, lio);
		}

		/* Destroy the the job */
		pool_put(&aio_job_pool, a_job);
	}

	/* NOTREACHED */
}

static void
aio_process(struct aio_job *a_job)
{
	struct proc *p = curlwp->l_proc;
	struct aiocb *aiocbp = &a_job->aiocbp;
	struct file *fp;
	struct filedesc	*fdp = p->p_fd;
	int fd = aiocbp->aio_fildes;
	int error = 0;

	KASSERT(fdp != NULL);
	KASSERT(a_job->aio_op != 0);

	if ((a_job->aio_op & (AIO_READ | AIO_WRITE)) != 0) {
		struct iovec aiov;
		struct uio auio;

		if (aiocbp->aio_nbytes > SSIZE_MAX) {
			error = EINVAL;
			goto done;
		}

		fp = fd_getfile(fdp, fd);
		if (fp == NULL) {
			error = EBADF;
			goto done;
		}

		aiov.iov_base = (void *)(uintptr_t)aiocbp->aio_buf;
		aiov.iov_len = aiocbp->aio_nbytes;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_resid = aiocbp->aio_nbytes;
		auio.uio_vmspace = p->p_vmspace;

		FILE_USE(fp);
		if (a_job->aio_op & AIO_READ) {
			/*
			 * Perform a Read operation
			 */
			KASSERT((a_job->aio_op & AIO_WRITE) == 0);

			if ((fp->f_flag & FREAD) == 0) {
				FILE_UNUSE(fp, curlwp);
				error = EBADF;
				goto done;
			}
			auio.uio_rw = UIO_READ;
			error = (*fp->f_ops->fo_read)(fp, &aiocbp->aio_offset,
			    &auio, fp->f_cred, FOF_UPDATE_OFFSET);
		} else {
			/*
			 * Perform a Write operation
			 */
			KASSERT(a_job->aio_op & AIO_WRITE);

			if ((fp->f_flag & FWRITE) == 0) {
				FILE_UNUSE(fp, curlwp);
				error = EBADF;
				goto done;
			}
			auio.uio_rw = UIO_WRITE;
			error = (*fp->f_ops->fo_write)(fp, &aiocbp->aio_offset,
			    &auio, fp->f_cred, FOF_UPDATE_OFFSET);
		}
		FILE_UNUSE(fp, curlwp);

		/* Store the result value */
		a_job->aiocbp.aio_nbytes -= auio.uio_resid;
		a_job->aiocbp._retval = (error == 0) ?
		    a_job->aiocbp.aio_nbytes : -1;

	} else if ((a_job->aio_op & (AIO_SYNC | AIO_DSYNC)) != 0) {
		/*
		 * Perform a file Sync operation
		 */
		struct vnode *vp;

		if ((error = getvnode(fdp, fd, &fp)) != 0)
			goto done; 

		if ((fp->f_flag & FWRITE) == 0) {
			FILE_UNUSE(fp, curlwp);
			error = EBADF;
			goto done;
		}

		vp = (struct vnode *)fp->f_data;
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		if (a_job->aio_op & AIO_DSYNC) {
			error = VOP_FSYNC(vp, fp->f_cred,
			    FSYNC_WAIT | FSYNC_DATAONLY, 0, 0);
		} else if (a_job->aio_op & AIO_SYNC) {
			error = VOP_FSYNC(vp, fp->f_cred,
			    FSYNC_WAIT, 0, 0);
			if (error == 0 && bioopsp != NULL &&
			    vp->v_mount &&
			    (vp->v_mount->mnt_flag & MNT_SOFTDEP))
			    bioopsp->io_fsync(vp, 0);
		}
		VOP_UNLOCK(vp, 0);
		FILE_UNUSE(fp, curlwp);

		/* Store the result value */
		a_job->aiocbp._retval = (error == 0) ? 0 : -1;

	} else
		panic("aio_process: invalid operation code\n");

done:
	/* Job is done, set the error, if any */
	a_job->aiocbp._errno = error;
	a_job->aiocbp._state = JOB_DONE;
}

/*
 * Send AIO signal.
 */
static void
aio_sendsig(struct proc *p, struct sigevent *sig)
{
	ksiginfo_t ksi;

	if (sig->sigev_signo == 0 || sig->sigev_notify == SIGEV_NONE)
		return;

	KSI_INIT(&ksi);
	ksi.ksi_signo = sig->sigev_signo;
	ksi.ksi_code = SI_ASYNCIO;
	ksi.ksi_value = sig->sigev_value;
	mutex_enter(&proclist_mutex);
	kpsignal(p, &ksi, NULL);
	mutex_exit(&proclist_mutex);
}

/*
 * Enqueue the job.
 */
static int
aio_enqueue_job(int op, void *aiocb_uptr, struct lio_req *lio)
{
	struct proc *p = curlwp->l_proc;
	struct aioproc *aio;
	struct aio_job *a_job;
	struct aiocb aiocbp;
	struct sigevent *sig;
	int error;

	/* Non-accurate check for the limit */
	if (aio_jobs_count + 1 > aio_max)
		return EAGAIN;

	/* Get the data structure from user-space */
	error = copyin(aiocb_uptr, &aiocbp, sizeof(struct aiocb));
	if (error)
		return error;

	/* Check if signal is set, and validate it */
	sig = &aiocbp.aio_sigevent;
	if (sig->sigev_signo < 0 || sig->sigev_signo >= NSIG ||
	    sig->sigev_notify < SIGEV_NONE || sig->sigev_notify > SIGEV_SA)
		return EINVAL;

	/* Buffer and byte count */
	if (((AIO_SYNC | AIO_DSYNC) & op) == 0)
		if (aiocbp.aio_buf == NULL || aiocbp.aio_nbytes > SSIZE_MAX)
			return EINVAL;

	/* Check the opcode, if LIO_NOP - simply ignore */
	if (op == AIO_LIO) {
		KASSERT(lio != NULL);
		if (aiocbp.aio_lio_opcode == LIO_WRITE)
			op = AIO_WRITE;
		else if (aiocbp.aio_lio_opcode == LIO_READ)
			op = AIO_READ;
		else
			return (aiocbp.aio_lio_opcode == LIO_NOP) ? 0 : EINVAL;
	} else {
		KASSERT(lio == NULL);
	}

	/*
	 * Look for already existing job.  If found - the job is in-progress.
	 * According to POSIX this is invalid, so return the error.
	 */
	aio = p->p_aio;
	if (aio) {
		mutex_enter(&aio->aio_mtx);
		if (aio->curjob) {
			a_job = aio->curjob;
			if (a_job->aiocb_uptr == aiocb_uptr) {
				mutex_exit(&aio->aio_mtx);
				return EINVAL;
			}
		}
		TAILQ_FOREACH(a_job, &aio->jobs_queue, list) {
			if (a_job->aiocb_uptr != aiocb_uptr)
				continue;
			mutex_exit(&aio->aio_mtx);
			return EINVAL;
		}
		mutex_exit(&aio->aio_mtx);
	}

	/*
	 * Check if AIO structure is initialized, if not - initialize it.
	 * In LIO case, we did that already.  We will recheck this with
	 * the lock in aio_init().
	 */
	if (lio == NULL && p->p_aio == NULL)
		if (aio_init(p))
			return EAGAIN;
	aio = p->p_aio;

	/*
	 * Set the state with errno, and copy data
	 * structure back to the user-space.
	 */
	aiocbp._state = JOB_WIP;
	aiocbp._errno = EINPROGRESS;
	aiocbp._retval = -1;
	error = copyout(&aiocbp, aiocb_uptr, sizeof(struct aiocb));
	if (error)
		return error;

	/* Allocate and initialize a new AIO job */
	a_job = pool_get(&aio_job_pool, PR_WAITOK);
	memset(a_job, 0, sizeof(struct aio_job));

	/*
	 * Set the data.
	 * Store the user-space pointer for searching.  Since we
	 * are storing only per proc pointers - it is safe.
	 */
	memcpy(&a_job->aiocbp, &aiocbp, sizeof(struct aiocb));
	a_job->aiocb_uptr = aiocb_uptr;
	a_job->aio_op |= op;
	a_job->lio = lio;

	/*
	 * Add the job to the queue, update the counters, and
	 * notify the AIO worker thread to handle the job.
	 */
	mutex_enter(&aio->aio_mtx);

	/* Fail, if the limit was reached */
	if (atomic_inc_uint_nv(&aio_jobs_count) > aio_max ||
	    aio->jobs_count >= aio_listio_max) {
		atomic_dec_uint(&aio_jobs_count);
		mutex_exit(&aio->aio_mtx);
		pool_put(&aio_job_pool, a_job);
		return EAGAIN;
	}

	TAILQ_INSERT_TAIL(&aio->jobs_queue, a_job, list);
	aio->jobs_count++;
	if (lio)
		lio->refcnt++;
	cv_signal(&aio->aio_worker_cv);

	mutex_exit(&aio->aio_mtx);

	/*
	 * One would handle the errors only with aio_error() function.
	 * This way is appropriate according to POSIX.
	 */
	return 0;
}

/*
 * Syscall functions.
 */

int
sys_aio_cancel(struct lwp *l, const struct sys_aio_cancel_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fildes;
		syscallarg(struct aiocb *) aiocbp;
	} */
	struct proc *p = l->l_proc;
	struct aioproc *aio;
	struct aio_job *a_job;
	struct aiocb *aiocbp_ptr;
	struct lio_req *lio;
	struct filedesc	*fdp = p->p_fd;
	unsigned int cn, errcnt, fildes;

	TAILQ_HEAD(, aio_job) tmp_jobs_list;

	/* Check for invalid file descriptor */
	fildes = (unsigned int)SCARG(uap, fildes);
	if (fildes >= fdp->fd_nfiles || fdp->fd_ofiles[fildes] == NULL)
		return EBADF;

	/* Check if AIO structure is initialized */
	if (p->p_aio == NULL) {
		*retval = AIO_NOTCANCELED;
		return 0;
	}

	aio = p->p_aio;
	aiocbp_ptr = (struct aiocb *)SCARG(uap, aiocbp);

	mutex_enter(&aio->aio_mtx);

	/* Cancel the jobs, and remove them from the queue */
	cn = 0;
	TAILQ_INIT(&tmp_jobs_list);
	TAILQ_FOREACH(a_job, &aio->jobs_queue, list) {
		if (aiocbp_ptr) {
			if (aiocbp_ptr != a_job->aiocb_uptr)
				continue;
			if (fildes != a_job->aiocbp.aio_fildes) {
				mutex_exit(&aio->aio_mtx);
				return EBADF;
			}
		} else if (a_job->aiocbp.aio_fildes != fildes)
			continue;

		TAILQ_REMOVE(&aio->jobs_queue, a_job, list);
		TAILQ_INSERT_TAIL(&tmp_jobs_list, a_job, list);

		/* Decrease the counters */
		atomic_dec_uint(&aio_jobs_count);
		aio->jobs_count--;
		lio = a_job->lio;
		if (lio != NULL && --lio->refcnt != 0)
			a_job->lio = NULL;

		cn++;
		if (aiocbp_ptr)
			break;
	}

	/* There are canceled jobs */
	if (cn)
		*retval = AIO_CANCELED;

	/* We cannot cancel current job */
	a_job = aio->curjob;
	if (a_job && ((a_job->aiocbp.aio_fildes == fildes) ||
	    (a_job->aiocb_uptr == aiocbp_ptr)))
		*retval = AIO_NOTCANCELED;

	mutex_exit(&aio->aio_mtx);

	/* Free the jobs after the lock */
	errcnt = 0;
	while (!TAILQ_EMPTY(&tmp_jobs_list)) {
		a_job = TAILQ_FIRST(&tmp_jobs_list);
		TAILQ_REMOVE(&tmp_jobs_list, a_job, list);
		/* Set the errno and copy structures back to the user-space */
		a_job->aiocbp._errno = ECANCELED;
		a_job->aiocbp._state = JOB_DONE;
		if (copyout(&a_job->aiocbp, a_job->aiocb_uptr,
		    sizeof(struct aiocb)))
			errcnt++;
		/* Send a signal if any */
		aio_sendsig(p, &a_job->aiocbp.aio_sigevent);
		if (a_job->lio) {
			lio = a_job->lio;
			aio_sendsig(p, &lio->sig);
			pool_put(&aio_lio_pool, lio);
		}
		pool_put(&aio_job_pool, a_job);
	}

	if (errcnt)
		return EFAULT;

	/* Set a correct return value */
	if (*retval == 0)
		*retval = AIO_ALLDONE;

	return 0;
}

int
sys_aio_error(struct lwp *l, const struct sys_aio_error_args *uap, register_t *retval)
{
	/* {
		syscallarg(const struct aiocb *) aiocbp;
	} */
	struct proc *p = l->l_proc;
	struct aioproc *aio = p->p_aio;
	struct aiocb aiocbp;
	int error;

	if (aio == NULL)
		return EINVAL;

	error = copyin(SCARG(uap, aiocbp), &aiocbp, sizeof(struct aiocb));
	if (error)
		return error;

	if (aiocbp._state == JOB_NONE)
		return EINVAL;

	*retval = aiocbp._errno;

	return 0;
}

int
sys_aio_fsync(struct lwp *l, const struct sys_aio_fsync_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) op;
		syscallarg(struct aiocb *) aiocbp;
	} */
	int op = SCARG(uap, op);

	if ((op != O_DSYNC) && (op != O_SYNC))
		return EINVAL;

	op = O_DSYNC ? AIO_DSYNC : AIO_SYNC;

	return aio_enqueue_job(op, SCARG(uap, aiocbp), NULL);
}

int
sys_aio_read(struct lwp *l, const struct sys_aio_read_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct aiocb *) aiocbp;
	} */

	return aio_enqueue_job(AIO_READ, SCARG(uap, aiocbp), NULL);
}

int
sys_aio_return(struct lwp *l, const struct sys_aio_return_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct aiocb *) aiocbp;
	} */
	struct proc *p = l->l_proc;
	struct aioproc *aio = p->p_aio;
	struct aiocb aiocbp;
	int error;

	if (aio == NULL)
		return EINVAL;

	error = copyin(SCARG(uap, aiocbp), &aiocbp, sizeof(struct aiocb));
	if (error)
		return error;

	if (aiocbp._errno == EINPROGRESS || aiocbp._state != JOB_DONE)
		return EINVAL;

	*retval = aiocbp._retval;

	/* Reset the internal variables */
	aiocbp._errno = 0;
	aiocbp._retval = -1;
	aiocbp._state = JOB_NONE;
	error = copyout(&aiocbp, SCARG(uap, aiocbp), sizeof(struct aiocb));

	return error;
}

int
sys_aio_suspend(struct lwp *l, const struct sys_aio_suspend_args *uap, register_t *retval)
{
	/* {
		syscallarg(const struct aiocb *const[]) list;
		syscallarg(int) nent;
		syscallarg(const struct timespec *) timeout;
	} */
	struct proc *p = l->l_proc;
	struct aioproc *aio;
	struct aio_job *a_job;
	struct aiocb **aiocbp_list;
	struct timespec ts;
	int i, error, nent, timo;

	if (p->p_aio == NULL)
		return EAGAIN;
	aio = p->p_aio;

	nent = SCARG(uap, nent);
	if (nent <= 0 || nent > aio_listio_max)
		return EAGAIN;

	if (SCARG(uap, timeout)) {
		/* Convert timespec to ticks */
		error = copyin(SCARG(uap, timeout), &ts,
		    sizeof(struct timespec));
		if (error)
			return error;
		timo = mstohz((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
		if (timo == 0 && ts.tv_sec == 0 && ts.tv_nsec > 0)
			timo = 1;
		if (timo <= 0)
			return EAGAIN;
	} else
		timo = 0;

	/* Get the list from user-space */
	aiocbp_list = kmem_zalloc(nent * sizeof(struct aio_job), KM_SLEEP);
	error = copyin(SCARG(uap, list), aiocbp_list,
	    nent * sizeof(struct aiocb));
	if (error) {
		kmem_free(aiocbp_list, nent * sizeof(struct aio_job));
		return error;
	}

	mutex_enter(&aio->aio_mtx);
	for (;;) {

		for (i = 0; i < nent; i++) {

			/* Skip NULL entries */
			if (aiocbp_list[i] == NULL)
				continue;

			/* Skip current job */
			if (aio->curjob) {
				a_job = aio->curjob;
				if (a_job->aiocb_uptr == aiocbp_list[i])
					continue;
			}

			/* Look for a job in the queue */
			TAILQ_FOREACH(a_job, &aio->jobs_queue, list)
				if (a_job->aiocb_uptr == aiocbp_list[i])
					break;

			if (a_job == NULL) {
				struct aiocb aiocbp;

				mutex_exit(&aio->aio_mtx);

				error = copyin(aiocbp_list[i], &aiocbp,
				    sizeof(struct aiocb));
				if (error == 0 && aiocbp._state != JOB_DONE) {
					mutex_enter(&aio->aio_mtx);
					continue;
				}

				kmem_free(aiocbp_list,
				    nent * sizeof(struct aio_job));
				return error;
			}
		}

		/* Wait for a signal or when timeout occurs */
		error = cv_timedwait_sig(&aio->done_cv, &aio->aio_mtx, timo);
		if (error) {
			if (error == EWOULDBLOCK)
				error = EAGAIN;
			break;
		}
	}
	mutex_exit(&aio->aio_mtx);

	kmem_free(aiocbp_list, nent * sizeof(struct aio_job));
	return error;
}

int
sys_aio_write(struct lwp *l, const struct sys_aio_write_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct aiocb *) aiocbp;
	} */

	return aio_enqueue_job(AIO_WRITE, SCARG(uap, aiocbp), NULL);
}

int
sys_lio_listio(struct lwp *l, const struct sys_lio_listio_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) mode;
		syscallarg(struct aiocb *const[]) list;
		syscallarg(int) nent;
		syscallarg(struct sigevent *) sig;
	} */
	struct proc *p = l->l_proc;
	struct aioproc *aio;
	struct aiocb **aiocbp_list;
	struct lio_req *lio;
	int i, error, errcnt, mode, nent;

	mode = SCARG(uap, mode);
	nent = SCARG(uap, nent);

	/* Non-accurate checks for the limit and invalid values */
	if (nent < 1 || nent > aio_listio_max)
		return EINVAL;
	if (aio_jobs_count + nent > aio_max)
		return EAGAIN;

	/* Check if AIO structure is initialized, if not - initialize it */
	if (p->p_aio == NULL)
		if (aio_init(p))
			return EAGAIN;
	aio = p->p_aio;

	/* Create a LIO structure */
	lio = pool_get(&aio_lio_pool, PR_WAITOK);
	lio->refcnt = 1;
	error = 0;

	switch (mode) {
	case LIO_WAIT:
		memset(&lio->sig, 0, sizeof(struct sigevent));
		break;
	case LIO_NOWAIT:
		/* Check for signal, validate it */
		if (SCARG(uap, sig)) {
			struct sigevent *sig = &lio->sig;

			error = copyin(SCARG(uap, sig), &lio->sig,
			    sizeof(struct sigevent));
			if (error == 0 &&
			    (sig->sigev_signo < 0 ||
			    sig->sigev_signo >= NSIG ||
			    sig->sigev_notify < SIGEV_NONE ||
			    sig->sigev_notify > SIGEV_SA))
				error = EINVAL;
		} else
			memset(&lio->sig, 0, sizeof(struct sigevent));
		break;
	default:
		error = EINVAL;
		break;
	}

	if (error != 0) {
		pool_put(&aio_lio_pool, lio);
		return error;
	}

	/* Get the list from user-space */
	aiocbp_list = kmem_zalloc(nent * sizeof(struct aio_job), KM_SLEEP);
	error = copyin(SCARG(uap, list), aiocbp_list,
	    nent * sizeof(struct aiocb));
	if (error) {
		mutex_enter(&aio->aio_mtx);
		goto err;
	}

	/* Enqueue all jobs */
	errcnt = 0;
	for (i = 0; i < nent; i++) {
		error = aio_enqueue_job(AIO_LIO, aiocbp_list[i], lio);
		/*
		 * According to POSIX, in such error case it may
		 * fail with other I/O operations initiated.
		 */
		if (error)
			errcnt++;
	}

	mutex_enter(&aio->aio_mtx);

	/* Return an error, if any */
	if (errcnt) {
		error = EIO;
		goto err;
	}

	if (mode == LIO_WAIT) {
		/*
		 * Wait for AIO completion.  In such case,
		 * the LIO structure will be freed here.
		 */
		while (lio->refcnt > 1 && error == 0)
			error = cv_wait_sig(&aio->done_cv, &aio->aio_mtx);
		if (error)
			error = EINTR;
	}

err:
	if (--lio->refcnt != 0)
		lio = NULL;
	mutex_exit(&aio->aio_mtx);
	if (lio != NULL) {
		aio_sendsig(p, &lio->sig);
		pool_put(&aio_lio_pool, lio);
	}
	kmem_free(aiocbp_list, nent * sizeof(struct aio_job));
	return error;
}

/*
 * SysCtl
 */

static int
sysctl_aio_listio_max(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error, newsize;

	node = *rnode;
	node.sysctl_data = &newsize;

	newsize = aio_listio_max;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (newsize < 1 || newsize > aio_max)
		return EINVAL;
	aio_listio_max = newsize;

	return 0;
}

static int
sysctl_aio_max(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error, newsize;

	node = *rnode;
	node.sysctl_data = &newsize;

	newsize = aio_max;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (newsize < 1 || newsize < aio_listio_max)
		return EINVAL;
	aio_max = newsize;

	return 0;
}

SYSCTL_SETUP(sysctl_aio_setup, "sysctl aio setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, "kern", NULL,
		NULL, 0, NULL, 0,
		CTL_KERN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_IMMEDIATE,
		CTLTYPE_INT, "posix_aio",
		SYSCTL_DESCR("Version of IEEE Std 1003.1 and its "
			     "Asynchronous I/O option to which the "
			     "system attempts to conform"),
		NULL, _POSIX_ASYNCHRONOUS_IO, NULL, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "aio_listio_max",
		SYSCTL_DESCR("Maximum number of asynchronous I/O "
			     "operations in a single list I/O call"),
		sysctl_aio_listio_max, 0, &aio_listio_max, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "aio_max",
		SYSCTL_DESCR("Maximum number of asynchronous I/O "
			     "operations"),
		sysctl_aio_max, 0, &aio_max, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);
}

/*
 * Debugging
 */
#if defined(DDB)
void
aio_print_jobs(void (*pr)(const char *, ...))
{
	struct proc *p = (curlwp == NULL ? NULL : curlwp->l_proc);
	struct aioproc *aio;
	struct aio_job *a_job;
	struct aiocb *aiocbp;

	if (p == NULL) {
		(*pr)("AIO: We are not in the processes right now.\n");
		return;
	}

	aio = p->p_aio;
	if (aio == NULL) {
		(*pr)("AIO data is not initialized (PID = %d).\n", p->p_pid);
		return;
	}

	(*pr)("AIO: PID = %d\n", p->p_pid);
	(*pr)("AIO: Global count of the jobs = %u\n", aio_jobs_count);
	(*pr)("AIO: Count of the jobs = %u\n", aio->jobs_count);

	if (aio->curjob) {
		a_job = aio->curjob;
		(*pr)("\nAIO current job:\n");
		(*pr)(" opcode = %d, errno = %d, state = %d, aiocb_ptr = %p\n",
		    a_job->aio_op, a_job->aiocbp._errno,
		    a_job->aiocbp._state, a_job->aiocb_uptr);
		aiocbp = &a_job->aiocbp;
		(*pr)("   fd = %d, offset = %u, buf = %p, nbytes = %u\n",
		    aiocbp->aio_fildes, aiocbp->aio_offset,
		    aiocbp->aio_buf, aiocbp->aio_nbytes);
	}

	(*pr)("\nAIO queue:\n");
	TAILQ_FOREACH(a_job, &aio->jobs_queue, list) {
		(*pr)(" opcode = %d, errno = %d, state = %d, aiocb_ptr = %p\n",
		    a_job->aio_op, a_job->aiocbp._errno,
		    a_job->aiocbp._state, a_job->aiocb_uptr);
		aiocbp = &a_job->aiocbp;
		(*pr)("   fd = %d, offset = %u, buf = %p, nbytes = %u\n",
		    aiocbp->aio_fildes, aiocbp->aio_offset,
		    aiocbp->aio_buf, aiocbp->aio_nbytes);
	}
}
#endif /* defined(DDB) */
