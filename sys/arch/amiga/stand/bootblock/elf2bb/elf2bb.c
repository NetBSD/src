/*	$NetBSD: elf2bb.c,v 1.28 2022/04/25 15:48:57 rin Exp $	*/

/*-
 * Copyright (c) 1996,2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>		/* of the machine we're running on */

#ifndef HAVE_NBTOOL_CONFIG_H
#include <sys/endian.h>		/* of the machine we're running on */
#endif

#include <sys/exec_elf.h>	/* TARGET */
#ifndef R_68K_32		/* XXX host not m68k XXX */
#define	R_68K_32	1
#define	R_68K_PC32	4
#define	R_68K_PC16	5
#endif

#include "elf2bb.h"
#include "chksum.h"

static void usage(void);
static int intcmp(const void *, const void *);
static int eval(Elf32_Sym *, uint32_t *);

#ifdef DEBUG
#define dprintf(x) if (debug) printf x
#else
#define dprintf(x)
#endif
int debug;

#define BBSIZE 8192

char *progname;
int bbsize = BBSIZE;
uint8_t *buffer;
uint32_t *relbuf;
	/* can't have more relocs than that */

static int
intcmp(const void *i, const void *j)
{
	int r;

	r = (*(uint32_t *)i) < (*(uint32_t *)j);
	
	return 2 * r - 1;
}

