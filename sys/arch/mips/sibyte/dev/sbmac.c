/* $NetBSD: sbmac.c,v 1.15 2004/03/18 05:57:58 simonb Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbmac.c,v 1.15 2004/03/18 05:57:58 simonb Exp $");

#include "bpfilter.h"
#include "opt_inet.h"
#include "opt_ns.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_ether.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/locore.h>

#include "sbobiovar.h"

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/mii_bitbang.h>

#include <mips/sibyte/include/sb1250_defs.h>
#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_mac.h>
#include <mips/sibyte/include/sb1250_dma.h>
#include <mips/sibyte/include/sb1250_scd.h>

/* Simple types */

typedef u_long sbmac_port_t;
typedef uint64_t sbmac_physaddr_t;
typedef uint64_t sbmac_enetaddr_t;

typedef enum { sbmac_speed_auto, sbmac_speed_10,
	       sbmac_speed_100, sbmac_speed_1000 } sbmac_speed_t;

typedef enum { sbmac_duplex_auto, sbmac_duplex_half,
	       sbmac_duplex_full } sbmac_duplex_t;

typedef enum { sbmac_fc_auto, sbmac_fc_disabled, sbmac_fc_frame,
	       sbmac_fc_collision, sbmac_fc_carrier } sbmac_fc_t;

typedef enum { sbmac_state_uninit, sbmac_state_off, sbmac_state_on,
	       sbmac_state_broken } sbmac_state_t;


/* Macros */

#define	SBMAC_EVENT_COUNTERS	/* Include counters for various events */

#define	SBDMA_NEXTBUF(d, f) ((((d)->f+1) == (d)->sbdma_dscrtable_end) ? \
			  (d)->sbdma_dscrtable : (d)->f+1)


#define	CACHELINESIZE 32
#define	NUMCACHEBLKS(x) (((x)+CACHELINESIZE-1)/CACHELINESIZE)
#define	KMALLOC(x) malloc((x), M_DEVBUF, M_DONTWAIT)
#define	KVTOPHYS(x) kvtophys((vaddr_t)(x))

#ifdef SBMACDEBUG
#define	dprintf(x)	printf x
#else
#define	dprintf(x)
#endif

#define	SBMAC_READCSR(t) mips3_ld((uint64_t *) (t))
#define	SBMAC_WRITECSR(t, v) mips3_sd((uint64_t *) (t), (v))

#define	PKSEG1(x) ((sbmac_port_t) MIPS_PHYS_TO_KSEG1(x))

#define	SBMAC_MAX_TXDESCR	64
#define	SBMAC_MAX_RXDESCR	64

#define	ETHER_ALIGN	2

/* DMA Descriptor structure */

typedef struct sbdmadscr_s {
	uint64_t dscr_a;
	uint64_t dscr_b;
} sbdmadscr_t;


/* DMA Controller structure */

typedef struct sbmacdma_s {

	/*
	 * This stuff is used to identify the channel and the registers
	 * associated with it.
	 */

	struct sbmac_softc *sbdma_eth;	/* back pointer to associated MAC */
	int		sbdma_channel;	/* channel number */
	int		sbdma_txdir;	/* direction (1=transmit) */
	int		sbdma_maxdescr;	/* total # of descriptors in ring */
	sbmac_port_t	sbdma_config0;	/* DMA config register 0 */
	sbmac_port_t	sbdma_config1;	/* DMA config register 1 */
	sbmac_port_t	sbdma_dscrbase;	/* Descriptor base address */
	sbmac_port_t	sbdma_dscrcnt; 	/* Descriptor count register */
	sbmac_port_t	sbdma_curdscr;	/* current descriptor address */

	/*
	 * This stuff is for maintenance of the ring
	 */

	sbdmadscr_t	*sbdma_dscrtable;	/* base of descriptor table */
	sbdmadscr_t	*sbdma_dscrtable_end;	/* end of descriptor table */

	struct mbuf	**sbdma_ctxtable;	/* context table, one per descr */

	paddr_t		sbdma_dscrtable_phys;	/* and also the phys addr */
	sbdmadscr_t	*sbdma_addptr;		/* next dscr for sw to add */
	sbdmadscr_t	*sbdma_remptr;		/* next dscr for sw to remove */
} sbmacdma_t;


/* Ethernet softc structure */

struct sbmac_softc {

	/*
	 * NetBSD-specific things
	 */
	struct device	sc_dev;		/* base device (must be first) */
	struct ethercom	sc_ethercom;	/* Ethernet common part */
	struct mii_data	sc_mii;
	struct callout	sc_tick_ch;

	int		sbm_if_flags;
	void		*sbm_intrhand;

	/*
	 * Controller-specific things
	 */

	sbmac_port_t	sbm_base;	/* MAC's base address */
	sbmac_state_t	sbm_state;	/* current state */

	sbmac_port_t	sbm_macenable;	/* MAC Enable Register */
	sbmac_port_t	sbm_maccfg;	/* MAC Configuration Register */
	sbmac_port_t	sbm_fifocfg;	/* FIFO configuration register */
	sbmac_port_t	sbm_framecfg;	/* Frame configuration register */
	sbmac_port_t	sbm_rxfilter;	/* receive filter register */
	sbmac_port_t	sbm_isr;	/* Interrupt status register */
	sbmac_port_t	sbm_imr;	/* Interrupt mask register */

	sbmac_speed_t	sbm_speed;	/* current speed */
	sbmac_duplex_t	sbm_duplex;	/* current duplex */
	sbmac_fc_t	sbm_fc;		/* current flow control setting */
	int		sbm_rxflags;	/* received packet flags */

	u_char		sbm_hwaddr[ETHER_ADDR_LEN];

	sbmacdma_t	sbm_txdma;	/* for now, only use channel 0 */
	sbmacdma_t	sbm_rxdma;

	int		sbm_pass3_dma;	/* chip has pass3 SOC DMA features */

#ifdef SBMAC_EVENT_COUNTERS
	struct evcnt	sbm_ev_rxintr;	/* Rx interrupts */
	struct evcnt	sbm_ev_txintr;	/* Tx interrupts */
	struct evcnt	sbm_ev_txdrop;	/* Tx dropped due to no mbuf alloc failed */
	struct evcnt	sbm_ev_txstall;	/* Tx stalled due to no descriptors free */

	struct evcnt	sbm_ev_txsplit;	/* pass3 Tx split mbuf */
	struct evcnt	sbm_ev_txkeep;	/* pass3 Tx didn't split mbuf */
#endif
};


#ifdef SBMAC_EVENT_COUNTERS
#define	SBMAC_EVCNT_INCR(ev)	(ev).ev_count++
#else
#define	SBMAC_EVCNT_INCR(ev)	do { /* nothing */ } while (0)
#endif

/* Externs */

extern paddr_t kvtophys(vaddr_t);

/* Prototypes */

static void sbdma_initctx(sbmacdma_t *d, struct sbmac_softc *s, int chan,
    int txrx, int maxdescr);
static void sbdma_channel_start(sbmacdma_t *d);
static int sbdma_add_rcvbuffer(sbmacdma_t *d, struct mbuf *m);
static int sbdma_add_txbuffer(sbmacdma_t *d, struct mbuf *m);
static void sbdma_emptyring(sbmacdma_t *d);
static void sbdma_fillring(sbmacdma_t *d);
static void sbdma_rx_process(struct sbmac_softc *sc, sbmacdma_t *d);
static void sbdma_tx_process(struct sbmac_softc *sc, sbmacdma_t *d);
static void sbmac_initctx(struct sbmac_softc *s);
static void sbmac_channel_start(struct sbmac_softc *s);
static void sbmac_channel_stop(struct sbmac_softc *s);
static sbmac_state_t sbmac_set_channel_state(struct sbmac_softc *,
    sbmac_state_t);
static void sbmac_promiscuous_mode(struct sbmac_softc *sc, int onoff);
static void sbmac_init_and_start(struct sbmac_softc *sc);
static uint64_t sbmac_addr2reg(u_char *ptr);
static void sbmac_intr(void *xsc, uint32_t status, uint32_t pc);
static void sbmac_start(struct ifnet *ifp);
static void sbmac_setmulti(struct sbmac_softc *sc);
static int sbmac_ether_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data);
static int sbmac_ioctl(struct ifnet *ifp, u_long command, caddr_t data);
static int sbmac_mediachange(struct ifnet *ifp);
static void sbmac_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr);
static void sbmac_watchdog(struct ifnet *ifp);
static int sbmac_match(struct device *parent, struct cfdata *match, void *aux);
static void sbmac_attach(struct device *parent, struct device *self, void *aux);
static int sbmac_set_speed(struct sbmac_softc *s, sbmac_speed_t speed);
static int sbmac_set_duplex(struct sbmac_softc *s, sbmac_duplex_t duplex,
    sbmac_fc_t fc);
static void sbmac_tick(void *arg);


/* Globals */

