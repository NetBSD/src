/*	$NetBSD: platform.h,v 1.2.12.2 2014/08/20 00:02:53 tls Exp $	*/
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

#ifndef _EVBARM_CUBIE_PLATFORM_H
#define _EVBARM_CUBIE_PLATFORM_H

#include <arm/allwinner/awin_reg.h>

/*
 * Memory may be mapped VA:PA starting at 0x80000000:0x40000000
 * Kernel VM space: 576MB at KERNEL_VM_BASE
 */
#define KERNEL_VM_BASE		0xc0000000
#define KERNEL_VM_SIZE		0x24000000

/*
 * We devmap IO starting at KERNEL_VM_BASE + KERNEL_VM_SIZE
 */
#define AWIN_KERNEL_IO_VBASE	(KERNEL_VM_BASE + KERNEL_VM_SIZE)
#define AWIN_CORE_VBASE		AWIN_KERNEL_IO_VBASE
#define AWIN_SRAM_VBASE		(AWIN_CORE_VBASE + AWIN_CORE_SIZE)
#define AWIN_KERNEL_IO_VEND	(AWIN_SRAM_VBASE + AWIN_SRAM_SIZE)
#define CONADDR_VA		((CONADDR - AWIN_CORE_PBASE) + AWIN_CORE_VBASE)
#ifndef _LOCORE
CTASSERT(AWIN_KERNEL_IO_VEND <= VM_MAX_KERNEL_ADDRESS);
#endif
#endif /* _EVBARM_CUBIE_PLATFORM_H */
