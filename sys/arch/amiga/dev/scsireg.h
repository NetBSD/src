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
 * AMD AM33C93A SCSI interface hardware description.
 *
 * Using parts of the Mach scsi driver for the 33C93
 */

#define	SBIC_myid	0
#define	SBIC_cdbsize	0
#define	SBIC_control	1
#define	SBIC_timeo	2
#define	SBIC_cdb1	3
#define	SBIC_tsecs	3
#define	SBIC_cdb2	4
#define	SBIC_theads	4
#define	SBIC_cdb3	5
#define	SBIC_tcyl_hi	5
#define	SBIC_cdb4	6
#define	SBIC_tcyl_lo	6
#define	SBIC_cdb5	7
#define	SBIC_addr_hi	7
#define	SBIC_cdb6	8
#define	SBIC_addr_2	8
#define	SBIC_cdb7	9
#define	SBIC_addr_3	9
#define	SBIC_cdb8	10
#define	SBIC_addr_lo	10
#define	SBIC_cdb9	11
#define	SBIC_secno	11
#define	SBIC_cdb10	12
#define	SBIC_headno	12
#define	SBIC_cdb11	13
#define	SBIC_cylno_hi	13
#define	SBIC_cdb12	14
#define	SBIC_cylno_lo	14
#define	SBIC_tlun	15
#define	SBIC_cmd_phase	16
#define	SBIC_syn	17
#define	SBIC_count_hi	18
#define	SBIC_count_med	19
#define	SBIC_count_lo	20
#define	SBIC_selid	21
#define	SBIC_rselid	22
#define	SBIC_csr	23
#define	SBIC_cmd	24
#define	SBIC_data	25
/* sbic_asr is addressed directly */

/*
 *	Register defines
 */

/*
 * Auxiliary Status Register
 */

#define SBIC_ASR_INT		0x80	/* Interrupt pending */
#define SBIC_ASR_LCI		0x40	/* Last command ignored */
#define SBIC_ASR_BSY		0x20	/* Busy, only cmd/data/asr readable */
#define SBIC_ASR_CIP		0x10	/* Busy, cmd unavail also */
#define SBIC_ASR_xxx		0x0c
#define SBIC_ASR_PE		0x02	/* Parity error (even) */
#define SBIC_ASR_DBR		0x01	/* Data Buffer Ready */

/*
 * My ID register, and/or CDB Size
 */

#define SBIC_ID_FS_8_10		0x00	/* Input clock is  8-10 Mhz */
					/* 11 Mhz is invalid */
#define SBIC_ID_FS_12_15	0x40	/* Input clock is 12-15 Mhz */
#define SBIC_ID_FS_16_20	0x80	/* Input clock is 16-20 Mhz */
#define SBIC_ID_EHP		0x10	/* Enable host parity */
#define SBIC_ID_EAF		0x08	/* Enable Advanced Features */
#define SBIC_ID_MASK		0x07
#define SBIC_ID_CBDSIZE_MASK	0x0f	/* if unk SCSI cmd group */

/*
 * Control register
 */

#define SBIC_CTL_DMA		0x80	/* Single byte dma */
#define SBIC_CTL_DBA_DMA	0x40	/* direct buffer acces (bus master)*/
#define SBIC_CTL_BURST_DMA	0x20	/* continuous mode (8237) */
#define SBIC_CTL_NO_DMA		0x00	/* Programmed I/O */
#define SBIC_CTL_HHP		0x10	/* Halt on host parity error */
#define SBIC_CTL_EDI		0x08	/* Ending disconnect interrupt */
#define SBIC_CTL_IDI		0x04	/* Intermediate disconnect interrupt*/
#define SBIC_CTL_HA		0x02	/* Halt on ATN */
#define SBIC_CTL_HSP		0x01	/* Halt on SCSI parity error */

/*
 * Timeout period register
 * [val in msecs, input clk in 0.1 Mhz]
 */

#define	SBIC_TIMEOUT(val,clk)	((((val)*(clk))/800)+1)

/*
 * CDBn registers, note that
 *	cdb11 is used for status byte in target mode (send-status-and-cc)
 *	cdb12 sez if linked command complete, and w/flag if so
 */

/*
 * Target LUN register
 * [holds target status when select-and-xfer]
 */

#define	SBIC_TLUN_VALID		0x80	/* did we receive an Identify msg */
#define	SBIC_TLUN_DOK		0x40	/* Disconnect OK */
#define	SBIC_TLUN_xxx		0x38
#define	SBIC_TLUN_MASK		0x07

/*
 * Command Phase register
 */

