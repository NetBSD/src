/*	$NetBSD: bootmain.c,v 1.8 2020/08/16 06:43:43 isaki Exp $	*/

/*-
 * Copyright (c) 1993, 1994 Takumi Nakamura.
 * Copyright (c) 1999, 2000 Itoh Yasufumi.
 * Copyright (c) 2001 Minoura Makoto.
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
 *	This product includes software developed by Takumi Nakamura.
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
 */

#include <sys/param.h>
#include <sys/types.h>
#include <machine/bootinfo.h>
#include <machine/disklabel.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include "xxboot.h"
#include "libx68k.h"
#include "iocs.h"
#include "exec_image.h"

#define EXSCSI_BDID	((void *)0x00ea0001)

static int get_scsi_host_adapter(char *);
int get_scsi_part(void);
void bootmain(void) __attribute__ ((__noreturn__));

#if defined(XXBOOT_DEBUG)
/* Print 'x' as 'width' digit hex number */
void
print_hex(unsigned int x, int width)
{

	if (width > 0) {
		print_hex(x >> 4, width - 1);
		x &= 0x0F;
		if (x > 9)
			x += 7;
		IOCS_B_PUTC((unsigned int) '0' + x);
	}
}
#endif

/*
 * Check the type of SCSI interface
 */
static int
get_scsi_host_adapter(char *devstr)
{
	uint8_t *bootrom;
	int ha;

#ifdef XXBOOT_DEBUG
	*(uint32_t *)(devstr +  0) = '/' << 24 | 's' << 16 | 'p' << 8 | 'c';
#if defined(CDBOOT)
	*(uint32_t *)(devstr +  4) = '@' << 24 | '0' << 16 | '/' << 8 | 'c';
#else
	*(uint32_t *)(devstr +  4) = '@' << 24 | '0' << 16 | '/' << 8 | 's';
#endif
	*(uint32_t *)(devstr +  8) = 'd' << 24 | '@' << 16 | '0' << 8 | ',';
	*(uint32_t *)(devstr + 12) = '0' << 24 | ':' << 16 | 'a' << 8 | '\0';
#endif

	bootrom = (uint8_t *)(BOOT_INFO & 0x00ffffe0);
	/*
	 * bootrom+0x24	"SCSIIN" ... Internal SCSI (spc@0)
	 *		"SCSIEX" ... External SCSI (spc@1 or mha@0)
	 */
	if (*(uint16_t *)(bootrom + 0x24 + 4) == 0x494e) {	/* "IN" */
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 0;
	} else if (badbaddr(EXSCSI_BDID)) {
		ha = (X68K_BOOT_SCSIIF_MHA << 4) | 0;
#ifdef XXBOOT_DEBUG
		*(uint32_t *)devstr =
		    ('/' << 24) | ('m' << 16) | ('h' << 8) | 'a';
#endif
	} else {
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 1;
#ifdef XXBOOT_DEBUG
		devstr[5] = '1';
#endif
	}

	return ha;
}

#define PARTTBL_TOP	(4)	/* sector pos of part info in 512byte/sector */
#define NPART		(15)	/* total number of Human68k partitions */
#define MAXPART		(6)
int
get_scsi_part(void)
{
	union {
		char pad[1024];
		struct {
			uint32_t magic;		/* 0x5836384b ("X68k") */
			uint32_t parttotal;	/* total block# -1 */
			uint32_t diskblocks;
			uint32_t diskblocks2;	/* backup? */
			struct dos_partition parttbl[NPART];
		} __packed;
	} partbuf;
	int i;
	int part_top;

	/*
	 * Read partition table.
	 * The actual partition table size we want to read is 256 bytes but
	 * raw_read() for SCSI requires bytelen a multiple of sector size
	 * (SCSI_CAP.blocksize).  Human68k supports sector size only 256,
	 * 512 and 1024 so that we always use 1024 bytes fixed length buffer.
	 */
	raw_read(PARTTBL_TOP, SCSI_CAP.blocksize, &partbuf);

	if (partbuf.magic != 0x5836384b/*"X68k"*/) {
		BOOT_ERROR("Bad Human68k partition table");
		/* NOTREACHED */
	}

	/*
	 * SCSI_PARTTOP is top sector # of this partition in sector size
	 * of this device (normally 512 bytes/sector).
	 * part_top is top block # of this partition in 1024 bytes/block.
	 * Human68k partition table uses 1024 bytes/block unit.
	 */
	part_top = SCSI_PARTTOP >> (2 - SCSI_BLKLEN);
	for (i = 0; i < MAXPART; i++) {
		if ((uint32_t)partbuf.parttbl[i].dp_start == part_top)
			goto found;
	}
	BOOT_ERROR("Can't find this partition?");
	/* NOTREACHED */
found:
	/* bsd disklabel's c: means whole disk.  Skip it */
	if (i >= 2)
		i++;
	return i;
}

