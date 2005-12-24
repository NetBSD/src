/*	$NetBSD: cpufunc.h,v 1.9 2005/12/24 20:07:48 perry Exp $	*/
/*	NetBSD: cpufunc.h,v 1.28 2004/01/14 11:31:55 yamt Exp 	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

#ifndef _I386_CPUFUNC_H_
#define	_I386_CPUFUNC_H_

/*
 * Functions to provide access to i386-specific instructions.
 */

#include <sys/cdefs.h>
#include <sys/types.h>

#include <machine/specialreg.h>
#include <machine/xen.h>
#include <machine/hypervisor.h>

static inline void
x86_pause(void)
{
	__asm volatile("pause");
}

static inline void
x86_lfence(void)
{

	/*
	 * XXX it's better to use real lfence insn if available.
	 */
	__asm volatile("lock; addl $0, 0(%%esp)" : : : "memory");
}

#ifdef _KERNEL

extern unsigned int cpu_feature;

#if 0
static inline void 
invlpg(u_int addr)
{
        __asm volatile("invlpg (%0)" : : "r" (addr) : "memory");
}  
#endif

static inline void
lidt(void *p)
{
	__asm volatile("lidt (%0)" : : "r" (p));
}

#if 0
static inline void
lldt(u_short sel)
{
	__asm volatile("lldt %0" : : "r" (sel));
}
#endif

#if 0
static inline void
ltr(u_short sel)
{
	__asm volatile("ltr %0" : : "r" (sel));
}

static inline void
lcr0(u_int val)
{
	__asm volatile("movl %0,%%cr0" : : "r" (val));
}

static inline u_int
rcr0(void)
{
	u_int val;
	__asm volatile("movl %%cr0,%0" : "=r" (val));
	return val;
}
#endif

static inline u_int
rcr2(void)
{
	return 0;
}

#if 0
static inline void
lcr3(u_int val)
{
	__asm volatile("movl %0,%%cr3" : : "r" (val));
}
#endif

static inline u_int
rcr3(void)
{
	u_int val;
	__asm volatile("movl %%cr3,%0" : "=r" (val));
	return val;
}

static inline void
lcr4(u_int val)
{
	__asm volatile("movl %0,%%cr4" : : "r" (val));
}

static inline u_int
rcr4(void)
{
	u_int val;
	__asm volatile("movl %%cr4,%0" : "=r" (val));
	return val;
}

#if 0
static inline void
tlbflush(void)
{
	u_int val;
	val = rcr3();
	lcr3(val);
}

static inline void
tlbflushg(void)
{
	/*
	 * Big hammer: flush all TLB entries, including ones from PTE's
	 * with the G bit set.  This should only be necessary if TLB
	 * shootdown falls far behind.
	 *
	 * Intel Architecture Software Developer's Manual, Volume 3,
	 *	System Programming, section 9.10, "Invalidating the
	 * Translation Lookaside Buffers (TLBS)":
	 * "The following operations invalidate all TLB entries, irrespective
	 * of the setting of the G flag:
	 * ...
	 * "(P6 family processors only): Writing to control register CR4 to
	 * modify the PSE, PGE, or PAE flag."
	 *
	 * (the alternatives not quoted above are not an option here.)
	 *
	 * If PGE is not in use, we reload CR3 for the benefit of
	 * pre-P6-family processors.
	 */

#if defined(I686_CPU)
	if (cpu_feature & CPUID_PGE) {
		u_int cr4 = rcr4();
		lcr4(cr4 & ~CR4_PGE);
		lcr4(cr4);
	} else
#endif
		tlbflush();
}
#endif

#ifdef notyet
void	setidt(int idx, /*XXX*/caddr_t func, int typ, int dpl);
#endif

/* debug register */
void dr0(caddr_t, u_int32_t, u_int32_t, u_int32_t);

#if 0
static inline u_int
rdr6(void)
{
	u_int val;

	__asm volatile("movl %%dr6,%0" : "=r" (val));
	return val;
}

static inline void
ldr6(u_int val)
{

	__asm volatile("movl %0,%%dr6" : : "r" (val));
}
#endif

/* XXXX ought to be in psl.h with spl() functions */

static inline void
disable_intr(void)
{
	__cli();
}

static inline void
enable_intr(void)
{
	__sti();
}

static inline u_long
read_eflags(void)
{
	u_long	ef;

	__asm volatile("pushfl; popl %0" : "=r" (ef));
	return (ef);
}

static inline void
write_eflags(u_long ef)
{
	__asm volatile("pushl %0; popfl" : : "r" (ef));
}

static inline u_int64_t
rdmsr(u_int msr)
{
	u_int64_t rv;

	__asm volatile("rdmsr" : "=A" (rv) : "c" (msr));
	return (rv);
}

static inline void
wrmsr(u_int msr, u_int64_t newval)
{
	__asm volatile("wrmsr" : : "A" (newval), "c" (msr));
}

static inline u_int64_t
rdtsc(void)
{
	u_int64_t rv;

	__asm volatile("rdtsc" : "=A" (rv));
	return (rv);
}

static inline u_int64_t
rdpmc(u_int pmc)
{
	u_int64_t rv;

	__asm volatile("rdpmc" : "=A" (rv) : "c" (pmc));
	return (rv);
}

/* Break into DDB/KGDB. */
static inline void
breakpoint(void)
{
	__asm volatile("int $3");
}

#define read_psl() (HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask)
#define write_psl(x) do {						\
    __insn_barrier();							\
    HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask = (x) ;	\
    x86_lfence();							\
    if ((x) == 0 && HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending) \
	hypervisor_force_callback();					\
} while (0)

    
/*
 * XXX Maybe these don't belong here...
 */

extern int (*copyout_func)(const void *, void *, size_t);
extern int (*copyin_func)(const void *, void *, size_t);

int	i386_copyout(const void *, void *, size_t);
int	i486_copyout(const void *, void *, size_t);

int	i386_copyin(const void *, void *, size_t);

#endif /* _KERNEL */

#endif /* !_I386_CPUFUNC_H_ */
