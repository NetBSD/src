/*	$NetBSD: node.c,v 1.4 2007/05/05 15:49:51 pooka Exp $	*/

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
__RCSID("$NetBSD: node.c,v 1.4 2007/05/05 15:49:51 pooka Exp $");
#endif /* !lint */

#include <assert.h>
#include <errno.h>
#include <puffs.h>
#include <stdio.h>

#include "ninepuffs.h"
#include "nineproto.h"

static void *
nodecmp(struct puffs_usermount *pu, struct puffs_node *pn, void *arg)
{
	struct vattr *vap = &pn->pn_va;
	struct qid9p *qid = arg;

	if (vap->va_fileid == qid->qidpath)
		return pn;

	return NULL;
}

int
puffs9p_node_lookup(struct puffs_cc *pcc, void *opc, void **newnode,
	enum vtype *newtype, voff_t *newsize, dev_t *newrdev,
	const struct puffs_cn *pcn)
{
	AUTOVAR(pcc);
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn, *pn_dir = opc;
	struct p9pnode *p9n_dir = pn_dir->pn_data;
	p9ptag_t tfid = NEXTFID(p9p);
	struct qid9p newqid;
	uint16_t nqid;

	p9pbuf_put_1(pb, P9PROTO_T_WALK);
	p9pbuf_put_2(pb, tag);
	p9pbuf_put_4(pb, p9n_dir->fid_base);
	p9pbuf_put_4(pb, tfid);
	p9pbuf_put_2(pb, 1);
	p9pbuf_put_str(pb, pcn->pcn_name);
	puffs_framebuf_enqueue_cc(pcc, pb);

	rv = proto_expect_walk_nqids(pb, &nqid);
	if (rv) {
		rv = ENOENT;
		goto out;
	}
	if (nqid != 1) {
		rv = EPROTO;
		goto out;
	}
	if ((rv = proto_getqid(pb, &newqid)))
		goto out;

	pn = puffs_pn_nodewalk(pu, nodecmp, &newqid);
	if (pn == NULL)
		pn = newp9pnode_qid(pu, &newqid, tfid);
	else
		proto_cc_clunkfid(pcc, tfid, 0);
		
	*newnode = pn;
	*newtype = pn->pn_va.va_type;
	*newsize = pn->pn_va.va_size;
	*newrdev = pn->pn_va.va_rdev;

 out:
	RETURN(rv);
}

/*
 * Problem is that 9P doesn't allow seeking into a directory.  So we
 * maintain a list of active fids for any given directory.  They
 * start living at the first read and exist either until the directory
 * is closed or until they reach the end.
 */
int
puffs9p_node_readdir(struct puffs_cc *pcc, void *opc, struct dirent *dent,
	off_t *readoff, size_t *reslen, const struct puffs_cred *pcr,
	int *eofflag, off_t *cookies, size_t *ncookies)
{
	AUTOVAR(pcc);
	struct puffs_node *pn = opc;
	struct p9pnode *p9n = pn->pn_data;
	struct vattr va;
	struct dirfid *dfp;
	char *name;
	uint32_t count;
	uint16_t statsize;

	rv = getdfwithoffset(pcc, p9n, *readoff, &dfp);
	if (rv)
		goto out;

	tag = NEXTTAG(p9p);
	p9pbuf_put_1(pb, P9PROTO_T_READ);
	p9pbuf_put_2(pb, tag);
	p9pbuf_put_4(pb, dfp->fid);
	p9pbuf_put_8(pb, *readoff);
	p9pbuf_put_4(pb, *reslen); /* XXX */
	puffs_framebuf_enqueue_cc(pcc, pb);

	p9pbuf_get_4(pb, &count);

	/*
	 * if count is 0, assume we at end-of-dir.  dfp is no longer
	 * useful, so nuke it
	 */
	if (count == 0) {
		*eofflag = 1;
		releasedf(pcc, dfp);
		goto out;
	}

	while (count > 0) {
		if ((rv = proto_getstat(pb, &va, &name, &statsize)))
			goto out;

		puffs_nextdent(&dent, name, va.va_fileid,
		    puffs_vtype2dt(va.va_type), reslen);

		count -= statsize;
		*readoff += statsize;
		dfp->seekoff += statsize;
	}

	storedf(p9n, dfp);

 out:
	RETURN(rv);
}