void
bootmain(void)
{
	int bootdev, fd;
	char bootdevstr[16];
	u_long marks[MARK_MAX];

	IOCS_B_PRINT(bootprog_name);
	IOCS_B_PRINT(" rev.");
	IOCS_B_PRINT(bootprog_rev);
	IOCS_B_PRINT("\r\n");

#if defined(XXBOOT_DEBUG)
	/* Print the initial registers */
	int i;
	for (i = 0; i < __arraycount(startregs); i++) {
		print_hex(startregs[i], 8);
		IOCS_B_PRINT((i & 7) == 7 ? "\r\n" : " ");
	}
	IOCS_B_PRINT("BOOT_INFO ");
	print_hex(BOOT_INFO, 8);
	IOCS_B_PRINT("\r\n");
#endif

	if (BINF_ISFD(&BOOT_INFO)) {
		/* floppy */
		int minor;
		/* fdNa for 1024 bytes/sector, fdNc for 512 bytes/sector */
		minor = (FDSEC.minsec.N == 3) ? 0 : 2;
		bootdev = X68K_MAKEBOOTDEV(X68K_MAJOR_FD, BOOT_INFO & 3, minor);
#ifdef XXBOOT_DEBUG
		*(uint32_t *)bootdevstr =
		    ('f' << 24) | ('d' << 16) | ('@' << 8) |
		    ('0' + (BOOT_INFO & 3));
		bootdevstr[4] = 'a' + minor;
		bootdevstr[5] = '\0';
#endif
	} else {
		/* SCSI */
		int major, ha, part;
		ha = get_scsi_host_adapter(bootdevstr);
		part = 0;
#if defined(CDBOOT)
		major = X68K_MAJOR_CD;
#else
		major = X68K_MAJOR_SD;
		if (SCSI_PARTTOP != 0)
			part = get_scsi_part();
#endif
		bootdev = X68K_MAKESCSIBOOTDEV(major, ha >> 4, ha & 15,
		    SCSI_ID & 7, 0, part);
#ifdef XXBOOT_DEBUG
		bootdevstr[10] = '0' + (SCSI_ID & 7);
		bootdevstr[14] = 'a' + part;
#endif
	}

#ifdef XXBOOT_DEBUG
	IOCS_B_PRINT("boot device: ");
	IOCS_B_PRINT(bootdevstr);
#endif
	IOCS_B_PRINT("\r\n");

	marks[MARK_START] = BOOT_TEXTADDR;

#if defined(XXBOOT_USTARFS)
	/* ustarfs requires mangled filename... */
	fd = loadfile("USTAR.volsize.4540", marks,
	    LOAD_TEXT|LOAD_DATA|LOAD_BSS);
#else
	/* XXX what is x68k/boot? */
	fd = loadfile("x68k/boot", marks, LOAD_TEXT|LOAD_DATA|LOAD_BSS);
	if (fd < 0)
		fd = loadfile("boot", marks, LOAD_TEXT|LOAD_DATA|LOAD_BSS);
#endif
	if (fd >= 0) {
		close(fd);
		exec_image(BOOT_TEXTADDR, /* image loaded at */
			   BOOT_TEXTADDR, /* image executed at */
			   BOOT_TEXTADDR, /* XXX: entry point */
			   0, 		  /* XXX: image size */
			   bootdev, 0);	  /* arguments */
	}
	IOCS_B_PRINT("can't load the secondary bootstrap.");
	exit(0);
}

int
devopen(struct open_file *f, const char *fname, char **file)
{

	*file = __UNCONST(fname);
	return DEV_OPEN()(f);
}