CFATTACH_DECL(sbmac, sizeof(struct sbmac_softc),
    sbmac_match, sbmac_attach, NULL, NULL);

static uint32_t sbmac_mii_bitbang_read(struct device *self);
static void sbmac_mii_bitbang_write(struct device *self, uint32_t val);

static const struct mii_bitbang_ops sbmac_mii_bitbang_ops = {
	sbmac_mii_bitbang_read,
	sbmac_mii_bitbang_write,
	{
		(uint32_t)M_MAC_MDIO_OUT,	/* MII_BIT_MDO */
		(uint32_t)M_MAC_MDIO_IN,	/* MII_BIT_MDI */
		(uint32_t)M_MAC_MDC,		/* MII_BIT_MDC */
		0,				/* MII_BIT_DIR_HOST_PHY */
		(uint32_t)M_MAC_MDIO_DIR	/* MII_BIT_DIR_PHY_HOST */
	}
};

static uint32_t
sbmac_mii_bitbang_read(struct device *self)
{
	struct sbmac_softc *sc = (void *) self;
	sbmac_port_t reg;

	reg = PKSEG1(sc->sbm_base + R_MAC_MDIO);
	return (uint32_t) SBMAC_READCSR(reg);
}

static void
sbmac_mii_bitbang_write(struct device *self, uint32_t val)
{
	struct sbmac_softc *sc = (void *) self;
	sbmac_port_t reg;

	reg = PKSEG1(sc->sbm_base + R_MAC_MDIO);

	SBMAC_WRITECSR(reg, (val &
	    (M_MAC_MDC|M_MAC_MDIO_DIR|M_MAC_MDIO_OUT|M_MAC_MDIO_IN)));
}

/*
 * Read an PHY register through the MII.
 */
static int
sbmac_mii_readreg(struct device *self, int phy, int reg)
{

	return (mii_bitbang_readreg(self, &sbmac_mii_bitbang_ops, phy, reg));
}

/*
 * Write to a PHY register through the MII.
 */
static void
sbmac_mii_writereg(struct device *self, int phy, int reg, int val)
{

	mii_bitbang_writereg(self, &sbmac_mii_bitbang_ops, phy, reg, val);
}

static void
sbmac_mii_statchg(struct device *self)
{
	struct sbmac_softc *sc = (struct sbmac_softc *)self;
	sbmac_state_t oldstate;

	/* Stop the MAC in preparation for changing all of the parameters. */
	oldstate = sbmac_set_channel_state(sc, sbmac_state_off);

	switch (sc->sc_ethercom.ec_if.if_baudrate) {
	default:		/* if autonegotiation fails, assume 10Mbit */
	case IF_Mbps(10):
		sbmac_set_speed(sc, sbmac_speed_10);
		break;

	case IF_Mbps(100):
		sbmac_set_speed(sc, sbmac_speed_100);
		break;

	case IF_Mbps(1000):
		sbmac_set_speed(sc, sbmac_speed_1000);
		break;
	}

	if (sc->sc_mii.mii_media_active & IFM_FDX) {
		/* Configure for full-duplex */
		/* XXX: is flow control right for 10, 100? */
		sbmac_set_duplex(sc, sbmac_duplex_full, sbmac_fc_frame);
	} else {
		/* Configure for half-duplex */
		/* XXX: is flow control right? */
		sbmac_set_duplex(sc, sbmac_duplex_half, sbmac_fc_disabled);
	}

	/* And put it back into its former state. */
	sbmac_set_channel_state(sc, oldstate);
}

/*
 *  SBDMA_INITCTX(d, s, chan, txrx, maxdescr)
 *
 *  Initialize a DMA channel context.  Since there are potentially
 *  eight DMA channels per MAC, it's nice to do this in a standard
 *  way.
 *
 *  Input parameters:
 *	d - sbmacdma_t structure (DMA channel context)
 *	s - sbmac_softc structure (pointer to a MAC)
 *	chan - channel number (0..1 right now)
 *	txrx - Identifies DMA_TX or DMA_RX for channel direction
 *	maxdescr - number of descriptors
 *
 *  Return value:
 *	nothing
 */

static void
sbdma_initctx(sbmacdma_t *d, struct sbmac_softc *s, int chan, int txrx,
    int maxdescr)
{
	/*
	 * Save away interesting stuff in the structure
	 */

	d->sbdma_eth = s;
	d->sbdma_channel = chan;
	d->sbdma_txdir = txrx;

	/*
	 * initialize register pointers
	 */

	d->sbdma_config0 = PKSEG1(s->sbm_base +
	    R_MAC_DMA_REGISTER(txrx, chan, R_MAC_DMA_CONFIG0));
	d->sbdma_config1 = PKSEG1(s->sbm_base +
	    R_MAC_DMA_REGISTER(txrx, chan, R_MAC_DMA_CONFIG1));
	d->sbdma_dscrbase = PKSEG1(s->sbm_base +
	    R_MAC_DMA_REGISTER(txrx, chan, R_MAC_DMA_DSCR_BASE));
	d->sbdma_dscrcnt = PKSEG1(s->sbm_base +
	    R_MAC_DMA_REGISTER(txrx, chan, R_MAC_DMA_DSCR_CNT));
	d->sbdma_curdscr = PKSEG1(s->sbm_base +
	    R_MAC_DMA_REGISTER(txrx, chan, R_MAC_DMA_CUR_DSCRADDR));

	/*
	 * Allocate memory for the ring
	 */

	d->sbdma_maxdescr = maxdescr;

	d->sbdma_dscrtable = (sbdmadscr_t *)
	    KMALLOC(d->sbdma_maxdescr*sizeof(sbdmadscr_t));

	bzero(d->sbdma_dscrtable, d->sbdma_maxdescr*sizeof(sbdmadscr_t));

	d->sbdma_dscrtable_end = d->sbdma_dscrtable + d->sbdma_maxdescr;

	d->sbdma_dscrtable_phys = KVTOPHYS(d->sbdma_dscrtable);

	/*
	 * And context table
	 */

	d->sbdma_ctxtable = (struct mbuf **)
	    KMALLOC(d->sbdma_maxdescr*sizeof(struct mbuf *));

	bzero(d->sbdma_ctxtable, d->sbdma_maxdescr*sizeof(struct mbuf *));
}

/*
 *  SBDMA_CHANNEL_START(d)
 *
 *  Initialize the hardware registers for a DMA channel.
 *
 *  Input parameters:
 *	d - DMA channel to init (context must be previously init'd
 *
 *  Return value:
 *	nothing
 */

static void
sbdma_channel_start(sbmacdma_t *d)
{
	/*
	 * Turn on the DMA channel
	 */

	SBMAC_WRITECSR(d->sbdma_config1, 0);

	SBMAC_WRITECSR(d->sbdma_dscrbase, d->sbdma_dscrtable_phys);

	SBMAC_WRITECSR(d->sbdma_config0, V_DMA_RINGSZ(d->sbdma_maxdescr) | 0);

	/*
	 * Initialize ring pointers
	 */

	d->sbdma_addptr = d->sbdma_dscrtable;
	d->sbdma_remptr = d->sbdma_dscrtable;
}

/*
 *  SBDMA_ADD_RCVBUFFER(d, m)
 *
 *  Add a buffer to the specified DMA channel.   For receive channels,
 *  this queues a buffer for inbound packets.
 *
 *  Input parameters:
 *	d - DMA channel descriptor
 *	m - mbuf to add, or NULL if we should allocate one.
 *
 *  Return value:
 *	0 if buffer could not be added (ring is full)
 *	1 if buffer added successfully
 */

static int
sbdma_add_rcvbuffer(sbmacdma_t *d, struct mbuf *m)
{
	sbdmadscr_t *dsc;
	sbdmadscr_t *nextdsc;
	struct mbuf *m_new = NULL;

	/* get pointer to our current place in the ring */

	dsc = d->sbdma_addptr;
	nextdsc = SBDMA_NEXTBUF(d, sbdma_addptr);

	/*
	 * figure out if the ring is full - if the next descriptor
	 * is the same as the one that we're going to remove from
	 * the ring, the ring is full
	 */

	if (nextdsc == d->sbdma_remptr)
		return ENOSPC;

	/*
	 * Allocate an mbuf if we don't already have one.
	 * If we do have an mbuf, reset it so that it's empty.
	 */

	if (m == NULL) {
		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			printf("%s: mbuf allocation failed\n",
			    d->sbdma_eth->sc_dev.dv_xname);
			return ENOBUFS;
		}

		MCLGET(m_new, M_DONTWAIT);
		if (!(m_new->m_flags & M_EXT)) {
			printf("%s: mbuf cluster allocation failed\n",
			    d->sbdma_eth->sc_dev.dv_xname);
			m_freem(m_new);
			return ENOBUFS;
		}

		m_new->m_len = m_new->m_pkthdr.len= MCLBYTES;
		m_adj(m_new, ETHER_ALIGN);
	} else {
		m_new = m;
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
		m_new->m_data = m_new->m_ext.ext_buf;
		m_adj(m_new, ETHER_ALIGN);
	}

	/*
	 * fill in the descriptor
	 */

	dsc->dscr_a = KVTOPHYS(mtod(m_new, caddr_t)) |
	    V_DMA_DSCRA_A_SIZE(NUMCACHEBLKS(ETHER_ALIGN + m_new->m_len)) |
	    M_DMA_DSCRA_INTERRUPT;

	/* receiving: no options */
	dsc->dscr_b = 0;

	/*
	 * fill in the context
	 */

	d->sbdma_ctxtable[dsc-d->sbdma_dscrtable] = m_new;

	/*
	 * point at next packet
	 */

	d->sbdma_addptr = nextdsc;

	/*
	 * Give the buffer to the DMA engine.
	 */

	SBMAC_WRITECSR(d->sbdma_dscrcnt, 1);

	return 0;					/* we did it */
}

