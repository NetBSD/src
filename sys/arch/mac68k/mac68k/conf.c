/*	$NetBSD: conf.c,v 1.53 1999/11/05 09:54:07 fredb Exp $	*/

/*
 * Copyright (c) 1990 The Regents of the University of California.
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
 */
/*-
 * Derived a long time ago from
 *      @(#)conf.c	7.9 (Berkeley) 5/28/91
 */

#include "opt_compat_svr4.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <dev/cons.h>

#include "ccd.h"
#include "cd.h"
#include "ch.h"
#include "fd.h"
#include "md.h"
#include "raid.h"
#include "sd.h"
#include "st.h"
#include "vcoda.h"
#include "vnd.h"

/* No cdev for md */

bdev_decl(ccd);
bdev_decl(cd);
bdev_decl(ch);
bdev_decl(fd);
bdev_decl(md);
bdev_decl(raid);
bdev_decl(sd);
bdev_decl(st);
bdev_decl(sw);
bdev_decl(vnd);

struct bdevsw	bdevsw[] =
{
	bdev_notdef(),			/* 0 */
	bdev_notdef(),			/* 1 */
	bdev_notdef(),			/* 2 */
	bdev_swap_init(1,sw),		/* 3: swap pseudo-device */
	bdev_disk_init(NSD,sd),		/* 4: SCSI disk */
	bdev_tape_init(NST,st),		/* 5: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 6: SCSI CD-ROM */
	bdev_notdef(),			/* 7 */
	bdev_disk_init(NVND,vnd),	/* 8: vnode disk driver */
	bdev_disk_init(NCCD,ccd),	/* 9: concatenated disk driver */
	bdev_notdef(),			/* 10 */
	bdev_notdef(),			/* 11 */
	bdev_notdef(),			/* 12 */
	bdev_disk_init(NMD,md),	 	/* 13: memory disk -- for install */
	bdev_lkm_dummy(),		/* 14 */
	bdev_lkm_dummy(),		/* 15 */
	bdev_lkm_dummy(),		/* 16 */
	bdev_lkm_dummy(),		/* 17 */
	bdev_lkm_dummy(),		/* 18 */
	bdev_lkm_dummy(),		/* 19 */
	bdev_disk_init(NRAID,raid),	/* 20: RAIDframe disk driver */
	bdev_disk_init(NFD, fd),	/* 21: Sony floppy disk */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

#define NADB 1 /* #include "adb.h" */
#include "aed.h"
#include "asc.h"
#include "bpfilter.h"
#include "ch.h"
#include "grf.h"
#include "ipfilter.h"
#include "ite.h"
#include "pty.h"
#include "rnd.h"
#include "se.h"
#include "ss.h"
#include "tun.h"
#include "uk.h"
#if 0
#include "wsdisplay.h"
#include "wskbd.h"
#include "wsmouse.h"
#include "wsmux.h"
#endif
#include "zsc.h"
#include "zstty.h"
#include "scsibus.h"

cdev_decl(aed);
cdev_decl(asc);
cdev_decl(bpf);
cdev_decl(ccd);
cdev_decl(ch);
cdev_decl(cn);
cdev_decl(ctty);
cdev_decl(fd);
cdev_decl(grf);
cdev_decl(ipl);
cdev_decl(ite);
cdev_decl(kbd);
cdev_decl(log);
cdev_decl(md);
#define mmread	mmrw
#define mmwrite	mmrw
cdev_decl(mm);
cdev_decl(ms);
#define	ptcioctl	ptyioctl
#define	ptctty		ptytty
cdev_decl(ptc);
#define	ptsioctl	ptyioctl
#define	ptstty		ptytty
cdev_decl(pts);
cdev_decl(raid);
cdev_decl(sd);
cdev_decl(se);
cdev_decl(ss);
cdev_decl(st);
cdev_decl(sw);
cdev_decl(tun);
cdev_decl(uk);
cdev_decl(vnd);
#if 0
cdev_decl(wskbd);
cdev_decl(wsmouse);
cdev_decl(wsmux);
cdev_decl(wsdisplay);
#endif
cdev_decl(zs);
cdev_decl(zsc);
cdev_decl(scsibus);
cdev_decl(vc_nb_);

dev_decl(filedesc,open);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1,sw),		/* 3: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_notdef(),			/* 7 */
	cdev_notdef(),			/* 8 */
	cdev_notdef(),			/* 9 */
	cdev_fb_init(NGRF,grf),		/* 10: frame buffer */
	cdev_tty_init(NITE,ite),	/* 11: console terminal emulator */
	cdev_tty_init(NZSTTY,zs),	/* 12: 2 mac serial ports -- BG*/
	cdev_disk_init(NSD,sd),		/* 13: SCSI disk */
	cdev_tape_init(NST,st),		/* 14: SCSI tape */
	cdev_disk_init(NCD,cd),		/* 15: SCSI CD-ROM */
	cdev_notdef(),			/* 16 */
	cdev_ch_init(NCH,ch),		/* 17: SCSI autochanger */
	cdev_notdef(),			/* 18 */
	cdev_disk_init(NVND,vnd),	/* 19: vnode disk driver */
	cdev_disk_init(NCCD,ccd),	/* 20: concatenated disk driver */
	cdev_fd_init(1,filedesc),	/* 21: file descriptor pseudo-device */
	cdev_bpftun_init(NBPFILTER,bpf),/* 22: Berkeley packet filter */
	cdev_mouse_init(NAED,aed),	/* 23: ADB event device */
	cdev_bpftun_init(NTUN,tun),	/* 24: network tunnel */
	cdev_lkm_init(NLKM,lkm),	/* 25: loadable module driver */
	cdev_lkm_dummy(),		/* 26 */
	cdev_lkm_dummy(),		/* 27 */
	cdev_lkm_dummy(),		/* 28 */
	cdev_lkm_dummy(),		/* 29 */
	cdev_lkm_dummy(),		/* 30 */
	cdev_lkm_dummy(),		/* 31 */
	cdev_disk_init(NMD,md),		/* 32: memory disk driver */
	cdev_scanner_init(NSS,ss),	/* 33: SCSI scanner */
	cdev_uk_init(NUK,uk),		/* 34: SCSI unknown */
	cdev_ipf_init(NIPFILTER,ipl),	/* 35: ip-filter device */
	cdev_audio_init(NASC,asc),	/* 36: ASC audio device */
	cdev_se_init(NSE, se),		/* 37: SCSI ethernet */
	cdev_rnd_init(NRND, rnd),	/* 38: random source pseudo-device */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 39: SCSI bus */
