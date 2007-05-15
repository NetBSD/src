/*      $NetBSD: subr.c,v 1.16 2007/05/15 13:46:48 pooka Exp $        */
        
/*      
 * Copyright (c) 2006  Antti Kantee.  All Rights Reserved.
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
#ifndef lint
__RCSID("$NetBSD: subr.c,v 1.16 2007/05/15 13:46:48 pooka Exp $");
#endif /* !lint */

#include <assert.h>
#include <errno.h>
#include <puffs.h>
#include <stdlib.h>
#include <util.h>

#include "psshfs.h"
#include "sftp_proto.h"

static void
freedircache(struct psshfs_dir *base, size_t count)
{
	int i;

	for (i = 0; i < count; i++) {
		free(base[i].entryname);
		base[i].entryname = NULL;
	}

	free(base);
}

#define ENTRYCHUNK 16
static void
allocdirs(struct psshfs_node *psn)
{
	size_t oldtot = psn->denttot;

	psn->denttot += ENTRYCHUNK;
	psn->dir = erealloc(psn->dir,
	    psn->denttot * sizeof(struct psshfs_dir));
	memset(psn->dir + oldtot, 0, ENTRYCHUNK * sizeof(struct psshfs_dir));
}

struct psshfs_dir *
lookup(struct psshfs_dir *bdir, size_t ndir, const char *name)
{
	struct psshfs_dir *test;
	int i;

	for (i = 0; i < ndir; i++) {
		test = &bdir[i];
		if (test->valid != 1)
			continue;
		if (strcmp(test->entryname, name) == 0)
			return test;
	}

	return NULL;
}

static struct psshfs_dir *
lookup_by_entry(struct psshfs_dir *bdir, size_t ndir, struct puffs_node *entry)
{
	struct psshfs_dir *test;
	int i;

	for (i = 0; i < ndir; i++) {
		test = &bdir[i];
		if (test->valid != 1)
			continue;
		if (test->entry == entry)
			return test;
	}

	return NULL;
}

#ifdef SUPERREADDIR
struct readdirattr {
	struct psshfs_node *psn;
	int idx;
	char entryname[MAXPATHLEN+1];
};

static void
readdir_getattr_resp(struct puffs_usermount *pu,
	struct puffs_framebuf *pb, void *arg)
{
	struct readdirattr *rda = arg;
	struct psshfs_node *psn = rda->psn;
	struct psshfs_dir *pdir;
	struct vattr va;

	pdir = lookup(psn->dir, psn->denttot, rda->entryname);
	if (!pdir)
		goto out;

	if (psbuf_expect_attrs(pb, &va))
		goto out;

	if (pdir->entry) {
		struct psshfs_node *psn_targ;
		psn_targ = pdir->entry->pn_data;

		puffs_setvattr(&pdir->entry->pn_va, &va);
		psn_targ->attrread = time(NULL);
	} else {
		puffs_setvattr(&pdir->va, &va);
		pdir->attrread = time(NULL);
	}

 out:
	free(rda);
	puffs_framebuf_destroy(pb);
}

static int
readdir_getattr(struct puffs_usermount *pu, struct psshfs_node *psn,
	const char *basepath, int idx)
{
	char path[MAXPATHLEN+1];
	struct psshfs_ctx *pctx = puffs_getspecific(pu);
	struct psshfs_dir *pdir = psn->dir;
	struct puffs_framebuf *pb;
	struct readdirattr *rda;
	const char *entryname = pdir[idx].entryname;
	uint32_t reqid = NEXTREQ(pctx);
	int rv = 0;

	rda = emalloc(sizeof(struct readdirattr));
	rda->psn = psn;
	rda->idx = idx;
	strlcpy(rda->entryname, entryname, sizeof(rda->entryname));

	strcpy(path, basepath);
	strcat(path, "/");
	strlcat(path, entryname, sizeof(path));

	pb = psbuf_makeout();
	psbuf_req_str(pb, SSH_FXP_LSTAT, reqid, path);
	SENDCB(pb, readdir_getattr_resp, rda);

 out:
	return rv;
}
#endif

