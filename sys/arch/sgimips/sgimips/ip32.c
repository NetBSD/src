/*	$NetBSD: ip32.c,v 1.17 2003/07/15 03:35:55 lukem Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * Copyright (c) 2001, 2002 Rafal K. Boni
 * Copyright (c) 2002 Christopher Sekiya
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip32.c,v 1.17 2003/07/15 03:35:55 lukem Exp $");

#include "opt_machtypes.h"

#ifdef IP32
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/sysconf.h>
#include <mips/locore.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include <sgimips/dev/crimereg.h>
#include <sgimips/dev/macereg.h>

void		ip32_init(void);
void		ip32_bus_reset(void);
void 		ip32_intr(u_int, u_int, u_int, u_int);
void		ip32_intr_establish(int, int, int (*)(void *), void *);
unsigned long	ip32_clkread(void);

void		crime_intr(u_int);
void		*crime_intr_establish(int, int, int, int (*)(void *), void *);
void		mace_intr(u_int);

u_int32_t next_clk_intr;
u_int32_t missed_clk_intrs;
static unsigned long last_clk_intr;

static struct evcnt mips_int0_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 0 (CRIME)");

static struct evcnt mips_int5_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 5 (clock)");

static struct evcnt mips_spurint_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "spurious interrupts");

void
ip32_init(void)
{
	u_int64_t baseline;
	u_int32_t cps;

	/* 
	 * NB: don't enable watchdog here as we do on IP22, since the
	 * fixed -- and overly short -- duration of the IP32 watchdog
	 * means that more likely than not it will reset the machine
	 * even before autoconfiguration is complete.  This is due to
	 * the fact that the watchdog is only kept at bay by the IP32
	 * interrupt handler and autoconfiguration is all done with
	 * interrupts disabled.
	 */

	/* Reset CRIME CPU & memory error registers */
	*(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_CPU_ERROR_STAT) = 0;
	*(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_MEM_ERROR_STAT) = 0;

#define WAIT_MS 50
	baseline = *(volatile u_int64_t *)
	    MIPS_PHYS_TO_KSEG1(CRIME_TIME) & CRIME_TIME_MASK;
	cps = mips3_cp0_count_read();

	while (((*(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(CRIME_TIME)
	    & CRIME_TIME_MASK) - baseline) < WAIT_MS * 1000000 / 15)
		continue;
	cps = mips3_cp0_count_read() - cps;
	cps = cps / 5;
#undef WAIT_MS
	
	/* Counter on R4k/R4400/R4600/R5k counts at half the CPU frequency */
	curcpu()->ci_cpu_freq = cps * 2 * hz;
	curcpu()->ci_cycles_per_hz = curcpu()->ci_cpu_freq / (2 * hz);
	curcpu()->ci_divisor_delay = curcpu()->ci_cpu_freq / (2 * 1000000);
	MIPS_SET_CI_RECIPRICAL(curcpu());

	platform.iointr = ip32_intr;
	platform.clkread = ip32_clkread;
	platform.bus_reset = ip32_bus_reset;
	platform.intr_establish = ip32_intr_establish;

	biomask = 0x0700;
	netmask = 0x0700;
	ttymask = 0x0700;
	clockmask = 0x8700;

	evcnt_attach_static(&mips_int0_evcnt);
	evcnt_attach_static(&mips_int5_evcnt);
	evcnt_attach_static(&mips_spurint_evcnt);

	printf("CPU clock speed = %lu.%02luMhz\n", 
				curcpu()->ci_cpu_freq / 1000000,
			    	(curcpu()->ci_cpu_freq / 10000) % 100);
}

void
ip32_bus_reset(void)
{
	*(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_CPU_ERROR_STAT) = 0;
	*(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_MEM_ERROR_STAT) = 0;
}

/*
 * NB: Do not re-enable interrupts here -- reentrancy here can cause all
 * sorts of Bad Things(tm) to happen, including kernel stack overflows.
 */
