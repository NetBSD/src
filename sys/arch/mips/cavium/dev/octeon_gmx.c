/*	$NetBSD: octeon_gmx.c,v 1.21 2022/05/04 07:32:50 andvar Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_gmx.c,v 1.21 2022/05/04 07:32:50 andvar Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <sys/cdefs.h>
#include <sys/kmem.h>
#include <sys/syslog.h>

#include <mips/locore.h>
#include <mips/include/cpuregs.h>

#include <mips/cavium/dev/octeon_asxvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_gmxreg.h>
#include <mips/cavium/dev/octeon_gmxvar.h>
#include <mips/cavium/dev/octeon_ipdvar.h>
#include <mips/cavium/dev/octeon_pipvar.h>
#include <mips/cavium/dev/octeon_smivar.h>

#include <mips/cavium/include/iobusvar.h>

/*
 * CNnnXX packet interface
 *
 *
 * CN30XX  - 1 GMX interface  x 3 ports
 * CN31XX  - 1 GMX interface  x 3 ports
 * CN38XX  - 2 GMX interfaces x 4 ports
 * CN50XX  - 1 GMX interface  x 3 ports
 * CN52XX  - 1 GMX interface  x 4 ports
 * CN56XX  - 2 GMX interfaces x 4 ports
 * CN58XX  - 2 GMX interfaces x 4 ports
 * CN61XX  - 2 GMX interfaces x 4 ports
 * CN63XX  - 1 GMX interface  x 4 ports
 * CN66XX  - 2 GMX interfaces x 4 ports
 * CN68XX  - 5 GMX interfaces x 4 ports
 * CN70XX  - 2 GMX interfaces x 4 ports
 * CNF71XX - 1 GMX interface  x 2 ports
 */

#define	dprintf(...)
#define	CNMAC_KASSERT	KASSERT

#define	ADDR2UINT64(u, a) \
	do { \
		u = \
		    (((uint64_t)a[0] << 40) | ((uint64_t)a[1] << 32) | \
		     ((uint64_t)a[2] << 24) | ((uint64_t)a[3] << 16) | \
		     ((uint64_t)a[4] <<  8) | ((uint64_t)a[5] <<  0)); \
	} while (0)
#define	UINT642ADDR(a, u) \
	do { \
		a[0] = (uint8_t)((u) >> 40); a[1] = (uint8_t)((u) >> 32); \
		a[2] = (uint8_t)((u) >> 24); a[3] = (uint8_t)((u) >> 16); \
		a[4] = (uint8_t)((u) >>  8); a[5] = (uint8_t)((u) >>  0); \
	} while (0)

#define	_GMX_RD8(sc, off) \
	bus_space_read_8((sc)->sc_port_gmx->sc_regt, (sc)->sc_port_gmx->sc_regh, (off))
#define	_GMX_WR8(sc, off, v) \
	bus_space_write_8((sc)->sc_port_gmx->sc_regt, (sc)->sc_port_gmx->sc_regh, (off), (v))
#define	_GMX_PORT_RD8(sc, off) \
	bus_space_read_8((sc)->sc_port_gmx->sc_regt, (sc)->sc_port_regh, (off))
#define	_GMX_PORT_WR8(sc, off, v) \
	bus_space_write_8((sc)->sc_port_gmx->sc_regt, (sc)->sc_port_regh, (off), (v))

#define PCS_READ_8(sc, reg) \
	bus_space_read_8((sc)->sc_port_gmx->sc_regt, (sc)->sc_port_pcs_regh, (reg))
#define PCS_WRITE_8(sc, reg, val) \
	bus_space_write_8((sc)->sc_port_gmx->sc_regt, (sc)->sc_port_pcs_regh, (reg), (val))

struct octgmx_port_ops {
	int	(*port_ops_enable)(struct octgmx_port_softc *, int);
	int	(*port_ops_speed)(struct octgmx_port_softc *);
	int	(*port_ops_timing)(struct octgmx_port_softc *);
};

static int	octgmx_match(device_t, struct cfdata *, void *);
static void	octgmx_attach(device_t, device_t, void *);
static int	octgmx_print(void *, const char *);
static int	octgmx_init(struct octgmx_softc *);

static int	octgmx_link_enable(struct octgmx_port_softc *, int);
static int	octgmx_rx_frm_ctl_xable(struct octgmx_port_softc *, uint64_t,
		    int);
static void	octgmx_tx_int_enable(struct octgmx_port_softc *, int);
static void	octgmx_rx_int_enable(struct octgmx_port_softc *, int);
static int	octgmx_rx_frm_ctl_enable(struct octgmx_port_softc *, uint64_t);
static int	octgmx_rx_frm_ctl_disable(struct octgmx_port_softc *, uint64_t);
static int	octgmx_tx_thresh(struct octgmx_port_softc *, int);

static int	octgmx_rgmii_enable(struct octgmx_port_softc *, int);
static int	octgmx_rgmii_speed(struct octgmx_port_softc *);
static int	octgmx_rgmii_speed_newlink(struct octgmx_port_softc *,
		    uint64_t *);
static int	octgmx_rgmii_speed_speed(struct octgmx_port_softc *);
static int	octgmx_rgmii_timing(struct octgmx_port_softc *);

static int	octgmx_sgmii_enable(struct octgmx_port_softc *, int);
static int	octgmx_sgmii_speed(struct octgmx_port_softc *);
static int	octgmx_sgmii_timing(struct octgmx_port_softc *);

static const int	octgmx_rx_adr_cam_regs[] = {
	GMX0_RX0_ADR_CAM0, GMX0_RX0_ADR_CAM1, GMX0_RX0_ADR_CAM2,
	GMX0_RX0_ADR_CAM3, GMX0_RX0_ADR_CAM4, GMX0_RX0_ADR_CAM5
};

