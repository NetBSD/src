/*	$NetBSD: conf.c,v 1.21.2.3 2002/04/22 22:15:38 he Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/vnode.h>

bdev_decl(sw);
#include "vnd.h"
bdev_decl(vnd);
#include "ccd.h"
bdev_decl(ccd);
#include "raid.h"
bdev_decl(raid);
#include "sd.h"
bdev_decl(sd);
#include "st.h"
bdev_decl(st);
#include "cd.h"
bdev_decl(cd);
#include "md.h"
bdev_decl(md);
#include "wd.h"
bdev_decl(wd);

struct bdevsw bdevsw[] = {
	bdev_notdef(),			/* 0: Openfirmware disk */
	bdev_swap_init(1,sw),		/* 1: swap pseudo device */
	bdev_disk_init(NVND,vnd),	/* 2: vnode disk driver */
	bdev_disk_init(NCCD,ccd),	/* 3: concatenated disk driver */
	bdev_disk_init(NSD,sd),		/* 4: SCSI disk */
	bdev_tape_init(NST,st),		/* 5: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 6: SCSI CD-ROM */
	bdev_lkm_dummy(),		/* 7 */
	bdev_lkm_dummy(),		/* 8 */
	bdev_disk_init(NMD,md),		/* 9: memory disk driver */
	bdev_disk_init(NWD,wd),		/* 10: IDE disk driver */
	bdev_lkm_dummy(),		/* 11 */
	bdev_disk_init(NRAID,raid),	/* 12: RAIDframe disk driver */
};
int nblkdev = sizeof bdevsw / sizeof bdevsw[0];

/* open, close, write, ioctl */
#define	cdev_lpt_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, \
	0, seltrue, (dev_type_mmap((*))) enodev }

cdev_decl(cn);
cdev_decl(ctty);
#define mmread	mmrw
#define	mmwrite	mmrw
cdev_decl(mm);
#include "pty.h"
#define	ptstty		ptytty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptctty		ptytty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);
cdev_decl(log);
cdev_decl(sw);
#include "bpfilter.h"
cdev_decl(bpf);
cdev_decl(raid);
#include "rnd.h"
cdev_decl(sd);
cdev_decl(st);
cdev_decl(cd);
#include "zstty.h"
cdev_decl(zs);
#include "ipfilter.h"
#include "ch.h"
cdev_decl(ch);
#include "audio.h"
cdev_decl(audio);
#include "midi.h"
cdev_decl(midi);
#include "sequencer.h"
#include "ss.h"
#include "uk.h"
#include "ite.h"
cdev_decl(ite);
#include "grf.h"
cdev_decl(grf);
#include "tun.h"
cdev_decl(tun);
cdev_decl(vnd);
cdev_decl(ccd);
#include "aed.h"
cdev_decl(aed);
cdev_decl(wd);
cdev_decl(ofc);
#include "nvram.h"
cdev_decl(nvram);
#include "cz.h"
cdev_decl(cztty);

#include "scsibus.h"
cdev_decl(scsibus);

#include "wsdisplay.h"
cdev_decl(wsdisplay);
#include "wskbd.h"
cdev_decl(wskbd);
#include "wsmouse.h"
cdev_decl(wsmouse);
#include "wsmux.h"
cdev_decl(wsmux);

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

#include "com.h"
cdev_decl(com);
#include "cy.h"
cdev_decl(cy);

