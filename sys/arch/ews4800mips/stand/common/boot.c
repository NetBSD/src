/*	$NetBSD: boot.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

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

#include "local.h"
#include "cmd.h"
#include "common.h"

#include <machine/sbd.h>
#include <machine/pdinfo.h>
#include <machine/vtoc.h>

#include "console.h"


extern const char bootprog_name[];
extern const char bootprog_rev[];
extern const char bootprog_date[];
extern const char bootprog_maker[];

struct cmd_batch_tab cmd_batch_tab[] = {
	/* func    argc   argp... */
#if 0
	{ cmd_boot, 1, { "mem:", 0, 0, 0, 0, 0, 0 } },
	{ cmd_boot, 1, { "sd0k:netbsd", 0, 0, 0, 0, 0, 0 } },
	{ cmd_load_binary, 1, { "0x80001000", 0, 0, 0, 0, 0, 0 } },
	{ cmd_jump, 2, { "0x80001000", "0x80001000", 0, 0, 0, 0, 0 } },
#endif
	{ NULL, 0, { 0, 0, 0, 0, 0, 0, 0 } } /* terminate */
};

struct ipl_args ipl_args;
struct device_capability DEVICE_CAPABILITY;
void set_device_capability(void);
boolean_t guess_boot_kernel(char *, size_t, int);
extern int kernel_binary_size;

void
main(int a0, int v0, int v1)
{
	extern char edata[], end[];
	char boot_kernel[32];
	char *args[CMDARG_MAX];
	int i;

	memset(edata, 0, end - edata);
	/* Save args for chain-boot to iopboot */
	ipl_args.a0 = a0;
	ipl_args.v0 = v0;
	ipl_args.v1 = v1;

	console_init();

	printf("\n");
	printf("%s boot, Revision %s\n", bootprog_name, bootprog_rev);
	printf("(%s, %s)\n", bootprog_date, bootprog_maker);


	/* Inquire IPL activated device */
	set_device_capability();

	if (!guess_boot_kernel(boot_kernel, sizeof boot_kernel, 0))
		goto prompt;
	printf(
	    ">> Press return to boot now, any other key for boot console.\n");

	for (i = 5000; i >= 0; i--) {
		int c;
		if (i % 1000 == 0)
			printf("booting %s - starting %d\r",
			    boot_kernel, i / 1000);
		if ((c = cnscan()) == -1) {
			delay(10);
			continue;
		}
		else if (c == '\r')
			break;
		else
			goto prompt;
	}
	printf("\n[non-interactive mode]\n");
	args[0] = "boot";
	args[1] = boot_kernel;
	cmd_boot(2, args, FALSE);
 prompt:

	printf("\ntype \"help\" for help.\n");
	console_cursor(TRUE);
	prompt();
	/* NOTREACHED */
}

boolean_t
guess_boot_kernel(char *name, size_t len, int pri)
{
	extern struct vtoc_sector vtoc;
	struct ux_partition *partition;
	int i, unit;

	if (!DEVICE_CAPABILITY.active)
		return FALSE;

	unit = DEVICE_CAPABILITY.booted_unit;

	switch (DEVICE_CAPABILITY.booted_device) {
	default:
		return FALSE;
	case NVSRAM_BOOTDEV_FLOPPYDISK:
		strncpy(name, "fd:netbsd", len);	/* ustarfs */
		return TRUE;

	case NVSRAM_BOOTDEV_HARDDISK:
		snprintf(name, len, "sd%d:netbsd", unit); /* ustarfs */
		if (!read_vtoc())
			return TRUE;

		partition = vtoc.partition;
		for (i = 0; i < VTOC_MAXPARTITIONS; i++, partition++) {
			if (partition->tag != __VTOC_TAG_BSDFFS)
				continue;
			/* ffs */
			snprintf(name, len, "sd%d%c:netbsd", unit, 'a' + i);
			return TRUE;
		}
		return TRUE;

	case NVSRAM_BOOTDEV_CGMT:
		break;
	case NVSRAM_BOOTDEV_NETWORK:
		/*FALLTHROUGH*/
	case NVSRAM_BOOTDEV_NETWORK_T_AND_D:
		if (kernel_binary_size) {
			strncpy(name, "mem:", len);	/* datafs */
			return TRUE;
		}
		if (DEVICE_CAPABILITY.network_enabled) {
			strncpy(name, "nfs:netbsd", len);	/* nfs */
			return TRUE;
		}
		break;
	}

	return FALSE;
}

