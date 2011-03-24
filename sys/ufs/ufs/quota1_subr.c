/* $NetBSD: quota1_subr.c,v 1.3 2011/03/24 17:05:45 bouyer Exp $ */
/*-
  * Copyright (c) 2010 Manuel Bouyer
  * All rights reserved.
  * This software is distributed under the following condiions
  * compliant with the NetBSD foundation policy.
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
  * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
  * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
  * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
  * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  * POSSIBILITY OF SUCH DAMAGE.
  */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: quota1_subr.c,v 1.3 2011/03/24 17:05:45 bouyer Exp $");

#include <sys/types.h>
#include <machine/limits.h>

#include <quota/quotaprop.h>
#include <ufs/ufs/quota1.h>

static uint64_t
dqblk2q2e_limit(uint32_t lim)
{
	if (lim == 0)
		return UQUAD_MAX;
	else
		return (lim - 1);
}

static uint32_t
q2e2dqblk_limit(uint64_t lim)
{
	if (lim == UQUAD_MAX)
		return 0;
	else
		return (lim + 1);
}

void
dqblk2ufsqe(const struct dqblk *dqblk, struct ufs_quota_entry *qe)
{
	qe[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit =
	    dqblk2q2e_limit(dqblk->dqb_bhardlimit);
	qe[QUOTA_LIMIT_BLOCK].ufsqe_softlimit =
	    dqblk2q2e_limit(dqblk->dqb_bsoftlimit);
	qe[QUOTA_LIMIT_BLOCK].ufsqe_cur       = dqblk->dqb_curblocks;
	qe[QUOTA_LIMIT_BLOCK].ufsqe_time      = dqblk->dqb_btime;

	qe[QUOTA_LIMIT_FILE].ufsqe_hardlimit =
	    dqblk2q2e_limit(dqblk->dqb_ihardlimit);
	qe[QUOTA_LIMIT_FILE].ufsqe_softlimit =
	    dqblk2q2e_limit(dqblk->dqb_isoftlimit);
	qe[QUOTA_LIMIT_FILE].ufsqe_cur       = dqblk->dqb_curinodes;
	qe[QUOTA_LIMIT_FILE].ufsqe_time      = dqblk->dqb_itime;
}

void
ufsqe2dqblk(const struct ufs_quota_entry *qe, struct dqblk *dqblk)
{
	dqblk->dqb_bhardlimit =
	    q2e2dqblk_limit(qe[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit);
	dqblk->dqb_bsoftlimit =
	    q2e2dqblk_limit(qe[QUOTA_LIMIT_BLOCK].ufsqe_softlimit);
	dqblk->dqb_curblocks  = qe[QUOTA_LIMIT_BLOCK].ufsqe_cur;
	dqblk->dqb_btime      = qe[QUOTA_LIMIT_BLOCK].ufsqe_time;

	dqblk->dqb_ihardlimit =
	    q2e2dqblk_limit(qe[QUOTA_LIMIT_FILE].ufsqe_hardlimit);
	dqblk->dqb_isoftlimit =
	    q2e2dqblk_limit(qe[QUOTA_LIMIT_FILE].ufsqe_softlimit);
	dqblk->dqb_curinodes  = qe[QUOTA_LIMIT_FILE].ufsqe_cur;
	dqblk->dqb_itime      = qe[QUOTA_LIMIT_FILE].ufsqe_time;
}

