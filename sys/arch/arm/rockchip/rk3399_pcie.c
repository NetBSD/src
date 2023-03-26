/* $NetBSD: rk3399_pcie.c,v 1.20 2023/03/26 19:10:33 andvar Exp $ */
/*
 * Copyright (c) 2018 Mark Kettenis <kettenis@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: rk3399_pcie.c,v 1.20 2023/03/26 19:10:33 andvar Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bitops.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <machine/intr.h>
#include <sys/bus.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>
#include <arm/cpufunc.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <arm/fdt/pcihost_fdtvar.h>
#include <sys/gpio.h>

#define SETREG(m, v)			((m)<<16|__SHIFTIN((v), (m)))
#define GETREG(m, v)			(__SHIFTOUT((v), (m)))

/* APB region */
#define PCIE_CLIENT_BASE		0x000000
#define PCIE_CLIENT_BASIC_STRAP_CONF	0x0000
#define  PCBSC_PCIE_GEN_SEL		__BIT(7)
#define   PCBSC_PGS_GEN1		SETREG(PCBSC_PCIE_GEN_SEL, 0)
#define   PCBSC_PGS_GEN2		SETREG(PCBSC_PCIE_GEN_SEL, 1)
#define  PCBSC_MODE_SELECT		__BIT(6)
#define   PCBSC_MS_ENDPOINT		SETREG(PCBSC_MODE_SELECT, 0)
#define   PCBSC_MS_ROOTPORT		SETREG(PCBSC_MODE_SELECT, 1)
#define  PCBSC_LANE_COUNT		__BITS(5,4)
#define   PCBSC_LC(x)			SETREG(PCBSC_LANE_COUNT, ilog2(x)) /* valid for x1,2,4 */
#define  PCBSC_ARI_EN			SETREG(__BIT(3), 1) /* Alternate Routing ID Enable */
#define  PCBSC_SR_IOV_EN		SETREG(__BIT(2), 1)
#define  PCBSC_LINK_TRAIN_EN		SETREG(__BIT(1), 1)
#define  PCBSC_CONF_EN			SETREG(__BIT(0), 1) /* Config enable */
#define PCIE_CLIENT_DEBUG_OUT_0		0x003c
#define PCIE_CLIENT_DEBUG_OUT_1		0x0040
#define PCIE_CLIENT_BASIC_STATUS0	0x0044
#define PCIE_CLIENT_BASIC_STATUS1	0x0048
#define  PCBS1_LINK_ST(x)		(u_int)__SHIFTOUT((x), __BITS(21,20))
#define   PCBS1_LS_NO_RECV		0	/* no receivers */
#define   PCBS1_LS_TRAINING		1	/* link training */
#define   PCBS1_LS_DL_INIT		2	/* link up, DL init progressing */
#define   PCBS1_LS_DL_DONE		3	/* link up, DL init complete */
#define PCIE_CLIENT_INT_MASK		0x004c
#define   PCIM_INTx_MASK(x)		SETREG(__BIT((x)+5), 1)
#define   PCIM_INTx_ENAB(x)		SETREG(__BIT((x)+5), 0)

#define PCIE_CORE_BASE			0x800000
#define PCIE_RC_NORMAL_BASE		(PCIE_CORE_BASE + 0x00000)

#define PCIE_LM_BASE			0x900000
#define PCIE_LM_CORE_CTRL		(PCIE_LM_BASE + 0x00)
#define   PCIE_CORE_PL_CONF_SPEED_5G            0x00000008
#define   PCIE_CORE_PL_CONF_SPEED_MASK          0x00000018
#define   PCIE_CORE_PL_CONF_LANE_MASK           0x00000006
#define   PCIE_CORE_PL_CONF_LANE_SHIFT          1
#define PCIE_LM_PLC1			(PCIE_LM_BASE + 0x04)
#define  PCIE_LM_PLC1_FTS_MASK			__BITS(23, 8)
#define PCIE_LM_VENDOR_ID		(PCIE_LM_BASE + 0x44)
#define PCIE_LM_LINKWIDTH		(PCIE_LM_BASE + 0x50)
#define PCIE_LM_LANEMAP			(PCIE_LM_BASE + 0x200)
#define PCIE_LM_DEBUG_MUX_CONTROL	(PCIE_LM_BASE + 0x208)
#define PCIE_LM_RCBAR			(PCIE_LM_BASE + 0x300)
#define  PCIE_LM_RCBARPME		__BIT(17)
#define  PCIE_LM_RCBARPMS		__BIT(18)
#define  PCIE_LM_RCBARPIE		__BIT(19)
#define  PCIE_LM_RCBARPIS		__BIT(20)

