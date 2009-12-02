/*	$NetBSD: libdevmapper.h,v 1.1.1.2 2009/12/02 00:25:41 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef LIB_DEVICE_MAPPER_H
#define LIB_DEVICE_MAPPER_H

#include <inttypes.h>
#include <stdarg.h>
#include <sys/types.h>

#ifdef linux
#  include <linux/types.h>
#endif

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*****************************************************************
 * The first section of this file provides direct access to the
 * individual device-mapper ioctls.  Since it is quite laborious to
 * build the ioctl arguments for the device-mapper, people are
 * encouraged to use this library.
 ****************************************************************/

/*
 * The library user may wish to register their own
 * logging function.  By default errors go to stderr.
 * Use dm_log_with_errno_init(NULL) to restore the default log fn.
 */

typedef void (*dm_log_with_errno_fn) (int level, const char *file, int line,
				      int dm_errno, const char *f, ...)
    __attribute__ ((format(printf, 5, 6)));

void dm_log_with_errno_init(dm_log_with_errno_fn fn);
void dm_log_init_verbose(int level);

/*
 * Original version of this function.
 * dm_errno is set to 0.
 *
 * Deprecated: Use the _with_errno_ versions above instead.
 */
typedef void (*dm_log_fn) (int level, const char *file, int line,
			   const char *f, ...)
    __attribute__ ((format(printf, 4, 5)));
void dm_log_init(dm_log_fn fn);
/*
 * For backward-compatibility, indicate that dm_log_init() was used
 * to set a non-default value of dm_log().
 */
int dm_log_is_non_default(void);

enum {
	DM_DEVICE_CREATE,
	DM_DEVICE_RELOAD,
	DM_DEVICE_REMOVE,
	DM_DEVICE_REMOVE_ALL,

	DM_DEVICE_SUSPEND,
	DM_DEVICE_RESUME,

	DM_DEVICE_INFO,
	DM_DEVICE_DEPS,
	DM_DEVICE_RENAME,

	DM_DEVICE_VERSION,

	DM_DEVICE_STATUS,
	DM_DEVICE_TABLE,
	DM_DEVICE_WAITEVENT,

	DM_DEVICE_LIST,

	DM_DEVICE_CLEAR,

	DM_DEVICE_MKNODES,

	DM_DEVICE_LIST_VERSIONS,
	
	DM_DEVICE_TARGET_MSG,

	DM_DEVICE_SET_GEOMETRY
};

/*
 * You will need to build a struct dm_task for
 * each ioctl command you want to execute.
 */

struct dm_task;

struct dm_task *dm_task_create(int type);
void dm_task_destroy(struct dm_task *dmt);

int dm_task_set_name(struct dm_task *dmt, const char *name);
int dm_task_set_uuid(struct dm_task *dmt, const char *uuid);

/*
 * Retrieve attributes after an info.
 */
struct dm_info {
	int exists;
	int suspended;
	int live_table;
	int inactive_table;
	int32_t open_count;
	uint32_t event_nr;
	uint32_t major;
	uint32_t minor;		/* minor device number */
	int read_only;		/* 0:read-write; 1:read-only */

	int32_t target_count;
};

struct dm_deps {
	uint32_t count;
	uint32_t filler;
	uint64_t device[0];
};

struct dm_names {
	uint64_t dev;
	uint32_t next;		/* Offset to next struct from start of this struct */
	char name[0];
};

struct dm_versions {
	uint32_t next;		/* Offset to next struct from start of this struct */
	uint32_t version[3];

	char name[0];
};

int dm_get_library_version(char *version, size_t size);
int dm_task_get_driver_version(struct dm_task *dmt, char *version, size_t size);
int dm_task_get_info(struct dm_task *dmt, struct dm_info *dmi);
const char *dm_task_get_name(const struct dm_task *dmt);
const char *dm_task_get_uuid(const struct dm_task *dmt);

struct dm_deps *dm_task_get_deps(struct dm_task *dmt);
struct dm_names *dm_task_get_names(struct dm_task *dmt);
struct dm_versions *dm_task_get_versions(struct dm_task *dmt);

int dm_task_set_ro(struct dm_task *dmt);
int dm_task_set_newname(struct dm_task *dmt, const char *newname);
int dm_task_set_minor(struct dm_task *dmt, int minor);
int dm_task_set_major(struct dm_task *dmt, int major);
int dm_task_set_major_minor(struct dm_task *dmt, int major, int minor, int allow_default_major_fallback);
int dm_task_set_uid(struct dm_task *dmt, uid_t uid);
int dm_task_set_gid(struct dm_task *dmt, gid_t gid);
int dm_task_set_mode(struct dm_task *dmt, mode_t mode);
int dm_task_set_cookie(struct dm_task *dmt, uint32_t *cookie, uint16_t flags);
int dm_task_set_event_nr(struct dm_task *dmt, uint32_t event_nr);
int dm_task_set_geometry(struct dm_task *dmt, const char *cylinders, const char *heads, const char *sectors, const char *start);
int dm_task_set_message(struct dm_task *dmt, const char *message);
int dm_task_set_sector(struct dm_task *dmt, uint64_t sector);
int dm_task_no_flush(struct dm_task *dmt);
int dm_task_no_open_count(struct dm_task *dmt);
int dm_task_skip_lockfs(struct dm_task *dmt);
int dm_task_query_inactive_table(struct dm_task *dmt);
int dm_task_suppress_identical_reload(struct dm_task *dmt);

