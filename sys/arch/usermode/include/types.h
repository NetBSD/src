/* $NetBSD: types.h,v 1.8.6.1 2017/12/03 11:36:47 jdolecek Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _USERMODE_TYPES_H_
#define _USERMODE_TYPES_H_

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <machine/int_types.h>

#if defined(_KERNEL)
typedef struct label_t {
	long val[8];
} label_t;
#endif

#if defined(_KERNEL) || defined(_KMEMUSER) || defined(_KERNTYPES) || defined(_STANDALONE)
typedef unsigned long	paddr_t;
typedef unsigned long	psize_t;
typedef unsigned long	vaddr_t;
typedef unsigned long	vsize_t;
typedef long int	register_t;
#define	PRIxPADDR	"lx"
#define	PRIxPSIZE	"lx"
#define	PRIuPSIZE	"lu"
#define	PRIxVADDR	"lx"
#define	PRIxVSIZE	"lx"
#define	PRIuVSIZE	"lu"
#define	PRIxREGISTER	"lx"
#endif

typedef unsigned char	__cpu_simple_lock_nv_t;
typedef long int	__register_t;

#define __CPU_SIMPLE_LOCK_PAD

#define __SIMPLELOCK_LOCKED	1
#define __SIMPLELOCK_UNLOCKED	0

#if defined(__i386__)
#define __HAVE_OLD_DISKLABEL
#endif

#define __HAVE_CPU_DATA_FIRST
#define __HAVE_MM_MD_KERNACC
#define	__HAVE_COMPAT_NETBSD32

#endif /* !_USERMODE_TYPES_H_ */
