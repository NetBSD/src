/*	$NetBSD: copy.S,v 1.2.34.1 2007/07/11 19:59:57 mjf Exp $	*/
/*	NetBSD: locore.S,v 1.34 2005/04/01 11:59:31 yamt Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)locore.s	7.3 (Berkeley) 5/13/91
 */

#include "opt_cputype.h"
#include "assym.h"

#include <sys/errno.h>

#include <machine/asm.h>
#include <machine/frameasm.h>
#include <machine/cputypes.h>

#define GET_CURPCB(reg)	\
	movl	CPUVAR(CURLWP), reg; \
	movl	L_ADDR(reg), reg

/*
 * The following primitives are used to fill and copy regions of memory.
 */

/*
 * XXX No section 9 man page for fillw.
 * fillw seems to be very sparsely used (only in pccons it seems.)
 * One wonders if it couldn't be done without.
 * -- Perry Metzger, May 7, 2001
 */
/*
 * void fillw(short pattern, void *addr, size_t len);
 * Write len copies of pattern at addr.
 */
/* LINTSTUB: Func: void fillw(short pattern, void *addr, size_t len) */
ENTRY(fillw)
	pushl	%edi
	movl	8(%esp),%eax
	movl	12(%esp),%edi
	movw	%ax,%cx
	rorl	$16,%eax
	movw	%cx,%ax
	cld
	movl	16(%esp),%ecx
	shrl	%ecx			# do longwords
	rep
	stosl
	movl	16(%esp),%ecx
	andl	$1,%ecx			# do remainder
	rep
	stosw
	popl	%edi
	ret

/*
 * int kcopy(const void *from, void *to, size_t len);
 * Copy len bytes, abort on fault.
 */
/* LINTSTUB: Func: int kcopy(const void *from, void *to, size_t len) */
ENTRY(kcopy)
	pushl	%esi
	pushl	%edi
	GET_CURPCB(%eax)		# load curpcb into eax and set on-fault
	pushl	PCB_ONFAULT(%eax)
	movl	$_C_LABEL(kcopy_fault), PCB_ONFAULT(%eax)

	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%ecx
	movl	%edi,%eax
	subl	%esi,%eax
	cmpl	%ecx,%eax		# overlapping?
	jb	1f
	cld				# nope, copy forward
	shrl	$2,%ecx			# copy by 32-bit words
	rep
	movsl
	movl	24(%esp),%ecx
	andl	$3,%ecx			# any bytes left?
	rep
	movsb

	GET_CURPCB(%edx)		# XXX save curpcb?
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret

	ALIGN_TEXT
1:	addl	%ecx,%edi		# copy backward
	addl	%ecx,%esi
	std
	andl	$3,%ecx			# any fractional bytes?
	decl	%edi
	decl	%esi
	rep
	movsb
	movl	24(%esp),%ecx		# copy remainder by 32-bit words
	shrl	$2,%ecx
	subl	$3,%esi
	subl	$3,%edi
	rep
	movsl
	cld

	GET_CURPCB(%edx)
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret

/*****************************************************************************/

/*
 * The following primitives are used to copy data in and out of the user's
 * address space.
 */

/*
 * Default to the lowest-common-denominator.  We will improve it
 * later.
 */
#if defined(I386_CPU)
#define	DEFAULT_COPYOUT		_C_LABEL(i386_copyout)
#define	DEFAULT_COPYIN		_C_LABEL(i386_copyin)
#elif defined(I486_CPU)
#define	DEFAULT_COPYOUT		_C_LABEL(i486_copyout)
#define	DEFAULT_COPYIN		_C_LABEL(i386_copyin)
#elif defined(I586_CPU)
#define	DEFAULT_COPYOUT		_C_LABEL(i486_copyout)	/* XXX */
#define	DEFAULT_COPYIN		_C_LABEL(i386_copyin)	/* XXX */
#elif defined(I686_CPU)
#define	DEFAULT_COPYOUT		_C_LABEL(i486_copyout)	/* XXX */
#define	DEFAULT_COPYIN		_C_LABEL(i386_copyin)	/* XXX */
#endif

	.data

	.globl	_C_LABEL(copyout_func)
_C_LABEL(copyout_func):
	.long	DEFAULT_COPYOUT

	.globl	_C_LABEL(copyin_func)
_C_LABEL(copyin_func):
	.long	DEFAULT_COPYIN

	.text

