/* $NetBSD: conf.c,v 1.6 2002/07/04 23:24:43 lukem Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: conf.c,v 1.6 2002/07/04 23:24:43 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>

bdev_decl(sw);
#include "st.h"
bdev_decl(st);
#include "cd.h"
bdev_decl(cd);
#include "sd.h"
bdev_decl(sd);
#include "vnd.h"
bdev_decl(vnd);
#include "ccd.h"
bdev_decl(ccd);
#include "md.h"
bdev_decl(md);
#include "raid.h"
bdev_decl(raid);

struct bdevsw	bdevsw[] =
{
	bdev_notdef(),			/* 0 */
	bdev_swap_init(1,sw),		/* 1: swap pseudo-device */
	bdev_disk_init(NSD,sd),		/* 2: SCSI disk */
	bdev_tape_init(NST,st),		/* 3: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 4: SCSI CD-ROM */
	bdev_disk_init(NCCD,ccd),	/* 5: concatenated disk driver */
	bdev_disk_init(NVND,vnd),	/* 6: vnode disk driver */
	bdev_disk_init(NMD,md),		/* 7: memory disk - for install tape */
	bdev_lkm_dummy(),		/* 8 */
	bdev_lkm_dummy(),		/* 9 */
	bdev_lkm_dummy(),		/* 10 */
	bdev_lkm_dummy(),		/* 11 */
	bdev_lkm_dummy(),		/* 12 */
	bdev_lkm_dummy(),		/* 13 */
	bdev_disk_init(NRAID,raid), 	/* 14: RAIDframe disk driver */
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

cdev_decl(cn);
cdev_decl(ctty);
#define	mmread  mmrw
#define	mmwrite mmrw
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
#include "tun.h"
cdev_decl(tun);
cdev_decl(sd);
cdev_decl(vnd);
cdev_decl(ccd);
dev_type_open(filedescopen);
#include "bpfilter.h"
cdev_decl(bpf);
cdev_decl(st);
cdev_decl(cd);
#include "ch.h"
cdev_decl(ch);
cdev_decl(md);
#include "ss.h"
cdev_decl(ss);
#include "uk.h"
cdev_decl(uk);
cdev_decl(fd);
#include "ipfilter.h"
cdev_decl(ipl);
#include "siotty.h"
cdev_decl(sio);

#include "wsdisplay.h"
cdev_decl(wsdisplay);
#include "wskbd.h"
cdev_decl(wskbd);
#include "wsmouse.h"
cdev_decl(wsmouse);

#include "scsibus.h"
cdev_decl(scsibus);

cdev_decl(raid);

#include "wsmux.h"
cdev_decl(wsmux);

#include "rnd.h"

#include "clockctl.h"
cdev_decl(clockctl);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1,sw),		/* 3: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_tty_init(NSIOTTY,sio),	/* 7: ttya */
	cdev_disk_init(NSD,sd),		/* 8: SCSI disk */
	cdev_tape_init(NST,st),		/* 9: SCSI tape */
	cdev_disk_init(NCD,cd),		/* 10: SCSI CD-ROM */
	cdev_ch_init(NCH,ch),		/* 11: SCSI autochanger */
	cdev_scanner_init(NSS,ss),	/* 12: SCSI scanner */
	cdev_uk_init(NUK,uk),		/* 13: SCSI unknown */
	cdev_disk_init(NCCD,ccd),	/* 14: concatenated disk driver */
	cdev_disk_init(NVND,vnd),	/* 15: vnode disk */
	cdev_disk_init(NMD,md),		/* 16: memory disk driver */
	cdev_mouse_init(NWSKBD, wskbd),	/* 17: keyboards */
	cdev_mouse_init(NWSMOUSE,
	    wsmouse),			/* 18: mice */
	cdev_wsdisplay_init(NWSDISPLAY,
	    wsdisplay),			/* 19: frame buffers, etc. */
	cdev_fd_init(1,filedesc),	/* 20: file descriptor pseudo-dev */
	cdev_bpftun_init(NBPFILTER,bpf),/* 21: berkeley packet filter */
	cdev_bpftun_init(NTUN,tun),	/* 22: network tunnel */
	cdev_ipf_init(NIPFILTER,ipl),	/* 23: ip-filter device */
	cdev_lkm_init(NLKM,lkm),	/* 24: loadable module driver */
	cdev_lkm_dummy(),		/* 25 */
	cdev_lkm_dummy(),		/* 26 */
	cdev_lkm_dummy(),		/* 27 */
	cdev_lkm_dummy(),		/* 28 */
	cdev_lkm_dummy(),		/* 29 */
	cdev_lkm_dummy(),		/* 30 */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 31: SCSI bus */
	cdev_disk_init(NRAID,raid),	/* 32: RAIDframe disk driver */
	cdev_mouse_init(NWSMUX,wsmux),	/* 33: ws multiplexor */
	cdev_rnd_init(NRND,rnd),	/* 34: random source pseudo-device */
	cdev_clockctl_init(NCLOCKCTL, clockctl),/* 35: clockctl pseudo device */
#ifdef SYSTRACE
	cdev_systrace_init(1, systrace),/* 36: system call tracing */
#else
	cdev_notdef(),			/* 36: system call tracing */
#endif
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
	/*VCHR*/	/*VBLK*/
	/*  0 */	NODEV,
	/*  1 */	NODEV,
	/*  2 */	NODEV,
	/*  3 */	NODEV,
	/*  4 */	NODEV,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	NODEV,
	/*  8 */	2,	/* sd */
	/*  9 */	3,	/* st */
	/* 10 */	4,	/* cd */
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	NODEV,
	/* 14 */	5,	/* ccd */
	/* 15 */	6,	/* vnd */
	/* 16 */	7,	/* md */
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
	/* 32 */	14,	/* raid */
	/* 33 */	NODEV,
	/* 34 */	NODEV,
	/* 35 */	NODEV,
	/* 36 */	NODEV,
};

/*
 * Convert a character device number to a block device number.
 */
dev_t
chrtoblk(dev)
	dev_t dev;
{
	int blkmaj;

	if (major(dev) >= nchrdev 
	  || major(dev) >= (sizeof(chrtoblktbl) / sizeof(chrtoblktbl[0])))
		return (NODEV);
	blkmaj = chrtoblktbl[major(dev)];
	if (blkmaj == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}
