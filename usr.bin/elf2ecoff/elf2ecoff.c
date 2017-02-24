/*	$NetBSD: elf2ecoff.c,v 1.33 2017/02/24 17:19:14 christos Exp $	*/

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
	uint32_t vaddr;
	uint32_t len;
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

static int     debug = 0;
static int     needswap;

static int     phcmp(Elf32_Phdr *, Elf32_Phdr *);
static char   *saveRead(int, off_t, off_t, const char *);
static void    safewrite(int, const void *, off_t, const char *);
static void    copy(int, int, off_t, off_t);
static void    combine(struct sect *, struct sect *, int);
static void    translate_syms(struct elf_syms *, struct ecoff_syms *);
static void    elf_symbol_table_to_ecoff(int, int, struct ecoff32_exechdr *,
    off_t, off_t, off_t, off_t);
static int     make_ecoff_section_hdrs(struct ecoff32_exechdr *,
    struct ecoff32_scnhdr *);
static void    write_ecoff_symhdr(int, struct ecoff32_exechdr *,
    struct ecoff32_symhdr *, int32_t, int32_t, int32_t, int32_t);
static void    pad16(int, int, const char *);
static void    bswap32_region(int32_t* , int);
static void    elf_read_syms(struct elf_syms *, int, off_t, off_t, off_t,
    off_t);


int
main(int argc, char **argv)
{
	Elf32_Ehdr ex;
	Elf32_Phdr *ph;
	Elf32_Shdr *sh;
	char   *shstrtab;
	int     strtabix, symtabix;
	size_t	i;
	int     pad;
	struct sect text, data, bss;	/* a.out-compatible sections */

	struct ecoff32_exechdr ep;
	struct ecoff32_scnhdr esecs[6];
	struct ecoff32_symhdr symhdr;

	int     infile, outfile;
	uint32_t cur_vma = UINT32_MAX;
	int     nsecs = 0;
	int	mipsel;


	text.len = data.len = bss.len = 0;
	text.vaddr = data.vaddr = bss.vaddr = 0;

	/* Check args... */
	if (argc < 3 || argc > 4) {
usage:
		fprintf(stderr,
		    "Usage: %s <elf executable> <ECOFF executable> [-s]\n",
		    getprogname());
		exit(1);
	}
	if (argc == 4) {
		if (strcmp(argv[3], "-s"))
			goto usage;
	}
	/* Try the input file... */
	if ((infile = open(argv[1], O_RDONLY)) < 0)
		err(1, "Can't open %s for read", argv[1]);
	/* Read the header, which is at the beginning of the file... */
	i = read(infile, &ex, sizeof ex);
	if (i != sizeof ex)
		err(1, "Short header read from %s", argv[1]);
	if (ex.e_ident[EI_DATA] == ELFDATA2LSB)
		mipsel = 1;
	else if (ex.e_ident[EI_DATA] == ELFDATA2MSB)
		mipsel = 0;
	else
		errx(1, "invalid ELF byte order %d", ex.e_ident[EI_DATA]);
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

	/*
	 * Figure out if we can cram the program header into an ECOFF
	 * header...  Basically, we can't handle anything but loadable
	 * segments, but we can ignore some kinds of segments.  We can't
	 * handle holes in the address space.  Segments may be out of order,
	 * so we sort them first.
	 */

	qsort(ph, ex.e_phnum, sizeof(Elf32_Phdr),
	    (int (*) (const void *, const void *)) phcmp);

	for (i = 0; i < ex.e_phnum; i++) {
		switch (ph[i].p_type) {
		case PT_NOTE:
		case PT_NULL:
		case PT_PHDR:
		case PT_MIPS_ABIFLAGS:
		case PT_MIPS_REGINFO:
			/* Section types we can ignore... */
			if (debug) {
				fprintf(stderr, "  skipping PH %zu type %#x "
				    "flags %#x\n",
				    i, ph[i].p_type, ph[i].p_flags);
			}
			continue;
		default:
			/* Section types we can't handle... */
			if (ph[i].p_type != PT_LOAD)
				errx(1, "Program header %zu type %#x can't be "
				    "converted", i, ph[i].p_type);
		}
		/* Writable (data) segment? */
		if (ph[i].p_flags & PF_W) {
			struct sect ndata, nbss;

			ndata.vaddr = ph[i].p_vaddr;
			ndata.len = ph[i].p_filesz;
			nbss.vaddr = ph[i].p_vaddr + ph[i].p_filesz;
			nbss.len = ph[i].p_memsz - ph[i].p_filesz;

			if (debug) {
				fprintf(stderr, "  combinining PH %zu type %d "
				    "flags %#x with data, ndata = %d, "
				    "nbss =%d\n", i, ph[i].p_type,
				    ph[i].p_flags, ndata.len, nbss.len);
			}
			combine(&data, &ndata, 0);
			combine(&bss, &nbss, 1);
		} else {
			struct sect ntxt;

			ntxt.vaddr = ph[i].p_vaddr;
			ntxt.len = ph[i].p_filesz;
			if (debug) {
				fprintf(stderr, "  combinining PH %zu type %d "
				    "flags %#x with text, len = %d\n",
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
	    text.vaddr + text.len > data.vaddr ||
	    data.vaddr + data.len > bss.vaddr)
		errx(1, "Sections ordering prevents a.out conversion");
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
	ep.f.f_nsyms = sizeof(struct ecoff32_symhdr);
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
	if ((outfile = open(argv[2], O_WRONLY | O_CREAT, 0777)) < 0)
		err(1, "Unable to create %s", argv[2]);

	/* Truncate file... */
	if (ftruncate(outfile, 0)) {
		warn("ftruncate %s", argv[2]);
	}
	/* Write the headers... */
	safewrite(outfile, &ep.f, sizeof(ep.f), "ep.f: write");
	if (debug)
		fprintf(stderr, "wrote %zu byte file header.\n", sizeof(ep.f));

	safewrite(outfile, &ep.a, sizeof(ep.a), "ep.a: write");
	if (debug)
		fprintf(stderr, "wrote %zu byte a.out header.\n", sizeof(ep.a));

	safewrite(outfile, &esecs, sizeof(esecs[0]) * nsecs, "esecs: write");
	if (debug)
		fprintf(stderr, "wrote %zu bytes of section headers.\n",
		    sizeof(esecs[0]) * nsecs);


	pad = ((sizeof ep.f + sizeof ep.a + sizeof esecs) & 15);
	if (pad) {
		pad = 16 - pad;
		pad16(outfile, pad, "ipad: write");
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
				uint32_t gap = ph[i].p_vaddr - cur_vma;
				char    obuf[1024];
				if (gap > 65536)
					errx(1, "Intersegment gap (%d bytes) "
					    "too large", gap);
				if (debug)
					fprintf(stderr, "Warning: %d byte "
					    "intersegment gap.\n", gap);
				memset(obuf, 0, sizeof obuf);
				while (gap) {
					int count = write(outfile, obuf,
					    (gap > sizeof obuf
					    ? sizeof obuf : gap));
					if (count < 0)
						err(1, "Error writing gap");
					gap -= count;
				}
			}
			if (debug)
				fprintf(stderr, "writing %d bytes...\n",
				    ph[i].p_filesz);
			copy(outfile, infile, ph[i].p_offset, ph[i].p_filesz);
			cur_vma = ph[i].p_vaddr + ph[i].p_filesz;
		}
	}


	if (debug)
		fprintf(stderr, "writing syms at offset %#x\n",
		    (uint32_t)(ep.f.f_symptr + sizeof(symhdr)));

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
		if (write(outfile, obuf, sizeof(obuf)) != sizeof(obuf))
			err(1, "Error writing PROM padding");
	}

	/* Looks like we won... */
	return 0;
}

