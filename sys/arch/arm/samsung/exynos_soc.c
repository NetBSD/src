/*	$NetBSD: exynos_soc.c,v 1.32.4.1 2018/07/28 04:37:29 pgoyette Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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

#include "opt_exynos.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exynos_soc.c,v 1.32.4.1 2018/07/28 04:37:29 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <prop/proplib.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <arm/locore.h>

#include <arm/mainbus/mainbus.h>
#include <arm/cortex/mpcore_var.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <arm/samsung/mct_reg.h>
#include <arm/samsung/smc.h>

#include <arm/cortex/pl310_var.h>
#include <arm/cortex/pl310_reg.h>

/* XXXNH */
#include <evbarm/exynos/platform.h>


/* these variables are retrieved in start.S and stored in .data */
uint32_t  exynos_soc_id = 0;
uint32_t  exynos_pop_id = 0;

/* cpu frequencies */
struct cpu_freq {
	uint64_t freq;
	int	 P;
	int	 M;
	int	 S;
};


#ifdef SOC_EXYNOS4
const struct cpu_freq cpu_freq_settings_exynos4[] = {
	{ 200, 3, 100, 2},
	{ 300, 4, 200, 2},
	{ 400, 3, 100, 1},
	{ 500, 3, 125, 1},
	{ 600, 4, 200, 1},
	{ 700, 3, 175, 1},
	{ 800, 3, 100, 0},
	{ 900, 4, 150, 0},
	{1000, 3, 125, 0},
	{1100, 6, 275, 0},
	{1200, 4, 200, 0},
	{1300, 6, 325, 0},
	{1400, 3, 175, 0},
	{1600, 3, 200, 0},
//	{1704, 3, 213, 0},
//	{1800, 4, 300, 0},
//	{1920, 3, 240, 0},
//	{2000, 3, 250, 0},
};
#endif


#ifdef SOC_EXYNOS5
#define EXYNOS5_DEFAULT_ENTRY 7
const struct cpu_freq cpu_freq_settings_exynos5[] = {
	{ 200,  3, 100, 2},
	{ 333,  4, 222, 2},
	{ 400,  3, 100, 1},
	{ 533, 12, 533, 1},
	{ 600,  4, 200, 1},
	{ 667,  7, 389, 1},
	{ 800,  3, 100, 0},
	{ 900,  4, 150, 0},
	{1000,  3, 125, 0},
	{1066, 12, 533, 0},
	{1200,  3, 150, 0},
	{1400,  3, 175, 0},
	{1600,  3, 200, 0},
};
#endif

static struct cpu_freq const *cpu_freq_settings = NULL;
static int ncpu_freq_settings = 0;

static int cpu_freq_target = 0;
#define NFRQS 18
static char sysctl_cpu_freqs_txt[NFRQS*5];

bus_space_handle_t exynos_core_bsh;
bus_space_handle_t exynos_audiocore_bsh;

bus_space_handle_t exynos_wdt_bsh;
bus_space_handle_t exynos_pmu_bsh;
bus_space_handle_t exynos_cmu_bsh;
bus_space_handle_t exynos_cmu_apll_bsh;
bus_space_handle_t exynos_sysreg_bsh;


static int sysctl_cpufreq_target(SYSCTLFN_ARGS);
static int sysctl_cpufreq_current(SYSCTLFN_ARGS);

#ifdef ARM_TRUSTZONE_FIRMWARE
int
exynos_do_idle(void)
{
        exynos_smc(SMC_CMD_SLEEP, 0, 0, 0);

	return 0;
}


int
exynos_set_cpu_boot_addr(int cpu, vaddr_t boot_addr)
{
	/* XXX we need to map in iRAM space for this XXX */
	return 0;
}


int
exynos_cpu_boot(int cpu)
{
	exynos_smc(SMC_CMD_CPU1BOOT, cpu, 0, 0);

	return 0;
}


