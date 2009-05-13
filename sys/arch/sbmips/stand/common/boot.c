/* $NetBSD: boot.c,v 1.4.2.1 2009/05/13 17:18:17 jym Exp $ */

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
#include <lib/libkern/libkern.h>
#include <lib/libsa/loadfile.h>

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_ecoff.h>

#include "stand/common/common.h"
#include "stand/common/cfe_api.h"

#include <machine/autoconf.h>

#if !defined(UNIFIED_BOOTBLOCK) && !defined(SECONDARY_BOOTBLOCK)
#error not UNIFIED_BOOTBLOCK and not SECONDARY_BOOTBLOCK
#endif

char boot_file[128];
char boot_flags[128];
extern int debug;

struct bootinfo_v1 bootinfo;

paddr_t ffp_save, ptbr_save;

int debug;

char *kernelnames[] = {
	"netbsd",		"netbsd.gz",
	"netbsd.bak",		"netbsd.bak.gz",
	"netbsd.old",		"netbsd.old.gz",
	"onetbsd",		"onetbsd.gz",
	"netbsd.mips",		"netbsd.mips.gz",
	NULL
};

int
#if defined(UNIFIED_BOOTBLOCK)
main(long fwhandle,long evector,long fwentry,long fwseal)
#else /* defined(SECONDARY_BOOTBLOCK) */
main(long fwhandle,long fd,long fwentry)
#endif
{
	char *name, **namep;
	u_long marks[MARK_MAX];
	u_int32_t entry;
	int win;

	/* Init prom callback vector. */
	cfe_init(fwhandle,fwentry);

	/* Init the console device */
	init_console();

	/* print a banner */
	printf("\n");
	printf("NetBSD/sbmips " NETBSD_VERS " " BOOT_TYPE_NAME " Bootstrap, Revision %s\n",
	    bootprog_rev);
	printf("(%s, %s)\n", bootprog_maker, bootprog_date);
	printf("\n");

	/* set up the booted device descriptor */
#if defined(UNIFIED_BOOTBLOCK)
        if (!booted_dev_open()) {
                printf("Boot device (%s) open failed.\n",
		    booted_dev_name[0] ? booted_dev_name : "unknown");
                goto fail;
        }
#else /* defined(SECONDARY_BOOTBLOCK) */
	booted_dev_setfd(fd);
#endif

	printf("\n");

	boot_file[0] = 0;
	cfe_getenv("KERNEL_FILE",boot_file,sizeof(boot_file));

	boot_flags[0] = 0;
	cfe_getenv("BOOT_FLAGS",boot_flags,sizeof(boot_flags));

	if (boot_file[0] != 0)
		(void)printf("Boot file: %s\n", boot_file);
	(void)printf("Boot flags: %s\n", boot_flags);

	if (strchr(boot_flags, 'i') || strchr(boot_flags, 'I')) {
		printf("Boot file: ");
		gets(boot_file);
	}

	memset(marks, 0, sizeof marks);
	if (boot_file[0] != '\0') {
		name = boot_file;
		win = loadfile(name, marks, LOAD_KERNEL) == 0;
	} else {
		name = NULL;	/* XXX gcc -Wuninitialized */
		for (namep = kernelnames, win = 0; *namep != NULL && !win;
		    namep++)
			win = loadfile(name = *namep, marks, LOAD_KERNEL) == 0;
	}

	entry = marks[MARK_ENTRY];
	booted_dev_close();
	printf("\n");
	if (!win)
		goto fail;

	(void)printf("Entering %s at 0x%lx...\n", name, (long)entry);

	cfe_flushcache(0);

	memset(&bootinfo, 0,sizeof(bootinfo));
	bootinfo.version = BOOTINFO_VERSION;
	bootinfo.reserved = 0;
	bootinfo.ssym = marks[MARK_SYM];
	bootinfo.esym = marks[MARK_END];
	strncpy(bootinfo.boot_flags,boot_flags,sizeof(bootinfo.boot_flags));
	strncpy(bootinfo.booted_kernel,name,sizeof(bootinfo.booted_kernel));
	bootinfo.fwhandle = fwhandle;
	bootinfo.fwentry = fwentry;

	(*(void (*)(long,long,long,long))entry)(fwhandle,
						BOOTINFO_MAGIC,
						(long)&bootinfo,
						fwentry);

	(void)printf("KERNEL RETURNED!\n");

fail:
	(void)printf("Boot failed!  Halting...\n");
	halt();
	return 1;
}



