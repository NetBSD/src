/*	$NetBSD: conf.c,v 1.41.2.1 2001/09/13 01:14:19 thorpej Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)conf.c	8.2 (Berkeley) 11/14/93
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include "ccd.h"
#include "cd.h"
#include "md.h"
#include "raid.h"
#include "sd.h"
#include "st.h"
#include "vnd.h"
bdev_decl(ccd);
bdev_decl(cd);
bdev_decl(md);
bdev_decl(raid);
bdev_decl(sd);
bdev_decl(st);
bdev_decl(sw);
bdev_decl(vnd);
#include "audio.h"
cdev_decl(audio);

struct bdevsw	bdevsw[] =
{
	bdev_notdef(),			/* 0: nodev */
	bdev_notdef(),			/* 1: ULTRIX vax ht */
	bdev_disk_init(NVND,vnd),	/* 2: vnode disk driver */
	bdev_notdef(),			/* 3: ULTRIX vax rk */
	bdev_swap_init(1,sw),		/* 4: swap pseudo-device*/
	bdev_notdef(),			/* 5: ULTRIX vax tm */
	bdev_notdef(),			/* 6: ULTRIX vax ts */
	bdev_notdef(),			/* 7: ULTRIX vax mt */
	bdev_notdef(),			/* 8: ULTRIX vax tu */
	bdev_notdef(),			/* 9: nodev */
	bdev_notdef(),			/* 10: ULTRIX ut */
	bdev_notdef(),			/* 11: ULTRIX 11/730 idc */
	bdev_notdef(),			/* 12: ULTRIX rx */
	bdev_notdef(),			/* 13: ULTRIX uu */
	bdev_notdef(),			/* 14: ULTRIX rl */
	bdev_notdef(),			/* 15: ULTRIX tmscp */
	bdev_notdef(),			/* 16: ULTRIX cs */
	bdev_disk_init(NMD,md),		/* 17: memory disk driver */
	bdev_tape_init(NST,st),		/* 18: MI SCSI tape */
	bdev_disk_init(NSD,sd),		/* 19: MI SCSI disk */
	bdev_notdef(),			/* 20: ULTRIX tz tape */
	bdev_notdef(),			/* 21: ULTRIX rz disk & CD-ROM */
	bdev_notdef(),			/* 22: nodev */
	bdev_notdef(),			/* 23: ULTRIX mscp */

	bdev_disk_init(NCCD,ccd),	/* 24: concatenated disk driver */
	bdev_disk_init(NCD,cd),		/* 25: MI SCSI CD-ROM */

	bdev_lkm_dummy(),		/* 26 */
	bdev_lkm_dummy(),		/* 27 */
	bdev_lkm_dummy(),		/* 28 */
	bdev_lkm_dummy(),		/* 29 */
	bdev_lkm_dummy(),		/* 30 */
	bdev_lkm_dummy(),		/* 31 */

	bdev_disk_init(NRAID,raid),	/* 32: RAIDframe disk driver */

};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

/*
 * Swapdev is a fake block device implemented  in sw.c and only used
 * internally to get to swstrategy.  It cannot be provided to the
 * users, because the swstrategy routine munches the b_dev and b_blkno
 * entries before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines.   User access (e.g., for libkvm
 * and ps) is provided through the /dev/drum character (raw) device.
 */
dev_t	swapdev = makedev(4, 0);


cdev_decl(cn);
cdev_decl(sw);
cdev_decl(ctty);
#define	mmread	mmrw
#define	mmwrite	mmrw
dev_type_read(mmrw);
cdev_decl(mm);
#include "pty.h"
#define ptstty ptytty
#define ptsioctl ptyioctl
cdev_decl(pts);
#define ptctty ptytty
#define ptcioctl ptyioctl
cdev_decl(ptc);
cdev_decl(log);
#include "bpfilter.h"
#include "ch.h"
#include "ipfilter.h"
#include "rnd.h"
#include "ss.h"
#include "tun.h"
#include "uk.h"
#include "scsibus.h"
cdev_decl(bpf);
cdev_decl(ccd);
cdev_decl(cd);
cdev_decl(ch);
cdev_decl(fd);
dev_decl(filedesc,open);
cdev_decl(md);
cdev_decl(raid);
cdev_decl(scsibus);
cdev_decl(sd);
cdev_decl(ss);
cdev_decl(st);
cdev_decl(tun);
cdev_decl(uk);
cdev_decl(vnd);
cdev_decl(vnd);

