/*	$NetBSD: conf.c,v 1.11 2001/03/21 22:25:54 lukem Exp $	*/

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
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/vnode.h>

#include <machine/conf.h>

bdev_decl(sw);
cdev_decl(sw);
#include "wd.h"
bdev_decl(wd);
cdev_decl(wd);
#include "com.h"
cdev_decl(com);
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
cdev_decl(scsibus);
#include "ses.h"
cdev_decl(ses);
#include "ld.h"

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

struct bdevsw bdevsw[] =
{
	bdev_notdef(),			/* 0 */
	bdev_swap_init(1,sw),		/* 1: swap pseudo-device */
	bdev_disk_init(NMD,md),		/* 2: memory disk */
	bdev_disk_init(NVND,vnd),	/* 3: vnode disk driver */
	bdev_disk_init(NCCD,ccd),	/* 4: concatenated disk driver */
	bdev_disk_init(NRAID,raid),	/* 5: RAIDframe */
	bdev_disk_init(NWD,wd),		/* 6: ATA disk */
	bdev_disk_init(NSD,sd),		/* 7: SCSI disk */
	bdev_disk_init(NCD,cd),		/* 8: SCSI CD-ROM */
	bdev_tape_init(NST,st),		/* 9: SCSI tape */
	bdev_disk_init(NLD,ld),		/* 10: logical disk driver */
	bdev_lkm_dummy(),		/* 11 */
	bdev_lkm_dummy(),		/* 12 */
	bdev_lkm_dummy(),		/* 13 */
	bdev_lkm_dummy(),		/* 14 */
	bdev_lkm_dummy(),		/* 15 */
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
	cdev_disk_init(NVND,vnd),	/* 3: vnode disk driver */
	cdev_disk_init(NCCD,ccd),	/* 4: concatenated disk driver */
	cdev_disk_init(NRAID,raid),	/* 5: RAIDframe disk driver */
	cdev_disk_init(NWD,wd),		/* 6: ATA disk */
	cdev_disk_init(NSD,sd),		/* 7: SCSI disk */
	cdev_disk_init(NCD,cd),		/* 8: SCSI CD-ROM */
	cdev_tape_init(NST,st),		/* 9: SCSI tape */
	cdev_mm_init(1,mm),		/* 10: /dev/{null,mem,kmem,...} */
	cdev_ctty_init(1,ctty),		/* 11: controlling terminal */
	cdev_tty_init(NPTY,pts),        /* 12: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),        /* 13: pseudo-tty master */
	cdev_log_init(1,log),		/* 14: /dev/klog */
	cdev_fd_init(1,filedesc),	/* 15: file descriptor pseudo-device */
	cdev_rnd_init(NRND,rnd),	/* 16: random source pseudo-device */
	cdev_lkm_init(NLKM,lkm),	/* 17: LKM */
	cdev_bpftun_init(NBPFILTER,bpf),/* 18: Berkeley packet filter */
	cdev_bpftun_init(NTUN,tun),	/* 19: network tunnel */
	cdev_ipf_init(NIPFILTER,ipl),	/* 20: IP filter */
	cdev_scanner_init(NSS,ss),	/* 21: SCSI scanner */
	cdev_ch_init(NCH,ch),		/* 22: SCSI changer */
	cdev_uk_init(NUK,uk),		/* 23: SCSI unknown */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 24: SCSI bus */
	cdev_ses_init(NSES,ses),	/* 25: SCSI SES/SAF-TE */
	cdev_tty_init(NCOM,com),        /* 26: com serial port */
	cdev_disk_init(NLD,ld),         /* 27: logical disk driver */
	cdev_i4b_init(NI4B, i4b),	/* 28: i4b main device */
	cdev_i4bctl_init(NI4BCTL, i4bctl),	/* 29: i4b control device */
	cdev_i4brbch_init(NI4BRBCH, i4brbch),	/* 30: i4b raw b-channel access */
	cdev_i4btrc_init(NI4BTRC, i4btrc),	/* 31: i4b trace device */
	cdev_i4btel_init(NI4BTEL, i4btel),	/* 32: i4b phone device */

};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int	mem_no = 10;	 	/* Major device number of memory special file */

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
	/*  6 */	6,
	/*  7 */	7,
	/*  8 */	8,
	/*  9 */	9,
	/* 10 */	27,
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
