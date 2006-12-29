/*	$NetBSD: node.c,v 1.1 2006/12/29 15:35:39 pooka Exp $	*/

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
__RCSID("$NetBSD: node.c,v 1.1 2006/12/29 15:35:39 pooka Exp $");
#endif /* !lint */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "psshfs.h"
#include "sftp_proto.h"

int
psshfs_node_lookup(struct puffs_cc *pcc, void *opc, void **newnode,
	enum vtype *newtype, voff_t *newsize, dev_t *newrdev,
	const struct puffs_cn *pcn)
{
        struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct psshfs_ctx *pctx = pu->pu_privdata;
	struct puffs_node *pn_dir = opc;
	struct psshfs_node *psn_dir = pn_dir->pn_data;
	struct puffs_node *pn;
	struct psshfs_dir *pd;
	int rv;

	if (pcn->pcn_flags & PUFFS_ISDOTDOT) {
		*newnode = psn_dir->parent;
		*newtype = VDIR;
		return 0;
	}

	rv = sftp_readdir(pcc, pctx, pn_dir);
	if (rv) {
		assert(rv != ENOENT);
		return rv;
	}

	pd = lookup(psn_dir->dir, psn_dir->dentnext, pcn->pcn_name);
	if (!pd)
		return ENOENT;

	if (pd->entry)
		pn = pd->entry;
	else
		pn = makenode(pu, pn_dir, pd, &pd->va);

	*newnode = pn;
	*newsize = pn->pn_va.va_size;
	*newtype = pn->pn_va.va_type;

	return 0;
}

int
psshfs_node_getattr(struct puffs_cc *pcc, void *opc, struct vattr *vap,
	const struct puffs_cred *pcr, pid_t pid)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn = opc;
	struct psshfs_node *psn = pn->pn_data;
	struct vattr va;

	rv = 0;

	/* XXX: expire by time */
	if (!psn->hasvattr) {
		psbuf_req_str(pb, SSH_FXP_LSTAT, reqid, pn->pn_path);
		pssh_outbuf_enqueue(pctx, pb, pcc, reqid);
		puffs_cc_yield(pcc);

		rv = psbuf_expect_attrs(pb, &va);
		if (rv)
			goto out;

		puffs_setvattr(&pn->pn_va, &va);
	}

	memcpy(vap, &pn->pn_va, sizeof(struct vattr));
	psn->hasvattr = 1;

 out:
	PSSHFSRETURN(rv);
}

int
psshfs_node_setattr(struct puffs_cc *pcc, void *opc,
	const struct vattr *va, const struct puffs_cred *pcr, pid_t pid)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn = opc;
	struct psshfs_node *psn = pn->pn_data;

	psbuf_req_str(pb, SSH_FXP_SETSTAT, reqid, pn->pn_path);
	psbuf_put_vattr(pb, va);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_expect_status(pb);
	if (rv == 0)
		psn->hasvattr = 0;

	PSSHFSRETURN(rv);
}

int
psshfs_node_create(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn = opc;
	struct puffs_node *pn_new;
	char *fhand = NULL;
	size_t fhandlen;

	pn_new = allocnode(pu, pn, pcn->pcn_name, va);
	if (!pn) {
		rv = ENOMEM;
		goto out;
	}

	psbuf_req_data(pb, SSH_FXP_OPEN, reqid, pcn->pcn_fullpath,
	    strlen(pcn->pcn_fullpath));
	psbuf_put_4(pb, SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC);
	psbuf_put_vattr(pb, va);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_expect_handle(pb, &fhand, &fhandlen);
	if (rv)
		goto out;

	reqid = NEXTREQ(pctx);
	psbuf_recycle(pb, PSB_OUT);
	psbuf_req_data(pb, SSH_FXP_CLOSE, reqid, fhand, fhandlen);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);
	rv = psbuf_expect_status(pb);

	if (rv == 0)
		*newnode = pn_new;
	else
		nukenode(pn_new, pcn->pcn_name);

 out:
	free(fhand);
	PSSHFSRETURN(rv);
}

