/*
 * Copyright (C) 2001 Sistina Software (UK) Limited.
 *
 * This file is released under the GPL.
 */

#include "dm.h"

#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <asm/atomic.h>

#define MAX_DEPTH 16
#define NODE_SIZE L1_CACHE_BYTES
#define KEYS_PER_NODE (NODE_SIZE / sizeof(sector_t))
#define CHILDREN_PER_NODE (KEYS_PER_NODE + 1)

struct dm_table {
	atomic_t holders;

	/* btree table */
	unsigned int depth;
	unsigned int counts[MAX_DEPTH];	/* in nodes */
	sector_t *index[MAX_DEPTH];

	unsigned int num_targets;
	unsigned int num_allocated;
	sector_t *highs;
	struct dm_target *targets;

	/*
	 * Indicates the rw permissions for the new logical
	 * device.  This should be a combination of FMODE_READ
	 * and FMODE_WRITE.
	 */
	int mode;

	/* a list of devices used by this table */
	struct list_head devices;

	/* events get handed up using this callback */
	void (*event_fn)(void *);
	void *event_context;
};

/*
 * Similar to ceiling(log_size(n))
 */
static unsigned int int_log(unsigned long n, unsigned long base)
{
	int result = 0;

	while (n > 1) {
		n = dm_div_up(n, base);
		result++;
	}

	return result;
}

/*
 * Calculate the index of the child node of the n'th node k'th key.
 */
static inline unsigned int get_child(unsigned int n, unsigned int k)
{
	return (n * CHILDREN_PER_NODE) + k;
}

/*
 * Return the n'th node of level l from table t.
 */
static inline sector_t *get_node(struct dm_table *t, unsigned int l,
				 unsigned int n)
{
	return t->index[l] + (n * KEYS_PER_NODE);
}

/*
 * Return the highest key that you could lookup from the n'th
 * node on level l of the btree.
 */
static sector_t high(struct dm_table *t, unsigned int l, unsigned int n)
{
	for (; l < t->depth - 1; l++)
		n = get_child(n, CHILDREN_PER_NODE - 1);

	if (n >= t->counts[l])
		return (sector_t) - 1;

	return get_node(t, l, n)[KEYS_PER_NODE - 1];
}

/*
 * Fills in a level of the btree based on the highs of the level
 * below it.
 */
static int setup_btree_index(unsigned int l, struct dm_table *t)
{
	unsigned int n, k;
	sector_t *node;

	for (n = 0U; n < t->counts[l]; n++) {
		node = get_node(t, l, n);

		for (k = 0U; k < KEYS_PER_NODE; k++)
			node[k] = high(t, l + 1, get_child(n, k));
	}

	return 0;
}



int dm_table_create(struct dm_table **result, int mode, unsigned num_targets)
{
	struct dm_table *t = kmalloc(sizeof(*t), GFP_KERNEL);

	if (!t)
		return -ENOMEM;

	memset(t, 0, sizeof(*t));
	INIT_LIST_HEAD(&t->devices);
	atomic_set(&t->holders, 1);

	num_targets = dm_round_up(num_targets, KEYS_PER_NODE);

	/* Allocate both the target array and offset array at once. */
	t->highs = (sector_t *) vcalloc(sizeof(struct dm_target) +
					sizeof(sector_t), num_targets);
	if (!t->highs) {
		kfree(t);
		return -ENOMEM;
	}

	memset(t->highs, -1, sizeof(*t->highs) * num_targets);

	t->targets = (struct dm_target *) (t->highs + num_targets);
	t->num_allocated = num_targets;
	t->mode = mode;
	*result = t;
	return 0;
}

static void free_devices(struct list_head *devices)
{
	struct list_head *tmp, *next;

	for (tmp = devices->next; tmp != devices; tmp = next) {
		struct dm_dev *dd = list_entry(tmp, struct dm_dev, list);
		next = tmp->next;
		kfree(dd);
	}
}

