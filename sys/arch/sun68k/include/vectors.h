/*	$NetBSD: vectors.h,v 1.1 2024/01/13 18:51:38 thorpej Exp $	*/

/*-
 * Copyright (c) 1996, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross, and by Jason R. Thorpe.
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

#ifndef _SUN68K_VECTORS_H_
#define	_SUN68K_VECTORS_H_

#ifdef _KERNEL

#include <m68k/vectors.h>

#define	MACHINE_RESET_SP	(void *)0xfffe000
#define	MACHINE_RESET_PC	(void *)0xfef0000

#define	MACHINE_BUSERR_HANDLER	buserr
#define	MACHINE_ADDRERR_HANDLER	addrerr

#define	MACHINE_AV0_HANDLER	_isr_autovec
#define	MACHINE_AV1_HANDLER	_isr_autovec
#define	MACHINE_AV2_HANDLER	_isr_autovec
#define	MACHINE_AV3_HANDLER	_isr_autovec
#define	MACHINE_AV4_HANDLER	_isr_autovec
#define	MACHINE_AV5_HANDLER	_isr_autovec
#define	MACHINE_AV6_HANDLER	_isr_autovec
#define	MACHINE_AV7_HANDLER	_isr_autovec

#endif /* _KERNEL */

#endif /* _SUN68K_VECTORS_H_ */
