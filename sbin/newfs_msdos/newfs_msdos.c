/*	$NetBSD: newfs_msdos.c,v 1.38.2.1 2012/11/20 03:00:49 tls Exp $	*/

/*
 * Copyright (c) 1998 Robert Nordier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char rcsid[] =
  "$FreeBSD: src/sbin/newfs_msdos/newfs_msdos.c,v 1.15 2000/10/10 01:49:37 wollman Exp $";
#else
__RCSID("$NetBSD: newfs_msdos.c,v 1.38.2.1 2012/11/20 03:00:49 tls Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/disk.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include <util.h>
#include <disktab.h>

#include "partutil.h"

#define MAXU16	  0xffff	/* maximum unsigned 16-bit quantity */
#define BPN	  4		/* bits per nibble */
#define NPB	  2		/* nibbles per byte */

#define DOSMAGIC  0xaa55	/* DOS magic number */
#define MINBPS	  512		/* minimum bytes per sector */
#define MAXSPC	  128		/* maximum sectors per cluster */
#define MAXNFT	  16		/* maximum number of FATs */
#define DEFBLK	  4096		/* default block size */
#define DEFBLK16  2048		/* default block size FAT16 */
#define DEFRDE	  512		/* default root directory entries */
#define RESFTE	  2		/* reserved FAT entries */
#define MINCLS12  1		/* minimum FAT12 clusters */
#define MINCLS16  0xff5		/* minimum FAT16 clusters */
#define MINCLS32  0xfff5	/* minimum FAT32 clusters */
#define MAXCLS12  0xff4 	/* maximum FAT12 clusters */
#define MAXCLS16  0xfff4	/* maximum FAT16 clusters */
#define MAXCLS32  0xffffff4	/* maximum FAT32 clusters */

#define mincls(fat)  ((fat) == 12 ? MINCLS12 :	\
		      (fat) == 16 ? MINCLS16 :	\
				    MINCLS32)

#define maxcls(fat)  ((fat) == 12 ? MAXCLS12 :	\
		      (fat) == 16 ? MAXCLS16 :	\
				    MAXCLS32)

#define mk1(p, x)				\
    (p) = (u_int8_t)(x)

#define mk2(p, x)				\
    (p)[0] = (u_int8_t)(x),			\
    (p)[1] = (u_int8_t)((x) >> 010)

#define mk4(p, x)				\
    (p)[0] = (u_int8_t)(x),			\
    (p)[1] = (u_int8_t)((x) >> 010),		\
    (p)[2] = (u_int8_t)((x) >> 020),		\
    (p)[3] = (u_int8_t)((x) >> 030)

#define argto1(arg, lo, msg)  argtou(arg, lo, 0xff, msg)
#define argto2(arg, lo, msg)  argtou(arg, lo, 0xffff, msg)
#define argto4(arg, lo, msg)  argtou(arg, lo, 0xffffffff, msg)
#define argtox(arg, lo, msg)  argtou(arg, lo, UINT_MAX, msg)

struct bs {
    u_int8_t jmp[3];		/* bootstrap entry point */
    u_int8_t oem[8];		/* OEM name and version */
};

struct bsbpb {
    u_int8_t bps[2];		/* bytes per sector */
    u_int8_t spc;		/* sectors per cluster */
    u_int8_t res[2];		/* reserved sectors */
    u_int8_t nft;		/* number of FATs */
    u_int8_t rde[2];		/* root directory entries */
    u_int8_t sec[2];		/* total sectors */
    u_int8_t mid;		/* media descriptor */
    u_int8_t spf[2];		/* sectors per FAT */
    u_int8_t spt[2];		/* sectors per track */
    u_int8_t hds[2];		/* drive heads */
    u_int8_t hid[4];		/* hidden sectors */
    u_int8_t bsec[4];		/* big total sectors */
};

struct bsxbpb {
    u_int8_t bspf[4];		/* big sectors per FAT */
    u_int8_t xflg[2];		/* FAT control flags */
    u_int8_t vers[2];		/* file system version */
    u_int8_t rdcl[4];		/* root directory start cluster */
    u_int8_t infs[2];		/* file system info sector */
    u_int8_t bkbs[2];		/* backup boot sector */
    u_int8_t rsvd[12];		/* reserved */
};

struct bsx {
    u_int8_t drv;		/* drive number */
    u_int8_t rsvd;		/* reserved */
    u_int8_t sig;		/* extended boot signature */
    u_int8_t volid[4];		/* volume ID number */
    u_int8_t label[11]; 	/* volume label */
    u_int8_t type[8];		/* file system type */
};

struct de {
    u_int8_t namext[11];	/* name and extension */
    u_int8_t attr;		/* attributes */
    u_int8_t rsvd[10];		/* reserved */
    u_int8_t time[2];		/* creation time */
    u_int8_t date[2];		/* creation date */
    u_int8_t clus[2];		/* starting cluster */
    u_int8_t size[4];		/* size */
};

