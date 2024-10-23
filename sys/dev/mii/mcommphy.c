/* $NetBSD: mcommphy.c,v 1.3 2024/10/23 05:55:45 skrll Exp $ */

/*
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * Motorcomm YT8511C / YT8511H Integrated 10/100/1000 Gigabit Ethernet
 * Transceiver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcommphy.c,v 1.3 2024/10/23 05:55:45 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/miidevs.h>

#define	YT85X1_MCOMMPHY_OUI		0x000000
#define	YT8511_MCOMMPHY_MODEL		0x10
#define	YT8521_MCOMMPHY_MODEL		0x11
#define	YT85X1_MCOMMPHY_REV		0x0a

#define	YTPHY_EXT_REG_ADDR		0x1e
#define	YTPHY_EXT_REG_DATA		0x1f

/* Extended registers */
#define	YT8511_CLOCK_GATING_REG		0x0c
#define	 YT8511_TX_CLK_DELAY_SEL	__BITS(7,4)
#define	 YT8511_CLK_25M_SEL		__BITS(2,1)
#define	 YT8511_CLK_25M_SEL_125M	3
#define	 YT8511_RX_CLK_DELAY_EN		__BIT(0)

#define	YT8511_DELAY_DRIVE_REG		0x0d
#define	 YT8511_FE_TX_CLK_DELAY_SEL	__BITS(15,12)

#define	YT8521_CLOCK_GATING_REG		0x0c
#define	 YT8521_RX_CLK_EN		__BIT(12)

#define	YT8511_SLEEP_CONTROL1_REG	0x27
#define	 YT8511_PLLON_IN_SLP		__BIT(14)

#define	YT8521_SLEEP_CONTROL1_REG	0x27
#define	 YT8521_PLLON_IN_SLP		__BIT(14)

#define YT8521_EXT_CHIP_CONFIG		0xa001
#define  YT8521_RXC_DLY_EN		__BIT(8)
#define  YT8521_CFG_LDO_MASK		__BITS(5, 4)
#define   YT8521_CFG_LDO_3V3		0
#define   YT8521_CFG_LDO_2V5		1
#define   YT8521_CFG_LDO_1V8		2	/* or 3 */

#define YT8521_EXT_SDS_CONFIG		0xa002

#define YT8521_EXT_RGMII_CONFIG1	0xa003
#define  YT8521_TX_CLK_SEL_INV		__BIT(14)
#define  YT8521_RX_DELAY_SEL_MASK	__BITS(13, 10)
#define  YT8521_FE_TX_DELAY_SEL_MASK	__BITS(7, 4)
#define  YT8521_GE_TX_DELAY_SEL_MASK	__BITS(3, 0)
#define  YT8521_DELAY_PS(ps)		((ps) / 150)
#define YT8521_DELAY_DEFAULT		1950

#define YT8521_EXT_RGMII_CONFIG2	0xa004

#define YT8521_EXT_PAD_DRIVE_STRENGTH	0xa010
#define  YT8531_RGMII_RXC_DS_MASK	__BITS(15, 13)
#define  YT8531_RGMII_RXD_DS_HIMASK	__BIT(12)
#define  YT8531_RGMII_RXD_DS_LOMASK	__BITS(5, 4)
#define YT8531_RGMII_RXD_DS_MASK \
    (YT8531_RGMII_RXD_DS_HIMASK | YT8531_RGMII_RXD_DS_LOMASK)

#define YT8531_RGMII_RXD_DS_HIBITS	__BIT(2)
#define YT8531_RGMII_RXD_DS_LOBITS	__BITS(1, 0)

CTASSERT(__SHIFTOUT_MASK(YT8531_RGMII_RXD_DS_HIMASK) ==
	 __SHIFTOUT_MASK(YT8531_RGMII_RXD_DS_HIBITS));
CTASSERT(__SHIFTOUT_MASK(YT8531_RGMII_RXD_DS_LOMASK) ==
	 __SHIFTOUT_MASK(YT8531_RGMII_RXD_DS_LOBITS));

#define YT8531_DEFAULT_DS		3