#define	SBIC_CPH_MASK		0x7f	/* values/restarts are cmd specific */
#define	SBIC_CPH(p)		((p)&SBIC_CPH_MASK)

/*
 * FIFO register
 */

#define SBIC_FIFO_DEEP		12

/*
 * maximum possible size in TC registers. Since this is 24 bit, it's easy
 */
#define SBIC_TC_MAX		((1<<24) - 1)

/*
 * Synchronous xfer register
 */

#define	SBIC_SYN_OFF_MASK	0x0f
#define	SBIC_SYN_MAX_OFFSET	SBIC_FIFO_DEEP
#define	SBIC_SYN_PER_MASK	0x70
#define	SBIC_SYN_MIN_PERIOD	2		/* upto 8, encoded as 0 */

#define	SBIC_SYN(o,p)		(((o)&SBIC_SYN_OFF_MASK)|(((p)<<4)&SBIC_SYN_PER_MASK))

/*
 * Transfer count register
 * optimal access macros depend on addressing
 */

/*
 * Destination ID (selid) register
 */

#define	SBIC_SID_SCC		0x80	/* Select command chaining (tgt) */
#define	SBIC_SID_DPD		0x40	/* Data phase direction (inittor) */
#	define	SBIC_SID_FROM_SCSI		0x40
#	define	SBIC_SID_TO_SCSI		0x00
#define	SBIC_SID_xxx		0x38
#define	SBIC_SID_IDMASK		0x07

/*
 * Source ID (rselid) register
 */

#define	SBIC_RID_ER		0x80	/* Enable reselection */
#define	SBIC_RID_ES		0x40	/* Enable selection */
#define	SBIC_RID_DSP		0x20	/* Disable select parity */
#define	SBIC_RID_SIV		0x08	/* Source ID valid */
#define	SBIC_RID_MASK		0x07

/*
 * Status register
 */

#define	SBIC_CSR_CAUSE		0xf0
#	define	SBIC_CSR_RESET		0x00	/* chip was reset */
#	define	SBIC_CSR_CMD_DONE	0x10	/* cmd completed */
#	define	SBIC_CSR_CMD_STOPPED	0x20	/* interrupted or abrted*/
#	define	SBIC_CSR_CMD_ERR	0x40	/* end with error */
#	define	SBIC_CSR_BUS_SERVICE	0x80	/* REQ pending on the bus */

#define	SBIC_CSR_QUALIFIER	0x0f

	/* Reset State Interrupts */
#	define	SBIC_CSR_RESET		0x00	/* reset w/advanced features*/
#	define	SBIC_CSR_RESET_AM	0x01	/* reset w/advanced features*/

	/* Successful Completion Interrupts */
#	define	SBIC_CSR_TARGET		0x10	/* reselect complete */
#	define	SBIC_CSR_INITIATOR	0x11	/* select complete */
#	define	SBIC_CSR_WO_ATN		0x13	/* tgt mode completion */
#	define	SBIC_CSR_W_ATN		0x14	/* ditto */
#	define	SBIC_CSR_XLATED		0x15	/* translate address cmd */
#	define	SBIC_CSR_S_XFERRED	0x16	/* initiator mode completion*/
#	define	SBIC_CSR_XFERRED		0x18	/* phase in low bits */

	/* Paused or Aborted Interrupts */
#	define	SBIC_CSR_MSGIN_W_ACK	0x20	/* (I) msgin, ACK asserted*/
#	define	SBIC_CSR_SDP		0x21	/* (I) SDP msg received */
#	define	SBIC_CSR_SEL_ABRT	0x22	/* sel/resel aborted */
#	define	SBIC_CSR_XFR_PAUSED	0x23	/* (T) no ATN */
#	define	SBIC_CSR_XFR_PAUSED_ATN	0x24	/* (T) ATN is asserted */
#	define	SBIC_CSR_RSLT_AM	0x27	/* (I) lost selection (AM) */
#	define	SBIC_CSR_MIS		0x28	/* (I) xfer aborted, ph mis */

	/* Terminated Interrupts */
#	define	SBIC_CSR_CMD_INVALID	0x40
#	define	SBIC_CSR_DISC		0x41	/* (I) tgt disconnected */
#	define	SBIC_CSR_SEL_TIMEO	0x42
#	define	SBIC_CSR_PE		0x43	/* parity error */
#	define	SBIC_CSR_PE_ATN		0x44	/* ditto, ATN is asserted */
#	define	SBIC_CSR_XLATE_TOOBIG	0x45
#	define	SBIC_CSR_RSLT_NOAM	0x46	/* (I) lost sel, no AM mode */
#	define	SBIC_CSR_BAD_STATUS	0x47	/* status byte was nok */
#	define	SBIC_CSR_MIS_1		0x48	/* ph mis, see low bits */

	/* Service Required Interrupts */