#ifdef SOC_EXYNOS4
/*
 * The latency values used below are `magic' and probably chosen empirically.
 * For the 4210 variant the data latency is lower, a 0x110. This is currently
 * not enforced.
 *
 * The prefetch values are also different for the revision 0 of the
 * Exynos4412, but why?
 */

int
exynos4_l2cc_init(void)
{
	const uint32_t tag_latency  = 0x110;
	const uint32_t data_latency = IS_EXYNOS4410_P() ? 0x110 : 0x120;
	const uint32_t prefetch4412   = /* 0111 0001 0000 0000 0000 0000 0000 0111 */
				PREFETCHCTL_DBLLINEF_EN  |
				PREFETCHCTL_INSTRPREF_EN |
				PREFETCHCTL_DATAPREF_EN  |
				PREFETCHCTL_PREF_DROP_EN |
				PREFETCHCTL_PREFETCH_OFFSET_7;
	const uint32_t prefetch4412_r0 = /* 0011 0000 0000 0000 0000 0000 0000 0111 */
				PREFETCHCTL_INSTRPREF_EN |
				PREFETCHCTL_DATAPREF_EN  |
				PREFETCHCTL_PREFETCH_OFFSET_7;
	const uint32_t aux_val      =    /* 0111 1100 0100 0111 0000 0000 0000 0001 */
				AUXCTL_EARLY_BRESP_EN |
				AUXCTL_I_PREFETCH     |
				AUXCTL_D_PREFETCH     |
				AUXCTL_NS_INT_ACC_CTL |
				AUXCTL_NS_INT_LOCK_EN |
				AUXCTL_SHARED_ATT_OVR |
				AUXCTL_WAY_SIZE_RSVD7 << 16 | /* why rsvd7 ??? */
				AUXCTL_FULL_LINE_WR0;
	const uint32_t aux_keepmask =    /* 1100 0010 0000 0000 1111 1111 1111 1111  */
				AUXCTL_RSVD31         |
				AUXCTL_EARLY_BRESP_EN |
				AUXCTL_CACHE_REPL_RR  |

				AUXCTL_SH_ATTR_INV_ENA|
				AUXCTL_EXCL_CACHE_CFG |
				AUXCTL_ST_BUF_DEV_LIM_EN |
				AUXCTL_HIPRO_SO_DEV_EN |
				AUXCTL_FULL_LINE_WR0  |
				0xffff;
	uint32_t prefetch;

	/* check the bitmaps are the same as the linux implementation uses */
	KASSERT(prefetch4412    == 0x71000007);
	KASSERT(prefetch4412_r0 == 0x30000007);
	KASSERT(aux_val         == 0x7C470001);
	KASSERT(aux_keepmask    == 0xC200FFFF);

	if (IS_EXYNOS4412_R0_P())
		prefetch = prefetch4412_r0;
	else
		prefetch = prefetch4412;	/* newer than >= r1_0 */
	;

	exynos_smc(SMC_CMD_L2X0SETUP1, tag_latency, data_latency, prefetch);
	exynos_smc(SMC_CMD_L2X0SETUP2,
		POWERCTL_DYNCLKGATE | POWERCTL_STANDBY,
		aux_val, aux_keepmask);
	exynos_smc(SMC_CMD_L2X0INVALL, 0, 0, 0);
	exynos_smc(SMC_CMD_L2X0CTRL, 1, 0, 0);

	return 0;
}
#endif
#endif /* ARM_TRUSTZONE_FIRMWARE */


