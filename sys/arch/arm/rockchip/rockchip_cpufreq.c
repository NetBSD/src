/* $NetBSD: rockchip_cpufreq.c,v 1.3.18.2 2017/12/03 11:35:55 jdolecek Exp $ */

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
#include "act8846pm.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rockchip_cpufreq.c,v 1.3.18.2 2017/12/03 11:35:55 jdolecek Exp $");

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

#include <arm/rockchip/rockchip_reg.h>
#include <arm/rockchip/rockchip_crureg.h>
#include <arm/rockchip/rockchip_var.h>

#include <dev/i2c/act8846.h>

#include <arm/cortex/a9tmr_var.h>

static bus_space_tag_t bst = &armv7_generic_bs_tag;
static bus_space_handle_t cru_bsh;
static bus_space_handle_t grf_bsh;

static u_int cpufreq_busy;
static struct sysctllog *cpufreq_log;
static int cpufreq_node_target, cpufreq_node_current, cpufreq_node_available;
static u_int (*cpufreq_set_rate)(u_int);
static u_int (*cpufreq_get_rate)(void);
static size_t (*cpufreq_get_available)(u_int *, size_t);

#define ROCKCHIP_CPUFREQ_MAX	8

static void	rockchip_cpufreq_cb(void *, void *);
static int	rockchip_cpufreq_freq_helper(SYSCTLFN_PROTO);
static char 	rockchip_cpufreq_available[ROCKCHIP_CPUFREQ_MAX * 5];

static u_int	rk3188_cpu_set_rate(u_int);
static u_int	rk3188_cpu_get_rate(void);
static size_t	rk3188_cpu_get_available(u_int *, size_t);

