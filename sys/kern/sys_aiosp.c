/*	$NetBSD: sys_aiosp.c,v 0.00 2025/05/18 12:00:00 ethan4984 Exp $	*/

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Implementation of service pools to support asynchronous I/O
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_aiosp.c,v 0.00 2025/05/18 12:00:00 ethan4984 Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/condvar.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/sdt.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/kthread.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/vnode.h>

MODULE(MODULE_CLASS_MISC, aiosp, NULL);

static kmutex_t		aiospb_mtx;
static u_int		aiospb_max = PRI_KTHREAD + NPRI_KTHREAD;
static struct aiosp	**aiospb;

static int		aiosp_initialize(struct aiosp *, pri_t);
static int		aiosp_destroy(struct aiosp *);
static int		aiosp_retrieve_bank(pri_t, struct aiosp **);
static int		aiosp_pri_idx(pri_t);

static int		aiost_create(struct aiosp *, struct aiost **);
static int		aiost_terminate(struct aiost *);
static int		aiost_configure(struct aiost *, struct aio_job *,
				vaddr_t *);
static int		aiost_process_rw(struct aiost *);
static int		aiost_process_sync(struct aiost *);
static void		aiost_entry(void *);
static void		aiost_sigsend(struct proc *, struct sigevent *);

/*
 * Tear down all service pools
 */
static int
aio_fini(void)
{
	struct aiosp *aiosp;
	int error;

	for (int i = 0; i < aiospb_max; i++) {
		aiosp = aiospb[i];
		if (aiosp == NULL) {
			continue;
		}

		error = aiosp_destroy(aiosp);
		if (error) {
			return error;
		}

		kmem_free(aiosp, sizeof(*aiosp));
	}

	kmem_free(aiospb, sizeof(*aiospb) * aiospb_max);

	return 0;
}

/*
 * Initialize global service pool state
 */
static int
aio_init(void)
{
	struct aiosp *aiosp;
	int error;

	mutex_init(&aiospb_mtx, MUTEX_DEFAULT, IPL_NONE);

	aiospb = kmem_zalloc(sizeof(*aiospb) * aiospb_max, KM_SLEEP);
	aiosp = kmem_zalloc(sizeof(*aiosp), KM_SLEEP);
	aiospb[aiosp_pri_idx(PRI_KTHREAD)] = aiosp;

	error = aiosp_initialize(aiosp, PRI_KTHREAD);
	if (error) {
		return error;
	}

	return 0;
}

/*
 * Module interface
 */
static int
aiosp_modcmd(modcmd_t cmd, void *arg)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		return aio_init();
	case MODULE_CMD_FINI:
		return aio_fini();
	default:
		return SET_ERROR(ENOTTY);
	}
}

/*
 * Distributes pending jobs to servicing threads. Allocates the requisite number
 * of servicing threads, creates new threads if necessary, then assigns a single
 * job to be completed by a servicing thread.
 */