/*
 * Control read_ahead.
 */
#define DM_READ_AHEAD_AUTO UINT32_MAX	/* Use kernel default readahead */
#define DM_READ_AHEAD_NONE 0		/* Disable readahead */

#define DM_READ_AHEAD_MINIMUM_FLAG	0x1	/* Value supplied is minimum */

/*
 * Read ahead is set with DM_DEVICE_CREATE with a table or DM_DEVICE_RESUME.
 */
int dm_task_set_read_ahead(struct dm_task *dmt, uint32_t read_ahead,
			   uint32_t read_ahead_flags);
uint32_t dm_task_get_read_ahead(const struct dm_task *dmt,
				uint32_t *read_ahead);

/*
 * Use these to prepare for a create or reload.
 */
int dm_task_add_target(struct dm_task *dmt,
		       uint64_t start,
		       uint64_t size, const char *ttype, const char *params);

/*
 * Format major/minor numbers correctly for input to driver.
 */
#define DM_FORMAT_DEV_BUFSIZE	13	/* Minimum bufsize to handle worst case. */
int dm_format_dev(char *buf, int bufsize, uint32_t dev_major, uint32_t dev_minor);

/* Use this to retrive target information returned from a STATUS call */
void *dm_get_next_target(struct dm_task *dmt,
			 void *next, uint64_t *start, uint64_t *length,
			 char **target_type, char **params);

/*
 * Call this to actually run the ioctl.
 */
int dm_task_run(struct dm_task *dmt);

/*
 * Call this to make or remove the device nodes associated with previously
 * issued commands.
 */
void dm_task_update_nodes(void);

/*
 * Configure the device-mapper directory
 */
int dm_set_dev_dir(const char *dir);
const char *dm_dir(void);

/*
 * Determine whether a major number belongs to device-mapper or not.
 */
int dm_is_dm_major(uint32_t major);

/*
 * Release library resources
 */
void dm_lib_release(void);
void dm_lib_exit(void) __attribute((destructor));

/*
 * Use NULL for all devices.
 */
int dm_mknodes(const char *name);
int dm_driver_version(char *version, size_t size);

/******************************************************
 * Functions to build and manipulate trees of devices *
 ******************************************************/
struct dm_tree;
struct dm_tree_node;

/*
 * Initialise an empty dependency tree.
 *
 * The tree consists of a root node together with one node for each mapped 
 * device which has child nodes for each device referenced in its table.
 *
 * Every node in the tree has one or more children and one or more parents.
 *
 * The root node is the parent/child of every node that doesn't have other 
 * parents/children.
 */
struct dm_tree *dm_tree_create(void);
void dm_tree_free(struct dm_tree *tree);

/*
 * Add nodes to the tree for a given device and all the devices it uses.
 */
int dm_tree_add_dev(struct dm_tree *tree, uint32_t major, uint32_t minor);

/*
 * Add a new node to the tree if it doesn't already exist.
 */
struct dm_tree_node *dm_tree_add_new_dev(struct dm_tree *tree,
					 const char *name,
					 const char *uuid,
					 uint32_t major, uint32_t minor,
					 int read_only,
					 int clear_inactive,
					 void *context);
struct dm_tree_node *dm_tree_add_new_dev_with_udev_flags(struct dm_tree *tree,
							 const char *name,
							 const char *uuid,
							 uint32_t major,
							 uint32_t minor,
							 int read_only,
							 int clear_inactive,
							 void *context,
							 uint16_t udev_flags);

/*
 * Search for a node in the tree.
 * Set major and minor to 0 or uuid to NULL to get the root node.
 */
struct dm_tree_node *dm_tree_find_node(struct dm_tree *tree,
					  uint32_t major,
					  uint32_t minor);
struct dm_tree_node *dm_tree_find_node_by_uuid(struct dm_tree *tree,
						  const char *uuid);

/*
 * Use this to walk through all children of a given node.
 * Set handle to NULL in first call.
 * Returns NULL after the last child.
 * Set inverted to use inverted tree.
 */
struct dm_tree_node *dm_tree_next_child(void **handle,
					   struct dm_tree_node *parent,
					   uint32_t inverted);

