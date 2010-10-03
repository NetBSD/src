/*  $NetBSD: subr.c,v 1.8 2010/10/03 05:46:47 manu Exp $ */

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

static size_t node_path(puffs_cookie_t, char *, size_t);

struct puffs_node *
perfuse_new_pn(pu, name, parent)
	struct puffs_usermount *pu;
	const char *name;
	struct puffs_node *parent;
{
	struct perfuse_state *ps;
	struct puffs_node *pn;
	struct perfuse_node_data *pnd;

	ps = puffs_getspecific(pu);

	if ((pnd = malloc(sizeof(*pnd))) == NULL)
		DERR(EX_OSERR, "malloc failed");

	if ((pn = puffs_pn_new(pu, pnd)) == NULL)
		DERR(EX_SOFTWARE, "puffs_pn_new failed");

	(void)memset(pnd, 0, sizeof(*pnd));
	pnd->pnd_rfh = FUSE_UNKNOWN_FH;
	pnd->pnd_wfh = FUSE_UNKNOWN_FH;
	pnd->pnd_ino = PERFUSE_UNKNOWN_INO;
	pnd->pnd_nlookup = 1;
	pnd->pnd_parent = parent;
	pnd->pnd_timestamp = time(NULL);
	pnd->pnd_pn = (puffs_cookie_t)pn;
	(void)strlcpy(pnd->pnd_name, name, MAXPATHLEN);
	TAILQ_INIT(&pnd->pnd_pcq);
	TAILQ_INIT(&pnd->pnd_children);

	if (parent != NULL) {
		struct perfuse_node_data *parent_pnd;

		parent_pnd = PERFUSE_NODE_DATA(parent);
		TAILQ_INSERT_TAIL(&parent_pnd->pnd_children, pnd, pnd_next);

		parent_pnd->pnd_childcount++;
	}

	return pn;
}

void
perfuse_destroy_pn(pu, pn)
	struct puffs_usermount *pu;
	struct puffs_node *pn;
{
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd;

	ps = puffs_getspecific(pu);
	pnd = PERFUSE_NODE_DATA(pn);

	if (pnd->pnd_parent != NULL) {
		struct perfuse_node_data *parent_pnd;

		parent_pnd = PERFUSE_NODE_DATA(pnd->pnd_parent);
		TAILQ_REMOVE(&parent_pnd->pnd_children, pnd, pnd_next);
	}

	if ((pnd = puffs_pn_getpriv(pn)) != NULL) {
		if (pnd->pnd_parent != NULL)
			PERFUSE_NODE_DATA(pnd->pnd_parent)->pnd_childcount--;

		if (pnd->pnd_dirent != NULL)
			free(pnd->pnd_dirent);

		if (pnd->pnd_all_fd != NULL)
			free(pnd->pnd_all_fd);
#ifdef PERFUSE_DEBUG
		if (pnd->pnd_flags & PND_OPEN)
			DERRX(EX_SOFTWARE, "%s: file open", __func__);

		if (!TAILQ_EMPTY(&pnd->pnd_pcq))
			DERRX(EX_SOFTWARE, "%s: non empty pnd_pcq", __func__);
#endif /* PERFUSE_DEBUG */

		free(pnd);
	}

	puffs_pn_put(pn);

	return;
}


void
perfuse_new_fh(opc, fh, mode)
	puffs_cookie_t opc;
	uint64_t fh;
	int mode;
{
	struct perfuse_node_data *pnd;

	pnd = PERFUSE_NODE_DATA(opc);

	if (mode & FWRITE) {
		if (pnd->pnd_flags & PND_WFH)
			DERRX(EX_SOFTWARE, "%s: opc = %p, write fh already set",
			      __func__, (void *)opc);	
		pnd->pnd_wfh = fh;
		pnd->pnd_flags |= PND_WFH;
	} 

	if (mode & FREAD) {
		if (pnd->pnd_flags & PND_RFH)
			DERRX(EX_SOFTWARE, "%s: opc = %p, read fh already set",
			      __func__, (void *)opc);	
		pnd->pnd_rfh = fh;
		pnd->pnd_flags |= PND_RFH;
	}

	return;
}

void
perfuse_destroy_fh(opc, fh)
	puffs_cookie_t opc;
	uint64_t fh; 
{
	struct perfuse_node_data *pnd;

	pnd = PERFUSE_NODE_DATA(opc);

	if (fh == pnd->pnd_rfh) {
		if (!(pnd->pnd_flags & PND_RFH) && (fh != FUSE_UNKNOWN_FH))
			DERRX(EX_SOFTWARE, 
			      "%s: opc = %p, unset rfh = %"PRIx64"",
			      __func__, (void *)opc, fh);	
		pnd->pnd_rfh = FUSE_UNKNOWN_FH;
		pnd->pnd_flags &= ~PND_RFH;
	}

	if (fh == pnd->pnd_wfh) {
		if (!(pnd->pnd_flags & PND_WFH) && (fh != FUSE_UNKNOWN_FH))
			DERRX(EX_SOFTWARE,
			      "%s: opc = %p, unset wfh = %"PRIx64"",
			      __func__, (void *)opc, fh);	
		pnd->pnd_wfh = FUSE_UNKNOWN_FH;
		pnd->pnd_flags &= ~PND_WFH;
	} 

	return;
}

uint64_t
perfuse_get_fh(opc, mode)
	puffs_cookie_t opc;
	int mode;
{
	struct perfuse_node_data *pnd;

	pnd = PERFUSE_NODE_DATA(opc);

	if (mode & FWRITE) {
		if (pnd->pnd_flags & PND_WFH)
			return pnd->pnd_wfh;
	}

	if (mode & FREAD) {
		if (pnd->pnd_flags & PND_RFH)
			return pnd->pnd_rfh;

		if (pnd->pnd_flags & PND_WFH)
			return pnd->pnd_wfh;

	}

	return FUSE_UNKNOWN_FH;
}

static size_t
node_path(opc, buf, buflen)
	puffs_cookie_t opc;
	char *buf;
	size_t buflen;
{
	struct perfuse_node_data *pnd;
	size_t written;
	
	pnd = PERFUSE_NODE_DATA(opc);
	if (pnd->pnd_parent == opc)
		return 0;

	written = node_path(pnd->pnd_parent, buf, buflen);
	buf += written;
	buflen -= written;
	
	return written + snprintf(buf, buflen, "/%s", pnd->pnd_name);
}

char *
perfuse_node_path(opc)
	puffs_cookie_t opc;
{
	static char buf[MAXPATHLEN + 1];

	if (node_path(opc, buf, sizeof(buf)) == 0)
		sprintf(buf, "/");

	return buf;
}
