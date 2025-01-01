/* $NetBSD: jh7110_pcie.c,v 1.1 2025/01/01 17:53:08 skrll Exp $ */

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: jh7110_pcie.c,v 1.1 2025/01/01 17:53:08 skrll Exp $");

#include <sys/param.h>

#include <sys/bitops.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <riscv/fdt/pcihost_fdtvar.h>

struct jh7110_pcie_irq {
	struct jh7110_pcie_softc *
				jpi_sc;
	void			*jpi_arg;
	int			(*jpi_fn)(void *);
	int			jpi_mpsafe;
};

struct jh7110_pcie_softc {
	bus_space_tag_t		sc_bst;

	struct pcihost_softc	sc_phsc;

	bus_space_handle_t	sc_apb_bsh;
	bus_addr_t		sc_apb_addr;
	bus_size_t		sc_apb_size;

	bus_space_handle_t	sc_cfg_bsh;
	bus_addr_t		sc_cfg_addr;
	bus_size_t		sc_cfg_size;

	// syscon
	const struct syscon *	sc_syscon;
	bus_size_t		sc_stg_base;

	struct fdtbus_gpio_pin *sc_perst_gpio;

	// # pins
	struct jh7110_pcie_irq	*sc_irq[PCI_INTERRUPT_PIN_MAX];
};

#define	RD4(sc, reg)							      \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_apb_bsh, (reg))
#define	WR4(sc, reg, val)						      \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_apb_bsh, (reg), (val))

#define SET4(sc, off, mask)						      \
    WR4((sc), (off), RD4((sc), (off)) | (mask))
#define CLR4(sc, off, mask)						      \
    WR4((sc), (off), RD4((sc), (off)) & ~(mask))
#define UPD4(sc, off, clr, set)						      \
    WR4((sc), (off), (RD4((sc), (off)) & ~(clr)) | (set))

#define JH7110_PCIE0_CFG_BASE	0x940000000UL
#define JH7110_PCIE1_CFG_BASE	0x9c0000000UL

/* PLDA register definitions */
#define PLDA_GEN_SETTINGS			0x80
#define  PLDA_GEN_RP_ENABLE			1
#define PLDA_PCI_IDS				0x9c
#define  PLDA_PCI_IDS_REVISION_MASK		__BITS( 7, 0)
#define  PLDA_PCI_IDS_CLASSCODE_MASK		__BITS(31, 8)
#define PLDA_MISC				0xb4
#define  PLDA_MISC_PHYFUNC_DISABLE		__BIT(15)
#define PLDA_WINROM				0xfc
#define  PLDA_WINROM_PREF64SUPPORT		__BIT(3)

#define PLDA_IMASK_LOCAL			0x180
#define  PLDA_IMASK_INT_INTA			__BIT(24)
#define  PLDA_IMASK_INT_INTB			__BIT(25)
#define  PLDA_IMASK_INT_INTC			__BIT(26)
#define  PLDA_IMASK_INT_INTD			__BIT(27)
#define  PLDA_IMASK_INT_INTX			__BITS(27, 24)
#define  PLDA_IMASK_INT_MSI			__BIT(28)
#define PLDA_ISTATUS_LOCAL			0x184
#define  PLDA_ISTATUS_INT_INTA			__BIT(24)
#define  PLDA_ISTATUS_INT_INTB			__BIT(25)
#define  PLDA_ISTATUS_INT_INTC			__BIT(26)
#define  PLDA_ISTATUS_INT_INTD			__BIT(27)
#define  PLDA_ISTATUS_INT_INTX			__BITS(27, 24)
#define  PLDA_ISTATUS_INT_MSI			__BIT(28)
#define PLDA_IMASK_HOST				0x188
#define PLDA_ISTATUS_HOST			0x18c
#define PLDA_IMSI_ADDR				0x190
#define PLDA_ISTATUS_MSI			0x194
#define PLDA_PMSG_SUPPORT_RX			0x3f0
#define  PLDA_PMSG_LTR_SUPPORT			__BIT(2)

