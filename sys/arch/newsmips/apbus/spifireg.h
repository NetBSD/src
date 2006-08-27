/*	$NetBSD: spifireg.h,v 1.2 2006/08/27 08:56:03 tsutsui Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

struct spifi_reg {
	volatile uint32_t spstat;	/* RO: SPIFI state		*/
	volatile uint32_t cmlen;	/* RW: Command/message length	*/
	volatile uint32_t cmdpage;	/* RW: Command page		*/
	volatile uint32_t count_hi;	/* RW: Data count (high)	*/
	volatile uint32_t count_mid;	/* RW:            (mid)		*/
	volatile uint32_t count_low;	/* RW:            (low)		*/
	volatile uint32_t svptr_hi;	/* RO: Saved data pointer (high)*/
	volatile uint32_t svptr_mid;	/* RO:                    (mid)	*/
	volatile uint32_t svptr_low;	/* RO:                    (low) */
	volatile uint32_t intr;		/* RW: Processor interrupt	*/
	volatile uint32_t imask;	/* RW: Processor interrupt mask	*/
	volatile uint32_t prctrl;	/* RW: Processor control	*/
	volatile uint32_t prstat;	/* RO: Processor status		*/
	volatile uint32_t init_status;	/* RO: Initiator status		*/
	volatile uint32_t fifoctrl;	/* RW: FIFO control		*/
	volatile uint32_t fifodata;	/* RW: FIFO data		*/
	volatile uint32_t config;	/* RW: Configuration		*/
	volatile uint32_t data_xfer;	/* RW: Data transfer		*/
	volatile uint32_t autocmd;	/* RW: Auto command control	*/
	volatile uint32_t autostat;	/* RW: Auto status control	*/
	volatile uint32_t resel;	/* RW: Reselection		*/
	volatile uint32_t select;	/* RW: Selection		*/
	volatile uint32_t prcmd;	/* WO: Processor command	*/
	volatile uint32_t auxctrl;	/* RW: Aux control		*/
	volatile uint32_t autodata;	/* RW: Auto data control	*/
	volatile uint32_t loopctrl;	/* RW: Loopback control		*/
	volatile uint32_t loopdata;	/* RW: Loopback data		*/
	volatile uint32_t identify;	/* WO: Identify (?)		*/
	volatile uint32_t complete;	/* WO: Command complete (?)	*/
	volatile uint32_t scsi_status;	/* WO: SCSI status (?)		*/
	volatile uint32_t data;		/* RW: Data register (?)	*/
	volatile uint32_t icond;	/* RO: Interrupt condition	*/
	volatile uint32_t fastwide;	/* RW: Fast/wide enable		*/
	volatile uint32_t exctrl;	/* RW: Extended control		*/
	volatile uint32_t exstat;	/* RW: Extended status		*/
	volatile uint32_t test;		/* RW: SPIFI test register	*/
	volatile uint32_t quematch;	/* RW: Queue match		*/
	volatile uint32_t quecode;	/* RW: Queue code		*/
	volatile uint32_t quetag;	/* RW: Queue tag		*/
	volatile uint32_t quepage;	/* RW: Queue page		*/
	uint32_t image[88];		/* (image of the above)		*/
	struct {
		volatile uint32_t cdb[12]; /* RW: Command descriptor block */
		volatile uint32_t quecode; /* RW: Queue code		*/
		volatile uint32_t quetag;  /* RW: Queue tag		*/
		volatile uint32_t idmsg;   /* RW: Identify message     	*/
		volatile uint32_t status;  /* RW: SCSI status		*/
	} cmbuf[8];
};

/* spstat */
#define SPS_IDLE	0x00
#define SPS_SEL		0x01
#define SPS_ARB		0x02
#define SPS_RESEL	0x03
#define SPS_MSGOUT	0x04
#define SPS_COMMAND	0x05
#define SPS_DISCON	0x06
#define SPS_NXIN	0x07
#define SPS_INTR	0x08
#define SPS_NXOUT	0x09
#define SPS_CCOMP	0x0a
#define SPS_SVPTR	0x0b
#define SPS_STATUS	0x0c
#define SPS_MSGIN	0x0d
#define SPS_DATAOUT	0x0e
#define SPS_DATAIN	0x0f

