/*	$NetBSD: framebuf.c,v 1.3 2007/05/06 10:54:41 pooka Exp $	*/

/*
 * Copyright (c) 2007  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
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
#if !defined(lint)
__RCSID("$NetBSD: framebuf.c,v 1.3 2007/05/06 10:54:41 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/queue.h>

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <puffs.h>
#include <stdlib.h>

#include "puffs_priv.h"

struct puffs_framebuf {
	struct puffs_cc *pcc;	/* pcc to continue with */
	/* OR */
	puffs_framebuf_cb fcb;	/* non-blocking callback */
	void *fcb_arg;		/* argument for previous */

	uint8_t *buf;		/* buffer base */
	size_t len;		/* total length */

	size_t offset;		/* cursor, telloff() */
	size_t maxoff;		/* maximum offset for data, tellsize() */

	volatile int rv;	/* errno value for pcc framebufs */

	int	istat;

	TAILQ_ENTRY(puffs_framebuf) pfb_entries;
};
#define ISTAT_NODESTROY	0x01	/* indestructible by framebuf_destroy() */
#define ISTAT_INTERNAL	0x02	/* never leaves library			*/
#define ISTAT_NOREPLY	0x04	/* nuke after sending 			*/

struct puffs_framectrl {
	puffs_framebuf_readframe_fn rfb;
	puffs_framebuf_writeframe_fn wfb;
	puffs_framebuf_respcmp_fn cmpfb;
	int io_fd;

	struct puffs_framebuf *cur_in;

	TAILQ_HEAD(, puffs_framebuf) snd_qing;	/* queueing to be sent */
	TAILQ_HEAD(, puffs_framebuf) res_qing;	/* q'ing for rescue */
};

#define PUFBUF_INCRALLOC 65536	/* 64k ought to be enough for anyone */
#define PUFBUF_REMAIN(p) (p->len - p->offset)

struct puffs_framebuf *
puffs_framebuf_make()
{
	struct puffs_framebuf *pufbuf;

	pufbuf = malloc(sizeof(struct puffs_framebuf));
	if (pufbuf == NULL)
		return NULL;
	memset(pufbuf, 0, sizeof(struct puffs_framebuf));

	pufbuf->buf = malloc(PUFBUF_INCRALLOC);
	pufbuf->len = PUFBUF_INCRALLOC;
	if (pufbuf->buf == NULL) {
		free(pufbuf);
		return NULL;
	}

	puffs_framebuf_recycle(pufbuf);
	return pufbuf;
}

void
puffs_framebuf_destroy(struct puffs_framebuf *pufbuf)
{

	assert((pufbuf->istat & ISTAT_NODESTROY) == 0);

	free(pufbuf->buf);
	free(pufbuf);
}

void
puffs_framebuf_recycle(struct puffs_framebuf *pufbuf)
{

	assert((pufbuf->istat & ISTAT_NODESTROY) == 0);

	pufbuf->offset = 0;
	pufbuf->maxoff = 0;
	pufbuf->istat = 0;
}

static int
reservespace(struct puffs_framebuf *pufbuf, size_t off, size_t wantsize)
{
	size_t incr;
	void *nd;

	if (off <= pufbuf->len && pufbuf->len - off >= wantsize)
		return 0;

	for (incr = PUFBUF_INCRALLOC;
	    pufbuf->len + incr < off + wantsize;
	    incr += PUFBUF_INCRALLOC)
		continue;

	nd = realloc(pufbuf->buf, pufbuf->offset + incr);
	if (nd == NULL)
		return -1;

	pufbuf->buf = nd;
	pufbuf->len += incr;

	return 0;
}

int
puffs_framebuf_reserve_space(struct puffs_framebuf *pufbuf, size_t wantsize)
{

	return reservespace(pufbuf, pufbuf->offset, wantsize);
}

