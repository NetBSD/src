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
#ident "$Id: autoconf.c,v 1.1.1.1 1993/09/29 06:09:19 briggs Exp $"
/*
 * from: Utah $Hdr: autoconf.c 1.31 91/01/21$
 *
 *	@(#)autoconf.c	7.5 (Berkeley) 5/7/91
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
#include "sys/systm.h"
/* #include "sys/map.h" */
#include "sys/buf.h"
#include "sys/dkstat.h"
#include "sys/conf.h"
#include "sys/dmap.h"
#include "sys/reboot.h"

#include "sys/disklabel.h"  /* CPC: For MAXPARTITIONS */
#include "sd.h"			/* for NSD MF */
int root_scsi_id;           /* CPC: set in locore.s */
/* MF Allen is going to hell for this one */
struct	sd_data
{
	int	flags;
	struct	scsi_switch *sc_sw;	/* address of scsi low level switch */
	int	ctlr;			/* so they know which one we want */
	int	targ;			/* our scsi target ID */
	int	lu;			/* out scsi lu */
} *sd_data[NSD];

#include "../include/vmparam.h"
#include "../include/param.h"  /* LAK: Added this for some constants */
#include "../include/cpu.h"
#include "machine/pte.h"
#include "isr.h"
#include "../dev/device.h"

/* #include "../dev/con.h" */   /* BARF -- where is this? */

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold;		    /* if 1, still working on cold-start */
int	dkn;		    /* number of iostat dk numbers assigned so far */
#if convert_from_hp300
	int	cpuspeed = MHZ_16;   /* relative cpu speed */
#endif
struct	isr isrqueue[NISR];
struct	nubus_hw nubus_table[MAXSLOTS];
struct	adb_hw adb_table[MAXADB];

#ifdef DEBUG
int	acdebug = 0;
#endif

/*
 * Determine mass storage and memory configuration for a machine.
 */
configure()
{
	register struct nubus_hw *hw;
	int nubus_num;
	int found;

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

#if 0
#include "cd.h"
#if NCD > 0
	/*
	 * Now deal with concatenated disks
	 */
	find_cdevices();
#endif
#endif

#if GENERIC && 0
	if ((boothowto & RB_ASKNAME) == 0) 
		setroot(); /* Make root dev <== load dev */
	setconf();
#else
	setroot(); /* Make root dev <== load dev */
#endif
	swapconf();
	cold = 0;
}

find_nubus()
{
   /* This functions sets up the array "nubus_table" which contains the
   basic information about each card in the Nubus slot.  When device
   drivers are initialized later, they can look through this array to
   see if their hardware is present and claim it. */

   register struct nubus_hw *nu;
   int nubus_num;

   for (nubus_num = 0; nubus_num < MAXSLOTS; nubus_num++)
     nubus_table[nubus_num].found = 0; /* Empty */

   /* LAK: For now we can only check 9..F because that's all we map
   in locore.s.  Eventually (i.e. near future) we should put THIS
   function in locore.s before enabling the MMU and only map the
   slots that have a card in them.  Also, the next loop should go from
   1 to 0xF inclusive (0 is "reserved") to cover all possible hardware.
   Even if the MacII only has 9..F, it won't hurt us to probe 1..8 also. */
   for (nubus_num = 0; nubus_num < 6; nubus_num++)
   {
      nu = nubus_table + nubus_num + 9;
      nu->addr = (caddr_t)(NBBASE + nubus_num * NBMEMSIZE);
      nu->rom = nu->addr + NBROMOFFSET;


      if(!badaddr(nu->rom))
      {
	
	 InitNubusSlot(nu->addr,&(nu->Slot));

         nu->found = 1;
         nu->claimed = 0; /* No driver has claimed this slot yet */

      }
   }
}

find_adbs()
{
   printf("No ADB drivers to match to devices; using default.\n");
}

