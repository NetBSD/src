/*	$NetBSD: fs.c,v 1.3 2007/05/11 11:43:08 pooka Exp $	*/

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
__RCSID("$NetBSD: fs.c,v 1.3 2007/05/11 11:43:08 pooka Exp $");
#endif /* !lint */

#include <err.h>
#include <errno.h>
#include <puffs.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ninepuffs.h"
#include "nineproto.h"

#define DO_IO(fname, a1, a2, a3, a4, rv)				\
	puffs_framebuf_seekset(a2, 0);					\
	*(a4) = 0;							\
	rv = fname(a1, a2, a3, a4);					\
	if (rv || a4 == 0) errx(1, "p9p_handshake_ io failed %d, %d", rv, *a4) 

struct puffs_node *
p9p_handshake(struct puffs_usermount *pu, const char *username)
{
	struct puffs9p *p9p = puffs_getspecific(pu);
	struct puffs_framebuf *pb;
	struct puffs_node *pn;
	struct vattr rootva;
	uint32_t maxreq;
	uint16_t dummy;
	p9ptag_t tagid, rtagid;
	uint8_t type;
	int rv, done, x = 1;

	/* send initial handshake */
	pb = p9pbuf_makeout();
	p9pbuf_put_1(pb, P9PROTO_T_VERSION);
	p9pbuf_put_2(pb, P9PROTO_NOTAG);
	p9pbuf_put_4(pb, p9p->maxreq);
	p9pbuf_put_str(pb, P9PROTO_VERSION);
	DO_IO(p9pbuf_write, pu, pb, p9p->servsock, &done, rv);

	puffs_framebuf_recycle(pb);
	DO_IO(p9pbuf_read, pu, pb, p9p->servsock, &done, rv);

	if ((type = p9pbuf_get_type(pb)) != P9PROTO_R_VERSION)
		errx(1, "server invalid response to Tversion: %d", type);
	if ((rtagid = p9pbuf_get_tag(pb)) != P9PROTO_NOTAG) {
		errx(1, "server invalid tag: %d vs. %d\n",
		    P9PROTO_NOTAG, rtagid);
		return NULL;
	}
	if (p9pbuf_get_4(pb, &maxreq))
		errx(1, "server invalid response: no request length");
	if (maxreq < P9P_MINREQLEN)
		errx(1, "server request length below minimum accepted: "
		    "%d vs. %d", P9P_MINREQLEN, maxreq);
	p9p->maxreq = maxreq;

	/* tell the server we don't support authentication */
	p9pbuf_recycleout(pb);
	tagid = NEXTTAG(p9p);
	p9pbuf_put_1(pb, P9PROTO_T_AUTH);
	p9pbuf_put_2(pb, tagid);
	p9pbuf_put_4(pb, P9PROTO_NOFID);
	p9pbuf_put_str(pb, username);
	p9pbuf_put_str(pb, "");
	DO_IO(p9pbuf_write, pu, pb, p9p->servsock, &done, rv);

	puffs_framebuf_recycle(pb);
	DO_IO(p9pbuf_read, pu, pb, p9p->servsock, &done, rv);

	/* assume all Rerror is "no auth" */
	if (p9pbuf_get_type(pb) != P9PROTO_R_ERROR)
		errx(1, "mount_9p supports only NO auth");
	if ((rtagid = p9pbuf_get_tag(pb)) != tagid)
		errx(1, "server invalid tag: %d vs. %d\n", tagid, rtagid);

	/* build attach message */
	p9pbuf_recycleout(pb);
	tagid = NEXTTAG(p9p);
	p9pbuf_put_1(pb, P9PROTO_T_ATTACH);
	p9pbuf_put_2(pb, tagid);
	p9pbuf_put_4(pb, P9P_ROOTFID);
	p9pbuf_put_4(pb, P9PROTO_NOFID);
	p9pbuf_put_str(pb, username);
	p9pbuf_put_str(pb, "");
	DO_IO(p9pbuf_write, pu, pb, p9p->servsock, &done, rv);

	puffs_framebuf_recycle(pb);
	DO_IO(p9pbuf_read, pu, pb, p9p->servsock, &done, rv);

	if ((type = p9pbuf_get_type(pb)) != P9PROTO_R_ATTACH)
		errx(1, "Rattach not received, got %d", type);
	if ((rtagid = p9pbuf_get_tag(pb)) != tagid)
		errx(1, "server invalid tag: %d vs. %d\n", tagid, rtagid);

	/* finally, stat the rootnode */
	p9pbuf_recycleout(pb);
	tagid = NEXTTAG(p9p);
	p9pbuf_put_1(pb, P9PROTO_T_STAT);
	p9pbuf_put_2(pb, tagid);
	p9pbuf_put_4(pb, P9P_ROOTFID);
	DO_IO(p9pbuf_write, pu, pb, p9p->servsock, &done, rv);

	puffs_framebuf_recycle(pb);
	DO_IO(p9pbuf_read, pu, pb, p9p->servsock, &done, rv);

	if ((type = p9pbuf_get_type(pb)) != P9PROTO_R_STAT)
		errx(1, "Rstat not received, got %d", type);
	if ((rtagid = p9pbuf_get_tag(pb)) != tagid)
		errx(1, "server invalid tag: %d vs. %d\n", tagid, rtagid);
	if (p9pbuf_get_2(pb, &dummy))
		errx(1, "couldn't get stat len parameter");
	if (proto_getstat(pb, &rootva, NULL, NULL))
		errx(1, "could not parse root attributes");
	puffs_framebuf_destroy(pb);

	rootva.va_nlink = 0156; /* guess, will be fixed with first readdir */
	pn = newp9pnode_va(pu, &rootva, P9P_ROOTFID);

	if (ioctl(p9p->servsock, FIONBIO, &x) == -1)
		err(1, "cannot set socket in nonblocking mode");

	return pn;
}

int
puffs9p_fs_unmount(struct puffs_cc *pcc, int flags, pid_t pid)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs9p *p9p = puffs_getspecific(pu);

	close(p9p->servsock);
	return 0;
}
