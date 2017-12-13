/* $NetBSD: platform.h,v 1.1.2.1 2017/12/13 01:22:35 matt Exp $ */
/*-
 * Copyright (c) 2017 Mediatek Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _EVBARM_MERCURY_PLATFORM_H
#define _EVBARM_MERCURY_PLATFORM_H

#include <arm/mediatek/mercury_reg.h>

#define KERNEL_VM_BASE			0xc0000000  //(0xc0000000 + 0x10000000)
#define KERNEL_VM_SIZE			0x30000000  //(0x20000000 - 0x10000000)

#define MERCURY_CBAR    0x10310000
/*
 * We devmap IO starting at KERNEL_VM_BASE + KERNEL_VM_SIZE
 */
#define MTK_KERNEL_IO_VBASE	    (KERNEL_VM_BASE + KERNEL_VM_SIZE)
#define MTK_KERNEL_IO_VEND	    (MTK_KERNEL_IO_VBASE + MTK_IO_SIZE)
#ifndef _LOCORE
CTASSERT(MTK_KERNEL_IO_VEND <= VM_MAX_KERNEL_ADDRESS);
#endif
#endif /* _EVBARM_MERCURY_PLATFORM_H */
