/* $NetBSD: conf.c,v 1.16.2.6 1997/07/22 05:54:24 cgd Exp $ */

/*
 * Copyright Notice:
 *
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 *
 * License:
 *
 * This License applies to this software ("Software"), created
 * by Christopher G. Demetriou ("Author").
 *
 * You may use, copy, modify and redistribute this Software without
 * charge, in either source code form, binary form, or both, on the
 * following conditions:
 *
 * 1.  (a) Binary code: (i) a complete copy of the above copyright notice
 * must be included within each copy of the Software in binary code form,
 * and (ii) a complete copy of the above copyright notice and all terms
 * of this License as presented here must be included within each copy of
 * all documentation accompanying or associated with binary code, in any
 * medium, along with a list of the software modules to which the license
 * applies.
 *
 * (b) Source Code: A complete copy of the above copyright notice and all
 * terms of this License as presented here must be included within: (i)
 * each copy of the Software in source code form, and (ii) each copy of
 * all accompanying or associated documentation, in any medium.
 *
 * 2. The following Acknowledgment must be used in communications
 * involving the Software as described below:
 *
 *      This product includes software developed by
 *      Christopher G. Demetriou for the NetBSD Project.
 *
 * The Acknowledgment must be conspicuously and completely displayed
 * whenever the Software, or any software, products or systems containing
 * the Software, are mentioned in advertising, marketing, informational
 * or publicity materials of any kind, whether in print, electronic or
 * other media (except for information provided to support use of
 * products containing the Software by existing users or customers).
 *
 * 3. The name of the Author may not be used to endorse or promote
 * products derived from this Software without specific prior written
 * permission (conditions (1) and (2) above are not considered
 * endorsement or promotion).
 *
 * 4.  This license applies to: (a) all copies of the Software, whether
 * partial or whole, original or modified, and (b) your actions, and the
 * actions of all those who may act on your behalf.  All uses not
 * expressly permitted are reserved to the Author.
 *
 * 5.  Disclaimer.  THIS SOFTWARE IS MADE AVAILABLE BY THE AUTHOR TO THE
 * PUBLIC FOR FREE AND "AS IS.''  ALL USERS OF THIS FREE SOFTWARE ARE
 * SOLELY AND ENTIRELY RESPONSIBLE FOR THEIR OWN CHOICE AND USE OF THIS
 * SOFTWARE FOR THEIR OWN PURPOSES.  BY USING THIS SOFTWARE, EACH USER
 * AGREES THAT THE AUTHOR SHALL NOT BE LIABLE FOR DAMAGES OF ANY KIND IN
 * RELATION TO ITS USE OR PERFORMANCE.
 *
 * 6.  If you have a special need for a change in one or more of these
 * license conditions, please contact the Author via electronic mail to
 *
 *     cgd@NetBSD.ORG
 *
 * or via the contact information on
 *
 *     http://www.NetBSD.ORG/People/Pages/cgd.html
 */

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

