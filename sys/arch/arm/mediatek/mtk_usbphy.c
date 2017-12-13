/*-
 * Copyright (c) 2017 Mediatek Inc.
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


#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/fdt/fdtvar.h>

/* #define MTKUSBPHY_DEBUG  1 */

#ifdef MTKUSBPHY_DEBUG
#define USBPHY_DBG(msg, ...)  aprint_normal(msg, ## __VA_ARGS__)
#else
#define USBPHY_DBG(msg, ...)
#endif

#define BITS_PER_BYTE		8
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

#ifdef CONFIG_64BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif /* CONFIG_64BIT */

#ifndef BITS_PER_LONG_LONG
#define BITS_PER_LONG_LONG 64
#endif

/*
 * Rounding to nearest.
 */
#define DIV_ROUND_CLOSEST(N, D)                                         \
        ((0 < (N)) ? (((N) + ((D) / 2)) / (D))                          \
            : (((N) - ((D) / 2)) / (D)))

/*
 * Rounding to what may or may not be powers of two.
 */
#define DIV_ROUND_UP(X, N)      (((X) + (N) - 1) / (N))
#define DIV_ROUND_UP_ULL(X, N)  DIV_ROUND_UP((unsigned long long)(X), (N))

/*
 * Create a contiguous bitmask starting at bit position @l and ending at
 * position @h. For example
 * GENMASK_ULL(39, 21) gives us the 64bit vector 0x000000ffffe00000.
 */
#define GENMASK(h, l) \
	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define GENMASK_ULL(h, l) \
	(((~0ULL) << (l)) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

#define TOPCKGEN_BASE					0
#define CLK_GATING_CTRL1 				(TOPCKGEN_BASE + 0x024)
#define BV_CLKGATINGCTRL1_SET_USB_SW_CG			(1ul << 13)

#define CLK_GATING_CTRL2 				(TOPCKGEN_BASE + 0x03C)
#define BV_CLKGATINGCTRL2_SET_78M_USB_SW_CG		(1ul << 31)
#define BV_CLKGATINGCTRL2_SET_USBIF_SW_CG		(1ul << 24)
#define BV_CLKGATINGCTRL2_SET_USB_1P_SW_CG		(1ul << 14)

#define CLR_CLK_GATING_CTRL1				(TOPCKGEN_BASE + 0x084)
#define CLR_CLK_GATING_CTRL2				(TOPCKGEN_BASE + 0x09C)
#define SET_CLK_GATING_CTRL1				(TOPCKGEN_BASE + 0x054)
#define SET_CLK_GATING_CTRL2				(TOPCKGEN_BASE + 0x06C)

#define U3P_USBPHYACR0					0x000
#define PA0_RG_U2PLL_FORCE_ON				(1ul <<15)
#define PA0_RG_USB20_INTR_EN				(1ul << 5)

#define U3P_USBPHYACR2					0x008
#define PA2_RG_SIF_U2PLL_FORCE_EN			(1ul << 18)

#define U3P_USBPHYACR5					0x014
#define PA5_RG_U2_HSTX_SRCAL_EN				(1ul << 15)
#define PA5_RG_U2_HSTX_SRCTRL				GENMASK(14, 12)
#define PA5_RG_U2_HSTX_SRCTRL_VAL(x)			((0x7 & (x)) << 12)
#define PA5_RG_U2_HS_100U_U3_EN				(1ul << 11)

#define U3P_USBPHYACR6					0x018
#define PA6_RG_U2_BC11_SW_EN				(1ul << 23)
#define PA6_RG_U2_OTG_VBUSCMP_EN			(1ul << 20)
#define PA6_RG_U2_SQTH					GENMASK(3, 0)
#define PA6_RG_U2_SQTH_VAL(x)				(0xf & (x))

#define U3P_U2PHYACR4					0x020
#define P2C_RG_USB20_GPIO_CTL				(1ul << 9)
#define P2C_USB20_GPIO_MODE				(1ul << 8)
#define P2C_U2_GPIO_CTR_MSK				(P2C_RG_USB20_GPIO_CTL | P2C_USB20_GPIO_MODE)

#define U3D_U2PHYDCR0					0x060
#define P2C_RG_SIF_U2PLL_FORCE_ON			(1ul << 24)

#define U3P_U2PHYDTM0					0x068
#define P2C_FORCE_UART_EN				(1ul << 26)
#define P2C_FORCE_DATAIN				(1ul << 23)
#define P2C_FORCE_DM_PULLDOWN				(1ul << 21)
#define P2C_FORCE_DP_PULLDOWN				(1ul << 20)
#define P2C_FORCE_XCVRSEL				(1ul << 19)
#define P2C_FORCE_SUSPENDM				(1ul << 18)
#define P2C_FORCE_TERMSEL				(1ul << 17)
#define P2C_RG_DATAIN					GENMASK(13, 10)
#define P2C_RG_DATAIN_VAL(x)				((0xf & (x)) << 10)
#define P2C_RG_DMPULLDOWN				(1ul << 7)
#define P2C_RG_DPPULLDOWN				(1ul << 6)
#define P2C_RG_XCVRSEL					GENMASK(5, 4)
#define P2C_RG_XCVRSEL_VAL(x)				((0x3 & (x)) << 4)
#define P2C_RG_SUSPENDM					(1ul << 3)
#define P2C_RG_TERMSEL					(1ul << 2)

#define P2C_DTM0_PART_MASK				(P2C_FORCE_DATAIN | P2C_FORCE_DM_PULLDOWN | \
								P2C_FORCE_DP_PULLDOWN | P2C_FORCE_XCVRSEL | \
								P2C_FORCE_TERMSEL | P2C_RG_DMPULLDOWN | \
								P2C_RG_DPPULLDOWN | P2C_RG_TERMSEL)

