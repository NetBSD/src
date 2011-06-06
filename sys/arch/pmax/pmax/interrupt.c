/*	$NetBSD: interrupt.c,v 1.17.2.1 2011/06/06 09:06:24 jruoho Exp $	*/

/*-
 * Copyright (c) 2001, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.17.2.1 2011/06/06 09:06:24 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/cpu.h>

#include <mips/psl.h>
#include <mips/locore.h>
#include <mips/regnum.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/intr.h>

struct evcnt pmax_clock_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "clock", "intr");
EVCNT_ATTACH_STATIC(pmax_clock_evcnt);
#ifndef NOFPU
struct evcnt pmax_fpu_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "fpu", "intr");
EVCNT_ATTACH_STATIC(pmax_fpu_evcnt);
#endif
struct evcnt pmax_memerr_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "memerr", "intr");
EVCNT_ATTACH_STATIC(pmax_memerr_evcnt);

static const char * const intrnames[] = {
	"serial0",
	"serial1",
	"ether",
	"scsi",
	"optslot0",
	"optslot1",
	"optslot2",
	"dtop",
	"isdn",
	"floppy"
};

void
intr_init(void)
{
	int i;

	for (i = 0; i < MAX_DEV_NCOOKIES; i++)
		evcnt_attach_dynamic(&intrtab[i].ih_count, EVCNT_TYPE_INTR,
		    NULL, "pmax", intrnames[i]);
}

/*
 * pmax uses standard mips1 convention, wiring FPU to hard interrupt 5.
 */
void
cpu_intr(int ppl, vaddr_t pc, uint32_t status)
{
	int ipl;
	uint32_t pending;

	curcpu()->ci_data.cpu_nintr++;

	while (ppl < (ipl = splintr(&pending))) {
		/* device interrupts */
		if (pending & (MIPS_INT_MASK_0|MIPS_INT_MASK_1|MIPS_INT_MASK_2|
				MIPS_INT_MASK_3|MIPS_INT_MASK_4)) {
			(*platform.iointr)(status, pc, pending);
		}
#if !defined(NOFPU)
		/* FPU notification */
		if (pending & MIPS_INT_MASK_5) {
			if (!USERMODE(status))
				panic("kernel used FPU: "
				    "PC %#"PRIxVADDR", SR %#x", pc, status);

			pmax_fpu_evcnt.ev_count++;
			mips_fpu_intr(pc, curlwp->l_md.md_utf);
		}
#endif
	}
}
