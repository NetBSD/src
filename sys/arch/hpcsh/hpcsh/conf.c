/*	$NetBSD: conf.c,v 1.4 2001/03/15 17:30:56 uch Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include <dev/cons.h>

/*
 * Block devices.
 */
bdev_decl(sw);
#include "wd.h"
bdev_decl(wd);
#include "sd.h"
bdev_decl(sd);
#include "st.h"
bdev_decl(st);
#include "cd.h"
bdev_decl(cd);
#include "vnd.h"
bdev_decl(vnd);
#include "raid.h"
bdev_decl(raid);
#include "ccd.h"
bdev_decl(ccd);
#include "md.h"
bdev_decl(md);

/*
 * Character devices.
 */
/* System */
cdev_decl(cn);
cdev_decl(ctty);
#define	mmread	mmrw
#define	mmwrite	mmrw
dev_type_read(mmrw);
cdev_decl(mm);
cdev_decl(sw);
#include "pty.h"
#define ptstty ptytty
#define ptsioctl ptyioctl
cdev_decl(pts);
#define ptctty ptytty
#define ptcioctl ptyioctl
cdev_decl(ptc);
cdev_decl(log);

/* Disk */
cdev_decl(wd);
cdev_decl(sd);
cdev_decl(st);
cdev_decl(cd);
cdev_decl(md);
cdev_decl(raid);
cdev_decl(vnd);
cdev_decl(ccd);

/* Network */
#include "tun.h"
cdev_decl(tun);
#include "bpfilter.h"
cdev_decl(bpf);
#include "ipfilter.h"
cdev_decl(ipl);

/* SCSI misc */
#include "scsibus.h"
cdev_decl(scsibus);
#include "ch.h"
cdev_decl(ch);
#include "ss.h"
cdev_decl(ss);
#include "uk.h"
cdev_decl(uk);

/* wscons */
#include "wsdisplay.h"
cdev_decl(wsdisplay);
#include "wskbd.h"
cdev_decl(wskbd);
#include "wsmouse.h"
cdev_decl(wsmouse);
#include "wsmux.h"
cdev_decl(wsmux);

/* misc */
#include "rnd.h"

/* SH specific */
#define scicnpollc	nullcnpollc
cons_decl(sci);
#define scifcnpollc	nullcnpollc
cons_decl(scif);
cons_decl(com);

#include "sci.h"
cdev_decl(sci);
#include "scif.h"
cdev_decl(scif);
#include "com.h"
cdev_decl(com);

#include "biconsdev.h"
cdev_decl(biconsdev);
#define biconscnpollc	nullcnpollc
cons_decl(bicons);

struct bdevsw bdevsw[] =
{
	bdev_swap_init(1,sw),		/* 0: swap pseudo-device */
	bdev_disk_init(NWD,wd),		/* 1: IDE disk driver */
	bdev_notdef(),			/* 2: reserve */
	bdev_disk_init(NSD,sd),		/* 3: SCSI disk driver */
	bdev_tape_init(NST,st),		/* 4: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 5: SCSI CD-ROM */
	bdev_disk_init(NMD,md),		/* 6: memory disk driver */
	bdev_disk_init(NCCD,ccd),	/* 7: concatenated disk driver */
	bdev_disk_init(NVND,vnd),	/* 8: vnode disk driver */
	bdev_disk_init(NRAID,raid),	/* 9: RAIDframe disk driver */
};

