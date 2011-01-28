/* $NetBSD: quota1_subr.c,v 1.1.2.1 2011/01/28 23:31:16 bouyer Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: quota1_subr.c,v 1.1.2.1 2011/01/28 23:31:16 bouyer Exp $");

#include <sys/types.h>
#include <machine/limits.h>

#include <ufs/ufs/quota2.h>
#include <ufs/ufs/quota1.h>

static uint64_t
dqblk2q2e_limit(uint32_t lim)
{
	if (lim == 0)
		return UQUAD_MAX;
	else if (lim == 1)
		return 0;
	else
		return lim;
}

static uint32_t
q2e2dqblk_limit(uint64_t lim)
{
	if (lim == UQUAD_MAX)
		return 0;
	else if (lim == 0)
		return 1;
	else
		return lim;
}

void
dqblk2q2e(const struct dqblk *dqblk, struct quota2_entry *q2e)
{
	q2e->q2e_val[Q2V_BLOCK].q2v_hardlimit =
	    dqblk2q2e_limit(dqblk->dqb_bhardlimit);
	q2e->q2e_val[Q2V_BLOCK].q2v_softlimit =
	    dqblk2q2e_limit(dqblk->dqb_bsoftlimit);
	q2e->q2e_val[Q2V_BLOCK].q2v_cur       = dqblk->dqb_curblocks;
	q2e->q2e_val[Q2V_BLOCK].q2v_time      = dqblk->dqb_btime;

	q2e->q2e_val[Q2V_FILE].q2v_hardlimit =
	    dqblk2q2e_limit(dqblk->dqb_ihardlimit);
	q2e->q2e_val[Q2V_FILE].q2v_softlimit =
	    dqblk2q2e_limit(dqblk->dqb_isoftlimit);
	q2e->q2e_val[Q2V_FILE].q2v_cur       = dqblk->dqb_curinodes;
	q2e->q2e_val[Q2V_FILE].q2v_time      = dqblk->dqb_itime;
}

void
q2e2dqblk(const struct quota2_entry *q2e, struct dqblk *dqblk)
{
	dqblk->dqb_bhardlimit =
	    q2e2dqblk_limit(q2e->q2e_val[Q2V_BLOCK].q2v_hardlimit);
	dqblk->dqb_bsoftlimit =
	    q2e2dqblk_limit(q2e->q2e_val[Q2V_BLOCK].q2v_softlimit);
	dqblk->dqb_curblocks  = q2e->q2e_val[Q2V_BLOCK].q2v_cur;
	dqblk->dqb_btime      = q2e->q2e_val[Q2V_BLOCK].q2v_time;

	dqblk->dqb_ihardlimit =
	    q2e2dqblk_limit(q2e->q2e_val[Q2V_FILE].q2v_hardlimit);
	dqblk->dqb_isoftlimit =
	    q2e2dqblk_limit(q2e->q2e_val[Q2V_FILE].q2v_softlimit);
	dqblk->dqb_curinodes  = q2e->q2e_val[Q2V_FILE].q2v_cur;
	dqblk->dqb_itime      = q2e->q2e_val[Q2V_FILE].q2v_time;
}