/*
 * int copyout(const void *from, void *to, size_t len);
 * Copy len bytes into the user's address space.
 * see copyout(9)
 */
/* LINTSTUB: Func: int copyout(const void *kaddr, void *uaddr, size_t len) */
ENTRY(copyout)
	DO_DEFERRED_SWITCH(%eax)
	jmp	*_C_LABEL(copyout_func)

#if defined(I386_CPU)
/* LINTSTUB: Func: int i386_copyout(const void *kaddr, void *uaddr, size_t len) */
ENTRY(i386_copyout)
	pushl	%esi
	pushl	%edi
	pushl	$0

	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%eax

	/*
	 * We check that the end of the destination buffer is not past the end
	 * of the user's address space.  If it's not, then we only need to
	 * check that each page is writable.  The 486 will do this for us; the
	 * 386 will not.  (We assume that pages in user space that are not
	 * writable by the user are not writable by the kernel either.)
	 */
	movl	%edi,%edx
	addl	%eax,%edx
	jc	_C_LABEL(copy_efault)
	cmpl	$VM_MAXUSER_ADDRESS,%edx
	ja	_C_LABEL(copy_efault)

	testl	%eax,%eax		# anything to do?
	jz	3f

	/*
	 * We have to check each PTE for (write) permission, since the CPU
	 * doesn't do it for us.
	 */

	/* Compute number of pages. */
	movl	%edi,%ecx
	andl	$PGOFSET,%ecx
	addl	%eax,%ecx
	decl	%ecx
	shrl	$PGSHIFT,%ecx

	/* Compute PTE offset for start address. */
	shrl	$PGSHIFT,%edi

	GET_CURPCB(%edx)
	movl	$2f,PCB_ONFAULT(%edx)

1:	/* Check PTE for each page. */
	testb	$PG_RW,PTE_BASE(,%edi,4)
	jz	2f

4:	incl	%edi
	decl	%ecx
	jns	1b

	movl	20(%esp),%edi
	movl	24(%esp),%eax
	jmp	3f

2:	/* Simulate a trap. */
	pushl	%ecx
	movl	%edi,%eax
	shll	$PGSHIFT,%eax
	pushl	%eax
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# pop argument
	popl	%ecx
	testl	%eax,%eax		# if not ok, return EFAULT
	jz	4b
	jmp	_C_LABEL(copy_efault)

3:	GET_CURPCB(%edx)
	movl	$_C_LABEL(copy_fault),PCB_ONFAULT(%edx)

	/* bcopy(%esi, %edi, %eax); */
	cld
	movl	%eax,%ecx
	shrl	$2,%ecx
	rep
	movsl
	movl	%eax,%ecx
	andl	$3,%ecx
	rep
	movsb

	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret
#endif /* I386_CPU */

#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
/* LINTSTUB: Func: int i486_copyout(const void *kaddr, void *uaddr, size_t len) */
ENTRY(i486_copyout)
	pushl	%esi
	pushl	%edi
	pushl	$0

	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%eax

	/*
	 * We check that the end of the destination buffer is not past the end
	 * of the user's address space.
	 */
	movl	%edi,%edx
	addl	%eax,%edx
	jc	_C_LABEL(copy_efault)
	cmpl	$VM_MAXUSER_ADDRESS,%edx
	ja	_C_LABEL(copy_efault)

	GET_CURPCB(%edx)
	movl	$_C_LABEL(copy_fault),PCB_ONFAULT(%edx)

	/* bcopy(%esi, %edi, %eax); */
	cld
	movl	%eax,%ecx
	shrl	$2,%ecx
	rep
	movsl
	movl	%eax,%ecx
	andl	$3,%ecx
	rep
	movsb

	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret
#endif /* I486_CPU || I586_CPU || I686_CPU */

/*
 * int copyin(const void *from, void *to, size_t len);
 * Copy len bytes from the user's address space.
 * see copyin(9)
 */
/* LINTSTUB: Func: int copyin(const void *uaddr, void *kaddr, size_t len) */
ENTRY(copyin)
	DO_DEFERRED_SWITCH(%eax)
	jmp	*_C_LABEL(copyin_func)

#if defined(I386_CPU) || defined(I486_CPU) || defined(I586_CPU) || \
    defined(I686_CPU)
