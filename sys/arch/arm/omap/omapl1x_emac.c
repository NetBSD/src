
/*
 * Copyright (c) 2013 Sughosh Ganu
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * TODO list
 * multicast address hash bit settings to be looked into
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: omapl1x_emac.c,v 1.2 2015/09/21 13:32:48 skrll Exp $");

#include "opt_omapl1x.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/callout.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/bpf.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <arm/omap/omapl1x_reg.h>
#include <arm/omap/omap_tipb.h>

struct emac_cppi_bd;

#define EMAC_TXDESCS_SIZE (EMAC_CPPI_RAM_SIZE / 2)
#define EMAC_RXDESCS_SIZE \
    (EMAC_CPPI_RAM_SIZE - EMAC_TXDESCS_SIZE)

#define EMAC_NTXDESCS (EMAC_TXDESCS_SIZE/sizeof(struct emac_cppi_bd))
#define EMAC_NRXDESCS (EMAC_RXDESCS_SIZE/sizeof(struct emac_cppi_bd))

struct emac_cppi_bd {
	u_int		desc_next;
	u_int		buffer;
	u_int		len;
	u_int		mode;
} __packed;

struct emac_chain {
	bus_dmamap_t dmamap;
	struct emac_cppi_bd *bd;
	struct mbuf *m;
	SIMPLEQ_ENTRY(emac_chain) link;
};

struct emac_desc {
	struct emac_cppi_bd *tx_desc[EMAC_NTXDESCS];
	struct emac_cppi_bd *rx_desc[EMAC_NRXDESCS];
};

struct emac_channel {
	u_int			hdp;
	u_int			cp;
	void			*desc_base;
	bus_dmamap_t		desc_map;
	kmutex_t		*lock;
	uint8_t			ch;
	uint8_t			run;
	int			desc_rseg;
	bus_dma_segment_t	desc_seg;
	SIMPLEQ_HEAD(emac_free, emac_chain) free_head;
	SIMPLEQ_HEAD(emac_inuse, emac_chain) inuse_head;
};

struct emac_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_addr;
	size_t			sc_size;
	int			sc_intr;
	bus_dma_tag_t		sc_desct;
	bus_dma_tag_t		sc_buft;
	kmutex_t		*sc_hwlock;
	kmutex_t		*sc_mii_lock;
	callout_t		sc_mii_callout;
	struct ethercom		sc_ec;
#define	sc_if			sc_ec.ec_if
	struct mii_data		sc_mii;
	struct ifmedia		sc_media;
	void			*sc_soft_ih;
	uint32_t		sc_soft_flags;
#define SOFT_RESET		0x1

	struct emac_desc	descs;
	uint8_t			sc_enaddr[ETHER_ADDR_LEN];
	struct emac_chain	*tx_chain[EMAC_NTXDESCS];
	struct emac_chain	*rx_chain[EMAC_NRXDESCS];
	struct emac_channel	tx_chan;
	struct emac_channel	rx_chan;
};

#define EMAC_INTROFF_RXTH	0
#define EMAC_INTROFF_RX		1
#define EMAC_INTROFF_TX		2
#define EMAC_INTROFF_MISC	3

#define EMAC_FULL_DUPLEX	__BIT(0)
#define EMAC_GMII_EN		__BIT(5)
#define EMAC_TXPRIO_FIXED	__BIT(9)
#define EMAC_RMII_SPEED		__BIT(15)
#define EMAC_MDIO_CLKDIV	0x24

#define EMAC_RXMCASTEN		__BIT(5)
#define EMAC_RXBROADEN		__BIT(13)

#define RXBROADCH		0
#define RXMCASTCH		0
#define RXCHAN			0
#define TXCHAN			0

#define RXBROADCH_MASK		0x7
#define RXMCASTCH_MASK		0x7
#define RXMCASTCH_SHIFT		0
#define RXBROADCH_SHIFT		8

#define RXMAXLEN		1522
#define HOSTMASK		0x2

#define RXCH0EN			__BIT(0)
#define TXCH0EN			__BIT(0)
#define TX0EN			__BIT(0)
#define RX0EN			__BIT(0)
#define TXEN			__BIT(0)
#define RXEN			__BIT(0)
#define HOSTPENDEN		__BIT(2)

#define MAX_CHANS		8

#define EMAC_INIT_RX_DESC	128
#define EMAC_MIN_PKT_LEN	60
#define EMAC_TX_DESC_FREE	32
#define EMAC_RX_DESC_FREE	16
#define TXCH			1
#define RXCH			2

#define C0TXDONE		0x2
#define C0RXDONE		0x1
#define TX0PEND			__BIT(16)
#define RX0PEND			__BIT(0)
#define HOSTPEND		__BIT(26)

#define SOP			__BIT(31)
#define EOP			__BIT(30)
#define OWNER			__BIT(29)
#define EOQ			__BIT(28)
#define TDOWNCMPLT		__BIT(27)
#define PASSCRC			__BIT(26)

#define BUFLEN_MASK		0xFFFF
#define MDIO_IDLE		__BIT(31)
#define MDIO_ENABLE		__BIT(30)

#define USERACCESS_GO		__BIT(31)
#define USERACCESS_ACK		__BIT(29)
#define USERACCESS_WRITE	__BIT(30)
#define USERACCESS_REG(x)	(((x) & 0x1f) << 21)
#define USERACCESS_PHY(x)	(((x) & 0x1f) << 16)
#define USERACCESS_DATA(x)	((x) & 0xffff)

extern struct arm32_bus_dma_tag omapl1x_bus_dma_tag;
extern struct arm32_bus_dma_tag omapl1x_desc_dma_tag;

#define	EMAC_READ(sc, o)		\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (o))
#define	EMAC_WRITE(sc, o, v)	\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (o), v)

static int emac_match(device_t parent, struct cfdata *match, void *aux);
static void emac_attach(device_t parent, device_t self, void *aux);

static int emac_intr(void *arg);
static void emac_int_disable(struct emac_softc * const sc);
static void emac_int_enable(struct emac_softc * const sc);

static int emac_ifinit(struct ifnet *ifp);
static void emac_ifstart(struct ifnet *ifp);
static void emac_ifstop(struct ifnet *ifp, int diable);
static int emac_ifioctl(struct ifnet *ifp, u_long cmd, void *data);
static void emac_ifwatchdog(struct ifnet *ifp);

static void emac_setmac(struct emac_softc * const sc, uint8_t *mac_addr);
static void emac_setmacsrcaddr(struct emac_softc *const sc, uint8_t *mac_addr);
static void emac_setmacaddr(struct emac_softc *const sc, uint8_t chan,
			    uint8_t *mac_addr);

static void emac_soft_intr(void *frame);

static int emac_rx_mbuf_map(struct emac_softc const *sc,
			    struct emac_chain *entry);

static int emac_tx_desc_dequeue(struct emac_softc *sc,
				struct emac_channel *chan);

static int emac_rx_desc_process(struct emac_softc *sc,
				struct emac_channel *chan);

static int emac_td_check(struct emac_softc * const sc,
			 bus_size_t cp, bus_size_t hdp);

static bus_addr_t emac_desc_phys(struct emac_channel *chan,
			    struct emac_cppi_bd *desc);

static void emac_add_desc_tail(struct emac_softc *sc, struct emac_channel *chan,
			      struct emac_chain *free_entry);

static int emac_desc_map(struct emac_softc * const sc,
			 struct emac_channel *chan, size_t memsize);

static void emac_desc_list_create(struct emac_cppi_bd **desc,
				  void *desc_base_ptr, int ndescs);

static u_int emac_free_descs(struct emac_softc * const sc,
			   struct emac_channel *chan, u_int num_desc);

static void emac_mii_statchg(struct ifnet *ifp);
static int emac_mii_wait(struct emac_softc * const sc);
static int emac_mii_readreg(device_t dev, int phy, int reg);
static void emac_mii_writereg(device_t dev, int phy, int reg, int val);

CFATTACH_DECL_NEW(emac, sizeof(struct emac_softc),
		  emac_match, emac_attach, NULL, NULL);

static int
emac_mii_wait (struct emac_softc * const sc)
{
	u_int tries;

	for(tries = 0; tries < 1000; tries++) {
		if ((EMAC_READ(sc, MACMDIOUSERACCESS0) & USERACCESS_GO) == 0)
			return 0;

		delay(1000);
	}

	return ETIMEDOUT;
}

static int
emac_mii_readreg (device_t dev, int phy, int reg)
{
	int ret = 0;
	uint32_t v, reg_mask = 0;
	struct emac_softc * const sc = device_private(dev);

	mutex_spin_enter(sc->sc_mii_lock);

	reg_mask = USERACCESS_GO | USERACCESS_REG(reg) | USERACCESS_PHY(phy);

	while (1) {
		ret = emac_mii_wait(sc);
		if (ret == ETIMEDOUT) {
			ret = 0;
			if ((EMAC_READ(sc, MACMDIOCONTROL) & MDIO_IDLE)) {
				EMAC_WRITE(sc, MACMDIOCONTROL,
					   MDIO_ENABLE | EMAC_MDIO_CLKDIV);
			}
			continue;
		}

		EMAC_WRITE(sc, MACMDIOUSERACCESS0, reg_mask);

		ret = emac_mii_wait(sc);
		if (ret == ETIMEDOUT) {
			ret = 0;
			if ((EMAC_READ(sc, MACMDIOCONTROL) & MDIO_IDLE)) {
				EMAC_WRITE(sc, MACMDIOCONTROL,
					   MDIO_ENABLE | EMAC_MDIO_CLKDIV);
			}
			continue;
		}
		break;
	}

	v = EMAC_READ(sc, MACMDIOUSERACCESS0);
	if (v & USERACCESS_ACK)
		ret = USERACCESS_DATA(v);
	else
		ret = 0;

	mutex_spin_exit(sc->sc_mii_lock);

	return ret;
}

static void
emac_mii_writereg (device_t dev, int phy, int reg, int val)
{
	int ret = 0;
	uint32_t reg_mask = 0;
	struct emac_softc * const sc = device_private(dev);

	mutex_spin_enter(sc->sc_mii_lock);

	reg_mask = USERACCESS_GO | USERACCESS_WRITE | USERACCESS_REG(reg) |
		USERACCESS_PHY(phy) | USERACCESS_DATA(val);

	while (1) {
		ret = emac_mii_wait(sc);
		if (ret == ETIMEDOUT) {
			if ((EMAC_READ(sc, MACMDIOCONTROL) & MDIO_IDLE)) {
				EMAC_WRITE(sc, MACMDIOCONTROL,
					   MDIO_ENABLE | EMAC_MDIO_CLKDIV);
			}
			continue;
		}

		EMAC_WRITE(sc, MACMDIOUSERACCESS0, reg_mask);

		ret = emac_mii_wait(sc);
		if (ret == ETIMEDOUT) {
			if ((EMAC_READ(sc, MACMDIOCONTROL) & MDIO_IDLE)) {
				EMAC_WRITE(sc, MACMDIOCONTROL,
					   MDIO_ENABLE | EMAC_MDIO_CLKDIV);
			}
			continue;
		}
		break;
	}

	mutex_spin_exit(sc->sc_mii_lock);
}

static void
emac_mii_statchg (struct ifnet *ifp)
{
	struct emac_softc * const sc = ifp->if_softc;
	uint32_t maccontrol = 0;
	uint32_t maccontrol_read;

	maccontrol_read = EMAC_READ(sc, MACCONTROL);

	/* speed config */
	if (IFM_SUBTYPE(sc->sc_mii.mii_media_active) == IFM_100_TX) {
		maccontrol |= EMAC_RMII_SPEED;
	} else {
		maccontrol &= ~EMAC_RMII_SPEED;
	}

	/* duplex */
	if (sc->sc_mii.mii_media_active & IFM_FDX) {
		maccontrol |= EMAC_FULL_DUPLEX;
	} else {
		maccontrol &= ~EMAC_FULL_DUPLEX;
	}

	if (maccontrol_read == 0) {
		EMAC_WRITE(sc, MACCONTROL, maccontrol);
	} else if ((maccontrol_read & (EMAC_FULL_DUPLEX | EMAC_RMII_SPEED)) !=
		   (maccontrol & (EMAC_FULL_DUPLEX | EMAC_RMII_SPEED))) {
		/* Things have changed. Re-init */
		sc->sc_soft_flags |= SOFT_RESET;
	}

	return;
}

