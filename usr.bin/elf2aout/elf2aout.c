/*	$NetBSD: elf2aout.c,v 1.23.10.2 2024/12/15 12:57:32 martin Exp $	*/

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
#include <sys/endian.h>
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

static void	combine(struct sect *, struct sect *, int);
static int	phcmp(const void *, const void *);
static void   *saveRead(int file, off_t offset, size_t len, const char *name);
static void	copy(int, int, off_t, off_t);
static void	translate_syms(int, int, off_t, off_t, off_t, off_t);

#if TARGET_BYTE_ORDER != BYTE_ORDER
static void	bswap32_region(int32_t* , int);
#endif

static int    *symTypeTable;
static int     debug;

static __dead void
usage(void)
{
	fprintf(stderr, "Usage: %s [-Os] <elf executable> <a.out executable>\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

static const struct {
	const char *n;
	int v;
} nv[] = {
	{ ".text", N_TEXT },
	{ ".rodata", N_TEXT },
	{ ".data", N_DATA },
	{ ".sdata", N_DATA },
	{ ".lit4", N_DATA },
	{ ".lit8", N_DATA },
	{ ".bss", N_BSS },
	{ ".sbss", N_BSS },
};

static int
get_symtab_type(const char *name)
{
	size_t i;
	for (i = 0; i < __arraycount(nv); i++) {
		if (strcmp(name, nv[i].n) == 0)
			return nv[i].v;
	}
	if (debug)
		warnx("section `%s' is not handled\n", name);
	return 0;
}

static uint32_t
get_mid(const Elf32_Ehdr *ex)
{
	switch (ex->e_machine) {
#ifdef notyet
	case EM_AARCH64:
		return MID_AARCH64;
	case EM_ALPHA:
		return MID_ALPHA;
#endif
	case EM_ARM:
		return MID_ARM6;
#ifdef notyet
	case EM_PARISC:
		return MID_HPPA;
#endif
	case EM_386:
		return MID_I386;
	case EM_68K:
		return MID_M68K;
	case EM_OR1K:
		return MID_OR1K;
	case EM_MIPS:
		if (ex->e_ident[EI_DATA] == ELFDATA2LSB)
			return MID_PMAX;
		else
			return MID_MIPS;
	case EM_PPC:
		return MID_POWERPC;
#ifdef notyet
	case EM_PPC64:
		return MID_POWERPC64;
		break;
#endif
	case EM_RISCV:
		return MID_RISCV;
	case EM_SH:
		return MID_SH3;
	case EM_SPARC:
	case EM_SPARC32PLUS:
	case EM_SPARCV9:
		if (ex->e_ident[EI_CLASS] == ELFCLASS32)
			return MID_SPARC;
#ifdef notyet
		return MID_SPARC64;
	case EM_X86_64:
		return MID_X86_64;
#else
		break;
#endif
	case EM_VAX:
		return MID_VAX;
	case EM_NONE:
		return MID_ZERO;
	default:
		break;
	}
	if (debug)
		warnx("Unsupported machine `%d'", ex->e_machine);
	return MID_ZERO;
}

static unsigned char
get_type(Elf32_Half shndx)
{
	switch (shndx) {
	case SHN_UNDEF:
		return N_UNDF;
	case SHN_ABS:
		return N_ABS;
	case SHN_COMMON:
	case SHN_MIPS_ACOMMON:
		return N_COMM;
	default:
		return (unsigned char)symTypeTable[shndx];
	}
}

int
main(int argc, char **argv)
{
	Elf32_Ehdr ex;
	Elf32_Phdr *ph;
	Elf32_Shdr *sh;
	char   *shstrtab;
	ssize_t i, strtabix, symtabix;
	struct sect text, data, bss;
	struct exec aex;
	int     infile, outfile;
	uint32_t cur_vma = UINT32_MAX;
	uint32_t mid;
	int symflag = 0, c;
	unsigned long magic = ZMAGIC;

	strtabix = symtabix = 0;
	text.len = data.len = bss.len = 0;
	text.vaddr = data.vaddr = bss.vaddr = 0;

	while ((c = getopt(argc, argv, "dOs")) != -1) {
		switch (c) {
		case 'd':
			debug++;
			break;
		case 's':
			symflag = 1;
			break;
		case 'O':
			magic = OMAGIC;
			break;
		case '?':
		default:
		usage:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	/* Check args... */
	if (argc != 2)
		goto usage;


	/* Try the input file... */
	if ((infile = open(argv[0], O_RDONLY)) < 0)
		err(EXIT_FAILURE, "Can't open `%s' for read", argv[0]);

	/* Read the header, which is at the beginning of the file... */
	i = read(infile, &ex, sizeof ex);
	if (i != sizeof ex) {
		if (i == -1)
			err(EXIT_FAILURE, "Error reading `%s'", argv[1]);
		else
			errx(EXIT_FAILURE, "End of file reading `%s'", argv[1]);
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
	// Not yet
	if (ex.e_ident[EI_CLASS] == ELFCLASS64)
		errx(EXIT_FAILURE, "Only 32 bit is supported");

	/* Read the program headers... */
	ph = saveRead(infile, ex.e_phoff,
	    (size_t)ex.e_phnum * sizeof(Elf32_Phdr), "ph");
#if TARGET_BYTE_ORDER != BYTE_ORDER
	bswap32_region((int32_t*)ph, sizeof(Elf32_Phdr) * ex.e_phnum);
#endif
	/* Read the section headers... */
	sh = saveRead(infile, ex.e_shoff,
	    (size_t)ex.e_shnum * sizeof(Elf32_Shdr), "sh");
#if TARGET_BYTE_ORDER != BYTE_ORDER
	bswap32_region((int32_t*)sh, sizeof(Elf32_Shdr) * ex.e_shnum);
#endif
	/* Read in the section string table. */
	shstrtab = saveRead(infile, sh[ex.e_shstrndx].sh_offset,
	    (size_t)sh[ex.e_shstrndx].sh_size, "shstrtab");

	/* Find space for a table matching ELF section indices to a.out symbol
	 * types. */
	symTypeTable = malloc(ex.e_shnum * sizeof(int));
	if (symTypeTable == NULL)
		err(EXIT_FAILURE, "symTypeTable: can't allocate");
	memset(symTypeTable, 0, ex.e_shnum * sizeof(int));

	/* Look for the symbol table and string table... Also map section
	 * indices to symbol types for a.out */
	for (i = 0; i < ex.e_shnum; i++) {
		char   *name = shstrtab + sh[i].sh_name;
		if (!strcmp(name, ".symtab"))
			symtabix = i;
		else if (!strcmp(name, ".strtab"))
			strtabix = i;
		else
			symTypeTable[i] = get_symtab_type(name);
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
		if (ph[i].p_type == PT_TLS) {
			if (debug)
				warnx("Can't handle TLS section");
			continue;
		}
		if (ph[i].p_type != PT_LOAD)
			errx(EXIT_FAILURE, "Program header %zd "
			    "type %d can't be converted.", i, ph[i].p_type);
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
	    text.vaddr + text.len > data.vaddr ||
	    data.vaddr + data.len > bss.vaddr)
		errx(EXIT_FAILURE, "Sections ordering prevents a.out "
		    "conversion.");
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
	mid = get_mid(&ex);
	aex.a_midmag = (u_long)htobe32(((u_long)symflag << 26)
	    | ((u_long)mid << 16) | magic);

	aex.a_text = text.len;
	aex.a_data = data.len;
	aex.a_bss = bss.len;
	aex.a_entry = ex.e_entry;
	aex.a_syms = (sizeof(struct nlist) *
	    (symtabix != -1 ? sh[symtabix].sh_size / sizeof(Elf32_Sym) : 0));
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
	if ((outfile = open(argv[1], O_WRONLY | O_CREAT, 0777)) < 0)
		err(EXIT_FAILURE, "Unable to create `%s'", argv[1]);
	/* Truncate file... */
	if (ftruncate(outfile, 0)) {
		warn("ftruncate %s", argv[1]);
	}
	/* Write the header... */
	i = write(outfile, &aex, sizeof aex);
	if (i != sizeof aex)
		err(EXIT_FAILURE, "Can't write `%s'", argv[1]);
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
					errx(EXIT_FAILURE,
			"Intersegment gap (%u bytes) too large", gap);
				if (debug)
					warnx("%u byte intersegment gap", gap);
				memset(obuf, 0, sizeof obuf);
				while (gap) {
					ssize_t count = write(outfile, obuf,
					    (gap > sizeof obuf
					    ? sizeof obuf : gap));
					if (count < 0)
						err(EXIT_FAILURE,
						    "Error writing gap");
					gap -= (uint32_t)count;
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

	free(ph);
	free(sh);
	free(shstrtab);
	free(symTypeTable);
	/* Looks like we won... */
	return EXIT_SUCCESS;
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
	ssize_t i, remaining, cur;
	char   *oldstrings;
	char   *newstrings, *nsp;
	size_t  newstringsize;
	uint32_t stringsizebuf;

	/* Zero the unused fields in the output buffer.. */
	memset(outbuf, 0, sizeof outbuf);

	/* Find number of symbols to process... */
	remaining = (ssize_t)(symsize / (off_t)sizeof(Elf32_Sym));

	/* Suck in the old string table... */
	oldstrings = saveRead(in, stroff, (size_t)strsize, "string table");

	/*
	 * Allocate space for the new one.  We will increase the space if
	 * this is too small
	 */
	newstringsize = (size_t)(strsize + remaining);
	newstrings = malloc(newstringsize);
	if (newstrings == NULL)
		err(EXIT_FAILURE, "No memory for new string table!");
	/* Initialize the table pointer... */
	nsp = newstrings;

	/* Go the start of the ELF symbol table... */
	if (lseek(in, symoff, SEEK_SET) < 0)
		err(EXIT_FAILURE, "Can't seek");
	/* Translate and copy symbols... */
	for (; remaining; remaining -= cur) {
		cur = remaining;
		if (cur > SYMS_PER_PASS)
			cur = SYMS_PER_PASS;
		if ((i = read(in, inbuf, (size_t)cur * sizeof(Elf32_Sym)))
		    != cur * (ssize_t)sizeof(Elf32_Sym)) {
			if (i < 0)
				err(EXIT_FAILURE, "%s: read error", __func__);
			else
				errx(EXIT_FAILURE, "%s: premature end of file",
					__func__);
		}
		/* Do the translation... */
		for (i = 0; i < cur; i++) {
			int     binding, type;
			size_t off, len;

#if TARGET_BYTE_ORDER != BYTE_ORDER
			inbuf[i].st_name  = bswap32(inbuf[i].st_name);
			inbuf[i].st_value = bswap32(inbuf[i].st_value);
			inbuf[i].st_size  = bswap32(inbuf[i].st_size);
			inbuf[i].st_shndx = bswap16(inbuf[i].st_shndx);
#endif
			off = (size_t)(nsp - newstrings);

			/* length of this symbol with leading '_' and trailing '\0' */
			len = strlen(oldstrings + inbuf[i].st_name) + 1 + 1;

			/* Does it fit? If not make more space */
			if (newstringsize - off < len) {
				char *nns;

				newstringsize += (size_t)(remaining) * len;
				nns = realloc(newstrings, newstringsize);
				if (nns == NULL)
					err(EXIT_FAILURE, "No memory for new string table!");
				newstrings = nns;
				nsp = newstrings + off;
			}
			/* Copy the symbol into the new table, but prepend an
			 * underscore. */
			*nsp = '_';
			strcpy(nsp + 1, oldstrings + inbuf[i].st_name);
			outbuf[i].n_un.n_strx = nsp - newstrings + 4;
			nsp += len;

			type = ELF32_ST_TYPE(inbuf[i].st_info);
			binding = ELF32_ST_BIND(inbuf[i].st_info);

			/* Convert ELF symbol type/section/etc info into a.out
			 * type info. */
			if (type == STT_FILE)
				outbuf[i].n_type = N_FN;
			else
				outbuf[i].n_type = get_type(inbuf[i].st_shndx);
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
		if ((i = write(out, outbuf, (size_t)cur * sizeof(struct nlist)))
		    != cur * (ssize_t)sizeof(struct nlist))
			err(EXIT_FAILURE, "%s: write failed", __func__);
	}
	/* Write out the string table length... */
	stringsizebuf = (uint32_t)newstringsize;
#if TARGET_BYTE_ORDER != BYTE_ORDER
	stringsizebuf = bswap32(stringsizebuf);
#endif
	if (write(out, &stringsizebuf, sizeof stringsizebuf)
	    != sizeof stringsizebuf)
		err(EXIT_FAILURE, "%s: newstringsize: write failed", __func__);
	/* Write out the string table... */
	if (write(out, newstrings, newstringsize) != (ssize_t)newstringsize)
		err(EXIT_FAILURE, "%s: newstrings: write failed", __func__);
	free(newstrings);
	free(oldstrings);
}

static void
copy(int out, int in, off_t offset, off_t size)
{
	char    ibuf[4096];
	ssize_t remaining, cur, count;

	/* Go to the start of the segment... */
	if (lseek(in, offset, SEEK_SET) < 0)
		err(EXIT_FAILURE, "%s: lseek failed", __func__);
	if (size > SSIZE_MAX)
		err(EXIT_FAILURE, "%s: can not copy this much", __func__);
	remaining = (ssize_t)size;
	while (remaining) {
		cur = remaining;
		if (cur > (int)sizeof ibuf)
			cur = sizeof ibuf;
		remaining -= cur;
		if ((count = read(in, ibuf, (size_t)cur)) != cur) {
			if (count < 0)
				err(EXIT_FAILURE, "%s: read error", __func__);
			else
				errx(EXIT_FAILURE, "%s: premature end of file",
					__func__);
		}
		if ((count = write(out, ibuf, (size_t)cur)) != cur)
			err(EXIT_FAILURE, "%s: write failed", __func__);
	}
}

/* Combine two segments, which must be contiguous.   If pad is true, it's
   okay for there to be padding between. */
static void
combine(struct sect *base, struct sect *new, int pad)
{

	if (base->len == 0)
		*base = *new;
	else
		if (new->len) {
			if (base->vaddr + base->len != new->vaddr) {
				if (pad)
					base->len = new->vaddr - base->vaddr;
				else
					errx(EXIT_FAILURE, "Non-contiguous "
					    "data can't be converted");
			}
			base->len += new->len;
		}
}

static int
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

static void *
saveRead(int file, off_t offset, size_t len, const char *name)
{
	char   *tmp;
	ssize_t count;
	off_t   off;

	if ((off = lseek(file, offset, SEEK_SET)) < 0)
		errx(EXIT_FAILURE, "%s: seek failed", name);
	if ((tmp = malloc(len)) == NULL)
		errx(EXIT_FAILURE,
		    "%s: Can't allocate %jd bytes.", name, (intmax_t)len);
	count = read(file, tmp, len);
	if ((size_t)count != len) {
		if (count < 0)
			err(EXIT_FAILURE, "%s: read error", name);
		else
			errx(EXIT_FAILURE, "%s: premature end of file",
			    name);
	}
	return tmp;
}

#if TARGET_BYTE_ORDER != BYTE_ORDER
/* swap a 32bit region */
static void
bswap32_region(int32_t* p, int len)
{
	size_t i;

	for (i = 0; i < len / sizeof(int32_t); i++, p++)
		*p = bswap32(*p);
}
#endif
