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
 *	@(#)siopreg.h	7.3 (Berkeley) 2/5/91
 *	$Id: siopreg.h,v 1.4 1994/05/12 05:57:24 chopps Exp $
 */

/*
 * NCR 53C710 SCSI interface hardware description.
 *
 * Using parts of the Mach scsi driver for the 53C700
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
typedef volatile siop_regmap_t *siop_regmap_p;

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
						3 --> 50.01..66.00 Mhz, div=3
					 */
#define	SIOP_DCNTL_S16		0x20	/* SCRIPTS fetches 16bits at a time */
#define	SIOP_DCNTL_SSM		0x10	/* Single step mode */
#define	SIOP_DCNTL_LLM		0x08	/* Enable Low-level mode */
#define	SIOP_DCNTL_STD		0x04	/* Start SCRIPTS operation */
#define	SIOP_DCNTL_RES		0x02
#define	SIOP_DCNTL_RST		0x01	/* Software reset */
