/* 	$NetBSD: ufs_trans.c,v 1.1.2.1 2007/03/30 00:11:01 mjf Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#if defined(_KERNEL_OPT)
#include "opt_ufs_trans.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_trans.h>

POOL_INIT(ufs_transchunk_pool, sizeof(struct ufs_trans_chunk), 0, 0, 0, 
    "ufstranschnkpl", &pool_allocator_nointr, IPL_NONE);

static void ufs_trans_free(struct ufs_trans *);

/*
 * Initialize UFS transaction subsystem. 
 *
 * NOTE: This has nothing to do with initializing a 'specific' transaction,
 * it merely initializes the transaction subsystem.
 *
 */
void
ufs_trans_init(struct ufsmount *ump)
{
	ump->um_journalcookie = 0;
}

#define UFS_TRANS_EMPTY(trans)	TAILQ_EMPTY(&trans->t_head)

/*
 * Begin a transaction.
 *
 */
struct ufs_trans *
ufs_trans_begin(intptr_t cookie)
{
	struct ufs_trans *tp;

	if (__predict_false(cookie == 0))
		panic("no ufs journal registered");

	tp = malloc(sizeof(*tp), M_TEMP, M_WAITOK);

	TAILQ_INIT(&tp->t_head);

	return tp;
}

/*
 * Commit a (possibly batch) transaction to the journal.
 */
int
ufs_trans_commit(struct ufs_trans *tp, intptr_t cookie)
{
	int error;
	struct gen_journal *journal;

	/* 
	 * XXX Ensure that the journal has not been unregistered
	 * from underneath us.
	 */
	if (__predict_false(cookie == 0))
		panic("no journal: cannot commit");

	journal = (struct gen_journal *)cookie;
	error = journal->gj_commit(tp, journal);

	/* We are now done with these data structures */
	ufs_trans_free(tp);
	return error;
}

/*
 * This is just a wrapper for the ufs_trans_add, ufs_trans_commit process.
 * It should only really be used when we don't want to batch transactions.
 */
int
ufs_trans_single_commit(struct ufs_trans *trans, intptr_t cookie,
			struct buf *bp)
{
	int error = 0;

	if ((error = ufs_trans_add(trans, bp)) != 0)
		return error;

	error = ufs_trans_commit(trans, cookie);
	return error;
}

/*
 * Add a transaction chunk to the transaction 'tp'.
 */
int
ufs_trans_add(struct ufs_trans *tp, struct buf *bp)
{
	struct ufs_trans_chunk *tc;

	tc = pool_get(&ufs_transchunk_pool, PR_WAITOK);

	if (__predict_false(tc == NULL))
		panic("could not get resource from transchunk pool");

	tc->tc_data = bp;

	/* 
	 * XXX Lock this buf so that it doesn't get released 
	 * underneath us. It is the responsibility of the journal
	 * commit function to release these bufs
	 */
	SET(tc->tc_data->b_flags, B_LOCKED);

	TAILQ_INSERT_TAIL(&tp->t_head, tc, tc_entries);

	return 0;
}

/*
 * XXX This may want removing, the whole idea of being able to abort
 * a transaction does not bode well with my gut.
 *
 * This function should be used by functions that beging and commit a
 * transaction. It is possible that, having passed a transaction pointer
 * to all the functions that may need it, there will be no valid data
 * in the transaction, i.e. nothing needs to be updated on disk. This can
 * be detected wih the UFS_TRANS_EMPTY macro above. If the transaction is
 * empty this function provides a clean way to destroy the data structures
 * that have been allocated.
 */
void
ufs_trans_abort(struct ufs_trans *trans)
{
	free(trans, M_TEMP);
	trans = NULL;
}

/*
 * Register a journal with a UFS mount point.
 *
 * Note, only one journal is allowed to be active for a single mount
 * point at any one time. The journal in effect will be the journal 
 * that was supplied to the last call to ufs_register_journal().
 *
 * cookie is the journal cookie
 */
int
ufs_register_journal(struct ufsmount *ump, intptr_t cookie)
{
	ump->um_journalcookie = cookie;
	return 0;
}

/*
 * Unregister a journal for the mount point 'ump'. We also
 * free the memory allocated to the journal.
 */
void
ufs_unregister_journal(struct ufsmount *ump)
{
	struct gen_journal *journal;

	journal = (struct gen_journal *)(intptr_t)ump->um_journalcookie;

	free(journal, M_TEMP);
	ump->um_journalcookie = 0;
}

/*
 * Clean up the UFS transaction subsystem.
 * 
 * NOTE: This has nothing to do with a specific transaction.
 * Unregisters the journal from the mount point 'mp'.
 */
void
ufs_trans_done(struct ufsmount *ump)
{
	/* XXX We would want to commit any outstanding trans */

	if (ump->um_journalcookie != 0)
		ufs_unregister_journal(ump);
}

/*
 * Free all data associated with a transaction.
 */
static void
ufs_trans_free(struct ufs_trans *tp)
{
	struct ufs_trans_chunk *cp;

	/* Return the memory to the pool */
	TAILQ_FOREACH(cp, &tp->t_head, tc_entries)
		pool_put(&ufs_transchunk_pool, cp);

	TAILQ_REMOVE(&tp->t_head, cp, tc_entries);

	/* Free the memory for the transaction */
	free(tp, M_TEMP);
}
