/*	$NetBSD: ym2149.c,v 1.3 2003/07/15 01:19:52 lukem Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
 * All rights reserved.
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
 *      This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ym2149.c,v 1.3 2003/07/15 01:19:52 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <machine/iomap.h>
#include <atari/dev/ym2149reg.h>

/*
 * Yahama YM-2149 Programmable Sound Generator
 *
 * For now only an init function is here...
 */
u_char	ym2149_ioa;	/* Soft-copy of port-A			*/

void
ym2149_init()
{
	/*
	 * Initialize the sound-chip YM2149:
	 *   All sounds off, both I/O ports output.
	 */
	YM2149->sd_selr = YM_MFR;
	YM2149->sd_wdat = 0xff;

	/*
	 * Sensible value for ioa:
	 */
	YM2149->sd_selr = YM_IOA;
	YM2149->sd_wdat = ym2149_ioa = PA_FDSEL|PA_PSTROBE;
}