#define YT8521_EXT_SYNCE_CFG		0xa012
#define  YT8531_EN_SYNC_E		__BIT(6)
#define  YT8531_CLK_FRE_SEL_125M	__BIT(4)
#define  YT8531_CLK_SRC_SEL_MASK	__BITS(3, 1)
#define   YT8531_CLK_SRC_SEL_PLL_125M	0
#define   YT8531_CLK_SRC_SEL_REF_25M	4


typedef enum mcommtype {
    YTUNK,
    YT8511,
    YT8521,
    YT8531,
} mcommtype_t;

struct mcomm_softc {
	struct mii_softc	sc_miisc;

	mcommtype_t		sc_type;

	bool			sc_tx_clk_adj_enabled;
	bool			sc_tx_clk_10_inverted;
	bool			sc_tx_clk_100_inverted;
	bool			sc_tx_clk_1000_inverted;
	u_int			sc_clk_out_frequency_hz;
	u_int			sc_rx_clk_drv_microamp;
	u_int			sc_rx_data_drv_microamp;
	u_int			sc_rx_internal_delay_ps;
	u_int			sc_tx_internal_delay_ps;
};

static int	mcommphymatch(device_t, cfdata_t, void *);
static void	mcommphyattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mcommphy, sizeof(struct mcomm_softc),
    mcommphymatch, mcommphyattach, mii_phy_detach, mii_phy_activate);

static int	mcommphy_service(struct mii_softc *, struct mii_data *, int);
static void	mcommphy_reset(struct mii_softc *);

static const struct mii_phy_funcs mcommphy_funcs = {
	.pf_service = mcommphy_service,
	.pf_status = ukphy_status,
	.pf_reset = mcommphy_reset,
};

static const struct mii_phydesc mcommphys[] = {
	MII_PHY_DESC(MOTORCOMM, YT8531),
	MII_PHY_END,
};

static void
mcomm_yt8531properties(struct mcomm_softc *msc, prop_dictionary_t dict)
{
	struct mii_softc * const sc = &msc->sc_miisc;

	if (!prop_dictionary_get_bool(dict, "motorcomm,tx-clk-adj-enabled",
	    &msc->sc_tx_clk_adj_enabled)) {
		msc->sc_tx_clk_adj_enabled = false;
	} else {
		aprint_verbose_dev(sc->mii_dev,
		    "motorcomm,tx-clk-adj-enabled is %s\n",
		    msc->sc_tx_clk_adj_enabled ? "true" : "false");
	}

	if (!prop_dictionary_get_bool(dict, "motorcomm,tx-clk-10-inverted",
	    &msc->sc_tx_clk_10_inverted)) {
		msc->sc_tx_clk_10_inverted = false;
	} else {
		aprint_verbose_dev(sc->mii_dev,
		    "motorcomm,tx_clk_10_inverted is %s\n",
		    msc->sc_tx_clk_10_inverted ? "true" : "false");
	}

	if (!prop_dictionary_get_bool(dict, "motorcomm,tx-clk-100-inverted",
	    &msc->sc_tx_clk_100_inverted)) {
		msc->sc_tx_clk_100_inverted = false;
	} else {
		aprint_verbose_dev(sc->mii_dev,
		    "motorcomm,tx-clk-100-inverted is %s\n",
		    msc->sc_tx_clk_100_inverted ? "true" : "false");
	}

	if (!prop_dictionary_get_bool(dict, "motorcomm,tx-clk-1000-inverted",
	    &msc->sc_tx_clk_1000_inverted)) {
		msc->sc_tx_clk_1000_inverted = false;
	} else {
		aprint_verbose_dev(sc->mii_dev,
		    "motorcomm,tx-clk-1000-inverted is %s\n",
		    msc->sc_tx_clk_1000_inverted ? "true" : "false");
	}

	if (!prop_dictionary_get_uint32(dict, "motorcomm,clk-out-frequency-hz",
	    &msc->sc_clk_out_frequency_hz)) {
		msc->sc_clk_out_frequency_hz = 0;
	} else {
		aprint_verbose_dev(sc->mii_dev,
		    "motorcomm,clk-out-frequency-hz is %u\n",
		    msc->sc_clk_out_frequency_hz);
	}

	if (!prop_dictionary_get_uint32(dict, "motorcomm,rx-clk-drv-microamp",
	    &msc->sc_rx_clk_drv_microamp)) {
		aprint_debug_dev(sc->mii_dev,
		    "motorcomm,rx-clk-drv-microamp not found\n");
	} else {
		aprint_verbose_dev(sc->mii_dev,
		    "rx-clk-drv-microamp is %u\n",
		    msc->sc_rx_clk_drv_microamp);
	}

	if (!prop_dictionary_get_uint32(dict, "motorcomm,rx-data-drv-microamp",
	    &msc->sc_rx_data_drv_microamp)) {
		aprint_debug_dev(sc->mii_dev,
		    "motorcomm,rx-data-drv-microamp not found\n");
	} else {
		aprint_verbose_dev(sc->mii_dev,
		    "motorcomm,rx-data-drv-microamp is %u\n",
		    msc->sc_rx_data_drv_microamp);
	}

	if (!prop_dictionary_get_uint32(dict, "rx-internal-delay-ps",
	    &msc->sc_rx_internal_delay_ps)) {
		aprint_debug_dev(sc->mii_dev,
		    "rx-internal-delay-ps not found\n");
	} else {
		aprint_verbose_dev(sc->mii_dev,
		    "rx-internal-delay-ps is %u\n",
		    msc->sc_rx_internal_delay_ps);
	}

	if (!prop_dictionary_get_uint32(dict, "tx-internal-delay-ps",
	    &msc->sc_tx_internal_delay_ps)) {
		aprint_debug_dev(sc->mii_dev,
		    "tx-internal-delay-ps not found\n");
	} else {
		aprint_verbose_dev(sc->mii_dev,
		    "tx-internal-delay-ps is %u\n",
		    msc->sc_tx_internal_delay_ps);
	}
}

