/*	$NetBSD: conf.c,v 1.11 2001/01/14 11:17:29 martin Exp $	*/

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

bdev_decl(sw);
#include "fdc.h"
bdev_decl(fd);
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

struct bdevsw	bdevsw[] =
{
	bdev_swap_init(1,sw),		/* 0: swap pseudo-device */
	bdev_disk_init(NWD,wd),		/* 1: IDE disk driver */
	bdev_disk_init(NFDC,fd),	/* 2: PC-ish floppy disk driver */
	bdev_disk_init(NSD,sd),		/* 3: SCSI disk driver */
	bdev_tape_init(NST,st),		/* 4: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 5: SCSI CD-ROM */
	bdev_disk_init(NMD,md),		/* 6: memory disk driver */
	bdev_disk_init(NCCD,ccd),	/* 7: concatenated disk driver */
	bdev_disk_init(NVND,vnd),	/* 8: vnode disk driver */
	bdev_disk_init(NRAID,raid),	/* 9: RAIDframe disk driver */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

/* open, close, write, ioctl */
#define	cdev_lpt_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, \
	0, seltrue, (dev_type_mmap((*))) enodev }

/*
 * Swapdev is a fake block device implemented  in sw.c and only used 
 * internally to get to swstrategy.  It cannot be provided to the
 * users, because the swstrategy routine munches the b_dev and b_blkno
 * entries before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines.   User access (e.g., for libkvm
 * and ps) is provided through the /dev/drum character (raw) device.
 */
dev_t	swapdev = makedev(0, 0);

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
#include "biconsdev.h"
cdev_decl(biconsdev);

cdev_decl(fd);
cdev_decl(wd);
cdev_decl(sd);
cdev_decl(st);
cdev_decl(cd);
cdev_decl(md);
cdev_decl(raid);
cdev_decl(vnd);
cdev_decl(ccd);
#include "tun.h"
cdev_decl(tun);
#include "bpfilter.h"
cdev_decl(bpf);
#include "ipfilter.h"
cdev_decl(ipl);

#include "com.h"
cdev_decl(com);
#include "tx39uart.h"
cdev_decl(txcom);
#include "ucbsnd.h"
cdev_decl(ucbsnd);
#if notyet
#include "lpt.h"
cdev_decl(lpt);
#endif

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

/* USB */
#include "usb.h"
cdev_decl(usb); 
#include "uhid.h"
cdev_decl(uhid);
#include "ugen.h"
cdev_decl(ugen);
#include "ulpt.h"
cdev_decl(ulpt);
#include "ucom.h"
cdev_decl(ucom);
#include "urio.h"
cdev_decl(urio);
#include "uscanner.h"
cdev_decl(uscanner);

/* open, close, ioctl */
#define cdev_i4bctl_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, 0, seltrue, \
	(dev_type_mmap((*))) enodev }

/* open, close, read, write, poll */
#define	cdev_i4brbch_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, \
	0, dev_init(c,n,poll), (dev_type_mmap((*))) enodev }

/* open, close, read, write, poll */
#define	cdev_i4btel_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), (dev_type_ioctl((*))) enodev, \
	(dev_type_stop((*))) enodev, \
	0, dev_init(c,n,poll), (dev_type_mmap((*))) enodev, D_TTY }

/* open, close, read, ioctl */
#define cdev_i4btrc_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, 0, (dev_type_poll((*))) enodev, \
	(dev_type_mmap((*))) enodev }

/* open, close, read, ioctl, poll */
#define cdev_i4b_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, 0, dev_init(c,n,poll), \
	(dev_type_mmap((*))) enodev }	

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

#include "rnd.h"