struct cdevsw cdevsw[] =
{
	cdev_mm_init(1, mm),            /*  0: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1, sw),          /*  1: /dev/drum(swap pseudo-device) */
	cdev_cn_init(1, cn),            /*  2: virtual console */
	cdev_ctty_init(1,ctty),         /*  3: controlling terminal */
	cdev_fd_init(1,filedesc),	/*  4: file descriptor pseudo-dev */
	cdev_log_init(1,log),           /*  5: /dev/klog */
	cdev_ptc_init(NPTY,ptc),        /*  6: pseudo-tty master */
	cdev_tty_init(NPTY,pts),        /*  7: pseudo-tty slave */
	cdev_tty_init(NCOM,com),	/*  8: NS16550 compatible */
	cdev_notdef(),		        /*  9: (reserved) parallel printer*/
	cdev_disk_init(NWD, wd),        /* 10: ST506/ESDI/IDE disk */
	cdev_notdef(),			/* 11: (reserved) floppy diskette */
	cdev_disk_init(NMD, md),        /* 12: memory disk driver */
	cdev_disk_init(NCCD,ccd),	/* 13: concatenated disk driver */
	cdev_disk_init(NVND,vnd),       /* 14: vnode disk driver */
	cdev_disk_init(NRAID,raid),	/* 15: RAIDframe disk driver */
	cdev_scsibus_init(NSCSIBUS,
	    scsibus),			/* 16: SCSI bus */
	cdev_disk_init(NSD,sd),		/* 17: SCSI disk */
	cdev_tape_init(NST,st),		/* 18: SCSI tape */
	cdev_disk_init(NCD,cd),		/* 19: SCSI CD-ROM */
	cdev_ch_init(NCH,ch),	 	/* 20: SCSI autochanger */
	cdev_uk_init(NUK,uk),	 	/* 21: SCSI unknown */
	cdev_scanner_init(NSS,ss),	/* 22: SCSI scanner */
	cdev_ipf_init(NIPFILTER,ipl),	/* 23: ip-filter device */
	cdev_bpftun_init(NTUN,tun),	/* 24: network tunnel */
	cdev_bpftun_init(NBPFILTER,bpf),/* 25: Berkeley packet filter */
	cdev_wsdisplay_init(NWSDISPLAY,
	    wsdisplay),			/* 26: ws displays. */
	cdev_mouse_init(NWSKBD, wskbd),	/* 27: ws keyboards */
	cdev_mouse_init(NWSMOUSE,
	    wsmouse),			/* 28: ws mice */
	cdev_mouse_init(NWSMUX,	wsmux), /* 29: ws multiplexor */
	cdev_rnd_init(NRND,rnd),	/* 30: random source pseudo-device */
	cdev_tty_init(NSCIF,scif),	/* 31: SH internal serial with FIFO */
	cdev_tty_init(NSCI,sci),	/* 32: SH internal serial */
	cdev_tty_init(NBICONSDEV,
		      biconsdev),	/* 34: bicons pseudo-dev */
};

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
	/*  9 */	NODEV,
	/* 10 */	1, /* wd */
	/* 11 */	NODEV, /* reserve */
	/* 12 */	6, /* md */
	/* 13 */	7, /* ccd */
	/* 14 */	8, /* vnd */
	/* 15 */	9, /* raid */
	/* 16 */	NODEV,
	/* 17 */	3, /* sd */
	/* 18 */	4, /* st */
	/* 19 */	5, /* cd */
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
};

/*
 * Swapdev is a fake block device implemented  in sw.c and only used 
 * internally to get to swstrategy.  It cannot be provided to the
 * users, because the swstrategy routine munches the b_dev and b_blkno
 * entries before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines.   User access (e.g., for libkvm
 * and ps) is provided through the /dev/drum character (raw) device.
 */
dev_t	swapdev = makedev(0, 0);
int	mem_no = 0; 	/* major device number of memory special file */
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 */
int
iskmemdev(dev_t dev)
{
	return (major(dev) == mem_no && (minor(dev) < 2 || minor(dev) == 14));
}

/*
 * Returns true if dev is /dev/zero.
 */
int
iszerodev(dev_t dev)
{
	return (major(dev) == mem_no && minor(dev) == 12);
}

/*
 * Routine to convert from character to block device number.
 */
dev_t
chrtoblk(dev_t dev)
{
	int blkmaj;

	if (major(dev) >= nchrdev)
		return (NODEV);
	blkmaj = chrtoblktbl[major(dev)];
	if (blkmaj == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}
