/*	$NetBSD: boot.c,v 1.22 2014/08/05 13:49:04 isaki Exp $	*/

/*
 * Copyright (c) 2001 Minoura Makoto
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <machine/bootinfo.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libsa/ufs.h>
#ifdef NETBOOT
#include <lib/libsa/dev_net.h>
#endif
#include <lib/libkern/libkern.h>

#include "libx68k.h"
#include "iocs.h"
#include "switch.h"

#include "exec_image.h"


#define HEAP_START	((void*) 0x00080000)
#define HEAP_END	((void*) 0x000fffff)
#define EXSCSI_BDID	((void*) 0x00ea0001)
#define SRAM_MEMSIZE	(*((long*) 0x00ed0008))

char default_kernel[20] =
#ifndef NETBOOT
    "sd0a:netbsd";
#else
    "nfs:netbsd";
#endif
int mpu;
#ifndef NETBOOT
int hostadaptor;
#endif
int console_device = -1;

#ifdef DEBUG
#ifdef NETBOOT
int debug = 1;
#endif
#endif

static void help(void);
#ifndef NETBOOT
static int get_scsi_host_adapter(void);
#endif
static void doboot(const char *, int);
static void boot(char *);
#ifndef NETBOOT
static void cmd_ls(char *);
#endif
int bootmenu(void);
void bootmain(int);
extern int detectmpu(void);
extern int badbaddr(void *);

#ifndef NETBOOT
/* from boot_ufs/bootmain.c */
static int
get_scsi_host_adapter(void)
{
	char *bootrom;
	int ha;

	bootrom = (char *) (IOCS_BOOTINF() & 0x00ffffe0);
	/*
	 * bootrom+0x24	"SCSIIN" ... Internal SCSI (spc@0)
	 *		"SCSIEX" ... External SCSI (spc@1 or mha@0)
	 */
	if (*(u_short *)(bootrom + 0x24 + 4) == 0x494e) {	/* "IN" */
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 0;
	} else if (badbaddr(EXSCSI_BDID)) {
		ha = (X68K_BOOT_SCSIIF_MHA << 4) | 0;
	} else {
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 1;
	}

	return ha;
}
#endif

static void
help(void)
{
	printf("Usage:\n");
	printf("boot [dev:][file] -[flags]\n");
#ifndef NETBOOT
	printf(" dev:   sd<ID><PART>, ID=0-7, PART=a-p\n");
	printf("        cd<ID>a, ID=0-7\n");
	printf("        fd<UNIT>a, UNIT=0-3, format is detected.\n");
#else
	printf(" dev:   nfs, first probed NE2000 is used.\n");
#endif
	printf(" file:  netbsd, netbsd.gz, etc.\n");
	printf(" flags: abdqsv\n");
#ifndef NETBOOT
	printf("ls [dev:][directory]\n");
#endif
	printf("switch [show | key=val]\n");
	printf("halt\nreboot\n");
}

static void
doboot(const char *file, int flags)
{
	u_long		marks[MARK_MAX];
	int fd;
	int dev, unit, part;
	char *name;
	short *p;
	int loadflag;

	printf("Starting %s, flags 0x%x\n", file, flags);

	loadflag = LOAD_KERNEL;
	if (file[0] == 'f')
		loadflag &= ~LOAD_BACKWARDS;
		
	marks[MARK_START] = 0x100000;
	if ((fd = loadfile(file, marks, loadflag)) == -1) {
		printf("loadfile failed\n");
		return;
	}
	close(fd);

	if (devparse(file, &dev, &unit, &part, &name) != 0) {
		printf("XXX: unknown corruption in /boot.\n");
	}

#ifdef DEBUG
#ifndef NETBOOT
	printf("dev = %x, unit = %d, part = %c, name = %s\n",
	       dev, unit, part + 'a', name);
#else
	printf("dev = %x, unit = %d, name = %s\n",
	       dev, unit, name);
#endif
#endif

#ifndef NETBOOT
	if (dev == 0) {		/* SCSI */
		dev = X68K_MAKESCSIBOOTDEV(X68K_MAJOR_SD,
					   hostadaptor >> 4,
					   hostadaptor & 15,
					   unit & 7, 0, 0);
	} else {
		dev = X68K_MAKEBOOTDEV(X68K_MAJOR_FD, unit & 3, 0);
	}
#else
	dev = X68K_MAKEBOOTDEV(X68K_MAJOR_NE, unit, 0);
#endif
#ifdef DEBUG
	printf("boot device = %x\n", dev);
#ifndef NETBOOT
	printf("if = %d, unit = %d, id = %d, lun = %d, part = %c\n",
	       B_X68K_SCSI_IF(dev),
	       B_X68K_SCSI_IF_UN(dev),
	       B_X68K_SCSI_ID(dev),
	       B_X68K_SCSI_LUN(dev),
	       B_X68K_SCSI_PART(dev) + 'a');
#else
	printf("if = %d, unit = %d\n",
	       B_X68K_SCSI_IF(dev),
	       B_X68K_SCSI_IF_UN(dev));
#endif
#endif

	p = ((short*) marks[MARK_ENTRY]) - 1;
#ifdef DEBUG
	printf("Kernel Version: 0x%x\n", *p);
#endif
	if (*p != 0x4e73 && *p != 0) {
		/*
		 * XXX temporary solution; compatibility loader
		 * must be written.
		 */
		printf("This kernel is too new to be loaded by "
		       "this version of /boot.\n");
		return;
	}

	exec_image(marks[MARK_START], 0, marks[MARK_ENTRY]-marks[MARK_START],
		   marks[MARK_END]-marks[MARK_START], dev, flags);

	return;
}

