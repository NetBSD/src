/* $NetBSD: xen_ipi.c,v 1.26 2018/07/24 12:26:14 bouyer Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cherry G. Mathew <cherry@zyx.in>
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

#include <sys/cdefs.h>			/* RCS ID macro */

/* 
 * Based on: x86/ipi.c
 * __KERNEL_RCSID(0, "$NetBSD: xen_ipi.c,v 1.26 2018/07/24 12:26:14 bouyer Exp $");
 */

__KERNEL_RCSID(0, "$NetBSD: xen_ipi.c,v 1.26 2018/07/24 12:26:14 bouyer Exp $");

#include "opt_ddb.h"

#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/mutex.h>
#include <sys/device.h>
#include <sys/xcall.h>
#include <sys/ipi.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <x86/fpu.h>
#include <machine/frame.h>
#include <machine/segments.h>

#include <xen/evtchn.h>
#include <xen/intr.h>
#include <xen/intrdefs.h>
#include <xen/hypervisor.h>
#include <xen/xen-public/vcpu.h>

#ifdef DDB
extern void ddb_ipi(struct trapframe);
static void xen_ipi_ddb(struct cpu_info *, struct intrframe *);
#endif

static void xen_ipi_halt(struct cpu_info *, struct intrframe *);
static void xen_ipi_synch_fpu(struct cpu_info *, struct intrframe *);
static void xen_ipi_xcall(struct cpu_info *, struct intrframe *);
static void xen_ipi_hvcb(struct cpu_info *, struct intrframe *);
static void xen_ipi_generic(struct cpu_info *, struct intrframe *);

static void (*ipifunc[XEN_NIPIS])(struct cpu_info *, struct intrframe *) =
{	/* In order of priority (see: xen/include/intrdefs.h */
	xen_ipi_halt,
	xen_ipi_synch_fpu,
#ifdef DDB
	xen_ipi_ddb,
#else
	NULL,
#endif
	xen_ipi_xcall,
	xen_ipi_hvcb,
	xen_ipi_generic,
};

static int
xen_ipi_handler(void *arg)
{
	uint32_t pending;
	int bit;
	struct cpu_info *ci;
	struct intrframe *regs;

	ci = curcpu();
	regs = arg;
	
	pending = atomic_swap_32(&ci->ci_ipis, 0);

	KDASSERT((pending >> XEN_NIPIS) == 0);
	while ((bit = ffs(pending)) != 0) {
		bit--;
		pending &= ~(1 << bit);
		ci->ci_ipi_events[bit].ev_count++;
		if (ipifunc[bit] != NULL) {
			(*ipifunc[bit])(ci, regs);
		} else {
			panic("ipifunc[%d] unsupported!\n", bit);
			/* NOTREACHED */
		}
	}

	return 0;
}

/* Must be called once for every cpu that expects to send/recv ipis */
void
xen_ipi_init(void)
{
	cpuid_t vcpu;
	evtchn_port_t evtchn;
	struct cpu_info *ci;
	char intr_xname[INTRDEVNAMEBUF];

	ci = curcpu();

	vcpu = ci->ci_cpuid;
	KASSERT(vcpu < XEN_LEGACY_MAX_VCPUS);

	evtchn = bind_vcpu_to_evtch(vcpu);
	ci->ci_ipi_evtchn = evtchn;

	KASSERT(evtchn != -1 && evtchn < NR_EVENT_CHANNELS);

	snprintf(intr_xname, sizeof(intr_xname), "%s ipi",
	    device_xname(ci->ci_dev));

	if (intr_establish_xname(0, &xen_pic, evtchn, IST_LEVEL, IPL_HIGH,
		xen_ipi_handler, ci, true, intr_xname) == NULL) {
		panic("%s: unable to register ipi handler\n", __func__);
		/* NOTREACHED */
	}

	hypervisor_enable_event(evtchn);
}

#ifdef DIAGNOSTIC
static inline bool /* helper */
valid_ipimask(uint32_t ipimask)
{
	uint32_t masks = XEN_IPI_GENERIC | XEN_IPI_HVCB | XEN_IPI_XCALL |
		 XEN_IPI_DDB | XEN_IPI_SYNCH_FPU |
		 XEN_IPI_HALT | XEN_IPI_KICK;

	if (ipimask & ~masks) {
		return false;
	} else {
		return true;
	}

}
#endif

