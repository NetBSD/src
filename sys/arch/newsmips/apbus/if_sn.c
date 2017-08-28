/*	$NetBSD: if_sn.c,v 1.34.14.4 2017/08/28 17:51:47 skrll Exp $	*/

/*
 * National Semiconductor  DP8393X SONIC Driver
 * Copyright (c) 1991   Algorithmics Ltd (http://www.algor.co.uk)
 * You may use, copy, and modify this program so long as you retain the
 * copyright line.
 *
 * This driver has been substantially modified since Algorithmics donated
 * it.
 *
 *   Denton Gentry <denny1@home.com>
 * and also
 *   Yanagisawa Takeshi <yanagisw@aa.ap.titech.ac.jp>
 * did the work to get this running on the Macintosh.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sn.c,v 1.34.14.4 2017/08/28 17:51:47 skrll Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#include <uvm/uvm_extern.h>

#include <net/bpf.h>
#include <net/bpfdesc.h>

#include <machine/cpu.h>
#include <newsmips/apbus/apbusvar.h>
#include <newsmips/apbus/if_snreg.h>
#include <newsmips/apbus/if_snvar.h>

/* #define SONIC_DEBUG */

#ifdef SONIC_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

static void	snwatchdog(struct ifnet *);
static int	sninit(struct sn_softc *sc);
static int	snstop(struct sn_softc *sc);
static int	snioctl(struct ifnet *ifp, u_long cmd, void *data);
static void	snstart(struct ifnet *ifp);
static void	snreset(struct sn_softc *sc);

static void	caminitialise(struct sn_softc *);
static void	camentry(struct sn_softc *, int, const u_char *ea);
static void	camprogram(struct sn_softc *);
static void	initialise_tda(struct sn_softc *);
static void	initialise_rda(struct sn_softc *);
static void	initialise_rra(struct sn_softc *);
#ifdef SNDEBUG
static void	camdump(struct sn_softc *sc);
#endif

static void	sonictxint(struct sn_softc *);
static void	sonicrxint(struct sn_softc *);

static inline u_int	sonicput(struct sn_softc *sc, struct mbuf *m0,
    int mtd_next);
static inline int	sonic_read(struct sn_softc *, void *, int);
static inline struct mbuf *sonic_get(struct sn_softc *, void *, int);

int sndebug = 0;

/*
 * SONIC buffers need to be aligned 16 or 32 bit aligned.
 * These macros calculate and verify alignment.
 */
#define SOALIGN(m, array)	(m ? (roundup((int)array, 4)) : \
				     (roundup((int)array, 2)))

#define LOWER(x) ((unsigned)(x) & 0xffff)
#define UPPER(x) ((unsigned)(x) >> 16)

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
int
snsetup(struct sn_softc	*sc, uint8_t *lladdr)
{
	struct ifnet *ifp = &sc->sc_if;
	uint8_t	*p;
	uint8_t	*pp;
	int	i;

	if (sc->space == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "memory allocation for descriptors failed\n");
		return 1;
	}

	/*
	 * Put the pup in reset mode (sninit() will fix it later),
	 * stop the timer, disable all interrupts and clear any interrupts.
	 */
	NIC_PUT(sc, SNR_CR, CR_STP);
	wbflush();
	NIC_PUT(sc, SNR_CR, CR_RST);
	wbflush();
	NIC_PUT(sc, SNR_IMR, 0);
	wbflush();
	NIC_PUT(sc, SNR_ISR, ISR_ALL);
	wbflush();

	/*
	 * because the SONIC is basically 16bit device it 'concatenates'
	 * a higher buffer address to a 16 bit offset--this will cause wrap
	 * around problems near the end of 64k !!
	 */
	p = sc->space;
	pp = (uint8_t *)roundup((int)p, PAGE_SIZE);
	p = pp;

	for (i = 0; i < NRRA; i++) {
		sc->p_rra[i] = (void *)p;
		sc->v_rra[i] = SONIC_GETDMA(p);
		p += RXRSRC_SIZE(sc);
	}
	sc->v_rea = SONIC_GETDMA(p);

	p = (uint8_t *)SOALIGN(sc, p);

	sc->p_cda = (void *)(p);
	sc->v_cda = SONIC_GETDMA(p);
	p += CDA_SIZE(sc);

	p = (uint8_t *)SOALIGN(sc, p);

	for (i = 0; i < NTDA; i++) {
		struct mtd *mtdp = &sc->mtda[i];
		mtdp->mtd_txp = (void *)p;
		mtdp->mtd_vtxp = SONIC_GETDMA(p);
		p += TXP_SIZE(sc);
	}

	p = (uint8_t *)SOALIGN(sc, p);

	if ((p - pp) > PAGE_SIZE) {
		aprint_error_dev(sc->sc_dev, "sizeof RRA (%ld) + CDA (%ld) +"
		    "TDA (%ld) > PAGE_SIZE (%d). Punt!\n",
		    (ulong)sc->p_cda - (ulong)sc->p_rra[0],
		    (ulong)sc->mtda[0].mtd_txp - (ulong)sc->p_cda,
		    (ulong)p - (ulong)sc->mtda[0].mtd_txp,
		    PAGE_SIZE);
		return 1;
	}

	p = pp + PAGE_SIZE;
	pp = p;

	sc->sc_nrda = PAGE_SIZE / RXPKT_SIZE(sc);
	sc->p_rda = (void *)p;
	sc->v_rda = SONIC_GETDMA(p);

	p = pp + PAGE_SIZE;

	for (i = 0; i < NRBA; i++) {
		sc->rbuf[i] = (void *)p;
		p += PAGE_SIZE;
	}

	pp = p;
	for (i = 0; i < NTDA; i++) {
		struct mtd *mtdp = &sc->mtda[i];

		mtdp->mtd_buf = p;
		mtdp->mtd_vbuf = SONIC_GETDMA(p);
		p += TXBSIZE;
	}