void
ip32_intr(status, cause, pc, ipending)
	u_int32_t status;
	u_int32_t cause;
	u_int32_t pc;
	u_int32_t ipending;
{
	u_int32_t newcnt;
	struct clockframe cf;
	u_int64_t crime_intstat, crime_intmask, crime_ipending;

	/* enable watchdog timer, clear it */
	*(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_CONTROL) |= 
		CRIME_CONTROL_DOG_ENABLE;
	*(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_WATCHDOG) = 0;

#if 1
	/* 
	 * XXXrkb: Even if this code makes sense (which I'm not sure of;
	 * the magic number of "6" seems to correspond to capability bits
	 * of the card/slot in question -- not 66 Mhz capable, fast B2B
	 * capable and a medium DEVSEL timing -- and also seem to corres-
	 * pond to on-reset values of this register), these errors should
	 * be dealt with in the MACE PCI interrupt, not here!
	 *
	 * The 0x00100000 is MACE_PERR_CONFIG_ADDR, so this code should
	 * panic on any other PCI errors except simple address errors on
	 * config. space accesses.  This also seems wrong, but I lack the
	 * PCI clue to figure out how to deal with other error ATM...
	 */
	if ((*(volatile u_int32_t *) MIPS_PHYS_TO_KSEG1(MACE_PCI_ERROR_FLAGS) & ~0x00100000) != 6)
		panic("pcierr: %x %x",
				*(volatile u_int32_t *) MIPS_PHYS_TO_KSEG1(MACE_PCI_ERROR_ADDR),
				*(volatile u_int32_t *) MIPS_PHYS_TO_KSEG1(MACE_PCI_ERROR_FLAGS));
#endif

	if (ipending & MIPS_INT_MASK_5) {
		last_clk_intr = mips3_cp0_count_read();

		next_clk_intr += curcpu()->ci_cycles_per_hz;
		mips3_cp0_compare_write(next_clk_intr);
		newcnt = mips3_cp0_count_read();

		/* 
		 * Missed one or more clock interrupts, so let's start 
		 * counting again from the current value.
		 */
		if ((next_clk_intr - newcnt) & 0x80000000) {
		    missed_clk_intrs++;

		    next_clk_intr = newcnt + curcpu()->ci_cycles_per_hz;
		    mips3_cp0_compare_write(next_clk_intr);
		}

		cf.pc = pc;
		cf.sr = status;

		hardclock(&cf);
		mips_int5_evcnt.ev_count++;
		cause &= ~MIPS_INT_MASK_5;
	}

	if (ipending & MIPS_INT_MASK_0) {
		crime_intmask = *(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_INTMASK);
		crime_intstat = *(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_INTSTAT);

		crime_ipending = (crime_intstat & crime_intmask);

		if (crime_ipending & 0xff)
			mace_intr(crime_ipending & 0xff);

		if (crime_ipending & 0xff00)
			crime_intr(crime_ipending & 0xff00);

		if (crime_ipending & 0xffff0000) {
			/*
 			 * CRIME interrupts for CPU and memory errors
			 */
			if (crime_ipending & CRIME_INT_MEMERR) {
				u_int64_t address = *(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_MEM_ERROR_ADDR);
				u_int64_t status = *(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_MEM_ERROR_STAT);
				printf("crime: memory error address %llx status %llx\n", address << 2, status);
				ip32_bus_reset();
			}

			if (crime_ipending & CRIME_INT_CRMERR) {
				u_int64_t stat = *(volatile u_int64_t *) MIPS_PHYS_TO_KSEG1(CRIME_CPU_ERROR_STAT);
				printf("crime: cpu error %llx\n", stat);
				ip32_bus_reset();
			}
		}

		mips_int0_evcnt.ev_count++;
		cause &= ~MIPS_INT_MASK_0;
	}

	if (cause & status & MIPS_HARD_INT_MASK) 
		mips_spurint_evcnt.ev_count++;
}

unsigned long
ip32_clkread(void)
{
	uint32_t res, count;

	count = mips3_cp0_count_read() - last_clk_intr;
	MIPS_COUNT_TO_MHZ(curcpu(), count, res);
	return (res);
}

void
ip32_intr_establish(int level, int ipl, int (*func)(void *), void *arg)
{
	/* 
	 * XXXrkb: should use level to determine if this is a MACE,
	 * CRIME, MACEISA, etc. device and then vector to the right
	 * interrupt table.
	 * 
	 * Once that's done, then all IP32 drivers should use the
	 * cpu_intr_establish() function rather than calling into
	 * the individual CRIME/MACE/... interrupt establishment
	 * functions.
	 * 
	 * Finally, since interrupt handlers should always have the
	 * level associated with them, the interrupt dispatch code
	 * can map from CRIME interrupt register bits to the right
	 * set of interrupt handlers and we won't have to call all
	 * our registered ISR's blindly in hopes we'll hit the right
	 * one.
	 */
	(void) crime_intr_establish(level, IST_LEVEL, ipl, func, arg);
}
#endif	/* IP32 */
