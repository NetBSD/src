/*	$Id: _alloca.c,v 1.1.1.2 2009/09/04 00:27:36 gmcgarry Exp $	*/
/*
 * This explanation of _alloca() comes from Chris Giese, posted to
 * alt.os.dev:
 *
 * By default, Windows reserves 1 meg of virtual memory for the stack.
 * No page of stack memory is actually allocated (commited) until the
 * page is accessed.  This is demand-allocation.  The page beyond the
 * top of the stack is the guard page.  If this page is accessed,
 * memory will be allocated for it, and the guard page moved downward
 * by 4K (one page).  Thus, the stack can grow beyond the initial 1 meg.
 * Windows will not, however, let you grow the stack by accessing
 * discontiguous pages of memory.  Going beyond the guard page causes
 * an exception.  Stack-probing code prevents this.
 */

#ifndef __MSC__

asm(	"	.text\n"
	"	.globl __alloca\n"
	"__alloca:\n"
#ifdef __i386__
	"	pop %edx\n"
	"	pop %eax\n"
	"	add $3,%eax\n"
	"	and $-4,%eax\n"
	"1:	cmp $4096,%eax\n"
	"	jge 2f\n"
	"	sub %eax,%esp\n"
	"	test %eax,(%esp)\n"
	"	mov %esp,%eax\n"
	"	push %edx\n"
	"	push %edx\n"
	"	ret\n"
	"2:	sub $4096,%esp\n"
	"	sub $4096,%eax\n"
	"	test %eax,(%esp)\n"
	"	jmp 1b\n"
#endif
);

#endif
