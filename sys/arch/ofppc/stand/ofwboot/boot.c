/*	$NetBSD: boot.c,v 1.8 2001/01/17 15:37:06 ws Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
 *
 * ELF support derived from NetBSD/alpha's boot loader, written
 * by Christopher G. Demetriou.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * First try for the boot code
 *
 * Input syntax is:
 *	[promdev[{:|,}partition]]/[filename] [flags]
 */

#define	ELFSIZE		32		/* We use 32-bit ELF. */

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <sys/boot_flag.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <machine/cpu.h>
#include <machine/machine_type.h>

#include "ofdev.h"
#include "openfirm.h"

char bootdev[128];
char bootfile[128];
int boothowto;
int debug;

#ifdef POWERPC_BOOT_ELF
int	elf_exec __P((int, Elf_Ehdr *, u_int32_t *, void **));
#endif

#ifdef POWERPC_BOOT_AOUT
int	aout_exec __P((int, struct exec *, u_int32_t *, void **));
#endif

static void
prom2boot(dev)
	char *dev;
{
	char *cp;
	
	for (cp = dev; *cp != '\0'; cp++)
		if (*cp == ':') {
			*cp = '\0';
			return;
		}
}

static void
parseargs(str, howtop)
	char *str;
	int *howtop;
{
	char *cp;

	/* Allow user to drop back to the PROM. */
	if (strcmp(str, "exit") == 0)
		OF_exit();

	*howtop = 0;
	for (cp = str; *cp; cp++)
		if (*cp == ' ' || *cp == '-')
			break;
	if (!*cp)
		return;
	
	*cp++ = '\0';
	while (*cp)
		BOOT_FLAG(*cp++, *howtop);
}

static void
chain(entry, args, esym)
	void (*entry)();
	char *args;
	void *esym;
{
	extern char end[];
	int l, machine_tag;

	freeall();

	/*
	 * Stash pointer to end of symbol table after the argument
	 * strings.
	 */
	l = strlen(args) + 1;
	bcopy(&esym, args + l, sizeof(esym));
	l += sizeof(esym);

	/*
	 * Tell the kernel we're an OpenFirmware system.
	 */
	machine_tag = POWERPC_MACHINE_OPENFIRMWARE;
	bcopy(&machine_tag, args + l, sizeof(machine_tag));
	l += sizeof(machine_tag);

	OF_chain((void *)RELOC, end - (char *)RELOC, entry, args, l);
	panic("chain");
}

int
loadfile(fd, args)
	int fd;
	char *args;
{
	union {
#ifdef POWERPC_BOOT_AOUT
		struct exec aout;
#endif
#ifdef POWERPC_BOOT_ELF
		Elf_Ehdr elf;
#endif
	} hdr;
	int rval;
	u_int32_t entry;
	void *esym;

	rval = 1;
	esym = NULL;

	/* Load the header. */
	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		printf("read header: %s\n", strerror(errno));
		goto err;
	}

	/* Determine file type, load kernel. */
#ifdef POWERPC_BOOT_AOUT
	if (N_BADMAG(hdr.aout) == 0 && N_GETMID(hdr.aout) == MID_POWERPC) {
		rval = aout_exec(fd, &hdr.aout, &entry, &esym);
	} else
#endif
#ifdef POWERPC_BOOT_ELF
	if (memcmp(hdr.elf.e_ident, ELFMAG, SELFMAG) == 0 &&
	    hdr.elf.e_ident[EI_CLASS] == ELFCLASS) {
		rval = elf_exec(fd, &hdr.elf, &entry, &esym);
	} else
#endif
	{
		printf("unknown executable format\n");
	}

	if (rval)
		goto err;

	printf(" start=0x%x\n", entry);

	close(fd);

	/* XXX this should be replaced w/ a mountroothook. */
	if (floppyboot) {
		printf("Please insert root disk and press ENTER ");
		getchar();
		printf("\n");
	}

	chain((void *)entry, args, esym);
	/* NOTREACHED */

 err:
	close(fd);
	return (rval);
}

__dead void
_rtt()
{

	OF_exit();
}