void table_destroy(struct dm_table *t)
{
	unsigned int i;

	/* free the indexes (see dm_table_complete) */
	if (t->depth >= 2)
		vfree(t->index[t->depth - 2]);

	/* free the targets */
	for (i = 0; i < t->num_targets; i++) {
		struct dm_target *tgt = t->targets + i;

		if (tgt->type->dtr)
			tgt->type->dtr(tgt);

		dm_put_target_type(tgt->type);
	}

	vfree(t->highs);

	/* free the device list */
	if (t->devices.next != &t->devices) {
		DMWARN("devices still present during destroy: "
		       "dm_table_remove_device calls missing");

		free_devices(&t->devices);
	}

	kfree(t);
}

void dm_table_get(struct dm_table *t)
{
	atomic_inc(&t->holders);
}

void dm_table_put(struct dm_table *t)
{
	if (atomic_dec_and_test(&t->holders))
		table_destroy(t);
}

/*
 * Convert a device path to a dev_t.
 */
static int lookup_device(const char *path, kdev_t *dev)
{
	int r;
	struct nameidata nd;
	struct inode *inode;

	if (!path_init(path, LOOKUP_FOLLOW, &nd))
		return 0;

	if ((r = path_walk(path, &nd)))
		goto out;

	inode = nd.dentry->d_inode;
	if (!inode) {
		r = -ENOENT;
		goto out;
	}

	if (!S_ISBLK(inode->i_mode)) {
		r = -ENOTBLK;
		goto out;
	}

	*dev = inode->i_rdev;

      out:
	path_release(&nd);
	return r;
}

/*
 * See if we've already got a device in the list.
 */
static struct dm_dev *find_device(struct list_head *l, kdev_t dev)
{
	struct list_head *tmp;

	list_for_each(tmp, l) {
		struct dm_dev *dd = list_entry(tmp, struct dm_dev, list);
		if (kdev_same(dd->dev, dev))
			return dd;
	}

	return NULL;
}

/*
 * Open a device so we can use it as a map destination.
 */
static int open_dev(struct dm_dev *dd)
{
	if (dd->bdev)
		BUG();

	dd->bdev = bdget(kdev_t_to_nr(dd->dev));
	if (!dd->bdev)
		return -ENOMEM;

	return blkdev_get(dd->bdev, dd->mode, 0, BDEV_RAW);
}

/*
 * Close a device that we've been using.
 */
static void close_dev(struct dm_dev *dd)
{
	if (!dd->bdev)
		return;

	blkdev_put(dd->bdev, BDEV_RAW);
	dd->bdev = NULL;
}

/*
 * If possible (ie. blk_size[major] is set), this checks an area
 * of a destination device is valid.
 */
static int check_device_area(kdev_t dev, sector_t start, sector_t len)
{
	int *sizes;
	sector_t dev_size;

	if (!(sizes = blk_size[major(dev)]) || !(dev_size = sizes[minor(dev)]))
		/* we don't know the device details,
		 * so give the benefit of the doubt */
		return 1;

	/* convert to 512-byte sectors */
	dev_size <<= 1;

	return ((start < dev_size) && (len <= (dev_size - start)));
}

/*
 * This upgrades the mode on an already open dm_dev.  Being
 * careful to leave things as they were if we fail to reopen the
 * device.
 */
static int upgrade_mode(struct dm_dev *dd, int new_mode)
{
	int r;
	struct dm_dev dd_copy;

	memcpy(&dd_copy, dd, sizeof(dd_copy));

	dd->mode |= new_mode;
	dd->bdev = NULL;
	r = open_dev(dd);
	if (!r)
		close_dev(&dd_copy);
	else
		memcpy(dd, &dd_copy, sizeof(dd_copy));

	return r;
}

/*
 * Add a device to the list, or just increment the usage count if
 * it's already present.
 */