int
puffs9p_node_getattr(struct puffs_cc *pcc, void *opc, struct vattr *vap,
	const struct puffs_cred *pcr, pid_t pid)
{
	AUTOVAR(pcc);
	struct puffs_node *pn = opc;
	struct p9pnode *p9n = pn->pn_data;

	p9pbuf_put_1(pb, P9PROTO_T_STAT);
	p9pbuf_put_2(pb, tag);
	p9pbuf_put_4(pb, p9n->fid_base);
	puffs_framebuf_enqueue_cc(pcc, pb);

	rv = proto_expect_stat(pb, &pn->pn_va);
	if (rv)
		goto out;

	memcpy(vap, &pn->pn_va, sizeof(struct vattr));

 out:
	RETURN(rv);
}

int
puffs9p_node_setattr(struct puffs_cc *pcc, void *opc,
	const struct vattr *va, const struct puffs_cred *pcr, pid_t pid)
{
	AUTOVAR(pcc);
	struct puffs_node *pn = opc;
	struct p9pnode *p9n = pn->pn_data;

	p9pbuf_put_1(pb, P9PROTO_T_WSTAT);
	p9pbuf_put_2(pb, tag);
	p9pbuf_put_4(pb, p9n->fid_base);
	proto_make_stat(pb, va, NULL);
	puffs_framebuf_enqueue_cc(pcc, pb);

	if (p9pbuf_get_type(pb) != P9PROTO_R_WSTAT)
		rv = EPROTO;

	RETURN(rv);
}

/*
 * Ok, time to get clever.  There are two possible cases: we are
 * opening a file or we are opening a directory.
 *
 * If it's a directory, don't bother opening it here, but rather
 * wait until readdir, since it's probable we need to be able to
 * open a directory there in any case.
 * 
 * If it's a regular file, open it here with whatever credentials
 * we happen to have.   Let the upper layers of the kernel worry
 * about permission control.
 *
 * XXX: this does not work fully for the mmap case
 */
int
puffs9p_node_open(struct puffs_cc *pcc, void *opc, int mode,
	const struct puffs_cred *pcr, pid_t pid)
{
	struct puffs9p *p9p = puffs_cc_getspecific(pcc);
	struct puffs_node *pn = opc;
	struct p9pnode *p9n = pn->pn_data;
	p9pfid_t nfid;
	int error = 0;

	p9n->opencount++;
	if (pn->pn_va.va_type != VDIR) {
		if (mode & FREAD && p9n->fid_read == P9P_INVALFID) {
			nfid = NEXTFID(p9p);
			error = proto_cc_open(pcc, p9n->fid_base, nfid,
			    P9PROTO_OMODE_READ);
			if (error)
				return error;
			p9n->fid_read = nfid;
		}
		if (mode & FWRITE && p9n->fid_write == P9P_INVALFID) {
			nfid = NEXTFID(p9p);
			error = proto_cc_open(pcc, p9n->fid_base, nfid,
			    P9PROTO_OMODE_WRITE);
			if (error)
				return error;
			p9n->fid_write = nfid;
		}
	}

	return 0;
}

int
puffs9p_node_close(struct puffs_cc *pcc, void *opc, int flags,
	const struct puffs_cred *pcr, pid_t pid)
{
	struct puffs_node *pn = opc;
	struct p9pnode *p9n = pn->pn_data;

	if (--p9n->opencount == 0) {
		if (pn->pn_va.va_type == VDIR) {
			nukealldf(pcc, p9n);
		} else  {
			if (p9n->fid_read != P9P_INVALFID) {
				proto_cc_clunkfid(pcc, p9n->fid_read, 0);
				p9n->fid_read = P9P_INVALFID;
			}
			if (p9n->fid_write != P9P_INVALFID) {
				proto_cc_clunkfid(pcc, p9n->fid_write, 0);
				p9n->fid_write = P9P_INVALFID;
			}
		}
	}

	return 0;
}

