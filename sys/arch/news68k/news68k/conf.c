/*	$NetBSD: conf.c,v 1.7 2002/06/17 16:33:11 christos Exp $	*/

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

#include "opt_compat_svr4.h"
#include "opt_systrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include "vnd.h"
#include "sd.h"
#include "st.h"
#include "fdc.h"
#include "ccd.h"
#include "md.h"
#include "ch.h"
#include "ss.h"
#include "cd.h"
#include "uk.h"
#include "raid.h"

bdev_decl(sw);
#include "sd.h"
bdev_decl(sd);
#include "ccd.h"
bdev_decl(ccd);
#include "raid.h"
bdev_decl(raid);
#include "vnd.h"
bdev_decl(vnd);
#include "st.h"
bdev_decl(st);
#include "cd.h"
bdev_decl(cd);
#include "md.h"
bdev_decl(md);
#include "fdc.h"
bdev_decl(fd);

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NSD,sd),		/* 0: SCSI disk */
	bdev_disk_init(NFD,fd),		/* 1: fd */
	bdev_disk_init(NMD,md),		/* 2: memory disk */
	bdev_notdef(),			/* 3: */
	bdev_swap_init(1,sw),		/* 4: swap pseudo-device */
	bdev_notdef(),			/* 5: rd */
	bdev_disk_init(NVND,vnd),	/* 6: vnode disk driver */
	bdev_disk_init(NCCD,ccd),	/* 7: concatenated disk driver */
	bdev_notdef(),			/* 8: zd */
	bdev_notdef(),			/* 9: */
	bdev_notdef(),			/* 10: */
	bdev_notdef(),			/* 11: vd */
	bdev_notdef(),			/* 12: xd */
	bdev_notdef(),			/* 13: */
	bdev_notdef(),			/* 14: */
	bdev_notdef(),			/* 15: */
	bdev_disk_init(NCD,cd),		/* 16: SCSI CD-ROM */
	bdev_tape_init(NST,st),		/* 17: SCSI tape */
	bdev_notdef(),			/* 18: */
	bdev_notdef(),			/* 19: */
	bdev_notdef(),			/* 20: */
	bdev_notdef(),			/* 21: */
	bdev_notdef(),			/* 22: */
	bdev_notdef(),			/* 23: */
	bdev_lkm_dummy(),		/* 24: */
	bdev_lkm_dummy(),		/* 25: */
	bdev_lkm_dummy(),		/* 26: */
	bdev_lkm_dummy(),		/* 27: */
	bdev_notdef(),			/* 28: */
	bdev_notdef(),			/* 29: */
	bdev_notdef(),			/* 30: */
	bdev_notdef(),			/* 31: */
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

#include "ipfilter.h"
cdev_decl(ipl);

cdev_decl(cn);
cdev_decl(ctty);
#define mmread  mmrw
#define mmwrite mmrw
cdev_decl(mm);
cdev_decl(sw);

#include "pty.h"
#define ptstty		ptytty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define ptctty		ptytty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);

cdev_decl(log);
cdev_decl(fd);

#include "zstty.h"
cdev_decl(zs);

cdev_decl(cd);
#include "ch.h"
cdev_decl(ch);
cdev_decl(sd);
#include "ss.h"
cdev_decl(ss);
cdev_decl(st);
#include "uk.h"
cdev_decl(uk);

cdev_decl(md);
cdev_decl(vnd);
cdev_decl(ccd);
cdev_decl(raid);

dev_decl(filedesc,open);

#include "rnd.h"
cdev_decl(rnd);

#include "ms.h"
cdev_decl(ms);


#include "bpfilter.h"
cdev_decl(bpf);

#include "tun.h"
cdev_decl(tun);

#if 0
#include "lpt.h"
cdev_decl(lpt);
#endif

