/*	$NetBSD: boot.c,v 1.19 2004/04/08 07:35:34 pk Exp $ */

/*-
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * 	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/boot_flag.h>
#include <sys/exec.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <machine/promlib.h>
#include <sparc/stand/common/promdev.h>

#include "bootinfo.h"

extern void	prom_patch __P((void));	/* prompatch.c */

static int	bootoptions __P((char *));

int	boothowto;
int	debug;
int	netif_debug;

char	fbuf[80], dbuf[128];
paddr_t bstart, bend;	/* physical start & end address of the boot program */

int	compatmode = 0;		/* For loading older kernels */
u_long	loadaddrmask = -1UL;

extern char bootprog_name[], bootprog_rev[], bootprog_date[], bootprog_maker[];

int	main __P((void));
typedef void (*entry_t)__P((caddr_t, int, int, int, long, long));

/*
 * Boot device is derived from ROM provided information, or if there is none,
 * this list is used in sequence, to find a kernel.
 */
char *kernels[] = {
	"netbsd",
	"netbsd.gz",
	"netbsd.old",
	"netbsd.old.gz",
	"onetbsd",
	"onetbsd.gz",
	"vmunix",
#ifdef notyet
	"netbsd.pl",
	"netbsd.pl.gz",
	"netbsd.el",
	"netbsd.el.gz",
#endif
	NULL
};

int
bootoptions(ap)
	char *ap;
{
	int v = 0;
	if (ap == NULL || *ap++ != '-')
		return (0);

	while (*ap != '\0' && *ap != ' ' && *ap != '\t' && *ap != '\n') {
		BOOT_FLAG(*ap, v);
		if (*ap == 'C')
			compatmode = 1;
		ap++;
	}

	if ((v & RB_KDB) != 0)
		debug = 1;

	return (v);
}

static paddr_t getphysmem(u_long size)
{
	struct	memarr *pmemarr;	/* physical memory regions */
	int	npmemarr;		/* number of entries in pmemarr */
	struct memarr *mp;
	int i;
	extern char start[];	/* top of stack (see srt0.S) */

	/*
	 * Find the physical memory area that's in use by the boot loader.
	 * Our stack grows down from label `start'; assume we need no more
	 * than 16K of stack space.
	 * The top of the boot loader is the next 4MB boundary.
	 */
	if (pmap_extract((vaddr_t)start - (16*1024), &bstart) != 0)
		return ((paddr_t)-1);

	bend = roundup(bstart, 0x400000);

	/*
	 * Get available physical memory from the prom.
	 */
	npmemarr = prom_makememarr(NULL, 0, MEMARR_AVAILPHYS);
	pmemarr = alloc(npmemarr*sizeof(struct memarr));
	if (pmemarr == NULL)
		return ((paddr_t)-1);
	npmemarr = prom_makememarr(pmemarr, npmemarr, MEMARR_AVAILPHYS);

	/*
	 * Find a suitable loading address.
	 */
	for (mp = pmemarr, i = npmemarr; --i >= 0; mp++) {
		paddr_t pa = (paddr_t)pmemarr[i].addr;
		u_long len = (u_long)pmemarr[i].len;

		/* Check whether it will fit in front of us */
		if (pa < bstart && len >= size && (bstart - pa) >= size)
			return (pa);

		/* Skip the boot program memory */
		if (pa < bend) {
			if (len < bend - pa)
				/* Not large enough */
				continue;

			/* Shrink this segment */
			len -=  bend - pa;
			pa = bend;
		}

		/* Does it fit in the remainder of this segment? */
		if (len >= size)
			return (pa);
	}
	return ((paddr_t)-1);
}