find_devs()
{
  /* LAK: This routine goes through the list of device drivers and
  asks each one to initialize themselves. */

  struct macdriver *md;
  int i;

  printf ("Initializing drivers:\n");
/* BARF this does funny frame buffer stuff MF */
/* grfconfig(); */

  for (i = 0; i < numdrivers; i++)
  {
    md = &macdriver[i]; /* macdriver[] in ioconf.c in "compile/" */
    md -> hwfound = 0;
/*    printf ("  -- ");  CPC 3/20/93 This was uncool, IMHO! */
    (md -> init)(md);
    /* print only if necessary. CPC */
    if( strcmp( md->name, "" ) )
      printf ("%s\n",md -> name);
  }

  /* Look for Nubus cards which were found but not claimed by any driver: */

  for (i = 0; i < MAXSLOTS; i++)
    if (nubus_table[i].found && !nubus_table[i].claimed)
    {
      printf ("slot %d, type %d not claimed: \"", i,nubus_table[i].Slot.type);
      printf ("%s, ",nubus_table[i].Slot.name);
      printf ("%s\"\n",nubus_table[i].Slot.manufacturer);
    }
}


#if defined(WE_DONT_NEED_THIS) /* LAK */

/*
 * Allocate/deallocate a cache-inhibited range of kernel virtual address
 * space mapping the indicated physical address range [pa - pa+size)
 */
caddr_t
iomap(pa, size)
	caddr_t pa;
	int size;
{
	int ix, npf;
	caddr_t kva;

#ifdef DEBUG
	if (((int)pa & PGOFSET) || (size & PGOFSET))
		panic("iomap: unaligned");
#endif
	npf = btoc(size);
	ix = rmalloc(extiomap, npf);
	if (ix == 0)
		return(0);
	kva = extiobase + ctob(ix-1);
	physaccess(kva, pa, size, PG_RW|PG_CI);
	return(kva);
}

iounmap(kva, size)
	caddr_t kva;
	int size;
{
	int ix;

#ifdef DEBUG
	if (((int)kva & PGOFSET) || (size & PGOFSET))
		panic("iounmap: unaligned");
	if (kva < extiobase || kva >= extiobase + ctob(EIOMAPSIZE))
		panic("iounmap: bad address");
#endif
	physunaccess(kva, size);
	ix = btoc(kva - extiobase) + 1;
	rmfree(extiomap, btoc(size), ix);
}

#endif /* IF_WE_DONT... */

#if NCD > 0
#include "../dev/cdvar.h"

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
	register int nblks;

	for (swp = swdevt; swp->sw_dev; swp++)
		if (bdevsw[major(swp->sw_dev)].d_psize) {
			nblks =
			  (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
			if (nblks != -1 &&
			    (swp->sw_nblks == 0 || swp->sw_nblks > nblks))
				swp->sw_nblks = nblks;
		}
	dumpconf();
}

#define	DOSWAP			/* Change swdevt and dumpdev too */
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
#if defined(THIS_IS_PROBABLY_NOT_NECESSARY)

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
	 * First, find the controller type which support this device.
	 */
	for (hd = hp_dinit; hd->hp_driver; hd++)
		if (hd->hp_driver->d_name[0] == devname[majdev][0] &&
		    hd->hp_driver->d_name[1] == devname[majdev][1])
			break;
	if (hd->hp_driver == 0)
		return;
	/*
	 * Next, find the controller of that type corresponding to
	 * the adaptor number.
	 */
	for (hc = hp_cinit; hc->hp_driver; hc++)
		if (hc->hp_alive && hc->hp_unit == adaptor &&
		    hc->hp_driver == hd->hp_cdriver)
			break;
	if (hc->hp_driver == 0)
		return;
	/*
	 * Finally, find the device in question attached to that controller.
	 */
	for (hd = hp_dinit; hd->hp_driver; hd++)
		if (hd->hp_alive && hd->hp_slave == unit &&
		    hd->hp_cdriver == hc->hp_driver &&
		    hd->hp_ctlr == hc->hp_unit)
			break;
	if (hd->hp_driver == 0)
		return;
	mindev = hd->hp_unit;
	/*
	 * Form a new rootdev
	 */
	mindev = (mindev << PARTITIONSHIFT) + part;
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

#else /* not THIS_IS_PROBABLY_NOT_NECESSARY */
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

{
	extern struct sd_data *sd_data[];
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


#endif /* THIS_IS_PROBABLY_NOT_NECESSARY */
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

#ifdef STRCMP_IN_KERN_SUBR_C
strcmp(s1, s2)
	register char *s1, *s2;
{
	while (*s1 == *s2++)
		if (*s1++=='\0')
			return (0);
	return (*s1 - *--s2);
}
#endif