int dm_get_device(struct dm_target *ti, const char *path, sector_t start,
		  sector_t len, int mode, struct dm_dev **result)
{
	int r;
	kdev_t dev;
	struct dm_dev *dd;
	unsigned major, minor;
	struct dm_table *t = ti->table;

	if (!t)
		BUG();

	if (sscanf(path, "%u:%u", &major, &minor) == 2) {
		/* Extract the major/minor numbers */
		dev = mk_kdev(major, minor);
	} else {
		/* convert the path to a device */
		if ((r = lookup_device(path, &dev)))
			return r;
	}

	dd = find_device(&t->devices, dev);
	if (!dd) {
		dd = kmalloc(sizeof(*dd), GFP_KERNEL);
		if (!dd)
			return -ENOMEM;

		dd->dev = dev;
		dd->mode = mode;
		dd->bdev = NULL;

		if ((r = open_dev(dd))) {
			kfree(dd);
			return r;
		}

		atomic_set(&dd->count, 0);
		list_add(&dd->list, &t->devices);

	} else if (dd->mode != (mode | dd->mode)) {
		r = upgrade_mode(dd, mode);
		if (r)
			return r;
	}
	atomic_inc(&dd->count);

	if (!check_device_area(dd->dev, start, len)) {
		DMWARN("device %s too small for target", path);
		dm_put_device(ti, dd);
		return -EINVAL;
	}

	*result = dd;

	return 0;
}

/*
 * Decrement a devices use count and remove it if neccessary.
 */
void dm_put_device(struct dm_target *ti, struct dm_dev *dd)
{
	if (atomic_dec_and_test(&dd->count)) {
		close_dev(dd);
		list_del(&dd->list);
		kfree(dd);
	}
}

/*
 * Checks to see if the target joins onto the end of the table.
 */
static int adjoin(struct dm_table *table, struct dm_target *ti)
{
	struct dm_target *prev;

	if (!table->num_targets)
		return !ti->begin;

	prev = &table->targets[table->num_targets - 1];
	return (ti->begin == (prev->begin + prev->len));
}

/*
 * Used to dynamically allocate the arg array.
 */
static char **realloc_argv(unsigned *array_size, char **old_argv)
{
	char **argv;
	unsigned new_size;

	new_size = *array_size ? *array_size * 2 : 64;
	argv = kmalloc(new_size * sizeof(*argv), GFP_KERNEL);
	if (argv) {
		memcpy(argv, old_argv, *array_size * sizeof(*argv));
		*array_size = new_size;
	}

	kfree(old_argv);
	return argv;
}

/*
 * Destructively splits up the argument list to pass to ctr.
 */
static int split_args(int *argc, char ***argvp, char *input)
{
	char *start, *end = input, *out, **argv = NULL;
	unsigned array_size = 0;

	*argc = 0;
	argv = realloc_argv(&array_size, argv);
	if (!argv)
		return -ENOMEM;

	while (1) {
		start = end;

		/* Skip whitespace */
		while (*start && isspace(*start))
			start++;

		if (!*start)
			break;	/* success, we hit the end */

		/* 'out' is used to remove any back-quotes */
		end = out = start;
		while (*end) {
			/* Everything apart from '\0' can be quoted */
			if (*end == '\\' && *(end + 1)) {
				*out++ = *(end + 1);
				end += 2;
				continue;
			}

			if (isspace(*end))
				break;	/* end of token */

			*out++ = *end++;
		}

		/* have we already filled the array ? */
		if ((*argc + 1) > array_size) {
			argv = realloc_argv(&array_size, argv);
			if (!argv)
				return -ENOMEM;
		}

		/* we know this is whitespace */
		if (*end)
			end++;

		/* terminate the string and put it in the array */
		*out = '\0';
		argv[*argc] = start;
		(*argc)++;
	}

	*argvp = argv;
	return 0;
}

int dm_table_add_target(struct dm_table *t, const char *type,
			sector_t start, sector_t len, char *params)
{
	int r = -EINVAL, argc;
	char **argv;
	struct dm_target *tgt;

	if (t->num_targets >= t->num_allocated)
		return -ENOMEM;

	tgt = t->targets + t->num_targets;
	memset(tgt, 0, sizeof(*tgt));

	tgt->type = dm_get_target_type(type);
	if (!tgt->type) {
		tgt->error = "unknown target type";
		return -EINVAL;
	}

	tgt->table = t;
	tgt->begin = start;
	tgt->len = len;
	tgt->error = "Unknown error";

	/*
	 * Does this target adjoin the previous one ?
	 */
	if (!adjoin(t, tgt)) {
		tgt->error = "Gap in table";
		r = -EINVAL;
		goto bad;
	}

	r = split_args(&argc, &argv, params);
	if (r) {
		tgt->error = "couldn't split parameters (insufficient memory)";
		goto bad;
	}

	r = tgt->type->ctr(tgt, argc, argv);
	kfree(argv);
	if (r)
		goto bad;

	t->highs[t->num_targets++] = tgt->begin + tgt->len - 1;
	return 0;

      bad:
	printk(KERN_ERR DM_NAME ": %s\n", tgt->error);
	dm_put_target_type(tgt->type);
	return r;
}

