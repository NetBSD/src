/*	$NetBSD: darwin_commpage_machdep.S,v 1.5.56.2 2007/12/27 00:43:05 mjf Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: darwin_commpage_machdep.S,v 1.5.56.2 2007/12/27 00:43:05 mjf Exp $");

#include <compat/darwin/darwin_syscall.h>

	.globl	_C_LABEL(darwin_commpage_bzero)
_C_LABEL(darwin_commpage_bzero):
	movl	8(%esp),%ecx
	xor	%eax,%eax
	cmpl	$0x0f,%ecx		/* avoid mispredicted branch... */

	pushl	%edi
	movl	8(%esp),%edi

	cld				/* set fill direction forward */

	/*
	 * if the string is too short, it's really not worth the overhead
	 * of aligning to word boundries, etc.  So we jump to a plain
	 * unaligned set.
	 *
	 * NB aligning the transfer is actually pointless on my athlon 700,
	 * It does make a difference to a PII though.
	 *
	 * The PII, PIII and PIV all seem to have a massive performance
	 * drop when the initial target address is an odd multiple of 4.
	 */
	jbe	by_bytes

	movl	%edi,%edx		/* detect misalignment */
	neg	%edx
	andl	$7,%edx
	jnz	align
aligned:
	movl	%eax,-4(%edi,%ecx)	/* zap last 4 bytes */
	shrl	$2,%ecx			/* zero by words */
	rep
	stosl
zdone:
	pop	%edi
	ret

align:
	movl	%eax,(%edi)		/* zap first 8 bytes */
	movl	%eax,4(%edi)
	subl	%edx,%ecx		/* remove from main count */
	add	%edx,%edi
	jmp	aligned

by_bytes:
	rep
	stosb

	popl	%edi
	ret
	.globl	_C_LABEL(darwin_commpage_bzero_size)
_C_LABEL(darwin_commpage_bzero_size) = .-_C_LABEL(darwin_commpage_bzero)


	.globl	_C_LABEL(darwin_commpage_pthread_self)
_C_LABEL(darwin_commpage_pthread_self):
	ret
	.globl	_C_LABEL(darwin_commpage_pthread_self_size)
_C_LABEL(darwin_commpage_pthread_self_size) = .-_C_LABEL(darwin_commpage_pthread_self)

	.globl	_C_LABEL(darwin_commpage_gettimeofday)
_C_LABEL(darwin_commpage_gettimeofday):
	ret
	.globl	_C_LABEL(darwin_commpage_gettimeofday_size)
_C_LABEL(darwin_commpage_gettimeofday_size) = .-_C_LABEL(darwin_commpage_gettimeofday)
	
	.globl	_C_LABEL(darwin_commpage_bigcopy)
_C_LABEL(darwin_commpage_bigcopy):
	.globl	_C_LABEL(darwin_commpage_memcpy)
_C_LABEL(darwin_commpage_memcpy):
	push	%esi
	mov	%edi,%edx
	movl	8(%esp),%edi
	movl	12(%esp),%esi
	movl	16(%esp),%ecx
	movl	%ecx,%eax
	cld			/* nope, copy forwards. */
	shrl	$2,%ecx		/* copy by words */
	rep
	movsl
	and	$3,%eax		/* any bytes left? */
	jnz	mtrailing
mdone:
	movl	8(%esp),%eax
	mov	%edx,%edi
	pop	%esi
	ret

mtrailing:
	cmp	$2,%eax
	jb	1f
	movw	(%esi),%ax
	movw	%ax,(%edi)
	je	mdone
	movb	2(%esi),%al
	movb	%al,2(%edi)
	jmp	mdone
1:	movb	(%esi),%al
	movb	%al,(%edi)
	jmp	mdone

	.globl	_C_LABEL(darwin_commpage_memcpy_size)
_C_LABEL(darwin_commpage_memcpy_size) = .-_C_LABEL(darwin_commpage_memcpy)
	.globl	_C_LABEL(darwin_commpage_bigcopy_size)
_C_LABEL(darwin_commpage_bigcopy_size) = .-_C_LABEL(darwin_commpage_bigcopy)

	.globl	_C_LABEL(darwin_commpage_pthread_specific)
_C_LABEL(darwin_commpage_pthread_specific):
	ret
	.globl	_C_LABEL(darwin_commpage_pthread_specific_size)
_C_LABEL(darwin_commpage_pthread_specific_size) = .-_C_LABEL(darwin_commpage_pthread_specific)

	.globl	_C_LABEL(darwin_commpage_mach_absolute_time)
_C_LABEL(darwin_commpage_mach_absolute_time):
	ret
	.globl	_C_LABEL(darwin_commpage_mach_absolute_time_size)
