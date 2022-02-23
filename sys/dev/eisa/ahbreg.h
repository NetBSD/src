/*	$NetBSD: ahbreg.h,v 1.19 2022/02/23 21:54:40 andvar Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Originally written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 */

/*
 * Offset of AHA1740 registers, relative from slot base.
 */
#define	AHB_EISA_SLOT_OFFSET	0x0c80
#define	AHB_EISA_IOSIZE		0x0080

/*
 * AHA1740 EISA board mode registers (relative to port offset)
 */
#define PORTADDR	0x40
#define	 PORTADDR_ENHANCED	0x80
#define BIOSADDR	0x41
#define	INTDEF		0x42
#define	SCSIDEF		0x43
#define	BUSDEF		0x44
/**** bit definitions for INTDEF ****/
#define	INT9	0x00
#define	INT10	0x01
#define	INT11	0x02
#define	INT12	0x03
#define	INT14	0x05
#define	INT15	0x06
#define INTHIGH 0x08		/* interrupt signal is active-high */
#define	INTEN	0x10
/**** bit definitions for SCSIDEF ****/
#define	HSCSIID	0x0F		/* our SCSI ID */
#define	RSTPWR	0x10		/* reset scsi bus on power up or reset */
/**** bit definitions for BUSDEF ****/
#define	B0uS	0x00		/* give up bus immediately */
#define	B4uS	0x01		/* delay 4uSec. */
#define	B8uS	0x02

/*
 * AHA1740 ENHANCED mode mailbox control regs (relative to port offset)
 */
#define MBOXOUT0	0x50
#define MBOXOUT1	0x51
#define MBOXOUT2	0x52
#define MBOXOUT3	0x53

#define	ATTN		0x54
#define	G2CNTRL		0x55
#define	G2INTST		0x56
#define G2STAT		0x57

#define	MBOXIN0		0x58
#define	MBOXIN1		0x59
#define	MBOXIN2		0x5A
#define	MBOXIN3		0x5B

#define G2STAT2		0x5C

/*
 * Bit definitions for the 5 control/status registers
 */
#define	ATTN_TARGET		0x0F
#define	ATTN_OPCODE		0xF0
#define  OP_IMMED		0x10
#define	  AHB_TARG_RESET	0x80
#define  OP_START_ECB		0x40
#define  OP_ABORT_ECB		0x50

#define	G2CNTRL_SET_HOST_READY	0x20
#define	G2CNTRL_CLEAR_EISA_INT	0x40
#define	G2CNTRL_HARD_RESET	0x80

#define	G2INTST_TARGET		0x0F
#define	G2INTST_INT_STAT	0xF0
#define	 AHB_ECB_OK		0x10
#define	 AHB_ECB_RECOVERED	0x50
#define	 AHB_HW_ERR		0x70
#define	 AHB_IMMED_OK		0xA0
#define	 AHB_ECB_ERR		0xC0
#define	 AHB_ASN		0xD0	/* for target mode */
#define	 AHB_IMMED_ERR		0xE0

#define	G2STAT_BUSY		0x01
#define	G2STAT_INT_PEND		0x02
#define	G2STAT_MBOX_EMPTY	0x04

#define	G2STAT2_HOST_READY	0x01

#define	AHB_NSEG	33	/* number of DMA segments supported */

struct ahb_dma_seg {
	uint32_t seg_addr;
	uint32_t seg_len;
};

struct ahb_ecb_status {
	uint16_t status;
#define	ST_DON	0x0001
#define	ST_DU	0x0002
#define	ST_QF	0x0008
#define	ST_SC	0x0010
#define	ST_DO	0x0020
#define	ST_CH	0x0040
#define	ST_INT	0x0080
#define	ST_ASA	0x0100
#define	ST_SNS	0x0200
#define	ST_INI	0x0800
#define	ST_ME	0x1000
#define	ST_ECA	0x4000
	uint8_t  host_stat;
#define	HS_OK			0x00
#define	HS_CMD_ABORTED_HOST	0x04
#define	HS_CMD_ABORTED_ADAPTER	0x05
#define	HS_TIMED_OUT		0x11
#define	HS_HARDWARE_ERR		0x20
#define	HS_SCSI_RESET_ADAPTER	0x22
#define	HS_SCSI_RESET_INCOMING	0x23
	uint8_t  target_stat;
	uint32_t  resid_count;
	uint32_t  resid_addr;
	uint16_t addit_status;
	uint8_t  sense_len;
	uint8_t  unused[9];
	uint8_t  cdb[6];
};

struct ahb_ecb {
	uint8_t  opcode;
#define	ECB_SCSI_OP	0x01
	        uint8_t:4;
	uint8_t  options:3;
	        uint8_t:1;
	uint16_t opt1;
#define	ECB_CNE	0x0001
#define	ECB_DI	0x0080
#define	ECB_SES	0x0400
#define	ECB_S_G	0x1000
#define	ECB_DSB	0x4000
#define	ECB_ARS	0x8000
	uint16_t opt2;
#define	ECB_LUN	0x0007
#define	ECB_TAG	0x0008
#define	ECB_TT	0x0030
#define	ECB_ND	0x0040
#define	ECB_DAT	0x0100
#define	ECB_DIR	0x0200
#define	ECB_ST	0x0400
#define	ECB_CHK	0x0800
#define	ECB_REC	0x4000
#define	ECB_NRB	0x8000
	uint16_t unused1;
	uint32_t data_addr;
	uint32_t data_length;
	uint32_t status;
	uint32_t link_addr;
	uint16_t unused2;
	uint16_t unused3;
	uint32_t sense_ptr;
	uint8_t  req_sense_length;
	uint8_t  scsi_cmd_length;
	uint16_t cksum;
	uint8_t	scsi_cmd[12];

	/*-----------------end of hardware supported fields----------------*/

	struct ahb_dma_seg ahb_dma[AHB_NSEG];
	struct ahb_ecb_status ecb_status;
	struct scsi_sense_data ecb_sense;

	TAILQ_ENTRY(ahb_ecb) chain;
	struct ahb_ecb *nexthash;
	uint32_t ecb_dma_addr;
	struct scsipi_xfer *xs;	/* the scsipi_xfer for this cmd */
	int flags;
#define	ECB_ALLOC	0x01
#define	ECB_ABORT	0x02
#define	ECB_IMMED	0x04
#define	ECB_IMMED_FAIL	0x08
	int timeout;

	/*
	 * This DMA map maps the buffer involved in the transfer.
	 * Its contents are loaded into "ahb_dma" above.
	 */
	bus_dmamap_t	dmamap_xfer;
};
