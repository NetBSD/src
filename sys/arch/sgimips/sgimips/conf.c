/*	$NetBSD: conf.c,v 1.8.2.2 2002/02/11 20:08:59 jdolecek Exp $	*/

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
#include <machine/conf.h>

#include "vnd.h"
#include "sd.h"
#include "st.h"
#include "ccd.h"
#include "md.h"
#include "ch.h"
#include "ss.h"
#include "cd.h"
#include "uk.h"
#include "raid.h"
#include "tun.h"
#include "pty.h"
#include "bpfilter.h"
#include "ipfilter.h"
#include "rnd.h"
#include "scsibus.h"
#include "wd.h"
bdev_decl(wd);
cdev_decl(wd);
#include "ld.h"
#include "zstty.h"
cdev_decl(zs);
#include "com.h"
cdev_decl(com);

#include "wsdisplay.h"
cdev_decl(wsdisplay); 
#include "wskbd.h" 
cdev_decl(wskbd);
#include "wsmouse.h"
cdev_decl(wsmouse);
#include "wsmux.h"
cdev_decl(wsmux);
#include "wsfont.h"
cdev_decl(wsfont);

#include "i4b.h"
#include "i4bctl.h"
#include "i4btrc.h"
#include "i4brbch.h"
#include "i4btel.h"
cdev_decl(i4b);
cdev_decl(i4bctl);
cdev_decl(i4btrc);
cdev_decl(i4brbch);
cdev_decl(i4btel);

cdev_decl(arcbios_tty);

#include "clockctl.h"
cdev_decl(clockctl);