void
exynos_sysctl_cpufreq_init(void)
{
	const struct sysctlnode *node, *cpunode, *freqnode;
	char *cpos;
	int i, val;
	int error;

	memset(sysctl_cpu_freqs_txt, (int) ' ', sizeof(sysctl_cpu_freqs_txt));
	cpos = sysctl_cpu_freqs_txt;
	for (i = 0; i < ncpu_freq_settings; i++) {
		val = cpu_freq_settings[i].freq;
		snprintf(cpos, 6, "%d ", val);
		cpos += (val < 1000) ? 4 : 5;
	}
	*cpos = 0;

	error = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
	if (error)
		printf("couldn't create `machdep' node\n");

	error = sysctl_createv(NULL, 0, &node, &cpunode,
	    0, CTLTYPE_NODE, "cpu", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		printf("couldn't create `cpu' node\n");

	error = sysctl_createv(NULL, 0, &cpunode, &freqnode,
	    0, CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		printf("couldn't create `frequency' node\n");

	error = sysctl_createv(NULL, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "target", NULL,
	    sysctl_cpufreq_target, 0, &cpu_freq_target, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		printf("couldn't create `target' node\n");

	error = sysctl_createv(NULL, 0, &freqnode, &node,
	    0, CTLTYPE_INT, "current", NULL,
	    sysctl_cpufreq_current, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		printf("couldn't create `current' node\n");

	error = sysctl_createv(NULL, 0, &freqnode, &node,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, sysctl_cpu_freqs_txt, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		printf("couldn't create `available' node\b");
}


uint64_t
exynos_get_cpufreq(void)
{
	uint32_t regval;
	uint32_t freq;

	regval = bus_space_read_4(&armv7_generic_bs_tag, exynos_cmu_apll_bsh,
			PLL_CON0_OFFSET);
	freq   = PLL_FREQ(EXYNOS_F_IN_FREQ, regval);

	return freq;
}


static void
exynos_set_cpufreq(const struct cpu_freq *freqreq)
{
	struct cpu_info *ci;
	uint32_t regval;
	int M, P, S;
	int cii;

	M = freqreq->M;
	P = freqreq->P;
	S = freqreq->S;

	regval = __SHIFTIN(M, PLL_CON0_M) |
		 __SHIFTIN(P, PLL_CON0_P) |
		 __SHIFTIN(S, PLL_CON0_S);

	/* enable PPL and write config */
	regval |= PLL_CON0_ENABLE;
	bus_space_write_4(&armv7_generic_bs_tag, exynos_cmu_apll_bsh, PLL_CON0_OFFSET,
		regval);

	/* update our cycle counter i.e. our CPU frequency for all CPUs */
	for (CPU_INFO_FOREACH(cii, ci)) {
		ci->ci_data.cpu_cc_freq = exynos_get_cpufreq();
	}
}


static int
sysctl_cpufreq_target(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint32_t t, curfreq, minfreq, maxfreq;
	int i, best_i, diff;
	int error;

	curfreq = exynos_get_cpufreq() / (1000*1000);
	t = *(int *)rnode->sysctl_data;
	if (t == 0)
		t = curfreq;

	node = *rnode;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	minfreq = cpu_freq_settings[0].freq;
	maxfreq = cpu_freq_settings[ncpu_freq_settings-1].freq;

	if ((t < minfreq) || (t > maxfreq))
		return EINVAL;

	if (t == curfreq) {
		*(int *)rnode->sysctl_data = t;
		return 0;
	}

	diff = maxfreq;
	best_i = -1;
	for (i = 0; i < ncpu_freq_settings; i++) {
		if (abs(t - cpu_freq_settings[i].freq) <= diff) {
			diff = labs(t - cpu_freq_settings[i].freq);
			best_i = i;
		}
	}
	if (best_i < 0)
		return EINVAL;

	exynos_set_cpufreq(&cpu_freq_settings[best_i]);

	*(int *)rnode->sysctl_data = t;
	return 0;
}


static int
sysctl_cpufreq_current(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	uint32_t freq;

	freq = exynos_get_cpufreq() / (1000*1000);
	node.sysctl_data = &freq;

	return sysctl_lookup(SYSCTLFN_CALL(&node));
}


#ifdef VERBOSE_INIT_ARM
#define DUMP_PLL(v, var) \
	reg = EXYNOS##v##_CMU_##var + PLL_CON0_OFFSET;\
	regval = bus_space_read_4(&armv7_generic_bs_tag, exynos_cmu_bsh, reg); \
	freq   = PLL_FREQ(EXYNOS_F_IN_FREQ, regval); \
	printf("%8s at %d Mhz\n", #var, freq/(1000*1000));


static void
exynos_dump_clocks(void)
{
	uint32_t reg = 0;
	uint32_t regval;
	uint32_t freq;

	printf("Initial PLL settings\n");
#ifdef SOC_EXYNOS4
	DUMP_PLL(4, APLL);
	DUMP_PLL(4, MPLL);
	DUMP_PLL(4, EPLL);
	DUMP_PLL(4, VPLL);
#endif
#ifdef SOC_EXYNOS5
	DUMP_PLL(5, APLL);
	DUMP_PLL(5, MPLL);
	DUMP_PLL(5, KPLL);
	DUMP_PLL(5, DPLL);
	DUMP_PLL(5, VPLL);
	DUMP_PLL(5, CPLL);
	DUMP_PLL(5, GPLL);
	DUMP_PLL(5, BPLL);
#endif
}
#undef DUMP_PLL
#endif


/* XXX clock stuff needs major work XXX */

void
exynos_init_clkout_for_usb(void)
{
	/* Select XUSBXTI as source for CLKOUT */
	bus_space_write_4(&armv7_generic_bs_tag, exynos_pmu_bsh,
	    EXYNOS_PMU_DEBUG_CLKOUT, 0x1000);
}


void
exynos_clocks_bootstrap(void)
{
	KASSERT(ncpu_freq_settings != 0);
	KASSERT(ncpu_freq_settings < NFRQS);
	int fsel;

#ifdef VERBOSE_INIT_ARM
	exynos_dump_clocks();
#endif

	/* set (max) cpufreq */
	fsel = ncpu_freq_settings-1;

#ifdef SOC_EXYNOS5
	/* XXX BUGFIX selecting freq on E5 goes wrong for now XXX */
	fsel = EXYNOS5_DEFAULT_ENTRY;
#endif

	exynos_set_cpufreq(&cpu_freq_settings[fsel]);

	/* set external USB frequency to XCLKOUT */
	exynos_init_clkout_for_usb();
}


void
exynos_bootstrap(vaddr_t iobase, vaddr_t uartbase)
{
	int error;
	size_t core_size, audiocore_size;
	bus_addr_t audiocore_pbase;
	bus_addr_t audiocore_vbase __diagused;
	bus_addr_t exynos_wdt_offset;
	bus_addr_t exynos_pmu_offset;
	bus_addr_t exynos_sysreg_offset;
	bus_addr_t exynos_cmu_apll_offset;

	/* set up early console so we can use printf() and friends */
#ifdef EXYNOS_CONSOLE_EARLY
	uart_base = (volatile uint8_t *) uartbase;
	cn_tab = &exynos_earlycons;
	printf("Exynos early console operational\n\n");
#endif

#ifdef SOC_EXYNOS4
	core_size = EXYNOS4_CORE_SIZE;
	audiocore_size = EXYNOS4_AUDIOCORE_SIZE;
	audiocore_pbase = EXYNOS4_AUDIOCORE_PBASE;
	audiocore_vbase = EXYNOS4_AUDIOCORE_VBASE;
	exynos_wdt_offset = EXYNOS4_WDT_OFFSET;
	exynos_pmu_offset = EXYNOS4_PMU_OFFSET;
	exynos_sysreg_offset = EXYNOS4_SYSREG_OFFSET;
	exynos_cmu_apll_offset = EXYNOS4_CMU_APLL;

	cpu_freq_settings = cpu_freq_settings_exynos4;
	ncpu_freq_settings = __arraycount(cpu_freq_settings_exynos4);
#endif

#ifdef SOC_EXYNOS5
	core_size = EXYNOS5_CORE_SIZE;
	audiocore_size = EXYNOS5_AUDIOCORE_SIZE;
	audiocore_pbase = EXYNOS5_AUDIOCORE_PBASE;
	audiocore_vbase = EXYNOS5_AUDIOCORE_VBASE;
	exynos_wdt_offset = EXYNOS5_WDT_OFFSET;
	exynos_pmu_offset = EXYNOS5_PMU_OFFSET;
	exynos_sysreg_offset = EXYNOS5_SYSREG_OFFSET;
	exynos_cmu_apll_offset = EXYNOS5_CMU_APLL;

	cpu_freq_settings = cpu_freq_settings_exynos5;
	ncpu_freq_settings = __arraycount(cpu_freq_settings_exynos5);
#endif

	/* map in the exynos io registers */
	error = bus_space_map(&armv7_generic_bs_tag, EXYNOS_CORE_PBASE,
		core_size, 0, &exynos_core_bsh);
	if (error)
		panic("%s: failed to map in Exynos SFR registers: %d",
			__func__, error);
	KASSERT(exynos_core_bsh == iobase);

	error = bus_space_map(&armv7_generic_bs_tag, audiocore_pbase,
		audiocore_size, 0, &exynos_audiocore_bsh);
	if (error)
		panic("%s: failed to map in Exynos audio SFR registers: %d",
			__func__, error);
	KASSERT(exynos_audiocore_bsh == audiocore_vbase);

	/* map in commonly used subregions and common used register banks */
	error = bus_space_subregion(&armv7_generic_bs_tag, exynos_core_bsh,
		exynos_wdt_offset, EXYNOS_BLOCK_SIZE, &exynos_wdt_bsh);
	if (error)
		panic("%s: failed to subregion wdt registers: %d",
			__func__, error);

	error = bus_space_subregion(&armv7_generic_bs_tag, exynos_core_bsh,
		exynos_pmu_offset, EXYNOS_BLOCK_SIZE, &exynos_pmu_bsh);
	if (error)
		panic("%s: failed to subregion pmu registers: %d",
			__func__, error);

	exynos_cmu_bsh = exynos_core_bsh;
	bus_space_subregion(&armv7_generic_bs_tag, exynos_core_bsh,
		exynos_sysreg_offset, EXYNOS_BLOCK_SIZE,
		&exynos_sysreg_bsh);
	if (error)
		panic("%s: failed to subregion sysreg registers: %d",
			__func__, error);

	error = bus_space_subregion(&armv7_generic_bs_tag, exynos_cmu_bsh,
		exynos_cmu_apll_offset, 0xfff, &exynos_cmu_apll_bsh);
	if (error)
		panic("%s: failed to subregion cmu apll registers: %d",
			__func__, error);

	/* gpio bootstrapping delayed */
}


void
exynos_device_register(device_t self, void *aux)
{
	if (device_is_a(self, "armperiph")
	    && device_is_a(device_parent(self), "mainbus")) {
		/*
		 * XXX KLUDGE ALERT XXX
		 * The iot mainbus supplies is completely wrong since it scales
		 * addresses by 2.  The simplest remedy is to replace with our
		 * bus space used for the armcore registers (which armperiph uses).
		 */
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = &armv7_generic_bs_tag;
		return;
	}
	if (device_is_a(self, "armgic")
	    && device_is_a(device_parent(self), "armperiph")) {
		/*
		 * The Exynos4420 armgic is located at a different location!
		 */

		extern uint32_t exynos_soc_id;

		switch (EXYNOS_PRODUCT_ID(exynos_soc_id)) {
#ifdef SOC_EXYNOS5
		case 0xe5410:
			/* offsets not changed on matt's request */
#if 0
			mpcaa->mpcaa_memh = EXYNOS_CORE_VBASE;
			mpcaa->mpcaa_off1 = EXYNOS5_GIC_IOP_DISTRIBUTOR_OFFSET;
			mpcaa->mpcaa_off2 = EXYNOS5_GIC_IOP_CONTROLLER_OFFSET;
#endif
			break;
		case 0xe5422: {
			struct mpcore_attach_args * const mpcaa = aux;

			mpcaa->mpcaa_memh = EXYNOS_CORE_VBASE;
			mpcaa->mpcaa_off1 = EXYNOS5_GIC_IOP_DISTRIBUTOR_OFFSET;
			mpcaa->mpcaa_off2 = EXYNOS5_GIC_IOP_CONTROLLER_OFFSET;
			break;
		}
#endif
#ifdef SOC_EXYNOS4
		case 0xe4410:
		case 0xe4412: {
			struct mpcore_attach_args * const mpcaa = aux;

			mpcaa->mpcaa_memh = EXYNOS_CORE_VBASE;
			mpcaa->mpcaa_off1 = EXYNOS4_GIC_DISTRIBUTOR_OFFSET;
			mpcaa->mpcaa_off2 = EXYNOS4_GIC_CNTR_OFFSET;
			break;
		      }
#endif
		default:
			panic("%s: unknown SoC product id %#x", __func__,
			    (u_int)EXYNOS_PRODUCT_ID(exynos_soc_id));
		}
		return;
	}
	if (device_is_a(self, "armgtmr") || device_is_a(self, "mct")) {
#ifdef SOC_EXYNOS5
		/*
		 * The global timer is dependent on the MCT running.
		 */
		bus_size_t o = EXYNOS5_MCT_OFFSET + MCT_G_TCON;
		uint32_t v = bus_space_read_4(&armv7_generic_bs_tag, exynos_core_bsh,
		     o);
		v |= G_TCON_START;
		bus_space_write_4(&armv7_generic_bs_tag, exynos_core_bsh, o, v);
#endif
		/*
		 * The frequencies of the timers are the reference
		 * frequency.
		 */
		prop_dictionary_set_uint32(device_properties(self),
		    "frequency", EXYNOS_F_IN_FREQ);
		return;
	}
}


void
exynos_device_register_post_config(device_t self, void *aux)
{
}

void
exynos_usb_soc_powerup(void)
{
	/* XXX 5422 XXX */
}


/*
 * USB Phy SoC dependent handling
 */

/* XXX 5422 not handled since its unknown how it handles this XXX*/
static void
exynos_usb2_set_isolation(bool on)
{
	uint32_t en_mask, regval;
	bus_addr_t reg;

	/* enable PHY */
	reg = EXYNOS_PMU_USB_PHY_CTRL;

	if (IS_EXYNOS5_P() || IS_EXYNOS4410_P()) {
		/* set usbhost mode */
		regval = on ? 0 : USB20_PHY_HOST_LINK_EN;
		bus_space_write_4(&armv7_generic_bs_tag, exynos_sysreg_bsh,
			EXYNOS5_SYSREG_USB20_PHY_TYPE, regval);
		reg = EXYNOS_PMU_USBHOST_PHY_CTRL;
	}

	/* do enable PHY */
	en_mask = PMU_PHY_ENABLE;
	regval = bus_space_read_4(&armv7_generic_bs_tag, exynos_pmu_bsh, reg);
	regval = on ? regval & ~en_mask : regval | en_mask;

	bus_space_write_4(&armv7_generic_bs_tag, exynos_pmu_bsh,
		reg, regval);

	if (IS_EXYNOS4X12_P()) {
		bus_space_write_4(&armv7_generic_bs_tag, exynos_pmu_bsh,
			EXYNOS_PMU_USB_HSIC_1_PHY_CTRL, regval);
		bus_space_write_4(&armv7_generic_bs_tag, exynos_pmu_bsh,
			EXYNOS_PMU_USB_HSIC_2_PHY_CTRL, regval);
	}
}


#ifdef SOC_EXYNOS4
static void
exynos4_usb2phy_enable(bus_space_handle_t usb2phy_bsh)
{
	uint32_t phypwr, rstcon, clkreg;

	/* write clock value */
	clkreg = FSEL_CLKSEL_24M;
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
		USB_PHYCLK, clkreg);

	/* set device and host to normal */
	phypwr = bus_space_read_4(&armv7_generic_bs_tag, usb2phy_bsh,
		USB_PHYPWR);

	/* enable analog, enable otg, unsleep phy0 (host) */
	phypwr &= ~PHYPWR_NORMAL_MASK_PHY0;
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
		USB_PHYPWR, phypwr);

	if (IS_EXYNOS4X12_P()) {
		/* enable hsic0 (host), enable hsic1 and phy1 (otg) */
		phypwr = bus_space_read_4(&armv7_generic_bs_tag, usb2phy_bsh,
			USB_PHYPWR);
		phypwr &= ~(PHYPWR_NORMAL_MASK_HSIC0 |
			    PHYPWR_NORMAL_MASK_HSIC1 |
			    PHYPWR_NORMAL_MASK_PHY1);
		bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
			USB_PHYPWR, phypwr);
	}

	/* reset both phy and link of device */
	rstcon = bus_space_read_4(&armv7_generic_bs_tag, usb2phy_bsh,
		USB_RSTCON);
	rstcon |= RSTCON_DEVPHY_SWRST;
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
		USB_RSTCON, rstcon);
	DELAY(10000);
	rstcon &= ~RSTCON_DEVPHY_SWRST;
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
		USB_RSTCON, rstcon);

	if (IS_EXYNOS4X12_P()) {
		/* reset both phy and link of host */
		rstcon = bus_space_read_4(&armv7_generic_bs_tag, usb2phy_bsh,
			USB_RSTCON);
		rstcon |= RSTCON_HOSTPHY_SWRST | RSTCON_HOSTPHYLINK_SWRST;
		bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
			USB_RSTCON, rstcon);
		DELAY(10000);
		rstcon &= ~(RSTCON_HOSTPHY_SWRST | RSTCON_HOSTPHYLINK_SWRST);
		bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
			USB_RSTCON, rstcon);
	}

	/* wait for everything to be initialized */
	DELAY(80000);
}
#endif


