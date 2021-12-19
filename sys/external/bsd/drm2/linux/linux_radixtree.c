/*	$NetBSD: linux_radixtree.c,v 1.2 2021/12/19 11:51:51 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_radixtree.c,v 1.2 2021/12/19 11:51:51 riastradh Exp $");

#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/lock.h>
#include <sys/radixtree.h>

#include <linux/gfp.h>
#include <linux/radix-tree.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>

/* XXX mega-kludgerific */
#define	__UNVOLANST(p)	((void *)(unsigned long)(const volatile void *)(p))

struct kludge {
	uint64_t	k_key;
	void		*k_datum;
	struct rcu_head	k_rcu;
};

void
INIT_RADIX_TREE(struct radix_tree_root *root, gfp_t gfp)
{

	radix_tree_init_tree(&root->rtr_tree);
	__cpu_simple_lock_init(&root->rtr_lock);
}

int
radix_tree_insert(struct radix_tree_root *root, unsigned long key, void *datum)
{
	struct kludge *kludge;
	int error;

	/* XXX No way to know whether the caller can sleep or not...  */
	if ((kludge = kzalloc(sizeof(*kludge), GFP_NOWAIT)) == NULL)
		return -ENOMEM;

	kludge->k_key = key;
	kludge->k_datum = datum;

	membar_exit();

	__cpu_simple_lock(&root->rtr_lock);
	error = radix_tree_insert_node(&root->rtr_tree, key, kludge);
	__cpu_simple_unlock(&root->rtr_lock);

	/* XXX errno NetBSD->Linux */
	return -error;
}

void *
radix_tree_delete(struct radix_tree_root *root, unsigned long key)
{
	struct kludge *kludge;
	void *datum = NULL;

	__cpu_simple_lock(&root->rtr_lock);
	kludge = radix_tree_remove_node(&root->rtr_tree, key);
	__cpu_simple_unlock(&root->rtr_lock);
	if (kludge == NULL)
		return NULL;

	datum = kludge->k_datum;
	kfree_rcu(kludge, k_rcu);

	return datum;
}

bool
radix_tree_empty(struct radix_tree_root *root)
{
	bool empty;

	__cpu_simple_lock(&root->rtr_lock);
	empty = radix_tree_empty_tree_p(&root->rtr_tree);
	__cpu_simple_unlock(&root->rtr_lock);

	return empty;
}

void *
radix_tree_lookup(const struct radix_tree_root *root, unsigned long key)
{
	struct kludge *kludge;

	__cpu_simple_lock(__UNVOLANST(&root->rtr_lock));
	kludge = radix_tree_lookup_node(__UNVOLANST(&root->rtr_tree), key);
	__cpu_simple_unlock(__UNVOLANST(&root->rtr_lock));
	if (kludge == NULL)
		return NULL;

	return kludge->k_datum;
}

void *
radix_tree_deref_slot(void **slot)
{

	return atomic_load_consume(slot);
}

void **
radix_tree_iter_init(struct radix_tree_iter *I, unsigned long start)
{

	I->index = start;
	I->rti_tree = NULL;
	return NULL;
}

void **
radix_tree_next_chunk(const struct radix_tree_root *root,
    struct radix_tree_iter *I, unsigned flags)
{
	void *result;
	struct kludge *kludge;
	unsigned nresults;

	KASSERT(flags == 0);
	__cpu_simple_lock(__UNVOLANST(&root->rtr_lock));
	nresults = radix_tree_gang_lookup_node(__UNVOLANST(&root->rtr_tree),
	    I->index, &result, /*maxresults*/1, /*dense*/false);
	__cpu_simple_unlock(__UNVOLANST(&root->rtr_lock));
	if (nresults == 0)
		return NULL;
	KASSERT(nresults == 1);

	kludge = result;

	I->index = kludge->k_key;
	I->rti_tree = root;
	return &kludge->k_datum;
}

void **
radix_tree_next_slot(void **slot, struct radix_tree_iter *I, unsigned flags)
{
	struct kludge *kludge;
	void *result;
	unsigned nresults;

	KASSERT(flags == 0);

	kludge = container_of(slot, struct kludge, k_datum);

	__cpu_simple_lock(__UNVOLANST(&I->rti_tree->rtr_lock));
	nresults = radix_tree_gang_lookup_node(
	    __UNVOLANST(&I->rti_tree->rtr_tree),
	    kludge->k_key, &result, /*maxresults*/1, /*dense*/true);
	__cpu_simple_unlock(__UNVOLANST(&I->rti_tree->rtr_lock));
	if (nresults == 0)
		return NULL;
	KASSERT(nresults == 1);

	kludge = result;

	I->index = kludge->k_key;
	/* XXX Hope the tree hasn't changed!  */
	return &kludge->k_datum;
}

void
radix_tree_iter_delete(struct radix_tree_root *root, struct radix_tree_iter *I,
    void **slot)
{
	struct kludge *kludge = container_of(slot, struct kludge, k_datum);
	struct kludge *kludge0 __diagused;

	__cpu_simple_lock(&root->rtr_lock);
	kludge0 = radix_tree_remove_node(&root->rtr_tree, kludge->k_key);
	__cpu_simple_unlock(&root->rtr_lock);

	KASSERT(kludge0 == kludge);
}
