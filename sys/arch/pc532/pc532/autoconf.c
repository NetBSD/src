/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)autoconf.c	7.1 (Berkeley) 5/9/91
 *
 *	$Id: autoconf.c,v 1.7 1994/06/26 15:00:39 phil Exp $
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba 
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/buf.h>
/* #include <sys/sl.h> */

#ifdef CONFIG_NEW
#include <sys/device.h>
#else
#include <dev/device.h>
#endif

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */

int	dkn;		/* number of iostat dk numbers assigned so far */
extern int	cold;		/* cold start flag initialized in locore.s */

/*
 * Determine i/o configuration for a machine.
 */
configure()
{
	/* Find out what the hardware configuration looks like! */
	pc532_configure();

	/* select the root device */
	setroot();

	/*
	 * Configure swap area and related system
	 * parameter based on device(s) used.
	 */
	swapconf(); 
	cold = 0;
}

/*
 * Configure swap space and related parameters.
 */

swapconf()
{
	register struct swdevt *swp;
	register int nblks;
	extern int Maxmem;


	for (swp = swdevt; swp->sw_dev > 0; swp++)
	{
		unsigned d = major(swp->sw_dev);

		if (d >= nblkdev) break;
		if (bdevsw[d].d_psize) {
			nblks = (*bdevsw[d].d_psize)(swp->sw_dev);
			if (nblks > 0 &&
			    (swp->sw_nblks == 0 || swp->sw_nblks > nblks))
				swp->sw_nblks = nblks;
			else
				swp->sw_nblks = 0;
		}
		swp->sw_nblks = ctod(dtoc(swp->sw_nblks));

	}
#if 0
	if (dumplo == 0 && bdevsw[major(dumpdev)].d_psize)
	/*dumplo = (*bdevsw[major(dumpdev)].d_psize)(dumpdev) - physmem;*/
		dumplo = (*bdevsw[major(dumpdev)].d_psize)(dumpdev) -
			Maxmem*NBPG/512;
	if (dumplo < 0)
#endif
		dumplo = 0;
}

#define	DOSWAP			/* change swdevt and dumpdev */
u_long	bootdev = 0;		/* should be dev_t, but not until 32 bits */

static	char devname[][2] = {
	's','d',	/* 0 = sd */
	's','w',	/* 1 = sw */
	's','t',	/* 2 = st */
	'r','d',	/* 3 = rd */
};

#define	PARTITIONMASK	0x7
#define	PARTITIONSHIFT	3

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */

extern int _boot_flags;
	/* 0 => no flags */

setroot()
{
#if 1
	int  majdev, mindev;
	char ans;
	dev_t temp;

	if (_boot_flags & 0x2) {
	    boothowto |= RB_SINGLE;
	}
	if (_boot_flags & 0x1) {
	    printf ("root dev (0=>/dev/rd, 1=>/dev/sd0a, 2=>/dev/sd1a) ?");
	    ans = cngetc();
	    printf ("%c\n", ans);
	    if (ans < '0' || ans > '2') {
		printf ("bad root dev, using config root dev \n");
		return;
	    }
	    if (ans == '0') {
		rootdev = makedev(2,0);
	    } else if (ans == '1') {
		rootdev = makedev(0,0);
	    } else {
		rootdev = makedev(0,8);
	    }
	}
#else
	int  majdev, mindev, unit, part, adaptor;
	dev_t temp, orootdev;
	struct swdevt *swp;

	printf("howto %x bootdev %x ", boothowto, bootdev);
	if (boothowto & RB_DFLTROOT ||
	    (bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;
	majdev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
	if (majdev > sizeof(devname) / sizeof(devname[0]))
		return;
	adaptor = (bootdev >> B_ADAPTORSHIFT) & B_ADAPTORMASK;
	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;
	mindev = (unit << PARTITIONSHIFT) + part;
	orootdev = rootdev;
	rootdev = makedev(majdev, mindev);
	/*
	 * If the original rootdev is the same as the one
	 * just calculated, don't need to adjust the swap configuration.
	 */
	if (rootdev == orootdev)
		return;
	printf("changing root device to %c%c%d%c\n",
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
#endif
}

#ifndef CONFIG_NEW

pc532_configure()
{
	struct pc532_device *dvp;
	struct pc532_driver *dp;
	int num;

	splhigh();  /* Just to make sure. */
	num = 0;
	for (dvp = pc532_devtab_tty; config_dev(dvp,&num); dvp++);
	if (num) printf ("\n");
	num = 0;
	for (dvp = pc532_devtab_bio; config_dev(dvp,&num); dvp++);
	if (num) printf ("\n");
	num = 0;
	for (dvp = pc532_devtab_net; config_dev(dvp,&num); dvp++);
	if (num) printf ("\n");
}

/*
 * Configure a pc532 device.
 */
config_dev(struct pc532_device *dp, int *num)
{
	struct pc532_driver *driv;
 
	if (driv = dp->pd_driver) {
		dp->pd_alive = (*driv->probe)(dp);
		if (dp->pd_alive)
			(*driv->attach)(dp);
		return (1);
	} else	return(0);
}

#else

pc532_configure()
{
  extern struct cfdata *cfdata;
  struct cfdata *data = cfdata;

  startrtclock();

  while (data->cf_driver != 0)
    {
      /* if (data-> */
      data++;
    }

  spl0();
}

#endif
