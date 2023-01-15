/*	$OpenBSD: topcatreg.h,v 1.2 2005/01/24 21:36:39 miod Exp $	*/
/*	$NetBSD: topcatreg.h,v 1.3 2023/01/15 06:19:45 tsutsui Exp $	*/

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
 * from: Utah $Hdr: grf_tcreg.h 1.11 92/01/21$
 *
 *	@(#)grf_tcreg.h	8.1 (Berkeley) 6/10/93
 */

#define tccm_waitbusy(regaddr) \
do { \
	while (((volatile struct tcboxfb *)(regaddr))->cmap_busy & 0x04) \
		DELAY(10); \
} while (/* CONSTCOND */0)

#define tc_waitbusy(regaddr,planes) \
do { \
	while (((volatile struct tcboxfb *)(regaddr))->busy & planes) \
		DELAY(10); \
} while (/* CONSTCOND */0)

struct tcboxfb {
	struct diofbreg regs;
	uint8_t f2[0x4040-0x5f-1];
	uint8_t vblank;			/* vertical blanking	      0x4040 */
	uint8_t :8,:8,:8;
	uint8_t busy;			/* window move active	      0x4044 */
	uint8_t :8,:8,:8;
	uint8_t vtrace_request;		/* vert retrace intr request  0x4048 */
	uint8_t :8,:8,:8;
	uint8_t move_request;		/* window move intr request   0x404C */
	uint8_t f3[0x4080-0x404c-1];
	uint8_t nblank;			/* display enable planes      0x4080 */
	uint8_t f4[0x4088-0x4080-1];
	uint8_t wen;			/* write enable plane	      0x4088 */
	uint8_t f5[0x408c-0x4088-1];
	uint8_t ren;			/* read enable plane          0x408c */
	uint8_t f6[0x4090-0x408c-1];
	uint8_t fben;			/* frame buffer write enable  0x4090 */
	uint8_t f7[0x409c-0x4090-1];
	uint8_t wmove;			/* start window move	      0x409c */
	uint8_t f8[0x40a0-0x409c-1];
	uint8_t blink;			/* enable blink planes	      0x40a0 */
	uint8_t f9[0x40a8-0x40a0-1];
	uint8_t altframe;		/* enable alternate frame     0x40a8 */
	uint8_t f10[0x40ac-0x40a8-1];
	uint8_t curon;			/* cursor control register    0x40ac */
	uint8_t f11[0x40ea-0x40ac-1];
	uint8_t prr;			/* pixel replacement rule     0x40ea */
	uint8_t f12[0x40ef-0x40ea-1];
	uint8_t wmrr;			/* move replacement rule      0x40ef */
	uint8_t f13[0x40f2-0x40ef-1];
	uint16_t source_x;		/* source x pixel #	      0x40f2 */
	uint8_t f14[0x40f6-0x40f2-2];
	uint16_t source_y;		/* source y pixel #	      0x40f6 */
	uint8_t f15[0x40fa-0x40f6-2];
	uint16_t dest_x;		/* dest x pixel #	      0x40fa */
	uint8_t f16[0x40fe -0x40fa-2];
	uint16_t dest_y;		/* dest y pixel #	      0x40fe */
	uint8_t f17[0x4102-0x40fe -2];
	uint16_t wwidth;		/* block mover pixel width    0x4102 */
	uint8_t f18[0x4106-0x4102-2];
	uint16_t wheight;		/* block mover pixel height   0x4106 */
  /* Catseye */
	uint8_t f19[0x4206-0x4106-2];
	uint16_t rug_cmdstat;		/* RUG Command/Staus	      0x4206 */
	uint8_t f20[0x4510-0x4206-2];
	uint16_t vb_select;		/* Vector/BitBlt Select	      0x4510 */
	uint16_t tcntrl;		/* Three Operand Control      0x4512 */
	uint16_t acntrl;		/* BitBlt Mode		      0x4514 */
	uint16_t pncntrl;		/* Plane Control	      0x4516 */
	uint8_t f21[0x4800-0x4516-2];
	uint16_t catseye_status;	/* Catseye Status	      0x4800 */
  /* End of Catseye */
	uint8_t f22[0x6002-0x4800-2];
	uint16_t cmap_busy;		/* Color Ram busy	      0x6002 */
	uint8_t f23[0x60b2-0x6002-2];
	uint16_t rdata;			/* color map red data	      0x60b2 */
	uint16_t gdata;			/* color map green data       0x60b4 */
	uint16_t bdata;			/* color map blue data	      0x60b6 */
	uint16_t cindex;		/* color map index	      0x60b8 */
	uint16_t plane_mask;		/* plane mask select	      0x60ba */
	uint8_t f24[0x60f0-0x60ba-2];
	uint16_t strobe;		/* color map trigger	      0x60f0 */
};
