/*
 * LANCE Ethernet driver header file
 *
 * Copyright (c) 1994 Charles Hannum.
 *
 * Copyright (C) 1993, Paul Richards. This software may be used, modified,
 *   copied, distributed, and sold, in both source and binary form provided
 *   that the above copyright and these terms are retained. Under no
 *   circumstances is the author responsible for the proper functioning
 *   of this software, nor does the author assume any responsibility
 *   for damages incurred with its use.
 *
 *	$Id: if_lereg.h,v 1.1 1994/07/01 20:25:27 mycroft Exp $
 */

/* Declarations specific to this driver */
#define NTBUF 2
#define TLEN 1
#define NRBUF 8
#define RLEN 3
#define BUFSIZE 1518

/* Board types */
#define	BICC		1
#define	BICC_RDP	0xc
#define	BICC_RAP	0xe
#define	NE2100		2
#define	NE2100_RDP	0x10
#define	NE2100_RAP	0x12
#define	DEPCA		3
#define	DEPCA_RDP	0x4
#define	DEPCA_RAP	0x6

/* Chip types */
#define	PROBE_MASK	0x0007
#define	LANCE		1
#define	LANCE_MASK	0x0007
#define	PCnet_ISA	2
#define	PCnet_ISA_MASK	0x0000

/* Control and status register 0 flags */
#define ERR	0x8000
#define BABL	0x4000
#define CERR	0x2000
#define MISS	0x1000
#define MERR	0x0800
#define RINT	0x0400
#define TINT	0x0200
#define IDON	0x0100
#define INTR	0x0080
#define INEA	0x0040
#define RXON	0x0020
#define TXON	0x0010
#define TDMD	0x0008
#define STOP	0x0004
#define STRT	0x0002
#define INIT	0x0001

/* Coontrol and status register 3 flags */
#define BSWP	0x0004
#define ACON	0x0002
#define BCON	0x0001

/* Initialisation block (must be on word boundary) */
struct init_block {
	u_short mode;		/* Mode register */
	u_char padr[6];		/* Ethernet address */
	u_long ladrf[2];	/* Logical address filter (multicast) */
	u_short rdra;		/* Low order pointer to receive ring */
	u_short rlen;		/* High order pointer and no. rings */
	u_short tdra;		/* Low order pointer to transmit ring */
	u_short tlen;		/* High order pointer and no rings */
};

/* Mode settings */
#define PROM	0x8000		/* Promiscuous */
#define INTL	0x0040		/* Internal loopback */
#define DRTY	0x0020		/* Disable retry */
#define COLL	0x0010		/* Force collision */
#define DTCR	0x0008		/* Disable transmit crc */
#define LOOP	0x0004		/* Loop back */
#define DTX	0x0002		/* Disable transmitter */
#define DRX	0x0001		/* Disable receiver */

/* Message descriptor structure */
struct mds {
	u_short addr;
	u_short flags;
	u_short bcnt;
	u_short mcnt;
};

/* Receive ring status flags */
#define OWN	0x8000		/* Owner bit, 0=host, 1=Lance */
#define MDERR	0x4000		/* Error */
#define FRAM	0x2000		/* Framing error error */
#define OFLO	0x1000		/* Silo overflow */
#define CRC	0x0800		/* CRC error */
#define RBUFF	0x0400		/* Buffer error */
#define STP	0x0200		/* Start of packet */
#define ENP	0x0100		/* End of packet */

/* Transmit ring flags */
#define MORE	0x1000		/* More than 1 retry */
#define ONE	0x0800		/* One retry */
#define DEF	0x0400		/* Deferred transmit */

/* Transmit errors */
#define TBUFF	0x8000		/* Buffer error */
#define UFLO	0x4000		/* Silo underflow */
#define LCOL	0x1000		/* Late collision */
#define LCAR	0x0800		/* Loss of carrier */
#define RTRY	0x0400		/* Tried 16 times */


/* DEPCA-specific definitions */
#define	DEPCA_CSR	0x0
#define	DEPCA_CSR_SHE		0x80	/* Shared memory enabled */
#define	DEPCA_CSR_SWAP32	0x40	/* Byte swapped */
#define	DEPCA_CSR_DUM		0x08	/* rev E compatibility */
#define	DEPCA_CSR_IM		0x04	/* Interrupt masked */
#define	DEPCA_CSR_IEN		0x02	/* Interrupt enabled */
#define	DEPCA_CSR_NORMAL \
	(DEPCA_CSR_SHE | DEPCA_CSR_DUM | DEPCA_CSR_IEN)

#define	DEPCA_ADP	0xc
