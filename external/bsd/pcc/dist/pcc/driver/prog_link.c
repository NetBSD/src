/*	Id	*/	
/*	$NetBSD: prog_link.c,v 1.1.1.1 2016/02/09 20:29:13 plunky Exp $	*/

/*-
 * Copyright (c) 2014 Iain Hibbert.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "driver.h"

static int
link_exec(const char *infile, const char *outfile)
{
	list_t *l;
	char *prog;

	l = list_alloc();

	/* startfiles */
 	if (target->crtbegin) list_add(l, "crtbegin.o");
 	if (target->crtbeginS) list_add(l, "crtbeginS.o");
 	if (target->crtbeginT) list_add(l, "crtbeginT.o");
 	if (target->crt0) list_add(l, "crt0.o");
 	if (target->crt1) list_add(l, "crt1.o");
 	if (target->crt2) list_add(l, "crt2.o");
 	if (target->crti) list_add(l, "crti.o");
 	if (target->dllcrt2) list_add(l, "dllcrt2.o");
 	if (target->gcrt0) list_add(l, "gcrt0.o");
 	if (target->gcrt1) list_add(l, "gcrt1.o");

	/* endfiles */
	if (target->crtend) list_add(l, "crtend.o");
	if (target->crtendS) list_add(l, "crtendS.o");
	if (target->crtn) list_add(l, "crtn.o");

	/* early link */
	&use.always,	"-dynamic-linker"
	&os.netbsd,	"/libexec/ld.elf.so"
	&os.linux,	"/lib64/ld-linux-x86-64.so.2"
	&use.always,	"-m"
	&mach.i386,	"elf_386"
	&mach.amd64,	"elf_x86_64"

	/* sysincdir */
	"=/usr/include"
	"=/usr/lib/gcc/x86_64-linux-gnu/4.4/include"

	/* crtdir */
	"=/usr/lib/i386", "=/usr/lib"
	"=/usr/lib"
	"=/usr/lib64", "=/usr/lib/gcc/x86_64-linux-gnu/4.4"

	/* stdlib */
	"-L/usr/lib/gcc/x86_64-linux-gnu/4.4"
	"-lgcc", "--as-needed", "-lgcc_s", "--no-as-needed", "-lc", "-lgcc", "--as-needed", "-lgcc_s", "--no-as-needed"
}
