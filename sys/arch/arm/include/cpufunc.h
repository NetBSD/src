/*	$NetBSD: cpufunc.h,v 1.6.2.2 2001/09/13 01:13:09 thorpej Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
 * All rights reserved.
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
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpufunc.h
 *
 * Prototypes for cpu, mmu and tlb related functions.
 */

#ifndef _ARM32_CPUFUNC_H_
#define _ARM32_CPUFUNC_H_

#include <sys/types.h>

#ifdef _KERNEL
#ifndef _LKM
#include "opt_cputypes.h"
#endif

struct cpu_functions {

	/* CPU functions */
	
	u_int	(*cf_id)		__P((void));

	/* MMU functions */

	u_int	(*cf_control)		__P((u_int bic, u_int eor));
	void	(*cf_domains)		__P((u_int domains));
	void	(*cf_setttb)		__P((u_int ttb));
	u_int	(*cf_faultstatus)	__P((void));
	u_int	(*cf_faultaddress)	__P((void));

	/* TLB functions */

	void	(*cf_tlb_flushID)	__P((void));	
	void	(*cf_tlb_flushID_SE)	__P((u_int va));	
	void	(*cf_tlb_flushI)	__P((void));
	void	(*cf_tlb_flushI_SE)	__P((u_int va));	
	void	(*cf_tlb_flushD)	__P((void));
	void	(*cf_tlb_flushD_SE)	__P((u_int va));	

	/* Cache functions */

	void	(*cf_cache_flushID)	__P((void));	
	void	(*cf_cache_flushID_SE)	__P((u_int va));	
	void	(*cf_cache_flushI)	__P((void));	
	void	(*cf_cache_flushI_SE)	__P((u_int va));	
	void	(*cf_cache_flushD)	__P((void));	
	void	(*cf_cache_flushD_SE)	__P((u_int va));	

	void	(*cf_cache_cleanID)	__P((void));	
	void	(*cf_cache_cleanID_E)	__P((u_int imp));	
	void	(*cf_cache_cleanD)	__P((void));	
	void	(*cf_cache_cleanD_E)	__P((u_int imp));	

	void	(*cf_cache_purgeID)	__P((void));	
	void	(*cf_cache_purgeID_E)	__P((u_int imp));	
	void	(*cf_cache_purgeD)	__P((void));	
	void	(*cf_cache_purgeD_E)	__P((u_int imp));	

	/* Other functions */

	void	(*cf_flush_prefetchbuf)	__P((void));
	void	(*cf_drain_writebuf)	__P((void));
	void	(*cf_flush_brnchtgt_C)	__P((void));
	void	(*cf_flush_brnchtgt_E)	__P((u_int va));

	void	(*cf_sleep)		__P((int mode));

	/* Soft functions */

	void	(*cf_cache_syncI)	__P((void));
	void	(*cf_cache_cleanID_rng)	__P((u_int start, u_int len));
	void	(*cf_cache_cleanD_rng)	__P((u_int start, u_int len));
	void	(*cf_cache_purgeID_rng)	__P((u_int start, u_int len));
	void	(*cf_cache_purgeD_rng)	__P((u_int start, u_int len));
	void	(*cf_cache_syncI_rng)	__P((u_int start, u_int len));

	int	(*cf_dataabt_fixup)	__P((void *arg));
	int	(*cf_prefetchabt_fixup)	__P((void *arg));

	void	(*cf_context_switch)	__P((void));

	void	(*cf_setup)		__P((char *string));
};

extern struct cpu_functions cpufuncs;
extern u_int cputype;

#define cpu_id()		cpufuncs.cf_id()

#define cpu_control(c, e)	cpufuncs.cf_control(c, e)
#define cpu_domains(d)		cpufuncs.cf_domains(d)
#define cpu_setttb(t)		cpufuncs.cf_setttb(t)
#define cpu_faultstatus()	cpufuncs.cf_faultstatus()
#define cpu_faultaddress()	cpufuncs.cf_faultaddress()

