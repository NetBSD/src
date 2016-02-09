/*	Id	*/	
/*	$NetBSD: prog_cpp.c,v 1.1.1.1 2016/02/09 20:29:13 plunky Exp $	*/

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

static void JUNK(void)
{
	// __LITTLE_ENDIAN__
	// __BIG_ENDIAN__

	/* Machine specific macros */
	switch (targmach) {
	case mach_amd64:
		list_add(l, "-D__amd64__");
		list_add(l, "-D__amd64");
		list_add(l, "-D__x86_64__");
		list_add(l, "-D__x86_64");
		list_add(l, "-D__LP64__");
		list_add(l, "-D_LP64");
		break;

	case mach_arm:
		list_add(l, "-D__arm__");
		break;

	case mach_i386:
		list_add(l, "-D__i386__");
		list_add(l, "-D__i386");
		break;

	case mach_nova:
		list_add(l, "-D__nova__");
		break;

	case mach_m16c:
		list_add(l, "-D__m16c__");
		break;

	case mach_mips:
		list_add(l, "-D__mips__");
		break;

	case mach_pdp10:
		list_add(l, "-D__pdp10__");
		break;

	case mach_pdp11:
		list_add(l, "-D__pdp11__");
		break;

	case mach_powerpc:
		list_add(l, "-D__powerpc__");
		list_add(l, "-D__ppc__");
		break;

	case mach_sparc:
		list_add(l, "-D__sparc__");
		list_add(l, "-D__sparc");
		break;

	case mach_sparc64:
		list_add(l, "-D__sparc64__");
		list_add(l, "-D__sparc_v9__");
		list_add(l, "-D__sparcv9");
		list_add(l, "-D__sparc__");
		list_add(l, "-D__sparc");
		list_add(l, "-D__LP64__");
		list_add(l, "-D_LP64");
		break;

	case mach_vax:
		list_add(l, "-D__vax__");
		break;
	}
}

static void
cpp_add_stddef(struct list *l)
{

	/* STDC */
	list_add(l, "-D__STDC__=1");
	list_add(l, "-D__STDC_HOSTED__=%d", opt.hosted ? 1 : 0);
	list_add(l, "-D__STDC_VERSION_=199901L");
	list_add(l, "-D__STDC_ISO_10646__=200009L");

	/* PCC specific */
	list_add(l, "-D__PCC__=%d", PCC_MAJOR);
	list_add(l, "-D__PCC_MINOR__=%d", PCC_MINOR);
	list_add(l, "-D__PCC_MINORMINOR__=%d", PCC_MINORMINOR);
	list_add(l, "-D__VERSION__=\"%s\"", VERSSTR);

	/* GNU C compat */
	if (opt.gnu89 || opt.gnu99) {
		list_add(l, "-D__GNUC__=4");
		list_add(l, "-D__GNUC_MINOR__=3");
		list_add(l, "-D__GNUC_PATCHLEVEL__=1");
		list_add(l, "-D__GNUC_%s_INLINE__", opt.gnu89 ? "GNU" : "STDC");
	}

	/* language specific */
	if (strcmp(infile->next, "I")
		list_add(l, "-D__cplusplus");
	if (strcmp(infile->next, "f")
		list_add(l, "-D__FORTRAN__");
	if (strcmp(infile->next, "s")
		list_add(l, "-D__ASSEMBLER__");

	/* runtime options */
	if (opt.optim > 0)
		list_add(l, "-D__OPTIMIZE__");
	if (opt.ssp > 0)
		list_add(l, "-D__SSP__");
	if (opt.pthread)
		list_add(l, "-D_PTHREADS");
	if (opt.uchar)
		list_add(l, "-D__CHAR_UNSIGNED__");

	/* target specific definitions */
	list_add_array(l, target_defs);
}

static int
cpp_exec(const char *infile, const char *outfile)
{
	list_t *l;
	const char *file;
	int rv;

	if (prog_cpp == NULL)
		error("No preprocessor is defined");

	l = list_alloc();

	list_add_file(l, prog_cpp, &progdirs, X_OK);

	for (int i = 0; i < opt.C; i++)
		list_add(l, "-C");
	if (opt.M)
		list_add(l, "-M");
	if (opt.P)
		list_add(l, "-P");
	if (opt.traditional)
		list_add(l, "-t");
	if (opt.verbose)
		list_add(l, "-v");

	Wflag_add(l, W_CPP);

	if (opt.stddefs)
		cpp_add_stddef(l);

	list_add_list(l, opt.DIU);

	list_add_list(l, opt.Wp);

	if (infile != NULL)
		list_add(l, infile);
	else if (outfile != NULL)
		list_add(l, "-");

	if (outfile != NULL)
		list_add(l, outfile);

	rv = list_exec(l);
	list_free(l);
	return rv;
}
