/*
 * dm-snapshot.c
 *
 * Copyright (C) 2001-2002 Sistina Software (UK) Limited.
 *
 * This file is released under the GPL.
 */

#include <linux/config.h>
#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/mempool.h>
#include <linux/device-mapper.h>
#include <linux/vmalloc.h>

#include "dm-snapshot.h"
#include "kcopyd.h"

/*
 * FIXME: Remove this before release.
 */
#if 0
#define DMDEBUG(x...) DMWARN( ## x)
#else
#define DMDEBUG(x...)
#endif

/*
 * The percentage increment we will wake up users at
 */
#define WAKE_UP_PERCENT 5

/*
 * kcopyd priority of snapshot operations
 */
#define SNAPSHOT_COPY_PRIORITY 2

/*
 * Each snapshot reserves this many pages for io
 * FIXME: calculate this
 */
#define SNAPSHOT_PAGES 256

struct pending_exception {
	struct exception e;

	/*
	 * Origin buffers waiting for this to complete are held
	 * in a list (using b_reqnext).
	 */
	struct buffer_head *origin_bhs;
	struct buffer_head *snapshot_bhs;

	/*
	 * Other pending_exceptions that are processing this
	 * chunk.  When this list is empty, we know we can
	 * complete the origins.
	 */
	struct list_head siblings;

	/* Pointer back to snapshot context */
	struct dm_snapshot *snap;

	/*
	 * 1 indicates the exception has already been sent to
	 * kcopyd.
	 */
	int started;
};

/*
 * Hash table mapping origin volumes to lists of snapshots and
 * a lock to protect it
 */
static kmem_cache_t *exception_cache;
static kmem_cache_t *pending_cache;
static mempool_t *pending_pool;

/*
 * One of these per registered origin, held in the snapshot_origins hash
 */
struct origin {
	/* The origin device */
	kdev_t dev;

	struct list_head hash_list;

	/* List of snapshots for this origin */
	struct list_head snapshots;
};

/*
 * Size of the hash table for origin volumes. If we make this
 * the size of the minors list then it should be nearly perfect
 */
#define ORIGIN_HASH_SIZE 256
#define ORIGIN_MASK      0xFF
static struct list_head *_origins;
static struct rw_semaphore _origins_lock;

static int init_origin_hash(void)
{
	int i;

	_origins = kmalloc(ORIGIN_HASH_SIZE * sizeof(struct list_head),
			   GFP_KERNEL);
	if (!_origins) {
		DMERR("Device mapper: Snapshot: unable to allocate memory");
		return -ENOMEM;
	}

	for (i = 0; i < ORIGIN_HASH_SIZE; i++)
		INIT_LIST_HEAD(_origins + i);
	init_rwsem(&_origins_lock);

	return 0;
}

static void exit_origin_hash(void)
{
	kfree(_origins);
}

static inline unsigned int origin_hash(kdev_t dev)
{
	return MINOR(dev) & ORIGIN_MASK;
}

static struct origin *__lookup_origin(kdev_t origin)
{
	struct list_head *slist;
	struct list_head *ol;
	struct origin *o;

	ol = &_origins[origin_hash(origin)];
	list_for_each(slist, ol) {
		o = list_entry(slist, struct origin, hash_list);

		if (o->dev == origin)
			return o;
	}

	return NULL;
}

static void __insert_origin(struct origin *o)
{
	struct list_head *sl = &_origins[origin_hash(o->dev)];
	list_add_tail(&o->hash_list, sl);
}

/*
 * Make a note of the snapshot and its origin so we can look it
 * up when the origin has a write on it.
 */
