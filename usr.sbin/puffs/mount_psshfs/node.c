/*	$NetBSD: node.c,v 1.35 2007/07/19 10:14:53 pooka Exp $	*/

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
__RCSID("$NetBSD: node.c,v 1.35 2007/07/19 10:14:53 pooka Exp $");
#endif /* !lint */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "psshfs.h"
#include "sftp_proto.h"

int
psshfs_node_lookup(struct puffs_cc *pcc, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn)
{
        struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct psshfs_ctx *pctx = puffs_getspecific(pu);
	struct puffs_node *pn_dir = opc;
	struct psshfs_node *psn, *psn_dir = pn_dir->pn_data;
	struct puffs_node *pn;
	struct psshfs_dir *pd;
	struct vattr va;
	int rv;

	if (PCNISDOTDOT(pcn)) {
		psn = psn_dir->parent->pn_data;
		psn->stat &= ~PSN_RECLAIMED;

		puffs_newinfo_setcookie(pni, psn_dir->parent);
		puffs_newinfo_setvtype(pni, VDIR);
		return 0;
	}

	rv = sftp_readdir(pcc, pctx, pn_dir);
	if (rv) {
		if (rv != EPERM)
			return rv;

		/*
		 * Can't read the directory.  We still might be
		 * able to find the node with getattr in -r+x dirs
		 */
		rv = getpathattr(pcc, PCNPATH(pcn), &va);
		if (rv)
			return rv;

		/* guess */
		if (va.va_type == VDIR)
			va.va_nlink = 2;
		else
			va.va_nlink = 1;

		pn = allocnode(pu, pn_dir, pcn->pcn_name, &va);
		psn = pn->pn_data;
		psn->attrread = time(NULL);
	} else {
		pd = lookup(psn_dir->dir, psn_dir->dentnext, pcn->pcn_name);
		if (!pd) {
			return ENOENT;
		}

		if (pd->entry)
			pn = pd->entry;
		else
			pn = makenode(pu, pn_dir, pd, &pd->va);
		psn = pn->pn_data;
	}

	psn->stat &= ~PSN_RECLAIMED;

	puffs_newinfo_setcookie(pni, pn);
	puffs_newinfo_setvtype(pni, pn->pn_va.va_type);
	puffs_newinfo_setsize(pni, pn->pn_va.va_size);

	return 0;
}

