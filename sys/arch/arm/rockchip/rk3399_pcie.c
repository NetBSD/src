/* $NetBSD: rk3399_pcie.c,v 1.1 2019/03/07 00:35:22 jakllsch Exp $ */
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

__KERNEL_RCSID(1, "$NetBSD: rk3399_pcie.c,v 1.1 2019/03/07 00:35:22 jakllsch Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bitops.h>
#include <sys/device.h>
#include <sys/extent.h>
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
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

struct rkpcie_softc {
	struct pcihost_softc	sc_phsc;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_space_handle_t	sc_bus_cfgh[32];
	bus_addr_t		sc_axi_addr;
	bus_addr_t		sc_apb_addr;
	bus_size_t		sc_axi_size;
	bus_size_t		sc_apb_size;

	struct extent		*sc_regionex;
};

static int rkpcie_match(device_t, cfdata_t, void *);
static void rkpcie_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rkpcie, sizeof(struct rkpcie_softc),
        rkpcie_match, rkpcie_attach, NULL, NULL);

static int
rkpcie_match(device_t parent, cfdata_t cf, void *aux)
{
        const char * const compatible[] = {
		"rockchip,rk3399-pcie",
		NULL
	};
	struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void	rkpcie_atr_init(struct rkpcie_softc *);

static int	rkpcie_bus_maxdevs(void *, int);
static pcitag_t rkpcie_make_tag(void *, int, int, int);
static void	rkpcie_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t rkpcie_conf_read(void *, pcitag_t, int);
static void	rkpcie_conf_write(void *, pcitag_t, int, pcireg_t);
static int	rkpcie_conf_hook(void *, int, int, int, pcireg_t);

static struct fdtbus_interrupt_controller_func rkpcie_intrfuncs;

static inline int
OF_getpropintarray(int handle, const char *prop, uint32_t *buf, int buflen)
{
	int len;
	int i;

	len = OF_getprop(handle, prop, buf, buflen);
	if (len < 0 || (len % sizeof(uint32_t)))
		return -1;

	for (i = 0; i < len / sizeof(uint32_t); i++)
		buf[i] = be32toh(buf[i]);

	return len;
}

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

static inline void
clock_enable(int phandle, const char *name)
{
	struct clk * clk = fdtbus_clock_get(phandle, name);
	if (clk == NULL)
		return;
	if (clk_enable(clk) != 0)
		return;
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
	//struct pcibus_attach_args pba;
	struct fdtbus_gpio_pin *ep_gpio;
	uint32_t bus_range[2];
	uint32_t status;
	bool retry = false;
	int timo;

	phsc->sc_dev = self;
	phsc->sc_bst = faa->faa_bst;
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

	if (bus_space_map(sc->sc_iot, sc->sc_apb_addr,
	    sc->sc_apb_size, 0, &sc->sc_ioh)) {
		printf(": can't map registers\n");
		sc->sc_axi_size = 0;
		sc->sc_apb_size = 0;
		return;
	}

	aprint_naive("\n");
	aprint_normal(": RK3399 PCIe\n");

	struct fdtbus_regulator *regulator;
	regulator = fdtbus_regulator_acquire(phandle, "vpcie3v3-supply");
	fdtbus_regulator_enable(regulator);
	fdtbus_regulator_release(regulator);
		
	fdtbus_clock_assign(phandle);
	clock_enable_all(phandle);

	ep_gpio = fdtbus_gpio_acquire(phandle, "ep-gpios", GPIO_PIN_OUTPUT);
	//retry = true;
again:
	fdtbus_gpio_write(ep_gpio, 0);

	reset_assert(phandle, "aclk");
	reset_assert(phandle, "pclk");
	reset_assert(phandle, "pm");

	//device_printf(self, "%s phy0\n", __func__);
	struct fdtbus_phy *phy[4];
	memset(phy, 0, sizeof(phy));
	phy[0] = fdtbus_phy_get(phandle, "pcie-phy-0");
	//device_printf(self, "%s phy1 %p\n", __func__, phy[0]);
	if (phy[0] == NULL) {
		phy[0] = fdtbus_phy_get(phandle, "pcie-phy");
		device_printf(self, "%s phy2 %p\n", __func__, phy);
	} else {
		/* XXX */
		phy[1] = fdtbus_phy_get(phandle, "pcie-phy-1");
		phy[2] = fdtbus_phy_get(phandle, "pcie-phy-2");
		phy[3] = fdtbus_phy_get(phandle, "pcie-phy-3");
	}

	reset_assert(phandle, "core");
	reset_assert(phandle, "mgmt");
	reset_assert(phandle, "mgmt-sticky");
	reset_assert(phandle, "pipe");

	delay(10);
	
	reset_deassert(phandle, "pm");
	reset_deassert(phandle, "aclk");
	reset_deassert(phandle, "pclk");

	if (retry)
		HWRITE4(sc, PCIE_CLIENT_BASIC_STRAP_CONF, PCBSC_PGS_GEN1);
	else
		HWRITE4(sc, PCIE_CLIENT_BASIC_STRAP_CONF, PCBSC_PGS_GEN2);

	/* Switch into Root Complex mode. */
	HWRITE4(sc, PCIE_CLIENT_BASIC_STRAP_CONF,
	    PCBSC_MS_ROOTPORT | PCBSC_CONF_EN | PCBSC_LC(4));
	//printf("%s PCBSC %x\n", __func__, HREAD4(sc, PCIE_CLIENT_BASIC_STRAP_CONF));

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

	/* FTS count */
	HWRITE4(sc, PCIE_LM_PLC1, HREAD4(sc, PCIE_LM_PLC1) | PCIE_LM_PLC1_FTS_MASK);

	/* XXX Advertise power limits? */

	/* common clock */
	HWRITE4(sc, PCIE_RC_CONFIG_LCSR, HREAD4(sc, PCIE_RC_CONFIG_LCSR) | PCIE_LCSR_COMCLKCFG);
	/* 128 RCB */
	HWRITE4(sc, PCIE_RC_CONFIG_LCSR, HREAD4(sc, PCIE_RC_CONFIG_LCSR) | PCIE_LCSR_RCB);

	/* Start link training. */
	HWRITE4(sc, PCIE_CLIENT_BASIC_STRAP_CONF, PCBSC_LINK_TRAIN_EN);

	fdtbus_gpio_write(ep_gpio, 1);

	for (timo = 500; timo > 0; timo--) {
		status = HREAD4(sc, PCIE_CLIENT_BASIC_STATUS1);
		if (PCBS1_LINK_ST(status) == PCBS1_LS_DL_DONE)
			break;
		delay(1000);
	}
	if (timo == 0) {
		device_printf(self, "link training timeout (link_st %u)\n",
		    PCBS1_LINK_ST(status));
		if (!retry) {
			retry = true;
			goto again;
		}
		return;
	}

	if (!retry) {
		HWRITE4(sc, PCIE_RC_CONFIG_LCSR, HREAD4(sc, PCIE_RC_CONFIG_LCSR) | PCIE_LCSR_RETRAIN);
		for (timo = 500; timo > 0; timo--) {
			status = HREAD4(sc, PCIE_LM_CORE_CTRL);
			if ((status & PCIE_CORE_PL_CONF_SPEED_MASK) == PCIE_CORE_PL_CONF_SPEED_5G)
				break;
			delay(1000);
		}
		if (timo == 0) {
			device_printf(self, "Gen2 link training timeout\n");
			retry = true;
			goto again;
		}
	}

#if 0
	printf("%s CBS0 %x\n", __func__, HREAD4(sc, PCIE_CLIENT_BASIC_STATUS1));
	HWRITE4(sc, PCIE_LM_DEBUG_MUX_CONTROL, (HREAD4(sc, PCIE_LM_DEBUG_MUX_CONTROL) & ~0xf) | 0);
	printf("%s CDO0 %x\n", __func__, HREAD4(sc, PCIE_CLIENT_DEBUG_OUT_0));
	HWRITE4(sc, PCIE_LM_DEBUG_MUX_CONTROL, (HREAD4(sc, PCIE_LM_DEBUG_MUX_CONTROL) & ~0xf) | 1);
	printf("%s CDO0 %x\n", __func__, HREAD4(sc, PCIE_CLIENT_DEBUG_OUT_0));
	HWRITE4(sc, PCIE_LM_DEBUG_MUX_CONTROL, (HREAD4(sc, PCIE_LM_DEBUG_MUX_CONTROL) & ~0xf) | 4);
	printf("%s CDO0 %x\n", __func__, HREAD4(sc, PCIE_CLIENT_DEBUG_OUT_0));
	HWRITE4(sc, PCIE_LM_DEBUG_MUX_CONTROL, (HREAD4(sc, PCIE_LM_DEBUG_MUX_CONTROL) & ~0xf) | 5);
	printf("%s CDO0 %x\n", __func__, HREAD4(sc, PCIE_CLIENT_DEBUG_OUT_0));
	printf("%s LINKWIDTH %x\n", __func__, HREAD4(sc, PCIE_LM_LINKWIDTH));
	//HWRITE4(sc, PCIE_LM_LINKWIDTH, 0x1000f);
	//printf("%s LINKWIDTH %x\n", __func__, HREAD4(sc, PCIE_LM_LINKWIDTH));
	printf("%s LANEMAP %x\n", __func__, HREAD4(sc, PCIE_LM_LANEMAP));
#endif

	fdtbus_gpio_release(ep_gpio);

	HWRITE4(sc, PCIE_RC_BASE + PCI_CLASS_REG,
	    PCI_CLASS_BRIDGE << PCI_CLASS_SHIFT |
	    PCI_SUBCLASS_BRIDGE_PCI << PCI_SUBCLASS_SHIFT);

	/* Initialize Root Complex registers. */
	HWRITE4(sc, PCIE_LM_VENDOR_ID, PCI_VENDOR_ROCKCHIP);
	HWRITE4(sc, PCIE_RC_BASE + PCI_CLASS_REG,
	    PCI_CLASS_BRIDGE << PCI_CLASS_SHIFT |
	    PCI_SUBCLASS_BRIDGE_PCI << PCI_SUBCLASS_SHIFT);
	HWRITE4(sc, PCIE_LM_RCBAR, PCIE_LM_RCBARPIE | PCIE_LM_RCBARPIS | PCIE_LM_RCBARPME | PCIE_LM_RCBARPMS);

	/* remove L1 substate cap */
	status = HREAD4(sc, PCIE_RC_CONFIG_THP_CAP);
	status &= ~PCIE_RC_CONFIG_THP_CAP_NEXT_MASK;
	HWRITE4(sc, PCIE_RC_CONFIG_THP_CAP, status);

	if (OF_getproplen(phandle, "aspm-no-l0s") == 0) {
		status = HREAD4(sc, PCIE_RC_PCIE_LCAP);
		status &= ~__SHIFTIN(1, PCIE_LCAP_ASPM);
		HWRITE4(sc, PCIE_RC_PCIE_LCAP, status);
	}

	status = HREAD4(sc, PCIE_RC_CONFIG_DCSR);
	status &= ~PCIE_DCSR_MAX_PAYLOAD;
	status |= __SHIFTIN(1, PCIE_DCSR_MAX_PAYLOAD);
	HWRITE4(sc, PCIE_RC_CONFIG_DCSR, status);

	/* Create extents for our address space. */
	sc->sc_regionex = extent_create("rkpcie", sc->sc_axi_addr,
	    sc->sc_axi_addr - 1 + 64 * 1048576, NULL, 0, EX_WAITOK);

	/* Set up bus range. */
	if (OF_getpropintarray(phandle, "bus-range", bus_range,
	    sizeof(bus_range)) != sizeof(bus_range) ||
	    bus_range[0] >= 32 || bus_range[1] >= 32) {
		bus_range[0] = 0;
		bus_range[1] = 31;
	}
	sc->sc_phsc.sc_bus_min = bus_range[0];
	sc->sc_phsc.sc_bus_max = bus_range[1];

	if (sc->sc_phsc.sc_bus_min != 0) {
		aprint_error_dev(self, "bus-range doesn't start at 0\n");
		return;
	}

	/* Configure Address Translation. */
	rkpcie_atr_init(sc);

	fdtbus_register_interrupt_controller(self, OF_child(sc->sc_phsc.sc_phandle),
	            &rkpcie_intrfuncs);

	sc->sc_phsc.sc_type = PCIHOST_ECAM;
	pcihost_init(&sc->sc_phsc.sc_pc, sc);
	sc->sc_phsc.sc_pc.pc_bus_maxdevs = rkpcie_bus_maxdevs;
	sc->sc_phsc.sc_pc.pc_make_tag = rkpcie_make_tag;
	sc->sc_phsc.sc_pc.pc_decompose_tag = rkpcie_decompose_tag;
	sc->sc_phsc.sc_pc.pc_conf_read = rkpcie_conf_read;
	sc->sc_phsc.sc_pc.pc_conf_write = rkpcie_conf_write;
	sc->sc_phsc.sc_pc.pc_conf_hook = rkpcie_conf_hook;
	pcihost_init2(&sc->sc_phsc);
}

static void
rkpcie_atr_init(struct rkpcie_softc *sc)
{
	uint32_t *ranges = NULL;
	struct extent * const ex = sc->sc_regionex;
	bus_addr_t aaddr;
	bus_addr_t addr;
	bus_size_t size, offset;
	uint32_t type;
	int len, region;
	int i;

	/* get root bus's config space out of the APB space */
	bus_space_subregion(sc->sc_iot, sc->sc_ioh, PCIE_RC_NORMAL_BASE, PCI_EXTCONF_SIZE * 8, &sc->sc_bus_cfgh[0]);

	len = OF_getproplen(sc->sc_phsc.sc_phandle, "ranges");
	if (len <= 0 || (len % (7 * sizeof(uint32_t))) != 0)
		goto fail;
	ranges = kmem_zalloc(len, KM_SLEEP);
	OF_getpropintarray(sc->sc_phsc.sc_phandle, "ranges", ranges, len);

	for (i = 0; i < len / sizeof(uint32_t); i += 7) {
		/* Handle IO and MMIO. */
		switch (ranges[i] & 0x03000000) {
		case 0x00000000:
			type = PCIE_ATR_HDR_CFG_TYPE0;
			break;
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

		addr = ((uint64_t)ranges[i + 1] << 32) + ranges[i + 2];
		aaddr = ((uint64_t)ranges[i + 3] << 32) + ranges[i + 4];
		size = (uint64_t)ranges[i+5] << 32 | ranges[i + 6];

		if (type == PCIE_ATR_HDR_CFG_TYPE0) {
			addr = __SHIFTOUT(ranges[i], PHYS_HI_BUS) << 20;
		}

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
		
		while (size > 0) {
			offset = aaddr - sc->sc_axi_addr;
			region = (offset / PCIE_ATR_OB_REGION_SIZE);
			if (region >= 0x20)
				region -= 0x1f;
			if (region > 32)
				continue;
			u_long regionsize = region ?
			    PCIE_ATR_OB_REGION_SIZE : PCIE_ATR_OB_REGION0_SIZE;
			uint32_t regionbits = ilog2(regionsize);

			//printf("%s %lx %lx %lx\n", __func__, addr, aaddr, regionsize);
			if (extent_alloc_region(ex, aaddr, regionsize, EX_WAITOK) != 0)
				goto fail;
			if (type == PCIE_ATR_HDR_CFG_TYPE0) {
				const uint32_t bus = (addr >> 20) & 0xff;
				if (bus == 0 ||
				    bus >= __arraycount(sc->sc_bus_cfgh))
					continue;
				bus_space_map(sc->sc_iot, aaddr, regionsize, 0, &sc->sc_bus_cfgh[bus]);
				if (bus > 1)
					type = PCIE_ATR_HDR_CFG_TYPE1;
			}
			HWRITE4(sc, PCIE_ATR_OB_ADDR0(region),
			    addr | (regionbits-1));
			HWRITE4(sc, PCIE_ATR_OB_ADDR1(region), addr >> 32);
			HWRITE4(sc, PCIE_ATR_OB_DESC0(region),
			    type | PCIE_ATR_HDR_RID);
			HWRITE4(sc, PCIE_ATR_OB_DESC1(region), 0);
			
			aaddr += regionsize;
			addr += regionsize;
			size -= regionsize;
		}
	}
	kmem_free(ranges, len);

	/* Passthrought inbound translations unmodified. */
	HWRITE4(sc, PCIE_ATR_IB_ADDR0(2), 32 - 1);
	HWRITE4(sc, PCIE_ATR_IB_ADDR1(2), 0);

	return;

fail:
	extent_print(ex);
	device_printf(sc->sc_phsc.sc_dev, "can't map ranges\n");
	kmem_free(ranges, len);
}

int
rkpcie_bus_maxdevs(void *v, int bus)
{
	struct rkpcie_softc *rksc = v;
	struct pcihost_softc *sc = &rksc->sc_phsc;

	if (bus == sc->sc_bus_min)
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

pcireg_t
rkpcie_conf_read(void *v, pcitag_t tag, int reg)
{
	struct rkpcie_softc *sc = v;
	struct pcihost_softc *phsc = &sc->sc_phsc;
	int bus, dev, fn;
	bus_size_t offset;
	uint32_t data;

	KASSERT(reg >= 0);
	KASSERT(reg < PCI_EXTCONF_SIZE);

	rkpcie_decompose_tag(sc, tag, &bus, &dev, &fn);
	if (bus > phsc->sc_bus_max)
		return 0xffffffff;
	if (bus == phsc->sc_bus_min + 1 && dev > 0)
		return 0xffffffff;
	offset = dev << 15 | fn << 12 | reg;
	if (bus_space_peek_4(sc->sc_iot, sc->sc_bus_cfgh[bus], offset, &data) == 0)
		return data;
	
	return 0xffffffff;
}

void
rkpcie_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{
	struct rkpcie_softc *sc = v;
	struct pcihost_softc *phsc = &sc->sc_phsc;
	int bus, dev, fn;
	bus_size_t offset;

	KASSERT(reg >= 0);
	KASSERT(reg < PCI_EXTCONF_SIZE);

	rkpcie_decompose_tag(sc, tag, &bus, &dev, &fn);
	if (bus > phsc->sc_bus_max)
		return;
	if (bus == phsc->sc_bus_min + 1 && dev > 0)
		return;
	offset = dev << 15 | fn << 12 | reg;
	bus_space_poke_4(sc->sc_iot, sc->sc_bus_cfgh[bus], offset, data);
}

static int
rkpcie_conf_hook(void *v, int b, int d, int f, pcireg_t id)
{
        return (PCI_CONF_DEFAULT & ~PCI_CONF_ENABLE_BM) | PCI_CONF_MAP_ROM;
}

/* INTx interrupt controller */
static void *
rkpcie_intx_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg)
{
	struct rkpcie_softc *sc = device_private(dev);
	void *cookie;

	const u_int pin = be32toh(specifier[0]);
	device_printf(sc->sc_phsc.sc_dev, "%s pin %u\n", __func__, pin);

	/* Unmask legacy interrupts. */
	HWRITE4(sc, PCIE_CLIENT_INT_MASK,
	    PCIM_INTx_ENAB(0) | PCIM_INTx_ENAB(1) |
	    PCIM_INTx_ENAB(2) | PCIM_INTx_ENAB(3));

	cookie = fdtbus_intr_establish_byname(sc->sc_phsc.sc_phandle, "legacy", ipl, flags, func, arg);

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