int
aiosp_distribute_jobs(struct aiosp *sp)
{
	struct aiost **aiost_list;
	struct aio_job *job;
	int total_dispensed;
	int error = 0;

	/*
	 * Check to see if the number of pending jobs exceeds the number of free
	 * service threads. If it does then that means we need to create new
	 * threads.
	 */
	if (sp->jobs_pending > sp->nthreads_free) {
		int nthreads_new = sp->jobs_pending - sp->nthreads_free;

		for (int i = 0; i < nthreads_new; i++) {
			struct aiost *aiost;

			error = aiost_create(sp, &aiost);
			if (error) {
				mutex_exit(&sp->mtx);
				return error;
			}
		}
	}

	if (!sp->jobs_pending) {
		return 0;
	}

	total_dispensed = 0;
	aiost_list = kmem_zalloc(sizeof(*aiost_list) *
		sp->jobs_pending, KM_SLEEP);

	/*
	 * Loop over all pending jobs and assign a thread from the freelist.
	 * Move from freelist to active. Configure service thread to work with
	 * respect to the job. Also signal the CV outside of sp->mtx to avoid
	 * any shenanigans.
	 */
	mutex_enter(&sp->mtx);
	struct aio_job *tmp;
	TAILQ_FOREACH_SAFE(job, &sp->jobs, list, tmp) {
		struct aiost *aiost = TAILQ_LAST(&sp->freelist, aiost_list);
		if (aiost == NULL) {
			panic("aiosp_distribute_jobs: aiost is null"); 
		}

		mutex_enter(&aiost->mtx);

		TAILQ_REMOVE(&sp->freelist, aiost, list);
		sp->nthreads_free--;

		TAILQ_INSERT_TAIL(&sp->active, aiost, list);
		sp->nthreads_active++;

		error = aiost_configure(aiost, job, &aiost->kbuf);
		if (error) {
			kmem_free(aiost_list + total_dispensed,
				sizeof(*aiost_list) * sp->jobs_pending);
			mutex_exit(&aiost->mtx);
			goto finish;
		}

		TAILQ_REMOVE(&sp->jobs, job, list);

		aiost->job = job;

		aiost_list[total_dispensed++] = aiost;
		sp->jobs_pending--;

		mutex_exit(&aiost->mtx);
	}
finish:
	mutex_exit(&sp->mtx);

	for (int i = 0; i < total_dispensed; i++) {
		struct aiost *aiost = aiost_list[i];
		aiost->state = AIOST_STATE_OPERATION;
		cv_signal(&aiost->service_cv);
	}

	kmem_free(aiost_list, sizeof(*aiost_list) * total_dispensed);

	return error;
}

/*
 * Distribute all pending operations on all service queues attached to the
 * primary bank
 */
int
aiosp_dispense_bank(void)
{
	int error;
	struct aiosp *sp;

	mutex_enter(&aiospb_mtx);

	for (int i = 0; i < aiosp_pri_idx(aiospb_max); i++) {
		sp = aiospb[i];
		if (sp == NULL) {
			continue;
		}

		error = aiosp_distribute_jobs(sp);
		if (error) {
			mutex_exit(&aiospb_mtx);
			return error;
		}
	}

	mutex_exit(&aiospb_mtx);

	return 0;
}

/*
 * Initializes a servicing pool.
 */
static int
aiosp_initialize(struct aiosp *sp, pri_t pri)
{
	sp->priority = pri;
	mutex_init(&sp->mtx, MUTEX_DEFAULT, IPL_NONE);
	TAILQ_INIT(&sp->freelist);
	TAILQ_INIT(&sp->active);
	TAILQ_INIT(&sp->jobs);

	return 0;
}

/*
 * Convert a priority into an index into the service pool bank.
 */
static int
aiosp_pri_idx(pri_t pri)
{
	if (pri < PRI_KTHREAD) {
		panic("aio_process: invalid priority for AIO (<PRI_KTHREAD)");
	}

	int idx = pri - PRI_KTHREAD;
	if (idx > aiospb_max) {
		panic("aio_process: invalid priority for AIO (>NPRI_KTHREAD");
	}

	return idx;
}

/*
 * Convert a priority into associative service pool. Initialize the pool if it
 * does not yet exist.
 */
static int
aiosp_retrieve_bank(pri_t pri, struct aiosp **aiosp)
{
	int error;
	int bank_pri_idx;

	mutex_enter(&aiospb_mtx);

	bank_pri_idx = aiosp_pri_idx(pri);

	*aiosp = aiospb[bank_pri_idx];
	if (*aiosp == NULL) {
		aiospb[bank_pri_idx] = kmem_zalloc(sizeof(**aiospb),
			KM_SLEEP);
		*aiosp = aiospb[bank_pri_idx];

		error = aiosp_initialize(*aiosp, pri);
		if (error) {
			mutex_exit(&aiospb_mtx);
			return error;
		}
	}

	mutex_exit(&aiospb_mtx);

	return 0;
}

/*
 * Each process keeps track of all the service threads instantiated to service
 * an asynchronous operation by the process. When a process is terminated we
 * must also terminate all of its active and pending asynchronous operation.
 * EXCEPTIONAL TROLLING FIX LATER
 */