/*
 * Get properties of a node.
 */
const char *dm_tree_node_get_name(struct dm_tree_node *node);
const char *dm_tree_node_get_uuid(struct dm_tree_node *node);
const struct dm_info *dm_tree_node_get_info(struct dm_tree_node *node);
void *dm_tree_node_get_context(struct dm_tree_node *node);
int dm_tree_node_size_changed(struct dm_tree_node *dnode);

/*
 * Returns the number of children of the given node (excluding the root node).
 * Set inverted for the number of parents.
 */
int dm_tree_node_num_children(struct dm_tree_node *node, uint32_t inverted);

/*
 * Deactivate a device plus all dependencies.
 * Ignores devices that don't have a uuid starting with uuid_prefix.
 */
int dm_tree_deactivate_children(struct dm_tree_node *dnode,
				   const char *uuid_prefix,
				   size_t uuid_prefix_len);
/*
 * Preload/create a device plus all dependencies.
 * Ignores devices that don't have a uuid starting with uuid_prefix.
 */
int dm_tree_preload_children(struct dm_tree_node *dnode,
			     const char *uuid_prefix,
			     size_t uuid_prefix_len);

/*
 * Resume a device plus all dependencies.
 * Ignores devices that don't have a uuid starting with uuid_prefix.
 */
int dm_tree_activate_children(struct dm_tree_node *dnode,
			      const char *uuid_prefix,
			      size_t uuid_prefix_len);

/*
 * Suspend a device plus all dependencies.
 * Ignores devices that don't have a uuid starting with uuid_prefix.
 */
int dm_tree_suspend_children(struct dm_tree_node *dnode,
				   const char *uuid_prefix,
				   size_t uuid_prefix_len);

/*
 * Skip the filesystem sync when suspending.
 * Does nothing with other functions.
 * Use this when no snapshots are involved.
 */ 
void dm_tree_skip_lockfs(struct dm_tree_node *dnode);

/*
 * Set the 'noflush' flag when suspending devices.
 * If the kernel supports it, instead of erroring outstanding I/O that
 * cannot be completed, the I/O is queued and resubmitted when the
 * device is resumed.  This affects multipath devices when all paths
 * have failed and queue_if_no_path is set, and mirror devices when
 * block_on_error is set and the mirror log has failed.
 */
void dm_tree_use_no_flush_suspend(struct dm_tree_node *dnode);

/*
 * Is the uuid prefix present in the tree?
 * Only returns 0 if every node was checked successfully.
 * Returns 1 if the tree walk has to be aborted.
 */
int dm_tree_children_use_uuid(struct dm_tree_node *dnode,
				 const char *uuid_prefix,
				 size_t uuid_prefix_len);

/*
 * Construct tables for new nodes before activating them.
 */
int dm_tree_node_add_snapshot_origin_target(struct dm_tree_node *dnode,
					       uint64_t size,
					       const char *origin_uuid);
int dm_tree_node_add_snapshot_target(struct dm_tree_node *node,
					uint64_t size,
					const char *origin_uuid,
					const char *cow_uuid,
					int persistent,
					uint32_t chunk_size);
int dm_tree_node_add_error_target(struct dm_tree_node *node,
				     uint64_t size);
int dm_tree_node_add_zero_target(struct dm_tree_node *node,
				    uint64_t size);
int dm_tree_node_add_linear_target(struct dm_tree_node *node,
				      uint64_t size);
int dm_tree_node_add_striped_target(struct dm_tree_node *node,
				       uint64_t size,
				       uint32_t stripe_size);

#define DM_CRYPT_IV_DEFAULT	UINT64_C(-1)	/* iv_offset == seg offset */
/*
 * Function accepts one string in cipher specification
 * (chainmode and iv should be NULL because included in cipher string)
 *   or
 * separate arguments which will be joined to "cipher-chainmode-iv"
 */
int dm_tree_node_add_crypt_target(struct dm_tree_node *node,
				  uint64_t size,
				  const char *cipher,
				  const char *chainmode,
				  const char *iv,
				  uint64_t iv_offset,
				  const char *key);
int dm_tree_node_add_mirror_target(struct dm_tree_node *node,
				      uint64_t size);
 
/* Mirror log flags */
#define DM_NOSYNC		0x00000001	/* Known already in sync */
#define DM_FORCESYNC		0x00000002	/* Force resync */
#define DM_BLOCK_ON_ERROR	0x00000004	/* On error, suspend I/O */
#define DM_CORELOG		0x00000008	/* In-memory log */

int dm_tree_node_add_mirror_target_log(struct dm_tree_node *node,
					  uint32_t region_size,
					  unsigned clustered,
					  const char *log_uuid,
					  unsigned area_count,
					  uint32_t flags);
int dm_tree_node_add_target_area(struct dm_tree_node *node,
				    const char *dev_name,
				    const char *dlid,
				    uint64_t offset);

