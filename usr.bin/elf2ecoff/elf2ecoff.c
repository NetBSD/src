/*	$NetBSD: elf2ecoff.c,v 1.22 2006/05/31 08:09:55 simonb Exp $	*/

/*
 * Copyright (c) 1997 Jonathan Stone
 *    All rights reserved.
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

/* elf2ecoff.c

   This program converts an elf executable to an ECOFF executable.
   No symbol table is retained.   This is useful primarily in building
   net-bootable kernels for machines (e.g., DECstation and Alpha) which
   only support the ECOFF object file format. */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/exec_elf.h>
#include <stdio.h>
#include <sys/exec_ecoff.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define	ISLAST(p)	(p->n_un.n_name == 0 || p->n_un.n_name[0] == 0)

struct sect {
	unsigned long vaddr;
	unsigned long len;
};

struct elf_syms {
	int     nsymbols;
	Elf32_Sym *elf_syms;
	off_t   stringsize;
	char   *stringtab;
};

struct ecoff_syms {
	int     nsymbols;
	struct ecoff_extsym *ecoff_syms;
	off_t   stringsize;
	char   *stringtab;
};

int     debug = 0;

int     phcmp(Elf32_Phdr * h1, Elf32_Phdr * h2);


char   *saveRead(int file, off_t offset, off_t len, char *name);
void    safewrite(int outfile, void *buf, off_t len, const char *msg);
void    copy(int, int, off_t, off_t);
void    combine(struct sect * base, struct sect * new, int paddable);
void    translate_syms(struct elf_syms *, struct ecoff_syms *);
void 
elf_symbol_table_to_ecoff(int out, int in,
    struct ecoff_exechdr * ep,
    off_t symoff, off_t symsize,
    off_t stroff, off_t strsize);


int 
make_ecoff_section_hdrs(struct ecoff_exechdr * ep,
    struct ecoff_scnhdr * esecs);

void 
write_ecoff_symhdr(int outfile, struct ecoff_exechdr * ep,
    struct ecoff_symhdr * symhdrp,
    long nesyms, long extsymoff, long extstroff,
    long strsize);

void    pad16(int fd, int size, const char *msg);
void	bswap32_region(int32_t* , int);

int    *symTypeTable;
int	needswap;




void
elf_read_syms(struct elf_syms * elfsymsp, int infile,
    off_t symoff, off_t symsize, off_t stroff, off_t strsize);