/* LINTSTUB: Func: int i386_copyin(const void *uaddr, void *kaddr, size_t len) */
ENTRY(i386_copyin)
	pushl	%esi
	pushl	%edi
	GET_CURPCB(%eax)
	pushl	$0
	movl	$_C_LABEL(copy_fault),PCB_ONFAULT(%eax)

	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%eax

	/*
	 * We check that the end of the destination buffer is not past the end
	 * of the user's address space.  If it's not, then we only need to
	 * check that each page is readable, and the CPU will do that for us.
	 */
	movl	%esi,%edx
	addl	%eax,%edx
	jc	_C_LABEL(copy_efault)
	cmpl	$VM_MAXUSER_ADDRESS,%edx
	ja	_C_LABEL(copy_efault)

	/* bcopy(%esi, %edi, %eax); */
	cld
	movl	%eax,%ecx
	shrl	$2,%ecx
	rep
	movsl
	movl	%eax,%ecx
	andl	$3,%ecx
	rep
	movsb

	GET_CURPCB(%edx)
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret
#endif /* I386_CPU || I486_CPU || I586_CPU || I686_CPU */

/* LINTSTUB: Ignore */
NENTRY(copy_efault)
	movl	$EFAULT,%eax

/*
 * kcopy_fault is used by kcopy and copy_fault is used by copyin/out.
 *
 * they're distinguished for lazy pmap switching.  see trap().
 */
/* LINTSTUB: Ignore */
NENTRY(kcopy_fault)
	GET_CURPCB(%edx)
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	ret

/* LINTSTUB: Ignore */
NENTRY(copy_fault)
	GET_CURPCB(%edx)
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	ret

/*
 * int copyoutstr(const void *from, void *to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 * see copyoutstr(9)
 */
/* LINTSTUB: Func: int copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done) */
ENTRY(copyoutstr)
	pushl	%esi
	pushl	%edi

	DO_DEFERRED_SWITCH(%eax)

	movl	12(%esp),%esi		# esi = from
	movl	16(%esp),%edi		# edi = to
	movl	20(%esp),%edx		# edx = maxlen

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	5f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	/* Compute number of bytes in first page. */
	movl	%edi,%eax
	andl	$PGOFSET,%eax
	movl	$PAGE_SIZE,%ecx
	subl	%eax,%ecx		# ecx = PAGE_SIZE - (src % PAGE_SIZE)

	GET_CURPCB(%eax)
	movl	$6f,PCB_ONFAULT(%eax)

1:	/*
	 * Once per page, check that we are still within the bounds of user
	 * space, and check for a write fault.
	 */
	cmpl	$VM_MAXUSER_ADDRESS,%edi
	jae	_C_LABEL(copystr_efault)

	/* Compute PTE offset. */
	movl	%edi,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address

	testb	$PG_RW,PTE_BASE(,%eax,4)
	jnz	2f

6:	/* Simulate a trap. */
	pushl	%edx
	pushl	%edi
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# clear argument from stack
	popl	%edx
	testl	%eax,%eax
	jnz	_C_LABEL(copystr_efault)

2:	/* Copy up to end of this page. */
	subl	%ecx,%edx		# predecrement total count
	jnc	3f
	addl	%edx,%ecx		# ecx += (edx - ecx) = edx
	xorl	%edx,%edx

3:	decl	%ecx
	js	4f
	lodsb
	stosb
	testb	%al,%al
	jnz	3b

	/* Success -- 0 byte reached. */
	addl	%ecx,%edx		# add back residual for this page
	xorl	%eax,%eax
	jmp	copystr_return

4:	/* Go to next page, if any. */
	movl	$PAGE_SIZE,%ecx
	testl	%edx,%edx
	jnz	1b

	/* edx is zero -- return ENAMETOOLONG. */
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return
#endif /* I386_CPU */

#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
5:	GET_CURPCB(%eax)
	movl	$_C_LABEL(copystr_fault),PCB_ONFAULT(%eax)
	/*
	 * Get min(%edx, VM_MAXUSER_ADDRESS-%edi).
	 */
	movl	$VM_MAXUSER_ADDRESS,%eax
	subl	%edi,%eax
	jc	_C_LABEL(copystr_efault)
	cmpl	%edx,%eax
	jae	1f
	movl	%eax,%edx
	movl	%eax,20(%esp)

1:	incl	%edx
	cld

1:	decl	%edx
	jz	2f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b

	/* Success -- 0 byte reached. */
	decl	%edx
	xorl	%eax,%eax
	jmp	copystr_return