int
puffs9p_node_read(struct puffs_cc *pcc, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr,
	int ioflag)
{
	AUTOVAR(pcc);
	struct puffs_node *pn = opc;
	struct p9pnode *p9n = pn->pn_data;
	uint32_t count;
	size_t nread;

	nread = 0;
	while (*resid > 0) {
		p9pbuf_put_1(pb, P9PROTO_T_READ);
		p9pbuf_put_2(pb, tag);
		p9pbuf_put_4(pb, p9n->fid_read);
		p9pbuf_put_8(pb, offset+nread);
		p9pbuf_put_4(pb, MIN((uint32_t)*resid,p9p->maxreq-24));
		puffs_framebuf_enqueue_cc(pcc, pb);

		if (p9pbuf_get_type(pb) != P9PROTO_R_READ) {
			rv = EPROTO;
			break;
		}

		p9pbuf_get_4(pb, &count);
		if ((rv = p9pbuf_read_data(pb, buf + nread, count)))
			break;

		*resid -= count;
		nread += count;

		p9pbuf_recycleout(pb);
	}
			
	RETURN(rv);
}

int
puffs9p_node_write(struct puffs_cc *pcc, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *cred,
	int ioflag)
{
	AUTOVAR(pcc);
	struct puffs_node *pn = opc;
	struct p9pnode *p9n = pn->pn_data;
	uint32_t chunk, count;
	size_t nwrite;

	if (ioflag & PUFFS_IO_APPEND)
		offset = pn->pn_va.va_size;

	nwrite = 0;
	while (*resid > 0) {
		chunk = MIN(*resid, p9p->maxreq-32);

		p9pbuf_put_1(pb, P9PROTO_T_WRITE);
		p9pbuf_put_2(pb, tag);
		p9pbuf_put_4(pb, p9n->fid_write);
		p9pbuf_put_8(pb, offset+nwrite);
		p9pbuf_put_4(pb, chunk);
		p9pbuf_write_data(pb, buf+nwrite, chunk);
		puffs_framebuf_enqueue_cc(pcc, pb);

		if (p9pbuf_get_type(pb) != P9PROTO_R_WRITE) {
			rv = EPROTO;
			break;
		}

		p9pbuf_get_4(pb, &count);
		*resid -= count;
		nwrite += count;

		if (count != chunk) {
			rv = EPROTO;
			break;
		}

		p9pbuf_recycleout(pb);
	}
			
	RETURN(rv);
}

static int
nodecreate(struct puffs_cc *pcc, struct puffs_node *pn, void **newnode,
	const char *name, const struct vattr *vap, uint32_t dirbit)
{
	AUTOVAR(pcc);
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn_new;
	struct p9pnode *p9n = pn->pn_data;
	p9pfid_t nfid = NEXTFID(p9p);
	struct qid9p nqid;
	int tries = 0;

 again:
	if (++tries > 5) {
		rv = EPROTO;
		goto out;
	}

	rv = proto_cc_dupfid(pcc, p9n->fid_base, nfid);
	if (rv)
		goto out;

	p9pbuf_put_1(pb, P9PROTO_T_CREATE);
	p9pbuf_put_2(pb, tag);
	p9pbuf_put_4(pb, nfid);
	p9pbuf_put_str(pb, name);
	p9pbuf_put_4(pb, dirbit | (vap->va_mode & 0777));
	p9pbuf_put_1(pb, 0);
	puffs_framebuf_enqueue_cc(pcc, pb);

	rv = proto_expect_qid(pb, P9PROTO_R_CREATE, &nqid);
	if (rv)
		goto out;

	/*
	 * Now, little problem here: create returns an *open* fid.
	 * So, clunk it and walk the parent directory to get a fid
	 * which is not open for I/O yet.
	 */
	proto_cc_clunkfid(pcc, nfid, 0);
	nfid = NEXTFID(p9p);

	p9pbuf_recycleout(pb);
	p9pbuf_put_1(pb, P9PROTO_T_WALK);
	p9pbuf_put_2(pb, tag);
	p9pbuf_put_4(pb, p9n->fid_base);
	p9pbuf_put_4(pb, nfid);
	p9pbuf_put_2(pb, 1);
	p9pbuf_put_str(pb, name);
	puffs_framebuf_enqueue_cc(pcc, pb);

	/*
	 * someone removed it already? try again
	 * note: this is kind of lose/lose
	 */
	if (p9pbuf_get_type(pb) != P9PROTO_R_WALK)
		goto again;

	pn_new = newp9pnode_va(pu, vap, nfid);
	qid2vattr(&pn_new->pn_va, &nqid);
	*newnode = pn_new;

 out:
	RETURN(rv);
}