#include "scsibus.h"
cdev_decl(scsibus);
#include "wsdisplay.h"
cdev_decl(wsdisplay);
#include "wskbd.h"
cdev_decl(wskbd);
#include "wsmouse.h"
cdev_decl(wsmouse);
#include "wsmux.h"
cdev_decl(wsmux);
#include "clockctl.h"
cdev_decl(clockctl);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_tty_init(NZSTTY,zs),      	/* 1: scc 8530 serial interface */
	cdev_ctty_init(1,ctty),		/* 2: controlling terminal */
	cdev_mm_init(1,mm),		/* 3: /dev/{null,mem,kmem,...} */
	cdev_disk_init(NSD,sd),		/* 4: sd */
	cdev_notdef(),			/* 5: fd */
	cdev_notdef(),			/* 6: lp */
	cdev_swap_init(1,sw),		/* 7: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),        /* 8: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),        /* 9: pseudo-tty master */
	cdev_notdef(),			/* 10: md */
	cdev_notdef(),			/* 11: kb */
	cdev_notdef(),			/* 12: (was mouse) */
	cdev_notdef(),			/* 13: xio */
	cdev_notdef(),			/* 14: (was frame buffer) */
	cdev_notdef(),			/* 15: */
	cdev_tape_init(NST,st),		/* 16: SCSI tape */
	cdev_notdef(),			/* 17: lbp */
	cdev_notdef(),			/* 18: ir */
	cdev_notdef(),			/* 19: vme */
	cdev_notdef(),			/* 20: gpib */
	cdev_notdef(),			/* 21: rd */
	cdev_notdef(),			/* 22: zd */
	cdev_notdef(),			/* 23: ether */
	cdev_bpftun_init(NBPFILTER,bpf),/* 24: Berkeley packet filter */
	cdev_bpftun_init(NTUN,tun),	/* 25: network tunnel */
	cdev_notdef(),			/* 26: */
	cdev_notdef(),			/* 27: */
	cdev_notdef(),			/* 28: scsi */
	cdev_notdef(),			/* 29: shm */
	cdev_notdef(),			/* 30: sem */
	cdev_notdef(),			/* 31: vvcrs */
	cdev_notdef(),			/* 32: fddi */
	cdev_log_init(1,log),		/* 33: /dev/klog */
	cdev_notdef(),			/* 34: ib */
	cdev_notdef(),			/* 35: sb */
	cdev_notdef(),			/* 36: sbe */
	cdev_notdef(),			/* 37: vd */
	cdev_notdef(),			/* 38: xd */
	cdev_notdef(),			/* 39: isdn */
	cdev_notdef(),			/* 40: rb */
	cdev_notdef(),			/* 41: gs */
	cdev_notdef(),			/* 42: rx */
	cdev_notdef(),			/* 43: vcd */
	cdev_notdef(),			/* 44: g3fax */
	cdev_lkm_init(NLKM,lkm),	/* 45: lkm */
	cdev_ipf_init(NIPFILTER,ipl),	/* 46: ipl */
	cdev_rnd_init(NRND,rnd),	/* 47: random source pseudo-device */
	cdev_fd_init(1,filedesc),	/* 48: file descriptor pseudo-dev */
	cdev_disk_init(NCCD,ccd),	/* 49: concatenated disk driver */
	cdev_disk_init(NVND,vnd),	/* 50: vnode disk driver */
	cdev_notdef(),			/* 51: mono frame buffer */
	cdev_notdef(),			/* 52: */
	cdev_notdef(),			/* 53: */
	cdev_uk_init(NUK,uk),		/* 54: SCSI unknown */
	cdev_disk_init(NCD,cd),		/* 55: SCSI CD-ROM */
	cdev_scanner_init(NSS,ss),	/* 56: SCSI scanner */
	cdev_ch_init(NCH,ch),		/* 57: SCSI changer */
	cdev_notdef(),			/* 58: */
	cdev_notdef(),			/* 59: */
	cdev_wsdisplay_init(NWSDISPLAY,wsdisplay), /* 60: wsdisplay */
	cdev_mouse_init(NWSKBD,wskbd),	/* 61: wskbd */
	cdev_mouse_init(NWSMOUSE,wsmouse), /* 62: wsmose */
	cdev_mouse_init(NWSMUX,wsmux),	/* 63: ws multiplexor */
	cdev_lkm_dummy(),		/* 64: */
	cdev_lkm_dummy(),		/* 65: */
	cdev_lkm_dummy(),		/* 66: */
	cdev_lkm_dummy(),		/* 67: */
	cdev_notdef(),			/* 68: */
	cdev_notdef(),			/* 69: */
	cdev_notdef(),			/* 70: */
	cdev_notdef(),			/* 71: */
	cdev_notdef(),			/* 72: */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 73: SCSI bus */
	cdev_disk_init(NRAID,raid),	/* 74: RAIDframe disk driver */
	cdev_svr4_net_init(NSVR4_NET,svr4_net), /* 75: svr4 net pseudo-device */
	cdev_clockctl_init(NCLOCKCTL, clockctl),/* 76: clockctl pseudo device */
#ifdef SYSTRACE
	cdev_systrace_init(1, systrace),/* 77: system call tracing */
#else
	cdev_notdef(),			/* 77: system call tracing */
#endif
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int	mem_no = 3; 	/* major device number of memory special file */

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 */
int
iskmemdev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && minor(dev) < 2);
}

/*
 * Returns true if dev is /dev/zero.
 */
int
iszerodev(dev)
	dev_t dev;
{
	return (major(dev) == mem_no && minor(dev) == 12);
}

static int chrtoblktbl[] =  {
	/* XXXX This needs to be dynamic for LKMs. */
	/*VCHR*/	/*VBLK*/
	/*  0 */	NODEV,
	/*  1 */	NODEV,
	/*  2 */	NODEV,
	/*  3 */	NODEV,
	/*  4 */	0,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	NODEV,
	/*  8 */	NODEV,
	/*  9 */	NODEV,
	/* 10 */	2,
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
	/* 74 */	32,
	/* 75 */	NODEV,
	/* 76 */	NODEV,
	/* 77 */	NODEV,
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
