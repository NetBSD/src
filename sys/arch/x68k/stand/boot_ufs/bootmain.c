/*	$NetBSD: bootmain.c,v 1.1 2001/09/27 10:14:50 minoura Exp $	*/

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
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/exec_aout.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ufs/dir.h>
#include <machine/bootinfo.h>
#ifdef SCSI_ADHOC_BOOTPART
#include <machine/disklabel.h>
#endif

#include "boot_ufs.h"
#include "readufs.h"
#include "exec_image.h"
#include "../../x68k/iodevice.h"
#define IODEVbase ((volatile struct IODEVICE *)PHYS_IODEV)

/* for debug; 起動時のレジスタが入っている */
unsigned int startregs[16];

#ifdef SCSI_ADHOC_BOOTPART
static int get_scsi_part (void);
#endif
#ifdef BOOT_DEBUG
static int get_scsi_host_adapter (char *);
#else
static int get_scsi_host_adapter (void);
#endif

#ifdef BOOT_DEBUG
void print_hex (unsigned int, int);
#endif

static int load_file (const char*, unsigned int, struct exec *);
static int load_file_ino (ino_t, const char*, unsigned int, struct exec *);

void bootufs (void) __attribute__ ((__noreturn__));

#ifdef BOOT_DEBUG
void
print_hex(x, l)
	unsigned int x;	/* 表示する数字 */
	int l;		/* 表示する桁数 */
{

	if (l > 0) {
		print_hex(x >> 4, l - 1);
		x &= 0x0F;
		if (x > 9)
			x += 7;
		B_PUTC((unsigned int) '0' + x);
	}
}
#endif

#ifdef SCSI_ADHOC_BOOTPART
/*
 * get partition # from partition start position
 */

#define NPART		15
#define PARTTBL_TOP	((unsigned)4)	/* pos of part inf in 512byte-blocks */
#define MAXPART		6
const unsigned char partition_conv[MAXPART + 1] = { 0, 1, 3, 4, 5, 6, 7 };

static int
get_scsi_part()
{
	struct {
		u_int32_t	magic;		/* 0x5836384B ("X68K") */
		u_int32_t	parttotal;
		u_int32_t	diskblocks;
		u_int32_t	diskblocks2;	/* backup? */
		struct dos_partition parttbl[NPART];
		unsigned char	formatstr[256];
		unsigned char	rest[512];
	} partbuf;
	int i;
	u_int32_t part_top;

#ifdef BOOT_DEBUG
	B_PRINT("seclen: ");
	print_hex(SCSI_BLKLEN, 8);	/* 0: 256, 1: 512, 2: 1024 */
	B_PRINT(", topsec: ");
	print_hex(SCSI_PARTTOP, 8);	/* partition top in sector */
#endif
	/*
	 * read partition table
	 */
	RAW_READ0(&partbuf, PARTTBL_TOP, sizeof partbuf);

	part_top = SCSI_PARTTOP >> (2 - SCSI_BLKLEN);
	for (i = 0; i < MAXPART; i++)
		if ((u_int32_t) partbuf.parttbl[i].dp_start == part_top)
			goto found;

	BOOT_ERROR("Can't boot from this partition");
	/* NOTREACHED */
found:
#ifdef BOOT_DEBUG
	B_PRINT("; sd");
	B_PUTC(ID + '0');	/* SCSI ID (not NetBSD unit #) */
	B_PUTC((unsigned int) partition_conv[i] + 'a');
	B_PRINT("\r\n");
#endif
	return partition_conv[i];
}
#endif	/* SCSI_ADHOC_BOOTPART */

/*
 * Check the type of SCSI interface
 */
#ifdef BOOT_DEBUG
static int
get_scsi_host_adapter(devstr)
	char *devstr;
#else
static int
get_scsi_host_adapter(void)
#endif
{
	char *bootrom;
	int ha;

#ifdef BOOT_DEBUG
	B_PRINT(" at ");
	*(int *)devstr = '/' << 24 | 's' << 16 | 'p' << 8 | 'c';
	*(int *)(devstr + 4) = '@' << 24 | '0' << 16 | '/' << 8 | 's';
	*(int *)(devstr + 8) = 'd' << 24 | '@' << 16 | '0' << 8 | ',';
	*(int *)(devstr + 12) = '0' << 24 | ':' << 16 | 'a' << 8 | '\0';
#endif

	bootrom = (char *) (BOOT_INFO & 0x00ffffe0);
	/*
	 * bootrom+0x24	"SCSIIN" ... Internal SCSI (spc@0)
	 *		"SCSIEX" ... External SCSI (spc@1 or mha@0)
	 */
	if (*(u_short *)(bootrom + 0x24 + 4) == 0x494e) {	/* "IN" */
#ifdef BOOT_DEBUG
		B_PRINT("spc0");
#endif
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 0;
	} else if (badbaddr(&IODEVbase->io_exspc.bdid)) {
#ifdef BOOT_DEBUG
		B_PRINT("mha0");
#endif
		ha = (X68K_BOOT_SCSIIF_MHA << 4) | 0;
#ifdef BOOT_DEBUG
		*(int *)devstr = '/' << 24 | 'm' << 16 | 'h' << 8 | 'a';
#endif
	} else {
#ifdef BOOT_DEBUG
		B_PRINT("spc1");
#endif
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 1;
#ifdef BOOT_DEBUG
		devstr[5] = '1';
#endif
	}

	return ha;
}

