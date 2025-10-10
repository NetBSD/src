/*	$NetBSD: aio.h,v 1.14 2025/10/10 15:53:55 christos Exp $	*/

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
#include <sys/tree.h>

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

/* Structure for tracking the status of a collection of OPS */
struct aiowaitgroup {
	kmutex_t mtx;		/* Protects entire structure */
	kcondvar_t done_cv;	/* Signaled when job completes */
	size_t completed;	/* Number of completed jobs in this wait group */
	size_t total;		/* Total jobs being waited on */
	bool active;		/* False after suspend returns/times out */
	int refcnt;		/* Reference count */
};

/* */
struct aiowaitgrouplk {
	kmutex_t mtx;	/* Protects wgs array modifications */
	void **wgs;	/* Dynamic array of waiting aiowaitgroups */
	size_t s;	/* Allocated size of wgs array */
	size_t n;	/* Current number of waitgroups */
};

/* Structure of AIO job */
struct aiost;
struct buf;
struct aio_job {
	kmutex_t mtx;		/* Protects completed flag */
	int aio_op;		/* Operation type (AIO_READ/WRITE/SYNC) */
	struct aiocb aiocbp;	/* User-visible AIO control block */
	void *aiocb_uptr;	/* User pointer for job identification */
	struct proc *p;		/* Originating process */
	bool completed;		/* Job completion status */
	bool on_queue;		/* Whether or not this job is on sp->jobs */
	struct file *fp;	/* File pointer associated with the job */
	struct aiowaitgrouplk lk; /* List of waitgroups waiting on this job */
	TAILQ_ENTRY(aio_job) list;
	struct lio_req *lio;	/* List I/O request (if part of lio_listio) */
};

#define AIOST_STATE_NONE	0x1
#define AIOST_STATE_OPERATION	0x2
#define AIOST_STATE_TERMINATE	0x4

#define AIOSP_SUSPEND_ANY	0x1
#define AIOSP_SUSPEND_ALL	0x2

struct aiost;
struct aiost_file_group {
	RB_ENTRY(aiost_file_group) tree;
	struct file *fp;
	struct aiost *aiost;
	kmutex_t mtx;
	TAILQ_HEAD(, aio_job) queue;
	size_t queue_size;
};

/* Structure for AIO servicing thread */
struct aiosp;
struct aiost {
	TAILQ_ENTRY(aiost) list;
	struct aiosp *aiosp;		/* Servicing pool of this thread */
	kmutex_t mtx;			/* Protects this structure */
	kcondvar_t service_cv;		/* Signal to activate thread */
	struct lwp *lwp;		/* Servicing thread LWP */
	int state;			/* The state of the thread */
	bool freelist;			/* Whether or not aiost is on freelist */
	struct aiost_file_group *fg;	/* File group associated with the thread */
	struct aio_job *job;		/* Singleton job */
};

struct aiocbp {
	TAILQ_ENTRY(aiocbp) list;
	const void *uptr;
	struct aio_job *job;
};

/* Structure for AIO servicing pool */
TAILQ_HEAD(aiost_list, aiost);
TAILQ_HEAD(aiocbp_list, aiocbp);
struct aiost_file_tree;
struct aiosp {
	struct aiost_list freelist;	/* Available service threads */
	size_t nthreads_free;		/* Length of freelist */
	struct aiost_list active;	/* Active servicing threads */ 
	size_t nthreads_active;		/* length of active list */
	TAILQ_HEAD(, aio_job) jobs;	/* Queue of pending jobs */
	size_t jobs_pending;		/* Number of pending jobs */
	kmutex_t mtx;			/* Protects structure */
	size_t nthreads_total;		/* Number of total servicing threads */
	volatile u_long njobs_processing;/* Number of total jobs currently being processed*/
	struct aiocbp_list *aio_hash;	/* Aiocbp hash root */
	kmutex_t aio_hash_mtx;		/* Protects the hash table */
	size_t aio_hash_size;		/* Total number of buckets */
	u_int aio_hash_mask;		/* Hash mask */
	struct aiost_file_tree *fg_root;/* RB tree of file groups */
};

/* LIO structure */
struct lio_req {
	u_int refcnt;		/* Reference counter */
	struct sigevent sig;	/* Signal of lio_listio() calls */
};

/* Structure of AIO data for process */
struct aioproc {
	kmutex_t aio_mtx;		/* Protects the entire structure */
	unsigned int jobs_count;	/* Count of the jobs */
	struct aiosp aiosp;		/* Per-process service pool */
};

extern u_int aio_listio_max;

/*
 * Prototypes
 */

void	aio_print_jobs(void (*)(const char *, ...) __printflike(1, 2));
int	aio_suspend1(struct lwp *, struct aiocb **, int, struct timespec *);

int	aiosp_initialize(struct aiosp *);
int	aiosp_destroy(struct aiosp *, int *);
int	aiosp_distribute_jobs(struct aiosp *);
int	aiosp_enqueue_job(struct aiosp *, struct aio_job *);
int	aiosp_suspend(struct aiosp *, struct aiocb **, int, struct timespec *,
		int);
int	aiosp_flush(struct aiosp *);
int	aiosp_validate_conflicts(struct aiosp *, const void *);
int	aiosp_error(struct aiosp *, const void *, register_t *); 
int	aiosp_return(struct aiosp *, const void *, register_t *); 

void	aiowaitgroup_init(struct aiowaitgroup *);
void	aiowaitgroup_fini(struct aiowaitgroup *);
int	aiowaitgroup_wait(struct aiowaitgroup *, int);
void	aiowaitgroup_done(struct aiowaitgroup *);
void	aiowaitgroup_join(struct aiowaitgroup *, struct aiowaitgrouplk *);
void	aiowaitgrouplk_init(struct aiowaitgrouplk *);
void	aiowaitgrouplk_fini(struct aiowaitgrouplk *);
void	aiowaitgrouplk_flush(struct aiowaitgrouplk *);


#endif /* _KERNEL */

#endif /* _SYS_AIO_H_ */