/*
 * Set readahead (in sectors) after loading the node.
 */
void dm_tree_node_set_read_ahead(struct dm_tree_node *dnode,
				 uint32_t read_ahead,
				 uint32_t read_ahead_flags);

void dm_tree_set_cookie(struct dm_tree_node *node, uint32_t cookie);
uint32_t dm_tree_get_cookie(struct dm_tree_node *node);

/*****************************************************************************
 * Library functions
 *****************************************************************************/

/*******************
 * Memory management
 *******************/

void *dm_malloc_aux(size_t s, const char *file, int line);
void *dm_malloc_aux_debug(size_t s, const char *file, int line);
char *dm_strdup_aux(const char *str, const char *file, int line);
void dm_free_aux(void *p);
void *dm_realloc_aux(void *p, unsigned int s, const char *file, int line);
int dm_dump_memory_debug(void);
void dm_bounds_check_debug(void);

#ifdef DEBUG_MEM

#  define dm_malloc(s) dm_malloc_aux_debug((s), __FILE__, __LINE__)
#  define dm_strdup(s) dm_strdup_aux((s), __FILE__, __LINE__)
#  define dm_free(p) dm_free_aux(p)
#  define dm_realloc(p, s) dm_realloc_aux(p, s, __FILE__, __LINE__)
#  define dm_dump_memory() dm_dump_memory_debug()
#  define dm_bounds_check() dm_bounds_check_debug()

#else

#  define dm_malloc(s) dm_malloc_aux((s), __FILE__, __LINE__)
#  define dm_strdup(s) strdup(s)
#  define dm_free(p) free(p)
#  define dm_realloc(p, s) realloc(p, s)
#  define dm_dump_memory() {}
#  define dm_bounds_check() {}

#endif


/*
 * The pool allocator is useful when you are going to allocate
 * lots of memory, use the memory for a bit, and then free the
 * memory in one go.  A surprising amount of code has this usage
 * profile.
 *
 * You should think of the pool as an infinite, contiguous chunk
 * of memory.  The front of this chunk of memory contains
 * allocated objects, the second half is free.  dm_pool_alloc grabs
 * the next 'size' bytes from the free half, in effect moving it
 * into the allocated half.  This operation is very efficient.
 *
 * dm_pool_free frees the allocated object *and* all objects
 * allocated after it.  It is important to note this semantic
 * difference from malloc/free.  This is also extremely
 * efficient, since a single dm_pool_free can dispose of a large
 * complex object.
 *
 * dm_pool_destroy frees all allocated memory.
 *
 * eg, If you are building a binary tree in your program, and
 * know that you are only ever going to insert into your tree,
 * and not delete (eg, maintaining a symbol table for a
 * compiler).  You can create yourself a pool, allocate the nodes
 * from it, and when the tree becomes redundant call dm_pool_destroy
 * (no nasty iterating through the tree to free nodes).
 *
 * eg, On the other hand if you wanted to repeatedly insert and
 * remove objects into the tree, you would be better off
 * allocating the nodes from a free list; you cannot free a
 * single arbitrary node with pool.
 */

struct dm_pool;

/* constructor and destructor */
struct dm_pool *dm_pool_create(const char *name, size_t chunk_hint);
void dm_pool_destroy(struct dm_pool *p);

/* simple allocation/free routines */
void *dm_pool_alloc(struct dm_pool *p, size_t s);
void *dm_pool_alloc_aligned(struct dm_pool *p, size_t s, unsigned alignment);
void dm_pool_empty(struct dm_pool *p);
void dm_pool_free(struct dm_pool *p, void *ptr);

/*
 * Object building routines:
 *
 * These allow you to 'grow' an object, useful for
 * building strings, or filling in dynamic
 * arrays.
 *
 * It's probably best explained with an example:
 *
 * char *build_string(struct dm_pool *mem)
 * {
 *      int i;
 *      char buffer[16];
 *
 *      if (!dm_pool_begin_object(mem, 128))
 *              return NULL;
 *
 *      for (i = 0; i < 50; i++) {
 *              snprintf(buffer, sizeof(buffer), "%d, ", i);
 *              if (!dm_pool_grow_object(mem, buffer, 0))
 *                      goto bad;
 *      }
 *
 *	// add null
 *      if (!dm_pool_grow_object(mem, "\0", 1))
 *              goto bad;
 *
 *      return dm_pool_end_object(mem);
 *
 * bad:
 *
 *      dm_pool_abandon_object(mem);
 *      return NULL;
 *}
 *
 * So start an object by calling dm_pool_begin_object
 * with a guess at the final object size - if in
 * doubt make the guess too small.
 *
 * Then append chunks of data to your object with
 * dm_pool_grow_object.  Finally get your object with
 * a call to dm_pool_end_object.
 *
 * Setting delta to 0 means it will use strlen(extra).
 */