static int register_snapshot(struct dm_snapshot *snap)
{
	struct origin *o;
	kdev_t dev = snap->origin->dev;

	down_write(&_origins_lock);
	o = __lookup_origin(dev);

	if (!o) {
		/* New origin */
		o = kmalloc(sizeof(*o), GFP_KERNEL);
		if (!o) {
			up_write(&_origins_lock);
			return -ENOMEM;
		}

		/* Initialise the struct */
		INIT_LIST_HEAD(&o->snapshots);
		o->dev = dev;

		__insert_origin(o);
	}

	list_add_tail(&snap->list, &o->snapshots);

	up_write(&_origins_lock);
	return 0;
}

static void unregister_snapshot(struct dm_snapshot *s)
{
	struct origin *o;

	down_write(&_origins_lock);
	o = __lookup_origin(s->origin->dev);

	list_del(&s->list);
	if (list_empty(&o->snapshots)) {
		list_del(&o->hash_list);
		kfree(o);
	}

	up_write(&_origins_lock);
}

/*
 * Implementation of the exception hash tables.
 */
static int init_exception_table(struct exception_table *et, uint32_t size)
{
	unsigned int i;

	et->hash_mask = size - 1;
	et->table = vcalloc(size, sizeof(struct list_head));
	if (!et->table)
		return -ENOMEM;

	for (i = 0; i < size; i++)
		INIT_LIST_HEAD(et->table + i);

	return 0;
}

static void exit_exception_table(struct exception_table *et, kmem_cache_t *mem)
{
	struct list_head *slot, *entry, *temp;
	struct exception *ex;
	int i, size;

	size = et->hash_mask + 1;
	for (i = 0; i < size; i++) {
		slot = et->table + i;

		list_for_each_safe(entry, temp, slot) {
			ex = list_entry(entry, struct exception, hash_list);
			kmem_cache_free(mem, ex);
		}
	}

	vfree(et->table);
}

/*
 * FIXME: check how this hash fn is performing.
 */
static inline uint32_t exception_hash(struct exception_table *et, chunk_t chunk)
{
	return chunk & et->hash_mask;
}

static void insert_exception(struct exception_table *eh, struct exception *e)
{
	struct list_head *l = &eh->table[exception_hash(eh, e->old_chunk)];
	list_add(&e->hash_list, l);
}

static inline void remove_exception(struct exception *e)
{
	list_del(&e->hash_list);
}

/*
 * Return the exception data for a sector, or NULL if not
 * remapped.
 */
static struct exception *lookup_exception(struct exception_table *et,
					  chunk_t chunk)
{
	struct list_head *slot, *el;
	struct exception *e;

	slot = &et->table[exception_hash(et, chunk)];
	list_for_each(el, slot) {
		e = list_entry(el, struct exception, hash_list);
		if (e->old_chunk == chunk)
			return e;
	}

	return NULL;
}

static inline struct exception *alloc_exception(void)
{
	struct exception *e;

	e = kmem_cache_alloc(exception_cache, GFP_NOIO);
	if (!e)
		e = kmem_cache_alloc(exception_cache, GFP_ATOMIC);

	return e;
}

static inline void free_exception(struct exception *e)
{
	kmem_cache_free(exception_cache, e);
}

static inline struct pending_exception *alloc_pending_exception(void)
{
	return mempool_alloc(pending_pool, GFP_NOIO);
}

static inline void free_pending_exception(struct pending_exception *pe)
{
	mempool_free(pe, pending_pool);
}

int dm_add_exception(struct dm_snapshot *s, chunk_t old, chunk_t new)
{
	struct exception *e;

	e = alloc_exception();
	if (!e)
		return -ENOMEM;

	e->old_chunk = old;
	e->new_chunk = new;
	insert_exception(&s->complete, e);
	return 0;
}

/*
 * Hard coded magic.
 */
static int calc_max_buckets(void)
{
	unsigned long mem;

	mem = num_physpages << PAGE_SHIFT;
	mem /= 50;
	mem /= sizeof(struct list_head);

	return mem;
}

/*
 * Rounds a number down to a power of 2.
 */
static inline uint32_t round_down(uint32_t n)
{
	while (n & (n - 1))
		n &= (n - 1);
	return n;
}

