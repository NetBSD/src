/*	$NetBSD: if_smap.c,v 1.9.32.1 2008/01/02 21:49:01 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: if_smap.c,v 1.9.32.1 2008/01/02 21:49:01 bouyer Exp $");

#include "debug_playstation2.h"

#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/syslog.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <playstation2/ee/eevar.h>

#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <playstation2/dev/spdvar.h>
#include <playstation2/dev/spdreg.h>
#include <playstation2/dev/emac3var.h>
#include <playstation2/dev/if_smapreg.h>

#ifdef SMAP_DEBUG
#include <playstation2/ee/gsvar.h>
int	smap_debug = 0;
#define	DPRINTF(fmt, args...)						\
	if (smap_debug)							\
		printf("%s: " fmt, __func__ , ##args) 
#define	DPRINTFN(n, arg)						\
	if (smap_debug > (n))						\
		printf("%s: " fmt, __func__ , ##args) 
#define STATIC
struct smap_softc *__sc;
void __smap_status(int);
void __smap_lock_check(const char *, int);
#define FUNC_ENTER()	__smap_lock_check(__func__, 1)
#define FUNC_EXIT()	__smap_lock_check(__func__, 0)
#else
#define	DPRINTF(arg...)		((void)0)
#define DPRINTFN(n, arg...)	((void)0)
#define STATIC			static
#define FUNC_ENTER()		((void)0)
#define FUNC_EXIT()		((void)0)
#endif

struct smap_softc {
	struct emac3_softc emac3;
	struct ethercom ethercom;

	u_int32_t *tx_buf;
	u_int32_t *rx_buf;
	struct smap_desc *tx_desc;
	struct smap_desc *rx_desc;

#define	SMAP_FIFO_ALIGN		4
	int tx_buf_freesize;	/* buffer usage */
	int tx_desc_cnt;	/* descriptor usage */
	u_int16_t tx_fifo_ptr;
	int tx_done_index, tx_start_index;
	int rx_done_index;

#if NRND > 0
	rndsource_element_t rnd_source;
#endif
};

#define DEVNAME		(sc->emac3.dev.dv_xname)
#define ROUND4(x)	(((x) + 3) & ~3)
#define ROUND16(x)	(((x) + 15) & ~15)

STATIC int smap_match(struct device *, struct cfdata *, void *);
STATIC void smap_attach(struct device *, struct device *, void *);

CFATTACH_DECL(smap, sizeof (struct smap_softc),
    smap_match, smap_attach, NULL, NULL);

STATIC int smap_intr(void *);
STATIC void smap_rxeof(void *);
STATIC void smap_txeof(void *);
STATIC void smap_start(struct ifnet *);
STATIC void smap_watchdog(struct ifnet *);
STATIC int smap_ioctl(struct ifnet *, u_long, void *);
STATIC int smap_init(struct ifnet *);
STATIC void smap_stop(struct ifnet *, int);

STATIC int smap_get_eaddr(struct smap_softc *, u_int8_t *);
STATIC int smap_fifo_init(struct smap_softc *);
STATIC int smap_fifo_reset(bus_addr_t);
STATIC void smap_desc_init(struct smap_softc *);

int
smap_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct spd_attach_args *spa = aux;

	if (spa->spa_slot != SPD_NIC)
		return (0);

	return (1);
}

