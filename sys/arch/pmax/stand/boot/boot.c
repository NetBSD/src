/*	$NetBSD: boot.c,v 1.8 1999/11/12 01:37:43 simonb Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#define PMAX_BOOT_AOUT
#define PMAX_BOOT_ECOFF
#define PMAX_BOOT_ELF

#include <stand.h>
#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <machine/dec_prom.h>

#include "common.h"
#include "bootinfo.h"
#include "loadfile.h"

extern	char bootprog_name[], bootprog_rev[], bootprog_date[], bootprog_maker[];

char *kernelnames[] = {
	"netbsd",	"netbsd.gz",
	"netbsd.bak",	"netbsd.bak.gz",
	"netbsd.old",	"netbsd.old.gz",
	"onetbsd",	"onetbsd.gz",
	"gennetbsd",	"gennetbsd.gz",
#ifdef notyet
	"netbsd.el",	"netbsd.el.gz",
#endif /*notyet*/
	NULL
};


static int devonly __P((char *));
int main __P((int, char **));

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
	char *name, **namep, *dev;
	char bootname[PATH_MAX], bootpath[PATH_MAX];
	int entry, win;
	u_long marks[MARK_MAX];
	struct btinfo_symtab bi_syms;
	struct btinfo_bootpath bi_bpath;

	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	/* initialise bootinfo structure early */
	bi_init(BOOTINFO_ADDR);

	/* check for DS5000 boot */
	if (strcmp(argv[0], "boot") == 0) {
		argc--;
		argv++;
	}
	name = argv[0];
	printf("Boot: %s\n", name);

	/* NOTE: devonly() can modify name[]. */
	strcpy(bootname, argv[0]);
	if (devonly(bootname)) {
		dev = bootname;
		name = NULL;
	}

	marks[MARK_START] = 0;
	if (name != NULL)
		win = (loadfile(name, marks, LOAD_KERNEL) != -1);
	else {
		win = 0;
		for (namep = kernelnames, win = 0; *namep != NULL && !win;
		    namep++) {
			name = *namep;
			strcpy(bootpath, dev);
			strcat(bootpath, name);
			printf("Loading: %s\n", bootpath);
			win = (loadfile(bootpath, marks, LOAD_ALL) != -1);
			if (win)
				name = bootpath;
		}
	}
	if (!win)
		goto fail;

	strncpy(bi_bpath.bootpath, name, BTINFO_BOOTPATH_LEN);
	bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));

	entry = marks[MARK_ENTRY];
	bi_syms.nsym = marks[MARK_NSYM];
	bi_syms.ssym = marks[MARK_SYM];
	bi_syms.esym = marks[MARK_END];
	bi_add(&bi_syms, BTINFO_SYMTAB, sizeof(bi_syms));

	printf("Starting at 0x%x\n\n", entry);
	if (callv == &callvec)
		startprog(entry, entry, argc, argv, 0, 0,
		    BOOTINFO_MAGIC, BOOTINFO_ADDR);
	else
		startprog(entry, entry, argc, argv, DEC_PROM_MAGIC,
		    callv, BOOTINFO_MAGIC, BOOTINFO_ADDR);
	(void)printf("KERNEL RETURNED!\n");

fail:
	(void)printf("Boot failed!  Halting...\n");
	return (0);
}


/*
 * Check whether or not fname is a device name only or a full
 * bootpath including the kernel name.  This code to do this
 * is copied from loadfile() in the first stage bootblocks.
 *
 * NOTE:  fname will be modified if it's of the form N/rzY
 *        without a trailing slash.
 */
static
int
devonly(fname)
	char *fname;
{
	char c;

	while ((c = *fname++) != '\0') {
		if (c == ')')
			break;
		if (c != '/')
			continue;
		while ((c = *fname++) != '\0')
			if (c == '/')
				break;
		/*
		 * Make "N/rzY" with no trailing '/' valid by adding
		 * the extra '/' before appending 'boot' to the path.
		 */
		if (c != '/') {
			fname--;
			*fname++ = '/';
			*fname = '\0';
		}
		break;
	}
	return (*fname == '\0');
}