static bool
mcommphy_isyt8511(struct mii_attach_args *ma)
{

	/*
	 * The YT8511C reports an OUI of 0. Best we can do here is to match
	 * exactly the contents of the PHY identification registers.
	 */
	if (MII_OUI(ma->mii_id1, ma->mii_id2) == YT85X1_MCOMMPHY_OUI &&
	    MII_MODEL(ma->mii_id2) == YT8511_MCOMMPHY_MODEL &&
	    MII_REV(ma->mii_id2) == YT85X1_MCOMMPHY_REV) {
		return true;
	}
	return false;
}


static int
mcommphymatch(device_t parent, cfdata_t match, void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, mcommphys) != NULL)
		return 10;

	if (mcommphy_isyt8511(ma))
		return 10;

	return 0;
}

static void
mcomm_yt8511_init(struct mcomm_softc *msc)
{
	struct mii_softc *sc = &msc->sc_miisc;
	uint16_t oldaddr, data;

	PHY_READ(sc, YTPHY_EXT_REG_ADDR, &oldaddr);

	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, YT8511_CLOCK_GATING_REG);

	PHY_READ(sc, YTPHY_EXT_REG_DATA, &data);

	data &= ~YT8511_CLK_25M_SEL;
	data |= __SHIFTIN(YT8511_CLK_25M_SEL_125M, YT8511_CLK_25M_SEL);
	if (ISSET(sc->mii_flags, MIIF_RXID)) {
		data |= YT8511_RX_CLK_DELAY_EN;
	} else {
		data &= ~YT8511_RX_CLK_DELAY_EN;
	}
	data &= ~YT8511_TX_CLK_DELAY_SEL;
	if (ISSET(sc->mii_flags, MIIF_TXID)) {
		data |= __SHIFTIN(0xf, YT8511_TX_CLK_DELAY_SEL);
	} else {
		data |= __SHIFTIN(0x2, YT8511_TX_CLK_DELAY_SEL);
	}
	PHY_WRITE(sc, YTPHY_EXT_REG_DATA, data);

	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, YT8511_SLEEP_CONTROL1_REG);
	PHY_READ(sc, YTPHY_EXT_REG_DATA, &data);

	data |= YT8511_PLLON_IN_SLP;

	PHY_WRITE(sc, YTPHY_EXT_REG_DATA, data);

	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, oldaddr);
}



