/*	$NetBSD: elf2aout.c,v 1.15 2011/07/10 05:07:48 tsutsui Exp $	*/

/*
 * Copyright (c) 1995
 *	Ted Lemon (hereinafter referred to as the author)
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* elf2aout.c

   This program converts an elf executable to a NetBSD a.out executable.
   The minimal symbol table is copied, but the debugging symbols and
   other informational sections are not. */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#ifndef TARGET_BYTE_ORDER
#define TARGET_BYTE_ORDER	BYTE_ORDER
#endif

#include <sys/types.h>
#include <sys/exec_aout.h>
#include <sys/exec_elf.h>

#include <a.out.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


struct sect {
	/* should be unsigned long, but assume no a.out binaries on LP64 */
	uint32_t vaddr;
	uint32_t len;
};

void	combine(struct sect *, struct sect *, int);
int	phcmp(const void *, const void *);
char   *saveRead(int file, off_t offset, off_t len, const char *name);
void	copy(int, int, off_t, off_t);
void	translate_syms(int, int, off_t, off_t, off_t, off_t);

#if TARGET_BYTE_ORDER != BYTE_ORDER
void	bswap32_region(int32_t* , int);
#endif

int    *symTypeTable;

int
main(int argc, char **argv)
{
	Elf32_Ehdr ex;
	Elf32_Phdr *ph;
	Elf32_Shdr *sh;
	char   *shstrtab;
	int     strtabix, symtabix;
	int     i;
	struct sect text, data, bss;
	struct exec aex;
	int     infile, outfile;
	uint32_t cur_vma = UINT32_MAX;
	uint32_t mid;
	int     symflag = 0;

	strtabix = symtabix = 0;
	text.len = data.len = bss.len = 0;
	text.vaddr = data.vaddr = bss.vaddr = 0;

	/* Check args... */
	if (argc < 3 || argc > 4) {
usage:
		fprintf(stderr,
		    "usage: elf2aout <elf executable> <a.out executable> [-s]\n");
		exit(1);
	}
	if (argc == 4) {
		if (strcmp(argv[3], "-s"))
			goto usage;
		symflag = 1;
	}
	/* Try the input file... */
	if ((infile = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "Can't open %s for read: %s\n",
		    argv[1], strerror(errno));
		exit(1);
	}
	/* Read the header, which is at the beginning of the file... */
	i = read(infile, &ex, sizeof ex);
	if (i != sizeof ex) {
		fprintf(stderr, "ex: %s: %s.\n",
		    argv[1], i ? strerror(errno) : "End of file reached");
		exit(1);
	}
#if TARGET_BYTE_ORDER != BYTE_ORDER
	ex.e_type	= bswap16(ex.e_type);
	ex.e_machine	= bswap16(ex.e_machine);
	ex.e_version	= bswap32(ex.e_version);
	ex.e_entry 	= bswap32(ex.e_entry);
	ex.e_phoff	= bswap32(ex.e_phoff);
	ex.e_shoff	= bswap32(ex.e_shoff);
	ex.e_flags	= bswap32(ex.e_flags);
	ex.e_ehsize	= bswap16(ex.e_ehsize);
	ex.e_phentsize	= bswap16(ex.e_phentsize);
	ex.e_phnum	= bswap16(ex.e_phnum);
	ex.e_shentsize	= bswap16(ex.e_shentsize);
	ex.e_shnum	= bswap16(ex.e_shnum);
	ex.e_shstrndx	= bswap16(ex.e_shstrndx);
#endif
	/* Read the program headers... */
	ph = (Elf32_Phdr *) saveRead(infile, ex.e_phoff,
	    ex.e_phnum * sizeof(Elf32_Phdr), "ph");
#if TARGET_BYTE_ORDER != BYTE_ORDER
	bswap32_region((int32_t*)ph, sizeof(Elf32_Phdr) * ex.e_phnum);
#endif
	/* Read the section headers... */
	sh = (Elf32_Shdr *) saveRead(infile, ex.e_shoff,
	    ex.e_shnum * sizeof(Elf32_Shdr), "sh");
#if TARGET_BYTE_ORDER != BYTE_ORDER
	bswap32_region((int32_t*)sh, sizeof(Elf32_Shdr) * ex.e_shnum);
#endif
	/* Read in the section string table. */
	shstrtab = saveRead(infile, sh[ex.e_shstrndx].sh_offset,
	    sh[ex.e_shstrndx].sh_size, "shstrtab");

	/* Find space for a table matching ELF section indices to a.out symbol
	 * types. */
	symTypeTable = malloc(ex.e_shnum * sizeof(int));
	if (symTypeTable == NULL) {
		fprintf(stderr, "symTypeTable: can't allocate.\n");
		exit(1);
	}
	memset(symTypeTable, 0, ex.e_shnum * sizeof(int));

	/* Look for the symbol table and string table... Also map section
	 * indices to symbol types for a.out */
	for (i = 0; i < ex.e_shnum; i++) {
		char   *name = shstrtab + sh[i].sh_name;
		if (!strcmp(name, ".symtab"))
			symtabix = i;
		else
			if (!strcmp(name, ".strtab"))
				strtabix = i;
			else
				if (!strcmp(name, ".text") || !strcmp(name, ".rodata"))
					symTypeTable[i] = N_TEXT;
				else
					if (!strcmp(name, ".data") || !strcmp(name, ".sdata") ||
					    !strcmp(name, ".lit4") || !strcmp(name, ".lit8"))
						symTypeTable[i] = N_DATA;
					else
						if (!strcmp(name, ".bss") || !strcmp(name, ".sbss"))
							symTypeTable[i] = N_BSS;
	}

	/* Figure out if we can cram the program header into an a.out
	 * header... Basically, we can't handle anything but loadable
	 * segments, but we can ignore some kinds of segments.   We can't
	 * handle holes in the address space, and we handle start addresses
	 * other than 0x1000 by hoping that the loader will know where to load
	 * - a.out doesn't have an explicit load address.   Segments may be
	 * out of order, so we sort them first. */
	qsort(ph, ex.e_phnum, sizeof(Elf32_Phdr), phcmp);
	for (i = 0; i < ex.e_phnum; i++) {
		/* Section types we can ignore... */
		if (ph[i].p_type == PT_NULL || ph[i].p_type == PT_NOTE ||
		    ph[i].p_type == PT_PHDR || ph[i].p_type == PT_MIPS_REGINFO)
			continue;
		/* Section types we can't handle... */
		else
			if (ph[i].p_type != PT_LOAD)
				errx(1, "Program header %d type %d can't be converted.", i, ph[i].p_type);
		/* Writable (data) segment? */
		if (ph[i].p_flags & PF_W) {
			struct sect ndata, nbss;

			ndata.vaddr = ph[i].p_vaddr;
			ndata.len = ph[i].p_filesz;
			nbss.vaddr = ph[i].p_vaddr + ph[i].p_filesz;
			nbss.len = ph[i].p_memsz - ph[i].p_filesz;

			combine(&data, &ndata, 0);
			combine(&bss, &nbss, 1);
		} else {
			struct sect ntxt;

			ntxt.vaddr = ph[i].p_vaddr;
			ntxt.len = ph[i].p_filesz;

			combine(&text, &ntxt, 0);
		}
		/* Remember the lowest segment start address. */
		if (ph[i].p_vaddr < cur_vma)
			cur_vma = ph[i].p_vaddr;
	}

	/* Sections must be in order to be converted... */
	if (text.vaddr > data.vaddr || data.vaddr > bss.vaddr ||
	    text.vaddr + text.len > data.vaddr || data.vaddr + data.len > bss.vaddr) {
		fprintf(stderr, "Sections ordering prevents a.out conversion.\n");
		exit(1);
	}
	/* If there's a data section but no text section, then the loader
	 * combined everything into one section.   That needs to be the text
	 * section, so just make the data section zero length following text. */
	if (data.len && text.len == 0) {
		text = data;
		data.vaddr = text.vaddr + text.len;
		data.len = 0;
	}
	/* If there is a gap between text and data, we'll fill it when we copy
	 * the data, so update the length of the text segment as represented
	 * in a.out to reflect that, since a.out doesn't allow gaps in the
	 * program address space. */
	if (text.vaddr + text.len < data.vaddr)
		text.len = data.vaddr - text.vaddr;

	/* We now have enough information to cons up an a.out header... */
	switch (ex.e_machine) {
	case EM_SPARC:
		mid = MID_SPARC;
		break;
	case EM_386:
		mid = MID_PC386;
		break;
	case EM_68K:
		mid = MID_M68K;
		break;
	case EM_MIPS:
		if (ex.e_ident[EI_DATA] == ELFDATA2LSB)
			mid = MID_PMAX;
		else
			mid = MID_MIPS;
		break;
	case EM_PPC:
		mid = MID_POWERPC;
		break;
	case EM_ARM:
		mid = MID_ARM6;
		break;
	case EM_VAX:
		mid = MID_VAX;
		break;
	case EM_NONE:
	default:
		mid = MID_ZERO;
	}
	aex.a_midmag = htonl((symflag << 26) | (mid << 16) | OMAGIC);

	aex.a_text = text.len;
	aex.a_data = data.len;
	aex.a_bss = bss.len;
	aex.a_entry = ex.e_entry;
	aex.a_syms = (sizeof(struct nlist) *
	    (symtabix != -1
		? sh[symtabix].sh_size / sizeof(Elf32_Sym) : 0));
	aex.a_trsize = 0;
	aex.a_drsize = 0;
#if TARGET_BYTE_ORDER != BYTE_ORDER
	aex.a_text = bswap32(aex.a_text);
	aex.a_data = bswap32(aex.a_data);
	aex.a_bss = bswap32(aex.a_bss);
	aex.a_entry = bswap32(aex.a_entry);
	aex.a_syms = bswap32(aex.a_syms);
	aex.a_trsize = bswap32(aex.a_trsize);
	aex.a_drsize = bswap32(aex.a_drsize);
#endif

	/* Make the output file... */
	if ((outfile = open(argv[2], O_WRONLY | O_CREAT, 0777)) < 0) {
		fprintf(stderr, "Unable to create %s: %s\n", argv[2], strerror(errno));
		exit(1);
	}
	/* Truncate file... */
	if (ftruncate(outfile, 0)) {
		warn("ftruncate %s", argv[2]);
	}
	/* Write the header... */
	i = write(outfile, &aex, sizeof aex);
	if (i != sizeof aex) {
		perror("aex: write");
		exit(1);
	}
	/* Copy the loadable sections.   Zero-fill any gaps less than 64k;
	 * complain about any zero-filling, and die if we're asked to
	 * zero-fill more than 64k. */
	for (i = 0; i < ex.e_phnum; i++) {
		/* Unprocessable sections were handled above, so just verify
		 * that the section can be loaded before copying. */
		if (ph[i].p_type == PT_LOAD && ph[i].p_filesz) {
			if (cur_vma != ph[i].p_vaddr) {
				uint32_t gap = ph[i].p_vaddr - cur_vma;
				char    obuf[1024];
				if (gap > 65536)
					errx(1,
			"Intersegment gap (%ld bytes) too large.", (long) gap);
#ifdef DEBUG
				warnx("Warning: %ld byte intersegment gap.",
				    (long)gap);
#endif
				memset(obuf, 0, sizeof obuf);
				while (gap) {
					int     count = write(outfile, obuf, (gap > sizeof obuf
						? sizeof obuf : gap));
					if (count < 0) {
						fprintf(stderr, "Error writing gap: %s\n",
						    strerror(errno));
						exit(1);
					}
					gap -= count;
				}
			}
			copy(outfile, infile, ph[i].p_offset, ph[i].p_filesz);
			cur_vma = ph[i].p_vaddr + ph[i].p_filesz;
		}
	}

	/* Copy and translate the symbol table... */
	translate_syms(outfile, infile,
	    sh[symtabix].sh_offset, sh[symtabix].sh_size,
	    sh[strtabix].sh_offset, sh[strtabix].sh_size);

	/* Looks like we won... */
	exit(0);
}
/* translate_syms (out, in, offset, size)

   Read the ELF symbol table from in at offset; translate it into a.out
   nlist format and write it to out. */

