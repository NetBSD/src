/*	$NetBSD: ncr5380reg.h,v 1.9 1996/03/08 21:55:00 leo Exp $	*/

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
 * NCR5380 common interface definitions.
 */

/*
 * Register numbers: (first argument to GET/SET_5380_REG )
 */
#define	NCR5380_DATA	0		/* Data register		*/
#define	NCR5380_ICOM	1		/* Initiator command register	*/
#define	NCR5380_MODE	2		/* Mode register		*/
#define	NCR5380_TCOM	3		/* Target command register	*/
#define	NCR5380_IDSTAT	4		/* Bus status register		*/
#define	NCR5380_DMSTAT	5		/* DMA status register		*/
#define	NCR5380_TRCV	6		/* Target receive register	*/
#define	NCR5380_IRCV	7		/* Initiator receive register	*/

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
			int i = GET_5380_REG(NCR5380_IRCV);		\
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

/*
 * SCSI completion status codes, should move to sys/scsi/????
 */
#define SCSMASK		0x1e	/* status code mask			*/
#define SCSGOOD		0x00	/* good status				*/
#define SCSCHKC		0x02	/* check condition			*/
#define SCSBUSY		0x08	/* busy status				*/
#define SCSCMET		0x04	/* condition met / good			*/

/*
 * Return values of check_intr()
 */
#define	INTR_SPURIOUS	0
#define	INTR_RESEL	2
#define	INTR_DMA	3

struct ncr_softc;
struct req_q;
/*
 * Function decls:
 */
static int  transfer_pio __P((u_char *, u_char *, u_long *, int));
static int  wait_req_true __P((void));
static int  wait_req_false __P((void));
static int  scsi_select __P((struct req_q *, int));
static int  handle_message __P((struct req_q *, u_int));
static void ack_message __P((void));
static void nack_message __P((struct req_q *, u_char));
static int  information_transfer __P((struct ncr_softc *));
static void reselect __P((struct ncr_softc *));
static int  dma_ready __P((void));
static void transfer_dma __P((struct req_q *, u_int, int));
static int  check_autosense __P((struct req_q *, int));
static int  reach_msg_out __P((struct ncr_softc *, u_long));
static int  check_intr __P((struct ncr_softc *));
       void finish_req __P((struct req_q *));
       int  command_size __P((u_char));
       void scsi_reset __P((void));
static void scsi_reset_verbose __P((struct ncr_softc *, const char *));
static int  scsi_dmaok __P((struct req_q *));
static void run_main __P((struct ncr_softc *));
static void scsi_main __P((struct ncr_softc *));
static void ncr_ctrl_intr __P((struct ncr_softc *));
static void ncr_dma_intr __P((struct ncr_softc *));
static void ncr_tprint __P((struct req_q *, char *, ...));
static void ncr_aprint __P((struct ncr_softc *, char *, ...));

       void scsi_show __P((void));
static void show_request __P((struct req_q *, char *));
static void show_data_sense __P((struct scsi_xfer *));
/* static void show_phase __P((struct req_q *, int)); */
static void show_signals __P((u_char, u_char));

#endif /* _NCR5380REG_H */