#ifdef SNDEBUG
	camdump(sc);
#endif
	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n",
	    ether_sprintf(lladdr));

#ifdef SNDEBUG
	aprint_debug_dev(sc->sc_dev, "buffers: rra=%p cda=%p rda=%p tda=%p\n",
	    device_xname(sc->sc_dev), sc->p_rra[0], sc->p_cda,
	    sc->p_rda, sc->mtda[0].mtd_txp);
#endif

	strcpy(ifp->if_xname, device_xname(sc->sc_dev));
	ifp->if_softc = sc;
	ifp->if_ioctl = snioctl;
	ifp->if_start = snstart;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;
	ifp->if_watchdog = snwatchdog;
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, lladdr);

	return 0;
}

static int
snioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ifaddr *ifa;
	struct sn_softc *sc = ifp->if_softc;
	int	s = splnet(), err = 0;
	int	temp;

	switch (cmd) {

	case SIOCINITIFADDR:
		ifa = (struct ifaddr *)data;
		ifp->if_flags |= IFF_UP;
		(void)sninit(sc);
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((err = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running,
			 * then stop it.
			 */
			snstop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped,
			 * then start it.
			 */
			(void)sninit(sc);
		} else {
			/*
			 * reset the interface to pick up any other changes
			 * in flags
			 */
			temp = ifp->if_flags & IFF_UP;
			snreset(sc);
			ifp->if_flags |= temp;
			snstart(ifp);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if ((err = ether_ioctl(ifp, cmd, data)) == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware
			 * filter accordingly. But remember UP flag!
			 */
			if (ifp->if_flags & IFF_RUNNING) {
				temp = ifp->if_flags & IFF_UP;
				snreset(sc);
				ifp->if_flags |= temp;
			}
			err = 0;
		}
		break;
	default:
		err = ether_ioctl(ifp, cmd, data);
		break;
	}
	splx(s);
	return err;
}

/*
 * Encapsulate a packet of type family for the local net.
 */
static void
snstart(struct ifnet *ifp)
{
	struct sn_softc	*sc = ifp->if_softc;
	struct mbuf	*m;
	int		mtd_next;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

outloop:
	/* Check for room in the xmit buffer. */
	if ((mtd_next = (sc->mtd_free + 1)) == NTDA)
		mtd_next = 0;

	if (mtd_next == sc->mtd_hw) {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	IF_DEQUEUE(&ifp->if_snd, m);
	if (m == 0)
		return;

	/* We need the header for m_pkthdr.len. */
	if ((m->m_flags & M_PKTHDR) == 0)
		panic("%s: snstart: no header mbuf", device_xname(sc->sc_dev));

	/*
	 * If bpf is listening on this interface, let it
	 * see the packet before we commit it to the wire.
	 */
	bpf_mtap(ifp, m);

	/*
	 * If there is nothing in the o/p queue, and there is room in
	 * the Tx ring, then send the packet directly.  Otherwise append
	 * it to the o/p queue.
	 */
	if ((sonicput(sc, m, mtd_next)) == 0) {
		IF_PREPEND(&ifp->if_snd, m);
		return;
	}

	sc->mtd_prev = sc->mtd_free;
	sc->mtd_free = mtd_next;

	ifp->if_opackets++;		/* # of pkts */

	/* Jump back for possibly more punishment. */
	goto outloop;
}

/*
 * reset and restart the SONIC.  Called in case of fatal
 * hardware/software errors.
 */
static void
snreset(struct sn_softc *sc)
{

	snstop(sc);
	sninit(sc);
}

static int 
sninit(struct sn_softc *sc)
{
	u_long	s_rcr;
	int	s;

	if (sc->sc_if.if_flags & IFF_RUNNING)
		/* already running */
		return 0;

	s = splnet();

	NIC_PUT(sc, SNR_CR, CR_RST);	/* DCR only accessible in reset mode! */

	/* config it */
	NIC_PUT(sc, SNR_DCR, (sc->snr_dcr |
		(sc->bitmode ? DCR_DW32 : DCR_DW16)));
	NIC_PUT(sc, SNR_DCR2, sc->snr_dcr2);

	s_rcr = RCR_BRD | RCR_LBNONE;
	if (sc->sc_if.if_flags & IFF_PROMISC)
		s_rcr |= RCR_PRO;
	if (sc->sc_if.if_flags & IFF_ALLMULTI)
		s_rcr |= RCR_AMC;
	NIC_PUT(sc, SNR_RCR, s_rcr);

#if 0
	NIC_PUT(sc, SNR_IMR, (IMR_PRXEN | IMR_PTXEN | IMR_TXEREN | IMR_LCDEN));
#else
	NIC_PUT(sc, SNR_IMR, IMR_PRXEN | IMR_PTXEN | IMR_TXEREN | IMR_LCDEN |
			     IMR_BREN  | IMR_HBLEN | IMR_RDEEN  | IMR_RBEEN |
			     IMR_RBAEEN | IMR_RFOEN);
#endif

	/* clear pending interrupts */
	NIC_PUT(sc, SNR_ISR, ISR_ALL);

	/* clear tally counters */
	NIC_PUT(sc, SNR_CRCT, -1);
	NIC_PUT(sc, SNR_FAET, -1);
	NIC_PUT(sc, SNR_MPT, -1);

	initialise_tda(sc);
	initialise_rda(sc);
	initialise_rra(sc);

	sn_md_init(sc);			/* MD initialization */

	/* enable the chip */
	NIC_PUT(sc, SNR_CR, 0);
	wbflush();

	/* program the CAM */
	camprogram(sc);

	/* get it to read resource descriptors */
	NIC_PUT(sc, SNR_CR, CR_RRRA);
	wbflush();
	while ((NIC_GET(sc, SNR_CR)) & CR_RRRA)
		continue;

	/* enable rx */
	NIC_PUT(sc, SNR_CR, CR_RXEN);
	wbflush();

	/* flag interface as "running" */
	sc->sc_if.if_flags |= IFF_RUNNING;
	sc->sc_if.if_flags &= ~IFF_OACTIVE;

	splx(s);
	return 0;
}

/*
 * close down an interface and free its buffers
 * Called on final close of device, or if sninit() fails
 * part way through.
 */
static int 
snstop(struct sn_softc *sc)
{
	struct mtd *mtd;
	int	s = splnet();

	/* stick chip in reset */
	NIC_PUT(sc, SNR_CR, CR_RST);
	wbflush();

	/* free all receive buffers (currently static so nothing to do) */

	/* free all pending transmit mbufs */
	while (sc->mtd_hw != sc->mtd_free) {
		mtd = &sc->mtda[sc->mtd_hw];
		if (mtd->mtd_mbuf)
			m_freem(mtd->mtd_mbuf);
		if (++sc->mtd_hw == NTDA) sc->mtd_hw = 0;
	}

	sc->sc_if.if_timer = 0;
	sc->sc_if.if_flags &= ~(IFF_RUNNING | IFF_UP);

	splx(s);
	return 0;
}

/*
 * Called if any Tx packets remain unsent after 5 seconds,
 * In all cases we just reset the chip, and any retransmission
 * will be handled by higher level protocol timeouts.
 */
static void
snwatchdog(struct ifnet *ifp)
{
	struct sn_softc *sc = ifp->if_softc;
	struct mtd *mtd;
	int	temp;

	if (sc->mtd_hw != sc->mtd_free) {
		/* something still pending for transmit */
		mtd = &sc->mtda[sc->mtd_hw];
		if (SRO(sc->bitmode, mtd->mtd_txp, TXP_STATUS) == 0)
			log(LOG_ERR, "%s: Tx - timeout\n",
			    device_xname(sc->sc_dev));
		else
			log(LOG_ERR, "%s: Tx - lost interrupt\n",
			    device_xname(sc->sc_dev));
		temp = ifp->if_flags & IFF_UP;
		snreset(sc);
		ifp->if_flags |= temp;
	}
}

/*
 * stuff packet into sonic (at splnet)
 */
static inline u_int
sonicput(struct sn_softc *sc, struct mbuf *m0, int mtd_next)
{
	struct mtd *mtdp;
	struct mbuf *m;
	u_char	*buff;
	void	*txp;
	u_int	len = 0;
	u_int	totlen = 0;

#ifdef whyonearthwouldyoudothis
	if (NIC_GET(sc, SNR_CR) & CR_TXP)
		return 0;
#endif

	/* grab the replacement mtd */
	mtdp = &sc->mtda[sc->mtd_free];

	buff = mtdp->mtd_buf;

	/* this packet goes to mtdnext fill in the TDA */
	mtdp->mtd_mbuf = m0;
	txp = mtdp->mtd_txp;

	/* Write to the config word. Every (NTDA/2)+1 packets we set an intr */
	if (sc->mtd_pint == 0) {
		sc->mtd_pint = NTDA/2;
		SWO(sc->bitmode, txp, TXP_CONFIG, TCR_PINT);
	} else {
		sc->mtd_pint--;
		SWO(sc->bitmode, txp, TXP_CONFIG, 0);
	}

	for (m = m0; m; m = m->m_next) {
		u_char *data = mtod(m, u_char *);
		len = m->m_len;
		totlen += len;
		memcpy(buff, data, len);
		buff += len;
	}
	if (totlen >= TXBSIZE) {
		panic("%s: sonicput: packet overflow",
		    device_xname(sc->sc_dev));
	}

	SWO(sc->bitmode, txp, TXP_FRAGOFF + (0 * TXP_FRAGSIZE) + TXP_FPTRLO,
	    LOWER(mtdp->mtd_vbuf));
	SWO(sc->bitmode, txp, TXP_FRAGOFF + (0 * TXP_FRAGSIZE) + TXP_FPTRHI,
	    UPPER(mtdp->mtd_vbuf));

	if (totlen < ETHERMIN + ETHER_HDR_LEN) {
		int pad = ETHERMIN + ETHER_HDR_LEN - totlen;
		memset((char *)mtdp->mtd_buf + totlen, 0, pad);
		totlen = ETHERMIN + ETHER_HDR_LEN;
	}

	SWO(sc->bitmode, txp, TXP_FRAGOFF + (0 * TXP_FRAGSIZE) + TXP_FSIZE,
	    totlen);
	SWO(sc->bitmode, txp, TXP_FRAGCNT, 1);
	SWO(sc->bitmode, txp, TXP_PKTSIZE, totlen);

	/* link onto the next mtd that will be used */
	SWO(sc->bitmode, txp, TXP_FRAGOFF + (1 * TXP_FRAGSIZE) + TXP_FPTRLO,
	    LOWER(sc->mtda[mtd_next].mtd_vtxp) | EOL);

	/*
	 * The previous txp.tlink currently contains a pointer to
	 * our txp | EOL. Want to clear the EOL, so write our
	 * pointer to the previous txp.
	 */
	SWO(sc->bitmode, sc->mtda[sc->mtd_prev].mtd_txp, sc->mtd_tlinko,
	    LOWER(mtdp->mtd_vtxp));

	/* make sure chip is running */
	wbflush();
	NIC_PUT(sc, SNR_CR, CR_TXP);
	wbflush();
	sc->sc_if.if_timer = 5;	/* 5 seconds to watch for failing to transmit */

	return totlen;
}

/*
 * These are called from sonicioctl() when /etc/ifconfig is run to set
 * the address or switch the i/f on.
 */
/*
 * CAM support
 */
static void 
caminitialise(struct sn_softc *sc)
{
	void	*p_cda = sc->p_cda;
	int	i;
	int	camoffset;

	for (i = 0; i < MAXCAM; i++) {
		camoffset = i * CDA_CAMDESC;
		SWO(bitmode, p_cda, (camoffset + CDA_CAMEP), i);
		SWO(bitmode, p_cda, (camoffset + CDA_CAMAP2), 0);
		SWO(bitmode, p_cda, (camoffset + CDA_CAMAP1), 0);
		SWO(bitmode, p_cda, (camoffset + CDA_CAMAP0), 0);
	}
	SWO(bitmode, p_cda, CDA_ENABLE, 0);
}

static void 
camentry(struct sn_softc *sc, int entry, const u_char *ea)
{
	void	*p_cda = sc->p_cda;
	int	camoffset = entry * CDA_CAMDESC;

	SWO(bitmode, p_cda, camoffset + CDA_CAMEP, entry);
	SWO(bitmode, p_cda, camoffset + CDA_CAMAP2, (ea[5] << 8) | ea[4]);
	SWO(bitmode, p_cda, camoffset + CDA_CAMAP1, (ea[3] << 8) | ea[2]);
	SWO(bitmode, p_cda, camoffset + CDA_CAMAP0, (ea[1] << 8) | ea[0]);
	SWO(bitmode, p_cda, CDA_ENABLE, 
	    (SRO(bitmode, p_cda, CDA_ENABLE) | (1 << entry)));
}

static void 
camprogram(struct sn_softc *sc)
{
	struct ether_multistep step;
	struct ether_multi *enm;
	struct ifnet *ifp;
	int	timeout;
	int	mcount = 0;

	caminitialise(sc);

	ifp = &sc->sc_if;

	/* Always load our own address first. */
	camentry(sc, mcount, CLLADDR(ifp->if_sadl));
	mcount++;

	/* Assume we won't need allmulti bit. */
	ifp->if_flags &= ~IFF_ALLMULTI;

	/* Loop through multicast addresses */
	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
	while (enm != NULL) {
		if (mcount == MAXCAM) {
			 ifp->if_flags |= IFF_ALLMULTI;
			 break;
		}

		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    sizeof(enm->enm_addrlo)) != 0) {
			/*
			 * SONIC's CAM is programmed with specific
			 * addresses. It has no way to specify a range.
			 * (Well, thats not exactly true. If the
			 * range is small one could program each addr
			 * within the range as a separate CAM entry)
			 */
			ifp->if_flags |= IFF_ALLMULTI;
			break;
		}

		/* program the CAM with the specified entry */
		camentry(sc, mcount, enm->enm_addrlo);
		mcount++;

		ETHER_NEXT_MULTI(step, enm);
	}

	NIC_PUT(sc, SNR_CDP, LOWER(sc->v_cda));
	NIC_PUT(sc, SNR_CDC, MAXCAM);
	NIC_PUT(sc, SNR_CR, CR_LCAM);
	wbflush();

	timeout = 10000;
	while ((NIC_GET(sc, SNR_CR) & CR_LCAM) && timeout--)
		delay(10);
	if (timeout == 0) {
		/* XXX */
		panic("%s: CAM initialisation failed",
		    device_xname(sc->sc_dev));
	}
	timeout = 10000;
	while (((NIC_GET(sc, SNR_ISR) & ISR_LCD) == 0) && timeout--)
		delay(10);

	if (NIC_GET(sc, SNR_ISR) & ISR_LCD)
		NIC_PUT(sc, SNR_ISR, ISR_LCD);
	else
		printf("%s: CAM initialisation without interrupt\n",
		    device_xname(sc->sc_dev));
}

