/*	$NetBSD: ip20.c,v 1.1 2003/11/22 03:35:44 sekiya Exp $	*/	

/*
 * Copyright (c) 2002,2003 Steve Rumble
 * Copyright (c) 2001 Rafal K. Boni
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include "opt_cputype.h"
#include "opt_machtypes.h"

#ifdef IP20

#include <sys/param.h>
#include <sys/proc.h> 
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/sysconf.h>
#include <machine/machtype.h>
#include <mips/locore.h>

#include <mips/cache.h>

u_int32_t next_clk_intr;
u_int32_t missed_clk_intrs;

static struct evcnt mips_int5_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 5 (clock)");

static struct evcnt mips_spurint_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "spurious interrupts");

void		ip20_init(void);
void            ip22_bus_reset(void);
int             ip22_local0_intr(void);
int             ip22_local1_intr(void);
int             ip22_mappable_intr(void *);
void            ip22_intr(u_int, u_int, u_int, u_int);
void            ip22_intr_establish(int, int, int (*)(void *), void *);

unsigned long   ip22_clkread(void);
unsigned long   ip22_cal_timer(u_int32_t, u_int32_t);

extern u_int32_t int23addr;

void 
ip20_init(void)
{
	u_int i;
	u_int32_t sysid; 
	unsigned long cps;
	unsigned long ctrdiff[3];

	mach_type = MACH_SGI_IP20;

	/* enable watchdog timer, clear it */
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fa00004) |= 0x100;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fa00014) = 0;

	sysid = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd0000);
	
	mach_subtype = 0;
	mach_boardrev = (sysid & 0x7000) >> 12;

	printf("machine HP2 (BlackJack), board rev %d\n", mach_boardrev); 

	int23addr = 0x1fb801c0;

	/* Reset timer interrupts */
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fb801e0) = 3;

	/* Clean out interrupt masks */
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fb801c0 + 0x04) = 0x00;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fb801c0 + 0x0c) = 0x00;

	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fb801c0 + 0x14) = 0x00;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fb801c0 + 0x18) = 0x00;

	platform.iointr = ip22_intr;
	platform.bus_reset = ip22_bus_reset;
	platform.intr_establish = ip22_intr_establish;

	biomask = MIPS_INT_MASK_0 | MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1;
	netmask = MIPS_INT_MASK_0 | MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1;
	ttymask = MIPS_INT_MASK_0 | MIPS_INT_MASK_1 | MIPS_SOFT_INT_MASK_0 |
						      MIPS_SOFT_INT_MASK_1;
	clockmask = MIPS_INT_MASK & ~MIPS_INT_MASK_4; 

	/* Prime cache */
	ip22_cal_timer(int23addr + 0x3c, int23addr + 0x38);

	cps = 0;
	for(i = 0; i < sizeof(ctrdiff) / sizeof(ctrdiff[0]); i++) {
	    do {
		ctrdiff[i] = ip22_cal_timer(int23addr + 0x3c, int23addr + 0x38);
	    } while (ctrdiff[i] == 0);

	    cps += ctrdiff[i];
	}

	cps = cps / (sizeof(ctrdiff) / sizeof(ctrdiff[0]));

	printf("Timer calibration, got %lu cycles (%lu, %lu, %lu)\n", cps, 
				ctrdiff[0], ctrdiff[1], ctrdiff[2]);

	platform.clkread = ip22_clkread;

	/* Counter on R4k/R4400 counts at half the CPU frequency */
	curcpu()->ci_cpu_freq = 2 * cps * hz;
	curcpu()->ci_cycles_per_hz = curcpu()->ci_cpu_freq / (2 * hz);
	curcpu()->ci_divisor_delay = curcpu()->ci_cpu_freq / (2 * 1000000);
	MIPS_SET_CI_RECIPRICAL(curcpu());

	evcnt_attach_static(&mips_int5_evcnt);
	evcnt_attach_static(&mips_spurint_evcnt);

	printf("CPU clock speed = %lu.%02luMhz\n",
				curcpu()->ci_cpu_freq / 1000000,
				(curcpu()->ci_cpu_freq / 10000) % 100);
}

#endif	/* IP20 */
