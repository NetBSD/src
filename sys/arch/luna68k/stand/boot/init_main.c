/*	$NetBSD: init_main.c,v 1.5.6.4 2014/08/20 00:03:10 tls Exp $	*/

/*
 * Copyright (c) 1992 OMRON Corporation.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)init_main.c	8.2 (Berkeley) 8/15/93
 */
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	@(#)init_main.c	8.2 (Berkeley) 8/15/93
 */

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <machine/cpu.h>
#include <luna68k/stand/boot/samachdep.h>
#include <luna68k/stand/boot/romvec.h>
#include <luna68k/stand/boot/status.h>
#include <lib/libsa/loadfile.h>
#ifdef SUPPORT_ETHERNET
#include <lib/libsa/dev_net.h>
#endif

static int get_plane_numbers(void);
static int reorder_dipsw(int);

int cpuspeed;	/* for DELAY() macro */
int hz = 60;
int machtype;
char default_file[64];
const char *default_bootdev;
int default_unit;

#define	VERS_LOCAL	"Phase-31"

int nplane;

/* for command parser */

#define BUFFSIZE 100
#define MAXARGS  30

char buffer[BUFFSIZE];

int   argc;
char *argv[MAXARGS];

#define BOOT_TIMEOUT 10
int boot_timeout = BOOT_TIMEOUT;

static const char prompt[] = "boot> ";

/*
 * PROM monitor's boot device info
 */

/* LUNA-I monitor's bootinfo structure */
/* This bootinfo data address is investigated on ROM Ver4.22 and Ver4.25 */
#define	LUNA1_BOOTINFOADDR	0x000008c0
struct luna1_bootinfo {
	uint8_t	bi_xxx1[3];	/* 0x08c0: ??? */
	uint8_t	bi_device;	/* 0x08c3: boot device */
#define	LUNA1_BTDEV_DK	0		/* Hard-Disk */
#define	LUNA1_BTDEV_FB	1		/* Floppy-Disk */
#define	LUNA1_BTDEV_SD	2		/* Streamer-Tape */
#define	LUNA1_BTDEV_P0	3		/* RS232c */
#define	LUNA1_BTDEV_ET	4		/* Ether-net */
#define	LUNA1_NBTDEV	5

	struct {
		uint8_t	bd_xxx1;	/*  0: ??? */
		uint8_t	bd_boot;	/*  1: 1 == booted */
		char	bd_name[2];	/*  2: device name (dk, fb, sd ... ) */
		uint8_t	bd_drv;		/*  4: drive number (not ID) */
		uint8_t	bd_xxx2;	/*  5: ??? */
		uint8_t	bd_xxx3;	/*  6: ??? */
		uint8_t	bd_part;	/*  7: dk partition / st record # */
		uint8_t	bd_xxx4[4];	/*  8: ??? */
		uint8_t	bd_xxx5[4];	/* 12: ??? */
	} bi_devinfo[LUNA1_NBTDEV];
} __packed;

/* LUNA-II monitor's bootinfo structure */
/* This bootinfo data address is investigated on ROM Version 1.11 */
#define	LUNA2_BOOTINFOADDR	0x00001d80
struct luna2_bootinfo {
	uint8_t	bi_xxx1[13];	/* 0x1d80: ??? */
	uint8_t	bi_device;	/* 0x1d8d: boot device */
#define	LUNA2_BTDEV_DK	0		/* Hard-Disk */
#define	LUNA2_BTDEV_FT	1		/* Floppy-Disk */
#define	LUNA2_BTDEV_SD	2		/* Streamer-Tape */
#define	LUNA2_BTDEV_P0	3		/* RS232c */
#define	LUNA2_NBTDEV	4

	struct {
		uint8_t	bd_xxx1;	/*  0: ??? */
		uint8_t	bd_boot;	/*  1: 1 == booted */
		char	bd_name[4];	/*  2: device name (dk, ft, sd ... ) */
		uint8_t	bd_xxx2;	/*  6: ??? */
		uint8_t	bd_ctlr;	/*  7: SCSI controller number */
		uint8_t	bd_id;		/*  8: SCSI ID number */
		uint8_t	bd_xxx3;	/*  9: device number index? */
		uint8_t	bd_xxx4;	/* 10: ??? */
		uint8_t	bd_part;	/* 11: dk partition / st record # */
		uint8_t	bd_xxx5[4];	/* 12: ??? */
		uint8_t	bd_xxx6[4];	/* 16: ??? */
	} bi_devinfo[LUNA2_NBTDEV];
} __packed;

/* #define BTINFO_DEBUG */

void
main(void)
{
	int i, status = ST_NORMAL;
	const char *machstr;
	const char *bootdev;
	uint32_t howto;
	int unit, part;
	int bdev, ctlr, id;

	/*
	 * Initialize the console before we print anything out.
	 */
	if (cputype == CPU_68030) {
		machtype = LUNA_I;
		machstr  = "LUNA-I";
		cpuspeed = MHZ_25;
		hz = 60;
	} else {
		machtype = LUNA_II;
		machstr  = "LUNA-II";
		cpuspeed = MHZ_25 * 2;	/* XXX */
		hz = 100;
	}

	nplane = get_plane_numbers();

	cninit();

	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (based on Stinger ver 0.0 [%s])\n", VERS_LOCAL);
	printf("\n");

	i = ROM_memsize;
	printf("Machine model   = %s\n", machstr);
	printf("Physical Memory = 0x%x  ", i);
	i >>= 20;
	printf("(%d MB)\n", i);
	printf("\n");

	/*
	 * IO configuration
	 */

#ifdef SUPPORT_ETHERNET
	try_bootp = 1;
#endif

	find_devs();
	printf("\n");

	/* use sd(0,0) for the default boot device */
	bootdev = "sd";
	unit = 0;
	part = 0;

	if (machtype == LUNA_I) {
		const struct luna1_bootinfo *bi1 = (void *)LUNA1_BOOTINFOADDR;

		bdev = bi1->bi_device;
		switch (bdev) {
		case LUNA1_BTDEV_DK:
			/* note: bd_drv is not SCSI ID */
			ctlr = 0;
			id   = 6 - bi1->bi_devinfo[bdev].bd_drv;
			unit = UNIT(ctlr, id);
			break;
		case LUNA1_BTDEV_ET:
			bootdev = "le";
			unit = 0;
			break;
		default:
			/* not supported */
			break;
		}
#ifdef BTINFO_DEBUG
		printf("bi1->bi_device = 0x%02x\n", bi1->bi_device);
		printf("bi1->bi_devinfo[bdev].bd_boot = 0x%02x\n",
		    bi1->bi_devinfo[bdev].bd_boot);
		printf("bi1->bi_devinfo[bdev].bd_name = %c%c\n",
		    bi1->bi_devinfo[bdev].bd_name[0],
		    bi1->bi_devinfo[bdev].bd_name[1]);
		printf("bi1->bi_devinfo[bdev].bd_drv = 0x%02x\n",
		    bi1->bi_devinfo[bdev].bd_drv);
		printf("bi1->bi_devinfo[bdev].bd_part = 0x%02x\n",
		    bi1->bi_devinfo[bdev].bd_part);
#endif
	} else {
		const struct luna2_bootinfo *bi2 = (void *)LUNA2_BOOTINFOADDR;

		bdev = bi2->bi_device;
		switch (bdev) {
		case LUNA2_BTDEV_DK:
			ctlr = bi2->bi_devinfo[bdev].bd_ctlr;
			id   = bi2->bi_devinfo[bdev].bd_id;
			unit = UNIT(ctlr, id);
			break;
		default:
			/* not supported */
			break;
		}
#ifdef BTINFO_DEBUG
		printf("bi2->bi_device = 0x%02x\n", bi2->bi_device);
		printf("bi2->bi_devinfo[bdev].bd_boot = 0x%02x\n",
		    bi2->bi_devinfo[bdev].bd_boot);
		printf("bi2->bi_devinfo[bdev].bd_name = %s\n",
		    bi2->bi_devinfo[bdev].bd_name);
		printf("bi2->bi_devinfo[bdev].bd_ctlr = 0x%02x\n",
		    bi2->bi_devinfo[bdev].bd_ctlr);
		printf("bi2->bi_devinfo[bdev].bd_id = 0x%02x\n",
		    bi2->bi_devinfo[bdev].bd_id);
		printf("bi2->bi_devinfo[bdev].bd_part = 0x%02x\n",
		    bi2->bi_devinfo[bdev].bd_part);
#endif
	}

	snprintf(default_file, sizeof(default_file),
	    "%s(%d,%d)%s", bootdev, unit, part, "netbsd");
	default_bootdev = bootdev;
	default_unit = unit;

	howto = reorder_dipsw(dipsw2);

	if ((howto & 0xFE) == 0) {
		char c;

		printf("Press return to boot now,"
		    " any other key for boot menu\n");
		printf("booting %s - starting in ", default_file);
		c = awaitkey("%d seconds. ", boot_timeout, true);
		if (c == '\r' || c == '\n' || c == 0) {
			printf("auto-boot %s\n", default_file);
			bootnetbsd(default_file, 0);
		}
	}

	/*
	 * Main Loop
	 */

	printf("type \"help\" for help.\n");

	do {
		memset(buffer, 0, BUFFSIZE);
		if (getline(prompt, buffer) > 0) {
			argc = getargs(buffer, argv,
			    sizeof(argv) / sizeof(char *));

			status = parse(argc, argv);
			if (status == ST_NOTFOUND)
				printf("Command \"%s\" is not found !!\n",
				    argv[0]);
		}
	} while (status != ST_EXIT);

	exit(0);
}

int
get_plane_numbers(void)
{
	int r = ROM_plane;
	int n = 0;

	for (; r ; r >>= 1)
		if (r & 0x1)
			n++;

	return n;
}

int
reorder_dipsw(int dipsw)
{
	int i, sw = 0;

	for (i = 0; i < 8; i++) {
		if ((dipsw & 0x01) == 0)
			sw += 1;

		if (i == 7)
			break;

		sw <<= 1;
		dipsw >>= 1;
	}

	return sw;
}