int
puffs_framebuf_putdata(struct puffs_framebuf *pufbuf,
	const void *data, size_t dlen)
{

	if (PUFBUF_REMAIN(pufbuf) < dlen)
		if (puffs_framebuf_reserve_space(pufbuf, dlen) == -1)
			return -1;

	memcpy(pufbuf->buf + pufbuf->offset, data, dlen);
	pufbuf->offset += dlen;

	if (pufbuf->offset > pufbuf->maxoff)
		pufbuf->maxoff = pufbuf->offset;

	return 0;
}

int
puffs_framebuf_putdata_atoff(struct puffs_framebuf *pufbuf, size_t offset,
	const void *data, size_t dlen)
{

	if (reservespace(pufbuf, offset, dlen) == -1)
		return -1;

	memcpy(pufbuf->buf + offset, data, dlen);

	if (offset + dlen > pufbuf->maxoff)
		pufbuf->maxoff = offset + dlen;

	return 0;
}

int
puffs_framebuf_getdata(struct puffs_framebuf *pufbuf, void *data, size_t dlen)
{

	if (pufbuf->maxoff < pufbuf->offset + dlen) {
		errno = ENOBUFS;
		return -1;
	}

	memcpy(data, pufbuf->buf + pufbuf->offset, dlen);
	pufbuf->offset += dlen;

	return 0;
}

int
puffs_framebuf_getdata_atoff(struct puffs_framebuf *pufbuf, size_t offset,
	void *data, size_t dlen)
{

	if (pufbuf->maxoff < offset + dlen) {
		errno = ENOBUFS;
		return -1;
	}

	memcpy(data, pufbuf->buf + offset, dlen);
	return 0;
}

size_t
puffs_framebuf_telloff(struct puffs_framebuf *pufbuf)
{

	return pufbuf->offset;
}

size_t
puffs_framebuf_tellsize(struct puffs_framebuf *pufbuf)
{

	return pufbuf->maxoff;
}

size_t
puffs_framebuf_remaining(struct puffs_framebuf *pufbuf)
{

	return puffs_framebuf_tellsize(pufbuf) - puffs_framebuf_telloff(pufbuf);
}

int
puffs_framebuf_seekset(struct puffs_framebuf *pufbuf, size_t newoff)
{

	if (reservespace(pufbuf, newoff, 0) == -1)
		return -1;

	pufbuf->offset = newoff;
	return 0;
}

int
puffs_framebuf_getwindow(struct puffs_framebuf *pufbuf, size_t winoff,
	void **data, size_t *dlen)
{
	size_t winlen;

#ifdef WINTESTING
	winlen = MIN(*dlen, 32);
#else
	winlen = *dlen;
#endif

	if (reservespace(pufbuf, winoff, winlen) == -1)
		return -1;

	*data = pufbuf->buf + winoff;
	if (pufbuf->maxoff < winoff + winlen)
		pufbuf->maxoff = winoff + winlen;

	return 0;
}

int
puffs_framebuf_enqueue_cc(struct puffs_cc *pcc, struct puffs_framebuf *pufbuf)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_framectrl *fctrl = pu->pu_framectrl;

	pufbuf->pcc = pcc;
	pufbuf->fcb = NULL;
	pufbuf->fcb_arg = NULL;

	pufbuf->offset = 0;
	pufbuf->istat |= ISTAT_NODESTROY;

	TAILQ_INSERT_TAIL(&fctrl->snd_qing, pufbuf, pfb_entries);

	puffs_cc_yield(pcc);
	if (pufbuf->rv) {
		errno = pufbuf->rv;
		return -1;
	}

	return 0;
}

void
puffs_framebuf_enqueue_cb(struct puffs_usermount *pu,
	struct puffs_framebuf *pufbuf, puffs_framebuf_cb fcb, void *arg)
{
	struct puffs_framectrl *fctrl = pu->pu_framectrl;

	pufbuf->pcc = NULL;
	pufbuf->fcb = fcb;
	pufbuf->fcb_arg = arg;

