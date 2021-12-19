/*	$NetBSD: linux_radixtree.c,v 1.1 2021/12/19 11:51:43 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_radixtree.c,v 1.1 2021/12/19 11:51:43 riastradh Exp $");

#include <sys/radixtree.h>

#include <linux/gfp.h>
#include <linux/radix-tree.h>

struct kludge {
	uint64_t	k_key;
	void		*k_datum;
};

void
INIT_RADIX_TREE(struct radix_tree_root *root, gfp_t gfp)
{

	radix_tree_init_tree(&root->rtr_tree);
}

int
radix_tree_insert(struct radix_tree_root *root, unsigned long key, void *datum)
{
	struct kludge *kludge;

	if ((kludge = kmem_zalloc(sizeof(*kludge), KM_NOSLEEP)) == NULL)
		return -ENOMEM;

	kludge->k_key = key;
	kludge->k_datum = datum;

	/* XXX errno NetBSD->Linux */
	return -radix_tree_insert_node(&root->rtr_tree, key, kludge);
}

void *
radix_tree_delete(struct radix_tree_root *root, unsigned long key)
{
	struct kludge *kludge;
	void *datum = NULL;

	if ((kludge = radix_tree_remove_node(&root->rtr_tree, key)) == NULL)
		return NULL;

	/* XXX RCU defer */
	datum = kludge->k_datum;
	kmem_free(kludge, sizeof(*kludge));

	return datum;
}

bool
radix_tree_empty(struct radix_tree_root *root)
{

	return radix_tree_empty_tree_p(&root->rtr_tree);
}

void *
radix_tree_lookup(const struct radix_tree_root *root, unsigned long key)
{
	struct kludge *kludge;

	kludge = radix_tree_lookup_node(&root->rtr_tree, key);
	if (kludge == NULL)
		NULL;

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

	KASSERT(flags == 0);
	if (radix_tree_gang_lookup_node(&root->rtr_tree, I->index,
		&result, /*maxresults*/1, /*dense*/false) == 0)
		return NULL;

	kludge = result;

	I->index = kludge->k_key;
	I->rti_tree = &root->rtr_tree;
	return &kludge->k_datum;
}

void **
radix_tree_next_slot(void **slot, struct radix_tree_iter *I, unsigned flags)
{
	struct kludge *kludge;
	void *result;

	KASSERT(flags == 0);
	kludge = container_of(slot, struct kludge, k_datum);
	if (radix_tree_gang_lookup_node(I->rtr_tree, kludge->k_key,
		&result, /*maxresults*/1, /*dense*/true) == 0)
		return NULL;

	kludge = result;

	I->index = kludge->k_key;
	I->rti_tree = &root->rtr_tree;
	return &kludge->k_datum;
}

void
radix_tree_iter_delete(struct radix_tree_root *root, struct radix_tree_iter *I,
    void **slot)
{
	struct kludge *kludge = container_of(slot, struct kludge, k_datum);
	struct kludge *kludge0 __diagused;

	kludge0 = radix_tree_remove_node(&root->rtr_tree, kludge->k_key);
	KASSERT(kludge0 == kludge);
}
