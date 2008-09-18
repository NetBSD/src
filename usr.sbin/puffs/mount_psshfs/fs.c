/*	$NetBSD: fs.c,v 1.15.8.1 2008/09/18 04:30:10 wrstuden Exp $	*/

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
__RCSID("$NetBSD: fs.c,v 1.15.8.1 2008/09/18 04:30:10 wrstuden Exp $");
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
do {									\
	puffs_framebuf_seekset(a2, 0);					\
	*(a4) = 0;							\
	rv = fname(a1, a2, a3, a4);					\
	if (rv || a4 == 0) {						\
		fprintf(stderr, "psshfs_handshake failed %d (%s) %d\n",	\
		    rv, strerror(rv), *a4);				\
		return rv ? rv : EPROTO;				\
	}								\
} while (/*CONSTCOND*/0)

#define reterr(str, rv)							\
do {									\
	fprintf str;							\
	return rv;							\
} while (/*CONSTCOND*/0)

int
psshfs_handshake(struct puffs_usermount *pu)
{
	struct psshfs_ctx *pctx = puffs_getspecific(pu);
	struct puffs_framebuf *pb;
	struct puffs_pathobj *po_root;
	struct puffs_node *pn_root;
	struct vattr va, *rva;
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
		reterr((stderr, "invalid server response: %d",
		    psbuf_get_type(pb)), EPROTO);
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
		reterr((stderr, "invalid server realpath response for \"%s\"",
		    pctx->mountpath), EPROTO);
	if (psbuf_get_4(pb, &count) == -1)
		reterr((stderr, "invalid realpath response: count"), EPROTO);
	if (psbuf_get_str(pb, &rootpath, NULL) == -1)
		reterr((stderr, "invalid realpath response: rootpath"), EPROTO);

	/* stat the rootdir so that we know it's a dir */
	psbuf_recycleout(pb);
	psbuf_req_str(pb, SSH_FXP_LSTAT, NEXTREQ(pctx), rootpath);
	DO_IO(psbuf_write, pu, pb, pctx->sshfd, &done, rv);

	puffs_framebuf_recycle(pb);
	DO_IO(psbuf_read, pu, pb, pctx->sshfd, &done, rv);

	rv = psbuf_expect_attrs(pb, &va);
	if (rv)
		reterr((stderr, "couldn't stat rootpath"), rv);
	puffs_framebuf_destroy(pb);

	if (puffs_mode2vt(va.va_mode) != VDIR)
		reterr((stderr, "remote path (%s) not a directory", rootpath),
		    ENOTDIR);

	pn_root = puffs_getroot(pu);
	rva = &pn_root->pn_va;
	puffs_setvattr(rva, &va);

	po_root = puffs_getrootpathobj(pu);
	if (po_root == NULL)
		err(1, "getrootpathobj");
	po_root->po_path = rootpath;
	po_root->po_len = strlen(rootpath);

	return 0;
}

int
psshfs_fs_unmount(struct puffs_usermount *pu, int flags)
{
	struct psshfs_ctx *pctx = puffs_getspecific(pu);

	kill(pctx->sshpid, SIGTERM);
	close(pctx->sshfd);
	return 0;
}

int
psshfs_fs_nodetofh(struct puffs_usermount *pu, puffs_cookie_t cookie,
	void *fid, size_t *fidsize)
{
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
psshfs_fs_fhtonode(struct puffs_usermount *pu, void *fid, size_t fidsize,
	struct puffs_newinfo *pni)
{
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
	rv = getnodeattr(pu, pn);
	if (rv)
		return EINVAL;

	puffs_newinfo_setcookie(pni, pn);
	puffs_newinfo_setvtype(pni, pn->pn_va.va_type);
	puffs_newinfo_setsize(pni, pn->pn_va.va_size);

	return 0;
}
