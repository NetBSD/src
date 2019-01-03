/* $NetBSD: sunxi_mc_smp.c,v 1.1 2019/01/03 11:01:59 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: sunxi_mc_smp.c,v 1.1 2019/01/03 11:01:59 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/fdt/fdtvar.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>
#include <arm/locore.h>

#include <arm/sunxi/sunxi_mc_smp.h>

#define	A83T_SMP_ENABLE_METHOD	"allwinner,sun8i-a83t-smp"

#define	PRCM_BASE		0x01f01400
#define	PRCM_SIZE		0x800

#define	 PRCM_CL_RST_CTRL(cluster)	(0x4 + (cluster) * 0x4)
#define	 PRCM_CL_PWROFF(cluster)	(0x100 + (cluster) * 0x4)
#define	 PRCM_CL_PWR_CLAMP(cluster, cpu) (0x140 + (cluster) * 0x10 + (cpu) * 0x4)

#define	CPUCFG_BASE	0x01f01c00
#define	CPUCFG_SIZE	0x400

#define	 CPUCFG_CL_RST(cluster)		(0x30 + (cluster) * 0x4)
#define	 CPUCFG_P_REG0			0x1a4

#define	CPUXCFG_BASE	0x01700000
#define	CPUXCFG_SIZE	0x400

#define	 CPUXCFG_CL_RST(cluster)	(0x80 + (cluster) * 0x4)
#define	  CPUXCFG_CL_RST_SOC_DBG_RST	__BIT(24)
#define	  CPUXCFG_CL_RST_ETM_RST(cpu)	__BIT(20 + (cpu))
#define	  CPUXCFG_CL_RST_DBG_RST(cpu)	__BIT(16 + (cpu))
#define	  CPUXCFG_CL_RST_H_RST		__BIT(12)
#define	  CPUXCFG_CL_RST_L2_RST		__BIT(8)
#define	 CPUXCFG_CL_CTRL0(cluster)	(0x0 + (cluster) * 0x10)
#define	 CPUXCFG_CL_CTRL1(cluster)	(0x4 + (cluster) * 0x10)
#define	  CPUXCFG_CL_CTRL1_ACINACTM	__BIT(0)

#define	CCI_BASE		0x01790000
#define	CCI_SLAVEIF3_BASE	(CCI_BASE + 0x4000)
#define	CCI_SLAVEIF4_BASE	(CCI_BASE + 0x5000)

extern struct bus_space arm_generic_bs_tag;

uint32_t sunxi_mc_cci_port[MAXCPUS] = {
	CCI_SLAVEIF3_BASE,
	CCI_SLAVEIF3_BASE,
	CCI_SLAVEIF3_BASE,
	CCI_SLAVEIF3_BASE,
	CCI_SLAVEIF4_BASE,
	CCI_SLAVEIF4_BASE,
	CCI_SLAVEIF4_BASE,
	CCI_SLAVEIF4_BASE,
};

static uint32_t
sunxi_mc_smp_pa(void)
{
	extern void sunxi_mc_mpstart(void);
	bool ok __diagused;
	paddr_t pa;

	ok = pmap_extract(pmap_kernel(), (vaddr_t)sunxi_mc_mpstart, &pa);
	KASSERT(ok);

	return pa;
}

static int
sunxi_mc_smp_start(bus_space_tag_t bst, bus_space_handle_t prcm, bus_space_handle_t cpucfg,
    bus_space_handle_t cpuxcfg, u_int cluster, u_int cpu)
{
	uint32_t val;
	int i;

	/* Set start vector */
	bus_space_write_4(bst, cpucfg, CPUCFG_P_REG0, sunxi_mc_smp_pa());

	/* Assert core reset */
	val = bus_space_read_4(bst, cpuxcfg, CPUXCFG_CL_RST(cluster));
	val &= ~__BIT(cpu);
	bus_space_write_4(bst, cpuxcfg, CPUXCFG_CL_RST(cluster), val);

	/* Assert power-on reset */
	val = bus_space_read_4(bst, cpucfg, CPUCFG_CL_RST(cluster));
	val &= ~__BIT(cpu);
	bus_space_write_4(bst, cpucfg, CPUCFG_CL_RST(cluster), val);

	/* Disable automatic L1 cache invalidate at reset */
	val = bus_space_read_4(bst, cpuxcfg, CPUXCFG_CL_CTRL0(cluster));
	val &= ~__BIT(cpu);
	bus_space_write_4(bst, cpuxcfg, CPUXCFG_CL_CTRL0(cluster), val);

	/* Release power clamp */
	for (i = 0; i <= 8; i++) {
		bus_space_write_4(bst, prcm, PRCM_CL_PWR_CLAMP(cluster, cpu), 0xff >> i);
		delay(10);
	}
	for (i = 100000; i > 0; i--) {
		if (bus_space_read_4(bst, prcm, PRCM_CL_PWR_CLAMP(cluster, cpu)) == 0)
			break;
	}
	if (i == 0) {
		printf("CPU %#llx failed to start\n", __SHIFTIN(cluster, MPIDR_AFF1) | __SHIFTIN(cpu, MPIDR_AFF0));
		return ETIMEDOUT;
	}

	/* Clear power-off gating */
	val = bus_space_read_4(bst, prcm, PRCM_CL_PWROFF(cluster));
	if (cpu == 0)
		val &= ~__BIT(4);
	val &= ~__BIT(cpu);
	bus_space_write_4(bst, prcm, PRCM_CL_PWROFF(cluster), val);

	/* De-assert power-on reset */
	val = bus_space_read_4(bst, prcm, PRCM_CL_RST_CTRL(cluster));
	val |= __BIT(cpu);
	bus_space_write_4(bst, prcm, PRCM_CL_RST_CTRL(cluster), val);

	val = bus_space_read_4(bst, cpucfg, CPUCFG_CL_RST(cluster));
	val |= __BIT(cpu);
	bus_space_write_4(bst, cpucfg, CPUCFG_CL_RST(cluster), val);
	delay(10);

	/* De-assert core reset */
	val = bus_space_read_4(bst, cpuxcfg, CPUXCFG_CL_RST(cluster));
	val |= __BIT(cpu);
	val |= CPUXCFG_CL_RST_SOC_DBG_RST;
	val |= CPUXCFG_CL_RST_ETM_RST(cpu);
	val |= CPUXCFG_CL_RST_DBG_RST(cpu);
	val |= CPUXCFG_CL_RST_L2_RST;
	val |= CPUXCFG_CL_RST_H_RST;
	bus_space_write_4(bst, cpuxcfg, CPUXCFG_CL_RST(cluster), val);

	/* De-assert ACINACTM */
	val = bus_space_read_4(bst, cpuxcfg, CPUXCFG_CL_CTRL1(cluster));
	val &= ~CPUXCFG_CL_CTRL1_ACINACTM;
	bus_space_write_4(bst, cpuxcfg, CPUXCFG_CL_CTRL1(cluster), val);

	return 0;
}

int
sunxi_mc_smp_enable(u_int mpidr)
{
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t prcm, cpucfg, cpuxcfg;
	int error;

	const u_int cluster = __SHIFTOUT(mpidr, MPIDR_AFF1);
	const u_int cpu = __SHIFTOUT(mpidr, MPIDR_AFF0);

	if (bus_space_map(bst, PRCM_BASE, PRCM_SIZE, 0, &prcm) != 0 ||
	    bus_space_map(bst, CPUCFG_BASE, CPUCFG_SIZE, 0, &cpucfg) != 0 ||
	    bus_space_map(bst, CPUXCFG_BASE, CPUXCFG_SIZE, 0, &cpuxcfg) != 0)
		return ENOMEM;

	error = sunxi_mc_smp_start(bst, prcm, cpucfg, cpuxcfg, cluster, cpu);

	bus_space_unmap(bst, cpuxcfg, CPUXCFG_SIZE);
	bus_space_unmap(bst, cpucfg, CPUCFG_SIZE);
	bus_space_unmap(bst, prcm, PRCM_SIZE);

	return error;
}

int
sunxi_mc_smp_match(const char *enable_method)
{
	return strcmp(enable_method, A83T_SMP_ENABLE_METHOD) == 0;
}
