/*	$NetBSD: at91dbgureg.h,v 1.3.12.2 2013/01/16 05:32:45 yamt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy
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
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _AT91DBGUREG_H_
#define _AT91DBGUREG_H_

#define	AT91DBGU_BASE	0xFFFFF200	/* DBGU unit base addresses (physical) */
#define	AT91DBGU_SIZE	0x200		/* DBGU peripheral address space size */

#define	DBGU_CR		0x00UL	/* Control Register			*/
#define	DBGU_MR		0x04UL	/* Mode Register			*/
#define	DBGU_IER	0x08UL	/* Interrupt Enable Register		*/
#define	DBGU_IDR	0x0CUL	/* Interrupt Disable Register		*/
#define	DBGU_IMR	0x10UL	/* Interrupt Mask Register		*/
#define	DBGU_SR		0x14UL	/* Status Register               	*/
#define	DBGU_RHR        0x18UL	/* Receive Holding Register      	*/
#define	DBGU_THR	0x1CUL	/* Transmit Holding Register     	*/
#define	DBGU_BRGR	0x20UL	/* Baud Rate Generator Register  	*/
#define	DBGU_CIDR	0x40UL	/* Chip ID Register              	*/
#define DBGU_EXID	0x44UL	/* Chip ID Extension Register		*/
#define	DBGU_FNR	0x48UL	/* Force NTRST Register     		*/
#define	DBGU_PDC	0x100UL

/* Control Register bits: */
#define	DBGU_CR_RSTSTA		0x100	/* 1 = reset the status bits	*/
#define	DBGU_CR_TXDIS		0x080	/* 1 = disable transmitter	*/
#define	DBGU_CR_TXEN		0x040	/* 1 = enable transmitter	*/
#define	DBGU_CR_RXDIS		0x020	/* 1 = disable receiver		*/
#define	DBGU_CR_RXEN		0x010	/* 1 = enable receiver		*/
#define	DBGU_CR_RSTTX		0x008	/* 1 = reset transmitter	*/
#define	DBGU_CR_RSTRX		0x004	/* 1 = reset receiver		*/

/* Mode Register bits: */
#define	DBGU_MR_CHMODE		0xC000	/* channel mode			*/
#define	DBGU_MR_CHMOD_NORMAL	0x0000
#define	DBGU_MR_CHMOD_ECHO	0x4000
#define	DBGU_MR_CHMOD_LOCAL_LOOP 0x8000
#define	DBGU_MR_CHMOD_REMOTE_LOOP 0xC000

#define	DBGU_MR_PAR		0x0E00	/* parity type			*/
#define	DBGU_MR_PAR_EVEN	0x0000
#define	DBGU_MR_PAR_ODD		0x0200
#define	DBGU_MR_PAR_SPACE	0x0400
#define	DBGU_MR_PAR_MARK	0x0600
#define	DBGU_MR_PAR_NONE	0x0800

/* Interrupt bits: */
#define	DBGU_INT_COMMRX		0x80000000 
#define	DBGU_INT_COMMTX		0x40000000
#define	DBGU_INT_RXBUFF		0x00001000
#define	DBGU_INT_TXBUFE		0x00000800
#define	DBGU_INT_TXEMPTY	0x00000200
#define	DBGU_INT_PARE		0x00000080
#define	DBGU_INT_FRAME		0x00000040
#define	DBGU_INT_OVRE		0x00000020
#define	DBGU_INT_ENDTX		0x00000010
#define	DBGU_INT_ENDRX		0x00000008
#define	DBGU_INT_TXRDY		0x00000002
#define	DBGU_INT_RXRDY		0x00000001

/* Status register bits: */
#define	DBGU_SR_COMMRX		0x80000000
#define	DBGU_SR_COMMTX		0x40000000
#define	DBGU_SR_RXBUFF		0x00001000
#define	DBGU_SR_TXBUFE		0x00000800
#define	DBGU_SR_TXEMPTY		0x00000200
#define	DBGU_SR_PARE		0x00000080
#define	DBGU_SR_FRAME		0x00000040
#define	DBGU_SR_OVRE		0x00000020
#define	DBGU_SR_ENDTX		0x00000010
#define	DBGU_SR_ENDRX		0x00000008
#define	DBGU_SR_TXRDY		0x00000002
#define	DBGU_SR_RXRDY		0x00000001