2:	/* edx is zero -- return EFAULT or ENAMETOOLONG. */
	cmpl	$VM_MAXUSER_ADDRESS,%edi
	jae	_C_LABEL(copystr_efault)
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return
#endif /* I486_CPU || I586_CPU || I686_CPU */

/*
 * int copyinstr(const void *from, void *to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 * see copyinstr(9)
 */
/* LINTSTUB: Func: int copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done) */
ENTRY(copyinstr)
	pushl	%esi
	pushl	%edi

	DO_DEFERRED_SWITCH(%eax)

	GET_CURPCB(%ecx)
	movl	$_C_LABEL(copystr_fault),PCB_ONFAULT(%ecx)

	movl	12(%esp),%esi		# %esi = from
	movl	16(%esp),%edi		# %edi = to
	movl	20(%esp),%edx		# %edx = maxlen

	/*
	 * Get min(%edx, VM_MAXUSER_ADDRESS-%esi).
	 */
	movl	$VM_MAXUSER_ADDRESS,%eax
	subl	%esi,%eax
	jc	_C_LABEL(copystr_efault)
	cmpl	%edx,%eax
	jae	1f
	movl	%eax,%edx
	movl	%eax,20(%esp)

1:	incl	%edx
	cld

1:	decl	%edx
	jz	2f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b

	/* Success -- 0 byte reached. */
	decl	%edx
	xorl	%eax,%eax
	jmp	copystr_return

2:	/* edx is zero -- return EFAULT or ENAMETOOLONG. */
	cmpl	$VM_MAXUSER_ADDRESS,%esi
	jae	_C_LABEL(copystr_efault)
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return

/* LINTSTUB: Ignore */
NENTRY(copystr_efault)
	movl	$EFAULT,%eax

/* LINTSTUB: Ignore */
NENTRY(copystr_fault)
copystr_return:
	/* Set *lencopied and return %eax. */
	GET_CURPCB(%ecx)
	movl	$0,PCB_ONFAULT(%ecx)
	movl	20(%esp),%ecx
	subl	%edx,%ecx
	movl	24(%esp),%edx
	testl	%edx,%edx
	jz	8f
	movl	%ecx,(%edx)

8:	popl	%edi
	popl	%esi
	ret

/*
 * int copystr(const void *from, void *to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NUL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 * see copystr(9)
 */
/* LINTSTUB: Func: int copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done) */
ENTRY(copystr)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%esi		# esi = from
	movl	16(%esp),%edi		# edi = to
	movl	20(%esp),%edx		# edx = maxlen
	incl	%edx
	cld

1:	decl	%edx
	jz	4f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b

	/* Success -- 0 byte reached. */
	decl	%edx
	xorl	%eax,%eax
	jmp	6f

4:	/* edx is zero -- return ENAMETOOLONG. */
	movl	$ENAMETOOLONG,%eax

6:	/* Set *lencopied and return %eax. */
	movl	20(%esp),%ecx
	subl	%edx,%ecx
	movl	24(%esp),%edx
	testl	%edx,%edx
	jz	7f
	movl	%ecx,(%edx)

7:	popl	%edi
	popl	%esi
	ret

/*
 * long fuword(const void *uaddr);
 * Fetch an int from the user's address space.
 * see fuword(9)
 */
/* LINTSTUB: Func: long fuword(const void *base) */
ENTRY(fuword)
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-4,%edx
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%ecx)
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)
	movl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret

/*
 * int fusword(const void *uaddr);
 * Fetch a short from the user's address space.
 * see fusword(9)
 */
/* LINTSTUB: Func: int fusword(const void *base) */
ENTRY(fusword)
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%ecx)
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)
	movzwl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret

/*
 * int fuswintr(const void *uaddr);
 * Fetch a short from the user's address space.  Can be called during an
 * interrupt.
 * see fuswintr(9)
 */
/* LINTSTUB: Func: int fuswintr(const void *base) */
ENTRY(fuswintr)
	cmpl	$TLBSTATE_VALID, CPUVAR(TLBSTATE)
	jnz	_C_LABEL(fusuaddrfault)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)
	movl	CPUVAR(CURLWP),%ecx
	movl	L_ADDR(%ecx),%ecx
	movl	$_C_LABEL(fusubail),PCB_ONFAULT(%ecx)
	movzwl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret

/*
 * int fubyte(const void *uaddr);
 * Fetch a byte from the user's address space.
 * see fubyte(9)
 */