int dm_pool_begin_object(struct dm_pool *p, size_t hint);
int dm_pool_grow_object(struct dm_pool *p, const void *extra, size_t delta);
void *dm_pool_end_object(struct dm_pool *p);
void dm_pool_abandon_object(struct dm_pool *p);

/* utilities */
char *dm_pool_strdup(struct dm_pool *p, const char *str);
char *dm_pool_strndup(struct dm_pool *p, const char *str, size_t n);
void *dm_pool_zalloc(struct dm_pool *p, size_t s);

/******************
 * bitset functions
 ******************/

typedef uint32_t *dm_bitset_t;

dm_bitset_t dm_bitset_create(struct dm_pool *mem, unsigned num_bits);
void dm_bitset_destroy(dm_bitset_t bs);

void dm_bit_union(dm_bitset_t out, dm_bitset_t in1, dm_bitset_t in2);
int dm_bit_get_first(dm_bitset_t bs);
int dm_bit_get_next(dm_bitset_t bs, int last_bit);

#define DM_BITS_PER_INT (sizeof(int) * CHAR_BIT)

#define dm_bit(bs, i) \
   (bs[(i / DM_BITS_PER_INT) + 1] & (0x1 << (i & (DM_BITS_PER_INT - 1))))

#define dm_bit_set(bs, i) \
   (bs[(i / DM_BITS_PER_INT) + 1] |= (0x1 << (i & (DM_BITS_PER_INT - 1))))

#define dm_bit_clear(bs, i) \
   (bs[(i / DM_BITS_PER_INT) + 1] &= ~(0x1 << (i & (DM_BITS_PER_INT - 1))))

#define dm_bit_set_all(bs) \
   memset(bs + 1, -1, ((*bs / DM_BITS_PER_INT) + 1) * sizeof(int))

#define dm_bit_clear_all(bs) \
   memset(bs + 1, 0, ((*bs / DM_BITS_PER_INT) + 1) * sizeof(int))

#define dm_bit_copy(bs1, bs2) \
   memcpy(bs1 + 1, bs2 + 1, ((*bs1 / DM_BITS_PER_INT) + 1) * sizeof(int))

/* Returns number of set bits */
static inline unsigned hweight32(uint32_t i)
{
	unsigned r = (i & 0x55555555) + ((i >> 1) & 0x55555555);

	r =    (r & 0x33333333) + ((r >>  2) & 0x33333333);
	r =    (r & 0x0F0F0F0F) + ((r >>  4) & 0x0F0F0F0F);
	r =    (r & 0x00FF00FF) + ((r >>  8) & 0x00FF00FF);
	return (r & 0x0000FFFF) + ((r >> 16) & 0x0000FFFF);
}

/****************
 * hash functions
 ****************/

struct dm_hash_table;
struct dm_hash_node;

typedef void (*dm_hash_iterate_fn) (void *data);

struct dm_hash_table *dm_hash_create(unsigned size_hint);
void dm_hash_destroy(struct dm_hash_table *t);
void dm_hash_wipe(struct dm_hash_table *t);

void *dm_hash_lookup(struct dm_hash_table *t, const char *key);
int dm_hash_insert(struct dm_hash_table *t, const char *key, void *data);
void dm_hash_remove(struct dm_hash_table *t, const char *key);

void *dm_hash_lookup_binary(struct dm_hash_table *t, const char *key, uint32_t len);
int dm_hash_insert_binary(struct dm_hash_table *t, const char *key, uint32_t len,
		       void *data);
void dm_hash_remove_binary(struct dm_hash_table *t, const char *key, uint32_t len);

unsigned dm_hash_get_num_entries(struct dm_hash_table *t);
void dm_hash_iter(struct dm_hash_table *t, dm_hash_iterate_fn f);

char *dm_hash_get_key(struct dm_hash_table *t, struct dm_hash_node *n);
void *dm_hash_get_data(struct dm_hash_table *t, struct dm_hash_node *n);
struct dm_hash_node *dm_hash_get_first(struct dm_hash_table *t);
struct dm_hash_node *dm_hash_get_next(struct dm_hash_table *t, struct dm_hash_node *n);

#define dm_hash_iterate(v, h) \
	for (v = dm_hash_get_first(h); v; \
	     v = dm_hash_get_next(h, v))

/****************
 * list functions
 ****************/

/*
 * A list consists of a list head plus elements.
 * Each element has 'next' and 'previous' pointers.
 * The list head's pointers point to the first and the last element.
 */

struct dm_list {
	struct dm_list *n, *p;
};

/*
 * Initialise a list before use.
 * The list head's next and previous pointers point back to itself.
 */
#define DM_LIST_INIT(name)	struct dm_list name = { &(name), &(name) }
void dm_list_init(struct dm_list *head);

