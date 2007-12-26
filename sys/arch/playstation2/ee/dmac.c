/*	$NetBSD: dmac.c,v 1.7.22.1 2007/12/26 19:42:35 ad Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dmac.c,v 1.7.22.1 2007/12/26 19:42:35 ad Exp $");

#include "debug_playstation2.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <mips/cache.h>

#include <playstation2/ee/eevar.h>
#include <playstation2/ee/dmacvar.h>
#include <playstation2/ee/dmacreg.h>
#include <playstation2/ee/gsvar.h>	/* debug monitor */

#include <playstation2/playstation2/interrupt.h>

#ifdef DEBUG
#define LEGAL_CHANNEL(x)	((x) >= 0 && (x) <= 15)
#define STATIC
#else
#define STATIC	static
#endif

#define _DMAC_NINTR	10

STATIC vaddr_t __dmac_channel_base[_DMAC_NINTR] = {
	D0_REGBASE,
	D1_REGBASE,
	D2_REGBASE,
	D3_REGBASE,
	D4_REGBASE,
	D5_REGBASE,
	D6_REGBASE,
	D7_REGBASE,
	D8_REGBASE,
	D9_REGBASE
};

u_int32_t __dmac_enabled_channel;

STATIC int __dmac_initialized;
STATIC struct _ipl_dispatcher __dmac_dispatcher[_DMAC_NINTR];
STATIC struct _ipl_holder __dmac_ipl_holder[_IPL_N];
STATIC SLIST_HEAD(, _ipl_dispatcher) __dmac_dispatcher_head =
 SLIST_HEAD_INITIALIZER(__dmac_dispatcher_head);

void
dmac_init()
{
	int i;

	if (__dmac_initialized++)
		return;

	/* disable DMAC */
	_reg_write_4(D_ENABLEW_REG, D_ENABLE_SUSPEND);

	/* disable all interrupt */
	for (i = 0; i < _DMAC_NINTR; i++)
		dmac_intr_disable(i);

	for (i = 0; i < _IPL_N; i++)
 		__dmac_ipl_holder[i].mask = 0xffffffff;

	if (_reg_read_4(D_STAT_REG) & D_STAT_SIM)
		_reg_write_4(D_STAT_REG, D_STAT_SIM);
	if (_reg_read_4(D_STAT_REG) & D_STAT_MEIM)
		_reg_write_4(D_STAT_REG, D_STAT_MEIM);

	/* clear all status */
	_reg_write_4(D_STAT_REG, _reg_read_4(D_STAT_REG) & D_STAT_CIS_MASK);

	/* enable DMAC */
	_reg_write_4(D_ENABLEW_REG, 0);
	_reg_write_4(D_CTRL_REG, D_CTRL_DMAE);
}

/*
 * Interrupt
 */
int
dmac_intr(u_int32_t mask)
{
	struct _ipl_dispatcher *dispatcher;
	u_int32_t r, dispatch, pending;

	r = _reg_read_4(D_STAT_REG);
	mask = D_STAT_CIM(mask);
	dispatch = r & ~mask & __dmac_enabled_channel;
	pending = r & mask & __dmac_enabled_channel;
#if 0
	__gsfb_print(2,
	    "DMAC stat=%08x, mask=%08x, pend=%08x, disp=%08x enable=%08x\n",
	    r, mask, pending, dispatch, __dmac_enabled_channel);
#endif
	if (dispatch == 0)
		return (pending == 0 ? 1 : 0);

	/* clear interrupt */
	_reg_write_4(D_STAT_REG, dispatch);
	
	/* dispatch interrupt handler */
	SLIST_FOREACH(dispatcher, &__dmac_dispatcher_head, link) {
		if (dispatcher->bit & dispatch) {
			KDASSERT(dispatcher->func);
			(*dispatcher->func)(dispatcher->arg);
			dispatch &= ~dispatcher->bit;
		}
	}

	/* disable spurious interrupt source */
	if (dispatch) {
		int i, bit;
		for (i = 0, bit = 1; i < _DMAC_NINTR; i++, bit <<= 1) {
			if (bit & dispatch) {
				dmac_intr_disable(i);
				printf("%s: spurious interrupt %d disabled.\n",
				    __func__, i);
			}
		}
	}


	return (pending == 0 ? 1 : 0);
}

void
dmac_intr_enable(enum dmac_channel ch)
{
	u_int32_t mask;

	KDASSERT(LEGAL_CHANNEL(ch));

	mask = D_STAT_CIM_BIT(ch);
	_reg_write_4(D_STAT_REG, (_reg_read_4(D_STAT_REG) & mask) ^ mask);
}

void
dmac_intr_disable(enum dmac_channel ch)
{
	KDASSERT(LEGAL_CHANNEL(ch));

	_reg_write_4(D_STAT_REG, _reg_read_4(D_STAT_REG) & D_STAT_CIM_BIT(ch));
}

void
dmac_update_mask(u_int32_t mask)
{
	u_int32_t cur_mask;

	mask = D_STAT_CIM(mask);
	cur_mask = _reg_read_4(D_STAT_REG);
	
	_reg_write_4(D_STAT_REG, ((cur_mask ^ ~mask) | (cur_mask & mask)) &
	    D_STAT_CIM(__dmac_enabled_channel));
}

