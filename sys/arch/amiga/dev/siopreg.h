/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson of Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)scsireg.h	7.3 (Berkeley) 2/5/91
 */

/*
 * NCR 53C710 SCSI interface hardware description.
 *
 */

typedef struct {
	volatile unsigned char	siop_sien;	/* rw: SCSI Interrupt Enable */
	volatile unsigned char	siop_sdid;	/* rw: SCSI Destination ID */
	volatile unsigned char	siop_scntl1;	/* rw: SCSI control reg 1 */
	volatile unsigned char	siop_scntl0;	/* rw: SCSI control reg 0 */
	volatile unsigned char	siop_socl;	/* rw: SCSI Output Control Latch */
	volatile unsigned char	siop_sodl;	/* rw: SCSI Output Data Latch */
	volatile unsigned char	siop_sxfer;	/* rw: SCSI Transfer reg */
	volatile unsigned char	siop_scid;	/* rw: SCSI Chip ID reg */
	volatile unsigned char	siop_sbcl;	/* ro: SCSI Bus Control Lines */
	volatile unsigned char	siop_sbdl;	/* ro: SCSI Bus Data Lines */
	volatile unsigned char	siop_sidl;	/* ro: SCSI Input Data Latch */
	volatile unsigned char	siop_sfbr;	/* ro: SCSI First Byte Received */
	volatile unsigned char	siop_sstat2;	/* ro: SCSI status reg 2 */
	volatile unsigned char	siop_sstat1;	/* ro: SCSI status reg 1 */
	volatile unsigned char	siop_sstat0;	/* ro: SCSI status reg 0 */
	volatile unsigned char	siop_dstat;	/* ro: DMA status */
	volatile unsigned long	siop_dsa;	/* rw: Data Structure Address */
	volatile unsigned char	siop_ctest3;	/* ro: Chip test register 3 */
	volatile unsigned char	siop_ctest2;	/* ro: Chip test register 2 */
	volatile unsigned char	siop_ctest1;	/* ro: Chip test register 1 */
	volatile unsigned char	siop_ctest0;	/* ro: Chip test register 0 */
	volatile unsigned char	siop_ctest7;	/* rw: Chip test register 7 */
	volatile unsigned char	siop_ctest6;	/* rw: Chip test register 6 */
	volatile unsigned char	siop_ctest5;	/* rw: Chip test register 5 */
	volatile unsigned char	siop_ctest4;	/* rw: Chip test register 4 */
	volatile unsigned long	siop_temp;	/* rw: Temporary Stack reg */
	volatile unsigned char	siop_lcrc;	/* rw: LCRC value */
	volatile unsigned char	siop_ctest8;	/* rw: Chip test register 8 */
	volatile unsigned char	siop_istat;	/* rw: Interrupt Status reg */
	volatile unsigned char	siop_dfifo;	/* rw: DMA FIFO */
	volatile unsigned char	siop_dcmd;	/* rw: DMA Command Register */
	volatile unsigned char	siop_dbc2;	/* rw: DMA Byte Counter reg */
	volatile unsigned char	siop_dbc1;
	volatile unsigned char	siop_dbc0;
	volatile unsigned long	siop_dnad;	/* rw: DMA Next Address */
	volatile unsigned long	siop_dsp;	/* rw: DMA SCRIPTS Pointer reg */
	volatile unsigned long	siop_dsps;	/* rw: DMA SCRIPTS Pointer Save reg */
	volatile unsigned long	siop_scratch;	/* rw: Scratch Register */
	volatile unsigned char	siop_dcntl;	/* rw: DMA Control reg */
	volatile unsigned char	siop_dwt;	/* rw: DMA Watchdog Timer */
	volatile unsigned char	siop_dien;	/* rw: DMA Interrupt Enable */
	volatile unsigned char	siop_dmode;	/* rw: DMA Mode reg */
	volatile unsigned long	siop_addr;

} siop_regmap_t;

/*
 * Register defines
 */

/* Scsi control register 0 (scntl0) */

