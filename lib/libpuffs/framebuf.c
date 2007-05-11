/*	$NetBSD: framebuf.c,v 1.5 2007/05/11 16:22:38 pooka Exp $	*/

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
__RCSID("$NetBSD: framebuf.c,v 1.5 2007/05/11 16:22:38 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/queue.h>

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <puffs.h>
#include <stdlib.h>
#include <unistd.h>

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

struct puffs_fctrl_io {
	int io_fd;
	int wrstat;

	struct puffs_framebuf *cur_in;

	TAILQ_HEAD(, puffs_framebuf) snd_qing;	/* queueing to be sent */
	TAILQ_HEAD(, puffs_framebuf) res_qing;	/* q'ing for rescue */

	LIST_ENTRY(puffs_fctrl_io) fio_entries;
};
#define EN_WRITE(fio) (fio->wrstat == 0 && !TAILQ_EMPTY(&fio->snd_qing))
#define RM_WRITE(fio) (fio->wrstat == 1 && TAILQ_EMPTY(&fio->snd_qing))

struct puffs_framectrl {
	puffs_framebuf_readframe_fn rfb;
	puffs_framebuf_writeframe_fn wfb;
	puffs_framebuf_respcmp_fn cmpfb;

	struct kevent *evs;
	struct kevent *ch_evs;
	size_t nevs;
	int kq;

	LIST_HEAD(, puffs_fctrl_io) fb_ios;
};

#define PUFBUF_INCRALLOC 65536	/* 64k ought to be enough for anyone */
#define PUFBUF_REMAIN(p) (p->len - p->offset)

static struct puffs_fctrl_io *
getfiobyfd(struct puffs_usermount *pu, int fd)
{
	struct puffs_fctrl_io *fio;

	LIST_FOREACH(fio, &pu->pu_framectrl->fb_ios, fio_entries)
		if (fio->io_fd == fd)
			return fio;
	return NULL;
}

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
puffs_framebuf_enqueue_cc(struct puffs_cc *pcc, int fd,
	struct puffs_framebuf *pufbuf)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_fctrl_io *fio;

	fio = getfiobyfd(pu, fd);
	if (fio == NULL) {
		errno = EINVAL;
		return -1;
	}

	pufbuf->pcc = pcc;
	pufbuf->fcb = NULL;
	pufbuf->fcb_arg = NULL;

	pufbuf->offset = 0;
	pufbuf->istat |= ISTAT_NODESTROY;

	TAILQ_INSERT_TAIL(&fio->snd_qing, pufbuf, pfb_entries);

	puffs_cc_yield(pcc);
	if (pufbuf->rv) {
		errno = pufbuf->rv;
		return -1;
	}

	return 0;
}

int
puffs_framebuf_enqueue_cb(struct puffs_usermount *pu, int fd,
	struct puffs_framebuf *pufbuf, puffs_framebuf_cb fcb, void *arg)
{
	struct puffs_fctrl_io *fio;

	fio = getfiobyfd(pu, fd);
	if (fio == NULL) {
		errno = EINVAL;
		return -1;
	}

	pufbuf->pcc = NULL;
	pufbuf->fcb = fcb;
	pufbuf->fcb_arg = arg;

	pufbuf->offset = 0;
	pufbuf->istat |= ISTAT_NODESTROY;

	TAILQ_INSERT_TAIL(&fio->snd_qing, pufbuf, pfb_entries);

	return 0;
}

int
puffs_framebuf_enqueue_justsend(struct puffs_usermount *pu, int fd,
	struct puffs_framebuf *pufbuf, int reply)
{
	struct puffs_fctrl_io *fio;

	fio = getfiobyfd(pu, fd);
	if (fio == NULL) {
		errno = EINVAL;
		return -1;
	}

	pufbuf->pcc = NULL;
	pufbuf->fcb = NULL;
	pufbuf->fcb_arg = NULL;

	pufbuf->offset = 0;
	pufbuf->istat |= ISTAT_NODESTROY;
	if (!reply)
		pufbuf->istat |= ISTAT_NOREPLY;

	TAILQ_INSERT_TAIL(&fio->snd_qing, pufbuf, pfb_entries);

	return 0;
}

static struct puffs_framebuf *
findbuf(struct puffs_usermount *pu, struct puffs_framectrl *fctrl,
	struct puffs_fctrl_io *fio, struct puffs_framebuf *findme)
{
	struct puffs_framebuf *cand;

	TAILQ_FOREACH(cand, &fio->res_qing, pfb_entries)
		if (fctrl->cmpfb(pu, findme, cand))
			break;

	if (cand == NULL)
		return NULL;

	TAILQ_REMOVE(&fio->res_qing, cand, pfb_entries);
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
	struct puffs_fctrl_io *fio, struct puffs_putreq *ppr)
{
	struct puffs_framebuf *pufbuf, *appbuf;
	int rv, complete;