#define PCIE_RC_BASE			0xa00000
#define PCIE_RC_CONFIG_DCSR		(PCIE_RC_BASE + 0x0c0 + PCIE_DCSR)
#define PCIE_RC_PCIE_LCAP		(PCIE_RC_BASE + 0x0c0 + PCIE_LCAP)
#define PCIE_RC_CONFIG_LCSR		(PCIE_RC_BASE + 0x0c0 + PCIE_LCSR)
#define PCIE_RC_CONFIG_THP_CAP          (PCIE_RC_BASE + 0x274)
#define   PCIE_RC_CONFIG_THP_CAP_NEXT_MASK      __BITS(31, 20)


#define PCIE_ATR_BASE			0xc00000
#define PCIE_ATR_OB_ADDR0(i)		(PCIE_ATR_BASE + 0x000 + (i) * 0x20)
#define PCIE_ATR_OB_ADDR1(i)		(PCIE_ATR_BASE + 0x004 + (i) * 0x20)
#define PCIE_ATR_OB_DESC0(i)		(PCIE_ATR_BASE + 0x008 + (i) * 0x20)
#define PCIE_ATR_OB_DESC1(i)		(PCIE_ATR_BASE + 0x00c + (i) * 0x20)
#define PCIE_ATR_IB_ADDR0(i)		(PCIE_ATR_BASE + 0x800 + (i) * 0x8)
#define PCIE_ATR_IB_ADDR1(i)		(PCIE_ATR_BASE + 0x804 + (i) * 0x8)
#define  PCIE_ATR_HDR_MEM		0x2
#define  PCIE_ATR_HDR_IO		0x6
#define  PCIE_ATR_HDR_CFG_TYPE0		0xa
#define  PCIE_ATR_HDR_CFG_TYPE1		0xb
#define  PCIE_ATR_HDR_RID		__BIT(23)

/* AXI region */
#define PCIE_ATR_OB_REGION0_SIZE	(32 * 1024 * 1024)
#define PCIE_ATR_OB_REGION_SIZE		(1 * 1024 * 1024)

#define HREAD4(sc, reg)							\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define AXIPEEK4(sc, reg, valp)						\
	bus_space_peek_4((sc)->sc_iot, (sc)->sc_axi_ioh, (reg), (valp))
#define AXIPOKE4(sc, reg, val)						\
	bus_space_poke_4((sc)->sc_iot, (sc)->sc_axi_ioh, (reg), (val))

struct rkpcie_softc {
	struct pcihost_softc	sc_phsc;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_space_handle_t	sc_axi_ioh;
	bus_addr_t		sc_axi_addr;
	bus_addr_t		sc_apb_addr;
	bus_size_t		sc_axi_size;
	bus_size_t		sc_apb_size;
	kmutex_t		sc_conf_lock;
};

static int rkpcie_match(device_t, cfdata_t, void *);
static void rkpcie_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rkpcie, sizeof(struct rkpcie_softc),
        rkpcie_match, rkpcie_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3399-pcie" },
	DEVICE_COMPAT_EOL
};

static int
rkpcie_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args *faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void	rkpcie_atr_init(struct rkpcie_softc *);

static int	rkpcie_bus_maxdevs(void *, int);
static pcitag_t rkpcie_make_tag(void *, int, int, int);
static void	rkpcie_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t rkpcie_conf_read(void *, pcitag_t, int);
static void	rkpcie_conf_write(void *, pcitag_t, int, pcireg_t);
static int	rkpcie_conf_hook(void *, int, int, int, pcireg_t);

static struct fdtbus_interrupt_controller_func rkpcie_intrfuncs;

static inline void
clock_enable_all(int phandle)
{
	for (u_int i = 0; i < 4; i++) {
		struct clk * clk = fdtbus_clock_get_index(phandle, i);
		if (clk == NULL)
			continue;
		if (clk_enable(clk) != 0)
			continue;
	}
}

