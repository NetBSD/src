/*	$NetBSD: darwin_commpage_machdep.S,v 1.3 2004/07/16 04:56:30 christos Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <machine/asm.h>

#include <compat/darwin/darwin_syscall.h>

	.globl	_C_LABEL(darwin_commpage_bzero)
_C_LABEL(darwin_commpage_bzero):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_bzero_size)
_C_LABEL(darwin_commpage_bzero_size) = .-_C_LABEL(darwin_commpage_bzero)


	.globl	_C_LABEL(darwin_commpage_pthread_self)
_C_LABEL(darwin_commpage_pthread_self):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_pthread_self_size)
_C_LABEL(darwin_commpage_pthread_self_size) = .-_C_LABEL(darwin_commpage_pthread_self)

	.globl	_C_LABEL(darwin_commpage_gettimeofday)
_C_LABEL(darwin_commpage_gettimeofday):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_gettimeofday_size)
_C_LABEL(darwin_commpage_gettimeofday_size) = .-_C_LABEL(darwin_commpage_gettimeofday)
	
	.globl	_C_LABEL(darwin_commpage_bigcopy)
_C_LABEL(darwin_commpage_bigcopy):
	.globl	_C_LABEL(darwin_commpage_memcpy)
_C_LABEL(darwin_commpage_memcpy):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_memcpy_size)
_C_LABEL(darwin_commpage_memcpy_size) = .-_C_LABEL(darwin_commpage_memcpy)
	.globl	_C_LABEL(darwin_commpage_bigcopy_size)
_C_LABEL(darwin_commpage_bigcopy_size) = .-_C_LABEL(darwin_commpage_bigcopy)

	.globl	_C_LABEL(darwin_commpage_pthread_specific)
_C_LABEL(darwin_commpage_pthread_specific):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_pthread_specific_size)
_C_LABEL(darwin_commpage_pthread_specific_size) = .-_C_LABEL(darwin_commpage_pthread_specific)

	.globl	_C_LABEL(darwin_commpage_mach_absolute_time)
_C_LABEL(darwin_commpage_mach_absolute_time):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_mach_absolute_time_size)
_C_LABEL(darwin_commpage_mach_absolute_time_size) = .-_C_LABEL(darwin_commpage_mach_absolute_time)

	.globl	_C_LABEL(darwin_commpage_spinlock_try)
_C_LABEL(darwin_commpage_spinlock_try):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_spinlock_try_size)
_C_LABEL(darwin_commpage_spinlock_try_size) = .-_C_LABEL(darwin_commpage_spinlock_try)

	.globl	_C_LABEL(darwin_commpage_spinlock_lock)
_C_LABEL(darwin_commpage_spinlock_lock):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_spinlock_lock_size)
_C_LABEL(darwin_commpage_spinlock_lock_size) = .-_C_LABEL(darwin_commpage_spinlock_lock)

	.globl	_C_LABEL(darwin_commpage_spinlock_unlock)
_C_LABEL(darwin_commpage_spinlock_unlock):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_spinlock_unlock_size)
_C_LABEL(darwin_commpage_spinlock_unlock_size) = .-_C_LABEL(darwin_commpage_spinlock_unlock)

	.globl	_C_LABEL(darwin_commpage_sys_dcache_flush)
_C_LABEL(darwin_commpage_sys_dcache_flush):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_sys_dcache_flush_size)
_C_LABEL(darwin_commpage_sys_dcache_flush_size) = .-_C_LABEL(darwin_commpage_sys_dcache_flush)

	.globl	_C_LABEL(darwin_commpage_sys_icache_invalidate)
_C_LABEL(darwin_commpage_sys_icache_invalidate):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_sys_icache_invalidate_size)
_C_LABEL(darwin_commpage_sys_icache_invalidate_size) = .-_C_LABEL(darwin_commpage_sys_icache_invalidate)

	.globl	_C_LABEL(darwin_commpage_spinlock_relinquish)
_C_LABEL(darwin_commpage_spinlock_relinquish):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_spinlock_relinquish_size)
_C_LABEL(darwin_commpage_spinlock_relinquish_size) = .-_C_LABEL(darwin_commpage_spinlock_relinquish)

	.globl	_C_LABEL(darwin_commpage_pthread_getspecific)
_C_LABEL(darwin_commpage_pthread_getspecific):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_pthread_getspecific_size)
_C_LABEL(darwin_commpage_pthread_getspecific_size) = .-_C_LABEL(darwin_commpage_pthread_getspecific)

	.globl	_C_LABEL(darwin_commpage_bcopy)
_C_LABEL(darwin_commpage_bcopy):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_bcopy_size)
_C_LABEL(darwin_commpage_bcopy_size) = .-_C_LABEL(darwin_commpage_bcopy)
