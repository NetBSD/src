/*	$Id: _ftol.c,v 1.1.1.1 2009/09/04 00:27:36 gmcgarry Exp $	*/

/*
 * _ftol() and _ftol2() implementations used on win32 when linking against
 * other code built with Visual Studio.
 */

asm(	"	.text\n"
	"	.globl __ftol\n"
	"	.globl __ftol2\n"
	"__ftol:\n"
	"__ftol2:\n"
	"	fnstcw -2(%esp)\n"
	"	movw -2(%esp),%ax\n"
	"	or %ax,0x0c00\n"
	"	movw %ax,-4(%esp)\n"
	"	fldcw -4(%esp)\n"
	"	fistpl -12(%esp)\n"
	"	fldcw -2(%esp)\n"
	"	movl -12(%esp),%eax\n"
	"	movl -8(%esp),%edx\n"
	"	ret\n"
);