static void
emac_mii_tick (void *arg)
{
	struct emac_softc * const sc = arg;

	mii_tick(&sc->sc_mii);

	int s = splnet();

	if (sc->sc_soft_flags & SOFT_RESET) {
		mutex_enter(sc->sc_hwlock);
		emac_int_disable(sc);
		mutex_exit(sc->sc_hwlock);

		softint_schedule(sc->sc_soft_ih);
	}

	splx(s);

	callout_schedule(&sc->sc_mii_callout, hz);
}

static void
emac_setmacaddr (struct emac_softc *const sc, uint8_t chan, uint8_t *mac_addr)
{
	uint32_t val;

	EMAC_WRITE(sc, MACINDEX, chan);

	val = (mac_addr[3] << 24 | mac_addr[2] << 16 |
	       mac_addr[1] << 8 | mac_addr[0]);

	EMAC_WRITE(sc, MACADDRHI, val);

	val = 0;
	val = (mac_addr[5] << 8 | mac_addr[4] | ((chan & 0x7) << 16) |
	       __BIT(19) | __BIT(20));
	EMAC_WRITE(sc, MACADDRLO, val);
}

static void
emac_setmacsrcaddr (struct emac_softc *const sc, uint8_t *mac_addr)
{
	uint32_t val;

	val = (mac_addr[3] << 24 | mac_addr[2] << 16 |
	       mac_addr[1] << 8 | mac_addr[0]);

	EMAC_WRITE(sc, MACSRCADDRHI, val);

	val = 0;
	val = (mac_addr[5] << 8 | mac_addr[4]);
	EMAC_WRITE(sc, MACSRCADDRLO, val);
}