void
smap_attach(struct device *parent, struct device *self, void *aux)
{
	struct spd_attach_args *spa = aux;
	struct smap_softc *sc = (void *)self;
	struct emac3_softc *emac3 = &sc->emac3;
	struct ifnet *ifp = &sc->ethercom.ec_if;
	struct mii_data *mii = &emac3->mii;
	void *txbuf, *rxbuf;
	u_int16_t r;

#ifdef SMAP_DEBUG
	__sc = sc;
#endif

	printf(": %s\n", spa->spa_product_name);

	/* SPD EEPROM */
	if (smap_get_eaddr(sc, emac3->eaddr) != 0)
		return;

	printf("%s: Ethernet address %s\n", DEVNAME,
	    ether_sprintf(emac3->eaddr));

	/* disable interrupts */
	r = _reg_read_2(SPD_INTR_ENABLE_REG16);
	r &= ~(SPD_INTR_RXEND | SPD_INTR_TXEND | SPD_INTR_RXDNV |
	    SPD_INTR_EMAC3);
	_reg_write_2(SPD_INTR_ENABLE_REG16, r);
	emac3_intr_disable();

	/* clear pending interrupts */
	_reg_write_2(SPD_INTR_CLEAR_REG16, SPD_INTR_RXEND | SPD_INTR_TXEND |
	    SPD_INTR_RXDNV);
	emac3_intr_clear();

	/* buffer descriptor mode */
	_reg_write_1(SMAP_DESC_MODE_REG8, 0);

	if (smap_fifo_init(sc) != 0)
		return;

	if (emac3_init(&sc->emac3) != 0)
		return;
	emac3_intr_disable();
	emac3_disable();

	smap_desc_init(sc);

	/* allocate temporary buffer */
	txbuf = malloc(ETHER_MAX_LEN - ETHER_CRC_LEN + SMAP_FIFO_ALIGN + 16,
	    M_DEVBUF, M_NOWAIT);
	if (txbuf == NULL) {
		printf("%s: no memory.\n", DEVNAME);		
		return;
	}

	rxbuf = malloc(ETHER_MAX_LEN + SMAP_FIFO_ALIGN + 16,
	    M_DEVBUF, M_NOWAIT);
	if (rxbuf == NULL) {
		printf("%s: no memory.\n", DEVNAME);
		free(txbuf, M_DEVBUF);
		return;
	}

	sc->tx_buf = (u_int32_t *)ROUND16((vaddr_t)txbuf);
	sc->rx_buf = (u_int32_t *)ROUND16((vaddr_t)rxbuf);

	/* 
	 * setup MI layer 
	 */
	strcpy(ifp->if_xname, DEVNAME);
	ifp->if_softc	= sc;
	ifp->if_start	= smap_start;
	ifp->if_ioctl	= smap_ioctl;
	ifp->if_init	= smap_init;
	ifp->if_stop	= smap_stop;
	ifp->if_watchdog= smap_watchdog;
	ifp->if_flags	= IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS |
	    IFF_MULTICAST;
	IFQ_SET_READY(&ifp->if_snd);

	/* ifmedia setup. */
	mii->mii_ifp		= ifp;
	mii->mii_readreg	= emac3_phy_readreg;
	mii->mii_writereg	= emac3_phy_writereg;
	mii->mii_statchg	= emac3_phy_statchg;
	ifmedia_init(&mii->mii_media, 0, emac3_ifmedia_upd, emac3_ifmedia_sts);
	mii_attach(&emac3->dev, mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, 0);
	    
	/* Choose a default media. */
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_NONE);
	} else {
		ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_AUTO);
	}

	if_attach(ifp);
	ether_ifattach(ifp, emac3->eaddr);
	
	spd_intr_establish(SPD_NIC, smap_intr, sc);

#if NRND > 0
	rnd_attach_source(&sc->rnd_source, DEVNAME,
	    RND_TYPE_NET, 0);
#endif
}

int
smap_ioctl(struct ifnet *ifp, u_long command, void *data)
{
	struct smap_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *) data;
	int error, s;

	s = splnet();

	switch (command) {
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->emac3.mii.mii_media,
		    command);
		break;

	default:
		error = ether_ioctl(ifp, command, data);

		if (error == ENETRESET) {
			if (ifp->if_flags & IFF_RUNNING)
				emac3_setmulti(&sc->emac3, &sc->ethercom);
			error = 0;
		}
		break;
	}

	splx(s);

	return (error);
}

