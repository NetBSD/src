/*	$NetBSD: conf.c,v 1.1.1.1 1995/03/26 07:12:20 leo Exp $	*/

/*
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
#include <dev/cons.h>

#ifdef BANKEDDEVPAGER
#include <sys/bankeddev.h>
#endif

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
	(c > 0 ? __CONCAT(n,t) : (__CONCAT(dev_type_,t)((*))) enxio)

/* bdevsw-specific initializations */
#define	dev_size_init(c,n)	(c > 0 ? __CONCAT(n,size) : 0)

#define	bdev_decl(n) \
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,strategy); \
	dev_decl(n,ioctl); dev_decl(n,dump); dev_decl(n,size)


#include "vnd.h"
#include "sd.h"
#include "cd.h"
bdev_decl(vnd);
bdev_decl(sd);
bdev_decl(cd);
#define	bdev_disk_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	dev_init(c,n,strategy), \
	dev_init(c,n,ioctl), \
	dev_init(c,n,dump), \
	dev_size_init(c,n), \
	0 \
}

#include "st.h"
bdev_decl(st);
#define bdev_tape_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	dev_init(c,n,strategy), \
	dev_init(c,n,ioctl), \
	dev_init(c,n,dump), \
	0, \
	B_TAPE \
}

#include "ch.h"
bdev_decl(ch);	/* XXX not used yet */
#define	bdev_ch_init(c) { \
	dev_init(c,ch,open), \
	dev_init(c,ch,close), \
	(dev_type_strategy((*))) xxx, \
	dev_init(c,ch,ioctl), \
	(dev_type_dump((*))) enxio, \
	(dev_type_size((*))) xxx, \
	0 \
}

#include "fd.h"
bdev_decl(fd);
dev_decl(Fd,open);
dev_decl(Fd,close);
#define	bdev_floppy_init(c) { \
	dev_init(c,Fd,open), \
	dev_init(c,Fd,close), \
	dev_init(c,fd,strategy), \
	dev_init(c,fd,ioctl), \
	(dev_type_dump((*))) enxio, \
	dev_size_init(c,fd), \
	0 \
}

#include "ramd.h"
bdev_decl(rd);
dev_decl(rd,open);
#define	bdev_ramd_init(c) { \
	dev_init(c,rd,open), \
	dev_init(c,rd,close), \
	dev_init(c,rd,strategy), \
	dev_init(c,rd,ioctl), \
	(dev_type_dump((*))) enxio, \
	dev_size_init(c,rd), \
	0 \
}

#define	bdev_swap_init() { \
	(dev_type_open((*))) enodev, \
	(dev_type_close((*))) enodev, \
	swstrategy, \
	(dev_type_ioctl((*))) enodev, \
	(dev_type_dump((*))) enodev, \
	0, \
	0 \
}

#define	bdev_notdef()	bdev_tape_init(0,no)
bdev_decl(no);	/* dummy declarations */

#ifdef LKM
int lkmenodev();
#else
#define lkmenodev	enodev
#endif

#define	LKM_BDEV() { \
	(dev_type_open((*))) lkmenodev, (dev_type_close((*))) lkmenodev, \
	(dev_type_strategy((*))) lkmenodev, (dev_type_ioctl((*))) lkmenodev, \
	(dev_type_dump((*))) lkmenodev, 0, 0 }

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NVND,vnd),	/* 0 : vnode disk (swap to files) */
	bdev_ramd_init(NRAMD),		/* 1 : ramdisk (for floppy boot   */
	bdev_floppy_init(NFD),		/* 2 : floppy			  */
	bdev_swap_init(),		/* 3 : swap pseudo-device	  */
	bdev_disk_init(NSD,sd),		/* 4 : scsi disk		  */
	bdev_tape_init(NST,st),		/* 5 : exabyte tape		  */
	bdev_disk_init(NCD,cd),		/* 6 : scsi cd.			  */
	LKM_BDEV(),			/* 7 : Empty slot for LKM	  */
	LKM_BDEV(),			/* 8 : Empty slot for LKM	  */
	LKM_BDEV(),			/* 9 : Empty slot for LKM	  */
	LKM_BDEV(),			/* 10: Empty slot for LKM	  */
	LKM_BDEV(),			/* 11: Empty slot for LKM	  */
	LKM_BDEV(),			/* 12: Empty slot for LKM	  */
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
	0 \
}

