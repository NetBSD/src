/*
 * Written by J.T. Conklin <jtc@acorntoolworks.com>
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: memchr.S,v 1.3 2009/07/19 23:45:29 christos Exp $")
#endif

ENTRY(memchr)
	movzbq	%sil,%rcx

	/*
	 * Align to word boundary.
	 * Consider unrolling loop?
	 */
	testq	%rdx,%rdx	/* nbytes == 0? */
	je	.Lzero
.Lalign:
	testb	$7,%dil
	je	.Lword_aligned
	movq	%rdi,%rax
	cmpb	(%rdi),%cl
	je	.Ldone
	incq	%rdi
	decq	%rdx
	jnz	.Lalign
	jmp	.Lzero

.Lword_aligned:
	/* copy char to all bytes in word */
	movb	%cl,%ch
	movq	%rcx,%rsi
	salq	$16,%rcx
	orq	%rsi,%rcx
	movq	%rcx,%rsi
	salq	$32,%rcx
	orq	%rsi,%rcx

	movabsq	$0x0101010101010101,%r8
	movabsq	$0x8080808080808080,%r9

	_ALIGN_TEXT
.Lloop:
	cmpq	$7,%rdx		/* nbytes > 8 */
	jbe	.Lbyte
	movq	(%rdi),%rsi
	addq	$8,%rdi
	xorq	%rcx,%rsi
	subq	$8,%rdx
	subq	%r8,%rsi
	testq	%r9,%rsi
	je	.Lloop

	/*
	 * In rare cases, the above loop may exit prematurely. We must
	 * return to the loop if none of the bytes in the word are
	 * equal to ch.
	 */

	leaq	-8(%rdi),%rax
	cmpb	-8(%rdi),%cl	/* 1st byte == ch? */
	je	.Ldone

	leaq	-7(%rdi),%rax
	cmpb	-7(%rdi),%cl	/* 2nd byte == ch? */
	je	.Ldone

	leaq	-6(%rdi),%rax
	cmpb	-6(%rdi),%cl	/* 3rd byte == ch? */
	je	.Ldone

	leaq	-5(%rdi),%rax
	cmpb	-5(%rdi),%cl	/* 4th byte == ch? */
	je	.Ldone

	leaq	-4(%rdi),%rax
	cmpb	-4(%rdi),%cl	/* 5th byte == ch? */
	je	.Ldone

	leaq	-3(%rdi),%rax
	cmpb	-3(%rdi),%cl	/* 6th byte == ch? */
	je	.Ldone

	leaq	-2(%rdi),%rax
	cmpb	-2(%rdi),%cl	/* 7th byte == ch? */
	je	.Ldone

	leaq	-1(%rdi),%rax
	cmpb	-1(%rdi),%cl	/* 7th byte == ch? */
	jne	.Lloop
	ret

.Lbyte:
	testq	%rdx,%rdx
	je	.Lzero
.Lbyte_loop:
	movq	%rdi,%rax
	cmpb	(%rdi),%cl
	je	.Ldone
	incq	%rdi
	decq	%rdx
	jnz	.Lbyte_loop

.Lzero:
	xorq	%rax,%rax

.Ldone:
	ret
