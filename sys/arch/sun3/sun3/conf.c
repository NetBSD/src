/*	$NetBSD: conf.c,v 1.30 1995/01/25 04:48:27 cgd Exp $	*/

/*-
 * Copyright (c) 1994 Adam Glass, Gordon W. Ross
 * Copyright (c) 1982, 1986, 1989, 1991, 1992, 1993
 *  The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)conf.c	7.9 (Berkeley) 5/28/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/vnode.h>

int	rawread		__P((dev_t, struct uio *, int));
int	rawwrite	__P((dev_t, struct uio *, int));
void	swstrategy	__P((struct buf *));
int	ttselect	__P((dev_t, int, struct proc *));

/*
 * macros for function declarations
 */
#define decl_open(f)     int f __P((dev_t, int, int, struct proc *))
#define decl_close(f)    int f __P((dev_t, int, int, struct proc *))
#define decl_read(f)     int f __P((dev_t, struct uio *, int))
#define decl_write(f)    int f __P((dev_t, struct uio *, int))
#define decl_ioctl(f)    int f __P((dev_t dev, u_long cmd, caddr_t data, \
				     int fflag, struct proc *p))
#define	decl_stop(f)     int f __P((struct tty *, int))
#define	decl_reset(f)    int f __P((int))
#define	decl_select(f)   int f __P((dev_t, int, struct proc*))
#define decl_mmap(f)     int f __P((/*dev_t, int, int*/))
#define	decl_strategy(f) void f __P((struct buf *))
#define	decl_dump(f)     int f ()
#define	decl_psize(f)    int f __P((dev_t))


/*
 * macros for functions that do nothing (not NULL pointers)
 */
#define	null_open     (decl_open((*)))    nullop
#define	null_close    (decl_close((*)))   nullop
#define	null_read     (decl_read((*)))    nullop
#define	null_write    (decl_write((*)))   nullop
#define	null_stop     (decl_stop((*)))    nullop
#define	null_reset    (decl_reset((*)))   nullop


/*
 * macros for functions not supported by a device
 */
#define	nsup_open     (decl_open((*)))    enodev
#define	nsup_close    (decl_close((*)))   enodev
#define	nsup_read     (decl_read((*)))    enodev
#define	nsup_write    (decl_write((*)))   enodev
#define nsup_ioctl    (decl_ioctl((*)))   enoioctl
#define	nsup_stop     (decl_stop((*)))    enodev
#define	nsup_reset    (decl_reset((*)))   enodev
#define	nsup_ttys     (struct tty**) 0
#define	nsup_select   (decl_select((*)))  enodev
#define	nsup_mmap     (decl_mmap((*)))     0
#define	nsup_strategy (decl_strategy((*))) 0
#define	nsup_dump     (decl_dump((*)))    enodev
#define	nsup_psize    (decl_psize((*)))    0


/*
 * macros for unconfigured device switch records
 */
#define	ndef_open     (decl_open((*)))    enxio
#define	ndef_close    (decl_close((*)))   enxio
#define	ndef_read     (decl_read((*)))    enxio
#define	ndef_write    (decl_write((*)))   enxio
#define ndef_ioctl    (decl_ioctl((*)))   enxio
#define	ndef_stop     (decl_stop((*)))    enxio
#define	ndef_reset    (decl_reset((*)))   enxio
#define	ndef_ttys     (struct tty**) 0
#define	ndef_select   (decl_select((*)))  enxio
#define	ndef_mmap     (decl_mmap((*)))     0
#define	ndef_strategy (decl_strategy((*))) 0
#define	ndef_dump     (decl_dump((*)))    enxio
#define	ndef_psize    (decl_psize((*)))    0


/*
 * Block device declarations
 */

/* device not configured */
#define	bdev_notdef { \
	ndef_open, \
	ndef_close, \
	ndef_strategy, \
	ndef_ioctl, \
	ndef_dump, \
	ndef_psize, \
	0 }

/* scsi disk */
#include "sd.h"
#if NSD > 0
decl_open(sdopen);
decl_close(sdclose);
decl_strategy(sdstrategy);
decl_ioctl(sdioctl);
decl_dump(sddump);
decl_psize(sdsize);
#else
#define	sdopen		ndef_open
#define	sdclose		ndef_close
#define	sdstrategy	ndef_strategy
#define	sdioctl		ndef_ioctl
#define	sddump		ndef_dump
#define	sdsize		ndef_psize
#endif