void
translate_syms(int out, int in, off_t symoff, off_t symsize,
    off_t stroff, off_t strsize)
{
#define SYMS_PER_PASS	64
	Elf32_Sym inbuf[64];
	struct nlist outbuf[64];
	int     i, remaining, cur;
	char   *oldstrings;
	char   *newstrings, *nsp;
	int     newstringsize, stringsizebuf;

	/* Zero the unused fields in the output buffer.. */
	memset(outbuf, 0, sizeof outbuf);

	/* Find number of symbols to process... */
	remaining = symsize / sizeof(Elf32_Sym);

	/* Suck in the old string table... */
	oldstrings = saveRead(in, stroff, strsize, "string table");

	/* Allocate space for the new one.   XXX We make the wild assumption
	 * that no two symbol table entries will point at the same place in
	 * the string table - if that assumption is bad, this could easily
	 * blow up. */
	newstringsize = strsize + remaining;
	newstrings = malloc(newstringsize);
	if (newstrings == NULL) {
		fprintf(stderr, "No memory for new string table!\n");
		exit(1);
	}
	/* Initialize the table pointer... */
	nsp = newstrings;

	/* Go the start of the ELF symbol table... */
	if (lseek(in, symoff, SEEK_SET) < 0) {
		perror("translate_syms: lseek");
		exit(1);
	}
	/* Translate and copy symbols... */
	while (remaining) {
		cur = remaining;
		if (cur > SYMS_PER_PASS)
			cur = SYMS_PER_PASS;
		remaining -= cur;
		if ((i = read(in, inbuf, cur * sizeof(Elf32_Sym)))
		    != cur * (ssize_t)sizeof(Elf32_Sym)) {
			if (i < 0)
				perror("translate_syms");
			else
				fprintf(stderr, "translate_syms: premature end of file.\n");
			exit(1);
		}
		/* Do the translation... */
		for (i = 0; i < cur; i++) {
			int     binding, type;

#if TARGET_BYTE_ORDER != BYTE_ORDER
			inbuf[i].st_name  = bswap32(inbuf[i].st_name);
			inbuf[i].st_value = bswap32(inbuf[i].st_value);
			inbuf[i].st_size  = bswap32(inbuf[i].st_size);
			inbuf[i].st_shndx = bswap16(inbuf[i].st_shndx);
#endif
			/* Copy the symbol into the new table, but prepend an
			 * underscore. */
			*nsp = '_';
			strcpy(nsp + 1, oldstrings + inbuf[i].st_name);
			outbuf[i].n_un.n_strx = nsp - newstrings + 4;
			nsp += strlen(nsp) + 1;

			type = ELF32_ST_TYPE(inbuf[i].st_info);
			binding = ELF32_ST_BIND(inbuf[i].st_info);

			/* Convert ELF symbol type/section/etc info into a.out
			 * type info. */
			if (type == STT_FILE)
				outbuf[i].n_type = N_FN;
			else
				if (inbuf[i].st_shndx == SHN_UNDEF)
					outbuf[i].n_type = N_UNDF;
				else
					if (inbuf[i].st_shndx == SHN_ABS)
						outbuf[i].n_type = N_ABS;
					else
						if (inbuf[i].st_shndx == SHN_COMMON ||
						    inbuf[i].st_shndx == SHN_MIPS_ACOMMON)
							outbuf[i].n_type = N_COMM;
						else
							outbuf[i].n_type = symTypeTable[inbuf[i].st_shndx];
			if (binding == STB_GLOBAL)
				outbuf[i].n_type |= N_EXT;
			/* Symbol values in executables should be compatible. */
			outbuf[i].n_value = inbuf[i].st_value;
#if TARGET_BYTE_ORDER != BYTE_ORDER
			outbuf[i].n_un.n_strx = bswap32(outbuf[i].n_un.n_strx);
			outbuf[i].n_desc      = bswap16(outbuf[i].n_desc);
			outbuf[i].n_value     = bswap32(outbuf[i].n_value);
#endif
		}
		/* Write out the symbols... */
		if ((i = write(out, outbuf, cur * sizeof(struct nlist)))
		    != cur * (ssize_t)sizeof(struct nlist)) {
			fprintf(stderr, "translate_syms: write: %s\n", strerror(errno));
			exit(1);
		}
	}
	/* Write out the string table length... */
	stringsizebuf = newstringsize;
#if TARGET_BYTE_ORDER != BYTE_ORDER
	stringsizebuf = bswap32(stringsizebuf);
#endif
	if (write(out, &stringsizebuf, sizeof stringsizebuf)
	    != sizeof stringsizebuf) {
		fprintf(stderr,
		    "translate_syms: newstringsize: %s\n", strerror(errno));
		exit(1);
	}
	/* Write out the string table... */
	if (write(out, newstrings, newstringsize) != newstringsize) {
		fprintf(stderr, "translate_syms: newstrings: %s\n", strerror(errno));
		exit(1);
	}
}

