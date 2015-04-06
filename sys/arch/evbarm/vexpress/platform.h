/*	$NetBSD: platform.h,v 1.2.2.2 2015/04/06 15:17:56 skrll Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Sergio L. Pascual.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_VEXPRESS_PLATFORM_H
#define _ARM_VEXPRESS_PLATFORM_H

/*
 * IO space
 */

#define VEXPRESS_CORE_VBASE	0xf0000000
#define VEXPRESS_CORE_PBASE	0x10000000
#define VEXPRESS_CORE_SIZE	0x10000000

/*
 * Kernel VM space 16Mb behind KERNEL_BASE upto 0xeff00000
 */
#define KERNEL_VM_BASE          0xc0000000
#define KERNEL_VM_SIZE          (VEXPRESS_CORE_VBASE - KERNEL_VM_BASE)

#define VEXPRESS_REF_FREQ	(24*1000*1000)	/* 24MHz */

#endif				/* _ARM_VEXPRESS_PLATFORM_H */
