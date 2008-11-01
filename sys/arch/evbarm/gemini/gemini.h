/*	$NetBSD: gemini.h,v 1.3 2008/11/01 07:43:19 cliff Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>
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

#ifndef _EVBARM_GEMINI_GEMINI_H
#define _EVBARM_GEMINI_GEMINI_H

#include <arm/gemini/gemini_reg.h>

/*
 * Kernel VM space: 192MB at KERNEL_VM_BASE
 */
#define	KERNEL_VM_BASE		((KERNEL_BASE + 0x01000000) & ~(0x400000-1))
#define KERNEL_VM_SIZE		0x0C000000

/*
 * We devmap IO starting at KERNEL_VM_BASE + KERNEL_VM_SIZE
 */
#define	GEMINI_KERNEL_IO_VBASE	(KERNEL_VM_BASE + KERNEL_VM_SIZE)
#define	GEMINI_GLOBAL_VBASE	GEMINI_KERNEL_IO_VBASE
#define	GEMINI_WATCHDOG_VBASE	(GEMINI_GLOBAL_VBASE   + L1_S_SIZE)
#define	GEMINI_CONSOLE_VBASE	(GEMINI_WATCHDOG_VBASE + L1_S_SIZE)
#define	GEMINI_TIMER_VBASE	(GEMINI_CONSOLE_VBASE  + L1_S_SIZE)


#endif /* _EVBARM_GEMINI_GEMINI_H */
