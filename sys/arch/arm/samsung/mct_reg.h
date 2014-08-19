/* $NetBSD */
/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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

#ifndef _ARM_SAMSUNG_MCT_REG_H_
#define _ARM_SAMSUNG_MCT_REG_H_

/* global timer register offsets */
#define MCT_G_CNT_L		0x100		/* lower bits */
#define MCT_G_CNT_U		0x104		/* higher bits */
#define MCT_G_CNT_WSTAT		0x110		/* wait for write OK cntr */
#define   G_CNT_WSTAT_L		__BIT(0)
#define   G_CNT_WSTAT_U		__BIT(1)

#define MCT_G_COMP0_L		0x200		/* compare0 lower bits */
#define MCT_G_COMP0_U		0x204		/* compare0 higher bits */
#define MCT_G_COMP0_ADD_INCR	0x208		/* compare0 auto add value */
#define MCT_G_TCON		0x240		/* configuration register */
#define   G_TCON_COMP0_ENABLE	__BIT(0)
#define   G_TCON_COMP0_AUTOINC	__BIT(1)
#define   G_TCON_START		__BIT(8)
#define MCT_G_INT_CSTAT		0x244		/* clear interrupt */
#define   G_INT_CSTAT_CLEAR	__BIT(0)
#define MCT_G_INT_ENB		0x248		/* enable interrupts */
#define   G_INT_ENB_ENABLE	__BIT(0)
#define MCT_G_WSTAT		0x24C		/* wait for write OK */
#define   G_WSTAT_COMP0_L	__BIT(0)
#define   G_WSTAT_COMP0_U	__BIT(1)
#define   G_WSTAT_ADD_INCR	__BIT(2)
#define   G_WSTAT_TCON		__BIT(16)

/* local cpu-bound timers */
#define MCT_L_OFFSET(x)		(0x300 + (0x100 * x))
#define MCT_L_MASK		0xffffff00

#define MCT_L_TCNTB		0x00		/* TODO: what */
#define MCT_L_ICNTB		0x08		/* interrupt count buffer */
#define   L_INCTB_UPDATE(x)	(__BIT(31) | (x))
#define MCT_L_TCON		0x20		/* configuration register */
#define   L_TCON_TIMER_START	__BIT(0)
#define   L_TCON_INT_START	__BIT(1)
#define   L_TCON_INTERVAL_MODE	__BIT(2)
#define MCT_L_INC_CSTAT		0x30
#define   L_INC_CSTAT_CLEAR	__BIT(0)
#define MCT_L_INT_ENB		0x34
#define   L_INT_ENB_ENABLE	__BIT(0)
#define MCT_L_WSTAT		0x40
#define   L_WSTAT_L_TCNTB	__BIT(0)
#define   L_WSTAT_L_ICNTB	__BIT(1)
#define   L_WSTAT_L_TCON	__BIT(3)

#endif /* _ARM_SAMSUNG_MCT_REG_H_ */

