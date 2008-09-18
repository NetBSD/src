/*
 * Copyright (C) 2002 Sistina Software (UK) Limited.
 *
 * This file is released under the GPL.
 */

#include <asm/atomic.h>

#include <linux/blkdev.h>
#include <linux/config.h>
#include <linux/device-mapper.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/locks.h>
#include <linux/mempool.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "kcopyd.h"
#include "dm-daemon.h"

/* FIXME: this is only needed for the DMERR macros */
#include "dm.h"

static struct dm_daemon _kcopyd;

#define SECTORS_PER_PAGE (PAGE_SIZE / SECTOR_SIZE)
#define SUB_JOB_SIZE 128
#define PAGES_PER_SUB_JOB (SUB_JOB_SIZE / SECTORS_PER_PAGE)
#define SUB_JOB_COUNT 8

/*-----------------------------------------------------------------
 * Each kcopyd client has its own little pool of preallocated
 * pages for kcopyd io.
 *---------------------------------------------------------------*/
struct kcopyd_client {
	struct list_head list;

	spinlock_t lock;
	struct list_head pages;
	unsigned int nr_pages;
	unsigned int nr_free_pages;
	unsigned int max_split;
};

static inline void __push_page(struct kcopyd_client *kc, struct page *p)
{
	list_add(&p->list, &kc->pages);
	kc->nr_free_pages++;
}

static inline struct page *__pop_page(struct kcopyd_client *kc)
{
	struct page *p;

	p = list_entry(kc->pages.next, struct page, list);
	list_del(&p->list);
	kc->nr_free_pages--;

	return p;
}

static int kcopyd_get_pages(struct kcopyd_client *kc,
			    unsigned int nr, struct list_head *pages)
{
	struct page *p;
	INIT_LIST_HEAD(pages);

	spin_lock(&kc->lock);
	if (kc->nr_free_pages < nr) {
		spin_unlock(&kc->lock);
		return -ENOMEM;
	}

	while (nr--) {
		p = __pop_page(kc);
		list_add(&p->list, pages);
	}
	spin_unlock(&kc->lock);

	return 0;
}

static void kcopyd_put_pages(struct kcopyd_client *kc, struct list_head *pages)
{
	struct list_head *tmp, *tmp2;

	spin_lock(&kc->lock);
	list_for_each_safe (tmp, tmp2, pages)
		__push_page(kc, list_entry(tmp, struct page, list));
	spin_unlock(&kc->lock);
}

/*
 * These three functions resize the page pool.
 */
static void release_pages(struct list_head *pages)
{
	struct page *p;
	struct list_head *tmp, *tmp2;

	list_for_each_safe (tmp, tmp2, pages) {
		p = list_entry(tmp, struct page, list);
		UnlockPage(p);
		__free_page(p);
	}
}

static int client_alloc_pages(struct kcopyd_client *kc, unsigned int nr)
{
	unsigned int i;
	struct page *p;
	LIST_HEAD(new);

	for (i = 0; i < nr; i++) {
		p = alloc_page(GFP_KERNEL);
		if (!p) {
			release_pages(&new);
			return -ENOMEM;
		}

		LockPage(p);
		list_add(&p->list, &new);
	}

	kcopyd_put_pages(kc, &new);
	kc->nr_pages += nr;
	kc->max_split = kc->nr_pages / PAGES_PER_SUB_JOB;
	if (kc->max_split > SUB_JOB_COUNT)
		kc->max_split = SUB_JOB_COUNT;

	return 0;
}

static void client_free_pages(struct kcopyd_client *kc)
{
	BUG_ON(kc->nr_free_pages != kc->nr_pages);
	release_pages(&kc->pages);
	kc->nr_free_pages = kc->nr_pages = 0;
}

/*-----------------------------------------------------------------
 * kcopyd_jobs need to be allocated by the *clients* of kcopyd,
 * for this reason we use a mempool to prevent the client from
 * ever having to do io (which could cause a deadlock).
 *---------------------------------------------------------------*/
struct kcopyd_job {
	struct kcopyd_client *kc;
	struct list_head list;
	unsigned int flags;

	/*
	 * Error state of the job.
	 */
	int read_err;
	unsigned int write_err;

	/*
	 * Either READ or WRITE
	 */
	int rw;
	struct io_region source;