/*
 * Insert an element before 'head'.
 * If 'head' is the list head, this adds an element to the end of the list.
 */
void dm_list_add(struct dm_list *head, struct dm_list *elem);

/*
 * Insert an element after 'head'.
 * If 'head' is the list head, this adds an element to the front of the list.
 */
void dm_list_add_h(struct dm_list *head, struct dm_list *elem);

/*
 * Delete an element from its list.
 * Note that this doesn't change the element itself - it may still be safe
 * to follow its pointers.
 */
void dm_list_del(struct dm_list *elem);

/*
 * Remove an element from existing list and insert before 'head'.
 */
void dm_list_move(struct dm_list *head, struct dm_list *elem);

/*
 * Is the list empty?
 */
int dm_list_empty(const struct dm_list *head);

/*
 * Is this the first element of the list?
 */
int dm_list_start(const struct dm_list *head, const struct dm_list *elem);

/*
 * Is this the last element of the list?
 */
int dm_list_end(const struct dm_list *head, const struct dm_list *elem);

/*
 * Return first element of the list or NULL if empty
 */
struct dm_list *dm_list_first(const struct dm_list *head);

/*
 * Return last element of the list or NULL if empty
 */
struct dm_list *dm_list_last(const struct dm_list *head);

/*
 * Return the previous element of the list, or NULL if we've reached the start.
 */
struct dm_list *dm_list_prev(const struct dm_list *head, const struct dm_list *elem);

/*
 * Return the next element of the list, or NULL if we've reached the end.
 */
struct dm_list *dm_list_next(const struct dm_list *head, const struct dm_list *elem);

/*
 * Given the address v of an instance of 'struct dm_list' called 'head' 
 * contained in a structure of type t, return the containing structure.
 */
#define dm_list_struct_base(v, t, head) \
    ((t *)((uintptr_t)(v) - (uintptr_t)&((t *) 0)->head))

/*
 * Given the address v of an instance of 'struct dm_list list' contained in
 * a structure of type t, return the containing structure.
 */
#define dm_list_item(v, t) dm_list_struct_base((v), t, list)

/*
 * Given the address v of one known element e in a known structure of type t,
 * return another element f.
 */
#define dm_struct_field(v, t, e, f) \
    (((t *)((uintptr_t)(v) - (uintptr_t)&((t *) 0)->e))->f)

/*
 * Given the address v of a known element e in a known structure of type t,
 * return the list head 'list'
 */
#define dm_list_head(v, t, e) dm_struct_field(v, t, e, list)

/*
 * Set v to each element of a list in turn.
 */
#define dm_list_iterate(v, head) \
	for (v = (head)->n; v != head; v = v->n)

/*
 * Set v to each element in a list in turn, starting from the element 
 * in front of 'start'.
 * You can use this to 'unwind' a list_iterate and back out actions on
 * already-processed elements.
 * If 'start' is 'head' it walks the list backwards.
 */
#define dm_list_uniterate(v, head, start) \
	for (v = (start)->p; v != head; v = v->p)

/*
 * A safe way to walk a list and delete and free some elements along
 * the way.
 * t must be defined as a temporary variable of the same type as v.
 */
#define dm_list_iterate_safe(v, t, head) \
	for (v = (head)->n, t = v->n; v != head; v = t, t = v->n)

/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The 'struct dm_list' variable within the containing structure is 'field'.
 */
#define dm_list_iterate_items_gen(v, head, field) \
	for (v = dm_list_struct_base((head)->n, typeof(*v), field); \
	     &v->field != (head); \
	     v = dm_list_struct_base(v->field.n, typeof(*v), field))

/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The list should be 'struct dm_list list' within the containing structure.
 */
#define dm_list_iterate_items(v, head) dm_list_iterate_items_gen(v, (head), list)

/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The 'struct dm_list' variable within the containing structure is 'field'.
 * t must be defined as a temporary variable of the same type as v.
 */
#define dm_list_iterate_items_gen_safe(v, t, head, field) \
	for (v = dm_list_struct_base((head)->n, typeof(*v), field), \
	     t = dm_list_struct_base(v->field.n, typeof(*v), field); \
	     &v->field != (head); \
	     v = t, t = dm_list_struct_base(v->field.n, typeof(*v), field))
/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The list should be 'struct dm_list list' within the containing structure.
 * t must be defined as a temporary variable of the same type as v.
 */
#define dm_list_iterate_items_safe(v, t, head) \
	dm_list_iterate_items_gen_safe(v, t, (head), list)

/*
 * Walk a list backwards, setting 'v' in turn to the containing structure 
 * of each item.
 * The containing structure should be the same type as 'v'.
 * The 'struct dm_list' variable within the containing structure is 'field'.
 */