struct cdevsw	cdevsw[] =
{
	cdev_mm_init(1, mm),            /*  0: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1, sw),          /*  1: /dev/drum (swap pseudo-device) */
	cdev_cn_init(1, cn),            /*  2: virtual console */
	cdev_ctty_init(1,ctty),         /*  3: controlling terminal */
	cdev_fd_init(1,filedesc),	/*  4: file descriptor pseudo-dev */
	cdev_log_init(1,log),           /*  5: /dev/klog */
	cdev_ptc_init(NPTY,ptc),        /*  6: pseudo-tty master */
	cdev_tty_init(NPTY,pts),        /*  7: pseudo-tty slave */
	cdev_tty_init(NCOM,com),	/*  8: serial port */
#if notyet
	cdev_lpt_init(NLPT,lpt),        /*  9: parallel printer */
#else
	cdev_notdef(),
#endif
	cdev_disk_init(NWD, wd),        /* 10: ST506/ESDI/IDE disk */
	cdev_disk_init(NFDC, fd),       /* 11: floppy diskette */
	cdev_disk_init(NMD, md),        /* 12: memory disk driver */
	cdev_disk_init(NCCD,ccd),	/* 13: concatenated disk driver */
	cdev_disk_init(NVND,vnd),       /* 14: vnode disk driver */
	cdev_disk_init(NRAID,raid),	/* 15: RAIDframe disk driver */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 16: SCSI bus */
	cdev_disk_init(NSD,sd),		/* 17: SCSI disk */
	cdev_tape_init(NST,st),		/* 18: SCSI tape */
	cdev_disk_init(NCD,cd),		/* 19: SCSI CD-ROM */
	cdev_ch_init(NCH,ch),	 	/* 20: SCSI autochanger */
	cdev_uk_init(NUK,uk),	 	/* 21: SCSI unknown */
	cdev_scanner_init(NSS,ss),	/* 22: SCSI scanner */
	cdev_usb_init(NUSB,usb),	/* 23: USB controller */
	cdev_usbdev_init(NUHID,uhid),	/* 24: USB generic HID */
	cdev_lpt_init(NULPT,ulpt),	/* 25: USB printer */
	cdev_ugen_init(NUGEN,ugen),	/* 26: USB generic driver */
	cdev_ipf_init(NIPFILTER,ipl),	/* 27: ip-filter device */
	cdev_bpftun_init(NTUN,tun),	/* 28: network tunnel */
	cdev_bpftun_init(NBPFILTER,bpf),/* 29: Berkeley packet filter */
	cdev_wsdisplay_init(NWSDISPLAY,
	    wsdisplay),			/* 30: frame buffers, etc. */
	cdev_mouse_init(NWSKBD, wskbd),	/* 31: keyboards */
	cdev_mouse_init(NWSMOUSE,
	    wsmouse),			/* 32: mice */
	cdev_rnd_init(NRND,rnd),	/* 33: random source pseudo-device */
	cdev_tty_init(NBICONSDEV,biconsdev),	/* 34: bicons pseudo-dev */
	cdev_tty_init(NTX39UART,txcom),	/* 35: TX39 internal UART */
	cdev_audio_init(NUCBSND,ucbsnd),/* 36: UCB1200 Codec (TX39 companion chip) */
	cdev_tty_init(NUCOM, ucom),	/* 37: USB tty */
	cdev_mouse_init(NWSMUX,	wsmux), /* 38: ws multiplexor */
	cdev_usbdev_init(NURIO,urio),	/* 39: Diamond Rio 500 */
	cdev_ugen_init(NUSCANNER,uscanner),/* 40: USB scanner */
	cdev_i4b_init(NI4B, i4b),	/* 41: i4b main device */
	cdev_i4bctl_init(NI4BCTL, i4bctl),	/* 42: i4b control device */
	cdev_i4brbch_init(NI4BRBCH, i4brbch),	/* 43: i4b raw b-channel access */
	cdev_i4btrc_init(NI4BTRC, i4btrc),	/* 44: i4b trace device */
	cdev_i4btel_init(NI4BTEL, i4btel),	/* 45: i4b phone device */
};

int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int	mem_no = 0; 	/* major device number of memory special file */

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
	/*  4 */	NODEV,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	NODEV,
	/*  8 */	NODEV,
	/*  9 */	NODEV,
	/* 10 */	1, /* wd */
	/* 11 */	2, /* fd */
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
	/* 41 */	NODEV,
	/* 42 */	NODEV,
	/* 43 */	NODEV,
	/* 44 */	NODEV,
	/* 45 */	NODEV,
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
