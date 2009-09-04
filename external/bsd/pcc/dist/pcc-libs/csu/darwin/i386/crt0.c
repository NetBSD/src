/*	$Id: crt0.c,v 1.1.1.2 2009/09/04 00:27:35 gmcgarry Exp $	*/
/*-
 * Copyright (c) 2008 Gregory McGarry <g.mcgarry@ieee.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "common.h"

void _start(int, char **, char **);

char **environ;
char *__progname = "";
int NXArgc;
char **NXArgv;

asm(
#ifdef DYNAMIC
	"	.text\n"
	"	.symbol_stub\n"
	"	.picsymbol_stub\n"
#endif
	"	.text\n"
	"	.globl start\n"
	"	.globl _start\n"
	"	.p2align 2\n"
	"start:\n"
	"_start:\n"
	"	pushl $0\n"
	"	movl %esp,%ebp\n"
	"	subl $16,%esp\n"
	"	andl $-16,%esp\n"
	"	movl 4(%ebp),%ebx\n"
	"	movl %ebx,(%esp)\n"
	"	leal 8(%ebp),%ecx\n"
	"	movl %ecx,4(%esp)\n"
	"	addl $1,%ebx\n"
	"	shll $2,%ebx\n"
	"	addl %ecx,%ebx\n"
	"	movl %ebx,8(%esp)\n"
	"	call __start\n"
	"	hlt\n"
);


void
_start(int argc, char *argv[], char *envp[])
{
	char *namep;

	environ = envp;
	NXArgc = argc;
	NXArgv = argv;

	if ((namep = argv[0]) != NULL) {
		if ((__progname = _strrchr(namep, '/')) == NULL)
			__progname = namep;
		else
			__progname++;
	}

	/*
	 * Initialise hooks inside libc
	 */
	if (mach_init_routine)
		(*mach_init_routine)();
	if (_cthread_init_routine)
		(*_cthread_init_routine)();

#ifdef PROFILE
	atexit(_mcleanup);
	moninit();
#endif

	atexit(_fini);
	_init();

	exit(main(argc, argv, environ));
}


#ifdef DYNAMIC

/*
 *  dylib constructors/destructors
 */
asm(
	"	.constructor\n"
	"	.p2align 2\n"
	"	.long __dyld_init\n"
	"	.destructor\n"
	"	.p2align 2\n"
	"	.long __dyld_fini\n"
);

/*
 *  void _dyld_func_lookup(const char *, void **);
 *
 *  jump to the linker via the pointer in the __dyld section
 */
asm(
	"	.text\n"
	"	.private_extern __dyld_func_lookup\n"
	"__dyld_func_lookup:\n"
	"	jmp *Ldyld_func_lookup\n"
);

/*
 *  void dyld_stub_binding_helper(void)
 */
asm(
	"	.text\n"
	"	.private_extern dyld_stub_binding_helper\n"
	"dyld_stub_binding_helper:\n"
	"	pushl $__mh_execute_header\n"
	"	jmp *Ldyld_lazy_binder\n"
);

/*
 * __dyld section
 */
asm(
	"	.dyld\n"
	"	.p2align 2\n"
	"Ldyld_lazy_binder:\n"
	"	.long 0x8fe01000\n"
	"Ldyld_func_lookup:\n"
	"	.long 0x8fe01008\n"
	"	.long __mh_execute_header\n"
	"	.long _NXArgc\n"
	"	.long _NXArgv\n"
	"	.long _environ\n"
	"	.long ___progname\n"
);

#endif

asm("\t.subsections_via_symbols\n");

#include "common.c"

IDENT("$Id: crt0.c,v 1.1.1.2 2009/09/04 00:27:35 gmcgarry Exp $");
