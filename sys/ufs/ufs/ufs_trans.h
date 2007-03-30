/* 	$NetBSD: ufs_trans.h,v 1.1.2.1 2007/03/30 00:11:01 mjf Exp $ */

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

#ifndef _UFS_UFS_TRANS_H_
#define _UFS_UFS_TRANS_H_

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/mutex.h>

#include <ufs/ufs/ufsmount.h>

/* 
 * This number represents the 'commit' block that is written out when
 * a transaction is safely in the journal. The commit block is the
 * last thing written out to the journal for a transaction.
 */
#define	UFS_TRANS_COMMITTED	0x19870331

#define UFS_ISDOINGTRANS(ump) (ump->um_journalcookie != 0)
	
/*
 * Transactions are composed of multiple ufs_trans_chunks, where each
 * chunk defines a specific change to some metadata block. 
 * 
 * Collections of these chunks allow updates to be batched together.
 * The suggested use for grouping is to group metadata updates that amount to
 * one logical filesystem operation, i.e. deleting a file.
 */

struct ufs_trans_chunk {
	struct buf *tc_data;
	TAILQ_ENTRY(ufs_trans_chunk) tc_entries;  /* link on chunk list */
};

/* 
 * This structure is the overall transaction. It is made up of ufs_trans_chunks
 * and represents a single logical filesystem operation.
 */
struct ufs_trans {
	TAILQ_HEAD(, ufs_trans_chunk) t_head;		/* head of chunks */
};

/* XXX This is just a place-holder for now */
struct gen_journal {
	/* commit trans to journal */
	int (*gj_commit)(struct ufs_trans *, struct gen_journal *); 
	kmutex_t gj_mutex;		/* mutex for this journal */
};

/* initialisation and finilisation for UFS transaction system */
void	ufs_trans_init(struct ufsmount *);
void	ufs_trans_done(struct ufsmount *);

/* transaction functions */
struct ufs_trans *ufs_trans_begin(intptr_t);
int	ufs_trans_commit(struct ufs_trans *, intptr_t);
int	ufs_trans_add(struct ufs_trans *, struct buf *);
int	ufs_trans_single_commit(struct ufs_trans *, intptr_t, struct buf *);
void	ufs_trans_abort(struct ufs_trans *);

/* journal functions for UFS transaction system */
int	ufs_register_journal(struct ufsmount *, intptr_t);
void	ufs_unregister_journal(struct ufsmount *);

#endif	/* !_UFS_UFS_TRANS_H_ */