static struct octgmx_port_ops octgmx_port_ops_mii = {
	/* XXX not implemented */
};

static struct octgmx_port_ops octgmx_port_ops_gmii = {
	.port_ops_enable = octgmx_rgmii_enable,
	.port_ops_speed = octgmx_rgmii_speed,
	.port_ops_timing = octgmx_rgmii_timing,
};

static struct octgmx_port_ops octgmx_port_ops_rgmii = {
	.port_ops_enable = octgmx_rgmii_enable,
	.port_ops_speed = octgmx_rgmii_speed,
	.port_ops_timing = octgmx_rgmii_timing,
};

static struct octgmx_port_ops octgmx_port_ops_sgmii = {
	.port_ops_enable = octgmx_sgmii_enable,
	.port_ops_speed = octgmx_sgmii_speed,
	.port_ops_timing = octgmx_sgmii_timing,
};

static struct octgmx_port_ops octgmx_port_ops_spi42 = {
	/* XXX not implemented */
};

static struct octgmx_port_ops *octgmx_port_ops[] = {
	[GMX_MII_PORT] = &octgmx_port_ops_mii,
	[GMX_GMII_PORT] = &octgmx_port_ops_gmii,
	[GMX_RGMII_PORT] = &octgmx_port_ops_rgmii,
	[GMX_SGMII_PORT] = &octgmx_port_ops_sgmii,
	[GMX_SPI42_PORT] = &octgmx_port_ops_spi42
};
static const char *octgmx_port_types[] = {
	[GMX_MII_PORT] = "MII",
	[GMX_GMII_PORT] = "GMII",
	[GMX_RGMII_PORT] = "RGMII",
	[GMX_SGMII_PORT] = "SGMII",
	[GMX_SPI42_PORT] = "SPI-4.2"
};

CFATTACH_DECL_NEW(octgmx, sizeof(struct octgmx_softc),
    octgmx_match, octgmx_attach, NULL, NULL);

static int
octgmx_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;
	if (cf->cf_unit != aa->aa_unitno)
		return 0;
	return 1;
}

static void
octgmx_attach(device_t parent, device_t self, void *aux)
{
	struct octgmx_softc *sc = device_private(self);
	struct iobus_attach_args *aa = aux;
	struct octsmi_softc *smi;
	struct octgmx_port_softc *port_sc;
	struct octgmx_attach_args gmx_aa;
	int port, status;
	int i;

	sc->sc_dev = self;
	sc->sc_regt = aa->aa_bust;
	sc->sc_unitno = aa->aa_unitno;
	
	aprint_normal("\n");

	status = bus_space_map(sc->sc_regt, aa->aa_unit->addr,
	    GMX_PORT_SIZE, 0, &sc->sc_regh);
	if (status != 0)
		panic(": can't map register");

	octgmx_init(sc);

	sc->sc_ports = kmem_zalloc(sizeof(*sc->sc_ports) * sc->sc_nports,
	    KM_SLEEP);

	for (i = 0; i < sc->sc_nports; i++) {
		port = GMX_PORT_NUM(sc->sc_unitno, i);
		smi = octsmi_lookup(/*XXX*/0, port);
		if (smi == NULL)
			continue;

		port_sc = &sc->sc_ports[i];
		port_sc->sc_port_gmx = sc;
		port_sc->sc_port_no = port;
		port_sc->sc_port_type = sc->sc_port_types[i];
		port_sc->sc_port_ops = octgmx_port_ops[port_sc->sc_port_type];
		status = bus_space_map(sc->sc_regt,
		    aa->aa_unit->addr + GMX_PORT_SIZE * i,
		    GMX_PORT_SIZE, 0, &port_sc->sc_port_regh);
		if (status != 0)
			panic(": can't map port register");

		switch (port_sc->sc_port_type) {
		case GMX_MII_PORT:
		case GMX_GMII_PORT:
		case GMX_RGMII_PORT: {
			struct octasx_attach_args asx_aa;

			asx_aa.aa_port = i;
			asx_aa.aa_regt = aa->aa_bust;
			octasx_init(&asx_aa, &port_sc->sc_port_asx);
			break;
		}
		case GMX_SGMII_PORT:
			if (bus_space_map(sc->sc_regt,
			    PCS_BASE(sc->sc_unitno, i), PCS_SIZE, 0,
			    &port_sc->sc_port_pcs_regh))
				panic("could not map PCS registers");
			break;
		default:
			/* nothing */
			break;
		}

		(void)memset(&gmx_aa, 0, sizeof(gmx_aa));
		gmx_aa.ga_regt = aa->aa_bust;
		gmx_aa.ga_addr = aa->aa_unit->addr;
		gmx_aa.ga_name = "cnmac";
		gmx_aa.ga_portno = port_sc->sc_port_no;
		gmx_aa.ga_port_type = sc->sc_port_types[i];
		gmx_aa.ga_smi = smi;
		gmx_aa.ga_gmx = sc;
		gmx_aa.ga_gmx_port = port_sc;
		config_found(self, &gmx_aa, octgmx_print, CFARGS_NONE);
	}
}

static int
octgmx_print(void *aux, const char *pnp)
{
	struct octgmx_attach_args *ga = aux;

	aprint_normal(": address=0x%" PRIx64 ": %s\n", ga->ga_addr,
	    octgmx_port_types[ga->ga_port_type]);

	return UNCONF;
}

