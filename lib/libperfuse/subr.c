/*  $NetBSD: subr.c,v 1.3 2010/09/01 14:57:24 manu Exp $ */

/*-
 *  Copyright (c) 2010 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <sysexits.h>
#include <syslog.h>
#include <puffs.h>
#include <paths.h>

#include "perfuse_priv.h"

struct puffs_node *
perfuse_new_pn(pu, parent)
	struct puffs_usermount *pu;
	struct puffs_node *parent;
{
	struct puffs_node *pn;
	struct perfuse_node_data *pnd;

	if ((pnd = malloc(sizeof(*pnd))) == NULL)
		DERR(EX_OSERR, "malloc failed");

	if ((pn = puffs_pn_new(pu, pnd)) == NULL)
		DERR(EX_SOFTWARE, "puffs_pn_new failed");

	(void)memset(pnd, 0, sizeof(*pnd));
	TAILQ_INIT(&pnd->pnd_fh);
	pnd->pnd_ino = PERFUSE_UNKNOWN_INO;
	pnd->pnd_nlookup = 1;
	pnd->pnd_parent = parent;
	TAILQ_INIT(&pnd->pnd_pcq);

	if (parent != NULL)
		PERFUSE_NODE_DATA(parent)->pnd_childcount++;

	return pn;
}

void
perfuse_destroy_pn(pn)
	struct puffs_node *pn;
{
	struct perfuse_node_data *pnd;

	if ((pnd = puffs_pn_getpriv(pn)) != NULL) {
		if (pnd->pnd_parent != NULL)
			PERFUSE_NODE_DATA(pnd->pnd_parent)->pnd_childcount--;

		if (pnd->pnd_dirent != NULL)
			free(pnd->pnd_dirent);

		if (pnd->pnd_all_fd != NULL)
			free(pnd->pnd_all_fd);
#ifdef PERFUSE_DEBUG
		if (!TAILQ_EMPTY(&pnd->pnd_fh))
			DERRX(EX_SOFTWARE, "%s: non empty pnd_fh", __func__);

		if (!TAILQ_EMPTY(&pnd->pnd_pcq))
			DERRX(EX_SOFTWARE, "%s: non empty pnd_pcq", __func__);
#endif /* PERFUSE_DEBUG */

		free(pnd);
	}

	puffs_pn_remove(pn);

	return;
}


void
perfuse_new_fh(opc, fh)
	puffs_cookie_t opc;
	uint64_t fh;
{
	struct perfuse_node_data *pnd;
	struct perfuse_file_handle *pfh;

	if (fh == FUSE_UNKNOWN_FH)
		return;

	pnd = PERFUSE_NODE_DATA(opc);
	pnd->pnd_flags |= PND_OPEN;

	if ((pfh = malloc(sizeof(*pfh))) == NULL)
		DERR(EX_OSERR, "malloc failed");

	pfh->pfh_fh = fh;

	TAILQ_INSERT_TAIL(&pnd->pnd_fh, pfh, pfh_entries);

	return;
}

void
perfuse_destroy_fh(opc, fh)
	puffs_cookie_t opc;
	uint64_t fh; 
{
	struct perfuse_node_data *pnd;
	struct perfuse_file_handle *pfh;

	pnd = PERFUSE_NODE_DATA(opc);

	TAILQ_FOREACH(pfh, &pnd->pnd_fh, pfh_entries) {
		if (pfh->pfh_fh == fh) {
			TAILQ_REMOVE(&pnd->pnd_fh, pfh, pfh_entries);
			free(pfh);
			break;
		}
	}

	if (TAILQ_EMPTY(&pnd->pnd_fh))
		pnd->pnd_flags &= ~PND_OPEN;

	if (pfh == NULL)
		DERRX(EX_SOFTWARE, 
		      "%s: unexistant fh = %"PRId64" (double close?)",
		      __func__, fh);
	
	return;
}

uint64_t
perfuse_get_fh(opc)
	puffs_cookie_t opc;
{
	struct perfuse_node_data *pnd;
	struct perfuse_file_handle *pfh;
	uint64_t fh = FUSE_UNKNOWN_FH;

	pnd = PERFUSE_NODE_DATA(opc);

	if ((pfh = TAILQ_FIRST(&pnd->pnd_fh)) != NULL)
		fh = pfh->pfh_fh;;

	return fh;
}