#ifdef SNDEBUG
static void 
camdump(struct sn_softc *sc)
{
	int	i;

	printf("CAM entries:\n");
	NIC_PUT(sc, SNR_CR, CR_RST);
	wbflush();

	for (i = 0; i < 16; i++) {
		ushort  ap2, ap1, ap0;
		NIC_PUT(sc, SNR_CEP, i);
		wbflush();
		ap2 = NIC_GET(sc, SNR_CAP2);
		ap1 = NIC_GET(sc, SNR_CAP1);
		ap0 = NIC_GET(sc, SNR_CAP0);
		printf("%d: ap2=0x%x ap1=0x%x ap0=0x%x\n", i, ap2, ap1, ap0);
	}
	printf("CAM enable 0x%x\n", NIC_GET(sc, SNR_CEP));

	NIC_PUT(sc, SNR_CR, 0);
	wbflush();
}
#endif

static void 
initialise_tda(struct sn_softc *sc)
{
	struct mtd *mtd;
	int	i;

	for (i = 0; i < NTDA; i++) {
		mtd = &sc->mtda[i];
		mtd->mtd_mbuf = 0;
	}

	sc->mtd_hw = 0;
	sc->mtd_prev = NTDA - 1;
	sc->mtd_free = 0;
	sc->mtd_tlinko = TXP_FRAGOFF + 1*TXP_FRAGSIZE + TXP_FPTRLO;
	sc->mtd_pint = NTDA/2;

	NIC_PUT(sc, SNR_UTDA, UPPER(sc->mtda[0].mtd_vtxp));
	NIC_PUT(sc, SNR_CTDA, LOWER(sc->mtda[0].mtd_vtxp));
}

