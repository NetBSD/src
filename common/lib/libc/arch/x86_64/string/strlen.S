/*
 * Written by J.T. Conklin <jtc@acorntoolworks.com>
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: strlen.S,v 1.2 2009/07/11 08:48:51 dsl Exp $")
#endif

ENTRY(strlen)
	movq	%rdi,%rax
	negq	%rdi

.Lalign:
	/* Consider unrolling loop? */
	testb	$7,%al
	je	.Lword_aligned
	cmpb	$0,(%rax)
	jne	1f
	leaq	(%rdi,%rax),%rax
	ret
1:	incq	%rax
	jmp	.Lalign

	/*
	 * There are many well known branch-free sequences which are used
	 * for determining whether a zero-byte is contained within a word.
	 * These sequences are generally much more efficent than loading
	 * and comparing each byte individually.
	 *
	 * The expression [1,2]:
	 *
	 * (1)  ~(((x & 0x7f....7f) + 0x7f....7f) | (x | 0x7f....7f))
	 *
	 * evaluates to a non-zero value if any of the bytes in the
	 * original word is zero.
	 *
	 * It also has the useful property that bytes in the result word
	 * that correspond to non-zero bytes in the original word have
	 * the value 0x00, while bytes corresponding to zero bytes have
	 * the value 0x80. This allows calculation of the first (and
	 * last) occurrence of a zero byte within the word (useful for C's
	 * str* primitives) by counting the number of leading (or
	 * trailing) zeros and dividing the result by 8.  On machines
	 * without (or with slow) clz() / ctz() instructions, testing
	 * each byte in the result word for zero is necessary.
	 *
	 * This typically takes 4 instructions (5 on machines without
	 * "not-or") not including those needed to load the constant.
	 *
	 *
	 * The expression:
	 *
	 * (2)  ((x - 0x01....01) & ~x & 0x80....80)
	 *
	 * evaluates to a non-zero value if any of the bytes in the
	 * original word is zero.
	 *
	 * On little endian machines, the first byte in the result word
	 * that corresponds to a zero byte in the original byte is 0x80,
	 * so clz() can be used as above.  On big endian machines, and
	 * little endian machines without (or with a slow) clz() insn,
	 * testing each byte in the original for zero is necessary.
	 *
	 * This typically takes 3 instructions (4 on machines without
	 * "and with complement") not including those needed to load
	 * constants.
	 *
	 *
	 * The expression:
	 *
	 * (3)  ((x - 0x01....01) & 0x80....80)
	 *
	 * always evaluates to a non-zero value if any of the bytes in
	 * the original word is zero or has the top bit set.
	 * For strings that are likely to only contain 7-bit ascii these
	 * false positives will be rare.
	 *
	 * To account for possible false positives, each byte of the
	 * original word must be checked when the expression evaluates to
	 * a non-zero value.  However, because it is simpler than those
	 * presented above, code that uses it will be faster as long as
	 * the rate of false positives is low.
	 *
	 * This is likely, because the the false positive can only occur
	 * if the most siginificant bit of a byte within the word is set.
	 * The expression will never fail for typical 7-bit ASCII strings.
	 *
	 * This typically takes 2 instructions not including those needed
	 * to load constants.
	 *
	 *
	 * [1] Henry S. Warren Jr., "Hacker's Delight", Addison-Westley 2003
	 *
	 * [2] International Business Machines, "The PowerPC Compiler Writer's
	 *     Guide", Warthman Associates, 1996
	 */

	_ALIGN_TEXT
.Lword_aligned:
	movabsq	$0x0101010101010101,%r8
	movabsq	$0x8080808080808080,%r9
.Lloop:
	movq	(%rax),%rdx
	addq	$8,%rax
	subq	%r8,%rdx
	testq	%r9,%rdx
	je	.Lloop

	/*
	 * For bytes 0x80-0xff the above loop will exit prematurely. We must
	 * return to the loop if none of the bytes in the word equal 0.
	 */
	cmpb	$0,-8(%rax)		/* 1st byte == 0? */
	je	.Lsub8
	cmpb	$0,-7(%rax)		/* 2nd byte == 0? */
	je	.Lsub7
	cmpb	$0,-6(%rax)		/* 3rd byte == 0? */
	je	.Lsub6
	cmpb	$0,-5(%rax)		/* 4th byte == 0? */
	je	.Lsub5
	cmpb	$0,-4(%rax)		/* 5th byte == 0? */
	je	.Lsub4
	cmpb	$0,-3(%rax)		/* 6th byte == 0? */
	je	.Lsub3
	cmpb	$0,-2(%rax)		/* 7th byte == 0? */
	je	.Lsub2
	cmpb	$0,-1(%rax)		/* 8th byte == 0? */
	jne	.Lloop

.Lsub1:
	leaq	-1(%rdi,%rax),%rax
	ret
.Lsub2:
	leaq	-2(%rdi,%rax),%rax
	ret
.Lsub3:
	leaq	-3(%rdi,%rax),%rax
	ret
.Lsub4:
	leaq	-4(%rdi,%rax),%rax
	ret
.Lsub5:
	leaq	-5(%rdi,%rax),%rax
	ret
.Lsub6:
	leaq	-6(%rdi,%rax),%rax
	ret
.Lsub7:
	leaq	-7(%rdi,%rax),%rax
	ret
.Lsub8:
	leaq	-8(%rdi,%rax),%rax
	ret
