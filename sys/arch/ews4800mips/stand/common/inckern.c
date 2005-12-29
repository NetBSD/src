/*	$NetBSD: inckern.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <unistd.h>

enum { TMPBUF_SIZE = 0x2000 };

int
main(int argc, char *argv[])
{
	unsigned char buf[TMPBUF_SIZE];
	FILE *ifd, *ofd;
	int err, i, n, total, nulldata;

	nulldata = 0;
	err = 1;
	total = 0;
	ifd = stdin;
	ofd = stdout;
	while ((i = getopt(argc, argv, "d:i:o:")) != -1) {
		switch (i) {
		case 'd':	/* Generate dummy file */
			if ((optarg == 0) || (ifd != stdin))
				goto bye;
			total = strtoul(optarg, 0, 0);
			nulldata = 1;
			break;
		case 'i':	/* Specify input file */
			if ((optarg == 0) || (total != 0) ||
			    (ifd = fopen(optarg, "r")) == 0)
				goto bye;
			break;
		case 'o':	/* Specify output file */
			if ((optarg == 0) || (ofd = fopen(optarg, "w")) == 0)
				goto bye;
			break;
		}
	}

	fprintf(ofd, "#include <lib/libsa/stand.h>\n");
	fprintf(ofd, "#include <lib/libkern/libkern.h>\n");
	fprintf(ofd, "#include \"local.h\"\n");
	fprintf(ofd, "uint8_t kernel_binary[");
	if (nulldata) {
		fprintf(ofd, "%d];\n", total);
		fprintf(ofd, "int kernel_binary_size = %d;\n", total);
	} else {
		fprintf(ofd, "] = {\n\t");
		while ((n = fread(buf, 1, TMPBUF_SIZE, ifd)) > 0) {
			for (i = 0; i < n; i++) {
				fprintf(ofd, "0x%02x, ", buf[i]);
				if (((i + 1) & 0x7) == 0)
					fprintf(ofd, "\n\t");
			}
			total += n;
		}
		fprintf(ofd, "\n};\nint kernel_binary_size = %d;\n", total);
	}
	err = 0;

 bye:
	if (err)
		perror(0);

	if (ifd != stdin)
		fclose(ifd);
	if (ofd != stdout)
		fclose(ofd);

	return err;
}
