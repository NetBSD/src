/*	$NetBSD: conf.c,v 1.19 1995/02/05 04:57:06 briggs Exp $	*/

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
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*-
 *      @(#)conf.c	7.9 (Berkeley) 5/28/91
 */
/*
   ALICE
      BG 06/02/92,23:25:54
         I have modified this file to correspond loosely to our requirements.
         We need to go back through and check on a few things:
            1) can we get rid of the _notdef()'s?
            2) should we keep all the devices I left here?  (e.g. cd...)
            3) did I put the new devices in the right places?
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <dev/cons.h>

#include "ite.h"
#include "ser.h"

int	rawread		__P((dev_t, struct uio *, int));
int	rawwrite	__P((dev_t, struct uio *, int));
void	swstrategy	__P((struct buf *));
int	ttselect	__P((dev_t, int, struct proc *));

#define	dev_type_open(n)	int n __P((dev_t, int, int, struct proc *))
#define	dev_type_close(n)	int n __P((dev_t, int, int, struct proc *))
#define	dev_type_strategy(n)	void n __P((struct buf *))
#define	dev_type_ioctl(n) \
	int n __P((dev_t, u_long, caddr_t, int, struct proc *))

/* bdevsw-specific types */
#define	dev_type_dump(n)	int n ()
#define	dev_type_size(n)	int n __P((dev_t))

#define	dev_decl(n,t)	__CONCAT(dev_type_,t)(__CONCAT(n,t))

#define	dev_init(c,n,t) \
	((c > 0) ? __CONCAT(n,t) : (__CONCAT(dev_type_,t)((*))) enxio) 

/* bdevsw-specific initializations */
#define	dev_size_init(c,n)	(c > 0 ? __CONCAT(n,size) : 0)

#define	bdev_decl(n) \
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,strategy); \
	dev_decl(n,ioctl); dev_decl(n,dump); dev_decl(n,size)

#define	bdev_disk_init(c,n) { \
	dev_init(c,n,open), \
	(dev_type_close((*))) nullop, \
	dev_init(c,n,strategy), \
	dev_init(c,n,ioctl), \
	dev_init(c,n,dump), \
	dev_size_init(c,n), \
	0 }

#define	bdev_cd_init(c,n) { \
	dev_init(c,n,open), \
	(dev_type_close((*))) nullop, \
	dev_init(c,n,strategy), \
	dev_init(c,n,ioctl), \
	(dev_type_dump((*))) enodev, \
	dev_size_init(c,n), \
	0 }

#define	bdev_tape_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	dev_init(c,n,strategy), \
	dev_init(c,n,ioctl), \
	(dev_type_dump((*))) enodev, \
	0, \
	B_TAPE }

#define	bdev_swap_init() { \
	(dev_type_open((*))) enodev, \
	(dev_type_close((*))) enodev, \
	swstrategy, \
	(dev_type_ioctl((*))) enodev, \
	(dev_type_dump((*))) enodev, \
	0, \
	0 }

#define	bdev_notdef()	bdev_tape_init(0,no)
bdev_decl(no);	/* dummy declarations */

#include "st.h"
#include "sd.h"
#include "cd.h"
#include "ch.h"
#include "vnd.h"

bdev_decl(st);
bdev_decl(sd);
bdev_decl(cd);
bdev_decl(vnd);

#ifdef LKM
int	lkmenodev();
#else
#define lkmenodev	enodev
#endif

#define LKM_BDEV() { \
	(dev_type_open((*))) lkmenodev, (dev_type_close((*))) lkmenodev, \
	(dev_type_strategy((*))) lkmenodev, (dev_type_ioctl((*))) lkmenodev, \
	(dev_type_dump((*))) lkmenodev, 0, 0 }

