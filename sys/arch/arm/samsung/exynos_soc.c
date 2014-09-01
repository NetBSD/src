/*	$NetBSD: exynos_soc.c,v 1.18 2014/09/01 14:19:27 reinoud Exp $	*/
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

#define	_ARM32_BUS_DMA_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exynos_soc.c,v 1.18 2014/09/01 14:19:27 reinoud Exp $");

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
#include <evbarm/odroid/platform.h>

bus_space_handle_t exynos_core_bsh;
bus_space_handle_t exynos_audiocore_bsh;

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


#ifdef EXYNOS4
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
};
#endif


#ifdef EXYNOS5
const struct cpu_freq cpu_freq_settings_exynos5[] = {
	{ 200,  3, 100, 2},
	{ 333,  4, 222, 2},
	{ 400,  3, 100, 1},
	{ 533, 12, 533, 1},
	{ 600,  4, 200, 1},
	{ 667,  7, 389, 1},
	{ 800,  3, 100, 0},
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
#define NFRQS 15
static char sysctl_cpu_freqs_txt[NFRQS*5];

static int sysctl_cpufreq_target(SYSCTLFN_ARGS);
static int sysctl_cpufreq_current(SYSCTLFN_ARGS);


/*
 * the early serial console
 */
#ifdef EXYNOS_CONSOLE_EARLY

#include "opt_sscom.h"
#include <arm/samsung/sscom_reg.h>
#include <arm/samsung/sscom_var.h>
#include <dev/cons.h>

static volatile uint8_t *uart_base;

#define CON_REG(a) (*((volatile uint32_t *)(uart_base + (a))))

static int
exynos_cngetc(dev_t dv)
{
        if ((CON_REG(SSCOM_UTRSTAT) & UTRSTAT_RXREADY) == 0)
		return -1;

	return CON_REG(SSCOM_URXH);
}

static void
exynos_cnputc(dev_t dv, int c)
{
	int timo = 150000;

	while ((CON_REG(SSCOM_UFSTAT) & UFSTAT_TXFULL) && --timo > 0);

	CON_REG(SSCOM_UTXH) = c & 0xff;
}

static struct consdev exynos_earlycons = {
	.cn_putc = exynos_cnputc,
	.cn_getc = exynos_cngetc,
	.cn_pollc = nullcnpollc,
};
#endif /* EXYNOS_CONSOLE_EARLY */


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


/*
 * The latency values used below are `magic' and probably chosen empirically.
 * For the 4210 variant the data latency is lower, a 0x110. This is currently
 * not enforced.
 *
 * The prefetch values are also different for the revision 0 of the
 * Exynos4412, but why?
 */

int
exynos_l2cc_init(void)
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
	uint32_t reg = 0;
	uint32_t regval;
	uint32_t freq;

#ifdef EXYNOS4
	if (IS_EXYNOS4_P())
		reg = EXYNOS4_CMU_APLL + PLL_CON0_OFFSET;
#endif
#ifdef EXYNOS5
	if (IS_EXYNOS5_P()) 
		reg = EXYNOS5_CMU_APLL + PLL_CON0_OFFSET;
#endif
	KASSERT(reg);

	regval = bus_space_read_4(&exynos_bs_tag, exynos_core_bsh, reg);
	freq   = PLL_FREQ(EXYNOS_F_IN_FREQ, regval);

	return freq;
}


