/*	$NetBSD: enreg.h,v 1.2 2002/09/11 01:46:30 mycroft Exp $ */
/*
 * Copyright (c) 1997 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
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
 * Ethernet chip description
 */

struct en_regs {
    unsigned char txstat; /* 0 */
    unsigned char txmask; /* 1 */
    unsigned char rxstat; /* 2 */
    unsigned char rxmask; /* 3 */
    unsigned char txmode; /* 4 */
    unsigned char rxmode; /* 5 */
    unsigned char reset; /* 6 */
    char pad;
    unsigned char addr[6]; /* 8 to d - "physical" network address */
};

#define EN_TXS_READY     0x80 /* ready for packet */
#define EN_TXS_BUSY      0x40 /* receive carrier detect */
#define EN_TXS_TXRECV    0x20 /* transmission received */
#define EN_TXS_SHORTED   0x10 /* possible coax short */
#define EN_TXS_UNDERFLOW 0x08 /* underflow on xmit */
#define EN_TXS_COLLERR   0x04 /* collision detected */
#define EN_TXS_COLLERR16 0x02 /* 16th collision error */
#define EN_TXS_PARERR    0x01 /* parity error in tx data */

#define EN_TXM_READYIE     0x80 /* tx int on packet ready */
#define EN_TXM_TXRXIE      0x20 /* tx int on transmit rec'd */
#define EN_TXM_UNDERFLOWIE 0x08 /* tx int on underflow */
#define EN_TXM_COLLIE      0x04 /* tx int on collision */
#define EN_TXM_COLL16IE    0x02 /* tx int on 16th collision */
#define EN_TXM_PARERRIE    0x01 /* tx int on parity error */

/* same bits for rxstat and rxmask */
#define EN_RX_OK       0x80 /* packet received ok */
#define EN_RX_RESET    0x10 /* reset packet received */
#define EN_RX_SHORT    0x08 /* < minimum length */
#define EN_RX_ALIGNERR 0x04 /* alignment error */
#define EN_RX_CRCERR   0x02 /* CRC error */
#define EN_RX_OVERFLOW 0x01 /* receiver FIFO overflow */

#define EN_TMD_COLLMASK   0xf0 /* collision count */
#define EN_TMD_COLLSHIFT     4
#define EN_TMD_PARIGNORE  0x08 /* ignore parity */
#define EN_TMD_TURBO1	0x04
#define EN_TMD_LB_DISABLE 0x02 /* loop back disabled */
#define EN_TMD_DISCONTENT 0x01 /* disable contention (rx carrier) */

#define EN_RMD_TEST         0x80 /* must be zero */
#define EN_RMD_ADDRSIZE     0x10 /* reduces NODE match to 5 chars */
#define EN_RMD_SHORTENABLE  0x08 /* "rx packets >= 10 bytes" - <? */
#define EN_RMD_RESETENABLE  0x04 /* dunno, treat as MBZ */
#define EN_RMD_WHATRECV     0x03 /* controls what packets are received */
#define EN_RMD_RECV_PROMISC 0x03 /* all packets */
#define EN_RMD_RECV_MULTI   0x02 /* accept broad/multicasts */
#define EN_RMD_RECV_NORMAL  0x01 /* accept broad/limited multicasts */
#define EN_RMD_RECV_NONE    0x00 /* accept no packets */

#define EN_RST_RESET 0x80 /* reset interface */

#define ENRX_EOP 0x80000000 /* end-of-packet flag */
#define ENRX_BOP 0x40000000 /* beginning-of-packet flag */
#define ENTX_EOP 0x80000000 /* end-of-packet flag */

