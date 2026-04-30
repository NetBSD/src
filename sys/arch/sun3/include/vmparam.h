/*	$NetBSD: vmparam.h,v 1.43 2026/04/30 15:10:13 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#ifndef _SUN3_VMPARAM_H_
#define _SUN3_VMPARAM_H_ 1

#ifdef	_SUN3_
#include <machine/vmparam3.h>
#endif	/* SUN3 */
#ifdef	_SUN3X_
#include <machine/vmparam3x.h>
#endif	/* SUN3X */

/* default for modules etc. */
#if !defined(_SUN3_) && !defined(_SUN3X_)
#include <machine/vmparam3.h>
#endif

/*
 * Kernel and user-space share the same virtual space on Sun3
 */
#define VM_MAX_ADDRESS		kernbase
#define VM_MIN_KERNEL_ADDRESS	kernbase
#define VM_MAX_KERNEL_ADDRESS	kern_end

/*
 * Size of phys_map, used for mapping user I/O buffer into kernel
 * space for physio.
 *
 * The actual limitation for physio requests will be the DVMA space,
 * and that is fixed by hardware design at 1MB.  We could make the
 * physio map larger than that, but it would not buy us much.
 */
#define	VM_PHYS_SIZE		(1 * 1024 * 1024)

/* This is needed by some LKMs. */
#define VM_PHYSSEG_MAX		4
#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH

#include <m68k/vmparam.h>

#endif	/* _SUN3_VMPARAM_H_ */