static void
mcomm_yt8521_init(struct mcomm_softc *msc)
{
	struct mii_softc *sc = &msc->sc_miisc;
	bool rx_delay_en = false;
	u_int rx_delay_val;
	u_int tx_delay_val;
	uint16_t oldaddr, data;

	PHY_READ(sc, YTPHY_EXT_REG_ADDR, &oldaddr);

	u_int rx_delay = msc->sc_rx_internal_delay_ps;
	if (rx_delay <= 15 * 150 && rx_delay % 150 == 0) {
		rx_delay_val = YT8521_DELAY_PS(rx_delay);
	} else {
		/* It's OK if this underflows */
		rx_delay -= 1900;
		if (rx_delay <= 15 * 150 && rx_delay % 150 == 0) {
			rx_delay_en = true;
			rx_delay_val = YT8521_DELAY_PS(rx_delay);
		} else {
			rx_delay_val = YT8521_DELAY_PS(YT8521_DELAY_DEFAULT);
		}
	}

	u_int tx_delay = msc->sc_tx_internal_delay_ps;
	if (tx_delay <= 15 * 150 && tx_delay % 150 == 0) {
		tx_delay_val = YT8521_DELAY_PS(tx_delay);
	} else {
		tx_delay_val = YT8521_DELAY_PS(YT8521_DELAY_DEFAULT);
	}

	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, YT8521_EXT_CHIP_CONFIG);
	PHY_READ(sc, YTPHY_EXT_REG_DATA, &data);

	if (rx_delay_en)
		data |= YT8521_RXC_DLY_EN;
	else
		data &= ~YT8521_RXC_DLY_EN;

	PHY_WRITE(sc, YTPHY_EXT_REG_DATA, data);

	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, YT8521_EXT_RGMII_CONFIG1);
	PHY_READ(sc, YTPHY_EXT_REG_DATA, &data);

	data &= ~(YT8521_RX_DELAY_SEL_MASK | YT8521_GE_TX_DELAY_SEL_MASK);
	// XXX FE?
	data |=
	    __SHIFTIN(rx_delay_val, YT8521_RX_DELAY_SEL_MASK) |
	    __SHIFTIN(tx_delay_val, YT8521_GE_TX_DELAY_SEL_MASK);

	PHY_WRITE(sc, YTPHY_EXT_REG_DATA, data);

	/* Restore address register. */
	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, oldaddr);
}

struct mcomm_ytphy_ds_map {
	u_int microamps;
	u_int ds;
};

struct mcomm_ytphy_ds_map mcomm_1d8v_ds_map[] = {
	{ .microamps = 1200, .ds = 0 },
	{ .microamps = 2100, .ds = 1 },
	{ .microamps = 2700, .ds = 2 },
	{ .microamps = 2910, .ds = 3 },
	{ .microamps = 3110, .ds = 4 },
	{ .microamps = 3600, .ds = 5 },
	{ .microamps = 3970, .ds = 6 },
	{ .microamps = 4350, .ds = 7 },
};
struct mcomm_ytphy_ds_map mcomm_3d3v_ds_map[] = {
	{ .microamps = 3070, .ds = 0 },
	{ .microamps = 4080, .ds = 1 },
	{ .microamps = 4370, .ds = 2 },
	{ .microamps = 4680, .ds = 3 },
	{ .microamps = 5020, .ds = 4 },
	{ .microamps = 5450, .ds = 5 },
	{ .microamps = 5740, .ds = 6 },
	{ .microamps = 6140, .ds = 7 },
};

static u_int
mcomm_yt8531_ds(struct mcomm_softc *msc,
    struct mcomm_ytphy_ds_map *ds_map, size_t n,
    u_int microamps, u_int millivolts)
{
	for (size_t i = 0; i < n; i++) {
		if (ds_map[i].microamps == microamps)
			return ds_map[i].ds;
	}
	if (microamps) {
		struct mii_softc *sc = &msc->sc_miisc;

		aprint_error_dev(sc->mii_dev, "unknown drive strength "
		    "(%u uA at %u mV)", microamps, millivolts);
	}
	return YT8531_DEFAULT_DS;
}

