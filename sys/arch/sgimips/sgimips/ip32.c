/*	$NetBSD: ip32.c,v 1.6.2.2 2002/04/01 07:42:29 nathanw Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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

#include "opt_machtypes.h"

#ifdef IP32
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/sysconf.h>
#include <mips/locore.h>

void	ip32_init(void);
void	ip32_bus_reset(void);
void 	ip32_intr(u_int, u_int, u_int, u_int);
void	ip32_intr_establish(int, int, int (*)(void *), void *);
int	crime_intr(void *);
void	*crime_intr_establish(int, int, int, int (*)(void *), void *);

static struct evcnt mips_int5_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 5 (clock)");

static unsigned long ticks_per_hz;

void ip32_init(void)
{
	/* XXXrkb: enable watchdog timer, clear it */
	*(volatile u_int32_t *)0xb400000c |= 0x200;
	*(volatile u_int32_t *)0xb4000034 = 0;

	ticks_per_hz = 1000000;

	platform.iointr = ip32_intr;
	platform.bus_reset = ip32_bus_reset;
	platform.intr_establish = ip32_intr_establish;

	biomask = 0x7f00;
	netmask = 0x7f00;
	ttymask = 0x7f00;
	clockmask = 0xff00;

	evcnt_attach_static(&mips_int5_evcnt);
}

void
ip32_bus_reset(void)
{
	/* do nothing */
}

void
ip32_intr(status, cause, pc, ipending)
	u_int32_t status;
	u_int32_t cause;
	u_int32_t pc;
	u_int32_t ipending;
{
	int i;
	unsigned long cycles;
	struct clockframe cf;

#if 0
printf("crm: %llx %llx %llx %llx\n", *(volatile u_int64_t *)0xb4000010,
				*(volatile u_int64_t *)0xb4000018,
				*(volatile u_int64_t *)0xb4000020,
				*(volatile u_int64_t *)0xb4000028);
#endif

#if 1
	/* XXX soren Reset O2 watchdog timer */
	*(volatile u_int32_t *)0xb4000034 = 0;
#endif

#if 1
if ((*(volatile u_int32_t *)0xbf080004 & ~0x00100000) != 6)
panic("pcierr: %x %x", *(volatile u_int32_t *)0xbf080004,
    *(volatile u_int32_t *)0xbf080000);
#endif

	*(volatile u_int64_t *)0xbf310018 = 0xffffffff;
	*(volatile u_int64_t *)0xb4000018 = 0x000000000000ffff;

#if 1
	if (ipending & 0x7800)
		panic("interesting cpu_intr, pending 0x%x\n", ipending);
#endif

	if (ipending & MIPS_INT_MASK_5) {
		cycles = mips3_cp0_count_read();
		mips3_cp0_compare_write(cycles + ticks_per_hz);

		cf.pc = pc;
		cf.sr = status;

		hardclock(&cf);
		mips_int5_evcnt.ev_count++;

		cause &= ~MIPS_INT_MASK_5;
	} else if (ipending & 0x7c00)
		crime_intr(NULL);

	for (i = 0; i < 5; i++) {
		if (ipending & (MIPS_INT_MASK_0 << i))
#if 0
			if (intrtab[i].func != NULL)
				if ((*intrtab[i].func)(intrtab[i].arg))
#endif
					cause &= ~(MIPS_INT_MASK_0 << i);
	}

	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);
}

void
ip32_intr_establish(int level, int ipl, int (*func)(void *), void *arg)
{
	(void) crime_intr_establish(level, IST_LEVEL, ipl, func, arg);
}
#endif	/* IP32 */