#	define	SBIC_CSR_RSLT_NI	0x80	/* reselected, no ify msg */
#	define	SBIC_CSR_RSLT_IFY	0x81	/* ditto, AM mode, got ify */
#	define	SBIC_CSR_SLT		0x82	/* selected, no ATN */
#	define	SBIC_CSR_SLT_ATN	0x83	/* selected with ATN */
#	define	SBIC_CSR_ATN		0x84	/* (T) ATN asserted */
#	define	SBIC_CSR_DISC_1		0x85	/* (I) bus is free */
#	define	SBIC_CSR_UNK_GROUP	0x87	/* strange CDB1 */
#	define	SBIC_CSR_MIS_2		0x88	/* (I) ph mis, see low bits */

#define	SBIC_PHASE(csr)		SCSI_PHASE(csr)

/*
 * Command register (command codes)
 */

#define SBIC_CMD_SBT		0x80	/* Single byte xfer qualifier */
#define	SBIC_CMD_MASK		0x7f

					/* Miscellaneous */
#define SBIC_CMD_RESET		0x00		/* (DTI) lev I */
#define SBIC_CMD_ABORT		0x01		/* (DTI) lev I */
#define SBIC_CMD_DISC		0x04		/* ( TI) lev I */
#define SBIC_CMD_SSCC		0x0d		/* ( TI) lev I */
#define SBIC_CMD_SET_IDI	0x0f		/* (DTI) lev I */
#define SBIC_CMD_XLATE		0x18		/* (DT ) lev II */

					/* Initiator state */
#define SBIC_CMD_SET_ATN	0x02		/* (  I) lev I */
#define SBIC_CMD_CLR_ACK	0x03		/* (  I) lev I */
#define SBIC_CMD_XFER_PAD	0x19		/* (  I) lev II */
#define SBIC_CMD_XFER_INFO	0x20		/* (  I) lev II */

					/* Target state */
#define SBIC_CMD_SND_DISC	0x0e		/* ( T ) lev II */
#define SBIC_CMD_RCV_CMD	0x10		/* ( T ) lev II */
#define SBIC_CMD_RCV_DATA	0x11		/* ( T ) lev II */
#define SBIC_CMD_RCV_MSG_OUT	0x12		/* ( T ) lev II */
#define SBIC_CMD_RCV		0x13		/* ( T ) lev II */
#define SBIC_CMD_SND_STATUS	0x14		/* ( T ) lev II */
#define SBIC_CMD_SND_DATA	0x15		/* ( T ) lev II */
#define SBIC_CMD_SND_MSG_IN	0x16		/* ( T ) lev II */
#define SBIC_CMD_SND		0x17		/* ( T ) lev II */

					/* Disconnected state */
#define SBIC_CMD_RESELECT	0x05		/* (D  ) lev II */
#define SBIC_CMD_SEL_ATN	0x06		/* (D  ) lev II */
#define SBIC_CMD_SEL		0x07		/* (D  ) lev II */
#define SBIC_CMD_SEL_ATN_XFER	0x08		/* (D I) lev II */
#define SBIC_CMD_SEL_XFER	0x09		/* (D I) lev II */
#define SBIC_CMD_RESELECT_RECV	0x0a		/* (DT ) lev II */
#define SBIC_CMD_RESELECT_SEND	0x0b		/* (DT ) lev II */
#define SBIC_CMD_WAIT_SEL_RECV	0x0c		/* (DT ) lev II */

/* approximate, but we won't do SBT on selects */
#define	sbic_isa_select(cmd)	(((cmd)>0x5)&&((cmd)<0xa))

#define PAD(n)          char n;
#ifdef A3000
#define SBIC_CLOCK_FREQUENCY    143	/* according to A3000T service manual */
#endif
#ifdef A2091
#define SBIC_CLOCK_FREQUENCY    77
#endif
#define SBIC_MACHINE_DMA_MODE   SBIC_CTL_DMA

typedef struct {
        volatile unsigned char  sbic_asr;       /* r : Aux Status Register */
#define               sbic_address sbic_asr     /* w : desired register no */
        PAD(pad1);
        volatile unsigned char  sbic_value;     /* rw: register value */
} sbic_padded_ind_regmap_t;


typedef sbic_padded_ind_regmap_t        sbic_padded_regmap_t;

#define	sbic_read_reg(regs,regno,val)	do { \
		(regs)->sbic_address = (regno);	\
		(val) = (regs)->sbic_value;	\
	} while (0)

