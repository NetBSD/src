/*	$NetBSD: wivar.h,v 1.33 2003/05/16 01:26:18 dyoung Exp $	*/

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
 */

/*
 * FreeBSD driver ported to NetBSD by Bill Sommerfeld in the back of the
 * Oslo IETF plenary meeting.
 */
struct wi_softc	{
	struct device		sc_dev;
	struct ieee80211com	sc_ic;
	void			*sc_ih;		/* interrupt handler */
	int			(*sc_enable)(struct wi_softc *);
	void			(*sc_disable)(struct wi_softc *);
	void			(*sc_reset)(struct wi_softc *);

	int			sc_attached;
	int			sc_enabled;
	int			sc_firmware_type;
#define	WI_NOTYPE	0
#define	WI_LUCENT	1
#define	WI_INTERSIL	2
#define	WI_SYMBOL	3
	int			sc_pri_firmware_ver;	/* Primary firm vers */
	int			sc_sta_firmware_ver;	/* Station firm vers */
	int			sc_pci;			/* attach to PCI-Bus */

	bus_space_tag_t		sc_iot;			/* bus cookie */
	bus_space_handle_t	sc_ioh;			/* bus i/o handle */

	struct ifmedia		sc_media;
	caddr_t			sc_drvbpf;
	int			sc_flags;
	int			sc_bap_id;
	int			sc_bap_off;

	u_int16_t		sc_portnum;

	u_int16_t		sc_dbm_adjust;
	u_int16_t		sc_max_datalen;
	u_int16_t		sc_frag_thresh;
	u_int16_t		sc_rts_thresh;
	u_int16_t		sc_system_scale;
	u_int16_t		sc_tx_rate;
	u_int16_t		sc_cnfauthmode;
	u_int16_t		sc_roaming_mode;
	u_int16_t		sc_microwave_oven;

	int			sc_nodelen;
	char			sc_nodename[IEEE80211_NWID_LEN];

	int			sc_buflen;
#define	WI_NTXBUF	3
	struct sc_txdesc {
		int		d_fid;
		int		d_len;
	}			sc_txd[WI_NTXBUF];
	int			sc_txnext;
	int			sc_txcur;
	int			sc_tx_timer;
	int			sc_scan_timer;
	int			sc_syn_timer;

	struct wi_counters	sc_stats;
	u_int16_t		sc_ibss_port;

	struct wi_apinfo	sc_aps[MAXAPINFO];
	int 			sc_naps;

	int			sc_false_syns;

	u_int16_t		sc_txbuf[IEEE80211_MAX_LEN/2];
};

#define	sc_if			sc_ic.ic_if

/* maximum consecutive false change-of-BSSID indications */
#define	WI_MAX_FALSE_SYNS		10	

#define	WI_SCAN_INQWAIT			3	/* wait sec before inquire */
#define	WI_SCAN_WAIT			5	/* maximum scan wait */

/* Values for wi_flags. */
#define	WI_FLAGS_ATTACHED		0x0001
#define	WI_FLAGS_INITIALIZED		0x0002
#define	WI_FLAGS_OUTRANGE		0x0004
#define	WI_FLAGS_HAS_MOR		0x0010
#define	WI_FLAGS_HAS_ROAMING		0x0020
#define	WI_FLAGS_HAS_DIVERSITY		0x0040
#define	WI_FLAGS_HAS_SYSSCALE		0x0080
#define	WI_FLAGS_BUG_AUTOINC		0x0100
#define	WI_FLAGS_HAS_FRAGTHR		0x0200
#define	WI_FLAGS_HAS_DBMADJUST		0x0400

struct wi_card_ident {
	u_int16_t	card_id;
	char		*card_name;
	u_int8_t	firm_type;
};

/*
 * register space access macros
 */
#ifdef WI_AT_BIGENDIAN_BUS_HACK
	/*
	 * XXX - ugly hack for sparc bus_space_* macro deficiencies:
	 *       assume the bus we are accessing is big endian.
	 */

#define CSR_WRITE_4(sc, reg, val)	\
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg) , htole32(val))
#define CSR_WRITE_2(sc, reg, val)	\
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg), htole16(val))
#define CSR_WRITE_1(sc, reg, val)	\
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg), val)

#define CSR_READ_4(sc, reg)		\
	le32toh(bus_space_read_4(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg)))
#define CSR_READ_2(sc, reg)		\
	le16toh(bus_space_read_2(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg)))
#define CSR_READ_1(sc, reg)		\
	bus_space_read_1(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg))

#else

#define CSR_WRITE_4(sc, reg, val)	\
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg) , val)
#define CSR_WRITE_2(sc, reg, val)	\
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg), val)
#define CSR_WRITE_1(sc, reg, val)	\
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg), val)

#define CSR_READ_4(sc, reg)		\
	bus_space_read_4(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg))
#define CSR_READ_2(sc, reg)		\
	bus_space_read_2(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg))
#define CSR_READ_1(sc, reg)		\
	bus_space_read_1(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg))
#endif

#ifndef __BUS_SPACE_HAS_STREAM_METHODS
#define bus_space_write_stream_2	bus_space_write_2
#define bus_space_write_multi_stream_2	bus_space_write_multi_2
#define bus_space_read_stream_2		bus_space_read_2
#define bus_space_read_multi_stream_2		bus_space_read_multi_2
#endif

#define CSR_WRITE_STREAM_2(sc, reg, val)	\
	bus_space_write_stream_2(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg), val)
#define CSR_WRITE_MULTI_STREAM_2(sc, reg, val, count)	\
	bus_space_write_multi_stream_2(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg), val, count)
#define CSR_READ_STREAM_2(sc, reg)		\
	bus_space_read_stream_2(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg))
#define CSR_READ_MULTI_STREAM_2(sc, reg, buf, count)		\
	bus_space_read_multi_stream_2(sc->sc_iot, sc->sc_ioh,	\
			(sc->sc_pci? reg * 2: reg), buf, count)


int	wi_attach(struct wi_softc *);
int	wi_detach(struct wi_softc *);
int	wi_activate(struct device *, enum devact);
int	wi_intr(void *arg);
void	wi_power(struct wi_softc *, int);
void	wi_shutdown(struct wi_softc *);
