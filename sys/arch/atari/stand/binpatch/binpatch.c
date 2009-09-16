/* $NetBSD: binpatch.c,v 1.4.62.2 2009/09/16 13:37:36 yamt Exp $ */

/*-
 * Copyright (c) 2009 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 Christopher G. Demetriou
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE_GC,v 1.1 2001/10/01 23:24:05 cgd Exp>>
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1996\
 Christopher G. Demetriou.  All rights reserved.");
#endif /* not lint */

#ifndef lint
__RCSID("$NetBSD: binpatch.c,v 1.4.62.2 2009/09/16 13:37:36 yamt Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/inttypes.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "extern.h"

int		main(int, char *[]);
static void	usage(void) __dead;

bool replace, verbose;
u_long addr, offset;
char *symbol;
size_t size;
uint64_t val;
	
#ifdef NLIST_AOUT
/*
 * Since we can't get the text address from an a.out executable, we
 * need to be able to specify it.  Note: there's no way to test to
 * see if the user entered a valid address!
 */
int	T_flag_specified;	/* the -T flag was specified */
u_long	text_start;		/* Start of kernel text */
#endif /* NLIST_AOUT */

static const struct {
	const char *name;
	int	(*check)(const char *, size_t);
	int	(*findoff)(const char *, size_t, u_long, size_t *);
} exec_formats[] = {
#ifdef NLIST_AOUT
	{	"a.out",	check_aout,	findoff_aout,	},
#endif
#ifdef NLIST_ECOFF
	{	"ECOFF",	check_ecoff,	findoff_ecoff,	},
#endif
#ifdef NLIST_ELF32
	{	"ELF32",	check_elf32,	findoff_elf32,	},
#endif
#ifdef NLIST_ELF64
	{	"ELF64",	check_elf64,	findoff_elf64,	},
#endif
#ifdef NLIST_COFF
	{	"COFF",		check_coff,	findoff_coff,	},
#endif
};


int
main(int argc, char *argv[])
{
	const char *fname;
	struct stat sb;
	struct nlist nl[2];
	char *mappedfile;
	size_t valoff;
	void *valp;
	uint8_t uval8;
	int8_t  sval8;
	uint16_t uval16;
	int16_t  sval16;
	uint32_t uval32;
	int32_t  sval32;
	uint64_t uval64;
	int64_t  sval64;
	int ch, fd, rv, i, n;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "bwldT:a:s:o:r:v")) != -1)
		switch (ch) {
		case 'b':
			size = sizeof(uint8_t);
			break;
		case 'w':
			size = sizeof(uint16_t);
			break;
		case 'l':
			size = sizeof(uint32_t);
			break;
		case 'd':
			size = sizeof(uint64_t);
			break;
		case 'a':
			if (addr != 0 || symbol != NULL)
				errx(EXIT_FAILURE,
				    "only one address/symbol allowed");
			addr = strtoul(optarg, NULL, 0);
			break;
		case 's':
			if (addr != 0 || symbol != NULL)
				errx(EXIT_FAILURE,
				    "only one address/symbol allowed");
			symbol = optarg;
			break;
		case 'o':
			if (offset != 0)
				err(EXIT_FAILURE,
				    "only one offset allowed");
			offset = strtoul(optarg, NULL, 0);
			break;
		case 'r':
			replace = true;
			val = strtoull(optarg, NULL, 0);
			break;
		case 'v':
			verbose = true;
			break;
		case 'T':
#ifdef NLIST_AOUT
			T_flag_specified = 1;
			text_start = strtoul(optarg, NULL, 0);
			break;
#else
			fprintf(stderr, "%s: unknown option -- %c\n",
			    getprogname(), (char)ch);
			/*FALLTHROUGH*/
#endif /* NLIST_AOUT */
		case '?':
		default:
			usage();
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if (addr == 0 && symbol == NULL) {
		warnx("no address or symbol specified");
		usage();
	}

	if (size == 0)
		size = sizeof(uint32_t);	/* default to int */

	fname = argv[0];

	if ((fd = open(fname, replace ? O_RDWR : O_RDONLY, 0))  == -1)
		err(EXIT_FAILURE, "open %s", fname);

	if (symbol != NULL) {
		nl[0].n_name = symbol;
		nl[1].n_name = NULL;
		if ((rv = __fdnlist(fd, nl)) != 0)
			errx(EXIT_FAILURE, "could not find symbol %s in %s",
			    symbol, fname);
		addr = nl[0].n_value;
		if (verbose)
			fprintf(stderr, "got symbol address 0x%lx from %s\n",
			    addr, fname);
	}

	addr += offset * size;

	if (fstat(fd, &sb) == -1)
		err(EXIT_FAILURE, "fstat %s", fname);
	if (sb.st_size != (ssize_t)sb.st_size)
		errx(EXIT_FAILURE, "%s too big to map", fname);

	if ((mappedfile = mmap(NULL, sb.st_size,
	    replace ? PROT_READ | PROT_WRITE : PROT_READ,
	    MAP_FILE | MAP_SHARED, fd, 0)) == (char *)-1)
		err(EXIT_FAILURE, "mmap %s", fname);
	if (verbose)
		fprintf(stderr, "mapped %s\n", fname);

	n = __arraycount(exec_formats);
	for (i = 0; i < n; i++) {
		if ((*exec_formats[i].check)(mappedfile, sb.st_size) == 0)
			break;
	}
	if (i == n)
		errx(EXIT_FAILURE, "%s: unknown executable format", fname);

	if (verbose) {
		fprintf(stderr, "%s is an %s binary\n", fname,
		    exec_formats[i].name);
#ifdef NLIST_AOUT
		if (T_flag_specified)
			fprintf(stderr, "kernel text loads at 0x%lx\n",
			    text_start);
#endif
	}

	if ((*exec_formats[i].findoff)(mappedfile, sb.st_size,
	    addr, &valoff) != 0)
		errx(EXIT_FAILURE, "couldn't find file offset for %s in %s",
		    symbol != NULL ? nl[0].n_name : "address" , fname);

	valp = mappedfile + valoff;

	if (symbol)
		printf("%s(0x%lx): ", symbol, addr);
	else
		printf("0x%lx: ", addr);

	switch (size) {
	case sizeof(uint8_t):
		uval8 = *(uint8_t *)valp;
		sval8 = *(int8_t *)valp;
		printf("0x%02" PRIx8 " (%" PRIu8, uval8, uval8);
		if (sval8 < 0)
			printf("/%" PRId8, sval8);
		printf(")");
		break;
	case sizeof(uint16_t):
		uval16 = *(uint16_t *)valp;
		sval16 = *(int16_t *)valp;
		printf("0x%04" PRIx16 " (%" PRIu16, uval16, uval16);
		if (sval16 < 0)
			printf("/%" PRId16, sval16);
		printf(")");
		break;
	case sizeof(uint32_t):
		uval32 = *(uint32_t *)valp;
		sval32 = *(int32_t *)valp;
		printf("0x%08" PRIx32 " (%" PRIu32, uval32, uval32);
		if (sval32 < 0)
			printf("/%" PRId32, sval32);
		printf(")");
		break;
	case sizeof(uint64_t):
		uval64 = *(uint64_t *)valp;
		sval64 = *(int64_t *)valp;
		printf("0x%016" PRIx64 " (%" PRIu64, uval64, uval64);
		if (sval64 < 0)
			printf("/%" PRId64, sval64);
		printf(")");
		break;
	}
	printf(", at offset %#lx in %s\n", (unsigned long)valoff, fname);

	if (!replace)
		goto done;

	printf("new value: ");

	switch (size) {
	case sizeof(uint8_t):
		uval8 = (uint8_t)val;
		sval8 = (int8_t)val;
		printf("0x%02" PRIx8 " (%" PRIu8, uval8, uval8);
		if (sval8 < 0)
			printf("/%" PRId8, sval8);
		printf(")");
		*(uint8_t *)valp = uval8;
		break;
	case sizeof(uint16_t):
		uval16 = (uint16_t)val;
		sval16 = (int16_t)val;
		printf("0x%04" PRIx16 " (%" PRIu16, uval16, uval16);
		if (sval16 < 0)
			printf("/%" PRId16, sval16);
		printf(")");
		*(uint16_t *)valp = uval16;
		break;
	case sizeof(uint32_t):
		uval32 = (uint32_t)val;
		sval32 = (int32_t)val;
		printf("0x%08" PRIx32 " (%" PRIu32, uval32, uval32);
		if (sval32 < 0)
			printf("/%" PRId32, sval32);
		printf(")");
		*(uint32_t *)valp = uval32;
		break;
	case sizeof(uint64_t):
		uval64 = (uint64_t)val;
		sval64 = (int64_t)val;
		printf("0x%016" PRIx64 " (%" PRIu64, uval64, uval64);
		if (sval64 < 0)
			printf("/%" PRId64, sval64);
		printf(")");
		*(uint64_t *)valp = uval64;
		break;
	}
	printf("\n");
	
 done:
	munmap(mappedfile, sb.st_size);
	close(fd);

	if (verbose)
		fprintf(stderr, "exiting\n");
	exit(EXIT_SUCCESS);
}

static void
usage(void)
{

	fprintf(stderr,
	    "usage: %s [-b|-w|-l|-d] [-a address | -s symbol] [-o offset]\n"
	    "                [-r value] "
#ifdef NLIST_AOUT
	    "[-T text_start] "
#endif
	    "[-v] binary\n", getprogname());
	exit(EXIT_FAILURE);
}