int
getpathattr(struct puffs_cc *pcc, const char *path, struct vattr *vap)
{
	PSSHFSAUTOVAR(pcc);

	psbuf_req_str(pb, SSH_FXP_LSTAT, reqid, path);
	GETRESPONSE(pb);

	rv = psbuf_expect_attrs(pb, vap);

 out:
	PSSHFSRETURN(rv);
}

int
getnodeattr(struct puffs_cc *pcc, struct puffs_node *pn)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct psshfs_node *psn = pn->pn_data;
	struct vattr va;
	int rv;

	if ((time(NULL) - psn->attrread) >= PSSHFS_REFRESHIVAL) {
		rv = getpathattr(pcc, PNPATH(pn), &va);
		if (rv)
			return rv;

		/*
		 * Check if the file was modified from below us.  If
		 * so, invalidate page cache.  This is the only sensible
		 * place we can do this in.
		 */
		if (psn->attrread)
			if (pn->pn_va.va_mtime.tv_sec != va.va_mtime.tv_sec)
				puffs_inval_pagecache_node(pu, pn);

		puffs_setvattr(&pn->pn_va, &va);
		psn->attrread = time(NULL);
	}

	return 0;
}

int
sftp_readdir(struct puffs_cc *pcc, struct psshfs_ctx *pctx,
	struct puffs_node *pn)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct psshfs_node *psn = pn->pn_data;
	struct psshfs_dir *olddir, *testd;
	struct puffs_framebuf *pb;
	uint32_t reqid = NEXTREQ(pctx);
	uint32_t count, dhandlen;
	char *dhand = NULL;
	size_t nent;
	char *longname;
	int idx, rv;

	assert(pn->pn_va.va_type == VDIR);
	idx = 0;
	olddir = psn->dir;
	nent = psn->dentnext;

	if (psn->dir && (time(NULL) - psn->dentread) < PSSHFS_REFRESHIVAL)
		return 0;

	puffs_inval_namecache_dir(puffs_cc_getusermount(pcc), pn);

	pb = psbuf_makeout();
	psbuf_req_str(pb, SSH_FXP_OPENDIR, reqid, PNPATH(pn));
	GETRESPONSE(pb);

	rv = psbuf_expect_handle(pb, &dhand, &dhandlen);
	if (rv)
		goto wayout;

	/*
	 * Well, the following is O(n^2), so feel free to improve if it
	 * gets too taxing on your system.
	 */

	/*
	 * note: for the "getattr in batch" to work, this must be before
	 * the attribute-getting.  Otherwise times for first entries in
	 * large directories might expire before the directory itself and
	 * result in one-by-one attribute fetching.
	 */
	psn->dentread = time(NULL);

	psn->dentnext = 0;
	psn->denttot = 0;
	psn->dir = NULL;

	for (;;) {
		reqid = NEXTREQ(pctx);
		psbuf_recycleout(pb);
		psbuf_req_data(pb, SSH_FXP_READDIR, reqid, dhand, dhandlen);
		GETRESPONSE(pb);

		/* check for EOF */
		if (psbuf_get_type(pb) == SSH_FXP_STATUS) {
			rv = psbuf_expect_status(pb);
			goto out;
		}
		rv = psbuf_expect_name(pb, &count);
		if (rv)
			goto out;

		for (; count--; idx++) {
			if (idx == psn->denttot)
				allocdirs(psn);
			if ((rv = psbuf_get_str(pb,
			    &psn->dir[idx].entryname, NULL)))
				goto out;
			if ((rv = psbuf_get_str(pb, &longname, NULL)))
				goto out;
			if ((rv = psbuf_get_vattr(pb, &psn->dir[idx].va)))
				goto out;
			if (sscanf(longname, "%*s%d",
			    &psn->dir[idx].va.va_nlink) != 1) {
				rv = EPROTO;
				goto out;
			}
			free(longname);

			testd = lookup(olddir, nent, psn->dir[idx].entryname);
			if (testd) {
				psn->dir[idx].entry = testd->entry;
				psn->dir[idx].va = testd->va;
			} else {
				psn->dir[idx].entry = NULL;
				psn->dir[idx].va.va_fileid = pctx->nextino++;
			}
#ifdef SUPERREADDIR
			/*
			 * XXX: there's a dangling pointer race here if
			 * the server responds to our queries out-of-order.
			 * fixxxme some day
			 */
			rv = readdir_getattr(puffs_cc_getusermount(pcc),
			    psn, PNPATH(pn), idx);
			if (rv)
				goto out;
#endif
			psn->dir[idx].valid = 1;
		}
	}

 out:
	/* XXX: rv */
	psn->dentnext = idx;
	freedircache(olddir, nent);

	reqid = NEXTREQ(pctx);
	psbuf_recycleout(pb);
	psbuf_req_data(pb, SSH_FXP_CLOSE, reqid, dhand, dhandlen);

	JUSTSEND(pb);
	free(dhand);
	return rv;

 wayout:
	free(dhand);
	PSSHFSRETURN(rv);
}