/* scsi tape */
#include "st.h"
#if NST > 0
decl_open(stopen);
decl_close(stclose);
decl_strategy(ststrategy);
decl_ioctl(stioctl);
decl_dump(stdump);
decl_psize(stsize);
#else
#define	stopen		ndef_open
#define	stclose		ndef_close
#define	ststrategy	ndef_strategy
#define	stioctl		ndef_ioctl
#define	stdump		ndef_dump
#endif

/* scsi cd-rom */
#include "cd.h"
#if NCD > 0
decl_open(cdopen);
decl_close(cdclose);
decl_strategy(cdstrategy);
decl_ioctl(cdioctl);
decl_dump(cddump);
decl_psize(cdsize);
#else
#define	cdopen		ndef_open
#define	cdclose		ndef_close
#define	cdstrategy	ndef_strategy
#define	cdioctl		ndef_ioctl
#define	cdsize		ndef_psize
#endif

#include "ch.h"
#if NCH > 0
decl_open(chopen);
decl_close(chclose);
decl_ioctl(chioctl);
#else
#define	chopen		ndef_open
#define	chclose		ndef_close
#define	chioctl		ndef_ioctl
#endif

#define	NVND 0 /* #include "vn.h" XXX */
#if NVND > 0
decl_open(vndopen);
decl_close(vndclose);
decl_strategy(vndstrategy);
decl_ioctl(vndioctl);
decl_dump(vnddump);
decl_psize(vndsize);
decl_read(vndread);
decl_write(vndwrite);
#else
#define	vndopen		ndef_open
#define	vndclose	ndef_close
#define	vndread		ndef_read
#define	vndwrite	ndef_write
#define	vndstrategy	ndef_strategy
#define	vndioctl	ndef_ioctl
#define	vnddump		ndef_dump
#define	vnsize		ndef_psize
#endif

#ifdef LKM
/* lkm interface routines */
decl_open(lkmopen);
decl_close(lkmclose);
decl_ioctl(lkmioctl);
/* lkm "nodev" routine */
decl_open(lkmenodev);
#else
#define	lkmopen		ndef_open
#define	lkmclose	ndef_close
#define	lkmioctl	ndef_ioctl
#define	lkmenodev	nsup_open
#endif
/* an easy way to define LKM devices */
#define	LKM_BDEV { \
	lkmenodev, \
	nsup_close, \
	nsup_strategy, \
	nsup_ioctl, \
	nsup_dump, \
	nsup_psize, \
	0 }
#define	LKM_CDEV { \
	lkmenodev, \
	nsup_close, \
	nsup_read, \
	nsup_write, \
	nsup_ioctl, \
	nsup_stop, \
	nsup_reset, \
	nsup_ttys, \
	nsup_select, \
	nsup_mmap, \
	nsup_strategy }

/*
 * Block device switch
 */

struct bdevsw	bdevsw[] =
{
	bdev_notdef,	/*  0 */
	bdev_notdef,	/*  1: /dev/mt (tapemaster tape) */
	bdev_notdef,	/*  2 */

	/*  3: XyLogics Disk (/dev/xy*) */
	bdev_notdef,

	/* BLK Major number of internal swap device. */
#define	SWAP_BMAJ 4
	{	/*  4: internal swap device */
		nsup_open,
		nsup_close,
		swstrategy,
		nsup_ioctl,
		nsup_dump,
		nsup_psize,
		0 },

	bdev_notdef,	/*  5 */
	bdev_notdef,	/*  6 */

	/*  7: /dev/sd* (SCSI disk) */
	{ sdopen, sdclose, sdstrategy, sdioctl, sddump, sdsize, 0 },

	bdev_notdef,	/*  8: /dev/xt* (do we need block tapes?) */

	bdev_notdef,	/*  9 */

	/* 10: /dev/xd* (Xylogics 7053 SMD Disk controller) */
	bdev_notdef,

	bdev_notdef,	/*  11: /dev/st* (do we need block tapes?) */
	bdev_notdef,	/*  12: Sun ns? */

	/*  13: /dev/rd* (RAM disk - for install tape) */
	bdev_notdef,

	bdev_notdef,	/*  14: Sun ft? */
	bdev_notdef,	/*  15: Sun hd? */

	bdev_notdef,	/*  16: Sun fd? */
	bdev_notdef,	/*  17: Sun vd_unused */
	bdev_notdef,	/*  18: /dev/sr* (SCSI CD-ROM) */