_C_LABEL(darwin_commpage_mach_absolute_time_size) = .-_C_LABEL(darwin_commpage_mach_absolute_time)

	.globl	_C_LABEL(darwin_commpage_spinlock_try)
_C_LABEL(darwin_commpage_spinlock_try):
	ret
	.globl	_C_LABEL(darwin_commpage_spinlock_try_size)
_C_LABEL(darwin_commpage_spinlock_try_size) = .-_C_LABEL(darwin_commpage_spinlock_try)

	.globl	_C_LABEL(darwin_commpage_spinlock_lock)
_C_LABEL(darwin_commpage_spinlock_lock):
	ret
	.globl	_C_LABEL(darwin_commpage_spinlock_lock_size)
_C_LABEL(darwin_commpage_spinlock_lock_size) = .-_C_LABEL(darwin_commpage_spinlock_lock)

	.globl	_C_LABEL(darwin_commpage_spinlock_unlock)
_C_LABEL(darwin_commpage_spinlock_unlock):
	ret
	.globl	_C_LABEL(darwin_commpage_spinlock_unlock_size)
_C_LABEL(darwin_commpage_spinlock_unlock_size) = .-_C_LABEL(darwin_commpage_spinlock_unlock)

	.globl	_C_LABEL(darwin_commpage_sys_dcache_flush)
_C_LABEL(darwin_commpage_sys_dcache_flush):
	ret
	.globl	_C_LABEL(darwin_commpage_sys_dcache_flush_size)
_C_LABEL(darwin_commpage_sys_dcache_flush_size) = .-_C_LABEL(darwin_commpage_sys_dcache_flush)

	.globl	_C_LABEL(darwin_commpage_sys_icache_invalidate)
_C_LABEL(darwin_commpage_sys_icache_invalidate):
	ret
	.globl	_C_LABEL(darwin_commpage_sys_icache_invalidate_size)
_C_LABEL(darwin_commpage_sys_icache_invalidate_size) = .-_C_LABEL(darwin_commpage_sys_icache_invalidate)

	.globl	_C_LABEL(darwin_commpage_spinlock_relinquish)
_C_LABEL(darwin_commpage_spinlock_relinquish):
	ret
	.globl	_C_LABEL(darwin_commpage_spinlock_relinquish_size)
_C_LABEL(darwin_commpage_spinlock_relinquish_size) = .-_C_LABEL(darwin_commpage_spinlock_relinquish)

	.globl	_C_LABEL(darwin_commpage_pthread_getspecific)
_C_LABEL(darwin_commpage_pthread_getspecific):
	ret
	.globl	_C_LABEL(darwin_commpage_pthread_getspecific_size)
_C_LABEL(darwin_commpage_pthread_getspecific_size) = .-_C_LABEL(darwin_commpage_pthread_getspecific)

	.globl	_C_LABEL(darwin_commpage_bcopy)
_C_LABEL(darwin_commpage_bcopy):
	push	%esi
	mov	%edi,%edx
	movl	8(%esp),%esi
	movl	12(%esp),%edi
	movl	16(%esp),%ecx
	movl	%edi,%eax
	subl	%esi,%eax
	cmpl	%ecx,%eax	/* overlapping? */
	movl	%ecx,%eax
	jb	backwards
	shrl	$2,%ecx		/* copy by words */
	rep
	movsl
	and	$3,%eax		/* any bytes left? */
	jnz	trailing
done:
	mov	%edx,%edi
	pop	%esi
	ret

trailing:
	cmp	$2,%eax
	jb	1f
	movw	(%esi),%ax
	movw	%ax,(%edi)
	je	done
	movb	2(%esi),%al
	movb	%al,2(%edi)
	jmp	done
1:	movb	(%esi),%al
	movb	%al,(%edi)
	jmp	done

backwards:
	addl	%ecx,%edi	/* copy backwards. */
	addl	%ecx,%esi
	and	$3,%eax		/* any fractional bytes? */
	jnz	back_align
back_aligned:
	shrl	$2,%ecx
	subl	$4,%esi
	subl	$4,%edi
	std
	rep
	movsl
	cld
	jmp	done

back_align:
	sub	%eax,%esi
	sub	%eax,%edi
	cmp	$2,%eax
	jb	1f
	je	2f
	movb	2(%esi),%al
	movb	%al,2(%edi)
2:	movw	(%esi),%ax
	movw	%ax,(%edi)
	jmp	back_aligned
1:	movb	(%esi),%al
	movb	%al,(%edi)
	jmp	back_aligned
	.globl	_C_LABEL(darwin_commpage_bcopy_size)
_C_LABEL(darwin_commpage_bcopy_size) = .-_C_LABEL(darwin_commpage_bcopy)
