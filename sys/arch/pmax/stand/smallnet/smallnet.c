/*	$NetBSD: smallnet.c,v 1.3 1999/11/13 21:33:13 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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

#include <stand.h>
#include <sys/param.h>
#include <sys/exec_elf.h>
#include <machine/dec_prom.h>
#include <lib/libz/zlib.h>
#include <lib/libkern/libkern.h>

#include "common.h"
#include "bootinfo.h"


typedef void (*entrypt) __P((int, char **, int, const void *));

int main __P((int, char **));

/*
 * These variables and array will be patched to contain a kernel image
 * and some information about the kernel.
 */

int maxkernel_size = KERNELSIZE;
entrypt kernel_entry = 0 /* (entrypt)0x80030000 */ /* XXX XXX XXX */;
u_long kernel_loadaddr = 0 /* 0x80030000 */ /* XXX XXX XXX */;
int kernel_size = 0 /* 387321 */ /* XXX XXX XXX */;
char kernel_image[KERNELSIZE] = "|This is the kernel image!\n";


/*
 * This gets arguments from the PROM, calls other routines to open
 * and load the secondary boot loader called boot, and then transfers
 * execution to that program.
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
	int ret;
	char *name;
	uLongf destlen;
	struct btinfo_bootpath bi_bpath;

	printf("NetBSD/pmax " NETBSD_VERS " " BOOT_TYPE_NAME
	    " Bootstrap, Revision %s\n", bootprog_rev);
	printf("(%s, %s)\n", bootprog_maker, bootprog_date);

	/* initialise bootinfo structure early */
	bi_init(BOOTINFO_ADDR);

	/* check for DS5000 boot */
	if (strcmp(argv[0], "boot") == 0) {
		argc--;
		argv++;
	}
	name = *argv;

	strncpy(bi_bpath.bootpath, name, BTINFO_BOOTPATH_LEN);
	bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));

	destlen = RELOC - kernel_loadaddr;
	printf("\n");
	printf("Decompressing %d bytes to 0x%lx\n", kernel_size,
	    kernel_loadaddr);
	ret = uncompress((Bytef *)kernel_loadaddr, &destlen, kernel_image,
	    kernel_size);
	if (ret != Z_OK) {
		printf("Error decompressing kernel\n");
		printf("libz error %d\n", ret);
		return (1);
	}

	if (callv == &callvec)
		kernel_entry(argc, argv, 0, 0);
	else
		kernel_entry(argc, argv, DEC_PROM_MAGIC, callv);
	return (1);
}
