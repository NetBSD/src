/* $NetBSD: types.h,v 1.1 2014/08/10 05:47:38 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef	_AARCH64_TYPES_H_
#define _AARCH64_TYPES_H_

#ifdef __aarch64__

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <aarch64/int_types.h>

/* NB: This should probably be if defined(_KERNEL) */
#if defined(_NETBSD_SOURCE)
typedef	unsigned long	vm_offset_t;	/* depreciated */
typedef	unsigned long	vm_size_t;	/* depreciated */

typedef unsigned long	paddr_t;
typedef unsigned long	psize_t;
typedef unsigned long	vaddr_t;
typedef unsigned long	vsize_t;
#define PRIxPADDR	"lx"
#define PRIxPSIZE	"lx"
#define PRIuPSIZE	"lu"
#define PRIxVADDR	"lx"
#define PRIxVSIZE	"lx"
#define PRIuVSIZE	"lu"
#endif

typedef unsigned long long int register_t;
typedef unsigned int register32_t;
#define PRIxREGISTER	"llx"
#define PRIxREGISTER32	"lx"

typedef unsigned long	pmc_evid_t;
#define PMC_INVALID_EVID	(-1)
typedef unsigned long	pmc_ctr_t;
typedef unsigned short	tlb_asid_t;

#if defined(_KERNEL)
typedef struct label_t {	/* Used by setjmp & longjmp */
        register_t lb_reg[13];	/* x19 .. x30, sp */
} label_t;
#endif
         
/*
 * This should have always been an 8-bit type.
 */
typedef	volatile unsigned char	__cpu_simple_lock_t;

#define __SIMPLELOCK_LOCKED	1
#define __SIMPLELOCK_UNLOCKED	0

#define __HAVE_FAST_SOFTINTS
#define __HAVE_MM_MD_DIRECT_MAPPED_PHYS
#define __HAVE_CPU_COUNTER
#define __HAVE_SYSCALL_INTERN
#define __HAVE_NEW_STYLE_BUS_H
#define __HAVE_MINIMAL_EMUL
#define __HAVE_CPU_DATA_FIRST
#define __HAVE___LWP_GETPRIVATE_FAST
#define __HAVE_COMMON___TLS_GET_ADDR
#define __HAVE_TLS_VARIANT_I

#if defined(_KERNEL) || defined(_KMEMUSER)
#define PCU_FPU			0
#define PCU_UNIT_COUNT		1
#endif

#if defined(_KERNEL)
#define __HAVE_RAS
#endif

#elif defined(__arm__)

#include <arm/types.h>

#endif

#endif	/* _AARCH64_TYPES_H_ */
