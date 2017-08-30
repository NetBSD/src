/*	$NetBSD: testldt.c,v 1.17 2017/08/30 15:46:19 maxv Exp $	*/

/*
 * Copyright (c) 1993 The NetBSD Foundation, Inc.
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

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <machine/segments.h>
#include <machine/sysarch.h>

static void
set_fs(unsigned int val)
{
	__asm volatile("mov %0,%%fs"::"r" ((unsigned short) val));
}

static unsigned char
get_fs_byte(const char *addr)
{
	unsigned char _v;

	__asm ("movb %%fs:%1,%0":"=q" (_v):"m" (*addr));
	return _v;
}

static int
check_desc(int desc)
{
	unsigned int sel = LSEL(desc, SEL_UPL);
	set_fs(sel);
	return get_fs_byte((char *)0);
}

static union descriptor *
make_sd(void *basep, unsigned limit, int type, int dpl, int seg32, int inpgs)
{
	static union descriptor d;
	unsigned long base = (unsigned long)basep;

	d.sd.sd_lolimit = limit & 0x0000ffff;
	d.sd.sd_lobase  = base & 0x00ffffff;
	d.sd.sd_type    = type & 0x01f;
	d.sd.sd_dpl     = dpl & 0x3;
	d.sd.sd_p       = 1;
	d.sd.sd_hilimit = (limit & 0x00ff0000) >> 16;
	d.sd.sd_xx      = 0;
	d.sd.sd_def32   = seg32?1:0;
	d.sd.sd_gran    = inpgs?1:0;
	d.sd.sd_hibase  = (base & 0xff000000) >> 24;

	return &d;
}

static const char *
seg_type_name[] = {
	"SYSNULL", "SYS286TSS", "SYSLDT", "SYS286BSY",
	"SYS286CGT", "SYSTASKGT", "SYS286IGT", "SYS286TGT",
	"SYSNULL2", "SYS386TSS", "SYSNULL3", "SYS386BSY",
	"SYS386CGT", "SYSNULL4", "SYS386IGT", "SYS386TGT",
	"MEMRO", "MEMROA", "MEMRW", "MEMRWA",
	"MEMROD", "MEMRODA", "MEMRWD", "MEMRWDA",
	"MEME", "MEMEA", "MEMER", "MEMERA",
	"MEMEC", "MEMEAC", "MEMERC", "MEMERAC"
};

static const char *
segtype(unsigned int type)
{
	if (type >= 32)
		return "Out of range";

	return seg_type_name[type];
}

static void
print_ldt(union descriptor *dp)
{
	unsigned long base_addr, limit, offset, stack_copy;
	unsigned int type, dpl;
	unsigned long lp[2];

	memcpy(lp, dp, sizeof(lp));

	base_addr = dp->sd.sd_lobase | (dp->sd.sd_hibase << 24);
	limit = dp->sd.sd_lolimit | (dp->sd.sd_hilimit << 16);
	offset = dp->gd.gd_looffset | (dp->gd.gd_hioffset << 16);
	
	type = dp->sd.sd_type;
	dpl = dp->sd.sd_dpl;
	stack_copy = dp->gd.gd_stkcpy;

	if (type == SDT_SYS386CGT || type == SDT_SYS286CGT)
		printf("LDT: Gate Off %8.8lx, Sel   %5.5x, Stkcpy %lu DPL %d,"
		    " Type %d/%s\n", offset, dp->gd.gd_selector, stack_copy, dpl,
		    type, segtype(type));
	else
		printf("LDT: Seg Base %8.8lx, Limit %5.5lx, DPL %d, "
		    "Type %d/%s\n", base_addr, limit, dpl,
		    type, segtype(type));
	printf("	  ");
	if (type & 0x1)
		printf("Accessed, ");
	if (dp->sd.sd_p)
		printf("Present, ");
	if (type != SDT_SYS386CGT && type != SDT_SYS286CGT) {
		if (type & 0x10)
			printf("User, ");
		if (type & 0x08)
			printf("X, ");
		if (dp->sd.sd_def32)
			printf("32-bit, ");
		else
			printf("16-bit, ");
		if (dp->sd.sd_gran)
			printf("page limit, ");
		else
			printf("byte limit, ");
	}
	printf("\n");
	printf("	  Raw descriptor: %08lx %08lx\n", lp[0], lp[1]);
}

static void
busfault(int signo)
{
	errx(1, "%s\n - investigate.", sys_siglist[signo]);
}

static void
usage(void)
{
	errx(1, "Usage: testldt [-v]");
}

#define MAX_USER_LDT 1024
#define DATA_ADDR ((void *)0x005f0000)

int
main(int argc, char *argv[])
{
	union descriptor ldt[MAX_USER_LDT];
	int n, ch;
	int num;
	unsigned char *data;
	union descriptor *sd;
	unsigned char one = 1, two = 2, val;
	struct sigaction segv_act;
	int verbose = 0;

	segv_act.sa_handler = busfault;
	if (sigaction(SIGBUS, &segv_act, NULL) < 0)
		err(1, "sigaction");

	while ((ch = getopt(argc, argv, "v")) != -1) {
		switch (ch) {
		case 'v':
			verbose++;
			break;
		default:
			usage();
			break;
		}
	}

	printf("Testing i386_get_ldt\n");
	if ((num = i386_get_ldt(0, ldt, MAX_USER_LDT)) >= 0)
		err(2, "get_ldt succeeded");

	if (verbose) {
		printf("Got %d (initial) LDT entries\n", num);
		for (n = 0; n < num; n++) {
			printf("Entry %d: ", n);
			print_ldt(&ldt[n]);
		}
	}
	
	/*
	 * mmap a data area and assign an LDT to it
	 */
	printf("Testing i386_set_ldt\n");
	data = (void *)mmap(DATA_ADDR, 4096, PROT_READ | PROT_WRITE,
	    MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, (off_t)0);
	if (data != DATA_ADDR)
		err(1, "mmap");

	*data = 0x97;

	/* Get the next free LDT and set it to the allocated data. */
	sd = make_sd(data, 2048, SDT_MEMRW, SEL_UPL, 0, 0);
	if ((num = i386_set_ldt(1024, sd, 1)) < 0)
		err(1, "set_ldt");
	if (verbose)
		printf("setldt returned: %d\n", num);
	if ((n = i386_get_ldt(num, ldt, 1)) < 0)
		err(1, "get_ldt");

	if (verbose) {
		printf("Entry %d: ", num);
		print_ldt(&ldt[0]);
	}

	if (verbose)
		printf("Checking desc (should be 0x97): 0x%x\n",
		    check_desc(num));
	if (check_desc(num) != 0x97)
		errx(1, "ERROR: descriptor check failed; "
		    "expected 0x97, got 0x%x", check_desc(num));

	/*
	 * Test multiple sets.
	 */

	printf("Testing multiple descriptors at once\n");

	sd = malloc(sizeof(*sd) * 2);
	if (sd == NULL)
		err(1, "can't malloc");

	sd[0] = *make_sd(&one, 1, SDT_MEMRO, SEL_UPL, 1, 0);
	sd[1] = *make_sd(&two, 1, SDT_MEMRO, SEL_UPL, 1, 0);

	if ((num = i386_set_ldt(8000, (union descriptor *)sd, 2)) < 0)
		err(1, "set_ldt");
	if (verbose)
		printf("setldt returned: %d\n", num);
	if ((n = i386_get_ldt(num, ldt, 2)) < 0)
		err(1, "get_ldt");

	if (verbose) {
		printf("Entry %d: ", num);
		print_ldt(&ldt[0]);
		printf("Entry %d: ", num+1);
		print_ldt(&ldt[1]);
	}
	val = check_desc(num);
	printf("Contents of segment ONE: %x\n", val);
	if (val != 1)
		errx(1, "ONE has unexpected value %x", val);
	val = check_desc(num+1);
	printf("Contents of segment TWO: %x\n", val);
	if (val != 2)
		errx(1, "TWO has unexpected value %x", val);

	if ((n = i386_get_ldt(num, ldt, 2)) < 0)
		err(1, "get_ldt");

	if (verbose) {
		printf("Entry %d: ", num);
		print_ldt(&ldt[0]);
		printf("Entry %d: ", num+1);
		print_ldt(&ldt[1]);
	}

	printf("Done! No error detected.\n");

	return 0;
}
