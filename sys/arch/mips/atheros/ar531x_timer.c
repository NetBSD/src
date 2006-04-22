/* $Id: ar531x_timer.c,v 1.1.8.2 2006/04/22 11:37:42 simonb Exp $ */
/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * This code was written by Garrett D'Amore for the Champaign-Urbana
 * Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ar531x_timer.c,v 1.1.8.2 2006/04/22 11:37:42 simonb Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <mips/atheros/include/ar531xreg.h>
#include <mips/atheros/include/ar531xvar.h>

void
ar531x_cal_timer(void)
{
	uint32_t	wisoc = GETSYSREG(AR531X_SYSREG_REVISION);

	uint32_t	predivmask;
	uint32_t	predivshift;
	uint32_t	multmask;
	uint32_t	multshift;
	uint32_t	doublermask;
	uint32_t	divisor;
	uint32_t	multiplier;
	uint32_t	cpufreq;
	uint32_t	clockctl;

	const int	predivide_table[4] = { 1, 2, 4, 5 };

	/* XXX: in theory we might be able to get clock from bootrom */

	/*
	 * This logic looks at the clock control register and
	 * determines the actual CPU frequency.  These parts lack any
	 * kind of real-time clock on them, but the cpu clocks should
	 * be very accurate -- WiFi requires usec resolution timers.
	 */

	if (AR531X_REVISION_MAJOR(wisoc) == AR531X_REVISION_MAJ_AR2313) {
		predivmask = AR2313_CLOCKCTL_PREDIVIDE_MASK;
		predivshift = AR2313_CLOCKCTL_PREDIVIDE_SHIFT;
		multmask = AR2313_CLOCKCTL_MULTIPLIER_MASK;
		multshift = AR2313_CLOCKCTL_MULTIPLIER_SHIFT;
		doublermask = AR2313_CLOCKCTL_DOUBLER_MASK;
	} else {
		predivmask = AR5312_CLOCKCTL_PREDIVIDE_MASK;
		predivshift = AR5312_CLOCKCTL_PREDIVIDE_SHIFT;
		multmask = AR5312_CLOCKCTL_MULTIPLIER_MASK;
		multshift = AR5312_CLOCKCTL_MULTIPLIER_SHIFT;
		doublermask = AR5312_CLOCKCTL_DOUBLER_MASK;
	}

	clockctl = GETSYSREG(AR531X_SYSREG_CLOCKCTL);
	divisor = predivide_table[(clockctl & predivmask) >> predivshift];
	multiplier = (clockctl & multmask) >> multshift;

	if (clockctl & doublermask)
		multiplier <<= 1;

	cpufreq = (40000000 / divisor) * multiplier;

	/* MIPS 4Kc CP0 counts every other clock */
	if (mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		cpufreq /= 2;

	curcpu()->ci_cycles_per_hz = (cpufreq + hz / 2) / hz;

	/* XXX: i don't understand this logic, it was borrowed from Malta */
	curcpu()->ci_divisor_delay = ((cpufreq + 500000) / 1000000);
	MIPS_SET_CI_RECIPRICAL(curcpu());

	/*
	 * Get correct cpu frequency if the CPU runs at twice the
	 * external/cp0-count frequency.
	 */
	curcpu()->ci_cpu_freq = cpufreq;
	if (mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		curcpu()->ci_cpu_freq *= 2;
}
