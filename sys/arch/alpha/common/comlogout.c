/* $NetBSD: comlogout.c,v 1.4 2001/01/03 21:40:25 thorpej Exp $ */

/*
 * Copyright (c) 1998 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: comlogout.c,v 1.4 2001/01/03 21:40:25 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/frame.h>
#include <machine/cpuconf.h>
#include <machine/logout.h>

/*
 * Common PAL Logout Handling Code
 */

/* (pretty much just structured console printout right now) */

void
ev5_logout_print(mc_hdr_ev5 *ev5lohdrp, mc_uc_ev5 *mcucev5p)
{
	int i;
	static const char *fmt1 = "        %-30s = 0x%l016x\n";

	printf("      Processor Machine Check (%x), Code 0x%lx\n",
	    ALPHA_PROC_MCHECK, ev5lohdrp->mcheck_code);
	/* Print PAL fields */
	for (i = 0; i < 24; i += 2) {
		printf("\tPAL temp[%d-%d]\t\t= 0x%16lx 0x%16lx\n", i, i+1,
		    mcucev5p->paltemp[i], mcucev5p->paltemp[i+1]);
	}
	for (i = 0; i < 8; i += 2) {
		printf("\tshadow[%d-%d]\t\t\t= 0x%16lx 0x%16lx\n", i, i+1,
		    mcucev5p->shadow[i], mcucev5p->shadow[i+1]);
	}
	printf("\n");
	printf(fmt1, "Excepting Instruction Addr", mcucev5p->exc_addr);
	printf(fmt1, "Summary of arithmetic traps", mcucev5p->exc_sum);
	printf(fmt1, "Exception mask", mcucev5p->exc_mask);
	printf(fmt1, "Base address for PALcode", mcucev5p->pal_base);
	printf(fmt1, "Interrupt Status Reg", mcucev5p->isr);
	printf(fmt1, "Current setup of EV5 IBOX", mcucev5p->icsr);
	printf(fmt1, (mcucev5p->ic_perr_stat & 0x800)?
	       "I-CACHE Reg Data parity error" :
	       "I-CACHE Reg Tag parity error", mcucev5p->ic_perr_stat);
	printf(fmt1, "D-CACHE error Reg", mcucev5p->dc_perr_stat);

	if (mcucev5p->dc_perr_stat & 0x2) {
		switch (mcucev5p->dc_perr_stat & 0x3c) {
		case 8:
			printf("        %-40s\n", "Data error in bank 1");
			break;
		case 4:
			printf("        %-40s\n", "Data error in bank 0");
			break;
		case 20:
			printf("        %-40s\n", "Tag error in bank 1");
			break;
		case 10:
			printf("        %-40s\n", "Tag error in bank 0");
			break;
		}
	}
	printf(fmt1, "Effective VA", mcucev5p->va);
	printf(fmt1, "Reason for D-stream", mcucev5p->mm_stat);
	printf(fmt1, "EV5 SCache address", mcucev5p->sc_addr);
	printf(fmt1, "EV5 SCache TAG/Data parity", mcucev5p->sc_stat);
	printf(fmt1, "EV5 BC_TAG_ADDR", mcucev5p->bc_tag_addr);
	printf(fmt1, "EV5 EI_ADDR Phys addr of Xfer", mcucev5p->ei_addr);
	printf(fmt1, "Fill Syndrome", mcucev5p->fill_syndrome);
	printf(fmt1, "ei_stat reg", mcucev5p->ei_stat);
	printf(fmt1, "ld_lock", mcucev5p->ld_lock);
}
