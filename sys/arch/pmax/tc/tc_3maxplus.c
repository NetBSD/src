/*	$NetBSD: tc_3maxplus.c,v 1.2.8.1 1999/12/27 18:33:37 wrstuden Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: tc_3maxplus.c,v 1.2.8.1 1999/12/27 18:33:37 wrstuden Exp $ ");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/tc/tcvar.h>
#include <pmax/pmax/kn03.h>

/*
 * 3MAX+ has 4 TC option slot address space starting at 0x1e00.0000.
 * TC slot size is 8MB.  Three option slots are available.  IOASIC,
 * which governs various baseboard devices like keyboard/mouse, RTC,
 * DMA assisted ASC SCSI, LANCE Ether, forms a system base.  IOASIC
 * is designed as a TC device and sits in slot #3 space.
 */
static struct tc_slotdesc tc_kn03_slots [4] = {
       	{ TC_KV(KN03_PHYS_TC_0_START), TC_C(0) },  /* 0 - tc option slot 0 */
	{ TC_KV(KN03_PHYS_TC_1_START), TC_C(1) },  /* 1 - tc option slot 1 */
	{ TC_KV(KN03_PHYS_TC_2_START), TC_C(2) },  /* 2 - tc option slot 2 */
	{ TC_KV(KN03_PHYS_TC_3_START), TC_C(3) }   /* 3 - IOASIC on b'board */
};

const struct tc_builtin tc_kn03_builtins[] = {
	{ "IOCTL   ",	3, 0x0, TC_C(3), }
};

struct tcbus_attach_args kn03_tc_desc = {
	NULL, 0,
	TC_SPEED_25_MHZ,
	KN03_TC_NSLOTS, tc_kn03_slots,
	1, tc_kn03_builtins,
	NULL, NULL,
	NULL,
};