struct bpb {
    u_int bps;			/* bytes per sector */
    u_int spc;			/* sectors per cluster */
    u_int res;			/* reserved sectors */
    u_int nft;			/* number of FATs */
    u_int rde;			/* root directory entries */
    u_int sec;			/* total sectors */
    u_int mid;			/* media descriptor */
    u_int spf;			/* sectors per FAT */
    u_int spt;			/* sectors per track */
    u_int hds;			/* drive heads */
    u_int hid;			/* hidden sectors */
    u_int bsec; 		/* big total sectors */
    u_int bspf; 		/* big sectors per FAT */
    u_int rdcl; 		/* root directory start cluster */
    u_int infs; 		/* file system info sector */
    u_int bkbs; 		/* backup boot sector */
};

#define INIT(a, b, c, d, e, f, g, h, i, j) \
    { .bps = a, .spc = b, .res = c, .nft = d, .rde = e, \
      .sec = f, .mid = g, .spf = h, .spt = i, .hds = j, }
static struct {
    const char *name;
    struct bpb bpb;
} stdfmt[] = {
    {"160",  INIT(512, 1, 1, 2,  64,  320, 0xfe, 1,  8, 1)},
    {"180",  INIT(512, 1, 1, 2,  64,  360, 0xfc, 2,  9, 1)},
    {"320",  INIT(512, 2, 1, 2, 112,  640, 0xff, 1,  8, 2)},
    {"360",  INIT(512, 2, 1, 2, 112,  720, 0xfd, 2,  9, 2)},
    {"640",  INIT(512, 2, 1, 2, 112, 1280, 0xfb, 2,  8, 2)},    
    {"720",  INIT(512, 2, 1, 2, 112, 1440, 0xf9, 3,  9, 2)},
    {"1200", INIT(512, 1, 1, 2, 224, 2400, 0xf9, 7, 15, 2)},
    {"1232", INIT(1024,1, 1, 2, 192, 1232, 0xfe, 2,  8, 2)},    
    {"1440", INIT(512, 1, 1, 2, 224, 2880, 0xf0, 9, 18, 2)},
    {"2880", INIT(512, 2, 1, 2, 240, 5760, 0xf0, 9, 36, 2)}
};

static u_int8_t bootcode[] = {
    0xfa,			/* cli		    */
    0x31, 0xc0, 		/* xor	   ax,ax    */
    0x8e, 0xd0, 		/* mov	   ss,ax    */
    0xbc, 0x00, 0x7c,		/* mov	   sp,7c00h */
    0xfb,			/* sti		    */
    0x8e, 0xd8, 		/* mov	   ds,ax    */
    0xe8, 0x00, 0x00,		/* call    $ + 3    */
    0x5e,			/* pop	   si	    */
    0x83, 0xc6, 0x19,		/* add	   si,+19h  */
    0xbb, 0x07, 0x00,		/* mov	   bx,0007h */
    0xfc,			/* cld		    */
    0xac,			/* lodsb	    */
    0x84, 0xc0, 		/* test    al,al    */
    0x74, 0x06, 		/* jz	   $ + 8    */
    0xb4, 0x0e, 		/* mov	   ah,0eh   */
    0xcd, 0x10, 		/* int	   10h	    */
    0xeb, 0xf5, 		/* jmp	   $ - 9    */
    0x30, 0xe4, 		/* xor	   ah,ah    */
    0xcd, 0x16, 		/* int	   16h	    */
    0xcd, 0x19, 		/* int	   19h	    */
    0x0d, 0x0a,
    'N', 'o', 'n', '-', 's', 'y', 's', 't',
    'e', 'm', ' ', 'd', 'i', 's', 'k',
    0x0d, 0x0a,
    'P', 'r', 'e', 's', 's', ' ', 'a', 'n',
    'y', ' ', 'k', 'e', 'y', ' ', 't', 'o',
    ' ', 'r', 'e', 'b', 'o', 'o', 't',
    0x0d, 0x0a,
    0
};

static int	got_siginfo = 0; /* received a SIGINFO */

static void check_mounted(const char *, mode_t);
static void getstdfmt(const char *, struct bpb *);
static void getbpbinfo(int, const char *, const char *, int, struct bpb *, int);
static void print_bpb(struct bpb *);
static u_int ckgeom(const char *, u_int, const char *);
static u_int argtou(const char *, u_int, u_int, const char *);
static off_t argtooff(const char *, const char *);
static int oklabel(const char *);
static void mklabel(u_int8_t *, const char *);
static void setstr(u_int8_t *, const char *, size_t);
__dead static void usage(void);
static void infohandler(int sig);

/*
 * Construct a FAT12, FAT16, or FAT32 file system.
 */