#include "dc.h"
#include "dtop.h"
#include "fb.h"
#include "px.h"
#include "rasterconsole.h"
#include "scc.h"
cdev_decl(dc);
cdev_decl(dtop);
cdev_decl(fb);
cdev_decl(px);
cdev_decl(rcons);
cdev_decl(scc);

/* a framebuffer with an attached mouse: */

/* open, close, ioctl, poll, mmap */
#define	cdev_fbm_init(c,n)	cdev__ocipm_init(c,n)

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_swap_init(1,sw),		/* 1: /dev/drum (swap pseudo-device) */
	cdev_ctty_init(1,ctty),		/* 2: controlling terminal */
	cdev_mm_init(1,mm),		/* 3: /dev/{null,mem,kmem,...} */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_fd_init(1,filedesc),	/* 7: file descriptor pseudo-dev */
	cdev_notdef(),			/* 8: ULTRIX fl */
	cdev_disk_init(NSD,sd),		/* 9: MI SCSI disk */
	cdev_notdef(),			/* 10: ULTRIX tz tape */
	cdev_disk_init(NVND,vnd),	/* 11: vnode disk driver */
	cdev_bpftun_init(NBPFILTER,bpf),/* 12: Berkeley packet filter */
	cdev_notdef(),			/* 13: ULTRIX up */
	cdev_notdef(),			/* 14: ULTRIX tm */
	cdev_tty_init(NDTOP,dtop),	/* 15: desktop bus interface */
	cdev_tty_init(NDC,dc),		/* 16: dc7085 serial interface */
	cdev_tty_init(NSCC,scc),	/* 17: scc 82530 serial interface */
	cdev_notdef(),			/* 18: ULTRIX ct */
	cdev_notdef(),			/* 19: ULTRIX mt */
	cdev_tty_init(NPTY,pts),	/* 20: ULTRIX pty slave	 */
	cdev_ptc_init(NPTY,ptc),	/* 21: ULTRIX pty master */
	cdev_notdef(),			/* 22: ULTRIX dmf */
	cdev_notdef(),			/* 23: ULTRIX vax 730 idc */
	cdev_notdef(),			/* 24: ULTRIX dn-11 */

		/* 25-28 CSRG reserved to local sites, DEC sez: */
	cdev_notdef(),		/* 25: ULTRIX gpib */
	cdev_notdef(),		/* 26: ULTRIX lpa */
	cdev_notdef(),		/* 27: ULTRIX ps */
	cdev_notdef(),		/* 28: ULTRIX ib */
	cdev_notdef(),		/* 29: ULTRIX ad */
	cdev_notdef(),		/* 30: ULTRIX rx */
	cdev_notdef(),		/* 31: ULTRIX ik */
	cdev_notdef(),		/* 32: ULTRIX rl-11 */
	cdev_notdef(),		/* 33: ULTRIX dhu/dhv */
	cdev_notdef(),		/* 34: ULTRIX vax Able dmz, mips dc */
	cdev_notdef(),		/* 35: ULTRIX qv */
	cdev_notdef(),		/* 36: ULTRIX tmscp */
	cdev_notdef(),		/* 37: ULTRIX vs */
	cdev_notdef(),		/* 38: ULTRIX vax cn console */
	cdev_notdef(),		/* 39: ULTRIX lta */
	cdev_notdef(),		/* 40: ULTRIX crl (Venus, aka 8600 aka 11/790 console RL02) */
	cdev_notdef(),		/* 41: ULTRIX cs */
	cdev_notdef(),		/* 42: ULTRIX qd, Qdss, vcb02 */
	cdev_mm_init(1,mm),	/* 43: ULTRIX errlog (VMS-lookalike puke) */
	cdev_notdef(),		/* 44: ULTRIX dmb */
	cdev_notdef(),		/* 45: ULTRIX vax ss, mips scc */
	cdev_tape_init(NST,st),	/* 46: MI SCSI tape */
	cdev_disk_init(NCD,cd),	/* 47: MI SCSI CD-ROM */
	cdev_notdef(),		/* 48: ULTRIX /dev/trace */
	cdev_notdef(),		/* 49: ULTRIX sm (sysV shm?) */
	cdev_notdef(),		/* 50: ULTRIX sg */
	cdev_notdef(),		/* 51: ULTRIX sh tty */
	cdev_notdef(),		/* 52: ULTRIX its */
	cdev_scanner_init(NSS,ss),	/* 53: MI SCSI scanner */
	cdev_ch_init(NCH,ch),	/* 54: MI SCSI autochanger */
	cdev_uk_init(NUK,uk),	/* 55: MI SCSI unknown */
	cdev_notdef(),		/* 56: ULTRIX rz SCSI, gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 57: nodev */
	cdev_notdef(),		/* 58: ULTRIX fc */
	cdev_notdef(),		/* 59: ULTRIX fg, again gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 60: ULTRIX mscp, again gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 61: ULTRIX mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 62: ULTRIX mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 63: ULTRIX mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 64: ULTRIX mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 65: ULTRIX mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 66: ULTRIX mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 67: ULTRIX mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 68: ULTRIX ld */
	cdev_notdef(),		/* 69: ULTRIX /dev/audit */
	cdev_notdef(),		/* 70: ULTRIX Mogul (nee' CMU) packetfilter */
	cdev_notdef(),		/* 71: ULTRIX xcons, mips /dev/xcons virtual console nightmare */
	cdev_notdef(),		/* 72: ULTRIX xa */
	cdev_notdef(),		/* 73: ULTRIX utx */
	cdev_notdef(),		/* 74: ULTRIX sp */
	cdev_notdef(),		/* 75: ULTRIX pr PrestoServe NVRAM pseudo-device control device */
	cdev_notdef(),		/* 76: ULTRIX disk shadowing */
	cdev_notdef(),		/* 77: ULTRIX ek */
	cdev_notdef(),		/* 78: ULTRIX msdup */
	cdev_notdef(),		/* 79: ULTRIX multimedia audio A */
	cdev_notdef(),		/* 80: ULTRIX multimedia audio B */
	cdev_notdef(),		/* 81: ULTRIX multimedia video in */
	cdev_notdef(),		/* 82: ULTRIX multimedia video out */
	cdev_notdef(),		/* 83: ULTRIX fd */
	cdev_notdef(),		/* 84: ULTRIX DTi */
	cdev_tty_init(NRASTERCONSOLE,rcons), /* 85: rcons pseudo-dev */
	cdev_fbm_init(NFB,fb),	/* 86: frame buffer pseudo-device */
	cdev_disk_init(NCCD,ccd),	/* 87: concatenated disk driver */
	cdev_notdef(),		/* 88: reserved for wscons fb */
	cdev_notdef(),		/* 89: reserved for wscons kbd */
	cdev_notdef(),		/* 90: reserved for wscons mouse */
	cdev_ipf_init(NIPFILTER,ipl),	/* 91: ip-filter device */
	cdev_rnd_init(NRND,rnd),	/* 92: random source pseudo-device */
	cdev_bpftun_init(NTUN,tun),	/* 93: network tunnel */
	cdev_lkm_init(NLKM,lkm),	/* 94: loadable module driver */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 95: SCSI bus */
	cdev_disk_init(NRAID,raid),	/* 96: RAIDframe disk driver */
	cdev_disk_init(NMD,md),	/* 97: memory disk  driver */
	cdev_fbm_init(NPX,px),	/* 98: PixelStamp board driver */
	cdev_audio_init(NAUDIO,audio),  /* 99: generic audio I/O */
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int	mem_no = 2; 	/* major device number of memory special file */

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 */
int
iskmemdev(dev)
	dev_t dev;
{

#ifdef COMPAT_BSD44
	return (major(dev) == 2 && minor(dev) < 2);
#else
	return (major(dev) == 3 && minor(dev) < 2);
#endif
}