#ifdef SOC_EXYNOS5
static void
exynos5410_usb2phy_enable(bus_space_handle_t usb2phy_bsh)
{
	uint32_t phyhost; //, phyotg;
	uint32_t phyhsic;
	uint32_t ehcictrl, ohcictrl;

	/* host configuration: */
	phyhost = bus_space_read_4(&armv7_generic_bs_tag, usb2phy_bsh,
	    USB_PHY_HOST_CTRL0);

	/* host phy reference clock; assumption its 24 MHz now */
	phyhost &= ~HOST_CTRL0_FSEL_MASK;
	phyhost |= __SHIFTIN(FSEL_CLKSEL_24M, HOST_CTRL0_FSEL_MASK);

	/* enable normal mode of operation */
	phyhost &= ~(HOST_CTRL0_FORCESUSPEND | HOST_CTRL0_FORCESLEEP);

	/* host phy reset */
	phyhost &= ~(HOST_CTRL0_PHY_SWRST | HOST_CTRL0_PHY_SWRST_ALL |
	    HOST_CTRL0_SIDDQ | HOST_CTRL0_FORCESUSPEND |
	    HOST_CTRL0_FORCESLEEP);

	/* host link reset */
	phyhost |= HOST_CTRL0_LINK_SWRST | HOST_CTRL0_UTMI_SWRST |
	    HOST_CTRL0_COMMONON_N;
	/* do the reset */
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh, USB_PHY_HOST_CTRL0,
	    phyhost);
	DELAY(10000);

	phyhost &= ~(HOST_CTRL0_LINK_SWRST | HOST_CTRL0_UTMI_SWRST);
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh, USB_PHY_HOST_CTRL0,
	   phyhost);

	/* HSIC control */
	phyhsic =
	    __SHIFTIN(HSIC_CTRL_REFCLKDIV_12, HSIC_CTRL_REFCLKDIV_MASK) |
	    __SHIFTIN(HSIC_CTRL_REFCLKSEL_DEFAULT, HSIC_CTRL_REFCLKSEL_MASK) |
	    HSIC_CTRL_PHY_SWRST;

	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh, USB_PHY_HSIC_CTRL1,
	   phyhsic);
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh, USB_PHY_HSIC_CTRL2,
	   phyhsic);
	DELAY(10);

	phyhsic &= ~HSIC_CTRL_PHY_SWRST;
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh, USB_PHY_HSIC_CTRL1,
	   phyhsic);
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh, USB_PHY_HSIC_CTRL2,
	   phyhsic);
	DELAY(80);