int
psshfs_node_readdir(struct puffs_cc *pcc, void *opc, struct dirent *dent,
	const struct puffs_cred *pcr, off_t *readoff, size_t *reslen)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct psshfs_ctx *pctx = pu->pu_privdata;
	struct puffs_node *pn = opc;
	struct psshfs_node *psn = pn->pn_data;
	struct psshfs_dir *pd;
	int i, rv;

	rv = sftp_readdir(pcc, pctx, pn);
	if (rv)
		return rv;

	for (i = *readoff; i < psn->dentnext; i++) {
		pd = &psn->dir[i];
		if (pd->valid == 0)
			continue;
		if (!puffs_nextdent(&dent, pd->entryname,
		    pd->va.va_fileid, puffs_vtype2dt(pd->va.va_type), reslen))
			break;
	}

	*readoff = i;
	return 0;
}

int
psshfs_node_read(struct puffs_cc *pcc, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr,
	int ioflag)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn = opc;
	char *fhand = NULL;
	size_t fhandlen;
	struct vattr va;
	uint32_t readlen;

	if (pn->pn_va.va_type == VDIR) {
		rv = EISDIR;
		goto out;
	}

	puffs_vattr_null(&va);
	psbuf_req_str(pb, SSH_FXP_OPEN, reqid, pn->pn_path);
	psbuf_put_4(pb, SSH_FXF_READ);
	psbuf_put_vattr(pb, &va);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_expect_handle(pb, &fhand, &fhandlen);
	if (rv)
		goto out;

	readlen = *resid;
	reqid = NEXTREQ(pctx);
	psbuf_recycle(pb, PSB_OUT);
	psbuf_req_data(pb, SSH_FXP_READ, reqid, fhand, fhandlen);
	psbuf_put_8(pb, offset);
	psbuf_put_4(pb, readlen);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_do_data(pb, buf, &readlen);
	if (rv == 0)
		*resid -= readlen;

	reqid = NEXTREQ(pctx);
	psbuf_recycle(pb, PSB_OUT);
	psbuf_req_data(pb, SSH_FXP_CLOSE, reqid, fhand, fhandlen);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	/* don't care */

 out:
	free(fhand);
	PSSHFSRETURN(rv);
}

/* XXX: we should getattr for size */
int
psshfs_node_write(struct puffs_cc *pcc, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *cred,
	int ioflag)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn = opc;
	char *fhand = NULL;
	size_t fhandlen;
	struct vattr va;
	uint32_t writelen, oflags;

	if (pn->pn_va.va_type == VDIR) {
		rv = EISDIR;
		goto out;
	}

	oflags = SSH_FXF_WRITE;
#if 0
	/*
	 * At least OpenSSH doesn't appear to support this, so can't
	 * do it the right way.
	 */
	if (ioflag & PUFFS_IO_APPEND)
		oflags |= SSH_FXF_APPEND;
#endif
	if (ioflag & PUFFS_IO_APPEND)
		offset = pn->pn_va.va_size;

	puffs_vattr_null(&va);
	va.va_mode = 0666;

	psbuf_req_str(pb, SSH_FXP_OPEN, reqid, pn->pn_path);
	psbuf_put_4(pb, oflags);
	psbuf_put_vattr(pb, &va);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_expect_handle(pb, &fhand, &fhandlen);
	if (rv)
		goto out;

	writelen = *resid;
	reqid = NEXTREQ(pctx);
	psbuf_recycle(pb, PSB_OUT);
	psbuf_req_data(pb, SSH_FXP_WRITE, reqid, fhand, fhandlen);
	psbuf_put_8(pb, offset);
	psbuf_put_data(pb, buf, writelen);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_expect_status(pb);
	if (rv == 0)
		*resid = 0;

	if (pn->pn_va.va_size < offset + writelen)
		pn->pn_va.va_size = offset + writelen;

	reqid = NEXTREQ(pctx);
	psbuf_recycle(pb, PSB_OUT);
	psbuf_req_data(pb, SSH_FXP_CLOSE, reqid, fhand, fhandlen);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	/* dontcare */
 out:
	free(fhand);
	PSSHFSRETURN(rv);
}

int
psshfs_node_readlink(struct puffs_cc *pcc, void *opc,
	const struct puffs_cred *cred, char *linkvalue, size_t *linklen)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn = opc;
	char *linktmp = NULL;
	uint32_t count;

	if (pctx->protover < 3) {
		rv = EOPNOTSUPP;
		goto out;
	}

	psbuf_req_str(pb, SSH_FXP_READLINK, reqid, pn->pn_path);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);
	
	rv = psbuf_expect_name(pb, &count);
	if (count != 1) {
		rv = EPROTO;
		goto out;
	}
	rv = psbuf_get_str(pb, &linktmp, linklen);
	if (rv)
		rv = 0;
	else {
		rv = EPROTO;
		goto out;
	}
	(void) memcpy(linkvalue, linktmp, *linklen);

 out:
	free(linktmp);
	PSSHFSRETURN(rv);
}

