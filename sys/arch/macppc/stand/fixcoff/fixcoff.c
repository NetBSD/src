/*	$NetBSD: fixcoff.c,v 1.1 1999/11/23 01:32:37 wrstuden Exp $ */

/*
 * Copyright (c) 1999 National Aeronautics & Space Administration
 * All rights reserved.
 *
 * This software was written by William Studenmund of the
 * Numerical Aerospace Similation Facility, NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the the name of the National Aeronautics & Space Administration
 *    nor the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NATIONAL AERONAUTICS & SPACE ADMINISTRATION
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ADMINISTRATION OR CONTRIB-
 * UTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This program fixes up the extended xcoff headers generated when an elf
 * file is turned into an xcoff one with the current objcopy. It should
 * go away someday, when objcopy will correctly fix up the output xcoff
 *
 * Partially inspired by hack-coff, written by Paul Mackerras.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "../../../../../gnu/dist/include/coff/rs6000.h"

extern char *__progname;

void
usage(prog)
	char	*prog;
{
	fprintf(stderr, "Usage: %s [-h] | [<file to fix>]\n", prog);
}

void
help(prog)
	char	*prog;
{
	fprintf(stderr, "%s\tis designed to fix the xcoff headers in a\n", prog);
	fprintf(stderr,
"\tbinary generated using objcopy from a non-xcoff source.\n");
	usage(prog);
	exit(0);
}

main(argc, argv)
	int		argc;
	char * const	*argv;
{
	int	fd, i, n, ch;
	struct	external_filehdr	efh;
	AOUTHDR				aoh;
	struct	external_scnhdr		shead;

	while ((ch = getopt(argc, argv, "h")) != -1)
	    switch (ch) {
		case 'h':
		help(__progname);
	}

	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage(__progname);
		exit(1);
	}

	if ((fd = open(argv[0], O_RDWR, 0)) == -1)
		err(i, "%s", argv[0]);

	/*
	 * Make sure it looks like an xcoff file..
	 */
	if (read(fd, &efh, sizeof(efh)) != sizeof(efh))
		err(1, "%s reading header", argv[0]);

	i = ntohs(*(u_int16_t *)efh.f_magic);
	if ((i != U802WRMAGIC) && (i != U802ROMAGIC) && (i != U802TOCMAGIC))
		errx(1, "%s: not a valid xcoff file", argv[0]);

	/* Does the AOUT "Optional header" make sence? */
	i = ntohs(*(u_int16_t *)efh.f_opthdr);

	if (i == SMALL_AOUTSZ)
		errx(1, "%s: file has small \"optional\" header, inappropriate for use with %s", argv[0], __progname);
	else if (i != AOUTSZ)
		errx(1, "%s: invalid \"optional\" header", argv[0]);

	if (read(fd, &aoh, i) != i)
		err(1, "%s reading \"optional\" header", argv[0]);

	/* Now start filing in the AOUT header */
	*(u_int16_t *)aoh.magic = htons(RS6K_AOUTHDR_ZMAGIC);
	n = ntohs(*(u_int16_t *)efh.f_nscns);

	for (i = 0; i < n; i++) {
		if (read(fd, &shead, sizeof(shead)) != sizeof(shead))
			err(1, "%s reading section headers", argv[0]);
		if (strcmp(shead.s_name, ".text") == 0) {
			*(u_int16_t *)(aoh.o_snentry) = htons(i+1);
			*(u_int16_t *)(aoh.o_sntext) = htons(i+1);
		} else if (strcmp(shead.s_name, ".data") == 0) {
			*(u_int16_t *)(aoh.o_sndata) = htons(i+1);
		} else if (strcmp(shead.s_name, ".bss") == 0) {
			*(u_int16_t *)(aoh.o_snbss) = htons(i+1);
		}
	}

	/* now write it out */
	if (pwrite(fd, &aoh, sizeof(aoh), sizeof(struct external_filehdr))
			!= sizeof(aoh))
		err(1, "%s writing modified header", argv[0]);
	close(fd);
	exit(0);
}
