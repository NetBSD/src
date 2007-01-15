/*	$NetBSD: paths.c,v 1.1 2007/01/15 00:39:02 pooka Exp $	*/

/*
 * Copyright (c) 2007  Antti Kantee.  All Rights Reserved.
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
__RCSID("$NetBSD: paths.c,v 1.1 2007/01/15 00:39:02 pooka Exp $");
#endif /* !lint */

#include <assert.h>
#include <errno.h>
#include <puffs.h>
#include <stdlib.h>

#include "puffs_priv.h"

/*
 * Generic routines for pathbuilding code
 */

int
puffs_path_pcnbuild(struct puffs_usermount *pu, struct puffs_cn *pcn,
	void *parent)
{
	struct puffs_node *pn_parent = PU_CMAP(pu, parent);
	struct puffs_cn pcn_orig;
	struct puffs_pathobj po;
	int rv;

	assert(pn_parent->pn_po.po_path != NULL);

	if (pu->pu_pathtransform) {
		rv = pu->pu_pathtransform(pu, &pn_parent->pn_po, pcn, &po);
		if (rv)
			return rv;
	} else {
		po.po_path = pcn->pcn_name;
		po.po_len = pcn->pcn_namelen;
	}

	if (pu->pu_namemod) {
		/* XXX: gcc complains if I do assignment */
		memcpy(&pcn_orig, pcn, sizeof(pcn_orig));
		rv = pu->pu_namemod(pu, &pn_parent->pn_po, pcn);
		if (rv)
			return rv;
	}

	rv = pu->pu_pathbuild(pu, &pn_parent->pn_po, &po, 0,
	    &pcn->pcn_po_full);

	if (pu->pu_pathtransform)
		pu->pu_pathfree(pu, &po);

	if (pu->pu_namemod && rv)
		*pcn = pcn_orig;

	return rv;
}

/*
 * substitute all (child) patch prefixes.  called from nodewalk, which
 * in turn is called from rename
 */
void *
puffs_path_prefixadj(struct puffs_usermount *pu, struct puffs_node *pn,
	void *arg)
{
	struct puffs_pathinfo *pi = arg;
	struct puffs_pathobj localpo;
	struct puffs_pathobj oldpo;
	int rv;

	/* can't be a path prefix */
	if (pn->pn_po.po_len < pi->pi_old->po_len)
		return NULL;

	if (pu->pu_pathcmp(pu, &pn->pn_po, pi->pi_old, pi->pi_old->po_len))
		return NULL;

	/* otherwise we'd have two nodes with an equal path */
	assert(pn->pn_po.po_len > pi->pi_old->po_len);

	/* found a matching prefix */
	rv = pu->pu_pathbuild(pu, pi->pi_new, &pn->pn_po,
	    pi->pi_old->po_len, &localpo);
	/*
	 * XXX: technically we shouldn't fail, but this is the only
	 * sensible thing to do here.  If the buildpath routine fails,
	 * we will have paths in an inconsistent state.  Should fix this,
	 * either by having two separate passes or by doing other tricks
	 * to make an invalid path with BUILDPATHS acceptable.
	 */
	if (rv != 0)
		abort();

	/* out with the old and in with the new */
	oldpo = pn->pn_po;
	pn->pn_po = localpo;
	pu->pu_pathfree(pu, &oldpo);

	/* continue the walk */
	return NULL;
}


/*
 * Routines provided to file systems which consider a path a tuple of
 * strings and / the component separator.
 */

/*ARGSUSED*/
int
puffs_path_cmppath(struct puffs_usermount *pu, struct puffs_pathobj *c1,
	struct puffs_pathobj *c2, size_t clen)
{
	char *p;
	int rv;

	rv = strncmp(c1->po_path, c2->po_path, clen);
	if (rv)
		return 1;

	/* sanity for next step */
	if (!(c1->po_len > c2->po_len))
		return 1;

	/* check if it's really a complete path prefix */
	p = c1->po_path;
	if ((*(p + clen)) != '/')
		return 1;

	return 0;
}

/*ARGSUSED*/
int
puffs_path_buildpath(struct puffs_usermount *pu, struct puffs_pathobj *po_pre,
	struct puffs_pathobj *po_comp, size_t offset,
	struct puffs_pathobj *newpath)
{
	char *path, *pcomp;
	size_t plen, complen;

	complen = po_comp->po_len - offset;

	/* seek to correct place & remove all leading '/' from component */
	pcomp = po_comp->po_path;
	pcomp += offset;
	while (*pcomp == '/') {
		pcomp++;
		complen--;
	}

	/* + '/' + '\0' */
	plen = po_pre->po_len + 1 + complen;
	path = malloc(plen + 1);
	if (path == NULL)
		return errno;

	strcpy(path, po_pre->po_path);
	strcat(path, "/");
	strncat(path, pcomp, complen);

	newpath->po_path = path;
	newpath->po_len = plen;

	return 0;
}

/*ARGSUSED*/
void
puffs_path_freepath(struct puffs_usermount *pu, struct puffs_pathobj *po)
{

	free(po->po_path);
}