#define	SIOP_SCNTL0_ARB		0xc0	/* Arbitration mode */
#	define	SIOP_ARB_SIMPLE	0x00
#	define	SIOP_ARB_FULL	0xc0
#define	SIOP_SCNTL0_START	0x20	/* Start Sequence */
#define	SIOP_SCNTL0_WATN	0x10	/* (Select) With ATN */
#define	SIOP_SCNTL0_EPC		0x08	/* Enable Parity Checking */
#define	SIOP_SCNTL0_EPG		0x04	/* Enable Parity Generation */
#define	SIOP_SCNTL0_AAP		0x02	/* Assert ATN on Parity Error */
#define	SIOP_SCNTL0_TRG		0x01	/* Target Mode */

/* Scsi control register 1 (scntl1) */

#define	SIOP_SCNTL1_EXC		0x80	/* Extra Clock Cycle of data setup */
#define	SIOP_SCNTL1_ADB		0x40	/* Assert Data Bus */
#define	SIOP_SCNTL1_ESR		0x20	/* Enable Selection/Reselection */
#define	SIOP_SCNTL1_CON		0x10	/* Connected */
#define	SIOP_SCNTL1_RST		0x08	/* Assert RST */
#define	SIOP_SCNTL1_PAR		0x04	/* Force bad Parity */
#define	SIOP_SCNTL1_SND		0x02	/* Start Send operation */
#define	SIOP_SCNTL1_RCV		0x01	/* Start Receive operation */

/* Scsi interrupt enable register (sien) */

#define	SIOP_SIEN_M_A		0x80	/* Phase Mismatch or ATN active */
#define	SIOP_SIEN_FC		0x40	/* Function Complete */
#define	SIOP_SIEN_STO		0x20	/* (Re)Selection timeout */
#define	SIOP_SIEN_SEL		0x10	/* (Re)Selected */
#define	SIOP_SIEN_SGE		0x08	/* SCSI Gross Error */
#define	SIOP_SIEN_UDC		0x04	/* Unexpected Disconnect */
#define	SIOP_SIEN_RST		0x02	/* RST asserted */
#define	SIOP_SIEN_PAR		0x01	/* Parity Error */

/* Scsi chip ID (scid) */

#define	SIOP_SCID_VALUE(i)	(1<<i)

/* Scsi transfer register (sxfer) */

#define	SIOP_SXFER_DHP		0x80	/* Disable Halt on Parity error/ ATN asserted */
#define	SIOP_SXFER_TP		0x70	/* Synch Transfer Period */
					/* see specs for formulas:
						Period = TCP * (4 + XFERP )
						TCP = 1 + CLK + 1..2;
					 */
#define	SIOP_SXFER_MO		0x0f	/* Synch Max Offset */
#	define	SIOP_MAX_OFFSET	8

/* Scsi output data latch register (sodl) */

/* Scsi output control latch register (socl) */

#define	SIOP_REQ		0x80	/* SCSI signal <x> asserted */
#define	SIOP_ACK		0x40
#define	SIOP_BSY		0x20
#define	SIOP_SEL		0x10
#define	SIOP_ATN		0x08
#define	SIOP_MSG		0x04
#define	SIOP_CD			0x02
#define	SIOP_IO			0x01

#define	SIOP_PHASE(socl)	SCSI_PHASE(socl)

/* Scsi first byte received register (sfbr) */

/* Scsi input data latch register (sidl) */

/* Scsi bus data lines register (sbdl) */

/* Scsi bus control lines register (sbcl).  Same as socl */

/* DMA status register (dstat) */

#define	SIOP_DSTAT_DFE		0x80	/* DMA FIFO empty */
#define	SIOP_DSTAT_RES		0x60
#define	SIOP_DSTAT_ABRT		0x10	/* Aborted */
#define	SIOP_DSTAT_SSI		0x08	/* SCRIPT Single Step */
#define	SIOP_DSTAT_SIR		0x04	/* SCRIPT Interrupt Instruction */
#define	SIOP_DSTAT_WTD		0x02	/* Watchdog Timeout Detected */
#define	SIOP_DSTAT_OPC		0x01	/* Invalid SCRIPTS Opcode */

/* Scsi status register 0 (sstat0) */

