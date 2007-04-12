/*	$NetBSD: psshfs.c,v 1.10 2007/04/12 15:09:02 pooka Exp $	*/

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

/*
 * psshfs: puffs sshfs
 *
 * psshfs implements sshfs functionality on top of puffs making it
 * possible to mount a filesystme through the sftp service.
 *
 * psshfs can execute multiple operations in "parallel" by using the
 * puffs_cc framework for continuations.
 *
 * Concurrency control is handled currently by vnode locking (this
 * will change in the future).  Context switch locations are easy to
 * find by grepping for puffs_cc_yield().
 *
 * The operation revolves around an event loop.  Incoming requests from
 * the kernel are dispatched off to the libpuffs event handler, which
 * eventually calls the callbacks.  The callbacks do whatever they need
 * to do, queue output and call puffs_cc_yield() to put them to sleep.
 * Meanwhile execution continues.  When an answer to the callback's
 * query arrives, the blocker is located by the protocol's request
 * id and awakened by calling puffs_docc().  All changes made to the
 * buffer (psbuf->buf) will be seen by the callback.  Here the buffer
 * contains the response of the sftp server.  The callback will then
 * proceed to parse the buffer.
 *
 * XXX: struct psbuf is a mess.  it will be fixed sooner or later
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: psshfs.c,v 1.10 2007/04/12 15:09:02 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <mntopts.h>
#include <poll.h>
#include <puffs.h>
#include <signal.h>
#include <stdlib.h>
#include <util.h>
#include <unistd.h>

#include "psshfs.h"

static void	psshfs_eventloop(struct puffs_usermount *, struct psshfs_ctx *);
static void	pssh_connect(struct psshfs_ctx *, char **);
static void	usage(void);

#define SSH_PATH "/usr/bin/ssh"

static void
usage()
{

	errx(1, "usage: %s [-s] [-o opts] user@host:path mountpath",
	    getprogname());
}

int
main(int argc, char *argv[])
{
	struct psshfs_ctx pctx;
	struct puffs_usermount *pu;
	struct puffs_ops *pops;
	mntoptparse_t mp;
	char *sshargs[16];
	char *userhost;
	char *hostpath;
	int mntflags, pflags, ch;
	int detach;

	setprogname(argv[0]);

	if (argc < 3)
		usage();

	mntflags = pflags = 0;
	detach = 1;
	while ((ch = getopt(argc, argv, "o:s")) != -1) {
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, puffsmopts, &mntflags, &pflags);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
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

#if 0
	/* XXX: noatime is mandatory for now */
	mntflags |= MNT_NOATIME;
#endif

	if (pflags & PUFFS_FLAG_OPDUMP)
		detach = 0;

	if (argc != 2)
		usage();

	PUFFSOP_INIT(pops);

	PUFFSOP_SET(pops, psshfs, fs, unmount);
	PUFFSOP_SETFSNOP(pops, sync); /* XXX */
	PUFFSOP_SETFSNOP(pops, statvfs);

	PUFFSOP_SET(pops, psshfs, node, lookup);
	PUFFSOP_SET(pops, psshfs, node, create);
	PUFFSOP_SET(pops, psshfs, node, readdir);
	PUFFSOP_SET(pops, psshfs, node, getattr);
	PUFFSOP_SET(pops, psshfs, node, setattr);
	PUFFSOP_SET(pops, psshfs, node, mkdir);
	PUFFSOP_SET(pops, psshfs, node, remove);
	PUFFSOP_SET(pops, psshfs, node, readlink);
	PUFFSOP_SET(pops, psshfs, node, rmdir);
	PUFFSOP_SET(pops, psshfs, node, symlink);
	PUFFSOP_SET(pops, psshfs, node, rename);
	PUFFSOP_SET(pops, psshfs, node, read);
	PUFFSOP_SET(pops, psshfs, node, write);
	PUFFSOP_SET(pops, psshfs, node, reclaim);

	memset(&pctx, 0, sizeof(pctx));
	TAILQ_INIT(&pctx.outbufq);
	TAILQ_INIT(&pctx.req_queue);

	userhost = argv[0];
	hostpath = strchr(userhost, ':');
	if (hostpath) {
		*hostpath++ = '\0';
		pctx.mountpath = hostpath;
	} else
		pctx.mountpath = ".";

	/* xblah */
	sshargs[0] = SSH_PATH;
	sshargs[1] = argv[0];
	sshargs[2] = "-oClearAllForwardings=yes";
	sshargs[3] = "-a";
	sshargs[4] = "-x";
	sshargs[5] = "-s";
	sshargs[6] = "sftp";
	sshargs[7] = 0;

	if ((pu = puffs_mount(pops, argv[1], mntflags, "psshfs", &pctx,
	    PUFFS_FLAG_BUILDPATH | pflags, 0))==NULL)
		err(1, "puffs_mount");

	pssh_connect(&pctx, sshargs);

	if (puffs_setblockingmode(pu, PUFFSDEV_NONBLOCK) == -1)
		err(1, "setblockingmode");
	if (psshfs_domount(pu) != 0)
		errx(1, "domount");

	if (detach)
		daemon(1, 0);

	psshfs_eventloop(pu, &pctx);
	return 0;
}

