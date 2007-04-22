/*	$NetBSD: ninepuffs.c,v 1.2 2007/04/22 18:10:48 pooka Exp $	*/

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

/*
 * 9puffs: access a 9P file server as a vfs via puffs
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ninepuffs.c,v 1.2 2007/04/22 18:10:48 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include <netinet/in.h>

#include <assert.h>
#include <err.h>
#include <netdb.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ninepuffs.h"

#define DEFPORT_9P 564

static void puffs9p_eventloop(struct puffs_usermount *, struct puffs9p *);

static void
usage(void)
{

	errx(1, "usage: %s user server mountpoint", getprogname());
}

/*
 * TCPv4 connection to 9P file server, forget IL and IPv6 for now.
 * Return connected socket or exit with error.
 */
static int
serverconnect(const char *addr, unsigned short port)
{
	struct sockaddr_in mysin;
	struct hostent *he;
	int s;

	he = gethostbyname2(addr, AF_INET);
	if (he == NULL) {
		herror("gethostbyname");
		exit(1);
	}

	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == -1)
		err(1, "socket");

	memset(&mysin, 0, sizeof(struct sockaddr_in));
	mysin.sin_family = AF_INET;
	mysin.sin_port = htons(port);
	memcpy(&mysin.sin_addr, he->h_addr, sizeof(struct in_addr));

	if (connect(s, (struct sockaddr *)&mysin, sizeof(mysin)) == -1)
		err(1, "connect");

	return s;
}

int
main(int argc, char *argv[])
{
	struct puffs9p p9p;
	struct puffs_usermount *pu;
	struct puffs_ops *pops;
	struct puffs_node *pn_root;
	struct statvfs svfsb;
	mntoptparse_t mp;
	char *srvhost;
	char *user;
	unsigned short port;
	int mntflags, pflags, ch;
	int detach;

	setprogname(argv[0]);

	if (argc < 3)
		usage();

	mntflags = pflags = 0;
	detach = 1;
	port = DEFPORT_9P;

	while ((ch = getopt(argc, argv, "o:p:s")) != -1) {
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, puffsmopts, &mntflags, &pflags);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 's':
			detach = 0;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 3)
		usage();

	user = argv[0];
	srvhost = argv[1];

	if (pflags & PUFFS_FLAG_OPDUMP)
		detach = 0;
	pflags |= PUFFS_KFLAG_WTCACHE;

	PUFFSOP_INIT(pops);

	PUFFSOP_SET(pops, puffs9p, fs, unmount);
	PUFFSOP_SETFSNOP(pops, sync);
	PUFFSOP_SETFSNOP(pops, statvfs);

	PUFFSOP_SET(pops, puffs9p, node, lookup);
	PUFFSOP_SET(pops, puffs9p, node, readdir);
	PUFFSOP_SET(pops, puffs9p, node, getattr);
	PUFFSOP_SET(pops, puffs9p, node, setattr);
	PUFFSOP_SET(pops, puffs9p, node, create);
	PUFFSOP_SET(pops, puffs9p, node, open);
	PUFFSOP_SET(pops, puffs9p, node, close);
	PUFFSOP_SET(pops, puffs9p, node, mkdir);
	PUFFSOP_SET(pops, puffs9p, node, remove);
	PUFFSOP_SET(pops, puffs9p, node, rmdir);
	PUFFSOP_SET(pops, puffs9p, node, read);
	PUFFSOP_SET(pops, puffs9p, node, write);
	PUFFSOP_SET(pops, puffs9p, node, rename);
#if 0
	PUFFSOP_SET(pops, puffs9p, node, reclaim);
	PUFFSOP_SET(pops, puffs9p, node, mknod);
#endif

	memset(&p9p, 0, sizeof(p9p));
	p9p.maxreq = P9P_DEFREQLEN;
	p9p.nextfid = 1;
	TAILQ_INIT(&p9p.outbufq);
	TAILQ_INIT(&p9p.req_queue);

	p9p.servsock = serverconnect(srvhost, port);
	pu = puffs_init(pops, "9P", &p9p, pflags);
	if (pu == NULL)
		err(1, "puffs_init");

	if ((pn_root = p9p_handshake(pu, user)) == NULL) {
		puffs_exit(pu, 1);
		exit(1);
	}

	if (puffs_setblockingmode(pu, PUFFSDEV_NONBLOCK) == -1)
		err(1, "setblockingmode");

	if (puffs_domount(pu, argv[2], mntflags) == -1)
		err(1, "puffs_domount");

	puffs_zerostatvfs(&svfsb);
	if (puffs_start(pu, pn_root, &svfsb) == -1)
		err(1, "puffs_start");

	if (detach)
		daemon(1, 0);

	puffs9p_eventloop(pu, &p9p);

	return 0;
}

/*
 * enqueue buffer to be handled with cc
 */
void
outbuf_enqueue(struct puffs9p *p9p, struct p9pbuf *pb,
	struct puffs_cc *pcc, uint16_t tagid)
{

	pb->p9pr.tagid = tagid;
	pb->p9pr.pcc = pcc;
	pb->p9pr.func = NULL;
	pb->p9pr.arg = NULL;
	TAILQ_INSERT_TAIL(&p9p->outbufq, pb, p9pr.entries);
}

/*
 * enqueue buffer to be handled with "f".  "f" must not block.
 * gives up struct p9pbuf ownership.
 */