	for (;;) {
		if ((pufbuf = fio->cur_in) == NULL) {
			pufbuf = puffs_framebuf_make();
			if (pufbuf == NULL)
				return -1;
			pufbuf->istat |= ISTAT_INTERNAL;
			fio->cur_in = pufbuf;
		}

		complete = 0;
		rv = fctrl->rfb(pu, pufbuf, fio->io_fd, &complete);

		/* error */
		if (rv) {
			errno = rv;
			return -1;
		}

		/* partial read, come back to fight another day */
		if (complete == 0)
			break;

		/* else: full read, process */

		appbuf = findbuf(pu, fctrl, fio, pufbuf);
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
		fio->cur_in = NULL;
	}

	return rv;
}

static int
handle_output(struct puffs_usermount *pu, struct puffs_framectrl *fctrl,
	struct puffs_fctrl_io *fio)
{
	struct puffs_framebuf *pufbuf, *pufbuf_next;
	int rv, complete;

	for (pufbuf = TAILQ_FIRST(&fio->snd_qing);
	    pufbuf;
	    pufbuf = pufbuf_next) {
		complete = 0;
		pufbuf_next = TAILQ_NEXT(pufbuf, pfb_entries);
		rv = fctrl->wfb(pu, pufbuf, fio->io_fd, &complete);

		if (rv) {
			TAILQ_REMOVE(&fio->snd_qing, pufbuf, pfb_entries);
			pufbuf->rv = rv;
			errno = rv;
			return -1;
		}

		/* partial write */
		if (complete == 0)
			return 0;

		/* else, complete write */
		TAILQ_REMOVE(&fio->snd_qing, pufbuf, pfb_entries);

		if ((pufbuf->istat & ISTAT_NOREPLY) == 0) {
			TAILQ_INSERT_TAIL(&fio->res_qing, pufbuf,
			    pfb_entries);
		} else {
			pufbuf->istat &= ~ISTAT_NODESTROY;
			puffs_framebuf_destroy(pufbuf);
		}
	}

	return 0;
}

int
puffs_framebuf_addfd(struct puffs_usermount *pu, int fd)
{
	struct puffs_framectrl *pfctrl = pu->pu_framectrl;
	struct puffs_fctrl_io *fio;
	struct kevent kev[2];
	struct kevent *newevs;
	int rv;

	newevs = realloc(pfctrl->evs, (2*pfctrl->nevs+1)*sizeof(struct kevent));
	if (newevs == NULL)
		return -1;
	pfctrl->evs = newevs;

	newevs = realloc(pfctrl->ch_evs, pfctrl->nevs * sizeof(struct kevent));
	if (newevs == NULL)
		return -1;
	pfctrl->ch_evs = newevs;

	fio = malloc(sizeof(struct puffs_fctrl_io));
	if (fio == NULL)
		return -1;

	EV_SET(&kev[0], fd, EVFILT_READ, EV_ADD, 0, 0, (intptr_t)fio);
	EV_SET(&kev[1], fd, EVFILT_WRITE, EV_ADD|EV_DISABLE, 0,0,(intptr_t)fio);
	rv = kevent(pfctrl->kq, kev, 2, NULL, 0, NULL);
	if (rv == -1) {
		free(fio);
		return -1;
	}

	fio->io_fd = fd;
	fio->cur_in = NULL;
	fio->wrstat = 0;
	TAILQ_INIT(&fio->snd_qing);
	TAILQ_INIT(&fio->res_qing);

	LIST_INSERT_HEAD(&pfctrl->fb_ios, fio, fio_entries);
	pfctrl->nevs++;

	return 0;
}