static void
copy(int out, int in, off_t offset, off_t size)
{
	char    ibuf[4096];
	size_t  remaining, cur, count;

	/* Go to the start of the ELF symbol table... */
	if (lseek(in, offset, SEEK_SET) < 0)
		err(1, "copy: lseek");
	remaining = size;
	while (remaining) {
		cur = remaining;
		if (cur > sizeof ibuf)
			cur = sizeof ibuf;
		remaining -= cur;
		if ((count = read(in, ibuf, cur)) != cur)
			err(1, "copy: short read");
		safewrite(out, ibuf, cur, "copy: write");
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
					errx(1, "Non-contiguous data can't be "
					    "converted");
			}
			base->len += new->len;
		}
}

static int
phcmp(Elf32_Phdr *h1, Elf32_Phdr *h2)
{

	if (h1->p_vaddr > h2->p_vaddr)
		return 1;
	else
		if (h1->p_vaddr < h2->p_vaddr)
			return -1;
		else
			return 0;
}

static char *
saveRead(int file, off_t offset, off_t len, const char *name)
{
	char   *tmp;
	int     count;
	off_t   off;

	if ((off = lseek(file, offset, SEEK_SET)) < 0)
		err(1, "%s: fseek", name);
	if ((tmp = malloc(len)) == NULL)
		err(1, "%s: Can't allocate %jd bytes", name, (intmax_t)len);
	count = read(file, tmp, len);
	if (count != len)
		err(1, "%s: short read", name);
	return tmp;
}