int
main(int argc, char *argv[])
{
    static const char opts[] = "@:NB:C:F:I:L:O:S:a:b:c:e:f:h:i:k:m:n:o:r:s:u:";
    static const char *opt_B, *opt_L, *opt_O, *opt_f;
    static u_int opt_F, opt_I, opt_S, opt_a, opt_b, opt_c, opt_e;
    static u_int opt_h, opt_i, opt_k, opt_m, opt_n, opt_o, opt_r;
    static u_int opt_s, opt_u;
    static int opt_N;
    static int Iflag, mflag, oflag;
    char buf[MAXPATHLEN];
    struct stat sb;
    struct timeval tv;
    struct bpb bpb;
    struct tm *tm;
    struct bs *bs;
    struct bsbpb *bsbpb;
    struct bsxbpb *bsxbpb;
    struct bsx *bsx;
    struct de *de;
    u_int8_t *img;
    const char *fname, *dtype, *bname;
    ssize_t n;
    time_t now;
    u_int fat, bss, rds, cls, dir, lsn, x, x1, x2;
    int ch, fd, fd1;
    static off_t opt_create=0, opt_ofs=0;

    while ((ch = getopt(argc, argv, opts)) != -1)
	switch (ch) {
	case '@':
	    opt_ofs = argtooff(optarg, "offset");
	    break;
	case 'N':
	    opt_N = 1;
	    break;
	case 'B':
	    opt_B = optarg;
	    break;
	case 'C':
	    opt_create = argtooff(optarg, "create size");
	    break;
	case 'F':
	    if (strcmp(optarg, "12") &&
		strcmp(optarg, "16") &&
		strcmp(optarg, "32"))
		errx(1, "%s: bad FAT type", optarg);
	    opt_F = atoi(optarg);
	    break;
	case 'I':
	    opt_I = argto4(optarg, 0, "volume ID");
	    Iflag = 1;
	    break;
	case 'L':
	    if (!oklabel(optarg))
		errx(1, "%s: bad volume label", optarg);
	    opt_L = optarg;
	    break;
	case 'O':
	    if (strlen(optarg) > 8)
		errx(1, "%s: bad OEM string", optarg);
	    opt_O = optarg;
	    break;
	case 'S':
	    opt_S = argto2(optarg, 1, "bytes/sector");
	    break;
	case 'a':
	    opt_a = argto4(optarg, 1, "sectors/FAT");
	    break;
	case 'b':
	    opt_b = argtox(optarg, 1, "block size");
	    opt_c = 0;
	    break;
	case 'c':
	    opt_c = argto1(optarg, 1, "sectors/cluster");
	    opt_b = 0;
	    break;
	case 'e':
	    opt_e = argto2(optarg, 1, "directory entries");
	    break;
	case 'f':
	    opt_f = optarg;
	    break;
	case 'h':
	    opt_h = argto2(optarg, 1, "drive heads");
	    break;
	case 'i':
	    opt_i = argto2(optarg, 1, "info sector");
	    break;
	case 'k':
	    opt_k = argto2(optarg, 1, "backup sector");
	    break;
	case 'm':
	    opt_m = argto1(optarg, 0, "media descriptor");
	    mflag = 1;
	    break;
	case 'n':
	    opt_n = argto1(optarg, 1, "number of FATs");
	    break;
	case 'o':
	    opt_o = argto4(optarg, 0, "hidden sectors");
	    oflag = 1;
	    break;
	case 'r':
	    opt_r = argto2(optarg, 1, "reserved sectors");
	    break;
	case 's':
	    opt_s = argto4(optarg, 1, "file system size");
	    break;
	case 'u':
	    opt_u = argto2(optarg, 1, "sectors/track");
	    break;
	default:
	    usage();
	}
    argc -= optind;
    argv += optind;
    if (argc < 1 || argc > 2)
	usage();
    fname = *argv++;
    if (!strchr(fname, '/') && !opt_create) {
	snprintf(buf, sizeof(buf), "%sr%s", _PATH_DEV, fname);
	if (!(fname = strdup(buf)))
	    err(1, NULL);
    }
    dtype = *argv;
    if (opt_create) {
	off_t pos;

	if (opt_N)
	    errx(1, "create (-C) is incompatible with -N");
	fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
	    errx(1, "failed to create %s", fname);
	pos = lseek(fd, opt_create - 1, SEEK_SET);
	if (write(fd, "\0", 1) != 1)
	    err(1, "failed to set file size");
	pos = lseek(fd, 0, SEEK_SET);
    } else if ((fd = open(fname, opt_N ? O_RDONLY : O_RDWR)) == -1 ||
	fstat(fd, &sb))
	err(1, "%s", fname);
    if (!opt_N)
	check_mounted(fname, sb.st_mode);
    if (!S_ISCHR(sb.st_mode) && !opt_create)
	warnx("warning, %s is not a character device", fname);
    if (opt_ofs && opt_ofs != lseek(fd, opt_ofs, SEEK_SET))
	errx(1, "cannot seek to %jd", (intmax_t)opt_ofs);
    memset(&bpb, 0, sizeof(bpb));
    if (opt_f) {
	getstdfmt(opt_f, &bpb);
	bpb.bsec = bpb.sec;
	bpb.sec = 0;
	bpb.bspf = bpb.spf;
	bpb.spf = 0;
    }
    if (opt_h)
	bpb.hds = opt_h;
    if (opt_u)
	bpb.spt = opt_u;
    if (opt_S)
	bpb.bps = opt_S;
    if (opt_s)
	bpb.bsec = opt_s;
    if (oflag)
	bpb.hid = opt_o;
    if (!(opt_f || (opt_h && opt_u && opt_S && opt_s && oflag))) {
	off_t delta;
	getbpbinfo(fd, fname, dtype, oflag, &bpb, opt_create != 0);
	bpb.bsec -= (opt_ofs / bpb.bps);
	delta = bpb.bsec % bpb.spt;
	if (delta != 0) {
	    warnx("trim %d sectors to adjust to a multiple of %d",
		(int)delta, bpb.spt);
	    bpb.bsec -= delta;
	}
	if (bpb.spc == 0) {     /* set defaults */
	    if (bpb.bsec <= 6000)       /* about 3MB -> 512 bytes */
		bpb.spc = 1;
	    else if (bpb.bsec <= (1<<17)) /* 64M -> 4k */
		bpb.spc = 8;
	    else if (bpb.bsec <= (1<<19)) /* 256M -> 8k */
		bpb.spc = 16;
	    else if (bpb.bsec <= (1<<21)) /* 1G -> 16k */
		bpb.spc = 32;
	    else
		bpb.spc = 64;           /* otherwise 32k */
	}
    }
    if (!powerof2(bpb.bps))
	errx(1, "bytes/sector (%u) is not a power of 2", bpb.bps);
    if (bpb.bps < MINBPS)
	errx(1, "bytes/sector (%u) is too small; minimum is %u",
	     bpb.bps, MINBPS);
    if (!(fat = opt_F)) {
	if (opt_f)
	    fat = 12;
	else if (!opt_e && (opt_i || opt_k))
	    fat = 32;
    }
    if ((fat == 32 && opt_e) || (fat != 32 && (opt_i || opt_k)))
	errx(1, "-%c is not a legal FAT%s option",
	     fat == 32 ? 'e' : opt_i ? 'i' : 'k',
	     fat == 32 ? "32" : "12/16");
    if (opt_f && fat == 32)
	bpb.rde = 0;
    if (opt_b) {
	if (!powerof2(opt_b))
	    errx(1, "block size (%u) is not a power of 2", opt_b);
	if (opt_b < bpb.bps)
	    errx(1, "block size (%u) is too small; minimum is %u",
		 opt_b, bpb.bps);
	if (opt_b > bpb.bps * MAXSPC)
	    errx(1, "block size (%u) is too large; maximum is %u",
		 opt_b, bpb.bps * MAXSPC);
	bpb.spc = opt_b / bpb.bps;
    }
    if (opt_c) {
	if (!powerof2(opt_c))
	    errx(1, "sectors/cluster (%u) is not a power of 2", opt_c);
	bpb.spc = opt_c;
    }
    if (opt_r)
	bpb.res = opt_r;
    if (opt_n) {
	if (opt_n > MAXNFT)
	    errx(1, "number of FATs (%u) is too large; maximum is %u",
		 opt_n, MAXNFT);
	bpb.nft = opt_n;
    }
    if (opt_e)
	bpb.rde = opt_e;
    if (mflag) {
	if (opt_m < 0xf0)
	    errx(1, "illegal media descriptor (%#x)", opt_m);
	bpb.mid = opt_m;
    }
    if (opt_a)
	bpb.bspf = opt_a;
    if (opt_i)
	bpb.infs = opt_i;
    if (opt_k)
	bpb.bkbs = opt_k;
    bss = 1;
    bname = NULL;
    fd1 = -1;
    if (opt_B) {
	bname = opt_B;
	if (!strchr(bname, '/')) {
	    snprintf(buf, sizeof(buf), "/boot/%s", bname);
	    if (!(bname = strdup(buf)))
		err(1, NULL);
	}
	if ((fd1 = open(bname, O_RDONLY)) == -1 || fstat(fd1, &sb))
	    err(1, "%s", bname);
	if (!S_ISREG(sb.st_mode) || sb.st_size % bpb.bps ||
	    sb.st_size < bpb.bps || sb.st_size > bpb.bps * MAXU16)
	    errx(1, "%s: inappropriate file type or format", bname);
	bss = sb.st_size / bpb.bps;
    }
    if (!bpb.nft)
	bpb.nft = 2;
    if (!fat) {
	if (bpb.bsec < (bpb.res ? bpb.res : bss) +
	    howmany((RESFTE + (bpb.spc ? MINCLS16 : MAXCLS12 + 1)) *
		    ((bpb.spc ? 16 : 12) / BPN), bpb.bps * NPB) *
	    bpb.nft +
	    howmany(bpb.rde ? bpb.rde : DEFRDE,
		    bpb.bps / sizeof(struct de)) +
	    (bpb.spc ? MINCLS16 : MAXCLS12 + 1) *
	    (bpb.spc ? bpb.spc : howmany(DEFBLK, bpb.bps)))
	    fat = 12;
	else if (bpb.rde || bpb.bsec <
		 (bpb.res ? bpb.res : bss) +
		 howmany((RESFTE + MAXCLS16) * 2, bpb.bps) * bpb.nft +
		 howmany(DEFRDE, bpb.bps / sizeof(struct de)) +
		 (MAXCLS16 + 1) *
		 (bpb.spc ? bpb.spc : howmany(8192, bpb.bps)))
	    fat = 16;
	else
	    fat = 32;
    }
    x = bss;
    if (fat == 32) {
	if (!bpb.infs) {
	    if (x == MAXU16 || x == bpb.bkbs)
		errx(1, "no room for info sector");
	    bpb.infs = x;
	}
	if (bpb.infs != MAXU16 && x <= bpb.infs)
	    x = bpb.infs + 1;
	if (!bpb.bkbs) {
	    if (x == MAXU16)
		errx(1, "no room for backup sector");
	    bpb.bkbs = x;
	} else if (bpb.bkbs != MAXU16 && bpb.bkbs == bpb.infs)
	    errx(1, "backup sector would overwrite info sector");
	if (bpb.bkbs != MAXU16 && x <= bpb.bkbs)
	    x = bpb.bkbs + 1;
    }
    if (!bpb.res)
	bpb.res = fat == 32 ? MAX(x, MAX(16384 / bpb.bps, 4)) : x;
    else if (bpb.res < x)
	errx(1, "too few reserved sectors (need %d have %d)", x, bpb.res);
    if (fat != 32 && !bpb.rde)
	bpb.rde = DEFRDE;
    rds = howmany(bpb.rde, bpb.bps / sizeof(struct de));
    if (!bpb.spc)
	for (bpb.spc = howmany(fat == 16 ? DEFBLK16 : DEFBLK, bpb.bps);
	     bpb.spc < MAXSPC &&
	     bpb.res +
	     howmany((RESFTE + maxcls(fat)) * (fat / BPN),
		     bpb.bps * NPB) * bpb.nft +
	     rds +
	     (u_int64_t)(maxcls(fat) + 1) * bpb.spc <= bpb.bsec;
	     bpb.spc <<= 1);
    if (fat != 32 && bpb.bspf > MAXU16)
	errx(1, "too many sectors/FAT for FAT12/16");
    x1 = bpb.res + rds;
    x = bpb.bspf ? bpb.bspf : 1;
    if (x1 + (u_int64_t)x * bpb.nft > bpb.bsec)
	errx(1, "meta data exceeds file system size");
    x1 += x * bpb.nft;
    x = (u_int64_t)(bpb.bsec - x1) * bpb.bps * NPB /
	(bpb.spc * bpb.bps * NPB + fat / BPN * bpb.nft);
    x2 = howmany((RESFTE + MIN(x, maxcls(fat))) * (fat / BPN),
		 bpb.bps * NPB);
    if (!bpb.bspf) {
	bpb.bspf = x2;
	x1 += (bpb.bspf - 1) * bpb.nft;
    }
    cls = (bpb.bsec - x1) / bpb.spc;
    x = (u_int64_t)bpb.bspf * bpb.bps * NPB / (fat / BPN) - RESFTE;
    if (cls > x)
	cls = x;
    if (bpb.bspf < x2)
	warnx("warning: sectors/FAT limits file system to %u clusters",
	      cls);
    if (cls < mincls(fat))
	errx(1, "%u clusters too few clusters for FAT%u, need %u", cls, fat,
	    mincls(fat));
    if (cls > maxcls(fat)) {
	cls = maxcls(fat);
	bpb.bsec = x1 + (cls + 1) * bpb.spc - 1;
	warnx("warning: FAT type limits file system to %u sectors",
	      bpb.bsec);
    }
    printf("%s: %u sector%s in %u FAT%u cluster%s "
	   "(%u bytes/cluster)\n", fname, cls * bpb.spc,
	   cls * bpb.spc == 1 ? "" : "s", cls, fat,
	   cls == 1 ? "" : "s", bpb.bps * bpb.spc);
    if (!bpb.mid)
	bpb.mid = !bpb.hid ? 0xf0 : 0xf8;
    if (fat == 32)
	bpb.rdcl = RESFTE;
    if (bpb.hid + bpb.bsec <= MAXU16) {
	bpb.sec = bpb.bsec;
	bpb.bsec = 0;
    }
    if (fat != 32) {
	bpb.spf = bpb.bspf;
	bpb.bspf = 0;
    }
    ch = 0;
    if (fat == 12)
	ch = 1;			/* 001 Primary DOS with 12 bit FAT */
    else if (fat == 16) {
	if (bpb.bsec == 0)
	    ch = 4;		/* 004 Primary DOS with 16 bit FAT <32M */
	else
	    ch = 6;		/* 006 Primary 'big' DOS, 16-bit FAT (> 32MB) */
				/*
				 * XXX: what about:
				 * 014 DOS (16-bit FAT) - LBA
				 *  ?
				 */
    } else if (fat == 32) {
	ch = 11;		/* 011 Primary DOS with 32 bit FAT */
				/*
				 * XXX: what about:
				 * 012 Primary DOS with 32 bit FAT - LBA
				 *  ?
				 */
    }
    if (ch != 0)
	printf("MBR type: %d\n", ch);
    print_bpb(&bpb);
    if (!opt_N) {
	gettimeofday(&tv, NULL);
	now = tv.tv_sec;
	tm = localtime(&now);
	if (!(img = malloc(bpb.bps)))
	    err(1, NULL);
	dir = bpb.res + (bpb.spf ? bpb.spf : bpb.bspf) * bpb.nft;
	signal(SIGINFO, infohandler);
	for (lsn = 0; lsn < dir + (fat == 32 ? bpb.spc : rds); lsn++) {
	    if (got_siginfo) {
		fprintf(stderr,"%s: writing sector %u of %u (%u%%)\n",
		    fname,lsn,(dir + (fat == 32 ? bpb.spc : rds)),
		    (lsn*100)/(dir + (fat == 32 ? bpb.spc : rds)));
		got_siginfo = 0;
	    }
	    x = lsn;
	    if (opt_B &&
		fat == 32 && bpb.bkbs != MAXU16 &&
		bss <= bpb.bkbs && x >= bpb.bkbs) {
		x -= bpb.bkbs;
		if (!x && lseek(fd1, opt_ofs, SEEK_SET))
		    err(1, "%s", bname);
	    }
	    if (opt_B && x < bss) {
		if ((n = read(fd1, img, bpb.bps)) == -1)
		    err(1, "%s", bname);
		if ((size_t)n != bpb.bps)
		    errx(1, "%s: can't read sector %u", bname, x);
	    } else
		memset(img, 0, bpb.bps);
	    if (!lsn ||
	      (fat == 32 && bpb.bkbs != MAXU16 && lsn == bpb.bkbs)) {
		x1 = sizeof(struct bs);
		bsbpb = (struct bsbpb *)(img + x1);
		mk2(bsbpb->bps, bpb.bps);
		mk1(bsbpb->spc, bpb.spc);
		mk2(bsbpb->res, bpb.res);
		mk1(bsbpb->nft, bpb.nft);
		mk2(bsbpb->rde, bpb.rde);
		mk2(bsbpb->sec, bpb.sec);
		mk1(bsbpb->mid, bpb.mid);
		mk2(bsbpb->spf, bpb.spf);
		mk2(bsbpb->spt, bpb.spt);
		mk2(bsbpb->hds, bpb.hds);
		mk4(bsbpb->hid, bpb.hid);
		mk4(bsbpb->bsec, bpb.bsec);
		x1 += sizeof(struct bsbpb);
		if (fat == 32) {
		    bsxbpb = (struct bsxbpb *)(img + x1);
		    mk4(bsxbpb->bspf, bpb.bspf);
		    mk2(bsxbpb->xflg, 0);
		    mk2(bsxbpb->vers, 0);
		    mk4(bsxbpb->rdcl, bpb.rdcl);
		    mk2(bsxbpb->infs, bpb.infs);
		    mk2(bsxbpb->bkbs, bpb.bkbs);
		    x1 += sizeof(struct bsxbpb);
		}
		bsx = (struct bsx *)(img + x1);
		mk1(bsx->sig, 0x29);
		if (Iflag)
		    x = opt_I;
		else
		    x = (((u_int)(1 + tm->tm_mon) << 8 |
			  (u_int)tm->tm_mday) +
			 ((u_int)tm->tm_sec << 8 |
			  (u_int)(tv.tv_usec / 10))) << 16 |
			((u_int)(1900 + tm->tm_year) +
			 ((u_int)tm->tm_hour << 8 |
			  (u_int)tm->tm_min));
		mk4(bsx->volid, x);
		mklabel(bsx->label, opt_L ? opt_L : "NO NAME");
		snprintf(buf, sizeof(buf), "FAT%u", fat);
		setstr(bsx->type, buf, sizeof(bsx->type));
		if (!opt_B) {
		    x1 += sizeof(struct bsx);
		    bs = (struct bs *)img;
		    mk1(bs->jmp[0], 0xeb);
		    mk1(bs->jmp[1], x1 - 2);
		    mk1(bs->jmp[2], 0x90);
		    setstr(bs->oem, opt_O ? opt_O : "NetBSD",
			   sizeof(bs->oem));
		    memcpy(img + x1, bootcode, sizeof(bootcode));
		    mk2(img + MINBPS - 2, DOSMAGIC);
		}
	    } else if (fat == 32 && bpb.infs != MAXU16 &&
		       (lsn == bpb.infs ||
			(bpb.bkbs != MAXU16 &&
			 lsn == bpb.bkbs + bpb.infs))) {
		mk4(img, 0x41615252);
		mk4(img + MINBPS - 28, 0x61417272);
		mk4(img + MINBPS - 24, 0xffffffff);
		mk4(img + MINBPS - 20, bpb.rdcl);
		mk2(img + MINBPS - 2, DOSMAGIC);
	    } else if (lsn >= bpb.res && lsn < dir &&
		       !((lsn - bpb.res) %
			 (bpb.spf ? bpb.spf : bpb.bspf))) {
		mk1(img[0], bpb.mid);
		for (x = 1; x < fat * (fat == 32 ? 3 : 2) / 8; x++)
		    mk1(img[x], fat == 32 && x % 4 == 3 ? 0x0f : 0xff);
	    } else if (lsn == dir && opt_L) {
		de = (struct de *)img;
		mklabel(de->namext, opt_L);
		mk1(de->attr, 050);
		x = (u_int)tm->tm_hour << 11 |
		    (u_int)tm->tm_min << 5 |
		    (u_int)tm->tm_sec >> 1;
		mk2(de->time, x);
		x = (u_int)(tm->tm_year - 80) << 9 |
		    (u_int)(tm->tm_mon + 1) << 5 |
		    (u_int)tm->tm_mday;
		mk2(de->date, x);
	    }
	    if ((n = write(fd, img, bpb.bps)) == -1)
		err(1, "%s", fname);
	    if ((size_t)n != bpb.bps)
		errx(1, "%s: can't write sector %u", fname, lsn);
	}
    }
    return 0;
}

