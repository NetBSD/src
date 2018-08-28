/*	$NetBSD: linux_idr.c,v 1.12 2018/08/27 15:24:53 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_idr.c,v 1.12 2018/08/27 15:24:53 riastradh Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/rbtree.h>
#include <sys/sdt.h>

#include <linux/err.h>
#include <linux/idr.h>
#include <linux/slab.h>

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#endif

#ifdef DDB
#include <ddb/ddb.h>
#endif

struct idr_node {
	rb_node_t		in_rb_node;
	int			in_index;
	void			*in_data;
};

struct idr_cache {
	struct idr_node		*ic_node;
	void			*ic_where;
};

SDT_PROBE_DEFINE0(sdt, linux, idr, leak);
SDT_PROBE_DEFINE1(sdt, linux, idr, init, "struct idr *"/*idr*/);
SDT_PROBE_DEFINE1(sdt, linux, idr, destroy, "struct idr *"/*idr*/);
SDT_PROBE_DEFINE4(sdt, linux, idr, replace,
    "struct idr *"/*idr*/, "int"/*id*/, "void *"/*odata*/, "void *"/*ndata*/);
SDT_PROBE_DEFINE3(sdt, linux, idr, remove,
    "struct idr *"/*idr*/, "int"/*id*/, "void *"/*data*/);
SDT_PROBE_DEFINE0(sdt, linux, idr, preload);
SDT_PROBE_DEFINE0(sdt, linux, idr, preload__end);
SDT_PROBE_DEFINE3(sdt, linux, idr, alloc,
    "struct idr *"/*idr*/, "int"/*id*/, "void *"/*data*/);

static specificdata_key_t idr_cache_key __read_mostly;

static void
idr_cache_warning(struct idr_cache *cache)
{
#ifdef DDB
	const char *name;
	db_expr_t offset;
#endif

	KASSERT(cache->ic_node != NULL);

#ifdef DDB
	db_find_sym_and_offset((db_addr_t)(uintptr_t)cache->ic_where,
	    &name, &offset);
	if (name) {
		printf("WARNING: idr preload at %s+%#"DDB_EXPR_FMT"x"
		    " leaked in lwp %s @ %p\n",
		    name, offset, curlwp->l_name, curlwp);
	} else
#endif
	{
		printf("WARNING: idr preload at %p leaked in lwp %s @ %p\n",
		    cache->ic_where, curlwp->l_name, curlwp);
	}
}

static void
idr_cache_dtor(void *cookie)
{
	struct idr_cache *cache = cookie;

	if (cache->ic_node) {
		SDT_PROBE0(sdt, linux, idr, leak);
		idr_cache_warning(cache);
		kmem_free(cache->ic_node, sizeof(*cache->ic_node));
	}
	kmem_free(cache, sizeof(*cache));
}

int
linux_idr_module_init(void)
{
	int error;

	error = lwp_specific_key_create(&idr_cache_key, &idr_cache_dtor);
	if (error)
		return error;

	return 0;
}

void
linux_idr_module_fini(void)
{

	lwp_specific_key_delete(idr_cache_key);
}

static signed int idr_tree_compare_nodes(void *, const void *, const void *);
static signed int idr_tree_compare_key(void *, const void *, const void *);

static const rb_tree_ops_t idr_rb_ops = {
	.rbto_compare_nodes = &idr_tree_compare_nodes,
	.rbto_compare_key = &idr_tree_compare_key,
	.rbto_node_offset = offsetof(struct idr_node, in_rb_node),
	.rbto_context = NULL,
};

static signed int
idr_tree_compare_nodes(void *ctx __unused, const void *na, const void *nb)
{
	const int a = ((const struct idr_node *)na)->in_index;
	const int b = ((const struct idr_node *)nb)->in_index;

	if (a < b)
		return -1;
	else if (b < a)
		 return +1;
	else
		return 0;
}

static signed int
idr_tree_compare_key(void *ctx __unused, const void *n, const void *key)
{
	const int a = ((const struct idr_node *)n)->in_index;
	const int b = *(const int *)key;

	if (a < b)
		return -1;
	else if (b < a)
		return +1;
	else
		return 0;
}

void
idr_init(struct idr *idr)
{

	mutex_init(&idr->idr_lock, MUTEX_DEFAULT, IPL_VM);
	rb_tree_init(&idr->idr_tree, &idr_rb_ops);
	SDT_PROBE1(sdt, linux, idr, init,  idr);
}

void
idr_destroy(struct idr *idr)
{

	SDT_PROBE1(sdt, linux, idr, destroy,  idr);
#if 0				/* XXX No rb_tree_destroy?  */
	rb_tree_destroy(&idr->idr_tree);
#endif
	mutex_destroy(&idr->idr_lock);
}

bool
idr_is_empty(struct idr *idr)
{

	return (RB_TREE_MIN(&idr->idr_tree) == NULL);
}

void *
idr_find(struct idr *idr, int id)
{
	const struct idr_node *node;
	void *data;

	mutex_spin_enter(&idr->idr_lock);
	node = rb_tree_find_node(&idr->idr_tree, &id);
	data = (node == NULL? NULL : node->in_data);
	mutex_spin_exit(&idr->idr_lock);

	return data;
}

void *
idr_get_next(struct idr *idr, int *idp)
{
	const struct idr_node *node;
	void *data;

	mutex_spin_enter(&idr->idr_lock);
	node = rb_tree_find_node_geq(&idr->idr_tree, idp);
	if (node == NULL) {
		data = NULL;
	} else {
		data = node->in_data;
		*idp = node->in_index;
	}
	mutex_spin_exit(&idr->idr_lock);

	return data;
}

