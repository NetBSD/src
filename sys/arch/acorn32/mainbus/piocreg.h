/*	$NetBSD: piocreg.h,v 1.1.4.2 2001/10/05 22:27:54 reinoud Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Peripheral I/O controller registers
 */

/*
 * 
 */

#define PIOC_SIZE		(0x1000 + 0x2000)	/* XXX */

/*
 * I/O registers for managing PIOC configuration
 */

#define PIOC_CM_SELECT_REG	0x3f0
#define PIOC_CM_DATA_REG	0x3f1

/*
 * Bytes to write to the select register to switch in and out for config mode
 */

#define PIOC_CM_ENTER_665	0x55	/* SMC FDC37GT665 */
#define PIOC_CM_ENTER_666	0x44	/* SMC FDC37GT666 */
#define PIOC_CM_EXIT		0xaa

/*
 * Configuration register selection codes
 */

#define PIOC_CM_CR0		0x0	/* IDE and floppy setup */
#define  PIOC_WDC_ENABLE	0x01	/* wdc enable */
#define  PIOC_FDC_ENABLE	0x10	/* fdc enable */
#define PIOC_CM_CR1		0x1	/* parallel and serial setup */
#define  PIOC_LPT_ADDR_MASK	0x03	/* lpt address mask */
#define  PIOC_LPT_ADDR_DISABLE	0x00	/* lpt disabled */
#define  PIOC_LPT_ADDR_1	0x01	/* lpt address 1 */
#define  PIOC_LPT_ADDR_2	0x02	/* lpt address 2 */
#define  PIOC_LPT_ADDR_3	0x03	/* lpt address 3 */
#define PIOC_CM_CR2		0x2	/* serial setup */
#define  PIOC_UART1_ADDR_MASK	0x03	/* uart1 address mask */
#define  PIOC_UART1_ADDR_COM1	0x00	/* uart1 address com1 */
#define  PIOC_UART1_ADDR_COM2	0x01	/* uart1 address com2 */
#define  PIOC_UART1_ADDR_COM3	0x02	/* uart1 address com3 */
#define  PIOC_UART1_ADDR_COM4	0x03	/* uart1 address com4 */
#define  PIOC_UART1_ENABLE	0x04	/* uart1 enable */
#define  PIOC_UART2_ADDR_MASK	0x30	/* uart2 address mask */
#define  PIOC_UART2_ADDR_COM1	0x00	/* uart2 address com1 */
#define  PIOC_UART2_ADDR_COM2	0x10	/* uart2 address com2 */
#define  PIOC_UART2_ADDR_COM3	0x20	/* uart2 address com3 */
#define  PIOC_UART2_ADDR_COM4	0x30	/* uart2 address com4 */
#define  PIOC_UART2_ENABLE	0x40	/* uart2 enable */
#define PIOC_CM_CR3		0x3	/* parallel setup */
#define PIOC_CM_CR4		0x4	/* parallel and serial extended setup */
#define PIOC_CM_CR5		0x5	/* floppy & IDE extended setup */
#define  PIOC_FDC_SECONDARY	0x01	/* fdc secondary address enable */
#define  PIOC_WDC_SECONDARY	0x02	/* wdc secondary address enable */
#define PIOC_CM_CR6		0x6	/* floppy drive types */
#define PIOC_CM_CR7		0x7	/* media ID & boot drive */
#define PIOC_CM_CR8		0x8	/* PIOC address low */
#define PIOC_CM_CR9		0x9	/* PIOC address high */
#define PIOC_CM_CRA		0xa	/* ECP FIFO threshold */
#define PIOC_CM_CRB		0xb	/* reserved */
#define PIOC_CM_CRC		0xc	/* reserved */
#define PIOC_CM_CRD		0xd	/* PIOC ID */
#define PIOC_CM_CRE		0xe	/* PIOC revision */
#define PIOC_CM_CRF		0xf	/* reserve for testing */
#define PIOC_CM_REGS		0x10	/* number of registers */

/*
 * PIOC ID values
 */

#define PIOC_CM_ID_665		0x65
#define PIOC_CM_ID_666		0x66

/*
 * PIOC offsets
 */

#define PIOC_FDC_PRIMARY_OFFSET		0x3f0
#define PIOC_FDC_SECONDARY_OFFSET	0x370
#define PIOC_WDC_PRIMARY_OFFSET		0x1f0
#define PIOC_WDC_SECONDARY_OFFSET	0x170
#define PIOC_LPT1_OFFSET		0x3bc
#define PIOC_LPT2_OFFSET		0x378
#define PIOC_LPT3_OFFSET		0x278
#define PIOC_COM1_OFFSET		0x3f8
#define PIOC_COM2_OFFSET		0x2f8
#define PIOC_COM3_OFFSET		0x338
#define PIOC_COM4_OFFSET		0x238

/* End of piocreg.h */
