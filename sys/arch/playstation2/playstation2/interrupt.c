/*	$NetBSD: interrupt.c,v 1.11.6.3 2017/12/03 11:36:35 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.11.6.3 2017/12/03 11:36:35 jdolecek Exp $");

#include "debug_playstation2.h"
#if defined INTR_DEBUG && !defined GSFB_DEBUG_MONITOR
#error "add option GSFB_DEBUG_MONITOR"
#endif

#include <sys/param.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>	/* uvmexp.intrs */

#include <machine/locore.h>	/* mips3_cp0_*() */

#include <playstation2/playstation2/interrupt.h>

#include <playstation2/ee/eevar.h>
#include <playstation2/ee/intcvar.h>
#include <playstation2/ee/intcreg.h>
#include <playstation2/ee/dmacreg.h>
#include <playstation2/ee/dmacvar.h>
#include <playstation2/ee/timervar.h>

#ifdef INTR_DEBUG
#include <playstation2/ee/gsvar.h>	/* debug monitor */
#endif

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

struct _playstation2_evcnt _playstation2_evcnt = {
	.clock	= EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "hard", "clock"),
	.sbus	= EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "hard", "sbus"),
	.dmac	= EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "hard", "dmac"),
};

STATIC struct {
	u_int32_t sr, imask;
} _sif_call_env;

struct clockframe playstation2_clockframe;

u_int32_t __icu_mask[_IPL_N];	/* interrupt mask of DMAC/INTC */
volatile u_int32_t md_imask;

#ifdef INTR_DEBUG
void _debug_print_ipl(void);
void _debug_print_intr(const char *);
#endif /* INTR_DEBUG */

void
interrupt_init_bootstrap(void)
{
	int i;

	/* initialize interrupt mask (masked all) */
	for (i = 0; i < _IPL_N; i++)
		__icu_mask[i] = 0xffffffff;

	/* intialize EE embeded device */
	timer_init();

	/* clear all pending interrupt and disable all */
	intc_init(); /* INT0 */
	dmac_init(); /* INT1 */
}

void
interrupt_init(void)
{
	evcnt_attach_static(&_playstation2_evcnt.clock);
	evcnt_attach_static(&_playstation2_evcnt.sbus);
	evcnt_attach_static(&_playstation2_evcnt.dmac);

	/* install software interrupt handler */
	intc_intr_establish(I_CH10_TIMER1, IPL_SOFT, timer1_intr, 0);
	intc_intr_establish(I_CH11_TIMER2, IPL_SOFTCLOCK, timer2_intr, 0);

	/* IPL_SOFTNET and IPL_SOFTSERIAL are shared interrupt. */
	intc_intr_establish(I_CH12_TIMER3, IPL_SOFTNET, timer3_intr, 0);

	/* enable SIF BIOS access */
	md_imask = ~D_STAT_CIM_BIT(D_CH5_SIF0);
	mips_cp0_status_write(0x00010801);
}

/*
 *  Hardware interrupt support
 */
void
cpu_intr(int ppl, vaddr_t pc, uint32_t status)
{
	struct cpu_info *ci;
	uint32_t ipending;
	int ipl;
#if 0
	_debug_print_intr(__func__);
#endif

	ci = curcpu();
	ci->ci_idepth++;
	ci->ci_data.cpu_nintr++;

	playstation2_clockframe.intr = (curcpu()->ci_idepth > 1);
	playstation2_clockframe.sr = status;
	playstation2_clockframe.pc = pc;

	while (ppl < (ipl = splintr(&ipending))) {
		splx(ipl);
		if (ipending & MIPS_INT_MASK_0) {
			intc_intr(md_imask);
		}

		if (ipending & MIPS_INT_MASK_1) {
			_playstation2_evcnt.dmac.ev_count++;
			dmac_intr(md_imask);
		}
		(void)splhigh();
	}
}
void
setsoft(int ipl)
{
	const static int timer_map[] = {
		[IPL_SOFTCLOCK]	= 1,
		[IPL_SOFTBIO]	= 2,
		[IPL_SOFTNET]	= 3,
		[IPL_SOFTSERIAL]= 3,
	};

	KDASSERT(ipl >= IPL_SOFTCLOCK && ipl <= IPL_SOFTSERIAL);

	/* kick one shot timer */
	timer_one_shot(timer_map[ipl]);
}

/*
 * SPL support
 */
void
md_ipl_register(enum ipl_type type, struct _ipl_holder *holder)
{
	u_int32_t mask, new;
	int i;

	mask = (type == IPL_DMAC) ? 0x0000ffff : 0xffff0000;

	for (i = 0; i < _IPL_N; i++) {
		new = __icu_mask[i];
		new &= mask;
		new |= (holder[i].mask & ~mask);
		__icu_mask[i] = new;
	}
}

int
splraise(int npl)
{
	int s, opl;

	s = _intr_suspend();
	opl = md_imask;
	md_imask = opl | npl;
	md_imask_update();
	_intr_resume(s);

	return (opl);
}

void
splset(int npl)
{
	int s;

	s = _intr_suspend();
	md_imask = npl;
	md_imask_update();
	_intr_resume(s);
}

void
spl0(void)
{

	splset(0);
	_spllower(0);
}

/*
 * SIF BIOS call of interrupt utility.
 */
void
_sif_call_start(void)
{
	int s;

	s = _intr_suspend();

	_sif_call_env.sr = mips_cp0_status_read();
	_sif_call_env.imask = md_imask;

	md_imask = ~D_STAT_CIM_BIT(D_CH5_SIF0);
	md_imask_update();

	mips_cp0_status_write(0x00010801);
	dmac_intr_enable(D_CH5_SIF0);

	_intr_resume(s);
}

void
_sif_call_end(void)
{
	int s;

	s = _intr_suspend();

	md_imask = _sif_call_env.imask;
	md_imask_update();
	mips_cp0_status_write(_sif_call_env.sr);
	
	_intr_resume(s);
}

#ifdef INTR_DEBUG
void
_debug_print_ipl(void)
{
	int i;

	printf("interrupt mask\n");
	for (i = 0; i < _IPL_N; i++)
		printf("%d: %08x\n", i, __icu_mask[i]);
}

void
_debug_print_intr(const char *ident)
{

	__gsfb_print(0,
	    "CLOCK %-5lld SBUS %-5lld DMAC %-5lld "

	    _playstation2_evcnt.clock.ev_count,
	    _playstation2_evcnt.sbus.ev_count,
	    _playstation2_evcnt.dmac.ev_count,
	    playstation2_clockframe.sr, playstation2_clockframe.pc,
	    md_imask,
	    (_reg_read_4(I_MASK_REG) << 16) |
	    (_reg_read_4(I_STAT_REG) & 0x0000ffff),
	    _reg_read_4(D_STAT_REG), sched_whichqs);
}
#endif /* INTR_DEBUG */