static int
octgmx_init(struct octgmx_softc *sc)
{
	int result = 0;
	uint64_t inf_mode;
	const mips_prid_t cpu_id = mips_options.mips_cpu_id;

	inf_mode = bus_space_read_8(sc->sc_regt, sc->sc_regh, GMX0_INF_MODE);
	if ((inf_mode & INF_MODE_EN) == 0) {
		aprint_normal("ports are disabled\n");
		sc->sc_nports = 0;
		return 1;
	}

	if (MIPS_PRID_CID(cpu_id) != MIPS_PRID_CID_CAVIUM)
		return 1;

	switch (MIPS_PRID_IMPL(cpu_id)) {
	case MIPS_CN31XX:
		/*
		 * Packet Interface Configuration
		 * GMX Registers, Interface Mode Register, GMX0_INF_MODE
		 */
		if ((inf_mode & INF_MODE_TYPE) == 0) {
			/* all three ports configured as RGMII */
			sc->sc_nports = 3;
			sc->sc_port_types[0] = GMX_RGMII_PORT;
			sc->sc_port_types[1] = GMX_RGMII_PORT;
			sc->sc_port_types[2] = GMX_RGMII_PORT;
		} else {
			/* port 0: RGMII, port 1: GMII, port 2: disabled */
			sc->sc_nports = 2;
			sc->sc_port_types[0] = GMX_RGMII_PORT;
			sc->sc_port_types[1] = GMX_GMII_PORT;
		}
		break;
	case MIPS_CN30XX:
	case MIPS_CN50XX:
		/*
		 * Packet Interface Configuration
		 * GMX Registers, Interface Mode Register, GMX0_INF_MODE
		 */
		if ((inf_mode & INF_MODE_P0MII) == 0)
			sc->sc_port_types[0] = GMX_RGMII_PORT;
		else
			sc->sc_port_types[0] = GMX_MII_PORT;
		if ((inf_mode & INF_MODE_TYPE) == 0) {
			/* port 1 and 2 are configred as RGMII ports */
			sc->sc_nports = 3;
			sc->sc_port_types[1] = GMX_RGMII_PORT;
			sc->sc_port_types[2] = GMX_RGMII_PORT;
		} else {
			/* port 1: GMII/MII, port 2: disabled */
			/* GMII or MII port is slected by GMX_PRT1_CFG[SPEED] */
			sc->sc_nports = 2;
			sc->sc_port_types[1] = GMX_GMII_PORT;
		}
#if 0 /* XXX XXX XXX */
		/* port 2 is in CN3010/CN5010 only */
		if ((octeon_model(id) != OCTEON_MODEL_CN3010) &&
		    (octeon_model(id) != OCTEON_MODEL_CN5010))
			if (sc->sc_nports == 3)
				sc->sc_nports = 2;
#endif
		break;
	case MIPS_CN70XX:
		switch (inf_mode & INF_MODE_MODE) {
		case INF_MODE_MODE_SGMII:
			sc->sc_nports = 4;
			for (int i = 0; i < sc->sc_nports; i++)
				sc->sc_port_types[i] = GMX_SGMII_PORT;
			break;
#ifdef notyet
		case INF_MODE_MODE_XAUI:
#endif
		default:
			sc->sc_nports = 0;
			result = 1;
		}
		break;
	default:
		aprint_normal("unsupported octeon model: 0x%x\n", cpu_id); 
		sc->sc_nports = 0;
		result = 1;
		break;
	}

	return result;
}

/* XXX RGMII specific */
static int
octgmx_link_enable(struct octgmx_port_softc *sc, int enable)
{
	uint64_t prt_cfg;

	octgmx_tx_int_enable(sc, enable);
	octgmx_rx_int_enable(sc, enable);

	prt_cfg = _GMX_PORT_RD8(sc, GMX0_PRT0_CFG);
	if (enable) {
		if (octgmx_link_status(sc)) {
			SET(prt_cfg, PRTN_CFG_EN);
		}
	} else {
		CLR(prt_cfg, PRTN_CFG_EN);
	}
	_GMX_PORT_WR8(sc, GMX0_PRT0_CFG, prt_cfg);
	/* software should read back to flush the write operation. */
	(void)_GMX_PORT_RD8(sc, GMX0_PRT0_CFG);

	return 0;
}

/* XXX RGMII specific */
int
octgmx_stats_init(struct octgmx_port_softc *sc)
{
        _GMX_PORT_WR8(sc, GMX0_RX0_STATS_PKTS, 0);
        _GMX_PORT_WR8(sc, GMX0_RX0_STATS_PKTS_DRP, 0);
        _GMX_PORT_WR8(sc, GMX0_RX0_STATS_PKTS_BAD, 0);
        _GMX_PORT_WR8(sc, GMX0_TX0_STAT0, 0);
        _GMX_PORT_WR8(sc, GMX0_TX0_STAT1, 0);
        _GMX_PORT_WR8(sc, GMX0_TX0_STAT3, 0);
        _GMX_PORT_WR8(sc, GMX0_TX0_STAT9, 0);

	return 0;
}

int
octgmx_tx_stats_rd_clr(struct octgmx_port_softc *sc, int enable)
{
	_GMX_PORT_WR8(sc, GMX0_TX0_STATS_CTL, enable ? 1 : 0);
	return 0;
}

int
octgmx_rx_stats_rd_clr(struct octgmx_port_softc *sc, int enable)
{
	_GMX_PORT_WR8(sc, GMX0_RX0_STATS_CTL, enable ? 1 : 0);
	return 0;
}