#define	SIOP_SSTAT0_M_A		0x80	/* Phase Mismatch or ATN active */
#define	SIOP_SSTAT0_FC		0x40	/* Function Complete */
#define	SIOP_SSTAT0_STO		0x20	/* (Re)Selection timeout */
#define	SIOP_SSTAT0_SEL		0x10	/* (Re)Selected */
#define	SIOP_SSTAT0_SGE		0x08	/* SCSI Gross Error */
#define	SIOP_SSTAT0_UDC		0x04	/* Unexpected Disconnect */
#define	SIOP_SSTAT0_RST		0x02	/* RST asserted */
#define	SIOP_SSTAT0_PAR		0x01	/* Parity Error */

/* Scsi status register 1 (sstat1) */

#define	SIOP_SSTAT1_ILF		0x80	/* Input latch (sidl) full */
#define	SIOP_SSTAT1_ORF		0x40	/* output reg (sodr) full */
#define	SIOP_SSTAT1_OLF		0x20	/* output latch (sodl) full */
#define	SIOP_SSTAT1_AIP		0x10	/* Arbitration in progress */
#define	SIOP_SSTAT1_LOA		0x08	/* Lost arbitration */
#define	SIOP_SSTAT1_WOA		0x04	/* Won arbitration */
#define	SIOP_SSTAT1_RST		0x02	/* SCSI RST current value */
#define	SIOP_SSTAT1_SDP		0x01	/* SCSI SDP current value */

/* Scsi status register 2 (sstat2) */

#define	SIOP_SSTAT2_FF		0xf0	/* SCSI FIFO flags (bytecount) */
#	define SIOP_SCSI_FIFO_DEEP	8
#define	SIOP_SSTAT2_SDP		0x08	/* Latched (on REQ) SCSI SDP */
#define	SIOP_SSTAT2_MSG		0x04	/* Latched SCSI phase */
#define	SIOP_SSTAT2_CD		0x02
#define	SIOP_SSTAT2_IO		0x01

/* Chip test register 0 (ctest0) */

#define	SIOP_CTEST0_RES		0xfc
#define	SIOP_CTEST0_RTRG	0x02	/* Real Target mode */
#define	SIOP_CTEST0_DDIR	0x01	/* Xfer direction (1-> from SCSI bus) */

/* Chip test register 1 (ctest1) */

#define	SIOP_CTEST1_FMT		0xf0	/* Byte empty in DMA FIFO bottom (high->byte3) */
#define	SIOP_CTEST1_FFL		0x0f	/* Byte full in DMA FIFO top, same */

/* Chip test register 2 (ctest2) */

#define	SIOP_CTEST2_RES		0xc0
#define	SIOP_CTEST2_SOFF	0x20	/* Synch Offset compare (1-> zero Init, max Tgt */
#define	SIOP_CTEST2_SFP		0x10	/* SCSI FIFO Parity */
#define	SIOP_CTEST2_DFP		0x08	/* DMA FIFO Parity */
#define	SIOP_CTEST2_TEOP	0x04	/* True EOP (a-la 5380) */
#define	SIOP_CTEST2_DREQ	0x02	/* DREQ status */
#define	SIOP_CTEST2_DACK	0x01	/* DACK status */

/* Chip test register 3 (ctest3) read-only, top of SCSI FIFO */

/* Chip test register 4 (ctest4) */

#define	SIOP_CTEST4_RES		0x80
#define	SIOP_CTEST4_ZMOD	0x40	/* High-impedance outputs */
#define	SIOP_CTEST4_SZM		0x20	/* ditto, SCSI "outputs" */
#define	SIOP_CTEST4_SLBE	0x10	/* SCSI loobpack enable */
#define	SIOP_CTEST4_SFWR	0x08	/* SCSI FIFO write enable (from sodl) */
#define	SIOP_CTEST4_FBL		0x07	/* DMA FIFO Byte Lane select (from ctest6)
					   4->0, .. 7->3 */

/* Chip test register 5 (ctest5) */

#define	SIOP_CTEST5_ADCK	0x80	/* Clock Address Incrementor */
#define	SIOP_CTEST5_BBCK	0x40	/* Clock Byte counter */
#define	SIOP_CTEST5_ROFF	0x20	/* Reset SCSI offset */
#define	SIOP_CTEST5_MASR	0x10	/* Master set/reset pulses (of bits 3-0) */
#define	SIOP_CTEST5_DDIR	0x08	/* (re)set internal DMA direction */
#define	SIOP_CTEST5_EOP		0x04	/* (re)set internal EOP */
#define	SIOP_CTEST5_DREQ	0x02	/* (re)set internal REQ */
#define	SIOP_CTEST5_DACK	0x01	/* (re)set internal ACK */