#define U3P_U2PHYDTM1					0x06C
#define P2C_RG_UART_EN					(1ul << 16)
#define P2C_FORCE_LINESTATE				(1ul << 14)
#define P2C_FORCE_VBUSVALID				(1ul << 13)
#define P2C_FORCE_SESSEND				(1ul << 12)
#define P2C_FORCE_BVALID				(1ul << 11)
#define P2C_FORCE_AVALID				(1ul << 10)
#define P2C_FORCE_IDDIG					(1ul << 9)
#define P2C_FORCE_IDPULLUP				(1ul << 8)
#define P2C_RG_LINESTATE				GENMASK(7, 6)
#define P2C_RG_VBUSVALID				(1ul << 5)
#define P2C_RG_SESSEND					(1ul << 4)
#define P2C_RG_BVALID					(1ul << 3)
#define P2C_RG_AVALID					(1ul << 2)
#define P2C_RG_IDDIG					(1ul << 1)
#define P2C_RG_IDPULLUP					(1ul << 0)

#define U3P_U2FREQ_FMCR0				0x700
#define P2F_RG_MONCLK_SEL				GENMASK(27, 26)
#define P2F_RG_MONCLK_SEL_VAL(x)			((0x3 & (x)) << 26)
#define P2F_RG_FREQDET_EN				(1ul << 24)
#define P2F_RG_CYCLECNT					GENMASK(23, 0)
#define P2F_RG_CYCLECNT_VAL(x)				((P2F_RG_CYCLECNT) & (x))

#define U3P_U2FREQ_VALUE				0x70c

#define U3P_U2FREQ_FMMONR1				0x710
#define P2F_USB_FM_VALID				(1ul << 0)
#define P2F_RG_FRCK_EN					(1ul << 8)

#define U3P_REF_CLK					26	/* MHZ */
#define U3P_SLEW_RATE_COEF				28
#define U3P_SR_COEF_DIVISOR				1000
#define U3P_FM_DET_CYCLE_CNT				1024


enum phy_mode {
	PHY_MODE_INVALID,
	PHY_MODE_USB_HOST,
	PHY_MODE_USB_DEVICE,
	PHY_MODE_USB_OTG
	};

enum mtk_phy_version {
	MTK_PHY_V1 = 1,
	MTK_PHY_V2,
};

static const struct of_compat_data mtk_phy_compat_data[] = {
	{ "mediatek,mt8516-usb20-phy",	MTK_PHY_V2 },
	{ NULL }
};

#define	MTK_MAXUSBPHY		2

struct mtk_usbphy {
	u_int					 phy_index;
	bus_addr_t				 phy_addr;
	bus_size_t 				 phy_size;
	bus_space_tag_t			 phy_bst;
	bus_space_handle_t		 phy_bsh;
	struct fdtbus_regulator *phy_reg;
};