#if 0
	cdev_mouse_init(NWSKBD, wskbd),	/* 40: wscons keyboard driver */
	cdev_mouse_init(NWSMOUSE,
	    wsmouse),			/* 41: wscons mouse driver */
#else
	cdev_notdef(),			/* 40 */
	cdev_notdef(),			/* 41 */
#endif
	cdev_disk_init(NRAID,raid),	/* 42: RAIDframe disk driver */
	cdev_disk_init(NFD,fd),		/* 43: Sony floppy disk */
	cdev_svr4_net_init(NSVR4_NET,svr4_net), /* 44: svr4 net pseudo-device */
#if 0
	cdev_mouse_init(NWSMUX, wsmux),	/* 45: ws multiplexor */
	cdev_wsdisplay_init(NWSDISPLAY,wsdisplay), /* 46: frame buffers, etc. */
#else
	cdev_notdef(),			/* 45 */
	cdev_notdef(),			/* 46 */
#endif
	cdev_vc_nb_init(NVCODA,vc_nb_),	/* 47: Venus cache driver (Coda) */
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int	mem_no = 2; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(3, 0);

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
int
iskmemdev(dev)
	dev_t	dev;
{

	return (major(dev) == mem_no && minor(dev) < 2);
}

/*
 * Returns true if dev is /dev/zero.
 */
int
iszerodev(dev)
	dev_t	dev;
{

	return (major(dev) == mem_no && minor(dev) == 12);
}

static int chrtoblktab[] = {
	/* XXXX This needs to be dynamic for LKMs. */
	/*VCHR*/	/*VBLK*/
	/*  0 */	NODEV,
	/*  1 */	NODEV,
	/*  2 */	NODEV,
	/*  3 */	3,
	/*  4 */	NODEV,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	NODEV,
	/*  8 */	NODEV,
	/*  9 */	NODEV,
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	4,
	/* 14 */	5,
	/* 15 */	6,
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	NODEV,
	/* 19 */	8,
	/* 20 */	9,
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
	/* 32 */	32,
	/* 33 */	NODEV,
	/* 34 */	NODEV,
	/* 35 */	NODEV,
	/* 36 */	NODEV,
	/* 37 */	NODEV,
	/* 38 */	NODEV,
	/* 39 */	NODEV,
	/* 40 */	NODEV,
	/* 41 */	NODEV,
	/* 42 */	20,
	/* 43 */	NODEV,
	/* 44 */	NODEV,
	/* 45 */	NODEV,
	/* 46 */	NODEV,
	/* 47 */	NODEV,
};

dev_t
chrtoblk(dev)
	dev_t	dev;
{
	int	blkmaj;

	if (major(dev) >= nchrdev)
		return NODEV;
	blkmaj = chrtoblktab[major(dev)];
	if (blkmaj == NODEV)
		return NODEV;
	return (makedev(blkmaj, minor(dev)));
}

#define itecnpollc	nullcnpollc
cons_decl(ite);
#define zscnpollc	nullcnpollc
cons_decl(zs);

struct	consdev constab[] = {
#if NITE > 0
	cons_init(ite),
#endif
#if NZSTTY > 0
	cons_init(zs),
#endif
	{ 0 },
};