/*
 * Exit with error if file system is mounted.
 */
static void
check_mounted(const char *fname, mode_t mode)
{
    struct statvfs *mp;
    const char *s1, *s2;
    size_t len;
    int n, r;

    if (!(n = getmntinfo(&mp, MNT_NOWAIT)))
	err(1, "getmntinfo");
    len = strlen(_PATH_DEV);
    s1 = fname;
    if (!strncmp(s1, _PATH_DEV, len))
	s1 += len;
    r = S_ISCHR(mode) && s1 != fname && *s1 == 'r';
    for (; n--; mp++) {
	s2 = mp->f_mntfromname;
	if (!strncmp(s2, _PATH_DEV, len))
	    s2 += len;
	if ((r && s2 != mp->f_mntfromname && !strcmp(s1 + 1, s2)) ||
	    !strcmp(s1, s2))
	    errx(1, "%s is mounted on %s", fname, mp->f_mntonname);
    }
}

/*
 * Get a standard format.
 */
static void
getstdfmt(const char *fmt, struct bpb *bpb)
{
    u_int x, i;

    x = sizeof(stdfmt) / sizeof(stdfmt[0]);
    for (i = 0; i < x && strcmp(fmt, stdfmt[i].name); i++);
    if (i == x)
	errx(1, "%s: unknown standard format", fmt);
    *bpb = stdfmt[i].bpb;
}