struct mtk_usbphy_softc {
	device_t				sc_dev;
	bus_space_tag_t			sc_bst;
	bus_space_handle_t		sc_bsh;
	enum mtk_phy_version	sc_type;
	enum phy_mode			sc_mode;
	struct mtk_usbphy		sc_phys[MTK_MAXUSBPHY];
	u_int					sc_nphys;

	struct fdtbus_gpio_pin	*sc_gpio_id_det;
	struct fdtbus_gpio_pin	*sc_gpio_vbus_det;
};

/**
 * readx_poll_timeout - Periodically poll an address until a condition is met or a timeout occurs
 * @op: accessor function (takes @addr as its only argument)
 * @addr: Address to poll
 * @val: Variable to read the value into
 * @cond: Break condition (usually involving @val)
 * @sleep_us: Maximum time to sleep between reads in us (0
 *            tight-loops).  Should be less than ~20ms since usleep_range
 *            is used (see Documentation/timers/timers-howto.txt).
 * @timeout_us: Timeout in us, 0 means never timeout
 *
 * Returns 0 on success and -ETIMEDOUT upon a timeout. In either
 * case, the last read value at @addr is stored in @val. Must not
 * be called from atomic context if sleep_us or timeout_us are used.
 *
 * When available, you'll probably want to use one of the specialized
 * macros defined below rather than this macro directly.
 */
#define readx_poll_timeout(op, sc, offest, val, cond, sleep_us, timeout_us) \
({ \
	struct timeval start, gap, now, end; \
	microtime(&start); \
	gap.tv_sec = timeout_us / 1000000; \
	gap.tv_usec= timeout_us % 1000000; \
	timeradd(&start, &gap, &end); \
	for (;;) { \
		(val) = op((sc), (offest)); \
		if (cond) \
			break; \
		microtime(&now); \
		if (timeout_us && timercmp(&now, &end, <)) { \
			(val) = op((sc), (offest)); \
			break; \
		} \
		if (sleep_us) \
			delay((sleep_us >> 2) + 1); \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
})

#define UBARR(phy) bus_space_barrier((phy)->phy_bst, (phy)->phy_bsh, 0, phy->phy_size, \
			BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE)

static __inline uint32_t
USBPHY_READ32(const struct mtk_usbphy *phy, bus_size_t offset)
{
	UBARR(phy);
	return bus_space_read_4((phy)->phy_bst, (phy)->phy_bsh, (offset));
}

#define USBPHY_WRITE32(phy, offset, val) \
	do {	UBARR(phy);\
		bus_space_write_4((phy)->phy_bst, (phy)->phy_bsh, (offset), (val));\
		USBPHY_DBG("USBPHY%i[0x%lX] = 0x%0x but 0x%0X\n", phy->phy_index,\
					(((phy)->phy_addr) + (offset)), val,\
							USBPHY_READ32(phy, offset));\
	} while (/*CONSTCOND*/0)

static int  mtk_usbphy_match(device_t, cfdata_t, void *);
static void mtk_usbphy_attach(device_t, device_t, void *);

static void u2_phy_instance_init(struct mtk_usbphy_softc *sc, const struct mtk_usbphy *phy);
static void u2_phy_instance_power_on(struct mtk_usbphy_softc *sc, const struct mtk_usbphy *phy);
static void hs_slew_rate_calibrate(struct mtk_usbphy_softc *sc, const struct mtk_usbphy *phy);
static void phy_instance_set_mode(struct mtk_usbphy_softc *sc, const struct mtk_usbphy *phy, enum phy_mode mode);
#if 0
static void u2_phy_instance_power_off(struct mtk_usbphy_softc *sc, const struct mtk_usbphy *phy);
#endif

CFATTACH_DECL_NEW(mtk_usbphy, sizeof(struct mtk_usbphy_softc),
	mtk_usbphy_match, mtk_usbphy_attach, NULL, NULL);


static bool
mtk_usbphy_vbus_detect(struct mtk_usbphy_softc *sc)
{
	if (sc->sc_gpio_vbus_det)
		return fdtbus_gpio_read(sc->sc_gpio_vbus_det);
	return 1;
}

static void *
mtk_usbphy_acquire(device_t dev, const void *data, size_t len)
{
	struct mtk_usbphy_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	const int phy_id = be32dec(data);
	if (phy_id >= sc->sc_nphys)
		return NULL;

	return &sc->sc_phys[phy_id];
}