static void
initialise_rda(struct sn_softc *sc)
{
	int		i;
	char 		*p_rda = 0;
	uint32_t	v_rda = 0;

	/* link the RDA's together into a circular list */
	for (i = 0; i < (sc->sc_nrda - 1); i++) {
		p_rda = (char *)sc->p_rda + (i * RXPKT_SIZE(sc));
		v_rda = sc->v_rda + ((i+1) * RXPKT_SIZE(sc));
		SWO(bitmode, p_rda, RXPKT_RLINK, LOWER(v_rda));
		SWO(bitmode, p_rda, RXPKT_INUSE, 1);
	}
	p_rda = (char *)sc->p_rda + ((sc->sc_nrda - 1) * RXPKT_SIZE(sc));
	SWO(bitmode, p_rda, RXPKT_RLINK, LOWER(sc->v_rda) | EOL);
	SWO(bitmode, p_rda, RXPKT_INUSE, 1);

	/* mark end of receive descriptor list */
	sc->sc_rdamark = sc->sc_nrda - 1;

	sc->sc_rxmark = 0;

	NIC_PUT(sc, SNR_URDA, UPPER(sc->v_rda));
	NIC_PUT(sc, SNR_CRDA, LOWER(sc->v_rda));
	wbflush();
}

static void
initialise_rra(struct sn_softc *sc)
{
	int	i;
	u_int	v;
	int	bitmode = sc->bitmode;

	if (bitmode)
		NIC_PUT(sc, SNR_EOBC, RBASIZE(sc) / 2 - 2);
	else
		NIC_PUT(sc, SNR_EOBC, RBASIZE(sc) / 2 - 1);

	NIC_PUT(sc, SNR_URRA, UPPER(sc->v_rra[0]));
	NIC_PUT(sc, SNR_RSA, LOWER(sc->v_rra[0]));
	/* rea must point just past the end of the rra space */
	NIC_PUT(sc, SNR_REA, LOWER(sc->v_rea));
	NIC_PUT(sc, SNR_RRP, LOWER(sc->v_rra[0]));
	NIC_PUT(sc, SNR_RSC, 0);

	/* fill up SOME of the rra with buffers */
	for (i = 0; i < NRBA; i++) {
		v = SONIC_GETDMA(sc->rbuf[i]);
		SWO(bitmode, sc->p_rra[i], RXRSRC_PTRHI, UPPER(v));
		SWO(bitmode, sc->p_rra[i], RXRSRC_PTRLO, LOWER(v));
		SWO(bitmode, sc->p_rra[i], RXRSRC_WCHI, UPPER(PAGE_SIZE/2));
		SWO(bitmode, sc->p_rra[i], RXRSRC_WCLO, LOWER(PAGE_SIZE/2));
	}
	sc->sc_rramark = NRBA;
	NIC_PUT(sc, SNR_RWP, LOWER(sc->v_rra[sc->sc_rramark]));
	wbflush();
}