/*
 * Get disk slice, partition, and geometry information.
 */
static void
getbpbinfo(int fd, const char *fname, const char *dtype, int oflag,
	    struct bpb *bpb, int create)
{
    struct disk_geom geo;
    struct dkwedge_info dkw;
    const char *s1, *s2;
    int part;
    int maxpartitions;

    part = -1;
    s1 = fname;
    if ((s2 = strrchr(s1, '/')))
	s1 = s2 + 1;
    for (s2 = s1; *s2 && !isdigit((unsigned char)*s2); s2++);
    if (!*s2 || s2 == s1)
	s2 = NULL;
    else
	while (isdigit((unsigned char)*++s2));
    s1 = s2;

    maxpartitions = getmaxpartitions();

    if (s2 && *s2 >= 'a' && *s2 <= 'a' + maxpartitions - 1) {
	part = *s2++ - 'a';
    }
    if (((part != -1) && ((!oflag && part != -1) || !bpb->bsec)) ||
	!bpb->bps || !bpb->spt || !bpb->hds) {
	if (create || getdiskinfo(fname, fd, NULL, &geo, &dkw) == -1) {
	    struct stat st;

	    if (fstat(fd, &st) == -1)
		errx(1, "Can't get disk size for `%s'", fname);
	    /* create a fake geometry for a file image */
	    geo.dg_secsize = 512;
	    geo.dg_nsectors = 63;
	    geo.dg_ntracks = 255;
	    dkw.dkw_size = st.st_size / geo.dg_secsize;
	}
	if (!bpb->bps)
	    bpb->bps = ckgeom(fname, geo.dg_secsize, "bytes/sector");

	if (geo.dg_nsectors > 63) {
		/*
		 * The kernel doesn't accept BPB with spt > 63.
		 * (see sys/fs/msdosfs/msdosfs_vfsops.c:msdosfs_mountfs())
		 * If values taken from disklabel don't match these
		 * restrictions, use popular BIOS default values instead.
		 */
		geo.dg_nsectors = 63;
	}
	if (!bpb->spt)
	    bpb->spt = ckgeom(fname, geo.dg_nsectors, "sectors/track");
	if (!bpb->hds)
	    bpb->hds = ckgeom(fname, geo.dg_ntracks, "drive heads");
	if (!bpb->bsec)
	    bpb->bsec = dkw.dkw_size;
    }
}

