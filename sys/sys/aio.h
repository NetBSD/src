/*	$NetBSD: aio.h,v 1.13 2016/04/09 19:55:33 riastradh Exp $	*/

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

#ifndef _SYS_AIO_H_
#define _SYS_AIO_H_

#include <sys/types.h>
#include <sys/signal.h>

/* Returned by aio_cancel() */
#define AIO_CANCELED		0x1
#define AIO_NOTCANCELED		0x2
#define AIO_ALLDONE		0x3

/* LIO opcodes */
#define LIO_NOP			0x0
#define LIO_WRITE		0x1
#define LIO_READ		0x2

/* LIO modes */
#define LIO_NOWAIT		0x0
#define LIO_WAIT		0x1

/*
 * Asynchronous I/O structure.
 * Defined in the Base Definitions volume of IEEE Std 1003.1-2001 .
 */
struct aiocb {
	off_t aio_offset;		/* File offset */
	void *aio_buf;			/* I/O buffer in process space */
	size_t aio_nbytes;		/* Length of transfer */
	int aio_fildes;			/* File descriptor */
	int aio_lio_opcode;		/* LIO opcode */
	int aio_reqprio;		/* Request priority offset */
	struct sigevent aio_sigevent;	/* Signal to deliver */

	/* Internal kernel variables */
	int _state;			/* State of the job */
	int _errno;			/* Error value */
	ssize_t _retval;		/* Return value */
};

/* Internal kernel data */
#ifdef _KERNEL

/* Default limits of allowed AIO operations */
#define AIO_LISTIO_MAX		512
#define AIO_MAX			(AIO_LISTIO_MAX * 16)

#include <sys/condvar.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/queue.h>

/* Operations (as flags) */
#define AIO_LIO			0x00
#define AIO_READ		0x01
#define AIO_WRITE		0x02
#define AIO_SYNC		0x04
#define AIO_DSYNC		0x08

/* Job states */
#define JOB_NONE		0x0
#define JOB_WIP			0x1
#define JOB_DONE		0x2

/* Structure of AIO job */
struct aiost;
struct aio_job {
	int aio_op;		/* Operation code */
	struct aiocb aiocbp;	/* AIO data structure */
	pri_t pri;		/* Job priority */
	void *aiocb_uptr;	/* User-space pointer for identification of job */
	struct proc *p;		/* Process that instantiated the job */
	struct aiost *aiost;	/* Service thread associated with this job */
	bool completed;		/* Marks the completion status of this job */
	TAILQ_ENTRY(aio_job) list;
	struct lio_req *lio;
};

#define AIOST_STATE_NONE	0x1
#define AIOST_STATE_OPERATION	0x2
#define AIOST_STATE_TERMINATE	0x4

#define AIOSP_SUSPEND_ANY	0x1
#define AIOSP_SUSPEND_ALL	0x2
#define AIOSP_SUSPEND_N		0x4

#define AIOSP_SUSPEND_NMASK(N)		((N) & 0xffff) << 16)
#define AIOSP_SUSPEND_NEXTRACT(FLAGS)	(((FLAGS) >> 16) & 0xffff)

/* Structure for tracking the status of a collection of OPS */
struct aiosp_ops {
	kmutex_t mtx;		/* Protects this structure */
	kcondvar_t done_cv;	/* Signals when a job is complete */ 
	size_t completed;	/* Keeps track of the number of completed jobs */
	size_t total;		/* Keeps track of the number of total jobs */
};

/* Structure for AIO servicing thread */
struct aiosp;
struct aiost {
	TAILQ_ENTRY(aiost) list;
	struct aiosp *aiosp;		/* Servicing pool of this thread */
	kmutex_t mtx;			/* Protects this structure */
	kcondvar_t service_cv;		/* Signal to activate thread */
	struct aio_job *job;		/* Jobs associated with the thread */
	struct lwp *lwp;		/* Servicing thread LWP */
	size_t ops_total;		/* Total number of connected ops */
	struct aiosp_ops **ops;		/* Array of ops */
	size_t ops_size;		/* Size of ops array */
	int state;			/* The state of the thread */
	bool freelist;			/* Whether or not aiost is on freelist */
};

/* Structure for AIO servicing pool */
TAILQ_HEAD(aiost_list, aiost);
struct aiosp {
	struct aiost_list freelist;	/* Available service threads */
	size_t nthreads_free;		/* Length of freelist */
	struct aiost_list active;	/* Active servicing threads */ 
	size_t nthreads_active;		/* length of active list */
	TAILQ_HEAD(, aio_job) jobs;	/* Queue of pending jobs */
	size_t jobs_pending;		/* Number of pending jobs */
	kmutex_t mtx;			/* Protects structure */
	size_t nthreads_total;		/* Number of total servicing threads */
	pri_t priority;			/* Thread priority of the pool */
};

struct aiocbp {
	TAILQ_ENTRY(aiocbp) list;
	void *uptr;
	struct aio_job *job;
};

/* LIO structure */
struct lio_req {
	u_int refcnt;		/* Reference counter */
	struct sigevent sig;	/* Signal of lio_listio() calls */
};

/* Structure of AIO data for process */
TAILQ_HEAD(aiocbp_list, aiocbp);
struct aioproc {
	kmutex_t aio_mtx;		/* Protects the entire structure */
	kcondvar_t aio_worker_cv;	/* Signals on a new job */
	kcondvar_t done_cv;		/* Signals when the job is done */
	struct aio_job *curjob;		/* Currently processing AIO job */
	unsigned int jobs_count;	/* Count of the jobs */
	TAILQ_HEAD(, aio_job) jobs_queue;/* Queue of the AIO jobs */
	struct lwp *aio_worker;		/* AIO worker thread */
	kmutex_t aio_hash_mtx;		/* Protects the hash table */
	struct aiost_list aiost_total;	/* Total list of servicing threads */
	struct aiocbp_list *aio_hash;	/* Aiocbp hash root */
	size_t aio_hash_size;		/* Total number of buckets */
	u_int aio_hash_mask;		/* Hash mask */
};

extern u_int aio_listio_max;

/*
 * Prototypes
 */

void	aio_print_jobs(void (*)(const char *, ...) __printflike(1, 2));
int	aio_suspend1(struct lwp *, struct aiocb **, int, struct timespec *);

int	aiosp_distribute_jobs(struct aiosp *);
int	aiosp_dispense_bank(void);
int	aiosp_enqueue_job(struct aio_job *);
int	aiosp_suspend(struct aioproc *, struct aiocb **, int, struct timespec *,
		uint32_t);
int	aiosp_flush(struct aioproc *);
int	aiosp_validate_conflicts(struct aioproc *, void *);

void	aiocbp_destroy(struct aioproc *);
int	aiocbp_init(struct aioproc *, u_int);
int 	aiocbp_lookup(struct aioproc *, struct aiocbp **, void *);
int 	aiocbp_remove(struct aioproc *, void *);
int 	aiocbp_insert(struct aioproc *, struct aiocbp *);


#endif /* _KERNEL */

#endif /* _SYS_AIO_H_ */