int
xen_send_ipi(struct cpu_info *ci, uint32_t ipimask)
{
	evtchn_port_t evtchn;

	KASSERT(ci != NULL && ci != curcpu());

	if ((ci->ci_flags & CPUF_RUNNING) == 0) {
		return ENOENT;
	}

	evtchn = ci->ci_ipi_evtchn;

	KASSERTMSG(valid_ipimask(ipimask) == true, 
		"xen_send_ipi() called with invalid ipimask\n");

	atomic_or_32(&ci->ci_ipis, ipimask);
	hypervisor_notify_via_evtchn(evtchn);

	return 0;
}

void
xen_broadcast_ipi(uint32_t ipimask)
{
	struct cpu_info *ci, *self = curcpu();
	CPU_INFO_ITERATOR cii;

	KASSERTMSG(valid_ipimask(ipimask) == true, 
		"xen_broadcast_ipi() called with invalid ipimask\n");

	/* 
	 * XXX-cherry: there's an implicit broadcast sending order
	 * which I dislike. Randomise this ? :-)
	 */

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == NULL)
			continue;
		if (ci == self)
			continue;
		if (ci->ci_data.cpu_idlelwp == NULL)
			continue;
		if ((ci->ci_flags & CPUF_PRESENT) == 0)
			continue;
		if (ci->ci_flags & (CPUF_RUNNING)) {
			if (0 != xen_send_ipi(ci, ipimask)) {
				panic("xen_ipi of %x from %s to %s failed\n",
				      ipimask, cpu_name(curcpu()),
				      cpu_name(ci));
			}
		}
	}
}

/* MD wrapper for the xcall(9) callback. */

static void
xen_ipi_halt(struct cpu_info *ci, struct intrframe *intrf)
{
	KASSERT(ci == curcpu());
	KASSERT(ci != NULL);
	if (HYPERVISOR_vcpu_op(VCPUOP_down, ci->ci_cpuid, NULL)) {
		panic("%s shutdown failed.\n", device_xname(ci->ci_dev));
	}

}

static void
xen_ipi_synch_fpu(struct cpu_info *ci, struct intrframe *intrf)
{
	KASSERT(ci != NULL);
	KASSERT(intrf != NULL);

	fpusave_cpu(true);
}

#ifdef DDB
static void
xen_ipi_ddb(struct cpu_info *ci, struct intrframe *intrf)
{
	KASSERT(ci != NULL);
	KASSERT(intrf != NULL);

#ifdef __x86_64__
	ddb_ipi(intrf->if_tf);
#else
	struct trapframe tf;
	tf.tf_gs = intrf->if_gs;
	tf.tf_fs = intrf->if_fs;
	tf.tf_es = intrf->if_es;
	tf.tf_ds = intrf->if_ds;
	tf.tf_edi = intrf->if_edi;
	tf.tf_esi = intrf->if_esi;
	tf.tf_ebp = intrf->if_ebp;
	tf.tf_ebx = intrf->if_ebx;
	tf.tf_ecx = intrf->if_ecx;
	tf.tf_eax = intrf->if_eax;
	tf.tf_trapno = intrf->__if_trapno;
	tf.tf_err = intrf->__if_err;
	tf.tf_eip = intrf->if_eip;
	tf.tf_cs = intrf->if_cs;
	tf.tf_eflags = intrf->if_eflags;
	tf.tf_esp = intrf->if_esp;
	tf.tf_ss = intrf->if_ss;

	ddb_ipi(tf);
#endif
}
#endif /* DDB */

static void
xen_ipi_xcall(struct cpu_info *ci, struct intrframe *intrf)
{
	KASSERT(ci != NULL);
	KASSERT(intrf != NULL);

	xc_ipi_handler();
}

void
xc_send_ipi(struct cpu_info *ci)
{

	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);
	if (ci) {
		if (0 != xen_send_ipi(ci, XEN_IPI_XCALL)) {
			panic("xen_send_ipi(XEN_IPI_XCALL) failed\n");
		}
	} else {
		xen_broadcast_ipi(XEN_IPI_XCALL);
	}
}

static void
xen_ipi_generic(struct cpu_info *ci, struct intrframe *intrf)
{
	KASSERT(ci != NULL);
	KASSERT(intrf != NULL);

	ipi_cpu_handler();
}

void
cpu_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);
	if (ci) {
		if (0 != xen_send_ipi(ci, XEN_IPI_GENERIC)) {
			panic("xen_send_ipi(XEN_IPI_GENERIC) failed\n");
		}
	} else {
		xen_broadcast_ipi(XEN_IPI_GENERIC);
	}
}

static void
xen_ipi_hvcb(struct cpu_info *ci, struct intrframe *intrf)
{
	KASSERT(ci != NULL);
	KASSERT(intrf != NULL);
	KASSERT(ci == curcpu());
	KASSERT(!ci->ci_vcpu->evtchn_upcall_mask);

	hypervisor_force_callback();
}