	bdev_notdef,	/*  19: Sun vd_unused */
	bdev_notdef,	/*  20: Sun vd_unused */
	bdev_notdef,	/*  21: Sun vd_unused */
	bdev_notdef,	/*  22: Sun IPI disks... */
	bdev_notdef,	/*  23: Sun IPI disks... */
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

/*
 * Corresponding CHR major numbers for BLK devices above.
 * Note that isdisk() assumes non-zero entries are disks.
 */
static int blktochrtbl[] = {
	0,	/*  0 */
	0,	/*  1 */
	0,	/*  2 */
	9,	/*  3: /dev/xy */
	0,	/*  4 */
	0,	/*  5 */
	0,	/*  6 */
	17,	/*  7: /dev/sd */
	0,	/*  8 */
	0,	/*  9 */
	42,	/* 10: /dev/xd */
	0,	/* 11 */
	0,	/* 12 */
	52,	/* 13: /dev/rd */
	0,	/* 14 */
	0,	/* 15 */
};
static int nblktochr = sizeof(blktochrtbl) / sizeof(blktochrtbl[0]);

/*
 * Character device declarations
 */

/* device not configured */
#define	cdev_notdef { \
	ndef_open, \
	ndef_close, \
	ndef_read, \
	ndef_write, \
	ndef_ioctl, \
	ndef_stop, \
	ndef_reset, \
	ndef_ttys, \
	ndef_select, \
	ndef_mmap, \
	ndef_strategy }

/* virtual console */
decl_open(cnopen);
decl_close(cnclose);
decl_read(cnread);
decl_write(cnwrite);
decl_ioctl(cnioctl);
decl_select(cnselect);

/* keyboard/display tty (internal, for console) */
#include "zs.h"
#if NZS > 1
decl_open(kdopen);
decl_close(kdclose);
decl_read(kdread);
decl_write(kdwrite);
decl_ioctl(kdioctl);
extern	struct tty *kd_tty[];
#else
#define	kdopen		ndef_open
#define	kdclose		ndef_close
#define	kdread		ndef_read
#define	kdwrite		ndef_write
#define	kdioctl		ndef_ioctl
#define	kd_tty		ndef_ttys
#endif

/* Controlling tty (/dev/tty) -- XXX should be a tty */
decl_open(cttyopen);
decl_read(cttyread);
decl_write(cttywrite);
decl_ioctl(cttyioctl);
decl_select(cttyselect);

/* Memory devices (/dev/{mem,kmem,null,...}) */
decl_read(mmrw);
decl_mmap(mmmap);

/* Swap pseudo-device (/dev/drum) */
decl_read(rawread);
decl_write(rawwrite);

/* File descriptor pseudo device. */
decl_open(fdopen);

/* Kernel message log (/dev/klog) -- XXX should be a generic device */
decl_open(logopen);
decl_close(logclose);
decl_read(logread);
decl_ioctl(logioctl);
decl_select(logselect);

/* PROM tty (internal, for console) */
#include "prom.h"
#if NPROM > 0
decl_open(promopen);
decl_close(promclose);
decl_read(promread);
decl_write(promwrite);
decl_ioctl(promioctl);
#else
#define	promopen		ndef_open
#define	promclose		ndef_close
#define	promread		ndef_read
#define	promwrite		ndef_write
#define	promioctl		ndef_ioctl
#endif

/* Zilog Zerial ports (/dev/tty{a,b}) */
/* #include "zs.h" (above) */
#if NZS > 0
decl_open(zsopen);
decl_close(zsclose);
decl_read(zsread);
decl_write(zswrite);
decl_ioctl(zsioctl);
decl_stop(zsstop);
extern struct tty *zs_tty[];
#else
#define	zsopen		ndef_open
#define	zsclose		ndef_close
#define	zsread		ndef_read
#define	zswrite		ndef_write
#define	zsioctl		ndef_ioctl
#define	zsstop		ndef_stop
#define	zs_tty		ndef_ttys
#endif
#if NZS > 1
/* Mouse pseudo-device (/dev/mouse) */
decl_open(msopen);
decl_close(msclose);
decl_read(msread);
decl_ioctl(msioctl);
decl_select(msselect);
/* Keyboard pseudo-device (/dev/kbd) */
decl_open(kbdopen);
decl_close(kbdclose);
decl_read(kbdread);
decl_ioctl(kbdioctl);
decl_select(kbdselect);
#else
#define msopen		ndef_open
#define msclose		ndef_close
#define msread		ndef_read
#define msioctl		ndef_ioctl
#define msselect	ndef_select
#define kbdopen		ndef_open
#define kbdclose	ndef_close
#define kbdread		ndef_read
#define kbdioctl	ndef_ioctl
#define kbdselect	ndef_select
#endif