cdev_decl(no);			/* dummy declarations */
#define	cdev_notdef() { \
	(dev_type_open((*))) enodev, \
	(dev_type_close((*))) enodev, \
	(dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, \
	(dev_type_ioctl((*))) enodev, \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	(dev_type_select((*))) seltrue, \
	(dev_type_map((*))) enodev, \
	0 \
}

/* disks */
cdev_decl(vnd);
cdev_decl(sd);
#define	cdev_disk_init(c,n) { \
	dev_init(c,n,open), \
	dev_init(c,n,close), \
	rawread, \
	rawwrite, \
	dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	(dev_type_select((*))) seltrue, \
	(dev_type_map((*))) enodev, \
	dev_init(c,n,strategy) \
}

/* scsi tape */
cdev_decl(st);
#define	cdev_st_init(c) { \
	dev_init(c,st,open), \
	dev_init(c,st,close), \
	(dev_type_read((*))) rawread, \
	(dev_type_write((*))) rawwrite, \
	dev_init(c,st,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	(dev_type_select((*))) seltrue, \
	(dev_type_map((*))) enodev, \
	dev_init(c,st,strategy) \
}


/* floppy disk */
cdev_decl(fd);
dev_decl(Fd, open);
dev_decl(Fd, close);
#define	cdev_floppy_init(c) { \
	dev_init(c,Fd,open), \
	dev_init(c,Fd,close), \
	dev_init(c,fd,read), \
	dev_init(c,fd,write), \
	dev_init(c,fd,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	seltrue, \
	(dev_type_map((*))) enodev, \
	dev_init(c,fd,strategy) \
}

cdev_decl(cn);
/* console */
#define	cdev_cn_init(c) { \
	dev_init(c,cn,open), \
	dev_init(c,cn,close), \
	dev_init(c,cn,read), \
	dev_init(c,cn,write), \
	dev_init(c,cn,ioctl), \
	(dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, \
	0, \
	dev_init(c,cn,select), \
	(dev_type_map((*))) enodev, \
	0 \
}

cdev_decl(ctty);
/* open, read, write, ioctl, select -- XXX should be a tty */
#define	cdev_ctty_init(c) { \
	dev_init(c,ctty,open), \
	(dev_type_close((*))) nullop, \
	dev_init(c,ctty,read), \
	dev_init(c,ctty,write), \
	dev_init(c,ctty,ioctl), \
	(dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, \
	0, \
	dev_init(c,ctty,select), \
	(dev_type_map((*))) enodev, \
	0 \
}

dev_type_read(mmrw);
/* read/write */
#define	cdev_mm_init(c) { \
	(dev_type_open((*))) nullop, \
	(dev_type_close((*))) nullop, \
	mmrw, \
	mmrw, \
	(dev_type_ioctl((*))) enodev, \
	(dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, \
	0, \
	(dev_type_select((*)))seltrue, \
	(dev_type_map((*))) enodev, \
	0 \
}

/* read, write, strategy */
#define	cdev_swap_init(c) { \
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
	dev_init(c,sw,strategy) \
}

#include "pty.h"
#define	pts_tty		pt_tty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptc_tty		pt_tty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);

/* open, close, read, write, ioctl, tty, select */
#define	cdev_ptc_init(c) { \
	dev_init(c,ptc,open), \
	dev_init(c,ptc,close), \
	dev_init(c,ptc,read), \
	dev_init(c,ptc,write), \
	dev_init(c,ptc,ioctl), \
	(dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, \
	dev_tty_init(c,ptc), \
	dev_init(c,ptc,select), \
	(dev_type_map((*))) enodev, \
	0 \
}

cdev_decl(log);
/* open, close, read, ioctl, select -- XXX should be a generic device */
#define	cdev_log_init(c) { \
	dev_init(c,log,open), \
	dev_init(c,log,close), \
	dev_init(c,log,read), \
	(dev_type_write((*))) enodev, \
	dev_init(c,log,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	dev_init(c,log,select), \
	(dev_type_map((*))) enodev, \
	0 \
}


#include "grf.h"
cdev_decl(grf);
/* open, close, ioctl, select, map -- XXX should be a map device */
#define	cdev_grf_init(c) { \
	dev_init(c,grf,open), \
	dev_init(c,grf,close), \
	(dev_type_read((*))) nullop, \
	(dev_type_write((*))) nullop, \
	dev_init(c,grf,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	dev_init(c,grf,select), \
	dev_init(c,grf,map), \
	0 \
}

