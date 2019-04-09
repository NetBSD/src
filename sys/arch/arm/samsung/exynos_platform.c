/* $NetBSD: exynos_platform.c,v 1.26 2019/04/09 07:37:16 skrll Exp $ */

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_arm_debug.h"
#include "opt_console.h"
#include "opt_exynos.h"
#include "opt_multiprocessor.h"
#include "opt_console.h"

#include "ukbd.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exynos_platform.c,v 1.26 2019/04/09 07:37:16 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <arm/samsung/mct_var.h>
#include <arm/samsung/sscom_reg.h>

#include <evbarm/exynos/platform.h>
#include <evbarm/fdt/machdep.h>

#include <arm/fdt/arm_fdtvar.h>

#include <libfdt.h>

void exynos_platform_early_putchar(char);

#define	EXYNOS5800_PMU_BASE		0x10040000
#define	EXYNOS5800_PMU_SIZE		0x20000
#define	 EXYNOS5800_PMU_SWRESET			0x0400
#define	  EXYNOS5800_PMU_KFC_ETM_RESET(n)	__BIT(20 + (n))
#define	  EXYNOS5800_PMU_KFC_CORE_RESET(n)	__BIT(8 + (n))
#define	 EXYNOS5800_PMU_SPARE2			0x0908
#define	 EXYNOS5800_PMU_SPARE3			0x090c
#define	  EXYNOS5800_PMU_SWRESET_KFC_SEL	0x3
#define	 EXYNOS5800_PMU_CORE_CONFIG(n)		(0x2000 + 0x80 * (n))
#define	 EXYNOS5800_PMU_CORE_STATUS(n)		(0x2004 + 0x80 * (n))
#define	  EXYNOS5800_PMU_CORE_POWER_EN		0x3
#define	 EXYNOS5800_PMU_COMMON_CONFIG(n)	(0x2500 + 0x80 * (n))
#define	  EXYNOS5800_PMU_COMMON_POWER_EN	0x3
#define	 EXYNOS5800_PMU_COMMON_OPTION(n)	(0x2508 + 0x80 * (n))
#define	  EXYNOS5800_PMU_USE_L2_COMMON_UP_STATE		__BIT(30)
#define	  EXYNOS5800_PMU_USE_ARM_CORE_DOWN_STATE	__BIT(29)
#define	  EXYNOS5800_PMU_AUTO_CORE_DOWN			__BIT(9)

#define	EXYNOS5800_SYSRAM_BASE		0x02073000
#define	EXYNOS5800_SYSRAM_SIZE		0x1000
#define	 EXYNOS5800_SYSRAM_HOTPLUG		0x001c