/*
 *  SBDMA_ADD_TXBUFFER(d, m)
 *
 *  Add a transmit buffer to the specified DMA channel, causing a
 *  transmit to start.
 *
 *  Input parameters:
 *	d - DMA channel descriptor
 *	m - mbuf to add
 *
 *  Return value:
 *	0 transmit queued successfully
 *	otherwise error code
 */

static int
sbdma_add_txbuffer(sbmacdma_t *d, struct mbuf *m)
{
	sbdmadscr_t *dsc;
	sbdmadscr_t *nextdsc;
	sbdmadscr_t *prevdsc;
	sbdmadscr_t *origdesc;
	int length;
	int num_mbufs = 0;
	struct sbmac_softc *sc = d->sbdma_eth;

	/* get pointer to our current place in the ring */

	dsc = d->sbdma_addptr;
	nextdsc = SBDMA_NEXTBUF(d, sbdma_addptr);

	/*
	 * figure out if the ring is full - if the next descriptor
	 * is the same as the one that we're going to remove from
	 * the ring, the ring is full
	 */

	if (nextdsc == d->sbdma_remptr) {
		SBMAC_EVCNT_INCR(sc->sbm_ev_txstall);
		return ENOSPC;
	}

	/*
	 * PASS3 parts do not have buffer alignment restriction.
	 * No need to copy/coalesce to new mbuf.  Also has different
	 * descriptor format
	 */
	if (sc->sbm_pass3_dma) {
		struct mbuf *m_temp = NULL;

		/*
		 * Loop thru this mbuf record.
		 * The head mbuf will have SOP set.
		 */
		dsc->dscr_a = KVTOPHYS(mtod(m,caddr_t)) |
		    M_DMA_DSCRA_INTERRUPT |
		    M_DMA_ETHTX_SOP;

		/*
		 * transmitting: set outbound options,buffer A size(+ low 5
		 * bits of start addr),and packet length.
		 */
		dsc->dscr_b =
		    V_DMA_DSCRB_OPTIONS(K_DMA_ETHTX_APPENDCRC_APPENDPAD) |
		    V_DMA_DSCRB_A_SIZE((m->m_len +
		      (mtod(m,unsigned int) & 0x0000001F))) |
		    V_DMA_DSCRB_PKT_SIZE_MSB( (m->m_pkthdr.len & 0xB000) ) |
		    V_DMA_DSCRB_PKT_SIZE(m->m_pkthdr.len);

		d->sbdma_addptr = nextdsc;
		origdesc = prevdsc = dsc;
		dsc = d->sbdma_addptr;
		num_mbufs++;

		/* Start with first non-head mbuf */
		for(m_temp = m->m_next; m_temp != 0; m_temp = m_temp->m_next) {
			int len, next_len;
			uint64_t addr;

			if (m_temp->m_len == 0)
				continue;	/* Skip 0-length mbufs */

			len = m_temp->m_len;
			addr = KVTOPHYS(mtod(m_temp, caddr_t));

			/*
			 * Check to see if the mbuf spans a page boundary.  If
			 * it does, and the physical pages behind the virtual
			 * pages are not contiguous, split it so that each
			 * virtual page uses it's own Tx descriptor.
			 */
			if (trunc_page(addr) != trunc_page(addr + len - 1)) {
				next_len = (addr + len) - trunc_page(addr + len);

				len -= next_len;

				if (addr + len ==
				    KVTOPHYS(mtod(m_temp, caddr_t) + len)) {
					SBMAC_EVCNT_INCR(sc->sbm_ev_txkeep);
					len += next_len;
					next_len = 0;
				} else {
					SBMAC_EVCNT_INCR(sc->sbm_ev_txsplit);
				}
			} else {
				next_len = 0;
			}

again:
			/*
			 * fill in the descriptor
			 */
			dsc->dscr_a = addr | M_DMA_DSCRA_INTERRUPT;

			/*
			 * transmitting: set outbound options,buffer A
			 * size(+ low 5 bits of start addr)
			 */
			dsc->dscr_b = V_DMA_DSCRB_OPTIONS(K_DMA_ETHTX_NOTSOP) |
			    V_DMA_DSCRB_A_SIZE((len + (addr & 0x0000001F)));

			d->sbdma_ctxtable[dsc-d->sbdma_dscrtable] = NULL;

			/*
			 * point at next descriptor
			 */
			nextdsc = SBDMA_NEXTBUF(d,sbdma_addptr);
			if (nextdsc == d->sbdma_remptr) {
				d->sbdma_addptr = origdesc;
				SBMAC_EVCNT_INCR(sc->sbm_ev_txstall);
				return ENOSPC;
			}
			d->sbdma_addptr = nextdsc;

			prevdsc = dsc;
			dsc = d->sbdma_addptr;
			num_mbufs++;

			if (next_len != 0) {
				addr = KVTOPHYS(mtod(m_temp, caddr_t) + len);
				len = next_len;

				next_len = 0;
				goto again;
			}

		}
		/*Set head mbuf to last context index*/
		d->sbdma_ctxtable[prevdsc-d->sbdma_dscrtable] = m;
	} else {
		struct mbuf *m_new = NULL;
		/*
		 * [BEGIN XXX]
		 * XXX Copy/coalesce the mbufs into a single mbuf cluster (we
		 * assume it will fit).  This is a temporary hack to get us
		 * going.
		 */

		MGETHDR(m_new,M_DONTWAIT,MT_DATA);
		if (m_new == NULL) {
			printf("%s: mbuf allocation failed\n",
			    d->sbdma_eth->sc_dev.dv_xname);
			SBMAC_EVCNT_INCR(sc->sbm_ev_txdrop);
			return ENOBUFS;
		}

		MCLGET(m_new,M_DONTWAIT);
		if (!(m_new->m_flags & M_EXT)) {
			printf("%s: mbuf cluster allocation failed\n",
			    d->sbdma_eth->sc_dev.dv_xname);
			m_freem(m_new);
			SBMAC_EVCNT_INCR(sc->sbm_ev_txdrop);
			return ENOBUFS;
		}

		m_new->m_len = m_new->m_pkthdr.len= MCLBYTES;
		/*m_adj(m_new,ETHER_ALIGN);*/

		/*
		 * XXX Don't forget to include the offset portion in the
		 * XXX cache block calculation when this code is rewritten!
		 */

		/*
		 * Copy data
		 */

		m_copydata(m,0,m->m_pkthdr.len,mtod(m_new,caddr_t));
		m_new->m_len = m_new->m_pkthdr.len = m->m_pkthdr.len;

		/* Free old mbuf 'm', actual mbuf is now 'm_new' */

		// XXX: CALLERS WILL FREE, they might have to bpf_mtap() if this
		// XXX: function succeeds.
		// m_freem(m);
		length = m_new->m_len;

		/* [END XXX] */
		/*
		 * fill in the descriptor
		 */

		dsc->dscr_a = KVTOPHYS(mtod(m_new,caddr_t)) |
		    V_DMA_DSCRA_A_SIZE(NUMCACHEBLKS(m_new->m_len)) |
		    M_DMA_DSCRA_INTERRUPT |
		    M_DMA_ETHTX_SOP;

		/* transmitting: set outbound options and length */
		dsc->dscr_b =
		    V_DMA_DSCRB_OPTIONS(K_DMA_ETHTX_APPENDCRC_APPENDPAD) |
		    V_DMA_DSCRB_PKT_SIZE(length);

		num_mbufs++;

		/*
		 * fill in the context
		 */

		d->sbdma_ctxtable[dsc-d->sbdma_dscrtable] = m_new;

		/*
		 * point at next packet
		 */
		d->sbdma_addptr = nextdsc;
	}

	/*
	 * Give the buffer to the DMA engine.
	 */

	SBMAC_WRITECSR(d->sbdma_dscrcnt, num_mbufs);

	return 0;					/* we did it */
}

/*
 *  SBDMA_EMPTYRING(d)
 *
 *  Free all allocated mbufs on the specified DMA channel;
 *
 *  Input parameters:
 *	d  - DMA channel
 *
 *  Return value:
 *	nothing
 */