void
copy(int out, int in, off_t offset, off_t size)
{
	char    ibuf[4096];
	int     remaining, cur, count;

	/* Go to the start of the ELF symbol table... */
	if (lseek(in, offset, SEEK_SET) < 0) {
		perror("copy: lseek");
		exit(1);
	}
	remaining = size;
	while (remaining) {
		cur = remaining;
		if (cur > (int)sizeof ibuf)
			cur = sizeof ibuf;
		remaining -= cur;
		if ((count = read(in, ibuf, cur)) != cur) {
			fprintf(stderr, "copy: read: %s\n",
			    count ? strerror(errno) : "premature end of file");
			exit(1);
		}
		if ((count = write(out, ibuf, cur)) != cur) {
			perror("copy: write");
			exit(1);
		}
	}
}
/* Combine two segments, which must be contiguous.   If pad is true, it's
   okay for there to be padding between. */
void
combine(struct sect *base, struct sect *new, int pad)
{

	if (base->len == 0)
		*base = *new;
	else
		if (new->len) {
			if (base->vaddr + base->len != new->vaddr) {
				if (pad)
					base->len = new->vaddr - base->vaddr;
				else {
					fprintf(stderr,
					    "Non-contiguous data can't be converted.\n");
					exit(1);
				}
			}
			base->len += new->len;
		}
}

