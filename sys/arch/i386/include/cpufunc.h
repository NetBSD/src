/*	$NetBSD: cpufunc.h,v 1.18 2000/03/28 01:38:22 thorpej Exp $	*/

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

#ifdef _KERNEL

static __inline void 
invlpg(u_int addr)
{ 
        __asm __volatile("invlpg (%0)" : : "r" (addr) : "memory");
}  

static __inline void
lidt(void *p)
{
	__asm __volatile("lidt (%0)" : : "r" (p));
}

static __inline void
lldt(u_short sel)
{
	__asm __volatile("lldt %0" : : "r" (sel));
}

static __inline void
ltr(u_short sel)
{
	__asm __volatile("ltr %0" : : "r" (sel));
}

static __inline void
lcr0(u_int val)
{
	__asm __volatile("movl %0,%%cr0" : : "r" (val));
}

static __inline u_int
rcr0(void)
{
	u_int val;
	__asm __volatile("movl %%cr0,%0" : "=r" (val));
	return val;
}

static __inline u_int
rcr2(void)
{
	u_int val;
	__asm __volatile("movl %%cr2,%0" : "=r" (val));
	return val;
}

static __inline void
lcr3(u_int val)
{
	__asm __volatile("movl %0,%%cr3" : : "r" (val));
}

static __inline u_int
rcr3(void)
{
	u_int val;
	__asm __volatile("movl %%cr3,%0" : "=r" (val));
	return val;
}

static __inline void
lcr4(u_int val)
{
	__asm __volatile("movl %0,%%cr4" : : "r" (val));
}

static __inline u_int
rcr4(void)
{
	u_int val;
	__asm __volatile("movl %%cr4,%0" : "=r" (val));
	return val;
}

static __inline void
tlbflush(void)
{
	u_int val;
	__asm __volatile("movl %%cr3,%0" : "=r" (val));
	__asm __volatile("movl %0,%%cr3" : : "r" (val));
}

#ifdef notyet
void	setidt	__P((int idx, /*XXX*/caddr_t func, int typ, int dpl));
#endif


/* XXXX ought to be in psl.h with spl() functions */

static __inline void
disable_intr(void)
{
	__asm __volatile("cli");
}

static __inline void
enable_intr(void)
{
	__asm __volatile("sti");
}

static __inline u_long
read_eflags(void)
{
	u_long	ef;

	__asm __volatile("pushfl; popl %0" : "=r" (ef));
	return (ef);
}

static __inline void
write_eflags(u_long ef)
{
	__asm __volatile("pushl %0; popfl" : : "r" (ef));
}

static __inline u_int64_t
rdmsr(u_int msr)
{
	u_int64_t rv;

	__asm __volatile(".byte 0x0f, 0x32" : "=A" (rv) : "c" (msr));
	return (rv);
}

static __inline void
wrmsr(u_int msr, u_int64_t newval)
{
	__asm __volatile(".byte 0x0f, 0x30" : : "A" (newval), "c" (msr));
}

static __inline void
wbinvd(void)
{
	__asm __volatile("wbinvd");
}

static __inline u_int64_t
rdtsc(void)
{
	u_int64_t rv;

	__asm __volatile(".byte 0x0f, 0x31" : "=A" (rv));
	return (rv);
}

static __inline u_int64_t
rdpmc(u_int pmc)
{
	u_int64_t rv;

	__asm __volatile(".byte 0x0f, 0x33" : "=A" (rv) : "c" (pmc));
	return (rv);
}

/* Break into DDB/KGDB. */
static __inline void
breakpoint(void)
{
	__asm __volatile("int $3");
}

#endif /* _KERNEL */

#endif /* !_I386_CPUFUNC_H_ */