/* PCIe Master table init defines */
#define PLDA_ATR0_PCIE_WIN0_SRCADDR_PARAM	0x600
#define  PLDA_ATR0_PCIE_ATR_SIZE		0x25
#define  PLDA_ATR0_PCIE_ATR_SIZE_SHIFT		1
#define PLDA_ATR0_PCIE_WIN0_SRC_ADDR		0x604
#define PLDA_ATR0_PCIE_WIN0_TRSL_ADDR_LSB	0x608
#define PLDA_ATR0_PCIE_WIN0_TRSL_ADDR_UDW	0x60c
#define PLDA_ATR0_PCIE_WIN0_TRSL_PARAM		0x610

#define PLDA_ATR_AXI4_SLV0_SRC_ADDR_LO(n)	(0x800 + (n) * 0x20)
#define  PLDA_ATR_SIZE_SRC_MASK			__BITS(31, 12)
#define  PLDA_ATR_SIZE_MASK			__BITS(6, 1)
#define  PLDA_ATR_IMPL				__BIT(0)
#define PLDA_ATR_AXI4_SLV0_SRC_ADDR_HI(n)	(0x804 + (n) * 0x20)
#define PLDA_ATR_AXI4_SLV0_TRSL_ADDR_LO(n)	(0x808 + (n) * 0x20)
#define PLDA_ATR_AXI4_SLV0_TRSL_ADDR_HI(n)	(0x80c + (n) * 0x20)
#define PLDA_ATR_AXI4_SLV0_TRSL_PARAM(n)	(0x810 + (n) * 0x20)
#define  PLDA_TRSL_ID_PCIE_RX_TX		0
#define  PLDA_TRSL_ID_PCIE_CONFIG		1

#define PCIE_FUNC_NUM				4

/* system control */
#define STG_SYSCON_PCIE0_BASE			0x0048
#define STG_SYSCON_PCIE1_BASE			0x01f8

#define STG_SYSCON_AR_OFFSET			0x0078
#define STG_SYSCON_AXI4_SLVL_AR_MASK		__BITS(22, 8)
#define  STG_SYSCON_AXI4_SLVL_PHY_AR_MASK	__BITS(20,17)
#define  STG_SYSCON_AXI4_SLVL_PHY_AR(x)		\
    __SHIFTIN((x), STG_SYSCON_AXI4_SLVL_PHY_AR_MASK)

#define STG_SYSCON_AW_OFFSET			0x007c
#define STG_SYSCON_CLKREQ			__BIT(22)
#define STG_SYSCON_CKREF_SRC_MASK		__BITS(19, 18)
#define STG_SYSCON_AXI4_SLVL_AW_MASK		__BITS(14,  0)
#define  STG_SYSCON_AXI4_SLVL_PHY_AW_MASK	__BITS(12,  9)
#define  STG_SYSCON_AXI4_SLVL_PHY_AW(x)		\
    __SHIFTIN((x), STG_SYSCON_AXI4_SLVL_PHY_AW_MASK)

#define STG_SYSCON_RP_NEP_OFFSET		0x00e8
#define STG_SYSCON_K_RP_NEP			__BIT(8)

#define STG_SYSCON_LNKSTA_OFFSET		0x0170
#define DATA_LINK_ACTIVE			__BIT(5)

#define ECAM_BUS_MASK				__BITS(27, 20)
#define ECAM_DEV_MASK				__BITS(19, 15)
#define ECAM_FUNC_MASK				__BITS(14, 12)
#define ECAM_OFFSET_MASK			__BITS(11,  0)

static int
jh7110_pcie_bus_maxdevs(void *v, int bus)
{
	struct pcihost_softc * const phsc = v;

	if (bus >= phsc->sc_bus_min || bus <= phsc->sc_bus_max)
		return 1;
	return 0;
}

static pcitag_t
jh7110_pcie_make_tag(void *v, int bus, int dev, int fn)
{
//	struct pcihost_softc * const phsc = v;

	/* Return ECAM address. */
	return
	    __SHIFTIN(bus, ECAM_BUS_MASK) |
	    __SHIFTIN(dev, ECAM_DEV_MASK) |
	    __SHIFTIN(fn, ECAM_FUNC_MASK) |
	    0;
}