/*
 * enqueue buffer to be handled with cc
 */
void
pssh_outbuf_enqueue(struct psshfs_ctx *pctx, struct psbuf *pb,
	struct puffs_cc *pcc, uint32_t reqid)
{

	pb->psr.reqid = reqid;
	pb->psr.pcc = pcc;
	pb->psr.func = NULL;
	pb->psr.arg = NULL;
	TAILQ_INSERT_TAIL(&pctx->outbufq, pb, psr.entries);
}

/*
 * enqueue buffer to be handled with "f".  "f" must not block.
 * gives up struct psbuf ownership.
 */
void
pssh_outbuf_enqueue_nocc(struct psshfs_ctx *pctx, struct psbuf *pb,
	void (*f)(struct psshfs_ctx *, struct psbuf *, void *), void *arg,
	uint32_t reqid)
{

	pb->psr.reqid = reqid;
	pb->psr.pcc = NULL;
	pb->psr.func = f;
	pb->psr.arg = arg;
	TAILQ_INSERT_TAIL(&pctx->outbufq, pb, psr.entries);
}

struct psbuf *
psshreq_get(struct psshfs_ctx *pctx, uint32_t reqid)
{
	struct psbuf *pb;

	TAILQ_FOREACH(pb, &pctx->req_queue, psr.entries)
		if (pb->psr.reqid == reqid)
			break;

	if (!pb)
		return NULL;

	TAILQ_REMOVE(&pctx->req_queue, pb, psr.entries);

	return pb;
}

static void
handlebuf(struct psshfs_ctx *pctx, struct psbuf *datapb,
	struct puffs_putreq *ppr)
{
	struct psreq psrtmp;
	struct psbuf *pb;

	/* is this something we are expecting? */
	pb = psshreq_get(pctx, datapb->reqid);

	if (pb == NULL) {
		printf("invalid server request response %d\n", datapb->reqid);
		psbuf_destroy(datapb);
		return;
	}

	/* keep psreq clean, xxx uknow */
	psrtmp = pb->psr;
	*pb = *datapb;
	pb->psr = psrtmp;
	free(datapb);

	/* don't allow both cc and handler func, but allow neither */
	assert((pb->psr.pcc && pb->psr.func) == 0);
	if (pb->psr.pcc) {
		puffs_docc(pb->psr.pcc, ppr);
	} else if (pb->psr.func) {
		pb->psr.func(pctx, pb, pb->psr.arg);
	} else {
		assert(pb->psr.arg == NULL);
		psbuf_destroy(pb);
	}
}

static int
psshinput(struct psshfs_ctx *pctx, struct puffs_putreq *ppr)
{
	struct psbuf *cb;
	int rv;

	for (;;) {
		if ((cb = pctx->curpb) == NULL) {
			cb = psbuf_make(PSB_IN);
			if (cb == NULL)
				return -1;
			pctx->curpb = cb;
		}

		rv = psbuf_read(pctx, cb);
		if (rv == -1)
			err(1, "psbuf read");
		if (rv == 0)
			break;

		handlebuf(pctx, cb, ppr);
		pctx->curpb = NULL;
	}

	return rv;
}