/* Chip test register 6 (ctest6)  DMA FIFO access */

/* Chip test register 7 (ctest7) */

#define	SIOP_CTEST7_RES		0xe0
#define	SIOP_CTEST7_STD		0x10	/* Disable selection timeout */
#define	SIOP_CTEST7_DFP		0x08	/* DMA FIFO parity bit */
#define	SIOP_CTEST7_EVP		0x04	/* Even parity (to host bus) */
#define	SIOP_CTEST7_DC		0x02	/* Drive DC pin low on SCRIPT fetches */
#define	SIOP_CTEST7_DIFF	0x01	/* Differential mode */

/* DMA FIFO register (dfifo) */

#define	SIOP_DFIFO_FLF		0x80	/* Flush (spill) DMA FIFO */
#define	SIOP_DFIFO_CLF		0x40	/* Clear DMA and SCSI FIFOs */
#define	SIOP_DFIFO_BO		0x3f	/* FIFO byte offset counter */

/* Interrupt status register (istat) */

#define	SIOP_ISTAT_ABRT		0x80	/* Abort operation */
#define	SIOP_ISTAT_RES		0x70
#define	SIOP_ISTAT_CON		0x08	/* Connected */
#define	SIOP_ISTAT_PRE		0x04	/* Pointer register empty */
#define	SIOP_ISTAT_SIP		0x02	/* SCSI Interrupt pending */
#define	SIOP_ISTAT_DIP		0x01	/* DMA Interrupt pending */


/* DMA Mode register (dmode) */

#define	SIOP_DMODE_BL_MASK	0xc0	/* 0->1 1->2 2->4 3->8 */
#define	SIOP_DMODE_BW16		0x20	/* Bus Width is 16 bits */
#define	SIOP_DMODE_286		0x10	/* 286 mode */
#define	SIOP_DMODE_IO_M		0x08	/* xfer data to memory or I/O space */
#define	SIOP_DMODE_FAM		0x04	/* fixed address mode */
#define	SIOP_DMODE_PIPE		0x02	/* SCRIPTS in Pipeline mode */
#define	SIOP_DMODE_MAN		0x01	/* SCRIPTS in Manual start mode */

/* DMA interrupt enable register (dien) */

#define	SIOP_DIEN_RES		0xe0
#define	SIOP_DIEN_ABRT		0x10	/* On Abort */
#define	SIOP_DIEN_SSI		0x08	/* On SCRIPTS sstep */
#define	SIOP_DIEN_SIR		0x04	/* On SCRIPTS intr instruction */
#define	SIOP_DIEN_WTD		0x02	/* On watchdog timeout */
#define	SIOP_DIEN_OPC		0x01	/* On SCRIPTS illegal opcode */

/* DMA control register (dcntl) */

#define	SIOP_DCNTL_CF_MASK	0xc0	/* Clock frequency dividers:
						0 --> 37.51..50.00 Mhz, div=2
						1 --> 25.01..37.50 Mhz, div=1.5
						2 --> 16.67..25.00 Mhz, div=1
						3 --> reserved
					 */
#define	SIOP_DCNTL_S16		0x20	/* SCRIPTS fetches 16bits at a time */
#define	SIOP_DCNTL_SSM		0x10	/* Single step mode */
#define	SIOP_DCNTL_LLM		0x08	/* Enable Low-level mode */
#define	SIOP_DCNTL_STD		0x04	/* Start SCRIPTS operation */
#define	SIOP_DCNTL_RES		0x02
#define	SIOP_DCNTL_RST		0x01	/* Software reset */

/* psns/pctl phase lines as bits */
#define	PHASE_MSG	0x04
#define	PHASE_CD	0x02		/* =1 if 'command' */
#define	PHASE_IO	0x01		/* =1 if data inbound */
/* Phase lines as values */
#define	PHASE		0x07		/* mask for psns/pctl phase */
#define	DATA_OUT_PHASE	0x00
#define	DATA_IN_PHASE	0x01
#define	CMD_PHASE	0x02
#define	STATUS_PHASE	0x03
#define	BUS_FREE_PHASE	0x04
#define	ARB_SEL_PHASE	0x05	/* Fuji chip combines arbitration with sel. */
#define	MESG_OUT_PHASE	0x06
#define	MESG_IN_PHASE	0x07