static int
octgmx_tx_ovr_bp_enable(struct octgmx_port_softc *sc, int enable)
{
	uint64_t ovr_bp;
	int index = GMX_PORT_INDEX(sc->sc_port_no);

	ovr_bp = _GMX_RD8(sc, GMX0_TX_OVR_BP);
	if (enable) {
		CLR(ovr_bp, __SHIFTIN(__BIT(index), TX_OVR_BP_EN));
		SET(ovr_bp, __SHIFTIN(__BIT(index), TX_OVR_BP_BP));
		/* XXX really??? */
		SET(ovr_bp, __SHIFTIN(__BIT(index), TX_OVR_BP_IGN_FULL));
	} else {
		SET(ovr_bp, __SHIFTIN(__BIT(index), TX_OVR_BP_EN));
		CLR(ovr_bp, __SHIFTIN(__BIT(index), TX_OVR_BP_BP));
		/* XXX really??? */
		SET(ovr_bp, __SHIFTIN(__BIT(index), TX_OVR_BP_IGN_FULL));
	}
	_GMX_WR8(sc, GMX0_TX_OVR_BP, ovr_bp);
	return 0;
}

static int
octgmx_rx_pause_enable(struct octgmx_port_softc *sc, int enable)
{
	if (enable) {
		octgmx_rx_frm_ctl_enable(sc, RXN_FRM_CTL_CTL_BCK);
	} else {
		octgmx_rx_frm_ctl_disable(sc, RXN_FRM_CTL_CTL_BCK);
	}

	return 0;
}

static void
octgmx_tx_int_enable(struct octgmx_port_softc *sc, int enable)
{
	uint64_t tx_int_xxx = 0;

	SET(tx_int_xxx,
	    TX_INT_REG_LATE_COL |
	    TX_INT_REG_XSDEF |
	    TX_INT_REG_XSCOL |
	    TX_INT_REG_UNDFLW |
	    TX_INT_REG_PKO_NXA);
	_GMX_WR8(sc, GMX0_TX_INT_REG, tx_int_xxx);
	_GMX_WR8(sc, GMX0_TX_INT_EN, enable ? tx_int_xxx : 0);
}

static void
octgmx_rx_int_enable(struct octgmx_port_softc *sc, int enable)
{
	uint64_t rx_int_xxx = 0;

	SET(rx_int_xxx, 0 |
	    RXN_INT_REG_PHY_DUPX |
	    RXN_INT_REG_PHY_SPD |
	    RXN_INT_REG_PHY_LINK |
	    RXN_INT_REG_IFGERR |
	    RXN_INT_REG_COLDET |
	    RXN_INT_REG_FALERR |
	    RXN_INT_REG_RSVERR |
	    RXN_INT_REG_PCTERR |
	    RXN_INT_REG_OVRERR |
	    RXN_INT_REG_NIBERR |
	    RXN_INT_REG_SKPERR |
	    RXN_INT_REG_RCVERR |
	    RXN_INT_REG_LENERR |
	    RXN_INT_REG_ALNERR |
	    RXN_INT_REG_FCSERR |
	    RXN_INT_REG_JABBER |
	    RXN_INT_REG_MAXERR |
	    RXN_INT_REG_CAREXT |
	    RXN_INT_REG_MINERR);
	_GMX_PORT_WR8(sc, GMX0_RX0_INT_REG, rx_int_xxx);
	_GMX_PORT_WR8(sc, GMX0_RX0_INT_EN, enable ? rx_int_xxx : 0);
}

static int
octgmx_rx_frm_ctl_enable(struct octgmx_port_softc *sc, uint64_t rx_frm_ctl)
{
	struct ifnet *ifp = &sc->sc_port_ec->ec_if;
	unsigned int maxlen;

	maxlen = roundup(ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN +
	    ETHER_VLAN_ENCAP_LEN, 8);
	_GMX_PORT_WR8(sc, GMX0_RX0_JABBER, maxlen);

	return octgmx_rx_frm_ctl_xable(sc, rx_frm_ctl, 1);
}

static int
octgmx_rx_frm_ctl_disable(struct octgmx_port_softc *sc, uint64_t rx_frm_ctl)
{
	return octgmx_rx_frm_ctl_xable(sc, rx_frm_ctl, 0);
}

static int
octgmx_rx_frm_ctl_xable(struct octgmx_port_softc *sc, uint64_t rx_frm_ctl,
    int enable)
{
	uint64_t tmp;

	tmp = _GMX_PORT_RD8(sc, GMX0_RX0_FRM_CTL);
	if (enable)
		SET(tmp, rx_frm_ctl);
	else
		CLR(tmp, rx_frm_ctl);
	_GMX_PORT_WR8(sc, GMX0_RX0_FRM_CTL, tmp);

	return 0;
}

static int
octgmx_tx_thresh(struct octgmx_port_softc *sc, int cnt)
{
	_GMX_PORT_WR8(sc, GMX0_TX0_THRESH, cnt);
	return 0;
}

int
octgmx_set_mac_addr(struct octgmx_port_softc *sc, const uint8_t *addr)
{
	uint64_t mac;
	int i;

	ADDR2UINT64(mac, addr);

	octgmx_link_enable(sc, 0);
	sc->sc_mac = mac;

	_GMX_PORT_WR8(sc, GMX0_SMAC0, mac);
	for (i = 0; i < 6; i++)
		_GMX_PORT_WR8(sc, octgmx_rx_adr_cam_regs[i], addr[i]);

	octgmx_link_enable(sc, 1);

	return 0;
}

