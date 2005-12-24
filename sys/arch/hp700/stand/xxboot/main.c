/*	$NetBSD: main.c,v 1.4 2005/12/24 20:07:04 perry Exp $	*/

/*
 * Copyright (c) 2003 ITOH Yasufumi.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary forms are unlimited.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/ufs/dinode.h>
#include <sys/disklabel.h>
#include <sys/exec_elf.h>
#include "readufs.h"

#define STACK_SIZE	((unsigned) (64*1024))
#define LOAD_ALIGN	((unsigned) 2048)

#define PZ_MEM_BOOT	0x3d0
#define DEV_CLASS	0x2c
#define DEV_CL_MASK	0xf
#define DEV_CL_SEQU	0x2	/* sequential record access media */

static char *hexstr __P((char *, unsigned));
void ipl_main __P((unsigned /*interactive*/,
    unsigned /*sptop*/, unsigned /*psw*/));
void load_file __P((const char *, unsigned /*loadadr*/,
    unsigned /*interactive*/, int /*part*/));
void load_file_ino __P((ino32_t, const char *, unsigned /*loadadr*/,
    unsigned /*interactive*/, int /*part*/));

struct loadinfo {
	void *sec_image;
	unsigned sec_size;
#if 0
	unsigned sec_pad;
#endif
	unsigned entry_offset;
};
static inline void xi_elf32 __P((struct loadinfo *, Elf32_Ehdr *));
static inline void xi_elf64 __P((struct loadinfo *, Elf64_Ehdr *));
int xi_load __P((struct loadinfo *, void *));

void reboot __P((void)), halt __P((void));
void dispatch __P((unsigned /*interactive*/, unsigned /*top*/,
    unsigned /*end*/, int /*part*/, unsigned /*entry*/));
void print __P((const char *));
void putch __P((int));
int getch __P((void));
int boot_input __P((void *, int /*len*/, int /*pos*/));

/* to make generated code relocatable, do NOT mark them as const */
extern char str_seekseq[], str_bit_firmware[];
extern char str_crlf[], str_space[], str_rubout[];
extern char str_bootpart[], str_booting_part[];
extern char str_warn_2GB[], str_warn_unused[], str_nolabel[];
extern char str_filesystem[], str_nofs[];
extern char str_lookup[], str_loading[], str_at[], str_dddot[], str_done[];
extern char str_boot1[], str_boot2[], str_boot3[];
extern char str_noboot[];
extern char str_ukfmt[];

#ifdef __GNUC__
#define memcpy(d, s, n)	__builtin_memcpy(d, s, n)
#else
void *memcpy __P((void *, const void *, size_t));
#endif
void *memmove __P((void *, const void *, size_t));

/* disklabel */
union {
	char dklsec[512];
	struct disklabel dkl;	/* to ensure alignment */
} labelsector;
#define dklabel	(*(struct disklabel *)(labelsector.dklsec + LABELOFFSET))

unsigned offset_raw_read;

extern char diskbuf[2048];
#define BLK_PER_READ	4
#define MASK_BLK_PER_READ	(BLK_PER_READ - 1)

void
RAW_READ(buf, blkpos, bytelen)
	void *buf;
	daddr_t blkpos;
	size_t bytelen;
{
	char *b = buf;
	size_t off, readlen;
	int devoff;
	static int prvdevoff = -dbtob(BLK_PER_READ);
	int pos;

	for ( ; bytelen > 0; b += readlen, bytelen -= readlen) {
		/*
		 * read 2KB, avoiding unneeded read
		 */
		devoff = dbtob(blkpos & ~MASK_BLK_PER_READ) + offset_raw_read;
		if (prvdevoff != devoff) {
#if 1	/* supports sequential media */
			if ((*(unsigned *)(PZ_MEM_BOOT+DEV_CLASS) & DEV_CL_MASK)
				== DEV_CL_SEQU) {
				/*
				 * sequential media
				 * -- read sequentially or rewind
				 */
				pos = prvdevoff + dbtob(BLK_PER_READ);
				if (devoff < pos)
					pos = 0;	/* rewind */

				/* "repositioning media...\r\n" */
				if (devoff - pos > 512 * 1024)
					print(str_seekseq);

				for (; pos < devoff; pos += dbtob(BLK_PER_READ))
					boot_input(diskbuf,
					    dbtob(BLK_PER_READ), pos);
			}
#endif
			prvdevoff = devoff;
			boot_input(diskbuf, dbtob(BLK_PER_READ), devoff);
		}
		/*
		 * copy specified size to the destination
		 */
		off = dbtob(blkpos & MASK_BLK_PER_READ),
		readlen = dbtob(BLK_PER_READ) - off;
		if (readlen > bytelen)
			readlen = bytelen;
		memcpy(b, diskbuf + off, readlen);
		blkpos = (blkpos & ~MASK_BLK_PER_READ) + BLK_PER_READ;
	}
}

