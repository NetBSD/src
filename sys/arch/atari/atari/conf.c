/*	$NetBSD: conf.c,v 1.50.2.1 2002/02/11 20:07:29 jdolecek Exp $	*/

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

#include "opt_compat_svr4.h"

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

#define	bdev_md_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,strategy), \
	dev_init(c,n,ioctl), dev_noimpl(dump,enxio), dev_size_init(c,n), 0 }

#include "vnd.h"
bdev_decl(vnd);
#include "md.h"
bdev_decl(md);

#include "fd.h"
#include "hdfd.h"
#include "fdisa.h"
#define	NFLOPPY		(NFD+NHDFD+NFDISA)
bdev_decl(fd);

bdev_decl(sw);
#include "sd.h"
bdev_decl(sd);
#include "st.h"
bdev_decl(st);
#include "cd.h"
bdev_decl(cd);
#include "ccd.h"
bdev_decl(ccd);
#include "raid.h"
bdev_decl(raid);
#include "wd.h"
bdev_decl(wd);

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NVND,vnd),	/* 0: vnode disk driver */
	bdev_md_init(NMD,md),		/* 1: memory disk - for install disk */
	bdev_disk_init(NFLOPPY,fd),	/* 2: floppy disk */
	bdev_swap_init(1,sw),		/* 3: swap pseudo-device */
	bdev_disk_init(NSD,sd),		/* 4: SCSI disk */
	bdev_tape_init(NST,st),		/* 5: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 6: SCSI CD-ROM */
	bdev_lkm_dummy(),		/* 7 */
	bdev_lkm_dummy(),		/* 8 */
	bdev_lkm_dummy(),		/* 9 */
	bdev_lkm_dummy(),		/* 10 */
	bdev_lkm_dummy(),		/* 11 */
	bdev_lkm_dummy(),		/* 12 */
	bdev_disk_init(NCCD,ccd),	/* 13: concatenated disk driver */
	bdev_disk_init(NWD,wd),		/* 14: IDE disk driver */
	bdev_disk_init(NRAID,raid),	/* 15: RAIDframe disk driver */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

/* open, close, write, ioctl */
#define	cdev_lp_init(c,n)	cdev__ocwi_init(c,n)

/* open, close, read, ioctl */
#define	cdev_ss_init(c,n)	cdev__ocri_init(c,n)

/* open, close, read, write */
#define	cdev_rtc_init(c,n)	cdev__ocrw_init(c,n)

/* open, close, read, write, ioctl, mmap */
#define cdev_et_init(c,n)	cdev__ocrwim_init(c,n)
#define cdev_leo_init(c,n)	cdev__ocrwim_init(c,n)

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

#include "audio.h"
#include "bpfilter.h"
#include "ch.h"
#include "et.h"
#include "grfcc.h"
#include "grfet.h"
#define	NGRF	(NGRFCC + NGRFET)
#include "ipfilter.h"
#include "ite.h"
#include "kbd.h"
#include "lp.h"
#include "mouse.h"
#include "pty.h"
#include "rnd.h"
#include "ser.h"
#include "ss.h"
#include "tun.h"
#include "uk.h"
#include "vga_pci.h"
#include "view.h"
#include "wsdisplay.h"
#include "zs.h"
#include "leo.h"
#include "scsibus.h"
#include "clockctl.h"

cdev_decl(audio);
cdev_decl(bpf);
cdev_decl(ccd);
cdev_decl(cd);
cdev_decl(ch);
cdev_decl(cn);
cdev_decl(ctty);
cdev_decl(fd);
 dev_decl(filedesc,open);