/* Pseudo-terminals */
#include "pty.h"
#if NPTY > 0
decl_open(ptsopen);
decl_close(ptsclose);
decl_read(ptsread);
decl_write(ptswrite);
decl_stop(ptsstop);
decl_open(ptcopen);
decl_close(ptcclose);
decl_read(ptcread);
decl_write(ptcwrite);
decl_select(ptcselect);
decl_ioctl(ptyioctl);
struct	tty *pt_tty[];
#else
#define	ptsopen		ndef_open
#define	ptsclose	ndef_close
#define	ptsread		ndef_read
#define	ptswrite	ndef_write
#define	ptcopen		ndef_open
#define	ptcclose	ndef_close
#define	ptcread		ndef_read
#define	ptcwrite	ndef_write
#define	ptyioctl	ndef_ioctl
#define	pt_tty		ndef_ttys
#define	ptcselect	ndef_select
#define	ptsstop		ndef_stop
#endif

/* Berkeley Packet Filter -- XXX should be generic device */
#include "bpfilter.h"
#if NBPFILTER > 0
decl_open(bpfopen);
decl_close(bpfclose);
decl_read(bpfread);
decl_write(bpfwrite);
decl_ioctl(bpfioctl);
decl_select(bpfselect);
#else
#define	bpfopen		ndef_open
#define	bpfclose	ndef_close
#define	bpfread		ndef_read
#define	bpfwrite	ndef_write
#define	bpfioctl	ndef_ioctl
#define	bpfselect	ndef_select
#endif

struct cdevsw	cdevsw[] =
{
	/*  0: virtual console (/dev/console) -- XXX should be a tty */
	{	cnopen, cnclose, cnread, cnwrite,
		cnioctl, null_stop, null_reset, nsup_ttys,
		cnselect, nsup_mmap, nsup_strategy },

	/*  1: keyboard/display (sun wc) */
	{	kdopen, kdclose, kdread, kdwrite,
		kdioctl, null_stop, null_reset, kd_tty,
		ttselect, nsup_mmap, nsup_strategy },

	/*  2: controlling terminal */
	{	cttyopen, null_close, cttyread, cttywrite,
		cttyioctl, null_stop, null_reset, nsup_ttys,
		cttyselect, nsup_mmap, nsup_strategy },

	/* CHR Major number of /dev/mem */
#define	MEM_CMAJ	3
	/*  3: /dev/{mem,kmem,null,...} */
	{	null_open, null_close, mmrw, mmrw,
		nsup_ioctl, null_stop, null_reset, nsup_ttys,
		seltrue, mmmap, nsup_strategy },

	/*  4: PROM console (old sun ip) */
	{	promopen, promclose, promread, promwrite,
		promioctl, null_stop, null_reset, nsup_ttys,
		ttselect, nsup_mmap, nsup_strategy },

	/*  5: /dev/mt (tapemaster tape) */
	cdev_notdef,

	/*  6: /dev/vp (systech/versatec) */
	cdev_notdef,

	/*  7: /dev/drum (swap pseudo-device) */
	{	null_open, null_close, rawread, rawwrite,
		nsup_ioctl, null_stop, null_reset, nsup_ttys,
		nsup_select, nsup_mmap, swstrategy },

	/*  8: /dev/ar (Archive QIC-11 tape) */
	cdev_notdef,

	/*  9: /dev/xy (Xylogics 450) */
	cdev_notdef,

	/* 10: (systech multi-terminal board) */
	cdev_notdef,

	/* 11: (DES encryption chip) */
	cdev_notdef,

	/* 12: /dev/tty{a,b} (zs serial) */
	{	zsopen, zsclose, zsread, zswrite,
		zsioctl, zsstop, null_reset, zs_tty,
		ttselect, nsup_mmap, nsup_strategy },

	/* 13: /dev/mouse */
	{	msopen, msclose, msread, nsup_write,
		msioctl, null_stop, null_reset, nsup_ttys,
		msselect, nsup_mmap, nsup_strategy },

	/* 14: old sun cgone */
	cdev_notdef,

	/* 15: sun /dev/winXXX */
	cdev_notdef,

