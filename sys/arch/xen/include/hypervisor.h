/*	$NetBSD: hypervisor.h,v 1.54 2022/05/25 14:35:15 bouyer Exp $	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * 
 * Communication to/from hypervisor.
 * 
 * Copyright (c) 2002-2004, K A Fraser
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */


#ifndef _XEN_HYPERVISOR_H_
#define _XEN_HYPERVISOR_H_

#include "opt_xen.h"
#include "isa.h"
#include "pci.h"

struct cpu_info;

int xen_hvm_init(void);
int xen_hvm_init_cpu(struct cpu_info *);
void xen_mainbus_attach(device_t, device_t, void *);

struct hypervisor_attach_args {
	const char 		*haa_busname;
};

struct xencons_attach_args {
	const char 		*xa_device;
};

struct xen_npx_attach_args {
	const char 		*xa_device;
};


#define	u8 uint8_t
#define	u16 uint16_t
#define	u32 uint32_t
#define	u64 uint64_t
#define	s8 int8_t
#define	s16 int16_t
#define	s32 int32_t
#define	s64 int64_t

#include <sys/atomic.h>

#include <xen/include/public/xen.h>
#include <xen/include/public/sched.h>
#include <xen/include/public/platform.h>
#if __XEN_INTERFACE_VERSION__ < 0x00030204
#include <xen/include/public/dom0_ops.h>
#endif
#include <xen/include/public/event_channel.h>
#include <xen/include/public/physdev.h>
#include <xen/include/public/memory.h>
#include <xen/include/public/io/netif.h>
#include <xen/include/public/io/blkif.h>
#include <xen/include/public/arch-x86/hvm/start_info.h>

#if __XEN_INTERFACE_VERSION < 0x00030208
/* Undo namespace damage from xen/include/public/io/ring.h
 * The proper fix is to get upstream to stop assuming that all OSs use
 * mb(), rmb(), wmb().
 */
#undef xen_mb
#undef xen_rmb
#undef xen_wmb

#define xen_mb()  membar_sync()
#define xen_rmb() membar_producer()
#define xen_wmb() membar_consumer()
#endif /* __XEN_INTERFACE_VERSION */

#include <machine/xen/hypercalls.h>

#undef u8
#undef u16
#undef u32
#undef u64
#undef s8
#undef s16
#undef s32
#undef s64



/*
 * a placeholder for the start of day information passed up from the hypervisor
 */
union start_info_union
{
    start_info_t start_info;
    char padding[512];
};
extern union start_info_union start_info_union;
#define xen_start_info (start_info_union.start_info)

extern struct hvm_start_info *hvm_start_info;

/* For use in guest OSes. */
extern volatile shared_info_t *HYPERVISOR_shared_info;

/* console */
extern volatile struct xencons_interface *xencons_interface;


/* Structural guest handles introduced in 0x00030201. */
#if __XEN_INTERFACE_VERSION__ >= 0x00030201
#define xenguest_handle(hnd)	(hnd).p
#else
#define xenguest_handle(hnd)	hnd
#endif

/* hypervisor.c */
struct intrframe;
struct cpu_info;
void do_hypervisor_callback(struct intrframe *regs);
#if defined(XENPV) && (NPCI > 0 || NISA > 0)
void hypervisor_prime_pirq_event(int, unsigned int);
void hypervisor_ack_pirq_event(unsigned int);
#endif /* XENPV && ( NPCI > 0 || NISA > 0 ) */

extern int xen_version;
#define XEN_MAJOR(x) (((x) & 0xffff0000) >> 16)
#define XEN_MINOR(x) ((x) & 0x0000ffff)

/*
 * Does the hypervisor we're running on support an api
 * call at the requested version number ?
 */
#define XEN_VERSION_SUPPORTED(major, minor)		\
	(XEN_MAJOR(xen_version) > (major) ||		\
	 (XEN_MAJOR(xen_version) == (major) &&		\
	  XEN_MINOR(xen_version) >= (minor)))

/* hypervisor_machdep.c */
void hypervisor_send_event(struct cpu_info *, unsigned int);
void hypervisor_unmask_event(unsigned int);
void hypervisor_mask_event(unsigned int);
void hypervisor_clear_event(unsigned int);
void hypervisor_enable_sir(unsigned int);
void hypervisor_set_ipending(uint32_t, int, int);
void hypervisor_machdep_attach(void);
void hypervisor_machdep_resume(void);

/* 
 * Force a proper event-channel callback from Xen after clearing the
 * callback mask. We do this in a very simple manner, by making a call
 * down into Xen. The pending flag will be checked by Xen on return. 
 */
static __inline void hypervisor_force_callback(void)
{
	(void)HYPERVISOR_xen_version(0, (void*)0);
} __attribute__((no_instrument_function)) /* used by mcount */

static __inline void
hypervisor_notify_via_evtchn(unsigned int port)
{
	evtchn_op_t op;

	op.cmd = EVTCHNOP_send;
	op.u.send.port = port;
	(void)HYPERVISOR_event_channel_op(&op);
}

void xen_init_ksyms(void);
void xen_map_vcpu(struct cpu_info *);

#endif /* _XEN_HYPERVISOR_H_ */
