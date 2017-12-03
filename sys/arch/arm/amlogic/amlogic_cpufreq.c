/* $NetBSD: amlogic_cpufreq.c,v 1.5.16.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "locators.h"

#include "opt_amlogic.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amlogic_cpufreq.c,v 1.5.16.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/xcall.h>
#include <sys/sysctl.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_crureg.h>
#include <arm/amlogic/amlogic_var.h>

#include <arm/cortex/a9tmr_var.h>

static u_int cpufreq_busy;
static struct sysctllog *cpufreq_log;
static int cpufreq_node_target, cpufreq_node_current, cpufreq_node_available;
static u_int (*cpufreq_set_rate)(u_int);
static u_int (*cpufreq_get_rate)(void);
static size_t (*cpufreq_get_available)(u_int *, size_t);

#define AMLOGIC_CPUFREQ_MAX	16

static void	amlogic_cpufreq_cb(void *, void *);
static int	amlogic_cpufreq_freq_helper(SYSCTLFN_PROTO);
static char 	amlogic_cpufreq_available[AMLOGIC_CPUFREQ_MAX * 5];

static u_int	meson8b_cpu_set_rate(u_int);
static u_int	meson8b_cpu_get_rate(void);
static size_t	meson8b_cpu_get_available(u_int *, size_t);

#define CBUS_READ(x)		\
	bus_space_read_4(&armv7_generic_bs_tag, amlogic_core_bsh, \
			 AMLOGIC_CBUS_OFFSET + (x))
#define CBUS_WRITE(x, v)	\
	bus_space_write_4(&armv7_generic_bs_tag, amlogic_core_bsh, \
			  AMLOGIC_CBUS_OFFSET + (x), (v))
#define CBUS_SET_CLEAR(x, s, c)	\
        amlogic_reg_set_clear(&armv7_generic_bs_tag, amlogic_core_bsh, \
                              AMLOGIC_CBUS_OFFSET + (x), (s), (c))

void
amlogic_cpufreq_bootstrap(void)
{
	u_int availfreq[AMLOGIC_CPUFREQ_MAX];

	cpufreq_set_rate = &meson8b_cpu_set_rate;
	cpufreq_get_rate = &meson8b_cpu_get_rate;
	cpufreq_get_available = &meson8b_cpu_get_available;

	if (cpufreq_get_available(availfreq, AMLOGIC_CPUFREQ_MAX) > 0) {
		if (cpufreq_set_rate(availfreq[0]) == 0) {
			amlogic_cpufreq_cb(NULL, NULL);
		}
	}
}

void
amlogic_cpufreq_init(void)
{
	const struct sysctlnode *node, *cpunode, *freqnode;
	u_int availfreq[AMLOGIC_CPUFREQ_MAX];
	size_t nfreq;
	int error;

	nfreq = cpufreq_get_available(availfreq, AMLOGIC_CPUFREQ_MAX);
	if (nfreq == 0)
		return;

	KASSERT(nfreq <= AMLOGIC_CPUFREQ_MAX);

	for (int i = 0; i < nfreq; i++) {
		char buf[6];
		snprintf(buf, sizeof(buf), i ? " %u" : "%u", availfreq[i]);
		strcat(amlogic_cpufreq_available, buf);
	}

	error = sysctl_createv(&cpufreq_log, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
	if (error)
		goto sysctl_failed;
	error = sysctl_createv(&cpufreq_log, 0, &node, &cpunode,
	    0, CTLTYPE_NODE, "cpu", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	error = sysctl_createv(&cpufreq_log, 0, &cpunode, &freqnode,
	    0, CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;

	error = sysctl_createv(&cpufreq_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "target", NULL,
	    amlogic_cpufreq_freq_helper, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	cpufreq_node_target = node->sysctl_num;

	error = sysctl_createv(&cpufreq_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "current", NULL,
	    amlogic_cpufreq_freq_helper, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	cpufreq_node_current = node->sysctl_num;

	error = sysctl_createv(&cpufreq_log, 0, &freqnode, &node,
	    0, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, amlogic_cpufreq_available, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	cpufreq_node_available = node->sysctl_num;

	return;

sysctl_failed:
	aprint_error("cpufreq: couldn't create sysctl nodes (%d)\n", error);
	sysctl_teardown(&cpufreq_log);
}

static void
amlogic_cpufreq_cb(void *arg1, void *arg2)
{
	struct cpu_info *ci = curcpu();

	ci->ci_data.cpu_cc_freq = cpufreq_get_rate() * 1000000;
}

static int
amlogic_cpufreq_freq_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int fq, oldfq = 0, error;
	uint64_t xc;

	node = *rnode;
	node.sysctl_data = &fq;

	fq = cpufreq_get_rate();
	if (rnode->sysctl_num == cpufreq_node_target)
		oldfq = fq;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (fq == oldfq || rnode->sysctl_num != cpufreq_node_target)
		return 0;

	if (atomic_cas_uint(&cpufreq_busy, 0, 1) != 0)
		return EBUSY;

	error = cpufreq_set_rate(fq);
	if (error == 0) {
		xc = xc_broadcast(0, amlogic_cpufreq_cb, NULL, NULL);
		xc_wait(xc);

		pmf_event_inject(NULL, PMFE_SPEED_CHANGED);
	}

	atomic_dec_uint(&cpufreq_busy);

	return error;
}

/*
 * meson8b
 */