	/*
	 * The destinations for the transfer.
	 */
	unsigned int num_dests;
	struct io_region dests[KCOPYD_MAX_REGIONS];

	sector_t offset;
	unsigned int nr_pages;
	struct list_head pages;

	/*
	 * Set this to ensure you are notified when the job has
	 * completed.  'context' is for callback to use.
	 */
	kcopyd_notify_fn fn;
	void *context;

	/*
	 * These fields are only used if the job has been split
	 * into more manageable parts.
	 */
	struct semaphore lock;
	atomic_t sub_jobs;
	sector_t progress;
};

/* FIXME: this should scale with the number of pages */
#define MIN_JOBS 512

static kmem_cache_t *_job_cache;
static mempool_t *_job_pool;

/*
 * We maintain three lists of jobs:
 *
 * i)   jobs waiting for pages
 * ii)  jobs that have pages, and are waiting for the io to be issued.
 * iii) jobs that have completed.
 *
 * All three of these are protected by job_lock.
 */
static spinlock_t _job_lock = SPIN_LOCK_UNLOCKED;

static LIST_HEAD(_complete_jobs);
static LIST_HEAD(_io_jobs);
static LIST_HEAD(_pages_jobs);

static int jobs_init(void)
{
	INIT_LIST_HEAD(&_complete_jobs);
	INIT_LIST_HEAD(&_io_jobs);
	INIT_LIST_HEAD(&_pages_jobs);

	_job_cache = kmem_cache_create("kcopyd-jobs",
				       sizeof(struct kcopyd_job),
				       __alignof__(struct kcopyd_job),
				       0, NULL, NULL);
	if (!_job_cache)
		return -ENOMEM;

	_job_pool = mempool_create(MIN_JOBS, mempool_alloc_slab,
				   mempool_free_slab, _job_cache);
	if (!_job_pool) {
		kmem_cache_destroy(_job_cache);
		return -ENOMEM;
	}

	return 0;
}

static void jobs_exit(void)
{
	BUG_ON(!list_empty(&_complete_jobs));
	BUG_ON(!list_empty(&_io_jobs));
	BUG_ON(!list_empty(&_pages_jobs));

	mempool_destroy(_job_pool);
	kmem_cache_destroy(_job_cache);
}

/*
 * Functions to push and pop a job onto the head of a given job
 * list.
 */
static inline struct kcopyd_job *pop(struct list_head *jobs)
{
	struct kcopyd_job *job = NULL;
	unsigned long flags;

	spin_lock_irqsave(&_job_lock, flags);

	if (!list_empty(jobs)) {
		job = list_entry(jobs->next, struct kcopyd_job, list);
		list_del(&job->list);
	}
	spin_unlock_irqrestore(&_job_lock, flags);

	return job;
}

static inline void push(struct list_head *jobs, struct kcopyd_job *job)
{
	unsigned long flags;

	spin_lock_irqsave(&_job_lock, flags);
	list_add_tail(&job->list, jobs);
	spin_unlock_irqrestore(&_job_lock, flags);
}

/*
 * These three functions process 1 item from the corresponding
 * job list.
 *
 * They return:
 * < 0: error
 *   0: success
 * > 0: can't process yet.
 */
static int run_complete_job(struct kcopyd_job *job)
{
	void *context = job->context;
	int read_err = job->read_err;
	unsigned int write_err = job->write_err;
	kcopyd_notify_fn fn = job->fn;

	kcopyd_put_pages(job->kc, &job->pages);
	mempool_free(job, _job_pool);
	fn(read_err, write_err, context);
	return 0;
}

static void complete_io(unsigned int error, void *context)
{
	struct kcopyd_job *job = (struct kcopyd_job *) context;

	if (error) {
		if (job->rw == WRITE)
			job->write_err &= error;
		else
			job->read_err = 1;

		if (!test_bit(KCOPYD_IGNORE_ERROR, &job->flags)) {
			push(&_complete_jobs, job);
			dm_daemon_wake(&_kcopyd);
			return;
		}
	}

	if (job->rw == WRITE)
		push(&_complete_jobs, job);

	else {
		job->rw = WRITE;
		push(&_io_jobs, job);
	}

	dm_daemon_wake(&_kcopyd);
}