#define	cpu_tlb_flushID()	cpufuncs.cf_tlb_flushID()
#define	cpu_tlb_flushID_SE(e)	cpufuncs.cf_tlb_flushID_SE(e)
#define	cpu_tlb_flushI()	cpufuncs.cf_tlb_flushI()
#define	cpu_tlb_flushI_SE(e)	cpufuncs.cf_tlb_flushI_SE(e)
#define	cpu_tlb_flushD()	cpufuncs.cf_tlb_flushD()
#define	cpu_tlb_flushD_SE(e)	cpufuncs.cf_tlb_flushD_SE(e)

#define	cpu_cache_flushID()	cpufuncs.cf_cache_flushID()
#define	cpu_cache_flushID_SE(e)	cpufuncs.cf_cache_flushID_SE(e)
#define	cpu_cache_flushI()	cpufuncs.cf_cache_flushI()
#define	cpu_cache_flushI_SE(e)	cpufuncs.cf_cache_flushI_SE(e)
#define	cpu_cache_flushD()	cpufuncs.cf_cache_flushD()
#define	cpu_cache_flushD_SE(e)	cpufuncs.cf_cache_flushD_SE(e)
#define	cpu_cache_cleanID()	cpufuncs.cf_cache_cleanID()
#define	cpu_cache_cleanID_E(e)	cpufuncs.cf_cache_cleanID_E(e)
#define	cpu_cache_cleanD()	cpufuncs.cf_cache_cleanD()
#define	cpu_cache_cleanD_E(e)	cpufuncs.cf_cache_cleanD_E(e)
#define	cpu_cache_purgeID()	cpufuncs.cf_cache_purgeID()
#define	cpu_cache_purgeID_E(e)	cpufuncs.cf_cache_purgeID_E(e)
#define	cpu_cache_purgeD()	cpufuncs.cf_cache_purgeD()
#define	cpu_cache_purgeD_E(e)	cpufuncs.cf_cache_purgeD_E(e)

#define	cpu_flush_prefetchbuf()	cpufuncs.cf_flush_prefetchbuf()
#define	cpu_drain_writebuf()	cpufuncs.cf_drain_writebuf()
#define	cpu_flush_brnchtgt_C()	cpufuncs.cf_flush_brnchtgt_C()
#define	cpu_flush_brnchtgt_E(e)	cpufuncs.cf_flush_brnchtgt_E(e)

#define cpu_sleep(m)		cpufuncs.cf_sleep(m)

#define	cpu_cache_syncI()		cpufuncs.cf_cache_syncI()
#define	cpu_cache_cleanID_rng(s,l)	cpufuncs.cf_cache_cleanID_rng(s,l)
#define	cpu_cache_cleanD_rng(s,l)	cpufuncs.cf_cache_cleanD_rng(s,l)
#define	cpu_cache_purgeID_rng(s,l)	cpufuncs.cf_cache_purgeID_rng(s,l)
#define	cpu_cache_purgeD_rng(s,l)	cpufuncs.cf_cache_purgeD_rng(s,l)
#define	cpu_cache_syncI_rng(s,l)	cpufuncs.cf_cache_syncI_rng(s,l)

#define cpu_dataabt_fixup(a)		cpufuncs.cf_dataabt_fixup(a)
#define cpu_prefetchabt_fixup(a)	cpufuncs.cf_prefetchabt_fixup(a)
#define ABORT_FIXUP_OK		0	/* fixup succeeded */
#define ABORT_FIXUP_FAILED	1	/* fixup failed */
#define ABORT_FIXUP_RETURN	2	/* abort handler should return */

#define cpu_setup(a)			cpufuncs.cf_setup(a)

int	set_cpufuncs		__P((void));
#define ARCHITECTURE_NOT_PRESENT	1	/* known but not configured */
#define ARCHITECTURE_NOT_SUPPORTED	2	/* not known */

