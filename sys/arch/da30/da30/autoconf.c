/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	from: Utah Hdr: autoconf.c 1.31 91/01/21
 *	from: @(#)autoconf.c	7.5 (Berkeley) 5/7/91
 *	$Id: autoconf.c,v 1.2 1994/06/18 12:09:42 paulus Exp $
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/conf.h>
#include <sys/dmap.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/vmparam.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <da30/da30/isr.h>
#include <da30/da30/iio.h>

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold;			/* if 1, still working on cold-start */
int	dkn;			/* # of iostat dk numbers assigned so far */
int	cpuspeed = MHZ_16;	/* relative cpu speed */
struct	isr isrqueue[NISR];

/*
 * Configuration routines for the internal I/O bus
 */
void iioattach __P((struct device *, struct device *, void *));
int  iiomatch __P((struct device *, struct cfdata *, void *));

struct iiosoftc {
    struct device	iio_dev;
};

struct cfdriver iiocd = {
    NULL, "iio", iiomatch, iioattach, DV_DULL, sizeof(struct iiosoftc), 0
};

int
iiomatch(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    return 1;
}

void
iioattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    printf("\n");
    while (config_found(self, NULL, NULL))
	;
}

void
iio_print(cf)
    struct cfdata *cf;
{
    printf(" slot %d", cf->cf_loc[0]);
    if (cf->cf_loc[1] > 0)
	printf(" ipl %d", cf->cf_loc[1]);
    printf("\n");
}

/*
 * Determine mass storage and memory configuration for a machine.
 */
configure()
{
	init_sir();
	isrinit();

	if (!config_rootfound("iio", NULL))
	    panic("autoconfig failed, no root");

#if GENERIC
	if ((boothowto & RB_ASKNAME) == 0)
	    setroot();
	setconf();
#else
	setroot();
#endif
	swapconf();
	cold = 0;
}

#if NCD > 0
#include <da30/dev/cdvar.h>

find_cdevices()
{
	register struct cddevice *cd;

	for (cd = cddevice; cd->cd_unit >= 0; cd++) {
		/*
		 * XXX
		 * Assign disk index first so that init routine
		 * can use it (saves having the driver drag around
		 * the cddevice pointer just to set up the dk_*
		 * info in the open routine).
		 */
		if (dkn < DK_NDRIVE)
			cd->cd_dk = dkn++;
		else
			cd->cd_dk = -1;
		if (cdinit(cd))
			printf("cd%d configured\n", cd->cd_unit);
		else if (cd->cd_dk >= 0) {
			cd->cd_dk = -1;
			dkn--;
		}
	}
}
#endif

isrinit()
{
	register int i;

	for (i = 0; i < NISR; i++)
		isrqueue[i].isr_forw = isrqueue[i].isr_back = &isrqueue[i];
}

void
isrlink(isr)
	register struct isr *isr;
{
	int i = ISRIPL(isr->isr_ipl);

	if (i < 0 || i >= NISR) {
		printf("bad IPL %d\n", i);
		panic("configure");
	}
	insque(isr, isrqueue[i].isr_back);
}

/*
 * Configure swap space and related parameters.
 */
swapconf()
{
	register struct swdevt *swp;
	register int nblks, swdev, i;

	for( swp = swdevt; swp->sw_dev; swp++ ){
	    swdev = major(swp->sw_dev);
	    if( bdevsw[swdev].d_psize ){
		nblks = (*bdevsw[swdev].d_psize)(swp->sw_dev);
		if( nblks <= 0 )
		    continue;	/* ignore this device */
		if( swp->sw_nblks > 0 && swp->sw_nblks < nblks )
		    nblks = swp->sw_nblks;
		swp->sw_nblks = nblks;
		printf("swap device %x: %dkB\n", swp->sw_dev, nblks / 2);
	    }
	}
	dumpconf();
}

#define	DOSWAP			/* Change swdevt and dumpdev too */
u_long	bootdev;		/* should be dev_t, but not until 32 bits */

static	char devname[][2] = {
	0,0,		/* 0 = xx */
	's','d',	/* 1 = sd */
	'w','d',	/* 2 = wd */
	0,0,		/* 3 = sw */
	'i','d',	/* 4 = id */
};

#define	PARTITIONMASK	0x7
#define	PARTITIONSHIFT	3

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
setroot()
{
	register struct hp_ctlr *hc;
	register struct hp_device *hd;
	int  majdev, mindev, unit, part, adaptor;
	dev_t temp, orootdev;
	struct swdevt *swp;

	if (boothowto & RB_DFLTROOT ||
	    (bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;
	majdev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
	if (majdev > sizeof(devname) / sizeof(devname[0]))
		return;
	adaptor = (bootdev >> B_ADAPTORSHIFT) & B_ADAPTORMASK;
	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;
	/*
	 * First, find the controller type which supports this device.
	 * Next, find the controller of that type corresponding to
	 * the adaptor number.
	 * Finally, find the device in question attached to that controller.
	 */
	/*
	 * Form a new rootdev
	 */
	mindev = (unit << PARTITIONSHIFT) + part;
	orootdev = rootdev;
	rootdev = makedev(majdev, mindev);
	/*
	 * If the original rootdev is the same as the one
	 * just calculated, don't need to adjust the swap configuration.
	 */
	if (rootdev == orootdev)
		return;

	printf("Changing root device to %c%c%d%c\n",
		devname[majdev][0], devname[majdev][1],
		mindev >> PARTITIONSHIFT, part + 'a');

#ifdef DOSWAP
	mindev &= ~PARTITIONMASK;
	for (swp = swdevt; swp->sw_dev; swp++) {
		if (majdev == major(swp->sw_dev) &&
		    mindev == (minor(swp->sw_dev) & ~PARTITIONMASK)) {
			temp = swdevt[0].sw_dev;
			swdevt[0].sw_dev = swp->sw_dev;
			swp->sw_dev = temp;
			break;
		}
	}
	if (swp->sw_dev == 0)
		return;

	/*
	 * If dumpdev was the same as the old primary swap
	 * device, move it to the new primary swap device.
	 */
	if (temp == dumpdev)
		dumpdev = swdevt[0].sw_dev;
#endif
}