	pufbuf->offset = 0;
	pufbuf->istat |= ISTAT_NODESTROY;

	TAILQ_INSERT_TAIL(&fctrl->snd_qing, pufbuf, pfb_entries);
}

void
puffs_framebuf_enqueue_justsend(struct puffs_usermount *pu,
	struct puffs_framebuf *pufbuf, int reply)
{
	struct puffs_framectrl *fctrl = pu->pu_framectrl;

	pufbuf->pcc = NULL;
	pufbuf->fcb = NULL;
	pufbuf->fcb_arg = NULL;

	pufbuf->offset = 0;
	pufbuf->istat |= ISTAT_NODESTROY;
	if (!reply)
		pufbuf->istat |= ISTAT_NOREPLY;

	TAILQ_INSERT_TAIL(&fctrl->snd_qing, pufbuf, pfb_entries);
}

static struct puffs_framebuf *
findbuf(struct puffs_usermount *pu, struct puffs_framectrl *fctrl,
	struct puffs_framebuf *findme)
{
	struct puffs_framebuf *cand;

	TAILQ_FOREACH(cand, &fctrl->res_qing, pfb_entries)
		if (fctrl->cmpfb(pu, findme, cand))
			break;

	if (cand == NULL)
		return NULL;

	TAILQ_REMOVE(&fctrl->res_qing, cand, pfb_entries);
	return cand;
}

static void
moveinfo(struct puffs_framebuf *from, struct puffs_framebuf *to)
{

	assert(from->istat & ISTAT_INTERNAL);

	/* migrate buffer */
	free(to->buf);
	to->buf = from->buf;
	from->buf = NULL;

	/* migrate buffer info */
	to->len = from->len;
	to->offset = from->offset;
	to->maxoff = from->maxoff;
}

static int
handle_input(struct puffs_usermount *pu, struct puffs_framectrl *fctrl,
	struct puffs_putreq *ppr)
{
	struct puffs_framebuf *pufbuf, *appbuf;
	int rv, complete;

	for (;;) {
		if ((pufbuf = fctrl->cur_in) == NULL) {
			pufbuf = puffs_framebuf_make();
			if (pufbuf == NULL)
				return -1;
			pufbuf->istat |= ISTAT_INTERNAL;
			fctrl->cur_in = pufbuf;
		}

		complete = 0;
		rv = fctrl->rfb(pu, pufbuf, fctrl->io_fd, &complete);

		/* error */
		if (rv) {
			errno = rv;
			return -1;
		}

		/* partial read, come back to fight another day */
		if (complete == 0)
			break;

		/* else: full read, process */

		appbuf = findbuf(pu, fctrl, pufbuf);
		if (appbuf == NULL) {
			errno = ENOMSG;
			return -1;
		}
			
		appbuf->istat &= ~ISTAT_NODESTROY;
		moveinfo(pufbuf, appbuf);
		if (appbuf->pcc) {
			puffs_docc(appbuf->pcc, ppr);
		} else if (appbuf->fcb) {
			appbuf->fcb(pu, appbuf, appbuf->fcb_arg);
		} else {
			puffs_framebuf_destroy(appbuf);
		}
		puffs_framebuf_destroy(pufbuf);

		/* hopeless romantics, here we go again */
		fctrl->cur_in = NULL;
	}

	return rv;
}