void *
dmac_intr_establish(enum dmac_channel ch, int ipl, int (*func)(void *),
    void *arg)
{
	struct _ipl_dispatcher *dispatcher = &__dmac_dispatcher[ch];
	struct _ipl_dispatcher *d;
	int i, s;

	KDASSERT(dispatcher->func == NULL);
	
	s = _intr_suspend();
	dispatcher->func = func;
	dispatcher->arg = arg;
	dispatcher->ipl = ipl;
	dispatcher->channel = ch;
	dispatcher->bit = D_STAT_CIS_BIT(ch);

	for (i = 0; i < _IPL_N; i++) {
		if (i < ipl)
			__dmac_ipl_holder[i].mask &= ~D_STAT_CIM_BIT(ch);
		else
			__dmac_ipl_holder[i].mask |= D_STAT_CIM_BIT(ch);
	}

	/* insert queue IPL order */
	if (SLIST_EMPTY(&__dmac_dispatcher_head)) {
		SLIST_INSERT_HEAD(&__dmac_dispatcher_head, dispatcher, link);
	} else {
		SLIST_FOREACH(d, &__dmac_dispatcher_head, link) {
			if (SLIST_NEXT(d, link) == 0 ||
			    SLIST_NEXT(d, link)->ipl < ipl) {
				SLIST_INSERT_AFTER(d, dispatcher, link);
				break;
			}
		}
	}

	md_ipl_register(IPL_DMAC, __dmac_ipl_holder);

	dmac_intr_enable(ch);
	__dmac_enabled_channel |= D_STAT_CIS_BIT(ch);

	_intr_resume(s);
	
	return ((void *)ch);
}

void
dmac_intr_disestablish(void *handle)
{
	int ch = (int)(handle);
	struct _ipl_dispatcher *dispatcher = &__dmac_dispatcher[ch];
	int i, s;

	s = _intr_suspend();

	dmac_intr_disable(ch);
	dispatcher->func = NULL;

	SLIST_REMOVE(&__dmac_dispatcher_head, dispatcher,
	    _ipl_dispatcher, link);

	for (i = 0; i < _IPL_N; i++)
		__dmac_ipl_holder[i].mask |= D_STAT_CIM_BIT(ch);

	md_ipl_register(IPL_DMAC, __dmac_ipl_holder);
	__dmac_enabled_channel &= ~D_STAT_CIS_BIT(ch);

	_intr_resume(s);
}

/*
 * Start/Stop
 */
void
dmac_start_channel(enum dmac_channel ch)
{
	bus_addr_t chcr = D_CHCR_REG(__dmac_channel_base[ch]);
	u_int32_t r;
	int s;

	/* suspend all channels */
	s = _intr_suspend();
	r = _reg_read_4(D_ENABLER_REG);
	_reg_write_4(D_ENABLEW_REG, r | D_ENABLE_SUSPEND);

	/* access CHCR */
	_reg_write_4(chcr, (_reg_read_4(chcr) | D_CHCR_STR));

	/* start all channels */
	_reg_write_4(D_ENABLEW_REG, r & ~D_ENABLE_SUSPEND);
	_intr_resume(s);
}

void
dmac_stop_channel(enum dmac_channel ch)
{
	bus_addr_t chcr = D_CHCR_REG(__dmac_channel_base[ch]);
	u_int32_t r;
	int s;

	/* suspend all channels */
	s = _intr_suspend();
	r = _reg_read_4(D_ENABLER_REG);
	_reg_write_4(D_ENABLEW_REG, r | D_ENABLE_SUSPEND);

	/* access CHCR */
	_reg_write_4(chcr, (_reg_read_4(chcr) & ~D_CHCR_STR));

	/* resume all chanells */
	_reg_write_4(D_ENABLEW_REG, r);
	_intr_resume(s);
}

void
dmac_sync_buffer()
{

	mips_dcache_wbinv_all();
	__asm volatile("sync.l");
}

/*
 * Polling
 *   DMAC status connected to CPCOND[0].
 */
void
dmac_cpc_set(enum dmac_channel ch)
{
	u_int32_t r;

	r = _reg_read_4(D_PCR_REG);
	KDASSERT((D_PCR_CPC(r) & ~D_PCR_CPC_BIT(ch)) == 0);
	
	/* clear interrupt status */
	_reg_write_4(D_STAT_REG, D_STAT_CIS_BIT(ch));
	
	_reg_write_4(D_PCR_REG, r | D_PCR_CPC_BIT(ch));
}

void
dmac_cpc_clear(enum dmac_channel ch)
{

	_reg_write_4(D_PCR_REG,	_reg_read_4(D_PCR_REG) & ~D_PCR_CPC_BIT(ch))
}

void
dmac_cpc_poll()
{
	__asm volatile(
		".set noreorder;"
	"1:	 nop;"
		"nop;"
		"nop;"
		"nop;"
		"nop;"
		"bc0f 1b;"
		" nop;"
		".set reorder");
}

/* not recommended. use dmac_cpc_poll as possible */
void
dmac_bus_poll(enum dmac_channel ch)
{
	bus_addr_t chcr = D_CHCR_REG(__dmac_channel_base[ch]);

	while (_reg_read_4(chcr) & D_CHCR_STR)
		;
}

/*
 * Misc
 */
void
dmac_chcr_write(enum dmac_channel ch, u_int32_t v)
{
	u_int32_t r;
	int s;

	/* suspend all channels */
	s = _intr_suspend();
	r = _reg_read_4(D_ENABLER_REG);
	_reg_write_4(D_ENABLEW_REG, r | D_ENABLE_SUSPEND);

	/* write CHCR reg */
	_reg_write_4(D_CHCR_REG(__dmac_channel_base[ch]), v);

	/* resume all chanells */
	_reg_write_4(D_ENABLEW_REG, r);
	_intr_resume(s);
}