static void
sbdma_emptyring(sbmacdma_t *d)
{
	int idx;
	struct mbuf *m;

	for (idx = 0; idx < d->sbdma_maxdescr; idx++) {
		m = d->sbdma_ctxtable[idx];
		if (m) {
			m_freem(m);
			d->sbdma_ctxtable[idx] = NULL;
		}
	}
}

/*
 *  SBDMA_FILLRING(d)
 *
 *  Fill the specified DMA channel (must be receive channel)
 *  with mbufs
 *
 *  Input parameters:
 *	d - DMA channel
 *
 *  Return value:
 *	nothing
 */

static void
sbdma_fillring(sbmacdma_t *d)
{
	int idx;

	for (idx = 0; idx < SBMAC_MAX_RXDESCR-1; idx++)
		if (sbdma_add_rcvbuffer(d, NULL) != 0)
			break;
}

/*
 *  SBDMA_RX_PROCESS(sc, d)
 *
 *  Process "completed" receive buffers on the specified DMA channel.
 *  Note that this isn't really ideal for priority channels, since
 *  it processes all of the packets on a given channel before
 *  returning.
 *
 *  Input parameters:
 *	sc - softc structure
 *	d - DMA channel context
 *
 *  Return value:
 *	nothing
 */

static void
sbdma_rx_process(struct sbmac_softc *sc, sbmacdma_t *d)
{
	int curidx;
	int hwidx;
	sbdmadscr_t *dsc;
	struct mbuf *m;
	int len;

	struct ifnet *ifp = &(sc->sc_ethercom.ec_if);

	for (;;) {
		/*
		 * figure out where we are (as an index) and where
		 * the hardware is (also as an index)
		 *
		 * This could be done faster if (for example) the
		 * descriptor table was page-aligned and contiguous in
		 * both virtual and physical memory -- you could then
		 * just compare the low-order bits of the virtual address
		 * (sbdma_remptr) and the physical address (sbdma_curdscr CSR)
		 */

		curidx = d->sbdma_remptr - d->sbdma_dscrtable;
		hwidx = (int)
		    (((SBMAC_READCSR(d->sbdma_curdscr) & M_DMA_CURDSCR_ADDR) -
		    d->sbdma_dscrtable_phys) / sizeof(sbdmadscr_t));

		/*
		 * If they're the same, that means we've processed all
		 * of the descriptors up to (but not including) the one that
		 * the hardware is working on right now.
		 */

		if (curidx == hwidx)
			break;

		/*
		 * Otherwise, get the packet's mbuf ptr back
		 */

		dsc = &(d->sbdma_dscrtable[curidx]);
		m = d->sbdma_ctxtable[curidx];
		d->sbdma_ctxtable[curidx] = NULL;

		len = (int)G_DMA_DSCRB_PKT_SIZE(dsc->dscr_b) - 4;

		/*
		 * Check packet status.  If good, process it.
		 * If not, silently drop it and put it back on the
		 * receive ring.
		 */

		if (! (dsc->dscr_a & M_DMA_ETHRX_BAD)) {

			/*
			 * Set length into the packet
			 * XXX do we remove the CRC here?
			 */
			m->m_pkthdr.len = m->m_len = len;

			ifp->if_ipackets++;
			m->m_pkthdr.rcvif = ifp;


			/*
			 * Add a new buffer to replace the old one.
			 */
			sbdma_add_rcvbuffer(d, NULL);

#if (NBPFILTER > 0)
			/*
			 * Handle BPF listeners. Let the BPF user see the
			 * packet, but don't pass it up to the ether_input()
			 * layer unless it's a broadcast packet, multicast
			 * packet, matches our ethernet address or the
			 * interface is in promiscuous mode.
			 */

			if (ifp->if_bpf)
				bpf_mtap(ifp->if_bpf, m);
#endif
			/*
			 * Pass the buffer to the kernel
			 */
			(*ifp->if_input)(ifp, m);
		} else {
			/*
			 * Packet was mangled somehow.  Just drop it and
			 * put it back on the receive ring.
			 */
			sbdma_add_rcvbuffer(d, m);
		}

		/*
		 * .. and advance to the next buffer.
		 */

		d->sbdma_remptr = SBDMA_NEXTBUF(d, sbdma_remptr);
	}
}

/*
 *  SBDMA_TX_PROCESS(sc, d)
 *
 *  Process "completed" transmit buffers on the specified DMA channel.
 *  This is normally called within the interrupt service routine.
 *  Note that this isn't really ideal for priority channels, since
 *  it processes all of the packets on a given channel before
 *  returning.
 *
 *  Input parameters:
 *	sc - softc structure
 *	d - DMA channel context
 *
 *  Return value:
 *	nothing
 */

static void
sbdma_tx_process(struct sbmac_softc *sc, sbmacdma_t *d)
{
	int curidx;
	int hwidx;
	struct mbuf *m;

	struct ifnet *ifp = &(sc->sc_ethercom.ec_if);

	for (;;) {
		/*
		 * figure out where we are (as an index) and where
		 * the hardware is (also as an index)
		 *
		 * This could be done faster if (for example) the
		 * descriptor table was page-aligned and contiguous in
		 * both virtual and physical memory -- you could then
		 * just compare the low-order bits of the virtual address
		 * (sbdma_remptr) and the physical address (sbdma_curdscr CSR)
		 */

		curidx = d->sbdma_remptr - d->sbdma_dscrtable;
		hwidx = (int)
		    (((SBMAC_READCSR(d->sbdma_curdscr) & M_DMA_CURDSCR_ADDR) -
		    d->sbdma_dscrtable_phys) / sizeof(sbdmadscr_t));

		/*
		 * If they're the same, that means we've processed all
		 * of the descriptors up to (but not including) the one that
		 * the hardware is working on right now.
		 */

		if (curidx == hwidx)
			break;

		/*
		 * Otherwise, get the packet's mbuf ptr back
		 */

		m = d->sbdma_ctxtable[curidx];
		d->sbdma_ctxtable[curidx] = NULL;

		/*
		 * for transmits, we just free buffers.
		 */

		m_freem(m);

		/*
		 * .. and advance to the next buffer.
		 */

		d->sbdma_remptr = SBDMA_NEXTBUF(d, sbdma_remptr);
	}

	/*
	 * Decide what to set the IFF_OACTIVE bit in the interface to.
	 * It's supposed to reflect if the interface is actively
	 * transmitting, but that's really hard to do quickly.
	 */

	ifp->if_flags &= ~IFF_OACTIVE;
}

/*
 *  SBMAC_INITCTX(s)
 *
 *  Initialize an Ethernet context structure - this is called
 *  once per MAC on the 1250.  Memory is allocated here, so don't
 *  call it again from inside the ioctl routines that bring the
 *  interface up/down
 *
 *  Input parameters:
 *	s - sbmac context structure
 *
 *  Return value:
 *	0
 */

static void
sbmac_initctx(struct sbmac_softc *s)
{
	uint64_t sysrev;

	/*
	 * figure out the addresses of some ports
	 */

	s->sbm_macenable = PKSEG1(s->sbm_base + R_MAC_ENABLE);
	s->sbm_maccfg    = PKSEG1(s->sbm_base + R_MAC_CFG);
	s->sbm_fifocfg   = PKSEG1(s->sbm_base + R_MAC_THRSH_CFG);
	s->sbm_framecfg  = PKSEG1(s->sbm_base + R_MAC_FRAMECFG);
	s->sbm_rxfilter  = PKSEG1(s->sbm_base + R_MAC_ADFILTER_CFG);
	s->sbm_isr       = PKSEG1(s->sbm_base + R_MAC_STATUS);
	s->sbm_imr       = PKSEG1(s->sbm_base + R_MAC_INT_MASK);

	/*
	 * Initialize the DMA channels.  Right now, only one per MAC is used
	 * Note: Only do this _once_, as it allocates memory from the kernel!
	 */

	sbdma_initctx(&(s->sbm_txdma), s, 0, DMA_TX, SBMAC_MAX_TXDESCR);
	sbdma_initctx(&(s->sbm_rxdma), s, 0, DMA_RX, SBMAC_MAX_RXDESCR);

	/*
	 * initial state is OFF
	 */

	s->sbm_state = sbmac_state_off;

	/*
	 * Initial speed is (XXX TEMP) 10MBit/s HDX no FC
	 */

	s->sbm_speed = sbmac_speed_10;
	s->sbm_duplex = sbmac_duplex_half;
	s->sbm_fc = sbmac_fc_disabled;

	/* 
	 * Determine SOC type.  112x has Pass3 SOC features.
	 */
	sysrev = SBMAC_READCSR( PKSEG1(A_SCD_SYSTEM_REVISION) );
	s->sbm_pass3_dma = (SYS_SOC_TYPE(sysrev) == K_SYS_SOC_TYPE_BCM1120 ||
			    SYS_SOC_TYPE(sysrev) == K_SYS_SOC_TYPE_BCM1125 ||
			    SYS_SOC_TYPE(sysrev) == K_SYS_SOC_TYPE_BCM1125H ||
			    (SYS_SOC_TYPE(sysrev) == K_SYS_SOC_TYPE_BCM1250 &&
			     0));
#ifdef SBMAC_EVENT_COUNTERS
	evcnt_attach_dynamic(&s->sbm_ev_rxintr, EVCNT_TYPE_INTR,
	    NULL, s->sc_dev.dv_xname, "rxintr");
	evcnt_attach_dynamic(&s->sbm_ev_txintr, EVCNT_TYPE_INTR,
	    NULL, s->sc_dev.dv_xname, "txintr");
	evcnt_attach_dynamic(&s->sbm_ev_txdrop, EVCNT_TYPE_MISC,
	    NULL, s->sc_dev.dv_xname, "txdrop");
	evcnt_attach_dynamic(&s->sbm_ev_txstall, EVCNT_TYPE_MISC,
	    NULL, s->sc_dev.dv_xname, "txstall");
	if (s->sbm_pass3_dma) {
		evcnt_attach_dynamic(&s->sbm_ev_txsplit, EVCNT_TYPE_MISC,
		    NULL, s->sc_dev.dv_xname, "pass3tx-split");
		evcnt_attach_dynamic(&s->sbm_ev_txkeep, EVCNT_TYPE_MISC,
		    NULL, s->sc_dev.dv_xname, "pass3tx-keep");
	}
#endif
}