#if 0
XXX Cannot do this until the DEC_XXX vs. NDEC_XXX nonsense is worked out.
#include <machine/options.h>		/* Config options headers */
#endif
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: conf.c,v 1.16.2.6 1997/07/22 05:54:24 cgd Exp $");
__KERNEL_COPYRIGHT(0, \
    "Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/vnode.h>
#include <machine/conf.h>

/* CPU support flag headers. */
#include "dec_2100_a50.h"
#include "dec_3000_300.h"
#include "dec_3000_500.h"
#include "dec_axppci_33.h"
#include "dec_eb164.h"
#include "dec_eb64plus.h"
#include "dec_kn20aa.h"
#include "dec_kn8ae.h"

/* Device support flag headers. */
#include "audio.h"
#include "bpfilter.h"
#include "ccd.h"
#include "cd.h"
#include "ch.h"
#include "com.h"
#include "fdc.h"
#include "ipfilter.h"
#include "lpt.h"
#include "md.h"
#include "pty.h"
#include "satlink.h"
#include "scc.h"
#include "sd.h"
#include "se.h"
#include "ss.h"
#include "st.h"
#include "tun.h"
#include "uk.h"
#include "vnd.h"
#include "awdc.h"
#include "wsdisplay.h"
#include "wskbd.h"
#include "wsmouse.h"


/*
 * CPU support switch table.
 */

const struct cpusw cpusw[] = {
	cpu_unknown(),				/*  0: ??? */
	cpu_notdef("Alpha Demonstration Unit"),	/*  1: ST_ADU */
	cpu_notdef("DEC 4000 (\"Cobra\")"),	/*  2: ST_DEC_4000 */
	cpu_notdef("DEC 7000 (\"Ruby\")"),	/*  3: ST_DEC_7000 */
	cpu_init("DEC 3000/500 (\"Flamingo\")",DEC_3000_500,dec_3000_500),
						/*  4: ST_DEC_3000_500 */
	cpu_unknown(),				/*  5: ??? */
	cpu_notdef("DEC 2000/300 (\"Jensen\")"),
						/*  6: ST_DEC_2000_300 */
	cpu_init("DEC 3000/300 (\"Pelican\")",DEC_3000_300,dec_3000_300),
						/*  7: ST_DEC_3000_300 */
	cpu_unknown(),				/*  8: ??? */
	cpu_notdef("DEC 2100/A500 (\"Sable\")"),
						/*  9: ST_DEC_2100_A500 */
	cpu_notdef("AXPvme 64"),		/* 10: ST_DEC_APXVME_64 */
	cpu_init("DEC AXPpci",DEC_AXPPCI_33,dec_axppci_33),
						/* 11: ST_DEC_AXPPCI_33 */
	cpu_init("AlphaServer 8400",DEC_KN8AE,dec_kn8ae),
						/* 12: ST_DEC_21000 */
	cpu_init("AlphaStation 200/400 (\"Avanti\")",DEC_2100_A50,dec_2100_a50),
						/* 13: ST_DEC_2100_A50 */
	cpu_notdef("Mustang"),			/* 14: ST_DEC_MUSTANG */
	cpu_init("AlphaStation 600 (KN20AA)",DEC_KN20AA,dec_kn20aa),
						/* 15: ST_DEC_KN20AA */
	cpu_unknown(),				/* 16: ??? */
	cpu_notdef("DEC 1000 (\"Mikasa\")"),	/* 17: ST_DEC_1000 */
	cpu_unknown(),				/* 18: ??? */
	cpu_notdef("EB66"),			/* 19: ST_EB66 */
	cpu_init("EB64+",DEC_EB64PLUS,dec_eb64plus),
						/* 20: ST_EB64P */
	cpu_unknown(),				/* 21: ??? */
	cpu_notdef("DEC 4100 (\"Rawhide\")"),	/* 22: ST_DEC_4100 */
	cpu_notdef("??? (\"Lego\")"),		/* 23: ST_DEC_EV45_PBP */
	cpu_notdef("DEC 2100A/A500 (\"Lynx\")"),
						/* 24: ST_DEC_2100A_A500 */
	cpu_unknown(),				/* 25: ??? */
	cpu_init("EB164",DEC_EB164,dec_eb164),	/* 26: ST_EB164 */
	cpu_notdef("DEC 1000A (\"Noritake\")"),	/* 27: ST_DEC_1000A */
	cpu_notdef("AlphaVME 224 (\"Cortex\")"),
						/* 28: ST_DEC_ALPHAVME_224 */
};
const int ncpusw = sizeof (cpusw) / sizeof (cpusw[0]);


/*
 * Device support switch tables.
 */

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NFDC,fd),	/* 0: PC-ish floppy disk driver */
	bdev_swap_init(1,sw),		/* 1: swap pseudo-device */
	bdev_tape_init(NST,st),		/* 2: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 3: SCSI CD-ROM */
	bdev_disk_init(NAWDC,wd),	/* 4: IDE disk driver */
	bdev_notdef(),			/* 5 */
	bdev_disk_init(NMD,md),		/* 6: memory disk driver */
	bdev_disk_init(NCCD,ccd),	/* 7: concatenated disk driver */
	bdev_disk_init(NSD,sd),		/* 8: SCSI disk driver */
	bdev_disk_init(NVND,vnd),	/* 9: vnode disk driver */
	bdev_lkm_dummy(),		/* 10 */
	bdev_lkm_dummy(),		/* 11 */
	bdev_lkm_dummy(),		/* 12 */
	bdev_lkm_dummy(),		/* 13 */
	bdev_lkm_dummy(),		/* 14 */
	bdev_lkm_dummy(),		/* 15 */
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1,sw),		/* 3: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_bpftun_init(NTUN,tun),	/* 7: network tunnel */
	cdev_disk_init(NSD,sd),		/* 8: SCSI disk */
	cdev_disk_init(NVND,vnd),	/* 9: vnode disk driver */
	cdev_fd_init(1,filedesc),	/* 10: file descriptor pseudo-dev */
	cdev_bpftun_init(NBPFILTER,bpf),/* 11: Berkeley packet filter */
	cdev_tape_init(NST,st),		/* 12: SCSI tape */
	cdev_disk_init(NCD,cd),		/* 13: SCSI CD-ROM */
	cdev_ch_init(NCH,ch),		/* 14: SCSI autochanger */
	cdev_tty_init(NSCC,scc),	/* 15: scc 8530 serial interface */
	cdev_lkm_init(NLKM,lkm),	/* 16: loadable module driver */
	cdev_lkm_dummy(),		/* 17 */
	cdev_lkm_dummy(),		/* 18 */
	cdev_lkm_dummy(),		/* 19 */
	cdev_lkm_dummy(),		/* 20 */
	cdev_lkm_dummy(),		/* 21 */
	cdev_lkm_dummy(),		/* 22 */
	cdev_tty_init(1,prom),          /* 23: XXX prom console */
	cdev_audio_init(NAUDIO,audio),	/* 24: generic audio I/O */
	cdev_wsdisplay_init(NWSDISPLAY,
	    wsdisplay),			/* 25: frame buffers, etc. */
	cdev_tty_init(NCOM,com),	/* 26: ns16550 UART */
	cdev_disk_init(NCCD,ccd),	/* 27: concatenated disk driver */
	cdev_disk_init(NMD,md),		/* 28: memory disk driver */
	cdev_mouse_init(NWSKBD, wskbd),	/* 29: keyboards */
	cdev_mouse_init(NWSMOUSE,
	    wsmouse),			/* 30: mice */
	cdev_lpt_init(NLPT,lpt),	/* 31: parallel printer */
	cdev_scanner_init(NSS,ss),	/* 32: SCSI scanner */
	cdev_uk_init(NUK,uk),		/* 33: SCSI unknown */
	cdev_disk_init(NFDC,fd),	/* 34: PC-ish floppy disk driver */
	cdev_ipf_init(NIPFILTER,ipl),	/* 35: ip-filter device */
	cdev_disk_init(NAWDC,wd),	/* 36: IDE disk driver */
	cdev_se_init(NSE,se),		/* 37: Cabletron SCSI<->Ethernet */
	cdev_satlink_init(NSATLINK,satlink), /* 38: planetconnect satlink */
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
	/*  8 */	8,		/* sd */
	/*  9 */	9,		/* vnd */
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	2,		/* st */
	/* 13 */	3,		/* cd */
	/* 14 */	NODEV,
	/* 15 */	NODEV,
	/* 16 */	NODEV,
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
	/* 27 */	7,		/* ccd */
	/* 28 */	6,		/* md */
	/* 29 */	NODEV,
	/* 30 */	NODEV,
	/* 31 */	NODEV,
	/* 32 */	NODEV,
	/* 33 */	NODEV,
	/* 34 */	0,		/* fd */
	/* 35 */	NODEV,
	/* 36 */	4,		/* wd */
	/* 37 */	NODEV,
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