static int
loadk(char *kernel, u_long *marks)
{
	int fd, error;
	vaddr_t va;
	paddr_t pa;
	u_long size;

	if ((fd = open(kernel, 0)) < 0)
		return (errno ? errno : ENOENT);

	marks[MARK_START] = 0;
	if ((error = fdloadfile(fd, marks, COUNT_KERNEL)) != 0)
		goto out;

	size = marks[MARK_END] - marks[MARK_START];

	/* We want that leading 16K in front of the kernel image */
	size += PROM_LOADADDR;
	va = marks[MARK_START] - PROM_LOADADDR;

	/*
	 * Extra space for bootinfo and kernel bootstrap.
	 * In compat mode, we get to re-use the space occupied by the
	 * boot program. Traditionally, we've silently assumed that
	 * is enough for the kernel to work with.
	 */
	size += BOOTINFO_SIZE;
	if (!compatmode)
		size += 512 * 1024;

	/* Get a physical load address */
	pa = getphysmem(size);
	if (pa == (paddr_t)-1) {
		error = EFBIG;
		goto out;
	}

	if (boothowto & AB_VERBOSE)
		printf("Loading at physical address %lx\n", pa);
	if (pmap_map(va, pa, size) != 0) {
		error = EFAULT;
		goto out;
	}

	/* XXX - to do: inspect kernel image and set compat mode */
	if (compatmode) {
		/* Double-map at VA 0 for compatibility */
		if (pa + size >= bstart) {
			printf("%s: too large for compat mode\n", kernel);
			error = EFBIG;
			goto out;
		}

		if (pa != 0 && pmap_map(0, pa, size) != 0) {
			error = EFAULT;
			goto out;
		}
		loadaddrmask = 0x07ffffffUL;
	}

	marks[MARK_START] = 0;
	error = fdloadfile(fd, marks, LOAD_KERNEL);
out:
	close(fd);
	return (error);
}

int
main()
{
	int	error, i;
	char	*kernel;
	u_long	marks[MARK_MAX], bootinfo;
	struct btinfo_symtab bi_sym;
	void	*arg;

#ifdef HEAP_VARIABLE
	{
		extern char end[];
		setheap((void *)ALIGN(end), (void *)0xffffffff);
	}
#endif
	prom_init();
	mmu_init();

	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	/* massage machine prom */
	prom_patch();

	/*
	 * get default kernel.
	 */
	prom_bootdevice = prom_getbootpath();
	kernel = prom_getbootfile();
	boothowto = bootoptions(prom_getbootargs());

	if (kernel != NULL && *kernel != '\0') {
		i = -1;	/* not using the kernels */
	} else {
		i = 0;
		kernel = kernels[i];
	}

	for (;;) {
		/*
		 * ask for a kernel first ..
		 */
		if (boothowto & RB_ASKNAME) {
			printf("device[%s] (\"halt\" to halt): ",
					prom_bootdevice);
			gets(dbuf);
			if (strcmp(dbuf, "halt") == 0)
				_rtt();
			if (dbuf[0])
				prom_bootdevice = dbuf;
			printf("boot (press RETURN to try default list): ");
			gets(fbuf);
			if (fbuf[0])
				kernel = fbuf;
			else {
				boothowto &= ~RB_ASKNAME;
				i = 0;
				kernel = kernels[i];
			}
		}

		printf("Booting %s\n", kernel);
		if ((error = loadk(kernel, marks)) == 0)
			break;

		if (error != ENOENT) {
			printf("Cannot load %s: error=%d\n", kernel, error);
			boothowto |= RB_ASKNAME;
		}

		/*
		 * if we have are not in askname mode, and we aren't using the
		 * prom bootfile, try the next one (if it exits).  otherwise,
		 * go into askname mode.
		 */
		if ((boothowto & RB_ASKNAME) == 0 &&
		    i != -1 && kernels[++i]) {
			kernel = kernels[i];
			printf(": trying %s...\n", kernel);
		} else {
			printf("\n");
			boothowto |= RB_ASKNAME;
		}
	}

	marks[MARK_END] = (((u_long)marks[MARK_END] + sizeof(u_long) - 1)) &
	    (-sizeof(u_long));
	arg = (prom_version() == PROM_OLDMON) ? (caddr_t)PROM_LOADADDR : romp;

	/* Setup boot info structure at the end of the kernel image */
	bootinfo = bi_init(marks[MARK_END] & loadaddrmask);

	/* Add kernel symbols to bootinfo */
	bi_sym.nsym = marks[MARK_NSYM] & loadaddrmask;
	bi_sym.ssym = marks[MARK_SYM] & loadaddrmask;
	bi_sym.esym = marks[MARK_END] & loadaddrmask;
	bi_add(&bi_sym, BTINFO_SYMTAB, sizeof(bi_sym));

	/* Add kernel path to bootinfo */
	i = sizeof(struct btinfo_common) + strlen(kernel) + 1;
	/* Impose limit (somewhat arbitrary) */
	if (i < BOOTINFO_SIZE / 2) {
		union {
			struct btinfo_kernelfile bi_file;
			char x[i];
		} U;
		strcpy(U.bi_file.name, kernel);
		bi_add(&U.bi_file, BTINFO_KERNELFILE, i);
	}

	(*(entry_t)marks[MARK_ENTRY])(arg, 0, 0, 0, bootinfo, DDB_MAGIC2);
	_rtt();
}