int
snintr(void *arg)
{
	struct sn_softc *sc = (struct sn_softc *)arg;
	int handled = 0;
	int	isr;

	while ((isr = (NIC_GET(sc, SNR_ISR) & ISR_ALL)) != 0) {
		/* scrub the interrupts that we are going to service */
		NIC_PUT(sc, SNR_ISR, isr);
		handled = 1;
		wbflush();

		if (isr & (ISR_BR | ISR_LCD | ISR_TC))
			printf("%s: unexpected interrupt status 0x%x\n",
			    device_xname(sc->sc_dev), isr);

		if (isr & (ISR_TXDN | ISR_TXER | ISR_PINT))
			sonictxint(sc);

		if (isr & ISR_PKTRX)
			sonicrxint(sc);

		if (isr & (ISR_HBL | ISR_RDE | ISR_RBE | ISR_RBAE | ISR_RFO)) {
			if (isr & ISR_HBL)
				/*
				 * The repeater is not providing a heartbeat.
				 * In itself this isn't harmful, lots of the
				 * cheap repeater hubs don't supply a heartbeat.
				 * So ignore the lack of heartbeat. Its only
				 * if we can't detect a carrier that we have a
				 * problem.
				 */
				;
			if (isr & ISR_RDE)
				printf("%s: receive descriptors exhausted\n",
				    device_xname(sc->sc_dev));
			if (isr & ISR_RBE)
				printf("%s: receive buffers exhausted\n",
				    device_xname(sc->sc_dev));
			if (isr & ISR_RBAE)
				printf("%s: receive buffer area exhausted\n",
				    device_xname(sc->sc_dev));
			if (isr & ISR_RFO)
				printf("%s: receive FIFO overrun\n",
				    device_xname(sc->sc_dev));
		}
		if (isr & (ISR_CRC | ISR_FAE | ISR_MP)) {
#ifdef notdef
			if (isr & ISR_CRC)
				sc->sc_crctally++;
			if (isr & ISR_FAE)
				sc->sc_faetally++;
			if (isr & ISR_MP)
				sc->sc_mptally++;
#endif
		}
		if_schedule_deferred_start(&sc->sc_if);
	}
	return handled;
}