struct bdevsw	bdevsw[] =
{
	bdev_notdef(),         		/* 0: */
	bdev_notdef(),			/* 1: */
	bdev_notdef(),         		/* 2: */
	bdev_swap_init(),		/* 3: swap pseudo-device */
	bdev_disk_init(NSD,sd),		/* 4: scsi disk */
	bdev_tape_init(NST,st),		/* 5: scsi tape */
	bdev_cd_init(NCD,cd),		/* 6: scsi CD driver */
	bdev_notdef(),        	 	/* 7: */
	bdev_disk_init(NVND,vnd),	/* 8: vnode disk driver. */
	bdev_notdef(),			/* 9: */
	LKM_BDEV(),			/* 10: Empty slot for LKM */
	LKM_BDEV(),			/* 11: Empty slot for LKM */
	LKM_BDEV(),			/* 12: Empty slot for LKM */
	LKM_BDEV(),			/* 13: Empty slot for LKM */
	LKM_BDEV(),			/* 14: Empty slot for LKM */
	LKM_BDEV(),			/* 15: Empty slot for LKM */
};

int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

/* cdevsw-specific types */
#define	dev_type_read(n)	int n __P((dev_t, struct uio *, int))
#define	dev_type_write(n)	int n __P((dev_t, struct uio *, int))
#define	dev_type_stop(n)	int n __P((struct tty *, int))
#define	dev_type_reset(n)	int n __P((int))
#define	dev_type_select(n)	int n __P((dev_t, int, struct proc *))
#define	dev_type_map(n)	int n __P(())

#define	cdev_decl(n) \
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,read); \
	dev_decl(n,write); dev_decl(n,ioctl); dev_decl(n,stop); \
	dev_decl(n,reset); dev_decl(n,select); dev_decl(n,map); \
	dev_decl(n,strategy); extern struct tty *__CONCAT(n,_tty)[]

#define	dev_tty_init(c,n)	(c > 0 ? __CONCAT(n,_tty) : 0)

/* open, read, write, ioctl, strategy */
#define	cdev_disk_init(c,n) { \
	dev_init(c,n,open), \
	(dev_type_close((*))) nullop, \
	dev_init(c,raw,read), \
	dev_init(c,raw,write), \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	seltrue, \
	(dev_type_map((*))) enodev, \
	dev_init(c,n,strategy) }

/* open, close, read, write, ioctl, strategy */
#define	cdev_tape_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	(dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	seltrue, \
	(dev_type_map((*))) enodev, \
	dev_init(c,n,strategy) }

/* open, close, read, write, ioctl, stop, tty */
#define	cdev_tty_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	dev_init(c,n,read), \
	dev_init(c,n,write), \
	dev_init(c,n,ioctl), \
	dev_init(c,n,stop), \
	(dev_type_reset((*))) nullop, \
	dev_tty_init(c,n), \
	ttselect, \
	(dev_type_map((*))) enodev, \
	0 }

#define	cdev_notdef() { \
	(dev_type_open((*))) enodev, \
	(dev_type_close((*))) enodev, \
	(dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, \
	(dev_type_ioctl((*))) enodev, \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	seltrue, \
	(dev_type_map((*))) enodev, 0 }

cdev_decl(no);			/* dummy declarations */

cdev_decl(cn);
#define cdev_cn_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	dev_init(c,n,read), \
	dev_init(c,n,write), \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, \
	0, \
	dev_init(c,n,select), \
	(dev_type_map((*))) enodev, \
	0 }

cdev_decl(ite);
#define cdev_ite_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	dev_init(c,n,read), \
	dev_init(c,n,write), \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	dev_tty_init(c,n), \
	ttselect, \
	(dev_type_map((*))) enodev, \
	0 }

cdev_decl(ctty);
/* open, read, write, ioctl, select -- XXX should be a tty */
#define	cdev_ctty_init(c,n) { \
	dev_init(c,n,open), \
	(dev_type_close((*))) nullop, \
	dev_init(c,n,read), \
	dev_init(c,n,write), \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, \
	0, \
	dev_init(c,n,select), \
	(dev_type_map((*))) enodev, \
	0 }

dev_type_read(mmrw);
/* read/write */
#define	cdev_mm_init(c,n) { \
	(dev_type_open((*))) nullop, \
	(dev_type_close((*))) nullop, \
	mmrw, \
	mmrw, \
	(dev_type_ioctl((*))) enodev, \
	(dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, \
	0, \
	seltrue, \
	(dev_type_map((*))) enodev, \
	0 }

/* read, write, strategy */
#define	cdev_swap_init(c,n) { \
	(dev_type_open((*))) nullop, \
	(dev_type_close((*))) nullop, \
	rawread, \
	rawwrite, \
	(dev_type_ioctl((*))) enodev, \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	(dev_type_select((*))) enodev, \
	(dev_type_map((*))) enodev, \
	dev_init(c,n,strategy) }