/*
 * Print out BPB values.
 */
static void
print_bpb(struct bpb *bpb)
{
    printf("bps=%u spc=%u res=%u nft=%u", bpb->bps, bpb->spc, bpb->res,
	   bpb->nft);
    if (bpb->rde)
	printf(" rde=%u", bpb->rde);
    if (bpb->sec)
	printf(" sec=%u", bpb->sec);
    printf(" mid=%#x", bpb->mid);
    if (bpb->spf)
	printf(" spf=%u", bpb->spf);
    printf(" spt=%u hds=%u hid=%u", bpb->spt, bpb->hds, bpb->hid);
    if (bpb->bsec)
	printf(" bsec=%u", bpb->bsec);
    if (!bpb->spf) {
	printf(" bspf=%u rdcl=%u", bpb->bspf, bpb->rdcl);
	printf(" infs=");
	printf(bpb->infs == MAXU16 ? "%#x" : "%u", bpb->infs);
	printf(" bkbs=");
	printf(bpb->bkbs == MAXU16 ? "%#x" : "%u", bpb->bkbs);
    }
    printf("\n");
}

/*
 * Check a disk geometry value.
 */
static u_int
ckgeom(const char *fname, u_int val, const char *msg)
{
    if (!val)
	errx(1, "%s: no default %s", fname, msg);
    if (val > MAXU16)
	errx(1, "%s: illegal %s", fname, msg);
    return val;
}