void *
idr_replace(struct idr *idr, void *replacement, int id)
{
	struct idr_node *node;
	void *result;

	mutex_spin_enter(&idr->idr_lock);
	node = rb_tree_find_node(&idr->idr_tree, &id);
	if (node == NULL) {
		result = ERR_PTR(-ENOENT);
	} else {
		result = node->in_data;
		node->in_data = replacement;
		SDT_PROBE4(sdt, linux, idr, replace,
		    idr, id, result, replacement);
	}
	mutex_spin_exit(&idr->idr_lock);

	return result;
}

void
idr_remove(struct idr *idr, int id)
{
	struct idr_node *node;

	mutex_spin_enter(&idr->idr_lock);
	node = rb_tree_find_node(&idr->idr_tree, &id);
	KASSERTMSG((node != NULL), "idr %p has no entry for id %d", idr, id);
	SDT_PROBE3(sdt, linux, idr, remove,  idr, id, node->in_data);
	rb_tree_remove_node(&idr->idr_tree, node);
	mutex_spin_exit(&idr->idr_lock);

	kmem_free(node, sizeof(*node));
}

void
idr_preload(gfp_t gfp)
{
	struct idr_cache *cache;
	struct idr_node *node;
	km_flag_t kmflag = ISSET(gfp, __GFP_WAIT) ? KM_SLEEP : KM_NOSLEEP;

	SDT_PROBE0(sdt, linux, idr, preload);

	/* If caller asked to wait, we had better be sleepable.  */
	if (ISSET(gfp, __GFP_WAIT))
		ASSERT_SLEEPABLE();

	/*
	 * Get the current lwp's private idr cache.
	 */
	cache = lwp_getspecific(idr_cache_key);
	if (cache == NULL) {
		/* lwp_setspecific must be sleepable.  */
		if (!ISSET(gfp, __GFP_WAIT))
			return;
		cache = kmem_zalloc(sizeof(*cache), kmflag);
		if (cache == NULL)
			return;
		lwp_setspecific(idr_cache_key, cache);
	}

	/*
	 * If there already is a node, a prior call to idr_preload must
	 * not have been matched by idr_preload_end.  Print a warning,
	 * claim the node, and record our return address for where this
	 * node came from so the next leak is attributed to us.
	 */
	if (cache->ic_node) {
		idr_cache_warning(cache);
		goto out;
	}

	/*
	 * No cached node.  Allocate a new one, store it in the cache,
	 * and record our return address for where this node came from
	 * so the next leak is attributed to us.
	 */
	node = kmem_alloc(sizeof(*node), kmflag);
	KASSERT(node != NULL || !ISSET(gfp, __GFP_WAIT));
	if (node == NULL)
		return;

	cache->ic_node = node;
out:	cache->ic_where = __builtin_return_address(0);
}

int
idr_alloc(struct idr *idr, void *data, int start, int end, gfp_t gfp)
{
	int maximum = (end <= 0? INT_MAX : (end - 1));
	struct idr_cache *cache;
	struct idr_node *node, *search, *collision __diagused;
	int id = start;

	/* Sanity-check inputs.  */
	if (ISSET(gfp, __GFP_WAIT))
		ASSERT_SLEEPABLE();
	if (__predict_false(start < 0))
		return -EINVAL;
	if (__predict_false(maximum < start))
		return -ENOSPC;

	/*
	 * Grab a node allocated by idr_preload, if we have a cache and
	 * it is populated.
	 */
	cache = lwp_getspecific(idr_cache_key);
	if (cache == NULL || cache->ic_node == NULL)
		return -ENOMEM;
	node = cache->ic_node;
	cache->ic_node = NULL;

	/* Find an id.  */
	mutex_spin_enter(&idr->idr_lock);
	search = rb_tree_find_node_geq(&idr->idr_tree, &start);
	while ((search != NULL) && (search->in_index == id)) {
		if (maximum <= id) {
			id = -ENOSPC;
			goto out;
		}
		search = rb_tree_iterate(&idr->idr_tree, search, RB_DIR_RIGHT);
		id++;
	}
	node->in_index = id;
	node->in_data = data;
	collision = rb_tree_insert_node(&idr->idr_tree, node);
	KASSERT(collision == node);
out:	mutex_spin_exit(&idr->idr_lock);

	/* Discard the node on failure.  */
	if (id < 0) {
		cache->ic_node = node;
	} else {
		SDT_PROBE3(sdt, linux, idr, alloc,  idr, id, data);
	}
	return id;
}

void
idr_preload_end(void)
{
	struct idr_cache *cache;

	SDT_PROBE0(sdt, linux, idr, preload__end);

	/* Get the cache, or bail if it's not there.  */
	cache = lwp_getspecific(idr_cache_key);
	if (cache == NULL)
		return;

	/*
	 * If there is a node, either because we didn't idr_alloc or
	 * because idr_alloc failed, chuck it.
	 *
	 * XXX If we are not sleepable, then while the caller may have
	 * used idr_preload(GFP_ATOMIC), kmem_free may still sleep.
	 * What to do?
	 */
	if (cache->ic_node) {
		struct idr_node *node;

		node = cache->ic_node;
		cache->ic_node = NULL;
		cache->ic_where = NULL;

		kmem_free(node, sizeof(*node));
	}
}

int
idr_for_each(struct idr *idr, int (*proc)(int, void *, void *), void *arg)
{
	struct idr_node *node;
	int error = 0;

	/* XXX Caller must exclude modifications.  */
	membar_consumer();
	RB_TREE_FOREACH(node, &idr->idr_tree) {
		error = (*proc)(node->in_index, node->in_data, arg);
		if (error)
			break;
	}

	return error;
}
