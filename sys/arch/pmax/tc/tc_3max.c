/*	$NetBSD: tc_3max.c,v 1.3 1999/11/15 09:50:43 nisimura Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: tc_3max.c,v 1.3 1999/11/15 09:50:43 nisimura Exp $ ");


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/tc/tcvar.h>
#include <pmax/pmax/kn02.h>

/* 3MAX (kn02) TURBOchannel slots  */
/* slot addreseses */
struct tc_slotdesc tc_kn02_slots [8] = {
       	{ TC_KV(KN02_PHYS_TC_0_START), TC_C(0), },	/* tc option slot 0 */
	{ TC_KV(KN02_PHYS_TC_1_START), TC_C(1), },	/* tc option slot 1 */
	{ TC_KV(KN02_PHYS_TC_2_START), TC_C(2), },	/* tc option slot 2 */
	{ TC_KV(KN02_PHYS_TC_3_START), TC_C(3), },	/*  - reserved */
	{ TC_KV(KN02_PHYS_TC_4_START), TC_C(4), },	/*  - reserved */
	{ TC_KV(KN02_PHYS_TC_5_START), TC_C(5), },	/* SCSI on b`board */
	{ TC_KV(KN02_PHYS_TC_6_START), TC_C(6), },	/* b'board Ether */
	{ TC_KV(KN02_PHYS_TC_7_START), TC_C(7), }	/* system CSR, etc. */
};

int tc_kn02_nslots = sizeof(tc_kn02_slots) / sizeof(tc_kn02_slots[0]);

#define TC_KN02_DEV_IOASIC     -1
#define TC_KN02_DEV_ETHER	6
#define TC_KN02_DEV_SCSI	5

const struct tc_builtin tc_kn02_builtins[] = {
	{ "KN02SYS ",	7, 0x0, TC_C(TC_KN02_DEV_IOASIC) /* TC_C(7)*/ },
	{ "PMAD-AA ",	6, 0x0, TC_C(TC_KN02_DEV_ETHER)  /* TC_C(6)*/ },
	{ "PMAZ-AA ",	5, 0x0, TC_C(TC_KN02_DEV_SCSI)   /* TC_C(5)*/ }
};


struct tcbus_attach_args kn02_tc_desc = {
	NULL,
	0,
  	TC_SPEED_25_MHZ,
	8, tc_kn02_slots,
	3, tc_kn02_builtins,	/*XXX*/
	NULL,
	NULL,
};
