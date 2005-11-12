/*	$NetBSD: epwdogreg.h,v 1.1 2005/11/12 05:33:23 hamajima Exp $	*/

/*
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*	Cirrus Logic EP9315
	Watchdog Timer register
	http://www.cirrus.com/jp/pubs/manual/EP9315_Users_Guide.pdf	*/

#ifndef	_EPWDOGREG_H_
#define	_EPWDOGREG_H_

#define	EP93XX_WDOG_Ctrl	0x00	/* Control Register */
		/* read */
#define	 EP93XX_WDOG_PLSDSN	(1<<6)
#define	 EP93XX_WDOG_OVRID	(1<<5)
#define	 EP93XX_WDOG_SWDIS	(1<<4)
#define	 EP93XX_WDOG_HWDIS	(1<<3)
#define	 EP93XX_WDOG_URST	(1<<2)
#define	 EP93XX_WDOG_K3RST	(1<<1)
#define	 EP93XX_WDOG_WD		(1<<0)
		/* write */
#define	 EP93XX_WDOG_RESTART	0x5555
#define	 EP93XX_WDOG_DISABLE	0xaa55
#define	 EP93XX_WDOG_ENABLE	0xaaaa

#define	EP93XX_WDOG_Status	0x04	/* Status Register (R/W) */
#define	 EP93XX_WDOG_STAT_MASK	0x0000007f

#endif	/* _EPWDOGREG_H_ */
