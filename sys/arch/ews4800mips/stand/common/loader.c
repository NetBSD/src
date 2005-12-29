/*	$NetBSD: loader.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <lib/libsa/loadfile.h>

#include <machine/bootinfo.h>

#include "console.h"
#include "cmd.h"
#include "local.h"

void r4k_pdcache_wbinv_all(uint32_t);
void r4k_sdcache_wbinv_all(uint32_t, int);
void boot_usage(void);

int
cmd_jump(int argc, char *argp[], int interactive)
{
	uint32_t addr;
	uint32_t sp;

	if (argc < 2) {
		printf("jump jump_addr [stack_addr]\n");
		return 1;
	}
	addr = strtoul(argp[1], 0, 0);
	if (argc == 3)
		sp = strtoul(argp[2], 0, 0);
	else
		__asm volatile("move %0, $29" : "=r"(sp)); /* current stack */

	printf("jump to %p. sp=%p Y/N\n", addr, sp);
	if (prompt_yesno(interactive)) {
		__asm volatile(
			".set noreorder;"
			"jalr	%0;"
			"move	$29, %1;"
			".set reorder" :: "r"(addr), "r"(sp));
		/* NOTREACHED */
	} else {
		printf("canceled.\n");
		return 1;
	}

	return 0;
}

int
cmd_load_binary(int argc, char *argp[], int interactive)
{
	extern uint8_t kernel_binary[];
	extern int kernel_binary_size;
	extern char start[];
	uint8_t *p, *q;
	int i, j;

	if (argc < 2) {
		printf("load load_addr\n");
		return 1;
	}

	if (kernel_binary_size == 0) {
		printf("no kernel image\n");
		return 1;
	}

	q = (uint8_t *)strtoul(argp[1], 0, 0);
	p = kernel_binary;

	/* check load reagion */
	printf("load end=%p loader start=%p\n",
	    q + kernel_binary_size, start);
	if ((uint32_t)(q + kernel_binary_size) >= (uint32_t)start) {
		printf("kernel load area is overlapped with loader.\n");
		return 1;
	}

	printf("load kernel to %p %dbytes. Y/N\n", q, kernel_binary_size);
	if (!prompt_yesno(interactive)) {
		printf("canceled.\n");
		return 1;
	}

	j = kernel_binary_size / 30;
	for (i = 0; i < kernel_binary_size; i ++) {
		*q++ = *p++;
		if ((i % j) == 0)
			printf("loading kernel. %d/%dbytes\r", i,
			    kernel_binary_size);
	}
	r4k_pdcache_wbinv_all(PD_CACHE_SIZE);
	r4k_sdcache_wbinv_all(SD_CACHE_SIZE, SD_CACHE_LINESIZE);

	printf("loading kernel. %d/%d\r", i , kernel_binary_size);
	printf("\ndone.\n");

	return 0;
}

int
cmd_boot_ux(int argc, char *argp[], int interactive)
{
	u_long marks[MARK_MAX];
	uint32_t entry;

	marks[MARK_START] = 0;
	console_cursor(FALSE);
	if (loadfile("sd0d:iopboot", marks, LOAD_KERNEL) != 0) {
		printf("load iopboot failed.\n");
		return 1;
	}
	printf("start=%x entry=%x nsym=%x sym=%x end=%x\n",
	    marks[MARK_START], marks[MARK_ENTRY], marks[MARK_NSYM],
	    marks[MARK_SYM], marks[MARK_END]);

	entry = marks[MARK_ENTRY];
	printf("jump to iopboot entry.(0x%x) Y/N\n", entry);

	r4k_pdcache_wbinv_all(PD_CACHE_SIZE);
	r4k_sdcache_wbinv_all(SD_CACHE_SIZE, SD_CACHE_LINESIZE);

	if (prompt_yesno(interactive)) {
		__asm volatile(
			".set noreorder;"
			"lw	$4, %1;"
			"lw	$2, %2;"
			"lw	$3, %3;"
			"jr	%0;"
			"move	$29, %0;"
			".set reorder"
			:: "r"(entry),
			"m"(ipl_args.a0),
			"m"(ipl_args.v0),
			"m"(ipl_args.v1));
		/* NOTREACHED */
	}
	console_cursor(TRUE);

	return 0;
}

