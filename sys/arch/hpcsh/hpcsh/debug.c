/*	$NetBSD: debug.c,v 1.2.2.3 2002/06/23 17:37:01 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include "debug_hpc.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/debug.h>
#include <machine/bootinfo.h>

#ifdef HPC_DEBUG_INTERRUPT_MONITOR
static struct intr_state_rgb16 {
	int cnt;
	int phase;
	/* R:G:B = [15:11][10:5][4:0] */
	u_int16_t color;
} __intr_state_rgb16[] = {
	{ 0, 0, RGB565_BLACK },
	{ 0, 0, RGB565_RED },
	{ 0, 0, RGB565_GREEN },
	{ 0, 0, RGB565_YELLOW },
	{ 0, 0, RGB565_BLUE },
	{ 0, 0, RGB565_MAGENTA },
	{ 0, 0, RGB565_CYAN },
	{ 0, 0, RGB565_WHITE },
};

void
__dbg_heart_beat(enum heart_beat cause) /* 16bpp R:G:B = 5:6:5 only */
{
#define LINE_STEP	2
	struct intr_state_rgb16 *intr_state_rgb16 = 
	    &__intr_state_rgb16[cause & 0x7];
	u_int16_t *fb = (u_int16_t *)bootinfo->fb_addr;
	int hline = bootinfo->fb_width;
	u_int16_t color = intr_state_rgb16->color;
	int i;

	fb += (cause & 0x7) * bootinfo->fb_line_bytes * LINE_STEP;
	if (++intr_state_rgb16->cnt > hline)
		intr_state_rgb16->cnt = 0, intr_state_rgb16->phase ^= 1;
	
	for (i = 0; i < 8; i++)
		*(fb + i) = color;
	*(fb + intr_state_rgb16->cnt) =
	    intr_state_rgb16->phase ? ~color : color;
#undef LINE_STEP
}
#endif /* HPC_DEBUG_INTERRUPT_MONITOR */