/*
 *  SBMAC_CHANNEL_START(s)
 *
 *  Start packet processing on this MAC.
 *
 *  Input parameters:
 *	s - sbmac structure
 *
 *  Return value:
 *	nothing
 */

static void
sbmac_channel_start(struct sbmac_softc *s)
{
	uint64_t reg;
	sbmac_port_t port;
	uint64_t cfg, fifo, framecfg;
	int idx;
	uint64_t dma_cfg0, fifo_cfg;
	sbmacdma_t *txdma;

	/*
	 * Don't do this if running
	 */

	if (s->sbm_state == sbmac_state_on)
		return;

	/*
	 * Bring the controller out of reset, but leave it off.
	 */

	SBMAC_WRITECSR(s->sbm_macenable, 0);

	/*
	 * Ignore all received packets
	 */

	SBMAC_WRITECSR(s->sbm_rxfilter, 0);

	/*
	 * Calculate values for various control registers.
	 */

	cfg = M_MAC_RETRY_EN |
	      M_MAC_TX_HOLD_SOP_EN |
	      V_MAC_TX_PAUSE_CNT_16K |
	      M_MAC_AP_STAT_EN |
	      M_MAC_SS_EN |
	      0;

	fifo = V_MAC_TX_WR_THRSH(4) |	/* Must be '4' or '8' */
	       V_MAC_TX_RD_THRSH(4) |
	       V_MAC_TX_RL_THRSH(4) |
	       V_MAC_RX_PL_THRSH(4) |
	       V_MAC_RX_RD_THRSH(4) |	/* Must be '4' */
	       V_MAC_RX_PL_THRSH(4) |
	       V_MAC_RX_RL_THRSH(8) |
	       0;

	framecfg = V_MAC_MIN_FRAMESZ_DEFAULT |
	    V_MAC_MAX_FRAMESZ_DEFAULT |
	    V_MAC_BACKOFF_SEL(1);

	/*
	 * Clear out the hash address map
	 */

	port = PKSEG1(s->sbm_base + R_MAC_HASH_BASE);
	for (idx = 0; idx < MAC_HASH_COUNT; idx++) {
		SBMAC_WRITECSR(port, 0);
		port += sizeof(uint64_t);
	}

	/*
	 * Clear out the exact-match table
	 */

	port = PKSEG1(s->sbm_base + R_MAC_ADDR_BASE);
	for (idx = 0; idx < MAC_ADDR_COUNT; idx++) {
		SBMAC_WRITECSR(port, 0);
		port += sizeof(uint64_t);
	}

	/*
	 * Clear out the DMA Channel mapping table registers
	 */

	port = PKSEG1(s->sbm_base + R_MAC_CHUP0_BASE);
	for (idx = 0; idx < MAC_CHMAP_COUNT; idx++) {
		SBMAC_WRITECSR(port, 0);
		port += sizeof(uint64_t);
	}

	port = PKSEG1(s->sbm_base + R_MAC_CHLO0_BASE);
	for (idx = 0; idx < MAC_CHMAP_COUNT; idx++) {
		SBMAC_WRITECSR(port, 0);
		port += sizeof(uint64_t);
	}

	/*
	 * Program the hardware address.  It goes into the hardware-address
	 * register as well as the first filter register.
	 */

	reg = sbmac_addr2reg(s->sbm_hwaddr);

	port = PKSEG1(s->sbm_base + R_MAC_ADDR_BASE);
	SBMAC_WRITECSR(port, reg);
	port = PKSEG1(s->sbm_base + R_MAC_ETHERNET_ADDR);
	SBMAC_WRITECSR(port, 0);			// pass1 workaround

	/*
	 * Set the receive filter for no packets, and write values
	 * to the various config registers
	 */

	SBMAC_WRITECSR(s->sbm_rxfilter, 0);
	SBMAC_WRITECSR(s->sbm_imr, 0);
	SBMAC_WRITECSR(s->sbm_framecfg, framecfg);
	SBMAC_WRITECSR(s->sbm_fifocfg, fifo);
	SBMAC_WRITECSR(s->sbm_maccfg, cfg);

	/*
	 * Initialize DMA channels (rings should be ok now)
	 */

	sbdma_channel_start(&(s->sbm_rxdma));
	sbdma_channel_start(&(s->sbm_txdma));

	/*
	 * Configure the speed, duplex, and flow control
	 */

	sbmac_set_speed(s, s->sbm_speed);
	sbmac_set_duplex(s, s->sbm_duplex, s->sbm_fc);

	/*
	 * Fill the receive ring
	 */

	sbdma_fillring(&(s->sbm_rxdma));

	/*
	 * Turn on the rest of the bits in the enable register
	 */

	SBMAC_WRITECSR(s->sbm_macenable, M_MAC_RXDMA_EN0 | M_MAC_TXDMA_EN0 |
	    M_MAC_RX_ENABLE | M_MAC_TX_ENABLE);


	/*
	 * Accept any kind of interrupt on TX and RX DMA channel 0
	 */
	SBMAC_WRITECSR(s->sbm_imr,
	    (M_MAC_INT_CHANNEL << S_MAC_TX_CH0) |
	    (M_MAC_INT_CHANNEL << S_MAC_RX_CH0));

	/*
	 * Enable receiving unicasts and broadcasts
	 */

	SBMAC_WRITECSR(s->sbm_rxfilter, M_MAC_UCAST_EN | M_MAC_BCAST_EN);

	/*
	 * On chips which support unaligned DMA features, set the descriptor
	 * ring for transmit channels to use the unaligned buffer format.
	 */
	txdma = &(s->sbm_txdma); 

	if (s->sbm_pass3_dma) {
		dma_cfg0 = SBMAC_READCSR(txdma->sbdma_config0);
		dma_cfg0 |= V_DMA_DESC_TYPE(K_DMA_DESC_TYPE_RING_UAL_RMW) |
		    M_DMA_TBX_EN | M_DMA_TDX_EN;
		SBMAC_WRITECSR(txdma->sbdma_config0,dma_cfg0);

		fifo_cfg =  SBMAC_READCSR(s->sbm_fifocfg);
		fifo_cfg |= V_MAC_TX_WR_THRSH(8) |
		    V_MAC_TX_RD_THRSH(8) | V_MAC_TX_RL_THRSH(8);
		SBMAC_WRITECSR(s->sbm_fifocfg,fifo_cfg);
	}

	/*
	 * we're running now.
	 */

	s->sbm_state = sbmac_state_on;
	s->sc_ethercom.ec_if.if_flags |= IFF_RUNNING;

	/*
	 * Program multicast addresses
	 */

	sbmac_setmulti(s);

	/*
	 * If channel was in promiscuous mode before, turn that on
	 */

	if (s->sc_ethercom.ec_if.if_flags & IFF_PROMISC)
		sbmac_promiscuous_mode(s, 1);

	/*
	 * Turn on the once-per-second timer
	 */

	callout_reset(&(s->sc_tick_ch), hz, sbmac_tick, s);
}

/*
 *  SBMAC_CHANNEL_STOP(s)
 *
 *  Stop packet processing on this MAC.
 *
 *  Input parameters:
 *	s - sbmac structure
 *
 *  Return value:
 *	nothing
 */