static int
load_file(path, addr, header)
	const char *path;
	unsigned int addr;
	struct exec *header;
{

	return load_file_ino(ufs_lookup_path(path), path, addr, header);
}

static int
load_file_ino(ino, fn, addr, header)
	ino_t ino;
	const char *fn;		/* for message only */
	unsigned int addr;
	struct exec *header;
{
	struct dinode dinode;

	/* look-up the file */
	if (ino == 0 || ufs_get_inode(ino, &dinode)) {
		B_PRINT(fn);
		B_PRINT(": not found\r\n");
		return 0;
	}

	ufs_read(&dinode, (void *)addr, 0, sizeof(struct exec));
	memcpy(header, (void *)addr, sizeof(struct exec));

	if ((N_GETMAGIC(*header) != OMAGIC) ||
	    (N_GETMID(*header) != MID_M68K)) {
		B_PRINT(fn);
		B_PRINT(": inappropriate format");
		return 0;
	}

	/* read text and data */
	ufs_read(&dinode, (void *)addr-sizeof(struct exec), 0,/* XXX */
		 header->a_text+header->a_data);

	/* clear out bss */
	memset((char*) addr + header->a_text+header->a_data,
	       0, header->a_bss);

	/* PLANNED: fallback NMAGIC loader for the kernel. */

	/* return the image size. */
	return header->a_text+header->a_data+header->a_bss;
}


void
bootufs(void)
{
	int bootdev;
#ifdef BOOT_DEBUG
	int i;
	char bootdevstr[16];
#endif
	struct exec header;
	int size;

#ifdef BOOT_DEBUG
	/* for debug; レジスタの状態をプリントする */
	for (i = 0; i < 16; i++) {
		print_hex(startregs[i], 8);
		B_PRINT((i & 7) == 7 ? "\r\n" : " ");
	}
#endif

	/*
	 * get boot device
	 */
	if (BINF_ISFD(&BOOT_INFO)) {
		/* floppy */
#ifdef BOOT_DEBUG
		*(int *)bootdevstr = ('f' << 24 | 'd' << 16 | '@' << 8 | '0') +
					(BOOT_INFO & 3);
		bootdevstr[4] = '\0';
#endif
		bootdev = X68K_MAKEBOOTDEV(X68K_MAJOR_FD, BOOT_INFO & 3,
				(FDSECMINMAX.minsec.N == 3) ? 0 : 2);
	} else {
		/* SCSI */
		int part, ha;

#ifdef SCSI_ADHOC_BOOTPART
		part = get_scsi_part();
#else
		part = 0;			/* sd?a only */
#endif
#ifndef BOOT_DEBUG
		ha = get_scsi_host_adapter();
#else
		ha = get_scsi_host_adapter(bootdevstr);
		bootdevstr[10] = '0' + (ID & 7);
		bootdevstr[14] = 'a' + part;
#endif
		bootdev = X68K_MAKESCSIBOOTDEV(X68K_MAJOR_SD, ha >> 4, ha & 15,
						ID & 7, 0, part);
	}
#ifdef BOOT_DEBUG
	B_PRINT("boot device: ");
	B_PRINT(bootdevstr);
#endif
	B_PRINT("\r\n");

	/* initialize filesystem code */
	if (ufs_init()) {
		BOOT_ERROR("bogus super block: "
			   "ルートファイルシステムが壊れています！");
		/* NOTREACHED */
	}
#if defined(BOOT_DEBUG) && defined(USE_FFS) && defined(USE_LFS)
	B_PRINT("file system: ");
	B_PUTC(ufs_info.fstype == UFSTYPE_FFS ?
			(unsigned int) 'F' : (unsigned int) 'L');
	B_PRINT("FS\r\n");
#endif

#ifdef BOOT_DEBUG
	B_PRINT("\r\nlooking up secondary boot... ");
#endif

	/*
	 * Look for the 2nd stage boot.
	 */

	/* Try "boot" first */
	size = load_file("boot", BOOT_TEXTADDR, &header);
#ifdef BOOT_DEBUG
	B_PRINT("done.\r\n");
#endif
	if (size > 0)
		exec_image(BOOT_TEXTADDR, /* image loaded at */
			   BOOT_TEXTADDR, /* image executed at */
			   header.a_entry, /* entry point */
			   size, /* image size */
			   bootdev, RB_SINGLE); /* arguments */

	B_PRINT("can't load the secondary bootstrap.;"
		"trying /netbsd...\r\n");

	/* fallback to /netbsd. */
	/* always fails since NMAGIC loader is not yet implemented. */

	size = load_file("netbsd", 0x6000, &header);
	if (size > 0) {
		if (*((short *)(0x6000 + header.a_entry - 2)) != 0) {
			B_PRINT("boot interface of /netbsd is too new!\r\n");
			goto fail;
		}
		exec_image(0x6000, /* image loaded at */
			   0,	/* image executed at */
			   header.a_entry, /* entry point */
			   size, /* image size */
			   bootdev, RB_SINGLE); /* arguments */
		/* NOTREACHED */
	}

 fail:
	BOOT_ERROR("can't load the secondary bootstrap nor the kernel.");
	/* NOTREACHED */
}