static int
aiosp_destroy(struct aiosp *sp)
{
	struct aiost *st;
	int freelist = 1;
	int error;

	mutex_enter(&sp->mtx);

	goto begin;
process:
	error = aiost_terminate(st);
	if (error) {
		mutex_exit(&sp->mtx);
		return error;
	}

	kmem_free(st, sizeof(*st));

	if (freelist) {
		goto freelist_next;
	} else {
		goto active_next;
	}

begin:
	TAILQ_FOREACH(st, &sp->freelist, list) {
		goto process;
freelist_next:
	}

	freelist = 0;
	TAILQ_FOREACH(st, &sp->active, list) {
		goto process;
active_next:
	}

	kmem_free(sp, sizeof(*sp));

	mutex_exit(&sp->mtx);

	return 0;
}

/*
 * Enqueue a job for processing by a servicing queue
 */
int
aiosp_enqueue_job(struct aio_job *job)
{
	int error;
	struct aiosp *sp;

	error = aiosp_retrieve_bank(job->pri, &sp);
	if (error) {
		return error;
	}

	mutex_enter(&sp->mtx);

	TAILQ_INSERT_TAIL(&sp->jobs, job, list);
	sp->jobs_pending++;

	mutex_exit(&sp->mtx);

	return 0;
}

/*
 * Create and initialise a new servicing thread and append it to the freelist.
 */
static int
aiost_create(struct aiosp *sp, struct aiost **ret)
{
	struct proc *p = curlwp->l_proc;
	struct aiost *st;

	st = kmem_zalloc(sizeof(*st), KM_SLEEP);

	mutex_init(&st->mtx, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&st->service_mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&st->service_cv, "aioservice");

	mutex_enter(&sp->mtx);

	int error = kthread_create(PRI_KERNEL, 0, NULL, aiost_entry,
		st, &st->lwp, "aio_%d_%d", p->p_pid, sp->nthreads_total);
	if (error) {
		mutex_exit(&sp->mtx);
		return error;
	}

	st->job = NULL; 
	st->state = AIOST_STATE_NONE;
	st->aiosp = sp;

	TAILQ_INSERT_TAIL(&sp->freelist, st, list);
	sp->nthreads_free++;
	sp->nthreads_total++;

	mutex_exit(&sp->mtx);

	if (ret) {
		*ret = st;
	}

	return 0;
}

/*
 * Servicing thread entry point. Process the operation. Notify all those
 * blocking on the completion of the operation. Send a signal if necessary. And
 * then mark the current servicing thread as free.
 */
static void
aiost_entry(void *arg)
{
	struct aiost *st = arg;
	struct aiosp *sp = st->aiosp;
	struct aio_job *job;
	int error;

	/*
	 * We want to handle abrupt process terminations effectively. We use
	 * st->exit to indicate that the thread must exit. When a thread is
	 * terminated aiost_terminate(st) unblocks those sleeping on
	 * st->service_cv
	 */
	for (;;) {
		for (;;) {
			mutex_enter(&st->mtx);

			if (st->state & AIOST_STATE_OPERATION) {
				goto process_operation;		
			} else if (st->state & AIOST_STATE_TERMINATE) {
				goto process_termination;
			} else if (st->state & AIOST_STATE_NONE) {
				mutex_exit(&st->mtx);
				mutex_enter(&st->service_mtx);
				error = cv_wait_sig(&st->service_cv,
					&st->service_mtx);
				mutex_exit(&st->service_mtx);
				if (error) {
					/*
					 * Thread was interrupt. Check for
					 * pending exit or suspension
					 */
					mutex_exit(&st->service_mtx);
					lwp_userret(curlwp);
				}
			} else {
				panic("aio_process: invalid aiost state {%x}\n",
					st->state);
			}
		}
process_termination:
		/*
		 * Remove st from the list of active service threads, do NOT
		 * append to the freelist, dance around locks, exit kthread
		 */
		mutex_enter(&sp->mtx);
		TAILQ_REMOVE(&sp->freelist, st, list);
		sp->nthreads_free--;
		mutex_exit(&sp->mtx);
		mutex_exit(&st->mtx);
		kthread_exit(0);
process_operation:
		job = st->job;
		if (job->aio_op & (AIO_READ | AIO_WRITE)) {
			error = aiost_process_rw(st);
			if (error) {
				mutex_exit(&st->mtx);
				goto next;
			}
		} else if (job->aio_op & AIO_SYNC) {
			error = aiost_process_sync(st);
			if (error) {
				mutex_exit(&st->mtx);
				goto next;
			}
		} else {
			panic("aio_process: invalid operation code\n");
		}

		aiost_sigsend(job->p, &job->aiocbp.aio_sigevent);
next:
		st->state = AIOST_STATE_NONE;
		mutex_exit(&st->mtx);

		/*
		 * Remove st from list of active service threads, append to
		 * freelist, dance around locks, then iterate loop and block on
		 * st->service_cv
		 */
		mutex_enter(&sp->mtx);

		TAILQ_REMOVE(&sp->active, st, list);
		sp->nthreads_active--;

		TAILQ_INSERT_TAIL(&sp->freelist, st, list);
		sp->nthreads_free++;

		mutex_exit(&sp->mtx);
	}
}