void
outbuf_enqueue_nocc(struct puffs9p *p9p, struct p9pbuf *pb,
	void (*f)(struct puffs9p *, struct p9pbuf *, void *), void *arg,
	uint16_t tagid)
{

	pb->p9pr.tagid = tagid;
	pb->p9pr.pcc = NULL;
	pb->p9pr.func = f;
	pb->p9pr.arg = arg;
	TAILQ_INSERT_TAIL(&p9p->outbufq, pb, p9pr.entries);
}

struct p9pbuf *
req_get(struct puffs9p *p9p, uint16_t tagid)
{
	struct p9pbuf *pb;

	TAILQ_FOREACH(pb, &p9p->req_queue, p9pr.entries)
		if (pb->p9pr.tagid == tagid)
			break;

	if (!pb)
		return NULL;

	TAILQ_REMOVE(&p9p->req_queue, pb, p9pr.entries);

	return pb;
}

static void
handlebuf(struct puffs9p *p9p, struct p9pbuf *datapb,
	struct puffs_putreq *ppr)
{
	struct p9preq p9prtmp;
	struct p9pbuf *pb;

	/* is this something we are expecting? */
	pb = req_get(p9p, datapb->tagid);

	if (pb == NULL) {
		printf("invalid server request response %d\n", datapb->tagid);
		p9pbuf_destroy(datapb);
		return;
	}

	/* keep p9preq clean, xxx uknow */
	p9prtmp = pb->p9pr;
	*pb = *datapb;
	pb->p9pr = p9prtmp;
	free(datapb);

	/* don't allow both cc and handler func, but allow neither */
	assert((pb->p9pr.pcc && pb->p9pr.func) == 0);
	if (pb->p9pr.pcc) {
		puffs_docc(pb->p9pr.pcc, ppr);
	} else if (pb->p9pr.func) {
		pb->p9pr.func(p9p, pb, pb->p9pr.arg);
	} else {
		assert(pb->p9pr.arg == NULL);
		p9pbuf_destroy(pb);
	}
}

static int
psshinput(struct puffs9p *p9p, struct puffs_putreq *ppr)
{
	struct p9pbuf *cb;
	int rv;

	for (;;) {
		if ((cb = p9p->curpb) == NULL) {
			cb = p9pbuf_make(p9p->maxreq, P9PB_IN);
			if (cb == NULL)
				return -1;
			p9p->curpb = cb;
		}

		rv = p9pbuf_read(p9p, cb);
		if (rv == -1)
			err(1, "p9pbuf read");
		if (rv == 0)
			break;

		handlebuf(p9p, cb, ppr);
		p9p->curpb = NULL;
	}

	return rv;
}

static int
psshoutput(struct puffs9p *p9p)
{
	struct p9pbuf *pb;
	int rv;

	TAILQ_FOREACH(pb, &p9p->outbufq, p9pr.entries) {
		rv = p9pbuf_write(p9p, pb);
		if (rv == -1)
			return -1;
		if (rv == 0)
			return 0;

		/* sent everything, move to cookiequeue */
		TAILQ_REMOVE(&p9p->outbufq, pb, p9pr.entries);
		free(pb->buf);
		TAILQ_INSERT_TAIL(&p9p->req_queue, pb, p9pr.entries);
	}

	return 1;
}

#define PFD_SOCK 0
#define PFD_PUFFS 1
static void
puffs9p_eventloop(struct puffs_usermount *pu, struct puffs9p *p9p)
{
	struct puffs_getreq *pgr;
	struct puffs_putreq *ppr;
	struct pollfd pfds[2];

	pgr = puffs_req_makeget(pu, puffs_getmaxreqlen(pu), 0);
	if (!pgr)
		err(1, "makegetreq");
	ppr = puffs_req_makeput(pu);
	if (!ppr)
		err(1, "makeputreq");

	while (puffs_getstate(pu) != PUFFS_STATE_UNMOUNTED) {
		memset(pfds, 0, sizeof(pfds));
		pfds[PFD_SOCK].events = POLLIN;
		if (!TAILQ_EMPTY(&p9p->outbufq))
			pfds[PFD_SOCK].events |= POLLOUT;
		pfds[PFD_SOCK].fd = p9p->servsock;
		pfds[PFD_PUFFS].fd = puffs_getselectable(pu);
		pfds[PFD_PUFFS].events = POLLIN;

		if (poll(pfds, 2, INFTIM) == -1)
			err(1, "poll");

		if (pfds[PFD_SOCK].revents & POLLOUT)
			if (psshoutput(p9p) == -1)
				err(1, "psshoutput");
		
		/* get & possibly dispatch events from kernel */
		if (pfds[PFD_PUFFS].revents & POLLIN)
			if (puffs_req_handle(pu, pgr, ppr, 0) == -1)
				err(1, "puffs_handlereqs");

		/* get input from sftpd, possibly build more responses */
		if (pfds[PFD_SOCK].revents & POLLIN)
			if (psshinput(p9p, ppr) == -1)
				errx(1, "psshinput");

		/* it's likely we got outputtables, poke the ice with a stick */
		if (psshoutput(p9p) == -1)
			err(1, "psshoutput");

		/* stuff all replies from both of the above into kernel */
		if (puffs_req_putput(ppr) == -1)
			err(1, "putputreq");
		puffs_req_resetput(ppr);
	}

	puffs_req_destroyget(pgr);
}