static int
psshoutput(struct psshfs_ctx *pctx)
{
	struct psbuf *pb;
	int rv;

	TAILQ_FOREACH(pb, &pctx->outbufq, psr.entries) {
		rv = psbuf_write(pctx, pb);
		if (rv == -1)
			return -1;
		if (rv == 0)
			return 0;

		/* sent everything, move to cookiequeue */
		TAILQ_REMOVE(&pctx->outbufq, pb, psr.entries);
		free(pb->buf);
		TAILQ_INSERT_TAIL(&pctx->req_queue, pb, psr.entries);
	}

	return 1;
}

volatile int timetodie;

#define PFD_SSH 0
#define PFD_PUFFS 1
static void
psshfs_eventloop(struct puffs_usermount *pu, struct psshfs_ctx *pctx)
{
	struct puffs_getreq *pgr;
	struct puffs_putreq *ppr;
	struct pollfd pfds[2];
	int x;

	pgr = puffs_req_makeget(pu, puffs_getmaxreqlen(pu), 0);
	if (!pgr)
		err(1, "makegetreq");
	ppr = puffs_req_makeput(pu);
	if (!ppr)
		err(1, "makeputreq");

	x = 1;
	if (ioctl(pctx->sshfd, FIONBIO, &x) == -1)
		err(1, "nonblocking descriptor");

	while (puffs_getstate(pu) != PUFFS_STATE_UNMOUNTED) {
		if (timetodie) {
			kill(pctx->sshpid, SIGTERM);
			break;
		}

		memset(pfds, 0, sizeof(pfds));
		pfds[PFD_SSH].events = POLLIN;
		if (!TAILQ_EMPTY(&pctx->outbufq))
			pfds[PFD_SSH].events |= POLLOUT;
		pfds[PFD_SSH].fd = pctx->sshfd;
		pfds[PFD_PUFFS].fd = puffs_getselectable(pu);
		pfds[PFD_PUFFS].events = POLLIN;

		if (poll(pfds, 2, INFTIM) == -1)
			err(1, "poll");

		if (pfds[PFD_SSH].revents & POLLOUT)
			if (psshoutput(pctx) == -1)
				err(1, "psshoutput");
		
		/* get & possibly dispatch events from kernel */
		if (pfds[PFD_PUFFS].revents & POLLIN)
			if (puffs_req_handle(pu, pgr, ppr, 0) == -1)
				err(1, "puffs_handlereqs");

		/* get input from sftpd, possibly build more responses */
		if (pfds[PFD_SSH].revents & POLLIN)
			if (psshinput(pctx, ppr) == -1)
				errx(1, "psshinput");

		/* it's likely we got outputtables, poke the ice with a stick */
		if (psshoutput(pctx) == -1)
			err(1, "psshoutput");

		/* stuff all replies from both of the above into kernel */
		if (puffs_req_putput(ppr) == -1)
			err(1, "putputreq");
		puffs_req_resetput(ppr);
	}

	puffs_req_destroyget(pgr);
}

static void
pssh_connect(struct psshfs_ctx *pctx, char **sshargs)
{
	int fds[2];
	pid_t pid;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1)
		err(1, "socketpair");

	pid = fork();
	switch (pid) {
	case -1:
		err(1, "fork");
		/*NOTREACHED*/
	case 0: /* child */
		if (dup2(fds[0], STDIN_FILENO) == -1)
			err(1, "child dup2");
		if (dup2(fds[0], STDOUT_FILENO) == -1)
			err(1, "child dup2");
		close(fds[0]);
		close(fds[1]);
		execvp(sshargs[0], sshargs);
		break;
	default:
		pctx->sshpid = pid;
		pctx->sshfd = fds[1];
		close(fds[0]);
		break;
	}
}
