/*	$NetBSD: serreg.h,v 1.6 1995/02/11 19:06:59 briggs Exp $	*/

/*
 * Copyright (C) 1993   Allen K. Briggs, Chris P. Caputo,
 *                      Michael L. Finch, Bradley A. Grantham, and
 *                      Lawrence A. Kesteloot
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
 *      This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *      Mac II serial device interface
 *
 *      Information used in this source was gleaned from low-memory
 *       global variables in MacOS and the Advanced Micro Devices
 *       1992 Data Book/Handbook.
 */

/* Gleaned from MacOS */
extern volatile unsigned char   *sccA;

/*
 * Following information taken from Zilog's SCC User manal(1992) and the
 * Zilog up and Peripherals databook (vol1/1992)
 *
 * Interrupt Source Priority:
 *		channel A rx           -highest
 *		channel A tx
 *		channel A ext/status
 *		channel B rx
 *		channel B tx
 *		channel B ext/status   -lowest
 *
 */

/*
 * Write register 0 (Command Register)
 */
   
#define SER_W0_RSTESINTS        0x10    /* Reset ext/status interrupts */
                                        /* after an ext/status interrupt the
                                         * status bits are latched into RR0
                                         * This re-enables the bits  and allows
                                         * further interrupts due to ext/status
                                         * change 
                                         */
#define SER_W0_ENBRXRDY         0x20    /* Enable interrupt on next receive */
                                        /* if using interrupt on 1st rx'd char
                                         * then this is used to reactivate
                                         * ints on rx after youve read the rx
                                         * fifo, otherwise you get another int
                                         * straight away if there's another 
                                         * rx char in the fifo
                                         */
#define SER_W0_RSTTXPND         0x28    /* Reset transmit interrupt pending */
                                        /* used when there are no more chars
                                         * to be sent. used to stop the tx'er
                                         * from int'ing when the tx buffer 
                                         * becomes empty (with tx ints enabl'd)
                                         */
#define SER_W0_RSTERR           0x30    /* Reset error */
                                        /* resets error bits in RR1. the datum
                                         * assoc'd with the error is held in
                                         * rx fifo and is lost after this cmd
                                         */
#define SER_W0_RSTIUS           0x38    /* Reset highest interrupt pending */
                                        /* used as the last cmd in an interrupt
                                         * service: lets lower priority 
                                         * interrupts to request service
                                         */
#define SER_W0_RSTTXUNDERRUN 0xc0       /* Reset transmit underrun/EOM latch */
                                        /* when TX underrun/eom latch has been
                                         * reset, the scc sends an abort and
                                         * flag on underrun. this command resets
                                         * that latch.
                                         */
/*
 * Write register 1 (tx/rx interrupt and data transfer mode definition)
 */

#define SER_W1_ENBEXTINT        0x01    /* Enable external int */
#define SER_W1_ENBTXINT         0x02    /* Enable transmit ready interrupt */
#define SER_W1_PARISSPEC	0x04	/* parity err is a special cond */
#define SER_W1_ENBR1INT         0x08    /* Rx Int on first char/special cond */
#define SER_W1_ENBRXINT         0x10    /* Rx Int on all chars/special cond */

/*
 * Write register 2 (interrupt vector) - see WR9
 */

/*
 * Write register 3 (rx parameters and Control)
 * can read from RR9 with extended read option on (for eSCC)
 */

#define SER_W3_ENBRX            0x01    /* rx enable */
#define SER_W3_AUTOEN           0x20    /* auto enable */
                                        /* causes CTS to become the tx
                                         * enable and DCD to become the rx
                                         * enable.
                                         */
#define SER_W3_RX5DBITS         0x00    /* Receive 5 data bits */
#define SER_W3_RX6DBITS         0x80    /* Receive 6 data bits */
#define SER_W3_RX7DBITS         0x40    /* Receive 7 data bits */
#define SER_W3_RX8DBITS         0xC0    /* Receive 8 data bits */

/*
 * Write Register 4 (tx/rx misc param and modes) 
 */

#define SER_W4_PARNONE          0x00    /* No parity */
#define SER_W4_PARODD           0x01    /* Odd parity */
#define SER_W4_PAREVEN          0x03    /* Even parity */
#define SER_W4_1SBIT            0x04    /* 1 stop bit */
#define SER_W4_2SBIT            0x0c    /* 2 stop bits */
#define SER_W4_CLKX1            0x00    /* clock rate = data rate */
#define SER_W4_CLKX16           0x40    /* clock rate = 16 * data rate */
#define SER_W4_CLKX32           0x80    /* clock rate = 32 * data rate */
#define SER_W4_CLKX64           0xc0    /* clock rate = 64 * data rate */

