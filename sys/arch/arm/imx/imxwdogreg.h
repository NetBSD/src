/*-
 * Copyright (c) 2010  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Watchdog register definitions for Freescale i.MX31 and i.MX51
 *
 *	MCIMX31 and MCIMX31L Application Processors
 *	Reference Manual
 *	MCIMC31RM
 *	Rev. 2.3
 *	1/2007
 *
 *	MCIMX51 Multimedia Applications Processor
 *      Reference Manual
 *      MCIMX51RM
 *      Rev. 1
 *      2/2010
 */

#ifndef _ARM_IMX_IMXWDOGREG_H
#define _ARM_IMX_IMXWDOGREG_H

#define	IMX_WDOG_WCR	0x0000	/* Watchdog Control Register */
#define	 WCR_WDZST	__BIT(0)	/* watchdog low power */
#define	 WCR_WDBG	__BIT(1)	/* watchdog debug enable */
#define	 WCR_WDE	__BIT(2)	/* watchdog enable */
#define	 WCR_WDT	__BIT(3)	/* timeout assertion */
#define	 WCR_SRS	__BIT(4)	/* software reset signal */
#define	 WCR_WDA	__BIT(5)	/* ipp_wdog* assertion */
#define	 WCR_WDW	__BIT(7)	/* disable for wait */
#define	 WCR_WT		__BITS(15, 8)
					/* watchdog timeout
					   0=0.5sec 0xff=128sec */

#define	IMX_WDOG_WSR	0x0002	/* Watchdog Service Register */
#define	 WSR_MAGIC1	0x5555	/* 1st word of service sequence */
#define	 WSR_MAGIC2	0xaaaa	/* 2nd word of service sequence */

#define	IMX_WDOG_WRSR	0x0004	/* Watchdog Reset Status Register */
#define	 WRSR_SFTW	__BIT(0)	/* reset is the result of a
					 * software reset */
#define	 WRSR_TOUT	__BIT(1)	/* reset is the result of a
					 * WDOG timeout */
/* only for i.MX31 */
#define	 WRSR_CMON	__BIT(2)
#define	 WRSR_EXT	__BIT(3)
#define	 WRSR_PWR	__BIT(4)
#define	 WRSR_JRST	__BIT(5)

/* only for i.MX51 */
#define	IMX_WDOG_WICR	0x0006	/* Watchdog Interrupt Control Register */
#define	 WICR_WICT	__BITS(7,0)	/* interrupt count timeout */
#define	 WICR_WTIS	__BIT(14)	/* interrupt status [w1c] */
#define	 WICR_WIE	__BIT(15)	/* interrupt enable */

/* only for i.MX51 */
#define	IMX_WDOG_WMCR	0x0008
#define	 WMCR_PDE	__BIT(0)	/* power down enable */

#endif /* _ARM_IMX_IMXWDOGREG_H */