#include "pty.h"
#define	pts_tty		pt_tty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptc_tty		pt_tty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);

/* open, close, read, write, ioctl, tty, select */
#define	cdev_ptc_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	dev_init(c,n,read), \
	dev_init(c,n,write), \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, \
	dev_tty_init(c,n), \
	dev_init(c,n,select), \
	(dev_type_map((*))) enodev, \
	0 }

cdev_decl(log);
/* open, close, read, ioctl, select -- XXX should be a generic device */
#define	cdev_log_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	dev_init(c,n,read), \
	(dev_type_write((*))) enodev, \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	dev_init(c,n,select), \
	(dev_type_map((*))) enodev,\
	0 }

cdev_decl(st);
cdev_decl(sd);
/* cdev_decl(ch); */

cdev_decl(grf);
/* open, close, ioctl, select, map -- XXX should be a map device */
#define	cdev_grf_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	(dev_type_read((*))) nullop, \
	(dev_type_write((*))) nullop, \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	dev_init(c,n,select), \
	dev_init(c,n,map), \
	0 }

/* ADB driver */
cdev_decl(adb);
/* open, close, read, ioctl, select, map -- XXX should be a map device */
#define	cdev_adb_init(n) { \
	dev_init(1,n,open), \
	dev_init(1,n,close), \
	dev_init(1,n,read), \
	(dev_type_write((*))) nullop, \
	dev_init(1,n,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	dev_init(1,n,select), \
	(dev_type_reset((*))) nullop, \
	0 }

cdev_decl(ser);

cdev_decl(cd);

#define NCLOCK 0 /* #include "clock.h" */
cdev_decl(clock);
/* open, close, ioctl, map -- XXX should be a map device */
#define	cdev_clock_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	(dev_type_read((*))) nullop, \
	(dev_type_write((*))) nullop, \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	(dev_type_select((*))) nullop, \
	dev_init(c,n,map), \
	0 }

cdev_decl(vnd);

dev_type_open(fdopen);
/* open */
#define	cdev_fd_init(c,n) { \
	dev_init(c,n,open), \
	(dev_type_close((*))) enodev, \
	(dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, \
	(dev_type_ioctl((*))) enodev, \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) enodev, \
	0, \
	(dev_type_select((*))) enodev, \
	(dev_type_map((*))) enodev, \
	0 }

#include "bpfilter.h"
cdev_decl(bpf);
/* open, close, read, write, ioctl, select -- XXX should be generic device */
#define	cdev_bpf_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	dev_init(c,n,read), \
	dev_init(c,n,write), \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) enodev, \
	0, \
	dev_init(c,n,select), \
	(dev_type_map((*))) enodev, \
	0 }

#include "tun.h"
cdev_decl(tun);
/* open, close, read, write, ioctl, select -- XXX should be generic device */
#define cdev_tun_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) enodev, 0, dev_init(c,n,select), \
	(dev_type_map((*))) enodev, 0 }

#ifdef LKM
#define NLKM	1
#else
#define NLKM	0
#endif

dev_type_open(lkmopen);
dev_type_close(lkmclose);
dev_type_ioctl(lkmioctl);

#define cdev_lkm_init(c) { \
	dev_init(c,lkm,open), \
	dev_init(c,lkm,close), \
	(dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, \
	dev_init(c,lkm,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) enodev, \
	(struct tty **) NULL, \
	(dev_type_select((*))) enodev,\
	(dev_type_map((*))) enodev, \
	(dev_type_strategy((*))) NULL \
}

