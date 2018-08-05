
#include <machine/asm.h>
#include "assym.h"

#if defined(__i386__)

ENTRY(breakpoint)
	pushl	%ebp
	movl	%esp, %ebp
	int	$0x03		/* paranoid, not 'int3' */
	popl	%ebp
	ret
END(breakpoint)

//#error TODO implement setjmp/longjmp for i386?

#elif defined(__amd64__)
ENTRY(breakpoint)
	.byte 0xcc	// BKPT_INST, int3
	ret

/*
 * int setjmp(label_t *)
 *
 * Used primarily by DDB.
 */
ENTRY(setjmp)
	/*
	 * Only save registers that must be preserved across function
	 * calls according to the ABI (%rbx, %rsp, %rbp, %r12-%r15)
	 * and %rip.
	 */
	movq	%rdi,%rax
	movq	%rbx,(%rax)
	movq	%rsp,8(%rax)
	movq	%rbp,16(%rax)
	movq	%r12,24(%rax)
	movq	%r13,32(%rax)
	movq	%r14,40(%rax)
	movq	%r15,48(%rax)
	movq	(%rsp),%rdx
	movq	%rdx,56(%rax)
	xorl	%eax,%eax
	ret
END(setjmp)

/*
 * int longjmp(label_t *)
 *
 * Used primarily by DDB.
 */
ENTRY(longjmp)
	movq	%rdi,%rax
	movq	(%rax),%rbx
	movq	8(%rax),%rsp
	movq	16(%rax),%rbp
	movq	24(%rax),%r12
	movq	32(%rax),%r13
	movq	40(%rax),%r14
	movq	48(%rax),%r15
	movq	56(%rax),%rdx
	movq	%rdx,(%rsp)
	movl	$1,%eax
	ret
END(longjmp)
#elif defined(__arm__)

ENTRY(breakpoint)
	BKPT_ASM
	mov pc, lr

#error implement setjmp/longjmp for arm32

#else

#error port me 

#endif