int
psshfs_node_getattr(struct puffs_cc *pcc, void *opc, struct vattr *vap,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{
	struct puffs_node *pn = opc;
	int rv;

	rv = getnodeattr(pcc, pn);
	if (rv)
		return rv;

	memcpy(vap, &pn->pn_va, sizeof(struct vattr));

	return 0;
}

int
psshfs_node_setattr(struct puffs_cc *pcc, void *opc,
	const struct vattr *va, const struct puffs_cred *pcr,
	const struct puffs_cid *pcid)
{
	PSSHFSAUTOVAR(pcc);
	struct vattr kludgeva;
	struct puffs_node *pn = opc;

	psbuf_req_str(pb, SSH_FXP_SETSTAT, reqid, PNPATH(pn));

	memcpy(&kludgeva, va, sizeof(struct vattr));

	/* XXX: kludge due to openssh server implementation */
	if (va->va_atime.tv_sec != PUFFS_VNOVAL
	    && va->va_mtime.tv_sec == PUFFS_VNOVAL) {
		if (pn->pn_va.va_mtime.tv_sec != PUFFS_VNOVAL)
			kludgeva.va_mtime.tv_sec = pn->pn_va.va_mtime.tv_sec;
		else
			kludgeva.va_mtime.tv_sec = va->va_atime.tv_sec;
	}
	if (va->va_mtime.tv_sec != PUFFS_VNOVAL
	    && va->va_atime.tv_sec == PUFFS_VNOVAL) {
		if (pn->pn_va.va_atime.tv_sec != PUFFS_VNOVAL)
			kludgeva.va_atime.tv_sec = pn->pn_va.va_atime.tv_sec;
		else
			kludgeva.va_atime.tv_sec = va->va_mtime.tv_sec;
	}
			
	psbuf_put_vattr(pb, &kludgeva);
	GETRESPONSE(pb);

	rv = psbuf_expect_status(pb);
	if (rv == 0)
		puffs_setvattr(&pn->pn_va, &kludgeva);

 out:
	PSSHFSRETURN(rv);
}

int
psshfs_node_create(struct puffs_cc *pcc, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn = opc;
	struct puffs_node *pn_new;
	char *fhand = NULL;
	uint32_t fhandlen;

	pn_new = allocnode(pu, pn, pcn->pcn_name, va);
	if (!pn_new) {
		rv = ENOMEM;
		goto out;
	}

	psbuf_req_str(pb, SSH_FXP_OPEN, reqid, PCNPATH(pcn));
	psbuf_put_4(pb, SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC);
	psbuf_put_vattr(pb, va);
	GETRESPONSE(pb);

	rv = psbuf_expect_handle(pb, &fhand, &fhandlen);
	if (rv == 0)
		puffs_newinfo_setcookie(pni, pn_new);
	else
		goto out;

	reqid = NEXTREQ(pctx);
	psbuf_recycleout(pb);
	psbuf_req_data(pb, SSH_FXP_CLOSE, reqid, fhand, fhandlen);
	JUSTSEND(pb);
	free(fhand);
	return 0;

 out:
	free(fhand);
	PSSHFSRETURN(rv);
}

int
psshfs_node_open(struct puffs_cc *pcc, void *opc, int mode,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{
	PSSHFSAUTOVAR(pcc);
	struct vattr va;
	struct puffs_node *pn = opc;
	struct psshfs_node *psn = pn->pn_data;

	if (pn->pn_va.va_type == VDIR)
		goto out;

	puffs_setback(pcc, PUFFS_SETBACK_INACT_N1);
	puffs_vattr_null(&va);
	if (mode & FREAD && psn->fhand_r == NULL) {
		psbuf_req_str(pb, SSH_FXP_OPEN, reqid, PNPATH(pn));
		psbuf_put_4(pb, SSH_FXF_READ);
		psbuf_put_vattr(pb, &va);
		GETRESPONSE(pb);

		rv = psbuf_expect_handle(pb, &psn->fhand_r, &psn->fhand_r_len);
		if (rv)
			goto out;
		psbuf_recycleout(pb);
	}
	if (mode & FWRITE && psn->fhand_w == NULL) {
		psbuf_req_str(pb, SSH_FXP_OPEN, reqid, PNPATH(pn));
		psbuf_put_4(pb, SSH_FXF_WRITE);
		psbuf_put_vattr(pb, &va);
		GETRESPONSE(pb);

		rv = psbuf_expect_handle(pb, &psn->fhand_w, &psn->fhand_w_len);
		if (rv)
			goto out;
	}

 out:
	PSSHFSRETURN(rv);
}

int
psshfs_node_inactive(struct puffs_cc *pcc, void *opc,
	const struct puffs_cid *pcid)
{
	struct psshfs_ctx *pctx = puffs_cc_getspecific(pcc);
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn = opc;
	struct psshfs_node *psn = pn->pn_data;
	uint32_t reqid = NEXTREQ(pctx);
	struct puffs_framebuf *pb1, *pb2;
	int rv;

	if (psn->fhand_r) {
		pb1 = psbuf_makeout();
		psbuf_req_data(pb1, SSH_FXP_CLOSE, reqid,
		    psn->fhand_r, psn->fhand_r_len);
		JUSTSEND(pb1);
		free(psn->fhand_r);
		psn->fhand_r = NULL;
	}
	if (psn->fhand_w) {
		pb2 = psbuf_makeout();
		psbuf_req_data(pb2, SSH_FXP_CLOSE, reqid,
		    psn->fhand_w, psn->fhand_w_len);
		JUSTSEND(pb2);
		free(psn->fhand_w);
		psn->fhand_w = NULL;
	}

 out:
	return 0;
}

int
psshfs_node_readdir(struct puffs_cc *pcc, void *opc, struct dirent *dent,
	off_t *readoff, size_t *reslen, const struct puffs_cred *pcr,
	int *eofflag, off_t *cookies, size_t *ncookies)
{
	struct psshfs_ctx *pctx = puffs_cc_getspecific(pcc);
	struct puffs_node *pn = opc;
	struct psshfs_node *psn = pn->pn_data;
	struct psshfs_dir *pd;
	int i, rv;

	*ncookies = 0;
	rv = sftp_readdir(pcc, pctx, pn);
	if (rv)
		return rv;

	/* find next dirent */
	for (i = *readoff;;i++) {
		if (i == psn->dentnext)
			goto out;
		pd = &psn->dir[i];
		if (pd->valid)
			break;
	}

	for (;;) {
		*readoff = i;
		if (!puffs_nextdent(&dent, pd->entryname,
		    pd->va.va_fileid, puffs_vtype2dt(pd->va.va_type), reslen))
			return 0;

		/* find next entry, store possible nfs key */
		do {
			if (++i == psn->dentnext)
				goto out;
			pd = &psn->dir[i];
		} while (pd->valid == 0);
		PUFFS_STORE_DCOOKIE(cookies, ncookies, (off_t)i);
	}

 out:
	if (i == psn->dentnext)
		*eofflag = 1;

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
	struct psshfs_node *psn = pn->pn_data;
	uint32_t readlen;

	if (pn->pn_va.va_type == VDIR) {
		rv = EISDIR;
		goto out;
	}

	readlen = *resid;
	psbuf_req_data(pb, SSH_FXP_READ, reqid, psn->fhand_r, psn->fhand_r_len);
	psbuf_put_8(pb, offset);
	psbuf_put_4(pb, readlen);
	GETRESPONSE(pb);

	rv = psbuf_do_data(pb, buf, &readlen);
	if (rv == 0)
		*resid -= readlen;

 out:
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
	struct psshfs_node *psn = pn->pn_data;
	uint32_t writelen;

	if (pn->pn_va.va_type == VDIR) {
		rv = EISDIR;
		goto out;
	}

	writelen = *resid;
	psbuf_req_data(pb, SSH_FXP_WRITE, reqid, psn->fhand_w,psn->fhand_w_len);
	psbuf_put_8(pb, offset);
	psbuf_put_data(pb, buf, writelen);
	GETRESPONSE(pb);

	rv = psbuf_expect_status(pb);
	if (rv == 0)
		*resid = 0;

	if (pn->pn_va.va_size < offset + writelen)
		pn->pn_va.va_size = offset + writelen;

 out:
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

	psbuf_req_str(pb, SSH_FXP_READLINK, reqid, PNPATH(pn));
	GETRESPONSE(pb);

	rv = psbuf_expect_name(pb, &count);
	if (rv)
		goto out;
	if (count != 1) {
		rv = EPROTO;
		goto out;
	}

	rv = psbuf_get_str(pb, &linktmp, (uint32_t *)linklen);
	if (rv)
		goto out;
	(void) memcpy(linkvalue, linktmp, *linklen);

 out:
	free(linktmp);
	PSSHFSRETURN(rv);
}

static int
doremove(struct puffs_cc *pcc, struct puffs_node *pn_dir,
	struct puffs_node *pn, const char *name)
{
	PSSHFSAUTOVAR(pcc);
	int op;

	if (pn->pn_va.va_type == VDIR)
		op = SSH_FXP_RMDIR;
	else
		op = SSH_FXP_REMOVE;

	psbuf_req_str(pb, op, reqid, PNPATH(pn));
	GETRESPONSE(pb);

	rv = psbuf_expect_status(pb);
	if (rv == 0)
		nukenode(pn, name, 0);

 out:
	PSSHFSRETURN(rv);
}

int
psshfs_node_remove(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node *pn_targ = targ;
	int rv;

	assert(pn_targ->pn_va.va_type != VDIR);

	rv = doremove(pcc, opc, targ, pcn->pcn_name);
	if (rv == 0)
		puffs_setback(pcc, PUFFS_SETBACK_NOREF_N2);

	return rv;
}

int
psshfs_node_rmdir(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node *pn_targ = targ;
	int rv;

	assert(pn_targ->pn_va.va_type == VDIR);

	rv = doremove(pcc, opc, targ, pcn->pcn_name);
	if (rv == 0)
		puffs_setback(pcc, PUFFS_SETBACK_NOREF_N2);

	return rv;
}

int
psshfs_node_mkdir(struct puffs_cc *pcc, void *opc,  struct puffs_newinfo *pni,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn = opc;
	struct puffs_node *pn_new;

	pn_new = allocnode(pu, pn, pcn->pcn_name, va);
	if (!pn_new) {
		rv = ENOMEM;
		goto out;
	}

	psbuf_req_str(pb, SSH_FXP_MKDIR, reqid, PCNPATH(pcn));
	psbuf_put_vattr(pb, va);
	GETRESPONSE(pb);

	rv = psbuf_expect_status(pb);

	if (rv == 0)
		puffs_newinfo_setcookie(pni, pn_new);
	else
		nukenode(pn_new, pcn->pcn_name, 1);

 out:
	PSSHFSRETURN(rv);
}

int
psshfs_node_symlink(struct puffs_cc *pcc, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn, const struct vattr *va,
	const char *link_target)
{
	PSSHFSAUTOVAR(pcc);
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn = opc;
	struct puffs_node *pn_new;

	if (pctx->protover < 3) {
		rv = EOPNOTSUPP;
		goto out;
	}

	pn_new = allocnode(pu, pn, pcn->pcn_name, va);
	if (!pn_new) {
		rv = ENOMEM;
		goto out;
	}

	/*
	 * XXX: ietf says: source, target.  openssh says: ietf who?
	 * Let's go with openssh and build quirk tables later if we care
	 */
	psbuf_req_str(pb, SSH_FXP_SYMLINK, reqid, link_target);
	psbuf_put_str(pb, PCNPATH(pcn));
	GETRESPONSE(pb);

	rv = psbuf_expect_status(pb);
	if (rv == 0)
		puffs_newinfo_setcookie(pni, pn_new);
	else
		nukenode(pn_new, pcn->pcn_name, 1);

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
		rv = doremove(pcc, targ_dir, pn_tf, pcn_targ->pcn_name);
		if (rv)
			goto out;
	}

	psbuf_req_str(pb, SSH_FXP_RENAME, reqid, PCNPATH(pcn_src));
	psbuf_put_str(pb, PCNPATH(pcn_targ));
	GETRESPONSE(pb);

	rv = psbuf_expect_status(pb);
	if (rv == 0) {
		struct psshfs_dir *pd;

		/*
		 * XXX: interfaces didn't quite work with rename..
		 * the song remains the same.  go figure .. ;)
		 */
		nukenode(pn_sf, pcn_src->pcn_name, 0);
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

/*
 * So this file system happened to be written in such a way that
 * lookup for ".." is hard if we lose the in-memory node.  We'd
 * need to recreate the entire directory structure from the root
 * node up to the ".." node we're looking up.
 *
 * And since our entire fs structure is purely fictional (i.e. it's
 * only in-memory, not fetchable from the server), the easiest way
 * to deal with it is to not allow nodes with children to be
 * reclaimed.
 *
 * If a node with children is being attempted to be reclaimed, we
 * just mark it "reclaimed" but leave it as is until all its children
 * have been reclaimed.  If a lookup for that node is done meanwhile,
 * it will be found by lookup() and we just remove the "reclaimed"
 * bit.
 */
int
psshfs_node_reclaim(struct puffs_cc *pcc, void *opc,
	const struct puffs_cid *pcid)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn = opc, *pn_next, *pn_root;
	struct psshfs_node *psn = pn->pn_data;

	/*
	 * don't reclaim if we have file handle issued, otherwise
	 * we can't do fhtonode
	 */
	if (psn->stat & PSN_HASFH)
		return 0;

	psn->stat |= PSN_RECLAIMED;
	pn_root = puffs_getroot(pu);
	for (; pn != pn_root; pn = pn_next) {
		psn = pn->pn_data;
		if ((psn->stat & PSN_RECLAIMED) == 0 || psn->childcount != 0)
			break;

		pn_next = psn->parent;
		doreclaim(pn);
	}

	return 0;
}