/*
 * send AIO signal.
 */
static void
aiost_sigsend(struct proc *p, struct sigevent *sig)
{
	ksiginfo_t ksi;

	if (sig->sigev_signo == 0 || sig->sigev_notify == SIGEV_NONE)
		return;

	KSI_INIT(&ksi);
	ksi.ksi_signo = sig->sigev_signo;
	ksi.ksi_code = SI_ASYNCIO;
	ksi.ksi_value = sig->sigev_value;

	mutex_enter(&proc_lock);
	kpsignal(p, &ksi, NULL);
	mutex_exit(&proc_lock);
}

/*
 * processes a read/write asynchronous operations
 */
static int
aiost_process_rw(struct aiost *aiost)
{
	struct aio_job *job = aiost->job;
	struct aiocb *aiocbp = &job->aiocbp;
	struct file *fp;
	int fd = aiocbp->aio_fildes;
	int error = 0;

	struct iovec aiov;
	struct uio auio;
                                                                        
	if (aiocbp->aio_nbytes > SSIZE_MAX) {
		error = SET_ERROR(EINVAL);
		goto done;
	}
                                                                        
	fp = fd_getfile(fd);
	if (fp == NULL) {
		error = SET_ERROR(EBADF);
		goto done;
	}
                                                                        
	aiov.iov_base = (void *)(uintptr_t)aiost->kbuf;
	aiov.iov_len = aiocbp->aio_nbytes;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = aiocbp->aio_nbytes;
	auio.uio_vmspace = NULL;

	if (job->aio_op & AIO_READ) {
		/*
		 * Perform a Read operation
		 */
		KASSERT((job->aio_op & AIO_WRITE) == 0);
                                                                        
		if ((fp->f_flag & FREAD) == 0) {
			fd_putfile(fd);
			error = SET_ERROR(EBADF);
			goto done;
		}
		auio.uio_rw = UIO_READ;
		error = (*fp->f_ops->fo_read)(fp, &aiocbp->aio_offset,
		    &auio, fp->f_cred, FOF_UPDATE_OFFSET);
	} else {
		/*
		 * Perform a Write operation
		 */
		KASSERT(job->aio_op & AIO_WRITE);
                                                                        
		if ((fp->f_flag & FWRITE) == 0) {
			fd_putfile(fd);
			error = SET_ERROR(EBADF);
			goto done;
		}
		auio.uio_rw = UIO_WRITE;
		error = (*fp->f_ops->fo_write)(fp, &aiocbp->aio_offset,
		    &auio, fp->f_cred, FOF_UPDATE_OFFSET);
	}
	fd_putfile(fd);
                                                                        
	/*
	 * Store the result value
	 */
	job->aiocbp.aio_nbytes -= auio.uio_resid;
	job->aiocbp._retval = (error == 0) ? job->aiocbp.aio_nbytes : -1;
done:
	job->aiocbp._errno = error;
	job->aiocbp._state = JOB_DONE;

	return 0;
}

/*
 * processes a sync/dsync asynchronous operations
 */
