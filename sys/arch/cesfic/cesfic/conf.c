/*	$NetBSD: conf.c,v 1.1 2001/05/14 18:23:00 drochner Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *
 *      @(#)conf.c	7.9 (Berkeley) 5/28/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include "ccd.h"
bdev_decl(ccd);
#include "vnd.h"
bdev_decl(vnd);
bdev_decl(sw);
#include "md.h"
bdev_decl(md);

struct bdevsw	bdevsw[] =
{
	bdev_swap_init(1,sw),		/* 0: swap pseudo-device */
	bdev_disk_init(NCCD,ccd),	/* 1: concatenated disk driver */
	bdev_disk_init(NVND,vnd),	/* 2: vnode disk driver */
	bdev_disk_init(NMD,md),		/* 3: memory disk driver */
	bdev_lkm_dummy(),		/* 4 */
	bdev_lkm_dummy(),		/* 5 */
	bdev_lkm_dummy(),		/* 6 */
	bdev_lkm_dummy(),		/* 7 */
	bdev_lkm_dummy(),		/* 8 */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

#include "zstty.h"
cdev_decl(zs);
cdev_decl(cn);
cdev_decl(ctty);
#define	mmread	mmrw
#define	mmwrite	mmrw
cdev_decl(mm);
cdev_decl(sw);
#include "pty.h"
#define	ptstty		ptytty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptctty		ptytty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);
cdev_decl(log);
cdev_decl(ccd);
cdev_decl(vnd);
dev_decl(filedesc,open);
#include "bpfilter.h"
cdev_decl(bpf);
#include "tun.h"
cdev_decl(tun);
#include "ipfilter.h"
#include "rnd.h"
cdev_decl(md);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1,sw),		/* 3: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_disk_init(NCCD,ccd),	/* 7: concatenated disk */
	cdev_ipf_init(NIPFILTER,ipl),	/* 8: ip-filter device */
	cdev_disk_init(NVND,vnd),	/* 9: vnode disk driver */
	cdev_tty_init(NZSTTY,zs),	/* 10: SCC serial ports */
	cdev_fd_init(1,filedesc),	/* 11: file descriptor pseudo-device */
	cdev_bpftun_init(NBPFILTER,bpf),/* 12: Berkeley packet filter */
	cdev_bpftun_init(NTUN,tun),	/* 13: network tunnel */
	cdev_lkm_init(NLKM,lkm),	/* 14: loadable module driver */
	cdev_disk_init(NMD,md),		/* 15: memory disk driver */
	cdev_lkm_dummy(),		/* 16 */
	cdev_lkm_dummy(),		/* 17 */
	cdev_lkm_dummy(),		/* 18 */
	cdev_lkm_dummy(),		/* 19 */
	cdev_lkm_dummy(),		/* 20 */
	cdev_rnd_init(NRND,rnd),	/* 21: random source pseudo-device */
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
dev_t	swapdev = makedev(0, 0);

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

	return (major(dev) == mem_no && minor(dev) == 12);
}

static int chrtoblktbl[] = {
	/* XXXX This needs to be dynamic for LKMs. */
	/*VCHR*/	/*VBLK*/
	/*  0 */	NODEV,
	/*  1 */	NODEV,
	/*  2 */	NODEV,
	/*  3 */	0,
	/*  4 */	NODEV,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	1,
	/*  8 */	NODEV,
	/*  9 */	2,
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	NODEV,
	/* 14 */	NODEV,
	/* 15 */	3,
	/* 16 */	NODEV,
	/* 17 */	NODEV,
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

