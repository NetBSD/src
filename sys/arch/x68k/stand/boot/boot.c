/*	$NetBSD: boot.c,v 1.4 2001/09/29 03:50:12 minoura Exp $	*/

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
#include <lib/libkern/libkern.h>

#include "libx68k.h"
#include "iocs.h"

#include "exec_image.h"


#define HEAP_START	((void*) 0x00080000)
#define HEAP_END	((void*) 0x000fffff)
#define EXSCSI_BDID	((void*) 0x00ea0001)
#define SRAM_MEMSIZE	(*((long*) 0x00ed0008))

char default_kernel[20] = "sd0a:netbsd";
int mpu, bootdev, hostadaptor;
int console_device = -1;

static void help(void);
static int get_scsi_host_adapter(void);
static void doboot(const char *, int);
static void boot(char *);
static void ls(char *);
int bootmenu(void);
void bootmain(int);
extern int detectmpu(void);
extern int badbaddr(caddr_t);

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


static void
help(void)
{
	printf("Usage:\n");
	printf("boot [dev:][file] -[flags]\n");
	printf(" dev:   sd<ID><PART>, ID=0-7, PART=a-p\n");
	printf("        cd<ID>a, ID=0-7\n");
	printf("        fd<UNIT>a, UNIT=0-3, format is detected.\n");
	printf(" file:  netbsd, netbsd.gz, etc.\n");
	printf(" flags: abdqsv\n");
	printf("ls [dev:][directory]\n");
	printf("halt\nreboot\n");
}

static void
doboot(const char *file, int flags)
{
	u_long		marks[MARK_MAX];
	int fd;
	int dev, unit, part;
	char *name;

	printf("Starting %s, flags 0x%x\n", file, flags);
	marks[MARK_START] = 0x100000;
	if ((fd = loadfile(file, marks, LOAD_KERNEL)) == -1)
		return;
	close(fd);

	if (devparse(file, &dev, &unit, &part, &name) != 0) {
		printf("XXX: unknown corruption in /boot.\n");
	}

	printf("dev = %x, unit = %d, part = %c, name = %s\n",
	       dev, unit, part + 'a', name);

	if (dev == 0) {		/* SCSI */
		dev = X68K_MAKESCSIBOOTDEV(X68K_MAJOR_SD,
					   hostadaptor >> 4,
					   hostadaptor & 15,
					   unit & 7, 0, 0);
	} else {
		dev = X68K_MAKEBOOTDEV(X68K_MAJOR_FD, unit & 3, 0);
	}
	printf("boot device = %x\n", dev);
	printf("if = %d, unit = %d, id = %d, lun = %d, part = %c\n",
	       B_X68K_SCSI_IF(dev),
	       B_X68K_SCSI_IF_UN(dev),
	       B_X68K_SCSI_ID(dev),
	       B_X68K_SCSI_LUN(dev),
	       B_X68K_SCSI_PART(dev) + 'a');

	{
		short *p = ((short*) marks[MARK_ENTRY]) - 1;
		printf("Kernel Version: 0x%x\n", *p);
		if (*p != 0x4e73 && *p != 0) {
			/*
			 * XXX temporary solution; compatibility loader
			 * must be written.
			 */
			printf("This kernel is too new to be loaded by "
			       "this version of /boot.\n");
			return;
		}
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

static void
ls(char *arg)
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
	ufs_ls(filename);
	devopen_open_dir = 0;
}

int
bootmenu(void)
{
	char input[80];
	int n = 5, c;

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
	       " instead of the NetBSD ones.\n");
	for (;;) {
		char *p, *options;

		printf("> ");
		gets(input);

		for (p = &input[0]; p - &input[0] < 80 && *p == ' '; p++);
		options = gettrailer(p);
		if (strcmp("boot", p) == 0)
			boot(options);
		else if (strcmp("help", p) == 0 ||
			 strcmp("?", p) == 0)
			help();
		else if ((strcmp("halt", p) == 0) ||(strcmp("reboot", p) == 0))
			exit(0);
		else if (strcmp("ls", p) == 0)
			ls(options);
		else
			printf("Unknown command %s\n", p);
	}
}


/*
 * Arguments from the boot block:
 *   bootdev - specifies the device from which /boot was read, in 
 *		bootdev format.
 */
void
bootmain(int bootdev)
{
	hostadaptor = get_scsi_host_adapter();
	mpu = detectmpu();

	if (mpu < 3) {		/* not tested on 68020 */
		printf("This MPU cannot run NetBSD.\n");
		exit(1);
	}
	if (SRAM_MEMSIZE < 4*1024*1024) {
		printf("Main memory too small.\n");
		exit(1);
	}

	console_device = consio_init(console_device);
	setheap(HEAP_START, HEAP_END);

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
	print_title("NetBSD/x68k bootstrap loader version %s", BOOT_VERS);
	bootmenu();
}