static int
handle_output(struct puffs_usermount *pu, struct puffs_framectrl *fctrl)
{
	struct puffs_framebuf *pufbuf, *pufbuf_next;
	int rv, complete;

	for (pufbuf = TAILQ_FIRST(&fctrl->snd_qing);
	    pufbuf;
	    pufbuf = pufbuf_next) {
		complete = 0;
		pufbuf_next = TAILQ_NEXT(pufbuf, pfb_entries);
		rv = fctrl->wfb(pu, pufbuf, fctrl->io_fd, &complete);

		if (rv) {
			TAILQ_REMOVE(&fctrl->snd_qing, pufbuf, pfb_entries);
			pufbuf->rv = rv;
			errno = rv;
			return -1;
		}

		/* partial write */
		if (complete == 0)
			return 0;

		/* else, complete write */
		TAILQ_REMOVE(&fctrl->snd_qing, pufbuf, pfb_entries);

		if ((pufbuf->istat & ISTAT_NOREPLY) == 0) {
			TAILQ_INSERT_TAIL(&fctrl->res_qing, pufbuf,
			    pfb_entries);
		} else {
			pufbuf->istat &= ~ISTAT_NODESTROY;
			puffs_framebuf_destroy(pufbuf);
		}
	}

	return 0;
}

#define FRAMEFD 0
#define PUFFSFD 1
int
puffs_framebuf_eventloop(struct puffs_usermount *pu, int io_fd,
	puffs_framebuf_readframe_fn rfb, puffs_framebuf_writeframe_fn wfb,
	puffs_framebuf_respcmp_fn cmpfb,
	puffs_framebuf_loop_fn lfb)
{
	struct puffs_getreq *pgr = NULL;
	struct puffs_putreq *ppr = NULL;
	struct puffs_framectrl *pfctrl = NULL;
	struct pollfd pfds[2];
	int puffsfd, rv;

	assert(puffs_getstate(pu) >= PUFFS_STATE_RUNNING);

	pgr = puffs_req_makeget(pu, puffs_getmaxreqlen(pu), 0);
	if (pgr == NULL)
		goto out;

	ppr = puffs_req_makeput(pu);
	if (ppr == NULL)
		goto out;

	pfctrl = malloc(sizeof(struct puffs_framectrl));
	if (pfctrl == NULL)
		goto out;
	pfctrl->rfb = rfb;
	pfctrl->wfb = wfb;
	pfctrl->cmpfb = cmpfb;
	pfctrl->io_fd = io_fd;
	TAILQ_INIT(&pfctrl->snd_qing);
	TAILQ_INIT(&pfctrl->res_qing);
	pu->pu_framectrl = pfctrl;

	puffsfd = puffs_getselectable(pu);
	while (puffs_getstate(pu) != PUFFS_STATE_UNMOUNTED) {
		if (lfb)
			lfb(pu);

		memset(pfds, 0, sizeof(pfds));
		pfds[FRAMEFD].events = POLLIN;
		if (!TAILQ_EMPTY(&pfctrl->snd_qing))
			pfds[FRAMEFD].events |= POLLOUT;
		pfds[FRAMEFD].fd = io_fd;
		pfds[PUFFSFD].events = POLLIN;
		pfds[PUFFSFD].fd = puffsfd;

		if (poll(pfds, 2, INFTIM) == -1)
			goto out;

		/* if there is room in the sucket for output ... */
		if (pfds[FRAMEFD].revents & POLLOUT)
			if (handle_output(pu, pfctrl) == -1)
				goto out;

		/* get & possibly dispatch events from kernel */
		if (pfds[PUFFSFD].revents & POLLIN)
			if (puffs_req_handle(pu, pgr, ppr, 0) == -1)
				goto out;

		/* get input from framefd, possibly build more responses */
		if (pfds[FRAMEFD].revents & POLLIN)
			if (handle_input(pu, pfctrl, ppr) == -1)
				goto out;

		/* it's likely we got outputtables, poke the ice with a stick */
		if (handle_output(pu, pfctrl) == -1)
			goto out;

		/* stuff all replies from both of the above into kernel */
		if (puffs_req_putput(ppr) == -1)
			goto out;
		puffs_req_resetput(ppr);
	}
	errno = 0;

 out:
	rv = errno;
	if (pfctrl) {
		pu->pu_framectrl = NULL;
		free(pfctrl);
	}

	if (ppr)
		puffs_req_destroyput(ppr);
	if (pgr)
		puffs_req_destroyget(pgr);

	return rv;
}