static void
reset_assert(int phandle, const char *name)
{
	struct fdtbus_reset *rst;

	rst = fdtbus_reset_get(phandle, name);
	fdtbus_reset_assert(rst);
	fdtbus_reset_put(rst);
}

static void
reset_deassert(int phandle, const char *name)
{
	struct fdtbus_reset *rst;

	rst = fdtbus_reset_get(phandle, name);
	fdtbus_reset_deassert(rst);
	fdtbus_reset_put(rst);
}

static void
rkpcie_attach(device_t parent, device_t self, void *aux)
{
	struct rkpcie_softc *sc = device_private(self);
	struct pcihost_softc * const phsc = &sc->sc_phsc;
	struct fdt_attach_args *faa = aux;
	struct fdtbus_gpio_pin *ep_gpio;
	u_int max_link_speed, num_lanes, bus_scan_delay_ms;
	struct fdtbus_phy *phy[4];
	const u_int *bus_range;
	uint32_t status;
	uint32_t delayed_ms = 0;
	int timo, len;

	phsc->sc_dev = self;
	phsc->sc_bst = faa->faa_bst;
	phsc->sc_pci_bst = faa->faa_bst;
	phsc->sc_dmat = faa->faa_dmat;
	sc->sc_iot = phsc->sc_bst;
	phsc->sc_phandle = faa->faa_phandle;
	const int phandle = phsc->sc_phandle;

	if (fdtbus_get_reg_byname(faa->faa_phandle, "axi-base", &sc->sc_axi_addr, &sc->sc_axi_size) != 0) {
		aprint_error(": couldn't get axi registers\n");
		return;
	}
	if (fdtbus_get_reg_byname(faa->faa_phandle, "apb-base", &sc->sc_apb_addr, &sc->sc_apb_size) != 0) {
		aprint_error(": couldn't get apb registers\n");
		sc->sc_axi_size = 0;
		return;
	}

	const int mapflags = BUS_SPACE_MAP_NONPOSTED;
	if (bus_space_map(sc->sc_iot, sc->sc_apb_addr, sc->sc_apb_size, mapflags, &sc->sc_ioh) != 0 ||
	    bus_space_map(sc->sc_iot, sc->sc_axi_addr, sc->sc_axi_size, mapflags, &sc->sc_axi_ioh) != 0) {
		printf(": can't map registers\n");
		sc->sc_axi_size = 0;
		sc->sc_apb_size = 0;
		return;
	}

	aprint_naive("\n");
	aprint_normal(": RK3399 PCIe\n");

	struct fdtbus_regulator *regulator;
	regulator = fdtbus_regulator_acquire(phandle, "vpcie3v3-supply");
	if (regulator != NULL) {
		fdtbus_regulator_enable(regulator);
		fdtbus_regulator_release(regulator);
	}
		
	fdtbus_clock_assign(phandle);
	clock_enable_all(phandle);

	ep_gpio = fdtbus_gpio_acquire(phandle, "ep-gpios", GPIO_PIN_OUTPUT);

	/*
	 * Let board-specific properties override the default, which is set
	 * to PCIe 1.x, due to errata in the RK3399 CPU.  We don't know exactly
	 * what these errata involved (not public), but posts from the
	 * @rock-chips.com domain to u-boot and linux-kernel lists indicate
	 * that there is a errata related to this, and indeed, the Datasheet
	 * since at least Rev 1.6 and including the latest Rev 1.8 say that the
	 * PCIe can handle 2.5GT/s (ie, PCIe 1.x).
	 */
	if (of_getprop_uint32(phandle, "max-link-speed", &max_link_speed) != 0)
		max_link_speed = 1;
	if (of_getprop_uint32(phandle, "num-lanes", &num_lanes) != 0)
		num_lanes = 1;

	/*
	 * If the DT has a "bus-scan-delay-ms" property, delay attaching the
	 * PCI bus this many microseconds.
	 */
	if (of_getprop_uint32(phandle, "bus-scan-delay-ms",
	    &bus_scan_delay_ms) != 0)
		bus_scan_delay_ms = 0;

again:
	fdtbus_gpio_write(ep_gpio, 0);

	reset_assert(phandle, "aclk");
	reset_assert(phandle, "pclk");
	reset_assert(phandle, "pm");

	memset(phy, 0, sizeof(phy));
	phy[0] = fdtbus_phy_get(phandle, "pcie-phy-0");
	if (phy[0] == NULL) {
		phy[0] = fdtbus_phy_get(phandle, "pcie-phy");
	} else {
		phy[1] = fdtbus_phy_get(phandle, "pcie-phy-1");
		phy[2] = fdtbus_phy_get(phandle, "pcie-phy-2");
		phy[3] = fdtbus_phy_get(phandle, "pcie-phy-3");
	}

	reset_assert(phandle, "core");
	reset_assert(phandle, "mgmt");
	reset_assert(phandle, "mgmt-sticky");
	reset_assert(phandle, "pipe");

	delay(1000);	/* TPERST. use 1ms */
	delayed_ms += 1;
	
	reset_deassert(phandle, "pm");
	reset_deassert(phandle, "aclk");
	reset_deassert(phandle, "pclk");

	if (max_link_speed == 1)
		HWRITE4(sc, PCIE_CLIENT_BASIC_STRAP_CONF, PCBSC_PGS_GEN1);
	else
		HWRITE4(sc, PCIE_CLIENT_BASIC_STRAP_CONF, PCBSC_PGS_GEN2);

	/* Switch into Root Complex mode. */
	HWRITE4(sc, PCIE_CLIENT_BASIC_STRAP_CONF,
	    PCBSC_MS_ROOTPORT | PCBSC_CONF_EN | PCBSC_LC(num_lanes));

	if (phy[3] && fdtbus_phy_enable(phy[3], true) != 0) {
		aprint_error(": couldn't enable phy3\n");
	}
	if (phy[2] && fdtbus_phy_enable(phy[2], true) != 0) {
		aprint_error(": couldn't enable phy2\n");
	}
	if (phy[1] && fdtbus_phy_enable(phy[1], true) != 0) {
		aprint_error(": couldn't enable phy1\n");
	}
	if (phy[0] && fdtbus_phy_enable(phy[0], true) != 0) {
		aprint_error(": couldn't enable phy0\n");
	}

	reset_deassert(phandle, "mgmt-sticky");
	reset_deassert(phandle, "core");
	reset_deassert(phandle, "mgmt");
	reset_deassert(phandle, "pipe");

	fdtbus_gpio_write(ep_gpio, 1);
	delay(20000);	/* 20 ms according to PCI-e BS "Conventional Reset" */
	delayed_ms += 20;

	/* Start link training. */
	HWRITE4(sc, PCIE_CLIENT_BASIC_STRAP_CONF, PCBSC_LINK_TRAIN_EN);

	for (timo = 500; timo > 0; timo--) {
		status = HREAD4(sc, PCIE_CLIENT_BASIC_STATUS1);
		if (PCBS1_LINK_ST(status) == PCBS1_LS_DL_DONE)
			break;
		delay(1000);
		delayed_ms += 1;
	}
	if (timo == 0) {
		device_printf(self, "link training timeout (link_st %u)\n",
		    PCBS1_LINK_ST(status));
		if (max_link_speed > 1) {
			--max_link_speed;
			goto again;
		}
		return;
	}

	if (max_link_speed == 2) {
		HWRITE4(sc, PCIE_RC_CONFIG_LCSR, HREAD4(sc, PCIE_RC_CONFIG_LCSR) | PCIE_LCSR_RETRAIN);
		for (timo = 500; timo > 0; timo--) {
			status = HREAD4(sc, PCIE_LM_CORE_CTRL);
			if ((status & PCIE_CORE_PL_CONF_SPEED_MASK) == PCIE_CORE_PL_CONF_SPEED_5G)
				break;
			delay(1000);
			delayed_ms += 1;
		}
		if (timo == 0) {
			device_printf(self, "Gen2 link training timeout\n");
			--max_link_speed;
			goto again;
		}
	}
	delay(80000);	/* wait 100 ms before CSR access. already waited 20. */
	delayed_ms += 80;

	fdtbus_gpio_release(ep_gpio);

	HWRITE4(sc, PCIE_RC_BASE + PCI_CLASS_REG,
	    PCI_CLASS_BRIDGE << PCI_CLASS_SHIFT |
	    PCI_SUBCLASS_BRIDGE_PCI << PCI_SUBCLASS_SHIFT);

	/* Initialize Root Complex registers. */
	HWRITE4(sc, PCIE_LM_VENDOR_ID, PCI_VENDOR_ROCKCHIP);
	HWRITE4(sc, PCIE_RC_BASE + PCI_CLASS_REG,
	    PCI_CLASS_BRIDGE << PCI_CLASS_SHIFT |
	    PCI_SUBCLASS_BRIDGE_PCI << PCI_SUBCLASS_SHIFT);
	HWRITE4(sc, PCIE_LM_RCBAR, PCIE_LM_RCBARPIE | PCIE_LM_RCBARPIS);

	/* remove L1 substate cap */
	status = HREAD4(sc, PCIE_RC_CONFIG_THP_CAP);
	status &= ~PCIE_RC_CONFIG_THP_CAP_NEXT_MASK;
	HWRITE4(sc, PCIE_RC_CONFIG_THP_CAP, status);

	if (of_hasprop(phandle, "aspm-no-l0s")) {
		status = HREAD4(sc, PCIE_RC_PCIE_LCAP);
		status &= ~__SHIFTIN(1, PCIE_LCAP_ASPM);
		HWRITE4(sc, PCIE_RC_PCIE_LCAP, status);
	}

	/* Default bus ranges */
	sc->sc_phsc.sc_bus_min = 0;
	sc->sc_phsc.sc_bus_max = 31;

	/* Override bus range from DT */
	bus_range = fdtbus_get_prop(phandle, "bus-range", &len);
	if (len == 8) {
		sc->sc_phsc.sc_bus_min = be32dec(&bus_range[0]);
		sc->sc_phsc.sc_bus_max = be32dec(&bus_range[1]);
	}

	if (sc->sc_phsc.sc_bus_min != 0) {
		aprint_error_dev(self, "bus-range doesn't start at 0\n");
		return;
	}

	/* Configure Address Translation. */
	rkpcie_atr_init(sc);

	fdtbus_register_interrupt_controller(self, OF_child(sc->sc_phsc.sc_phandle),
	            &rkpcie_intrfuncs);

	sc->sc_phsc.sc_type = PCIHOST_ECAM;
	sc->sc_phsc.sc_pci_flags |= PCI_FLAGS_MSI_OKAY;
	sc->sc_phsc.sc_pci_flags |= PCI_FLAGS_MSIX_OKAY;
	pcihost_init(&sc->sc_phsc.sc_pc, sc);
	sc->sc_phsc.sc_pc.pc_bus_maxdevs = rkpcie_bus_maxdevs;
	sc->sc_phsc.sc_pc.pc_make_tag = rkpcie_make_tag;
	sc->sc_phsc.sc_pc.pc_decompose_tag = rkpcie_decompose_tag;
	sc->sc_phsc.sc_pc.pc_conf_read = rkpcie_conf_read;
	sc->sc_phsc.sc_pc.pc_conf_write = rkpcie_conf_write;
	sc->sc_phsc.sc_pc.pc_conf_hook = rkpcie_conf_hook;

	if (bus_scan_delay_ms > delayed_ms) {
		uint32_t ms = bus_scan_delay_ms - delayed_ms;

		aprint_verbose_dev(phsc->sc_dev,
		    "waiting %u extra ms for reset (already waited %u)\n",
		    ms, delayed_ms);
		delay(ms * 1000);
	}

	mutex_init(&sc->sc_conf_lock, MUTEX_DEFAULT, IPL_HIGH);
	pcihost_init2(&sc->sc_phsc);
}