static void
mtk_usbphy_release(device_t dev, void *priv)
{
}

static int
mtk_usbphy_enable(device_t dev, void *priv, bool enable)
{
	struct mtk_usbphy_softc * const sc = device_private(dev);
	struct mtk_usbphy * const phy = priv;

	if (sc->sc_type == MTK_PHY_V2) {

	}

	if (phy->phy_index > 0) {
		/* Enable passby */
		u2_phy_instance_init(sc, phy);
		u2_phy_instance_power_on(sc, phy);
		hs_slew_rate_calibrate(sc, phy);
		phy_instance_set_mode(sc, phy, PHY_MODE_USB_HOST);
#if 0
		u2_phy_instance_power_off(sc);
#endif
	}

	if (phy->phy_reg == NULL)
		return 0;

	if (enable) {
		/* If an external vbus is detected, do not enable phy 0 */
		if (phy->phy_index == 0 && mtk_usbphy_vbus_detect(sc))
			return 0;
		return fdtbus_regulator_enable(phy->phy_reg);
	} else {
		return fdtbus_regulator_disable(phy->phy_reg);
	}
}

const struct fdtbus_phy_controller_func mtk_usbphy_funcs = {
	.acquire = mtk_usbphy_acquire,
	.release = mtk_usbphy_release,
	.enable = mtk_usbphy_enable,
};


static int
mtk_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	static const char * const fdt_mtkusbphy_compatible[] = { 
		"mediatek,mercury-usb20-phy",
		NULL
		};
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	int match;

	/* Check compatible string */
	match = of_match_compatible(phandle, fdt_mtkusbphy_compatible);

	return match;
}

static void
mtk_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct mtk_usbphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	//struct fdtbus_reset *rst;
	struct mtk_usbphy *phy;
	//struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	//char pname[20];
	//u_int n;

	USBPHY_DBG(": USB PHY\n");
	USBPHY_DBG("mtk_usbphy_attach: faa->faa_name: %s\n", faa->faa_name);
	USBPHY_DBG("mtk_usbphy_attach: faa->faa_bst: %p\n", faa->faa_bst);
	USBPHY_DBG("mtk_usbphy_attach: faa->faa_a4x_bst: %p\n", faa->faa_a4x_bst);
	USBPHY_DBG("mtk_usbphy_attach: faa->faa_dmat: %p\n", faa->faa_dmat);
	USBPHY_DBG("mtk_usbphy_attach: faa->faa_phandle: %i\n", faa->faa_phandle);
	USBPHY_DBG("mtk_usbphy_attach: faa->faa_quiet: %i\n", faa->faa_quiet);

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_type = of_search_compatible(phandle, mtk_phy_compat_data)->data;

	for (sc->sc_nphys = 0; sc->sc_nphys < MTK_MAXUSBPHY; sc->sc_nphys++) {
		if (fdtbus_get_reg(phandle, sc->sc_nphys, &addr, &size) != 0)
			break;
		phy = &sc->sc_phys[sc->sc_nphys];
		phy->phy_index = sc->sc_nphys;
		phy->phy_addr = addr;
		phy->phy_size = size;
		phy->phy_bst  = faa->faa_bst;
		if (bus_space_map(phy->phy_bst, phy->phy_addr, phy->phy_size, 0, &phy->phy_bsh) != 0) {
			aprint_error(": failed to map reg #%d\n", sc->sc_nphys);
			return;
		}
		/* Get optional regulator */
		//snprintf(pname, sizeof(pname), "usb%d_vbus-supply", sc->sc_nphys);
		//phy->phy_reg = fdtbus_regulator_acquire(phandle, pname);
		phy->phy_reg = NULL;

		u2_phy_instance_init(sc, phy);
		u2_phy_instance_power_on(sc, phy);
		hs_slew_rate_calibrate(sc, phy);
		phy_instance_set_mode(sc, phy, PHY_MODE_USB_HOST);
#if 0
		u2_phy_instance_power_off(sc);
#endif

	}
#if 0
	/* Enable clocks */
	for (n = 0; (clk = fdtbus_clock_get_index(phandle, n)) != NULL; n++) {
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock #%d\n", n);
			return;
		}
	}