/*
 * Allocate room for a suitable hash table.
 */
static int init_hash_tables(struct dm_snapshot *s)
{
	sector_t hash_size, cow_dev_size, origin_dev_size, max_buckets;

	/*
	 * Calculate based on the size of the original volume or
	 * the COW volume...
	 */
	cow_dev_size = get_dev_size(s->cow->dev);
	origin_dev_size = get_dev_size(s->origin->dev);
	max_buckets = calc_max_buckets();

	hash_size = min(origin_dev_size, cow_dev_size) / s->chunk_size;
	hash_size = min(hash_size, max_buckets);

	/* Round it down to a power of 2 */
	hash_size = round_down(hash_size);
	if (init_exception_table(&s->complete, hash_size))
		return -ENOMEM;

	/*
	 * Allocate hash table for in-flight exceptions
	 * Make this smaller than the real hash table
	 */
	hash_size >>= 3;
	if (!hash_size)
		hash_size = 64;

	if (init_exception_table(&s->pending, hash_size)) {
		exit_exception_table(&s->complete, exception_cache);
		return -ENOMEM;
	}

	return 0;
}

/*
 * Round a number up to the nearest 'size' boundary.  size must
 * be a power of 2.
 */
static inline ulong round_up(ulong n, ulong size)
{
	size--;
	return (n + size) & ~size;
}

/*
 * Construct a snapshot mapping: <origin_dev> <COW-dev> <p/n> <chunk-size>
 */
static int snapshot_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct dm_snapshot *s;
	unsigned long chunk_size;
	int r = -EINVAL;
	char persistent;
	char *origin_path;
	char *cow_path;
	char *value;
	int blocksize;

	if (argc < 4) {
		ti->error = "dm-snapshot: requires exactly 4 arguments";
		r = -EINVAL;
		goto bad1;
	}

	origin_path = argv[0];
	cow_path = argv[1];
	persistent = toupper(*argv[2]);

	if (persistent != 'P' && persistent != 'N') {
		ti->error = "Persistent flag is not P or N";
		r = -EINVAL;
		goto bad1;
	}

	chunk_size = simple_strtoul(argv[3], &value, 10);
	if (chunk_size == 0 || value == NULL) {
		ti->error = "Invalid chunk size";
		r = -EINVAL;
		goto bad1;
	}

	s = kmalloc(sizeof(*s), GFP_KERNEL);
	if (s == NULL) {
		ti->error = "Cannot allocate snapshot context private "
		    "structure";
		r = -ENOMEM;
		goto bad1;
	}

	r = dm_get_device(ti, origin_path, 0, ti->len, FMODE_READ, &s->origin);
	if (r) {
		ti->error = "Cannot get origin device";
		goto bad2;
	}

	/* FIXME: get cow length */
	r = dm_get_device(ti, cow_path, 0, 0,
			  FMODE_READ | FMODE_WRITE, &s->cow);
	if (r) {
		dm_put_device(ti, s->origin);
		ti->error = "Cannot get COW device";
		goto bad2;
	}

	/*
	 * Chunk size must be multiple of page size.  Silently
	 * round up if it's not.
	 */
	chunk_size = round_up(chunk_size, PAGE_SIZE / SECTOR_SIZE);

	/* Validate the chunk size against the device block size */
	blocksize = get_hardsect_size(s->cow->dev);
	if (chunk_size % (blocksize / SECTOR_SIZE)) {
		ti->error = "Chunk size is not a multiple of device blocksize";
		r = -EINVAL;
		goto bad3;
	}

	/* Check the sizes are small enough to fit in one kiovec */
	if (chunk_size > KIO_MAX_SECTORS) {
		ti->error = "Chunk size is too big";
		r = -EINVAL;
		goto bad3;
	}

	/* Check chunk_size is a power of 2 */
	if (chunk_size & (chunk_size - 1)) {
		ti->error = "Chunk size is not a power of 2";
		r = -EINVAL;
		goto bad3;
	}

	s->chunk_size = chunk_size;
	s->chunk_mask = chunk_size - 1;
	s->type = persistent;
	for (s->chunk_shift = 0; chunk_size;
	     s->chunk_shift++, chunk_size >>= 1)
		;
	s->chunk_shift--;

	s->valid = 1;
	s->have_metadata = 0;
	s->last_percent = 0;
	init_rwsem(&s->lock);
	s->table = ti->table;

	/* Allocate hash table for COW data */
	if (init_hash_tables(s)) {
		ti->error = "Unable to allocate hash table space";
		r = -ENOMEM;
		goto bad3;
	}

	/*
	 * Check the persistent flag - done here because we need the iobuf
	 * to check the LV header
	 */
	s->store.snap = s;

	if (persistent == 'P')
		r = dm_create_persistent(&s->store, s->chunk_size);
	else
		r = dm_create_transient(&s->store, s, blocksize);

	if (r) {
		ti->error = "Couldn't create exception store";
		r = -EINVAL;
		goto bad4;
	}

	r = kcopyd_client_create(SNAPSHOT_PAGES, &s->kcopyd_client);
	if (r) {
		ti->error = "Could not create kcopyd client";
		goto bad5;
	}

	/* Flush IO to the origin device */
	fsync_dev(s->origin->dev);

	/* Add snapshot to the list of snapshots for this origin */
	if (register_snapshot(s)) {
		r = -EINVAL;
		ti->error = "Cannot register snapshot origin";
		goto bad6;
	}

	ti->private = s;
	return 0;

 bad6:
	kcopyd_client_destroy(s->kcopyd_client);

 bad5:
	s->store.destroy(&s->store);

 bad4:
	exit_exception_table(&s->pending, pending_cache);
	exit_exception_table(&s->complete, exception_cache);

 bad3:
	dm_put_device(ti, s->cow);
	dm_put_device(ti, s->origin);

 bad2:
	kfree(s);

 bad1:
	return r;
}

