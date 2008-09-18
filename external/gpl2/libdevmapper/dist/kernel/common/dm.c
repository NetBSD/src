/*
 * Copyright (C) 2001, 2002 Sistina Software (UK) Limited.
 *
 * This file is released under the GPL.
 */

#include "dm.h"
#include "kcopyd.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/blk.h>
#include <linux/blkpg.h>
#include <linux/mempool.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/kdev_t.h>
#include <linux/lvm.h>

#include <asm/uaccess.h>

static const char *_name = DM_NAME;
#define DEFAULT_READ_AHEAD 64

struct dm_io {
	struct mapped_device *md;

	struct dm_target *ti;
	int rw;
	union map_info map_context;
	void (*end_io) (struct buffer_head * bh, int uptodate);
	void *context;
};

struct deferred_io {
	int rw;
	struct buffer_head *bh;
	struct deferred_io *next;
};

/*
 * Bits for the md->flags field.
 */
#define DMF_BLOCK_IO 0
#define DMF_SUSPENDED 1

struct mapped_device {
	struct rw_semaphore lock;
	atomic_t holders;

	kdev_t dev;
	unsigned long flags;

	/*
	 * A list of ios that arrived while we were suspended.
	 */
	atomic_t pending;
	wait_queue_head_t wait;
	struct deferred_io *deferred;

	/*
	 * The current mapping.
	 */
	struct dm_table *map;

	/*
	 * io objects are allocated from here.
	 */
	mempool_t *io_pool;

	/*
	 * Event handling.
	 */
	uint32_t event_nr;
	wait_queue_head_t eventq;
};

#define MIN_IOS 256
static kmem_cache_t *_io_cache;

static struct mapped_device *get_kdev(kdev_t dev);
static int dm_request(request_queue_t *q, int rw, struct buffer_head *bh);
static int dm_user_bmap(struct inode *inode, struct lv_bmap *lvb);

/*-----------------------------------------------------------------
 * In order to avoid the 256 minor number limit we are going to
 * register more major numbers as neccessary.
 *---------------------------------------------------------------*/
#define MAX_MINORS (1 << MINORBITS)

struct major_details {
	unsigned int major;

	int transient;
	struct list_head transient_list;

	unsigned int first_free_minor;
	int nr_free_minors;

	struct mapped_device *mds[MAX_MINORS];
	int blk_size[MAX_MINORS];
	int blksize_size[MAX_MINORS];
	int hardsect_size[MAX_MINORS];
};

static struct rw_semaphore _dev_lock;
static struct major_details *_majors[MAX_BLKDEV];

/*
 * This holds a list of majors that non-specified device numbers
 * may be allocated from.  Only majors with free minors appear on
 * this list.
 */
static LIST_HEAD(_transients_free);

static int __alloc_major(unsigned int major, struct major_details **result)
{
	int r;
	unsigned int transient = !major;
	struct major_details *maj;

	/* Major already allocated? */
	if (major && _majors[major])
		return 0;

	maj = kmalloc(sizeof(*maj), GFP_KERNEL);
	if (!maj)
		return -ENOMEM;

	memset(maj, 0, sizeof(*maj));
	INIT_LIST_HEAD(&maj->transient_list);

	maj->nr_free_minors = MAX_MINORS;

	r = register_blkdev(major, _name, &dm_blk_dops);
	if (r < 0) {
		DMERR("register_blkdev failed for %d", major);
		kfree(maj);
		return r;
	}
	if (r > 0)
		major = r;

	maj->major = major;

	if (transient) {
		maj->transient = transient;
		list_add_tail(&maj->transient_list, &_transients_free);
	}

	_majors[major] = maj;

	blk_size[major] = maj->blk_size;
	blksize_size[major] = maj->blksize_size;
	hardsect_size[major] = maj->hardsect_size;
	read_ahead[major] = DEFAULT_READ_AHEAD;

	blk_queue_make_request(BLK_DEFAULT_QUEUE(major), dm_request);

	*result = maj;
	return 0;
}

static void __free_major(struct major_details *maj)
{
	unsigned int major = maj->major;

	list_del(&maj->transient_list);

	read_ahead[major] = 0;
	blk_size[major] = NULL;
	blksize_size[major] = NULL;
	hardsect_size[major] = NULL;

	_majors[major] = NULL;
	kfree(maj);

	if (unregister_blkdev(major, _name) < 0)
		DMERR("devfs_unregister_blkdev failed");
}