int
puffs9p_node_create(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{

	return nodecreate(pcc, opc, newnode, pcn->pcn_name, va, 0);
}

int
puffs9p_node_mkdir(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{

	return nodecreate(pcc, opc, newnode, pcn->pcn_name,
	    va, P9PROTO_CPERM_DIR);
}

/*
 * Need to be a bit clever again: the fid is clunked no matter if
 * the remove succeeds or not.  Re-getting a fid would be way too
 * difficult in case the remove failed for a valid reason (directory
 * not empty etcetc.).  So walk ourselves another fid to prod the
 * ice with.
 */
static int
noderemove(struct puffs_cc *pcc, struct p9pnode *p9n)
{
	AUTOVAR(pcc);
	p9pfid_t testfid = NEXTFID(p9p);

	rv = proto_cc_dupfid(pcc, p9n->fid_base, testfid);

	p9pbuf_put_1(pb, P9PROTO_T_REMOVE);
	p9pbuf_put_2(pb, tag);
	p9pbuf_put_4(pb, testfid);
	puffs_framebuf_enqueue_cc(pcc, pb);

	if (p9pbuf_get_type(pb) != P9PROTO_R_REMOVE) {
		rv = EPROTO;
	} else {
		proto_cc_clunkfid(pcc, p9n->fid_base, 0);
		p9n->fid_base = P9P_INVALFID;
	}

	RETURN(rv);
}

int
puffs9p_node_remove(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node *pn = targ;

	if (pn->pn_va.va_type == VDIR)
		return EISDIR;

	return noderemove(pcc, pn->pn_data);
}

int
puffs9p_node_rmdir(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node *pn = targ;

	if (pn->pn_va.va_type != VDIR)
		return ENOTDIR;

	return noderemove(pcc, pn->pn_data);
}

/*
 * 9P supports renames only for regular files within a directory
 * from what I could tell.  So just support in-directory renames
 * for now.
 */ 
int
puffs9p_node_rename(struct puffs_cc *pcc, void *opc, void *src,
	const struct puffs_cn *pcn_src, void *targ_dir, void *targ,
	const struct puffs_cn *pcn_targ)
{
	AUTOVAR(pcc);
	struct puffs_node *pn_src = src;
	struct p9pnode *p9n_src = pn_src->pn_data;

	if (opc != targ_dir) {
		rv = EOPNOTSUPP;
		goto out;
	}

	/* 9P doesn't allow to overwrite in rename */
	if (targ) {
		struct puffs_node *pn_targ = targ;

		rv = noderemove(pcc, pn_targ->pn_data);
		if (rv)
			goto out;
	}

	p9pbuf_put_1(pb, P9PROTO_T_WSTAT);
	p9pbuf_put_2(pb, tag);
	p9pbuf_put_4(pb, p9n_src->fid_base);
	proto_make_stat(pb, NULL, pcn_targ->pcn_name);
	puffs_framebuf_enqueue_cc(pcc, pb);

	if (p9pbuf_get_type(pb) != P9PROTO_R_WSTAT)
		rv = EPROTO;

 out:
	RETURN(rv);
}

/*
 * - "here's one"
 * - "9P"
 * ~ "i'm not dead"
 * - "you're not fooling anyone you know, you'll be stone dead in a minute
 * - "he says he's not quite dead"
 * - "isn't there anything you could do?"
 * - *clunk*!
 * - "thanks"
 */
int
puffs9p_node_reclaim(struct puffs_cc *pcc, void *opc, pid_t pid)
{
#if 0
	if (p9n->fid_open != P9P_INVALFID)
		proto_cc_clunkfid(pcc, p9n->fid_open, 0);
#endif

	return 0;
}