/*
 * Request io on as many buffer heads as we can currently get for
 * a particular job.
 */
static int run_io_job(struct kcopyd_job *job)
{
	int r;

	if (job->rw == READ)
		r = dm_io_async(1, &job->source, job->rw,
				list_entry(job->pages.next, struct page, list),
				job->offset, complete_io, job);

	else
		r = dm_io_async(job->num_dests, job->dests, job->rw,
				list_entry(job->pages.next, struct page, list),
				job->offset, complete_io, job);

	return r;
}

static int run_pages_job(struct kcopyd_job *job)
{
	int r;

	job->nr_pages = dm_div_up(job->dests[0].count + job->offset,
				  SECTORS_PER_PAGE);
	r = kcopyd_get_pages(job->kc, job->nr_pages, &job->pages);
	if (!r) {
		/* this job is ready for io */
		push(&_io_jobs, job);
		return 0;
	}

	if (r == -ENOMEM)
		/* can't complete now */
		return 1;

	return r;
}

/*
 * Run through a list for as long as possible.  Returns the count
 * of successful jobs.
 */
static int process_jobs(struct list_head *jobs, int (*fn) (struct kcopyd_job *))
{
	struct kcopyd_job *job;
	int r, count = 0;

	while ((job = pop(jobs))) {

		r = fn(job);

		if (r < 0) {
			/* error this rogue job */
			if (job->rw == WRITE)
				job->write_err = (unsigned int) -1;
			else
				job->read_err = 1;
			push(&_complete_jobs, job);
			break;
		}

		if (r > 0) {
			/*
			 * We couldn't service this job ATM, so
			 * push this job back onto the list.
			 */
			push(jobs, job);
			break;
		}

		count++;
	}

	return count;
}

/*
 * kcopyd does this every time it's woken up.
 */
static void do_work(void)
{
	/*
	 * The order that these are called is *very* important.
	 * complete jobs can free some pages for pages jobs.
	 * Pages jobs when successful will jump onto the io jobs
	 * list.  io jobs call wake when they complete and it all
	 * starts again.
	 */
	process_jobs(&_complete_jobs, run_complete_job);
	process_jobs(&_pages_jobs, run_pages_job);
	process_jobs(&_io_jobs, run_io_job);
	run_task_queue(&tq_disk);
}

/*
 * If we are copying a small region we just dispatch a single job
 * to do the copy, otherwise the io has to be split up into many
 * jobs.
 */
static void dispatch_job(struct kcopyd_job *job)
{
	push(&_pages_jobs, job);
	dm_daemon_wake(&_kcopyd);
}

static void segment_complete(int read_err,
			     unsigned int write_err, void *context)
{
	/* FIXME: tidy this function */
	sector_t progress = 0;
	sector_t count = 0;
	struct kcopyd_job *job = (struct kcopyd_job *) context;

	down(&job->lock);

	/* update the error */
	if (read_err)
		job->read_err = 1;

	if (write_err)
		job->write_err &= write_err;

	/*
	 * Only dispatch more work if there hasn't been an error.
	 */
	if ((!job->read_err && !job->write_err) ||
	    test_bit(KCOPYD_IGNORE_ERROR, &job->flags)) {
		/* get the next chunk of work */
		progress = job->progress;
		count = job->source.count - progress;
		if (count) {
			if (count > SUB_JOB_SIZE)
				count = SUB_JOB_SIZE;

			job->progress += count;
		}
	}
	up(&job->lock);

	if (count) {
		int i;
		struct kcopyd_job *sub_job = mempool_alloc(_job_pool, GFP_NOIO);

		memcpy(sub_job, job, sizeof(*job));
		sub_job->source.sector += progress;
		sub_job->source.count = count;

		for (i = 0; i < job->num_dests; i++) {
			sub_job->dests[i].sector += progress;
			sub_job->dests[i].count = count;
		}

		sub_job->fn = segment_complete;
		sub_job->context = job;
		dispatch_job(sub_job);

	} else if (atomic_dec_and_test(&job->sub_jobs)) {

		/*
		 * To avoid a race we must keep the job around
		 * until after the notify function has completed.
		 * Otherwise the client may try and stop the job
		 * after we've completed.
		 */
		job->fn(read_err, write_err, job->context);
		mempool_free(job, _job_pool);
	}
}