struct bdevsw bdevsw[] =
{
	bdev_notdef(),			/* 0: */
	bdev_swap_init(1,sw),		/* 1: swap pseudo-device */
	bdev_disk_init(NMD,md),		/* 2: memory disk */
	bdev_disk_init(NCCD,ccd),	/* 3: concatenated disk driver */
	bdev_disk_init(NVND,vnd),	/* 4: vnode disk driver */
	bdev_disk_init(NRAID,raid),	/* 5: RAIDframe */
	bdev_notdef(),			/* 6: */
	bdev_notdef(),			/* 7: */
	bdev_notdef(),			/* 8: */
	bdev_notdef(),			/* 9: */
	bdev_disk_init(NSD,sd),		/* 10: SCSI disk */
	bdev_tape_init(NST,st),		/* 11: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 12: SCSI CD-ROM */
	bdev_disk_init(NWD,wd),		/* 13: ATA disk */
	bdev_disk_init(NLD,ld),		/* 14: logical disk driver */
	bdev_notdef(),			/* 15: */
	bdev_notdef(),			/* 16: */
	bdev_notdef(),			/* 17: */
	bdev_notdef(),			/* 18: */
	bdev_notdef(),			/* 19: */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

/*
 * swapdev is a fake block device implemented in sw.c and only used 
 * internally to get to swstrategy.  It cannot be provided to the
 * users, because the swstrategy routine munches the b_dev and b_blkno
 * entries before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines.  User access (e.g., for libkvm
 * and ps) is provided through the /dev/drum character (raw) device.
 */
dev_t	swapdev = makedev(1, 0);

struct cdevsw cdevsw[] =
{
	cdev_cn_init(1,cn),             /* 0: console */
	cdev_swap_init(1,sw),		/* 1: /dev/drum (swap pseudo-device) */
	cdev_disk_init(NMD,md),		/* 2: memory disk driver */
	cdev_disk_init(NCCD,ccd),	/* 3: concatenated disk driver */
	cdev_disk_init(NVND,vnd),	/* 4: vnode disk driver */
	cdev_disk_init(NRAID,raid),	/* 5: RAIDframe disk driver */
	cdev_notdef(),			/* 6: */
	cdev_notdef(),			/* 7: */
	cdev_notdef(),			/* 8: */
	cdev_notdef(),			/* 9: */
	cdev_disk_init(NSD,sd),		/* 10: SCSI disk */
	cdev_tape_init(NST,st),		/* 11: SCSI tape */
	cdev_disk_init(NCD,cd),		/* 12: SCSI CD-ROM */
	cdev_disk_init(NWD,wd),		/* 13: ATA disk */
	cdev_disk_init(NLD,ld),		/* 14: logical disk driver */
	cdev_notdef(),			/* 15: */
	cdev_notdef(),			/* 16: */
	cdev_notdef(),			/* 17: */
	cdev_notdef(),			/* 18: */
	cdev_notdef(),			/* 19: */
	cdev_mm_init(1,mm),		/* 20: /dev/{null,mem,kmem,...} */
	cdev_ctty_init(1,ctty),		/* 21: controlling terminal */
	cdev_tty_init(NPTY,pts),        /* 22: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),        /* 23: pseudo-tty master */
	cdev_log_init(1,log),		/* 24: /dev/klog */
	cdev_lkm_init(NLKM,lkm),	/* 25: lkm */
	cdev_fd_init(1,filedesc),	/* 26: file descriptor pseudo-device */
	cdev_bpftun_init(NBPFILTER,bpf),/* 27: Berkeley packet filter */
	cdev_bpftun_init(NTUN,tun),	/* 28: network tunnel */
	cdev_ipf_init(NIPFILTER,ipl),	/* 29: ipl */
	cdev_rnd_init(NRND,rnd),	/* 30: random source pseudo-device */
	cdev_uk_init(NUK,uk),		/* 31: SCSI unknown */
	cdev_scanner_init(NSS,ss),	/* 32: SCSI scanner */
	cdev_ch_init(NCH,ch),		/* 33: SCSI changer */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 34: SCSI bus */
	cdev_tty_init(NZSTTY,zs),	/* 35: Zilog 8530 serial port */
	cdev_tty_init(NCOM,com),	/* 36: com serial port */
	cdev_tty_init(1,arcbios_tty),	/* 37: ARCS PROM console */
	cdev_i4b_init(NI4B, i4b),	/* 38: i4b main device */
	cdev_i4bctl_init(NI4BCTL, i4bctl),	/* 39: i4b control device */
	cdev_i4brbch_init(NI4BRBCH, i4brbch),	/* 40: i4b raw b-channel access */
	cdev_i4btrc_init(NI4BTRC, i4btrc),	/* 41: i4b trace device */
	cdev_i4btel_init(NI4BTEL, i4btel),	/* 42: i4b phone device */
	cdev_notdef(),			/* 43: */
	cdev_notdef(),			/* 44: */
	cdev_notdef(),			/* 45: */
	cdev_notdef(),			/* 46: */
	cdev_notdef(),			/* 47: */
	cdev_notdef(),			/* 48: */
	cdev_notdef(),			/* 49: */
	cdev_wsdisplay_init(NWSDISPLAY,
			wsdisplay),	/* 50: frame buffers, etc. */
	cdev_mouse_init(NWSKBD,wskbd),	/* 51: keyboards */
	cdev_mouse_init(NWSMOUSE,
			wsmouse),	/* 52: mice */
	cdev_mouse_init(NWSMUX, wsmux),	/* 53: ws multiplexor */
	cdev__oci_init(NWSFONT,wsfont),	/* 54: wsfont pseudo-device */
	cdev_notdef(),			/* 55: */
	cdev_notdef(),			/* 56: */
	cdev_notdef(),			/* 57: */
	cdev_notdef(),			/* 58: */
	cdev_notdef(),			/* 59: */
	cdev_clockctl_init(NCLOCKCTL, clockctl),/* 60: clockctl pseudo device */
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int	mem_no = 20;	 	/* Major device number of memory special file */

int
iskmemdev(dev)
	dev_t dev;
{
	return (major(dev) == mem_no && minor(dev) < 2);
}

int
iszerodev(dev)
	dev_t dev;
{
	return (major(dev) == mem_no && minor(dev) == 12);
}

static int chrtoblktbl[] =  {
	/* XXX This needs to be dynamic for LKMs. */
	/* VCHR */	/* VBLK */
	/*  0 */	NODEV,
	/*  1 */	1,
	/*  2 */	2,
	/*  3 */	3,
	/*  4 */	4,
	/*  5 */	5,
	/*  6 */	NODEV,
	/*  7 */	NODEV,
	/*  8 */	NODEV,
	/*  9 */	NODEV,
	/* 10 */	10,
	/* 11 */	11,
	/* 12 */	12,
	/* 13 */	13,
	/* 14 */	14,
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
};

dev_t
chrtoblk(dev)
	dev_t dev;
{
	int blkmaj;

	if (major(dev) >= nchrdev)
		return NODEV;
	blkmaj = chrtoblktbl[major(dev)];
	if (blkmaj == NODEV)
		return NODEV;
	return (makedev(blkmaj, minor(dev)));
}