static void free_all_majors(void)
{
	unsigned int major = ARRAY_SIZE(_majors);

	down_write(&_dev_lock);

	while (major--)
		if (_majors[major])
			__free_major(_majors[major]);

	up_write(&_dev_lock);
}

static void free_dev(kdev_t dev)
{
	unsigned int major = major(dev);
	unsigned int minor = minor(dev);
	struct major_details *maj;

	down_write(&_dev_lock);

	maj = _majors[major];
	if (!maj)
		goto out;

	maj->mds[minor] = NULL;
	maj->nr_free_minors++;

	if (maj->nr_free_minors == MAX_MINORS) {
		__free_major(maj);
		goto out;
	}

	if (!maj->transient)
		goto out;

	if (maj->nr_free_minors == 1)
		list_add_tail(&maj->transient_list, &_transients_free);

	if (minor < maj->first_free_minor)
		maj->first_free_minor = minor;

      out:
	up_write(&_dev_lock);
}

static void __alloc_minor(struct major_details *maj, unsigned int minor,
			  struct mapped_device *md)
{
	maj->mds[minor] = md;
	md->dev = mk_kdev(maj->major, minor);
	maj->nr_free_minors--;

	if (maj->transient && !maj->nr_free_minors)
		list_del_init(&maj->transient_list);
}

/*
 * See if requested kdev_t is available.
 */
static int specific_dev(kdev_t dev, struct mapped_device *md)
{
	int r = 0;
	unsigned int major = major(dev);
	unsigned int minor = minor(dev);
	struct major_details *maj;

	if (!major || (major > MAX_BLKDEV) || (minor >= MAX_MINORS)) {
		DMWARN("device number requested out of range (%d, %d)",
		       major, minor);
		return -EINVAL;
	}

	down_write(&_dev_lock);
	maj = _majors[major];

	/* Register requested major? */
	if (!maj) {
		r = __alloc_major(major, &maj);
		if (r)
			goto out;

		major = maj->major;
	}

	if (maj->mds[minor]) {
		r = -EBUSY;
		goto out;
	}

	__alloc_minor(maj, minor, md);

      out:
	up_write(&_dev_lock);

	return r;
}

/*
 * Find first unused device number, requesting a new major number if required.
 */
static int first_free_dev(struct mapped_device *md)
{
	int r = 0;
	struct major_details *maj;

	down_write(&_dev_lock);

	if (list_empty(&_transients_free)) {
		r = __alloc_major(0, &maj);
		if (r)
			goto out;
	} else
		maj = list_entry(_transients_free.next, struct major_details,
				 transient_list);

	while (maj->mds[maj->first_free_minor++])
		;

	__alloc_minor(maj, maj->first_free_minor - 1, md);

      out:
	up_write(&_dev_lock);

	return r;
}

static struct mapped_device *get_kdev(kdev_t dev)
{
	struct mapped_device *md;
	struct major_details *maj;

	down_read(&_dev_lock);
	maj = _majors[major(dev)];
	if (!maj) {
		md = NULL;
		goto out;
	}
	md = maj->mds[minor(dev)];
	if (md)
		dm_get(md);
      out:
	up_read(&_dev_lock);

	return md;
}

/*-----------------------------------------------------------------
 * init/exit code
 *---------------------------------------------------------------*/

static __init int local_init(void)
{
	init_rwsem(&_dev_lock);

	/* allocate a slab for the dm_ios */
	_io_cache = kmem_cache_create("dm io",
				      sizeof(struct dm_io), 0, 0, NULL, NULL);

	if (!_io_cache)
		return -ENOMEM;

	return 0;
}

static void local_exit(void)
{
	kmem_cache_destroy(_io_cache);
	free_all_majors();

	DMINFO("cleaned up");
}

/*
 * We have a lot of init/exit functions, so it seems easier to
 * store them in an array.  The disposable macro 'xx'
 * expands a prefix into a pair of function names.
 */
static struct {
	int (*init) (void);
	void (*exit) (void);

} _inits[] = {
#define xx(n) {n ## _init, n ## _exit},
	xx(local)
	xx(kcopyd)
	xx(dm_target)
	xx(dm_linear)
	xx(dm_stripe)
	xx(dm_snapshot)
	xx(dm_interface)
#undef xx
};

