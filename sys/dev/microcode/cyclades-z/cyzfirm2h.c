/*	$NetBSD: cyzfirm2h.c,v 1.13 2014/04/01 15:35:41 christos Exp $	*/

/*-
 * Copyright (c) 2000 Zembu Labs, Inc.
 * All rights reserved.
 *
 * Author: Jason R. Thorpe <thorpej@zembu.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Zembu Labs, Inc.
 * 4. Neither the name of Zembu Labs nor the names of its employees may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZEMBU LABS, INC. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAR-
 * RANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DIS-
 * CLAIMED.  IN NO EVENT SHALL ZEMBU LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This program converts a binary Cyclades-Z firmware file into a
 * C header file for use in a device driver.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: cyzfirm2h.c,v 1.13 2014/04/01 15:35:41 christos Exp $");

#include <sys/types.h>
#include <sys/mman.h>
#include <err.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void	usage(void) __dead;
#ifdef DEBUG
#define MAXLINE 8
#else
#define MAXLINE 10
#endif

int
main(int argc, char *argv[])
{
	off_t in_len;
	u_char *in_ptr;
	FILE *out_file;
	char *include_name, *cp;
	int i;

	if (argc != 3)
		usage();

	i = open(argv[1], O_RDONLY, 0644);
	if (i < 0)
		err(1, "unable to open %s", argv[1]);

	out_file = fopen(argv[2], "w+");
	if (out_file == NULL)
		err(1, "unable to create %s", argv[2]);

	/*
	 * Create the string used in the header file for multiple
	 * inclusion protection.
	 */
	include_name = strdup(argv[2]);
	if (include_name == NULL)
		err(1, "unable to allocate include name");

	for (cp = include_name; *cp != '\0'; cp++) {
		if (isalpha((unsigned char)*cp))
			*cp = toupper((unsigned char)*cp);
		else if (*cp == '.')
			*cp = '_';
	}

	in_len = lseek(i, 0, SEEK_END);
	if (in_len == (off_t) -1)
		err(1, "unable to determine length of input file");

	in_ptr = mmap(NULL, in_len, PROT_READ, MAP_FILE|MAP_SHARED,
	    i, (off_t) 0);
	if (in_ptr == MAP_FAILED)
		err(1, "unable to mmap input file");
	(void) close(i);

	fprintf(out_file, "/*\t$""NetBSD""$\t*/\n\n");
	fprintf(out_file,
	    "/*\n"
	    " * Firmware for Cyclades Z series multiport serial boards.\n"
	    " * Automatically generated from:\n"
	    " *\n"
	    " *\t%s\n"
	    " */\n\n", argv[1]);
	fprintf(out_file, "#ifndef _%s_\n", include_name);
	fprintf(out_file, "#define\t_%s_\n\n", include_name);

	fprintf(out_file, "static const uint8_t cycladesz_firmware[] = {\n");

	i = 0;
	while (in_len != 0) {
		if (i == 0)
			fprintf(out_file, "\t");
		if (*in_ptr == '@' && in_len > 4 &&
		    memcmp(in_ptr, "@(#)", 4) == 0)
			fprintf(out_file, "0x%02x,", '_');
		else
			fprintf(out_file, "0x%02x,", *in_ptr);
		in_ptr++;
		in_len--;
		i++;
		if (i == MAXLINE) {
#ifdef DEBUG
			size_t j;
			fprintf(out_file, "\t/* ");
			for (j = 0; j < 8; j++) {
				unsigned char c = (in_ptr - 8)[j];
				fputc(isprint(c) ? c : '.', out_file);
			}
			fprintf(out_file, " */");
#endif
			fprintf(out_file, "\n");
			i = 0;
		} else if (in_len != 0) {
			fprintf(out_file, " ");
		}
	}
	fprintf(out_file, "\n};\n\n");

	fprintf(out_file, "#endif /* _%s_ */\n", include_name);
	return 0;
}

__dead static void
usage(void)
{

	fprintf(stderr, "usage: %s infile outfile\n", getprogname());
	exit(1);
}