/* SCSI Messages */

#define	MSG_CMD_COMPLETE	0x00
#define MSG_EXT_MESSAGE		0x01
#define	MSG_SAVE_DATA_PTR	0x02
#define	MSG_RESTORE_PTR		0x03
#define	MSG_DISCONNECT		0x04
#define	MSG_INIT_DETECT_ERROR	0x05
#define	MSG_ABORT		0x06
#define	MSG_REJECT		0x07
#define	MSG_NOOP		0x08
#define	MSG_PARITY_ERROR	0x09
#define	MSG_BUS_DEVICE_RESET	0x0C
#define	MSG_IDENTIFY		0x80
#define	MSG_IDENTIFY_DR		0xc0	/* (disconnect/reconnect allowed) */
#define	MSG_SYNC_REQ 		0x01

/* SCSI Commands */

#define CMD_TEST_UNIT_READY	0x00
#define CMD_REQUEST_SENSE	0x03
#define	CMD_INQUIRY		0x12
#define CMD_SEND_DIAGNOSTIC	0x1D

#define CMD_REWIND		0x01
#define CMD_FORMAT_UNIT		0x04
#define CMD_READ_BLOCK_LIMITS	0x05
#define CMD_REASSIGN_BLOCKS	0x07
#define CMD_READ		0x08
#define CMD_WRITE		0x0A
#define CMD_WRITE_FILEMARK	0x10
#define CMD_SPACE		0x11
#define CMD_MODE_SELECT		0x15
#define CMD_RELEASE_UNIT	0x17
#define CMD_ERASE		0x19
#define CMD_MODE_SENSE		0x1A
#define CMD_LOADUNLOAD		0x1B
#define CMD_RECEIVE_DIAG	0x1C
#define CMD_SEND_DIAG		0x1D
#define CMD_P_A_MEDIA_REMOVAL	0x1E
#define CMD_READ_CAPACITY	0x25
#define CMD_READ_EXT		0x28
#define CMD_WRITE_EXT		0x2A
#define CMD_READ_DEFECT_DATA	0x37
#define		SD_MANUFAC_DEFECTS	0x14000000
#define		SD_GROWN_DEFECTS	0x0c000000
#define CMD_READ_BUFFER		0x3B
#define CMD_WRITE_BUFFER	0x3C
#define CMD_READ_FULL		0xF0
#define CMD_MEDIA_TEST		0xF1
#define CMD_ACCESS_LOG		0xF2
#define CMD_WRITE_FULL		0xFC
#define CMD_MANAGE_PRIMARY	0xFD
#define CMD_EXECUTE_DATA	0xFE

/* SCSI status bits */

#define	STS_CHECKCOND	0x02	/* Check Condition (ie., read sense) */
#define	STS_CONDMET	0x04	/* Condition Met (ie., search worked) */
#define	STS_BUSY	0x08
#define	STS_INTERMED	0x10	/* Intermediate status sent */
#define	STS_EXT		0x80	/* Extended status valid */

/* command descriptor blocks */

struct scsi_cdb6 {
	u_char	cmd;		/* command code */
	u_char	lun:  3,	/* logical unit on ctlr */
		lbah: 5;	/* msb of read/write logical block addr */
	u_char	lbam;		/* middle byte of l.b.a. */
	u_char	lbal;		/* lsb of l.b.a. */
	u_char	len;		/* transfer length */
	u_char	xtra;
};

struct scsi_cdb10 {
	u_char	cmd;		/* command code */
	u_char	lun: 3,		/* logical unit on ctlr */
		   : 4,
		rel: 1;		/* l.b.a. is relative addr if =1 */
	u_char	lbah;		/* msb of read/write logical block addr */
	u_char	lbahm;		/* high middle byte of l.b.a. */
	u_char	lbalm;		/* low middle byte of l.b.a. */
	u_char	lbal;		/* lsb of l.b.a. */
	u_char	reserved;
	u_char	lenh;		/* msb transfer length */
	u_char	lenl;		/* lsb transfer length */
	u_char	xtra;
};