static void
emac_setmac (struct emac_softc *const sc, uint8_t *mac_addr)
{
	uint8_t i;

	for (i = 0; i < MAX_CHANS; i++)
		emac_setmacaddr(sc, i, mac_addr);
	emac_setmacsrcaddr(sc, mac_addr);
}

static bus_addr_t
emac_desc_phys (struct emac_channel *chan, struct emac_cppi_bd *desc)
{
	vaddr_t offset;

	offset = (vaddr_t)desc - (vaddr_t)chan->desc_base;

	return chan->desc_map->dm_segs[0].ds_addr + offset;
}

static void
emac_add_desc_tail (struct emac_softc *sc, struct emac_channel *chan,
	       struct emac_chain *free_entry)
{
	struct emac_cppi_bd *desc, *tail_desc;
	struct emac_chain *tail_entry;
	bus_dmamap_t map;
	u_int mode;
	bus_addr_t desc1_offset, desc2_offset, desc_base;
	bus_size_t buf_len;

	/* Prepare the desc for adding to the end of the chain */
	desc = free_entry->bd;
	map = free_entry->dmamap;
	buf_len = map->dm_segs[0].ds_len;
	desc_base = (bus_addr_t)chan->desc_base;
	memset(desc, 0, sizeof(*desc));

	desc1_offset = (bus_addr_t)desc - desc_base;