/*
 * Transmit interrupt routine
 */
static void 
sonictxint(struct sn_softc *sc)
{
	struct mtd	*mtd;
	void		*txp;
	unsigned short	txp_status;
	int		mtd_hw;
	struct ifnet	*ifp = &sc->sc_if;

	mtd_hw = sc->mtd_hw;

	if (mtd_hw == sc->mtd_free)
		return;

	while (mtd_hw != sc->mtd_free) {
		mtd = &sc->mtda[mtd_hw];

		txp = mtd->mtd_txp;

		if (SRO(sc->bitmode, txp, TXP_STATUS) == 0) {
			break; /* it hasn't really gone yet */
		}

#ifdef SNDEBUG
		{
			struct ether_header *eh;

			eh = (struct ether_header *) mtd->mtd_buf;
			printf("%s: xmit status=0x%x len=%d type=0x%x from %s",
			    device_xname(sc->sc_dev),
			    SRO(sc->bitmode, txp, TXP_STATUS),
			    SRO(sc->bitmode, txp, TXP_PKTSIZE),
			    htons(eh->ether_type),
			    ether_sprintf(eh->ether_shost));
			printf(" (to %s)\n", ether_sprintf(eh->ether_dhost));
		}
#endif /* SNDEBUG */

		ifp->if_flags &= ~IFF_OACTIVE;

		if (mtd->mtd_mbuf != 0) {
			m_freem(mtd->mtd_mbuf);
			mtd->mtd_mbuf = 0;
		}
		if (++mtd_hw == NTDA) mtd_hw = 0;

		txp_status = SRO(sc->bitmode, txp, TXP_STATUS);

		ifp->if_collisions += (txp_status & TCR_EXC) ? 16 :
			((txp_status & TCR_NC) >> 12);

		if ((txp_status & TCR_PTX) == 0) {
			ifp->if_oerrors++;
			printf("%s: Tx packet status=0x%x\n",
			    device_xname(sc->sc_dev), txp_status);

			/* XXX - DG This looks bogus */
			if (mtd_hw != sc->mtd_free) {
				printf("resubmitting remaining packets\n");
				mtd = &sc->mtda[mtd_hw];
				NIC_PUT(sc, SNR_CTDA, LOWER(mtd->mtd_vtxp));
				NIC_PUT(sc, SNR_CR, CR_TXP);
				wbflush();
				break;
			}
		}
	}

	sc->mtd_hw = mtd_hw;
	return;
}

