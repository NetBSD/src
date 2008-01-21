/* $NetBSD: systemsw.c,v 1.9.16.4 2008/01/21 09:39:12 yamt Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: systemsw.c,v 1.9.16.4 2008/01/21 09:39:12 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/cpu.h>

#include <mips/locore.h>
#include <mips/mips3_clock.h>

#include <machine/intr.h>
#include <machine/systemsw.h>


/* trivial functions for function switch */
static void	clock_init_triv(void *);
static void	cpu_intr_triv(uint32_t, uint32_t, uint32_t, uint32_t);

/* system function switch */
struct systemsw systemsw = {
	cpu_intr_triv,

	NULL,			/* clock intr arg */
	clock_init_triv,

	NULL,			/* statclock arg */
	NULL,			/* s_statclock_init: dflt no-op */
	NULL,			/* s_statclock_setrate: dflt no-op */

	NULL,			/* intr_establish */
};

int
system_set_clockfns(void *arg, void (*init)(void *))
{

	if (systemsw.s_clock_init != clock_init_triv)
		return 1;
	systemsw.s_clock_arg = arg;
	systemsw.s_clock_init = init;
	return 0;
}

static void
cpu_intr_triv(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{

	panic("cpu_intr_triv");
}

void
cpu_intr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{

	(*systemsw.s_cpu_intr)(status, cause, pc, ipending);
}

static void
clock_init_triv(void *arg)
{

	panic("clock_init_triv");
}

void
cpu_initclocks(void)
{

	(*systemsw.s_clock_init)(systemsw.s_clock_arg);

	if (systemsw.s_statclock_init != NULL)
		(*systemsw.s_statclock_init)(systemsw.s_statclock_arg);

	/*
	 * ``Disable'' the compare interrupt by setting it to it's largest
	 * value.  Each hard clock interrupt we'll reset the CP0 compare
	 * register to just bind the CP0 clock register.
	 */
	mips3_cp0_compare_write(~0u);
	mips3_cp0_count_write(0);

	mips3_init_tc();

	/*
	 * Now we can enable all interrupts including hardclock(9).
	 */
	_splnone();
}

void
setstatclockrate(int hzrate)
{

	if (systemsw.s_statclock_setrate != NULL)
		(*systemsw.s_statclock_setrate)(systemsw.s_statclock_arg,
		    hzrate);
}