static const u_int meson8b_rates[] = {
	1536, 1488, 1320, 1200, 1008, 816, 720, 600, 504, 408, 312
};

static size_t
meson8b_cpu_get_available(u_int *pavail, size_t maxavail)
{
	KASSERT(__arraycount(meson8b_rates) <= maxavail);

	memcpy(pavail, meson8b_rates, sizeof(meson8b_rates));

	return __arraycount(meson8b_rates);
}

static u_int
meson8b_cpu_get_rate(void)
{
	return amlogic_get_rate_a9() / 1000000;
}

static u_int
meson8b_cpu_set_rate(u_int rate)
{
	const uint32_t xtal_rate = amlogic_get_rate_xtal();
	const u_int old_rate = meson8b_cpu_get_rate();
	u_int new_rate = 0;

	for (int i = 0; i < __arraycount(meson8b_rates); i++) {
		if (meson8b_rates[i] == rate) {
			new_rate = meson8b_rates[i] * 1000000;
			break;
		}
	}
	if (new_rate == 0) {
		return EINVAL;
	}
	if (old_rate == new_rate) {
		return 0;
	}

	uint32_t cntl0 = CBUS_READ(HHI_SYS_CPU_CLK_CNTL0_REG);
	uint32_t cntl = CBUS_READ(HHI_SYS_PLL_CNTL_REG);

	u_int new_mul = new_rate / xtal_rate;
	u_int new_div = 1;
	u_int new_od = 0;

	if (new_rate < 600 * 1000000) {
		new_od = 2;
		new_mul *= 4;
	} else if (new_rate < 1200 * 1000000) {
		new_od = 1;
		new_mul *= 2;
	}

	/*
	 * XXX make some assumptions about the state of cpu clk cntl regs
	 */
	if ((cntl0 & HHI_SYS_CPU_CLK_CNTL0_CLKSEL) == 0)
		return EIO;
	if (__SHIFTOUT(cntl0, HHI_SYS_CPU_CLK_CNTL0_PLLSEL) != 1)
		return EIO;
	if (__SHIFTOUT(cntl0, HHI_SYS_CPU_CLK_CNTL0_SOUTSEL) != 0)
		return EIO;

	cntl &= ~HHI_SYS_PLL_CNTL_MUL;
	cntl |= __SHIFTIN(new_mul, HHI_SYS_PLL_CNTL_MUL);
	cntl &= ~HHI_SYS_PLL_CNTL_DIV;
	cntl |= __SHIFTIN(new_div, HHI_SYS_PLL_CNTL_DIV);
	cntl &= ~HHI_SYS_PLL_CNTL_OD;
	cntl |= __SHIFTIN(new_od, HHI_SYS_PLL_CNTL_OD);

	/* Switch CPU to XTAL clock */
	CBUS_SET_CLEAR(HHI_SYS_CPU_CLK_CNTL0_REG, 0,
	    HHI_SYS_CPU_CLK_CNTL0_CLKSEL);

	delay((100 * old_rate) / xtal_rate);

	/* Update multiplier */
	do {
		CBUS_WRITE(HHI_SYS_PLL_CNTL_REG, cntl);

		/* Switch CPU to sys pll */
		CBUS_SET_CLEAR(HHI_SYS_CPU_CLK_CNTL0_REG,
		    HHI_SYS_CPU_CLK_CNTL0_CLKSEL, 0);

		delay((500 * old_rate) / new_rate);
	} while (!(CBUS_READ(HHI_SYS_PLL_CNTL_REG) & HHI_SYS_PLL_CNTL_LOCK));

	if (!cold) {
		a9tmr_update_freq(amlogic_get_rate_a9periph());
	}

	return 0;
}