#endif
	aprint_naive("\n");
	aprint_normal(": USB PHY\n");
	

	fdtbus_register_phy_controller(self, phandle, &mtk_usbphy_funcs);
}

static void
u2_phy_instance_init(struct mtk_usbphy_softc *sc, const struct mtk_usbphy *phy)
{
	uint32_t val = 0;
	uint32_t index = phy->phy_index;


	/* switch to USB function. (system register, force ip into usb mode) */
	val = USBPHY_READ32(phy, U3P_U2PHYDTM0);
	val &= ~(P2C_FORCE_UART_EN);
	val |= P2C_RG_XCVRSEL_VAL(1) | P2C_RG_DATAIN_VAL(0);
	USBPHY_WRITE32(phy, U3P_U2PHYDTM0, val);

	val =  USBPHY_READ32(phy, U3P_U2PHYDTM1);
	val &= ~P2C_RG_UART_EN;
	USBPHY_WRITE32(phy, U3P_U2PHYDTM1, val);

	val =  USBPHY_READ32(phy, U3P_USBPHYACR0);
	val |= PA0_RG_USB20_INTR_EN;
	USBPHY_WRITE32(phy, U3P_USBPHYACR0, val);

	/* disable switch 100uA current to SSUSB */
	val =  USBPHY_READ32(phy, U3P_USBPHYACR5);
	val &= ~PA5_RG_U2_HS_100U_U3_EN;
	USBPHY_WRITE32(phy, U3P_USBPHYACR5, val);

	if (!index) {
		val =  USBPHY_READ32(phy, U3P_U2PHYACR4);
		val &= ~P2C_U2_GPIO_CTR_MSK;
		USBPHY_WRITE32(phy, U3P_U2PHYACR4, val);
	}

	val =  USBPHY_READ32(phy, U3D_U2PHYDCR0);
	val |= P2C_RG_SIF_U2PLL_FORCE_ON;
	USBPHY_WRITE32(phy, U3D_U2PHYDCR0, val);

	val =  USBPHY_READ32(phy, U3P_U2PHYDTM0);
	val |= P2C_RG_SUSPENDM | P2C_FORCE_SUSPENDM;
	USBPHY_WRITE32(phy, U3P_U2PHYDTM0, val);

	val =  USBPHY_READ32(phy, U3P_USBPHYACR6);
	val &= ~PA6_RG_U2_BC11_SW_EN;	/* DP/DM BC1.1 path Disable */
	val &= ~PA6_RG_U2_SQTH;
	val |= PA6_RG_U2_SQTH_VAL(2);
	USBPHY_WRITE32(phy, U3P_USBPHYACR6, val);

	USBPHY_DBG("%s(%d)\n", __func__, index);
}



static void
u2_phy_instance_power_on(struct mtk_usbphy_softc *sc, const struct mtk_usbphy *phy)
{
	uint32_t val = 0;

	/* (force_suspendm=0) (let suspendm=1, enable usb 480MHz pll) */
	val =  USBPHY_READ32(phy, U3P_U2PHYDTM0);
	val &= ~(P2C_FORCE_SUSPENDM | P2C_RG_XCVRSEL);
	val &= ~(P2C_RG_DATAIN | P2C_DTM0_PART_MASK);
	USBPHY_WRITE32(phy, U3P_U2PHYDTM0, val);

#ifdef MTK_MUSB_OTG_ENABLE
	/* OTG Enable */
	val =  USBPHY_READ32(phy, U3P_USBPHYACR6);
	val |= PA6_RG_U2_OTG_VBUSCMP_EN;
	USBPHY_WRITE32(phy, U3P_USBPHYACR6, val);

	val =  USBPHY_READ32(phy, U3P_U2PHYDTM1);
	val |= P2C_RG_VBUSVALID | P2C_RG_AVALID;
	val &= ~P2C_RG_SESSEND;
	USBPHY_WRITE32(phy, U3P_U2PHYDTM1, val);
#else
	val =  USBPHY_READ32(phy, U3P_USBPHYACR6);
	val &= ~(PA6_RG_U2_OTG_VBUSCMP_EN);
	USBPHY_WRITE32(phy, U3P_USBPHYACR6, val);

	val =  USBPHY_READ32(phy, U3P_U2PHYDTM1);
	val &= ~(P2C_RG_IDDIG);
	val |= P2C_FORCE_IDDIG;
	val |= P2C_FORCE_AVALID | P2C_RG_AVALID;
	val |= P2C_FORCE_BVALID | P2C_RG_BVALID;
	val &= ~(P2C_RG_SESSEND);
	val |= P2C_FORCE_SESSEND;
	val |= P2C_FORCE_VBUSVALID | P2C_RG_VBUSVALID;
	USBPHY_WRITE32(phy, U3P_U2PHYDTM1, val);
#endif
	USBPHY_DBG("%s(%d)\n", __func__, phy->phy_index);
}