static void
sbmac_channel_stop(struct sbmac_softc *s)
{
	uint64_t ctl;

	/* don't do this if already stopped */

	if (s->sbm_state == sbmac_state_off)
		return;

	/* don't accept any packets, disable all interrupts */

	SBMAC_WRITECSR(s->sbm_rxfilter, 0);
	SBMAC_WRITECSR(s->sbm_imr, 0);

	/* Turn off ticker */

	callout_stop(&(s->sc_tick_ch));

	/* turn off receiver and transmitter */

	ctl = SBMAC_READCSR(s->sbm_macenable);
	ctl &= ~(M_MAC_RXDMA_EN0 | M_MAC_TXDMA_EN0);
	SBMAC_WRITECSR(s->sbm_macenable, ctl);

	/* We're stopped now. */

	s->sbm_state = sbmac_state_off;
	s->sc_ethercom.ec_if.if_flags &= ~IFF_RUNNING;

	/* Empty the receive and transmit rings */

	sbdma_emptyring(&(s->sbm_rxdma));
	sbdma_emptyring(&(s->sbm_txdma));
}

/*
 *  SBMAC_SET_CHANNEL_STATE(state)
 *
 *  Set the channel's state ON or OFF
 *
 *  Input parameters:
 *	state - new state
 *
 *  Return value:
 *	old state
 */

static sbmac_state_t
sbmac_set_channel_state(struct sbmac_softc *sc, sbmac_state_t state)
{
	sbmac_state_t oldstate = sc->sbm_state;

	/*
	 * If same as previous state, return
	 */

	if (state == oldstate)
		return oldstate;

	/*
	 * If new state is ON, turn channel on
	 */

	if (state == sbmac_state_on)
		sbmac_channel_start(sc);
	else
		sbmac_channel_stop(sc);

	/*
	 * Return previous state
	 */

	return oldstate;
}

/*
 *  SBMAC_PROMISCUOUS_MODE(sc, onoff)
 *
 *  Turn on or off promiscuous mode
 *
 *  Input parameters:
 *	sc - softc
 *	onoff - 1 to turn on, 0 to turn off
 *
 *  Return value:
 *	nothing
 */

static void
sbmac_promiscuous_mode(struct sbmac_softc *sc, int onoff)
{
	uint64_t reg;

	if (sc->sbm_state != sbmac_state_on)
		return;

	if (onoff) {
		reg = SBMAC_READCSR(sc->sbm_rxfilter);
		reg |= M_MAC_ALLPKT_EN;
		SBMAC_WRITECSR(sc->sbm_rxfilter, reg);
	} else {
		reg = SBMAC_READCSR(sc->sbm_rxfilter);
		reg &= ~M_MAC_ALLPKT_EN;
		SBMAC_WRITECSR(sc->sbm_rxfilter, reg);
	}
}

/*
 *  SBMAC_INIT_AND_START(sc)
 *
 *  Stop the channel and restart it.  This is generally used
 *  when we have to do something to the channel that requires
 *  a swift kick.
 *
 *  Input parameters:
 *	sc - softc
 */

static void
sbmac_init_and_start(struct sbmac_softc *sc)
{
	int s;

	s = splnet();

	mii_pollstat(&sc->sc_mii);		/* poll phy for current speed */
	sbmac_mii_statchg((struct device *) sc); /* set state to new speed */
	sbmac_set_channel_state(sc, sbmac_state_on);

	splx(s);
}

/*
 *  SBMAC_ADDR2REG(ptr)
 *
 *  Convert six bytes into the 64-bit register value that
 *  we typically write into the SBMAC's address/mcast registers
 *
 *  Input parameters:
 *	ptr - pointer to 6 bytes
 *
 *  Return value:
 *	register value
 */

static uint64_t
sbmac_addr2reg(u_char *ptr)
{
	uint64_t reg = 0;

	ptr += 6;

	reg |= (uint64_t) *(--ptr);
	reg <<= 8;
	reg |= (uint64_t) *(--ptr);
	reg <<= 8;
	reg |= (uint64_t) *(--ptr);
	reg <<= 8;
	reg |= (uint64_t) *(--ptr);
	reg <<= 8;
	reg |= (uint64_t) *(--ptr);
	reg <<= 8;
	reg |= (uint64_t) *(--ptr);

	return reg;
}

/*
 *  SBMAC_SET_SPEED(s, speed)
 *
 *  Configure LAN speed for the specified MAC.
 *  Warning: must be called when MAC is off!
 *
 *  Input parameters:
 *	s - sbmac structure
 *	speed - speed to set MAC to (see sbmac_speed_t enum)
 *
 *  Return value:
 *	1 if successful
 *	0 indicates invalid parameters
 */

static int
sbmac_set_speed(struct sbmac_softc *s, sbmac_speed_t speed)
{
	uint64_t cfg;
	uint64_t framecfg;

	/*
	 * Save new current values
	 */

	s->sbm_speed = speed;

	if (s->sbm_state != sbmac_state_off)
		panic("sbmac_set_speed while MAC not off");

	/*
	 * Read current register values
	 */

	cfg = SBMAC_READCSR(s->sbm_maccfg);
	framecfg = SBMAC_READCSR(s->sbm_framecfg);

	/*
	 * Mask out the stuff we want to change
	 */

	cfg &= ~(M_MAC_BURST_EN | M_MAC_SPEED_SEL);
	framecfg &= ~(M_MAC_IFG_RX | M_MAC_IFG_TX | M_MAC_IFG_THRSH |
	    M_MAC_SLOT_SIZE);

	/*
	 * Now add in the new bits
	 */

	switch (speed) {
	case sbmac_speed_10:
		framecfg |= V_MAC_IFG_RX_10 |
		    V_MAC_IFG_TX_10 |
		    K_MAC_IFG_THRSH_10 |
		    V_MAC_SLOT_SIZE_10;
		cfg |= V_MAC_SPEED_SEL_10MBPS;
		break;

	case sbmac_speed_100:
		framecfg |= V_MAC_IFG_RX_100 |
		    V_MAC_IFG_TX_100 |
		    V_MAC_IFG_THRSH_100 |
		    V_MAC_SLOT_SIZE_100;
		cfg |= V_MAC_SPEED_SEL_100MBPS ;
		break;

	case sbmac_speed_1000:
		framecfg |= V_MAC_IFG_RX_1000 |
		    V_MAC_IFG_TX_1000 |
		    V_MAC_IFG_THRSH_1000 |
		    V_MAC_SLOT_SIZE_1000;
		cfg |= V_MAC_SPEED_SEL_1000MBPS | M_MAC_BURST_EN;
		break;

	case sbmac_speed_auto:		/* XXX not implemented */
		/* fall through */
	default:
		return 0;
	}

	/*
	 * Send the bits back to the hardware
	 */

	SBMAC_WRITECSR(s->sbm_framecfg, framecfg);
	SBMAC_WRITECSR(s->sbm_maccfg, cfg);

	return 1;
}

/*
 *  SBMAC_SET_DUPLEX(s, duplex, fc)
 *
 *  Set Ethernet duplex and flow control options for this MAC
 *  Warning: must be called when MAC is off!
 *
 *  Input parameters:
 *	s - sbmac structure
 *	duplex - duplex setting (see sbmac_duplex_t)
 *	fc - flow control setting (see sbmac_fc_t)
 *
 *  Return value:
 *	1 if ok
 *	0 if an invalid parameter combination was specified
 */

static int
sbmac_set_duplex(struct sbmac_softc *s, sbmac_duplex_t duplex, sbmac_fc_t fc)
{
	uint64_t cfg;

	/*
	 * Save new current values
	 */

	s->sbm_duplex = duplex;
	s->sbm_fc = fc;

	if (s->sbm_state != sbmac_state_off)
		panic("sbmac_set_duplex while MAC not off");

	/*
	 * Read current register values
	 */

	cfg = SBMAC_READCSR(s->sbm_maccfg);

	/*
	 * Mask off the stuff we're about to change
	 */

	cfg &= ~(M_MAC_FC_SEL | M_MAC_FC_CMD | M_MAC_HDX_EN);

	switch (duplex) {
	case sbmac_duplex_half:
		switch (fc) {
		case sbmac_fc_disabled:
			cfg |= M_MAC_HDX_EN | V_MAC_FC_CMD_DISABLED;
			break;

		case sbmac_fc_collision:
			cfg |= M_MAC_HDX_EN | V_MAC_FC_CMD_ENABLED;
			break;

		case sbmac_fc_carrier:
			cfg |= M_MAC_HDX_EN | V_MAC_FC_CMD_ENAB_FALSECARR;
			break;

		case sbmac_fc_auto:		/* XXX not implemented */
			/* fall through */
		case sbmac_fc_frame:		/* not valid in half duplex */
		default:			/* invalid selection */
			panic("%s: invalid half duplex fc selection %d",
			    s->sc_dev.dv_xname, fc);
			return 0;
		}
		break;

	case sbmac_duplex_full:
		switch (fc) {
		case sbmac_fc_disabled:
			cfg |= V_MAC_FC_CMD_DISABLED;
			break;

		case sbmac_fc_frame:
			cfg |= V_MAC_FC_CMD_ENABLED;
			break;

		case sbmac_fc_collision:	/* not valid in full duplex */
		case sbmac_fc_carrier:		/* not valid in full duplex */
		case sbmac_fc_auto:		/* XXX not implemented */
			/* fall through */
		default:
			panic("%s: invalid full duplex fc selection %d",
			    s->sc_dev.dv_xname, fc);
			return 0;
		}
		break;

	default:
		/* fall through */
	case sbmac_duplex_auto:
		panic("%s: bad duplex %d", s->sc_dev.dv_xname, duplex);
		/* XXX not implemented */
		break;
	}

	/*
	 * Send the bits back to the hardware
	 */

	SBMAC_WRITECSR(s->sbm_maccfg, cfg);

	return 1;
}