int
cmd_info(int argc, char *argp[], int interactive)
{
	extern char _ftext[], _etext[], _fdata[], _edata[];
	extern char _fbss[], end[];
	uint32_t m;
	int i, size, total;
	struct sbdinfo *sbd = SBD_INFO;

	printf("\n>> %s boot, rev. %s [%s, %s] <<\n", bootprog_name,
	    bootprog_rev, bootprog_date, bootprog_maker);

	printf("IPL args: 0x%x 0x%x 0x%x\n", ipl_args.a0, ipl_args.v0,
	    ipl_args.v1);
	printf("\ttext : %p-%p\n\tdata : %p-%p\n\t"
	    "bss  : %p-%p\n\tstack: %p\n\theap : %p\n",
	       _ftext, _etext, _fdata, _edata,
	       _fbss, end, _ftext, end);

	m = ipl_args.v1;
	total = 0;
	printf("Memory Area:\n\t");
	for (i = 0; i < 8; i++, m >>= 4) {
		size = m & 0xf ? ((m & 0xf) << 4) : 0;
		total += size;
		if (size)
			printf("M%d=%dMB ", i, size);
	}
	printf(" total %dMB\n", total);

	printf("Board Revision:\n");
	printf("\tmachine=0x%x, ", sbd->machine);
	printf("model=0x%x\n", sbd->model);
	printf("\tpmmu=%d, ", sbd->mmu);
	printf("cache=%d, ", sbd->cache);
	printf("panel=%d, ", sbd->panel);
	printf("fdd=%d\n", sbd->fdd);
	printf("\tcpu=%d, fpp=%d, fpa=%d, iop=%d\n",
	    sbd->cpu, sbd->fpp, sbd->fpa, sbd->iop);
	printf("\tclock=%d\n", sbd->clock);
	printf("\tipl=%d, cpu_ex=%d, fpp_ex=%d\n",
	    sbd->ipl, sbd->cpu_ex, sbd->fpp_ex);
	printf("\tkbms=%d, sio=%d, battery=%d, scsi=%d\n",
	    sbd->kbms, sbd->sio, sbd->battery, sbd->scsi);
	printf("model name=%s\n", sbd->model_name);

	return 0;
}

int
cmd_reboot(int argc, char *argp[], int interactive)
{
	int bootdev = -1;

	if (argc > 1)
		bootdev = strtoul(argp[1], 0, 0); /* next boot device. */
	if (bootdev != NVSRAM_BOOTDEV_FLOPPYDISK &&
	    bootdev != NVSRAM_BOOTDEV_HARDDISK &&
	    bootdev != NVSRAM_BOOTDEV_CGMT &&
	    bootdev != NVSRAM_BOOTDEV_NETWORK) {
		printf("invalid boot device.");
		bootdev = -1;
	}

	switch (SBD_INFO->machine) {
	case MACHINE_TR2A:
		if (bootdev != -1)
			*(uint8_t *)0xbe493030 = bootdev;
		*(volatile uint32_t *)0xbe000064 |= 0x80000000;
		*(volatile uint8_t *)0xba000004 = 1;
		*(uint8_t *)0xbfbffffc = 255;
		break;
	case MACHINE_TR2:
		if (bootdev != -1)
			*(uint8_t *)0xbb023030 = bootdev;
		*(volatile uint32_t *)0xbfb00000 |= 0x10;
		break;
	default:
		ROM_MONITOR();
	}

	while (/*CONSTCOND*/1)
		;
	/* NOTREACHED */
	return 0;
}