static void
jh7110_pcie_decompose_tag(void *v, pcitag_t tag,
    int *busp, int *devp, int *fnp)
{
//	struct pcihost_softc * const phsc = v;

	if (busp != NULL)
		*busp = __SHIFTOUT(tag, ECAM_BUS_MASK);
	if (devp != NULL)
		*devp = __SHIFTOUT(tag, ECAM_DEV_MASK);
	if (fnp != NULL)
		*fnp = __SHIFTOUT(tag, ECAM_FUNC_MASK);
}

static bool
jh7110_pcie_conf_ok(struct jh7110_pcie_softc *sc,
    int bus, int dev, int fn, int offset)
{

	/* Only one device on root port and the first subordinate port. */
	if (bus < 2 && dev < 1)
		return true;

	return false;
}

static pcireg_t
jh7110_pcie_conf_read(void *v, pcitag_t tag, int offset)
{
	struct pcihost_softc * const phsc = v;
	struct jh7110_pcie_softc * const sc =
	    container_of(phsc, struct jh7110_pcie_softc, sc_phsc);
	int bus, dev, fn;

	KASSERT(offset >= 0);
	KASSERT(offset < PCI_EXTCONF_SIZE);

	jh7110_pcie_decompose_tag(phsc, tag, &bus, &dev, &fn);

	if (!jh7110_pcie_conf_ok(sc, bus, dev, fn, offset))
		return 0xffffffff;

	bus_size_t reg =
	    __SHIFTIN(bus, ECAM_BUS_MASK) |
	    __SHIFTIN(dev, ECAM_DEV_MASK) |
	    __SHIFTIN(fn, ECAM_FUNC_MASK) |
	    offset;

	return bus_space_read_4(sc->sc_bst, sc->sc_cfg_bsh, reg);
}

static void
jh7110_pcie_conf_write(void *v, pcitag_t tag, int offset, pcireg_t data)
{
	struct pcihost_softc * const phsc = v;
	struct jh7110_pcie_softc * const sc =
	    container_of(phsc, struct jh7110_pcie_softc, sc_phsc);
	int bus, dev, fn;

	KASSERT(offset >= 0);
	KASSERT(offset < PCI_EXTCONF_SIZE);

	jh7110_pcie_decompose_tag(phsc, tag, &bus, &dev, &fn);

	if (!jh7110_pcie_conf_ok(sc, bus, dev, fn, offset))
		return;

	bus_size_t reg =
	    __SHIFTIN(bus, ECAM_BUS_MASK) |
	    __SHIFTIN(dev, ECAM_DEV_MASK) |
	    __SHIFTIN(fn, ECAM_FUNC_MASK) |
	    offset;

	bus_space_write_4(sc->sc_bst, sc->sc_cfg_bsh, reg, data);
}

/* INTx interrupt controller */
static void *
jh7110_pcie_intx_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct jh7110_pcie_softc * const sc = device_private(dev);
	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;
	const u_int pin = be32toh(specifier[0]) - 1;

	KASSERT((RD4(sc, PLDA_IMASK_LOCAL) & (PLDA_IMASK_INT_INTA << pin)) == 0);

	struct jh7110_pcie_irq *jpi = sc->sc_irq[pin];
	if (jpi == NULL) {
		jpi = kmem_alloc(sizeof(*jpi), KM_SLEEP);
		jpi->jpi_sc = sc;
		jpi->jpi_fn = func;
		jpi->jpi_arg = arg;
		jpi->jpi_mpsafe = mpsafe;

		sc->sc_irq[pin] = jpi;
	} else {
		device_printf(dev, "shared interrupts not supported\n");
		return NULL;
	}

	/* Unmask the interrupt. */
	SET4(sc, PLDA_IMASK_LOCAL, (PLDA_IMASK_INT_INTA << pin));

	return jpi;
}