void
rockchip_cpufreq_init(void)
{
	const struct sysctlnode *node, *cpunode, *freqnode;
	u_int availfreq[ROCKCHIP_CPUFREQ_MAX];
	size_t nfreq;
	int error;

	bus_space_subregion(bst, rockchip_core1_bsh, ROCKCHIP_CRU_OFFSET,
	    ROCKCHIP_CRU_SIZE, &cru_bsh);
	bus_space_subregion(bst, rockchip_core1_bsh, ROCKCHIP_GRF_OFFSET,
	    ROCKCHIP_GRF_SIZE, &grf_bsh);

	switch (rockchip_chip_id()) {
	case ROCKCHIP_CHIP_ID_RK3066: /* XXX */
	case ROCKCHIP_CHIP_ID_RK3188:
	case ROCKCHIP_CHIP_ID_RK3188PLUS:
		cpufreq_set_rate = &rk3188_cpu_set_rate;
		cpufreq_get_rate = &rk3188_cpu_get_rate;
		cpufreq_get_available = &rk3188_cpu_get_available;
		break;
	default:
		return;
	}

	nfreq = cpufreq_get_available(availfreq, ROCKCHIP_CPUFREQ_MAX);
	if (nfreq == 0)
		return;

	KASSERT(nfreq <= ROCKCHIP_CPUFREQ_MAX);

	for (int i = 0; i < nfreq; i++) {
		char buf[6];
		snprintf(buf, sizeof(buf), i ? " %u" : "%u", availfreq[i]);
		strcat(rockchip_cpufreq_available, buf);
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
	    rockchip_cpufreq_freq_helper, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	cpufreq_node_target = node->sysctl_num;

	error = sysctl_createv(&cpufreq_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "current", NULL,
	    rockchip_cpufreq_freq_helper, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	cpufreq_node_current = node->sysctl_num;

	error = sysctl_createv(&cpufreq_log, 0, &freqnode, &node,
	    0, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, rockchip_cpufreq_available, 0,
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
rockchip_cpufreq_cb(void *arg1, void *arg2)
{
	struct cpu_info *ci = curcpu();

	ci->ci_data.cpu_cc_freq = cpufreq_get_rate() * 1000000;
}

static int
rockchip_cpufreq_freq_helper(SYSCTLFN_ARGS)
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
		xc = xc_broadcast(0, rockchip_cpufreq_cb, NULL, NULL);
		xc_wait(xc);

		pmf_event_inject(NULL, PMFE_SPEED_CHANGED);
	}

	atomic_dec_uint(&cpufreq_busy);

	return error;
}

/*
 * RK3188 / RK3188+
 */
struct rk3188_apll_rate {
	u_int rate;
	u_int nr, nf, no;
	u_int core_div, core_periph_div, core_axi_div;
	u_int aclk_div, hclk_div, pclk_div, ahb2apb_div;
	u_int voltage;
};

#define RK3188_RATE(_r, _nf, _no, _p, _a, _aclk, _hclk, _pclk, _ahb2apb, _v) \
	{ .rate = (_r) * 1000000, .nr = 1, .nf = (_nf), .no = (_no),     \
	  .core_div = 1, .core_periph_div = (_p), .core_axi_div = (_a),  \
	  .aclk_div = (_aclk), .hclk_div = (_hclk), .pclk_div = (_pclk), \
	  .ahb2apb_div = (_ahb2apb), .voltage = (_v) }

static const struct rk3188_apll_rate rk3188_apll_rates[] = {
	RK3188_RATE(1608, 67, 1, 8, 4, 4, 2, 4, 2, 1350),
	RK3188_RATE(1416, 59, 1, 8, 4, 4, 2, 4, 2, 1250),
	RK3188_RATE(1200, 50, 1, 8, 4, 4, 2, 4, 2, 1200),
	RK3188_RATE(1008, 42, 1, 8, 4, 3, 2, 4, 2, 1075),
	RK3188_RATE( 816, 68, 2, 8, 4, 3, 2, 4, 2, 1000),
	RK3188_RATE( 600, 50, 2, 4, 4, 3, 2, 4, 2, 1000),
};

#define GRF_STATUS0_REG			0x015c
#define GRF_STATUS0_APLL_LOCK		__BIT(5)
#define RK3188_GRF_STATUS0_REG		0x00ac
#define RK3188_GRF_STATUS0_APLL_LOCK	__BIT(6)

static size_t
rk3188_cpu_get_available(u_int *pavail, size_t maxavail)
{
	u_int n;

	KASSERT(__arraycount(rk3188_apll_rates) <= maxavail);

	for (n = 0; n < __arraycount(rk3188_apll_rates); n++) {
		pavail[n] = rk3188_apll_rates[n].rate / 1000000;
	}

	return __arraycount(rk3188_apll_rates);
}

static u_int
rk3188_cpu_get_rate(void)
{
	return rockchip_cpu_get_rate() / 1000000;
}

static u_int
rk3188_cpu_set_rate(u_int rate)
{
	const struct rk3188_apll_rate *r = NULL;
	uint32_t apll_con0, apll_con1, apll_con2, clksel0_con, clksel1_con;
	uint32_t reset_mask, reset, status0_reg, status0_apll_lock;
	u_int cpu_aclk_div_con;
	u_int old_rate = rk3188_cpu_get_rate();
	u_int new_rate;
#if NACT8846PM > 0
	device_t pmic;
	struct act8846_ctrl *dcdc3;

	pmic = device_find_by_driver_unit("act8846pm", 0);
	if (pmic == NULL) {
		printf("%s: no PMIC driver found\n", __func__);
		return ENXIO;
	}
	dcdc3 = act8846_lookup(pmic, "DCDC3");
	KASSERT(dcdc3 != NULL);
#endif

#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("%s: rate=%u\n", __func__, rate);
#endif

	/* Pick the closest rate (nearest 100MHz increment) */
	for (int i = 0; i < __arraycount(rk3188_apll_rates); i++) {
		u_int arate = ((rk3188_apll_rates[i].rate / 1000000) + 50)
		    / 100 * 100;
		if (arate <= rate) {
			r = &rk3188_apll_rates[i];
			break;
		}
	}
	if (r == NULL) {
#ifdef ROCKCHIP_CLOCK_DEBUG
		printf("CPU: No matching rate found for %u MHz\n", rate);
#endif
		return EINVAL;
	}

	switch (rockchip_chip_id()) {
	case ROCKCHIP_CHIP_ID_RK3066:
	case ROCKCHIP_CHIP_ID_RK3188PLUS:
	    	reset_mask = CRU_PLL_CON3_RESET_MASK;
		reset = CRU_PLL_CON3_RESET;
		apll_con0 = CRU_PLL_CON0_CLKR_MASK | CRU_PLL_CON0_CLKOD_MASK;
		apll_con0 |= __SHIFTIN(r->nr - 1, CRU_PLL_CON0_CLKR);
		apll_con0 |= __SHIFTIN(r->no - 1, CRU_PLL_CON0_CLKOD);
		apll_con1 = CRU_PLL_CON1_CLKF_MASK;
		apll_con1 |= __SHIFTIN(r->nf - 1, CRU_PLL_CON1_CLKF);
		apll_con2 = CRU_PLL_CON2_BWADJ_MASK;
		apll_con2 |= __SHIFTIN(r->nf >> 1, CRU_PLL_CON2_BWADJ);
		break;
	case ROCKCHIP_CHIP_ID_RK3188:
		reset_mask = CRU_PLL_CON3_POWER_DOWN_MASK;
		reset = CRU_PLL_CON3_POWER_DOWN;
		apll_con0 =
		    CRU_PLL_CON0_CLKR_MASK | RK3188_CRU_PLL_CON0_CLKOD_MASK;
		apll_con0 |= __SHIFTIN(r->nr - 1, CRU_PLL_CON0_CLKR);
		apll_con0 |= __SHIFTIN(r->no - 1, RK3188_CRU_PLL_CON0_CLKOD);
		apll_con1 = RK3188_CRU_PLL_CON1_CLKF_MASK;
		apll_con1 |= __SHIFTIN(r->nf - 1, RK3188_CRU_PLL_CON1_CLKF);
		apll_con2 = 0;
		break;
	default:
		return EINVAL;
	}

	switch (r->core_axi_div) {
	case 1:	cpu_aclk_div_con = 0; break;
	case 2: cpu_aclk_div_con = 1; break;
	case 3: cpu_aclk_div_con = 2; break;
	case 4: cpu_aclk_div_con = 3; break;
	case 8: cpu_aclk_div_con = 4; break;
	default: panic("bad core_axi_div");
	}

	switch (rockchip_chip_id()) {
	case ROCKCHIP_CHIP_ID_RK3066:
		clksel0_con = CRU_CLKSEL_CON0_A9_CORE_DIV_CON_MASK |
			      CRU_CLKSEL_CON0_CORE_PERI_DIV_CON_MASK;
		clksel0_con |= __SHIFTIN(r->core_div - 1,
					 CRU_CLKSEL_CON0_A9_CORE_DIV_CON);
		clksel0_con |= __SHIFTIN(ffs(r->core_periph_div) - 2,
					 CRU_CLKSEL_CON0_CORE_PERI_DIV_CON);
		clksel1_con = CRU_CLKSEL_CON1_AHB2APB_PCLKEN_DIV_CON_MASK |
			      CRU_CLKSEL_CON1_CPU_PCLK_DIV_CON_MASK |
			      CRU_CLKSEL_CON1_CPU_HCLK_DIV_CON_MASK |
			      CRU_CLKSEL_CON1_CPU_ACLK_DIV_CON_MASK;
		clksel1_con |= __SHIFTIN(ffs(r->ahb2apb_div) - 1,
					 CRU_CLKSEL_CON1_AHB2APB_PCLKEN_DIV_CON);
		clksel1_con |= __SHIFTIN(ffs(r->hclk_div) - 1,
					 CRU_CLKSEL_CON1_CPU_HCLK_DIV_CON);
		clksel1_con |= __SHIFTIN(ffs(r->pclk_div) - 1,
					 CRU_CLKSEL_CON1_CPU_PCLK_DIV_CON);
		clksel1_con |= __SHIFTIN(cpu_aclk_div_con,
					 CRU_CLKSEL_CON1_CPU_ACLK_DIV_CON);
		status0_reg = GRF_STATUS0_REG;
		status0_apll_lock = GRF_STATUS0_APLL_LOCK;
		break;
	case ROCKCHIP_CHIP_ID_RK3188:
	case ROCKCHIP_CHIP_ID_RK3188PLUS:
		clksel0_con = RK3188_CRU_CLKSEL_CON0_A9_CORE_DIV_CON_MASK |
			      CRU_CLKSEL_CON0_CORE_PERI_DIV_CON_MASK;
		clksel0_con |= __SHIFTIN(r->core_div - 1,
					 RK3188_CRU_CLKSEL_CON0_A9_CORE_DIV_CON);
		clksel0_con |= __SHIFTIN(ffs(r->core_periph_div) - 2,
					 CRU_CLKSEL_CON0_CORE_PERI_DIV_CON);
		clksel1_con = CRU_CLKSEL_CON1_AHB2APB_PCLKEN_DIV_CON_MASK |
			      CRU_CLKSEL_CON1_CPU_PCLK_DIV_CON_MASK |
			      CRU_CLKSEL_CON1_CPU_HCLK_DIV_CON_MASK |
			      RK3188_CRU_CLKSEL_CON1_CPU_ACLK_DIV_CON_MASK;
		clksel1_con |= __SHIFTIN(ffs(r->ahb2apb_div) - 1,
					 CRU_CLKSEL_CON1_AHB2APB_PCLKEN_DIV_CON);
		clksel1_con |= __SHIFTIN(ffs(r->hclk_div) - 1,
					 CRU_CLKSEL_CON1_CPU_HCLK_DIV_CON);
		clksel1_con |= __SHIFTIN(ffs(r->pclk_div) - 1,
					 CRU_CLKSEL_CON1_CPU_PCLK_DIV_CON);
		clksel1_con |= __SHIFTIN(cpu_aclk_div_con,
					 RK3188_CRU_CLKSEL_CON1_CPU_ACLK_DIV_CON);
		status0_reg = RK3188_GRF_STATUS0_REG;
		status0_apll_lock = RK3188_GRF_STATUS0_APLL_LOCK;
		break;
	default:
		return EINVAL;
	}

#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("%s: Set frequency to %u MHz...\n", __func__, r->rate);
	printf("before: APLL_CON0: %#x\n",
	    bus_space_read_4(bst, cru_bsh, CRU_APLL_CON0_REG));
	printf("before: APLL_CON1: %#x\n",
	    bus_space_read_4(bst, cru_bsh, CRU_APLL_CON1_REG));
	printf("before: CLKSEL0_CON: %#x\n",
	    bus_space_read_4(bst, cru_bsh, CRU_CLKSEL_CON_REG(0)));
	printf("before: CLKSEL1_CON: %#x\n",
	    bus_space_read_4(bst, cru_bsh, CRU_CLKSEL_CON_REG(1)));
#endif

	new_rate = r->rate / 1000000;
	if (new_rate > old_rate) {
#if NACT8846PM > 0
		act8846_set_voltage(dcdc3, r->voltage, r->voltage);
#endif
	}

	bus_space_write_4(bst, cru_bsh, CRU_MODE_CON_REG,
	    CRU_MODE_CON_APLL_WORK_MODE_MASK |
	    __SHIFTIN(CRU_MODE_CON_APLL_WORK_MODE_SLOW,
		      CRU_MODE_CON_APLL_WORK_MODE));

	/* Power down */
	bus_space_write_4(bst, cru_bsh, CRU_APLL_CON3_REG, reset_mask | reset);

	/* Update APLL regs */
	bus_space_write_4(bst, cru_bsh, CRU_APLL_CON0_REG, apll_con0);
	bus_space_write_4(bst, cru_bsh, CRU_APLL_CON1_REG, apll_con1);
	if (apll_con2)
		bus_space_write_4(bst, cru_bsh, CRU_APLL_CON2_REG, apll_con2);

	for (volatile int i = 5000; i >= 0; i--)
		;

	/* Power up */
	bus_space_write_4(bst, cru_bsh, CRU_APLL_CON3_REG, reset_mask);

	/* Wait for PLL lock */
#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("%s: Waiting for PLL lock...\n", __func__);
#endif
	for (volatile int i = 50000; i >= 0; i--)
		;
	int retry = ROCKCHIP_REF_FREQ;
	while (--retry > 0) {
		uint32_t status = bus_space_read_4(bst, grf_bsh, status0_reg);
		if (status & status0_apll_lock)
			break;
	}
	if (retry == 0)
		printf("%s: PLL lock timeout\n", __func__);

	/* Update CLKSEL regs */
	bus_space_write_4(bst, cru_bsh, CRU_CLKSEL_CON_REG(0), clksel0_con);
	bus_space_write_4(bst, cru_bsh, CRU_CLKSEL_CON_REG(1), clksel1_con);

	/* Slow -> Normal mode */
	bus_space_write_4(bst, cru_bsh, CRU_MODE_CON_REG,
	    CRU_MODE_CON_APLL_WORK_MODE_MASK |
	    __SHIFTIN(CRU_MODE_CON_APLL_WORK_MODE_NORMAL,
		      CRU_MODE_CON_APLL_WORK_MODE));

#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("after: APLL_CON0: %#x\n",
	    bus_space_read_4(bst, cru_bsh, CRU_APLL_CON0_REG));
	printf("after: APLL_CON1: %#x\n",
	    bus_space_read_4(bst, cru_bsh, CRU_APLL_CON1_REG));
	printf("after: CLKSEL0_CON: %#x\n",
	    bus_space_read_4(bst, cru_bsh, CRU_CLKSEL_CON_REG(0)));
	printf("after: CLKSEL1_CON: %#x\n",
	    bus_space_read_4(bst, cru_bsh, CRU_CLKSEL_CON_REG(1)));
#endif

	a9tmr_update_freq(rockchip_a9periph_get_rate());

#if NACT8846PM > 0
	if (new_rate < old_rate) {
		act8846_set_voltage(dcdc3, r->voltage, r->voltage);
	}
#endif

	return 0;
}