static void snapshot_dtr(struct dm_target *ti)
{
	struct dm_snapshot *s = (struct dm_snapshot *) ti->private;

	dm_table_event(ti->table);

	unregister_snapshot(s);

	exit_exception_table(&s->pending, pending_cache);
	exit_exception_table(&s->complete, exception_cache);

	/* Deallocate memory used */
	s->store.destroy(&s->store);

	dm_put_device(ti, s->origin);
	dm_put_device(ti, s->cow);
	kcopyd_client_destroy(s->kcopyd_client);
	kfree(s);
}

/*
 * We hold lists of buffer_heads, using the b_reqnext field.
 */
static void queue_buffer(struct buffer_head **queue, struct buffer_head *bh)
{
	bh->b_reqnext = *queue;
	*queue = bh;
}

/*
 * FIXME: inefficient.
 */
static void queue_buffers(struct buffer_head **queue, struct buffer_head *bhs)
{
	while (*queue)
		queue = &((*queue)->b_reqnext);

	*queue = bhs;
}

/*
 * Flush a list of buffers.
 */
static void flush_buffers(struct buffer_head *bh)
{
	struct buffer_head *n;

	DMDEBUG("begin flush");
	while (bh) {
		n = bh->b_reqnext;
		bh->b_reqnext = NULL;
		DMDEBUG("flushing %p", bh);
		generic_make_request(WRITE, bh);
		bh = n;
	}

	run_task_queue(&tq_disk);
}

/*
 * Error a list of buffers.
 */
static void error_buffers(struct buffer_head *bh)
{
	struct buffer_head *n;

	while (bh) {
		n = bh->b_reqnext;
		bh->b_reqnext = NULL;
		buffer_IO_error(bh);
		bh = n;
	}
}

static struct buffer_head *__flush_bhs(struct pending_exception *pe)
{
	struct pending_exception *sibling;