static void
exynos_set_cpufreq(const struct cpu_freq *freqreq)
{
	struct cpu_info *ci;
	uint32_t reg = 0;
	uint32_t regval;
	int M, P, S;
	int cii;

	M = freqreq->M;
	P = freqreq->P;
	S = freqreq->S;

	regval = __SHIFTIN(M, PLL_CON0_M) |
		 __SHIFTIN(P, PLL_CON0_P) |
		 __SHIFTIN(S, PLL_CON0_S);

#ifdef EXYNOS4
	if (IS_EXYNOS4_P())
		reg = EXYNOS4_CMU_APLL + PLL_CON0_OFFSET;
#endif
#ifdef EXYNOS5
	if (IS_EXYNOS5_P())
		reg = EXYNOS5_CMU_APLL + PLL_CON0_OFFSET;
#endif
	KASSERT(reg);

	/* enable PPL and write config */
	regval |= PLL_CON0_ENABLE;
	bus_space_write_4(&exynos_bs_tag, exynos_core_bsh, reg, regval);

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


void
exynos_clocks_bootstrap(void)
{
#ifdef EXYNOS4
	if (IS_EXYNOS4_P()) {
		cpu_freq_settings = cpu_freq_settings_exynos4;
		ncpu_freq_settings = __arraycount(cpu_freq_settings_exynos4);
	}
#endif
#ifdef EXYNOS5
	if (IS_EXYNOS5_P()) {
		cpu_freq_settings = cpu_freq_settings_exynos5;
		ncpu_freq_settings = __arraycount(cpu_freq_settings_exynos5);
	}
#endif
	KASSERT(ncpu_freq_settings != 0);
	KASSERT(ncpu_freq_settings < NFRQS);

	/* set max cpufreq */
	exynos_set_cpufreq(&cpu_freq_settings[ncpu_freq_settings-1]);
}


void
exynos_bootstrap(vaddr_t iobase, vaddr_t uartbase)
{
	int error;
	size_t core_size, audiocore_size;
	size_t audiocore_pbase, audiocore_vbase __diagused;

#ifdef EXYNOS4
	if (IS_EXYNOS4_P()) {
		core_size = EXYNOS4_CORE_SIZE;
		audiocore_size = EXYNOS4_AUDIOCORE_SIZE;
		audiocore_pbase = EXYNOS4_AUDIOCORE_PBASE;
		audiocore_vbase = EXYNOS4_AUDIOCORE_VBASE;
	}
#endif

#ifdef EXYNOS5
	if (IS_EXYNOS5_P()) {
		core_size = EXYNOS5_CORE_SIZE;
		audiocore_size = EXYNOS5_AUDIOCORE_SIZE;
		audiocore_pbase = EXYNOS5_AUDIOCORE_PBASE;
		audiocore_vbase = EXYNOS5_AUDIOCORE_VBASE;
	}
#endif

	/* set up early console so we can use printf() and friends */
#ifdef EXYNOS_CONSOLE_EARLY
	uart_base = (volatile uint8_t *) uartbase;
	cn_tab = &exynos_earlycons;
	printf("Exynos early console operational\n\n");
#endif
	/* map in the exynos io registers */
	error = bus_space_map(&exynos_bs_tag, EXYNOS_CORE_PBASE,
		core_size, 0, &exynos_core_bsh);
	if (error)
		panic("%s: failed to map in Exynos SFR registers: %d",
			__func__, error);
	KASSERT(exynos_core_bsh == iobase);

	error = bus_space_map(&exynos_bs_tag, audiocore_pbase,
		audiocore_size, 0, &exynos_audiocore_bsh);
	if (error)
		panic("%s: failed to map in Exynos audio SFR registers: %d",
			__func__, error);
	KASSERT(exynos_audiocore_bsh == audiocore_vbase);

	/* init bus dma tags */
	exynos_dma_bootstrap(physmem * PAGE_SIZE);

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
		 * bus space used for the armcore regisers (which armperiph uses).
		 */
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = &exynos_bs_tag;
		return;
	}
	if (device_is_a(self, "armgic")
	    && device_is_a(device_parent(self), "armperiph")) {
		/*
		 * The Exynos4420 armgic is located at a different location!
		 */

		extern uint32_t exynos_soc_id;

		switch (EXYNOS_PRODUCT_ID(exynos_soc_id)) {
#if defined(EXYNOS5)
		case 0xe5410:
			/* offsets not changed on matt's request */
#if 0
			mpcaa->mpcaa_memh = EXYNOS_CORE_VBASE;
			mpcaa->mpcaa_off1 = EXYNOS5_GIC_IOP_DISTRIBUTOR_OFFSET;
			mpcaa->mpcaa_off2 = EXYNOS5_GIC_IOP_CONTROLLER_OFFSET;
#endif
			break;
#endif
#if defined(EXYNOS4)
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
#ifdef EXYNOS5
		/*
		 * The global timer is dependent on the MCT running.
		 */
		bus_size_t o = EXYNOS5_MCT_OFFSET + MCT_G_TCON;
		uint32_t v = bus_space_read_4(&exynos_bs_tag, exynos_core_bsh,
		     o);
		v |= G_TCON_START;
		bus_space_write_4(&exynos_bs_tag, exynos_core_bsh, o, v);
#endif
		/*
		 * The frequencies of the timers are the reference
		 * frequency.
		 */
		prop_dictionary_set_uint32(device_properties(self),
		    "frequency", EXYNOS_F_IN_FREQ);
		return;
	}

	exyo_device_register(self, aux);
}


void
exynos_device_register_post_config(device_t self, void *aux)
{
	exyo_device_register_post_config(self, aux);
}

