/*	$NetBSD: conf.c,v 1.29.2.3 1998/10/16 04:10:09 nisimura Exp $	*/

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
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>


bdev_decl(sw);
#include "sd.h"
bdev_decl(sd);
#include "st.h"
bdev_decl(st);
#include "cd.h"
bdev_decl(cd);
#include "ccd.h"
bdev_decl(ccd);
#include "vnd.h"
bdev_decl(vnd);

struct bdevsw	bdevsw[] =
{
	bdev_notdef(),			/* 0: nodev */
	bdev_notdef(),			/* 1: ULTRIX vax ht */
	bdev_disk_init(NVND,vnd),	/* 2: vnode disk driver */
	bdev_notdef(),			/* 3: ULTRIX vax rk*/
	bdev_swap_init(1,sw),		/* 4: swap pseudo-device */
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
	bdev_notdef(),			/* 17: md */
	bdev_tape_init(NST,st),		/* 18: SCSI tape */
	bdev_disk_init(NSD,sd),		/* 19: SCSI disk */
	bdev_notdef(),			/* 20: ULTRIX tz */
	bdev_notdef(),			/* 21: ULTRIX rz */
	bdev_notdef(),			/* 22: nodev */
	bdev_notdef(),			/* 23: ULTRIX mscp */

	bdev_disk_init(NCCD,ccd),	/* 24: concatenated disk driver */
	bdev_disk_init(NCD,cd),		/* 25: SCSI CD-ROM */

	bdev_lkm_dummy(),		/* 26 */
	bdev_lkm_dummy(),		/* 27 */
	bdev_lkm_dummy(),		/* 28 */
	bdev_lkm_dummy(),		/* 29 */
	bdev_lkm_dummy(),		/* 30 */
	bdev_lkm_dummy(),		/* 31 */

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

#include "bpfilter.h"
cdev_decl(bpf);
cdev_decl(ccd);
cdev_decl(cd);
cdev_decl(cn);
cdev_decl(ctty);
#include "dc.h"
cdev_decl(dc);
dev_decl(filedesc,open);
cdev_decl(fd);
cdev_decl(log);
#define	mmread	mmrw
#define	mmwrite	mmrw
dev_type_read(mmrw);
cdev_decl(mm);
#include "pty.h"
#define ptsioctl ptyioctl
#define ptstty ptytty
cdev_decl(pts);
#define ptcioctl ptyioctl
#define ptctty ptytty
cdev_decl(ptc);
cdev_decl(sd);
#include "ss.h"
cdev_decl(ss);
cdev_decl(st);
cdev_decl(sw);
#include "tun.h"
cdev_decl(tun);
#include "uk.h"
cdev_decl(uk);
cdev_decl(vnd);
cdev_decl(vnd);
#include "wsdisplay.h"
cdev_decl(wsdisplay);
#include "wskbd.h"
cdev_decl(wskbd);
#include "wsmouse.h"
cdev_decl(wsmouse);
#include "zstty.h"
cdev_decl(zs);
#include "ipfilter.h"
#include "rnd.h"

#include "scsibus.h"
cdev_decl(scsibus);

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
	cdev_disk_init(NCD,cd),		/* 8: SCSI CDROM */
	cdev_disk_init(NSD,sd),		/* 9: SCSI disk */
	cdev_tape_init(NST,st),		/* 10: SCSI tape */
	cdev_disk_init(NVND,vnd),	/* 11: vnode disk driver */
	cdev_bpftun_init(NBPFILTER,bpf),/* 12: Berkeley packet filter */
	cdev_notdef(),			/* 13: nodev */
	cdev_notdef(),			/* 14: nodev */
	cdev_notdef(),			/* 15: nodev */
	cdev_tty_init(NDC,dc),		/* 16: dc7085 serial interface */
	cdev_tty_init(NZSTTY,zs),	/* 17: z8530 serial interface */
	cdev_notdef(),			/* 18: nodev */
	cdev_notdef(),			/* 19: ULTRIX mt */
	cdev_tty_init(NPTY,pts),	/* 20: ULTRIX pty master */
	cdev_ptc_init(NPTY,ptc),	/* 21: ULTRIX pty slave */
	cdev_notdef(),			/* 22: ULTRIX dmf */
	cdev_notdef(),			/* 23: ULTRIX vax 730 idc */
	cdev_notdef(),			/* 24: ULTRIX dn-11 */