int
octgmx_set_filter(struct octgmx_port_softc *sc)
{
	struct ethercom *ec = sc->sc_port_ec;
	struct ifnet *ifp = &ec->ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint64_t ctl = 0;
	int multi = 0;
	uint64_t cam_en = 1;	/* enable CAM 0 for self MAC addr */

	octgmx_link_enable(sc, 0);

	if (ISSET(ifp->if_flags, IFF_BROADCAST)) {
		dprintf("accept broadcast\n");
		SET(ctl, RXN_ADR_CTL_BCST);
	}
	if (ISSET(ifp->if_flags, IFF_PROMISC)) {
		dprintf("promiscas(reject cam)\n");
		CLR(ctl, RXN_ADR_CTL_CAM_MODE);
	} else {
		dprintf("not promiscas(accept cam)\n");
		SET(ctl, RXN_ADR_CTL_CAM_MODE);
	}

	/*
	 * Note first entry is self MAC address; other 7 entries are available
	 * for multicast addresses.
	 */

	ETHER_LOCK(ec);
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		int i;

		dprintf("%d: lo(%02x:%02x:%02x:%02x:%02x:%02x) - "
		    "hi(%02x:%02x:%02x:%02x:%02x:%02x)\n",
		    multi + 1,
		    enm->enm_addrlo[0], enm->enm_addrlo[1],
		    enm->enm_addrlo[2], enm->enm_addrlo[3],
		    enm->enm_addrlo[4], enm->enm_addrlo[5],
		    enm->enm_addrhi[0], enm->enm_addrhi[1],
		    enm->enm_addrhi[2], enm->enm_addrhi[3],
		    enm->enm_addrhi[4], enm->enm_addrhi[5]);
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			dprintf("all multicast\n");
			SET(ifp->if_flags, IFF_ALLMULTI);
			ETHER_UNLOCK(ec);
			goto setmulti;
		}
		multi++;

		/* XXX XXX XXX */
		if (multi >= 8) {
			SET(ifp->if_flags, IFF_ALLMULTI);
			ETHER_UNLOCK(ec);
			goto setmulti;
		}
		/* XXX XXX XXX */

		/* XXX XXX XXX */
		SET(cam_en, __BIT(multi));
		/* XXX XXX XXX */

		for (i = 0; i < 6; i++) {
			uint64_t tmp;

			/* XXX XXX XXX */
			tmp = _GMX_PORT_RD8(sc, octgmx_rx_adr_cam_regs[i]);
			CLR(tmp, 0xffULL << (8 * multi));
			SET(tmp, (uint64_t)enm->enm_addrlo[i] << (8 * multi));
			_GMX_PORT_WR8(sc, octgmx_rx_adr_cam_regs[i], tmp);
			/* XXX XXX XXX */
			    
		}
		for (i = 0; i < 6; i++)
			dprintf("cam%d = 0x%016lx\n", i,
			    _GMX_PORT_RD8(sc, octgmx_rx_adr_cam_regs[i]));
		ETHER_NEXT_MULTI(step, enm);
	}
	ETHER_UNLOCK(ec);
	CLR(ifp->if_flags, IFF_ALLMULTI);

	CNMAC_KASSERT(enm == NULL);

setmulti:
	/* XXX XXX XXX */
	if (ISSET(ifp->if_flags, IFF_ALLMULTI) ||
	    ISSET(ifp->if_flags, IFF_PROMISC)) {
		/* XXX XXX XXX */
		dprintf("accept all multicast\n");
		ctl |= __SHIFTIN(RXN_ADR_CTL_MCST_ACCEPT, RXN_ADR_CTL_MCST);
		/* XXX XXX XXX */
	} else if (multi) {
		/* XXX XXX XXX */
		dprintf("use cam\n");
		ctl |= __SHIFTIN(RXN_ADR_CTL_MCST_AFCAM, RXN_ADR_CTL_MCST);
		/* XXX XXX XXX */
	} else {
		/* XXX XXX XXX */
		dprintf("reject all multicast\n");
		ctl |= __SHIFTIN(RXN_ADR_CTL_MCST_REJECT, RXN_ADR_CTL_MCST);
		/* XXX XXX XXX */
	}
	/* XXX XXX XXX */

	/* XXX XXX XXX */
	if (ISSET(ifp->if_flags, IFF_PROMISC)) {
		cam_en = 0x00ULL;
	} else if (ISSET(ifp->if_flags, IFF_ALLMULTI)) {
		cam_en = 0x01ULL;
	}
	/* XXX XXX XXX */

	dprintf("ctl = %#lx, cam_en = %#lx\n", ctl, cam_en);
	_GMX_PORT_WR8(sc, GMX0_RX0_ADR_CTL, ctl);
	_GMX_PORT_WR8(sc, GMX0_RX0_ADR_CAM_EN, cam_en);

	octgmx_link_enable(sc, 1);

	return 0;
}

int
octgmx_port_enable(struct octgmx_port_softc *sc, int enable)
{
	(*sc->sc_port_ops->port_ops_enable)(sc, enable);
	return 0;
}

int
octgmx_reset_speed(struct octgmx_port_softc *sc)
{
	struct ifnet *ifp = &sc->sc_port_ec->ec_if;
	if (ISSET(sc->sc_port_mii->mii_flags, MIIF_DOINGAUTO)) {
		log(LOG_WARNING,
		    "%s: autonegotiation has not been completed yet\n",
		    ifp->if_xname);
		return 1;
	}
	(*sc->sc_port_ops->port_ops_speed)(sc);
	return 0;
}

int
octgmx_reset_timing(struct octgmx_port_softc *sc)
{
	(*sc->sc_port_ops->port_ops_timing)(sc);
	return 0;
}

int
octgmx_reset_flowctl(struct octgmx_port_softc *sc)
{
	struct ifmedia_entry *ife = sc->sc_port_mii->mii_media.ifm_cur;

	/*
	 * Get flow control negotiation result.
	 */
	if (IFM_SUBTYPE(ife->ifm_media) == IFM_AUTO &&
	    (sc->sc_port_mii->mii_media_active & IFM_ETH_FMASK) !=
			sc->sc_port_flowflags) {
		sc->sc_port_flowflags =
			sc->sc_port_mii->mii_media_active & IFM_ETH_FMASK;
		sc->sc_port_mii->mii_media_active &= ~IFM_ETH_FMASK;
	}

	/*
	 * 802.3x Flow Control Capabilities
	 */
	if (sc->sc_port_flowflags & IFM_ETH_TXPAUSE) {
		octgmx_tx_ovr_bp_enable(sc, 1);
	} else {
		octgmx_tx_ovr_bp_enable(sc, 0);
	}
	if (sc->sc_port_flowflags & IFM_ETH_RXPAUSE) {
		octgmx_rx_pause_enable(sc, 1);
	} else {
		octgmx_rx_pause_enable(sc, 0);
	}

	return 0;
}