#define	sbic_write_reg(regs,regno,val)	do { \
		(regs)->sbic_address = (regno);	\
		(regs)->sbic_value = (val);	\
	} while (0)

#define SET_SBIC_myid(regs,val)         sbic_write_reg(regs,SBIC_myid,val)
#define GET_SBIC_myid(regs,val)         sbic_read_reg(regs,SBIC_myid,val)
#define SET_SBIC_cdbsize(regs,val)      sbic_write_reg(regs,SBIC_cdbsize,val)
#define GET_SBIC_cdbsize(regs,val)      sbic_read_reg(regs,SBIC_cdbsize,val)
#define SET_SBIC_control(regs,val)      sbic_write_reg(regs,SBIC_control,val)
#define GET_SBIC_control(regs,val)      sbic_read_reg(regs,SBIC_control,val)
#define SET_SBIC_timeo(regs,val)        sbic_write_reg(regs,SBIC_timeo,val)
#define GET_SBIC_timeo(regs,val)        sbic_read_reg(regs,SBIC_timeo,val)
#define SET_SBIC_cdb1(regs,val)         sbic_write_reg(regs,SBIC_cdb1,val)
#define GET_SBIC_cdb1(regs,val)         sbic_read_reg(regs,SBIC_cdb1,val)
#define SET_SBIC_cdb2(regs,val)         sbic_write_reg(regs,SBIC_cdb2,val)
#define GET_SBIC_cdb2(regs,val)         sbic_read_reg(regs,SBIC_cdb2,val)
#define SET_SBIC_cdb3(regs,val)         sbic_write_reg(regs,SBIC_cdb3,val)
#define GET_SBIC_cdb3(regs,val)         sbic_read_reg(regs,SBIC_cdb3,val)
#define SET_SBIC_cdb4(regs,val)         sbic_write_reg(regs,SBIC_cdb4,val)
#define GET_SBIC_cdb4(regs,val)         sbic_read_reg(regs,SBIC_cdb4,val)
#define SET_SBIC_cdb5(regs,val)         sbic_write_reg(regs,SBIC_cdb5,val)
#define GET_SBIC_cdb5(regs,val)         sbic_read_reg(regs,SBIC_cdb5,val)
#define SET_SBIC_cdb6(regs,val)         sbic_write_reg(regs,SBIC_cdb6,val)
#define GET_SBIC_cdb6(regs,val)         sbic_read_reg(regs,SBIC_cdb6,val)
#define SET_SBIC_cdb7(regs,val)         sbic_write_reg(regs,SBIC_cdb7,val)
#define GET_SBIC_cdb7(regs,val)         sbic_read_reg(regs,SBIC_cdb7,val)
#define SET_SBIC_cdb8(regs,val)         sbic_write_reg(regs,SBIC_cdb8,val)
#define GET_SBIC_cdb8(regs,val)         sbic_read_reg(regs,SBIC_cdb8,val)
#define SET_SBIC_cdb9(regs,val)         sbic_write_reg(regs,SBIC_cdb9,val)
#define GET_SBIC_cdb9(regs,val)         sbic_read_reg(regs,SBIC_cdb9,val)
#define SET_SBIC_cdb10(regs,val)        sbic_write_reg(regs,SBIC_cdb10,val)
#define GET_SBIC_cdb10(regs,val)        sbic_read_reg(regs,SBIC_cdb10,val)
#define SET_SBIC_cdb11(regs,val)        sbic_write_reg(regs,SBIC_cdb11,val)
#define GET_SBIC_cdb11(regs,val)        sbic_read_reg(regs,SBIC_cdb11,val)
#define SET_SBIC_cdb12(regs,val)        sbic_write_reg(regs,SBIC_cdb12,val)
#define GET_SBIC_cdb12(regs,val)        sbic_read_reg(regs,SBIC_cdb12,val)
#define SET_SBIC_tlun(regs,val)         sbic_write_reg(regs,SBIC_tlun,val)
#define GET_SBIC_tlun(regs,val)         sbic_read_reg(regs,SBIC_tlun,val)
#define SET_SBIC_cmd_phase(regs,val)    sbic_write_reg(regs,SBIC_cmd_phase,val)
#define GET_SBIC_cmd_phase(regs,val)    sbic_read_reg(regs,SBIC_cmd_phase,val)
#define SET_SBIC_syn(regs,val)          sbic_write_reg(regs,SBIC_syn,val)
#define GET_SBIC_syn(regs,val)          sbic_read_reg(regs,SBIC_syn,val)
#define SET_SBIC_count_hi(regs,val)     sbic_write_reg(regs,SBIC_count_hi,val)
#define GET_SBIC_count_hi(regs,val)     sbic_read_reg(regs,SBIC_count_hi,val)
#define SET_SBIC_count_med(regs,val)    sbic_write_reg(regs,SBIC_count_med,val)
#define GET_SBIC_count_med(regs,val)    sbic_read_reg(regs,SBIC_count_med,val)
#define SET_SBIC_count_lo(regs,val)     sbic_write_reg(regs,SBIC_count_lo,val)
#define GET_SBIC_count_lo(regs,val)     sbic_read_reg(regs,SBIC_count_lo,val)
#define SET_SBIC_selid(regs,val)        sbic_write_reg(regs,SBIC_selid,val)
#define GET_SBIC_selid(regs,val)        sbic_read_reg(regs,SBIC_selid,val)
#define SET_SBIC_rselid(regs,val)       sbic_write_reg(regs,SBIC_rselid,val)
#define GET_SBIC_rselid(regs,val)       sbic_read_reg(regs,SBIC_rselid,val)
#define SET_SBIC_csr(regs,val)          sbic_write_reg(regs,SBIC_csr,val)
#define GET_SBIC_csr(regs,val)          sbic_read_reg(regs,SBIC_csr,val)
#define SET_SBIC_cmd(regs,val)          sbic_write_reg(regs,SBIC_cmd,val)
#define GET_SBIC_cmd(regs,val)          sbic_read_reg(regs,SBIC_cmd,val)
#define SET_SBIC_data(regs,val)         sbic_write_reg(regs,SBIC_data,val)
#define GET_SBIC_data(regs,val)         sbic_read_reg(regs,SBIC_data,val)