static void
mcomm_yt8531_init(struct mcomm_softc *msc)
{
	struct mii_softc *sc = &msc->sc_miisc;
	uint16_t oldaddr, data;

	mcomm_yt8521_init(msc);

	/* Save address register. */
	PHY_READ(sc, YTPHY_EXT_REG_ADDR, &oldaddr);

	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, YT8521_EXT_CHIP_CONFIG);
	PHY_READ(sc, YTPHY_EXT_REG_DATA, &data);

	struct mcomm_ytphy_ds_map *ds_map = NULL;
	size_t ds_mapsize;
	u_int millivolts = 1800;
	switch (__SHIFTOUT(data, YT8521_CFG_LDO_MASK)) {
	case YT8521_CFG_LDO_3V3:
		ds_map = mcomm_3d3v_ds_map;
		ds_mapsize = __arraycount(mcomm_3d3v_ds_map);
		millivolts = 3300;
		break;
	case YT8521_CFG_LDO_2V5:
		millivolts = 2500;
		break;
	default:
		ds_map = mcomm_1d8v_ds_map;
		ds_mapsize = __arraycount(mcomm_1d8v_ds_map);
		break;
	}

	if (ds_map) {
		u_int rx_clk_ds = mcomm_yt8531_ds(msc, ds_map, ds_mapsize,
		    msc->sc_rx_clk_drv_microamp, millivolts);
		u_int rx_data_ds = mcomm_yt8531_ds(msc, ds_map, ds_mapsize,
		    msc->sc_rx_data_drv_microamp, millivolts);

		PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, YT8521_EXT_PAD_DRIVE_STRENGTH);
		PHY_READ(sc, YTPHY_EXT_REG_DATA, &data);

		data &= ~(YT8531_RGMII_RXC_DS_MASK | YT8531_RGMII_RXD_DS_MASK);

		u_int rx_data_ds_hi =
		    __SHIFTOUT(rx_data_ds, YT8531_RGMII_RXD_DS_HIBITS);
		u_int rx_data_ds_lo =
		    __SHIFTOUT(rx_data_ds, YT8531_RGMII_RXD_DS_LOBITS);
		data |=
		    __SHIFTIN(rx_clk_ds, YT8531_RGMII_RXC_DS_MASK) |
		    __SHIFTIN(rx_data_ds_hi, YT8531_RGMII_RXD_DS_HIMASK) |
		    __SHIFTIN(rx_data_ds_lo, YT8531_RGMII_RXD_DS_LOMASK);

		PHY_WRITE(sc, YTPHY_EXT_REG_DATA, data);
	}

	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, YT8521_EXT_SYNCE_CFG);
	PHY_READ(sc, YTPHY_EXT_REG_DATA, &data);

	data &= ~(YT8531_EN_SYNC_E | YT8531_CLK_SRC_SEL_MASK);

	switch (msc->sc_clk_out_frequency_hz) {
	case 125000000:
		data |= YT8531_EN_SYNC_E;
		data |= YT8531_CLK_FRE_SEL_125M;
		data |= __SHIFTIN(YT8531_CLK_SRC_SEL_PLL_125M, YT8531_CLK_SRC_SEL_MASK);
		break;
	case 25000000:
		data |= YT8531_EN_SYNC_E;
		data |= __SHIFTIN(YT8531_CLK_SRC_SEL_REF_25M, YT8531_CLK_SRC_SEL_MASK);
		break;
	default:
		break;
	}

	PHY_WRITE(sc, YTPHY_EXT_REG_DATA, data);

	/* Restore address register. */
	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, oldaddr);
}


static void
mcommphy_reset(struct mii_softc *sc)
{
	struct mcomm_softc * const msc =
	    container_of(sc, struct mcomm_softc, sc_miisc);

	KASSERT(mii_locked(sc->mii_pdata));

	mii_phy_reset(sc);

	switch (msc->sc_type) {
	case YT8511:
		mcomm_yt8511_init(msc);
		break;
	case YT8531:
		mcomm_yt8531_init(msc);
		break;
	default:
		return;
	}
}


