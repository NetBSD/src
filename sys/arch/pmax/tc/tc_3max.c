/*	$NetBSD: tc_3max.c,v 1.7 2000/06/04 19:14:55 cgd Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: tc_3max.c,v 1.7 2000/06/04 19:14:55 cgd Exp $ ");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/tc/tcvar.h>
#include <pmax/pmax/kn02.h>

/*
 * 3MAX has 8 TC slot address space starting at 0x1e00.0000 with 4MB
 * range for each.  Three option slots are available as #0,1,2.  Two
 * devices on baseboard, ASC SCSI and LANCE Ether, are designed as TC
 * option cards and populated in distinct slots.  Slot #7, which
 * contains RTC and serial chip, forms 3MAX system base.
 */
static struct tc_slotdesc tc_kn02_slots [8] = {
    { KV(KN02_PHYS_TC_0_START), C(SYS_DEV_OPT0),  },	/* 0 - opt slot 0 */
    { KV(KN02_PHYS_TC_1_START), C(SYS_DEV_OPT1),  },	/* 1 - opt slot 1 */
    { KV(KN02_PHYS_TC_2_START), C(SYS_DEV_OPT2),  },	/* 2 - opt slot 2 */
    { KV(KN02_PHYS_TC_3_START), C(SYS_DEV_BOGUS), },	/* 3 - unused */
    { KV(KN02_PHYS_TC_4_START), C(SYS_DEV_BOGUS), },	/* 4 - unused */
    { KV(KN02_PHYS_TC_5_START), C(SYS_DEV_SCSI),  },	/* 5 - b`board SCSI */
    { KV(KN02_PHYS_TC_6_START), C(SYS_DEV_LANCE), },	/* 6 - b`board LANCE */
    { KV(KN02_PHYS_TC_7_START), C(SYS_DEV_BOGUS), },	/* 7 - system CSRs */
};

const struct tc_builtin tc_kn02_builtins[] = {
        { "KN02SYS ",   7, 0x0, C(SYS_DEV_BOGUS), },
        { "PMAD-AA ",   6, 0x0, C(SYS_DEV_LANCE), },
        { "PMAZ-AA ",   5, 0x0, C(SYS_DEV_SCSI), },
};

struct tcbus_attach_args kn02_tc_desc = {
	NULL, 0,
  	TC_SPEED_25_MHZ,
	KN02_TC_NSLOTS, tc_kn02_slots,
	3, tc_kn02_builtins,
	NULL, NULL, NULL,
	NULL,
};
