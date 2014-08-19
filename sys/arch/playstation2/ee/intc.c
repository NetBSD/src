/*	$NetBSD: intc.c,v 1.8.6.2 2014/08/20 00:03:18 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: intc.c,v 1.8.6.2 2014/08/20 00:03:18 tls Exp $");

#include "debug_playstation2.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <playstation2/ee/eevar.h>
#include <playstation2/ee/intcvar.h>
#include <playstation2/ee/intcreg.h>
#include <playstation2/ee/gsvar.h>	/* debug monitor */

#include <playstation2/playstation2/interrupt.h>

#ifdef DEBUG
#define LEGAL_CHANNEL(x)	((x) >= 0 && (x) <= 15)
#define STATIC
#else
#define STATIC	static
#endif

#define _INTC_NINTR	16

u_int32_t __intc_enabled_channel;

STATIC int __intc_initialized;
STATIC struct _ipl_dispatcher __intc_dispatcher[_INTC_NINTR];
STATIC struct _ipl_holder __intc_ipl_holder[_IPL_N];

STATIC SLIST_HEAD(, _ipl_dispatcher) __intc_dispatcher_head =
 SLIST_HEAD_INITIALIZER(__intc_dispatcher_head);

void
intc_init(void)
{
	int i;

	if (__intc_initialized++)
		return;

	/* disable all channel */
	for (i = 0; i < _INTC_NINTR; i++)
		intc_intr_disable(i);

	/* clear interrupts */
	_reg_write_4(I_STAT_REG, _reg_read_4(I_STAT_REG));

	for (i = 0; i < _IPL_N; i++)
		__intc_ipl_holder[i].mask = 0xffffffff;
}

int
intc_intr(u_int32_t mask)
{
	struct _ipl_dispatcher *dispatcher;
	u_int32_t r, dispatch, pending;

	r = _reg_read_4(I_STAT_REG);
	dispatch = r & ~mask & __intc_enabled_channel;
	pending = r & mask & __intc_enabled_channel;

#if 0
	__gsfb_print(1,
	    "INTC stat=%08x, mask=%08x, pend=%08x, disp=%08x enable=%08x\n",
	    r, mask, pending, dispatch, __intc_enabled_channel);
#endif
	if (dispatch == 0)
		return (pending == 0 ? 1 : 0);

	/* clear interrupt */
	_reg_write_4(I_STAT_REG, dispatch);
	
	/* dispatch interrupt handler */
	SLIST_FOREACH(dispatcher, &__intc_dispatcher_head, link) {
		if (dispatcher->bit & dispatch) {
			KDASSERT(dispatcher->func);
			(*dispatcher->func)(dispatcher->arg);
			dispatch &= ~dispatcher->bit;
		}
	}

	/* disable spurious interrupt source */
	if (dispatch) {
		int i, bit;
		for (i = 0, bit = 1; i < _INTC_NINTR; i++, bit <<= 1) {
			if (bit & dispatch) {
				intc_intr_disable(i);
				printf("%s: spurious interrupt %d disabled.\n",
				    __func__, i);
			}
		}
	}

	return (pending == 0 ? 1 : 0);
}

void
intc_intr_enable(enum intc_channel ch)
{
	u_int32_t mask;

	KDASSERT(LEGAL_CHANNEL(ch));
	mask = 1 << ch;
	_reg_write_4(I_MASK_REG, (_reg_read_4(I_MASK_REG) & mask) ^ mask);
}

void
intc_intr_disable(enum intc_channel ch)
{

	KDASSERT(LEGAL_CHANNEL(ch));
	_reg_write_4(I_MASK_REG, _reg_read_4(I_MASK_REG) & (1 << ch));
}

void
intc_update_mask(u_int32_t mask)
{
	u_int32_t cur_mask;

	cur_mask = _reg_read_4(I_MASK_REG);

	_reg_write_4(I_MASK_REG, ((cur_mask ^ ~mask) | (cur_mask & mask)) &
	    __intc_enabled_channel);
}

void *
intc_intr_establish(enum intc_channel ch, int ipl, int (*func)(void *),
    void *arg)
{
	struct _ipl_dispatcher *dispatcher = &__intc_dispatcher[ch];
	struct _ipl_dispatcher *d;
	u_int32_t bit;
	int i, s;

	KDASSERT(dispatcher->func == NULL);
	
	s = _intr_suspend();
	dispatcher->func = func;
	dispatcher->arg = arg;
	dispatcher->ipl = ipl;
	dispatcher->channel = ch;
	dispatcher->bit = bit = (1 << ch);

	for (i = 0; i < _IPL_N; i++)
		if (i < ipl)
			__intc_ipl_holder[i].mask &= ~bit;
		else
			__intc_ipl_holder[i].mask |= bit;

	/* insert queue IPL order */
	if (SLIST_EMPTY(&__intc_dispatcher_head)) {
		SLIST_INSERT_HEAD(&__intc_dispatcher_head, dispatcher, link);
	} else {
		SLIST_FOREACH(d, &__intc_dispatcher_head, link) {
			if (SLIST_NEXT(d, link) == 0 ||
			    SLIST_NEXT(d, link)->ipl < ipl) {
				SLIST_INSERT_AFTER(d, dispatcher, link);
				break;
			}
		}
	}

	md_ipl_register(IPL_INTC, __intc_ipl_holder);

	intc_intr_enable(ch);
	__intc_enabled_channel |= bit;

	_intr_resume(s);

	return ((void *)ch);
}

void
intc_intr_disestablish(void *handle)
{
	int ch = (int)(handle);
	struct _ipl_dispatcher *dispatcher = &__intc_dispatcher[ch];
	u_int32_t bit;
	int i, s;

	s = _intr_suspend();

	intc_intr_disable(ch);
	dispatcher->func = NULL;

	SLIST_REMOVE(&__intc_dispatcher_head, dispatcher,
	    _ipl_dispatcher, link);

	bit = dispatcher->bit;
	for (i = 0; i < _IPL_N; i++)
		__intc_ipl_holder[i].mask |= bit;

	md_ipl_register(IPL_INTC, __intc_ipl_holder);
	__intc_enabled_channel &= ~bit;

	_intr_resume(s);
}