	bus_dmamap_sync(sc->sc_desct, chan->desc_map,
			desc1_offset, sizeof(*desc),
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	desc->desc_next = 0;
	desc->buffer = (u_int)map->dm_segs[0].ds_addr;
	desc->len = (u_int)(buf_len & 0xffff);
	if (chan->ch == TXCH) {
		desc->mode = SOP | EOP | OWNER | (u_int)buf_len;
	} else {
		desc->mode = OWNER;
	}
	if (SIMPLEQ_EMPTY(&chan->inuse_head)) {
		/* idle list */
		SIMPLEQ_REMOVE_HEAD(&chan->free_head, link);
		SIMPLEQ_INSERT_HEAD(&chan->inuse_head, free_entry, link);
		EMAC_WRITE(sc, chan->hdp, (bus_size_t)emac_desc_phys(chan, desc));
		goto sync2;
	}

	tail_entry = SIMPLEQ_LAST(&chan->inuse_head, emac_chain, link);
	tail_desc = tail_entry->bd;

	desc2_offset = (bus_addr_t)tail_desc - desc_base;

	bus_dmamap_sync(sc->sc_desct, chan->desc_map,
			desc2_offset, sizeof(*desc),
			BUS_DMASYNC_PREREAD);

	tail_desc->desc_next = (u_int)emac_desc_phys(chan, desc);

	mode = tail_desc->mode;
	if ((mode & (EOQ | OWNER)) == EOQ) {
		tail_desc->mode &= ~EOQ;
		EMAC_WRITE(sc, chan->hdp,
			   (bus_size_t)emac_desc_phys(chan, desc));
	}

	SIMPLEQ_REMOVE_HEAD(&chan->free_head, link);
	SIMPLEQ_INSERT_TAIL(&chan->inuse_head, free_entry, link);

	desc2_offset = (bus_addr_t)tail_desc - desc_base;

	bus_dmamap_sync(sc->sc_desct, chan->desc_map,
			desc2_offset, sizeof(*desc),
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
sync2:
	bus_dmamap_sync(sc->sc_desct, chan->desc_map,
			desc1_offset, sizeof(*desc),
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
}

static int
emac_rx_mbuf_map (struct emac_softc const *sc, struct emac_chain *entry)
{
	int error;
	struct mbuf *m;
	bus_dmamap_t map;

	MGETHDR(m, M_WAIT, MT_DATA);
	if (__predict_false(m == NULL))
		return ENOMEM;

	MCLGET(m, M_WAIT);
	if (__predict_false((m->m_flags & M_EXT) == 0)) {
		m_freem(m);
		return ENOMEM;
	}

	entry->m = m;
	map = entry->dmamap;
	error = bus_dmamap_load(sc->sc_buft, map, m->m_ext.ext_buf,
				m->m_ext.ext_size, NULL,
				BUS_DMA_READ | BUS_DMA_WAITOK);
	if (error)
		return error;

	bus_dmamap_sync(sc->sc_buft, map, 0, map->dm_mapsize,
			BUS_DMASYNC_PREREAD);

	return 0;
}

static int
emac_tx_desc_dequeue (struct emac_softc *sc, struct emac_channel *chan)
{
	struct emac_cppi_bd *desc;
	struct emac_chain *entry;
	bus_dmamap_t map;
	bus_addr_t desc_offset;
	bus_addr_t desc_base;

	if ((entry = SIMPLEQ_FIRST(&chan->inuse_head)) == NULL)
		return ENOENT;

	desc = entry->bd;
	map = entry->dmamap;
	desc_base = (bus_addr_t)chan->desc_base;

	desc_offset = (bus_addr_t)desc - desc_base;
	bus_dmamap_sync(sc->sc_desct, chan->desc_map, desc_offset,
			sizeof(*desc),
			BUS_DMASYNC_POSTREAD);

	if (desc->mode & OWNER) {
		return EBUSY;
	}

	if (desc->mode & EOQ) {
		EMAC_WRITE(sc, chan->hdp, 0);
	}

	bus_dmamap_unload(sc->sc_buft, map);
	m_freem(entry->m);
	entry->m = NULL;

	EMAC_WRITE(sc, chan->cp, emac_desc_phys(chan, desc));

	SIMPLEQ_REMOVE_HEAD(&chan->inuse_head, link);
	SIMPLEQ_INSERT_TAIL(&chan->free_head, entry, link);

	return 0;
}

static int
emac_rx_desc_process (struct emac_softc *sc, struct emac_channel *chan)
{
	struct emac_cppi_bd *desc;
	struct ifnet * const ifp = &sc->sc_if;
	struct emac_chain *entry;
	bus_dmamap_t map;
	bus_addr_t desc_offset;
	struct mbuf *mb;
	bus_addr_t desc_base;
	u_int buf_len;

	if ((entry = SIMPLEQ_FIRST(&chan->inuse_head)) == NULL) {
		return ENOENT;
	}

	desc = entry->bd;
	map = entry->dmamap;
	mb = entry->m;
	desc_base = (bus_addr_t)chan->desc_base;

	desc_offset = (bus_addr_t)desc - desc_base;
	bus_dmamap_sync(sc->sc_desct, chan->desc_map, desc_offset,
			sizeof(*desc), BUS_DMASYNC_PREREAD);

	if (desc->mode & OWNER) {
		return EBUSY;
	}

	if ((desc->mode & (SOP | EOP)) != (SOP | EOP)) {
		/* This needs a look */
		device_printf(sc->sc_dev,
			     "Received packet spanning multiple buffers\n");
	}

	//off = __SHIFTOUT(desc->len, (uint32_t)__BITS(26, 16));
	buf_len = __SHIFTOUT(desc->mode, (uint32_t)__BITS(10,  0));

	if (desc->mode & PASSCRC)
		buf_len -= ETHER_CRC_LEN;

	bus_dmamap_unload(sc->sc_buft, map);
	mb->m_pkthdr.rcvif = ifp;
	mb->m_pkthdr.len = mb->m_len = buf_len;
	ifp->if_ipackets++;

	bpf_mtap(ifp, mb);
	(*ifp->if_input)(ifp, mb);

	entry->m = NULL;

	if (desc->mode & EOQ)
		EMAC_WRITE(sc, chan->hdp, 0);

	EMAC_WRITE(sc, chan->cp, emac_desc_phys(chan, desc));

	SIMPLEQ_REMOVE_HEAD(&chan->inuse_head, link);
	SIMPLEQ_INSERT_TAIL(&chan->free_head, entry, link);

	return 0;
}

static void
emac_rx_chain_create (struct emac_softc *sc, struct emac_channel *chan)
{
	int i, error;
	struct emac_chain *entry;

	mutex_enter(chan->lock);

	for (i = 0; i < EMAC_INIT_RX_DESC; i++) {
		if ((entry = SIMPLEQ_FIRST(&chan->free_head)) != NULL) {
			error = emac_rx_mbuf_map(sc, entry);
			if (error) {
				device_printf(sc->sc_dev,
					      "%s: Could not map Rx mbufs"
					      " Queued up %d descs\n",
					      __func__, i);
				goto unlock;
			}
			emac_add_desc_tail(sc, chan, entry);
		} else {
			/*
			 * If we don't have the descriptors at init
			 * time, something is seriously wrong
			 * XXX Should this be a panic, or should the driver
			 * be cleanly exited.
			 */
			panic("%s: Only %d descriptors at init time!\n",
			      __func__, i);
		}
	}

unlock:
	mutex_exit(chan->lock);
}

static int
emac_desc_dequeue (struct emac_softc *sc, struct emac_channel *chan)
{
	int ret;
	struct emac_chain *entry;
	struct ifnet * const ifp = &sc->sc_if;

	if (chan->ch == TXCH) {
		ret = emac_tx_desc_dequeue(sc, chan);
		if (ret == 0) {
			ifp->if_flags &= ~IFF_OACTIVE;
		}
	} else {
		/* Process the received packet */
		ret = emac_rx_desc_process(sc, chan);
		if (ret == 0) {
			/* Now add a desc to the rx list */
			if ((entry = SIMPLEQ_FIRST(&chan->free_head)) != NULL) {
				ret = emac_rx_mbuf_map(sc, entry);
				if (ret == 0) {
					emac_add_desc_tail(sc, chan, entry);
				}
			}
		}
	}

	return ret;
}

static u_int
emac_free_descs (struct emac_softc * const sc, struct emac_channel *chan,
		 u_int num_desc)
{
	int i, ret;

	mutex_enter(chan->lock);

	for (i = 0; i < num_desc; i++) {
		ret = emac_desc_dequeue(sc, chan);
		if (ret > 0)
			break;
	}

	mutex_exit(chan->lock);

	return i;
}

static void
emac_int_disable (struct emac_softc * const sc)
{
	EMAC_WRITE(sc, MAC_CR_C_TX_EN(0), 0x0);
	EMAC_WRITE(sc, MAC_CR_C_RX_EN(0), 0x0);
	EMAC_WRITE(sc, MAC_CR_C_MISC_EN(0), 0x0);
}

static void
emac_int_enable (struct emac_softc * const sc)
{
	EMAC_WRITE(sc, MACTXINTMASKSET, TX0EN);
	EMAC_WRITE(sc, MACRXINTMASKSET, RX0EN);
	EMAC_WRITE(sc, MACINTMASKSET, HOSTMASK);
	EMAC_WRITE(sc, MAC_CR_C_TX_EN(0), TXCH0EN);
	EMAC_WRITE(sc, MAC_CR_C_RX_EN(0), RXCH0EN);
	EMAC_WRITE(sc, MAC_CR_C_MISC_EN(0), HOSTPENDEN);
}

static void
emac_soft_intr (void *arg)
{
	uint32_t mask;
	struct emac_softc * const sc = arg;
	struct ifnet * const ifp = &sc->sc_if;
	u_int soft_flags = atomic_swap_uint(&sc->sc_soft_flags, 0);

	if (soft_flags & SOFT_RESET) {
		int s = splnet();
		emac_ifinit(ifp);
		splx(s);
		return;
	}

	mutex_enter(sc->sc_hwlock);

	mask = EMAC_READ(sc, MACINVECTOR);

	/* We are working on channel 0 */
	if (mask & TX0PEND) {
		emac_free_descs(sc, &sc->tx_chan, EMAC_TX_DESC_FREE);
	}

	EMAC_WRITE(sc, MACEOIVECTOR, C0TXDONE);

	if (mask & RX0PEND) {
		emac_free_descs(sc, &sc->rx_chan, EMAC_RX_DESC_FREE);
	}

	EMAC_WRITE(sc, MACEOIVECTOR, C0RXDONE);

	if (__predict_false(mask & HOSTPEND)) {
		device_printf(sc->sc_dev,
			      "Host Error. Stopping the device\n");
		emac_ifstop(ifp, 0);

		return;
	}

	emac_int_enable(sc);

	mutex_exit(sc->sc_hwlock);

	return;
}

static int
emac_intr (void *arg)
{
	struct emac_softc * const sc = arg;

	mutex_enter(sc->sc_hwlock);
	emac_int_disable(sc);
	mutex_exit(sc->sc_hwlock);

	softint_schedule(sc->sc_soft_ih);

	return 1;
}

static int
emac_td_check (struct emac_softc * const sc, bus_size_t cp, bus_size_t hdp)
{
	uint32_t cp_val;

	cp_val = EMAC_READ(sc, cp);

	if (cp_val == 0xfffffffc) {
		EMAC_WRITE(sc, cp, 0xfffffffc);
		EMAC_WRITE(sc, hdp, 0);
		return 1;
	}

	return 0;
}

static void
emac_ifstart (struct ifnet *ifp)
{
	struct emac_softc * const sc = ifp->if_softc;
	struct mbuf *mb, *m;
	struct emac_chain *entry;
	struct emac_channel *chan;
	bus_dmamap_t map;
	int error;

	if (__predict_false((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) !=
			    IFF_RUNNING)) {
		return;
	}

	chan = &sc->tx_chan;
	mutex_enter(chan->lock);

	m = NULL;
	IFQ_POLL(&ifp->if_snd, mb);
	if (mb == NULL)
		goto unlock;

	while ((entry = SIMPLEQ_FIRST(&chan->free_head)) != NULL) {
		if (mb->m_pkthdr.len < ETHER_MIN_LEN) {
remap:
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				printf("%s: unable to allocate Tx mbuf\n",
				       device_xname(sc->sc_dev));
				goto unlock;
			}
			if (mb->m_pkthdr.len > MHLEN) {
				MCLGET(m, M_DONTWAIT);
				if ((m->m_flags & M_EXT) == 0) {
					printf("%s: unable to allocate Tx "
					    "cluster\n", device_xname(sc->sc_dev));
					m_freem(m);
					goto unlock;
				}
			}
			m_copydata(mb, 0, mb->m_pkthdr.len, mtod(m, void *));
			m->m_pkthdr.len = m->m_len = mb->m_pkthdr.len;
			if (m->m_pkthdr.len < ETHER_MIN_LEN) {
				if (M_TRAILINGSPACE(m) < ETHER_MIN_LEN -
				    m->m_pkthdr.len) {
					panic("emac_ifstart: M_TRAILINGSPACE\n");
				}
				memset(mtod(m, uint8_t *) + m->m_pkthdr.len, 0,
				    ETHER_MIN_LEN - ETHER_CRC_LEN - m->m_pkthdr.len);
				m->m_pkthdr.len = m->m_len = ETHER_MIN_LEN;
			}
		}

		IFQ_DEQUEUE(&ifp->if_snd, mb);
		if (m != NULL) {
			m_freem(mb);
			mb = m;
		}

		/* We have a msg to xmit */
		entry->m = mb;
		map = entry->dmamap;
		error = bus_dmamap_load_mbuf(sc->sc_buft, map, mb,
					     BUS_DMA_NOWAIT);
		if (error == EFBIG) {
			/*
			 * Ok, so our mbuf chain spans across
			 * discontiguous segments, unify them
			 */
			goto remap;
		} else if (error != 0) {
			device_printf(sc->sc_dev, "error\n");
			goto unlock;
		}

		bus_dmamap_sync(sc->sc_buft, map, 0, map->dm_mapsize,
				BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		if (__predict_false(map->dm_nsegs > 1))
			panic("%s: Cannot handle more than one segment\n",
			      __func__);

		emac_add_desc_tail(sc, chan, entry);
		bpf_mtap(ifp, mb);

		IFQ_POLL(&ifp->if_snd, mb);
		if (mb == NULL)
			goto unlock;
	}

	device_printf(sc->sc_dev, "TX desc's full, setting IFF_OACTIVE\n");
	ifp->if_flags |= IFF_OACTIVE;

unlock:
	mutex_exit(chan->lock);
}


static void
emac_ifstop (struct ifnet *ifp, int disable)
{
	struct emac_chain *entry, *next;
	struct emac_softc * const sc = ifp->if_softc;
	struct emac_channel *tx_chan, *rx_chan;
	u_int i;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	tx_chan = &sc->tx_chan;
	rx_chan = &sc->rx_chan;

	callout_stop(&sc->sc_mii_callout);
	mii_down(&sc->sc_mii);

	mutex_enter(sc->sc_hwlock);
	emac_int_disable(sc);
	mutex_exit(sc->sc_hwlock);

	EMAC_WRITE(sc, MACTXTEARDOWN, 0);
	EMAC_WRITE(sc, MACRXTEARDOWN, 0);

	i = 0;
	while ((tx_chan->run || rx_chan->run) && i < 10000) {
		delay(10);
		if ((tx_chan->run == true) &&
		    emac_td_check(sc, tx_chan->cp, tx_chan->hdp)) {
			tx_chan->run = false;
		}

		if ((rx_chan->run == true) &&
		    emac_td_check(sc, rx_chan->cp, rx_chan->hdp)) {
			rx_chan->run = false;
		}

		i++;
	}

	mutex_enter(tx_chan->lock);

	/* Release any queued transmit buffers. */
	SIMPLEQ_FOREACH_SAFE(entry, &tx_chan->inuse_head, link, next) {
		bus_dmamap_unload(sc->sc_buft, entry->dmamap);
		m_free(entry->m);
		memset(entry->bd, 0, sizeof(*entry->bd));
		entry->m = NULL;
		SIMPLEQ_REMOVE_HEAD(&tx_chan->inuse_head, link);
		SIMPLEQ_INSERT_TAIL(&tx_chan->free_head, entry, link);
	}

	mutex_exit(tx_chan->lock);


	mutex_enter(rx_chan->lock);

	/* Release any queued rx buffers */
	SIMPLEQ_FOREACH_SAFE(entry, &rx_chan->inuse_head, link, next) {
		bus_dmamap_unload(sc->sc_buft, entry->dmamap);
		m_free(entry->m);
		memset(entry->bd, 0, sizeof(*entry->bd));
		entry->m = NULL;
		SIMPLEQ_REMOVE_HEAD(&rx_chan->inuse_head, link);
		SIMPLEQ_INSERT_TAIL(&rx_chan->free_head, entry, link);
	}

	mutex_exit(rx_chan->lock);

	ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);
}

static int
emac_ifinit (struct ifnet *ifp)
{
	struct emac_softc * const sc = ifp->if_softc;
	struct emac_channel *tx_chan, *rx_chan;
	struct mii_data * const mii = &sc->sc_mii;
	int i;
	u_int32_t maccontrol, mbp_enable;


	tx_chan = &sc->tx_chan;
	rx_chan = &sc->rx_chan;

	emac_ifstop(ifp, 0);

	EMAC_WRITE(sc, MACHASH1, 0);
	EMAC_WRITE(sc, MACHASH2, 0);

	EMAC_WRITE(sc, MACSOFTRESET, 1);
	while (EMAC_READ(sc, MACSOFTRESET) == 1);

	/* Till we figure out mcast, do this */
	EMAC_WRITE(sc, MACHASH1, 0xffffffff);
	EMAC_WRITE(sc, MACHASH2, 0xffffffff);

	for (i = 0; i < 8; i++) {
		EMAC_WRITE(sc, MAC_TX_HDP(i), 0);
		EMAC_WRITE(sc, MAC_RX_HDP(i), 0);
		EMAC_WRITE(sc, MAC_TX_CP(i), 0);
		EMAC_WRITE(sc, MAC_RX_CP(i), 0);
	}

	EMAC_WRITE(sc, MACCONTROL, 0);

	mii_mediachg(mii);

	emac_setmac(sc, sc->sc_enaddr);

	EMAC_WRITE(sc, MACRXBUFOFFSET, 0);

	EMAC_WRITE(sc, MACRXUNICASTCLEAR, 0xff);

	EMAC_WRITE(sc, MACRXUNICASTCLEAR, 0xff);
	EMAC_WRITE(sc, MACRXUNICASTSET, RXCH0EN);

	mbp_enable = EMAC_RXBROADEN | EMAC_RXMCASTEN |
		((RXBROADCH & RXBROADCH_MASK) << RXBROADCH_SHIFT) |
		((RXMCASTCH & RXMCASTCH_MASK) << RXMCASTCH_SHIFT);
	EMAC_WRITE(sc, MACRXMBPEN, mbp_enable);

	EMAC_WRITE(sc, MACRXMAXLEN, RXMAXLEN);

	emac_rx_chain_create(sc, &sc->rx_chan);

	EMAC_WRITE(sc, MACMDIOCONTROL, __BIT(30) | __BIT(18) |
		   EMAC_MDIO_CLKDIV);

	EMAC_WRITE(sc, MACTXCONTROL, TXEN);
	EMAC_WRITE(sc, MACRXCONTROL, RXEN);

	/* Turn ON mii */
	maccontrol = EMAC_READ(sc, MACCONTROL);
	maccontrol |= EMAC_GMII_EN;
	EMAC_WRITE(sc, MACCONTROL, maccontrol);

	tx_chan->run = true;
	rx_chan->run = true;
	callout_schedule(&sc->sc_mii_callout, hz);
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	mutex_enter(sc->sc_hwlock);
	emac_int_enable(sc);
	mutex_exit(sc->sc_hwlock);

	return 0;
}

static int
emac_ifioctl (struct ifnet *ifp, u_long cmd, void *data)
{
	const int s = splnet();
	int error = 0;

	switch (cmd) {
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			error = 0;
		}
		break;
	}

	splx(s);

	return error;
}

static void
emac_ifwatchdog (struct ifnet *ifp)
{
}

static int
emac_desc_map (struct emac_softc * const sc, struct emac_channel *chan,
	       size_t memsize)
{
	void **kvap;
	bus_dmamap_t *map;
	bus_dma_tag_t dmat;

	map = &chan->desc_map;
	kvap = &chan->desc_base;
	dmat = sc->sc_desct;

	if (bus_dmamem_alloc(dmat, memsize, PAGE_SIZE, 0, &chan->desc_seg, 1,
			     &chan->desc_rseg, 0)) {
		device_printf(sc->sc_dev, "can't alloc descriptors\n");
		return 1;
	}

	if (bus_dmamem_map(dmat, &chan->desc_seg, chan->desc_rseg, memsize,
			   kvap, 0)) {
		device_printf(sc->sc_dev, "can't map descriptors (%zu bytes)\n",
				 memsize);
		goto fail1;
	}

	if (bus_dmamap_create(dmat, memsize, 1, memsize, 0, 0, map)) {
		device_printf(sc->sc_dev, "can't create descriptor dma map\n");
		goto fail2;
	}

	if (bus_dmamap_load(dmat, *map, *kvap, memsize, NULL, 0)) {
		device_printf(sc->sc_dev, "can't load dma map\n");
		goto fail3;
	}

	return 0;

fail3:
	bus_dmamap_destroy(sc->sc_desct, *map);

fail2:
	bus_dmamem_unmap(dmat, *kvap, memsize);
	*kvap = NULL;

fail1:
	bus_dmamem_free(dmat, &chan->desc_seg, chan->desc_rseg);
	return 1;
}

static void
emac_desc_list_create (struct emac_cppi_bd **desc, void *desc_base_ptr, int ndescs)
{
	int i;
	struct emac_cppi_bd *ptr = desc_base_ptr;
	
	for (i = 0; i < ndescs; i++)
		desc[i] = ptr + i;
}

static int
emac_match (device_t parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
emac_attach (device_t parent, device_t self, void *aux)
{
	struct emac_softc * const sc = device_private(self);
	struct ifnet * const ifp = &sc->sc_if;
	struct tipb_attach_args *tipb = aux;
	const char * const xname = device_xname(self);
	prop_dictionary_t dict = device_properties(self);
	struct emac_channel *tx_chan, *rx_chan;
	bus_dmamap_t dmamap;
	struct emac_chain *entry;
	int i;

	sc->sc_iot = tipb->tipb_iot;
	sc->sc_intr = tipb->tipb_intr;
	sc->sc_addr = tipb->tipb_addr;
	sc->sc_size = OMAPL1X_EMAC_SIZE;

	/* descriptors to be allocated from cppi ram range */
	sc->sc_desct = &omapl1x_desc_dma_tag;
	sc->sc_buft = &omapl1x_bus_dma_tag;

	if (bus_space_map(sc->sc_iot, sc->sc_addr, sc->sc_size, 0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	sc->tx_chan.hdp = MAC_TX_HDP(0);
	sc->tx_chan.cp = MAC_TX_CP(0);

	sc->rx_chan.hdp = MAC_RX_HDP(0);
	sc->rx_chan.cp = MAC_RX_CP(0);

	tx_chan = &sc->tx_chan;
	rx_chan = &sc->rx_chan;
	tx_chan->ch = TXCH;
	rx_chan->ch = RXCH;

	tx_chan->lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SOFTNET);
	rx_chan->lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SOFTNET);
	sc->sc_hwlock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_VM);
	sc->sc_mii_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_VM);