static void
mcommphyattach(device_t parent, device_t self, void *aux)
{
	struct mcomm_softc *msc = device_private(self);
	struct mii_softc *sc = &msc->sc_miisc;
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;

	msc->sc_type = YTUNK;
	if (mcommphy_isyt8511(ma)) {
		msc->sc_type = YT8511;
		aprint_normal(": Motorcomm YT8511 GbE PHY\n");
	} else {
		KASSERT(mii_phy_match(ma, mcommphys) != NULL);
		msc->sc_type = YT8531;
		aprint_normal(": Motorcomm YT8531 GbE PHY\n");
	}
	aprint_naive(": Media interface\n");

	sc->mii_dev = self;
	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_mpd_oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	sc->mii_mpd_model = MII_MODEL(ma->mii_id2);
	sc->mii_mpd_rev = MII_REV(ma->mii_id2);
	sc->mii_funcs = &mcommphy_funcs;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;

	prop_dictionary_t dict = device_properties(parent);
	switch (msc->sc_type) {
	case YT8521:
	case YT8531:
		/* Default values */
		msc->sc_rx_internal_delay_ps = YT8521_DELAY_DEFAULT;
		msc->sc_tx_internal_delay_ps = YT8521_DELAY_DEFAULT;

		mcomm_yt8531properties(msc, dict);
		break;
	default:
		break;
	}

	mii_lock(mii);

	PHY_RESET(sc);

	PHY_READ(sc, MII_BMSR, &sc->mii_capabilities);
	sc->mii_capabilities &= ma->mii_capmask;
	if (sc->mii_capabilities & BMSR_EXTSTAT)
		PHY_READ(sc, MII_EXTSR, &sc->mii_extcapabilities);

	mii_unlock(mii);

	mii_phy_add_media(sc);
}

static void
ytphy_yt8521_update(struct mcomm_softc *msc)
{
	struct mii_softc *sc = &msc->sc_miisc;
	struct mii_data *mii = sc->mii_pdata;
	bool tx_clk_inv = false;
	uint16_t oldaddr, data;

	if (sc->mii_media_active == mii->mii_media_active)
		return;

	if (!msc->sc_tx_clk_adj_enabled)
		return;

	switch (IFM_SUBTYPE(mii->mii_media_active)) {
	case IFM_1000_T:
		tx_clk_inv = msc->sc_tx_clk_1000_inverted;
		break;
	case IFM_100_TX:
		tx_clk_inv = msc->sc_tx_clk_100_inverted;
		break;
	case IFM_10_T:
		tx_clk_inv = msc->sc_tx_clk_10_inverted;
		break;
	}

	/* Save address register. */
	PHY_READ(sc, YTPHY_EXT_REG_ADDR, &oldaddr);

	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, YT8521_EXT_RGMII_CONFIG1);
	PHY_READ(sc, YTPHY_EXT_REG_DATA, &data);

	if (tx_clk_inv)
		data |= YT8521_TX_CLK_SEL_INV;
	else
		data &= ~YT8521_TX_CLK_SEL_INV;

	PHY_WRITE(sc, YTPHY_EXT_REG_DATA, data);

	/* Restore address register. */
	PHY_WRITE(sc, YTPHY_EXT_REG_ADDR, oldaddr);
}

static int
mcommphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	struct mcomm_softc * const msc =
	    container_of(sc, struct mcomm_softc, sc_miisc);

	KASSERT(mii_locked(mii));

	switch (cmd) {
	case MII_POLLSTAT:
		/* If we're not polling our PHY instance, just return. */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst)
			return 0;
		break;

	case MII_MEDIACHG:
		/* If the interface is not up, don't do anything. */
		if ((mii->mii_ifp->if_flags & IFF_UP) == 0)
			break;

		mii_phy_setmedia(sc);
		break;

	case MII_TICK:
		/* If we're not currently selected, just return. */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst)
			return 0;

		if (mii_phy_tick(sc) == EJUSTRETURN)
			return 0;
		break;

	case MII_DOWN:
		mii_phy_down(sc);
		return 0;
	}

	/* Update the media status. */
	mii_phy_status(sc);

	/* Callback if something changed. */
	if (msc->sc_type != YT8511)
		ytphy_yt8521_update(msc);

	mii_phy_update(sc, cmd);
	return 0;
}
