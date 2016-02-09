/*	Id	*/	
/*	$NetBSD: prog_asm.c,v 1.1.1.1 2016/02/09 20:29:12 plunky Exp $	*/

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
asm_exec(const char *infile, const char *outfile)
{
	list_t *l;
	const char *file;
	int rv;

	if (opt.prog_asm == NULL)
		error("No assembler is defined");

	l = list_alloc();

	list_add_file(l, opt.prog_asm, &progdirs, X_OK);

	Wflag_add(l, W_ASM);

	list_add_list(l, opt.Wa);

	{
		-p gnu
		-f <type>
#if defined(USE_YASM)
		av[na++] = "-p";
		av[na++] = "gnu";

		av[na++] = "-f";
#if defined(os_win32)
		av[na++] = "win32";
#elif defined(os_darwin)
		av[na++] = "macho";
#else
		av[na++] = "elf";
#endif
#endif

#if defined(os_sunos) && defined(mach_sparc64)
		av[na++] = "-m64";
#endif

#if defined(os_darwin)
		if (Bstatic)
			av[na++] = "-static";
#endif

#if !defined(USE_YASM)
		if (vflag)
			av[na++] = "-v";
#endif
		if (kflag)
			av[na++] = "-k";

#ifdef os_darwin
		av[na++] = "-arch";
#if mach_amd64
		av[na++] = amd64_i386 ? "i386" : "x86_64";
#else
		av[na++] = "i386";
#endif
#else
#ifdef mach_amd64
		if (amd64_i386)
			av[na++] = "--32";
#endif
#endif

		av[na++] = "-o";
		if (outfile && cflag)
			ermfile = av[na++] = outfile;
		else if (cflag)
			ermfile = av[na++] = olist[i] = setsuf(clist[i], 'o');
		else
			ermfile = av[na++] = olist[i] = gettmp();
		av[na++] = assource;
		av[na++] = 0;

		if (callsys(as, av)) {
			cflag++;
			eflag++;
			cunlink(tmp4);
			continue;
		}
	}

	if (infile != NULL)
		list_add(l, infile);
	else if (outfile != NULL)
		list_add(l, "-");

	if (outfile != NULL) {
		list_add(l, "-o");
		list_add(l, outfile);
	}

	rv = list_exec(l);
	list_free(l);
	return rv;
}