static void
jh7110_pcie_intx_disestablish(device_t dev, void *ih)
{
	struct jh7110_pcie_softc * const sc = device_private(dev);
	struct pcihost_softc * const phsc = &sc->sc_phsc;

	device_printf(dev, "%s\n", __func__);

	fdtbus_intr_disestablish(phsc->sc_phandle, ih);
}

static bool
jh7110_pcie_intx_string(device_t dev, u_int *specifier, char *buf,
    size_t buflen)
{
	struct jh7110_pcie_softc * const sc = device_private(dev);
	struct pcihost_softc * const phsc = &sc->sc_phsc;

	fdtbus_intr_str(phsc->sc_phandle, 0, buf, buflen);

	return true;
}

static int
jh7110_pcie_intx_intr(struct jh7110_pcie_softc *sc, uint32_t status)
{
	int handled = 0;
	u_int pin;

	CTASSERT(__arraycount(sc->sc_irq) == 4);
	for (pin = 0; pin < __arraycount(sc->sc_irq); pin++) {
		if ((status & (PLDA_IMASK_INT_INTA << pin)) == 0)
			continue;

		struct jh7110_pcie_irq *jpi = sc->sc_irq[pin];

		if (jpi == NULL)
			continue;

		if (!jpi->jpi_mpsafe)
			KERNEL_LOCK(1, NULL);
		handled |= jpi->jpi_fn(jpi->jpi_arg);
		if (!jpi->jpi_mpsafe)
			KERNEL_UNLOCK_ONE(NULL);
	}

	return handled;
}


static int
jh7110_pcie_intr(void *v)
{
	struct jh7110_pcie_softc * const sc = v;
	int handled = 0;

	uint32_t status = RD4(sc, PLDA_ISTATUS_LOCAL);
	if (status == 0)
		return 0;

	if (status & PLDA_ISTATUS_INT_INTX)
		handled |= jh7110_pcie_intx_intr(sc, status);

	WR4(sc, PLDA_ISTATUS_LOCAL, status);

	return handled;
}

static struct fdtbus_interrupt_controller_func jh7110_pcie_intxfuncs = {
	.establish = jh7110_pcie_intx_establish,
	.disestablish = jh7110_pcie_intx_disestablish,
	.intrstr = jh7110_pcie_intx_string,
};

#define SCRD4(sc, off)		syscon_read_4((sc), (off))
#define SCWR4(sc, off, val)	syscon_write_4((sc), (off), (val))

#define SCSET4(sc, off, mask)						      \
    SCWR4((sc), (off), SCRD4((sc), (off)) | (mask))
#define SCCLR4(sc, off, mask)						      \
    SCWR4((sc), (off), SCRD4((sc), (off)) & ~(mask))
#define SCUPD4(sc, off, clr, set)					      \
    SCWR4((sc), (off), (SCRD4((sc), (off)) & ~(clr)) | (set))


