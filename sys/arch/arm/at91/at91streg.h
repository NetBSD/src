/*	$Id: at91streg.h,v 1.3.12.1 2013/01/16 05:32:45 yamt Exp $	*/
/*      $NetBSD: at91streg.h,v 1.3.12.1 2013/01/16 05:32:45 yamt Exp $	*/

/*-
 * Copyright (c) 2007 Embedtronics Oy
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
 */

#ifndef _AT91STREG_H_
#define _AT91STREG_H_

#define	AT91ST_BASE	0xFFFFFD00UL

/* System Timer (ST),
 * at91rm9200.pdf, page 289: */

#define	ST_CR		0x00UL	/* 0x00: Control Register (W/O)		*/
#define	ST_PIMR		0x04UL	/* 0x04: Period Interval Mode Reg	*/
#define	ST_WDMR		0x08UL	/* 0x08: Watchdog Mode Reg		*/
#define	ST_RTMR		0x0CUL	/* 0x0C: Real-time Mode Reg		*/
#define	ST_SR		0x10UL	/* 0x10: Status Register		*/
#define	ST_IER		0x14UL	/* 0x14: Interrupt Enable Register	*/
#define	ST_IDR		0x18UL	/* 0x18: Interrupt Disable Register	*/
#define	ST_IMR		0x1CUL	/* 0x1C: Interrupt Mask Register	*/
#define	ST_RTAR		0x20UL	/* 0x20: Real-time Alarm Register	*/
#define	ST_CRTR		0x24UL	/* 0x24: Current Real-time Register	*/

/* Control Register bits: */
#define	ST_CR_WDRST	0x1	/* 1=reload the start-up value in wdt	*/

/* Watchdog Mode Register bits: */
#define	ST_WDMR_EXTEN	0x20000	/* 1 = external signal assertion enable	*/
#define	ST_WDMR_RSTEN	0x10000	/* 1 = generate internal reset on wdt	*/
#define	ST_WDMR_WDV	0xFFFF

/* Status Register bits: */
#define	ST_SR_ALMS	0x8	/* 1 = alarm compare detected		*/
#define	ST_SR_RTTINC	0x4	/* 1 = real-time timer incremented	*/
#define	ST_SR_WDOVF	0x2	/* 1 = watchdog overflowed		*/
#define	ST_SR_PITS	0x1	/* 1 = period interval timer overflowed	*/

/* CRTR */
#define	ST_CRTR_CRTV	0xFFFFF
#define	ST_CRTR_CRTV_BITS 20

/* watchdog macros */
#define	WDT_TIMEOUT	20000	/* milliseconds				*/

//
#define	STREG(reg)	*((volatile uint32_t *)(AT91ST_BASE + (reg)))
#define	WDog()		do {CPUReg->ST.CR = ST_CR_WDRST;} while (0)

#endif /* _AT91STREG_H_ */