	if (list_empty(&pe->siblings))
		return pe->origin_bhs;

	sibling = list_entry(pe->siblings.next,
			     struct pending_exception, siblings);

	list_del(&pe->siblings);

	/* FIXME: I think there's a race on SMP machines here, add spin lock */
	queue_buffers(&sibling->origin_bhs, pe->origin_bhs);

	return NULL;
}

static void pending_complete(struct pending_exception *pe, int success)
{
	struct exception *e;
	struct dm_snapshot *s = pe->snap;
	struct buffer_head *flush = NULL;

	if (success) {
		e = alloc_exception();
		if (!e) {
			DMWARN("Unable to allocate exception.");
			down_write(&s->lock);
			s->store.drop_snapshot(&s->store);
			s->valid = 0;
			flush = __flush_bhs(pe);
			up_write(&s->lock);

			error_buffers(pe->snapshot_bhs);
			goto out;
		}

		/*
		 * Add a proper exception, and remove the
		 * in-flight exception from the list.
		 */
		down_write(&s->lock);

		memcpy(e, &pe->e, sizeof(*e));
		insert_exception(&s->complete, e);
		remove_exception(&pe->e);
		flush = __flush_bhs(pe);

		/* Submit any pending write BHs */
		up_write(&s->lock);

		flush_buffers(pe->snapshot_bhs);
		DMDEBUG("Exception completed successfully.");

		/* Notify any interested parties */
		if (s->store.fraction_full) {
			sector_t numerator, denominator;
			int pc;

			s->store.fraction_full(&s->store, &numerator,
					       &denominator);
			pc = numerator * 100 / denominator;

			if (pc >= s->last_percent + WAKE_UP_PERCENT) {
				dm_table_event(s->table);
				s->last_percent = pc - pc % WAKE_UP_PERCENT;
			}
		}

	} else {
		/* Read/write error - snapshot is unusable */
		down_write(&s->lock);
		if (s->valid)
			DMERR("Error reading/writing snapshot");
		s->store.drop_snapshot(&s->store);
		s->valid = 0;
		remove_exception(&pe->e);
		flush = __flush_bhs(pe);
		up_write(&s->lock);

		error_buffers(pe->snapshot_bhs);

		dm_table_event(s->table);
		DMDEBUG("Exception failed.");
	}

 out:
	if (flush)
		flush_buffers(flush);

	free_pending_exception(pe);
}

static void commit_callback(void *context, int success)
{
	struct pending_exception *pe = (struct pending_exception *) context;
	pending_complete(pe, success);
}

/*
 * Called when the copy I/O has finished.  kcopyd actually runs
 * this code so don't block.
 */
static void copy_callback(int read_err, unsigned int write_err, void *context)
{
	struct pending_exception *pe = (struct pending_exception *) context;
	struct dm_snapshot *s = pe->snap;

	if (read_err || write_err)
		pending_complete(pe, 0);

	else
		/* Update the metadata if we are persistent */
		s->store.commit_exception(&s->store, &pe->e, commit_callback,
					  pe);
}

/*
 * Dispatches the copy operation to kcopyd.
 */
static inline void start_copy(struct pending_exception *pe)
{
	struct dm_snapshot *s = pe->snap;
	struct io_region src, dest;
	kdev_t dev = s->origin->dev;
	int *sizes = blk_size[major(dev)];
	sector_t dev_size = (sector_t) -1;

	if (pe->started)
		return;

	/* this is protected by snap->lock */
	pe->started = 1;

	if (sizes && sizes[minor(dev)])
		dev_size = sizes[minor(dev)] << 1;

	src.dev = dev;
	src.sector = chunk_to_sector(s, pe->e.old_chunk);
	src.count = min(s->chunk_size, dev_size - src.sector);

	dest.dev = s->cow->dev;
	dest.sector = chunk_to_sector(s, pe->e.new_chunk);
	dest.count = src.count;

	/* Hand over to kcopyd */
	kcopyd_copy(s->kcopyd_client,
		    &src, 1, &dest, 0, copy_callback, pe);
}