		/* 25-29 CSRG reserved to local sites, DEC sez: */
	cdev_notdef(),		/* 25: ULTRIX gpib */
	cdev_notdef(),		/* 26: ULTRIX lpa */
	cdev_notdef(),		/* 27: ULTRIX psi */
	cdev_notdef(),		/* 28: ULTRIX ib */
	cdev_notdef(),		/* 29: ULTRIX ad */
	cdev_notdef(),		/* 30: ULTRIX rx */
	cdev_notdef(),		/* 31: ULTRIX ik */
	cdev_notdef(),		/* 32: ULTRIX rl-11 */
	cdev_notdef(),		/* 33: ULTRIX dhu/dhv */
	cdev_notdef(),		/* 34: ULTRIX Vax Able dmz, mips dc  */
	cdev_notdef(),		/* 35: ULTRIX qv */
	cdev_notdef(),		/* 36: ULTRIX tmscp */
	cdev_notdef(),		/* 37: ULTRIX vs */
	cdev_notdef(),		/* 38: ULTRIX vax cn console */
	cdev_notdef(),		/* 39: ULTRIX lta */
	cdev_notdef(),		/* 40: ULTRIX crl (Venus, aka 8600 aka 11/790 console RL02) */
	cdev_notdef(),		/* 41: ULTRIX cs */
	cdev_notdef(),		/* 42: ULTRIX qd, Qdss, vcb02 */
	cdev_mm_init(1,mm),	/* 43: errlog (VMS-lookalike puke) */
	cdev_notdef(),		/* 44: ULTRIX dmb */
	cdev_notdef(),		/* 45: ULTRIX vax ss, mips scc */
	cdev_notdef(),		/* 46: ULTRIX vax st */
	cdev_notdef(),		/* 47: ULTRIX vax sd */
	cdev_notdef(),		/* 48: ULTRIX /dev/trace */
	cdev_notdef(),		/* 49: ULTRIX vax sm */
	cdev_notdef(),		/* 50: ULTRIX vax sg */
	cdev_notdef(),		/* 51: ULTRIX vax sh */
	cdev_notdef(),		/* 52: ULTRIX its */
	cdev_notdef(),		/* 53: nodev */
	cdev_notdef(),		/* 54: nodev */
	cdev_notdef(),		/* 55: ULTRIX tz SCSI tape */
	cdev_notdef(),		/* 56: ULTRIX rz SCSI disk */
	cdev_notdef(),		/* 57: nodev */
	cdev_notdef(),		/* 58: ULTRIX fc */
	cdev_notdef(),		/* 59: ULTRIX fg */
	cdev_notdef(),		/* 60: mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 61: mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 62: mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 63: mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 64: mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 65: mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 66: mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 67: mscp, again Ultrix gross coupling to PrestoServe driver */
	cdev_notdef(),		/* 68: ULTRIX ld */
	cdev_notdef(),		/* 69: ULTRIX /dev/audit */
	cdev_notdef(),		/* 70: ULTRIX Mogul (nee' CMU) packetfilter */
	cdev_notdef(),		/* 71: ULTRIX xcons, mips Ultrix /dev/xcons virtual console nightmare */
	cdev_notdef(),		/* 72: ULTRIX xa */
	cdev_notdef(),		/* 73: ULTRIX utx */
	cdev_notdef(),		/* 74: ULTRIX sp - MV2000/VS3100 */
	cdev_notdef(),		/* 75: ULTRIX pr - Prestoserve control device */
	cdev_notdef(),		/* 76: ULTRIX disk shadowing */
	cdev_notdef(),		/* 77: ULTRIX ek */
	cdev_notdef(),		/* 78: ULTRIX msdup */
	cdev_notdef(),		/* 79: ULTRIX multimedia audio A */
	cdev_notdef(),		/* 80: ULTRIX multimedia audio B */
	cdev_notdef(),		/* 81: ULTRIX multimedia video in */
	cdev_notdef(),		/* 82: ULTRIX multimedia video out */
	cdev_notdef(),		/* 83: Personal DECstation fdi */
	cdev_notdef(),		/* 84: Personal DECstation dti */
	cdev_notdef(),		/* 85: */
	cdev_notdef(),		/* 86: */
	cdev_disk_init(NCCD,ccd),	/* 87: concatenated disk driver */
	cdev_wsdisplay_init(NWSDISPLAY,wsdisplay), /* 88: wsdisplay */
	cdev_mouse_init(NWSKBD,wskbd),		/* 89: kbd */
	cdev_mouse_init(NWSMOUSE,wsmouse),	/* 90: mice */
	cdev_ipf_init(NIPFILTER,ipl),	/* 91: ip-filter device */
	cdev_rnd_init(NRND,rnd),	/* 92: random source pseudo-device */
	cdev_bpftun_init(NTUN,tun),	/* 93: network tunnel */
	cdev_lkm_init(NLKM,lkm),	/* 94: loadable module driver */
	cdev_scsibus_init(NSCSIBUS,scsibus),	/* 95: SCSI bus */
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 */
int
iskmemdev(dev)
	dev_t dev;
{
	return (major(dev) == 3 && minor(dev) < 2);
}

/*
 * Returns true if dev is /dev/zero.
 */
int
iszerodev(dev)
	dev_t dev;
{
	return (major(dev) == 3 && minor(dev) == 12);
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
	/*  8 */	25,		/* cd */
	/*  9 */	19,		/* sd */
	/* 10 */	18,		/* st */
	/* 11 */	NODEV,
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
	/* 46 */	NODEV,
	/* 47 */	NODEV,
	/* 48 */	NODEV,
	/* 49 */	NODEV,
	/* 50 */	NODEV,
	/* 51 */	NODEV,
	/* 52 */	NODEV,
	/* 53 */	NODEV,
	/* 54 */	NODEV,
	/* 55 */	NODEV,
	/* 56 */	NODEV,
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
	/* 87 */	24,	/* ccd */
	/* 88 */	NODEV,
	/* 89 */	NODEV,
	/* 90 */	NODEV,
	/* 91 */	NODEV,
	/* 92 */	NODEV,
	/* 93 */	NODEV,
	/* 94 */	NODEV,
	/* 95 */	NODEV,
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