int
main(int argc, char *argv[])
{
	int ifd, ofd;
	void *image;
	Elf32_Ehdr *eh;
	Elf32_Shdr *sh;
	char *shstrtab;
	Elf32_Sym *symtab;
	char *strtab;
	uint32_t *lptr;
	int i, l, delta;
	uint8_t *rpo;
	uint32_t oldaddr, addrdiff;
	uint32_t tsz, dsz, bsz, trsz, relver;
	uint32_t pcrelsz, r32sz;
	int sumsize = 16;
	int c;
	uint32_t *sect_offset;
	int undefsyms;
	uint32_t tmp32;
	uint16_t tmp16;
	int Sflag = 0;

	progname = argv[0];

	/* insert getopt here, if needed */
	while ((c = getopt(argc, argv, "dFS")) != -1)
	switch(c) {
	case 'F':
		sumsize = 2;
		break;
	case 'S':
		/* Dynamically size second-stage boot */
		Sflag = 1;
		break;
	case 'd':
		debug = 1;
		break;
	default:
		usage();
	}
	argv += optind;
	argc -= optind;

	if (argc < 2)
		usage();

	ifd = open(argv[0], O_RDONLY, 0);
	if (ifd < 0)
		err(1, "Can't open %s", argv[0]);

	image = mmap(0, 65536, PROT_READ, MAP_FILE|MAP_PRIVATE, ifd, 0);
	if (image == MAP_FAILED)
		err(1, "Can't mmap %s", argv[1]);

	eh = (Elf32_Ehdr *)image; /* XXX endianness */

	dprintf(("%04x sections, offset %08x\n", be16toh(eh->e_shnum),
	    be32toh(eh->e_shoff)));
	if (be16toh(eh->e_type) != ET_REL)
		errx(1, "%s isn't a relocatable file, type=%d",
		    argv[0], be16toh(eh->e_type));
	if (be16toh(eh->e_machine) != EM_68K)
		errx(1, "%s isn't M68K, machine=%d", argv[0],
		    be16toh(eh->e_machine));

	/* Calculate sizes from section headers. */
	tsz = dsz = bsz = trsz = 0;
	sh = (Elf32_Shdr *)(image + be32toh(eh->e_shoff));
	shstrtab = (char *)(image +
	    be32toh(sh[be16toh(eh->e_shstrndx)].sh_offset));
	symtab = NULL;	/* XXX */
	strtab = NULL;	/* XXX */
	dprintf(("    name                      type     flags"
		 "    addr     offset   size     align\n"));
	for (i = 0; i < be16toh(eh->e_shnum); ++i) {
		uint32_t sh_size;

		dprintf(("%2d: %08x %-16s %08x %08x %08x %08x %08x %08x\n", i,
		    be32toh(sh[i].sh_name), shstrtab + be32toh(sh[i].sh_name),
		    be32toh(sh[i].sh_type), be32toh(sh[i].sh_flags),
		    be32toh(sh[i].sh_addr), be32toh(sh[i].sh_offset),
		    be32toh(sh[i].sh_size), be32toh(sh[i].sh_addralign)));
		sh_size = (be32toh(sh[i].sh_size) +
		    be32toh(sh[i].sh_addralign) - 1) &
		    (- be32toh(sh[i].sh_addralign));
		/*
		 * If section allocates memory, add to text, data,
		 * or bss size.
		 */
		if (be32toh(sh[i].sh_flags) & SHF_ALLOC) {
			if (be32toh(sh[i].sh_type) == SHT_PROGBITS) {
				if (be32toh(sh[i].sh_flags) & SHF_WRITE)
					dsz += sh_size;
				else
					tsz += sh_size;
			} else
				bsz += sh_size;
		/* If it's relocations, add to relocation count */
		} else if (be32toh(sh[i].sh_type) == SHT_RELA) {
			trsz += be32toh(sh[i].sh_size);
		}
		/* Check for SHT_REL? */
		/* Get symbol table location. */
		else if (be32toh(sh[i].sh_type) == SHT_SYMTAB) {
			symtab = (Elf32_Sym *)(image +
			    be32toh(sh[i].sh_offset));
		} else if (strcmp(".strtab", shstrtab +
		    be32toh(sh[i].sh_name)) == 0) {
			strtab = image + be32toh(sh[i].sh_offset);
		}
	}
	dprintf(("tsz = 0x%x, dsz = 0x%x, bsz = 0x%x, total 0x%x\n",
	    tsz, dsz, bsz, tsz + dsz + bsz));

	if (trsz == 0)
		errx(1, "%s has no relocation records.", argv[0]);

	dprintf(("%d relocs\n", trsz / 12));

	if (Sflag) {
		/*
		 * For second-stage boot, there's no limit for binary size,
		 * and we dynamically scale it. However, it should be small
		 * enough so that
		 *
		 *  (1) all R_68K_PC16 symbols get relocated, and
		 *
		 *  (2) all values in our relocation table for R_68K_32
		 *      symbols fit within 16-bit integer.
		 *
		 * Both will be checked by codes below.
		 *
		 * At the moment, (2) is satisfied with sufficient margin.
		 * But if it is not the case in the future, format for
		 * relocation table should be modified.
		 */
		bbsize = roundup(tsz + dsz, 512);
		sumsize = bbsize / 512;
	} else {
		/*
		 * We have one contiguous area allocated by the ROM to us.
		 */
		if (tsz + dsz + bsz > bbsize)
			errx(1, "%s: resulting image too big %d+%d+%d=%d",
			    argv[0], tsz, dsz, bsz, tsz + dsz + bsz);
	}

	buffer = NULL;
	relbuf = NULL;

 retry:
	pcrelsz = r32sz = 0;

	buffer = realloc(buffer, bbsize);
	relbuf = realloc(relbuf, bbsize);
	if (buffer == NULL || relbuf == NULL)
		err(1, "Unable to allocate memory\n");

	memset(buffer, 0, bbsize);

	/* Allocate and load loadable sections */
	sect_offset = malloc(be16toh(eh->e_shnum) * sizeof(uint32_t));
	for (i = 0, l = 0; i < be16toh(eh->e_shnum); ++i) {
		if (be32toh(sh[i].sh_flags) & SHF_ALLOC) {
			dprintf(("vaddr 0x%04x size 0x%04x offset 0x%04x section %s\n",
			    l, be32toh(sh[i].sh_size), be32toh(sh[i].sh_offset),
			    shstrtab + be32toh(sh[i].sh_name)));
			if (be32toh(sh[i].sh_type) == SHT_PROGBITS)
				memcpy(buffer + l,
				    image + be32toh(sh[i].sh_offset),
				    be32toh(sh[i].sh_size));
			sect_offset[i] = l;
			l += (be32toh(sh[i].sh_size) +
			    be32toh(sh[i].sh_addralign) - 1) &
			    (- be32toh(sh[i].sh_addralign));
		}
	}

	/*
	 * Hm. This tool REALLY should understand more than one
	 * relocator version. For now, check that the relocator at
	 * the image start does understand what we output.
	 */
	relver = be32toh(*(uint32_t *)(buffer + 4));
	switch (relver) {
	default:
		errx(1, "%s: unrecognized relocator version %d",
			argv[0], relver);
		/* NOTREACHED */

	case RELVER_RELATIVE_BYTES:
		rpo = buffer + bbsize - 1;
		delta = -1;
		break;

	case RELVER_RELATIVE_BYTES_FORWARD:
		rpo = buffer + tsz + dsz;
		delta = +1;
		*(uint16_t *)(buffer + 14) = htobe16(tsz + dsz);
		break;
	}

	if (symtab == NULL)
		errx(1, "No symbol table found");
	/*
	 * Link sections and generate relocation data
	 * Nasty:  .text, .rodata, .data, .bss sections are not linked
	 *         Symbol table values relative to start of sections.
	 * For each relocation entry:
	 *    Symbol value needs to be calculated: value + section offset
	 *    Image data adjusted to calculated value of symbol + addend
	 *    Add relocation table entry for 32-bit relocatable values
	 *    PC-relative entries will be absolute and don't need relocation
	 */
	undefsyms = 0;
	for (i = 0; i < be16toh(eh->e_shnum); ++i) {
		int n;
		Elf32_Rela *ra;
		uint8_t *base;

		if (be32toh(sh[i].sh_type) != SHT_RELA)
			continue;
		base = NULL;
		if (strncmp(shstrtab + be32toh(sh[i].sh_name), ".rela", 5) != 0)
			err(1, "bad relocation section name %s",
			    shstrtab + be32toh(sh[i].sh_name));
		for (n = 0; n < be16toh(eh->e_shnum); ++n) {
			if (strcmp(shstrtab + be32toh(sh[i].sh_name) + 5,
			    shstrtab + be32toh(sh[n].sh_name)) != 0)
				continue;
			base = buffer + sect_offset[n];
			break;
		}
		if (base == NULL)
			errx(1, "Can't find section for reloc %s",
			    shstrtab + be32toh(sh[i].sh_name));
		ra = (Elf32_Rela *)(image + be32toh(sh[i].sh_offset));
		for (n = 0; n < be32toh(sh[i].sh_size);
		    n += sizeof(Elf32_Rela), ++ra) {
			Elf32_Sym *s;
			int value;

			s = &symtab[ELF32_R_SYM(be32toh(ra->r_info))];
			if (s->st_shndx == ELF_SYM_UNDEFINED) {
				fprintf(stderr, "Undefined symbol: %s\n",
				    strtab + be32toh(s->st_name));
				++undefsyms;
			}
			value = be32toh(ra->r_addend) + eval(s, sect_offset);
			dprintf(("reloc %04x info %04x (type %d sym %d) add 0x%x val %x\n",
			    be32toh(ra->r_offset), be32toh(ra->r_info),
			    ELF32_R_TYPE(be32toh(ra->r_info)),
			    ELF32_R_SYM(be32toh(ra->r_info)),
			    be32toh(ra->r_addend), value));
			switch (ELF32_R_TYPE(be32toh(ra->r_info))) {
			case R_68K_32:
				tmp32 = htobe32(value);
				memcpy(base + be32toh(ra->r_offset), &tmp32,
				       sizeof(tmp32));
				relbuf[r32sz++] = (base - buffer) +
				    be32toh(ra->r_offset);
				break;
			case R_68K_PC32:
				++pcrelsz;
				tmp32 = htobe32(value - be32toh(ra->r_offset));
				memcpy(base + be32toh(ra->r_offset), &tmp32,
				       sizeof(tmp32));
				break;
			case R_68K_PC16:
				++pcrelsz;
				value -= be32toh(ra->r_offset);
				if (value < -0x8000 || value > 0x7fff)
					errx(1, "PC-relative offset out of range: %x\n",
					    value);
				tmp16 = htobe16(value);
				memcpy(base + be32toh(ra->r_offset), &tmp16,
				       sizeof(tmp16));
				break;
			default:
				errx(1, "Relocation type %d not supported",
				    ELF32_R_TYPE(be32toh(ra->r_info)));
			}
		}
	}
	dprintf(("%d PC-relative relocations, %d 32-bit relocations\n",
	    pcrelsz, r32sz));
	printf("%d absolute reloc%s found, ", r32sz, r32sz == 1 ? "" : "s");
	
	i = r32sz;
	if (i > 1)
		heapsort(relbuf, r32sz, 4, intcmp);

	oldaddr = 0;
	
	for (--i; i >= 0; --i) {
		dprintf(("0x%04x: ", relbuf[i]));
		lptr = (uint32_t *)&buffer[relbuf[i]];
		addrdiff = relbuf[i] - oldaddr;
		dprintf(("(0x%04x, 0x%04x): ", *lptr, addrdiff));
		if (addrdiff > 0xffff) {
			errx(1, "addrdiff overflows: relbuf = 0x%08x, "
			    "oldaddr = 0x%08x, abort.\n", relbuf[i], oldaddr);
		} else if (addrdiff > 0xff) {
			*rpo = 0;
			if (delta > 0) {
				++rpo;
				*rpo++ = (relbuf[i] >> 8) & 0xff;
				*rpo++ = relbuf[i] & 0xff;
				dprintf(("%02x%02x%02x\n",
				    rpo[-3], rpo[-2], rpo[-1]));
			} else {
				*--rpo = relbuf[i] & 0xff;
				*--rpo = (relbuf[i] >> 8) & 0xff;
				--rpo;
				dprintf(("%02x%02x%02x\n",
				    rpo[0], rpo[1], rpo[2]));
			}
		} else {
			*rpo = addrdiff;
			dprintf(("%02x\n", *rpo));
			rpo += delta;
		}

		oldaddr = relbuf[i];

		if (delta < 0 ?
		    rpo <= buffer + tsz + dsz : rpo >= buffer + bbsize) {
			printf("relocs don't fit, ");
			if (Sflag) {
				printf("retry.\n");
				bbsize += 512;
				sumsize++;
				goto retry;
			} else
				errx(1, "abort.");
		}
	}
	*rpo = 0; rpo += delta;
	*rpo = 0; rpo += delta;
	*rpo = 0; rpo += delta;

	printf("using %td bytes, %td bytes remaining.\n",
	    delta > 0 ? rpo - buffer - tsz - dsz : buffer + bbsize - rpo,
	    delta > 0 ? buffer + bbsize - rpo : rpo - buffer - tsz - dsz);
	/*
	 * RELOCs must fit into the bss area.
	 */
	if (delta < 0 ?
	    rpo <= buffer + tsz + dsz : rpo >= buffer + bbsize) {
		printf("relocs don't fit, ");
		if (Sflag) {
			printf("retry.\n");
			bbsize += 512;
			sumsize++;
			goto retry;
		} else
			errx(1, "abort.");
	}

	if (undefsyms > 0)
		errx(1, "Undefined symbols referenced");

	((uint32_t *)buffer)[1] = 0;
	((uint32_t *)buffer)[1] = htobe32((0xffffffff -
	    chksum((uint32_t *)buffer, sumsize * 512 / 4)));

	ofd = open(argv[1], O_CREAT|O_WRONLY, 0644);
	if (ofd < 0)
		err(1, "Can't open %s", argv[1]);

	if (write(ofd, buffer, bbsize) != bbsize)
		err(1, "Writing output file");

	exit(0);
}

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-F] bootprog bootprog.bin\n",
	    progname);
	exit(1);
	/* NOTREACHED */
}

static int
eval(Elf32_Sym *s, uint32_t *o)
{
	int value;

	value = be32toh(s->st_value);
	if (be16toh(s->st_shndx) < 0xf000)
		value += o[be16toh(s->st_shndx)];
	else
		printf("eval: %x\n", be16toh(s->st_shndx));
	return value;
}
