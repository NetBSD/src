/*	$NetBSD: boot.c,v 1.2 2003/08/07 16:27:17 agc Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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

#include <machine/cpu.h>
#include <machine/leds.h>
#include <machine/vmparam.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include "boot.h"
#include "cons.h"
#include "common.h"

char *kernelnames[] = {
	"netbsd",
	"netbsd.gz",
	"onetbsd",
	"onetbsd.gz",
	"netbsd.bak",
	"netbsd.bak.gz",
	"netbsd.old",
	"netbsd.old.gz",
	"netbsd.cobalt",
	"netbsd.cobalt.gz",
	"netbsd.elf",
	"netbsd.elf.gz",
	NULL
};

extern long end;
static char *bootstring;

static int patch_bootstring	(char *bootspec);
static int get_bsdbootname	(char **dev, char **name, char **kname);
static int prominit		(unsigned int memsize);
static int print_banner		(unsigned int memsize);

void start(void);
int cpu_reboot(void);
int main(unsigned int memsize);

/*
 * Perform CPU reboot.
 */
int
cpu_reboot()
{
	printf("rebooting...\n\n");

	*(volatile char *)MIPS_PHYS_TO_KSEG1(LED_ADDR) = LED_RESET;
	printf("WARNING: reboot failed!\n");

	for (;;);
}

/*
 * Substitute root value with NetBSD root partition name.
 */
int
patch_bootstring(bootspec)
	char *bootspec;
{
	char *sp = bootstring;
	u_int8_t unit, part;
	int dev, error;
	char *file;

	DPRINTF(("patch_bootstring: %s\n", bootspec));

	/* get boot parameters */
	if (devparse(bootspec, &dev, &unit, &part, (const char **)&file) != 0)
		unit = part = 0;

	DPRINTF(("patch_bootstring: %d, %d\n", unit, part));

	/* take out the 'root=xxx' parameter */
	if ( (sp = strstr(bootstring, "root=")) != NULL) {
		const char *end;
		
		end = strchr(sp, ' ');

		/* strip off leading spaces */
		for (--sp; (sp > bootstring) && (*sp == ' '); --sp)
			;

		if (end != NULL)
			strcpy(++sp, end);
		else
			*++sp = '\0';
	}

	DPRINTF(("patch_bootstring: [%s]\n", bootstring));

#define DEVNAMESIZE	(MAXDEVNAME + sizeof(" root=/dev/hd") + sizeof("0a"))
	/* bsd notation -> linux notation (wd0a -> hda1) */
	if (strlen(bootstring) <= (511 - DEVNAMESIZE)) {
		int len;

		strcat(bootstring, " root=/dev/hd");

		len = strlen(bootstring);
		bootstring[len++] = unit + 'a';
		bootstring[len++] = part + '1';
		bootstring[len++] = '\0';
	}

	DPRINTF(("patch_bootstring: -> %s\n", bootstring));
	return (0);
}

/*
 * Extract NetBSD boot specification
 */
int
get_bsdbootname(dev, name, kname)
	char **dev;
	char **name;
	char **kname;
{
	char *spec;

	*dev = NULL;
	*name = NULL;
	*kname = NULL;

	if ( (spec = strstr(bootstring, "nbsd=")) != NULL) {
		int len;
		char *ptr = strchr(spec, ' ');

		spec += 5; 	/* skip 'nbsd=' */
		len = (ptr == NULL) ? strlen(spec) : ptr - spec;

		if (len > 0) {
			char *devname = alloc(len + 1);
			if (devname != NULL) {
				memcpy(devname, spec, len);
				devname[len] = '\0';

				if ( (ptr = memchr(devname,':',len)) != NULL) {
					/* wdXX:kernel */
					*ptr = '\0';
					*dev = devname;

					if (*++ptr)
						*kname = ptr;

					devname = alloc(len + 1);
					if (devname != NULL) {
						memcpy(devname, spec, len);
						devname[len] = '\0';
					}
				}

				*name = devname;
				return (0);
			}
		}
	}

	return (1);
}

/*
 * Get the bootstring from PROM.
 */
int
prominit(memsize)
	unsigned int memsize;
{
	bootstring = (char *)(memsize - 512);
	bootstring[511] = '\0';
}

/*
 * Print boot message.
 */
int
print_banner(memsize)
	unsigned int memsize;
{
	printf("\n");
	printf(">> %s " NETBSD_VERS " Bootloader, Revision %s [@%p]\n",
			bootprog_name, bootprog_rev,
			(unsigned long)&start & ~PAGE_MASK);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
	printf(">> Memory:\t\t%u k\n", (memsize - MIPS_KSEG0_START) / 1024);
	printf(">> PROM boot string:\t%s\n", bootstring);
}

/*
 * Entry point.
 * Parse PROM boot string, load the kernel and jump into it
 */
int
main(memsize)
	unsigned int memsize;
{
	char *name, **namep, *dev, *kernel, *spec;
	char bootpath[PATH_MAX];
	int win;
	u_long marks[MARK_MAX];
	void (*entry) __P((unsigned int));

	int addr, speed;

	prominit(memsize);
	cninit(&addr, &speed);

	print_banner(memsize);

	memset(marks, 0, sizeof marks);
	get_bsdbootname(&dev, &name, &kernel);

	if (kernel != NULL) {
		DPRINTF(("kernel: %s\n", kernel));
		patch_bootstring(name);
		win = (loadfile(name, marks, LOAD_KERNEL) == 0);
	} else {
		win = 0;
		DPRINTF(("kernel: NULL\n"));
		for (namep = kernelnames, win = 0;
				(*namep != NULL) && !win;
				namep++) {
			kernel = *namep;

			bootpath[0] = '\0';

			strcpy(bootpath, (dev != NULL) ? dev : "wd0a");
			strcat(bootpath, ":");
			strcat(bootpath, kernel);

			printf("Loading: %s\n", bootpath);
			patch_bootstring(bootpath);
			win = (loadfile(bootpath, marks, LOAD_ALL) != -1);
		}
	}

	if (win) {
		entry = (void *) marks[MARK_ENTRY];

		printf("Starting at 0x%x\n\n", (u_int)entry);
		(*entry)(memsize);
	}

	(void)printf("Boot failed! Rebooting...\n");
	return (0);
}