	/* 16: /dev/klog */
	{	logopen, logclose, logread, nsup_write,
		logioctl, null_stop, null_reset, nsup_ttys,
		logselect, nsup_mmap, nsup_strategy },

	/* 17: /dev/sd* (SCSI disk) */
	{	sdopen, sdclose, rawread, rawwrite,
		sdioctl, null_stop, null_reset, nsup_ttys,
		seltrue, nsup_mmap, sdstrategy },

	/* 18: scsi tape */
	{	stopen, stclose, rawread, rawwrite,
		stioctl, null_stop, null_reset, nsup_ttys,
		seltrue, nsup_mmap, ststrategy },

	/* 19: old sun nd (network disk protocol - unused) */
	cdev_notdef,

	/* 20: pseudo-tty slave */
	{	ptsopen, ptsclose, ptsread, ptswrite,
		ptyioctl, ptsstop, null_reset, pt_tty,
		ttselect, nsup_mmap, nsup_strategy },

	/* 21: pseudo-tty master */
	{	ptcopen, ptcclose, ptcread, ptcwrite,
		ptyioctl, null_stop, null_reset, pt_tty,
		ptcselect, nsup_mmap, nsup_strategy },

	/* 22: /dev/fb indirect driver */
	cdev_notdef,

	/* 23: old sun ropc (unused) */
	/* File descriptors (/dev/std{in,out,err}) */
	{	fdopen, null_close, nsup_read, nsup_write,
		nsup_ioctl, null_stop, null_reset, nsup_ttys,
		nsup_select, nsup_mmap, nsup_strategy },

	/* 24: /dev/sky (Sky FPA) */
	cdev_notdef,

	/* 25: sun pi? */
	cdev_notdef,

	/* 26: old sun bwone (unused) */
	cdev_notdef,

 	/* 27: /dev/bwtwo */
	cdev_notdef,

	/* 28: /dev/vpc (Systech VPC-2200 versatec/centronics) */
	cdev_notdef,

	/* 29: /dev/kbd */
	{	kbdopen, kbdclose, kbdread, nsup_write,
		kbdioctl, null_stop, null_reset, nsup_ttys,
		kbdselect, nsup_mmap, nsup_strategy },

	/* 30: /dev/xt (Xylogics 472 tape controller) */
	cdev_notdef,

	/* 31: /dev/cgtwo */
	cdev_notdef,

	/* 32: /dev/gpone */
	cdev_notdef,

	/* 33: (unused) */
	cdev_notdef,

	/* 34: /dev/fpa (Floating Point Accelerator) */
	cdev_notdef,

	/* 35: (sp) */
	cdev_notdef,

	/* 36: (unused) */
	/* Berkeley Packet Filter (old sun ip) */
	{	bpfopen, bpfclose, bpfread, bpfwrite,
		bpfioctl, null_stop, null_reset, nsup_ttys,
		bpfselect, nsup_mmap, nsup_strategy },

	/* 37: (clone device) */
	cdev_notdef,

	/* 38: (pc) */
	cdev_notdef,

	/* 39: Sun cg4 board */
	cdev_notdef,

	cdev_notdef,	/* 40: (sni) */
	cdev_notdef,	/* 41: (sun dump) */

	/* 42: /dev/xd* (Xylogics 7053 SMD Disk controller) */
	cdev_notdef,

	cdev_notdef,	/* 43: (sun hrc) */
	cdev_notdef,	/* 44: (mcp) */
	cdev_notdef,	/* 45: (sun ifd) */
	cdev_notdef,	/* 46: (dcp) */
	cdev_notdef,	/* 47: (dna) */
	cdev_notdef,	/* 48: (tbi) */
	cdev_notdef,	/* 49: (chat) */
	cdev_notdef,	/* 50: (chut) */
	cdev_notdef,	/* 51: (chut) */
	cdev_notdef,	/* 52: /dev/rd* (RAM disk) */
	cdev_notdef,	/* 53: (hd - N/A) */
	cdev_notdef,	/* 54: (fd - N/A) */

	/* 55: /dev/cgthree */
	cdev_notdef,

	cdev_notdef,	/* 56: (pp) */
	cdev_notdef,	/* 57: (vd) Loadable Kernel Module control */
	cdev_notdef,	/* 58 /dev/sr* (SCSI CD-ROM) */
	cdev_notdef,	/* 59: (vd) Loadable Kernel Module stub */
	cdev_notdef,	/* 60:    ||      ||     ||    ||  */
	cdev_notdef,	/* 61:    ||      ||     ||    ||  */

