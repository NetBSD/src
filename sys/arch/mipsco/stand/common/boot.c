/*	$NetBSD: boot.c,v 1.9.14.1 2014/08/20 00:03:13 tls Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan Stone, Michael Hitch, Simon Burge and Wayne Knowles.
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

#include <machine/prom.h>

#include "common.h"
#include "bootinfo.h"

/*
 * We won't go overboard with gzip'd kernel names.  After all we can
 * still boot a gzip'd kernel called "netbsd.mipsco" - it doesn't need
 * the .gz suffix.
 */
char *kernelnames[] = {
	"netbsd",
	"netbsd.gz",
	"netbsd.bak",
	"netbsd.old",
	"netbsd.mipsco",
	"netbsd.elf",
	NULL
};


static char *devsplit(char *, char *);
int main(int, char **);

/*
 * This gets arguments from the first stage boot lader, calls PROM routines
 * to open and load the program to boot, and then transfers execution to
 * that new program.
 */
int
main(int argc, char **argv)
{
	char *name, **namep, *dev, *kernel;
	char bootname[PATH_MAX], bootpath[PATH_MAX];
	int win;
	u_long marks[MARK_MAX];
	struct btinfo_symtab bi_syms;
	struct btinfo_bootpath bi_bpath;
	extern void prom_init(void);
	void (*entry)(int, char **, char **, u_int, char *);

	prom_init();

	/* print a banner */
	printf("\n");
	printf("NetBSD/mipsco " NETBSD_VERS " " BOOT_TYPE_NAME 
	       " Bootstrap, Revision %s\n", bootprog_rev);

	/* initialise bootinfo structure early */
	bi_init(BOOTINFO_ADDR);

	dev = name = NULL;
	if (argc > 1) {
		kernel = devsplit(argv[1], bootname);
		if (*bootname) {
			dev = bootname;
			if (*kernel)
				name = argv[1];
			++argv;
			--argc;
		}

	} else
		kernel = NULL;

	if (dev == NULL) {
		(void) devsplit(argv[0], bootname);
		dev = bootname;
	}

	memset(marks, 0, sizeof marks);
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
	if (!win || !kernel)
		goto fail;

	strncpy(bi_bpath.bootpath, kernel, BTINFO_BOOTPATH_LEN);
	bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));

	entry = (void *) marks[MARK_ENTRY];
	bi_syms.nsym = marks[MARK_NSYM];
	bi_syms.ssym = marks[MARK_SYM];
	bi_syms.esym = marks[MARK_END];
	bi_add(&bi_syms, BTINFO_SYMTAB, sizeof(bi_syms));

	printf("Starting at 0x%x\n\n", (u_int)entry);

	(*entry)(argc, argv, NULL, BOOTINFO_MAGIC, (char *)BOOTINFO_ADDR);

	(void)printf("KERNEL RETURNED!\n");

fail:
	(void)printf("Boot failed!  Halting...\n");
	return (0);
}

/*
 * strip out device name and kernel name
 */
static char *
devsplit(char *fname, char *devname)
{
	char *src, *dst;

	dst = devname;
	for (src = fname; *src;/**/)
		if ((*dst++ = *src++) == ')') {
			*dst = (char) 0;
			return src;
		}
	*devname = (char) 0;
	return fname;
}
