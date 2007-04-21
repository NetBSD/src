/*	$NetBSD: fs.c,v 1.1 2007/04/21 14:21:43 pooka Exp $	*/

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
__RCSID("$NetBSD: fs.c,v 1.1 2007/04/21 14:21:43 pooka Exp $");
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

struct puffs_node *
p9p_handshake(struct puffs_usermount *pu, const char *username)
{
	struct puffs9p *p9p = puffs_getspecific(pu);
	struct p9pbuf *pb;
	struct puffs_node *pn;
	struct vattr rootva;
	uint32_t maxreq;
	uint16_t dummy;
	p9ptag_t tagid;
	int rv, x = 1;

	/* send initial handshake */
	pb = p9pbuf_make(p9p->maxreq, P9PB_OUT);
	p9pbuf_put_1(pb, P9PROTO_T_VERSION);
	p9pbuf_put_2(pb, P9PROTO_NOTAG);
	p9pbuf_put_4(pb, p9p->maxreq);
	p9pbuf_put_str(pb, P9PROTO_VERSION);
	while ((rv = p9pbuf_write(p9p, pb)) != 1) {
		if (rv == -1) {
			warn("Tversion send failed");
			return NULL;
		}
	}

	p9pbuf_recycle(pb, P9PB_IN);
	while ((rv = p9pbuf_read(p9p, pb)) != 1) {
		if (rv == -1) {
			warn("Rversion receive failed");
			return NULL;
		}
	}

	if (pb->type != P9PROTO_R_VERSION) {
		warnx("server invalid response to Tversion: %d", pb->type);
		return NULL;
	}
	if (pb->tagid != P9PROTO_NOTAG) {
		warnx("server invalid tag: %d vs. %d\n",
		    P9PROTO_NOTAG, pb->tagid);
		return NULL;
	}
	if (!p9pbuf_get_4(pb, &maxreq)) {
		warnx("server invalid response: no request length");
		return NULL;
	}
	if (maxreq < P9P_MINREQLEN) {
		warnx("server request length below minimum accepted: %d vs. %d",
		    P9P_MINREQLEN, maxreq);
		return NULL;
	}
	p9p->maxreq = maxreq;

	/* tell the server we don't support authentication */
	tagid = NEXTTAG(p9p);
	p9pbuf_recycle(pb, P9PB_OUT);
	p9pbuf_put_1(pb, P9PROTO_T_AUTH);
	p9pbuf_put_2(pb, tagid);
	p9pbuf_put_4(pb, P9PROTO_NOFID);
	p9pbuf_put_str(pb, username);
	p9pbuf_put_str(pb, "");
	while ((rv = p9pbuf_write(p9p, pb)) != 1) {
		if (rv == -1) {
			warn("Tauth send failed");
			return NULL;
		}
	}

	p9pbuf_recycle(pb, P9PB_IN);
	while ((rv = p9pbuf_read(p9p, pb)) != 1) {
		if (rv == -1) {
			warn("Rauth receive failed");
			return NULL;
		}
	}

	/* assume all Rerror is "no auth" */
	if (pb->type != P9PROTO_R_ERROR) {
		warnx("mount_9p supports only NO auth");
		return NULL;
	}
	if (pb->tagid != tagid) {
		warnx("server invalid tag: %d vs. %d\n", tagid, pb->tagid);
		return NULL;
	}

	/* build attach message */
	tagid = NEXTTAG(p9p);
	p9pbuf_recycle(pb, P9PB_OUT);
	p9pbuf_put_1(pb, P9PROTO_T_ATTACH);
	p9pbuf_put_2(pb, tagid);
	p9pbuf_put_4(pb, P9P_ROOTFID);
	p9pbuf_put_4(pb, P9PROTO_NOFID);
	p9pbuf_put_str(pb, username);
	p9pbuf_put_str(pb, "");
	while ((rv = p9pbuf_write(p9p, pb)) != 1) {
		if (rv == -1) {
			warn("Tattach send failed");
			return NULL;
		}
	}

	p9pbuf_recycle(pb, P9PB_IN);
	while ((rv = p9pbuf_read(p9p, pb)) != 1) {
		if (rv == -1) {
			warn("Rattach receive failed");
			return NULL;
		}
	}

	if (pb->type != P9PROTO_R_ATTACH) {
		warnx("Rattach not received, got %d", pb->type);
		return NULL;
	}
	if (pb->tagid != tagid) {
		warnx("server invalid tag: %d vs. %d\n", tagid, pb->tagid);
		return NULL;
	}
#if 0
	if (!proto_getqid(pb, rqid)) {
		warnx("cannot read root node qid");
		return NULL;
	}
#endif

	/* finally, stat the rootnode */
	tagid = NEXTTAG(p9p);
	p9pbuf_recycle(pb, P9PB_OUT);
	p9pbuf_put_1(pb, P9PROTO_T_STAT);
	p9pbuf_put_2(pb, tagid);
	p9pbuf_put_4(pb, P9P_ROOTFID);
	while ((rv = p9pbuf_write(p9p, pb)) != 1) {
		if (rv == -1) {
			warn("Tstat send failed");
			return NULL;
		}
	}

	p9pbuf_recycle(pb, P9PB_IN);
	while ((rv = p9pbuf_read(p9p, pb)) != 1) {
		if (rv == -1) {
			warn("Rstat receive failed");
			return NULL;
		}
	}

	if (pb->type != P9PROTO_R_STAT) {
		warnx("Rstat not received, got %d", pb->type);
		return NULL;
	}
	if (pb->tagid != tagid) {
		warnx("server invalid tag: %d vs. %d\n", tagid, pb->tagid);
		return NULL;
	}
	if (!p9pbuf_get_2(pb, &dummy)) {
		warnx("couldn't get stat len parameter");
		return NULL;
	}
	if (!proto_getstat(pb, &rootva, NULL, NULL)) {
		warnx("could not parse root attributes");
		return NULL;
	}
	p9pbuf_destroy(pb);

	rootva.va_nlink = 0156; /* guess, will be fixed with first readdir */
	pn = newp9pnode_va(pu, &rootva, P9P_ROOTFID);

	if (ioctl(p9p->servsock, FIONBIO, &x) == -1) {
		warnx("cannot set socket in nonblocking mode");
		return NULL;
	}

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
