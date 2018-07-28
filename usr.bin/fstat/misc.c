/*	$NetBSD: misc.c,v 1.17.12.2 2018/07/28 04:38:13 pgoyette Exp $	*/

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
__RCSID("$NetBSD: misc.c,v 1.17.12.2 2018/07/28 04:38:13 pgoyette Exp $");

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
#define copyout_t int
#include <sys/ksem.h>
#define _LIB_LIBKERN_LIBKERN_H_
#define mutex_enter(a)
#define mutex_exit(a)
#undef _KERNEL
#include <sys/cprng.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#include <net/bpfdesc.h>

#include <err.h>
#include <util.h>
#include <string.h>
#include <kvm.h>
#include "fstat.h"

static struct nlist nl[] = {
#define NL_BPF		0
    { .n_name = "bpf_fileops", },
#define NL_CRYPTO	1
    { .n_name = "cryptofops" },
#define NL_DMIO		2
    { .n_name = "dmio_fileops", },
#define NL_DRVCTL	3
    { .n_name = "drvctl_fileops", },
#define NL_DTV_DEMUX	4
    { .n_name = "dtv_demux_fileops", },
#define NL_FILEMON	5
    { .n_name = "filemon_fileops", },
#define NL_KQUEUE	6
    { .n_name = "kqueueops" },
#define NL_MQUEUE	7
    { .n_name = "mqops" },
#define NL_PIPE		8
    { .n_name = "pipeops" },
#define NL_PUTTER	9
    { .n_name = "putter_fileops", },
#define NL_RND		10
    { .n_name = "rnd_fileops", },
#define NL_SEM		11
    { .n_name = "semops", },
#define NL_SOCKET	12
    { .n_name = "socketops" },
#define NL_SVR4_NET	13
    { .n_name = "svr4_netops" },
#define NL_SVR4_32_NET	14
    { .n_name = "svr4_32_netops" },
#define NL_TAP		15
    { .n_name = "tap_fileops", },
#define NL_VNOPS	16
    { .n_name = "vnops" },
#define NL_XENEVT	17
    { .n_name = "xenevt_fileops" },
#define NL_AUDIO	18
    { .n_name = "audio_fileops" },
#define NL_PAD		19
    { .n_name = "pad_fileops" },
#define NL_MAX		20
    { .n_name = NULL }
};

extern int vflg;


static int
p_bpf(struct file *f)
{
	struct bpf_d bpf;
	struct bpf_if bi;
	struct ifnet ifn;

	strlcpy(ifn.if_xname, "???", sizeof(ifn.if_xname));

	if (!KVM_READ(f->f_data, &bpf, sizeof(bpf))) {
		dprintf("can't read bpf at %p for pid %d", f->f_data, Pid);
		return 0;
	}
	if (bpf.bd_bif != NULL) {
		if (!KVM_READ(bpf.bd_bif, &bi, sizeof(bi)))
			dprintf("can't read bpf interface at %p for pid %d",
			    bpf.bd_bif, Pid);
		if (bi.bif_ifp != NULL)
			if (!KVM_READ(bi.bif_ifp, &ifn, sizeof(ifn)))
				dprintf("can't read net interfsace"
				    " at %p for pid %d", bi.bif_ifp, Pid);
	}
	(void)printf("* bpf@%s rec=%lu, dr=%lu, cap=%lu, pid=%lu", ifn.if_xname,
	    bpf.bd_rcount, bpf.bd_dcount, bpf.bd_ccount,
	    (unsigned long)bpf.bd_pid);
	if (bpf.bd_promisc)
		(void)printf(", promisc");
	if (bpf.bd_immediate)
		(void)printf(", immed");
	if (bpf.bd_direction == BPF_D_IN)
		(void)printf(", in");
	else if (bpf.bd_direction == BPF_D_INOUT)
		(void)printf(", inout");
	else if (bpf.bd_direction == BPF_D_OUT)
		(void)printf(", out");
	if (bpf.bd_jitcode != NULL)
		(void)printf(", jit");
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
p_sem(struct file *f)
{
	ksem_t ks;
	if (!KVM_READ(f->f_data, &ks, sizeof(ks))) {
		dprintf("can't read sem at %p for pid %d", f->f_data, Pid);
		return 0;
	}
	(void)printf("* ksem ref=%u, value=%u, waiters=%u, flags=0x%x, "
	    "mode=%o, uid=%u, gid=%u", ks.ks_ref, ks.ks_value, ks.ks_waiters,
	    ks.ks_flags, ks.ks_mode, ks.ks_uid, ks.ks_gid);
	if (ks.ks_name && ks.ks_namelen) {
		char buf[64];
		if (ks.ks_namelen >= sizeof(buf))
			ks.ks_namelen = sizeof(buf) - 1;
		if (!KVM_READ(ks.ks_name, buf, ks.ks_namelen)) {
			dprintf("can't read sem name at %p for pid %d",
			    ks.ks_name, Pid);
		} else {
			buf[ks.ks_namelen] = '\0';
			(void)printf(", name=%s\n", buf);
			return 0;
		}
	}
	(void)printf("\n");
	return 0;
}

static int
p_mqueue(struct file *f)
{
	struct mqueue mq;

	if (!KVM_READ(f->f_data, &mq, sizeof(mq))) {
		dprintf("can't read mqueue at %p for pid %d", f->f_data, Pid);
		return 0;
	}
	(void)printf("* mqueue \"%s\"\n", mq.mq_name);
	return 0;
}

static int
p_rnd(struct file *f)
{
	struct cprng_strong {
		char cs_name[16];
		int  cs_flags;
		/*...*/
	} str;
	struct rnd_ctx {
		struct cprng_strong *rc_cprng;
		bool rc_hard;
	} ctx;
	char buf[1024];

	if (!KVM_READ(f->f_data, &ctx, sizeof(ctx))) {
		dprintf("can't read rnd_ctx at %p for pid %d", f->f_data, Pid);
		return 0;
	}
	if (!KVM_READ(ctx.rc_cprng, &str, sizeof(str))) {
		dprintf("can't read cprng_strong at %p for pid %d", f->f_data,\
		    Pid);
		return 0;
	}
	snprintb(buf, sizeof(buf), CPRNG_FMT, str.cs_flags);
	(void)printf("* rnd \"%s\" flags %s\n", str.cs_name, buf);
	return 0;
}

static int
p_kqueue(struct file *f)
{
	struct kqueue kq;

	if (!KVM_READ(f->f_data, &kq, sizeof(kq))) {
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
		else if (n != 0 && vflg) {
			char buf[1024];
			buf[0] = '\0';
			for (struct nlist *l = nl; l->n_name != NULL; l++) {
				if (l->n_value != 0)
					continue;
				strlcat(buf, ", ", sizeof(buf));
				strlcat(buf, l->n_name, sizeof(buf));
			}
			warnx("Could not find %d symbols: %s", n, buf + 2);
		}
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
	case NL_RND:
		return p_rnd(f);
	case NL_SEM:
		return p_sem(f);
	case NL_TAP:
		printf("* tap %lu\n", (unsigned long)(intptr_t)f->f_data);
		return 0;
	case NL_CRYPTO:
		printf("* crypto %p\n", f->f_data);
		return 0;
	case NL_AUDIO:
		printf("* audio %p\n", f->f_data);
		return 0;
	case NL_PAD:
		printf("* pad %p\n", f->f_data);
		return 0;
	case NL_MAX:
		printf("* %s ops=%p %p\n", name, f->f_ops, f->f_data);
		return 0;
	default:
		printf("* %s %p\n", nl[i].n_name, f->f_data);
		return 0;
	}
}