void
set_device_capability(void)
{
	const char *devname[] = {
		"Floppy disk",
		"Unknown",
		"Hard disk",
		"Unknown",
		"CGMT",
		"Unknown",
		"Network",
		"Unknown",
		"Network T&D"
	};
	int booted_device, booted_unit, fd_format;

	boot_device(&booted_device, &booted_unit, &fd_format);
	if (booted_device > NVSRAM_BOOTDEV_MAX ||
	    booted_device < NVSRAM_BOOTDEV_MIN) {
		printf(
		    "invalid booted device. NVSRAM infomation isn't valid\n");
	} else {
		DEVICE_CAPABILITY.booted_device = booted_device;
	}
	DEVICE_CAPABILITY.booted_unit = booted_unit;

	switch (SBD_INFO->machine) {
	case MACHINE_TR2A:
		DEVICE_CAPABILITY.active = TRUE;
		/* boot has LANCE driver */
		DEVICE_CAPABILITY.network_enabled = TRUE;
		break;
	case MACHINE_TR2:
		DEVICE_CAPABILITY.active = TRUE;
		break;
	default:
		DEVICE_CAPABILITY.active = FALSE;
		break;
	}

	DEVICE_CAPABILITY.fd_enabled = TRUE;	/* always enabled */

	if (DEVICE_CAPABILITY.active) {
		/*
		 * When NETWORK IPL, FD IPL doesn't activate ROM DISK routine.
		 */
		if (DEVICE_CAPABILITY.booted_device == NVSRAM_BOOTDEV_HARDDISK)
			DEVICE_CAPABILITY.disk_enabled = TRUE;
	}

	printf("FD[%c] DISK[%c] NETWORK[%c] COMPILED[%c]\n",
	    DEVICE_CAPABILITY.fd_enabled ? 'x' : '_',
	    DEVICE_CAPABILITY.disk_enabled ? 'x' : '_',
	    DEVICE_CAPABILITY.network_enabled ? 'x' : '_',
	    kernel_binary_size ? 'x' : '_');

	printf("booted from %s IPL",
	    devname[DEVICE_CAPABILITY.booted_device], booted_unit);
	if ((DEVICE_CAPABILITY.booted_device == NVSRAM_BOOTDEV_NETWORK) ||
	    (DEVICE_CAPABILITY.booted_device == NVSRAM_BOOTDEV_NETWORK_T_AND_D))
	{
		printf("\n");
	} else {
		printf(" unit %d\n", DEVICE_CAPABILITY.booted_unit);
	}
}

int
cmd_test(int argc, char *argp[], int interactive)
{

	/* MISC TEST ROUTINE */
	extern int fdd_test(void);
	fdd_test();
#if 0
	int i;

	printf("argc=%d\n", argc);
	for (i = 0; i < argc; i++)
		printf("[%d] %s\n", i, argp[i]);
#endif
#if 0	/* Recover my 360ADII NVSRAM.. */
	uint8_t *p = (uint8_t *)0xbe490000;
	uint8_t *q = nvsram_tr2a;
	int i;

	for (i = 0; i < sizeof nvsram_tr2a; i++) {
		*p = *q;
		p += 4;
		q += 1;
	}
#endif
#if 0	/* ROM PUTC test */
	char a[]= "ohayotest!";
	int i;
	for (i = 0; i < 10; i++)
		ROM_PUTC(120 + i * 12, 24 * 10, a[i]);
#endif
#if 0	/* ROM SCSI disk routine test TR2 */
	uint8_t buf[512*2];
	uint8_t *p;
	int i;

	printf("type=%d\n", *(uint8_t *)0xbb023034);
	memset(buf, 0, sizeof buf);
	p = (uint8_t *)(((uint32_t)buf + 511) & ~511);
	i = ROM_DK_READ(0, 0, 1, p);
	printf("err=%d\n", i);
	for (i = 0; i < 64; i++) {
		printf("%x ", p[i]);
		if (((i + 1) & 0xf) == 0)
			printf("\n");
	}
#endif
#if 0
	/*XXX failed. */
	__asm volatile(
		".set noreorder;"
		"li	$4, 2;"
		"mtc0	$4, $16;" /* Config */
		"lui	$4, 0xbfc2;"
		"jr	$4;"
		"nop;"
		".set reorder");
	/* NOTREACHED */
#endif
#if 0
	/* FPU test */
	{
		int v;
		__asm volatile(
			".set noreorder;"
			"lui	%0, 0x2000;"
			"mtc0	%0, $12;" /* Cu1 */
			"nop;"
			"nop;"
			"cfc1	%0, $%1;"
			"nop;"
			"nop;"
			".set reorder"
			: "=r"(v) : "i"(0));
		printf("FPUId: %x\n", v);
	}
#endif
	return 0;
}
