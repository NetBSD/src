/*	$NetBSD: anvar.h,v 1.4 2000/12/19 08:00:56 onoe Exp $	*/
/*
 * Copyright (c) 1997, 1998, 1999
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/an/if_aironet_ieee.h,v 1.2 2000/11/13 23:04:12 wpaul Exp $
 */

#ifndef _DEV_IC_ANVAR_H
#define _DEV_IC_ANVAR_H

/*
 * This header defines a simple command interface to the FreeBSD
 * Aironet driver (an) driver, which is used to set certain
 * device-specific parameters which can't be easily managed through
 * ifconfig(8). No, sysctl(2) is not the answer. I said a _simple_
 * interface, didn't I.
 */

#ifndef SIOCSAIRONET
#define SIOCSAIRONET	SIOCSIFGENERIC
#endif

#ifndef SIOCGAIRONET
#define SIOCGAIRONET	SIOCGIFGENERIC
#endif

#define AN_TIMEOUT	65536

/* Default network name: ANY */
#define AN_DEFAULT_NETNAME	""

/* The nodename must be less than 16 bytes */
#define AN_DEFAULT_NODENAME	"NetBSD"

#define AN_DEFAULT_IBSS		"NetBSD IBSS"

/*
 * register space access macros
 */
#define CSR_WRITE_2(sc, reg, val)	\
	bus_space_write_2(sc->an_btag, sc->an_bhandle, reg, val)

#define CSR_READ_2(sc, reg)		\
	bus_space_read_2(sc->an_btag, sc->an_bhandle, reg)

#define CSR_WRITE_1(sc, reg, val)	\
	bus_space_write_1(sc->an_btag, sc->an_bhandle, reg, val)

#define CSR_READ_1(sc, reg)		\
	bus_space_read_1(sc->an_btag, sc->an_bhandle, reg)

/*
 * Technically I don't think there's a limit to a record
 * length. The largest record is the one that contains the CIS
 * data, which is 240 words long, so 256 should be a safe
 * value.
 */
#define AN_MAX_DATALEN	512

struct an_req {
	u_int16_t	an_len;
	u_int16_t	an_type;
	u_int16_t	an_val[AN_MAX_DATALEN];
};

/*
 * Private LTV records (interpreted only by the driver). This is
 * a minor kludge to allow reading the interface statistics from
 * the driver.
 */
#define AN_RID_IFACE_STATS	0x0100
#define AN_RID_MGMT_XMIT	0x0200
#define AN_RID_ZERO_CACHE	0x0300
#define AN_RID_READ_CACHE	0x0400
#define AN_RID_TX_SPEED		0x1234

/* 
 * Aironet IEEE signal strength cache
 *
 * driver keeps cache of last
 * MAXANCACHE packets to arrive including signal strength info.
 * daemons may read this via ioctl
 *
 * Each entry in the wi_sigcache has a unique macsrc.
 */
#define MAXANCACHE      10

struct an_sigcache {
	char	macsrc[6];	/* unique MAC address for entry */
	int	ipsrc;		/* ip address associated with packet */
	int	signal;		/* signal strength of the packet */
	int	noise;		/* noise value */
	int	quality;	/* quality of the packet */
};

#define AN_TXGAP_80211		0
#define AN_TXGAP_8023		0

#define AN_TX_RING_CNT		4
#define AN_INC(x, y)		(x) = (x + 1) % y
 
struct an_tx_ring_data {        
	u_int16_t		an_tx_fids[AN_TX_RING_CNT];
	u_int16_t		an_tx_ring[AN_TX_RING_CNT];
	int			an_tx_prod;
	int			an_tx_cons;
};

struct an_softc	{
	struct device		an_dev;
	struct ethercom		arpcom;
	int			(*sc_enable) __P((struct an_softc *));
	void			(*sc_disable) __P((struct an_softc *));
	int			sc_enabled;
	int			sc_invalid;
	struct ifmedia		sc_media;

	bus_space_handle_t	an_bhandle;
	bus_space_tag_t		an_btag;
	struct an_ltv_genconfig	an_config;
	struct an_ltv_caps	an_caps;
	struct an_ltv_ssidlist	an_ssidlist;
	struct an_ltv_aplist	an_aplist;
        struct an_ltv_wepkey	an_temp_keys;
        struct an_ltv_wepkey	an_perm_keys;
	int			an_tx_rate;
	int			an_rxmode;
	int			an_if_flags;
	u_int8_t		an_txbuf[1536];
	struct an_tx_ring_data	an_rdata;
	struct an_ltv_stats	an_stats;
	struct an_ltv_status	an_status;
	u_int8_t		an_associated;
	int			an_sigitems;
	struct an_sigcache	an_sigcache[MAXANCACHE];
	int			an_nextitem;
	struct callout		an_stat_ch;
};

int	an_attach		__P((struct an_softc *));
int	an_detach		__P((struct an_softc *));
int	an_activate		__P((struct device *, enum devact));
void	an_power	        __P((int, void *));
void	an_shutdown	        __P((void *));
int	an_intr			__P((void *));

#endif	/* _DEV_IC_ANVAR_H */