cdev_decl(grf);
cdev_decl(ipl);
cdev_decl(ite);
cdev_decl(kbd);
cdev_decl(log);
cdev_decl(lp);
#define	mmread	mmrw
#define	mmwrite	mmrw
cdev_decl(mm);
cdev_decl(ms);
#define	ptstty		ptytty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptctty		ptytty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);
cdev_decl(raid);
cdev_decl(rtc);
cdev_decl(sd);
cdev_decl(ser);
cdev_decl(ss);
cdev_decl(st);
cdev_decl(sw);
cdev_decl(tun);
cdev_decl(uk);
cdev_decl(view);
cdev_decl(wd);
cdev_decl(wsdisplay);
cdev_decl(zs);
cdev_decl(et);
cdev_decl(leo);
cdev_decl(scsibus);
cdev_decl(clockctl);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1,sw),		/* 3: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave	*/
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_tty_init(NZS,zs),		/* 7: 8530 SCC */
	cdev_disk_init(NSD,sd),		/* 8: SCSI disk */
	cdev_disk_init(NCD,cd),		/* 9: SCSI CD-ROM */
	cdev_tape_init(NST,st),		/* 10: SCSI tape */
	cdev_grf_init(NGRF,grf),	/* 11: frame buffer */
	cdev_tty_init(NITE,ite),	/* 12: console terminal emulator */
	cdev_view_init(NVIEW,view),	/* 13: /dev/view00 /dev/view01, ... */
	cdev_mouse_init(NKBD,kbd),	/* 14: /dev/kbd	*/
	cdev_mouse_init(NMOUSE,ms),	/* 15: /dev/mouse0 /dev/mouse1 */
	cdev_disk_init(NFD+NHDFD,fd),	/* 16: floppy disk */
	cdev_disk_init(NVND,vnd),	/* 17: vnode disk driver */
	cdev_fd_init(1,filedesc),	/* 18: file descriptor pseudo-device */
	cdev_bpftun_init(NBPFILTER,bpf),/* 19: Berkeley packet filter */
	cdev_lkm_init(NLKM,lkm),	/* 20: loadable module driver */
	cdev_lkm_dummy(),		/* 21 */
	cdev_lkm_dummy(),		/* 22 */
	cdev_lkm_dummy(),		/* 23 */
	cdev_lkm_dummy(),		/* 24 */
	cdev_lkm_dummy(),		/* 25 */
	cdev_lkm_dummy(),		/* 26 */
	cdev_disk_init(NCCD,ccd),	/* 27: concatenated disk driver */
	cdev_bpftun_init(NTUN,tun),	/* 28: network tunnel */
	cdev_lp_init(NLP, lp),		/* 29: Centronics */
	cdev_ch_init(NCH,ch),		/* 30: SCSI autochanger	*/
	cdev_uk_init(NUK,uk),		/* 31: SCSI unknown	*/
	cdev_ss_init(NSS,ss),		/* 32: SCSI scanner	*/
	cdev_rtc_init(1,rtc),		/* 33: RealTimeClock	*/
	cdev_disk_init(NWD,wd),		/* 34: IDE disk driver	*/
	cdev_tty_init(NSER,ser),	/* 35: 68901 UART	*/
	cdev_ipf_init(NIPFILTER,ipl),	/* 36: ip-filter device */
	cdev_disk_init(NMD,md),		/* 37: memory disk - for install disk */
	cdev_rnd_init(NRND,rnd),	/* 38: random source pseudo-device */
  	cdev_leo_init(NLEO,leo),	/* 39: Circad Leonardo video */
	cdev_et_init(NET,et),		/* 40: ET4000 color video */
        cdev_wsdisplay_init(NWSDISPLAY,
			wsdisplay),	/* 41: wscons placeholder	*/
  	cdev_audio_init(NAUDIO,audio),	/* 42 */
  	cdev_notdef(),			/* 43 */
	cdev_i4b_init(NI4B, i4b),		/* 44: i4b main device */
	cdev_i4bctl_init(NI4BCTL, i4bctl),	/* 45: i4b control device */
	cdev_i4brbch_init(NI4BRBCH, i4brbch),	/* 46: i4b raw b-channel access */
	cdev_i4btrc_init(NI4BTRC, i4btrc),	/* 47: i4b trace device */
	cdev_i4btel_init(NI4BTEL, i4btel),	/* 48: i4b phone device */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 49: SCSI bus */
	cdev_disk_init(NRAID,raid),	/* 50: RAIDframe disk driver */
	cdev_svr4_net_init(NSVR4_NET,svr4_net), /* 51: svr4 net pseudo-device */
	cdev_clockctl_init(NCLOCKCTL, clockctl),/* 52: clockctl pseudo device */
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

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

static int chrtoblktab[] = {
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
	/*  8 */	4,
	/*  9 */	6,
	/* 10 */	5,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	NODEV,
	/* 14 */	NODEV,
	/* 15 */	NODEV,
	/* 16 */	2,
	/* 17 */	0,
	/* 18 */	NODEV,
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
	/* 22 */	NODEV,
	/* 23 */	NODEV,
	/* 24 */	NODEV,
	/* 25 */	NODEV,
	/* 26 */	NODEV,
	/* 27 */	13,
	/* 28 */	NODEV,
	/* 29 */	NODEV,
	/* 30 */	NODEV,
	/* 31 */	NODEV,
	/* 32 */	NODEV,
	/* 33 */	NODEV,
	/* 34 */	14,
	/* 35 */	NODEV,
	/* 36 */	NODEV,
	/* 37 */	1,
	/* 38 */	NODEV,
	/* 39 */	NODEV,
	/* 40 */	NODEV,
	/* 41 */	NODEV,
	/* 42 */	NODEV,
	/* 43 */	NODEV,
	/* 44 */	NODEV,
	/* 45 */	NODEV,
	/* 46 */	NODEV,
	/* 47 */	NODEV,
	/* 48 */	NODEV,
	/* 49 */	NODEV,
	/* 50 */	15,
	/* 51 */	NODEV,
	/* 52 */	NODEV,
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
		return(NODEV);
	blkmaj = chrtoblktab[major(dev)];
	if (blkmaj == NODEV)
		return(NODEV);
	return (makedev(blkmaj, minor(dev)));
}

/*
 * This entire table could be autoconfig()ed but that would mean that
 * the kernel's idea of the console would be out of sync with that of
 * the standalone boot.  I think it best that they both use the same
 * known algorithm unless we see a pressing need otherwise.
 */
cons_decl(ser);
#define	itecnpollc	nullcnpollc
cons_decl(ite);
cons_decl(vga);

struct	consdev constab[] = {
#if NSER > 0
	cons_init(ser),
#endif
#if NITE > 0
	cons_init(ite),
#endif
#if NVGA_PCI > 0
	{ dev_init(1,vga,cnprobe), dev_init(1,vga,cninit) },
#endif
	{ 0 },
};
