/*	$NetBSD: boot.c,v 1.11.2.1 2008/02/04 09:22:27 yamt Exp $	*/

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
#include <sys/boot_flag.h>

#include <dev/arcbios/arcbios.h>

#include "common.h"
#include "bootinfo.h"

/*
 * We won't go overboard with gzip'd kernel names.  After all we can
 * still boot a gzip'd kernel called "netbsd.sgimips" - it doesn't need
 * the .gz suffix.
 *
 * For arcane reasons, the first byte of the first element of this struct will
 * contain a zero.  We therefore start from one.
 */

char           *kernelnames[] = {
	"placekeeper",
	"netbsd.sgimips",
	"netbsd",
	"netbsd.gz",
	"netbsd.bak",
	"netbsd.old",
	"onetbsd",
	"gennetbsd",
	NULL
};

extern const struct arcbios_fv *ARCBIOS;
static int      debug = 0;

int             main(int, char **);

/* Storage must be static. */
struct btinfo_symtab bi_syms;
struct btinfo_bootpath bi_bpath;

static uint8_t bootinfo[BOOTINFO_SIZE];

/*
 * This gets arguments from the ARCS monitor, calls ARCS routines to open
 * and load the program to boot, then transfers execution to the new program.
 *
 * argv[0] will be the ARCS path to the bootloader (i.e.,
 * "pci(0)scsi(0)disk(2)rdisk(0)partition(8)/boot.ip3").
 *
 * argv[1] through argv[n] will contain arguments passed from the PROM, if any.
 */

int
main(int argc, char **argv)
{
	const char      *kernel = NULL;
	const char      *bootpath = NULL;
	char            bootfile[PATH_MAX];
	void            (*entry) (int, char *[], int, void *);
	u_long          marks[MARK_MAX];
	int             win = 0;
	int             i;
	int             ch;

	/* print a banner */
	printf("\n");
	printf("NetBSD/sgimips " NETBSD_VERS " Bootstrap, Revision %s\n",
	       bootprog_rev);
	printf("(%s, %s)\n", bootprog_maker, bootprog_date);
	printf("\n");

	memset(marks, 0, sizeof marks);

	/* initialise bootinfo structure early */
	bi_init(bootinfo);

	/* Parse arguments, if present.  */

	while ((ch = getopt(argc, argv, "v")) != -1) {
		switch (ch) {
		case 'v':
			debug = 1;
			break;
		}
	}

	/*
	 * How to find partition and file to load?
	 *
	 * If argv[0] contains the string "cdrom(", we're probably doing an
	 * install.  The bootpath will therefore be partition 0 of whatever
	 * device we've booted from.  Derive the install kernel name from
	 * the bootloader name ("ip32boot", "ip22boot", or "aoutboot").
	 */

	if (strstr(argv[0], "cdrom("))
	{
		strcpy(bootfile, argv[0]);
		i = (strrchr(bootfile, ')') - bootfile);
		bootfile[i-1] = '0';
		if (strstr(bootfile, "ip3x"))
			sprintf( (strrchr(bootfile, ')') + 1), "ip3x");
		else
			sprintf( (strrchr(bootfile, ')') + 1), "ip2x");
		if ( (loadfile(bootfile, marks, LOAD_KERNEL)) >= 0 )
			goto finish;
	}

	bootpath = ARCBIOS->GetEnvironmentVariable("OSLoadPartition");

	if (bootpath == NULL) {
		/* XXX need to actually do the fixup */
		printf("\nPlease set the OSLoadPartition environment variable.\n");
		return 0;
	}

	/*
	 * Grab OSLoadFilename from ARCS.
	 */

	kernel = ARCBIOS->GetEnvironmentVariable("OSLoadFilename");

	/*
	 * argv[1] is assumed to contain the name of the kernel to boot,
	 * if it a) does not start with a hyphen and b) does not contain
	 * an equals sign.
	 */

	if (((strchr(argv[1], '=')) == NULL) && (argv[1][0] != '-'))
		kernel = argv[1];

	if (kernel != NULL) {
		/*
		 * if the name contains parenthesis, we assume that it
		 * contains the bootpath and ignore anything passed through
		 * the environment
		 */
		if (strchr(kernel, '('))
			win = loadfile(kernel, marks, LOAD_KERNEL);
		else {
			strcpy(bootfile, bootpath);
			strcat(bootfile, kernel);
			win = loadfile(bootfile, marks, LOAD_KERNEL);
		}

	} else {
		i = 1;
		while (kernelnames[i] != NULL) {
			strcpy(bootfile, bootpath);
			strcat(bootfile, kernelnames[i]);
			kernel = kernelnames[i];
			win = loadfile(bootfile, marks, LOAD_KERNEL);
			if (win != -1)
				break;
			i++;
		}

	}

	if (win < 0) {
		printf("Boot failed!  Halting...\n");
		return 0;
	}

finish:
	strlcpy(bi_bpath.bootpath, kernel, BTINFO_BOOTPATH_LEN);
	bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));

	bi_syms.nsym = marks[MARK_NSYM];
	bi_syms.ssym = marks[MARK_SYM];
	bi_syms.esym = marks[MARK_END];
	bi_add(&bi_syms, BTINFO_SYMTAB, sizeof(bi_syms));
	entry = (void *)marks[MARK_ENTRY];

	if (debug) {
		printf("Starting at %p\n\n", entry);
		printf("nsym 0x%lx ssym 0x%lx esym 0x%lx\n", marks[MARK_NSYM],
		       marks[MARK_SYM], marks[MARK_END]);
	}
	(*entry)(argc, argv, BOOTINFO_MAGIC, bootinfo);

	printf("Kernel returned!  Halting...\n");
	return (0);
}
