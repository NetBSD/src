/*	$NetBSD: misc.c,v 1.5 2009/07/13 17:57:35 christos Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas
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
__RCSID("$NetBSD: misc.c,v 1.5 2009/07/13 17:57:35 christos Exp $");

#include <stdbool.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/condvar.h>
#include <sys/selinfo.h>
#include <sys/filedesc.h>
#define _KERNEL
#include <sys/mqueue.h>
#include <sys/eventvar.h>
#undef _KERNEL
#include <sys/proc.h>
#define _KERNEL
#include <sys/file.h>
#undef _KERNEL
#include <sys/vnode.h>
#include <sys/mount.h>

#include <net/bpfdesc.h>

#include <err.h>
#include <kvm.h>
#include "fstat.h"

static struct nlist nl[] = {
#define NL_KQUEUE	0
    { .n_name = "kqueueops" },
#define NL_MQUEUE	1
    { .n_name = "mqops" },
#define NL_PIPE		2
    { .n_name = "pipeops" },
#define NL_SOCKET	3
    { .n_name = "socketops" },
#define NL_VNOPS	4
    { .n_name = "vnops" },
#define NL_CRYPTO	5
    { .n_name = "cryptofops" },
#define NL_BPF		6
    { .n_name = "bpf_fileops", },
#define NL_TAP		7
    { .n_name = "tap_fileops", },
#define NL_MAX		8
    { .n_name = NULL }
};

extern int vflg;


static int
p_bpf(struct file *f)
{
	struct bpf_d bpf;

	if (!KVM_READ(f->f_data, &bpf, sizeof (bpf))) {
		dprintf("can't read bpf at %p for pid %d", f->f_data, Pid);
		return 0;
	}
	(void)printf("* bpf rec=%lu, dr=%lu, cap=%lu, pid=%lu",
	    bpf.bd_rcount, bpf.bd_dcount, bpf.bd_ccount,
	    (unsigned long)bpf.bd_pid);
	if (bpf.bd_promisc)
		(void)printf(", promisc");
	if (bpf.bd_immediate)
		(void)printf(", immed");
	if (bpf.bd_seesent)
		(void)printf(", seesent");
	if (bpf.bd_async)
		(void)printf(", asyncgrp=%lu", (unsigned long)bpf.bd_pgid);
	if (bpf.bd_state == BPF_IDLE)
		(void)printf(", idle");
	else if (bpf.bd_state == BPF_WAITING)
		(void)printf(", waiting");
	else if (bpf.bd_state == BPF_TIMED_OUT)
		(void)printf(", timeout");
	(void)printf("\n");
	return 0;
}

static int
p_mqueue(struct file *f)
{
	struct mqueue mq;

	if (!KVM_READ(f->f_data, &mq, sizeof (mq))) {
		dprintf("can't read mqueue at %p for pid %d", f->f_data, Pid);
		return 0;
	}
	(void)printf("* mqueue \"%s\"\n", mq.mq_name);
	return 0;
}

static int
p_kqueue(struct file *f)
{
	struct kqueue kq;

	if (!KVM_READ(f->f_data, &kq, sizeof (kq))) {
		dprintf("can't read kqueue at %p for pid %d", f->f_data, Pid);
		return 0;
	}
	(void)printf("* kqueue pending %d\n", kq.kq_count);
	return 0;
}

int
pmisc(struct file *f, const char *name)
{
	size_t i;
	if (nl[0].n_value == 0) {
		int n;
		if ((n = KVM_NLIST(nl)) == -1)
			errx(1, "Cannot list kernel symbols (%s)",
			    KVM_GETERR());
		else if (n != 0 && vflg)
			warnx("Could not find %d symbols", n);
	}
	for (i = 0; i < NL_MAX; i++)
		if ((uintptr_t)f->f_ops == nl[i].n_value)
			break;
	switch (i) {
	case NL_BPF:
		return p_bpf(f);
	case NL_MQUEUE:
		return p_mqueue(f);
	case NL_KQUEUE:
		return p_kqueue(f);
	case NL_TAP:
		printf("* tap %lu\n", (unsigned long)(intptr_t)f->f_data);
		return 0;
	case NL_CRYPTO:
		printf("* crypto %p\n", f->f_data);
		return 0;
	default:
		printf("* %s %p\n", name, f->f_data);
		return 0;
	}
}
