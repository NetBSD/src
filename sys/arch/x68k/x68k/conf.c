/*	$NetBSD: conf.c,v 1.26.2.4 2002/06/24 07:05:00 jdolecek Exp $	*/

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

#include "opt_compat_svr4.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>

bdev_decl(sw);
#include "fd.h"
bdev_decl(fd);
#include "sd.h"
bdev_decl(sd);
#include "cd.h"
bdev_decl(cd);
#include "ccd.h"
bdev_decl(ccd);
#include "raid.h"
bdev_decl(raid);
#include "vnd.h"
bdev_decl(vnd);
#include "st.h"
bdev_decl(st);
#include "md.h"
bdev_decl(md);

struct bdevsw	bdevsw[] =
{
	bdev_notdef(),			/* 0: */
	bdev_notdef(),			/* 1: */
	bdev_disk_init(NFD,fd),		/* 2: floppy diskette */
	bdev_swap_init(1,sw),		/* 3: swap pseudo-device */
	bdev_disk_init(NSD,sd),		/* 4: SCSI disk */
	bdev_tape_init(NST,st),		/* 5: SCSI tape */
	bdev_disk_init(NVND,vnd),	/* 6: vnode disk driver */
	bdev_disk_init(NCD,cd),		/* 7: SCSI CD-ROM */
	bdev_disk_init(NMD,md),		/* 8: memory disk */
	bdev_lkm_dummy(),		/* 9 */
	bdev_lkm_dummy(),		/* 10 */
	bdev_lkm_dummy(),		/* 11 */
	bdev_lkm_dummy(),		/* 12 */
	bdev_lkm_dummy(),		/* 13 */
	bdev_lkm_dummy(),		/* 14 */
	bdev_disk_init(NCCD,ccd),	/* 15: concatenated disk driver */
	bdev_disk_init(NRAID,raid),	/* 16: RAIDframe disk driver */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

/* open, close, write, ioctl */
#define	cdev_par_init(c,n)	cdev__ocwi_init(c,n)

/* open, close, ioctl */
#define	cdev_sram_init(c,n)	cdev__oci_init(c,n)
#define	cdev_pow_init(c,n)	cdev__oci_init(c,n)
#define	cdev_bell_init(c,n)	cdev__oci_init(c,n)

#include "isdn.h"
#include "isdnctl.h"
#include "isdntrc.h"
#include "isdnbchan.h"
#include "isdntel.h"
cdev_decl(isdn);
cdev_decl(isdnctl);
cdev_decl(isdntrc);
cdev_decl(isdnbchan);
cdev_decl(isdntel);

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
cdev_decl(sd);
#include "ss.h"
cdev_decl(ss);
cdev_decl(cd);
#include "grf.h"
cdev_decl(grf);
#include "par.h"
cdev_decl(par);
#include "ite.h"
cdev_decl(ite);
cdev_decl(ccd);
cdev_decl(raid);
cdev_decl(vnd);
cdev_decl(md);
cdev_decl(st);
cdev_decl(fd);
#include "kbd.h"
cdev_decl(kbd);
#include "ms.h"
cdev_decl(ms);
dev_decl(filedesc,open);
#include "audio.h"
cdev_decl(audio);
#include "sram.h"
cdev_decl(sram);
#include "bpfilter.h"
cdev_decl(bpf);
#include "tun.h"
cdev_decl(tun);

#include "xcom.h"
cdev_decl(com);
#include "zstty.h"
cdev_decl(zs);
#include "pow.h"
cdev_decl(pow);
#include "bell.h"
cdev_decl(bell);
#include "ch.h"
cdev_decl(ch);
#include "uk.h"
cdev_decl(uk);
#include "clockctl.h"
cdev_decl(clockctl);
#include "ipfilter.h"
#include "rnd.h"

#include "scsibus.h"
cdev_decl(scsibus);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1,sw),		/* 3: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_disk_init(NMD,md),		/* 7: memory disk */
	cdev_disk_init(NSD,sd),		/* 8: SCSI disk */
	cdev_disk_init(NCD,cd),		/* 9: SCSI cdrom */
	cdev_grf_init(NGRF,grf),	/* 10: frame buffer */
	cdev_par_init(NPAR,par),	/* 11: parallel interface */
	cdev_tty_init(NZSTTY,zs),	/* 12: zs serial */
	cdev_tty_init(NITE,ite),	/* 13: console terminal emulator */
#if NKBD > 0
	cdev__ocrwip_init(1,kbd),	/* 14: /dev/kbd */