static int
octgmx_rgmii_enable(struct octgmx_port_softc *sc, int enable)
{
	uint64_t mode;

	/* XXX XXX XXX */
	mode = _GMX_RD8(sc, GMX0_INF_MODE);
	if (ISSET(mode, INF_MODE_EN)) {
		octasx_enable(sc->sc_port_asx, 1);
	}
	/* XXX XXX XXX */
	return 0;
}

static int
octgmx_rgmii_speed(struct octgmx_port_softc *sc)
{
	struct ifnet *ifp = &sc->sc_port_ec->ec_if;
	uint64_t newlink;
	int baudrate;

	/* XXX XXX XXX */
	octgmx_link_enable(sc, 1);

	octgmx_rgmii_speed_newlink(sc, &newlink);
	if (sc->sc_link == newlink) {
		return 0;
	}
	sc->sc_link = newlink;

	switch (__SHIFTOUT(sc->sc_link, RXN_RX_INBND_SPEED)) {
	case RXN_RX_INBND_SPEED_2_5:
		baudrate = IF_Mbps(10);
		break;
	case RXN_RX_INBND_SPEED_25:
		baudrate = IF_Mbps(100);
		break;
	case RXN_RX_INBND_SPEED_125:
		baudrate = IF_Mbps(1000);
		break;
	default:
		baudrate = 0/* XXX */;
		panic("unable to get baudrate");
		break;
	}
	ifp->if_baudrate = baudrate;

	octgmx_link_enable(sc, 0);

	/*
	 * wait a max_packet_time
	 * max_packet_time(us) = (max_packet_size(bytes) * 8) / link_speed(Mbps)
	 */
	delay((GMX_FRM_MAX_SIZ * 8) / (baudrate / 1000000));

	octgmx_rgmii_speed_speed(sc);

	octgmx_link_enable(sc, 1);
	octasx_enable(sc->sc_port_asx, 1);

	return 0;
}

static int
octgmx_rgmii_speed_newlink(struct octgmx_port_softc *sc, uint64_t *rnewlink)
{
	uint64_t newlink;

	/* Inband status does not seem to work */
	newlink = _GMX_PORT_RD8(sc, GMX0_RX0_RX_INBND);

	*rnewlink = newlink;
	return 0;
}

static int
octgmx_rgmii_speed_speed(struct octgmx_port_softc *sc)
{
	uint64_t prt_cfg;
	uint64_t tx_clk, tx_slot, tx_burst;

	prt_cfg = _GMX_PORT_RD8(sc, GMX0_PRT0_CFG);

	switch (__SHIFTOUT(sc->sc_link, RXN_RX_INBND_SPEED)) {
	case RXN_RX_INBND_SPEED_2_5:
		/* 10Mbps */
		/*
		 * GMX Tx Clock Generation Registers
		 * 8ns x 50 = 400ns (2.5MHz TXC clock)
		 */
		tx_clk = 50;
		/*
		 * TX Slottime Counter Registers
		 * 10/100Mbps: set SLOT to 0x40
		 */
		tx_slot = 0x40;
		/*
		 * TX Burst-Counter Registers
		 * 10/100Mbps: set BURST to 0x0
		 */
		tx_burst = 0;
		/*
		 * GMX Tx Port Configuration Registers
		 * Slot time for half-duplex operation
		 *   0 = 512 bittimes (10/100Mbps operation)
		 */
		CLR(prt_cfg, PRTN_CFG_SLOTTIME);
		/*
		 * GMX Port Configuration Registers
		 * Link speed
		 *   0 = 10/100Mbps operation
		 *     in RGMII mode: GMX0_TX(0..2)_CLK[CLK_CNT] > 1
		 */
		CLR(prt_cfg, PRTN_CFG_SPEED);
		break;
	case RXN_RX_INBND_SPEED_25:
		/* 100Mbps */
		/*
		 * GMX Tx Clock Generation Registers
		 *  8ns x 5 = 40ns (25.0MHz TXC clock)
		 */
		tx_clk = 5;
		/*
		 * TX Slottime Counter Registers
		 *  10/100Mbps: set SLOT to 0x40
		 */
		tx_slot = 0x40;
		/*
		 * TX Burst-Counter Registers
		 *  10/100Mbps: set BURST to 0x0
		 */
		tx_burst = 0;
		/*
		 * GMX Tx Port Configuration Registers
		 *  Slot time for half-duplex operation
		 *    0 = 512 bittimes (10/100Mbps operation)
		 */
		CLR(prt_cfg, PRTN_CFG_SLOTTIME);
		/*
		 * GMX Port Configuration Registers
		 *  Link speed
		 *    0 = 10/100Mbps operation
		 *      in RGMII mode: GMX0_TX(0..2)_CLK[CLK_CNT] > 1
		 */
		CLR(prt_cfg, PRTN_CFG_SPEED);
		break;
	case RXN_RX_INBND_SPEED_125:
		/* 1000Mbps */
		/*
		 * GMX Tx Clock Generation Registers
		 *  8ns x 1 = 8ns (125.0MHz TXC clock)
		 */
		tx_clk = 1;
		/*
		 * TX Slottime Counter Registers
		 * > 1000Mbps: set SLOT to 0x200
		 */
		tx_slot = 0x200;
		/*
		 * "TX Burst-Counter Registers
		 * > 1000Mbps: set BURST to 0x2000
		 */
		tx_burst = 0x2000;
		/*
		 * GMX Tx Port Configuration Registers
		 *  Slot time for half-duplex operation
		 *    1 = 4096 bittimes (1000Mbps operation)
		 */
		SET(prt_cfg, PRTN_CFG_SLOTTIME);
		/*
		 * GMX Port Configuration Registers
		 *  Link speed
		 *    1 = 1000Mbps operation
		 */
		SET(prt_cfg, PRTN_CFG_SPEED);
		break;
	default:
		/* NOT REACHED! */
		/* Following configuration is default value of system.
		*/
		tx_clk = 1;
		tx_slot = 0x200;
		tx_burst = 0x2000;
		SET(prt_cfg, PRTN_CFG_SLOTTIME);
		SET(prt_cfg, PRTN_CFG_SPEED);
		break;
	}

	/* Setup Duplex mode(negotiated) */
	/*
	 * GMX Port Configuration Registers
	 *  Duplex mode: 0 = half-duplex mode, 1=full-duplex
	 */
	if (__SHIFTOUT(sc->sc_link, RXN_RX_INBND_DUPLEX)) {
		/* Full-Duplex */
		SET(prt_cfg, PRTN_CFG_DUPLEX);
	} else {
		/* Half-Duplex */
		CLR(prt_cfg, PRTN_CFG_DUPLEX);
	}

	_GMX_PORT_WR8(sc, GMX0_TX0_CLK, tx_clk);
	_GMX_PORT_WR8(sc, GMX0_TX0_SLOT, tx_slot);
	_GMX_PORT_WR8(sc, GMX0_TX0_BURST, tx_burst);
	_GMX_PORT_WR8(sc, GMX0_PRT0_CFG, prt_cfg);

	return 0;
}