void	cpufunc_nullop		__P((void));
int	cpufunc_null_fixup	__P((void *));
int	early_abort_fixup	__P((void *));
int	late_abort_fixup	__P((void *));
u_int	cpufunc_id		__P((void));
u_int	cpufunc_control		__P((u_int clear, u_int bic));
void	cpufunc_domains		__P((u_int domains));
u_int	cpufunc_faultstatus	__P((void));
u_int	cpufunc_faultaddress	__P((void));

#ifdef CPU_ARM3
u_int	arm3_control		__P((u_int clear, u_int bic));
void	arm3_cache_flush	__P((void));
#endif	/* CPU_ARM3 */

#if defined(CPU_ARM6) || defined(CPU_ARM7)
void	arm67_setttb		__P((u_int ttb));
void	arm67_tlb_flush		__P((void));
void	arm67_tlb_purge		__P((u_int va));
void	arm67_cache_flush	__P((void));
void	arm67_context_switch	__P((void));
#endif	/* CPU_ARM6 || CPU_ARM7 */

#ifdef CPU_ARM6
void	arm6_setup		__P((char *string));
#endif	/* CPU_ARM6 */

#ifdef CPU_ARM7
void	arm7_setup		__P((char *string));
#endif	/* CPU_ARM7 */

#ifdef CPU_ARM7TDMI
int	arm7_dataabt_fixup	__P((void *arg));
void	arm7tdmi_setup		__P((char *string));
void	arm7tdmi_setttb		__P((u_int ttb));
void	arm7tdmi_tlb_flushID	__P((void));
void	arm7tdmi_tlb_flushID_SE	__P((u_int va));
void	arm7tdmi_cache_flushID	__P((void));
void	arm7tdmi_context_switch	__P((void));
#endif /* CPU_ARM7TDMI */

#ifdef CPU_ARM8
void	arm8_setttb		__P((u_int ttb));
void	arm8_tlb_flushID	__P((void));
void	arm8_tlb_flushID_SE	__P((u_int va));
void	arm8_cache_flushID	__P((void));
void	arm8_cache_flushID_E	__P((u_int entry));
void	arm8_cache_cleanID	__P((void));
void	arm8_cache_cleanID_E	__P((u_int entry));
void	arm8_cache_purgeID	__P((void));
void	arm8_cache_purgeID_E	__P((u_int entry));

void	arm8_cache_syncI	__P((void));
void	arm8_cache_cleanID_rng	__P((u_int start, u_int end));
void	arm8_cache_cleanD_rng	__P((u_int start, u_int end));
void	arm8_cache_purgeID_rng	__P((u_int start, u_int end));
void	arm8_cache_purgeD_rng	__P((u_int start, u_int end));
void	arm8_cache_syncI_rng	__P((u_int start, u_int end));

void	arm8_context_switch	__P((void));

void	arm8_setup		__P((char *string));

u_int	arm8_clock_config	__P((u_int, u_int));
#endif

#ifdef CPU_SA110
void	sa110_setttb		__P((u_int ttb));
void	sa110_tlb_flushID	__P((void));
void	sa110_tlb_flushID_SE	__P((u_int va));
void	sa110_tlb_flushI	__P((void));
void	sa110_tlb_flushD	__P((void));
void	sa110_tlb_flushD_SE	__P((u_int va));

void	sa110_cache_flushID	__P((void));
void	sa110_cache_flushI	__P((void));
void	sa110_cache_flushD	__P((void));
void	sa110_cache_flushD_SE	__P((u_int entry));

void	sa110_cache_cleanID	__P((void));
void	sa110_cache_cleanD	__P((void));
void	sa110_cache_cleanD_E	__P((u_int entry));