/* LINTSTUB: Func: int fubyte(const void *base) */
ENTRY(fubyte)
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-1,%edx
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%ecx)
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)
	movzbl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret

/*
 * Handle faults from [fs]u*().  Clean up and return -1.
 */
/* LINTSTUB: Ignore */
NENTRY(fusufault)
	movl	$0,PCB_ONFAULT(%ecx)
	movl	$-1,%eax
	ret

/*
 * Handle faults from [fs]u*().  Clean up and return -1.  This differs from
 * fusufault() in that trap() will recognize it and return immediately rather
 * than trying to page fault.
 */
/* LINTSTUB: Ignore */
NENTRY(fusubail)
	movl	$0,PCB_ONFAULT(%ecx)
	movl	$-1,%eax
	ret

/*
 * Handle earlier faults from [fs]u*(), due to our of range addresses.
 */
/* LINTSTUB: Ignore */
NENTRY(fusuaddrfault)
	movl	$-1,%eax
	ret

/*
 * int suword(void *uaddr, long x);
 * Store an int in the user's address space.
 * see suword(9)
 */
/* LINTSTUB: Func: int suword(void *base, long c) */
ENTRY(suword)
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-4,%edx
	ja	_C_LABEL(fusuaddrfault)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	GET_CURPCB(%eax)
	movl	$3f,PCB_ONFAULT(%eax)

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,PTE_BASE(,%eax,4)
	jnz	1f

3:	/* Simulate a trap. */
	pushl	%edx
	pushl	%edx
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# clear parameter from the stack
	popl	%edx
	GET_CURPCB(%ecx)
	testl	%eax,%eax
	jnz	_C_LABEL(fusufault)

1:	/* XXX also need to check the following 3 bytes for validity! */
#endif

2:	GET_CURPCB(%ecx)
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)

	movl	8(%esp),%eax
	movl	%eax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	ret

/*
 * int susword(void *uaddr, short x);
 * Store a short in the user's address space.
 * see susword(9)
 */
/* LINTSTUB: Func: int susword(void *base, short c) */
ENTRY(susword)
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	GET_CURPCB(%eax)
	movl	$3f,PCB_ONFAULT(%eax)

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,PTE_BASE(,%eax,4)
	jnz	1f

3:	/* Simulate a trap. */
	pushl	%edx
	pushl	%edx
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# clear parameter from the stack
	popl	%edx
	GET_CURPCB(%ecx)
	testl	%eax,%eax
	jnz	_C_LABEL(fusufault)

1:	/* XXX also need to check the following byte for validity! */
#endif

2:	GET_CURPCB(%ecx)
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)

	movl	8(%esp),%eax
	movw	%ax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	ret

/*
 * int suswintr(void *uaddr, short x);
 * Store a short in the user's address space.  Can be called during an
 * interrupt.
 * see suswintr(9)
 */
/* LINTSTUB: Func: int suswintr(void *base, short c) */
ENTRY(suswintr)
	cmpl	$TLBSTATE_VALID, CPUVAR(TLBSTATE)
	jnz	_C_LABEL(fusuaddrfault)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)
	movl	CPUVAR(CURLWP),%ecx
	movl	L_ADDR(%ecx),%ecx
	movl	$_C_LABEL(fusubail),PCB_ONFAULT(%ecx)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,PTE_BASE(,%eax,4)
	jnz	1f

	/* Simulate a trap. */
	jmp	_C_LABEL(fusubail)

1:	/* XXX also need to check the following byte for validity! */
#endif

2:	movl	8(%esp),%eax
	movw	%ax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	ret

/*
 * int subyte(void *uaddr, char x);
 * Store a byte in the user's address space.
 * see subyte(9)
 */
/* LINTSTUB: Func: int subyte(void *base, int c) */
ENTRY(subyte)
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-1,%edx
	ja	_C_LABEL(fusuaddrfault)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	GET_CURPCB(%eax)
	movl	$3f,PCB_ONFAULT(%eax)

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,PTE_BASE(,%eax,4)
	jnz	1f

3:	/* Simulate a trap. */
	pushl	%edx
	pushl	%edx
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# clear parameter from the stack
	popl	%edx
	GET_CURPCB(%ecx)
	testl	%eax,%eax
	jnz	_C_LABEL(fusufault)

1:
#endif

2:	GET_CURPCB(%ecx)
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)

	movb	8(%esp),%al
	movb	%al,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	ret