#ifdef POWERPC_BOOT_AOUT
int
aout_exec(fd, hdr, entryp, esymp)
	int fd;
	struct exec *hdr;
	u_int32_t *entryp;
	void **esymp;
{
	int n, *addr;

	/* Display the load address (entry point) for a.out. */
	printf("Booting %s @ 0x%lx\n", opened_name, hdr->a_entry);
	addr = (void *)(hdr->a_entry);

	/*
	 * Determine memory needed for kernel and allocate it from
	 * the firmware.
	 */
	n = hdr->a_text + hdr->a_data + hdr->a_bss + hdr->a_syms + sizeof(int);

	/* Load text. */
	lseek(fd, N_TXTOFF(*hdr), SEEK_SET);
	printf("%lu", hdr->a_text);
	if (read(fd, addr, hdr->a_text) != hdr->a_text) {
		printf("read text: %s\n", strerror(errno));
		return (1);
	}
	__syncicache((void *)addr, hdr->a_text);

	/* Load data. */
	printf("+%lu", hdr->a_data);
	if (read(fd, (char *)addr + hdr->a_text, hdr->a_data) != hdr->a_data) {
		printf("read data: %s\n", strerror(errno));
		return (1);
	}

	/* Zero BSS. */
	printf("+%lu", hdr->a_bss);
	bzero((char *)addr + hdr->a_text + hdr->a_data, hdr->a_bss);

	/* Symbols. */
	*esymp = addr;
	addr = (int *)((char *)addr + hdr->a_text + hdr->a_data + hdr->a_bss);
	*addr++ = hdr->a_syms;
	if (hdr->a_syms) {
		printf(" [%lu", hdr->a_syms);
		if (read(fd, addr, hdr->a_syms) != hdr->a_syms) {
			printf("read symbols: %s\n", strerror(errno));
			return (1);
		}
		addr = (int *)((char *)addr + hdr->a_syms);
		if (read(fd, &n, sizeof(int)) != sizeof(int)) {
			printf("read symbols: %s\n", strerror(errno));
			return (1);
		}
		*addr++ = n;
		if (read(fd, addr, n - sizeof(int)) != n - sizeof(int)) {
			printf("read symbols: %s\n", strerror(errno));
			return (1);
		}
		printf("+%d]", n - sizeof(int));
		*esymp = addr + (n - sizeof(int));
	}

	*entryp = hdr->a_entry;
	return (0);
}
#endif /* POWERPC_BOOT_AOUT */

