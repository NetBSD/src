/*	$NetBSD: octeon_gmxvar.h,v 1.4 2020/05/31 06:27:06 simonb Exp $	*/

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

#ifndef _OCTEON_GMXVAR_H_
#define _OCTEON_GMXVAR_H_

#include "opt_octeon.h"

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#define GMX_MII_PORT	1
#define GMX_GMII_PORT	2
#define GMX_RGMII_PORT	3
#define GMX_SPI42_PORT	4

#define GMX_FRM_MAX_SIZ	0x600

enum CNMAC_QUIRKS {
	CNMAC_QUIRKS_NO_RX_INBND = 1 << 0,
	CNMAC_QUIRKS_NO_PRE_ALIGN = 1 << 1,
};

#if 1
struct octgmx_softc;
struct octgmx_port_softc;

struct octgmx_port_softc {
	struct octgmx_softc	*sc_port_gmx;
	bus_space_handle_t	sc_port_regh;
	int			sc_port_no;	/* GMX0:0, GMX0:1, ... */
	int			sc_port_type;
	uint64_t		sc_mac;
	int			sc_quirks;
	uint64_t		sc_link;
	struct mii_data		*sc_port_mii;
	struct ethercom		*sc_port_ec;
	struct octgmx_port_ops
				*sc_port_ops;
	struct octasx_softc	*sc_port_asx;
	struct octipd_softc	*sc_ipd;
	int			sc_port_flowflags;

	int			sc_clk_tx_setting;
	int			sc_clk_rx_setting;

#if defined(OCTEON_DEBUG) || defined(CNMAC_DEBUG)
#if 0
	/* XXX */
	struct evcnt		sc_ev_pausedrp;
	struct evcnt		sc_ev_phydupx;
	struct evcnt		sc_ev_physpd;
	struct evcnt		sc_ev_phylink;
#endif
	struct evcnt		sc_ev_ifgerr;
	struct evcnt		sc_ev_coldet;
	struct evcnt		sc_ev_falerr;
	struct evcnt		sc_ev_rcverr;
	struct evcnt		sc_ev_rsverr;
	struct evcnt		sc_ev_pckterr;
	struct evcnt		sc_ev_ovrerr;
	struct evcnt		sc_ev_niberr;
	struct evcnt		sc_ev_skperr;
	struct evcnt		sc_ev_lenerr;
	struct evcnt		sc_ev_alnerr;
	struct evcnt		sc_ev_fcserr;
	struct evcnt		sc_ev_jabber;
	struct evcnt		sc_ev_maxerr;
	struct evcnt		sc_ev_carext;
	struct evcnt		sc_ev_minerr;
#endif
};

struct octgmx_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	int			sc_unitno;	/* GMX0, GMX1, ... */
	int			sc_nports;
	int			sc_port_types[4/* XXX */];

	struct octgmx_port_softc
				*sc_ports;

#if defined(OCTEON_DEBUG) || defined(CNMAC_DEBUG)
	struct evcnt		sc_ev_latecol;
	struct evcnt		sc_ev_xsdef;
	struct evcnt		sc_ev_xscol;
	struct evcnt		sc_ev_undflw;
	struct evcnt		sc_ev_pkonxa;
#endif
};
#endif

struct octgmx_attach_args {
        bus_space_tag_t         ga_regt;
	bus_addr_t		ga_addr;
	const char		*ga_name;
	int			ga_portno;
	int			ga_port_type;

	struct octgmx_softc	*ga_gmx;
	struct octgmx_port_softc
				*ga_gmx_port;
};

#define	CN30XXGMX_FILTER_NADDRS_MAX	8	/* XXX elsewhere */

enum CN30XXGMX_FILTER_POLICY {
	CN30XXGMX_FILTER_POLICY_ACCEPT_ALL,
	CN30XXGMX_FILTER_POLICY_ACCEPT,
	CN30XXGMX_FILTER_POLICY_REJECT,
	CN30XXGMX_FILTER_POLICY_REJECT_ALL
};

int		octgmx_link_enable(struct octgmx_port_softc *, int);
int		octgmx_tx_stats_rd_clr(struct octgmx_port_softc *, int);
int		octgmx_rx_stats_rd_clr(struct octgmx_port_softc *, int);
void		octgmx_rx_stats_dec_bad(struct octgmx_port_softc *);
int		octgmx_stats_init(struct octgmx_port_softc *);
void		octgmx_tx_int_enable(struct octgmx_port_softc *, int);
void		octgmx_rx_int_enable(struct octgmx_port_softc *, int);
int		octgmx_setfilt(struct octgmx_port_softc *,
		    enum CN30XXGMX_FILTER_POLICY, size_t, uint8_t **);
int		octgmx_rx_frm_ctl_enable(struct octgmx_port_softc *, uint64_t);
int		octgmx_rx_frm_ctl_disable(struct octgmx_port_softc *, uint64_t);
int		octgmx_tx_thresh(struct octgmx_port_softc *, int);
int		octgmx_set_mac_addr(struct octgmx_port_softc *, uint8_t *);
int		octgmx_set_filter(struct octgmx_port_softc *);
int		octgmx_port_enable(struct octgmx_port_softc *, int);
int		octgmx_reset_speed(struct octgmx_port_softc *);
int		octgmx_reset_flowctl(struct octgmx_port_softc *);
int		octgmx_reset_timing(struct octgmx_port_softc *);
int		octgmx_reset_board(struct octgmx_port_softc *);
void		octgmx_stats(struct octgmx_port_softc *);
uint64_t	octgmx_get_rx_int_reg(struct octgmx_port_softc *sc);
uint64_t	octgmx_get_tx_int_reg(struct octgmx_port_softc *sc);
static __inline int	octgmx_link_status(struct octgmx_port_softc *);

/* XXX RGMII specific */
static __inline int
octgmx_link_status(struct octgmx_port_softc *sc)
{
	return (sc->sc_link & RXN_RX_INBND_STATUS) ? 1 : 0;
}

#endif