void	sa110_cache_purgeID	__P((void));
void	sa110_cache_purgeID_E	__P((u_int entry));
void	sa110_cache_purgeD	__P((void));
void	sa110_cache_purgeD_E	__P((u_int entry));

void	sa110_drain_writebuf	__P((void));

void	sa110_cache_syncI	__P((void));
void	sa110_cache_cleanID_rng	__P((u_int start, u_int end));
void	sa110_cache_cleanD_rng	__P((u_int start, u_int end));
void	sa110_cache_purgeID_rng	__P((u_int start, u_int end));
void	sa110_cache_purgeD_rng	__P((u_int start, u_int end));
void	sa110_cache_syncI_rng	__P((u_int start, u_int end));

void	sa110_context_switch	__P((void));

void	sa110_setup		__P((char *string));
#endif	/* CPU_SA110 */

#ifdef CPU_XSCALE
void	xscale_setttb		__P((u_int ttb));
void	xscale_tlb_flushID	__P((void));
void	xscale_tlb_flushID_SE	__P((u_int va));
void	xscale_tlb_flushI	__P((void));
void	xscale_tlb_flushD	__P((void));
void	xscale_tlb_flushD_SE	__P((u_int va));

void	xscale_cache_flushID	__P((void));
void	xscale_cache_flushI	__P((void));
void	xscale_cache_flushD	__P((void));
void	xscale_cache_flushD_SE	__P((u_int entry));

void	xscale_cache_cleanID	__P((void));
void	xscale_cache_cleanD	__P((void));
void	xscale_cache_cleanD_E	__P((u_int entry));

void	xscale_cache_purgeID	__P((void));
void	xscale_cache_purgeID_E	__P((u_int entry));
void	xscale_cache_purgeD	__P((void));
void	xscale_cache_purgeD_E	__P((u_int entry));

void	xscale_drain_writebuf	__P((void));

void	xscale_cache_syncI	__P((void));
void	xscale_cache_cleanID_rng	__P((u_int start, u_int end));
void	xscale_cache_cleanD_rng	__P((u_int start, u_int end));
void	xscale_cache_purgeID_rng	__P((u_int start, u_int end));
void	xscale_cache_purgeD_rng	__P((u_int start, u_int end));
void	xscale_cache_syncI_rng	__P((u_int start, u_int end));

void	xscale_context_switch	__P((void));

void	xscale_setup		__P((char *string));
#endif	/* CPU_XSCALE */

#define tlb_flush	cpu_tlb_flushID
#define setttb		cpu_setttb
#define cache_clean	cpu_cache_purgeID
#define sync_caches	cpu_cache_syncI
#define sync_icache	cpu_cache_syncI
#define drain_writebuf	cpu_drain_writebuf

/*
 * Macros for manipulating CPU interrupts
 */

#define disable_interrupts(mask) \
	(SetCPSR((mask) & (I32_bit | F32_bit), (mask) & (I32_bit | F32_bit)))

#define enable_interrupts(mask) \
	(SetCPSR((mask) & (I32_bit | F32_bit), 0))

#define restore_interrupts(old_cpsr) \
	(SetCPSR((I32_bit | F32_bit), (old_cpsr) & (I32_bit | F32_bit)))

/*
 * Functions to manipulate the CPSR
 * (in arm/arm32/setcpsr.S)
 */

u_int SetCPSR		__P((u_int bic, u_int eor));
u_int GetCPSR		__P((void));

/*
 * Functions to manipulate cpu r13
 * (in arm/arm32/setstack.S)
 */

void set_stackptr	__P((u_int mode, u_int address));
u_int get_stackptr	__P((u_int mode));

/*
 * Miscellany
 */

int get_pc_str_offset();

/*
 * CPU functions from locore.S
 */

void cpu_reset		__P((void)) __attribute__((__noreturn__));

#endif	/* _KERNEL */
#endif	/* _ARM32_CPUFUNC_H_ */

/* End of cpufunc.h */