int
psshfs_node_remove(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn_targ = targ;

	if (pn_targ->pn_va.va_type == VDIR) {
		rv = EISDIR;
		goto out;
	}

	psbuf_req_str(pb, SSH_FXP_REMOVE, reqid, pn_targ->pn_path);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_expect_status(pb);

	if (rv == 0)
		nukenode(pn_targ, pcn->pcn_name);

 out:
	PSSHFSRETURN(rv);
}

int
psshfs_node_mkdir(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn = opc;
	struct puffs_node *pn_new;

	pn_new = allocnode(pu, pn, pcn->pcn_name, va);
	if (!pn_new) {
		rv = ENOMEM;
		goto out;
	}

	psbuf_req_str(pb, SSH_FXP_MKDIR, reqid, pcn->pcn_fullpath);
	psbuf_put_vattr(pb, va);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_expect_status(pb);

	if (rv == 0)
		*newnode = pn_new;
	else
		nukenode(pn_new, pcn->pcn_name);

 out:
	PSSHFSRETURN(rv);
}

int
psshfs_node_rmdir(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn_targ = targ;

	psbuf_req_str(pb, SSH_FXP_RMDIR, reqid, pn_targ->pn_path);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_expect_status(pb);
	if (rv == 0)
		nukenode(pn_targ, pcn->pcn_name);

	PSSHFSRETURN(rv);
}

int
psshfs_node_symlink(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va,
	const char *link_target)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn = opc;
	struct puffs_node *pn_new;

	if (pctx->protover < 3) {
		rv = EOPNOTSUPP;
		goto out;
	}

	pn_new = allocnode(pu, pn, pcn->pcn_name, va);
	if (!pn) {
		rv = ENOMEM;
		goto out;
	}

	/*
	 * XXX: ietf says: source, target.  openssh says: ietf who?
	 * Let's go with openssh and build quirk tables later if we care
	 */
	psbuf_req_str(pb, SSH_FXP_SYMLINK, reqid, link_target);
	psbuf_put_str(pb, pcn->pcn_fullpath);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_expect_status(pb);
	if (rv == 0)
		*newnode = pn_new;
	else
		nukenode(pn_new, pcn->pcn_name);

 out:
	PSSHFSRETURN(rv);
}

int
psshfs_node_rename(struct puffs_cc *pcc, void *opc, void *src,
	const struct puffs_cn *pcn_src, void *targ_dir, void *targ,
	const struct puffs_cn *pcn_targ)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_node *pn_sf = src;
	struct puffs_node *pn_td = targ_dir, *pn_tf = targ;
	struct psshfs_node *psn_targdir = pn_td->pn_data;

	if (pctx->protover < 2) {
		rv = EOPNOTSUPP;
		goto out;
	}

	if (pn_tf) {
		/* XXX: no backend implementation for now, so call directly */
		rv = psshfs_node_remove(pcc, targ_dir, pn_tf, pcn_targ);
		if (rv)
			goto out;
	}

	psbuf_req_str(pb, SSH_FXP_RENAME, reqid, pcn_src->pcn_fullpath);
	psbuf_put_str(pb, pcn_targ->pcn_fullpath);
	pssh_outbuf_enqueue(pctx, pb, pcc, reqid);

	puffs_cc_yield(pcc);

	rv = psbuf_expect_status(pb);
	if (rv == 0) {
		struct psshfs_dir *pd;

		/*
		 * XXX: interfaces didn't quite work with rename..
		 * the song remains the same.  go figure .. ;)
		 */
		nukenode(pn_sf, pcn_src->pcn_name);
		pd = direnter(pn_td, pcn_targ->pcn_name);
		pd->entry = pn_sf;
		puffs_setvattr(&pd->va, &pn_sf->pn_va);

		if (opc != targ_dir) {
			psn_targdir->childcount++;
			if (pn_sf->pn_va.va_type == VDIR)
				pn_td->pn_va.va_nlink++;
		}
	}

 out:
	PSSHFSRETURN(rv);
}