#include "view.h"
cdev_decl(view);
/* open, close, ioctl, select, map -- XXX should be a map device */
#define	cdev_view_init(c) { \
	dev_init(c,view,open), \
	dev_init(c,view,close), \
	(dev_type_read((*))) nullop, \
	(dev_type_write((*))) nullop, \
	dev_init(c,view,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	0, \
	dev_init(c,view,select), \
	dev_init(c,view,map), \
	0 \
}

#include "ite.h"
cdev_decl(ite);
/* open, close, read, write, ioctl, tty -- XXX should be a tty! */
#define	cdev_ite_init(c) { \
	dev_init(c,ite,open), \
	dev_init(c,ite,close), \
	dev_init(c,ite,read), \
	dev_init(c,ite,write), \
	dev_init(c,ite,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	dev_tty_init(c,ite), \
	ttselect, \
	(dev_type_map((*))) enodev, \
	0 \
}

#include "kbd.h"
cdev_decl(kbd);
/* open, close, read, write, ioctl, select */
#define cdev_kbd_init(c) { \
	dev_init(c,kbd,open), \
	dev_init(c,kbd,close), \
	dev_init(c,kbd,read), \
	dev_init(c,kbd,write), \
	dev_init(c,kbd,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, \
	(struct tty **) NULL, \
	dev_init(c,kbd,select), \
	(dev_type_map((*))) enodev, \
	(dev_type_strategy((*))) 0 \
}

dev_type_open(fdopen);
/* open */
#define	cdev_fd_init(c) { \
	dev_init(c,fd,open), \
	(dev_type_close((*))) enodev, \
	(dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, \
	(dev_type_ioctl((*))) enodev, \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) enodev, \
	(struct tty **) NULL, \
	(dev_type_select((*))) enodev, \
	(dev_type_map((*))) enodev, \
	(dev_type_strategy((*))) NULL \
}

#include "bpfilter.h"
cdev_decl(bpf);
/* open, close, read, write, ioctl, select -- XXX should be generic device */
#define	cdev_bpf_init(c) { \
	dev_init(c,bpf,open), \
	dev_init(c,bpf,close), \
	dev_init(c,bpf,read), \
	dev_init(c,bpf,write), \
	dev_init(c,bpf,ioctl), \
	(dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) enodev, \
	(struct tty **) NULL, \
	dev_init(c,bpf,select), \
	(dev_type_map((*))) enodev, \
	(dev_type_strategy((*))) NULL \
}


#ifdef LKM
#define NLKM 1
#else
#define NLKM 0
#endif

dev_type_open(lkmopen);
dev_type_close(lkmclose);
dev_type_ioctl(lkmioctl);

#define	cdev_lkm_init(c) { \
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

#define	LKM_CDEV() { \
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

#include "zs.h"
cdev_decl(zs);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1),		/* 0 : virtual console		      */
	cdev_ctty_init(1),		/* 1 : controlling terminal	      */
	cdev_mm_init(1),		/* 2 : /dev/{null,mem,kmem,...}       */
	cdev_swap_init(1),		/* 3 : /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 4 : pseudo-tty slave		      */
	cdev_ptc_init(NPTY),		/* 5 : pseudo-tty master	      */
	cdev_log_init(1),		/* 6 : /dev/klog		      */
	cdev_tty_init(NZS,zs),		/* 7 : 8530 SCC			      */
	cdev_disk_init(NSD,sd),		/* 8 : scsi disk		      */
	cdev_disk_init(NCD,cd),		/* 9 : scsi cdrom		      */
	cdev_st_init(NST),		/* 10: scsi tape		      */
	cdev_grf_init(NGRF),		/* 11: frame buffer		      */
	cdev_ite_init(NITE),		/* 12: console terminal emulator      */
	cdev_view_init(NVIEW),		/* 13: /dev/view00 /dev/view01 ...    */
	cdev_kbd_init(NKBD),		/* 14: /dev/kbd			      */
	cdev_notdef(),			/* 15: /dev/mouse0 /dev/mouse1	      */
	cdev_floppy_init(NFD),		/* 16: floppy			      */
	cdev_disk_init(NVND,vnd),	/* 17: vnode disk		      */
	cdev_fd_init(1),		/* 18: file descriptor pseudo-dev     */
	cdev_bpf_init(NBPFILTER),	/* 19: berkeley packet filter	      */
	cdev_lkm_init(NLKM),		/* 20: loadable kernel modules pdev   */
	LKM_CDEV(),			/* 21: Empty slot for LKM	      */
	LKM_CDEV(),			/* 22: Empty slot for LKM	      */
	LKM_CDEV(),			/* 23: Empty slot for LKM	      */
	LKM_CDEV(),			/* 24: Empty slot for LKM	      */
	LKM_CDEV(),			/* 25: Empty slot for LKM	      */
	LKM_CDEV(),			/* 26: Empty slot for LKM	      */
};