#if 0
	/* otg configuration: */
	phyotg = bus_space_read_4(&armv7_generic_bs_tag, usb2phy_bsh,
		USB_PHY_OTG_SYS);

	/* otg phy refrence clock: assumption its 24 Mhz now */
	phyotg &= ~OTG_SYS_FSEL_MASK;
	phyotg |= __SHIFTIN(OTG_SYS_FSEL_MASK, FSEL_CLKSEL_24M);

	/* enable normal mode of operation */
	phyotg &= ~(OTG_SYS_FORCESUSPEND | OTG_SYS_FORCESLEEP |
		OTG_SYS_SIDDQ_UOTG | OTG_SYS_REFCLKSEL_MASK |
		OTG_SYS_COMMON_ON);

	/* OTG phy and link reset */
	phyotg |= OTG_SYS_PHY0_SWRST | OTG_SYS_PHYLINK_SWRST |
		OTG_SYS_OTGDISABLE | OTG_SYS_REFCLKSEL_MASK;

	/* do the reset */
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
		USB_PHY_OTG_SYS, phyotg);
	DELAY(10000);
	phyotg &= ~(OTG_SYS_PHY0_SWRST | OTG_SYS_LINK_SWRST_UOTG |
		OTG_SYS_PHYLINK_SWRST);
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
		USB_PHY_OTG_SYS, phyotg);