int
phcmp(const void *vh1, const void *vh2)
{
	const Elf32_Phdr *h1, *h2;

	h1 = (const Elf32_Phdr *)vh1;
	h2 = (const Elf32_Phdr *)vh2;

	if (h1->p_vaddr > h2->p_vaddr)
		return 1;
	else
		if (h1->p_vaddr < h2->p_vaddr)
			return -1;
		else
			return 0;
}

char *
saveRead(int file, off_t offset, off_t len, const char *name)
{
	char   *tmp;
	int     count;
	off_t   off;
	if ((off = lseek(file, offset, SEEK_SET)) < 0) {
		fprintf(stderr, "%s: fseek: %s\n", name, strerror(errno));
		exit(1);
	}
	if ((tmp = malloc(len)) == NULL)
		errx(1, "%s: Can't allocate %ld bytes.", name, (long)len);
	count = read(file, tmp, len);
	if (count != len) {
		fprintf(stderr, "%s: read: %s.\n",
		    name, count ? strerror(errno) : "End of file reached");
		exit(1);
	}
	return tmp;
}

#if TARGET_BYTE_ORDER != BYTE_ORDER
/* swap a 32bit region */
void
bswap32_region(int32_t* p, int len)
{
	size_t i;

	for (i = 0; i < len / sizeof(int32_t); i++, p++)
		*p = bswap32(*p);
}
#endif