static void
boot(char *arg)
{
	char filename[80];
	char *p;
	int flags = 0;

	if (*arg == 0 || *arg == '-') {
		strcpy(filename, default_kernel);
		if (*arg == '-')
			if (parseopts(arg, &flags) == 0) {
				help();
				return;
			}
		doboot(filename, flags);
		return;
	} else {
		p = gettrailer(arg);
		if (strchr(arg, ':')) {
			strcpy(filename, arg);
			if (arg[strlen(arg) - 1] == ':')
				strcat(filename, "netbsd");
		} else {
			strcpy(filename, default_kernel);
			strcpy(strchr(filename, ':') + 1, arg);
		}
		if (*p == '-') {
			if (parseopts(p, &flags) == 0)
				return;
		} else if (*p != 0) {
			help();
			return;
		}

		doboot(filename, flags);
		return;
	}
}

#ifndef NETBOOT
static void
cmd_ls(char *arg)
{
	char filename[80];

	devopen_open_dir = 1;
	if (*arg == 0) {
		strcpy(filename, default_kernel);
		strcpy(strchr(filename, ':')+1, "/");
	} else if (strchr(arg, ':') == 0) {
		strcpy(filename, default_kernel);
		strcpy(strchr(filename, ':')+1, arg);
	} else {
		strcpy(filename, arg);
		if (*(strchr(arg, ':')+1) == 0)
			strcat(filename, "/");
	}
	ls(filename);
	devopen_open_dir = 0;
}
#endif

int
bootmenu(void)
{
	char input[80];
	int n = 5, c;

	printf("Press return to boot now, any other key for boot menu\n");
	printf("booting %s - starting in %d seconds. ",
		default_kernel, n);
	while (n-- > 0 && (c = awaitkey_1sec()) == 0) {
		printf("\r");
		printf("booting %s - starting in %d seconds. ",
		       default_kernel, n);
	}
	printf("\r");
	printf("booting %s - starting in %d seconds. ", default_kernel, 0);
	printf("\n");
	
	if (c == 0 || c == '\r') {
		doboot(default_kernel, 0);
		printf("Could not start %s; ", default_kernel);
		strcat(default_kernel, ".gz");
		printf("trying %s.\n", default_kernel);
		doboot(default_kernel, 0);
		printf("Could not start %s; ", default_kernel);
	}

	printf("Please use the absolute unit# (e.g. SCSI ID)"
	       " instead of the NetBSD logical #.\n");
	for (;;) {
		char *p, *options;

		printf("> ");
		gets(input);

		for (p = &input[0]; p - &input[0] < 80 && *p == ' '; p++)
			;
		options = gettrailer(p);
		if (strcmp("boot", p) == 0)
			boot(options);
		else if (strcmp("help", p) == 0 ||
			 strcmp("?", p) == 0)
			help();
		else if (strcmp("halt", p) == 0 ||
			 strcmp("reboot", p) == 0)
			exit(0);
		else if (strcmp("switch", p) == 0)
			cmd_switch(options);
#ifndef NETBOOT
		else if (strcmp("ls", p) == 0)
			cmd_ls(options);
#endif
		else
			printf("Unknown command %s\n", p);
	}
}

static u_int
checkmemsize(void)
{
	u_int m;

#define MIN_MB 4
#define MAX_MB 12

	for (m = MIN_MB; m <= MAX_MB; m++) {
		if (badbaddr((void *)(m * 1024 * 1024 - 1))) {
			/* no memory */
			break;
		}
	}

	return (m - 1) * 1024 * 1024;
}

extern const char bootprog_rev[];
extern const char bootprog_name[];

/*
 * Arguments from the boot block:
 *   bootdev - specifies the device from which /boot was read, in 
 *		bootdev format.
 */
void
bootmain(int bootdev)
{
	u_int sram_memsize;
	u_int probed_memsize;

#ifndef NETBOOT
	hostadaptor = get_scsi_host_adapter();
#else
	rtc_offset = RTC_OFFSET;
	try_bootp = 1;
#endif
	mpu = detectmpu();

	if (mpu < 3) {		/* not tested on 68020 */
		printf("This MPU cannot run NetBSD.\n");
		exit(1);
	}
	sram_memsize = SRAM_MEMSIZE;
	if (sram_memsize < 4*1024*1024) {
		printf("Main memory too small.\n");
		exit(1);
	}

	console_device = consio_init(console_device);
	setheap(HEAP_START, HEAP_END);

#ifndef NETBOOT
	switch (B_TYPE(bootdev)) {
	case X68K_MAJOR_FD:
		default_kernel[0] = 'f';
		default_kernel[2] = '0' + B_UNIT(bootdev);
		default_kernel[3] = 'a';
		break;
	case X68K_MAJOR_SD:
		default_kernel[2] = '0' + B_X68K_SCSI_ID(bootdev);
		default_kernel[3] =
			'a' + sd_getbsdpartition(B_X68K_SCSI_ID(bootdev),
						 B_X68K_SCSI_PART(bootdev));
		break;
	case X68K_MAJOR_CD:
		default_kernel[0] = 'c';
		default_kernel[2] = '0' + B_X68K_SCSI_ID(bootdev);
		default_kernel[3] = 'a';
		break;
	default:
		printf("Warning: unknown boot device: %x\n", bootdev);
	}
#endif
	print_title("%s, Revision %s\n", bootprog_name, bootprog_rev);

	/* check actual memory size for machines with a dead SRAM battery */
	probed_memsize = checkmemsize();
	if (sram_memsize != probed_memsize) {
		printf("\x1b[1mWarning: SRAM Memory Size (%d MB) "
		    "is different from probed Memory Size (%d MB)\n"
		    "         Check and reset SRAM values.\x1b[m\n\n",
		    sram_memsize / (1024 * 1024),
		    probed_memsize / (1024 * 1024));
	}

	bootmenu();
}