static int
exynos5800_mpstart(void)
{
	int ret = 0;
#if defined(MULTIPROCESSOR)
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t pmu_bsh, sysram_bsh;
	uint64_t mpidr, bp_mpidr;
	uint32_t val, started = 0;
	u_int cpuindex, n;
	int child;

	bus_space_map(bst, EXYNOS5800_PMU_BASE, EXYNOS5800_PMU_SIZE, 0, &pmu_bsh);
	bus_space_map(bst, EXYNOS5800_SYSRAM_BASE, EXYNOS5800_SYSRAM_SIZE, 0, &sysram_bsh);

	const int cpus = OF_finddevice("/cpus");
	if (cpus == -1) {
		aprint_error("%s: no /cpus node found\n", __func__);
		return ret;
	}

	/* MPIDR affinity levels of boot processor. */
	bp_mpidr = cpu_mpidr_aff_read();

	/* Setup KFC reset */
	bus_space_write_4(bst, pmu_bsh, EXYNOS5800_PMU_SPARE3, EXYNOS5800_PMU_SWRESET_KFC_SEL);

	const uint32_t option = EXYNOS5800_PMU_USE_L2_COMMON_UP_STATE |
	    EXYNOS5800_PMU_USE_ARM_CORE_DOWN_STATE |
	    EXYNOS5800_PMU_AUTO_CORE_DOWN;
	val = bus_space_read_4(bst, pmu_bsh, EXYNOS5800_PMU_COMMON_OPTION(0));
	bus_space_write_4(bst, pmu_bsh, EXYNOS5800_PMU_COMMON_OPTION(0), val | option);
	val = bus_space_read_4(bst, pmu_bsh, EXYNOS5800_PMU_COMMON_OPTION(1));
	bus_space_write_4(bst, pmu_bsh, EXYNOS5800_PMU_COMMON_OPTION(1), val | option);

	bus_space_write_4(bst, sysram_bsh, EXYNOS5800_SYSRAM_HOTPLUG, KERN_VTOPHYS((vaddr_t)cpu_mpstart));
	arm_dsb();

	/* Power on clusters */
	bus_space_write_4(bst, pmu_bsh, EXYNOS5800_PMU_COMMON_CONFIG(0),
	    EXYNOS5800_PMU_COMMON_POWER_EN);
	bus_space_write_4(bst, pmu_bsh, EXYNOS5800_PMU_COMMON_CONFIG(1),
	    EXYNOS5800_PMU_COMMON_POWER_EN);

	/* Boot APs */
	cpuindex = 1;
	for (child = OF_child(cpus); child; child = OF_peer(child)) {
		if (fdtbus_get_reg64(child, 0, &mpidr, NULL) != 0)
			continue;

		if (mpidr == bp_mpidr)
			continue;	/* BP already started */

		const u_int cluster = __SHIFTOUT(mpidr, MPIDR_AFF1);
		const u_int aff0 = __SHIFTOUT(mpidr, MPIDR_AFF0);
		const u_int cpu = cluster * 4 + aff0;

		val = bus_space_read_4(bst, pmu_bsh, EXYNOS5800_PMU_CORE_STATUS(cpu));
		bus_space_write_4(bst, pmu_bsh, EXYNOS5800_PMU_CORE_CONFIG(cpu),
		    EXYNOS5800_PMU_CORE_POWER_EN);

		for (n = 0x100000; n > 0; n--) {
			val = bus_space_read_4(bst, pmu_bsh, EXYNOS5800_PMU_CORE_STATUS(cpu));
			if ((val & EXYNOS5800_PMU_CORE_POWER_EN) == EXYNOS5800_PMU_CORE_POWER_EN) {
				started |= __BIT(cpuindex);
				break;
			}
		}
		if (n == 0)
			aprint_error("cpu%d: WARNING: AP failed to power on\n", cpuindex);

		if (cluster == 1 && __SHIFTOUT(bp_mpidr, MPIDR_AFF1) == 1) {
			while (bus_space_read_4(bst, pmu_bsh, EXYNOS5800_PMU_SPARE2) == 0)
				;
			bus_space_write_4(bst, pmu_bsh, EXYNOS5800_PMU_SWRESET,
			    EXYNOS5800_PMU_KFC_CORE_RESET(aff0) |
			    EXYNOS5800_PMU_KFC_ETM_RESET(aff0));
		}

		/* Wait for AP to start */
		for (n = 0x100000; n > 0; n--) {
			membar_consumer();
			if (arm_cpu_hatched & __BIT(cpuindex))
				break;
		}
		if (n == 0) {
			ret++;
			aprint_error("cpu%d: WARNING: AP failed to start\n", cpuindex);
		}

		cpuindex++;
	}

	bus_space_unmap(bst, sysram_bsh, EXYNOS5800_SYSRAM_SIZE);
	bus_space_unmap(bst, pmu_bsh, EXYNOS5800_PMU_SIZE);
#endif
	return ret;
}

static struct of_compat_data mp_compat_data[] = {
	{ "samsung,exynos5800",		(uintptr_t)exynos5800_mpstart },
	{ NULL }
};

static int
exynos_platform_mpstart(void)
{

	int (*mp_start)(void) = NULL;

	const struct of_compat_data *cd = of_search_compatible(OF_finddevice("/"), mp_compat_data);
	if (cd)
		mp_start = (int (*)(void))cd->data;

	if (mp_start)
		return mp_start();

	return 0;
}