#define LKM_CDEV() { \
	(dev_type_open((*))) lkmenodev, \
	(dev_type_close((*))) lkmenodev, \
	(dev_type_read((*))) lkmenodev, \
	(dev_type_write((*))) lkmenodev, \
	(dev_type_ioctl((*))) lkmenodev, \
	(dev_type_stop((*))) lkmenodev, \
	(dev_type_reset((*))) nullop, \
	(struct tty **) NULL, \
	(dev_type_select((*))) seltrue, \
	(dev_type_map((*))) lkmenodev, \
	(dev_type_strategy((*))) NULL \
}

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1,sw),		/* 3: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_notdef(),			/* 7: */
	cdev_notdef(),			/* 8: */
	cdev_notdef(),			/* 9: */
	cdev_grf_init(1,grf),		/* 10: frame buffer */
	cdev_ite_init(NITE,ite),	/* 11: console terminal emulator */
	cdev_tty_init(NSER,ser),	/* 12: 2 mac serial ports -- BG*/
	cdev_disk_init(NSD,sd),		/* 13: scsi disk */
	cdev_tape_init(NST,st),		/* 14: scsi tape */
	cdev_tape_init(NCD,cd),		/* 15: scsi compact disc */
	cdev_notdef(),			/* 16: */
/*	cdev_disk_init(NCH,ch),		 17: scsi changer device */
	cdev_notdef(),			/* 17: until we find chstrategy... */
	cdev_clock_init(NCLOCK,clock),	/* 18: mapped clock */
	cdev_disk_init(NVND,vnd),	/* 19: vnode disk */
	cdev_tape_init(NST,st),		/* 20: exabyte tape */
	cdev_fd_init(1,fd),		/* 21: file descriptor pseudo-dev */
	cdev_bpf_init(NBPFILTER,bpf),	/* 22: berkeley packet filter */
	cdev_adb_init(adb),		/* 23: ADB event interface */
	cdev_tun_init(NTUN,tun),	/* 24: network tunnel */
	cdev_lkm_init(NLKM),		/* 25: loadable kernel modules pdev */
	LKM_CDEV(),			/* 26: Empty slot for LKM */
	LKM_CDEV(),			/* 27: Empty slot for LKM */
	LKM_CDEV(),			/* 28: Empty slot for LKM */
	LKM_CDEV(),			/* 29: Empty slot for LKM */
	LKM_CDEV(),			/* 30: Empty slot for LKM */
	LKM_CDEV(),			/* 31: Empty slot for LKM */
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
dev_t	swapdev = makedev(3, 0);

iszerodev(dev)
	dev_t	dev;
{
	return 0;
}

/*
 * return true is memory device (kmem or mem)
 */
int
iskmemdev(dev)
	dev_t	dev;
{
	if (major(dev) == mem_no && minor(dev) < 2)
		return (1);
	return (0);
}

/*
 * return true is a disk
 */
int
isdisk(dev, type)
	dev_t	dev;
	int	type;
{
	switch (major(dev)) {
		case 4: /* SCSI disk */
		case 6: /* SCSI CD-ROM */
		/* Floppy blk device will go here */
			if (type == VBLK)
				return (1);
			break;
		case 13: /* SCSI disk */
		case 15: /* SCSI CD-ROM */
		/* Floppy chr device will go here */
			if (type == VCHR)
				return (1);
			break;
		default:;
	}
	return (0);
}

static int chrtoblktab[] = {
	/* CHR*/	/* BLK*/	/* CHR*/	/* BLK*/
	/*  0 */	NODEV,		/*  1 */	NODEV,
	/*  2 */	NODEV,		/*  3 */	3,
	/*  4 */	NODEV,		/*  5 */	NODEV,
	/*  6 */	NODEV,		/*  7 */	NODEV,
	/*  8 */	NODEV,		/*  9 */	NODEV,
	/* 10 */	NODEV,		/* 11 */	NODEV,
	/* 12 */	NODEV,		/* 13 */	4,
	/* 14 */	5,		/* 15 */	6,
	/* 16 */	NODEV,		/* 17 */	NODEV,
	/* 18 */	NODEV,		/* 19 */	NODEV,
	/* 20 */	NODEV,		/* 21 */	NODEV,
	/* 22 */	NODEV,
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

#if NITE > 0
int	itecnprobe(), itecninit(), itecngetc(), itecnputc();
#endif
#if NSER > 0
int	sercnprobe(), sercninit(), sercngetc(), sercnputc();
#endif
void	nullcnpollc();

struct	consdev constab[] = {
#if NITE > 0
	{ itecnprobe, itecninit, itecngetc, itecnputc, nullcnpollc },
#endif
#if NSER > 0
	{ sercnprobe, sercninit, sercngetc, sercnputc, nullcnpollc },
#endif
	{ 0 },
};