	mutex_enter(sc->sc_hwlock);
	emac_int_disable(sc);
	mutex_exit(sc->sc_hwlock);

	callout_init(&sc->sc_mii_callout, 0);
	callout_setfunc(&sc->sc_mii_callout, emac_mii_tick, sc);

	prop_data_t eaprop = prop_dictionary_get(dict, "mac-address");
        if (eaprop == NULL) {
		device_printf(sc->sc_dev,
		"using fake station address\n");
		/* 'N' happens to have the Local bit set */
		sc->sc_enaddr[0] = 'N';
		sc->sc_enaddr[1] = 'e';
		sc->sc_enaddr[2] = 't';
		sc->sc_enaddr[3] = 'B';
		sc->sc_enaddr[4] = 'S';
		sc->sc_enaddr[5] = 'D';
	} else {
		KASSERT(prop_object_type(eaprop) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(eaprop) == ETHER_ADDR_LEN);
		memcpy(sc->sc_enaddr, prop_data_data_nocopy(eaprop),
		    ETHER_ADDR_LEN);
	}

	sc->sc_dev = self;

	/* First map the tx and rx descriptors */
	if (emac_desc_map(sc, tx_chan, EMAC_TXDESCS_SIZE)) {
		aprint_error_dev(self, "Can't map tx desc's\n");
		return;
	}

