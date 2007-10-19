/*	$NetBSD: requests.c,v 1.11 2007/10/19 14:38:45 pooka Exp $	*/

/*
 * Copyright (c) 2006 Antti Kantee.  All Rights Reserved.
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
__RCSID("$NetBSD: requests.c,v 1.11 2007/10/19 14:38:45 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/queue.h>

#include <assert.h>
#include <errno.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "puffs_priv.h"

/*
 * XXX: a lot of this stuff is together just with string and bubblegum now
 */

/*ARGSUSED*/
struct puffs_getreq *
puffs_req_makeget(struct puffs_usermount *pu, size_t buflen, int maxops)
{
	struct puffs_getreq *pgr;

	pgr = malloc(sizeof(struct puffs_getreq));
	if (!pgr)
		return NULL;

	pgr->pgr_pu = pu;
	pgr->pgr_buf = NULL;

	return pgr;
}

int
puffs_req_loadget(struct puffs_getreq *pgr)
{
	struct puffs_frame pfr;
	uint8_t *buf;
	size_t rlen;
	int fd = pgr->pgr_pu->pu_fd;

	assert(pgr->pgr_buf == NULL);

	if (read(fd, &pfr, sizeof(struct puffs_frame)) == -1) {
		if (errno == EWOULDBLOCK)
			return 0;
		return -1;
	}
	buf = malloc(pfr.pfr_alloclen);
	assert(buf != NULL); /* XXX: a bit more grace here, thanks */
	memcpy(buf, &pfr, sizeof(pfr));

	rlen = pfr.pfr_len - sizeof(pfr);
	if (read(fd, buf + sizeof(pfr), rlen) != rlen) { /* XXX */
		free(buf);
		return -1;
	}
	pgr->pgr_buf = buf;

	return 0;
}

struct puffs_req *
puffs_req_get(struct puffs_getreq *pgr)
{
	void *buf;

	buf = pgr->pgr_buf;
	pgr->pgr_buf = NULL;
	return buf;
}

int
puffs_req_remainingget(struct puffs_getreq *pgr)
{

	return pgr->pgr_buf != NULL;
}

/*ARGSUSED*/
void
puffs_req_setmaxget(struct puffs_getreq *pgr, int maxops)
{

	/* nada */
}

void
puffs_req_destroyget(struct puffs_getreq *pgr)
{

	free(pgr->pgr_buf);
	free(pgr);
}


struct puffs_putreq *
puffs_req_makeput(struct puffs_usermount *pu)
{
	struct puffs_putreq *ppr;

	ppr = malloc(sizeof(struct puffs_putreq));
	if (!ppr)
		return NULL;
	memset(ppr, 0, sizeof(*ppr));

	TAILQ_INIT(&ppr->ppr_pccq);

	ppr->ppr_pu = pu;

	return ppr;
}

void
puffs_req_put(struct puffs_putreq *ppr, struct puffs_req *preq)
{
	ssize_t n;

	preq->preq_frhdr.pfr_len = preq->preq_buflen;
	n = write(ppr->ppr_pu->pu_fd, preq, preq->preq_frhdr.pfr_len);
	assert(n == preq->preq_frhdr.pfr_len);
	free(preq);
}

/*
 * instead of a direct preq, put a cc onto the push queue
 */
void
puffs_req_putcc(struct puffs_putreq *ppr, struct puffs_cc *pcc)
{

	puffs_req_put(ppr, pcc->pcc_preq);
	TAILQ_INSERT_TAIL(&ppr->ppr_pccq, pcc, entries);
}

/*ARGSUSED*/
int
puffs_req_putput(struct puffs_putreq *ppr)
{

	return 0;
}

void
puffs_req_resetput(struct puffs_putreq *ppr)
{
	struct puffs_cc *pcc;

	while ((pcc = TAILQ_FIRST(&ppr->ppr_pccq)) != NULL) {
		TAILQ_REMOVE(&ppr->ppr_pccq, pcc, entries);
		puffs_cc_destroy(pcc);
	}
}

void
puffs_req_destroyput(struct puffs_putreq *ppr)
{

	puffs_req_resetput(ppr);
	free(ppr);
}

/*ARGSUSED*/
int
puffs_req_handle(struct puffs_getreq *pgr, struct puffs_putreq *ppr, int maxops)
{
	struct puffs_usermount *pu;
	struct puffs_req *preq;
	int pval;

	assert(pgr->pgr_pu == ppr->ppr_pu);
	pu = pgr->pgr_pu;

	if (puffs_req_loadget(pgr) == -1)
		return -1;

	pval = 0;
	while ((preq = puffs_req_get(pgr)) != NULL
	    && puffs_getstate(pu) != PUFFS_STATE_UNMOUNTED)
		pval = puffs_dopreq(pu, preq, ppr);

	return pval;
}