int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

#ifdef BANKEDDEVPAGER
extern int grfbanked_get __P((int, int, int));
extern int grfbanked_set __P((int, int));
extern int grfbanked_cur __P((int));

struct bankeddevsw bankeddevsw[sizeof (cdevsw) / sizeof (cdevsw[0])] = {
  { 0, 0, 0 },						/* 0 */
  { 0, 0, 0 },						/* 1 */
  { 0, 0, 0 },						/* 2 */
  { 0, 0, 0 },						/* 3 */
  { 0, 0, 0 },						/* 4 */
  { 0, 0, 0 },						/* 5 */
  { 0, 0, 0 },						/* 6 */
  { 0, 0, 0 },						/* 7 */
  { 0, 0, 0 },						/* 8 */
  { 0, 0, 0 },						/* 9 */
  { grfbanked_get, grfbanked_cur, grfbanked_set },	/* 10 */
  /* rest { 0, 0, 0 } */
};
#endif

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
 * return true if memory device (kmem or mem)
 */
int
iskmemdev(dev)
dev_t dev;
{
	if (major(dev) == mem_no && minor(dev) < 2)
		return(1);
	return(0);
}

/*
 * return true if /dev/zero
 */
int
iszerodev(dev)
	dev_t dev;
{
	if (major(dev) == mem_no && minor(dev) == 12)
		return(1);
	return(0);
}

/*
 * return true if a disk
 */
int
isdisk(dev, type)
	dev_t dev;
	int type;
{
	switch (major(dev)) {
	case 2:	/* fd: floppy */
	case 4:	/* sd: scsi disk */
	case 7:	/* cd: scsi cd-rom */
		if (type == VBLK)
			return(1);
		break;
	case 8:	 /* sd: scsi disk */
	case 9:  /* cd: scsi cdrom */
	case 18: /* fd: floppy */
		if (type == VCHR)
			return(1);
		break;
	}
	return(0);
}

static int chrtoblktab[] = {
	/* CHR*/	/* BLK*/	/* CHR*/	/* BLK*/
	/*  0 */	NODEV,		/*  1 */	NODEV,
	/*  2 */	NODEV,		/*  3 */	3,
	/*  4 */	NODEV,		/*  5 */	NODEV,
	/*  6 */	NODEV,		/*  7 */	NODEV,
	/*  8 */	4,		/*  9 */	7,
	/* 10 */	NODEV,		/* 11 */	NODEV,
	/* 12 */	NODEV,		/* 13 */	NODEV,
	/* 14 */	NODEV,		/* 15 */	NODEV,
	/* 16 */	NODEV,		/* 17 */	NODEV,
	/* 18 */	2,		/* 19 */	6,
	/* 20 */	5,		/* 21 */	NODEV,
	/* 22 */	NODEV,		/* 23 */	NODEV,
	/* 24 */	NODEV,		/* 25 */	NODEV,
	/* 26 */	NODEV,		/* 27 */	NODEV,
	/* 28 */	NODEV,		/* 29 */	NODEV,
	/* 30 */	NODEV,
};

/*
 * convert chr dev to blk dev
 */
dev_t
chrtoblk(dev)
	dev_t dev;
{
	int blkmaj;

	if (major(dev) >= nchrdev)
		return(NODEV);
	blkmaj = chrtoblktab[major(dev)];
	if (blkmaj == NODEV)
		return(NODEV);
	return(makedev(blkmaj, minor(dev)));
}


/*
 * console capable devices entry points.
 */
int sercnprobe(), sercninit(), sercngetc(), sercnputc();
int ite_cnprobe(), ite_cninit(), ite_cngetc(), ite_cnputc();

struct	consdev constab[] = {
#if NSER > 0
	{ sercnprobe, sercninit, sercngetc, sercnputc, (void(*))nullop },
#endif
#if NITE > 0
	{ ite_cnprobe, ite_cninit, ite_cngetc, ite_cnputc, (void(*))nullop },
#endif
	{ 0 },
};
