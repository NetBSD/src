/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: @(#)coff.c	7.1 (Berkeley) 1/7/92
 * $Id: coff.c,v 1.1.1.1 1993/10/12 03:22:51 deraadt Exp $
 */

#define COFF
#include <sys/exec.h>

/*
 * Print header info for coff files.
 */
void
main(argc, argv)
	int argc;
	char **argv;
{
	register struct devices *dp;
	register int fd, i, n;
	char *fname;
	struct exec aout;

	if (argc < 2) {
		printf("usage: %s <file>\n");
		goto err;
	}
	if ((fd = open(fname = argv[1], 0)) < 0)
		goto err;

	/* read the COFF header */
	i = read(fd, (char *)&aout, sizeof(aout));
	if (i != sizeof(aout)) {
		printf("No a.out header\n");
		goto cerr;
	}
	printf("HDR: magic 0x%x(0%o) nsec %d nsym %d optheader %d textoff %x\n",
		aout.ex_fhdr.magic,
		aout.ex_fhdr.magic,
		aout.ex_fhdr.numSections,
		aout.ex_fhdr.numSyms,
		aout.ex_fhdr.optHeader,
		N_TXTOFF(aout));
	printf("A.out: magic 0x%x(0%o) ver %d entry %x gprM %x gpV %x\n",
		aout.ex_aout.magic,
		aout.ex_aout.magic,
		aout.ex_aout.verStamp,
		aout.ex_aout.entry,
		aout.ex_aout.gprMask,
		aout.ex_aout.gpValue);
	printf("\tstart %x,%x,%x size %d+%d+%d\n",
		aout.ex_aout.codeStart,
		aout.ex_aout.heapStart,
		aout.ex_aout.bssStart,
		aout.ex_aout.codeSize,
		aout.ex_aout.heapSize,
		aout.ex_aout.bssSize);

cerr:
	close(fd);
err:
	exit(0);
}
