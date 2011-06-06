/*	$NetBSD: boot.c,v 1.1.8.2 2011/06/06 09:06:14 jruoho Exp $	*/

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
#include <sys/boot_flag.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <machine/cpu.h>

#include "boot.h"
#include "cons.h"
#include "common.h"
#include "bootinfo.h"

char *kernelnames[] = {
	"netbsd",
	"netbsd.gz",
	"onetbsd",
	"onetbsd.gz",

	NULL
};

extern u_long end;		/* Boot loader code end address */
void start(void);

static int get_bsdbootname(char **, char **, int *);
static int parse_bootname(char *, int, char **, char **);
static void print_banner(void);

int main(void);

/*
 * Extract NetBSD boot specification
 */
static int
get_bsdbootname(char **dev, char **kern, int *howtop)
{
	int len;
	char *ptr, *spec;
	char c, namebuf[PATH_MAX];

	spec = NULL;
	len = 0;

	memset(namebuf, 0, sizeof namebuf);
	printf("Boot [" DEFBOOTDEV ":%s]: ", DEFKERNELNAME);

	if (tgets(namebuf) == -1)
		printf("\n");

	ptr = namebuf;
	while ((c = *ptr) != '\0') {
		while (c == ' ')
			c = *++ptr;
		if (c == '\0')
			break;
		if (c == '-')
			while ((c = *++ptr) && c != ' ')
				BOOT_FLAG(c, *howtop);
		else {
			spec = ptr;
			while ((c = *++ptr) && c != ' ')
				;
			if (c)
				*ptr++ = '\0';
			len = strlen(spec);
		}
	}

	if (len > 0)
		if (parse_bootname(spec, len, dev, kern))
			return 1;

	DPRINTF(("dev = %s, kern = %s\n",
	    *dev ? *dev : "<NULL>",
	    *kern ? *kern : "<NULL>"));

	return 0;
}

static int
parse_bootname(char *spec, int len, char **dev, char **kern)
{
	char *bootname, *ptr;

	bootname = alloc(len + 1);
	if (bootname == NULL)
		return 1;
	memcpy(bootname, spec, len);
	bootname[len] = '\0';

	if ((ptr = memchr(bootname, ':', len)) != NULL) {
		/* "wdXX:kernel" */
		*ptr = '\0';
		*dev = bootname;
		if (*++ptr)
			*kern = ptr;
	} else
		/* "kernel" */
		*kern = bootname;
	return 0;
}

/*
 * Print boot message.
 */
static void
print_banner(void)
{

	printf("\n");
	printf(">> %s " NETBSD_VERS " Bootloader, Revision %s [@%p]\n",
			bootprog_name, bootprog_rev, (void*)&start);
}

int
main(void)
{
	char **namep, *dev, *kernel, *bi_addr;
	char bootpath[PATH_MAX];
	int win;
	u_long marks[MARK_MAX];
	void (*entry)(u_int, char *);

	struct btinfo_bootdev bi_bdev;
	struct btinfo_bootpath bi_bpath;
	struct btinfo_howto bi_howto;
	int howto;

	/* Initialize boot info early */
	dev = NULL;
	kernel = NULL;
	howto = 0x0;
	bi_addr = bi_init();

#if defined(SH4)
	tmu_init();
#endif
	cninit();

	print_banner();

	memset(marks, 0, sizeof(marks));
	get_bsdbootname(&dev, &kernel, &howto);

	if (kernel != NULL) {
		DPRINTF(("kernel: %s\n", kernel));
		kernelnames[0] = kernel;
		kernelnames[1] = NULL;
	} else {
		DPRINTF(("kernel: NULL\n"));
	}

	win = 0;
	DPRINTF(("Kernel names: %p\n", kernelnames));
	for (namep = kernelnames, win = 0; (*namep != NULL) && !win; namep++) {
		kernel = *namep;

		bootpath[0] = '\0';

		strcpy(bootpath, dev ? dev : DEFBOOTDEV);
		strcat(bootpath, ":");
		strcat(bootpath, kernel);

		printf("Loading: %s", bootpath);
		if (howto)
			printf(" (howto 0x%x)", howto);
		printf("\n");
		win = (loadfile(bootpath, marks, LOAD_ALL) != -1);
	}

	if (win) {
		if (dev != NULL)
			strncpy(bi_bdev.bootdev, dev, BTINFO_BOOTDEV_LEN);
		else
			strncpy(bi_bdev.bootdev, DEFBOOTDEV,
			    BTINFO_BOOTDEV_LEN);
		bi_add(&bi_bdev, BTINFO_BOOTDEV, sizeof(bi_bdev));

		strncpy(bi_bpath.bootpath, kernel, BTINFO_BOOTPATH_LEN);
		bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));

		bi_howto.bi_howto = howto;
		bi_add(&bi_howto, BTINFO_HOWTO, sizeof(bi_howto));

		entry = (void *)marks[MARK_ENTRY];

		DPRINTF(("Bootinfo @ 0x%lx\n", (u_long)bi_addr));
		printf("Starting at 0x%lx\n\n", (u_long)entry);

		delay(10000);	/* XXXX: Wait output to console. */

		(*entry)(BOOTINFO_MAGIC, bi_addr);
	}

	(void)printf("Boot failed! Rebooting...\n");
	delay(20000);
	return 0;
}

void
_rtt(void)
{

	/* XXXX */
}