/* Chip ID Register bits: */
#define	DBGU_CIDR_EXT		0x80000000	/* 1 = Extended Chip ID exists */

#define	DBGU_CIDR_NVPTYP	0x70000000	/* Nonvolatile Pgm Mem Type */
#define	DBGU_CIDR_NVPTYP_ROM	0x00000000
#define	DBGU_CIDR_NVPTYP_ROMLESS 0x10000000
#define	DBGU_CIDR_NVPTYP_SRAM	0x40000000
#define	DBGU_CIDR_NVPTYP_FLASH	0x20000000
#define	DBGU_CIDR_NVTYP_ROM_FLASH 0x30000000	/* NVPSIZ is ROM size, NVPSIZ2 is Flash Size */

#define	DBGU_CIDR_ARCH		0x0FF00000	/* Architecture identifier	*/
#define	DBGU_CIDR_ARCH_AT91SAM9XX 0x01900000	/* AT91SAM9xx Series */
#define	DBGU_CIDR_ARCH_AT91SAM9XEXX 0x02900000	/* AT91SAM9XExx Series */
#define	DBGU_CIDR_ARCH_AT91X34	0x03400000	/* AT91x34 Series */
#define	DBGU_CIDR_ARCH_CAP9	0x03900000	/* CAP9 Series */
#define	DBGU_CIDR_ARCH_AT91X40	0x04000000	/* AT91x40 Series */
#define	DBGU_CIDR_ARCH_AT91X42	0x04200000	/* AT91x42 Series */
#define	DBGU_CIDR_ARCH_AT91X55	0x05500000	/* AT91x55 Series */
#define	DBGU_CIDR_ARCH_AT91SAM7AXX 0x06000000	/* AT91SAM7Axx Series */
#define	DBGU_CIDR_ARCH_AT91X63	0x06300000	/* AT91x63 Series */
#define	DBGU_CIDR_ARCH_AT91SAM7SXX 0x07000000	/* AT91SAM7Sxx Series */
#define	DBGU_CIDR_ARCH_AT91SAM7XCXX 0x07100000	/* AT91SAM7XCxx Series */
#define	DBGU_CIDR_ARCH_AT91SAM7SEXX 0x07200000	/* AT91SAM7SExx Series */
#define	DBGU_CIDR_ARCH_AT91SAM7LXX 0x07300000	/* AT91SAM7LExx Series */
#define	DBGU_CIDR_ARCH_AT91SAM7XXX 0x07500000	/* AT91SAM7Xxx Series */
#define	DBGU_CIDR_ARCH_AT91X92	0x09200000	/* AT91x92 Series */
#define	DBGU_CIDR_ARCH_AT75CXX	0x0F000000	/* AT75Cxx Series */

#define	DBGU_CIDR_SRAMSIZ	0x000F0000	/* Internal SRAM Size	*/
#define	DBGU_CIDR_SRAMSIZ_1K	0x00010000
#define	DBGU_CIDR_SRAMSIZ_2K	0x00020000
#define	DBGU_CIDR_SRAMSIZ_4K	0x00050000
#define	DBGU_CIDR_SRAMSIZ_8K	0x00080000
#define	DBGU_CIDR_SRAMSIZ_16K	0x00090000
#define	DBGU_CIDR_SRAMSIZ_32K	0x000A0000
#define	DBGU_CIDR_SRAMSIZ_64K	0x000B0000
#define	DBGU_CIDR_SRAMSIZ_128K	0x000C0000
#define	DBGU_CIDR_SRAMSIZ_256K	0x000D0000
#define	DBGU_CIDR_SRAMSIZ_96K	0x000E0000
#define	DBGU_CIDR_SRAMSIZ_512K	0x000F0000

#define	DBGU_CIDR_NVPSIZ	0x00000F00	/* Nonvolatile Pgm Mem Size */
#define	DBGU_CIDR_NVPSIZ_NONE	0x00000000
#define	DBGU_CIDR_NVPSIZ_8K	0x00000100
#define	DBGU_CIDR_NVPSIZ_16K	0x00000200
#define	DBGU_CIDR_NVPSIZ_32K	0x00000300
#define	DBGU_CIDR_NVPSIZ_64K	0x00000500
#define	DBGU_CIDR_NVPSIZ_128K	0x00000700
#define	DBGU_CIDR_NVPSIZ_256K	0x00000900
#define	DBGU_CIDR_NVPSIZ_512K	0x00000A00
#define	DBGU_CIDR_NVPSIZ_1024K	0x00000C00
#define	DBGU_CIDR_NVPSIZ_2048K	0x00000E00