static void
rkpcie_atr_init(struct rkpcie_softc *sc)
{
	const u_int *ranges;
	bus_addr_t aaddr;
	bus_addr_t addr;
	bus_size_t size, resid, offset;
	uint32_t type;
	int region, i, ranges_len;

	/* Use region 0 to map PCI configuration space */
	HWRITE4(sc, PCIE_ATR_OB_ADDR0(0), 20 - 1);
	HWRITE4(sc, PCIE_ATR_OB_ADDR1(0), 0);
	HWRITE4(sc, PCIE_ATR_OB_DESC0(0), PCIE_ATR_HDR_CFG_TYPE0 | PCIE_ATR_HDR_RID);
	HWRITE4(sc, PCIE_ATR_OB_DESC1(0), 0);

	ranges = fdtbus_get_prop(sc->sc_phsc.sc_phandle, "ranges", &ranges_len);
	if (ranges == NULL)
		goto fail;
	const int ranges_cells = ranges_len / 4;

	for (i = 0; i < ranges_cells; i += 7) {
		/* Handle IO and MMIO. */
		switch (be32toh(ranges[i]) & 0x03000000) {
		case 0x01000000:
			type = PCIE_ATR_HDR_IO;
			break;
		case 0x02000000:
		case 0x03000000:
			type = PCIE_ATR_HDR_MEM;
			break;
		default:
			continue;
		}

		addr = ((uint64_t)be32toh(ranges[i + 1]) << 32) + be32toh(ranges[i + 2]);
		aaddr = ((uint64_t)be32toh(ranges[i + 3]) << 32) + be32toh(ranges[i + 4]);
		size = be32toh(ranges[i + 6]);

		/* Only support mappings aligned on a region boundary. */
		if (addr & (PCIE_ATR_OB_REGION_SIZE - 1))
			goto fail;
		if (aaddr & (PCIE_ATR_OB_REGION_SIZE - 1))
			goto fail;
		if (size & (PCIE_ATR_OB_REGION_SIZE - 1))
			goto fail;

		/* Mappings should lie in AXI region. */
		if (aaddr < sc->sc_axi_addr)
			goto fail;
		if (aaddr + size > sc->sc_axi_addr + 64*1024*1024)
			goto fail;

		offset = addr - sc->sc_axi_addr - PCIE_ATR_OB_REGION0_SIZE;
		region = 1 + (offset / PCIE_ATR_OB_REGION_SIZE);
		resid = size;
		while (resid > 0) {
			HWRITE4(sc, PCIE_ATR_OB_ADDR0(region), 32 - 1);
			HWRITE4(sc, PCIE_ATR_OB_ADDR1(region), 0);
			HWRITE4(sc, PCIE_ATR_OB_DESC0(region), type | PCIE_ATR_HDR_RID);
			HWRITE4(sc, PCIE_ATR_OB_DESC1(region), 0);

			addr += PCIE_ATR_OB_REGION_SIZE;
			resid -= PCIE_ATR_OB_REGION_SIZE;
			region++;
		}
	}

	/* Passthrought inbound translations unmodified. */
	HWRITE4(sc, PCIE_ATR_IB_ADDR0(2), 32 - 1);
	HWRITE4(sc, PCIE_ATR_IB_ADDR1(2), 0);

	return;

fail:
	device_printf(sc->sc_phsc.sc_dev, "can't map ranges\n");
}

