/*	$NetBSD: pool.h,v 1.2 2008/12/19 15:24:09 haad Exp $	*/

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

#ifndef _DM_POOL_H
#define _DM_POOL_H

#include <string.h>
#include <stdlib.h>

/*
 * The pool allocator is useful when you are going to allocate
 * lots of memory, use the memory for a bit, and then free the
 * memory in one go.  A surprising amount of code has this usage
 * profile.
 *
 * You should think of the pool as an infinite, contiguous chunk
 * of memory.  The front of this chunk of memory contains
 * allocated objects, the second half is free.  pool_alloc grabs
 * the next 'size' bytes from the free half, in effect moving it
 * into the allocated half.  This operation is very efficient.
 *
 * pool_free frees the allocated object *and* all objects
 * allocated after it.  It is important to note this semantic
 * difference from malloc/free.  This is also extremely
 * efficient, since a single pool_free can dispose of a large
 * complex object.
 *
 * pool_destroy frees all allocated memory.
 *
 * eg, If you are building a binary tree in your program, and
 * know that you are only ever going to insert into your tree,
 * and not delete (eg, maintaining a symbol table for a
 * compiler).  You can create yourself a pool, allocate the nodes
 * from it, and when the tree becomes redundant call pool_destroy
 * (no nasty iterating through the tree to free nodes).
 *
 * eg, On the other hand if you wanted to repeatedly insert and
 * remove objects into the tree, you would be better off
 * allocating the nodes from a free list; you cannot free a
 * single arbitrary node with pool.
 */

struct pool;

/* constructor and destructor */
struct pool *pool_create(const char *name, size_t chunk_hint);
void pool_destroy(struct pool *p);

/* simple allocation/free routines */
void *pool_alloc(struct pool *p, size_t s);
void *pool_alloc_aligned(struct pool *p, size_t s, unsigned alignment);
void pool_empty(struct pool *p);
void pool_free(struct pool *p, void *ptr);

/*
 * Object building routines:
 *
 * These allow you to 'grow' an object, useful for
 * building strings, or filling in dynamic
 * arrays.
 *
 * It's probably best explained with an example:
 *
 * char *build_string(struct pool *mem)
 * {
 *      int i;
 *      char buffer[16];
 *
 *      if (!pool_begin_object(mem, 128))
 *              return NULL;
 *
 *      for (i = 0; i < 50; i++) {
 *              snprintf(buffer, sizeof(buffer), "%d, ", i);
 *              if (!pool_grow_object(mem, buffer, strlen(buffer)))
 *                      goto bad;
 *      }
 *
 *	// add null
 *      if (!pool_grow_object(mem, "\0", 1))
 *              goto bad;
 *
 *      return pool_end_object(mem);
 *
 * bad:
 *
 *      pool_abandon_object(mem);
 *      return NULL;
 *}
 *
 * So start an object by calling pool_begin_object
 * with a guess at the final object size - if in
 * doubt make the guess too small.
 *
 * Then append chunks of data to your object with
 * pool_grow_object.  Finally get your object with
 * a call to pool_end_object.
 *
 */
int pool_begin_object(struct pool *p, size_t hint);
int pool_grow_object(struct pool *p, const void *extra, size_t delta);
void *pool_end_object(struct pool *p);
void pool_abandon_object(struct pool *p);

/* utilities */
char *pool_strdup(struct pool *p, const char *str);
char *pool_strndup(struct pool *p, const char *str, size_t n);
void *pool_zalloc(struct pool *p, size_t s);

#endif
/*	$NetBSD: pool.h,v 1.2 2008/12/19 15:24:09 haad Exp $	*/

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

#ifndef _DM_POOL_H
#define _DM_POOL_H

#include <string.h>
#include <stdlib.h>

/*
 * The pool allocator is useful when you are going to allocate
 * lots of memory, use the memory for a bit, and then free the
 * memory in one go.  A surprising amount of code has this usage
 * profile.
 *
 * You should think of the pool as an infinite, contiguous chunk
 * of memory.  The front of this chunk of memory contains
 * allocated objects, the second half is free.  pool_alloc grabs
 * the next 'size' bytes from the free half, in effect moving it
 * into the allocated half.  This operation is very efficient.
 *
 * pool_free frees the allocated object *and* all objects
 * allocated after it.  It is important to note this semantic
 * difference from malloc/free.  This is also extremely
 * efficient, since a single pool_free can dispose of a large
 * complex object.
 *
 * pool_destroy frees all allocated memory.
 *
 * eg, If you are building a binary tree in your program, and
 * know that you are only ever going to insert into your tree,
 * and not delete (eg, maintaining a symbol table for a
 * compiler).  You can create yourself a pool, allocate the nodes
 * from it, and when the tree becomes redundant call pool_destroy
 * (no nasty iterating through the tree to free nodes).
 *
 * eg, On the other hand if you wanted to repeatedly insert and
 * remove objects into the tree, you would be better off
 * allocating the nodes from a free list; you cannot free a
 * single arbitrary node with pool.
 */

struct pool;

/* constructor and destructor */
struct pool *pool_create(const char *name, size_t chunk_hint);
void pool_destroy(struct pool *p);

/* simple allocation/free routines */
void *pool_alloc(struct pool *p, size_t s);
void *pool_alloc_aligned(struct pool *p, size_t s, unsigned alignment);
void pool_empty(struct pool *p);
void pool_free(struct pool *p, void *ptr);

/*
 * Object building routines:
 *
 * These allow you to 'grow' an object, useful for
 * building strings, or filling in dynamic
 * arrays.
 *
 * It's probably best explained with an example:
 *
 * char *build_string(struct pool *mem)
 * {
 *      int i;
 *      char buffer[16];
 *
 *      if (!pool_begin_object(mem, 128))
 *              return NULL;
 *
 *      for (i = 0; i < 50; i++) {
 *              snprintf(buffer, sizeof(buffer), "%d, ", i);
 *              if (!pool_grow_object(mem, buffer, strlen(buffer)))
 *                      goto bad;
 *      }
 *
 *	// add null
 *      if (!pool_grow_object(mem, "\0", 1))
 *              goto bad;
 *
 *      return pool_end_object(mem);
 *
 * bad:
 *
 *      pool_abandon_object(mem);
 *      return NULL;
 *}
 *
 * So start an object by calling pool_begin_object
 * with a guess at the final object size - if in
 * doubt make the guess too small.
 *
 * Then append chunks of data to your object with
 * pool_grow_object.  Finally get your object with
 * a call to pool_end_object.
 *
 */
int pool_begin_object(struct pool *p, size_t hint);
int pool_grow_object(struct pool *p, const void *extra, size_t delta);
void *pool_end_object(struct pool *p);
void pool_abandon_object(struct pool *p);

/* utilities */
char *pool_strdup(struct pool *p, const char *str);
char *pool_strndup(struct pool *p, const char *str, size_t n);
void *pool_zalloc(struct pool *p, size_t s);

#endif
