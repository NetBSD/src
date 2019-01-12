/*	$NetBSD: iris_boot.c,v 1.1 2019/01/12 16:44:47 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Naruaki Etomi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Silicon Graphics "IRIS" series MIPS processors machine bootloader.
 *
 * Notes:
 *   The amount of physical memory space available to
 *   the system is 3661820 (0x80002000 - 0x8037fffc) bytes.
 *   This space is too tight for kernel and bootloader.
 *   So we keep it simple.
 */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/boot_flag.h>

#ifndef	INDIGO_R3K_MODE
#include <dev/arcbios/arcbios.h>
#endif

#include "iris_machdep.h"

#include "common.h"
#include "bootinfo.h"

int main(int, char **);

/* Storage must be static. */
struct btinfo_symtab bi_syms;
struct btinfo_bootpath bi_bpath;

static uint8_t bootinfo[BOOTINFO_SIZE];

/*
 * This gets arguments from the PROM monitor.
 * argv[0] will be path to the bootloader (i.e., "dksc(X,Y,8)/bootiris").
 *
 * argv[1] through argv[n] will contain arguments passed from the PROM, if any.
 */

int
main(int argc, char **argv)
{
	char kernelname[1 + 32];
	void (*entry) (int, char *[], int, void *);
	u_long marks[MARK_MAX];
	int win = 0;
	int zs_addr, speed;

	cninit(&zs_addr, &speed);

 	/* print a banner */
	printf("\n");
	printf("%s " NETBSD_VERS " Yet another Bootstrap, Revision %s\n",
	    bootprog_name, bootprog_rev);
	printf("\n");

	memset(marks, 0, sizeof marks);

	/* initialise bootinfo structure early */
	bi_init(bootinfo);

	switch (argc) {
#ifdef INDIGO_R3K_MODE
	case 1:
		again();
		break;
#endif
	case 2:
		/* To specify HDD on Indigo R3K */ 
		if (strstr(argv[1], "dksc(")) {
			parse(argv, kernelname);
		} else {
			again();
		}
		break;
	default:
		/* To specify HDD on Indigo R4K and Indy */ 
		if (strstr(argv[1], "dksc(")) {
			parse(argv, kernelname);
		} else {
			again();
		}
		break;
	}

	find_devs();

	win = loadfile(kernelname, marks, LOAD_KERNEL);

	if (win < 0) {
		printf("Boot failed!  Halting...\n");
		reboot();
	}

	strlcpy(bi_bpath.bootpath, kernelname, BTINFO_BOOTPATH_LEN);
	bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));

	bi_syms.nsym = marks[MARK_NSYM];
	bi_syms.ssym = marks[MARK_SYM];
	bi_syms.esym = marks[MARK_END];
	bi_add(&bi_syms, BTINFO_SYMTAB, sizeof(bi_syms));
	entry = (void *)marks[MARK_ENTRY];

	(*entry)(argc, argv, BOOTINFO_MAGIC, bootinfo);

	printf("Kernel returned!  Halting...\n");
	return 0;
}

void
again(void)
{
	printf("Invalid argument\n");
	printf("i.e., dksc(0,X,8)loader dksc(0,X,0)/kernel\n");
	reboot();
}

void
reboot(void)
{
#ifdef INDIGO_R3K_MODE
	romrestart();
#else
	arcbios_Reboot();
#endif
}