static int setup_indexes(struct dm_table *t)
{
	int i;
	unsigned int total = 0;
	sector_t *indexes;

	/* allocate the space for *all* the indexes */
	for (i = t->depth - 2; i >= 0; i--) {
		t->counts[i] = dm_div_up(t->counts[i + 1], CHILDREN_PER_NODE);
		total += t->counts[i];
	}

	indexes = (sector_t *) vcalloc(total, (unsigned long) NODE_SIZE);
	if (!indexes)
		return -ENOMEM;

	/* set up internal nodes, bottom-up */
	for (i = t->depth - 2, total = 0; i >= 0; i--) {
		t->index[i] = indexes;
		indexes += (KEYS_PER_NODE * t->counts[i]);
		setup_btree_index(i, t);
	}

	return 0;
}

/*
 * Builds the btree to index the map.
 */
int dm_table_complete(struct dm_table *t)
{
	int r = 0;
	unsigned int leaf_nodes;

	/* how many indexes will the btree have ? */
	leaf_nodes = dm_div_up(t->num_targets, KEYS_PER_NODE);
	t->depth = 1 + int_log(leaf_nodes, CHILDREN_PER_NODE);

	/* leaf layer has already been set up */
	t->counts[t->depth - 1] = leaf_nodes;
	t->index[t->depth - 1] = t->highs;

	if (t->depth >= 2)
		r = setup_indexes(t);

	return r;
}

static spinlock_t _event_lock = SPIN_LOCK_UNLOCKED;
void dm_table_event_callback(struct dm_table *t,
			     void (*fn)(void *), void *context)
{
	spin_lock_irq(&_event_lock);
	t->event_fn = fn;
	t->event_context = context;
	spin_unlock_irq(&_event_lock);
}

void dm_table_event(struct dm_table *t)
{
	spin_lock(&_event_lock);
	if (t->event_fn)
		t->event_fn(t->event_context);
	spin_unlock(&_event_lock);
}

sector_t dm_table_get_size(struct dm_table *t)
{
	return t->num_targets ? (t->highs[t->num_targets - 1] + 1) : 0;
}

struct dm_target *dm_table_get_target(struct dm_table *t, unsigned int index)
{
	if (index > t->num_targets)
		return NULL;

	return t->targets + index;
}

/*
 * Search the btree for the correct target.
 */
struct dm_target *dm_table_find_target(struct dm_table *t, sector_t sector)
{
	unsigned int l, n = 0, k = 0;
	sector_t *node;

	for (l = 0; l < t->depth; l++) {
		n = get_child(n, k);
		node = get_node(t, l, n);

		for (k = 0; k < KEYS_PER_NODE; k++)
			if (node[k] >= sector)
				break;
	}

	return &t->targets[(KEYS_PER_NODE * n) + k];
}

unsigned int dm_table_get_num_targets(struct dm_table *t)
{
	return t->num_targets;
}

struct list_head *dm_table_get_devices(struct dm_table *t)
{
	return &t->devices;
}

int dm_table_get_mode(struct dm_table *t)
{
	return t->mode;
}

void dm_table_suspend_targets(struct dm_table *t)
{
	int i;

	for (i = 0; i < t->num_targets; i++) {
		struct dm_target *ti = t->targets + i;

		if (ti->type->suspend)
			ti->type->suspend(ti);
	}
}

void dm_table_resume_targets(struct dm_table *t)
{
	int i;

	for (i = 0; i < t->num_targets; i++) {
		struct dm_target *ti = t->targets + i;

		if (ti->type->resume)
			ti->type->resume(ti);
	}
}

EXPORT_SYMBOL(dm_get_device);
EXPORT_SYMBOL(dm_put_device);
EXPORT_SYMBOL(dm_table_event);
EXPORT_SYMBOL(dm_table_get_mode);
