/*	$NetBSD: conf.c,v 1.7 2002/06/17 16:33:01 christos Exp $	*/

/*
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
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

#include "opt_systrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include <machine/conf.h>

#include "wd.h"
bdev_decl(wd);
bdev_decl(sw);
#include "sd.h"
bdev_decl(sd);
#include "st.h"
bdev_decl(st);
#include "cd.h"
bdev_decl(cd);
#include "vnd.h"
bdev_decl(vnd);
#include "ccd.h"
bdev_decl(ccd);
#include "raid.h"
bdev_decl(raid);
#include "md.h"
bdev_decl(md);
#include "gdrom.h"
bdev_decl(gdrom);

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NWD,wd),		/* 0: ST506/ESDI/IDE disk */
	bdev_swap_init(1,sw),		/* 1: swap pseudo-device */
	bdev_notdef(),			/* 2 */
	bdev_notdef(),			/* 3 */
	bdev_disk_init(NSD,sd),		/* 4: SCSI disk */
	bdev_tape_init(NST,st),		/* 5: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 6: SCSI CD-ROM */
	bdev_notdef(),			/* 7 */
	bdev_lkm_dummy(),		/* 8 */
	bdev_lkm_dummy(),		/* 9 */
	bdev_lkm_dummy(),		/* 10 */
	bdev_lkm_dummy(),		/* 11 */
	bdev_lkm_dummy(),		/* 12 */
	bdev_lkm_dummy(),		/* 13 */
	bdev_disk_init(NVND,vnd),	/* 14: vnode disk driver */
	bdev_notdef(),			/* 15 */
	bdev_disk_init(NCCD,ccd),	/* 16: concatenated disk driver */
	bdev_disk_init(NMD,md),		/* 17: memory disk driver */
	bdev_disk_init(NRAID,raid),	/* 18: RAIDframe disk driver */
	bdev_disk_init(NGDROM,gdrom),   /* 19: GDROM */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

cdev_decl(cn);
cdev_decl(ctty);
#define	mmread	mmrw
#define	mmwrite	mmrw
cdev_decl(mm);
#include "wdog.h"
cdev_decl(wdog);
cdev_decl(wd);
cdev_decl(sw);
#include "pty.h"
#define	ptstty		ptytty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptctty		ptytty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);
cdev_decl(log);
#include "scif.h"
cdev_decl(scif);
cdev_decl(sd);
cdev_decl(st);
#include "ss.h"
cdev_decl(ss);
#include "uk.h"
cdev_decl(uk);
cdev_decl(cd);
#include "ch.h"
cdev_decl(ch);
dev_decl(filedesc,open);
#include "bpfilter.h"
cdev_decl(bpf);
cdev_decl(md);
#include "cy.h"
cdev_decl(cy);
#include "tun.h"
cdev_decl(tun);
cdev_decl(vnd);
#include "audio.h"
cdev_decl(audio);
cdev_decl(svr4_net);
cdev_decl(ccd);
cdev_decl(raid);
#include "vcoda.h"
cdev_decl(vc_nb_);

#include "esh.h"
cdev_decl(esh_fp);
#include "scsibus.h"
cdev_decl(scsibus);

#include "pvr.h"
#include "mkbd.h"

#include "ipfilter.h"
#include "rnd.h"

#include "wsdisplay.h"
cdev_decl(wsdisplay);
#include "wskbd.h"
cdev_decl(wskbd);
#include "wsmouse.h"
cdev_decl(wsmouse);
#include "wsmux.h"
cdev_decl(wsmux);

cdev_decl(gdrom);
#include "maple.h"
cdev_decl(maple);