/*
 * Convert and check a numeric option argument.
 */
static u_int
argtou(const char *arg, u_int lo, u_int hi, const char *msg)
{
    char *s;
    u_long x;

    errno = 0;
    x = strtoul(arg, &s, 0);
    if (errno || !*arg || *s || x < lo || x > hi)
	errx(1, "%s: bad %s", arg, msg);
    return x;
}

/*
 * Same for off_t, with optional skmgpP suffix
 */
static off_t
argtooff(const char *arg, const char *msg)
{
    char *s;
    off_t x;

    errno = 0;
    x = strtoll(arg, &s, 0);
    /* allow at most one extra char */
    if (errno || x < 0 || (s[0] && s[1]) )
	errx(1, "%s: bad %s", arg, msg);
    if (*s) {	/* the extra char is the multiplier */
	switch (*s) {
	default:
	    errx(1, "%s: bad %s", arg, msg);
	    /* notreached */
	
	case 's':	/* sector */
	case 'S':
	    x <<= 9;	/* times 512 */
	    break;

	case 'k':	/* kilobyte */
	case 'K':
	    x <<= 10;	/* times 1024 */
	    break;

	case 'm':	/* megabyte */
	case 'M':
	    x <<= 20;	/* times 1024*1024 */
	    break;

	case 'g':	/* gigabyte */
	case 'G':
	    x <<= 30;	/* times 1024*1024*1024 */
	    break;

	case 'p':	/* partition start */
	case 'P':	/* partition start */
	case 'l':	/* partition length */
	case 'L':	/* partition length */
	    errx(1, "%s: not supported yet %s", arg, msg);
	    /* notreached */
	}
    }
    return x;
}