	cdev_notdef,	/* 62: (taac) */
	cdev_notdef,	/* 63: (tcp/tli) */

	/* 64: /dev/cgeight */
	cdev_notdef,

	cdev_notdef,	/* 65: old IPI */
	cdev_notdef,	/* 66: (mcp) parallel printer */

	/* 67: /dev/cgsix */
	cdev_notdef,

	/* 68: /dev/cgnine */
	cdev_notdef,

	cdev_notdef,	/* 69: /dev/audio */
	cdev_notdef,	/* 70: open prom */
	cdev_notdef,	/* 71: (sg?) */

};
int nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

/*
 * Swapdev is a fake device implemented in vm/vm_swap.c
 * and used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(SWAP_BMAJ, 0);

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
iskmemdev(dev)
	dev_t dev;
{
	return ((major(dev) == MEM_CMAJ) && (minor(dev) < 2));
}

/*
 * Returns true if dev is /dev/zero.
 */
iszerodev(dev)
	dev_t dev;
{
	return ((major(dev) == MEM_CMAJ) && (minor(dev) == 12));
}

/*
 * Convert a character device number to a block device number.
 */
static int *chrtoblktbl;
dev_t chrtoblk(dev)
	dev_t dev;
{
	int maj = major(dev);

#ifdef	DIAGNOSTIC
	if (!chrtoblktbl)
		panic("chrtoblk: conf_init not done");
#endif

	if (maj < 0 || maj >= nchrdev)
		return (NODEV);
	maj = chrtoblktbl[maj];
	if (maj == NODEV)
		return (NODEV);
	return (makedev(maj, minor(dev)));
}

/*
 * Returns true if dev is a disk device.  For now at least,
 * all non-zero entries in blktochrtbl are disks.
 */
isdisk(dev, type)
	dev_t dev;
	int type;
{
	int maj = major(dev);

#ifdef	DIAGNOSTIC
	if (!chrtoblktbl)
		panic("chrtoblk: conf_init not done");
#endif

	if (type == VCHR) {
		/* Convert to BLK major number. */
		if (maj < 0 || maj >= nchrdev)
			return (0);
		maj = chrtoblktbl[maj];
	}
	/* Now have a BLK major number. */
	if (maj < 0 || maj >= nblktochr)
		return(0);

	return (blktochrtbl[maj]);
}

/*
 * Build chrtoblktbl from blktochrtbl
 */
conf_init()
{
	int b, c;

	chrtoblktbl = malloc(nchrdev * sizeof(int), M_DEVBUF, M_NOWAIT);
	if (!chrtoblktbl)
		panic("conf_init: malloc");

	/* Clear the CHR to BLK table. */
	for (c = 0; c < nchrdev; c++)
		chrtoblktbl[c] = NODEV;

	/* Set CHR dev slots with corresponging BLK devices. */
	for (b = 0; b < nblktochr; b++) {
		c = blktochrtbl[b];
		if (c > 0 && c < nchrdev) {
			chrtoblktbl[c] = b;
		}
	}
}

/*
 * The constab is the console configuration for this type of machine.
 * This entire table could be autoconfig()ed but that would mean that
 * the kernel's idea of the console could be out of sync with that of
 * the standalone boot.  I think it best that they both use the same
 * known algorithm unless we see a pressing need otherwise.
 */
#include <dev/cons.h>

#if NZS > 1
int kdcnprobe(), kdcninit(), kdcngetc(), kdcnputc();
#endif
#if NPROM > 0
int promcnprobe(), promcninit(), promcngetc(), promcnputc();
#endif
#if NZS > 0
int zscnprobe_a(), zscnprobe_b(), zscninit(), zscngetc(), zscnputc();
#endif

extern void nullcnpollc();

struct	consdev constab[] = {
#if NZS > 1
	{ kdcnprobe, kdcninit, kdcngetc, kdcnputc, nullcnpollc },
#endif
#if	NZS
	{ zscnprobe_a, zscninit, zscngetc, zscnputc, nullcnpollc },
#endif
#if	NZS
	{ zscnprobe_b, zscninit, zscngetc, zscnputc, nullcnpollc },
#endif
#if NPROM
	{ promcnprobe, promcninit, promcngetc, promcnputc, nullcnpollc },
#endif
    { 0 }	/* End marker. */
};