/*
 * Receive interrupt routine
 */
static void 
sonicrxint(struct sn_softc *sc)
{
	void *	rda;
	int	orra;
	int	len;
	int	rramark;
	int	rdamark;
	uint16_t rxpkt_ptr;

	rda = (char *)sc->p_rda + (sc->sc_rxmark * RXPKT_SIZE(sc));

	while (SRO(bitmode, rda, RXPKT_INUSE) == 0) {
		u_int status = SRO(bitmode, rda, RXPKT_STATUS);

		orra = RBASEQ(SRO(bitmode, rda, RXPKT_SEQNO)) & RRAMASK;
		rxpkt_ptr = SRO(bitmode, rda, RXPKT_PTRLO);
		len = SRO(bitmode, rda, RXPKT_BYTEC) - FCSSIZE;
		if (status & RCR_PRX) {
			void *pkt =
			    (char *)sc->rbuf[orra & RBAMASK] +
				 (rxpkt_ptr & PGOFSET);
			if (sonic_read(sc, pkt, len) == 0)
				sc->sc_if.if_ierrors++;
		} else
			sc->sc_if.if_ierrors++;

		/*
		 * give receive buffer area back to chip.
		 *
		 * If this was the last packet in the RRA, give the RRA to
		 * the chip again.
		 * If sonic read didnt copy it out then we would have to
		 * wait !!
		 * (dont bother add it back in again straight away)
		 *
		 * Really, we're doing p_rra[rramark] = p_rra[orra] but
		 * we have to use the macros because SONIC might be in
		 * 16 or 32 bit mode.
		 */
		if (status & RCR_LPKT) {
			void *tmp1, *tmp2;

			rramark = sc->sc_rramark;
			tmp1 = sc->p_rra[rramark];
			tmp2 = sc->p_rra[orra];
			SWO(bitmode, tmp1, RXRSRC_PTRLO,
			    SRO(bitmode, tmp2, RXRSRC_PTRLO));
			SWO(bitmode, tmp1, RXRSRC_PTRHI,
			    SRO(bitmode, tmp2, RXRSRC_PTRHI));
			SWO(bitmode, tmp1, RXRSRC_WCLO,
			    SRO(bitmode, tmp2, RXRSRC_WCLO));
			SWO(bitmode, tmp1, RXRSRC_WCHI,
			    SRO(bitmode, tmp2, RXRSRC_WCHI));

			/* zap old rra for fun */
			SWO(bitmode, tmp2, RXRSRC_WCHI, 0);
			SWO(bitmode, tmp2, RXRSRC_WCLO, 0);

			sc->sc_rramark = (++rramark) & RRAMASK;
			NIC_PUT(sc, SNR_RWP, LOWER(sc->v_rra[rramark]));
			wbflush();
		}

		/*
		 * give receive descriptor back to chip simple
		 * list is circular
		 */
		rdamark = sc->sc_rdamark;
		SWO(bitmode, rda, RXPKT_INUSE, 1);
		SWO(bitmode, rda, RXPKT_RLINK,
		    SRO(bitmode, rda, RXPKT_RLINK) | EOL);
		SWO(bitmode, ((char *)sc->p_rda + (rdamark * RXPKT_SIZE(sc))),
		    RXPKT_RLINK,
		    SRO(bitmode, ((char *)sc->p_rda +
			(rdamark * RXPKT_SIZE(sc))),
		    RXPKT_RLINK) & ~EOL);
		sc->sc_rdamark = sc->sc_rxmark;

		if (++sc->sc_rxmark >= sc->sc_nrda)
			sc->sc_rxmark = 0;
		rda = (char *)sc->p_rda + (sc->sc_rxmark * RXPKT_SIZE(sc));
	}
}