/* cmlen */
#define CML_LENMASK	0x0f
#define CML_AMSG_EN	0x40
#define CML_ACOM_EN	0x80

/* intr and imask */
#define INTR_BSRQ	0x01
#define INTR_COMRECV	0x02
#define INTR_PERR	0x04
#define INTR_TIMEO	0x08
#define INTR_DERR	0x10
#define INTR_TGSEL	0x20
#define INTR_DISCON	0x40
#define INTR_FCOMP	0x80

#define INTR_BITMASK \
    "\20\10FCOMP\07DISCON\06TGSEL\05DERR\04TIMEO\03PERR\02COMRECV\01BSRQ"

/* prstat */
#define PRS_IO		0x08
#define PRS_CD		0x10
#define PRS_MSG		0x20
#define PRS_ATN		0x40
#define PRS_Z		0x80
#define PRS_PHASE	(PRS_MSG | PRS_CD | PRS_IO)

#define PRS_BITMASK "\20\10Z\07ATN\06MSG\05CD\04IO"

/* init_status */
#define IST_ACK		0x40

/* fifoctrl */
#define FIFOC_FSLOT	0x0f	/* Free slots in FIFO */
#define FIFOC_SSTKACT	0x10	/* Synchronous stack active (?) */
#define FIFOC_RQOVRN	0x20
#define FIFOC_CLREVEN	0x00
#define FIFOC_CLRODD	0x40
#define FIFOC_FLUSH	0x80
#define FIFOC_LOAD	0xc0

/* config */
#define CONFIG_PGENEN	0x08	/* Parity generation enable */
#define CONFIG_PCHKEN	0x10	/* Parity checking enable */
#define CONFIG_WORDEN	0x20
#define CONFIG_AUTOID	0x40
#define CONFIG_DMABURST	0x80

/* select */
#define SEL_SETATN	0x02
#define SEL_IRESELEN	0x04
#define SEL_ISTART	0x08
#define SEL_WATN	0x80

/* prcmd */
#define PRC_DATAOUT	0
#define PRC_DATAIN	1
#define PRC_COMMAND	2
#define PRC_STATUS	3
#define PRC_TRPAD	4
#define PRC_MSGOUT	6
#define PRC_MSGIN	7
#define PRC_KILLREQ	0x08
#define PRC_CLRACK	0x10
#define PRC_NJMP	0x80

/* auxctrl */
#define AUXCTRL_DMAEDGE	0x04
#define AUXCTRL_SETRST	0x20	/* Bus reset (?) */
#define AUXCTRL_CRST	0x40
#define AUXCTRL_SRST	0x80

/* autodata */
#define ADATA_IN	0x40
#define ADATA_EN	0x80

/* icond */
#define ICOND_ADATAOFF	0x02
#define ICOND_AMSGOFF	0x06
#define ICOND_ACMDOFF	0x0a
#define ICOND_ASTATOFF	0x0e
#define ICOND_SVPTEXP	0x10
#define ICOND_ADATAMIS	0x20
#define ICOND_CNTZERO	0x40
#define ICOND_UXPHASEZ	0x80
#define ICOND_UXPHASENZ	0x81
#define ICOND_NXTREQ	0xa0
#define ICOND_UKMSGZ	0xc0
#define ICOND_UKMSGNZ	0xc1
#define ICOND_UBF	0xe0	/* Unexpected bus free */

/* fastwide */
#define FAST_FASTEN	0x01

/* exctrl */
#define EXC_IPLOCK	0x04	/* Initiator page lock */

/* exstat */
#define EXS_UBF		0x08	/* Unexpected bus free */
