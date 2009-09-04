/*	$Id: dylib1.c,v 1.1.1.1 2009/09/04 00:27:35 gmcgarry Exp $	*/
/*-
 * Copyright (c) 2009 Gregory McGarry <g.mcgarry@ieee.org>
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

/*
 *  void _dyld_func_lookup(const char *, void **);
 *
 *  jump to the linker via the pointer in the __dyld section
 */
asm(
	"	.text\n"
	"	.private_extern __dyld_func_lookup\n"
	"__dyld_func_lookup:\n"
	"	call L__dyld_func_lookup$pb\n"
	"L__dyld_func_lookup$pb:\n"
	"	popl %eax\n"
	"	movl Ldyld_func_lookup-L__dyld_func_lookup$pb(%eax),%eax\n"
	"	jmp *%eax\n"
);

/*
 *  void dyld_stub_binding_helper(void)
 */
asm(
	"	.data\n"
	"	.p2align 2\n"
	"dyld__mach_header:\n"
	"	.long __mh_dylib_header\n"
	"	.text\n"
	"	.private_extern dyld_stub_binding_helper\n"
	"	.p2align 2\n"
	"dyld_stub_binding_helper:\n"
	"	call Ldyld_stub_binding_helper$pb\n"
	"Ldyld_stub_binding_helper$pb:\n"
	"	popl %eax\n"
	"	pushl dyld__mach_header-Ldyld_stub_binding_helper$pb(%eax)\n"
	"	movl Ldyld_lazy_binder-Ldyld_stub_binding_helper$pb(%eax),%eax\n"
	"	jmp *%eax\n"
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
);

asm("\t.subsections_via_symbols\n");

IDENT("$Id: dylib1.c,v 1.1.1.1 2009/09/04 00:27:35 gmcgarry Exp $");