static int
jh7110_pcie_host_init(struct jh7110_pcie_softc *sc)
{
	struct pcihost_softc * const phsc = &sc->sc_phsc;

	syscon_lock(sc->sc_syscon);
	SCSET4(sc->sc_syscon, sc->sc_stg_base + STG_SYSCON_RP_NEP_OFFSET,
	    STG_SYSCON_K_RP_NEP);

	SCUPD4(sc->sc_syscon, sc->sc_stg_base + STG_SYSCON_AW_OFFSET,
	    STG_SYSCON_CKREF_SRC_MASK,
	    __SHIFTIN(2, STG_SYSCON_CKREF_SRC_MASK));

	SCSET4(sc->sc_syscon, sc->sc_stg_base + STG_SYSCON_AW_OFFSET,
	    STG_SYSCON_CLKREQ);

	/* enable clocks */
	struct clk *clk;
	fdtbus_clock_assign(phsc->sc_phandle);
	for (u_int c = 0;
	    (clk = fdtbus_clock_get_index(phsc->sc_phandle, c)) != NULL;
	    c++) {
		if (clk_enable(clk) != 0) {
			aprint_error_dev(phsc->sc_dev,
			    ": couldn't enable clock #%d\n", c);
			return ENXIO;
		}
	}
	/* de-assert resets */
	struct fdtbus_reset *rst;
	for (u_int r = 0;
	    (rst = fdtbus_reset_get_index(phsc->sc_phandle, r)) != NULL;
	    r++) {
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error_dev(phsc->sc_dev,
			    ": couldn't de-assert reset #%d\n", r);
			return ENXIO;
		}
	}

	fdtbus_gpio_write(sc->sc_perst_gpio, 1);

	/* Disable additional functions. */
	for (u_int i = 1; i < PCIE_FUNC_NUM; i++) {

		SCUPD4(sc->sc_syscon, sc->sc_stg_base + STG_SYSCON_AR_OFFSET,
		    STG_SYSCON_AXI4_SLVL_AR_MASK,
		    __SHIFTIN(i, STG_SYSCON_AXI4_SLVL_PHY_AR_MASK));

		SCUPD4(sc->sc_syscon, sc->sc_stg_base + STG_SYSCON_AW_OFFSET,
		    STG_SYSCON_AXI4_SLVL_AW_MASK,
		    __SHIFTIN(i, STG_SYSCON_AXI4_SLVL_PHY_AW_MASK));

		SET4(sc, PLDA_MISC, PLDA_MISC_PHYFUNC_DISABLE);
	}

	SCCLR4(sc->sc_syscon, sc->sc_stg_base + STG_SYSCON_AR_OFFSET,
	    STG_SYSCON_AXI4_SLVL_AR_MASK);

	SCCLR4(sc->sc_syscon, sc->sc_stg_base + STG_SYSCON_AW_OFFSET,
	    STG_SYSCON_AXI4_SLVL_AW_MASK);

	/* Configure controller as root port. */
	UPD4(sc, PLDA_GEN_SETTINGS, PLDA_GEN_RP_ENABLE, PLDA_GEN_RP_ENABLE);
	uint64_t base_addr = 0;

#define CONFIG_SPACE_ADDR_OFFSET 0x1000

	SET4(sc, CONFIG_SPACE_ADDR_OFFSET + 0x10, BUS_ADDR_LO32(base_addr));
	SET4(sc, CONFIG_SPACE_ADDR_OFFSET + 0x14, BUS_ADDR_HI32(base_addr));

	/* Configure as PCI bridge. */
	UPD4(sc, PLDA_PCI_IDS,
	    PLDA_PCI_IDS_CLASSCODE_MASK,
	    PCI_CLASS_CODE(PCI_CLASS_BRIDGE,
		PCI_SUBCLASS_BRIDGE_PCI,
		PCI_INTERFACE_BRIDGE_PCI_PCI));

	/* Enable prefetchable memory windows. */
	SET4(sc, PLDA_WINROM, PLDA_WINROM_PREF64SUPPORT);

	/* Disable LTR message forwarding. */
	CLR4(sc, PLDA_PMSG_SUPPORT_RX, PLDA_PMSG_LTR_SUPPORT);

	/*
	 * PERST# must remain asserted for at least 100us after the
	 * reference clock becomes stable.  But also has to remain
	 * active at least 100ms after power up.  Since we may have
	 * just powered on the device, play it safe and use 100ms.
	 */
	delay(100 * 1000);

	/* Deassert PERST#. */
	fdtbus_gpio_write(sc->sc_perst_gpio, 0);

	/* Wait for link to come up. */
	uint32_t reg;
	for (int timo = 100; timo > 0; timo--) {
		reg = SCRD4(sc->sc_syscon,
		    sc->sc_stg_base + STG_SYSCON_LNKSTA_OFFSET);
		if (reg & DATA_LINK_ACTIVE)
			break;
		delay(1000);
	}

	syscon_unlock(sc->sc_syscon);

	if ((reg & DATA_LINK_ACTIVE) == 0) {
		aprint_error_dev(phsc->sc_dev, ": link not up\n");
		    return ENXIO;
	}

	return 0;
}