struct puffs_node *
makenode(struct puffs_usermount *pu, struct puffs_node *parent,
	struct psshfs_dir *pd, const struct vattr *vap)
{
	struct psshfs_node *psn_parent = parent->pn_data;
	struct psshfs_node *psn;
	struct puffs_node *pn;

	psn = emalloc(sizeof(struct psshfs_node));
	memset(psn, 0, sizeof(struct psshfs_node));

	pn = puffs_pn_new(pu, psn);
	if (!pn) {
		free(psn);
		return NULL;
	}
	puffs_setvattr(&pn->pn_va, &pd->va);
	psn->attrread = pd->attrread;
	puffs_setvattr(&pn->pn_va, vap);

	pd->entry = pn;
	psn->parent = parent;
	psn_parent->childcount++;

	return pn;
}

struct puffs_node *
allocnode(struct puffs_usermount *pu, struct puffs_node *parent,
	const char *entryname, const struct vattr *vap)
{
	struct psshfs_ctx *pctx = puffs_getspecific(pu);
	struct psshfs_dir *pd;
	struct puffs_node *pn;

	pd = direnter(parent, entryname);

	pd->va.va_fileid = pctx->nextino++;
	if (vap->va_type == VDIR) {
		pd->va.va_nlink = 2;
		parent->pn_va.va_nlink++;
	} else {
		pd->va.va_nlink = 1;
	}

	pn = makenode(pu, parent, pd, vap);
	if (pn)
		pd->va.va_fileid = pn->pn_va.va_fileid;

	return pn;
}

struct psshfs_dir *
direnter(struct puffs_node *parent, const char *entryname)
{
	struct psshfs_node *psn_parent = parent->pn_data;
	struct psshfs_dir *pd;
	int i;

	/* create directory entry */
	if (psn_parent->denttot == psn_parent->dentnext)
		allocdirs(psn_parent);

	i = psn_parent->dentnext;
	pd = &psn_parent->dir[i];
	pd->entryname = estrdup(entryname);
	pd->valid = 1;
	pd->attrread = 0;
	puffs_vattr_null(&pd->va);
	psn_parent->dentnext++;

	return pd;
}

void
doreclaim(struct puffs_node *pn)
{
	struct psshfs_node *psn = pn->pn_data;
	struct psshfs_node *psn_parent;
	struct psshfs_dir *dent;

	psn_parent = psn->parent->pn_data;
	psn_parent->childcount--;

	/*
	 * Null out entry from directory.  Do not treat a missing entry
	 * as an invariant error, since the node might be removed from
	 * under us, and we might do a readdir before the reclaim resulting
	 * in no directory entry in the parent directory.
	 */
	dent = lookup_by_entry(psn_parent->dir, psn_parent->dentnext, pn);
	if (dent)
		dent->entry = NULL;

	if (pn->pn_va.va_type == VDIR)
		freedircache(psn->dir, psn->dentnext);

	puffs_pn_put(pn);
}

void
nukenode(struct puffs_node *node, const char *entryname, int reclaim)
{
	struct psshfs_node *psn, *psn_parent;
	struct psshfs_dir *pd;

	psn = node->pn_data;
	psn_parent = psn->parent->pn_data;
	pd = lookup(psn_parent->dir, psn_parent->dentnext, entryname);
	assert(pd != NULL);
	pd->valid = 0;
	free(pd->entryname);
	pd->entryname = NULL;

	if (node->pn_va.va_type == VDIR)
		psn->parent->pn_va.va_nlink--;

	if (reclaim)
		doreclaim(node);
}
