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
	"	mr r26,r1\n"
	"	subi r1,r1,4\n"
	"	clrrwi r1,r1,5\n"
	"	li r0,0\n"
	"	stw r0,0(r1)\n"
	"	stwu r1,-64(r1)\n"
	"	lwz r3,0(r26)\n"
	"	addi r4,r26,4\n"
	"	addi r27,r3,1\n"
	"	slwi r27,r27,2\n"
	"	add r5,r4,r27\n"
	"	bl __start\n"
	"	trap\n"
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
	"	.p2align 2\n"
	"	.private_extern __dyld_func_lookup\n"
	"__dyld_func_lookup:\n"
	"	lis r11,ha16(Ldyld_func_lookup)\n"
	"	lwz r11,lo16(Ldyld_func_lookup)(r11)\n"
	"	mtctr r11\n"
	"	bctr\n"
);

/*
 *  void dyld_stub_binding_helper(void)
 */
asm(
	"	.text\n"
	"	.p2align 2\n"
	"	.private_extern dyld_stub_binding_helper\n"
	"dyld_stub_binding_helper:\n"
	"	lis r12,ha16(Ldyld_lazy_binder)\n"
	"	lwz r12,lo16(Ldyld_lazy_binder)(r12)\n"
	"	mtctr r12\n"
	"	lis r12,ha16(__mh_execute_header)\n"
	"	addi r12,r12,lo16(__mh_execute_header)\n"
	"	bctr\n"
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