int
cmd_boot(int argc, char *argp[], int interactive)
{
	u_long marks[MARK_MAX];
	uint32_t entry;
	struct bootinfo bi;
	char *filename;

	if (argc < 2) {
		boot_usage();
		return 1;
	} else {
		filename = argp[1];
	}

	marks[MARK_START] = 0;
	console_cursor(FALSE);
	if (loadfile(filename, marks, LOAD_KERNEL) != 0) {
		printf("load file failed.\n");
		return 1;
	}
	printf("start=%x entry=%x nsym=%x sym=%x end=%x\n",
	    marks[MARK_START], marks[MARK_ENTRY], marks[MARK_NSYM],
	    marks[MARK_SYM], marks[MARK_END]);

	entry = marks[MARK_ENTRY];
	printf("jump to kernel entry.(0x%x)%s\n", entry,
	    interactive ? " Y/N" : "");

	/* Setup argument */
	bi.bi_version = 0x1;
	bi.bi_size = sizeof bi;
	bi.bi_nsym = marks[MARK_NSYM];
	bi.bi_ssym = (uint8_t *)marks[MARK_SYM];
	bi.bi_esym = (uint8_t *)marks[MARK_END];
	bi.bi_mainfo = ipl_args.v1;

	r4k_pdcache_wbinv_all(PD_CACHE_SIZE);
	r4k_sdcache_wbinv_all(SD_CACHE_SIZE, SD_CACHE_LINESIZE);

	if (prompt_yesno(interactive)) {
		__asm volatile(
			".set noreorder;"
			"lw	$4, %1;"
			"lw	$5, %2;"
			"la	$6, %3;"
			"jr	%0;"
			"move	$29, %0;"
			".set reorder"
			:: "r"(entry), "m"(argc), "m"(argp), "m"(bi));
		/* NOTREACHED */
	}
	console_cursor(TRUE);

	return 0;
}

void
r4k_pdcache_wbinv_all(uint32_t pdcache_size)
{
	uint32_t va = 0x80000000;
	uint32_t eva = va + pdcache_size;

	while (va < eva) {
		__asm volatile(
			".set noreorder;"
			".set mips3;"
			"cache %1, 0x000(%0); cache %1, 0x010(%0);"
			"cache %1, 0x020(%0); cache %1, 0x030(%0);"
			"cache %1, 0x040(%0); cache %1, 0x050(%0);"
			"cache %1, 0x060(%0); cache %1, 0x070(%0);"
			"cache %1, 0x080(%0); cache %1, 0x090(%0);"
			"cache %1, 0x0a0(%0); cache %1, 0x0b0(%0);"
			"cache %1, 0x0c0(%0); cache %1, 0x0d0(%0);"
			"cache %1, 0x0e0(%0); cache %1, 0x0f0(%0);"
			"cache %1, 0x100(%0); cache %1, 0x110(%0);"
			"cache %1, 0x120(%0); cache %1, 0x130(%0);"
			"cache %1, 0x140(%0); cache %1, 0x150(%0);"
			"cache %1, 0x160(%0); cache %1, 0x170(%0);"
			"cache %1, 0x180(%0); cache %1, 0x190(%0);"
			"cache %1, 0x1a0(%0); cache %1, 0x1b0(%0);"
			"cache %1, 0x1c0(%0); cache %1, 0x1d0(%0);"
			"cache %1, 0x1e0(%0); cache %1, 0x1f0(%0);"
			".set reorder"
			: : "r" (va), "i" (1 |(0 << 2)) : "memory");
		va += (32 * 16);
	}
}

void
r4k_sdcache_wbinv_all(uint32_t sdcache_size, int line_size)
{
	uint32_t va = 0x80000000;
	uint32_t eva = va + sdcache_size;

	while (va < eva) {
		__asm volatile(
			".set noreorder;"
			".set mips3;"
			"cache %1, 0(%0);"
			".set reorder"
			: : "r" (va), "i" (3 |(0 << 2)) : "memory");
		va += line_size;
	}
}

void
boot_usage(void)
{

	printf("boot dev:filename [argument to kernel]\n");
	printf("\tex).\n");
	printf("\t Disk 0, Partition 10, /netbsd => sd0k:netbsd\n");
	printf("\t NFS, /netbsd => nfs:netbsd\n");
	printf("\t `kernel embeded in data section' => mem:\n");
}
