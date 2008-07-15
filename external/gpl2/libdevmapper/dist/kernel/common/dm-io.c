/*
 * Copyright (C) 2003 Sistina Software
 *
 * This file is released under the GPL.
 */

#include "dm-io.h"

#include <linux/mempool.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/bitops.h>

/* FIXME: can we shrink this ? */
struct io_context {
	int rw;
	unsigned int error;
	atomic_t count;
	struct task_struct *sleeper;
	io_notify_fn callback;
	void *context;
};

/*
 * We maintain a pool of buffer heads for dispatching the io.
 */
static unsigned int _num_bhs;
static mempool_t *_buffer_pool;

/*
 * io contexts are only dynamically allocated for asynchronous
 * io.  Since async io is likely to be the majority of io we'll
 * have the same number of io contexts as buffer heads ! (FIXME:
 * must reduce this).
 */
mempool_t *_io_pool;

static void *alloc_bh(int gfp_mask, void *pool_data)
{
	struct buffer_head *bh;

	bh = kmem_cache_alloc(bh_cachep, gfp_mask);
	if (bh) {
		bh->b_reqnext = NULL;
		init_waitqueue_head(&bh->b_wait);
		INIT_LIST_HEAD(&bh->b_inode_buffers);
	}

	return bh;
}

static void *alloc_io(int gfp_mask, void *pool_data)
{
	return kmalloc(sizeof(struct io_context), gfp_mask);
}

static void free_io(void *element, void *pool_data)
{
	kfree(element);
}

static unsigned int pages_to_buffers(unsigned int pages)
{
	return 4 * pages;	/* too many ? */
}

static int resize_pool(unsigned int new_bhs)
{
	int r = 0;

	if (_buffer_pool) {
		if (new_bhs == 0) {
			/* free off the pools */
			mempool_destroy(_buffer_pool);
			mempool_destroy(_io_pool);
			_buffer_pool = _io_pool = NULL;
		} else {
			/* resize the pools */
			r = mempool_resize(_buffer_pool, new_bhs, GFP_KERNEL);
			if (!r)
				r = mempool_resize(_io_pool,
						   new_bhs, GFP_KERNEL);
		}
	} else {
		/* create new pools */
		_buffer_pool = mempool_create(new_bhs, alloc_bh,
					      mempool_free_slab, bh_cachep);
		if (!_buffer_pool)
			r = -ENOMEM;

		_io_pool = mempool_create(new_bhs, alloc_io, free_io, NULL);
		if (!_io_pool) {
			mempool_destroy(_buffer_pool);
			_buffer_pool = NULL;
			r = -ENOMEM;
		}
	}

	if (!r)
		_num_bhs = new_bhs;

	return r;
}

int dm_io_get(unsigned int num_pages)
{
	return resize_pool(_num_bhs + pages_to_buffers(num_pages));
}

void dm_io_put(unsigned int num_pages)
{
	resize_pool(_num_bhs - pages_to_buffers(num_pages));
}

/*-----------------------------------------------------------------
 * We need to keep track of which region a buffer is doing io
 * for.  In order to save a memory allocation we store this in an
 * unused field of the buffer head, and provide these access
 * functions.
 *
 * FIXME: add compile time check that an unsigned int can fit
 * into a pointer.
 *
 *---------------------------------------------------------------*/
static inline void bh_set_region(struct buffer_head *bh, unsigned int region)
{
	bh->b_journal_head = (void *) region;
}

static inline int bh_get_region(struct buffer_head *bh)
{
	return (unsigned int) bh->b_journal_head;
}

/*-----------------------------------------------------------------
 * We need an io object to keep track of the number of bhs that
 * have been dispatched for a particular io.
 *---------------------------------------------------------------*/
static void dec_count(struct io_context *io, unsigned int region, int error)
{
	if (error)
		set_bit(region, &io->error);

	if (atomic_dec_and_test(&io->count)) {
		if (io->sleeper)
			wake_up_process(io->sleeper);

		else {
			int r = io->error;
			io_notify_fn fn = io->callback;
			void *context = io->context;

			mempool_free(io, _io_pool);
			fn(r, context);
		}
	}
}

static void endio(struct buffer_head *bh, int uptodate)
{
	struct io_context *io = (struct io_context *) bh->b_private;

	if (!uptodate && io->rw != WRITE) {
		/*
		 * We need to zero this region, otherwise people
		 * like kcopyd may write the arbitrary contents
		 * of the page.
		 */
		memset(bh->b_data, 0, bh->b_size);
	}

	dec_count((struct io_context *) bh->b_private,
		  bh_get_region(bh), !uptodate);
	mempool_free(bh, _buffer_pool);
}

/*
 * Primitives for alignment calculations.
 */
int fls(unsigned n)
{
	return generic_fls32(n);
}

