/*	Id	*/	
/*	$NetBSD: prog_ccom.c,v 1.1.1.1 2016/02/09 20:29:12 plunky Exp $	*/

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
ccom_exec(const char *infile, const char *outfile)
{
	list_t *l;
	char *prog;

	if (ccom_name == NULL)
		error("No C compiler was defined");

	l = list_alloc();

	prog = find_file(ccom_name, &progdirs, X_OK);
	list_add_nocopy(l, prog);

	Wflag_add(l, W_CCOM);

	if (opt.stabs)
		list_add(l, "-g");
	if (opt.O > 0) {
		list_add(l, "-xtemps");
		list_add(l, "-xdeljumps");
		list_add(l, "-xinline");
		list_add(l, "-xdce");
	}
	if (opt.gnu89 || opt.gnu99)
		list_add(l, "-xgcc");
	if (opt.pic)
		list_add(l, "-k");
	if (opt.profile)
		list_add(l, "-p");

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
