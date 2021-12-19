/*	$NetBSD: linux_notifier.c,v 1.2 2021/12/19 11:47:08 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_notifier.c,v 1.2 2021/12/19 11:47:08 riastradh Exp $");

#include <sys/types.h>

#include <sys/pserialize.h>
#include <sys/xcall.h>

#include <linux/notifier.h>
#include <linux/spinlock.h>

void
ATOMIC_INIT_NOTIFIER_HEAD(struct atomic_notifier_head *H)
{

	spin_lock_init(&H->anh_lock);
	PSLIST_INIT(&H->anh_list);
}

void
ATOMIC_CLEANUP_NOTIFIER_HEAD(struct atomic_notifier_head *H)
{

	PSLIST_DESTROY(&H->anh_list);
	spin_lock_destroy(&H->anh_lock);
}

void
atomic_notifier_chain_register(struct atomic_notifier_head *H,
    struct notifier_block *B)
{

	spin_lock(&H->anh_lock);
	PSLIST_WRITER_INSERT_HEAD(&H->anh_list, B, nb_entry);
	spin_unlock(&H->anh_lock);
}

void
atomic_notifier_chain_unregister(struct atomic_notifier_head *H,
    struct notifier_block *B)
{

	spin_lock(&H->anh_lock);
	PSLIST_WRITER_REMOVE(B, nb_entry);
	spin_unlock(&H->anh_lock);

	xc_barrier(0);
}

void
atomic_notifier_call_chain(struct atomic_notifier_head *H,
    unsigned long arg0, void *arg1)
{
	struct notifier_block *B;
	int s;

	s = pserialize_read_enter();
	PSLIST_READER_FOREACH(B, &H->anh_list, struct notifier_block, nb_entry)
	{
		if ((*B->notifier_call)(B, arg0, arg1) == NOTIFY_DONE)
			break;
	}
	pserialize_read_exit(s);
}