#define dm_list_iterate_back_items_gen(v, head, field) \
	for (v = dm_list_struct_base((head)->p, typeof(*v), field); \
	     &v->field != (head); \
	     v = dm_list_struct_base(v->field.p, typeof(*v), field))

/*
 * Walk a list backwards, setting 'v' in turn to the containing structure 
 * of each item.
 * The containing structure should be the same type as 'v'.
 * The list should be 'struct dm_list list' within the containing structure.
 */
#define dm_list_iterate_back_items(v, head) dm_list_iterate_back_items_gen(v, (head), list)

/*
 * Return the number of elements in a list by walking it.
 */
unsigned int dm_list_size(const struct dm_list *head);

/*********
 * selinux
 *********/
int dm_set_selinux_context(const char *path, mode_t mode);

/*********************
 * string manipulation
 *********************/

/*
 * Break up the name of a mapped device into its constituent
 * Volume Group, Logical Volume and Layer (if present).
 * If mem is supplied, the result is allocated from the mempool.
 * Otherwise the strings are changed in situ.
 */
int dm_split_lvm_name(struct dm_pool *mem, const char *dmname,
		      char **vgname, char **lvname, char **layer);

/*
 * Destructively split buffer into NULL-separated words in argv.
 * Returns number of words.
 */
int dm_split_words(char *buffer, unsigned max,
		   unsigned ignore_comments, /* Not implemented */
		   char **argv);

/* 
 * Returns -1 if buffer too small
 */
int dm_snprintf(char *buf, size_t bufsize, const char *format, ...);

/*
 * Returns pointer to the last component of the path.
 */
char *dm_basename(const char *path);

/**************************
 * file/stream manipulation
 **************************/

/*
 * Create a directory (with parent directories if necessary).
 * Returns 1 on success, 0 on failure.
 */
int dm_create_dir(const char *dir);

/*
 * Close a stream, with nicer error checking than fclose's.
 * Derived from gnulib's close-stream.c.
 *
 * Close "stream".  Return 0 if successful, and EOF (setting errno)
 * otherwise.  Upon failure, set errno to 0 if the error number
 * cannot be determined.  Useful mainly for writable streams.
 */
int dm_fclose(FILE *stream);

/*
 * Returns size of a buffer which is allocated with dm_malloc.
 * Pointer to the buffer is stored in *buf.
 * Returns -1 on failure leaving buf undefined.
 */
int dm_asprintf(char **buf, const char *format, ...);

/*********************
 * regular expressions
 *********************/
struct dm_regex;

/*
 * Initialise an array of num patterns for matching.
 * Uses memory from mem.
 */
struct dm_regex *dm_regex_create(struct dm_pool *mem, const char **patterns,
				 unsigned num_patterns);

/*
 * Match string s against the patterns.
 * Returns the index of the highest pattern in the array that matches,
 * or -1 if none match.
 */
int dm_regex_match(struct dm_regex *regex, const char *s);

/*********************
 * reporting functions
 *********************/

struct dm_report_object_type {
	uint32_t id;			/* Powers of 2 */
	const char *desc;
	const char *prefix;		/* field id string prefix (optional) */
	void *(*data_fn)(void *object);	/* callback from report_object() */
};

struct dm_report_field;

/*
 * dm_report_field_type flags
 */
#define DM_REPORT_FIELD_MASK		0x000000FF
#define DM_REPORT_FIELD_ALIGN_MASK	0x0000000F
#define DM_REPORT_FIELD_ALIGN_LEFT	0x00000001
#define DM_REPORT_FIELD_ALIGN_RIGHT	0x00000002
#define DM_REPORT_FIELD_TYPE_MASK	0x000000F0
#define DM_REPORT_FIELD_TYPE_STRING	0x00000010
#define DM_REPORT_FIELD_TYPE_NUMBER	0x00000020

struct dm_report;
struct dm_report_field_type {
	uint32_t type;		/* object type id */
	uint32_t flags;		/* DM_REPORT_FIELD_* */
	uint32_t offset;	/* byte offset in the object */
	int32_t width;		/* default width */
	const char id[32];	/* string used to specify the field */
	const char heading[32];	/* string printed in header */
	int (*report_fn)(struct dm_report *rh, struct dm_pool *mem,
			 struct dm_report_field *field, const void *data,
			 void *private);
	const char *desc;	/* description of the field */
};

/*
 * dm_report_init output_flags
 */
#define DM_REPORT_OUTPUT_MASK			0x000000FF
#define DM_REPORT_OUTPUT_ALIGNED		0x00000001
#define DM_REPORT_OUTPUT_BUFFERED		0x00000002
#define DM_REPORT_OUTPUT_HEADINGS		0x00000004
#define DM_REPORT_OUTPUT_FIELD_NAME_PREFIX	0x00000008
#define DM_REPORT_OUTPUT_FIELD_UNQUOTED		0x00000010
#define DM_REPORT_OUTPUT_COLUMNS_AS_ROWS	0x00000020