/*
 *  SBMAC_INTR()
 *
 *  Interrupt handler for MAC interrupts
 *
 *  Input parameters:
 *	MAC structure
 *
 *  Return value:
 *	nothing
 */

/* ARGSUSED */
static void
sbmac_intr(void *xsc, uint32_t status, uint32_t pc)
{
	struct sbmac_softc *sc = (struct sbmac_softc *) xsc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint64_t isr;

	for (;;) {

		/*
		 * Read the ISR (this clears the bits in the real register)
		 */

		isr = SBMAC_READCSR(sc->sbm_isr);

		if (isr == 0)
			break;

		/*
		 * Transmits on channel 0
		 */

		if (isr & (M_MAC_INT_CHANNEL << S_MAC_TX_CH0)) {
			sbdma_tx_process(sc, &(sc->sbm_txdma));
			SBMAC_EVCNT_INCR(sc->sbm_ev_txintr);
		}

		/*
		 * Receives on channel 0
		 */

		if (isr & (M_MAC_INT_CHANNEL << S_MAC_RX_CH0)) {
			sbdma_rx_process(sc, &(sc->sbm_rxdma));
			SBMAC_EVCNT_INCR(sc->sbm_ev_rxintr);
		}
	}

	/* try to get more packets going */
	sbmac_start(ifp);
}


/*
 *  SBMAC_START(ifp)
 *
 *  Start output on the specified interface.  Basically, we
 *  queue as many buffers as we can until the ring fills up, or
 *  we run off the end of the queue, whichever comes first.
 *
 *  Input parameters:
 *	ifp - interface
 *
 *  Return value:
 *	nothing
 */

static void
sbmac_start(struct ifnet *ifp)
{
	struct sbmac_softc	*sc;
	struct mbuf		*m_head = NULL;
	int			rv;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	sc = ifp->if_softc;

	for (;;) {

		IF_DEQUEUE(&ifp->if_snd, m_head);
		if (m_head == NULL)
		    break;

		/*
		 * Put the buffer on the transmit ring.  If we
		 * don't have room, set the OACTIVE flag and wait
		 * for the NIC to drain the ring.
		 */

		rv = sbdma_add_txbuffer(&(sc->sbm_txdma), m_head);

		if (rv == 0) {
			/*
			 * If there's a BPF listener, bounce a copy of this
			 * frame to it.
			 */
#if (NBPFILTER > 0)
			if (ifp->if_bpf)
				bpf_mtap(ifp->if_bpf, m_head);
#endif
			if (!sc->sbm_pass3_dma) {
				/*
				 * Don't free mbuf if we're not copying to new
				 * mbuf in sbdma_add_txbuffer.  It will be
				 * freed in sbdma_tx_process.
				 */
				m_freem(m_head);
			}
		} else {
		    IF_PREPEND(&ifp->if_snd, m_head);
		    ifp->if_flags |= IFF_OACTIVE;
		    break;
		}
	}
}

/*
 *  SBMAC_SETMULTI(sc)
 *
 *  Reprogram the multicast table into the hardware, given
 *  the list of multicasts associated with the interface
 *  structure.
 *
 *  Input parameters:
 *	sc - softc
 *
 *  Return value:
 *	nothing
 */

static void
sbmac_setmulti(struct sbmac_softc *sc)
{
	struct ifnet *ifp;
	uint64_t reg;
	sbmac_port_t port;
	int idx;
	struct ether_multi *enm;
	struct ether_multistep step;

	ifp = &sc->sc_ethercom.ec_if;

	/*
	 * Clear out entire multicast table.  We do this by nuking
	 * the entire hash table and all the direct matches except
	 * the first one, which is used for our station address
	 */

	for (idx = 1; idx < MAC_ADDR_COUNT; idx++) {
		port = PKSEG1(sc->sbm_base +
		    R_MAC_ADDR_BASE+(idx*sizeof(uint64_t)));
		SBMAC_WRITECSR(port, 0);
	}

	for (idx = 0; idx < MAC_HASH_COUNT; idx++) {
		port = PKSEG1(sc->sbm_base +
		    R_MAC_HASH_BASE+(idx*sizeof(uint64_t)));
		SBMAC_WRITECSR(port, 0);
	}

	/*
	 * Clear the filter to say we don't want any multicasts.
	 */

	reg = SBMAC_READCSR(sc->sbm_rxfilter);
	reg &= ~(M_MAC_MCAST_INV | M_MAC_MCAST_EN);
	SBMAC_WRITECSR(sc->sbm_rxfilter, reg);

	if (ifp->if_flags & IFF_ALLMULTI) {
		/*
		 * Enable ALL multicasts.  Do this by inverting the
		 * multicast enable bit.
		 */
		reg = SBMAC_READCSR(sc->sbm_rxfilter);
		reg |= (M_MAC_MCAST_INV | M_MAC_MCAST_EN);
		SBMAC_WRITECSR(sc->sbm_rxfilter, reg);
		return;
	}

	/*
	 * Progam new multicast entries.  For now, only use the
	 * perfect filter.  In the future we'll need to use the
	 * hash filter if the perfect filter overflows
	 */

	/*
	 * XXX only using perfect filter for now, need to use hash
	 * XXX if the table overflows
	 */

	idx = 1;		/* skip station address */
	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
	while ((enm != NULL) && (idx < MAC_ADDR_COUNT)) {
		reg = sbmac_addr2reg(enm->enm_addrlo);
		port = PKSEG1(sc->sbm_base +
		    R_MAC_ADDR_BASE+(idx*sizeof(uint64_t)));
		SBMAC_WRITECSR(port, reg);
		idx++;
		ETHER_NEXT_MULTI(step, enm);
	}

	/*
	 * Enable the "accept multicast bits" if we programmed at least one
	 * multicast.
	 */

	if (idx > 1) {
	    reg = SBMAC_READCSR(sc->sbm_rxfilter);
	    reg |= M_MAC_MCAST_EN;
	    SBMAC_WRITECSR(sc->sbm_rxfilter, reg);
	}
}

/*
 *  SBMAC_ETHER_IOCTL(ifp, cmd, data)
 *
 *  Generic IOCTL requests for this interface.  The basic
 *  stuff is handled here for bringing the interface up,
 *  handling multicasts, etc.
 *
 *  Input parameters:
 *	ifp - interface structure
 *	cmd - command code
 *	data - pointer to data
 *
 *  Return value:
 *	return value (0 is success)
 */

static int
sbmac_ether_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct sbmac_softc *sc = ifp->if_softc;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			sbmac_init_and_start(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
		case AF_NS:
		{
			struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)LLADDR(ifp->if_sadl);
			else
				bcopy(ina->x_host.c_host, LLADDR(ifp->if_sadl),
				    ifp->if_addrlen);
			/* Set new address. */
			sbmac_init_and_start(sc);
			break;
		}
#endif
		default:
			sbmac_init_and_start(sc);
			break;
		}
		break;

	default:
		return (EINVAL);
	}

	return (0);
}

/*
 *  SBMAC_IOCTL(ifp, command, data)
 *
 *  Main IOCTL handler - dispatches to other IOCTLs for various
 *  types of requests.
 *
 *  Input parameters:
 *	ifp - interface pointer
 *	command - command code
 *	data - pointer to argument data
 *
 *  Return value:
 *	0 if ok
 *	else error code
 */