/*
 * convert number to hex string
 * buf must have enough space
 */
static char *
hexstr(buf, val)
	char *buf;
	unsigned val;
{
	unsigned v;
	char rev[16];
	char *r = rev, *b = buf;

	/* inverse order */
	do {
		v = val & 0xf;
		*r++ = (v <= 9) ? '0' + v : 'a' - 10 + v;
		val >>= 4;
	} while (val);

	/* reverse string */
	while (r > rev)
		*b++ = *--r;

	*b = '\0';
	return buf;
}

void
ipl_main(interactive, sptop, psw)
	unsigned interactive;		/* parameters from PDC */
	unsigned sptop;			/* value of sp on function entry */
	unsigned psw;			/* PSW on startup */
{
	char buf[32];
	int part = 0;		/* default partition "a" */
	unsigned secsz, partoff, partsz;
	int c, c1;
	unsigned loadadr;

#if 0
	print(hexstr(buf, interactive));
	print(str_crlf);
	print(hexstr(buf, sptop));
	print(str_crlf);
	print(hexstr(buf, psw));
	print(str_crlf);
#endif

	print(hexstr(buf, (psw & 0x08000000) ? (unsigned) 0x64 : 0x32));
	print(str_bit_firmware);	/* "bit firmware\r\n" */

	/*
	 * check disklabel
	 * (dklabel has disklabel on startup)
	 */
	if (dklabel.d_magic == DISKMAGIC && (secsz = dklabel.d_secsize) != 0) {
		/*
		 * select boot partition
		 */
		if (interactive) {
		select_partition:
			/* "boot partition (a-p, ! to reboot) [a]:" */
			print(str_bootpart);
			part = 0;		/* default partition "a" */
			c1 = 0;
			while ((c = getch()) >= 0) {
				switch (c) {
				case '\n':
				case '\r':
					goto break_while;
				case '\b':
				case '\177':
					if (c1) {
						print(str_rubout);
						part = c1 = 0;
					}
					break;
				case '!':	/* reset */
					if (c1 == 0) {
						part = -1;
						goto echoback;
					}
					break;
				default:
					if (c1 == 0 && c >= 'a' && c <= 'p') {
						part = c - 'a';
					echoback:
						putch(c);
						c1 = 1;
					}
					break;
				}
			}
		break_while:
			if (part == -1)
				return;		/* reset */
		}

		/*
		 * "\r\nbooting from partition _\r\n"
		 */
		str_booting_part[25] = 'a' + part;
		print(str_booting_part);

		partoff = dklabel.d_partitions[part].p_offset;
		partsz  = dklabel.d_partitions[part].p_size;

		if (part >= (int) dklabel.d_npartitions || partsz == 0) {
			print(str_warn_unused);	/* "unused partition\r\n" */
			goto select_partition;
		}

		/* boot partition must be below 2GB */
		if (partoff + partsz >=
		    (unsigned)((unsigned)2*1024*1024*1024 -1 + secsz) / secsz) {
			/* "boot partition exceeds 2GB boundary\r\n" */
			print(str_warn_2GB);
			goto select_partition;
		}

		/*
		 * following device accesses are only in the partition
		 */
		offset_raw_read = partoff * secsz;
	} else {
		/*
		 * no disklabel --- assume the whole of the device
		 * is a filesystem
		 */
		print(str_nolabel);	/* "no disklabel\r\n" */
	}

	if (ufs_init()) {
		print(str_nofs);	/* "no filesystem found\r\n" */
		return;
	}
	str_filesystem[12] = (ufs_info.fstype == UFSTYPE_FFS) ? 'F' : 'L';
	print(str_filesystem);		/* "filesystem: _FS\r\n" */

	loadadr = (sptop + STACK_SIZE + LOAD_ALIGN - 1) & (-LOAD_ALIGN);
	load_file(str_boot1, loadadr, interactive, part); /* "boot.hp700" */
	load_file(str_boot2, loadadr, interactive, part); /* "boot" */
	load_file(str_boot3, loadadr, interactive, part); /* "usr/mdec/boot" */

	print(str_noboot);		/* "no secondary boot found\r\n" */
}

void
load_file(path, loadadr, interactive, part)
	const char *path;
	unsigned loadadr, interactive;
	int part;
{

	/* look-up the file */
	print(str_lookup);	/* "looking up " */
	print(path);
	print(str_crlf);
	load_file_ino(ufs_lookup_path(path), path, loadadr, interactive, part);
}