#endif

	/* enable EHCI DMA burst: */
	ehcictrl = bus_space_read_4(&armv7_generic_bs_tag, usb2phy_bsh,
	    USB_PHY_HOST_EHCICTRL);
	ehcictrl |= HOST_EHCICTRL_ENA_INCRXALIGN |
	    HOST_EHCICTRL_ENA_INCR4 | HOST_EHCICTRL_ENA_INCR8 |
	    HOST_EHCICTRL_ENA_INCR16;
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
	    USB_PHY_HOST_EHCICTRL, ehcictrl);

	/* Set OHCI suspend */
	ohcictrl = bus_space_read_4(&armv7_generic_bs_tag, usb2phy_bsh,
	    USB_PHY_HOST_OHCICTRL);
	ohcictrl |= HOST_OHCICTRL_SUSPLGCY;
	bus_space_write_4(&armv7_generic_bs_tag, usb2phy_bsh,
	    USB_PHY_HOST_OHCICTRL, ohcictrl);
}


static void
exynos5422_usb2phy_enable(bus_space_handle_t usb2phy_bsh)
{
	aprint_error("%s not implemented\n", __func__);
}
#endif


void
exynos_usb_phy_init(bus_space_handle_t usb2phy_bsh)
{
	/* disable phy isolation */
	exynos_usb2_set_isolation(false);

#ifdef SOC_EXYNOS4
	exynos4_usb2phy_enable(usb2phy_bsh);
#endif
#ifdef SOC_EXYNOS5
	if (IS_EXYNOS5410_P()) {
		exynos5410_usb2phy_enable(usb2phy_bsh);
		/* TBD: USB3 phy init */
	} else if (IS_EXYNOS5422_P()) {
		exynos5422_usb2phy_enable(usb2phy_bsh);
		/* TBD: USB3 phy init */
	}
#endif
}