/*
 * sonic_read -- pull packet off interface and forward to
 * appropriate protocol handler
 */
static inline int 
sonic_read(struct sn_softc *sc, void *pkt, int len)
{
	struct ifnet *ifp = &sc->sc_if;
	struct mbuf *m;

#ifdef SNDEBUG
	{
		printf("%s: rcvd %p len=%d type=0x%x from %s",
		    devoce_xname(sc->sc_dev), et, len, htons(et->ether_type),
		    ether_sprintf(et->ether_shost));
		printf(" (to %s)\n", ether_sprintf(et->ether_dhost));
	}
#endif /* SNDEBUG */

	if (len < (ETHER_MIN_LEN - ETHER_CRC_LEN) ||
	    len > (ETHER_MAX_LEN - ETHER_CRC_LEN)) {
		printf("%s: invalid packet length %d bytes\n",
		    device_xname(sc->sc_dev), len);
		return 0;
	}

	m = sonic_get(sc, pkt, len);
	if (m == NULL)
		return 0;
	if_percpuq_enqueue(ifp->if_percpuq, m);
	return 1;
}

/*
 * munge the received packet into an mbuf chain
 */
static inline struct mbuf *
sonic_get(struct sn_softc *sc, void *pkt, int datalen)
{
	struct	mbuf *m, *top, **mp;
	int	len;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return 0;
	m_set_rcvif(m, &sc->sc_if);
	m->m_pkthdr.len = datalen;
	len = MHLEN;
	top = 0;
	mp = &top;

	while (datalen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return 0;
			}
			len = MLEN;
		}
		if (datalen >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				if (top)
					m_freem(top);
				else
					m_freem(m);
				return 0;
			}
			len = MCLBYTES;
		}

		if (mp == &top) {
			char *newdata = (char *)
			    ALIGN((char *)m->m_data + 
				sizeof(struct ether_header)) -
			    sizeof(struct ether_header);
			len -= newdata - m->m_data; 
			m->m_data = newdata;
		}

		m->m_len = len = min(datalen, len);

		memcpy(mtod(m, void *), pkt, (unsigned) len);
		pkt = (char *)pkt + len;
		datalen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	return top;
}