struct cdevsw cdevsw[] = {
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: control tty */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_tty_init(NPTY,pts),	/* 3: pseudo tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 4: pseudo tty master */
	cdev_log_init(1,log),		/* 5: /dev/klog */
	cdev_swap_init(1,sw),		/* 6: /dev/drum pseudo device */
	cdev_notdef(),			/* 7: Openfirmware console */
	cdev_notdef(),			/* 8: Openfirmware disk */
	cdev_notdef(),			/* 9: Openfirmware RTC */
	cdev_bpftun_init(NBPFILTER,bpf),/* 10: Berkeley packet filter */
	cdev_bpftun_init(NTUN,tun),	/* 11: network tunnel */
	cdev_tty_init(NZSTTY,zs),	/* 12: Zilog 8530 serial port */
	cdev_disk_init(NSD,sd),		/* 13: SCSI disk */
	cdev_tape_init(NST,st),		/* 14: SCSI tape */
	cdev_disk_init(NCD,cd),		/* 15: SCSI CD-ROM */
	cdev_ch_init(NCH,ch),		/* 16: SCSI autochanger */
	cdev_scanner_init(NSS,ss),	/* 17: SCSI scanners */
	cdev_uk_init(NUK,uk),		/* 18: SCSI unknown */
	cdev_lkm_dummy(),		/* 19: */
	cdev_lkm_dummy(),		/* 20: */
	cdev_ipf_init(NIPFILTER,ipl),	/* 21: ip-filter device */
	cdev_tty_init(NITE,ite),	/* 22: console terminal emulator */
	cdev_fb_init(NGRF,grf),		/* 23: frame buffer */
	cdev_rnd_init(NRND,rnd),	/* 24: random source pseudo-device */
	cdev_disk_init(NVND,vnd),	/* 25: vnode disk driver */
	cdev_disk_init(NCCD,ccd),	/* 26: concatenated disk driver */
	cdev_disk_init(NMD,md),		/* 27: memory disk driver */
	cdev_mouse_init(NAED,aed),	/* 28: ADB event interface */
	cdev_lkm_dummy(),		/* 29: */
	cdev_disk_init(NWD,wd),		/* 30: IDE disk driver */
	cdev_lkm_init(NLKM,lkm),	/* 31: loadable module driver */
	cdev_fd_init(1,filedesc),	/* 32: file descriptor pseudo-device */
	cdev_mm_init(NNVRAM,nvram),	/* 33: nvram device */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 34: SCSI bus */
	cdev_wsdisplay_init(NWSDISPLAY,wsdisplay), /* 35: wsdisplay */
	cdev_mouse_init(NWSKBD,wskbd),	/* 36: wskbd */
	cdev_mouse_init(NWSMOUSE,wsmouse), /* 37: wsmouse */
	cdev_disk_init(NRAID,raid),	/* 38: RAIDframe disk driver */
	cdev_usb_init(NUSB,usb),	/* 39: USB controller */
	cdev_usbdev_init(NUHID,uhid),	/* 40: USB generic HID */
	cdev_lpt_init(NULPT,ulpt),	/* 41: USB printer */
	cdev_ugen_init(NUGEN,ugen),	/* 42: USB generic driver */
	cdev_mouse_init(NWSMUX,wsmux),  /* 43: ws multiplexor */
	cdev_tty_init(NUCOM,ucom),	/* 44: USB tty */
	cdev_tty_init(NCOM,com),	/* 45: NS16x50 compatible ports */
	cdev_tty_init(NCZ,cztty),	/* 46: Cyclades-Z serial port */
	cdev_tty_init(NCY,cy),		/* 47: Cyclom-Y serial port */
	cdev_audio_init(NAUDIO,audio),	/* 48: generic audio I/O */
	cdev_midi_init(NMIDI,midi),	/* 49: MIDI I/O */
	cdev_midi_init(NSEQUENCER,sequencer),	/* 50: sequencer I/O */
};
int nchrdev = sizeof cdevsw / sizeof cdevsw[0];

int mem_no = 2;				/* major number of /dev/mem */

/*
 * Swapdev is a fake device implemented in sw.c.
 * It is used only internally to get to swstrategy.
 */
dev_t swapdev = makedev(1, 0);

/*
 * Check whether dev is /dev/mem or /dev/kmem.
 */
int
iskmemdev(dev)
	dev_t dev;
{
	return major(dev) == mem_no && minor(dev) < 2;
}

/*
 * Check whether dev is /dev/zero.
 */
int
iszerodev(dev)
	dev_t dev;
{
	return major(dev) == mem_no && minor(dev) == 12;
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
	/*  8 */	0,
	/*  9 */	NODEV,
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	4,
	/* 14 */	NODEV,
	/* 15 */	6,
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	NODEV,
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
	/* 22 */	NODEV,
	/* 23 */	NODEV,
	/* 24 */	NODEV,
	/* 25 */	2,
	/* 26 */	3,
	/* 27 */	9,
	/* 28 */	NODEV,
	/* 29 */	NODEV,
	/* 30 */	10,
	/* 31 */	NODEV,
	/* 32 */	NODEV,
	/* 33 */	NODEV,
	/* 34 */	NODEV,
	/* 35 */	NODEV,
	/* 36 */	NODEV,
	/* 37 */	NODEV,
	/* 38 */	12,
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
	/* 50 */	NODEV,
};

/*
 * Return accompanying block dev for a char dev.
 */
dev_t
chrtoblk(dev)
	dev_t dev;
{
	int major;
	
	if ((major = major(dev)) >= nchrdev)
		return NODEV;
	if ((major = chrtoblktbl[major]) == NODEV)
		return NODEV;
	return makedev(major, minor(dev));
}
