/* $NetBSD: xen_ipi.c,v 1.1.2.1 2011/06/03 13:27:41 cherry Exp $ */

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
 * __KERNEL_RCSID(0, "$NetBSD: xen_ipi.c,v 1.1.2.1 2011/06/03 13:27:41 cherry Exp $"); 
 */

__KERNEL_RCSID(0, "$NetBSD: xen_ipi.c,v 1.1.2.1 2011/06/03 13:27:41 cherry Exp $");

#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/xcall.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/fpu.h>
#include <machine/frame.h>

#include <xen/intr.h>
#include <xen/intrdefs.h>
#include <xen/hypervisor.h>
#include <xen/xen3-public/vcpu.h>

extern void ddb_ipi(struct trapframe);

static void xen_ipi_halt(struct cpu_info *, struct intrframe *);
static void xen_ipi_synch_fpu(struct cpu_info *, struct intrframe *);
static void xen_ipi_ddb(struct cpu_info *, struct intrframe *);
static void xen_ipi_xcall(struct cpu_info *, struct intrframe *);

static void (*ipifunc[XEN_NIPIS])(struct cpu_info *, struct intrframe *) =
{	/* In order of priority (see: xen/include/intrdefs.h */
	xen_ipi_halt,
	xen_ipi_synch_fpu,
	xen_ipi_ddb,
	xen_ipi_xcall
};

static void
xen_ipi_handler(struct cpu_info *ci, struct intrframe *regs)
{
	uint32_t pending;
	int bit;

	pending = atomic_swap_32(&ci->ci_ipis, 0);

	KDASSERT((pending >> XEN_NIPIS) == 0);
	while ((bit = ffs(pending)) != 0) {
		bit--;
		pending &= ~(1 << bit);
		ci->ci_ipi_events[bit].ev_count++;
		if (ipifunc[bit] != NULL) {
			(*ipifunc[bit])(ci, regs);
		}
		else {
			panic("ipifunc[%d] unsupported!\n", bit);
			/* NOTREACHED */
		}
	}

	return;
}

/* Must be called once for every cpu that expects to send/recv ipis */
void
xen_ipi_init(void)
{
	cpuid_t vcpu;
	evtchn_port_t evtchn;
	struct cpu_info *ci;

	ci = curcpu();

	vcpu = ci->ci_cpuid;
	KASSERT(vcpu < MAX_VIRT_CPUS);

	evtchn = ci->ci_ipi_evtchn = bind_vcpu_to_evtch(vcpu);

	KASSERT(evtchn != -1 && evtchn < NR_EVENT_CHANNELS);

	if (0 != event_set_handler(evtchn, (int (*)(void *))xen_ipi_handler,
				   ci, IPL_HIGH, "ipi")) {
		panic("event_set_handler(...) KPI violation\n");
		/* NOTREACHED */
	}

	hypervisor_enable_event(evtchn);
	return;
}

/* prefer this to global variable */
static inline u_int max_cpus(void)
{
	return maxcpus;
}

static inline bool /* helper */
valid_ipimask(uint32_t ipimask)
{
	uint32_t masks =  XEN_IPI_XCALL | XEN_IPI_DDB | 
		XEN_IPI_SYNCH_FPU | XEN_IPI_HALT | 
		XEN_IPI_KICK;

	if (ipimask & ~masks) {
		return false;
	}
	else {
		return true;
	}

}

int
xen_send_ipi(struct cpu_info *ci, uint32_t ipimask)
{
	evtchn_port_t evtchn;

	KASSERT(ci != NULL || ci != curcpu());

	if (!(ci->ci_flags & CPUF_RUNNING)) {
		return ENOENT;
	}

	evtchn = ci->ci_ipi_evtchn;
	if (false == valid_ipimask(ipimask)) {
		panic("xen_send_ipi() called with invalid ipimask\n");
		/* NOTREACHED */
	}

	atomic_or_32(&ci->ci_ipis, ipimask);
	hypervisor_notify_via_evtchn(evtchn);

	return 0;
}

void
xen_broadcast_ipi(uint32_t ipimask)
{
	struct cpu_info *ci, *self = curcpu();
	CPU_INFO_ITERATOR cii;

	if (false == valid_ipimask(ipimask)) {
		panic("xen_broadcast_ipi() called with invalid ipimask\n");
		/* NOTREACHED */
	}

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

	return;
	/* NOTREACHED */
}

/* MD wrapper for the xcall(9) callback. */
#define PRIuCPUID	"lu" /* XXX: move this somewhere more appropriate */

static void
xen_ipi_halt(struct cpu_info *ci, struct intrframe *intrf)
{
	KASSERT(ci == curcpu());
	KASSERT(ci != NULL);
	if (HYPERVISOR_vcpu_op(VCPUOP_down, ci->ci_cpuid, NULL)) {
		panic("vcpu%" PRIuCPUID "shutdown failed.\n", ci->ci_cpuid);
	}

	return;
}

static void
xen_ipi_synch_fpu(struct cpu_info *ci, struct intrframe *intrf)
{
	KASSERT(ci != NULL);
	KASSERT(intrf != NULL);

	fpusave_cpu(true);
	return;
}

static void
xen_ipi_ddb(struct cpu_info *ci, struct intrframe *intrf)
{
	KASSERT(ci != NULL);
	KASSERT(intrf != NULL);

	ddb_ipi(intrf->if_tf);
}

static void
xen_ipi_xcall(struct cpu_info *ci, struct intrframe *intrf)
{
	KASSERT(ci != NULL);
	KASSERT(intrf != NULL);

	xc_ipi_handler();
	return;
}

void
xc_send_ipi(struct cpu_info *ci)
{

	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);
	printf("xc_send_ipi called \n");
	if (ci) {
		if (0 != xen_send_ipi(ci, XEN_IPI_XCALL)) {
			panic("xen_send_ipi(XEN_IPI_XCALL) failed\n");
		};
	} else {
		xen_broadcast_ipi(XEN_IPI_XCALL);
	}
}