static int
jh7110_pcie_atr_init(struct jh7110_pcie_softc *sc)
{
	const uint32_t *ranges;
	bus_addr_t phyaddr;	// PADDR?
	bus_addr_t pciaddr;
	bus_size_t size;

	WR4(sc, PLDA_ATR_AXI4_SLV0_SRC_ADDR_LO(0),
	    PLDA_ATR_IMPL |
	    __SHIFTIN(ilog2(sc->sc_cfg_size) - 1, PLDA_ATR_SIZE_MASK) |
	    BUS_ADDR_LO32(sc->sc_cfg_addr));
	WR4(sc, PLDA_ATR_AXI4_SLV0_SRC_ADDR_HI(0), BUS_ADDR_HI32(sc->sc_cfg_addr));
	WR4(sc, PLDA_ATR_AXI4_SLV0_TRSL_ADDR_LO(0), 0);
	WR4(sc, PLDA_ATR_AXI4_SLV0_TRSL_ADDR_HI(0), 0);
	WR4(sc, PLDA_ATR_AXI4_SLV0_TRSL_PARAM(0), PLDA_TRSL_ID_PCIE_CONFIG);

	struct pcihost_softc * const phsc = &sc->sc_phsc;
	int ranges_len;
	ranges = fdtbus_get_prop(phsc->sc_phandle, "ranges", &ranges_len);
	if (ranges == NULL) {
		aprint_error_dev(phsc->sc_dev,
		    ": couldn't find 'ranges' property\n");
		return ENXIO;
	}
	const int ranges_cells = ranges_len / sizeof(uint32_t);

	for (u_int i = 0, n = 1; i < ranges_cells && n < 8; i += 7, n++) {
		pciaddr =
		    __SHIFTIN(be32toh(ranges[i + 1]), __BITS(63, 32)) |
		    __SHIFTIN(be32toh(ranges[i + 2]), __BITS(31,  0));
		phyaddr =
		    __SHIFTIN(be32toh(ranges[i + 3]), __BITS(63, 32)) |
		    __SHIFTIN(be32toh(ranges[i + 4]), __BITS(31,  0));
		size = be32toh(ranges[i + 6]);

		WR4(sc, PLDA_ATR_AXI4_SLV0_SRC_ADDR_LO(n),
		    PLDA_ATR_IMPL |
		    __SHIFTIN(ilog2(size) /* - 1 */, PLDA_ATR_SIZE_MASK) |
		    BUS_ADDR_LO32(phyaddr));
		WR4(sc, PLDA_ATR_AXI4_SLV0_SRC_ADDR_HI(n), BUS_ADDR_HI32(phyaddr));
		WR4(sc, PLDA_ATR_AXI4_SLV0_TRSL_ADDR_LO(n), BUS_ADDR_LO32(pciaddr));
		WR4(sc, PLDA_ATR_AXI4_SLV0_TRSL_ADDR_HI(n), BUS_ADDR_HI32(pciaddr));
		WR4(sc, PLDA_ATR_AXI4_SLV0_TRSL_PARAM(n), PLDA_TRSL_ID_PCIE_RX_TX);
	}

	uint32_t val;
	val = RD4(sc, PLDA_ATR0_PCIE_WIN0_SRCADDR_PARAM);
	val |= (PLDA_ATR0_PCIE_ATR_SIZE << PLDA_ATR0_PCIE_ATR_SIZE_SHIFT);
	WR4(sc, PLDA_ATR0_PCIE_WIN0_SRCADDR_PARAM, val);
	WR4(sc, PLDA_ATR0_PCIE_WIN0_SRC_ADDR, 0);

	return 0;
}

/* Compat string(s) */
static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7110-pcie" },
	DEVICE_COMPAT_EOL
};


