/*	$NetBSD: dtfs_vnops.c,v 1.3 2006/10/25 18:18:16 pooka Exp $	*/

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

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "dtfs.h"

int
dtfs_lookup(struct puffs_usermount *pu, void *opc, void **newnode,
	enum vtype *newtype, const struct puffs_cn *pcn)
{
	struct dtfs_file *df = DTFS_CTOF(opc);
	struct dtfs_dirent *dfd;

	/* parent dir? */
	if (pcn->pcn_flags & PUFFS_ISDOTDOT) {
		*newnode = df->df_dotdot;
		*newtype = df->df_dotdot->pn_va.va_type;
		assert(*newtype == VDIR);

		return 0;
	}

	dfd = dtfs_dirgetbyname(df, pcn->pcn_name);
	if (dfd) {
		*newnode = dfd->dfd_node;
		*newtype = dfd->dfd_node->pn_va.va_type;
		return 0;
	} else if ((pcn->pcn_nameiop & PUFFSLOOKUP_OPMASK) == PUFFSLOOKUP_CREATE
	    || (pcn->pcn_nameiop & PUFFSLOOKUP_OPMASK) == PUFFSLOOKUP_RENAME){
		*newnode = NULL;
		*newtype = 0;
		return -1; /* no error, but no beef */
	}

	return ENOENT;
}

/* no credcheck */
int
dtfs_getattr(struct puffs_usermount *pu, void *opc,
	struct vattr *va, const struct puffs_cred *pcr, pid_t pid)
{
	struct puffs_node *pn = opc;

	memcpy(va, &pn->pn_va, sizeof(struct vattr));
	return 0;
}

/* no credcheck */
int
dtfs_setattr(struct puffs_usermount *pu, void *opc,
	const struct vattr *va, const struct puffs_cred *pcr, pid_t pid)
{
	struct puffs_node *pn = opc;

	/* check if we need to modify our internal size */
	/* (must be called before setattr! XXX) */
	if (va->va_size != PUFFS_VNOVAL) {
		dtfs_setsize(pn, va->va_size, 0);
		pn->pn_va.va_bytes = va->va_size;
	}

	puffs_setvattr(&pn->pn_va, va);

	return 0;
}

/* create a new node in the parent directory specified by opc */
int
dtfs_create(struct puffs_usermount *pu, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct puffs_node *pn_parent = opc;
	struct puffs_node *pn_new;

	if (va->va_type != VREG)
		return ENODEV;

	pn_new = dtfs_genfile(pn_parent, pcn->pcn_name, va->va_type);
	puffs_setvattr(&pn_new->pn_va, va);

	*newnode = pn_new;

	return 0;
}

int
dtfs_remove(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node *pn_parent = opc;
	struct puffs_node *pn = targ;

	if (pn->pn_va.va_type == VDIR)
		return EISDIR;

	dtfs_nukenode(targ, pn_parent, pcn->pcn_name);

	return 0;
}

int
dtfs_mkdir(struct puffs_usermount *pu, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct puffs_node *pn_parent = opc;
	struct puffs_node *pn_new;

	pn_new = dtfs_genfile(pn_parent, pcn->pcn_name, VDIR);
	puffs_setvattr(&pn_new->pn_va, va);

	*newnode = pn_new;

	return 0;
}

int
dtfs_rmdir(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node *pn_parent = opc;
	struct dtfs_file *df = DTFS_CTOF(targ);

	if (!LIST_EMPTY(&df->df_dirents))
		return ENOTEMPTY;

	dtfs_nukenode(targ, pn_parent, pcn->pcn_name);

	return 0;
}

int
dtfs_readdir(struct puffs_usermount *pi, void *opc,
	struct dirent *dent, const struct puffs_cred *pcr, off_t *readoff,
	size_t *reslen)
{
	struct puffs_node *pn = opc;
	struct puffs_node *pn_nth;
	struct dtfs_dirent *dfd_nth;

	if (pn->pn_type != VDIR)
		return ENOTDIR;

 again:
	if (*readoff == DENT_DOT || *readoff == DENT_DOTDOT) {
		puffs_gendotdent(&dent, pn->pn_va.va_fileid, *readoff, reslen);
		(*readoff)++;
		goto again;
	}

	for (;;) {
		dfd_nth = dtfs_dirgetnth(pn->pn_data, DENT_ADJ(*readoff));
		if (!dfd_nth)
			return 0;
		pn_nth = dfd_nth->dfd_node;

		if (!puffs_nextdent(&dent, dfd_nth->dfd_name,
		    pn_nth->pn_va.va_fileid,
		    puffs_vtype2dt(pn_nth->pn_type),
		    reslen))
			return 0;
		(*readoff)++;
	}
}

