/*	$NetBSD: boot.c,v 1.18 2009/12/16 19:01:24 matt Exp $	*/

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
#include <lib/libsa/dev_net.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <machine/cpu.h>

#include <cobalt/dev/gtreg.h>

#include "boot.h"
#include "cons.h"
#include "common.h"
#include "bootinfo.h"

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

u_int cobalt_id;
static const char * const cobalt_model[] =
{
	[0]                  = "Unknown Cobalt",
	[COBALT_ID_QUBE2700] = "Cobalt Qube 2700",
	[COBALT_ID_RAQ]      = "Cobalt RaQ",
	[COBALT_ID_QUBE2]    = "Cobalt Qube 2",
	[COBALT_ID_RAQ2]     = "Cobalt RaQ 2"
};
#define COBALT_MODELS	__arraycount(cobalt_model)

extern u_long end;		/* Boot loader code end address */
void start(void);

static char *bootstring;

static int patch_bootstring(char *bootspec);
static int get_bsdbootname(char **, char **, int *);
static int parse_bootname(char *, int, char **, char **);
static void prominit(unsigned int memsize);
static void print_banner(unsigned int memsize);
static u_int read_board_id(void);

void cpu_reboot(void);

int main(unsigned int memsize);

/*
 * Perform CPU reboot.
 */