#define	DBGU_CIDR_NPVSIZ2	0x0000F000	/* Nonvolatile Pgm 2 Mem Size */
#define	DBGU_CIDR_NVPSIZ2_NONE	0x00000000
#define	DBGU_CIDR_NVPSIZ2_8K	0x00001000
#define	DBGU_CIDR_NVPSIZ2_16K	0x00002000
#define	DBGU_CIDR_NVPSIZ2_32K	0x00003000
#define	DBGU_CIDR_NVPSIZ2_64K	0x00005000
#define	DBGU_CIDR_NVPSIZ2_128K	0x00007000
#define	DBGU_CIDR_NVPSIZ2_256K	0x00009000
#define	DBGU_CIDR_NVPSIZ2_512K	0x0000A000
#define	DBGU_CIDR_NVPSIZ2_1024K	0x0000C000
#define	DBGU_CIDR_NVPSIZ2_2048K	0x0000E000

#define	DBGU_CIDR_EPROC		0x000000E0	/* Embedded Processor ID */
#define	DBGU_CIDR_EPROC_946ES	0x00000020
#define	DBGU_CIDR_EPROC_7TDMI	0x00000040
#define	DBGU_CIDR_EPROC_920T	0x00000080
#define	DBGU_CIDR_EPROC_926EJS	0x000000E0

#define	DBGU_CIDR_VERSION	0x0000001F	/* version of the device */

#define	DBGU_CIDR_AT91RM9200	0x09290781
#define	DBGU_CIDR_AT91SAM9260	0x019803A2
#define	DBGU_CIDR_AT91SAM9261	0x019703A0
#define	DBGU_CIDR_AT91SAM9263	0x019607A0

#define	AT91RM9200_CHIP_ID	DBGU_CIDR_AT91RM9200
#define	AT91SAM9260_CHIP_ID	DBGU_CIDR_AT91SAM9260
#define	AT91SAM9261_CHIP_ID	DBGU_CIDR_AT91SAM9261
#define	AT91SAM9263_CHIP_ID	DBGU_CIDR_AT91SAM9263

#define	DBGUREG(reg)		*((volatile uint32_t*)(AT91DBGU_BASE + (reg)))

#define	DBGU_INIT(mstclk, speed) do {					\
  DBGUREG(DBGU_PDC + PDC_PTCR) = PDC_PTCR_TXTDIS | PDC_PTCR_RXTDIS;	\
  DBGUREG(DBGU_BRGR) = ((mstclk) / 16 + (speed) / 2) / (speed);		\
  DBGUREG(DBGU_MR) = DBGU_MR_PAR_NONE;					\
  DBGUREG(DBGU_CR) = DBGU_CR_RSTSTA | DBGU_CR_RSTTX | DBGU_CR_RSTRX;	\
  DBGUREG(DBGU_CR) = DBGU_CR_TXEN | DBGU_CR_RXEN;			\
  (void)DBGUREG(DBGU_SR);						\
} while (/*CONSTCOND*/0)

#define	DBGU_PUTC(ch) do {						\
  int s = splserial();							\
  while ((DBGUREG(DBGU_SR) & DBGU_SR_TXRDY) == 0) {			\
    splx(s); s = splserial();						\
  };									\
  DBGUREG(DBGU_THR) = ch;						\
  splx(s);								\
} while (/*CONSTCOND*/0)

#define	DBGU_PEEKC() ((DBGUREG(DBGU_SR) & DBGU_SR_RXRDY) ? DBGUREG(DBGU_RHR) : -1)

#define	DBGU_PUTS(string) do {						\
  const char *_ptr = (string);						\
  while (*_ptr) {							\
    DBGU_PUTC(*_ptr);							\
    _ptr++;								\
  }									\
} while (/*CONSTCOND*/0)

#endif	// _AT91DBGUREG_H_

