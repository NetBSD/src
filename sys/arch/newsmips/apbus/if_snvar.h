/*	$NetBSD: if_snvar.h,v 1.12.32.1 2011/06/06 09:06:19 jruoho Exp $	*/

/*
 * Copyright (c) 1991   Algorithmics Ltd (http://www.algor.co.uk)
 * You may use, copy, and modify this program so long as you retain the
 * copyright line.
 */

/*
 * if_snvar.h -- National Semiconductor DP8393X (SONIC) NetBSD/newsmips vars
 */

/*
 * Memory access macros. Since we handle SONIC in 16 bit mode (PB5X0)
 * and 32 bit mode (everything else) using a single GENERIC kernel
 * binary, all structures have to be accessed using macros which can
 * adjust the offsets appropriately.
 */
#define	SWO(m, a, o, x)	(*(uint32_t *)((uint32_t *)(a) + (o)) = (x))
#define	SRO(m, a, o)	(*(uint32_t *)((uint32_t *)(a) + (o)) & 0xffff)

/*
 * Register access macros. We use bus_space_* to talk to the Sonic
 * registers. A mapping table is used in case a particular configuration
 * hooked the regs up at non-word offsets.
 */
#define	NIC_GET(sc, reg)	((sc)->sc_regbase[(reg) * 4 + 3])
#define	NIC_PUT(sc, reg, val)	((sc)->sc_regbase[(reg) * 4 + 3] = val)

#define	SONIC_GETDMA(p)	((uint32_t)(p))

#define	SN_REGSIZE	(SN_NREGS * 4)

#include <mips/locore.h>

/*
 * buffer sizes in 32 bit mode
 * 1 TXpkt is 4 hdr words + (3 * FRAGMAX) + 1 link word == 23 words == 92 bytes
 *
 * 1 RxPkt is 7 words == 28 bytes
 * 1 Rda   is 4 words == 16 bytes
 *
 * The CDA is 17 words == 68 bytes
 *
 * total space in page 0 = NTDA * 92 + NRRA * 16 + NRDA * 28 + 68
 */

#define NRBA    4		/* # receive buffers < NRRA */
#define RBAMASK (NRBA - 1)
#define NTDA    4		/* # transmit descriptors */
#define NRRA    8		/* # receive resource descriptors */
#define RRAMASK (NRRA - 1)	/* the reason why NRRA must be power of two */

#define FCSSIZE 4		/* size of FCS appended to packets */

/*
 * maximum receive packet size plus 2 byte pad to make each
 * one aligned. 4 byte slop (required for eobc)
 */
#define RBASIZE(sc)	(ETHER_HDR_LEN + ETHERMTU + FCSSIZE + 6)

/*
 * transmit buffer area
 */
#define TXBSIZE	1536	/* 6*2^8 -- the same size as the 8390 TXBUF */

#define	SN_NPAGES	2 + NRBA + (NTDA / 2)

typedef struct mtd {
	void		*mtd_txp;
	uint32_t	mtd_vtxp;
	void 		*mtd_buf;
	uint32_t	mtd_vbuf;
	struct mbuf	*mtd_mbuf;
} mtd_t;

/*
 * The sn_softc for NEWS5000 if_sn.
 */
struct sn_softc {
	device_t	sc_dev;
	struct ethercom	sc_ethercom;
#define sc_if	sc_ethercom.ec_if	/* network visible interface */

	void *		sc_hwbase;	/* hardware base address */
	volatile uint16_t *sc_regbase;	/* register base address */

	int		bitmode;	/* 32 bit mode == 1, 16 == 0 */

	uint16_t	snr_dcr;	/* DCR for this instance */
	uint16_t	snr_dcr2;	/* DCR2 for this instance */
	int		slotno;		/* Slot number */

	int		sc_rramark;	/* index into p_rra of wp */
	void		*p_rra[NRRA];	/* RX resource descs */
	uint32_t	v_rra[NRRA];	/* DMA addresses of p_rra */
	uint32_t	v_rea;		/* ptr to the end of the rra space */

	int		sc_rxmark;	/* current hw pos in rda ring */
	int		sc_rdamark;	/* current sw pos in rda ring */
	int		sc_nrda;	/* total number of RDAs */
	void		*p_rda;
	uint32_t	v_rda;