/* basic sense data */

struct scsi_sense {
	u_char	valid: 1,	/* l.b.a. is valid */
		class: 3,
		code:  4;
	u_char	vu:    4,	/* vendor unique */
		lbah:  4;
	u_char	lbam;
	u_char	lbal;
};

struct scsi_xsense {
	u_char	valid: 1,	/* l.b.a. is valid */
		class: 3,
		code:  4;
	u_char	segment;
	u_char	filemark: 1,
		eom:      1,
		ili:      1,	/* illegal length indicator */
		rsvd:	  1,
		key:	  4;
	u_char	info1;
	u_char	info2;
	u_char	info3;
	u_char	info4;
	u_char	len;		/* additional sense length */
};

/* inquiry data */
struct scsi_inquiry {
	u_char	type;
	u_char	qual;
	u_char	version;
	u_char	rsvd;
	u_char	len;
	char	class[3];
	char	vendor_id[8];
	char	product_id[16];
	char	rev[4];
};

struct scsi_format_parms {		/* physical BFI format */
	u_short	reserved;
	u_short	list_len;
	struct defect {
		unsigned cyl  : 24;
		unsigned head : 8;
		long	bytes_from_index;
	} defect[127];
} format_parms;

struct scsi_reassign_parms {
	u_short	reserved;
	u_short	list_len;	/* length in bytes of defects only */
	struct new_defect {
		unsigned lba;	/* logical block address */
	} new_defect[2];
} reassign_parms;

struct scsi_modesel_hdr {
	u_char	rsvd1;
	u_char	media_type;
	u_char 	rsvd2;
	u_char	block_desc_len;
	u_int	density		: 8;
	u_int	number_blocks	:24;
	u_int	rsvd3		: 8;
	u_int	block_length	:24;
}; 

struct scsi_modesense_hdr {
	u_char	len;
	u_char	media_type;
	u_char 	wp    : 1;
	u_char 	rsvd1 : 7;
	u_char	block_desc_len;
	u_int	density		: 8;
	u_int	number_blocks	:24;
	u_int	rsvd2		: 8;
	u_int	block_length	:24;
}; 

/*
 * Mode Select / Mode sense "pages"
 */

/*
 * Page One - Error Recovery Parameters 
 */
struct scsi_err_recovery {
	u_char	page_savable	: 1;	/* save parameters */
	u_char	reserved	: 1;
	u_char	page_code	: 6;	/* = 0x01 */
	u_char	page_length;		/* = 6 */
	u_char	awre		: 1;	/* auto write realloc enabled */
	u_char	arre		: 1;	/* auto read realloc enabled */
	u_char	tb		: 1;	/* transfer block */
	u_char 	rc		: 1;	/* read continuous */
	u_char	eec		: 1;	/* enable early correction */
	u_char	per		: 1;	/* post error */
	u_char	dte		: 1;	/* disable transfer on error */
	u_char	dcr		: 1;	/* disable correction */
	u_char	retry_count;
	u_char	correction_span;
	u_char	head_offset_count;
	u_char	strobe_offset_count;
	u_char	recovery_time_limit;
};

/*
 * Page Two - Disconnect / Reconnect Control Parameters
 */
struct scsi_disco_reco {
	u_char	page_savable	: 1;	/* save parameters */
	u_char	rsvd		: 1;
	u_char	page_code	: 6;	/* = 0x02 */
	u_char	page_length;		/* = 10 */
	u_char	buffer_full_ratio;	/* write, how full before reconnect? */
	u_char	buffer_empty_ratio;	/* read, how full before reconnect? */

	u_short	bus_inactivity_limit;	/* how much bus time for busy */
	u_short	disconnect_time_limit;	/* min to remain disconnected */
	u_short	connect_time_limit;	/* min to remain connected */
	u_short	reserved_1;
};

/*
 * Page Three - Direct Access Device Format Parameters
 */