static inline int log2_floor(unsigned n)
{
	return ffs(n) - 1;
}

static inline int log2_align(unsigned n)
{
	return fls(n) - 1;
}

/*
 * Returns the next block for io.
 */
static int do_page(kdev_t dev, sector_t *block, sector_t end_block,
		   unsigned int block_size,
		   struct page *p, unsigned int offset,
		   unsigned int region, struct io_context *io)
{
	struct buffer_head *bh;
	sector_t b = *block;
	sector_t blocks_per_page = PAGE_SIZE / block_size;
	unsigned int this_size; /* holds the size of the current io */
	sector_t len;

	if (!blocks_per_page) {
		DMERR("dm-io: PAGE_SIZE (%lu) < block_size (%u) unsupported",
		      PAGE_SIZE, block_size);
		return 0;
	}

	while ((offset < PAGE_SIZE) && (b != end_block)) {
		bh = mempool_alloc(_buffer_pool, GFP_NOIO);
		init_buffer(bh, endio, io);
		bh_set_region(bh, region);

		/*
		 * Block size must be a power of 2 and aligned
		 * correctly.
		 */

		len = min(end_block - b, blocks_per_page);
		len = min(len, blocks_per_page - offset / block_size);

		if (!len) {
			DMERR("dm-io: Invalid offset/block_size (%u/%u).",
			      offset, block_size);
			return 0;
		}

		this_size = 1 << log2_align(len);
		if (b)
			this_size = min(this_size,
					(unsigned) 1 << log2_floor(b));

		/*
		 * Add in the job offset.
		 */
		bh->b_blocknr = (b / this_size);
		bh->b_size = block_size * this_size;
		set_bh_page(bh, p, offset);
		bh->b_this_page = bh;

		bh->b_dev = dev;
		atomic_set(&bh->b_count, 1);

		bh->b_state = ((1 << BH_Uptodate) | (1 << BH_Mapped) |
			       (1 << BH_Lock));

		if (io->rw == WRITE)
			clear_bit(BH_Dirty, &bh->b_state);

		atomic_inc(&io->count);
		submit_bh(io->rw, bh);

		b += this_size;
		offset += block_size * this_size;
	}

	*block = b;
	return (b == end_block);
}

static void do_region(unsigned int region, struct io_region *where,
		      struct page *page, unsigned int offset,
		      struct io_context *io)
{
	unsigned int block_size = get_hardsect_size(where->dev);
	unsigned int sblock_size = block_size >> 9;
	sector_t block = where->sector / sblock_size;
	sector_t end_block = (where->sector + where->count) / sblock_size;

	while (1) {
		if (do_page(where->dev, &block, end_block, block_size,
			    page, offset, region, io))
			break;

		offset = 0;	/* only offset the first page */

		page = list_entry(page->list.next, struct page, list);
	}
}

static void dispatch_io(unsigned int num_regions, struct io_region *where,
			struct page *pages, unsigned int offset,
			struct io_context *io)
{
	int i;

	for (i = 0; i < num_regions; i++)
		if (where[i].count)
			do_region(i, where + i, pages, offset, io);

	/*
	 * Drop the extra refence that we were holding to avoid
	 * the io being completed too early.
	 */
	dec_count(io, 0, 0);
}

/*
 * Synchronous io
 */
int dm_io_sync(unsigned int num_regions, struct io_region *where,
	       int rw, struct page *pages, unsigned int offset,
	       unsigned int *error_bits)
{
	struct io_context io;

	BUG_ON(num_regions > 1 && rw != WRITE);

	io.rw = rw;
	io.error = 0;
	atomic_set(&io.count, 1); /* see dispatch_io() */
	io.sleeper = current;

	dispatch_io(num_regions, where, pages, offset, &io);
	run_task_queue(&tq_disk);

	while (1) {
		set_current_state(TASK_UNINTERRUPTIBLE);

		if (!atomic_read(&io.count))
			break;

		schedule();
	}
	set_current_state(TASK_RUNNING);

	*error_bits = io.error;
	return io.error ? -EIO : 0;
}

/*
 * Asynchronous io
 */
int dm_io_async(unsigned int num_regions, struct io_region *where, int rw,
		struct page *pages, unsigned int offset,
		io_notify_fn fn, void *context)
{
	struct io_context *io = mempool_alloc(_io_pool, GFP_NOIO);

	io->rw = rw;
	io->error = 0;
	atomic_set(&io->count, 1); /* see dispatch_io() */
	io->sleeper = NULL;
	io->callback = fn;
	io->context = context;

	dispatch_io(num_regions, where, pages, offset, io);
	return 0;
}

EXPORT_SYMBOL(dm_io_get);
EXPORT_SYMBOL(dm_io_put);
EXPORT_SYMBOL(dm_io_sync);
EXPORT_SYMBOL(dm_io_async);