static void
exynos_platform_init_attach_args(struct fdt_attach_args *faa)
{
	extern struct bus_space armv7_generic_bs_tag;
	extern struct bus_space armv7_generic_a4x_bs_tag;
	extern struct arm32_bus_dma_tag arm_generic_dma_tag;

	faa->faa_bst = &armv7_generic_bs_tag;
	faa->faa_a4x_bst = &armv7_generic_a4x_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

void
exynos_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	(CONSADDR - EXYNOS_CORE_PBASE + EXYNOS_CORE_VBASE)

	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

	while ((uartaddr[SSCOM_UFSTAT / 4] & UFSTAT_TXFULL) != 0)
		;

	uartaddr[SSCOM_UTXH / 4] = c;
#endif
}

static void
exynos_platform_device_register(device_t self, void *aux)
{
	exynos_device_register(self, aux);
}

static void
exynos5_platform_reset(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	bus_space_map(bst, EXYNOS5800_PMU_BASE + EXYNOS5800_PMU_SWRESET, 4, 0, &bsh);
	bus_space_write_4(bst, bsh, 0, 1);
}

static u_int
exynos_platform_uart_freq(void)
{
	return EXYNOS_UART_FREQ;
}


#if defined(SOC_EXYNOS4)
static const struct pmap_devmap *
exynos4_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(EXYNOS_CORE_VBASE,
			     EXYNOS_CORE_PBASE,
			     EXYNOS4_CORE_SIZE),
		DEVMAP_ENTRY(EXYNOS4_AUDIOCORE_VBASE,
			     EXYNOS4_AUDIOCORE_PBASE,
			     EXYNOS4_AUDIOCORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
exynos4_platform_bootstrap(void)
{

	exynos_bootstrap(4);

#if defined(MULTIPROCESSOR)
	arm_cpu_max = 1 + __SHIFTOUT(armreg_l2ctrl_read(), L2CTRL_NUMCPU);
#endif
}

static const struct arm_platform exynos4_platform = {
	.ap_devmap = exynos4_platform_devmap,
//	.ap_mpstart = exynos4_mpstart,
	.ap_bootstrap = exynos4_platform_bootstrap,
	.ap_init_attach_args = exynos_platform_init_attach_args,
	.ap_device_register = exynos_platform_device_register,
	.ap_reset = exynos5_platform_reset,
	.ap_delay = mct_delay,
	.ap_uart_freq = exynos_platform_uart_freq,
};

ARM_PLATFORM(exynos4, "samsung,exynos4", &exynos4_platform);
#endif


#if defined(SOC_EXYNOS5)
static const struct pmap_devmap *
exynos5_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(EXYNOS_CORE_VBASE,
			     EXYNOS_CORE_PBASE,
			     EXYNOS5_CORE_SIZE),
		DEVMAP_ENTRY(EXYNOS5_AUDIOCORE_VBASE,
			     EXYNOS5_AUDIOCORE_PBASE,
			     EXYNOS5_AUDIOCORE_SIZE),
		DEVMAP_ENTRY(EXYNOS5_SYSRAM_VBASE,
			     EXYNOS5_SYSRAM_PBASE,
			     EXYNOS5_SYSRAM_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
exynos5_platform_bootstrap(void)
{

	exynos_bootstrap(5);

	arm_fdt_cpu_bootstrap();
}

static const struct arm_platform exynos5_platform = {
	.ap_devmap = exynos5_platform_devmap,
	.ap_bootstrap = exynos5_platform_bootstrap,
	.ap_mpstart = exynos_platform_mpstart,
	.ap_init_attach_args = exynos_platform_init_attach_args,
	.ap_device_register = exynos_platform_device_register,
	.ap_reset = exynos5_platform_reset,
	.ap_delay = mct_delay,
	.ap_uart_freq = exynos_platform_uart_freq,
};

ARM_PLATFORM(exynos5, "samsung,exynos5", &exynos5_platform);
#endif