static int __init dm_init(void)
{
	const int count = ARRAY_SIZE(_inits);

	int r, i;

	for (i = 0; i < count; i++) {
		r = _inits[i].init();
		if (r)
			goto bad;
	}

	return 0;

      bad:
	while (i--)
		_inits[i].exit();

	return r;
}

static void __exit dm_exit(void)
{
	int i = ARRAY_SIZE(_inits);

	while (i--)
		_inits[i].exit();
}

/*
 * Block device functions
 */
static int dm_blk_open(struct inode *inode, struct file *file)
{
	struct mapped_device *md;

	md = get_kdev(inode->i_rdev);
	if (!md)
		return -ENXIO;

	return 0;
}

static int dm_blk_close(struct inode *inode, struct file *file)
{
	struct mapped_device *md;

	md = get_kdev(inode->i_rdev);
	dm_put(md);		/* put the reference gained by dm_blk_open */
	dm_put(md);
	return 0;
}

static inline struct dm_io *alloc_io(struct mapped_device *md)
{
	return mempool_alloc(md->io_pool, GFP_NOIO);
}

static inline void free_io(struct mapped_device *md, struct dm_io *io)
{
	mempool_free(io, md->io_pool);
}

static inline struct deferred_io *alloc_deferred(void)
{
	return kmalloc(sizeof(struct deferred_io), GFP_NOIO);
}

static inline void free_deferred(struct deferred_io *di)
{
	kfree(di);
}

static inline sector_t volume_size(kdev_t dev)
{
	return blk_size[major(dev)][minor(dev)] << 1;
}

/* FIXME: check this */
static int dm_blk_ioctl(struct inode *inode, struct file *file,
			unsigned int command, unsigned long a)
{
	kdev_t dev = inode->i_rdev;
	long size;

	switch (command) {
	case BLKROSET:
	case BLKROGET:
	case BLKRASET:
	case BLKRAGET:
	case BLKFLSBUF:
	case BLKSSZGET:
		//case BLKRRPART: /* Re-read partition tables */
		//case BLKPG:
	case BLKELVGET:
	case BLKELVSET:
	case BLKBSZGET:
	case BLKBSZSET:
		return blk_ioctl(dev, command, a);
		break;

	case BLKGETSIZE:
		size = volume_size(dev);
		if (copy_to_user((void *) a, &size, sizeof(long)))
			return -EFAULT;
		break;

	case BLKGETSIZE64:
		size = volume_size(dev);
		if (put_user((u64) ((u64) size) << 9, (u64 *) a))
			return -EFAULT;
		break;

	case BLKRRPART:
		return -ENOTTY;

	case LV_BMAP:
		return dm_user_bmap(inode, (struct lv_bmap *) a);

	default:
		DMWARN("unknown block ioctl 0x%x", command);
		return -ENOTTY;
	}

	return 0;
}

/*
 * Add the buffer to the list of deferred io.
 */
static int queue_io(struct mapped_device *md, struct buffer_head *bh, int rw)
{
	struct deferred_io *di;

	di = alloc_deferred();
	if (!di)
		return -ENOMEM;

	down_write(&md->lock);

	if (!test_bit(DMF_BLOCK_IO, &md->flags)) {
		up_write(&md->lock);
		free_deferred(di);
		return 1;
	}

	di->bh = bh;
	di->rw = rw;
	di->next = md->deferred;
	md->deferred = di;

	up_write(&md->lock);
	return 0;		/* deferred successfully */
}

/*
 * bh->b_end_io routine that decrements the pending count
 * and then calls the original bh->b_end_io fn.
 */
static void dec_pending(struct buffer_head *bh, int uptodate)
{
	int r;
	struct dm_io *io = bh->b_private;
	dm_endio_fn endio = io->ti->type->end_io;

	if (endio) {
		r = endio(io->ti, bh, io->rw, uptodate ? 0 : -EIO,
			  &io->map_context);
		if (r < 0)
			uptodate = 0;

		else if (r > 0)
			/* the target wants another shot at the io */
			return;
	}

	if (atomic_dec_and_test(&io->md->pending))
		/* nudge anyone waiting on suspend queue */
		wake_up(&io->md->wait);

	bh->b_end_io = io->end_io;
	bh->b_private = io->context;
	free_io(io->md, io);

	bh->b_end_io(bh, uptodate);
}

/*
 * Do the bh mapping for a given leaf
 */
static inline int __map_buffer(struct mapped_device *md, int rw,
			       struct buffer_head *bh, struct dm_io *io)
{
	struct dm_target *ti;

