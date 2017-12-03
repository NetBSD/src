/*	$NetBSD: qvareg.h,v 1.1.18.2 2017/12/03 11:36:48 jdolecek Exp $	*/

/*
 * Copyright (c) 2015 Charles H. Dickman. All rights reserved.
 * Derived from sgimips port
 * Copyright (c) 1996, 1997 Philip L. Budne.
 * Copyright (c) 1993 Philip A. Nelson.
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
 *	This product includes software developed by Philip A. Nelson.
 * 4. The name of Philip A. Nelson may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP NELSON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	scnreg.h: definitions for qvss scn2681 DUART
 */

/*
 * register offsets
 */

/* per-channel regs (channel B's at SCN_REG(8-11)) */
#define CH_MR(x)	SCN_REG(0 + 8*(x))	/* rw mode register */
#define CH_SR(x)	SCN_REG(1 + 8*(x))	/* ro status register */
#define CH_CSR(x)	SCN_REG(1 + 8*(x))	/* wo clock select reg */
#define CH_CR(x)	SCN_REG(2 + 8*(x))	/* wo command reg */
#define CH_DAT(x)	SCN_REG(3 + 8*(x))	/* rw data reg */

/* duart-wide regs */
#define DU_IPCR	SCN_REG(4)	/* ro input port change reg */
#define DU_ACR	SCN_REG(4)	/* wo aux control reg */
#define DU_ISR	SCN_REG(5)	/* ro interrupt stat reg */
#define DU_IMR	SCN_REG(5)	/* wo interrupt mask reg */
#define DU_CTUR	SCN_REG(6)	/* rw counter timer upper reg */
#define DU_CTLR	SCN_REG(7)	/* rw counter timer lower reg */
				/* SCN_REG(8-11) channel b (see above) */
				/* SCN_REG(12): reserved */
#define DU_IP	SCN_REG(13)	/* ro input port */
#define DU_OPCR	SCN_REG(13)	/* wo output port cfg reg */
#define DU_CSTRT SCN_REG(14)	/* ro start C/T cmd */
#define DU_OPSET SCN_REG(14)	/* wo output port set */
#define DU_CSTOP SCN_REG(15)	/* ro stop C/T cmd */
#define DU_OPCLR SCN_REG(15)	/* wo output port reset */



struct  qvaux_ch_regs  {
        bus_addr_t qr_mr;        
        bus_addr_t qr_sr;        
        bus_addr_t qr_csr;        
        bus_addr_t qr_cr;        
        bus_addr_t qr_dat;                
};

struct	qvaux_regs	{
        bus_addr_t qr_ipcr;        
        bus_addr_t qr_acr;        
        bus_addr_t qr_isr;        
        bus_addr_t qr_imr;        
        bus_addr_t qr_ctur;        
        bus_addr_t qr_ctlr;        
        bus_addr_t qr_ip;        
        bus_addr_t qr_opcr;        
        bus_addr_t qr_cstrt;        
        bus_addr_t qr_opset;        
        bus_addr_t qr_cstop;        
        bus_addr_t qr_opclr;        
        struct qvaux_ch_regs qr_ch_regs[2];
        
	bus_addr_t qr_firstreg;
	bus_addr_t qr_winsize;
};

/*
 * Data Values
 */

/*
 * MR (mode register) -- per channel
 */

/* MR0 (scn26c92 only) need to use CR_CMD_MR0 before access */
#define MR0_MODE	0x07	/* extended baud rate mode (MR0A only) */
#define MR0_TXINT	0x30	/* Tx int threshold */
#define MR0_RXINT	0x40	/* Rx int threshold (along with MR1_FFULL) */
#define MR0_RXWD	0x80	/* Rx watchdog (8 byte-times after last rx) */

#define MR0_MODE_0	0x00	/* Normal mode */
#define MR0_MODE_1	0x01	/* Extended mode 1 */
#define MR0_MODE_2	0x04	/* Extended mode 2 */

#define MR0_TXINT_EMPTY	0x00	/* TxInt when 8 FIFO bytes empty  (default) */
#define MR0_TXINT_4	0x10	/* TxInt when 4 or more FIFO bytes empty */
#define MR0_TXINT_6	0x20	/* TxInt when 6 or more FIFO bytes empty */
#define MR0_TXINT_TXRDY	0x30	/* TxInt when 1 or more FIFO bytes empty */

/* MR1 (need to use CR_CMD_MR1 before each access) */
#define MR1_CS5		0x00
#define MR1_CS6		0x01
#define MR1_CS7		0x02
#define MR1_CS8		0x03

#define	MR1_PEVEN	0x00
#define MR1_PODD	0x04
#define MR1_PNONE	0x10

#define MR1_RXBLK	0x20	/* "block" error mode */
#define MR1_FFULL	0x40	/* wait until FIFO full for rxint (cf MR0) */
#define MR1_RXRTS	0x80	/* auto RTS input flow ctrl */

/* MR2 (any access to MR after MR1) */
#define MR2_STOP	0x0f	/* mask for stop bits */
#define MR2_STOP1	0x07
#define MR2_STOP2	0x0f

#define MR2_TXCTS	0x10	/* transmitter follows CTS */
#define MR2_TXRTS	0x20	/* RTS follows transmitter */
#define MR2_MODE	0xc0	/* mode mask */

/*
 * IP (input port)
 */
#define IP_IP0		0x01
#define IP_IP1		0x02
#define IP_IP2		0x04
#define IP_IP3		0x08
#define IP_IP4		0x10
#define IP_IP5		0x20
#define IP_IP6		0x40
/* D7 is always 1 */

