/* $NetBSD: loadelf.c,v 1.1 2004/04/17 23:20:37 cl Exp $ */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
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
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: loadelf.c,v 1.1 2004/04/17 23:20:37 cl Exp $");

#if !defined(ELFSIZE)
#define ELFSIZE 32
#endif

#include <sys/exec_elf.h>
#include <sys/lock.h>

#include <lib/libsa/stand.h>

#include <machine/vmparam.h>

#include "xenload.h"

#ifdef DEBUG
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)
#endif

/* 
 * NetBSD memory layout:
 *
 * ---------------- *virt_load_addr = ehdr.e_entry (0xc0100000)
 * | kernel text  |
 * |              |
 * ----------------
 * | kernel data  |
 * |              |
 * ----------------
 * | kernel bss   |
 * |              |
 * ---------------- *symtab_addr
 * | symtab size  |   = *symtab_len
 * ----------------
 * | elf header   |   offsets to symbol sections mangled to be relative
 * |              |   to headers location
 * ----------------
 * | sym section  |
 * | headers      |
 * ----------------
 * | sym sections |
 * |              |
 * ---------------- *symtab_addr + *symtab_len
 * | padding      |
 * ---------------- ehdr.e_entry + *ksize << PAGE_SHIFT
 */

#define	IS_ELF(ehdr)	((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
			 (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
			 (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
			 (ehdr).e_ident[EI_MAG3] == ELFMAG3)
#define	IS_TEXT(p)	(p.p_flags & PF_X)
#define	IS_DATA(p)	(p.p_flags & PF_W)
#define	IS_BSS(p)	(p.p_filesz < p.p_memsz)

#define ELFROUND	(ELFSIZE / 8)

#define	MAX_PHEADERS	5
#define	MAX_SHEADERS	25

int
prepareelfimage(vaddr_t elfimage, vaddr_t *virt_load_addr,
    vaddr_t *symtab_addr, unsigned long *symtab_len)
{
	Elf_Ehdr ehdr;
	Elf_Phdr phdr[MAX_PHEADERS];
	Elf_Shdr shdr[MAX_SHEADERS];
	unsigned long maxva, symva;
	vaddr_t offset;
	int curpos, h, i, ret;

	ret = -1;
	maxva = 0;

	memcpy(&ehdr, (void *)elfimage, sizeof(Elf_Ehdr));
	curpos = sizeof(Elf_Ehdr);

	if (!IS_ELF(ehdr)) {
		ERROR("Image does not have an ELF header.");
		goto out;
	}

	*virt_load_addr = ehdr.e_entry;

	if ((*virt_load_addr & (PAGE_SIZE-1)) != 0) {
		ERROR("We can only deal with page-aligned load addresses");
		goto out;
	}

	if (ehdr.e_phnum > MAX_PHEADERS) {
		ERROR("Too many Program Headers, increase MAX_PHEADERS");
		goto out;
	}

	offset = elfimage - *virt_load_addr;

	curpos = ehdr.e_phoff;
	memcpy(phdr, (void *)elfimage + ehdr.e_phoff,
	    ehdr.e_phnum * sizeof(Elf_Phdr));
	curpos += ehdr.e_phnum * sizeof(Elf_Phdr);

	/* Copy run-time 'load' segments that are writeable and/or
           executable. */
	for (h = 0; h < ehdr.e_phnum; h++) {
		if ((phdr[h].p_type != PT_LOAD) ||
		    ((phdr[h].p_flags & (PF_W|PF_X)) == 0))
			continue;

		if (IS_TEXT(phdr[h]) || IS_DATA(phdr[h])) {
			curpos = phdr[h].p_offset;
			DPRINTF(("move segment %p/%x to va %p/%p\n",
			    elfimage + phdr[h].p_offset,
			    phdr[h].p_filesz, (void *)phdr[h].p_vaddr,
			    (void *)phdr[h].p_vaddr + offset));

			memmove((void *)phdr[h].p_vaddr + offset,
			    (void *)elfimage + phdr[h].p_offset,
			    phdr[h].p_filesz);

			if (phdr[h].p_vaddr + phdr[h].p_filesz > maxva)
				maxva = phdr[h].p_vaddr + phdr[h].p_filesz;
		}

		if (IS_BSS(phdr[h])) {
			/* XXX maybe clear phdr[h].p_memsz bytes from
			   phdr[h].p_vaddr + phdr[h].p_filesz ??? */
			if (phdr[h].p_vaddr + phdr[h].p_memsz > maxva)
				maxva = phdr[h].p_vaddr + phdr[h].p_memsz;
			DPRINTF(("bss from %p to %p, maxva %p\n",
			    (void *)(phdr[h].p_vaddr + phdr[h].p_filesz),
			    (void *)(phdr[h].p_vaddr + phdr[h].p_memsz),
			    (void *)maxva));
		}
	}

	if (ehdr.e_shnum > MAX_SHEADERS) {
		ERROR("Too many Section Headers, increase MAX_SHEADERS");
		goto out;
	}

	curpos = ehdr.e_shoff;
	memcpy(shdr, (void *)elfimage + ehdr.e_shoff,
	    ehdr.e_shnum * sizeof(Elf_Shdr));
	curpos += ehdr.e_shnum * sizeof(Elf_Shdr);

	maxva = (maxva + ELFROUND - 1) & ~(ELFROUND - 1);
	symva = maxva;
	maxva += sizeof(int);
	*symtab_addr = maxva;
	*symtab_len = 0;
	maxva += sizeof(Elf_Ehdr) + ehdr.e_shnum * sizeof(Elf_Shdr);
	maxva = (maxva + ELFROUND - 1) & ~(ELFROUND - 1);

	/* Copy kernel string / symbol tables into physical memory */
	for (h = 0; h < ehdr.e_shnum; h++) {
		if (shdr[h].sh_type == SHT_STRTAB) {
			/* Look for a strtab @i linked to symtab @h. */
			for (i = 0; i < ehdr.e_shnum; i++)
				if ((shdr[i].sh_type == SHT_SYMTAB) &&
				    (shdr[i].sh_link == h))
					break;
			/* Skip symtab @h if we found no corresponding
                           strtab @i. */
			if (i == ehdr.e_shnum) {
				shdr[h].sh_offset = 0;
				continue;
			}
		}

		if ((shdr[h].sh_type == SHT_STRTAB) ||
		    (shdr[h].sh_type == SHT_SYMTAB)) {
			curpos = shdr[h].sh_offset;

			DPRINTF(("copy section %d, addr %p size 0x%x from "
			    "%p\n", h, (void *)maxva + offset, shdr[h].sh_size,
			    (void *)elfimage + shdr[h].sh_offset));
			memmove((void *)maxva + offset, (void *)elfimage +
			    shdr[h].sh_offset, shdr[h].sh_size);

			/* Mangled to be based on ELF header location. */
			shdr[h].sh_offset = maxva - *symtab_addr;

			maxva += shdr[h].sh_size;
			*symtab_len += shdr[h].sh_size;
			maxva = (maxva + ELFROUND - 1) & ~(ELFROUND - 1);

		}
		shdr[h].sh_name = 0;  /* Name is NULL. */
	}

	if (*symtab_len == 0) {
		DPRINTF(("no symbol table\n"));
		ret = 0;
		goto out;
	}

	DPRINTF(("sym header va %p from %p size %x, addrs %p/%p\n",
	    (void *)symva, shdr, ehdr.e_shnum * sizeof(Elf_Shdr),
	    (void *)symva + sizeof(int),
	    (void *)symva + sizeof(int) + sizeof(Elf_Ehdr)));
	ehdr.e_phoff = 0;
	ehdr.e_shoff = sizeof(Elf_Ehdr);
	ehdr.e_phentsize = 0;
	ehdr.e_phnum = 0;
	ehdr.e_shstrndx = SHN_UNDEF;

	symva += offset;

	/* Copy total length, crafted ELF header and section
	   header table */
	*(int *)symva = maxva - *symtab_addr;
	memmove((void *)symva + sizeof(int), &ehdr, sizeof(Elf_Ehdr));
	memmove((void *)symva + sizeof(int) + sizeof(Elf_Ehdr), shdr,
	    ehdr.e_shnum * sizeof(Elf_Shdr));

	*symtab_len = maxva - *symtab_addr;

	ret = 0;

 out:
	if (ret == 0) {
		maxva = (maxva + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

		DPRINTF(("virt_addr %p, symtab_addr %p, symtab_len %p, "
		    "maxva %p\n", (void *)*virt_load_addr,
		    (void *)*symtab_addr, (void *)*symtab_len, (void *)maxva));
	}

	return ret;
}