static int
jh7110_pcie_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh7110_pcie_attach(device_t parent, device_t self, void *aux)
{
	struct jh7110_pcie_softc * const sc = device_private(self);
	struct pcihost_softc * const phsc = &sc->sc_phsc;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int error;

	sc->sc_bst = faa->faa_bst;

	phsc->sc_dev = self;
	phsc->sc_bst = faa->faa_bst;
	phsc->sc_pci_bst = faa->faa_bst;
	phsc->sc_dmat = faa->faa_dmat;
	phsc->sc_phandle = faa->faa_phandle;

	/* Handle old and new binding names. */
	if (fdtbus_get_reg_byname(phandle, "apb",
	    &sc->sc_apb_addr, &sc->sc_apb_size) != 0) {
		aprint_error(": couldn't get apb registers\n");
		return;
	}
	if (fdtbus_get_reg_byname(phandle, "cfg",
	    &sc->sc_cfg_addr, &sc->sc_cfg_size) != 0) {
		aprint_error(": couldn't get cfg registers\n");
		return;
	}

	const int mapflags = 0 /*BUS_SPACE_MAP_NONPOSTED*/;
	error = bus_space_map(sc->sc_bst, sc->sc_apb_addr,
	    sc->sc_apb_size, mapflags, &sc->sc_apb_bsh);
	if (error) {
		aprint_error(": can't map APB registers\n");
		return;
	}
	error = bus_space_map(sc->sc_bst, sc->sc_cfg_addr,
	    sc->sc_cfg_size, mapflags, &sc->sc_cfg_bsh);
	if (error) {
		aprint_error(": can't map CFG registers\n");
		return;
	}
	int len;
	const char *stgsyscon = "starfive,stg-syscon";
	const u_int *stgsyscon_data =
	    fdtbus_get_prop(phandle, stgsyscon, &len);
	if (stgsyscon_data == NULL) {
		aprint_error(": couldn't get '%s' property\n", stgsyscon);
		return;
	}

	if (len != 1 * sizeof(uint32_t)) {
		aprint_error(": incorrect '%s' data (len = %u)\n", stgsyscon,
		    len);
		return;
	}
	int syscon_phandle =
	    fdtbus_get_phandle_from_native(be32dec(&stgsyscon_data[0]));

	sc->sc_syscon = fdtbus_syscon_lookup(syscon_phandle);
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get '%s' (%d)\n", stgsyscon,
		    syscon_phandle);
		return;
	}

	sc->sc_perst_gpio = fdtbus_gpio_acquire(phandle,
	    "perst-gpios", GPIO_PIN_OUTPUT);
	if (sc->sc_perst_gpio == NULL) {
		aprint_error(": couldn't get 'perst-gpios'");
		return;
	}

	switch (sc->sc_cfg_addr) {
	case JH7110_PCIE0_CFG_BASE:
		sc->sc_stg_base = STG_SYSCON_PCIE0_BASE;
		break;
	case JH7110_PCIE1_CFG_BASE:
		sc->sc_stg_base = STG_SYSCON_PCIE1_BASE;
		break;
	default:
		aprint_error(": unknown controller at 0x%lx\n",
		    sc->sc_cfg_addr);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": PCIe\n");

	error = jh7110_pcie_host_init(sc);
	if (error) {
		/* error already printed */
		return;
	}

	/* Configure Address Translation. */
	error = jh7110_pcie_atr_init(sc);
	if (error) {

	}

	/* Mask and acknowledge all interrupts. */
	WR4(sc, PLDA_IMASK_LOCAL, 0);
	WR4(sc, PLDA_ISTATUS_LOCAL, 0xffffffff);

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	void *ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, jh7110_pcie_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		// XXXNH unwind
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	fdtbus_register_interrupt_controller(self,
	    OF_child(phsc->sc_phandle), &jh7110_pcie_intxfuncs);

	phsc->sc_type = PCIHOST_ECAM;
	pcihost_init(&phsc->sc_pc, phsc);

	phsc->sc_pc.pc_bus_maxdevs = jh7110_pcie_bus_maxdevs;
	phsc->sc_pc.pc_make_tag = jh7110_pcie_make_tag;
	phsc->sc_pc.pc_decompose_tag = jh7110_pcie_decompose_tag;
	phsc->sc_pc.pc_conf_read = jh7110_pcie_conf_read;
	phsc->sc_pc.pc_conf_write = jh7110_pcie_conf_write;

	pcihost_init2(phsc);
}

CFATTACH_DECL_NEW(jh7110_pcie, sizeof(struct jh7110_pcie_softc),
	jh7110_pcie_match, jh7110_pcie_attach, NULL, NULL);