#else
	cdev_notdef(),
#endif
#if NMS > 0
	cdev__ocrwip_init(1,ms),	/* 15: /dev/mouse */
#else
	cdev_notdef(),
#endif
	cdev_tty_init(NXCOM,com),	/* 16: serial port */
	cdev_audio_init(NAUDIO,audio),	/* 17: /dev/adpcm /dev/pcm /dev/audio */
	cdev_disk_init(NFD,fd),		/* 18: floppy disk */
	cdev_disk_init(NVND,vnd),	/* 19: vnode disk driver */
	cdev_tape_init(NST,st),		/* 20: SCSI tape */
	cdev_fd_init(1,filedesc),	/* 21: file descriptor pseudo-dev */
	cdev_bpftun_init(NBPFILTER,bpf),/* 22: berkeley packet filter */
	cdev_sram_init(NSRAM,sram),	/* 23: /dev/sram */
	cdev_lkm_init(NLKM,lkm),	/* 24: loadable module driver */
	cdev_lkm_dummy(),		/* 25 */
	cdev_lkm_dummy(),		/* 26 */
	cdev_lkm_dummy(),		/* 27 */
	cdev_lkm_dummy(),		/* 28 */
	cdev_lkm_dummy(),		/* 29 */
	cdev_lkm_dummy(),		/* 30 */
	cdev_bpftun_init(NTUN,tun),	/* 31: network tunnel */
	cdev_pow_init(NPOW,pow),	/* 32: power switch device */
	cdev_bell_init(NBELL,bell),	/* 33: opm bell device */
	cdev_disk_init(NCCD,ccd),	/* 34: concatenated disk driver */
	cdev_scanner_init(NSS,ss),	/* 35: SCSI scanner */
	cdev_ch_init(NCH,ch),		/* 36: SCSI changer device */
	cdev_uk_init(NUK,uk),		/* 37: SCSI unknown device */
	cdev_ipf_init(NIPFILTER,ipl),	/* 38: IP filter device */
	cdev_rnd_init(NRND,rnd),	/* 39: random source pseudo-device */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 40: SCSI bus */
	cdev_disk_init(NRAID,raid),	/* 41: RAIDframe disk driver */
	cdev_svr4_net_init(NSVR4_NET,svr4_net), /* 42: svr4 net pseudo-device */
	cdev_isdn_init(NISDN, isdn),		/* 43: isdn main device */
	cdev_isdnctl_init(NISDNCTL, isdnctl),	/* 44: isdn control device */
	cdev_isdnbchan_init(NISDNBCHAN, isdnbchan),	/* 45: isdn raw b-channel access */
	cdev_isdntrc_init(NISDNTRC, isdntrc),	/* 46: isdn trace device */
	cdev_isdntel_init(NISDNTEL, isdntel),	/* 47: isdn phone device */
	cdev_clockctl_init(NCLOCKCTL, clockctl), /* 48: settimeofday driver */
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
dev_t	swapdev = makedev(3, 0);

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
	/* CHR*/	/* BLK*/	/* CHR*/	/* BLK*/
	/*  0 */	NODEV,		/*  1 */	NODEV,
	/*  2 */	NODEV,		/*  3 */	3,
	/*  4 */	NODEV,		/*  5 */	NODEV,
	/*  6 */	NODEV,		/*  7 */	8,
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
	/* 30 */	NODEV,		/* 31 */	NODEV,
	/* 32 */	NODEV,		/* 33 */	NODEV,
	/* 34 */	15,		/* 35 */	NODEV,
	/* 36 */	NODEV,		/* 37 */	NODEV,
	/* 38 */	NODEV,		/* 39 */	NODEV,
	/* 40 */	NODEV,		/* 41 */	16,
	/* 42 */	NODEV,		/* 43 */	NODEV,
	/* 44 */	NODEV,		/* 45 */	NODEV,
	/* 46 */	NODEV,		/* 47 */	NODEV,
	/* 48 */	NODEV,
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

/*
 * This entire table could be autoconfig()ed but that would mean that
 * the kernel's idea of the console would be out of sync with that of
 * the standalone boot.  I think it best that they both use the same
 * known algorithm unless we see a pressing need otherwise.
 */
#include <dev/cons.h>

#define	itecnpollc	nullcnpollc
cons_decl(ite);
cons_decl(zs);

struct	consdev constab[] = {
#if NITE > 0 && NKBD > 0
	cons_init(ite),
#endif
#if NZSTTY > 0
	cons_init(zs),
#endif
	{ 0 },
};