void
cpu_reboot(void)
{

	printf("rebooting...\n\n");

	*(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(LED_ADDR) = LED_RESET;
	printf("WARNING: reboot failed!\n");

	for (;;)
		;
}

/*
 * Substitute root value with NetBSD root partition name.
 */
int
patch_bootstring(char *bootspec)
{
	char *sp = bootstring;
	uint8_t unit, part;
	int dev;
	char *file;

	DPRINTF(("patch_bootstring: %s\n", bootspec));

	/* get boot parameters */
	if (devparse(bootspec, &dev, &unit, &part, (const char **)&file) != 0)
		unit = part = 0;

	DPRINTF(("patch_bootstring: unit = %d, part = %d\n", unit, part));

	/* take out the 'root=xxx' parameter */
	if ((sp = strstr(bootstring, "root=")) != NULL) {
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
	if (strcmp(devsw[dev].dv_name, "wd") == 0 &&
	    strlen(bootstring) <= (511 - DEVNAMESIZE)) {
		int len;

		/* omit "nfsroot=" arg on wd boot */
		if ((sp = strstr(bootstring, "nfsroot=")) != NULL) {
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

		/* bsd notation -> linux notation (wd0a -> hda1) */
		strcat(bootstring, " root=/dev/hd");

		len = strlen(bootstring);
		bootstring[len++] = unit + 'a';
		bootstring[len++] = part + '1';
		bootstring[len++] = '\0';
	}

	DPRINTF(("patch_bootstring: -> %s\n", bootstring));
	return 0;
}

/*
 * Extract NetBSD boot specification
 */
static int
get_bsdbootname(char **dev, char **kname, int *howtop)
{
	int len;
	int bootunit, bootpart;
	char *bootstr_dev, *bootstr_kname;
	char *prompt_dev, *prompt_kname;
	char *ptr, *spec;
	char c, namebuf[PATH_MAX];
	static char bootdev[] = "wd0a";
	static char nfsbootdev[] = "nfs";

	bootstr_dev = prompt_dev = NULL;
	bootstr_kname = prompt_kname = NULL;

	/* first, get root device specified by the firmware */
	spec = bootstring;

	/* assume the last one is valid */
	while ((spec = strstr(spec, "root=")) != NULL) {
		spec += 5;	/* skip 'root=' */
		ptr = strchr(spec, ' ');
		len = (ptr == NULL) ? strlen(spec) : ptr - spec;
		/* decode unit and part from "/dev/hd[ab][1-4]" strings */
		if (len == 9 && memcmp("/dev/hd", spec, 7) == 0) {
			bootunit = spec[7] - 'a';
			bootpart = spec[8] - '1';
			if (bootunit >= 0 && bootunit < 2 &&
			    bootpart >= 0 && bootpart < 4) {
				bootdev[sizeof(bootdev) - 3] = '0' + bootunit;
#if 0				/* bootpart is fdisk partition of Linux root */
				bootdev[sizeof(bootdev) - 2] = 'a' + bootpart;
#endif
				bootstr_dev = bootdev;
			}
		}
		spec += len;
	}

	/* second, get bootname from bootstrings */
	if ((spec = strstr(bootstring, "nbsd=")) != NULL) {
		ptr = strchr(spec, ' ');
		spec += 5; 	/* skip 'nbsd=' */
		len = (ptr == NULL) ? strlen(spec) : ptr - spec;
		if (len > 0) {
			if (parse_bootname(spec, len,
			    &bootstr_dev, &bootstr_kname))
				return 1;
		}
	}

	/* third, check if netboot */
	if (strstr(bootstring, "nfsroot=") != NULL) {
		bootstr_dev = nfsbootdev;
	}

	DPRINTF(("bootstr_dev = %s, bootstr_kname = %s\n",
	    bootstr_dev ? bootstr_dev : "<NULL>",
	    bootstr_kname ? bootstr_kname : "<NULL>"));

	spec = NULL;
	len = 0;

	memset(namebuf, 0, sizeof namebuf);
	printf("Boot [%s:%s]: ",
	    bootstr_dev ? bootstr_dev : DEFBOOTDEV,
	    bootstr_kname ? bootstr_kname : DEFKERNELNAME);

	if (tgets(namebuf) == -1)
		printf("\n");

	ptr = namebuf;
	while ((c = *ptr) != '\0') {
		while (c == ' ')
			c = *++ptr;
		if (c == '\0')
			break;
		if (c == '-') {
			while ((c = *++ptr) && c != ' ')
				BOOT_FLAG(c, *howtop);
		} else {
			spec = ptr;
			while ((c = *++ptr) && c != ' ')
				;
			if (c)
				*ptr++ = '\0';
			len = strlen(spec);
		}
	}

	if (len > 0) {
		if (parse_bootname(spec, len, &prompt_dev, &prompt_kname))
			return 1;
	}

	DPRINTF(("prompt_dev = %s, prompt_kname = %s\n",
	    prompt_dev ? prompt_dev : "<NULL>",
	    prompt_kname ? prompt_kname : "<NULL>"));

	if (prompt_dev)
		*dev = prompt_dev;
	else
		*dev = bootstr_dev;

	if (prompt_kname)
		*kname = prompt_kname;
	else
		*kname = bootstr_kname;

	DPRINTF(("dev = %s, kname = %s\n",
	    *dev ? *dev : "<NULL>",
	    *kname ? *kname : "<NULL>"));

	return 0;
}

static int
parse_bootname(char *spec, int len, char **dev, char **kname)
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
			*kname = ptr;
	} else
		/* "kernel" */
		*kname = bootname;
	return 0;
}

/*
 * Get the bootstring from PROM.
 */
void
prominit(unsigned int memsize)
{

	bootstring = (char *)(memsize - 512);
	bootstring[511] = '\0';
}

/*
 * Print boot message.
 */
void
print_banner(unsigned int memsize)
{

	lcd_banner();

	printf("\n");
	printf(">> %s " NETBSD_VERS " Bootloader, Revision %s [@%p]\n",
			bootprog_name, bootprog_rev, (void*)&start);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
	printf(">> Model:\t\t%s\n", cobalt_model[cobalt_id]);
	printf(">> Memory:\t\t%lu k\n", (memsize - MIPS_KSEG0_START) / 1024);
	printf(">> PROM boot string:\t%s\n", bootstring);
}

u_int
read_board_id(void)
{
	uint32_t tag, reg;

#define PCIB_PCI_BUS		0
#define PCIB_PCI_DEV		9
#define PCIB_PCI_FUNC		0
#define PCIB_BOARD_ID_REG	0x94
#define COBALT_BOARD_ID(reg)	((reg & 0x000000f0) >> 4)

	tag = (PCIB_PCI_BUS << 16) | (PCIB_PCI_DEV << 11) |
	    (PCIB_PCI_FUNC << 8);
	reg = pcicfgread(tag, PCIB_BOARD_ID_REG);

	return COBALT_BOARD_ID(reg);
}

/*
 * Entry point.
 * Parse PROM boot string, load the kernel and jump into it
 */
int
main(unsigned int memsize)
{
	char **namep, *dev, *kernel, *bi_addr;
	char bootpath[PATH_MAX];
	int win;
	u_long marks[MARK_MAX];
	void (*entry)(unsigned int, u_int, char *);

	struct btinfo_flags bi_flags;
	struct btinfo_symtab bi_syms;
	struct btinfo_bootpath bi_bpath;
	struct btinfo_howto bi_howto;
	int addr, speed, howto;

	try_bootp = 1;

	/* Initialize boot info early */
	dev = NULL;
	kernel = NULL;
	howto = 0x0;
	bi_flags.bi_flags = 0x0;
	bi_addr = bi_init();

	lcd_init();
	cobalt_id = read_board_id();
	prominit(memsize);
	if (cninit(&addr, &speed) != NULL)
		bi_flags.bi_flags |= BI_SERIAL_CONSOLE;

	print_banner(memsize);

	memset(marks, 0, sizeof marks);
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

		lcd_loadfile(bootpath);
		printf("Loading: %s", bootpath);
		if (howto)
			printf(" (howto 0x%x)", howto);
		printf("\n");
		patch_bootstring(bootpath);
		win = (loadfile(bootpath, marks, LOAD_ALL) != -1);
	}

	if (win) {
		strncpy(bi_bpath.bootpath, kernel, BTINFO_BOOTPATH_LEN);
		bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));

		entry = (void *)marks[MARK_ENTRY];
		bi_syms.nsym = marks[MARK_NSYM];
		bi_syms.ssym = marks[MARK_SYM];
		bi_syms.esym = marks[MARK_END];
		bi_add(&bi_syms, BTINFO_SYMTAB, sizeof(bi_syms));

		bi_add(&bi_flags, BTINFO_FLAGS, sizeof(bi_flags));

		bi_howto.bi_howto = howto;
		bi_add(&bi_howto, BTINFO_HOWTO, sizeof(bi_howto));

		entry = (void *)marks[MARK_ENTRY];

		DPRINTF(("Bootinfo @ 0x%x\n", bi_addr));
		printf("Starting at 0x%x\n\n", (u_int)entry);
		(*entry)(memsize, BOOTINFO_MAGIC, bi_addr);
	}

	delay(20000);
	lcd_failed();
	(void)printf("Boot failed! Rebooting...\n");
	return 0;
}