int
rkpcie_bus_maxdevs(void *v, int bus)
{
	struct rkpcie_softc *rksc = v;
	struct pcihost_softc *sc = &rksc->sc_phsc;

	if (bus == sc->sc_bus_min || bus == sc->sc_bus_min + 1)
		return 1;
	return 32;
}

pcitag_t
rkpcie_make_tag(void *v, int bus, int device, int function)
{
	/* Return ECAM address. */
	return ((bus << 20) | (device << 15) | (function << 12));
}

void
rkpcie_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp != NULL)
		*bp = (tag >> 20) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 15) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 12) & 0x7;
}

/* Only one device on root port and the first subordinate port. */
static bool
rkpcie_conf_ok(int bus, int dev, int fn, int offset, struct rkpcie_softc *sc)
{
	int bus_min = sc->sc_phsc.sc_bus_min;

	if ((unsigned int)offset >= (1<<12))
		return false;
	/* first two buses use type 0 cfg which doesn't use bus/device numbers */
	if (dev != 0 && (bus == bus_min || bus == bus_min + 1))
		return false;
	return true;
}

pcireg_t
rkpcie_conf_read(void *v, pcitag_t tag, int offset)
{
	struct rkpcie_softc *sc = v;
	int bus_min = sc->sc_phsc.sc_bus_min;
	int bus, dev, fn;
	u_int reg;
	int32_t val;

	KASSERT(offset >= 0);
	KASSERT(offset < PCI_EXTCONF_SIZE);

	rkpcie_decompose_tag(sc, tag, &bus, &dev, &fn);
	if (!rkpcie_conf_ok(bus, dev, fn, offset, sc))
		return 0xffffffff;
	reg = (dev << 15) | (fn << 12) | offset;

	if (bus == bus_min)
		val = HREAD4(sc, PCIE_RC_NORMAL_BASE + reg);
	else {
		mutex_spin_enter(&sc->sc_conf_lock);
		HWRITE4(sc, PCIE_ATR_OB_ADDR0(0),
		    (bus << 20) | (20 - 1));
		HWRITE4(sc, PCIE_ATR_OB_DESC0(0),
		    PCIE_ATR_HDR_RID | ((bus == bus_min + 1)
		    ? PCIE_ATR_HDR_CFG_TYPE0 : PCIE_ATR_HDR_CFG_TYPE1));
		bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0, sc->sc_apb_size,
		      BUS_SPACE_BARRIER_READ);
		if (AXIPEEK4(sc, reg, &val) != 0)
			val = 0xffffffff;
		bus_space_barrier(sc->sc_iot, sc->sc_axi_ioh,
		    0, sc->sc_axi_size,
		    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);
		mutex_spin_exit(&sc->sc_conf_lock);
	}
	return val;
}