static int
aiost_process_sync(struct aiost *aiost)
{
	struct aio_job *job = aiost->job;
	struct aiocb *aiocbp = &job->aiocbp;
	struct file *fp;
	int fd = aiocbp->aio_fildes;
	int error = 0;

	/*
	 * Perform a file sync operation
	 */
	struct vnode *vp;
                                                        
	if ((error = fd_getvnode(fd, &fp)) != 0) {
		goto done;
	}
                                                        
	if ((fp->f_flag & FWRITE) == 0) {
		fd_putfile(fd);
		error = SET_ERROR(EBADF);
		goto done;
	}
                                                        
	vp = fp->f_vnode;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (job->aio_op & AIO_DSYNC) {
		error = VOP_FSYNC(vp, fp->f_cred,
		    FSYNC_WAIT | FSYNC_DATAONLY, 0, 0);
	} else if (job->aio_op & AIO_SYNC) {
		error = VOP_FSYNC(vp, fp->f_cred,
		    FSYNC_WAIT, 0, 0);
	}
	VOP_UNLOCK(vp);
	fd_putfile(fd);
                                                        
	/*
	 * Store the result value
	 */
	job->aiocbp._retval = (error == 0) ? 0 : -1;
done:
	job->aiocbp._errno = error;
	job->aiocbp._state = JOB_DONE;

	return 0;
}

/*
 * Destroy a servicing thread. Set st->exit high such that when we unblock the
 * thread blocking on st->service_cv it will invoke an exit routine within
 * aiost_entry.
 */
static int
aiost_terminate(struct aiost *st)
{
	mutex_enter(&st->mtx);
	st->state = AIOST_STATE_TERMINATE;
	mutex_exit(&st->mtx);

	cv_signal(&st->service_cv);
	kthread_join(st->lwp);

	cv_destroy(&st->service_cv);
	mutex_destroy(&st->mtx);
	kmem_free(st, sizeof(*st));

	return 0;
}

/*
 * Configure a servicing thread to handle a specific job. Initialise operation
 * and establish the 'shared' memory region.
 */
static int
aiost_configure(struct aiost *aiost, struct aio_job *job, vaddr_t *kbuf)
{
	struct vmspace *vm = job->p->p_vmspace;
	struct aiocb *aiocb = &job->aiocbp;
	vaddr_t uva, kva;
	paddr_t upa;
	int error;

	vm_prot_t protections = VM_PROT_NONE;
	if (job->aio_op == AIO_READ) {
		protections = VM_PROT_READ;
	} else if(job->aio_op == AIO_WRITE) {
		protections = VM_PROT_READ | VM_PROT_WRITE;
	} else {
		return 0;
	}

	/*
	 * To account for the case where the memory is anonymously mapped and
	 * has not yet been fulfilled.
	 */
	error = uvm_vslock(vm, job->aiocb_uptr, aiocb->aio_nbytes,
		protections);
	if (error) {
		return error;
	}

	kva = uvm_km_alloc(kernel_map, aiocb->aio_nbytes, 0,
		 UVM_KMF_VAONLY);
	if (!kva) {
		uvm_vsunlock(vm, job->aiocb_uptr, aiocb->aio_nbytes);
		return ENOMEM;
	}

	/*
	 * Extract physical memory and map to the kernel
	 */
	for (uva = trunc_page((vaddr_t)aiocb->aio_buf);
		uva < round_page((vaddr_t)aiocb->aio_buf + aiocb->aio_nbytes);
		uva += PAGE_SIZE) {

		error = pmap_extract(vm_map_pmap(&vm->vm_map), uva, &upa);
		if (error) {
			uvm_km_free(kernel_map, kva, aiocb->aio_nbytes,
				UVM_KMF_VAONLY);
			uvm_vsunlock(vm, job->aiocb_uptr,
				aiocb->aio_nbytes);
			return EFAULT;
		}

		pmap_kenter_pa(kva + (uva - trunc_page((vaddr_t)aiocb->aio_buf)),
			upa, protections, 0);
	}

	pmap_update(pmap_kernel());
	*kbuf = kva;

	return 0;
}