/*
 * Looks to see if this snapshot already has a pending exception
 * for this chunk, otherwise it allocates a new one and inserts
 * it into the pending table.
 */
static struct pending_exception *find_pending_exception(struct dm_snapshot *s,
							struct buffer_head *bh)
{
	struct exception *e;
	struct pending_exception *pe;
	chunk_t chunk = sector_to_chunk(s, bh->b_rsector);

	/*
	 * Is there a pending exception for this already ?
	 */
	e = lookup_exception(&s->pending, chunk);
	if (e) {
		/* cast the exception to a pending exception */
		pe = list_entry(e, struct pending_exception, e);

	} else {
		/* Create a new pending exception */
		pe = alloc_pending_exception();
		pe->e.old_chunk = chunk;
		pe->origin_bhs = pe->snapshot_bhs = NULL;
		INIT_LIST_HEAD(&pe->siblings);
		pe->snap = s;
		pe->started = 0;

		if (s->store.prepare_exception(&s->store, &pe->e)) {
			free_pending_exception(pe);
			s->valid = 0;
			return NULL;
		}

		insert_exception(&s->pending, &pe->e);
	}

	return pe;
}

static inline void remap_exception(struct dm_snapshot *s, struct exception *e,
				   struct buffer_head *bh)
{
	bh->b_rdev = s->cow->dev;
	bh->b_rsector = chunk_to_sector(s, e->new_chunk) +
	    (bh->b_rsector & s->chunk_mask);
}

static int snapshot_map(struct dm_target *ti, struct buffer_head *bh, int rw,
			union map_info *map_context)
{
	struct exception *e;
	struct dm_snapshot *s = (struct dm_snapshot *) ti->private;
	int r = 1;
	chunk_t chunk;
	struct pending_exception *pe;

	chunk = sector_to_chunk(s, bh->b_rsector);

	/* Full snapshots are not usable */
	if (!s->valid)
		return -1;

	/*
	 * Write to snapshot - higher level takes care of RW/RO
	 * flags so we should only get this if we are
	 * writeable.
	 */
	if (rw == WRITE) {

		down_write(&s->lock);

		/* If the block is already remapped - use that, else remap it */
		e = lookup_exception(&s->complete, chunk);
		if (e)
			remap_exception(s, e, bh);

		else {
			pe = find_pending_exception(s, bh);

			if (!pe) {
				s->store.drop_snapshot(&s->store);
				s->valid = 0;
				r = -EIO;
			} else {
				remap_exception(s, &pe->e, bh);
				queue_buffer(&pe->snapshot_bhs, bh);
				start_copy(pe);
				r = 0;
			}
		}

		up_write(&s->lock);

	} else {
		/*
		 * FIXME: this read path scares me because we
		 * always use the origin when we have a pending
		 * exception.  However I can't think of a
		 * situation where this is wrong - ejt.
		 */

		/* Do reads */
		down_read(&s->lock);

		/* See if it it has been remapped */
		e = lookup_exception(&s->complete, chunk);
		if (e)
			remap_exception(s, e, bh);
		else
			bh->b_rdev = s->origin->dev;

		up_read(&s->lock);
	}

	return r;
}

void snapshot_resume(struct dm_target *ti)
{
	struct dm_snapshot *s = (struct dm_snapshot *) ti->private;

	if (s->have_metadata)
		return;

	if (s->store.read_metadata(&s->store)) {
		down_write(&s->lock);
		s->valid = 0;
		up_write(&s->lock);
	}

	s->have_metadata = 1;
}