	emac_desc_list_create(sc->descs.tx_desc, tx_chan->desc_base,
			      EMAC_NTXDESCS);

	if (emac_desc_map(sc, rx_chan, EMAC_RXDESCS_SIZE)) {
		aprint_error_dev(self, "Can't map rx desc's\n");
		return;
	}
	emac_desc_list_create(sc->descs.rx_desc, rx_chan->desc_base,
			      EMAC_NRXDESCS);

	/* Get the dma map's for the tx buffers */
	SIMPLEQ_INIT(&tx_chan->free_head);
	SIMPLEQ_INIT(&tx_chan->inuse_head);
	for (i = 0; i < EMAC_NTXDESCS; i++) {
		/* 
		 * Ok, we keep this simple, one dma segment per tx dma map.
		 * This makes the mapping of the desc's and the dma map's
		 * pretty straightforward.
		 * We club together the mbuf chain in the if_start routine
		 * if it's fragmented.
		 */
		if (bus_dmamap_create(sc->sc_buft, MCLBYTES, 1, MCLBYTES, 0,
		    BUS_DMA_WAITOK, &dmamap)) {
			aprint_error_dev(self, "Can't create TX dmamap\n");
			goto fail;
		}

		entry = kmem_zalloc(sizeof(*entry), KM_SLEEP);
		if (!entry) {
			aprint_error_dev(self, "Can't alloc txmap entry\n");
			bus_dmamap_destroy(sc->sc_desct, dmamap);
			goto fail;
		}

		entry->dmamap = dmamap;
		entry->bd = sc->descs.tx_desc[i];
		entry->m = NULL;
		sc->tx_chain[i] = entry;
		SIMPLEQ_INSERT_HEAD(&tx_chan->free_head, entry, link);
	}