static void
hs_slew_rate_calibrate(struct mtk_usbphy_softc *sc, const struct mtk_usbphy *phy)
{
	uint32_t val = 0;
	uint32_t index		= phy->phy_index;
	int calibration_val;
	int fm_out;

	/* enable USB ring oscillator */
	val =  USBPHY_READ32(phy, U3P_USBPHYACR5);
	val |= PA5_RG_U2_HSTX_SRCAL_EN;
	USBPHY_WRITE32(phy, U3P_USBPHYACR5, val);
	delay(1);

	/*enable free run clock */
	val =  USBPHY_READ32(phy, U3P_U2FREQ_FMMONR1);
	val |= P2F_RG_FRCK_EN;
	USBPHY_WRITE32(phy, U3P_U2FREQ_FMMONR1, val);

	/* set cycle count as 1024, and select u2 channel */
	val =  USBPHY_READ32(phy, U3P_U2FREQ_FMCR0);
	val &= ~(P2F_RG_CYCLECNT | P2F_RG_MONCLK_SEL);
	val |= P2F_RG_CYCLECNT_VAL(U3P_FM_DET_CYCLE_CNT);
	//if (tphy->pdata->version == MTK_PHY_V1)
	//	val |= P2F_RG_MONCLK_SEL_VAL(index >> 1);

	USBPHY_WRITE32(phy, U3P_U2FREQ_FMCR0, val);

	/* enable frequency meter */
	val =  USBPHY_READ32(phy, U3P_U2FREQ_FMCR0);
	val |= P2F_RG_FREQDET_EN;
	USBPHY_WRITE32(phy, U3P_U2FREQ_FMCR0, val);

	/* ignore return value */
	readx_poll_timeout(USBPHY_READ32, phy, U3P_U2FREQ_FMMONR1, val,
		  (val & P2F_USB_FM_VALID), 10, 200);

	fm_out = USBPHY_READ32(phy, U3P_U2FREQ_VALUE);

	/* disable frequency meter */
	val =  USBPHY_READ32(phy, U3P_U2FREQ_FMCR0);
	val &= ~P2F_RG_FREQDET_EN;
	USBPHY_WRITE32(phy, U3P_U2FREQ_FMCR0, val);

	/*disable free run clock */
	val =  USBPHY_READ32(phy, U3P_U2FREQ_FMMONR1);
	val &= ~P2F_RG_FRCK_EN;
	USBPHY_WRITE32(phy, U3P_U2FREQ_FMMONR1, val);

	if (fm_out) {
		/* ( 1024 / FM_OUT ) x reference clock frequency x 0.028 */
		val = U3P_FM_DET_CYCLE_CNT * U3P_REF_CLK * U3P_SLEW_RATE_COEF;
		val /= fm_out;
		calibration_val = DIV_ROUND_CLOSEST(val, U3P_SR_COEF_DIVISOR);
	} else {
		/* if FM detection fail, set default value */
		calibration_val = 4;
	}
	device_printf(sc->sc_dev, "phy:%d, fm_out:%d, calib:%d\n",
				index, fm_out, calibration_val);

	/* set HS slew rate */
	val =  USBPHY_READ32(phy, U3P_USBPHYACR5);
	val &= ~PA5_RG_U2_HSTX_SRCTRL;
	val |= PA5_RG_U2_HSTX_SRCTRL_VAL(calibration_val);
	USBPHY_WRITE32(phy, U3P_USBPHYACR5, val);

	/* disable USB ring oscillator */
	val =  USBPHY_READ32(phy, U3P_USBPHYACR5);
	val &= ~PA5_RG_U2_HSTX_SRCAL_EN;
	USBPHY_WRITE32(phy, U3P_USBPHYACR5, val);

	USBPHY_DBG("%s(%d)\n", __func__, index);
}


