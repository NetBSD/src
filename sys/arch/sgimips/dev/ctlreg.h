/*	$NetBSD: ctlreg.h,v 1.1.6.2 2009/05/13 17:18:18 jym Exp $	*/

/*
 * Copyright (c) 2009 Stephen M. Rumble
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARCH_SGIMIPS_DEV_CTLREG_H_
#define	_ARCH_SGIMIPS_DEV_CTLREG_H_

#define CTL_MEMCFG		0x00000000	/* Memory configuration (8) */

#define CTL_SYSID		0x00000001	/* System ID (8) */
#define CTL_SYSID_FPU		0x02		/* FPU present */

#define CTL_CPUCTRL		0x00080002	/* Misc. config bits (16) */
#define CTL_CPUCTRL_SYSRESET	0x0200		/* VME SYSRESET: reset system */
#define CTL_CPUCTRL_PARITY	0x0400		/* Enable parity checking */
#define CTL_CPUCTRL_SLAVE	0x0800		/* Enable slave accesses */
#define CTL_CPUCTRL_WDOG	0x4000		/* Watchdog enable */

#define CTL_LAN_PAR_CLR		0x002a0000	/* Clear LAN parity error (8) */
#define CTL_DMA_PAR_CLR		0x002a0001	/* Clear DMA parity error (8) */
#define CTL_CPU_PAR_CLR		0x002a0002	/* Clear CPU parity error (8) */
#define CTL_VME_PAR_CLR		0x002a0003	/* Clear VME parity error (8) */

#define CTL_AUX_CPUCTRL		0x000e0000	/* Aux. cpu control reg (8) */
#define CTL_AUX_CPUCTRL_CONSLED	0x10		/* Enable console led */
#define CTL_AUX_CPUCTRL_HRTBEAT 0x01		/* Heartbeat */

#endif	/* _ARCH_SGIMIPS_DEV_CTLREG_H_ */