static int
removefio(struct puffs_usermount *pu, struct puffs_fctrl_io *fio)
{
	struct puffs_framectrl *pfctrl = pu->pu_framectrl;
	struct puffs_framebuf *pufbuf;
	struct kevent kev[2];

	/* found, now remove */
	EV_SET(&kev[0], fio->io_fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
	EV_SET(&kev[1], fio->io_fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
	(void) kevent(pfctrl->kq, kev, 2, NULL, 0, NULL);

	LIST_REMOVE(fio, fio_entries);

	/* free buffers */
	while ((pufbuf = TAILQ_FIRST(&fio->snd_qing)) != NULL) {
		TAILQ_REMOVE(&fio->snd_qing, pufbuf, pfb_entries);
		puffs_framebuf_destroy(pufbuf);
	}
	while ((pufbuf = TAILQ_FIRST(&fio->res_qing)) != NULL) {
		TAILQ_REMOVE(&fio->res_qing, pufbuf, pfb_entries);
		puffs_framebuf_destroy(pufbuf);
	}
	if (fio->cur_in)
		puffs_framebuf_destroy(fio->cur_in);
	free(fio);

	/* don't bother with realloc */
	pfctrl->nevs--;

	return 0;

}

int
puffs_framebuf_removefd(struct puffs_usermount *pu, int fd)
{
	struct puffs_fctrl_io *fio;

	fio = getfiobyfd(pu, fd);
	if (fio == NULL) {
		errno = ENXIO;
		return -1;
	}

	return removefio(pu, fio);
}

int
puffs_framebuf_eventloop(struct puffs_usermount *pu, int *io_fds, size_t nfds,
	puffs_framebuf_readframe_fn rfb, puffs_framebuf_writeframe_fn wfb,
	puffs_framebuf_respcmp_fn cmpfb,
	puffs_framebuf_loop_fn lfb)
{
	struct puffs_getreq *pgr = NULL;
	struct puffs_putreq *ppr = NULL;
	struct puffs_framectrl *pfctrl = NULL;
	struct puffs_fctrl_io *fio;
	struct kevent kev;
	struct kevent *curev;
	size_t nchanges;
	int puffsfd, sverrno;
	int ndone;

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
	pfctrl->evs = malloc(sizeof(struct kevent));
	if (pfctrl->evs == NULL)
		goto out;
	pfctrl->ch_evs = NULL;
	pfctrl->nevs = 1;
	pfctrl->kq = kqueue();
	if (pfctrl->kq == -1)
		goto out;
	LIST_INIT(&pfctrl->fb_ios);
	pu->pu_framectrl = pfctrl;

	for (; nfds--; io_fds++)
		if (puffs_framebuf_addfd(pu, *io_fds) == -1)
			goto out;

	puffsfd = puffs_getselectable(pu);
	EV_SET(&kev, puffsfd, EVFILT_READ, EV_ADD, 0, 0, 0);
	if (kevent(pfctrl->kq, &kev, 1, NULL, 0, NULL) == -1)
		goto out;

	while (puffs_getstate(pu) != PUFFS_STATE_UNMOUNTED) {
		if (lfb)
			lfb(pu);

		/*
		 * Build list of which to enable/disable in writecheck.
		 * Don't bother worrying about O(n) for now.
		 */
		nchanges = 0;
		LIST_FOREACH(fio, &pfctrl->fb_ios, fio_entries) {
			/*
			 * Try to write out everything to avoid the
			 * need for enabling EVFILT_WRITE.  The likely
			 * case is that we can fit everything into the
			 * socket buffer.
			 */
			if (handle_output(pu, pfctrl, fio) == -1)
				goto out;

			assert((EN_WRITE(fio) && RM_WRITE(fio)) == 0);
			if (EN_WRITE(fio)) {
				EV_SET(&pfctrl->ch_evs[nchanges], fio->io_fd,
				    EVFILT_WRITE, EV_ENABLE, 0, 0,
				    (uintptr_t)fio);
				fio->wrstat = 1; /* XXX: not before call */
				nchanges++;
			}
			if (RM_WRITE(fio)) {
				EV_SET(&pfctrl->ch_evs[nchanges], fio->io_fd,
				    EVFILT_WRITE, EV_DISABLE, 0, 0,
				    (uintptr_t)fio);
				fio->wrstat = 0; /* XXX: not before call */
				nchanges++;
			}
		}

		ndone = kevent(pfctrl->kq, pfctrl->ch_evs, nchanges,
		    pfctrl->evs, pfctrl->nevs, NULL);
		if (ndone == -1)
			goto out;

		/* XXX: handle errors */

		/* iterate over the results */
		for (curev = pfctrl->evs; ndone--; curev++) {
			/* get & possibly dispatch events from kernel */
			if (curev->ident == puffsfd) {
				if (puffs_req_handle(pgr, ppr, 0) == -1)
					goto out;
				continue;
			}

			if (curev->filter == EVFILT_READ) {
				if (handle_input(pu, pfctrl,
				    (void *)curev->udata, ppr) == -1)
					goto out;
			}

			if (curev->filter == EVFILT_WRITE) {
				if (handle_output(pu, pfctrl,
				    (void *)curev->udata) == -1)
					goto out;
			}
		}

		/* stuff all replies from both of the above into kernel */
		if (puffs_req_putput(ppr) == -1)
			goto out;
		puffs_req_resetput(ppr);
	}
	errno = 0;

 out:
	/* store the real error for a while */
	sverrno = errno;

	if (pfctrl) {
		while ((fio = LIST_FIRST(&pfctrl->fb_ios)) != NULL)
			removefio(pu, fio);
		pu->pu_framectrl = NULL;
		free(pfctrl->evs);
		close(pfctrl->kq); /* takes care of puffsfd */
		free(pfctrl);
	}

	if (ppr)
		puffs_req_destroyput(ppr);
	if (pgr)
		puffs_req_destroyget(pgr);

	errno = sverrno;
	if (errno)
		return -1;
	else
		return 0;
}