#define SBIC_TC_PUT(regs,val)   {                               \
                sbic_write_reg(regs,SBIC_count_hi,((val)>>16)); \
                (regs)->sbic_value = (val)>>8;  	        \
                (regs)->sbic_value = (val);                     \
        }
#define SBIC_TC_GET(regs,val)   {                               \
                sbic_read_reg(regs,SBIC_count_hi,(val));        \
                (val) = ((val)<<8) | (regs)->sbic_value;        \
                (val) = ((val)<<8) | (regs)->sbic_value;        \
        }

#define SBIC_LOAD_COMMAND(regs,cmd,cmdsize)     {		\
                register int n=cmdsize-1;                       \
                register char *ptr = (char*)(cmd);              \
                sbic_write_reg(regs,SBIC_cdb1,*ptr++);          \
                while (n-- > 0) (regs)->sbic_value = *ptr++;    \
        }

#define GET_SBIC_asr(regs,val)          (val) = (regs)->sbic_asr

#define WAIT_CIP(regs) do ; while ((regs)->sbic_asr & SBIC_ASR_CIP)

/* transmit a byte in programmed I/O mode */
#define SEND_BYTE(regs, ch) \
  WAIT_CIP (regs); \
  SET_SBIC_cmd (regs, SBIC_CMD_SBT | SBIC_CMD_XFER_INFO); \
  SBIC_WAIT (regs, SBIC_ASR_DBR, 0); \
  SET_SBIC_data (regs, ch);

/* receive a byte in programmed I/O mode */
#define RECV_BYTE(regs, ch) \
  WAIT_CIP (regs); \
  SET_SBIC_cmd (regs, SBIC_CMD_SBT | SBIC_CMD_XFER_INFO); \
  SBIC_WAIT (regs, SBIC_ASR_DBR, 0); \
  GET_SBIC_data (regs, ch);





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

extern void scsi_delay (int delay);
extern int scsiinit (register struct amiga_ctlr *ac);
extern void scsireset (void);
extern int scsi_test_unit_rdy (int slave);
extern int scsi_request_sense (int slave, u_char *buf, unsigned int len);
extern int scsi_immed_command (int slave, struct scsi_fmt_cdb *cdb, u_char *buf, unsigned int len, int rd);
extern int scsi_tt_read (int slave, u_char *buf, u_int len, daddr_t blk, int bshift);
extern int scsi_tt_write (int slave, u_char *buf, u_int len, daddr_t blk, int bshift);
extern int scsireq (register struct devqueue *dq);
extern int scsiustart (int unit);
extern void scsistart (int unit);
extern int scsigo (int unit, int slave, struct buf *bp, struct scsi_fmt_cdb *cdb, int pad);
extern void scsidone (int unit);
extern int scsiintr (int unit);
extern void scsifree (register struct devqueue *dq);
extern int scsi_tt_oddio (int slave, u_char *buf, u_int len, int b_flags, int freedma);