	if (!md->map)
		return -EINVAL;

	ti = dm_table_find_target(md->map, bh->b_rsector);
	if (!ti->type)
		return -EINVAL;

	/* hook the end io request fn */
	atomic_inc(&md->pending);
	io->md = md;
	io->ti = ti;
	io->rw = rw;
	io->end_io = bh->b_end_io;
	io->context = bh->b_private;
	bh->b_end_io = dec_pending;
	bh->b_private = io;

	return ti->type->map(ti, bh, rw, &io->map_context);
}

/*
 * Checks to see if we should be deferring io, if so it queues it
 * and returns 1.
 */
static inline int __deferring(struct mapped_device *md, int rw,
			      struct buffer_head *bh)
{
	int r;

	/*
	 * If we're suspended we have to queue this io for later.
	 */
	while (test_bit(DMF_BLOCK_IO, &md->flags)) {
		up_read(&md->lock);

		/*
		 * There's no point deferring a read ahead
		 * request, just drop it.
		 */
		if (rw == READA) {
			down_read(&md->lock);
			return -EIO;
		}

		r = queue_io(md, bh, rw);
		down_read(&md->lock);

		if (r < 0)
			return r;

		if (r == 0)
			return 1;	/* deferred successfully */

	}

	return 0;
}

static int dm_request(request_queue_t *q, int rw, struct buffer_head *bh)
{
	int r;
	struct dm_io *io;
	struct mapped_device *md;

	md = get_kdev(bh->b_rdev);
	if (!md) {
		buffer_IO_error(bh);
		return 0;
	}

	io = alloc_io(md);
	down_read(&md->lock);

	r = __deferring(md, rw, bh);
	if (r < 0)
		goto bad;

	else if (!r) {
		/* not deferring */
		r = __map_buffer(md, rw, bh, io);
		if (r < 0)
			goto bad;
	} else
		r = 0;

	up_read(&md->lock);
	dm_put(md);
	return r;

      bad:
	buffer_IO_error(bh);
	up_read(&md->lock);
	dm_put(md);
	return 0;
}

static int check_dev_size(kdev_t dev, unsigned long block)
{
	unsigned int major = major(dev);
	unsigned int minor = minor(dev);

	/* FIXME: check this */
	unsigned long max_sector = (blk_size[major][minor] << 1) + 1;
	unsigned long sector = (block + 1) * (blksize_size[major][minor] >> 9);

	return (sector > max_sector) ? 0 : 1;
}

/*
 * Creates a dummy buffer head and maps it (for lilo).
 */
static int __bmap(struct mapped_device *md, kdev_t dev, unsigned long block,
		  kdev_t *r_dev, unsigned long *r_block)
{
	struct buffer_head bh;
	struct dm_target *ti;
	union map_info map_context;
	int r;

	if (test_bit(DMF_BLOCK_IO, &md->flags)) {
		return -EPERM;
	}

	if (!check_dev_size(dev, block)) {
		return -EINVAL;
	}

	if (!md->map)
		return -EINVAL;

	/* setup dummy bh */
	memset(&bh, 0, sizeof(bh));
	bh.b_blocknr = block;
	bh.b_dev = bh.b_rdev = dev;
	bh.b_size = blksize_size[major(dev)][minor(dev)];
	bh.b_rsector = block * (bh.b_size >> 9);

	/* find target */
	ti = dm_table_find_target(md->map, bh.b_rsector);

	/* do the mapping */
	r = ti->type->map(ti, &bh, READ, &map_context);
	ti->type->end_io(ti, &bh, READ, 0, &map_context);

	if (!r) {
		*r_dev = bh.b_rdev;
		*r_block = bh.b_rsector / (bh.b_size >> 9);
	}

	return r;
}

/*
 * Marshals arguments and results between user and kernel space.
 */
static int dm_user_bmap(struct inode *inode, struct lv_bmap *lvb)
{
	struct mapped_device *md;
	unsigned long block, r_block;
	kdev_t r_dev;
	int r;

	if (get_user(block, &lvb->lv_block))
		return -EFAULT;

	md = get_kdev(inode->i_rdev);
	if (!md)
		return -ENXIO;

	down_read(&md->lock);
	r = __bmap(md, inode->i_rdev, block, &r_dev, &r_block);
	up_read(&md->lock);
	dm_put(md);

	if (!r && (put_user(kdev_t_to_nr(r_dev), &lvb->lv_dev) ||
		   put_user(r_block, &lvb->lv_block)))
		r = -EFAULT;

	return r;
}

