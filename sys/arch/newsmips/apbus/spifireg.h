/*	$NetBSD: spifireg.h,v 1.1 2000/10/30 10:07:35 tsubai Exp $	*/

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
	volatile int spstat;		/* RO: SPIFI state		*/
	volatile int cmlen;		/* RW: Command/message length	*/
	volatile int cmdpage;		/* RW: Command page		*/
	volatile int count_hi;		/* RW: Data count (high)	*/
	volatile int count_mid;		/* RW:            (mid)		*/
	volatile int count_low;		/* RW:            (low)		*/
	volatile int svptr_hi;		/* RO: Saved data pointer (high)*/
	volatile int svptr_mid;		/* RO:                    (mid)	*/
	volatile int svptr_low;		/* RO:                    (low) */
	volatile int intr;		/* RW: Processor interrupt	*/
	volatile int imask;		/* RW: Processor interrupt mask	*/
	volatile int prctrl;		/* RW: Processor control	*/
	volatile int prstat;		/* RO: Processor status		*/
	volatile int init_status;	/* RO: Initiator status		*/
	volatile int fifoctrl;		/* RW: FIFO control		*/
	volatile int fifodata;		/* RW: FIFO data		*/
	volatile int config;		/* RW: Configuration		*/
	volatile int data_xfer;		/* RW: Data transfer		*/
	volatile int autocmd;		/* RW: Auto command control	*/
	volatile int autostat;		/* RW: Auto status control	*/
	volatile int resel;		/* RW: Reselection		*/
	volatile int select;		/* RW: Selection		*/
	volatile int prcmd;		/* WO: Processor command	*/
	volatile int auxctrl;		/* RW: Aux control		*/
	volatile int autodata;		/* RW: Auto data control	*/
	volatile int loopctrl;		/* RW: Loopback control		*/
	volatile int loopdata;		/* RW: Loopback data		*/
	volatile int identify;		/* WO: Identify (?)		*/
	volatile int complete;		/* WO: Command complete (?)	*/
	volatile int scsi_status;	/* WO: SCSI status (?)		*/
	volatile int data;		/* RW: Data register (?)	*/
	volatile int icond;		/* RO: Interrupt condition	*/
	volatile int fastwide;		/* RW: Fast/wide enable		*/
	volatile int exctrl;		/* RW: Extended control		*/
	volatile int exstat;		/* RW: Extended status		*/
	volatile int test;		/* RW: SPIFI test register	*/
	volatile int quematch;		/* RW: Queue match		*/
	volatile int quecode;		/* RW: Queue code		*/
	volatile int quetag;		/* RW: Queue tag		*/
	volatile int quepage;		/* RW: Queue page		*/
	int image[88];			/* (image of the above)		*/
	struct {
		volatile int cdb[12];	/* RW: Command descriptor block */
		volatile int quecode;	/* RW: Queue code		*/
		volatile int quetag;	/* RW: Queue tag		*/
		volatile int idmsg;	/* RW: Identify message		*/
		volatile int status;	/* RW: SCSI status		*/
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
