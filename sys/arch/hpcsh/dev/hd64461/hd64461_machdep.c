/*	$NetBSD: hd64461_machdep.c,v 1.1 2002/03/03 14:34:02 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcsh/dev/hd64461/hd64461var.h>
#include <hpcsh/dev/hd64461/hd64461pcmciavar.h>
#include <hpcsh/dev/hd64461/hd64461pcmciareg.h>

/*
 * Platform dependent hooks.
 */

void
hd64461pcmcia_power(int ch, enum pcmcia_voltage vol, int on)
{
#define VCC0_ON()							\
do {									\
	r = hd64461_reg_read_1(gcr);					\
	r |= HD64461_PCCGCR_VCC0;					\
	hd64461_reg_write_1(gcr, r);					\
} while (/*CONSTCOND*/0)
#define VCC0_OFF()							\
do {									\
	r = hd64461_reg_read_1(gcr);					\
	r &= ~HD64461_PCCGCR_VCC0;					\
	hd64461_reg_write_1(gcr, r);					\
} while (/*CONSTCOND*/0)
#define VCC1_ON()							\
do {									\
	r = hd64461_reg_read_1(scr);					\
	r |= HD64461_PCCSCR_VCC1;					\
	hd64461_reg_write_1(scr, r);					\
} while (/*CONSTCOND*/0)
#define VCC1_OFF()							\
do {									\
	r = hd64461_reg_read_1(scr);					\
	r &= ~HD64461_PCCSCR_VCC1;					\
	hd64461_reg_write_1(scr, r);					\
} while (/*CONSTCOND*/0)
	bus_addr_t isr, gcr, scr;
	u_int8_t r;
	
	isr = HD64461_PCCISR(ch);
	gcr = HD64461_PCCGCR(ch);
	scr = HD64461_PCCSCR(ch);

	/* 3.3 V */
	if (vol == V_3_3) {
		if (ch == 1) {
			if (on) {
				VCC0_OFF();
				VCC1_OFF();
			} else {
				VCC0_ON();
				VCC1_ON();
			}
		} else {
			if (on) {
				VCC0_ON();
				VCC1_OFF();
			} else {
				VCC0_OFF();
				VCC1_ON();
			}
		}
		return;
	}

	/* 5 V */
	if (platid_match(&platid, &platid_mask_MACH_HP)) {
		if (on) {
			VCC0_OFF();
			VCC1_OFF();
		} else {
			VCC0_ON();
			VCC1_ON();
		}
		return;
	} else if (platid_match(&platid, &platid_mask_MACH_HITACHI)) {
		if (on) {
			VCC0_OFF();
			VCC1_ON();
		} else {
			VCC0_ON();
			VCC1_OFF();
		}
		return;
	}

	/* x.x V, y.y V */
	printf("x.x/y.y V not supported.\n");
#undef VCC0_ON
#undef VCC0_OFF
#undef VCC1_ON
#undef VCC1_OFF
}