static void free_md(struct mapped_device *md)
{
	free_dev(md->dev);
	mempool_destroy(md->io_pool);
	kfree(md);
}

/*
 * Allocate and initialise a blank device with a given minor.
 */
static struct mapped_device *alloc_md(kdev_t dev)
{
	int r;
	struct mapped_device *md = kmalloc(sizeof(*md), GFP_KERNEL);

	if (!md) {
		DMWARN("unable to allocate device, out of memory.");
		return NULL;
	}

	memset(md, 0, sizeof(*md));

	/* Allocate suitable device number */
	if (!dev)
		r = first_free_dev(md);
	else
		r = specific_dev(dev, md);

	if (r) {
		kfree(md);
		return NULL;
	}

	md->io_pool = mempool_create(MIN_IOS, mempool_alloc_slab,
				     mempool_free_slab, _io_cache);
	if (!md->io_pool) {
		free_md(md);
		kfree(md);
		return NULL;
	}

	init_rwsem(&md->lock);
	atomic_set(&md->holders, 1);
	atomic_set(&md->pending, 0);
	init_waitqueue_head(&md->wait);
	init_waitqueue_head(&md->eventq);

	return md;
}

/*
 * The hardsect size for a mapped device is the largest hardsect size
 * from the devices it maps onto.
 */
static int __find_hardsect_size(struct list_head *devices)
{
	int result = 512, size;
	struct list_head *tmp;

	list_for_each (tmp, devices) {
		struct dm_dev *dd = list_entry(tmp, struct dm_dev, list);
		size = get_hardsect_size(dd->dev);
		if (size > result)
			result = size;
	}

	return result;
}

/*
 * Bind a table to the device.
 */
static void event_callback(void *context)
{
	struct mapped_device *md = (struct mapped_device *) context;

	down_write(&md->lock);
	md->event_nr++;
	wake_up_interruptible(&md->eventq);
	up_write(&md->lock);
}

static int __bind(struct mapped_device *md, struct dm_table *t)
{
	unsigned int minor = minor(md->dev);
	unsigned int major = major(md->dev);
	md->map = t;

	/* in k */
	blk_size[major][minor] = dm_table_get_size(t) >> 1;
	blksize_size[major][minor] = BLOCK_SIZE;
	hardsect_size[major][minor] =
	    __find_hardsect_size(dm_table_get_devices(t));
	register_disk(NULL, md->dev, 1, &dm_blk_dops, blk_size[major][minor]);

	dm_table_event_callback(md->map, event_callback, md);
	dm_table_get(t);
	return 0;
}

static void __unbind(struct mapped_device *md)
{
	unsigned int minor = minor(md->dev);
	unsigned int major = major(md->dev);

	if (md->map) {
		dm_table_event_callback(md->map, NULL, NULL);
		dm_table_put(md->map);
		md->map = NULL;

	}

	blk_size[major][minor] = 0;
	blksize_size[major][minor] = 0;
	hardsect_size[major][minor] = 0;
}

/*
 * Constructor for a new device.
 */
int dm_create(kdev_t dev, struct mapped_device **result)
{
	struct mapped_device *md;

	md = alloc_md(dev);
	if (!md)
		return -ENXIO;

	__unbind(md);	/* Ensure zero device size */

	*result = md;
	return 0;
}

void dm_get(struct mapped_device *md)
{
	atomic_inc(&md->holders);
}

void dm_put(struct mapped_device *md)
{
	if (atomic_dec_and_test(&md->holders)) {
		if (md->map)
			dm_table_suspend_targets(md->map);
		__unbind(md);
		free_md(md);
	}
}

/*
 * Requeue the deferred io by calling generic_make_request.
 */
static void flush_deferred_io(struct deferred_io *c)
{
	struct deferred_io *n;

	while (c) {
		n = c->next;
		generic_make_request(c->rw, c->bh);
		free_deferred(c);
		c = n;
	}
}

/*
 * Swap in a new table (destroying old one).
 */
int dm_swap_table(struct mapped_device *md, struct dm_table *table)
{
	int r;

	down_write(&md->lock);

	/*
	 * The device must be suspended, or have no table bound yet.
	 */
	if (md->map && !test_bit(DMF_SUSPENDED, &md->flags)) {
		up_write(&md->lock);
		return -EPERM;
	}

	__unbind(md);
	r = __bind(md, table);
	if (r)
		return r;

	up_write(&md->lock);
	return 0;
}

