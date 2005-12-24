/*	$NetBSD: x68k_init.c,v 1.10 2005/12/24 22:45:40 perry Exp $	*/

/*
 * Copyright (c) 1996 Masaru Oki.
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
 *      This product includes software developed by Masaru Oki.
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
__KERNEL_RCSID(0, "$NetBSD: x68k_init.c,v 1.10 2005/12/24 22:45:40 perry Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/mfp.h>
#include <x68k/x68k/iodevice.h>
#define zsdev IODEVbase->io_inscc

volatile struct IODEVICE *IODEVbase = (volatile struct IODEVICE *) PHYS_IODEV;

void intr_reset(void);

extern int iera;
extern int ierb;
/*
 * disable all interrupt.
 */
void
intr_reset(void)
{
	/* I/O Controller */
	ioctlr.intr = 0;

	/* Internal RS-232C port */
	zsdev.zs_chan_a.zc_csr = 1;
	__asm("nop");
	zsdev.zs_chan_a.zc_csr = 0;
	__asm("nop");

	/* mouse */
	zsdev.zs_chan_b.zc_csr = 1;
	__asm("nop");
	zsdev.zs_chan_b.zc_csr = 0;
	__asm("nop");

	mfp_send_usart(0x41);

	/* MFP (hard coded interrupt vector XXX) */
	mfp_set_vr(0x40);
	mfp_set_iera(0);
	mfp_set_ierb(0);
}