	void 		*rbuf[NRBA];

	struct mtd	mtda[NTDA];
	int		mtd_hw;		/* idx of first mtd given to hw */
	int		mtd_prev;	/* idx of last mtd given to hardware */
	int		mtd_free;	/* next free mtd to use */
	int		mtd_tlinko;	/*
					 * offset of tlink of last txp given
					 * to SONIC. Need to clear EOL on
					 * this word to add a desc.
					 */
	int		mtd_pint;	/* Counter to set TXP_PINT */

	void		*p_cda;
	uint32_t	v_cda;

	unsigned char	*space;
};

/*
 * Accessing SONIC data structures and registers as 32 bit values
 * makes code endianess independent.  The SONIC is however always in
 * bigendian mode so it is necessary to ensure that data structures shared
 * between the CPU and the SONIC are always in bigendian order.
 */

/*
 * Receive Resource Descriptor
 * This structure describes the buffers into which packets
 * will be received.  Note that more than one packet may be
 * packed into a single buffer if constraints permit.
 */
#define	RXRSRC_PTRLO	0	/* buffer address LO */
#define	RXRSRC_PTRHI	1	/* buffer address HI */
#define	RXRSRC_WCLO	2	/* buffer size (16bit words) LO */
#define	RXRSRC_WCHI	3	/* buffer size (16bit words) HI */

#define	RXRSRC_SIZE(sc)	(4 * 4)

/*
 * Receive Descriptor
 * This structure holds information about packets received.
 */
#define	RXPKT_STATUS	0
#define	RXPKT_BYTEC	1
#define	RXPKT_PTRLO	2
#define	RXPKT_PTRHI	3
#define	RXPKT_SEQNO	4
#define	RXPKT_RLINK	5
#define	RXPKT_INUSE	6
#define	RXPKT_SIZE(sc)	(7 * 4)

#define RBASEQ(x) (((x) >> 8) & 0xff)
#define PSNSEQ(x) ((x) & 0xff)

/*
 * Transmit Descriptor
 * This structure holds information about packets to be transmitted.
 */
#define FRAGMAX	8		/* maximum number of fragments in a packet */

#define	TXP_STATUS	0	/* + transmitted packet status */
#define	TXP_CONFIG	1	/* transmission configuration */
#define	TXP_PKTSIZE	2	/* entire packet size in bytes */
#define	TXP_FRAGCNT	3	/* # fragments in packet */

#define	TXP_FRAGOFF	4	/* offset to first fragment */
#define	TXP_FRAGSIZE	3	/* size of each fragment desc */
#define	TXP_FPTRLO	0	/* ptr to packet fragment LO */
#define	TXP_FPTRHI	1	/* ptr to packet fragment HI */
#define	TXP_FSIZE	2	/* fragment size */

#define	TXP_WORDS	(TXP_FRAGOFF + (FRAGMAX * TXP_FRAGSIZE) + 1)	/* 1 for tlink */
#define	TXP_SIZE(sc)	(TXP_WORDS*4)

#define EOL	0x0001		/* end of list marker for link fields */

/*
 * CDA, the CAM descriptor area. The SONIC has a 16 entry CAM to
 * match incoming addresses against. It is programmed via DMA
 * from a memory region.
 */
#define MAXCAM	16	/* number of user entries in CAM */
#define	CDA_CAMDESC	4	/* # words i na descriptor */
#define	CDA_CAMEP	0	/* CAM Address Port 0 xx-xx-xx-xx-YY-YY */
#define	CDA_CAMAP0	1	/* CAM Address Port 1 xx-xx-YY-YY-xx-xx */
#define	CDA_CAMAP1	2	/* CAM Address Port 2 YY-YY-xx-xx-xx-xx */
#define	CDA_CAMAP2	3
#define	CDA_ENABLE	64	/* mask enabling CAM entries */
#define	CDA_SIZE(sc)	((4 * 16 + 1) * ((sc->bitmode) ? 4 : 2))

int	snsetup(struct sn_softc *sc, uint8_t *);
int	snintr(void *);
void	sn_md_init(struct sn_softc *);