static int
sbmac_ioctl(struct ifnet *ifp, u_long command, caddr_t data)
{
	struct sbmac_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *) data;
	int s, error = 0;

	s = splnet();

	switch(command) {
	case SIOCSIFADDR:
	case SIOCGIFADDR:
		error = sbmac_ether_ioctl(ifp, command, data);
		break;
	case SIOCSIFMTU:
		if (ifr->ifr_mtu > ETHER_MAX_LEN)
			error = EINVAL;
		else {
			ifp->if_mtu = ifr->ifr_mtu;
			/* XXX Program new MTU here */
		}
		break;
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			/*
			 * If only the state of the PROMISC flag changed,
			 * just tweak the hardware registers.
			 */
			if ((ifp->if_flags & IFF_RUNNING) &&
			    (ifp->if_flags & IFF_PROMISC)) {
				/* turn on promiscuous mode */
				sbmac_promiscuous_mode(sc, 1);
			} else if (ifp->if_flags & IFF_RUNNING &&
			    !(ifp->if_flags & IFF_PROMISC)) {
			    /* turn off promiscuous mode */
			    sbmac_promiscuous_mode(sc, 0);
			} else
			    sbmac_set_channel_state(sc, sbmac_state_on);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				sbmac_set_channel_state(sc, sbmac_state_off);
		}

		sc->sbm_if_flags = ifp->if_flags;
		error = 0;
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (ifp->if_flags & IFF_RUNNING) {
			sbmac_setmulti(sc);
			error = 0;
		}
		break;
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, command);
		break;
	default:
		error = EINVAL;
		break;
	}

	(void)splx(s);

	return(error);
}

/*
 *  SBMAC_IFMEDIA_UPD(ifp)
 *
 *  Configure an appropriate media type for this interface,
 *  given the data in the interface structure
 *
 *  Input parameters:
 *	ifp - interface
 *
 *  Return value:
 *	0 if ok
 *	else error code
 */

static int
sbmac_mediachange(struct ifnet *ifp)
{
	struct sbmac_softc *sc = ifp->if_softc;

	if (ifp->if_flags & IFF_UP)
		mii_mediachg(&sc->sc_mii);
	return(0);
}

/*
 *  SBMAC_IFMEDIA_STS(ifp, ifmr)
 *
 *  Report current media status (used by ifconfig, for example)
 *
 *  Input parameters:
 *	ifp - interface structure
 *	ifmr - media request structure
 *
 *  Return value:
 *	nothing
 */

static void
sbmac_mediastatus(struct ifnet *ifp, struct ifmediareq *req)
{
	struct sbmac_softc	*sc = ifp->if_softc;

  	mii_pollstat(&sc->sc_mii);
	req->ifm_status = sc->sc_mii.mii_media_status;
	req->ifm_active = sc->sc_mii.mii_media_active;
}

/*
 *  SBMAC_WATCHDOG(ifp)
 *
 *  Called periodically to make sure we're still happy.
 *
 *  Input parameters:
 *	ifp - interface structure
 *
 *  Return value:
 *	nothing
 */

static void
sbmac_watchdog(struct ifnet *ifp)
{

	/* XXX do something */
}

/*
 * One second timer, used to tick MII.
 */
static void
sbmac_tick(void *arg)
{
	struct sbmac_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_tick_ch, hz, sbmac_tick, sc);
}


/*
 *  SBMAC_MATCH(parent, match, aux)
 *
 *  Part of the config process - see if this device matches the
 *  info about what we expect to find on the bus.
 *
 *  Input parameters:
 *	parent - parent bus structure
 *	match -
 *	aux - bus-specific args
 *
 *  Return value:
 *	1 if we match
 *	0 if we don't match
 */

static int
sbmac_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct sbobio_attach_args *sap = aux;

	/*
	 * Make sure it's a MAC
	 */

	if (sap->sa_locs.sa_type != SBOBIO_DEVTYPE_MAC)
		return 0;

	/*
	 * Yup, it is.
	 */

	return 1;
}

/*
 *  SBMAC_PARSE_XDIGIT(str)
 *
 *  Parse a hex digit, returning its value
 *
 *  Input parameters:
 *	str - character
 *
 *  Return value:
 *	hex value, or -1 if invalid
 */

static int
sbmac_parse_xdigit(char str)
{
	int digit;

	if ((str >= '0') && (str <= '9'))
		digit = str - '0';
	else if ((str >= 'a') && (str <= 'f'))
		digit = str - 'a' + 10;
	else if ((str >= 'A') && (str <= 'F'))
		digit = str - 'A' + 10;
	else
		digit = -1;

	return digit;
}

/*
 *  SBMAC_PARSE_HWADDR(str, hwaddr)
 *
 *  Convert a string in the form xx:xx:xx:xx:xx:xx into a 6-byte
 *  Ethernet address.
 *
 *  Input parameters:
 *	str - string
 *	hwaddr - pointer to hardware address
 *
 *  Return value:
 *	0 if ok, else -1
 */

static int
sbmac_parse_hwaddr(char *str, u_char *hwaddr)
{
	int digit1, digit2;
	int idx = 6;

	while (*str && (idx > 0)) {
		digit1 = sbmac_parse_xdigit(*str);
		if (digit1 < 0)
			return -1;
		str++;
		if (!*str)
			return -1;

		if ((*str == ':') || (*str == '-')) {
			digit2 = digit1;
			digit1 = 0;
		} else {
			digit2 = sbmac_parse_xdigit(*str);
			if (digit2 < 0)
				return -1;
			str++;
		}

		*hwaddr++ = (digit1 << 4) | digit2;
		idx--;

		if (*str == '-')
			str++;
		if (*str == ':')
			str++;
	}
	return 0;
}

/*
 *  SBMAC_ATTACH(parent, self, aux)
 *
 *  Attach routine - init hardware and hook ourselves into NetBSD.
 *
 *  Input parameters:
 *	parent - parent bus device
 *	self - our softc
 *	aux - attach data
 *
 *  Return value:
 *	nothing
 */

static void
sbmac_attach(struct device *parent, struct device *self, void *aux)
{
	struct ifnet *ifp;
	struct sbmac_softc *sc;
	struct sbobio_attach_args *sap = aux;
	u_char *eaddr;
	static int unit = 0;	/* XXX */
	uint64_t ea_reg;
	int idx;

	sc = (struct sbmac_softc *)self;

	/* Determine controller base address */

	sc->sbm_base = (sbmac_port_t) sap->sa_base + sap->sa_locs.sa_offset;

	eaddr = sc->sbm_hwaddr;

	/*
	 * Initialize context (get pointers to registers and stuff), then
	 * allocate the memory for the descriptor tables.
	 */

	sbmac_initctx(sc);

	callout_init(&(sc->sc_tick_ch));

	/*
	 * Read the ethernet address.  The firwmare left this programmed
	 * for us in the ethernet address register for each mac.
	 */

	ea_reg = SBMAC_READCSR(PKSEG1(sc->sbm_base + R_MAC_ETHERNET_ADDR));
	for (idx = 0; idx < 6; idx++) {
		eaddr[idx] = (uint8_t) (ea_reg & 0xFF);
		ea_reg >>= 8;
	}

#define	SBMAC_DEFAULT_HWADDR "40:00:00:00:01:00"
	if (eaddr[0] == 0 && eaddr[1] == 0 && eaddr[2] == 0 &&
		eaddr[3] == 0 && eaddr[4] == 0 && eaddr[5] == 0) {
		sbmac_parse_hwaddr(SBMAC_DEFAULT_HWADDR, eaddr);
		eaddr[5] = unit;
	}

#ifdef SBMAC_ETH0_HWADDR
	if (unit == 0)
		sbmac_parse_hwaddr(SBMAC_ETH0_HWADDR, eaddr);
#endif
#ifdef SBMAC_ETH1_HWADDR
	if (unit == 1)
		sbmac_parse_hwaddr(SBMAC_ETH1_HWADDR, eaddr);
#endif
#ifdef SBMAC_ETH2_HWADDR
	if (unit == 2)
		sbmac_parse_hwaddr(SBMAC_ETH2_HWADDR, eaddr);
#endif
	unit++;

	/*
	 * Display Ethernet address (this is called during the config process
	 * so we need to finish off the config message that was being displayed)
	 */
	printf(": Ethernet%s\n",
	    sc->sbm_pass3_dma ? ", using unaligned tx DMA" : "");
	printf("%s: Ethernet address: %s\n", self->dv_xname,
	    ether_sprintf(eaddr));


	/*
	 * Set up ifnet structure
	 */

	ifp = &sc->sc_ethercom.ec_if;
	ifp->if_softc = sc;
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST |
	    IFF_NOTRAILERS;
	ifp->if_ioctl = sbmac_ioctl;
	ifp->if_start = sbmac_start;
	ifp->if_watchdog = sbmac_watchdog;
	ifp->if_snd.ifq_maxlen = SBMAC_MAX_TXDESCR - 1;

	/*
	 * Set up ifmedia support.
	 */

	/*
	 * Initialize MII/media info.
	 */
	sc->sc_mii.mii_ifp      = ifp;
	sc->sc_mii.mii_readreg  = sbmac_mii_readreg;
	sc->sc_mii.mii_writereg = sbmac_mii_writereg;
	sc->sc_mii.mii_statchg  = sbmac_mii_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, sbmac_mediachange,
	    sbmac_mediastatus);
	mii_attach(&sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, 0);

	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else {
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
	}


	/*
	 * map/route interrupt
	 */

	sc->sbm_intrhand = cpu_intr_establish(sap->sa_locs.sa_intr[0], IPL_NET,
	    sbmac_intr, sc);

	/*
	 * Call MI attach routines.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, eaddr);
}