static void
safewrite(int outfile, const void *buf, off_t len, const char *msg)
{
	ssize_t     written;

	written = write(outfile, buf, len);
	if (written != len)
		err(1, "%s", msg);
}


/*
 * Output only three ECOFF sections, corresponding to ELF psecs
 * for text, data, and bss.
 */
static int
make_ecoff_section_hdrs(struct ecoff32_exechdr *ep, struct ecoff32_scnhdr *esecs)
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

	esecs[0].s_scnptr = ECOFF32_TXTOFF(ep);
	esecs[1].s_scnptr = ECOFF32_DATOFF(ep);
#if 0
	esecs[2].s_scnptr = esecs[1].s_scnptr +
	    ECOFF_ROUND(esecs[1].s_size, ECOFF32_SEGMENT_ALIGNMENT(ep));
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
static void
write_ecoff_symhdr(int out, struct ecoff32_exechdr *ep,
    struct ecoff32_symhdr *symhdrp, int32_t nesyms,
    int32_t extsymoff, int32_t extstroff, int32_t strsize)
{

	if (debug)
		fprintf(stderr,
		    "writing symhdr for %d entries at offset %#x\n",
		    nesyms, ep->f.f_symptr);

	ep->f.f_nsyms = sizeof(struct ecoff32_symhdr);

	memset(symhdrp, 0, sizeof(*symhdrp));
	symhdrp->esymMax = nesyms;
	symhdrp->magic = 0x7009;/* XXX */
	symhdrp->cbExtOffset = extsymoff;
	symhdrp->cbSsExtOffset = extstroff;

	symhdrp->issExtMax = strsize;
	if (debug)
		fprintf(stderr,
		    "ECOFF symhdr: symhdr %zx, strsize %x, symsize %zx\n",
		    sizeof(*symhdrp), strsize,
		    (nesyms * sizeof(struct ecoff32_extsym)));

	if (needswap) {
		bswap32_region(&symhdrp->ilineMax,
		    sizeof(*symhdrp) -  sizeof(symhdrp->magic) -
		    sizeof(symhdrp->ilineMax));
		symhdrp->magic = bswap16(symhdrp->magic);
		symhdrp->ilineMax = bswap16(symhdrp->ilineMax);
	}

	safewrite(out, symhdrp, sizeof(*symhdrp),
	    "writing symbol header");
}


static void
elf_read_syms(struct elf_syms *elfsymsp, int in, off_t symoff, off_t symsize,
    off_t stroff, off_t strsize)
{
	int nsyms;
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


static void
elf_symbol_table_to_ecoff(int out, int in, struct ecoff32_exechdr *ep,
    off_t symoff, off_t symsize, off_t stroff, off_t strsize)
{

	struct elf_syms elfsymtab;
	struct ecoff_syms ecoffsymtab;
	uint32_t ecoff_symhdr_off, symtaboff, stringtaboff;
	uint32_t nextoff, symtabsize, ecoff_strsize;
	int     nsyms, i;
	struct ecoff32_symhdr symhdr;
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
	    (ECOFF32_SEGMENT_ALIGNMENT(ep)));


	nextoff = stringtaboff + ecoff_strsize;
	symtaboff = nextoff;
	symtabsize = nsyms * sizeof(struct ecoff_extsym);
	symtabsize = ECOFF_ROUND(symtabsize, ECOFF32_SEGMENT_ALIGNMENT(ep));

	/* Write out the symbol header ... */
	write_ecoff_symhdr(out, ep, &symhdr, nsyms, symtaboff,
	    stringtaboff, ecoffsymtab.stringsize);

	/* Write out the string table... */
	padding = ecoff_strsize - ecoffsymtab.stringsize;
	safewrite(out, ecoffsymtab.stringtab, ecoffsymtab.stringsize,
	    "string table: write");
	if (padding)
		pad16(out, padding, "string table: padding");


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
	    "symbol table: write");
	if (padding)
		pad16(out, padding, "symbols: padding");
}



/*
 * In-memory translation of ELF symbosl to ECOFF.
 */
static void
translate_syms(struct elf_syms *elfp, struct ecoff_syms *ecoffp)
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
	if (newstrings == NULL)
		errx(1, "No memory for new string table");
	/* Copy and translate  symbols... */
	idx = 0;
	for (i = 0; i < nsyms; i++) {
		int     binding;

		binding = ELF32_ST_BIND((elfp->elf_syms[i].st_info));

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
static void
pad16(int fd, int size, const char *msg)
{

	safewrite(fd, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0", size, msg);
}

/* swap a 32bit region */
static void
bswap32_region(int32_t* p, int len)
{
	size_t i;

	for (i = 0; i < len / sizeof(int32_t); i++, p++)
		*p = bswap32(*p);
}
