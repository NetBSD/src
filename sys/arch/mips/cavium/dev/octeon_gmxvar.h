/*	$NetBSD: octeon_gmxvar.h,v 1.2 2018/04/19 21:50:06 christos Exp $	*/

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

enum OCTEON_ETH_QUIRKS {
	OCTEON_ETH_QUIRKS_NO_RX_INBND = 1 << 0,
	OCTEON_ETH_QUIRKS_NO_PRE_ALIGN = 1 << 1,
};

#if 1
struct octeon_gmx_softc;
struct octeon_gmx_port_softc;

struct octeon_gmx_port_softc {
	struct octeon_gmx_softc	*sc_port_gmx;
	bus_space_handle_t	sc_port_regh;
	int			sc_port_no;	/* GMX0:0, GMX0:1, ... */
	int			sc_port_type;
	uint64_t		sc_mac;
	int			sc_quirks;
	uint64_t		sc_link;
	struct mii_data		*sc_port_mii;
	struct ethercom		*sc_port_ec;
	struct octeon_gmx_port_ops
				*sc_port_ops;
	struct octeon_asx_softc	*sc_port_asx;
	struct octeon_ipd_softc	*sc_ipd;
	int			sc_port_flowflags;

#if defined(OCTEON_DEBUG) || defined(OCTEON_ETH_DEBUG)
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

struct octeon_gmx_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	int			sc_unitno;	/* GMX0, GMX1, ... */
	int			sc_nports;
	int			sc_port_types[4/* XXX */];

	struct octeon_gmx_port_softc
				*sc_ports;

#if defined(OCTEON_DEBUG) || defined(OCTEON_ETH_DEBUG)
	struct evcnt		sc_ev_latecol;
	struct evcnt		sc_ev_xsdef;
	struct evcnt		sc_ev_xscol;
	struct evcnt		sc_ev_undflw;
	struct evcnt		sc_ev_pkonxa;
#endif
};
#endif

struct octeon_gmx_attach_args {
        bus_space_tag_t         ga_regt;
	bus_addr_t		ga_addr;
	const char		*ga_name;
	int			ga_portno;
	int			ga_port_type;

	struct octeon_gmx_softc *ga_gmx;
	struct octeon_gmx_port_softc
				*ga_gmx_port;
};

#define	CN30XXGMX_FILTER_NADDRS_MAX	8	/* XXX elsewhere */

enum CN30XXGMX_FILTER_POLICY {
	CN30XXGMX_FILTER_POLICY_ACCEPT_ALL,
	CN30XXGMX_FILTER_POLICY_ACCEPT,
	CN30XXGMX_FILTER_POLICY_REJECT,
	CN30XXGMX_FILTER_POLICY_REJECT_ALL
};

int		octeon_gmx_link_enable(struct octeon_gmx_port_softc *, int);
int		octeon_gmx_tx_stats_rd_clr(struct octeon_gmx_port_softc *, int);
int		octeon_gmx_rx_stats_rd_clr(struct octeon_gmx_port_softc *, int);
void		octeon_gmx_rx_stats_dec_bad(struct octeon_gmx_port_softc *);
int		octeon_gmx_stats_init(struct octeon_gmx_port_softc *);
void		octeon_gmx_tx_int_enable(struct octeon_gmx_port_softc *, int);
void		octeon_gmx_rx_int_enable(struct octeon_gmx_port_softc *, int);
int		octeon_gmx_setfilt(struct octeon_gmx_port_softc *,
		    enum CN30XXGMX_FILTER_POLICY, size_t, uint8_t **);
int		octeon_gmx_rx_frm_ctl_enable(struct octeon_gmx_port_softc *,
		    uint64_t rx_frm_ctl);
int		octeon_gmx_rx_frm_ctl_disable(struct octeon_gmx_port_softc *,
		    uint64_t rx_frm_ctl);
int		octeon_gmx_tx_thresh(struct octeon_gmx_port_softc *, int);
int		octeon_gmx_set_mac_addr(struct octeon_gmx_port_softc *, uint8_t *);
int		octeon_gmx_set_filter(struct octeon_gmx_port_softc *);
int		octeon_gmx_port_enable(struct octeon_gmx_port_softc *, int);
int		octeon_gmx_reset_speed(struct octeon_gmx_port_softc *);
int		octeon_gmx_reset_flowctl(struct octeon_gmx_port_softc *);
int		octeon_gmx_reset_timing(struct octeon_gmx_port_softc *);
int		octeon_gmx_reset_board(struct octeon_gmx_port_softc *);
void		octeon_gmx_stats(struct octeon_gmx_port_softc *);
uint64_t	octeon_gmx_get_rx_int_reg(struct octeon_gmx_port_softc *sc);
uint64_t	octeon_gmx_get_tx_int_reg(struct octeon_gmx_port_softc *sc);
static __inline int	octeon_gmx_link_status(struct octeon_gmx_port_softc *);

/* XXX RGMII specific */
static __inline int
octeon_gmx_link_status(struct octeon_gmx_port_softc *sc)
{
	return (sc->sc_link & RXN_RX_INBND_STATUS) ? 1 : 0;
}

#endif