#ifdef POWERPC_BOOT_ELF
int
elf_exec(fd, elf, entryp, esymp)
	int fd;
	Elf_Ehdr *elf;
	u_int32_t *entryp;
	void **esymp;
{
	Elf32_Shdr *shp;
	Elf32_Off off;
	void *addr;
	size_t size;
	int i, first = 1;
	int n;

	/*
	 * Don't display load address for ELF; it's encoded in
	 * each section.
	 */
	printf("Booting %s\n", opened_name);

	for (i = 0; i < elf->e_phnum; i++) {
		Elf_Phdr phdr;
		(void)lseek(fd, elf->e_phoff + sizeof(phdr) * i, SEEK_SET);
		if (read(fd, (void *)&phdr, sizeof(phdr)) != sizeof(phdr)) {
			printf("read phdr: %s\n", strerror(errno));
			return (1);
		}
		if (phdr.p_type != PT_LOAD ||
		    (phdr.p_flags & (PF_W|PF_X)) == 0)
			continue;

		/* Read in segment. */
		printf("%s%lu@0x%lx", first ? "" : "+", phdr.p_filesz,
		    (u_long)phdr.p_vaddr);
		(void)lseek(fd, phdr.p_offset, SEEK_SET);
		if (read(fd, (void *)phdr.p_vaddr, phdr.p_filesz) !=
		    phdr.p_filesz) {
			printf("read segment: %s\n", strerror(errno));
			return (1);
		}
		__syncicache((void *)phdr.p_vaddr, phdr.p_filesz);

		/* Zero BSS. */
		if (phdr.p_filesz < phdr.p_memsz) {
			printf("+%lu@0x%lx", phdr.p_memsz - phdr.p_filesz,
			    (u_long)(phdr.p_vaddr + phdr.p_filesz));
			bzero((void *)phdr.p_vaddr + phdr.p_filesz,
			    phdr.p_memsz - phdr.p_filesz);
		}
		first = 0;
	}

	printf(" \n");

#if 0 /* I want to rethink this... --thorpej@netbsd.org */
	/*
	 * Compute the size of the symbol table.
	 */
	size = sizeof(Elf_Ehdr) + (elf->e_shnum * sizeof(Elf32_Shdr));
	shp = addr = alloc(elf->e_shnum * sizeof(Elf32_Shdr));
	(void)lseek(fd, elf->e_shoff, SEEK_SET);
	if (read(fd, addr, elf->e_shnum * sizeof(Elf32_Shdr)) !=
	    elf->e_shnum * sizeof(Elf32_Shdr)) {
		printf("read section headers: %s\n", strerror(errno));
		return (1);
	}
	for (i = 0; i < elf->e_shnum; i++, shp++) {
		if (shp->sh_type == SHT_NULL)
			continue;
		if (shp->sh_type != SHT_SYMTAB
		    && shp->sh_type != SHT_STRTAB) {
			shp->sh_offset = 0; 
			shp->sh_type = SHT_NOBITS;
			continue;
		}
		size += shp->sh_size;
	}
	shp = addr;

	panic("no space for symbol table");

	/*
	 * Copy the headers.
	 */
	elf->e_phoff = 0;
	elf->e_shoff = sizeof(Elf_Ehdr);
	elf->e_phentsize = 0;
	elf->e_phnum = 0;
	bcopy(elf, addr, sizeof(Elf_Ehdr));
	bcopy(shp, addr + sizeof(Elf_Ehdr), elf->e_shnum * sizeof(Elf32_Shdr));
	free(shp, elf->e_shnum * sizeof(Elf32_Shdr));
	*ssymp = addr;

	/*
	 * Now load the symbol sections themselves.
	 */
	shp = addr + sizeof(Elf_Ehdr);
	addr += sizeof(Elf_Ehdr) + (elf->e_shnum * sizeof(Elf32_Shdr));
	off = sizeof(Elf_Ehdr) + (elf->e_shnum * sizeof(Elf32_Shdr));
	for (first = 1, i = 0; i < elf->e_shnum; i++, shp++) {
		if (shp->sh_type == SHT_SYMTAB
		    || shp->sh_type == SHT_STRTAB) {
			if (first)
				printf("symbols @ 0x%lx ", (u_long)addr);
			printf("%s%d", first ? "" : "+", shp->sh_size);
			(void)lseek(fd, shp->sh_offset, SEEK_SET);
			if (read(fd, addr, shp->sh_size) != shp->sh_size) {
				printf("read symbols: %s\n", strerror(errno));
				return (1);
			}
			addr += shp->sh_size;
			shp->sh_offset = off;
			off += shp->sh_size;
			first = 0;
		}
	}
	*esymp = addr;
#endif /* 0 */

	*entryp = elf->e_entry;
	return (0);
}
#endif /* POWERPC_BOOT_ELF */

void
main()
{
	extern char bootprog_name[], bootprog_rev[],
	    bootprog_maker[], bootprog_date[];
	int chosen;
	char bootline[512];		/* Should check size? */
	char *cp;
	int fd;
	
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	/*
	 * Get the boot arguments from Openfirmware
	 */
	if ((chosen = OF_finddevice("/chosen")) == -1 ||
	    OF_getprop(chosen, "bootpath", bootdev, sizeof bootdev) < 0 ||
	    OF_getprop(chosen, "bootargs", bootline, sizeof bootline) < 0) {
		printf("Invalid Openfirmware environment\n");
		OF_exit();
	}

	prom2boot(bootdev);
	parseargs(bootline, &boothowto);

	for (;;) {
		if (boothowto & RB_ASKNAME) {
			printf("Boot: ");
			gets(bootline);
			parseargs(bootline, &boothowto);
		}
		if ((fd = open(bootline, 0)) >= 0)
			break;
		if (errno)
			printf("open %s: %s\n", opened_name, strerror(errno));
		boothowto |= RB_ASKNAME;
	}
#ifdef	__notyet__
	OF_setprop(chosen, "bootpath", opened_name, strlen(opened_name) + 1);
	cp = bootline;
#else
	strcpy(bootline, opened_name);
	cp = bootline + strlen(bootline);
	*cp++ = ' ';
#endif
	*cp = '-';
	if (boothowto & RB_ASKNAME)
		*++cp = 'a';
	if (boothowto & RB_SINGLE)
		*++cp = 's';
	if (boothowto & RB_KDB)
		*++cp = 'd';
	if (*cp == '-')
#ifdef	__notyet__
		*cp = 0;
#else
		*--cp = 0;
#endif
	else
		*++cp = 0;
#ifdef	__notyet__
	OF_setprop(chosen, "bootargs", bootline, strlen(bootline) + 1);
#endif
	/* XXX void, for now */
	(void)loadfile(fd, bootline);

	OF_exit();
}
