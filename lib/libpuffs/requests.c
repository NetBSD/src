/*	$NetBSD: requests.c,v 1.9.6.2 2007/07/19 12:52:29 pooka Exp $	*/

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
__RCSID("$NetBSD: requests.c,v 1.9.6.2 2007/07/19 12:52:29 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/queue.h>

#include <assert.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>

#include "puffs_priv.h"

struct puffs_getreq *
puffs_req_makeget(struct puffs_usermount *pu, size_t buflen, int maxops)
{
	struct puffs_getreq *pgr;
	uint8_t *buf;

	pgr = malloc(sizeof(struct puffs_getreq));
	if (!pgr)
		return NULL;

	buf = malloc(buflen);
	if (!buf) {
		free(pgr);
		return NULL;
	}

	pgr->pgr_phg_orig.phg_buf = buf;
	pgr->pgr_phg_orig.phg_buflen = buflen;
	pgr->pgr_phg_orig.phg_nops = maxops;

	pgr->pgr_pu = pu;
	pgr->pgr_nppr = 0;

	return pgr;
}

int
puffs_req_loadget(struct puffs_getreq *pgr)
{

	assert(pgr->pgr_nppr == 0);

	/* reset */
	pgr->pgr_phg = pgr->pgr_phg_orig;

	if (ioctl(pgr->pgr_pu->pu_fd, PUFFSGETOP, &pgr->pgr_phg) == -1)
		return -1;

	pgr->pgr_nextpreq = pgr->pgr_phg.phg_buf;
	pgr->pgr_advance = pgr->pgr_nextpreq->preq_buflen;

	return 0;
}

struct puffs_req *
puffs_req_get(struct puffs_getreq *pgr)
{
	struct puffs_req *preq;

	if (pgr->pgr_phg.phg_nops == 0)
		return NULL;

	preq = pgr->pgr_nextpreq;
	/*LINTED*/
	pgr->pgr_nextpreq =
	    (struct puffs_req*)((uint8_t*)preq + pgr->pgr_advance);
	pgr->pgr_advance = pgr->pgr_nextpreq->preq_buflen;
	pgr->pgr_phg.phg_nops--;

	return preq;
}

int
puffs_req_remainingget(struct puffs_getreq *pgr)
{

	return pgr->pgr_phg.phg_nops;
}

void
puffs_req_setmaxget(struct puffs_getreq *pgr, int maxops)
{

	pgr->pgr_phg.phg_nops = maxops;
	pgr->pgr_phg_orig.phg_nops = maxops;
}

void
puffs_req_destroyget(struct puffs_getreq *pgr)
{

	assert(pgr->pgr_nppr == 0);

	free(pgr->pgr_phg_orig.phg_buf);
	free(pgr);
}


struct puffs_putreq *
puffs_req_makeput(struct puffs_usermount *pu)
{
	struct puffs_putreq *ppr;

	ppr = malloc(sizeof(struct puffs_putreq));
	if (!ppr)
		return NULL;

	ppr->ppr_php.php_nops = 0;
	TAILQ_INIT(&ppr->ppr_pccq);

	ppr->ppr_pu = pu;
	ppr->ppr_pgr = NULL;

	puffs_req_resetput(ppr);

	return ppr;
}

void
puffs_req_put(struct puffs_putreq *ppr, struct puffs_req *preq)
{

	ppr->ppr_php.php_nops++;

	/* store data */
	*ppr->ppr_buf = preq;
	*ppr->ppr_buflen = preq->preq_buflen;
	*ppr->ppr_id = preq->preq_id;

	/* and roll forward for next request */
	ppr->ppr_buf = &preq->preq_nextbuf;
	ppr->ppr_buflen = &preq->preq_buflen;
	ppr->ppr_id = &preq->preq_id;
}

/*
 * instead of a direct preq, put a cc onto the push queue
 */
void
puffs_req_putcc(struct puffs_putreq *ppr, struct puffs_cc *pcc)
{

	TAILQ_INSERT_TAIL(&ppr->ppr_pccq, pcc, entries);
	puffs_req_put(ppr, pcc->pcc_preq);
}

int
puffs_req_putput(struct puffs_putreq *ppr)
{

	if (ppr->ppr_php.php_nops)
		if (ioctl(ppr->ppr_pu->pu_fd, PUFFSPUTOP, &ppr->ppr_php) == -1)
			return -1;

	return 0;
}

void
puffs_req_resetput(struct puffs_putreq *ppr)
{
	struct puffs_cc *pcc;

	if (ppr->ppr_pgr != NULL) {
		ppr->ppr_pgr->pgr_nppr--;
		ppr->ppr_pgr = NULL;
	}

	ppr->ppr_buf = &ppr->ppr_php.php_buf;
	ppr->ppr_buflen = &ppr->ppr_php.php_buflen;
	ppr->ppr_id = &ppr->ppr_php.php_id;

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

int
puffs_req_handle(struct puffs_getreq *pgr, struct puffs_putreq *ppr, int maxops)
{
	struct puffs_usermount *pu;
	struct puffs_req *preq;
	int pval;

	assert(pgr->pgr_pu == ppr->ppr_pu);
	pu = pgr->pgr_pu;

	puffs_req_setmaxget(pgr, maxops);
	if (puffs_req_loadget(pgr) == -1)
		return -1;

	/* interlink pgr and ppr for diagnostic asserts */
	pgr->pgr_nppr++;
	ppr->ppr_pgr = pgr;

	pval = 0;
	while ((preq = puffs_req_get(pgr)) != NULL
	    && puffs_getstate(pu) != PUFFS_STATE_UNMOUNTED)
		pval = puffs_dopreq(pu, preq, ppr);

	return pval;
}