	/* Now on to the RX buffers */
	SIMPLEQ_INIT(&rx_chan->free_head);
	SIMPLEQ_INIT(&rx_chan->inuse_head);
	for (i = 0; i < EMAC_NRXDESCS; i++) {
		if (bus_dmamap_create(sc->sc_buft, MCLBYTES, 1, MCLBYTES, 0,
		    BUS_DMA_WAITOK, &dmamap)) {
			aprint_error_dev(self, "Can't create RX dmamap\n");
			goto fail;
		}

		entry = kmem_zalloc(sizeof(*entry), KM_SLEEP);
		if (!entry) {
			aprint_error_dev(self, "Can't alloc rxmap entry\n");
			bus_dmamap_destroy(sc->sc_buft, dmamap);
			goto fail;
		}

		entry->dmamap = dmamap;
		entry->bd = sc->descs.rx_desc[i];
		entry->m = NULL;
		sc->rx_chain[i] = entry;
		SIMPLEQ_INSERT_HEAD(&rx_chan->free_head, entry, link);
	}

	/* mii related stuff */
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = emac_mii_readreg;
	sc->sc_mii.mii_writereg = emac_mii_writereg;
	sc->sc_mii.mii_statchg = emac_mii_statchg;
	sc->sc_ec.ec_mii = &sc->sc_mii;