int
smap_intr(void *arg)
{
	struct smap_softc *sc = arg;
	struct ifnet *ifp;
	u_int16_t cause, disable, r;

	cause = _reg_read_2(SPD_INTR_STATUS_REG16) &
	    _reg_read_2(SPD_INTR_ENABLE_REG16);

	disable = cause & (SPD_INTR_RXDNV | SPD_INTR_TXDNV);
	if (disable) {
		r = _reg_read_2(SPD_INTR_ENABLE_REG16);
		r &= ~disable;
		_reg_write_2(SPD_INTR_ENABLE_REG16, r);

		printf("%s: invalid descriptor. (%c%c)\n", DEVNAME,
		    disable & SPD_INTR_RXDNV ? 'R' : '_',
		    disable & SPD_INTR_TXDNV ? 'T' : '_');

		if (disable & SPD_INTR_RXDNV)
			smap_rxeof(arg);

		_reg_write_2(SPD_INTR_CLEAR_REG16, disable);
	}

	if (cause & SPD_INTR_TXEND) {
		_reg_write_2(SPD_INTR_CLEAR_REG16, SPD_INTR_TXEND);
		if (_reg_read_1(SMAP_RXFIFO_FRAME_REG8) > 0)
			cause |= SPD_INTR_RXEND;
		smap_txeof(arg);
	}

	if (cause & SPD_INTR_RXEND) {
		_reg_write_2(SPD_INTR_CLEAR_REG16, SPD_INTR_RXEND);
		smap_rxeof(arg);
		if (sc->tx_desc_cnt > 0 &&
		    sc->tx_desc_cnt > _reg_read_1(SMAP_TXFIFO_FRAME_REG8))
			smap_txeof(arg);
	}

	if (cause & SPD_INTR_EMAC3)
		emac3_intr(arg);
	
	/* if transmission is pending, start here */
	ifp = &sc->ethercom.ec_if;
	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		smap_start(ifp);
#if NRND > 0
	rnd_add_uint32(&sc->rnd_source, cause | sc->tx_fifo_ptr << 16);
#endif

	return (1);
}

void
smap_rxeof(void *arg)
{
	struct smap_softc *sc = arg;
	struct smap_desc *d;
	struct ifnet *ifp = &sc->ethercom.ec_if;
	struct mbuf *m;
	u_int16_t r16, stat;
	u_int32_t *p;
	int i, j, sz, rxsz, cnt;

	FUNC_ENTER();

	i = sc->rx_done_index;

	for (cnt = 0;; cnt++, i = (i + 1) & 0x3f) {
		m = NULL;
		d = &sc->rx_desc[i];
		stat = d->stat;

		if ((stat & SMAP_RXDESC_EMPTY) != 0) {
			break;
		} else if (stat & 0x7fff) {
			ifp->if_ierrors++;
			goto next_packet;
		}

		sz = d->sz;
		rxsz = ROUND4(sz);

		KDASSERT(sz >= ETHER_ADDR_LEN * 2 + ETHER_TYPE_LEN);
		KDASSERT(sz <= ETHER_MAX_LEN);

		/* load data from FIFO */
		_reg_write_2(SMAP_RXFIFO_PTR_REG16, d->ptr & 0x3ffc);
		p = sc->rx_buf;
		for (j = 0; j < rxsz; j += sizeof(u_int32_t)) {
			*p++ = _reg_read_4(SMAP_RXFIFO_DATA_REG);
		}

		/* put to mbuf */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			printf("%s: unable to allocate Rx mbuf\n", DEVNAME);
			ifp->if_ierrors++;
			goto next_packet;
		}

		if (sz > (MHLEN - 2)) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				printf("%s: unable to allocate Rx cluster\n",
				    DEVNAME);
				m_freem(m);
				m = NULL;
				ifp->if_ierrors++;
				goto next_packet;
			}
		}

		m->m_data += 2; /* for alignment */
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = sz;
		memcpy(mtod(m, void *), (void *)sc->rx_buf, sz);

	next_packet:
		ifp->if_ipackets++;

		_reg_write_1(SMAP_RXFIFO_FRAME_DEC_REG8, 1);

		/* free descriptor */
		d->sz	= 0;
		d->ptr	= 0;
		d->stat	= SMAP_RXDESC_EMPTY;
		_wbflush();
		
		if (m != NULL) {
#if NBPFILTER > 0
			if (ifp->if_bpf)
				bpf_mtap(ifp->if_bpf, m);
#endif
			(*ifp->if_input)(ifp, m);
		}
	}
	sc->rx_done_index = i;

	r16 = _reg_read_2(SPD_INTR_ENABLE_REG16);
	if (((r16 & SPD_INTR_RXDNV) == 0) && cnt > 0) {
		r16  |= SPD_INTR_RXDNV;
		_reg_write_2(SPD_INTR_ENABLE_REG16, r16);
	}

	FUNC_EXIT();
}

