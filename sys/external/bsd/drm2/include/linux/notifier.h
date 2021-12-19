/*	$NetBSD: notifier.h,v 1.5 2021/12/19 11:47:08 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_NOTIFIER_H_
#define _LINUX_NOTIFIER_H_

#include <sys/cdefs.h>

#include <sys/pslist.h>

#include <linux/spinlock.h>

/* namespace */
#define	ATOMIC_INIT_NOTIFIER_HEAD	linux_ATOMIC_INIT_NOTIFIER_HEAD
#define	ATOMIC_CLEANUP_NOTIFIER_HEAD	linux_ATOMIC_CLEANUP_NOTIFIER_HEAD
#define	atomic_notifier_call_chain	linux_atomic_notifier_call_chain
#define	atomic_notifier_chain_register	linux_atomic_notifier_chain_register
#define	atomic_notifier_chain_unregister linux_atomic_notifier_chain_unregister

#define	NOTIFY_DONE	0
#define	NOTIFY_OK	1

struct notifier_block {
	int (*notifier_call)(struct notifier_block *, unsigned long, void *);
	struct pslist_entry	nb_entry;
};

struct atomic_notifier_head {
	spinlock_t		anh_lock;
	struct pslist_head	anh_list;
};

void ATOMIC_INIT_NOTIFIER_HEAD(struct atomic_notifier_head *);
void ATOMIC_CLEANUP_NOTIFIER_HEAD(struct atomic_notifier_head *);

void atomic_notifier_chain_register(struct atomic_notifier_head *,
    struct notifier_block *);
void atomic_notifier_chain_unregister(struct atomic_notifier_head *,
    struct notifier_block *);
void atomic_notifier_call_chain(struct atomic_notifier_head *, unsigned long,
    void *);

#endif  /* _LINUX_NOTIFIER_H_ */
