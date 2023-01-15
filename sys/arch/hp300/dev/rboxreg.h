/*	$OpenBSD: rboxreg.h,v 1.2 2005/01/24 21:36:39 miod Exp $	*/
/*	$NetBSD: rboxreg.h,v 1.3 2023/01/15 06:19:45 tsutsui Exp $	*/

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
 * from: Utah $Hdr: grf_rbreg.h 1.9 92/01/21$
 *
 *	@(#)grf_rbreg.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Map of the Renaissance frame buffer controller chip in memory ...
 */

#define rb_waitbusy(regaddr)						\
do {									\
	while (((volatile struct rboxfb *)(regaddr))->wbusy & 0x01)	\
		DELAY(10);						\
} while (/* CONSTCOND */0)

#define	RBOX_DUALROP(rop)	((rop) << 4 | (rop))

#define	CM1RED(fb)	((volatile struct rencm *)((fb)->regkva + 0x6400))
#define	CM1GRN(fb)	((volatile struct rencm *)((fb)->regkva + 0x6800))
#define	CM1BLU(fb)	((volatile struct rencm *)((fb)->regkva + 0x6C00))
#define	CM2RED(fb)	((volatile struct rencm *)((fb)->regkva + 0x7400))
#define	CM2GRN(fb)	((volatile struct rencm *)((fb)->regkva + 0x7800))
#define	CM2BLU(fb)	((volatile struct rencm *)((fb)->regkva + 0x7C00))

struct rencm {
	uint8_t  :8, :8, :8;
	uint8_t value;
};

struct rboxfb {
	struct diofbreg regs;
	uint8_t filler2[16359];
	uint8_t wbusy;			/* window mover is active     0x4047 */
	uint8_t filler3[0x405b - 0x4048];
	uint8_t scanbusy;		/* scan converteris active    0x405B */
	uint8_t filler3b[0x4083 - 0x405c];
	uint8_t video_enable;		/* drive vid. refresh bus     0x4083 */
	uint8_t filler4[3];
	uint8_t display_enable;		/* enable the display	      0x4087 */
	uint8_t filler5[8];
	uint32_t write_enable;		/* write enable register      0x4090 */
	uint8_t filler6[11];
	uint8_t wmove;			/* start window mover	      0x409f */
	uint8_t filler7[3];
	uint8_t blink;			/* blink register	      0x40a3 */
	uint8_t filler8[15];
	uint8_t fold;			/* fold  register	      0x40b3 */
	uint32_t opwen;			/* overlay plane write enable 0x40b4 */
	uint8_t filler9[3];
	uint8_t tmode;			/* Tile mode size	      0x40bb */
	uint8_t filler9a[3];
	uint8_t drive;			/* drive register	      0x40bf */
	uint8_t filler10[3];
	uint8_t vdrive;			/* vdrive register	      0x40c3 */
	uint8_t filler10a[0x40cb-0x40c4];
	uint8_t zconfig;		/* Z-buffer mode	      0x40cb */
	uint8_t filler11a[2];
	uint16_t tpatt;			/* Transparency pattern	      0x40ce */
	uint8_t filler11b[3];
	uint8_t dmode;			/* dither mode		      0x40d3 */
	uint8_t filler11c[3];
	uint8_t en_scan;		/* enable scan board to DTACK 0x40d7 */
	uint8_t filler11d[0x40ef-0x40d8];
	uint8_t rep_rule;		/* replacement rule	      0x40ef */
	uint8_t filler12[2];
	uint16_t source_x;		/* source x		      0x40f2 */
	uint8_t filler13[2];
	uint16_t source_y;		/* source y		      0x40f6 */
	uint8_t filler14[2];
	uint16_t dest_x;		/* dest x		      0x40fa */
	uint8_t filler15[2];
	uint16_t dest_y;		/* dest y		      0x40fe */
	uint8_t filler16[2];
	uint16_t wwidth;		/* window width		      0x4102 */
	uint8_t filler17[2];
	uint16_t wheight;		/* window height	      0x4106 */
	uint8_t filler18[18];
	uint16_t patt_x;		/* pattern x		      0x411a */
	uint8_t filler19[2];
	uint16_t patt_y;		/* pattern y		      0x411e */
	uint8_t filler20[0x8012 - 0x4120];
	uint16_t te_status;		/* transform engine status    0x8012 */
	uint8_t filler21[0x1ffff-0x8014];
};
