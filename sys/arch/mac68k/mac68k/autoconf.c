/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 */
/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * from: Utah $Hdr: autoconf.c 1.31 91/01/21$
 *
 *	from: from: @(#)autoconf.c	7.5 (Berkeley) 5/7/91
 *	$Id: autoconf.c,v 1.3 1993/12/15 03:27:50 briggs Exp $
 */

/*
   ALICE 
      05/23/92 BG
      I've started to re-write this procedure to use our devices and strip 
      out all the useless HP stuff, but I only got to line 120 or so 
      before I had a really bad attack of kompernelphobia and blacked out.
*/

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include "sys/param.h"
#include "sys/device.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/dkstat.h"
#include "sys/conf.h"
#include "sys/dmap.h"
#include "sys/reboot.h"

#include "sys/disklabel.h"  /* CPC: For MAXPARTITIONS */
#include "sd.h"			/* for NSD MF */
int root_scsi_id;           /* CPC: set in locore.s */

#include "../include/vmparam.h"
#include "../include/param.h"  /* LAK: Added this for some constants */
#include "../include/cpu.h"
#include "machine/pte.h"
#include "isr.h"
#include "../dev/device.h"

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold;		    /* if 1, still working on cold-start */
int	dkn;		    /* number of iostat dk numbers assigned so far */
struct	isr isrqueue[NISR];
struct	adb_hw adb_table[MAXADB];

#ifdef DEBUG
int	acdebug = 0;
#endif

/*
 * Determine mass storage and memory configuration for a machine.
 */
configure()
{
	int found;

#ifdef NEWCONFIG
	VIA_initialize();

	adb_init();		/* ADB device subsystem & driver */

	isrinit();

	startrtclock();

	if (config_rootfound("mainbus", "mainbus") == 0)
		panic("No main device!");
#else

	VIA_initialize();

	/*
	 * XXX: these should be consolidated into some kind of table
	 */

/* ALICE 05/23/92 BG -- I've taken the liberty of removing your horses brain. */

	/* initialize ADB subsystem and re-number devices. */
	adb_init();		/* ADB device subsystem & driver */
	isrinit();

/* BARF Allen's idea MF typing */
/* what we think is happening is that via_init is BLOWING
   away the clock that we started earlier, so restart the sucker */
/* MF Allen was right, go figure*/
	startrtclock();

	/* Initialize the Nubus subsystem -- Find all cards present */
	find_nubus();

	/* Find all ADB devices */
	find_adbs();

	/* Ask each driver if they find their hardware and init it: */
	find_devs();

#endif
	setroot(); /* Make root dev <== load dev */
	swapconf();
	cold = 0;
}

find_adbs()
{
   printf("No ADB drivers to match to devices; using default.\n");
}

struct newconf_S {
	char	*name;
	int	req;
};

static int
mbprint(aux, name)
	void	*aux;
	char	*name;
{
	struct newconf_S	*c = (struct newconf_S *) aux;

	if (name)
		printf("%s at %s", c->name, name);
	return(UNCONF);
}

static int
root_matchbyname(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
	return (strcmp(cf->cf_driver->cd_name, (char *)aux) == 0);
}

extern int
matchbyname(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
	struct newconf_S	*c = (struct newconf_S *) aux;

	return (strcmp(cf->cf_driver->cd_name, c->name) == 0);
}

static void
mainbus_attach(parent, dev, aux)
	struct device	*parent, *dev;
	void		*aux;
{
	struct newconf_S	conf_data[] = {
					{"adb",    1},
/*					{"clock",  1},	*/
					{"nubus",  1},
					{"ser",    0},
					{"ncr",    1},
					{"audio",  0},
					{"floppy", 0},
					{NULL, 0}
			 	};
	struct newconf_S	*c;
	int			fail=0, warn=0;

	printf("\n");
	for (c=conf_data ; c->name ; c++) {
		if (config_found(dev, c, mbprint)) {
		} else {
			if (c->req) {
				fail++;
			}
			warn++;
		}
	}

	if (fail) {
		printf("Failed to find %d required devices.\n", fail);
		panic("Can't continue.");
	}
}

struct cfdriver mainbuscd =
      { NULL, "mainbus", root_matchbyname, mainbus_attach,
	DV_DULL, sizeof(struct device), NULL, 0 };

extern int dummy_match(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
	return 0;
}

static void
dummy_attach(parent, dev, aux)
	struct device	*parent, *dev;
	void		*aux;
{
	printf("\n");
}

struct cfdriver scsibuscd =
      { NULL, "scsibus", dummy_match, dummy_attach,
	DV_DULL, sizeof(struct device), NULL, 0 };
struct cfdriver sdcd =
      { NULL, "sd", dummy_match, dummy_attach,
	DV_DISK, sizeof(struct device), NULL, 0 };
struct cfdriver stcd =
      { NULL, "st", dummy_match, dummy_attach,
	DV_TAPE, sizeof(struct device), NULL, 0 };
struct cfdriver cdcd =
      { NULL, "cd", dummy_match, dummy_attach,
	DV_DISK, sizeof(struct device), NULL, 0 };

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
	register int nblks, tblks;

	for (swp = swdevt; swp->sw_dev != NODEV ; swp++)
		if (bdevsw[major(swp->sw_dev)].d_psize) {
			tblks += nblks =
			  (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
			if (nblks != -1 &&
			    (swp->sw_nblks == 0 || swp->sw_nblks > nblks))
				swp->sw_nblks = nblks;
		}
	if (tblks == 0) {
		printf("No swap partitions configured?\n");
	}
	dumpconf();
}

u_long	bootdev;		/* should be dev_t, but not until 32 bits */

static	char devname[][2] = {
	0,0,		/* 0 = ct */
	0,0,		/* 1 = xx */
	'r','d',	/* 2 = rd */
	0,0,		/* 3 = sw */
	's','d',	/* 4 = rd */
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
/* MF BARF BARF */
/* what we should do is take root_scsi_id and figure out the real
disk to mount, the value to mount is the n'th mountable volume
so scsiid5 may be device 0 to the system, beware, this has bitten
us before.  Remember to be nice to women, even the evil hateful ones,
which is all of them, or so the rumor goes... */

/* MF we really want sd0 to be root, so when we find the real
   disk that is root, lets switch it with sd0 !!! HAHAHAHA!

   Please close your eyes, that may be dirty. Don't try this
   kludge in kansas, utah, or south dakota
*/

#if 0
{
	extern struct	sd_data
	{
		int	flags;
		struct	scsi_switch *sc_sw;	/* address of scsi low level switch */
		int	ctlr;			/* so they know which one we want */
		int	targ;			/* our scsi target ID */
		int	lu;			/* out scsi lu */
	} *sd_data[];
	int i;

	for(i=0;i<NSD;i++)
	{
		if (root_scsi_id==sd_data[i]->targ)
		{
			struct sd_data *swap;

			swap=sd_data[i];
			sd_data[i]=sd_data[0];
			sd_data[0]=swap;

			printf("Swapping sd%d and sd%d\n",i,0);
			break;
		}
	}


	/* rootdev = makedev(4, i * MAXPARTITIONS); */
	rootdev = makedev(4, 0 * MAXPARTITIONS);


}
#endif
	rootdev = makedev(4, 0 * MAXPARTITIONS);

#if NO_SERIAL_ECHO
	{
		extern char	serial_boot_echo;

		if (serial_boot_echo) {
			printf("Serial echo going out now.\n");
			serial_boot_echo = 0;
		}
	}
#endif
}

/*
	The following four functions should be filled in to work...
*/

caddr_t
sctova(sc)
	register int sc;
{
}

caddr_t
iomap(pa, size)
	caddr_t pa;
	int size;
{
}

patosc(addr)
	register caddr_t addr;
{
}

vatosc(addr)
	register caddr_t addr;
{
}

caddr_t
sctopa(sc)
	register int sc;
{
}