struct scsi_format {
	u_char	page_savable	: 1;	/* save parameters */
	u_char	rsvd		: 1;
	u_char	page_code	: 6;	/* = 0x03 */
	u_char	page_length;		/* = 22 */
	u_short	tracks_per_zone;	/*  Handling of Defects Fields */
	u_short	alt_sect_zone;
	u_short alt_tracks_zone;
	u_short	alt_tracks_vol;
	u_short	sect_track;		/* Track Format Field */
	u_short data_sect;		/* Sector Format Fields */
	u_short	interleave;
	u_short	track_skew_factor;
	u_short	cyl_skew_factor;
	u_char	ssec		: 1;	/* Drive Type Field */
	u_char	hsec		: 1;
	u_char	rmb		: 1;
	u_char	surf		: 1;
	u_char	ins		: 1;
	u_char	reserved_1	: 3;
	u_char	reserved_2;
	u_char	reserved_3;
	u_char	reserved_4;
};

/*
 * Page Four - Rigid Disk Drive Geometry Parameters 
 */
struct scsi_geometry {
	u_char	page_savable	: 1;	/* save parameters */
	u_char	rsvd		: 1;
	u_char	page_code	: 6;	/* = 0x04 */
	u_char	page_length;		/* = 18 */
	u_char	cyl_ub;			/* number of cylinders */
	u_char	cyl_mb;
	u_char	cyl_lb;
	u_char	heads;			/* number of heads */
	u_char	precomp_cyl_ub;		/* cylinder to start precomp */
	u_char	precomp_cyl_mb;
	u_char	precomp_cyl_lb;
	u_char	current_cyl_ub;		/* cyl to start reduced current */
	u_char	current_cyl_mb;
	u_char	current_cyl_lb;
	u_short	step_rate;		/* drive step rate */
	u_char	landing_cyl_ub;		/* landing zone cylinder */
	u_char	landing_cyl_mb;
	u_char	landing_cyl_lb;
	u_char	reserved_1;
	u_char	reserved_2;
	u_char	reserved_3;
};

/*
 * Page 0x38 - Cache Control Parameters
 */
struct scsi_cache {
	u_char	page_savable	: 1;	/* save parameters */
	u_char	rsvd		: 1;
	u_char	page_code	: 6;	/* = 0x38 */
	u_char	page_length;		/* = 14 */
	u_char rsvd_1	: 1;
	u_char wie	: 1; 		/* write index enable */
	u_char rsvd_2	: 1;
	u_char ce	: 1; 		/* cache enable */
	u_char table_size : 4;
	u_char	prefetch_threshold;
	u_char	maximum_threshold;
	u_char	maximumprefetch_multiplier;
	u_char	minimum_threshold;
	u_char	minimum_prefetch_multiplier;
	u_char	reserved[8];
};

/*
 * Driver ioctl's for various scsi operations.
 */
#ifndef _IOCTL_
#include "ioctl.h"
#endif

/*
 * Control for SCSI "format" mode.
 *
 * "Format" mode allows a privileged process to issue direct SCSI
 * commands to a drive (it is intended primarily to allow on-line
 * formatting).  SDIOCSFORMAT with a non-zero arg will put the drive
 * into format mode; a zero arg will take it out.  When in format
 * mode, only the process that issued the SDIOCFORMAT can read or
 * write the drive.
 *
 * In format mode, process is expected to
 *	- do SDIOCSCSICOMMAND to supply cdb for next SCSI op
 *	- do read or write as appropriate for cdb
 *	- if i/o error, optionally do SDIOCSENSE to get completion
 *	  status and sense data from last scsi operation.
 */

struct scsi_fmt_cdb {
	int len;		/* cdb length (in bytes) */
	u_char cdb[28];		/* cdb to use on next read/write */
};

struct scsi_fmt_sense {
	u_int status;		/* completion status of last op */
	u_char sense[28];	/* sense data (if any) from last op */
};

#define	SDIOCSFORMAT		_IOW('S', 0x1, int)
#define	SDIOCGFORMAT		_IOR('S', 0x2, int)
#define	SDIOCSCSICOMMAND	_IOW('S', 0x3, struct scsi_fmt_cdb)
#define	SDIOCSENSE		_IOR('S', 0x4, struct scsi_fmt_sense)

extern void siopreset (int unit);
extern void siopstart (int unit);
extern int siopgo (int ctlr, int slave, int unit, struct buf *bp, struct scsi_fmt_cdb *cdb, int pad);
extern void siopdone (int unit);
#if 0
extern int siopintr2 (int unit);
#else
extern int siopintr2 (void);
#endif
