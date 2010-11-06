/*	$NetBSD: vmparam.h,v 1.36 2010/11/06 15:42:49 uebayasi Exp $	*/

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

/*
 * We use 8K pages on both the sun3 and sun3x.  Override PAGE_*
 * to be compile-time constants.
 */
#define	PAGE_SHIFT	13
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

#define	USRSTACK	kernbase	/* for modules */
#define	USRSTACK3	KERNBASE3	/* for asm not in modules */
#define	USRSTACK3X	KERNBASE3X

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
 * PTEs for mapping user space into the kernel for phyio operations.
 * The actual limitation for physio requests will be the DVMA space,
 * and that is fixed by hardware design at 1MB.  We could make the
 * physio map larger than that, but it would not buy us much.
 */
#ifndef USRIOSIZE
#define USRIOSIZE	128		/* 1 MB */
#endif

/* This is needed by some LKMs. */
#define VM_PHYSSEG_MAX		4

/*
 * Mach-derived constants:
 */

/* user/kernel map constants */
#define VM_MIN_ADDRESS		((vaddr_t)0)
#define VM_MAX_ADDRESS		kernbase
#define VM_MAXUSER_ADDRESS	kernbase
#define VM_MIN_KERNEL_ADDRESS	kernbase
#define VM_MAX_KERNEL_ADDRESS	kern_end

/* virtual sizes (bytes) for various kernel submaps */
#define VM_PHYS_SIZE		(USRIOSIZE*PAGE_SIZE)

#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#endif	/* _SUN3_VMPARAM_H_ */