static int snapshot_status(struct dm_target *ti, status_type_t type,
			   char *result, unsigned int maxlen)
{
	struct dm_snapshot *snap = (struct dm_snapshot *) ti->private;
	char cow[16];
	char org[16];

	switch (type) {
	case STATUSTYPE_INFO:
		if (!snap->valid)
			snprintf(result, maxlen, "Invalid");
		else {
			if (snap->store.fraction_full) {
				sector_t numerator, denominator;
				snap->store.fraction_full(&snap->store,
							  &numerator,
							  &denominator);
				snprintf(result, maxlen,
					 SECTOR_FORMAT "/" SECTOR_FORMAT,
					 numerator, denominator);
			}
			else
				snprintf(result, maxlen, "Unknown");
		}
		break;

	case STATUSTYPE_TABLE:
		/*
		 * kdevname returns a static pointer so we need
		 * to make private copies if the output is to
		 * make sense.
		 */
		strncpy(cow, dm_kdevname(snap->cow->dev), sizeof(cow));
		strncpy(org, dm_kdevname(snap->origin->dev), sizeof(org));
		snprintf(result, maxlen, "%s %s %c %ld", org, cow,
			 snap->type, snap->chunk_size);
		break;
	}

	return 0;
}

/*-----------------------------------------------------------------
 * Origin methods
 *---------------------------------------------------------------*/
static void list_merge(struct list_head *l1, struct list_head *l2)
{
	struct list_head *l1_n, *l2_p;

	l1_n = l1->next;
	l2_p = l2->prev;

	l1->next = l2;
	l2->prev = l1;

	l2_p->next = l1_n;
	l1_n->prev = l2_p;
}

static int __origin_write(struct list_head *snapshots, struct buffer_head *bh)
{
	int r = 1, first = 1;
	struct list_head *sl;
	struct dm_snapshot *snap;
	struct exception *e;
	struct pending_exception *pe, *last = NULL;
	chunk_t chunk;

	/* Do all the snapshots on this origin */
	list_for_each(sl, snapshots) {
		snap = list_entry(sl, struct dm_snapshot, list);

		/* Only deal with valid snapshots */
		if (!snap->valid)
			continue;

		down_write(&snap->lock);

		/*
		 * Remember, different snapshots can have
		 * different chunk sizes.
		 */
		chunk = sector_to_chunk(snap, bh->b_rsector);

		/*
		 * Check exception table to see if block
		 * is already remapped in this snapshot
		 * and trigger an exception if not.
		 */
		e = lookup_exception(&snap->complete, chunk);
		if (!e) {
			pe = find_pending_exception(snap, bh);
			if (!pe) {
				snap->store.drop_snapshot(&snap->store);
				snap->valid = 0;

			} else {
				if (last)
					list_merge(&pe->siblings,
						   &last->siblings);

				last = pe;
				r = 0;
			}
		}

		up_write(&snap->lock);
	}

	/*
	 * Now that we have a complete pe list we can start the copying.
	 */
	if (last) {
		pe = last;
		do {
			down_write(&pe->snap->lock);
			if (first)
				queue_buffer(&pe->origin_bhs, bh);
			start_copy(pe);
			up_write(&pe->snap->lock);
			first = 0;
			pe = list_entry(pe->siblings.next,
					struct pending_exception, siblings);

		} while (pe != last);
	}

	return r;
}

/*
 * Called on a write from the origin driver.
 */
int do_origin(struct dm_dev *origin, struct buffer_head *bh)
{
	struct origin *o;
	int r;

	down_read(&_origins_lock);
	o = __lookup_origin(origin->dev);
	if (!o)
		BUG();

	r = __origin_write(&o->snapshots, bh);
	up_read(&_origins_lock);

	return r;
}

/*
 * Origin: maps a linear range of a device, with hooks for snapshotting.
 */

/*
 * Construct an origin mapping: <dev_path>
 * The context for an origin is merely a 'struct dm_dev *'
 * pointing to the real device.
 */