struct dm_report *dm_report_init(uint32_t *report_types,
				 const struct dm_report_object_type *types,
				 const struct dm_report_field_type *fields,
				 const char *output_fields,
				 const char *output_separator,
				 uint32_t output_flags,
				 const char *sort_keys,
				 void *private);
int dm_report_object(struct dm_report *rh, void *object);
int dm_report_output(struct dm_report *rh);
void dm_report_free(struct dm_report *rh);

/*
 * Prefix added to each field name with DM_REPORT_OUTPUT_FIELD_NAME_PREFIX
 */
int dm_report_set_output_field_name_prefix(struct dm_report *rh,
					   const char *report_prefix);

/*
 * Report functions are provided for simple data types.
 * They take care of allocating copies of the data.
 */
int dm_report_field_string(struct dm_report *rh, struct dm_report_field *field,
			   const char **data);
int dm_report_field_int32(struct dm_report *rh, struct dm_report_field *field,
			  const int32_t *data);
int dm_report_field_uint32(struct dm_report *rh, struct dm_report_field *field,
			   const uint32_t *data);
int dm_report_field_int(struct dm_report *rh, struct dm_report_field *field,
			const int *data);
int dm_report_field_uint64(struct dm_report *rh, struct dm_report_field *field,
			   const uint64_t *data);

/*
 * For custom fields, allocate the data in 'mem' and use
 * dm_report_field_set_value().
 * 'sortvalue' may be NULL if it matches 'value'
 */
void dm_report_field_set_value(struct dm_report_field *field, const void *value,
			       const void *sortvalue);

/* Cookie prefixes.
 * The cookie value consists of a prefix (16 bits) and a base (16 bits).
 * We can use the prefix to store the flags. These flags are sent to
 * kernel within given dm task. When returned back to userspace in
 * DM_COOKIE udev environment variable, we can control several aspects
 * of udev rules we use by decoding the cookie prefix. When doing the
 * notification, we replace the cookie prefix with DM_COOKIE_MAGIC,
 * so we notify the right semaphore.
 * It is still possible to use cookies for passing the flags to udev
 * rules even when udev_sync is disabled. The base part of the cookie
 * will be zero (there's no notification semaphore) and prefix will be
 * set then. However, having udev_sync enabled is highly recommended.
 */
#define DM_COOKIE_MAGIC 0x0D4D
#define DM_UDEV_FLAGS_MASK 0xFFFF0000
#define DM_UDEV_FLAGS_SHIFT 16

/*
 * DM_UDEV_DISABLE_DM_RULES_FLAG is set in case we need to disable
 * basic device-mapper udev rules that create symlinks in /dev/<DM_DIR>
 * directory. However, we can't reliably prevent creating default
 * nodes by udev (commonly /dev/dm-X, where X is a number).
 */
#define DM_UDEV_DISABLE_DM_RULES_FLAG 0x0001
/*
 * DM_UDEV_DISABLE_SUBSYTEM_RULES_FLAG is set in case we need to disable
 * subsystem udev rules, but still we need the general DM udev rules to
 * be applied (to create the nodes and symlinks under /dev and /dev/disk).
 */
#define DM_UDEV_DISABLE_SUBSYSTEM_RULES_FLAG 0x0002
/*
 * DM_UDEV_DISABLE_DISK_RULES_FLAG is set in case we need to disable
 * general DM rules that set symlinks in /dev/disk directory.
 */
#define DM_UDEV_DISABLE_DISK_RULES_FLAG 0x0004
/*
 * DM_UDEV_DISABLE_OTHER_RULES_FLAG is set in case we need to disable
 * all the other rules that are not general device-mapper nor subsystem
 * related (the rules belong to other software or packages). All foreign
 * rules should check this flag directly and they should ignore further
 * rule processing for such event.
 */
#define DM_UDEV_DISABLE_OTHER_RULES_FLAG 0x0008
/*
 * DM_UDEV_LOW_PRIORITY_FLAG is set in case we need to instruct the
 * udev rules to give low priority to the device that is currently
 * processed. For example, this provides a way to select which symlinks
 * could be overwritten by high priority ones if their names are equal.
 * Common situation is a name based on FS UUID while using origin and
 * snapshot devices.
 */
#define DM_UDEV_LOW_PRIORITY_FLAG 0x0010

int dm_cookie_supported(void);

/*
 * Udev synchronisation functions.
 */
void dm_udev_set_sync_support(int sync_with_udev);
int dm_udev_get_sync_support(void);
int dm_udev_complete(uint32_t cookie);
int dm_udev_wait(uint32_t cookie);

#define DM_DEV_DIR_UMASK 0022

#endif				/* LIB_DEVICE_MAPPER_H */
