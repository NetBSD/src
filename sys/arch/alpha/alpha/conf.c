/*	$NetBSD: conf.c,v 1.4 1995/04/10 01:54:00 mycroft Exp $	*/

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

int	rawread		__P((dev_t, struct uio *, int));
int	rawwrite	__P((dev_t, struct uio *, int));
void	swstrategy	__P((struct buf *));
int	ttselect	__P((dev_t, int, struct proc *));

#ifndef LKM
#define	lkmenodev	enodev
#else
int	lkmenodev();
#endif

#include "st.h"
bdev_decl(st);
#include "cd.h"
bdev_decl(cd);
#include "sd.h"
bdev_decl(sd);
#include "vnd.h"
bdev_decl(vnd);

struct bdevsw	bdevsw[] =
{
	bdev_notdef(),			/*  0: unused (XXX) */
	bdev_swap_init(),		/*  1: swap pseudo-device */
	bdev_tape_init(NST,st),		/*  2: SCSI tape */
	bdev_disk_init(NCD,cd),		/*  3: SCSI CD-ROM */
	bdev_notdef(),			/*  4: unused (XXX) */
	bdev_notdef(),			/*  5: unused (XXX) */
	bdev_notdef(),			/*  6: unused (XXX) */
	bdev_notdef(),			/*  7: unused (XXX) */
	bdev_disk_init(NSD,sd),		/*  8: SCSI disk */
	bdev_disk_init(NVND,vnd),	/*  9: vnode disk driver */
	bdev_lkm_dummy(),		/* 10: reserved for LKM */
	bdev_lkm_dummy(),		/* 11: reserved for LKM */
	bdev_lkm_dummy(),		/* 12: reserved for LKM */
	bdev_lkm_dummy(),		/* 13: reserved for LKM */
	bdev_lkm_dummy(),		/* 14: reserved for LKM */
	bdev_lkm_dummy(),		/* 15: reserved for LKM */
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

cdev_decl(cn);
cdev_decl(ctty);
#define	mmread  mmrw
#define	mmwrite mmrw
cdev_decl(mm);
#include "pty.h"
#define	pts_tty		pt_tty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptc_tty		pt_tty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);
cdev_decl(log);
#include "tun.h"
cdev_decl(tun);
cdev_decl(sd);
cdev_decl(vnd);
dev_type_open(fdopen);
#include "bpfilter.h"
cdev_decl(bpf);
cdev_decl(st);
cdev_decl(cd);
#include "ch.h"
cdev_decl(ch);
#include "scc.h"
cdev_decl(scc);
#ifdef LKM
#define	NLKM	1
#else
#define	NLKM	0
#endif
cdev_decl(lkm);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/*  0: virtual console */
	cdev_ctty_init(1,ctty),		/*  1: controlling terminal */
	cdev_mm_init(1,mm),		/*  2: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1,sw),		/*  3: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/*  4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/*  5: pseudo-tty master */
	cdev_log_init(1,log),		/*  6: /dev/klog */
	cdev_bpftun_init(NTUN,tun),	/*  7: network tunnel */
	cdev_disk_init(NSD,sd),		/*  8: scsi disk */
	cdev_disk_init(NVND,vnd),	/*  9: vnode disk */
	cdev_fd_init(1,fd),		/* 10: file descriptor pseudo-dev */
	cdev_bpftun_init(NBPFILTER,bpf),/* 11: Berkeley packet filter */
	cdev_tape_init(NST,st),		/* 12: SCSI tape */
	cdev_disk_init(NCD,cd),		/* 13: SCSI CD-ROM */
	cdev_ch_init(NCH,ch),		/* 14: SCSI autochanger */
	cdev_tty_init(NSCC,scc),	/* 15: scc 8530 serial interface */
	cdev_lkm_init(NLKM,lkm),	/* 16: loadable module driver */
	cdev_lkm_dummy(),		/* 17: reserved for LKM */
	cdev_lkm_dummy(),		/* 18: reserved for LKM */
	cdev_lkm_dummy(),		/* 19: reserved for LKM */
	cdev_lkm_dummy(),		/* 20: reserved for LKM */
	cdev_lkm_dummy(),		/* 21: reserved for LKM */
	cdev_lkm_dummy(),		/* 22: reserved for LKM */
};
int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

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
dev_t	swapdev = makedev(1, 0);

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
iskmemdev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && minor(dev) < 2);
}

/*
 * Returns true if dev is /dev/zero.
 */
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
	/*  3 */	NODEV,
	/*  4 */	NODEV,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	NODEV,
	/*  8 */	8,
	/*  9 */	9,
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	3,
	/* 14 */	NODEV,
	/* 15 */	NODEV,
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	NODEV,
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
	/* 22 */	NODEV,
};

/*
 * Convert a character device number to a block device number.
 */
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