static int
octgmx_rgmii_timing(struct octgmx_port_softc *sc)
{
	uint64_t rx_frm_ctl;

	/* RGMII TX Threshold Registers
	 * Number of 16-byte ticks to accumulate in the TX FIFO before
	 * sending on the RGMII interface. This field should be large
	 * enough to prevent underflow on the RGMII interface and must
	 * never be set to less than 0x4. This register cannot exceed
	 * the TX FIFO depth of 0x40 words.
	 */
	/* Default parameter of CN30XX */
	octgmx_tx_thresh(sc, 32);

	rx_frm_ctl = 0 |
	    /* RXN_FRM_CTL_NULL_DIS |	(cn5xxx only) */
	    /* RXN_FRM_CTL_PRE_ALIGN |	(cn5xxx only) */
	    /* RXN_FRM_CTL_PAD_LEN |	(cn3xxx only) */
	    /* RXN_FRM_CTL_VLAN_LEN |	(cn3xxx only) */
	    RXN_FRM_CTL_PRE_FREE |
	    RXN_FRM_CTL_CTL_SMAC |
	    RXN_FRM_CTL_CTL_MCST |
	    RXN_FRM_CTL_CTL_DRP |
	    RXN_FRM_CTL_PRE_STRP |
	    RXN_FRM_CTL_PRE_CHK;
	octgmx_rx_frm_ctl_enable(sc, rx_frm_ctl);

	/* RGMII RX Clock-Delay Registers
	 * Delay setting to place n RXC (RGMII receive clock) delay line.
	 * The intrinsic delay can range from 50ps to 80ps per tap,
	 * which corresponds to skews of 1.25ns to 2.00ns at 25 taps(CSR+1).
	 * This is the best match for the RGMII specification which wants
	 * 1ns - 2.6ns of skew.
	 */
	/* RGMII TX Clock-Delay Registers
	 * Delay setting to place n TXC (RGMII transmit clock) delay line.
	 */

	octasx_clk_set(sc->sc_port_asx,
			   sc->sc_clk_tx_setting, sc->sc_clk_rx_setting);

	return 0;
}

static int
octgmx_sgmii_enable(struct octgmx_port_softc *sc, int enable)
{
	uint64_t ctl_reg, status, timer_count;
	uint64_t cpu_freq_mhz = curcpu()->ci_cpu_freq / 1000000;
	int done;
	int i;

	if (!enable)
		return 0;

	/* Set link timer interval to 1.6ms.  Timer multiple is 1024 (2^10). */
	/*
	 * XXX Should set timer to 10ms if not in SGMII mode (ie,
	 * "cavium,sgmii-mac-1000x-mode" property exists
	 */
	timer_count = PCS_READ_8(sc, PCS_LINK_TIMER_COUNT);
	CLR(timer_count, PCS_LINK_TIMER_COUNT_MASK);
	SET(timer_count,
	    __SHIFTIN((1600 * cpu_freq_mhz) >> 10, PCS_LINK_TIMER_COUNT_MASK));
	PCS_WRITE_8(sc, PCS_LINK_TIMER_COUNT, timer_count);

	/* Reset the PCS. */
	ctl_reg = PCS_READ_8(sc, PCS_MR_CONTROL);
	SET(ctl_reg, PCS_MR_CONTROL_RESET);
	PCS_WRITE_8(sc, PCS_MR_CONTROL, ctl_reg);

	/* Wait for the reset to complete. */
	done = 0;
	for (i = 0; i < 1000000; i++) {
		ctl_reg = PCS_READ_8(sc, PCS_MR_CONTROL);
		if (!ISSET(ctl_reg, PCS_MR_CONTROL_RESET)) {
			done = 1;
			break;
		}
	}
	if (!done) {
		printf("SGMII reset timeout on port %d\n", sc->sc_port_no);
		return 1;
	}

	/* Start a new SGMII autonegotiation. */
	SET(ctl_reg, PCS_MR_CONTROL_AN_EN);
	SET(ctl_reg, PCS_MR_CONTROL_RST_AN);
	CLR(ctl_reg, PCS_MR_CONTROL_PWR_DN);
	PCS_WRITE_8(sc, PCS_MR_CONTROL, ctl_reg);

	/* Wait for the SGMII autonegotiation to complete. */
	done = 0;
	for (i = 0; i < 1000000; i++) {
		status = PCS_READ_8(sc, PCS_MR_STATUS);
		if (ISSET(status, PCS_MR_STATUS_AN_CPT)) {
			done = 1;
			break;
		}
	}
	if (!done) {
		printf("SGMII autonegotiation timeout on port %d\n",
		    sc->sc_port_no);
		return 1;
	}

	return 0;
}