/*
 * ACR (Aux Control Register)
 */

#define ACR_DELTA_IP0	0x01	/* enable IP0 delta interrupt */
#define ACR_DELTA_IP1	0x02	/* enable IP1 delta interrupt */
#define ACR_DELTA_IP2	0x04	/* enable IP2 delta interrupt */
#define ACR_DELTA_IP3	0x08	/* enable IP3 delta interrupt */
#define ACR_CT		0x70	/* counter/timer mode (ACT_CT_xxx) */
#define ACR_BRG		0x80	/* baud rate generator speed set */

/* counter/timer mode */
#define ACR_CT_CEXT	0x00	/* counter: external (IP2) */
#define ACR_CT_CTXA	0x10	/* counter: TxCA x 1 */
#define ACR_CT_CTXB	0x20	/* counter: TxCB x 1 */
#define ACR_CT_CCLK	0x30	/* counter: X1/CLK div 16 */
#define ACR_CT_TEXT1	0x40	/* timer: external (IP2) */
#define ACR_CT_TEXT16	0x50	/* timer: external (IP2) div 16 */
#define ACR_CT_TCLK1	0x60	/* timer: X1/CLK */
#define ACR_CT_TCLK16	0x70	/* timer: X1/CLK div 16 */

/*
 * IPCR (Input Port Change Register) -- per channel
 */
#define IPCR_IP0	0x01
#define IPCR_IP1	0x02
#define IPCR_IP2	0x04
#define IPCR_IP3	0x08
#define IPCR_DELTA_IP0	0x10
#define IPCR_DELTA_IP1	0x20
#define IPCR_DELTA_IP2	0x40
#define IPCR_DELTA_IP3	0x80

/*
 * output port config register
 * if bit(s) clear OP line follows OP register OPn bit
 */

#define OPCR_OP7_TXRDYB	0x80	/* OP7: TxRDYB */
#define OPCR_OP6_TXRDYA	0x40	/* OP6: TxRDYA */
#define OPCR_OP5_RXRDYB	0x20	/* OP5: ch B RxRDY/FFULL */
#define OPCR_OP4_RXRDYA	0x10	/* OP4: ch A RxRDY/FFULL */

#define OPCR_OP3	0xC0	/* OP3: mask */
#define OPCR_OP2	0x03	/* OP2: mask */

/*
 * output port
 */
#define OP_OP0		0x01
#define OP_OP1		0x02
#define OP_OP2		0x04
#define OP_OP3		0x08
#define OP_OP4		0x10
#define OP_OP5		0x20
#define OP_OP6		0x40
#define OP_OP7		0x80

/*
 * CR (command register) -- per channel
 */

/* bits (may be or'ed together, with a command) */
#define CR_ENA_RX	0x01
#define CR_DIS_RX	0x02
#define CR_ENA_TX	0x04
#define CR_DIS_TX	0x08

/* commands */
#define CR_CMD_NOP	0x00
#define CR_CMD_MR1	0x10
#define CR_CMD_RESET_RX	0x20
#define CR_CMD_RESET_TX	0x30
#define CR_CMD_RESET_ERR 0x40
#define CR_CMD_RESET_BRK 0x50
#define CR_CMD_START_BRK 0x60
#define CR_CMD_STOP_BRK	0x70

/* 2692-only commands */
#define CR_CMD_RTS_ON	0x80	/* raise RTS */
#define CR_CMD_RTS_OFF	0x90	/* lower RTS */
#define CR_CMD_TIM_ON	0xa0	/* enable timeout mode */
#define CR_CMD_TIM_OFF	0xc0	/* reset timeout mode */
#define CR_CMD_PDN_ON	0xe0	/* power down mode on */
#define CR_CMD_PDN_RUN	0xf0	/* power down mode off (normal run) */

/* 26C92-only commands */
#define CR_CMD_MR0	0xb0	/* MR0 select */

/*
 * CSR (clock select register) -- per channel
 */
#define CSR_B75	        0x0
#define CSR_B110	0x1
#define CSR_B134	0x2
#define CSR_B150	0x3
#define CSR_B300	0x4
#define CSR_B600	0x5
#define CSR_B1200	0x6
#define CSR_B2000	0x7
#define CSR_B2400	0x8
#define CSR_B4800	0x9
#define CSR_B7200	0xa
#define CSR_B9600	0xb
#define CSR_B19200	0xc

/*
 * SR (status register) -- per channel
 */
#define SR_RX_RDY	0x01
#define SR_RX_FFULL	0x02	/* rx fifo full */
#define SR_TX_RDY	0x04	/* tx room for more */
#define SR_TX_EMPTY	0x08	/* tx dry */

#define SR_OVERRUN	0x10

/* bits cleared by reset error (see MR1 error mode bit) */
#define SR_PARITY	0x20	/* received parity error */
#define SR_FRAME	0x40	/* received framing error */
#define SR_BREAK	0x80	/* received break */

/*
 * Interrupt Mask Register (IMR) and ISR (Interrupt Status Register)
 */
#define INT_TXA		0x01	/* Tx Ready A */
#define INT_RXA		0x02	/* Rx Ready/FIFO Full A */
#define INT_BRKA	0x04	/* Delta Break A */
#define INT_CTR		0x08	/* counter ready */
#define INT_TXB		0x10	/* Tx Ready B */
#define INT_RXB		0x20	/* Rx Ready/FIFO Full B */
#define INT_BRKB	0x40	/* Delta Break B */
#define INT_IP		0x80	/* input port change */
