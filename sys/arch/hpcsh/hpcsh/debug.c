/*	$NetBSD: debug.c,v 1.2.2.1 2002/02/11 20:08:18 jdolecek Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/debug.h>
#include <machine/bootinfo.h>

#ifdef INTERRUPT_MONITOR
void
__dbg_heart_beat(enum heart_beat cause) /* 16bpp R:G:B = 5:6:5 only */
{
#define LINE_STEP	2
	struct state{
		int cnt;
		int phase;
		u_int16_t color;
	};
	static struct state __state[] = {
		{ 0, 0, 0x07ff }, /* cyan */
		{ 0, 0, 0xf81f }, /* magenta */
		{ 0, 0, 0x001f }, /* blue */
		{ 0, 0, 0xffe0 }, /* yellow */
		{ 0, 0, 0x07e0 }, /* green */
		{ 0, 0, 0xf800 }, /* red */
		{ 0, 0, 0xffff }, /* white */
		{ 0, 0, 0x0000 }  /* black */
	};
	struct state *state = &__state[cause & 0x7];
	u_int16_t *fb = (u_int16_t *)bootinfo->fb_addr;
	int hline = bootinfo->fb_width;
	u_int16_t color = state->color;
	int i;

	fb += (cause & 0x7) * bootinfo->fb_line_bytes * LINE_STEP;
	if (++state->cnt > hline)
		state->cnt = 0, state->phase ^= 1;
	
	for (i = 0; i < 8; i++)
		*(fb + i) = color;
	*(fb + state->cnt) = state->phase ? ~color : color;
#undef LINE_STEP
}
#endif /* INTERRUPT_MONITOR */
