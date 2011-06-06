/*	$NetBSD: gboxreg.h,v 1.2.6.2 2011/06/06 09:05:35 jruoho Exp $	*/
/*	$OpenBSD: gboxreg.h,v 1.2 2005/01/24 21:36:39 miod Exp $	*/
/*	NetBSD: grf_gbreg.h,v 1.4 1994/10/26 07:23:53 cgd Exp 	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: grf_gbreg.h 1.11 92/01/21$
 *
 *	@(#)grf_gbreg.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Gatorbox driver regs
 */

#define TILER_ENABLE		0x80
#define LINE_MOVER_ENABLE	0x80
#define UP_LEFT			0x00
#define DOWN_RIGHT		0x40
#define MOVE_UP_LEFT		(TILER_ENABLE | UP_LEFT)
#define MOVE_DOWN_RIGHT		(TILER_ENABLE | DOWN_RIGHT)

#define tile_mover_waitbusy(regaddr)					\
do {									\
	while (((volatile struct gboxfb *)(regaddr))->regs.sec_interrupt & 0x10) \
		DELAY(1);						\
} while (/* CONSTCOND */0)

#define line_mover_waitbusy(regaddr)					\
do {									\
	while ((((volatile struct gboxfb *)(regaddr))->status & 0x80) == 0) \
		DELAY(1);						\
} while (/* CONSTCOND */0)

#define gbcm_waitbusy(regaddr)						\
do {									\
	while (((volatile struct gboxfb *)(regaddr))->cmap_busy != 0xff) \
		DELAY(1);						\
} while (/* CONSTCOND */0)

struct gboxfb {
	struct diofbreg regs;
	uint8_t f2[0x4000-0x5f-1];
	uint8_t crtc_address;		/* CTR controller address reg 0x4000 */
	uint8_t status;			/* Status register	      0x4001 */
	uint8_t crtc_data;		/* CTR controller data reg    0x4002 */
	uint8_t f3[6];
	uint8_t line_mover_rep_rule;	/* Line move rep rule		     */
	uint8_t :8, :8;
	uint8_t line_mover_width;	/* Line move width		     */
	uint8_t f4[0xff3];
	uint8_t width;			/* width in tiles	      0x5001 */
	uint8_t :8;
	uint8_t height;			/* height in tiles	      0x5003 */
	uint8_t f5[3];
	uint8_t rep_rule;		/* replacement rule	      0x5007 */
	uint8_t f6[0x6001-0x5007-1];
	uint8_t blink1;			/* blink 1		      0x6001 */
	uint8_t f7[3];
	uint8_t blink2;			/* blink 2		      0x6005 */
	uint8_t f8[3];
	uint8_t write_protect;		/* write protect	      0x6009 */
	uint8_t f9[0x6803-0x6009-1];
	uint8_t cmap_busy;		/* color map busy	      0x6803 */
	uint8_t f10[0x68b9-0x6803-1];
	uint8_t creg_select;		/* color map register select  0x68b8 */
	uint8_t f11[0x68f1-0x68b9-1];
	uint8_t cmap_write;		/* color map write trigger    0x68f1 */
	uint8_t f12[0x69b3-0x68f1-1];
	uint8_t cmap_red;		/* red value register	      0x69b3 */
	uint8_t :8;
	uint8_t cmap_grn;		/* green value register	      0x69b5 */
	uint8_t :8;
	uint8_t cmap_blu;		/* blue value register	      0x69b6 */
};