/*
 * Check a volume label.
 */
static int
oklabel(const char *src)
{
    int c, i;

    for (i = 0; i <= 11; i++) {
	c = (u_char)*src++;
	if (c < ' ' + !i || strchr("\"*+,./:;<=>?[\\]|", c))
	    break;
    }
    return i && !c;
}

/*
 * Make a volume label.
 */
static void
mklabel(u_int8_t *dest, const char *src)
{
    int c, i;

    for (i = 0; i < 11; i++) {
	c = *src ? toupper((unsigned char)*src++) : ' ';
	*dest++ = !i && c == '\xe5' ? 5 : c;
    }
}

/*
 * Copy string, padding with spaces.
 */
static void
setstr(u_int8_t *dest, const char *src, size_t len)
{
    while (len--)
	*dest++ = *src ? *src++ : ' ';
}

/*
 * Print usage message.
 */
static void
usage(void)
{
    fprintf(stderr,
	    "usage: %s [ -options ] special [disktype]\n", getprogname());
    fprintf(stderr, "where the options are:\n");
    fprintf(stderr, "\t-N don't create file system: "
	    "just print out parameters\n");
    fprintf(stderr, "\t-B get bootstrap from file\n");
    fprintf(stderr, "\t-F FAT type (12, 16, or 32)\n");
    fprintf(stderr, "\t-I volume ID\n");
    fprintf(stderr, "\t-L volume label\n");
    fprintf(stderr, "\t-O OEM string\n");
    fprintf(stderr, "\t-S bytes/sector\n");
    fprintf(stderr, "\t-a sectors/FAT\n");
    fprintf(stderr, "\t-b block size\n");
    fprintf(stderr, "\t-c sectors/cluster\n");
    fprintf(stderr, "\t-e root directory entries\n");
    fprintf(stderr, "\t-f standard format\n");
    fprintf(stderr, "\t-h drive heads\n");
    fprintf(stderr, "\t-i file system info sector\n");
    fprintf(stderr, "\t-k backup boot sector\n");
    fprintf(stderr, "\t-m media descriptor\n");
    fprintf(stderr, "\t-n number of FATs\n");
    fprintf(stderr, "\t-o hidden sectors\n");
    fprintf(stderr, "\t-r reserved sectors\n");
    fprintf(stderr, "\t-s file system size (sectors)\n");
    fprintf(stderr, "\t-u sectors/track\n");
    exit(1);
}

void
infohandler(int sig)
{
    got_siginfo = 1;
}