/*
 * Write Register 5 (Tx params and control)
 */

#define SER_W5_RTS              0x02    /* RTS enable */
#define SER_W5_ENBTX            0x08    /* Enable transmission */
#define SER_W5_BREAK            0x10    /* Send break */
#define SER_W5_TX5DBITS         0x00    /* Send 5 data bits */
#define SER_W5_TX6DBITS         0x40    /* Send 6 data bits */
#define SER_W5_TX7DBITS         0x20    /* Send 7 data bits */
#define SER_W5_TX8DBITS         0x60    /* Send 8 data bits */
#define SER_W5_DTR              0x80    /* DTR enable */

/*
 * Write register 8 (Transmit buffer)
 */

/*
 * Write Register 9 (Master interrupt control)
 */
#define SER_W9_ARESET		0x80	/* reset channel A */
#define SER_W9_BRESET		0x40	/* reset channel A */
#define SER_W9_HWRESET          ( SER_W9_ARESET | SER_W9_BRESET ) /* both */
#define SER_W9_NV               0x02    /* There is no interrupt vector */
#define SER_W9_DLC              0x04    /* Disable lower interrupt chain */
#define SER_W9_MIE              0x08    /* Enable master interrupt */
					/* (MIE is cleared on a HWRESET) */

/* Write Register 10 (Misc tx/rx control bits */

#define SER_W10_NRZ             0x00    /* Set NRZ encoding */
#define SER_W10_URFLG		0x04	/* abort/flag on underrun (sdlc only) */

/* Write Register 11 (Clock mode control) */

#define SER_W11_TXBR            0x10    /* Transmit clock is BR generator */
#define SER_W11_RXBR            0x40    /* Receive clock is BR generator */

/* Write Register 12 (Lower byte of baud constant)
 * Write Register 13 (Upper byte of baud constant)
 * Write Register 14 (Misc control of baud)
 *
 * The baud constant is computed by:
 *
 * Baud constant = ( clock_freq / ( 2* desired_rate * BR_clk_period )) - 2
 */

#define SERBRD(x)       (mac68k_machine.sccClkConst / (x) -2 )

#define SER_W14_ENBBR           0x01    /* Enable BR generator */

/*
 * Write Register 15 (Ext/ststus interrupt control)
 */

#define SER_W15_DCDINT		0x08	/* enable DCD interrupts */
#define SER_W15_CTSINT		0x20	/* enable CTS interrupts */
#define SER_W15_BRKINT          0x80    /* Abort pending interrups */

/*
 * Read Register 0 (tx/rx buffer status & ext status)
 */

#define SER_R0_RXREADY          0x01    /* Received character available */
#define SER_R0_TXREADY          0x04    /* Ready to transmit character */
#define SER_R0_DCD              0x08    /* Carrier detect */
#define SER_R0_CTS              0x20    /* Clear to send */
#define SER_R0_TXUNDERRUN       0x40    /* Tx Underrun/EOM */
#define SER_R0_BREAK  		0x80    /* Break/abort */

#define SER_R1_RXOVERRUN        0x20
#define SER_R1_PARITYERR        0x10
#define SER_R1_CRCERR           0x40
#define SER_R1_ENDOFFRAME       0x80

/*
 * Read Register 2
 * channel A: is contents of WR2
 * channel B: is status of an interrupt (low here if WR9 appropriate)
 */

#define SER_R2_MASK		0x06	/* what kind of int was it */
#define SER_R2_TX		0x00	/* tx buffer empty */
#define SER_R2_EXTCHG		0x02	/* external/status change */
#define SER_R2_RX		0x04	/* rx char avail */
#define SER_R2_SPEC		0x06	/* special recv condition */

#define SER_R2_CHANMASK		0x08	/* which channell caused int */
#define SER_R2_CHANA		0x08	
#define SER_R2_CHANB		0x00

/* only from channel A */
#define SER_R3_BIPES		0x01	/* chanB ext/stat interrupt pending */
#define SER_R3_BIPTX		0x02	/* chanB tx ip */
#define SER_R3_BIPRX		0x04	/* chanB rx ip */
#define SER_R3_AIPES		0x08	/* chanA ext/stat interrupt pending */
#define SER_R3_AIPTX		0x10	/* chanA tx ip */
#define SER_R3_AIPRX		0x20	/* chanA rx ip */

#define SCCCNTL(unit)   (sccA[2 - ((unit) << 1)])
#define SCCRDWR(unit)   (sccA[6 - ((unit) << 1)])

#define SER_DOCNTL(unit, reg, val)      \
        SCCCNTL(unit) = (reg), SCCCNTL(unit) = (val)
#define SER_STATUS(unit, reg)   \
        (SCCCNTL(unit) = (reg), SCCCNTL(unit))