void
smap_txeof(void *arg)
{
	struct smap_softc *sc = arg;
	struct ifnet *ifp = &sc->ethercom.ec_if;
	struct smap_desc *d;
	int i;

	FUNC_ENTER();

	/* clear the timeout timer. */
	ifp->if_timer = 0;

	/* garbage collect */
	for (i = sc->tx_done_index;; i = (i + 1) & 0x3f) {
		u_int16_t stat;

		d = &sc->tx_desc[i];
		stat = d->stat;
		if (stat & SMAP_TXDESC_READY) {
			/* all descriptor processed. */
			break;
		} else if (stat & 0x7fff) {
			if (stat & (SMAP_TXDESC_ECOLL | SMAP_TXDESC_LCOLL |
			    SMAP_TXDESC_MCOLL | SMAP_TXDESC_SCOLL))
				ifp->if_collisions++;
			else
				ifp->if_oerrors++;
		} else {
			ifp->if_opackets++;
		}

		if (sc->tx_desc_cnt == 0)
			break;
		
		sc->tx_buf_freesize += ROUND4(d->sz);
		sc->tx_desc_cnt--;

		d->sz = 0;
		d->ptr = 0;
		d->stat = 0;
		_wbflush();
	}
	sc->tx_done_index = i;

	/* OK to start transmit */
	ifp->if_flags &= ~IFF_OACTIVE;

	FUNC_EXIT();
}

void
smap_start(struct ifnet *ifp)
{
	struct smap_softc *sc = ifp->if_softc;
	struct smap_desc *d;
	struct mbuf *m0, *m;
	u_int8_t *p, *q;
	u_int32_t *r;
	int i, sz, pktsz;
	u_int16_t fifop;
	u_int16_t r16;

	KDASSERT(ifp->if_flags & IFF_RUNNING);
	FUNC_ENTER();

	while (1) {
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			goto end;

		pktsz = m0->m_pkthdr.len;
		KDASSERT(pktsz <= ETHER_MAX_LEN - ETHER_CRC_LEN);
		sz = ROUND4(pktsz);

		if (sz > sc->tx_buf_freesize ||
		    sc->tx_desc_cnt >= SMAP_DESC_MAX ||
		    emac3_tx_done() != 0) {
			ifp->if_flags |= IFF_OACTIVE;
			goto end;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);
		KDASSERT(m0 != NULL);
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif

		p = (u_int8_t *)sc->tx_buf;
		q = p + sz;
		/* copy to temporary buffer area */
		for (m = m0; m != 0; m = m->m_next) {
			memcpy(p, mtod(m, void *), m->m_len);
			p += m->m_len;
		}
		m_freem(m0);

		/* zero padding area */
		for (; p < q; p++)
			*p = 0;

		/* put to FIFO */
		fifop = sc->tx_fifo_ptr;
		KDASSERT((fifop & 3) == 0);
		_reg_write_2(SMAP_TXFIFO_PTR_REG16, fifop);
		sc->tx_fifo_ptr = (fifop + sz) & 0xfff;

		r = sc->tx_buf;
		for (i = 0; i < sz; i += sizeof(u_int32_t))
			*(volatile u_int32_t *)SMAP_TXFIFO_DATA_REG = *r++;
		_wbflush();

		/* put FIFO to EMAC3 */
		d = &sc->tx_desc[sc->tx_start_index];
		KDASSERT((d->stat & SMAP_TXDESC_READY) == 0);

		d->sz = pktsz;
		d->ptr = fifop + SMAP_TXBUF_BASE;
		d->stat = SMAP_TXDESC_READY | SMAP_TXDESC_GENFCS |
		    SMAP_TXDESC_GENPAD;
		_wbflush();

		sc->tx_buf_freesize -= sz;
		sc->tx_desc_cnt++;
		sc->tx_start_index = (sc->tx_start_index + 1) & 0x3f;
		_reg_write_1(SMAP_TXFIFO_FRAME_INC_REG8, 1);

		emac3_tx_kick();
		r16 = _reg_read_2(SPD_INTR_ENABLE_REG16);
		if ((r16 & SPD_INTR_TXDNV) == 0) {
			r16 |= SPD_INTR_TXDNV;
			_reg_write_2(SPD_INTR_ENABLE_REG16, r16);
		}
	}
 end:
	/* set watchdog timer */
	ifp->if_timer = 5;

	FUNC_EXIT();
}

void
smap_watchdog(struct ifnet *ifp)
{
	struct smap_softc *sc = ifp->if_softc;

	printf("%s: watchdog timeout\n",DEVNAME);
	sc->ethercom.ec_if.if_oerrors++;

	smap_fifo_init(sc);
	smap_desc_init(sc);
	emac3_reset(&sc->emac3);
}