int
dtfs_rename(struct puffs_usermount *pu, void *opc, void *src,
	const struct puffs_cn *pcn_src, void *targ_dir, void *targ,
	const struct puffs_cn *pcn_targ)
{
	struct dtfs_dirent *dfd_src;
	struct puffs_node *pn_sdir = opc;
	struct puffs_node *pn_tdir = targ_dir;
	struct puffs_node *pn_tfile = targ;

	dfd_src = dtfs_dirgetbyname(DTFS_PTOF(pn_sdir),
	    pcn_src->pcn_name);

	/* asked for "." or ".." XXX: make sure? */
	if (!dfd_src)
		return EINVAL;

	/* if there's a target file, nuke it for atomic replacement */
	if (pn_tfile) {
		assert(pn_tfile->pn_type != VDIR); /* XXX? */
		dtfs_nukenode(pn_tfile, pn_sdir, pcn_targ->pcn_name);
	}

	/* out with the old */
	dtfs_removedent(pn_sdir, dfd_src);
	/* and in with the new */
	dtfs_adddent(pn_tdir, dfd_src);

	/* update name */
	free(dfd_src->dfd_name);
	dfd_src->dfd_name = estrdup(pcn_targ->pcn_name);

	return 0;
}

int
dtfs_link(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node *pn_dir = opc;
	struct dtfs_dirent *dfd;

	dfd = emalloc(sizeof(struct dtfs_dirent));
	dfd->dfd_node = targ;
	dfd->dfd_name = estrdup(pcn->pcn_name);
	dtfs_adddent(pn_dir, dfd);

	return 0;
}

int
dtfs_symlink(struct puffs_usermount *pu, void *opc, void **newnode,
	const struct puffs_cn *pcn_src, const struct vattr *va,
	const char *link_target)
{
	struct puffs_node *pn_parent = opc;
	struct puffs_node *pn_new;
	struct dtfs_file *df_new;

	if (va->va_type != VLNK)
		return ENODEV;

	pn_new = dtfs_genfile(pn_parent, pcn_src->pcn_name, VLNK);
	puffs_setvattr(&pn_new->pn_va, va);
	df_new = DTFS_PTOF(pn_new);
	df_new->df_linktarget = estrdup(link_target);
	pn_new->pn_va.va_size = strlen(df_new->df_linktarget);

	*newnode = pn_new;

	return 0;
}

int
dtfs_readlink(struct puffs_usermount *pu, void *opc,
	const struct puffs_cred *cred, char *link, size_t *linklen)
{
	struct dtfs_file *df = DTFS_CTOF(opc);
	struct puffs_node *pn = opc;

	assert(pn->pn_type == VLNK);
	strlcpy(link, df->df_linktarget, *linklen);
	*linklen = strlen(link);

	return 0;
}

#if 0
int
dtfs_mknod(struct puffs_usermount *pu, void *opc,
	struct puffs_vnreq_mknod *arg)
{
	struct puffs_node *pn_parent = opc;
	struct puffs_node *pn_new;

	assert(arg->pvnr_va.va_type == VBLK || arg->pvnr_va.va_type == VCHR);

	pn_new = dtfs_genfile(pn_parent, arg->pvnr_cn.pcn_name,
	    arg->pvnr_va.va_type);
	puffs_setvattr(&pn_new->pn_va, &arg->pvnr_va);

	arg->pvnr_newnode = pn_new;

	return 0;
}
#endif

/*
 * Read operations (three^Wtwo different ones)
 */
int
dtfs_read(struct puffs_usermount *pu, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr, int ioflag)
{
	struct puffs_node *pn = opc;
	struct dtfs_file *df = DTFS_CTOF(opc);
	quad_t xfer;

	if (pn->pn_va.va_type != VREG)
		return EISDIR;

	xfer = MIN(*resid, df->df_datalen - offset);
	if (xfer < 0)
		return EINVAL;

	memcpy(buf, df->df_data + offset, xfer);
	*resid -= xfer;

	return 0;
}

/*
 * write operations on the wing
 */
int
dtfs_write(struct puffs_usermount *pu, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr, int ioflag)
{
	struct puffs_node *pn = opc;
	struct dtfs_file *df = DTFS_CTOF(opc);

	if (pn->pn_va.va_type != VREG)
		return EISDIR;

	if (ioflag & PUFFS_IO_APPEND)
		offset = pn->pn_va.va_size;

	dtfs_setsize(pn, *resid + offset, 1);
	memcpy(df->df_data + offset, buf, *resid);
	*resid = 0;

	return 0;
}

int
dtfs_reclaim(struct puffs_usermount *pu, void *opc, pid_t pid)
{
	struct puffs_node *pn = opc;

	if (pn->pn_va.va_nlink == 0)
		dtfs_freenode(pn);

	return 0;
}

int
dtfs_inactive(struct puffs_usermount *pu, void *opc, pid_t pid, int *refcount)
{
	struct puffs_node *pn = opc;

	*refcount = pn->pn_va.va_nlink;

	return 0;
}