/*
 * We need to be able to change a mapping table under a mounted
 * filesystem.  For example we might want to move some data in
 * the background.  Before the table can be swapped with
 * dm_bind_table, dm_suspend must be called to flush any in
 * flight io and ensure that any further io gets deferred.
 */
int dm_suspend(struct mapped_device *md)
{
	int r = 0;
	DECLARE_WAITQUEUE(wait, current);

	down_write(&md->lock);

	/*
	 * First we set the BLOCK_IO flag so no more ios will be
	 * mapped.
	 */
	if (test_bit(DMF_BLOCK_IO, &md->flags)) {
		up_write(&md->lock);
		return -EINVAL;
	}

	set_bit(DMF_BLOCK_IO, &md->flags);
	add_wait_queue(&md->wait, &wait);
	up_write(&md->lock);

	/*
	 * Then we wait for the already mapped ios to
	 * complete.
	 */
	run_task_queue(&tq_disk);
	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);

		if (!atomic_read(&md->pending) || signal_pending(current))
			break;

		schedule();
	}
	set_current_state(TASK_RUNNING);

	down_write(&md->lock);
	remove_wait_queue(&md->wait, &wait);

	/* did we flush everything ? */
	if (atomic_read(&md->pending)) {
		clear_bit(DMF_BLOCK_IO, &md->flags);
		r = -EINTR;
	} else {
		set_bit(DMF_SUSPENDED, &md->flags);
		if (md->map)
			dm_table_suspend_targets(md->map);
	}
	up_write(&md->lock);

	return r;
}

int dm_resume(struct mapped_device *md)
{
	struct deferred_io *def;

	down_write(&md->lock);
	if (!test_bit(DMF_SUSPENDED, &md->flags)) {
		up_write(&md->lock);
		return -EINVAL;
	}

	if (md->map)
		dm_table_resume_targets(md->map);

	clear_bit(DMF_SUSPENDED, &md->flags);
	clear_bit(DMF_BLOCK_IO, &md->flags);
	def = md->deferred;
	md->deferred = NULL;
	up_write(&md->lock);

	flush_deferred_io(def);
	run_task_queue(&tq_disk);

	return 0;
}

struct dm_table *dm_get_table(struct mapped_device *md)
{
	struct dm_table *t;

	down_read(&md->lock);
	t = md->map;
	if (t)
		dm_table_get(t);
	up_read(&md->lock);

	return t;
}

/*-----------------------------------------------------------------
 * Event notification.
 *---------------------------------------------------------------*/
uint32_t dm_get_event_nr(struct mapped_device *md)
{
	uint32_t r;

	down_read(&md->lock);
	r = md->event_nr;
	up_read(&md->lock);

	return r;
}

int dm_add_wait_queue(struct mapped_device *md, wait_queue_t *wq,
		      uint32_t event_nr)
{
	down_write(&md->lock);
	if (event_nr != md->event_nr) {
		up_write(&md->lock);
		return 1;
	}

	add_wait_queue(&md->eventq, wq);
	up_write(&md->lock);

	return 0;
}

const char *dm_kdevname(kdev_t dev)
{
	static char buffer[32];
	sprintf(buffer, "%03d:%03d", MAJOR(dev), MINOR(dev));
	return buffer;
}

void dm_remove_wait_queue(struct mapped_device *md, wait_queue_t *wq)
{
	down_write(&md->lock);
	remove_wait_queue(&md->eventq, wq);
	up_write(&md->lock);
}

kdev_t dm_kdev(struct mapped_device *md)
{
	kdev_t dev;

	down_read(&md->lock);
	dev = md->dev;
	up_read(&md->lock);

	return dev;
}

int dm_suspended(struct mapped_device *md)
{
	return test_bit(DMF_SUSPENDED, &md->flags);
}

struct block_device_operations dm_blk_dops = {
	.open = dm_blk_open,
	.release = dm_blk_close,
	.ioctl = dm_blk_ioctl,
	.owner = THIS_MODULE
};

/*
 * module hooks
 */
module_init(dm_init);
module_exit(dm_exit);

MODULE_DESCRIPTION(DM_NAME " driver");
MODULE_AUTHOR("Joe Thornber <thornber@sistina.com>");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(dm_kdevname);
