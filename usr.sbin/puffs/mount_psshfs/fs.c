/*	$NetBSD: fs.c,v 1.10 2007/05/18 16:13:47 pooka Exp $	*/

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
__RCSID("$NetBSD: fs.c,v 1.10 2007/05/18 16:13:47 pooka Exp $");
#endif /* !lint */

#include <err.h>
#include <errno.h>
#include <puffs.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "psshfs.h"
#include "sftp_proto.h"

#define DO_IO(fname, a1, a2, a3, a4, rv)				\
	puffs_framebuf_seekset(a2, 0);					\
	*(a4) = 0;							\
	rv = fname(a1, a2, a3, a4);					\
	if (rv || a4 == 0) errx(1, "psshfs_domount io failed %d, %d", rv, *a4) 

int
psshfs_domount(struct puffs_usermount *pu)
{
	struct psshfs_ctx *pctx = puffs_getspecific(pu);
	struct psshfs_node *root = &pctx->psn_root;
	struct puffs_pathobj *po_root;
	struct puffs_node *pn_root;
	struct vattr va;
	struct vattr *rva;
	struct puffs_framebuf *pb;
	char *rootpath;
	uint32_t count;
	int rv, done;

	pb = psbuf_makeout();
	psbuf_put_1(pb, SSH_FXP_INIT);
	psbuf_put_4(pb, SFTP_PROTOVERSION);
	DO_IO(psbuf_write, pu, pb, pctx->sshfd, &done, rv);

	puffs_framebuf_recycle(pb);
	DO_IO(psbuf_read, pu, pb, pctx->sshfd, &done, rv);
	if (psbuf_get_type(pb) != SSH_FXP_VERSION)
		errx(1, "invalid server response: %d", psbuf_get_type(pb));
	pctx->protover = psbuf_get_reqid(pb);
	/* might contain some other stuff, but we're not interested */

	/* scope out our rootpath */
	psbuf_recycleout(pb);
	psbuf_put_1(pb, SSH_FXP_REALPATH);
	psbuf_put_4(pb, NEXTREQ(pctx));
	psbuf_put_str(pb, pctx->mountpath);
	DO_IO(psbuf_write, pu, pb, pctx->sshfd, &done, rv);

	puffs_framebuf_recycle(pb);
	DO_IO(psbuf_read, pu, pb, pctx->sshfd, &done, rv);
	if (psbuf_get_type(pb) != SSH_FXP_NAME)
		errx(1, "invalid server realpath response for \"%s\"",
		    pctx->mountpath);
	if (psbuf_get_4(pb, &count) == -1)
		errx(1, "invalid realpath response: count");
	if (psbuf_get_str(pb, &rootpath, NULL) == -1)
		errx(1, "invalid realpath response: rootpath");

	/* stat the rootdir so that we know it's a dir */
	psbuf_recycleout(pb);
	psbuf_req_str(pb, SSH_FXP_LSTAT, NEXTREQ(pctx), rootpath);
	DO_IO(psbuf_write, pu, pb, pctx->sshfd, &done, rv);

	puffs_framebuf_recycle(pb);
	DO_IO(psbuf_read, pu, pb, pctx->sshfd, &done, rv);

	rv = psbuf_expect_attrs(pb, &va);
	if (rv)
		errx(1, "couldn't stat rootpath");
	puffs_framebuf_destroy(pb);

	if (puffs_mode2vt(va.va_mode) != VDIR)
		errx(1, "remote path (%s) not a directory", rootpath);

	pctx->nextino = 2;

	memset(root, 0, sizeof(struct psshfs_node));
	pn_root = puffs_pn_new(pu, root);
	if (pn_root == NULL)
		return errno;
	puffs_setroot(pu, pn_root);

	po_root = puffs_getrootpathobj(pu);
	if (po_root == NULL)
		err(1, "getrootpathobj");
	po_root->po_path = rootpath;
	po_root->po_len = strlen(rootpath);

	rva = &pn_root->pn_va;
	puffs_setvattr(rva, &va);
	rva->va_fileid = pctx->nextino++;
	rva->va_nlink = 0156; /* XXX */

	return 0;
}

int
psshfs_fs_unmount(struct puffs_cc *pcc, int flags, pid_t pid)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct psshfs_ctx *pctx = puffs_getspecific(pu);

	kill(pctx->sshpid, SIGTERM);
	close(pctx->sshfd);
	return 0;
}

int
psshfs_fs_nodetofh(struct puffs_cc *pcc, void *cookie,
	void *fid, size_t *fidsize)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct psshfs_ctx *pctx = puffs_getspecific(pu);
	struct puffs_node *pn = cookie;
	struct psshfs_node *psn = pn->pn_data;
	struct psshfs_fid *pf = fid;

	pf->mounttime = pctx->mounttime;
	pf->node = pn;

	psn->stat |= PSN_HASFH;

	return 0;
}

int
psshfs_fs_fhtonode(struct puffs_cc *pcc, void *fid, size_t fidsize,
	void **fcookie, enum vtype *ftype, voff_t *fsize, dev_t *fdev)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct psshfs_ctx *pctx = puffs_getspecific(pu);
	struct psshfs_fid *pf = fid;
	struct puffs_node *pn = pf->node;
	struct psshfs_node *psn;
	int rv;

	if (pf->mounttime != pctx->mounttime)
		return EINVAL;
	if (pn == 0)
		return EINVAL;
	psn = pn->pn_data;
	if ((psn->stat & PSN_HASFH) == 0)
		return EINVAL;

	/* update node attributes */
	rv = getnodeattr(pcc, pn);
	if (rv)
		return EINVAL;

	*fcookie = pn;
	*ftype = pn->pn_va.va_type;
	*fsize = pn->pn_va.va_size;

	return 0;
}