void
rkpcie_conf_write(void *v, pcitag_t tag, int offset, pcireg_t data)
{
	struct rkpcie_softc *sc = v;
	int bus_min = sc->sc_phsc.sc_bus_min;
	int bus, dev, fn;
	u_int reg;

	KASSERT(offset >= 0);
	KASSERT(offset < PCI_EXTCONF_SIZE);

	rkpcie_decompose_tag(sc, tag, &bus, &dev, &fn);
	if (!rkpcie_conf_ok(bus, dev, fn, offset, sc))
		return;
	reg = (dev << 15) | (fn << 12) | offset;

	if (bus == bus_min)
		HWRITE4(sc, PCIE_RC_NORMAL_BASE + reg, data);
	else {
		mutex_spin_enter(&sc->sc_conf_lock);
		HWRITE4(sc, PCIE_ATR_OB_ADDR0(0),
		    (bus << 20) | (20 - 1));
		HWRITE4(sc, PCIE_ATR_OB_DESC0(0),
		    PCIE_ATR_HDR_RID | ((bus == bus_min + 1)
		    ? PCIE_ATR_HDR_CFG_TYPE0 : PCIE_ATR_HDR_CFG_TYPE1));
		bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0, sc->sc_apb_size,
		    BUS_SPACE_BARRIER_WRITE);
		AXIPOKE4(sc, reg, data);
		bus_space_barrier(sc->sc_iot, sc->sc_axi_ioh,
		    0, sc->sc_axi_size,
		    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);
		mutex_spin_exit(&sc->sc_conf_lock);
	}
}

