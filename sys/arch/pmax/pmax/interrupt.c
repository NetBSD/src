/*	$NetBSD: interrupt.c,v 1.13.6.1 2008/01/08 22:10:17 bouyer Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.13.6.1 2008/01/08 22:10:17 bouyer Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <mips/psl.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/intr.h>

struct evcnt pmax_clock_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "clock", "intr");
struct evcnt pmax_fpu_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "fpu", "intr");
struct evcnt pmax_memerr_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "memerr", "intr");

extern void MachFPInterrupt(unsigned, unsigned, unsigned, struct frame *);

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
cpu_intr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
	struct cpu_info *ci;

	ci = curcpu();
	ci->ci_idepth++;
	uvmexp.intrs++;

	/* device interrupts */
	if (ipending & (MIPS_INT_MASK_0|MIPS_INT_MASK_1|MIPS_INT_MASK_2|
			MIPS_INT_MASK_3|MIPS_INT_MASK_4)) {
		(*platform.iointr)(status, cause, pc, ipending);
	}
	/* FPU notification */
	if (ipending & MIPS_INT_MASK_5) {
		if (!USERMODE(status))
			goto kerneltouchedFPU;
		pmax_fpu_evcnt.ev_count++;
#if !defined(SOFTFLOAT)
		MachFPInterrupt(status, cause, pc, curlwp->l_md.md_regs);
#endif
	}

	ci->ci_idepth--;

#ifdef notyet
	/* For __HAVE_FAST_SOFTINTS */
	ipending &= (MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;
	_clrsoftintr(ipending);
	mips_softint_dispatch(ipending);
#endif

	return;
kerneltouchedFPU:
	panic("kernel used FPU: PC %x, CR %x, SR %x", pc, cause, status);
}

const int *ipl2spl_table;

ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._spl = ipl2spl_table[ipl]};
}