int
main(int argc, char **argv, char **envp)
{
	Elf32_Ehdr ex;
	Elf32_Phdr *ph;
	Elf32_Shdr *sh;
	char   *shstrtab;
	int     strtabix, symtabix;
	int     i, pad;
	struct sect text, data, bss;	/* a.out-compatible sections */
	struct sect rdata, sdata, sbss;	/* ECOFF-only sections */

	struct ecoff_exechdr ep;
	struct ecoff_scnhdr esecs[6];
	struct ecoff_symhdr symhdr;

	int     infile, outfile;
	unsigned long cur_vma = ULONG_MAX;
	int     symflag = 0;
	int     nsecs = 0;
	int	mipsel;


	text.len = data.len = bss.len = 0;
	text.vaddr = data.vaddr = bss.vaddr = 0;

	rdata.len = sdata.len = sbss.len = 0;
	rdata.vaddr = sdata.vaddr = sbss.vaddr = 0;

	/* Check args... */
	if (argc < 3 || argc > 4) {
usage:
		fprintf(stderr,
		    "usage: elf2ecoff <elf executable> <ECOFF executable> [-s]\n");
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
	if (ex.e_ident[EI_DATA] == ELFDATA2LSB)
		mipsel = 1;
	else if (ex.e_ident[EI_DATA] == ELFDATA2MSB)
		mipsel = 0;
	else {
		fprintf(stderr, "invalid ELF byte order %d\n",
		    ex.e_ident[EI_DATA]);
		exit(1);
	}
#if BYTE_ORDER == BIG_ENDIAN
	if (mipsel)
		needswap = 1;
	else
		needswap = 0;
#elif BYTE_ORDER == LITTLE_ENDIAN
	if (mipsel)
		needswap = 0;
	else
		needswap = 1;
#else
#error "unknown endian"
#endif

	if (needswap) {
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
	}

	/* Read the program headers... */
	ph = (Elf32_Phdr *) saveRead(infile, ex.e_phoff,
	    ex.e_phnum * sizeof(Elf32_Phdr), "ph");
	if (needswap)
		bswap32_region((int32_t*)ph, sizeof(Elf32_Phdr) * ex.e_phnum);
	/* Read the section headers... */
	sh = (Elf32_Shdr *) saveRead(infile, ex.e_shoff,
	    ex.e_shnum * sizeof(Elf32_Shdr), "sh");
	if (needswap) 
		bswap32_region((int32_t*)sh, sizeof(Elf32_Shdr) * ex.e_shnum);

	/* Read in the section string table. */
	shstrtab = saveRead(infile, sh[ex.e_shstrndx].sh_offset,
	    sh[ex.e_shstrndx].sh_size, "shstrtab");


	/* Look for the symbol table and string table... Also map section
	 * indices to symbol types for a.out */
	symtabix = 0;
	strtabix = 0;
	for (i = 0; i < ex.e_shnum; i++) {
		char   *name = shstrtab + sh[i].sh_name;
		if (!strcmp(name, ".symtab"))
			symtabix = i;
		else
			if (!strcmp(name, ".strtab"))
				strtabix = i;

	}

	/* Figure out if we can cram the program header into an ECOFF
	 * header...  Basically, we can't handle anything but loadable
	 * segments, but we can ignore some kinds of segments.  We can't
	 * handle holes in the address space.  Segments may be out of order,
	 * so we sort them first. */

	qsort(ph, ex.e_phnum, sizeof(Elf32_Phdr),
	    (int (*) (const void *, const void *)) phcmp);

	for (i = 0; i < ex.e_phnum; i++) {
		/* Section types we can ignore... */
		if (ph[i].p_type == PT_NULL || ph[i].p_type == PT_NOTE ||
		    ph[i].p_type == PT_PHDR ||
		    ph[i].p_type == PT_MIPS_REGINFO) {

			if (debug) {
				fprintf(stderr, "  skipping PH %d type %d flags 0x%x\n",
				    i, ph[i].p_type, ph[i].p_flags);
			}
			continue;
		}
		/* Section types we can't handle... */
		else
			if (ph[i].p_type != PT_LOAD) {
				fprintf(stderr, "Program header %d type %d can't be converted.\n",
				    i, ph[i].p_type);
				exit(1);
			}
		/* Writable (data) segment? */
		if (ph[i].p_flags & PF_W) {
			struct sect ndata, nbss;

			ndata.vaddr = ph[i].p_vaddr;
			ndata.len = ph[i].p_filesz;
			nbss.vaddr = ph[i].p_vaddr + ph[i].p_filesz;
			nbss.len = ph[i].p_memsz - ph[i].p_filesz;

			if (debug) {
				fprintf(stderr,
				    "  combinining PH %d type %d flags 0x%x with data, ndata = %ld, nbss =%ld\n", i, ph[i].p_type, ph[i].p_flags, ndata.len, nbss.len);
			}
			combine(&data, &ndata, 0);
			combine(&bss, &nbss, 1);
		} else {
			struct sect ntxt;

			ntxt.vaddr = ph[i].p_vaddr;
			ntxt.len = ph[i].p_filesz;
			if (debug) {

				fprintf(stderr,
				    "  combinining PH %d type %d flags 0x%x with text, len = %ld\n",
				    i, ph[i].p_type, ph[i].p_flags, ntxt.len);
			}
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
	if (data.len && !text.len) {
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
	ep.a.magic = ECOFF_OMAGIC;
	ep.a.vstamp = 2 * 256 + 10;	/* compatible with version 2.10 */
	ep.a.tsize = text.len;
	ep.a.dsize = data.len;
	ep.a.bsize = bss.len;
	ep.a.entry = ex.e_entry;
	ep.a.text_start = text.vaddr;
	ep.a.data_start = data.vaddr;
	ep.a.bss_start = bss.vaddr;
	ep.a.gprmask = 0xf3fffffe;
	memset(&ep.a.cprmask, 0, sizeof ep.a.cprmask);
	ep.a.gp_value = 0;	/* unused. */

	if (mipsel)
		ep.f.f_magic = ECOFF_MAGIC_MIPSEL;
	else 
		ep.f.f_magic = ECOFF_MAGIC_MIPSEB;

	ep.f.f_nscns = 6;
	ep.f.f_timdat = 0;	/* bogus */
	ep.f.f_symptr = 0;
	ep.f.f_nsyms = sizeof(struct ecoff_symhdr);
	ep.f.f_opthdr = sizeof ep.a;
	ep.f.f_flags = 0x100f;	/* Stripped, not sharable. */

	memset(esecs, 0, sizeof(esecs));

	/* Make  ECOFF section headers, with empty stubs for
	 * .rdata/.sdata/.sbss. */
	make_ecoff_section_hdrs(&ep, esecs);

	nsecs = ep.f.f_nscns;

	if (needswap) {
		ep.f.f_magic	= bswap16(ep.f.f_magic);
		ep.f.f_nscns	= bswap16(ep.f.f_nscns);
		ep.f.f_timdat	= bswap32(ep.f.f_timdat);
		ep.f.f_symptr	= bswap32(ep.f.f_symptr);
		ep.f.f_nsyms	= bswap32(ep.f.f_nsyms);
		ep.f.f_opthdr	= bswap16(ep.f.f_opthdr);
		ep.f.f_flags	= bswap16(ep.f.f_flags);
		ep.a.magic	= bswap16(ep.a.magic);
		ep.a.vstamp	= bswap16(ep.a.vstamp);
		ep.a.tsize	= bswap32(ep.a.tsize);
		ep.a.dsize	= bswap32(ep.a.dsize);
		ep.a.bsize	= bswap32(ep.a.bsize);
		ep.a.entry	= bswap32(ep.a.entry);
		ep.a.text_start	= bswap32(ep.a.text_start);
		ep.a.data_start	= bswap32(ep.a.data_start);
		ep.a.bss_start	= bswap32(ep.a.bss_start);
		ep.a.gprmask	= bswap32(ep.a.gprmask);
		bswap32_region((int32_t*)ep.a.cprmask, sizeof(ep.a.cprmask));
		ep.a.gp_value	= bswap32(ep.a.gp_value);
		for (i = 0; i < sizeof(esecs) / sizeof(esecs[0]); i++) {
			esecs[i].s_paddr	= bswap32(esecs[i].s_paddr);
			esecs[i].s_vaddr	= bswap32(esecs[i].s_vaddr);
			esecs[i].s_size 	= bswap32(esecs[i].s_size);
			esecs[i].s_scnptr	= bswap32(esecs[i].s_scnptr);
			esecs[i].s_relptr	= bswap32(esecs[i].s_relptr);
			esecs[i].s_lnnoptr	= bswap32(esecs[i].s_lnnoptr);
			esecs[i].s_nreloc	= bswap16(esecs[i].s_nreloc);
			esecs[i].s_nlnno	= bswap16(esecs[i].s_nlnno);
			esecs[i].s_flags	= bswap32(esecs[i].s_flags);
		}
	}

	/* Make the output file... */
	if ((outfile = open(argv[2], O_WRONLY | O_CREAT, 0777)) < 0) {
		fprintf(stderr, "Unable to create %s: %s\n", argv[2], strerror(errno));
		exit(1);
	}
	/* Truncate file... */
	if (ftruncate(outfile, 0)) {
		warn("ftruncate %s", argv[2]);
	}
	/* Write the headers... */
	safewrite(outfile, &ep.f, sizeof(ep.f), "ep.f: write: %s\n");
	if (debug)
		fprintf(stderr, "wrote %d byte file header.\n", sizeof(ep.f));

	safewrite(outfile, &ep.a, sizeof(ep.a), "ep.a: write: %s\n");
	if (debug)
		fprintf(stderr, "wrote %d byte a.out header.\n", sizeof(ep.a));

	safewrite(outfile, &esecs, sizeof(esecs[0]) * nsecs,
	    "esecs: write: %s\n");
	if (debug)
		fprintf(stderr, "wrote %d bytes of section headers.\n",
		    sizeof(esecs[0]) * nsecs);


	pad = ((sizeof ep.f + sizeof ep.a + sizeof esecs) & 15);
	if (pad) {
		pad = 16 - pad;
		pad16(outfile, pad, "ipad: write: %s\n");
		if (debug)
			fprintf(stderr, "wrote %d byte pad.\n", pad);
	}
	/* Copy the loadable sections.   Zero-fill any gaps less than 64k;
	 * complain about any zero-filling, and die if we're asked to
	 * zero-fill more than 64k. */
	for (i = 0; i < ex.e_phnum; i++) {
		/* Unprocessable sections were handled above, so just verify
		 * that the section can be loaded before copying. */
		if (ph[i].p_type == PT_LOAD && ph[i].p_filesz) {
			if (cur_vma != ph[i].p_vaddr) {
				unsigned long gap = ph[i].p_vaddr - cur_vma;
				char    obuf[1024];
				if (gap > 65536) {
					fprintf(stderr, "Intersegment gap (%ld bytes) too large.\n",
					    gap);
					exit(1);
				}
				if (debug)
					fprintf(stderr, "Warning: %ld byte intersegment gap.\n", gap);
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
			if (debug)
				fprintf(stderr, "writing %d bytes...\n", ph[i].p_filesz);
			copy(outfile, infile, ph[i].p_offset, ph[i].p_filesz);
			cur_vma = ph[i].p_vaddr + ph[i].p_filesz;
		}
	}


	if (debug)
		fprintf(stderr, "writing syms at offset 0x%lx\n",
		    (u_long) ep.f.f_symptr + sizeof(symhdr));

	/* Copy and translate the symbol table... */
	elf_symbol_table_to_ecoff(outfile, infile, &ep,
	    sh[symtabix].sh_offset, sh[symtabix].sh_size,
	    sh[strtabix].sh_offset, sh[strtabix].sh_size);

	/*
         * Write a page of padding for boot PROMS that read entire pages.
         * Without this, they may attempt to read past the end of the
         * data section, incur an error, and refuse to boot.
         */
	{
		char    obuf[4096];
		memset(obuf, 0, sizeof obuf);
		if (write(outfile, obuf, sizeof(obuf)) != sizeof(obuf)) {
			fprintf(stderr, "Error writing PROM padding: %s\n",
			    strerror(errno));
			exit(1);
		}
	}

	/* Looks like we won... */
	exit(0);
}

void
copy(out, in, offset, size)
	int     out, in;
	off_t   offset, size;
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
		if (cur > sizeof ibuf)
			cur = sizeof ibuf;
		remaining -= cur;
		if ((count = read(in, ibuf, cur)) != cur) {
			fprintf(stderr, "copy: read: %s\n",
			    count ? strerror(errno) : "premature end of file");
			exit(1);
		}
		safewrite(out, ibuf, cur, "copy: write: %s\n");
	}
}
/* Combine two segments, which must be contiguous.   If pad is true, it's
   okay for there to be padding between. */
void
combine(base, new, pad)
	struct sect *base, *new;
	int     pad;
{
	if (!base->len)
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
phcmp(h1, h2)
	Elf32_Phdr *h1, *h2;
{
	if (h1->p_vaddr > h2->p_vaddr)
		return 1;
	else
		if (h1->p_vaddr < h2->p_vaddr)
			return -1;
		else
			return 0;
}

char
       *
saveRead(int file, off_t offset, off_t len, char *name)
{
	char   *tmp;
	int     count;
	off_t   off;
	if ((off = lseek(file, offset, SEEK_SET)) < 0) {
		fprintf(stderr, "%s: fseek: %s\n", name, strerror(errno));
		exit(1);
	}
	if (!(tmp = (char *) malloc(len))) {
		fprintf(stderr, "%s: Can't allocate %ld bytes.\n", name, (long) len);
		exit(1);
	}
	count = read(file, tmp, len);
	if (count != len) {
		fprintf(stderr, "%s: read: %s.\n",
		    name, count ? strerror(errno) : "End of file reached");
		exit(1);
	}
	return tmp;
}

void
safewrite(int outfile, void *buf, off_t len, const char *msg)
{
	int     written;
	written = write(outfile, (char *) buf, len);
	if (written != len) {
		fprintf(stderr, msg, strerror(errno));
		exit(1);
	}
}


/*
 * Output only three ECOFF sections, corresponding to ELF psecs
 * for text, data, and bss.
 */
int
make_ecoff_section_hdrs(ep, esecs)
	struct ecoff_exechdr *ep;
	struct ecoff_scnhdr *esecs;

{
	ep->f.f_nscns = 6;	/* XXX */

	strcpy(esecs[0].s_name, ".text");
	strcpy(esecs[1].s_name, ".data");
	strcpy(esecs[2].s_name, ".bss");

	esecs[0].s_paddr = esecs[0].s_vaddr = ep->a.text_start;
	esecs[1].s_paddr = esecs[1].s_vaddr = ep->a.data_start;
	esecs[2].s_paddr = esecs[2].s_vaddr = ep->a.bss_start;
	esecs[0].s_size = ep->a.tsize;
	esecs[1].s_size = ep->a.dsize;
	esecs[2].s_size = ep->a.bsize;

	esecs[0].s_scnptr = ECOFF_TXTOFF(ep);
	esecs[1].s_scnptr = ECOFF_DATOFF(ep);
#if 0
	esecs[2].s_scnptr = esecs[1].s_scnptr +
	    ECOFF_ROUND(esecs[1].s_size, ECOFF_SEGMENT_ALIGNMENT(ep));
#endif

	esecs[0].s_relptr = esecs[1].s_relptr = esecs[2].s_relptr = 0;
	esecs[0].s_lnnoptr = esecs[1].s_lnnoptr = esecs[2].s_lnnoptr = 0;
	esecs[0].s_nreloc = esecs[1].s_nreloc = esecs[2].s_nreloc = 0;
	esecs[0].s_nlnno = esecs[1].s_nlnno = esecs[2].s_nlnno = 0;

	esecs[1].s_flags = 0x100;	/* ECOFF rdata */
	esecs[3].s_flags = 0x200;	/* ECOFF sdata */
	esecs[4].s_flags = 0x400;	/* ECOFF sbss */

	/*
	 * Set the symbol-table offset  to point at the end of any
	 * sections we loaded above, so later code can use it to write
	 * symbol table info..
	 */
	ep->f.f_symptr = esecs[1].s_scnptr + esecs[1].s_size;
	return (ep->f.f_nscns);
}


/*
 * Write the ECOFF symbol header.
 * Guess at how big the symbol table will be.
 * Mark all symbols as EXTERN (for now).
 */
void
write_ecoff_symhdr(out, ep, symhdrp, nesyms, extsymoff, extstroff, strsize)
	int     out;
	struct ecoff_exechdr *ep;
	struct ecoff_symhdr *symhdrp;
	long    nesyms, extsymoff, extstroff, strsize;
{
	if (debug)
		fprintf(stderr, "writing symhdr for %ld entries at offset 0x%lx\n",
		    nesyms, (u_long) ep->f.f_symptr);

	ep->f.f_nsyms = sizeof(struct ecoff_symhdr);

	memset(symhdrp, 0, sizeof(*symhdrp));
	symhdrp->esymMax = nesyms;
	symhdrp->magic = 0x7009;/* XXX */
	symhdrp->cbExtOffset = extsymoff;
	symhdrp->cbSsExtOffset = extstroff;

	symhdrp->issExtMax = strsize;
	if (debug)
		fprintf(stderr,
		    "ECOFF symhdr: symhdr %x, strsize %lx, symsize %lx\n",
		    sizeof(*symhdrp), strsize,
		    (nesyms * sizeof(struct ecoff_extsym)));

	if (needswap) {
		bswap32_region(&symhdrp->ilineMax,
		    sizeof(*symhdrp) -  sizeof(symhdrp->magic) -
		    sizeof(symhdrp->ilineMax));
		symhdrp->magic = bswap16(symhdrp->magic);
		symhdrp->ilineMax = bswap16(symhdrp->ilineMax);
	}
		
	safewrite(out, symhdrp, sizeof(*symhdrp),
	    "writing symbol header: %s\n");
}


void
elf_read_syms(elfsymsp, in, symoff, symsize, stroff, strsize)
	struct elf_syms *elfsymsp;
	int     in;
	off_t   symoff, symsize;
	off_t   stroff, strsize;
{
	register int nsyms;
	int i;
	nsyms = symsize / sizeof(Elf32_Sym);

	/* Suck in the ELF symbol list... */
	elfsymsp->elf_syms = (Elf32_Sym *)
	    saveRead(in, symoff, nsyms * sizeof(Elf32_Sym),
	    "ELF symboltable");
	elfsymsp->nsymbols = nsyms;
	if (needswap) {
		for (i = 0; i < nsyms; i++) {
			Elf32_Sym *s = &elfsymsp->elf_syms[i];
			s->st_name	= bswap32(s->st_name);
			s->st_value	= bswap32(s->st_value);
			s->st_size	= bswap32(s->st_size);
			s->st_shndx	= bswap16(s->st_shndx);
		}
	}

	/* Suck in the ELF string table... */
	elfsymsp->stringtab = (char *)
	    saveRead(in, stroff, strsize, "ELF string table");
	elfsymsp->stringsize = strsize;
}


/*
 *
 */
void
elf_symbol_table_to_ecoff(out, in, ep, symoff, symsize, stroff, strsize)
	int     out, in;
	struct ecoff_exechdr *ep;
	off_t   symoff, symsize;
	off_t   stroff, strsize;
{

	struct elf_syms elfsymtab;
	struct ecoff_syms ecoffsymtab;
	register u_long ecoff_symhdr_off, symtaboff, stringtaboff;
	register u_long nextoff, symtabsize, ecoff_strsize;
	int     nsyms, i;
	struct ecoff_symhdr symhdr;
	int     padding;

	/* Read in the ELF symbols. */
	elf_read_syms(&elfsymtab, in, symoff, symsize, stroff, strsize);

	/* Approximate translation to ECOFF. */
	translate_syms(&elfsymtab, &ecoffsymtab);
	nsyms = ecoffsymtab.nsymbols;

	/* Compute output ECOFF symbol- and string-table offsets. */
	ecoff_symhdr_off = ep->f.f_symptr;

	nextoff = ecoff_symhdr_off + sizeof(struct ecoff_symhdr);
	stringtaboff = nextoff;
	ecoff_strsize = ECOFF_ROUND(ecoffsymtab.stringsize,
	    (ECOFF_SEGMENT_ALIGNMENT(ep)));


	nextoff = stringtaboff + ecoff_strsize;
	symtaboff = nextoff;
	symtabsize = nsyms * sizeof(struct ecoff_extsym);
	symtabsize = ECOFF_ROUND(symtabsize, ECOFF_SEGMENT_ALIGNMENT(ep));

	/* Write out the symbol header ... */
	write_ecoff_symhdr(out, ep, &symhdr, nsyms, symtaboff,
	    stringtaboff, ecoffsymtab.stringsize);

	/* Write out the string table... */
	padding = ecoff_strsize - ecoffsymtab.stringsize;
	safewrite(out, ecoffsymtab.stringtab, ecoffsymtab.stringsize,
	    "string table: write: %s\n");
	if (padding)
		pad16(out, padding, "string table: padding: %s\n");


	/* Write out the symbol table... */
	padding = symtabsize - (nsyms * sizeof(struct ecoff_extsym));

	for (i = 0; i < nsyms; i++) {
		struct ecoff_extsym *es = &ecoffsymtab.ecoff_syms[i];
		es->es_flags	= bswap16(es->es_flags);
		es->es_ifd	= bswap16(es->es_ifd);
		bswap32_region(&es->es_strindex,
		    sizeof(*es) - sizeof(es->es_flags) - sizeof(es->es_ifd));
	}
	safewrite(out, ecoffsymtab.ecoff_syms,
	    nsyms * sizeof(struct ecoff_extsym),
	    "symbol table: write: %s\n");
	if (padding)
		pad16(out, padding, "symbols: padding: %s\n");
}



/*
 * In-memory translation of ELF symbosl to ECOFF.
 */
void
translate_syms(elfp, ecoffp)
	struct elf_syms *elfp;
	struct ecoff_syms *ecoffp;
{

	int     i;
	char   *oldstringbase;
	char   *newstrings, *nsp;

	int     nsyms, idx;

	nsyms = elfp->nsymbols;
	oldstringbase = elfp->stringtab;

	/* Allocate space for corresponding ECOFF symbols. */
	memset(ecoffp, 0, sizeof(*ecoffp));

	ecoffp->nsymbols = 0;
	ecoffp->ecoff_syms = malloc(sizeof(struct ecoff_extsym) * nsyms);

	/* we are going to be no bigger than the ELF symbol table. */
	ecoffp->stringsize = elfp->stringsize;
	ecoffp->stringtab = malloc(elfp->stringsize);

	newstrings = (char *) ecoffp->stringtab;
	nsp = (char *) ecoffp->stringtab;
	if (!newstrings) {
		fprintf(stderr, "No memory for new string table!\n");
		exit(1);
	}
	/* Copy and translate  symbols... */
	idx = 0;
	for (i = 0; i < nsyms; i++) {
		int     binding, type;

		binding = ELF32_ST_BIND((elfp->elf_syms[i].st_info));
		type = ELF32_ST_TYPE((elfp->elf_syms[i].st_info));

		/* skip strange symbols */
		if (binding == 0) {
			continue;
		}
		/* Copy the symbol into the new table */
		strcpy(nsp, oldstringbase + elfp->elf_syms[i].st_name);
		ecoffp->ecoff_syms[idx].es_strindex = nsp - newstrings;
		nsp += strlen(nsp) + 1;

		/* translate symbol types to ECOFF XXX */
		ecoffp->ecoff_syms[idx].es_type = 1;
		ecoffp->ecoff_syms[idx].es_class = 5;

		/* Symbol values in executables should be compatible. */
		ecoffp->ecoff_syms[idx].es_value = elfp->elf_syms[i].st_value;
		ecoffp->ecoff_syms[idx].es_symauxindex = 0xfffff;

		idx++;
	}

	ecoffp->nsymbols = idx;
	ecoffp->stringsize = nsp - newstrings;
}
/*
 * pad to a 16-byte boundary
 */
void
pad16(int fd, int size, const char *msg)
{
	safewrite(fd, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0", size, msg);
}

/* swap a 32bit region */
void
bswap32_region(int32_t* p, int len)
{
	int i;

	for (i = 0; i < len / sizeof(int32_t); i++, p++)
		*p = bswap32(*p);
}
