/*
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
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
 * $Id: serreg.h,v 1.3.2.1 1994/07/24 01:23:19 cgd Exp $
 *
 *	Mac II serial device interface
 *
 * 	Information used in this source was gleaned from low-memory
 *	 global variables in MacOS and the Advanced Micro Devices
 *	 1992 Data Book/Handbook.
 */

/* Gleaned from MacOS */
extern volatile unsigned char	*sccA;

#define SER_W0_RSTESINTS	0x10	/* Reset ext/status interrupts */
#define SER_W0_ENBRXRDY		0x20	/* Enable interrupt on next receive */
#define SER_W0_RSTTXPND		0x28	/* Reset transmit interrupt pending */
#define	SER_W0_RSTERR		0x30	/* Reset error */
#define SER_W0_RSTIUS		0x38	/* Reset highest interrupt pending */
#define SER_W0_RSTTXUNDERRUN	0xc0	/* Reset transmit underrun/EOM latch */

#define	SER_W1_ENBEXTINT	0x01	/* Enable external int */
#define SER_W1_ENBTXINT		0x02	/* Enable transmit ready interrupt */
#define SER_W1_ENBR1INT		0x08	/* Rx Int on first char/special cond */
#define SER_W1_ENBRXINT		0x10	/* Rx Int on all chars/special cond */

#define SER_W3_ENBRX		0x01	/* Enable reception */
#define SER_W3_RX5DBITS		0x00	/* Receive 5 data bits */
#define SER_W3_RX6DBITS		0x80	/* Receive 6 data bits */
#define SER_W3_RX7DBITS		0x40	/* Receive 7 data bits */
#define SER_W3_RX8DBITS		0xC0	/* Receive 8 data bits */

#define SER_W4_PARNONE		0x00	/* No parity */
#define SER_W4_PARODD		0x01	/* Odd parity */
#define SER_W4_PAREVEN		0x03	/* Even parity */
#define SER_W4_1SBIT		0x04	/* 1 stop bit */
#define SER_W4_2SBIT		0x0c	/* 2 stop bits */

#define SER_W5_RTS		0x02	/* RTS enable */
#define SER_W5_ENBTX		0x08	/* Enable transmission */
#define SER_W5_BREAK		0x10	/* Send break */
#define SER_W5_TX5DBITS		0x00	/* Send 5 data bits */
#define SER_W5_TX6DBITS		0x40	/* Send 6 data bits */
#define SER_W5_TX7DBITS		0x20	/* Send 7 data bits */
#define SER_W5_TX8DBITS		0x60	/* Send 8 data bits */
#define SER_W5_DTR		0x80	/* DTR enable */

#define SER_W9_HWRESET		0xC0	/* Force Hardware Reset */
#define SER_W9_NV		0x02	/* There is no interrupt vector */
#define SER_W9_DLC		0x04	/* Disable lower interrupt chain */
#define SER_W9_MIE		0x08	/* Enable master interrupt */

#define SER_W10_NRZ		0x00	/* Set NRZ encoding */
#define SER_W11_TXBR		0x80	/* Transmit clock is BR generator */
#define SER_W11_RXBR		0x40	/* Receive clock is BR generator */
#define SER_W14_ENBBR		0x01	/* Enable BR generator */
#define SER_W15_ABRTINT		0x80	/* Abort pending interrups */

#define SER_R0_RXREADY		0x01	/* Received character available */
#define SER_R0_TXREADY		0x04	/* Ready to transmit character */
#define SER_R0_DCD		0x08	/* Carrier detect */
#define SER_R0_CTS		0x20	/* Clear to send */
#define SER_R0_TXUNDERRUN	0x40	/* Tx Underrun/EOM */

#define	SER_R1_RXOVERRUN	0x20
#define	SER_R1_PARITYERR	0x10
#define	SER_R1_CRCERR		0x40
#define	SER_R1_ENDOFFRAME	0x80

#define	SERBRD(x)	(mac68k_machine.sccClkConst / (x) - 2)
#define SCCCNTL(unit)	(sccA[2 - ((unit) << 1)])
#define SCCRDWR(unit)	(sccA[6 - ((unit) << 1)])

#define SER_DOCNTL(unit, reg, val)	\
	{SCCCNTL(unit) = (reg); SCCCNTL(unit) = (val);}
#define SER_STATUS(unit, reg)	\
	(SCCCNTL(unit) = (reg), SCCCNTL(unit))