/*
 * Returns true if dev is /dev/zero.
 */
int
iszerodev(dev)
	dev_t dev;
{

#ifdef COMPAT_BSD44
	return (major(dev) == 2 && minor(dev) == 12);
#else
	return (major(dev) == 3 && minor(dev) == 12);
#endif
}

static int chrtoblktbl[] =  {
	/* XXXX This needs to be dynamic for LKMs. */
	/*VCHR*/	/*VBLK*/
	/*  0 */	NODEV,
	/*  1 */	NODEV,
	/*  2 */	NODEV,
	/*  3 */	NODEV,
	/*  4 */	NODEV,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	NODEV,
	/*  8 */	NODEV,
	/*  9 */	19,		/* MI SCSI disk */
	/* 10 */	20,		/* ULTRIX tz */
	/* 11 */	2,		/* vnode disk */
	/* 12 */	NODEV,
	/* 13 */	NODEV,
	/* 14 */	NODEV,
	/* 15 */	NODEV,
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	NODEV,
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
	/* 22 */	NODEV,
	/* 23 */	NODEV,
	/* 24 */	NODEV,
	/* 25 */	NODEV,
	/* 26 */	NODEV,
	/* 27 */	NODEV,
	/* 28 */	NODEV,
	/* 29 */	NODEV,
	/* 30 */	NODEV,
	/* 31 */	NODEV,
	/* 32 */	NODEV,
	/* 33 */	NODEV,
	/* 34 */	NODEV,
	/* 35 */	NODEV,
	/* 36 */	NODEV,
	/* 37 */	NODEV,
	/* 38 */	NODEV,
	/* 39 */	NODEV,
	/* 40 */	NODEV,
	/* 41 */	NODEV,
	/* 42 */	NODEV,
	/* 43 */	NODEV,
	/* 44 */	NODEV,
	/* 45 */	NODEV,
	/* 46 */	18,		/* MI SCSI tape */
	/* 47 */	25,		/* MI SCSI CD-ROM */
	/* 48 */	NODEV,
	/* 49 */	NODEV,
	/* 50 */	NODEV,
	/* 51 */	NODEV,
	/* 52 */	NODEV,
	/* 53 */	NODEV,
	/* 54 */	NODEV,
	/* 55 */	NODEV,
	/* 56 */	21,		/* ULTRIX rz */
	/* 57 */	NODEV,
	/* 58 */	NODEV,
	/* 59 */	NODEV,
	/* 60 */	NODEV,
	/* 61 */	NODEV,
	/* 62 */	NODEV,
	/* 63 */	NODEV,
	/* 64 */	NODEV,
	/* 65 */	NODEV,
	/* 66 */	NODEV,
	/* 67 */	NODEV,
	/* 68 */	NODEV,
	/* 69 */	NODEV,
	/* 70 */	NODEV,
	/* 71 */	NODEV,
	/* 72 */	NODEV,
	/* 73 */	NODEV,
	/* 74 */	NODEV,
	/* 75 */	NODEV,
	/* 76 */	NODEV,
	/* 77 */	NODEV,
	/* 78 */	NODEV,
	/* 79 */	NODEV,
	/* 80 */	NODEV,
	/* 81 */	NODEV,
	/* 82 */	NODEV,
	/* 83 */	NODEV,
	/* 84 */	NODEV,
	/* 85 */	NODEV,
	/* 86 */	NODEV,
	/* 87 */	24,		/* concatenated disk */
	/* 88 */	NODEV,
	/* 89 */	NODEV,
	/* 90 */	NODEV,
	/* 91 */	NODEV,
	/* 92 */	NODEV,
	/* 93 */	NODEV,
	/* 94 */	NODEV,
	/* 95 */	NODEV,
	/* 96 */	32,		/* RAIDframe */
	/* 97 */	17,		/* memory disk */
	/* 98 */	NODEV,
	/* 99 */	NODEV,
};

/*
 * Routine to convert from character to block device number.
 */
dev_t
chrtoblk(dev)
	dev_t dev;
{
	int blkmaj;

	if (major(dev) >= nchrdev)
		return (NODEV);
	blkmaj = chrtoblktbl[major(dev)];
	if (blkmaj == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}