#include "clockctl.h"
cdev_decl(clockctl);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_disk_init(NWD,wd),		/* 3: ST506/ESDI/IDE disk */
	cdev_swap_init(1,sw),		/* 4: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 5: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 6: pseudo-tty master */
	cdev_log_init(1,log),		/* 7: /dev/klog */
	cdev_notdef(),			/* 8 */
	cdev_notdef(),			/* 9 */
	cdev_tty_init(NSCIF,scif),	/* 10: serial with FIFO */
	cdev_notdef(),			/* 11: */
	cdev_notdef(),			/* 12: */
	cdev_disk_init(NSD,sd),		/* 13: SCSI disk */
	cdev_tape_init(NST,st),		/* 14: SCSI tape */
	cdev_disk_init(NCD,cd),		/* 15: SCSI CD-ROM */
	cdev_notdef(),			/* 16: */
	cdev_ch_init(NCH,ch),		/* 17: SCSI autochanger */
	cdev_disk_init(NCCD,ccd),	/* 18: concatenated disk driver */
	cdev_scanner_init(NSS,ss),	/* 19: SCSI scanner */
	cdev_uk_init(NUK,uk),		/* 20: SCSI unknown */
	cdev_notdef(),			/* 21 */
	cdev_fd_init(1,filedesc),	/* 22: file descriptor pseudo-device */
	cdev_bpftun_init(NBPFILTER,bpf),/* 23: Berkeley packet filter */
	cdev_disk_init(NMD,md),		/* 24: memory disk driver */
	cdev_notdef(),			/* 25 */
	cdev_notdef(),			/* 26 */
	cdev_notdef(),			/* 27 */
	cdev_lkm_init(NLKM,lkm),	/* 28: loadable module driver */
	cdev_lkm_dummy(),		/* 29 */
	cdev_lkm_dummy(),		/* 30 */
	cdev_lkm_dummy(),		/* 31 */
	cdev_lkm_dummy(),		/* 32 */
	cdev_lkm_dummy(),		/* 33 */
	cdev_lkm_dummy(),		/* 34 */
	cdev_notdef(),			/* 35 */
	cdev_notdef(),			/* 36 */
	cdev_notdef(),			/* 37 */
	cdev_notdef(),			/* 38 */
	cdev_notdef(),			/* 39 */
	cdev_bpftun_init(NTUN,tun),	/* 40: network tunnel */
	cdev_disk_init(NVND,vnd),	/* 41: vnode disk driver */
	cdev_audio_init(NAUDIO,audio),	/* 42: generic audio I/O */
	cdev_notdef(),			/* 43 */
	cdev_ipf_init(NIPFILTER,ipl),	/* 44: ip-filter device */
	cdev_notdef(),			/* 45 */
	cdev_rnd_init(NRND,rnd),	/* 46: random source pseudo-device */
	cdev_vc_nb_init(NVCODA,vc_nb_),	/* 47: coda file system psdev */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 48: SCSI bus */
	cdev_disk_init(NRAID,raid),	/* 49: RAIDframe disk driver */
	cdev_esh_init(NESH, esh_fp),	/* 50: HIPPI (esh) raw device */
	cdev_wdog_init(NWDOG,wdog),	/* 51: watchdog timer */

	cdev_wsdisplay_init(NWSDISPLAY, wsdisplay), /* 52: frame buffers, etc. */
	cdev_mouse_init(NWSKBD, wskbd), /* 53: keyboards */
	cdev_mouse_init(NWSMOUSE, wsmouse),       /* 54: mice */
	cdev_svr4_net_init(NSVR4_NET,svr4_net), /* 55: svr4 net pseudo-device */
	cdev_mouse_init(NWSMUX, wsmux),  /* 56: ws multiplexor */
	cdev_disk_init(NGDROM,gdrom),	/* 57: GDROM */
	cdev__oci_init(NMAPLE, maple),	/* 58: Maple bus */
	cdev_clockctl_init(NCLOCKCTL, clockctl),/* 59: clockctl pseudo device */
#ifdef SYSTRACE
	cdev_systrace_init(1, systrace),/* 60: system call tracing */
#else
	cdev_notdef(),			/* 60: system call tracing */
#endif
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
dev_t	swapdev = makedev(1, 0);

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
int
iskmemdev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && (minor(dev) < 2 || minor(dev) == 14));
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
	/*  7 */	NODEV,
	/*  8 */	NODEV,
	/*  9 */	NODEV,
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	4,
	/* 14 */	5,
	/* 15 */	6,
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	16,
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
	/* 22 */	NODEV,
	/* 23 */	NODEV,
	/* 24 */	17,
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
	/* 41 */	14,
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
	/* 57 */	19,
	/* 58 */	NODEV,
	/* 59 */	NODEV,
	/* 60 */	NODEV,
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

#include <dev/cons.h>

#define scifcnpollc	nullcnpollc

#if NPVR > 0
#if NWSKBD > 0
#define pvrcngetc wskbd_cngetc
#else /* NWSKBD > 0 */
static int
pvrcngetc(dev_t dev)
{
	return 0;
}
#endif /* NWSKBD > 0 */

#define pvrcnputc wsdisplay_cnputc
#define	pvrcnpollc nullcnpollc
#endif /* NPVR > 0 */

cons_decl(scif);
cons_decl(pvr);

struct consdev constab[] = {
#if NPVR > 0
	cons_init(pvr),
#endif
#if NSCIF > 0
	cons_init(scif),
#endif
	{ 0 },
};