	EMAC_WRITE(sc, MACMDIOCONTROL, __BIT(30) | __BIT(18) |
		   EMAC_MDIO_CLKDIV);

	ifmedia_init(&sc->sc_mii.mii_media, 0, ether_mediachange,
	    ether_mediastatus);
	mii_attach(self, &sc->sc_mii, 0xffffffff, MII_PHY_ANY, 0, 0);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		aprint_error_dev(self, "no PHY found!\n");
		ifmedia_add(&sc->sc_mii.mii_media,
		    IFM_ETHER|IFM_MANUAL, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL);
	} else {
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
	}

	strlcpy(ifp->if_xname, xname, IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_softc = sc;
	ifp->if_start = emac_ifstart;
	ifp->if_ioctl = emac_ifioctl;
	ifp->if_init = emac_ifinit;
	ifp->if_stop = emac_ifstop;
	ifp->if_watchdog = emac_ifwatchdog;
	ifp->if_mtu = ETHERMTU;
	IFQ_SET_READY(&ifp->if_snd);

	emac_ifstop(ifp, 0);

	sc->sc_soft_ih = softint_establish(SOFTINT_NET, emac_soft_intr, sc);

	/* Register all the emac interrupts */
	intr_establish(sc->sc_intr + EMAC_INTROFF_RXTH, IPL_VM, IST_LEVEL_HIGH,
		       emac_intr, sc);
	intr_establish(sc->sc_intr + EMAC_INTROFF_RX, IPL_VM, IST_LEVEL_HIGH,
		       emac_intr, sc);
	intr_establish(sc->sc_intr + EMAC_INTROFF_TX, IPL_VM, IST_LEVEL_HIGH,
		       emac_intr, sc);
	intr_establish(sc->sc_intr + EMAC_INTROFF_MISC, IPL_VM, IST_LEVEL_HIGH,
		       emac_intr, sc);

	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

	return;

fail:
	while ((entry = SIMPLEQ_FIRST(&sc->tx_chan.free_head)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->tx_chan.free_head, link);
		bus_dmamap_destroy(sc->sc_desct, entry->dmamap);
	}

	while ((entry = SIMPLEQ_FIRST(&sc->rx_chan.free_head)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->tx_chan.free_head, link);
		bus_dmamap_destroy(sc->sc_desct, entry->dmamap);
	}

	bus_dmamap_destroy(sc->sc_desct, rx_chan->desc_map);
	bus_dmamap_destroy(sc->sc_desct, tx_chan->desc_map);

	bus_dmamem_unmap(sc->sc_desct, rx_chan->desc_base, EMAC_RXDESCS_SIZE);
	bus_dmamem_unmap(sc->sc_desct, tx_chan->desc_base, EMAC_TXDESCS_SIZE);
	sc->tx_chan.desc_base = NULL;
	sc->rx_chan.desc_base = NULL;

	bus_dmamem_free(sc->sc_desct, &tx_chan->desc_seg, tx_chan->desc_rseg);
	bus_dmamem_free(sc->sc_desct, &rx_chan->desc_seg, rx_chan->desc_rseg);

	return;
}
