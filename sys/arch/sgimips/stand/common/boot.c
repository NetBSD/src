/*	$NetBSD: boot.c,v 1.4 2003/08/07 16:29:27 agc Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan Stone, Michael Hitch and Simon Burge.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <dev/arcbios/arcbios.h>

#include "common.h"
#include "bootinfo.h"

/*
 * We won't go overboard with gzip'd kernel names.  After all we can
 * still boot a gzip'd kernel called "netbsd.sgimips" - it doesn't need
 * the .gz suffix.
 */
char *kernelnames[] = {
	"netbsd.sgimips",
	"netbsd",	"netbsd.gz",
	"netbsd.bak",
	"netbsd.old",
	"onetbsd",
	"gennetbsd",
#ifdef notyet
	"netbsd.el",
#endif /*notyet*/
	NULL
};

extern const struct arcbios_fv *ARCBIOS;

int main __P((int, char *[]));

#ifdef HEAP_VARIABLE
void setheap(void *, void*);
#endif

/* Storage must be static. */
struct btinfo_symtab bi_syms;
struct btinfo_bootpath bi_bpath;

/*
 * This gets arguments from the first stage boot lader, calls PROM routines
 * to open and load the program to boot, and then transfers execution to
 * that new program.
 *
 * Argv[0] should be something like "rz(0,0,0)netbsd" on a DECstation 3100.
 * Argv[0,1] should be something like "boot 5/rz0/netbsd" on a DECstation 5000.
 * The argument "-a" means netbsd should do an automatic reboot.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	char *name/*, **namep, *dev, *kernel*/;
	char /*bootname[PATH_MAX],*/ bootpath[PATH_MAX];
	void (*entry)(int, char *[], int, void *);
	u_long marks[MARK_MAX];
	struct arcbios_mem *mem;
	int win;

	/* print a banner */
	printf("\n");
	printf("NetBSD/sgimips " NETBSD_VERS " Bootstrap, Revision %s\n",
	    bootprog_rev);
	printf("(%s, %s)\n", bootprog_maker, bootprog_date);
	printf("\n");

	/* Initialize heap from free memory descriptors */
	mem = 0;
	do {
		mem = ARCBIOS->GetMemoryDescriptor(mem);
		if (mem != 0)
			printf("Mem block: type %d base 0x%x size 0x%x\n",
			    mem->Type, mem->BasePage * 4096, mem->PageCount * 4096);
	} while (mem != 0);
#ifdef HEAP_VARIABLE
	setheap((void *)0x88200000, (void *)0x88300000);	/* XXXX */
#endif
	printf("Local storage %x\n", (int)&mem);
	for (win = 0; win < argc; ++win)
		printf("argv[%d]: %s\n", win, argv[win]);

	/* initialise bootinfo structure early */
	bi_init();

	/*
	 * How to find partition and file to load?
	 * OSLoaderPartition=scsi(n)disk(n)rdisk(n)partition(n)
	 * OSLoadFilename=netbsd
	 * path=???
	 * argument passed to boot program
	 */
	bootpath[0] = 0;
	name = ARCBIOS->GetEnvironmentVariable("OSLoadPartition");
	if (name) {
		strcpy(bootpath, name);
		name = ARCBIOS->GetEnvironmentVariable("OSLoadFilename");
		if (name)
			strcat(bootpath, name);
		else
			strcat(bootpath, "netbsd");
	}
	if (strchr(argv[1], '=') == NULL) {
		strcpy(bootpath, argv[1]);
		if (strchr(argv[1], '(') == NULL) {
			strcpy(bootpath,
			    ARCBIOS->GetEnvironmentVariable("OSLoadPartition"));
			strcat(bootpath, argv[1]);
		}
	}
	printf("Boot: %s\n", bootpath);

	memset(marks, 0, sizeof marks);
#if 0
	if (name != NULL)
		win = (loadfile(name, marks, LOAD_KERNEL) == 0);
	else {
		win = 0;
		for (namep = kernelnames, win = 0; *namep != NULL && !win;
		    namep++) {
			kernel = *namep;
			strcpy(bootpath, dev);
			strcat(bootpath, kernel);
			printf("Loading: %s\n", bootpath);
			win = (loadfile(bootpath, marks, LOAD_ALL) != -1);
			if (win) {
				name = bootpath;
			}
		}
	}
#else
	win = loadfile(bootpath, marks, LOAD_KERNEL) == 0;
#endif
	if (!win)
		goto fail;

#if 0
	strncpy(bi_bpath.bootpath, kernel, BTINFO_BOOTPATH_LEN);
	bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));
#endif

	entry = (void *) marks[MARK_ENTRY];
	bi_syms.nsym = marks[MARK_NSYM];
	bi_syms.ssym = marks[MARK_SYM];
	bi_syms.esym = marks[MARK_END];
	bi_add(&bi_syms, BTINFO_SYMTAB);

	printf("Starting at %p\n\n", entry);
	printf("nsym 0x%lx ssym 0x%lx esym 0x%lx\n", marks[MARK_NSYM],
	    marks[MARK_SYM], marks[MARK_END]);
	ARCBIOS->FlushAllCaches();
	(*entry)(argc, argv, BOOTINFO_MAGIC, bootinfo);

	printf("Kernel returned!  Halting...\n");
	return (0);

 fail:
	(void)printf("Boot failed!  Halting...\n");
	return (0);
}