void
load_file_ino(ino, fn, loadadr, interactive, part)
	ino32_t ino;
	const char *fn;		/* for message only */
	unsigned loadadr, interactive;
	int part;
{
	union ufs_dinode dinode;
	size_t sz;
	struct loadinfo inf;
	char buf[32];

	if (ino == 0 || ufs_get_inode(ino, &dinode))
		return;		/* not found */

	print(str_loading);		/* "loading " */
	print(fn);
	print(str_at);			/* " at 0x" */
	print(hexstr(buf, loadadr));
	print(str_dddot);		/* "..." */

	sz = DI_SIZE(&dinode);
	ufs_read(&dinode, (void *) loadadr, 0, sz);

	print(str_done);		/* "done\r\n" */

	/* digest executable format */
	inf.sec_size = sz;
	inf.entry_offset = 0;
	if (xi_load(&inf, (void *) loadadr)) {
		print(fn);
		print(str_ukfmt); /* ": unknown format -- exec from top\r\n" */
	}

	/* pass control to the secondary boot */
	dispatch(interactive, loadadr, loadadr + inf.sec_size, part,
	    loadadr + inf.entry_offset);
}

/*
 * fill in loading information from an ELF executable
 */
static inline void
xi_elf32(inf, hdr)
	struct loadinfo *inf;
	Elf32_Ehdr *hdr;
{
	char *top = (void *) hdr;
	Elf32_Phdr *ph;

	/* text + data, bss */
	ph = (void *) (top + hdr->e_phoff);
	inf->sec_image = top + ph->p_offset;
	inf->sec_size = ph->p_filesz;
#if 0
	inf->sec_pad = ph->p_memsz - ph->p_filesz;
#endif
	/* entry */
	inf->entry_offset = hdr->e_entry - ph->p_vaddr;
}

static inline void
xi_elf64(inf, hdr)
	struct loadinfo *inf;
	Elf64_Ehdr *hdr;
{
	char *top = (void *) hdr;
	Elf64_Phdr *ph;

	/*
	 * secondary boot is not so large, so 32bit (unsigned) arithmetic
	 * is enough
	 */
	/* text + data, bss */
	ph = (void *) (top + (unsigned) hdr->e_phoff);
	inf->sec_image = top + (unsigned) ph->p_offset;
	inf->sec_size = (unsigned) ph->p_filesz;
#if 0
	inf->sec_pad = (unsigned) ph->p_memsz - (unsigned) ph->p_filesz;
#endif
	/* entry */
	inf->entry_offset = (unsigned) hdr->e_entry - (unsigned) ph->p_vaddr;
}

int
xi_load(inf, buf)
	struct loadinfo *inf;
	void *buf;
{
	Elf32_Ehdr *e32hdr = buf;
	Elf64_Ehdr *e64hdr = buf;
	u_int16_t class_data;

	/*
	 * check ELF header
	 * (optimized assuming big endian byte order)
	 */
	/* ELF magic */
	if (*(u_int32_t *)&e32hdr->e_ident[EI_MAG0] !=
		(ELFMAG0 << 24 | ELFMAG1 << 16 | ELFMAG2 << 8 | ELFMAG3) ||
	    e32hdr->e_ident[EI_VERSION] != EV_CURRENT)
		return 1;	/* Not an ELF */

	/* file and machine type */
	if (*(u_int32_t *)&e32hdr->e_type != (ET_EXEC << 16 | EM_PARISC))
		return 1;	/* Not an executable / Wrong architecture */

	if ((class_data = *(u_int16_t *)&e32hdr->e_ident[EI_CLASS]) ==
	    (ELFCLASS32 << 8 | ELFDATA2MSB)) {

		/* support one section executable (ld -N) only */
		if (e32hdr->e_phnum != 1)
			return 1;	/* Wrong number of loading sections */

		/* fill in loading information */
		xi_elf32(inf, e32hdr);

	} else if (class_data == (ELFCLASS64 << 8 | ELFDATA2MSB)) {

		/* support one section executable (ld -N) only */
		if (e64hdr->e_phnum != 1)
			return 1;	/* Wrong number of loading sections */

		/* fill in loading information */
		xi_elf64(inf, e64hdr);

	} else
		return 1;	/* Not a 32bit or 64bit ELF */

	/* move text + data to the top address */
	memmove(buf, inf->sec_image, inf->sec_size);

#if 0	/* XXX bss clear is done by the secondary boot itself */
	bzero((char *) buf + inf->sec_size, inf->sec_pad);
#endif

	return 0;
}