int
smap_init(struct ifnet *ifp)
{
	struct smap_softc *sc = ifp->if_softc;
	u_int16_t r16;

	smap_fifo_init(sc);
	emac3_reset(&sc->emac3);
	smap_desc_init(sc);

	_reg_write_2(SPD_INTR_CLEAR_REG16, SPD_INTR_RXEND | SPD_INTR_TXEND |
	    SPD_INTR_RXDNV);
	emac3_intr_clear();

	r16 = _reg_read_2(SPD_INTR_ENABLE_REG16);
	r16 |=  SPD_INTR_EMAC3 | SPD_INTR_RXEND | SPD_INTR_TXEND |
	    SPD_INTR_RXDNV;
	_reg_write_2(SPD_INTR_ENABLE_REG16, r16);
	emac3_intr_enable();

	emac3_enable();

	/* Program the multicast filter, if necessary. */
	emac3_setmulti(&sc->emac3, &sc->ethercom);

	/* Set current media. */
	mii_mediachg(&sc->emac3.mii);

	ifp->if_flags |= IFF_RUNNING;

	return (0); 
}

void
smap_stop(struct ifnet *ifp, int disable)
{
	struct smap_softc *sc = ifp->if_softc;

	mii_down(&sc->emac3.mii);

	if (disable)
		emac3_disable();

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

/*
 * FIFO
 */
int
smap_fifo_init(struct smap_softc *sc)
{

	if (smap_fifo_reset(SMAP_TXFIFO_CTRL_REG8) != 0)
		goto error;

	if (smap_fifo_reset(SMAP_RXFIFO_CTRL_REG8) != 0)
		goto error;

	return (0);
error:
	printf("%s: FIFO reset not complete.\n", DEVNAME);

	return (1);
}

int
smap_fifo_reset(bus_addr_t a)
{
	int retry = 10000;

	_reg_write_1(a, SMAP_FIFO_RESET);

	while ((_reg_read_1(a) & SMAP_FIFO_RESET) && --retry > 0)
		;

	return (retry == 0);
}

/*
 * Buffer descriptor
 */
void
smap_desc_init(struct smap_softc *sc)
{
	struct smap_desc *d;
	int i;

	sc->tx_desc = (void *)SMAP_TXDESC_BASE;
	sc->rx_desc = (void *)SMAP_RXDESC_BASE;
	
	sc->tx_buf_freesize = SMAP_TXBUF_SIZE;
	sc->tx_fifo_ptr = 0;
	sc->tx_start_index = 0;
	sc->tx_done_index = 0;
	sc->rx_done_index = 0;

	/* intialize entry */
	d = sc->tx_desc;
	for (i = 0; i < SMAP_DESC_MAX; i++, d++) {
		d->stat = 0;
		d->__reserved = 0;
		d->sz = 0;
		d->ptr = 0;
	}
	
	d = sc->rx_desc;
	for (i = 0; i < SMAP_DESC_MAX; i++, d++) {
		d->stat = SMAP_RXDESC_EMPTY;
		d->__reserved = 0;
		d->sz = 0;
		d->ptr = 0;
	}
	_wbflush();
}


/*
 * EEPROM
 */
int
smap_get_eaddr(struct smap_softc *sc, u_int8_t *eaddr)
{
	u_int16_t checksum, *p = (u_int16_t *)eaddr;

	spd_eeprom_read(0, p, 3);
	spd_eeprom_read(3, &checksum, 1);

	if (checksum != (u_int16_t)(p[0] + p[1] + p[2])) {
		printf("%s: Ethernet address checksum error.(%s)\n",
		    DEVNAME, ether_sprintf(eaddr));
		return (1);
	}

	return (0);
}

#ifdef SMAP_DEBUG
#include <mips/locore.h>
void
__smap_lock_check(const char *func, int enter)
{
	static int cnt;
	static const char *last;

	cnt += enter ? 1 : -1;

	if (cnt < 0 || cnt > 1)
		panic("%s cnt=%d last=%s", func, cnt, last);

	last = func;
}

void
__smap_status(int msg)
{
	static int cnt;
	__gsfb_print(1, "%d: tx=%d rx=%d txcnt=%d free=%d cnt=%d\n", msg,
	    _reg_read_1(SMAP_TXFIFO_FRAME_REG8),
	    _reg_read_1(SMAP_RXFIFO_FRAME_REG8), __sc->tx_desc_cnt,
	    __sc->tx_buf_freesize, cnt++);
}
#endif /* SMAP_DEBUG */
