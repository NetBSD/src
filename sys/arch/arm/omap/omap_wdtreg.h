/*
 * adapted/extracted from omap_wdt.c
 *
 * Copyright (c) 2007 Microsoft
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
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef  _OMAP_WDTREG_H
#define  _OMAP_WDTREG_H

#define PTV			1		/* prescaler ratio: clock is divided by 2 */
#define PRE			1		/* enable divided input clock */
#define TCLK		32768	/* timer clock period in normal mode */

#define WIDR		0x00	/* watchdog ID reg offset */
#define WD_REV		0xff	/* WIDR rev field mask */

#define WD_SYSCONFIG		0x10	/* WD System Configuration register */
#define WD_SYSCONFIG_AUTOIDLE	0x0	/* interface clock autogating */

#define WCLR		0x24	/* watchdog control register */
#define WCLR_PRE(PRE)           ((PRE & 0x1)<<5)
#define WCLR_PTV(PTV)           ((PTV & 0x7)<<2)

#define WCRR		0x28	/* watchdog counter register */
#define WLDR		0x2c	/* watchdog load register */
#define WTGR		0x30	/* watchdog trigger register */

#define WWPS		0x34	/* watchdog write pending register */
#define W_PEND_WSPR          (1<<4)
#define W_PEND_WTGR          (1<<3)
#define W_PEND_WLDR          (1<<2)
#define W_PEND_WCRR          (1<<1)
#define W_PEND_WCLR          (1<<0)

#define WSPR		0x48	/* watchdog start/stop register */
#define WD_ENABLE_WORD1         0x0000BBBB
#define WD_ENABLE_WORD2         0x00004444
#define WD_DISABLE_WORD1        0x0000AAAA
#define WD_DISABLE_WORD2        0x00005555


/* compute number of ticks corresponding to timeout seconds */
#define WATCHDOG_COUNT(timeout)	(~((timeout) * TCLK / (1<<PTV) -1))


#endif	/* _OMAP_WDTREG_H */
