/*	$NetBSD: mb8795reg.h,v 1.2 2001/03/31 06:56:54 dbj Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Darrin B. Jewell
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Fujitsu Ethernet Data Link Controller (MB8795)
 * and the Fujitsu Manchester Encoder/Decoder (MB502).
 */

#if 0
struct xe_regs {
  unsigned char	xe_txstat;      /* tx status */
  unsigned char xe_txmask;      /* tx interrupt condition mask */
  unsigned char xe_rxstat;      /* rx status */
  unsigned char xe_rxmask;      /* rx interrupt condition mask */
  unsigned char xe_txmode;      /* tx control/mode register */
  unsigned char xe_rxmode;      /* rx control/mode register */
  unsigned char xe_reset;       /* reset mode */
  unsigned char xe_tdc_lsb;     /* transmit data count LSB */
  unsigned char xe_addr[6];     /* physical address */
  unsigned char xe_reserved;
  unsigned char xe_tdc_msb;     /* transmit data count MSB */
};
#endif

/* transmitter status (address 0) */
#define XE_TXSTAT               0

#define XE_TXSTAT_READY		0x80	/* ready for packet */
#define XE_TXSTAT_BUSY		0x40	/* receive carrier detect */
#define XE_TXSTAT_TXRECV	0x20	/* transmission received */
#define XE_TXSTAT_SHORTED	0x10	/* possible coax short */
#define XE_TXSTAT_UNDERFLOW	0x08	/* underflow on xmit */
#define XE_TXSTAT_COLLERR	0x04	/* collision detected */
#define XE_TXSTAT_COLLERR16	0x02	/* 16th collision error */
#define XE_TXSTAT_PARERR	0x01	/* parity error in tx data */
#define XE_TXSTAT_CLEAR		0xff	/* clear all status bits */

#define XE_TXSTAT_BITS \
"\20\10READY\07BUSY\06TXRECV\05SHORTED\
\04UNDERFLOW\03COLLERR\02COLLERR16\01PARERR"

/* transmit masks (address 1) */
#define XE_TXMASK               1

#define XE_TXMASK_READYIE	0x80	/* tx int on packet ready */
#define XE_TXMASK_TXRXIE	0x20	/* tx int on transmit rec'd */
#define XE_TXMASK_UNDERFLOWIE	0x08	/* tx int on underflow */
#define XE_TXMASK_COLLIE	0x04	/* tx int on collision */
#define XE_TXMASK_COLL16IE	0x02	/* tx int on 16th collision */
#define XE_TXMASK_PARERRIE	0x01	/* tx int on parity error */

#define XE_TXMASK_BITS \
"\20\10READYIE\06TXRXIE\04UNDERFLOWIE\03COLLIE\02COLL16IE\01PARERRIE"

/* cummulative receiver status (address 2) */
#define XE_RXSTAT               2

#define XE_RXSTAT_OK		0x80	/* packet received is correct */
#define XE_RXSTAT_RESET		0x10	/* reset packet received */
#define XE_RXSTAT_SHORT		0x08	/* < minimum length */
#define XE_RXSTAT_ALIGNERR	0x04	/* alignment error */
#define XE_RXSTAT_CRCERR	0x02	/* CRC error */
#define XE_RXSTAT_OVERFLOW	0x01	/* receiver FIFO overflow */
#define XE_RXSTAT_CLEAR		0xff	/* clear all status bits */

#define XE_RXSTAT_BITS \
"\20\10OK\05RESET\04SHORT\03ALIGNERR\02CRCERR\01OVERFLOW"

/* receiver masks (address 3) */
#define XE_RXMASK               3

#define XE_RXMASK_OKIE		0x80	/* rx int on packet ok */
#define XE_RXMASK_RESETIE	0x10	/* rx int on reset packet */
#define XE_RXMASK_SHORTIE	0x08	/* rx int on short packet */
#define XE_RXMASK_ALIGNERRIE	0x04	/* rx int on align error */
#define XE_RXMASK_CRCERRIE	0x02	/* rx int on CRC error */
#define XE_RXMASK_OVERFLOWIE	0x01	/* rx int on overflow error */

#define XE_RXMASK_BITS \
"\20\10OKIE\05RESETIE\04SHORTIE\03ALIGNERRIE\02CRCERRIE\01OVERFLOWIE"

/* transmitter mode (address 4) */
#define XE_TXMODE               4

#define XE_TXMODE_COLLMASK	0xF0	/* collision count */
#define XE_TXMODE_PARIGNORE	0x08	/* ignore parity */
#define XE_TXMODE_LB_DISABLE	0x02	/* loop back disabled */
#define XE_TXMODE_DISCONTENT	0x01	/* disable contention (rx carrier) */

/* Should probably figure out how to put in the COLLMASK value in here */
#define XE_TXMODE_BITS \
"\20\04PARIGNORE\02LB_DISABLE\01DISCONTENT"

/* receiver mode (address 5) */
#define XE_RXMODE               5

#define XE_RXMODE_TEST		0x80	/* Must be zero for normal op */
#define XE_RXMODE_ADDRSIZE	0x10	/* reduces NODE match to 5 chars */
#define XE_RXMODE_SHORTENABLE	0x08	/* rx packets >= 10 bytes */
#define XE_RXMODE_RESETENABLE	0x04	/* must be zero */
#define XE_RXMODE_PROMISCUOUS	0x03	/* accept all packets */
#define XE_RXMODE_MULTICAST	0x02	/* accept broad/multicasts */
#define XE_RXMODE_NORMAL	0x01	/* accept broad/limited multicasts */
#define XE_RXMODE_OFF		0x00	/* accept no packets */

/* this define is less useful for the promiscuous bits, bit I leave it here */
#define XE_RXMODE_BITS \
"\20\10TEST\05ADDRSIZE\04SHORTENABLE\03RESETENABLE\02MULTICAST\01NORMAL"

/* reset mode (address 6) */
#define XE_RESET_MODE		0x80	/* reset mode */
#define XE_RESET                6

#define XE_TDC_LSB              7
#define XE_ENADDR               8
#define XE_TDC_MSB              15

#define	ENRX_EOP	0x80000000	/* end-of-packet flag */
#define	ENRX_BOP	0x40000000	/* beginning-of-packet flag */
#define ENTX_EOP	0x80000000	/* end-of-packet flag */

/* Size of register area to be mapped */
#define XE_SIZE                 16
