#define	DEBUG
/*	$NetBSD: boot.c,v 1.4.2.1 2006/02/01 14:51:37 yamt Exp $	*/

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
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <machine/cpu.h>

#include "cache.h"
#include "extern.h"
#include "ofdev.h"
#include "openfirm.h"

#ifdef DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (/*CONSTCOND*/0) printf
#endif

char bootdev[128];
char bootfile[128];
int boothowto;
int debug;

#ifdef notyet
static int ofw_version = 0;
#endif
static const char *kernels[] = {
    "/netbsd", "/netbsd.gz", "/netbsd.shark", NULL
};

static void
prom2boot(char *dev)
{
	char *cp, *ocp;
	
	ocp = dev;
	cp = dev + strlen(dev) - 1;
	for (; cp >= ocp; cp--) {
		if (*cp == ':') {
			*cp = '\0';
			return;
		}
	}
}

static void
parseargs(char *str, int *howtop)
{
	char *cp;

	/* Allow user to drop back to the PROM. */
	if (strcmp(str, "exit") == 0)
		OF_exit();

	*howtop = 0;

	for (cp = str; *cp; cp++)
		if (*cp == ' ' || *cp == '-')
			goto found;
	
	return;

 found:
	*cp++ = '\0';
	while (*cp)
		BOOT_FLAG(*cp++, *howtop);
}

static void
chain(void (*entry)(int (*)(void *), void *, u_int), char *args, void *ssym,
    void *esym)
{
	extern char end[];
	u_int l, magic = 0x19730224;

	freeall();

	/*
	 * Stash pointer to start and end of symbol table after the argument
	 * strings.
	 */
	l = strlen(args) + 1;
	l = (l + 3) & ~3;			/* align */
	DPRINTF("magic @ %p\n", args + l);
	memcpy(args + l, &magic, sizeof(magic));
	l += sizeof(magic);
	DPRINTF("ssym @ %p\n", args + l);
	memcpy(args + l, &ssym, sizeof(ssym));
	l += sizeof(ssym); 
	DPRINTF("esym @ %p\n", args + l);
	memcpy(args + l, &esym, sizeof(esym));
	l += sizeof(esym);
	DPRINTF("args + l -> %p\n", args + l);

	DPRINTF("Calling OF_chain(%p, %tx, %p, %p, %u)\n",
	    (void *)RELOC, end - (char *)RELOC, entry, args, l);
	OF_chain((void *)RELOC, end - (char *)RELOC, entry, args, l);
	panic("chain");
}

__dead void
_rtt(void)
{

	OF_exit();
}

void
main(void)
{
	extern char bootprog_name[], bootprog_rev[],
		    bootprog_maker[], bootprog_date[];
	int chosen;
	char bootline[512];		/* Should check size? */
	char *cp, *startbuf, *endbuf;
	u_long marks[MARK_MAX], size;
	u_int32_t entry;
	void *ssym, *esym;

	printf("\n");
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
	DPRINTF("bootline=%s\n", bootline);

	/*
	 * Per the ARM OpenFirmware bindings, the firmware must
	 * allocate and map at least 6MB of physical memory starting
	 * at VA 0xf0000000.  We have been loaded at 0xf0000000,
	 * and the memory after us has been unmapped and freed.
	 * We expect to load the kernel at 0xf0100000, so we will
	 * allocate 5MB of virtual memory starting there, and
	 * unmap/free what we don't use.
	 */
	startbuf = OF_claim((void *) 0xf0100000, (5 * 1024 * 1024), 0);
	if (startbuf != (void *) 0xf0100000) {
		printf("Unable to claim buffer for kernel\n");
		OF_exit();
	}
	endbuf = startbuf + (5 * 1024 * 1024);

	for (;;) {
		int i;

		if (boothowto & RB_ASKNAME) {
			printf("Boot: ");
			gets(bootline);
			parseargs(bootline, &boothowto);
		}

		if (bootline[0]) {
			kernels[0] = bootline;
			kernels[1] = NULL;
		}

		for (i = 0; kernels[i]; i++) {
			DPRINTF("Trying %s\n", kernels[i]);

			marks[MARK_START] = 0xf0100000;
			if (loadfile(kernels[i], marks, LOAD_KERNEL) >= 0)
				goto loaded;
		}

		boothowto |= RB_ASKNAME;
	}
 loaded:
	/*
	 * Okay, kernel is loaded, free the extra memory at the end.
	 * Round to the ARM OpenFirmare page size (4k).
	 */
	cp = (char *) ((marks[MARK_END] + 0xfff) & ~0xfff);
	size = (u_long) (endbuf - cp);
	if (size)
		OF_release(cp, size);

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

	entry = marks[MARK_ENTRY];
	ssym = (void *)marks[MARK_SYM];
	esym = (void *)marks[MARK_END];

	printf(" start=0x%x\n", entry);

	if (cache_syncI != NULL) {
		DPRINTF("Syncing I$...\n");
		(*cache_syncI)();
	}

	chain((void *)entry, bootline, ssym, esym);

	OF_exit();
}
