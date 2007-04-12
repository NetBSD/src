/*	$NetBSD: pnode.c,v 1.5 2007/04/12 15:09:01 pooka Exp $	*/

/*
 * Copyright (c) 2006 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: pnode.c,v 1.5 2007/04/12 15:09:01 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>

#include <assert.h>
#include <puffs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "puffs_priv.h"

/*
 * Well, you're probably wondering why this isn't optimized.
 * The reason is simple: my available time is not optimized for
 * size ... so please be patient ;)
 */
struct puffs_node *
puffs_pn_new(struct puffs_usermount *pu, void *privdata)
{
	struct puffs_node *pn;

	pn = calloc(1, sizeof(struct puffs_node));
	if (pn == NULL)
		return NULL;

	pn->pn_data = privdata;
	pn->pn_mnt = pu;
	puffs_vattr_null(&pn->pn_va);

	LIST_INSERT_HEAD(&pu->pu_pnodelst, pn, pn_entries);

	return pn;
}

void
puffs_pn_put(struct puffs_node *pn)
{
	struct puffs_usermount *pu = pn->pn_mnt;

	pu->pu_pathfree(pu, &pn->pn_po);
	LIST_REMOVE(pn, pn_entries);
	free(pn);
}

/* walk list, rv can be used either to halt or to return a value */
void *
puffs_pn_nodewalk(struct puffs_usermount *pu, puffs_nodewalk_fn fn, void *arg)
{
	struct puffs_node *pn_cur, *pn_next;
	void *rv;

	pn_cur = LIST_FIRST(&pu->pu_pnodelst);
	while (pn_cur) {
		pn_next = LIST_NEXT(pn_cur, pn_entries);
		rv = fn(pu, pn_cur, arg);
		if (rv)
			return rv;
		pn_cur = pn_next;
	}

	return NULL;
}

/* convenience / shortcut */
void *
puffs_pn_getmntspecific(struct puffs_node *pn)
{

	return pn->pn_mnt->pu_privdata;
}
