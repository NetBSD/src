/*	$NetBSD: ncr5380reg.h,v 1.1.1.1 1995/03/26 07:12:15 leo Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
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

#ifndef _NCR5380REG_H
#define _NCR5380REG_H
/*
 * Atari TT hardware:
 * SCSI interface + DMA.
 * The SCSI chip is an NCR5380
 */


#define	SCSI_DMA	((struct scsi_dma *)AD_SCSI_DMA)
#define	SCSI_5380	((struct scsi_5380 *)AD_NCR5380)

struct scsi_dma {
	volatile u_char		s_dma_ptr[8];	/* use only the odd bytes */
	volatile u_char		s_dma_cnt[8];	/* use only the odd bytes */
	volatile u_char		s_dma_res[4];	/* data residue register  */
	volatile u_char		s_dma_gap;	/* not used		  */
	volatile u_char		s_dma_ctrl;	/* control register	  */
};

#define	set_scsi_dma(addr, val)	(void)(					\
	{								\
	u_char	*address = (u_char*)addr+1;				\
	u_long	nval	 = (u_long)val;					\
	__asm("movepl	%0, %1@(0)": :"d" (nval), "a" (address));	\
	})

#define	get_scsi_dma(addr, res)	(					\
	{								\
	u_char	*address = (u_char*)addr+1;				\
	u_long	nval;							\
	__asm("movepl	%1@(0), %0": "=d" (nval) : "a" (address));	\
	res = (u_long)nval;						\
	})

/*
 * Defines for DMA control register
 */
#define	SD_BUSERR	0x80		/* 1 = transfer caused bus error*/
#define	SD_ZERO		0x40		/* 1 = byte counter is zero	*/
#define	SD_ENABLE	0x02		/* 1 = Enable DMA		*/
#define	SD_OUT		0x01		/* Direction: memory to SCSI	*/
#define	SD_IN		0x00		/* Direction: SCSI to memory	*/

struct scsi_5380 {
	volatile u_char	scsi_5380[16];	/* use only the odd bytes	*/
};

#define	scsi_data	scsi_5380[ 1]	/* Data register		*/
#define	scsi_icom	scsi_5380[ 3]	/* Initiator command register	*/
#define	scsi_mode	scsi_5380[ 5]	/* Mode register		*/
#define	scsi_tcom	scsi_5380[ 7]	/* Target command register	*/
#define	scsi_idstat	scsi_5380[ 9]	/* Bus status register		*/
#define	scsi_dmstat	scsi_5380[11]	/* DMA status register		*/
#define	scsi_trcv	scsi_5380[13]	/* Target receive register	*/
#define	scsi_ircv	scsi_5380[15]	/* Initiator receive register	*/

/*
 * Definitions for Initiator command register.
 */
#define	SC_A_RST	0x80	/* RW - Assert RST			*/
#define	SC_TEST		0x40	/* W  - Test mode			*/
#define	SC_AIP		0x40	/* R  - Arbitration in progress		*/
#define	SC_LA		0x20	/* R  - Lost arbitration		*/
#define	SC_A_ACK	0x10	/* RW - Assert ACK			*/
#define	SC_A_BSY	0x08	/* RW - Assert BSY			*/
#define	SC_A_SEL	0x04	/* RW - Assert SEL			*/
#define	SC_A_ATN	0x02	/* RW - Assert ATN			*/
#define	SC_ADTB		0x01	/* RW - Assert Data To Bus		*/

/*
 * Definitions for mode register
 */
#define	SC_B_DMA	0x80	/* RW - Block mode DMA (not on TT!)	*/
#define	SC_T_MODE	0x40	/* RW - Target mode			*/
#define	SC_E_PAR	0x20	/* RW - Enable parity check		*/
#define	SC_E_PARI	0x10	/* RW - Enable parity interrupt		*/
#define	SC_E_EOPI	0x08	/* RW - Enable End Of Process Interrupt	*/
#define	SC_MON_BSY	0x04	/* RW - Monitor BSY			*/
#define	SC_M_DMA	0x02	/* RW - Set DMA mode			*/
#define	SC_ARBIT	0x01	/* RW - Arbitrate			*/

/*
 * Definitions for tcom register
 */
#define	SC_LBS		0x80	/* RW - Last Byte Send (not on TT!)	*/
#define	SC_A_REQ	0x08	/* RW - Assert REQ			*/
#define	SC_A_MSG	0x04	/* RW - Assert MSG			*/
#define	SC_A_CD		0x02	/* RW - Assert C/D			*/
#define	SC_A_IO		0x01	/* RW - Assert I/O			*/

/*
 * Definitions for idstat register
 */
#define	SC_S_RST	0x80	/* R  - RST is set			*/
#define	SC_S_BSY	0x40	/* R  - BSY is set			*/
#define	SC_S_REQ	0x20	/* R  - REQ is set			*/
#define	SC_S_MSG	0x10	/* R  - MSG is set			*/
#define	SC_S_CD		0x08	/* R  - C/D is set			*/
#define	SC_S_IO		0x04	/* R  - I/O is set			*/
#define	SC_S_SEL	0x02	/* R  - SEL is set			*/
#define	SC_S_PAR	0x01	/* R  - Parity bit			*/

/*
 * Definitions for dmastat register
 */
#define	SC_END_DMA	0x80	/* R  - End of DMA			*/
#define	SC_DMA_REQ	0x40	/* R  - DMA request			*/
#define	SC_PAR_ERR	0x20	/* R  - Parity error			*/
#define	SC_IRQ_SET	0x10	/* R  - IRQ is active			*/
#define	SC_PHS_MTCH	0x08	/* R  - Phase Match			*/
#define	SC_BSY_ERR	0x04	/* R  - Busy error			*/
#define	SC_ATN_STAT	0x02	/* R  - State of ATN line		*/
#define	SC_ACK_STAT	0x01	/* R  - State of ACK line		*/
#define	SC_S_SEND	0x00	/* W  - Start DMA output		*/

#define	SC_CLINT	{ 		/* Clear interrupts	*/	\
			int i = SCSI_5380->scsi_ircv;			\
			}


/*
 * Definition of SCSI-bus phases. The values are determined by signals
 * on the SCSI-bus. DO NOT CHANGE!
 * The values must be used to index the pointers in SCSI-PARMS.
 */
#define	NR_PHASE	8
#define	PH_DATAOUT	0
#define	PH_DATAIN	1
#define	PH_CMD		2
#define	PH_STATUS	3
#define	PH_RES1		4
#define	PH_RES2		5
#define	PH_MSGOUT	6
#define	PH_MSGIN	7

#define	PH_OUT(phase)	(!(phase & 1))	/* TRUE if output phase		*/
#define	PH_IN(phase)	(phase & 1)	/* TRUE if input phase		*/

/*
 * Id of Host-adapter
 */
#define SC_HOST_ID	0x80

/*
 * Base setting for 5380 mode register
 */
#define	IMODE_BASE	SC_E_PAR

#endif /* _NCR5380REG_H */