/*
 * Create some little jobs that will do the move between
 * them.
 */
static void split_job(struct kcopyd_job *job)
{
	int nr;

	nr = dm_div_up(job->source.count, SUB_JOB_SIZE);
	if (nr > job->kc->max_split)
		nr = job->kc->max_split;

	atomic_set(&job->sub_jobs, nr);
	while (nr--)
		segment_complete(0, 0u, job);
}

int kcopyd_copy(struct kcopyd_client *kc, struct io_region *from,
		unsigned int num_dests, struct io_region *dests,
		unsigned int flags, kcopyd_notify_fn fn, void *context)
{
	struct kcopyd_job *job;

	/*
	 * Allocate a new job.
	 */
	job = mempool_alloc(_job_pool, GFP_NOIO);

	/*
	 * set up for the read.
	 */
	job->kc = kc;
	job->flags = flags;
	job->read_err = 0;
	job->write_err = 0;
	job->rw = READ;

	memcpy(&job->source, from, sizeof(*from));

	job->num_dests = num_dests;
	memcpy(&job->dests, dests, sizeof(*dests) * num_dests);

	job->offset = 0;
	job->nr_pages = 0;
	INIT_LIST_HEAD(&job->pages);

	job->fn = fn;
	job->context = context;

	if (job->source.count < SUB_JOB_SIZE)
		dispatch_job(job);

	else {
		init_MUTEX(&job->lock);
		job->progress = 0;
		split_job(job);
	}

	return 0;
}

/*
 * Cancels a kcopyd job, eg. someone might be deactivating a
 * mirror.
 */
int kcopyd_cancel(struct kcopyd_job *job, int block)
{
	/* FIXME: finish */
	return -1;
}

/*-----------------------------------------------------------------
 * Unit setup
 *---------------------------------------------------------------*/
static DECLARE_MUTEX(_client_lock);
static LIST_HEAD(_clients);

static int client_add(struct kcopyd_client *kc)
{
	down(&_client_lock);
	list_add(&kc->list, &_clients);
	up(&_client_lock);
	return 0;
}

static void client_del(struct kcopyd_client *kc)
{
	down(&_client_lock);
	list_del(&kc->list);
	up(&_client_lock);
}

int kcopyd_client_create(unsigned int nr_pages, struct kcopyd_client **result)
{
	int r = 0;
	struct kcopyd_client *kc;

	if (nr_pages * SECTORS_PER_PAGE < SUB_JOB_SIZE) {
		DMERR("kcopyd client requested %u pages: minimum is %lu",
		      nr_pages, SUB_JOB_SIZE / SECTORS_PER_PAGE);
		return -ENOMEM;
	}

	kc = kmalloc(sizeof(*kc), GFP_KERNEL);
	if (!kc)
		return -ENOMEM;

	kc->lock = SPIN_LOCK_UNLOCKED;
	INIT_LIST_HEAD(&kc->pages);
	kc->nr_pages = kc->nr_free_pages = 0;
	r = client_alloc_pages(kc, nr_pages);
	if (r) {
		kfree(kc);
		return r;
	}

	r = dm_io_get(nr_pages);
	if (r) {
		client_free_pages(kc);
		kfree(kc);
		return r;
	}

	r = client_add(kc);
	if (r) {
		dm_io_put(nr_pages);
		client_free_pages(kc);
		kfree(kc);
		return r;
	}

	*result = kc;
	return 0;
}

void kcopyd_client_destroy(struct kcopyd_client *kc)
{
	dm_io_put(kc->nr_pages);
	client_free_pages(kc);
	client_del(kc);
	kfree(kc);
}


int __init kcopyd_init(void)
{
	int r;

	r = jobs_init();
	if (r)
		return r;

	r = dm_daemon_start(&_kcopyd, "kcopyd", do_work);
	if (r)
		jobs_exit();

	return r;
}

void kcopyd_exit(void)
{
	jobs_exit();
	dm_daemon_stop(&_kcopyd);
}

EXPORT_SYMBOL(kcopyd_client_create);
EXPORT_SYMBOL(kcopyd_client_destroy);
EXPORT_SYMBOL(kcopyd_copy);
EXPORT_SYMBOL(kcopyd_cancel);