static int
rkpcie_conf_hook(void *v, int b, int d, int f, pcireg_t id)
{
        return (PCI_CONF_DEFAULT & ~PCI_CONF_ENABLE_BM) | PCI_CONF_MAP_ROM;
}

/* INTx interrupt controller */
static void *
rkpcie_intx_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct rkpcie_softc *sc = device_private(dev);
	void *cookie;

#if notyet
	const u_int pin = be32toh(specifier[0]);
#endif

	/* Unmask legacy interrupts. */
	HWRITE4(sc, PCIE_CLIENT_INT_MASK,
	    PCIM_INTx_ENAB(0) | PCIM_INTx_ENAB(1) |
	    PCIM_INTx_ENAB(2) | PCIM_INTx_ENAB(3));

	cookie = fdtbus_intr_establish_byname(sc->sc_phsc.sc_phandle,
	    "legacy", ipl, flags, func, arg, xname);

	return cookie;
}

static void
rkpcie_intx_disestablish(device_t dev, void *ih)
{
	struct rkpcie_softc *sc = device_private(dev);
	device_printf(dev, "%s\n", __func__);
	fdtbus_intr_disestablish(sc->sc_phsc.sc_phandle, ih);
}

static bool
rkpcie_intx_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	struct rkpcie_softc *sc = device_private(dev);

	fdtbus_intr_str(sc->sc_phsc.sc_phandle, 1, buf, buflen);

	return true;
}

static struct fdtbus_interrupt_controller_func rkpcie_intrfuncs = {
	.establish = rkpcie_intx_establish,
	.disestablish = rkpcie_intx_disestablish,
	.intrstr = rkpcie_intx_intrstr,
};
