/*	$NetBSD: beagle.h,v 1.2.18.1 2010/08/17 06:44:14 uebayasi Exp $	*/
/*
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

#ifndef _EVBARM_BEAGLE_BEAGLE_H
#define _EVBARM_BEAGLE_BEAGLE_H

#include <arm/omap/omap2_reg.h>

/*
 * Kernel VM space: 576MB at KERNEL_VM_BASE
 */
#define	KERNEL_VM_BASE		((KERNEL_BASE + 0x01000000) & ~(0x400000-1))
#define KERNEL_VM_SIZE		0x24000000

/*
 * We devmap IO starting at KERNEL_VM_BASE + KERNEL_VM_SIZE
 */
#define	OMAP3530_KERNEL_IO_VBASE	(KERNEL_VM_BASE + KERNEL_VM_SIZE)
#define	OMAP3530_L4_CORE_VBASE		OMAP3530_KERNEL_IO_VBASE
#define	OMAP3530_L4_PERIPHERAL_VBASE	(OMAP3530_L4_CORE_VBASE   + OMAP3530_L4_CORE_SIZE)
#define	OMAP3530_L4_WAKEUP_VBASE	(OMAP3530_L4_PERIPHERAL_VBASE + OMAP3530_L4_PERIPHERAL_SIZE)
#define	OMAP3530_KERNEL_IO_VEND		(OMAP3530_L4_WAKEUP_VBASE + OMAP3530_L4_WAKEUP_SIZE)

#define CONSADDR_VA	((CONSADDR - OMAP3530_L4_PERIPHERAL_BASE) + OMAP3530_L4_PERIPHERAL_VBASE)


#endif /* _EVBARM_BEAGLE_BEAGLE_H */
