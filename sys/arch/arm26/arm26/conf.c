/* $NetBSD: conf.c,v 1.4 2001/03/21 22:25:53 lukem Exp $ */
/*-
 * Copyright (c) 1998, 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * conf.c -- Device switch tables and related gumf.
 */

#include <sys/param.h>

__RCSID("$NetBSD: conf.c,v 1.4 2001/03/21 22:25:53 lukem Exp $");

#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <dev/cons.h>

#define mmread  mmrw
#define mmwrite mmrw
cdev_decl(mm);
bdev_decl(sw);
cdev_decl(sw);
#include "pty.h"
#include "md.h"
#include "vnd.h"
#include "ccd.h"
#include "wd.h"
bdev_decl(wd);
cdev_decl(wd);
#include "sd.h"
#include "cd.h" 
#include "bpfilter.h"
#include "tun.h"
#include "ipfilter.h"
#include "rnd.h"
#include "rs.h"
#include "wsdisplay.h"
cdev_decl(wsdisplay);
#include "wskbd.h"
cdev_decl(wskbd);
#include "wsmouse.h"
cdev_decl(wsmouse);
#include "wsmux.h"
cdev_decl(wsmux);
#include "com.h"
cdev_decl(com);
#include "lpt.h"
cdev_decl(lpt);

cons_decl(rs);

struct bdevsw bdevsw[] = {
	bdev_swap_init(1, sw),		/* 0: swap pseudo-device */
	bdev_disk_init(NMD,  md),	/* 1: memory "disk" */
	bdev_disk_init(NVND, vnd),	/* 2: vnode "disk" */
	bdev_disk_init(NCCD, ccd),	/* 3: concatenated disks */
	bdev_disk_init(NWD, wd),	/* 4: IDE disks */
	bdev_disk_init(NSD, sd),	/* 5: SCSI disks */
	bdev_disk_init(NCD, cd),	/* 6: SCSI CD-ROMs */
};

int nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

struct cdevsw cdevsw[] = {
	/* First seven are standard across most ports */
	cdev_cn_init(1, cn),		/* 0: /dev/console */
	cdev_ctty_init(1, ctty),	/* 1: /dev/tty */
	cdev_mm_init(1, mm),		/* 2: /dev/{null,mem,kmem,zero} */
	cdev_swap_init(1, sw),		/* 3: /dev/drum */
	cdev_tty_init(NPTY, pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY, ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1, log),		/* 6: /dev/klog */
	cdev_fd_init(1, filedesc),	/* 7: file descriptors */
	cdev_disk_init(NMD, md),	/* 8: memory "disk" */
	cdev_disk_init(NVND, vnd),	/* 9: vnode "disk" */
	cdev_disk_init(NCCD, ccd),	/* 10: concatenated disks */
	cdev_mouse_init(NWSKBD, wskbd),	/* 11: keyboards */
	cdev_mouse_init(NWSMOUSE, wsmouse),
					/* 12: mice */
	cdev_mouse_init(NWSMUX, wsmux),	/* 13: keyboard/mouse multiplexor */
	cdev_wsdisplay_init(NWSDISPLAY, wsdisplay),
					/* 14: console display */
	cdev_disk_init(NWD, wd),	/* 15: IDE disks */
	cdev_disk_init(NSD, sd),	/* 16: SCSI disks */
	cdev_disk_init(NCD, cd),	/* 17: SCSI CD-ROMs */
	cdev_bpftun_init(NBPFILTER, bpf),/*18: Berkeley packet filter */
	cdev_bpftun_init(NTUN,tun),	/* 19: network tunnel */
	cdev_tty_init(NCOM, com),	/* 20: ns8250 etc serial */
	cdev_lpt_init(NLPT, lpt),	/* 21: PC-style parallel */
};

int nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);


int mem_no = 2; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(1, 0);

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
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
	return (major(dev) == mem_no && minor(dev) == 3);
}


static int chrtoblktbl[] = {
/* XXXX This needs to be dynamic for LKMs. */
	/*VCHR*/        /*VBLK*/
	/*  0 */        NODEV,
	/*  1 */        NODEV,
	/*  2 */        NODEV,
	/*  3 */        NODEV,
	/*  4 */        NODEV,
	/*  5 */        NODEV,
	/*  6 */        NODEV,
	/*  7 */        NODEV,
	/*  8 */        1,		/* md */
	/*  9 */        2,		/* vnd */
	/* 10 */        3,		/* ccd */
	/* 11 */        NODEV,
	/* 12 */        NODEV,
	/* 13 */        NODEV,
	/* 14 */	NODEV,
	/* 15 */	4,		/* wd */
	/* 16 */	5,		/* sd */
	/* 17 */	6,		/* cd */
	/* 18 */	NODEV,
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
};

/*
 * Convert a character device number to a block device number.
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

struct consdev constab[] = {
#if NRS > 0
	cons_init(rs),
#endif
	{ 0 }
};