static void
phy_instance_set_mode(struct mtk_usbphy_softc *sc, const struct mtk_usbphy *phy, enum phy_mode mode)
{
	uint32_t val = 0;
	uint32_t otg = 0;

	otg =  USBPHY_READ32(phy, U3P_USBPHYACR6);
	otg &= (PA6_RG_U2_OTG_VBUSCMP_EN);

	val = USBPHY_READ32(phy, U3P_U2PHYDTM1);
	switch (mode) {
	case PHY_MODE_USB_DEVICE:
		val |= P2C_FORCE_IDDIG | P2C_RG_IDDIG;
		if (otg & PA6_RG_U2_OTG_VBUSCMP_EN) {
			val |= P2C_FORCE_AVALID | P2C_RG_AVALID;
			val |= P2C_FORCE_BVALID | P2C_RG_BVALID;
			val &= ~(P2C_RG_SESSEND);
			val |= P2C_FORCE_SESSEND;
			val |= P2C_FORCE_VBUSVALID | P2C_RG_VBUSVALID;
		}
		break;
	case PHY_MODE_USB_HOST:
		val &= ~(P2C_RG_IDDIG);
		val |= P2C_FORCE_IDDIG;
		if (otg & PA6_RG_U2_OTG_VBUSCMP_EN) {
			val |= P2C_FORCE_AVALID | P2C_RG_AVALID;
			val |= P2C_FORCE_BVALID | P2C_RG_BVALID;
			val &= ~(P2C_RG_SESSEND);
			val |= P2C_FORCE_SESSEND;
			val |= P2C_FORCE_VBUSVALID | P2C_RG_VBUSVALID;
		}
		break;
#ifdef MTK_MUSB_OTG_ENABLE
	case PHY_MODE_USB_OTG:
		val &= ~(P2C_FORCE_IDDIG | P2C_RG_IDDIG);
		break;
#endif
	default:
		return;
	}
	USBPHY_WRITE32(phy, U3P_U2PHYDTM1, val);

	val = USBPHY_READ32(phy, U3P_U2PHYDTM0);
	aprint_normal("USBPHY%i[0x%lX] = 0x%0x\n",
				phy->phy_index,
				phy->phy_addr + (U3P_U2PHYDTM0),
				val);

	val = USBPHY_READ32(phy, U3P_U2PHYDTM1);
	aprint_normal("USBPHY%i[0x%lX] = 0x%0x\n",
				phy->phy_index,
				phy->phy_addr + (U3P_U2PHYDTM1),
				val);

	device_printf(sc->sc_dev, "%s had acted as USB (%s) role\n", __func__,
				(mode == PHY_MODE_USB_DEVICE) ? "Device" :
				(mode == PHY_MODE_USB_HOST)   ? "Host"   :
				(mode == PHY_MODE_USB_HOST)   ? "OTG"    : "UNKNOWN");
}

#if 0
static void
u2_phy_instance_power_off(struct mtk_usbphy_softc *sc, const struct mtk_usbphy *phy)
{
	uint32_t val = 0;
	uint32_t index = phy->phy_index;

	val =  USBPHY_READ32(sc, U3P_U2PHYDTM0);
	val &= ~(P2C_RG_XCVRSEL | P2C_RG_DATAIN);
	val |= P2C_FORCE_SUSPENDM;
	USBPHY_WRITE32(sc, U3P_U2PHYDTM0, val);

	/* OTG Disable */
	val =  USBPHY_READ32(sc, U3P_USBPHYACR6);
	val &= ~PA6_RG_U2_OTG_VBUSCMP_EN;
	USBPHY_WRITE32(sc, U3P_USBPHYACR6, val);

	/* let suspendm=0, set utmi into analog power down */
	val =  USBPHY_READ32(sc, U3P_U2PHYDTM0);
	val &= ~P2C_RG_SUSPENDM;
	USBPHY_WRITE32(sc, U3P_U2PHYDTM0, val);
	delay(1);

	val =  USBPHY_READ32(sc, U3P_U2PHYDTM1);
	val &= ~(P2C_RG_VBUSVALID | P2C_RG_AVALID);
	val |= P2C_RG_SESSEND;
	USBPHY_WRITE32(sc, U3P_U2PHYDTM1, val);

	USBPHY_DBG("%s(%d)\n", __func__, index);
}
#endif