static int origin_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	int r;
	struct dm_dev *dev;

	if (argc != 1) {
		ti->error = "dm-origin: incorrect number of arguments";
		return -EINVAL;
	}

	r = dm_get_device(ti, argv[0], 0, ti->len,
			  dm_table_get_mode(ti->table), &dev);
	if (r) {
		ti->error = "Cannot get target device";
		return r;
	}

	ti->private = dev;
	return 0;
}

static void origin_dtr(struct dm_target *ti)
{
	struct dm_dev *dev = (struct dm_dev *) ti->private;
	dm_put_device(ti, dev);
}

static int origin_map(struct dm_target *ti, struct buffer_head *bh, int rw,
		      union map_info *map_context)
{
	struct dm_dev *dev = (struct dm_dev *) ti->private;
	bh->b_rdev = dev->dev;

	/* Only tell snapshots if this is a write */
	return (rw == WRITE) ? do_origin(dev, bh) : 1;
}

static int origin_status(struct dm_target *ti, status_type_t type, char *result,
			 unsigned int maxlen)
{
	struct dm_dev *dev = (struct dm_dev *) ti->private;

	switch (type) {
	case STATUSTYPE_INFO:
		result[0] = '\0';
		break;

	case STATUSTYPE_TABLE:
		snprintf(result, maxlen, "%s", dm_kdevname(dev->dev));
		break;
	}

	return 0;
}

static struct target_type origin_target = {
	name:	"snapshot-origin",
	version: {1, 0, 1},
	module:	THIS_MODULE,
	ctr:	origin_ctr,
	dtr:	origin_dtr,
	map:	origin_map,
	status:	origin_status,
};

static struct target_type snapshot_target = {
	name:	"snapshot",
	version: {1, 0, 1},
	module:	THIS_MODULE,
	ctr:	snapshot_ctr,
	dtr:	snapshot_dtr,
	map:	snapshot_map,
	resume: snapshot_resume,
	status:	snapshot_status,
};

int __init dm_snapshot_init(void)
{
	int r;

	r = dm_register_target(&snapshot_target);
	if (r) {
		DMERR("snapshot target register failed %d", r);
		return r;
	}

	r = dm_register_target(&origin_target);
	if (r < 0) {
		DMERR("Device mapper: Origin: register failed %d\n", r);
		goto bad1;
	}

	r = init_origin_hash();
	if (r) {
		DMERR("init_origin_hash failed.");
		goto bad2;
	}

	exception_cache = kmem_cache_create("dm-snapshot-ex",
					    sizeof(struct exception),
					    __alignof__(struct exception),
					    0, NULL, NULL);
	if (!exception_cache) {
		DMERR("Couldn't create exception cache.");
		r = -ENOMEM;
		goto bad3;
	}

	pending_cache =
	    kmem_cache_create("dm-snapshot-in",
			      sizeof(struct pending_exception),
			      __alignof__(struct pending_exception),
			      0, NULL, NULL);
	if (!pending_cache) {
		DMERR("Couldn't create pending cache.");
		r = -ENOMEM;
		goto bad4;
	}

	pending_pool = mempool_create(128, mempool_alloc_slab,
				      mempool_free_slab, pending_cache);
	if (!pending_pool) {
		DMERR("Couldn't create pending pool.");
		r = -ENOMEM;
		goto bad5;
	}

	return 0;

      bad5:
	kmem_cache_destroy(pending_cache);
      bad4:
	kmem_cache_destroy(exception_cache);
      bad3:
	exit_origin_hash();
      bad2:
	dm_unregister_target(&origin_target);
      bad1:
	dm_unregister_target(&snapshot_target);
	return r;
}

void dm_snapshot_exit(void)
{
	int r;

	r = dm_unregister_target(&snapshot_target);
	if (r)
		DMERR("snapshot unregister failed %d", r);

	r = dm_unregister_target(&origin_target);
	if (r)
		DMERR("origin unregister failed %d", r);

	exit_origin_hash();
	mempool_destroy(pending_pool);
	kmem_cache_destroy(pending_cache);
	kmem_cache_destroy(exception_cache);
}
