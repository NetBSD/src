/*	$NetBSD: gs.c,v 1.2 2003/07/15 02:54:37 lukem Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: gs.c,v 1.2 2003/07/15 02:54:37 lukem Exp $");

#include <sys/param.h>

#include <playstation2/playstation2/sifbios.h>
#include <playstation2/ee/eevar.h>
#include <playstation2/ee/gsvar.h>
#include <playstation2/ee/gsreg.h>
#include <playstation2/ee/gifreg.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC	static
#endif

STATIC const struct gs_crt_param {
	int w, h, dvemode;
	u_int64_t smode1, smode2, srfsh, synch1, synch2, syncv, display;
} gs_crt_param[] = {
	[NTSC_NONINTER] = {
		.w	= 640,
		.h	= 240,
		.dvemode= 0,
		.smode1	= SMODE1(0, 1, 1, 1, 1, 0, 0, 0, 0, 0,
		    4, 0, 0, 1, 1, 0, 2, 0, 1, 32, 4),
		.smode2 = SMODE2(0, 0, 0),
		.srfsh	= SRFSH(8),
		.synch1	= SYNCH1(254, 1462, 124, 222, 64),
		.synch2	= SYNCH2(1652, 1240),
		.syncv	= SYNCV(6, 480, 6, 26, 6, 2),
		.display=DISPLAY(239, 2559, 0, 3, 25, 632)
	},
	[NTSC_INTER] = {
		.w	= 640,
		.h	= 480,
		.dvemode= 0,
		.smode1	= SMODE1(0, 1, 1, 1, 1, 0, 0, 0, 0, 0,
		    4, 0, 0, 1, 1, 0, 2, 0, 1, 32, 4),
		.smode2	= SMODE2(0, 0, 1),
		.srfsh	= SRFSH(8),
		.synch1	= SYNCH1(254, 1462, 124, 222, 64),
		.synch2	= SYNCH2(1652, 1240),
		.syncv	= SYNCV(6, 480, 6, 26, 6, 1),
		.display= DISPLAY(479, 2559, 0, 3, 50, 632)
	},
	[PAL_NONINTER] = { 
		.w	= 640,
		.h	= 288,
		.dvemode= 1,
		.smode1	= SMODE1(0, 1, 1, 1, 1, 0, 0, 0, 0, 0,
		    4, 0, 0, 1, 1, 0, 3, 0, 1, 32, 4),
		.smode2	= SMODE2(0, 0, 0),
		.srfsh	= SRFSH(8),
		.synch1	= SYNCH1(254, 1474, 127, 262, 48),
		.synch2	= SYNCH2(1680, 1212),
		.syncv	= SYNCV(5, 576, 5, 33, 5, 4),
		.display= DISPLAY(287, 2559, 0, 3, 36, 652)
	},
	[PAL_INTER] = {
		.w	= 640,
		.h	= 576,
		.dvemode= 1,
		.smode1	= SMODE1(0, 1, 1, 1, 1, 0, 0, 0, 0, 0,
		    4, 0, 0, 1, 1, 0, 3, 0, 1, 32, 4),
		.smode2	= SMODE2(0, 0, 1),
		.srfsh	= SRFSH(8),
		.synch1	= SYNCH1(254, 1474, 127, 262, 48),
		.synch2	= SYNCH2(1680, 1212),
		.syncv	= SYNCV(5, 576, 5, 33, 5, 4),
		.display= DISPLAY(575,2559,0,3,72,652)
	},
	[VESA_1A] = { 
		.w	= 640,
		.h	= 480,
		.dvemode= 2,
		.smode1	= SMODE1(1, 0, 1, 1, 1, 0, 0, 0, 0, 0,
		    2, 0, 0, 1, 0, 0, 0, 0, 1, 15, 2),
		.smode2	= SMODE2(0, 0, 0),
		.srfsh	= SRFSH(4),
		.synch1	= SYNCH1(192, 608, 192, 84, 32),
		.synch2	= SYNCH2(768, 524),
		.syncv	= SYNCV(2, 480, 0, 33, 0, 10),
		.display= DISPLAY(479, 1279, 0, 1, 34, 276)
	}
};

void
gs_init(enum gs_crt_type type)
{
	const struct gs_crt_param *p = &gs_crt_param[type];
	u_int64_t smode1 = p->smode1;

	/* GS reset */
	_reg_write_8(GS_S_CSR_REG, 1 << 9);

	/* setup PCRTC */
	_reg_write_8(GS_S_PMODE_REG, 0); /* disable circuit 1/2 */

	_reg_write_8(GS_S_SMODE1_REG, smode1 | ((u_int64_t)1 << 16));
	_reg_write_8(GS_S_SYNCH1_REG, p->synch1);
	_reg_write_8(GS_S_SYNCH2_REG, p->synch2);
	_reg_write_8(GS_S_SYNCV_REG, p->syncv);
	_reg_write_8(GS_S_SMODE2_REG, p->smode2);
	_reg_write_8(GS_S_SRFSH_REG, p->srfsh);

	if (p->dvemode == 2) { /* PLL on */
		_reg_write_8(GS_S_SMODE1_REG, smode1 & ~((u_int64_t)1 << 16));
		delay(2500);
	}

	/* start sync */
	_reg_write_8(GS_S_SMODE1_REG,
	    smode1 & ~((u_int64_t)1 << 16) & ~((u_int64_t)1 << 17));
	
	sifbios_setdve(p->dvemode);

	/* enable circuit */
	_reg_write_8(GS_S_PMODE_REG, 0x66);

	/* display environment */
	_reg_write_8(GS_S_DISPLAY2_REG, p->display);
	_reg_write_8(GS_S_DISPFB2_REG, (p->w >> 6) << 9);
	_reg_write_8(GS_S_SMODE2_REG, p->smode2);
	_reg_write_8(GS_S_BGCOLOR_REG, 0);

	/* Flush GS FIFO */
	_reg_write_8(GS_S_CSR_REG, 1 << 8);

	/* GIF reset */
	_reg_write_4(GIF_CTRL_REG, 1);
}