static int
octgmx_sgmii_speed(struct octgmx_port_softc *sc)
{
	uint64_t misc_ctl, prt_cfg;
	int tx_burst, tx_slot;

	octgmx_link_enable(sc, 0);

	prt_cfg = _GMX_PORT_RD8(sc, GMX0_PRT0_CFG);

	if (ISSET(sc->sc_port_mii->mii_media_active, IFM_FDX))
		SET(prt_cfg, PRTN_CFG_DUPLEX);
	else
		CLR(prt_cfg, PRTN_CFG_DUPLEX);

	misc_ctl = PCS_READ_8(sc, PCS_MISC_CTL);
	CLR(misc_ctl, PCS_MISC_CTL_SAMP_PT);

	/* Disable the GMX port if the link is down. */
	if (octgmx_link_status(sc))
		CLR(misc_ctl, PCS_MISC_CTL_GMXENO);
	else
		SET(misc_ctl, PCS_MISC_CTL_GMXENO);

	switch (sc->sc_port_ec->ec_if.if_baudrate) {
	case IF_Mbps(10):
		tx_slot = 0x40;
		tx_burst = 0;
		CLR(prt_cfg, PRTN_CFG_SPEED);
		SET(prt_cfg, PRTN_CFG_SPEED_MSB);
		CLR(prt_cfg, PRTN_CFG_SLOTTIME);
		misc_ctl |= 25 & PCS_MISC_CTL_SAMP_PT;
		break;
	case IF_Mbps(100):
		tx_slot = 0x40;
		tx_burst = 0;
		CLR(prt_cfg, PRTN_CFG_SPEED);
		CLR(prt_cfg, PRTN_CFG_SPEED_MSB);
		CLR(prt_cfg, PRTN_CFG_SLOTTIME);
		misc_ctl |= 5 & PCS_MISC_CTL_SAMP_PT;
		break;
	case IF_Gbps(1):
	default:
		tx_slot = 0x200;
		tx_burst = 0x2000;
		SET(prt_cfg, PRTN_CFG_SPEED);
		CLR(prt_cfg, PRTN_CFG_SPEED_MSB);
		SET(prt_cfg, PRTN_CFG_SLOTTIME);
		misc_ctl |= 1 & PCS_MISC_CTL_SAMP_PT;
		break;
	}

	PCS_WRITE_8(sc, PCS_MISC_CTL, misc_ctl);

	_GMX_PORT_WR8(sc, GMX0_TX0_SLOT, tx_slot);
	_GMX_PORT_WR8(sc, GMX0_TX0_BURST, tx_burst);
	_GMX_PORT_WR8(sc, GMX0_PRT0_CFG, prt_cfg);

	octgmx_link_enable(sc, 1);

	return 0;
}

static int
octgmx_sgmii_timing(struct octgmx_port_softc *sc)
{
	uint64_t rx_frm_ctl;

	octgmx_tx_thresh(sc, 32);

	rx_frm_ctl =
	    RXN_FRM_CTL_PRE_FREE |
	    RXN_FRM_CTL_CTL_SMAC |
	    RXN_FRM_CTL_CTL_MCST |
	    RXN_FRM_CTL_CTL_DRP |
	    RXN_FRM_CTL_PRE_STRP |
	    RXN_FRM_CTL_PRE_CHK;
	octgmx_rx_frm_ctl_enable(sc, rx_frm_ctl);

	return 0;
}

void
octgmx_stats(struct octgmx_port_softc *sc)
{
	struct ifnet *ifp = &sc->sc_port_ec->ec_if;
	uint64_t tmp;

	/*
	 *  GMX0_RX0_STATS_PKTS is not count.
         *  input packet is counted when recepted packet in if_cnmac.
         */
	/*
         *  GMX0_RX0_STATS_PKTS_BAD count is included
         *  receive error of work queue entry.
         *  this is not add to input packet errors of interface.
         */
	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);
	if_statadd_ref(nsr, if_iqdrops,
	    (uint32_t)_GMX_PORT_RD8(sc, GMX0_RX0_STATS_PKTS_DRP));
	if_statadd_ref(nsr, if_opackets,
	    (uint32_t)_GMX_PORT_RD8(sc, GMX0_TX0_STAT3));

	tmp = _GMX_PORT_RD8(sc, GMX0_TX0_STAT0);
	if_statadd_ref(nsr, if_oerrors,
	    (uint32_t)tmp + ((uint32_t)(tmp >> 32) * 16));
	if_statadd_ref(nsr, if_collisions, (uint32_t)tmp);

	tmp = _GMX_PORT_RD8(sc, GMX0_TX0_STAT1);
	if_statadd_ref(nsr, if_collisions,
	    (uint32_t)tmp + (uint32_t)(tmp >> 32));

	tmp = _GMX_PORT_RD8(sc, GMX0_TX0_STAT9);
	if_statadd_ref(nsr, if_oerrors, (uint32_t)(tmp >> 32));
	IF_STAT_PUTREF(ifp);
}
