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
 * from: Utah $Hdr: autoconf.c 1.31 91/01/21$
 *
 *	@(#)autoconf.c	7.5 (Berkeley) 5/7/91
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
#include "sys/buf.h"
#include "sys/dkstat.h"
#include "sys/conf.h"
#include "sys/dmap.h"
#include "sys/reboot.h"

#include "machine/vmparam.h"
#include "machine/cpu.h"
#include "pte.h"
#include "../dev/device.h"

#include <libraries/configregs.h>
#include <libraries/configvars.h>

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold;		    /* if 1, still working on cold-start */
int	dkn;		    /* number of iostat dk numbers assigned so far */
int	cpuspeed = MHZ_8;   /* relative cpu speed */
struct	amiga_hw sc_table[MAXCTLRS];

#ifdef DEBUG
int	acdebug = 0;
#endif

/*
 * Determine mass storage and memory configuration for a machine.
 */
configure()
{
	register struct amiga_hw *hw;
	int found;

	/*
	 * XXX: these should be consolidated into some kind of table
	 */
	dmainit();

	/*
	 * Look over each hardware device actually found and attempt
	 * to match it with an ioconf.c table entry.
	 */
	for (hw = sc_table; hw->hw_type; hw++) {
		if (HW_ISCTLR(hw))
			found = find_controller(hw);
		else
			found = find_device(hw);
#ifdef DEBUG
		if (!found && acdebug) {
			printf("unconfigured device %d/%d\n",
			       hw->hw_manufacturer, hw->hw_product);
		}
#endif
	}

#if 0
#include "cd.h"
#if NCD > 0
	/*
	 * Now deal with concatenated disks
	 */
	find_cdevices();
#endif
#endif

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

#define dr_type(d, s)	\
	(strcmp((d)->d_name, (s)) == 0)

#define same_hw_ctlr(hw, ac) \
	(HW_ISFLOPPY(hw) && dr_type((ac)->amiga_driver, "floppy") || \
	 HW_ISSCSI(hw) && dr_type((ac)->amiga_driver, "scsi"))

find_controller(hw)
	register struct amiga_hw *hw;
{
	register struct amiga_ctlr *ac;
	struct amiga_ctlr *match_c;
	caddr_t oaddr;
	int sc;

#ifdef DEBUG
	if (acdebug)
		printf("find_controller: hw: [%d/%d] (%x), type %x...",
		       hw->hw_manufacturer, hw->hw_product, 
		       hw->hw_kva, hw->hw_type);
#endif
	sc = (hw->hw_manufacturer << 16) | hw->hw_product;
	match_c = NULL;
	for (ac = amiga_cinit; ac->amiga_driver; ac++) {
		if (ac->amiga_alive)
			continue;
		/*
		 * Make sure we are looking at the right
		 * controller type.
		 */
		if (!same_hw_ctlr(hw, ac))
			continue;
		/*
		 * Exact match; all done
		 */
		if ((int)ac->amiga_addr == sc) {
			match_c = ac;
			break;
		}
		/*
		 * Wildcard; possible match so remember first instance
		 * but continue looking for exact match.
		 */
		if (ac->amiga_addr == NULL && match_c == NULL)
			match_c = ac;
	}
#ifdef DEBUG
	if (acdebug) {
		if (match_c)
			printf("found %s%d\n",
			       match_c->amiga_driver->d_name,
			       match_c->amiga_unit);
		else
			printf("not found\n");
	}
#endif
	/*
	 * Didn't find an ioconf entry for this piece of hardware,
	 * just ignore it.
	 */
	if (match_c == NULL)
		return(0);
	/*
	 * Found a match, attempt to initialize and configure all attached
	 * slaves.  Note, we can still fail if HW won't initialize.
	 */
	ac = match_c;
	oaddr = ac->amiga_addr;
	ac->amiga_addr = hw->hw_kva;
	if ((*ac->amiga_driver->d_init)(ac)) {
		ac->amiga_alive = 1;
		printf ("%s%d", ac->amiga_driver->d_name, ac->amiga_unit);
		printf (" [%d/%d]", hw->hw_manufacturer, hw->hw_product);
		if (ac->amiga_flags)
			printf(", flags 0x%x", ac->amiga_flags);
		printf("\n");
		find_slaves(ac);
	} else
		ac->amiga_addr = oaddr;
	return(1);
}

find_device(hw)
	register struct amiga_hw *hw;
{
	register struct amiga_device *ad;
	struct amiga_device *match_d;
	caddr_t oaddr;
	int sc;

#ifdef DEBUG
	if (acdebug)
		printf("find_device: hw: [%d/%d] (%x), type %x...",
		       hw->hw_manufacturer, hw->hw_product,
		       hw->hw_kva, hw->hw_type);
#endif
	match_d = NULL;
	for (ad = amiga_dinit; ad->amiga_driver; ad++) {
		if (ad->amiga_alive)
			continue;
		/* Must not be a slave */
		if (ad->amiga_cdriver)
			continue;

		sc = (int) ad->amiga_addr;
		/*
		 * Exact match; all done.
		 */
		if (sc > 0 && sc == ((hw->hw_manufacturer << 16) | hw->hw_product)) {
			match_d = ad;
			break;
		}
		/*
		 * Wildcard; possible match so remember first instance
		 * but continue looking for exact match.
		 */
		if (sc == 0 && same_hw_device(hw, ad) && match_d == NULL)
			match_d = ad;
	}
#ifdef DEBUG
	if (acdebug) {
		if (match_d)
			printf("found %s%d\n",
			       match_d->amiga_driver->d_name,
			       match_d->amiga_unit);
		else
			printf("not found\n");
	}
#endif
	/*
	 * Didn't find an ioconf entry for this piece
	 * of hardware, just ignore it.
	 */
	if (match_d == NULL)
		return(0);
	/*
	 * Found a match, attempt to initialize.
	 * Note, we can still fail if HW won't initialize.
	 */
	ad = match_d;
	oaddr = ad->amiga_addr;
	ad->amiga_addr = hw->hw_kva;
	if ((*ad->amiga_driver->d_init)(ad)) {
		ad->amiga_alive = 1;
		printf("%s%d", ad->amiga_driver->d_name, ad->amiga_unit);
		printf (" [%d/%d]", hw->hw_manufacturer, hw->hw_product);
		if (ad->amiga_flags)
			printf(", flags 0x%x", ad->amiga_flags);
		printf("\n");
	} else
		ad->amiga_addr = oaddr;
	return(1);
}

find_slaves(ac)
	struct amiga_ctlr *ac;
{
	if (dr_type(ac->amiga_driver, "floppy"))
		find_busslaves(ac, 4);
	else if (dr_type(ac->amiga_driver, "scsi"))
		find_busslaves(ac, 7);
}

/*
 */
find_busslaves(ac, maxslaves)
	register struct amiga_ctlr *ac;
	int maxslaves;
{
	register int s;
	register struct amiga_device *ad;
	struct amiga_device *match_s;
	int new_s, new_c, old_s, old_c;
	int rescan;
	
#ifdef DEBUG
	if (acdebug)
		printf("find_busslaves: for %s%d\n",
		       ac->amiga_driver->d_name, ac->amiga_unit);
#endif
	for (s = 0; s < maxslaves; s++) {
		rescan = 1;
		match_s = NULL;
		for (ad = amiga_dinit; ad->amiga_driver; ad++) {
			/*
			 * Rule out the easy ones:
			 * 1. slave already assigned or not a slave
			 * 2. not of the proper type
			 * 3. controller specified but not this one
			 * 4. slave specified but not this one
			 */
			if (ad->amiga_alive || ad->amiga_cdriver == NULL)
				continue;
			if (!dr_type(ac->amiga_driver, ad->amiga_cdriver->d_name))
				continue;
			if (ad->amiga_ctlr >= 0 && ad->amiga_ctlr != ac->amiga_unit)
				continue;
			if (ad->amiga_slave >= 0 && ad->amiga_slave != s)
				continue;
			/*
			 * Case 0: first possible match.
			 * Remember it and keep looking for better.
			 */
			if (match_s == NULL) {
				match_s = ad;
				new_c = ac->amiga_unit;
				new_s = s;
				continue;
			}
			/*
			 * Case 1: exact match.
			 * All done.  Note that we do not attempt any other
			 * matches if this one fails.  This allows us to
			 * "reserve" locations for dynamic addition of
			 * disk/tape drives by fully qualifing the location.
			 */
			if (ad->amiga_slave == s && ad->amiga_ctlr == ac->amiga_unit) {
				match_s = ad;
				rescan = 0;
				break;
			}
			/*
			 * Case 2: right controller, wildcarded slave.
			 * Remember first and keep looking for an exact match.
			 */
			if (ad->amiga_ctlr == ac->amiga_unit &&
			    match_s->amiga_ctlr < 0) {
				match_s = ad;
				new_s = s;
				continue;
			}
			/*
			 * Case 3: right slave, wildcarded controller.
			 * Remember and keep looking for a better match.
			 */
			if (ad->amiga_slave == s &&
			    match_s->amiga_ctlr < 0 && match_s->amiga_slave < 0) {
				match_s = ad;
				new_c = ac->amiga_unit;
				continue;
			}
			/*
			 * OW: we had a totally wildcarded spec.
			 * If we got this far, we have found a possible
			 * match already (match_s != NULL) so there is no
			 * reason to remember this one.
			 */
			continue;
		}
		/*
		 * Found a match.  We need to set amiga_ctlr/amiga_slave properly
		 * for the init routines but we also need to remember all
		 * the old values in case this doesn't pan out.
		 */
		if (match_s) {
			ad = match_s;
			old_c = ad->amiga_ctlr;
			old_s = ad->amiga_slave;
			if (ad->amiga_ctlr < 0)
				ad->amiga_ctlr = new_c;
			if (ad->amiga_slave < 0)
				ad->amiga_slave = new_s;
#ifdef DEBUG
			if (acdebug)
				printf("looking for %s%d at slave %d...",
				       ad->amiga_driver->d_name,
				       ad->amiga_unit, ad->amiga_slave);
#endif

			if ((*ad->amiga_driver->d_init)(ad)) {
#ifdef DEBUG
				if (acdebug)
					printf("found\n");
#endif
				printf("%s%d at %s%d, slave %d",
				       ad->amiga_driver->d_name, ad->amiga_unit,
				       ac->amiga_driver->d_name, ad->amiga_ctlr,
				       ad->amiga_slave);
				if (ad->amiga_flags)
					printf(" flags 0x%x", ad->amiga_flags);
				printf("\n");
				ad->amiga_alive = 1;
				if (ad->amiga_dk && dkn < DK_NDRIVE)
					ad->amiga_dk = dkn++;
				else
					ad->amiga_dk = -1;
				rescan = 1;
			} else {
#ifdef DEBUG
				if (acdebug)
					printf("not found\n");
#endif
				ad->amiga_ctlr = old_c;
				ad->amiga_slave = old_s;
			}
			/*
			 * XXX: This should be handled better.
			 * Re-scan a slave.  There are two reasons to do this.
			 * 1. It is possible to have both a tape and disk
			 *    (e.g. 7946) or two disks (e.g. 9122) at the
			 *    same slave address.  Here we need to rescan
			 *    looking only at entries with a different
			 *    physical unit number (amiga_flags).
			 * 2. It is possible that an init failed because the
			 *    slave was there but of the wrong type.  In this
			 *    case it may still be possible to match the slave
			 *    to another ioconf entry of a different type.
			 *    Here we need to rescan looking only at entries
			 *    of different types.
			 * In both cases we avoid looking at undesirable
			 * ioconf entries of the same type by setting their
			 * alive fields to -1.
			 */
			if (rescan) {
				for (ad = amiga_dinit; ad->amiga_driver; ad++) {
					if (ad->amiga_alive)
						continue;
					if (match_s->amiga_alive == 1) {	/* 1 */
						if (ad->amiga_flags == match_s->amiga_flags)
							ad->amiga_alive = -1;
					} else {			/* 2 */
						if (ad->amiga_driver == match_s->amiga_driver)
							ad->amiga_alive = -1;
					}
				}
				s--;
				continue;
			}
		}
		/*
		 * Reset bogon alive fields prior to attempting next slave
		 */
		for (ad = amiga_dinit; ad->amiga_driver; ad++)
			if (ad->amiga_alive == -1)
				ad->amiga_alive = 0;
	}
}

same_hw_device(hw, ad)
	struct amiga_hw *hw;
	struct amiga_device *ad;
{
	int found = 0;

	switch (hw->hw_type & ~B_MASK) {
	case C_FLOPPY:
		found = dr_type(ad->amiga_driver, "floppy");
		break;
	case C_SCSI:
		found = dr_type(ad->amiga_driver, "scsi");
		break;
	case D_BITMAP:
		found = dr_type(ad->amiga_driver, "grf");
		break;
	case D_LAN:
		found = dr_type(ad->amiga_driver, "le");
		break;
	case D_COMMSER:
		found = dr_type(ad->amiga_driver, "ser");
		break;
	default:
		break;
	}
	return(found);
}

char notmappedmsg[] = "WARNING: no space to map IO card, ignored\n";

/*
 * Scan the IO space looking for devices.
 */
find_devs()
{
  short sc;
  u_char *id_reg;
  register caddr_t addr;
  register struct amiga_hw *hw;
  int didmap, sctop;
  extern int num_ConfigDev;
  extern struct ConfigDev *ConfigDev;
  struct ConfigDev *cd;

#if 0
  /*
   * Initialize IO resource map for iomap().
   */
  rminit(extiomap, (long)EIOMAPSIZE, (long)1, "extio", EIOMAPSIZE/16);
#endif
  hw = sc_table;
  
  /* first enter builtin devices */
  
  /* this is only for the A3000, but... */
  hw->hw_pa	      = 0xdd0000;
  hw->hw_size	      = NBPG;
  hw->hw_kva	      = 0;			/* filled out in scsiinit */
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_SCSI;
  hw->hw_type	      = B_BUILTIN | C_SCSI;
  hw++;
  
  hw->hw_pa	      = 0xdc0000;
  hw->hw_size	      = NBPG;
  hw->hw_kva	      = 0;			/* should be mapped here! */
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_CLOCK;
  hw->hw_type	      = B_BUILTIN | D_CLOCK;
  hw++;

  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = 0;			/* already mapped, uses CUSTOMbase */
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_FLOPPY;
  hw->hw_type	      = B_BUILTIN | C_FLOPPY;
  hw++;
  
  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = 0;			/* already mapped, uses CUSTOMbase */
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_RS232;
  hw->hw_type	      = B_BUILTIN | D_COMMSER;
  hw++;
  
  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = 0;			/* already mapped, uses CUSTOMbase */
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_KEYBOARD;
  hw->hw_type	      = B_BUILTIN | D_KEYBOARD;
  hw++;
  
  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = 0;			/* already mapped, uses CUSTOMbase */
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_PPORT;
  hw->hw_type	      = B_BUILTIN | D_PPORT;
  hw++;
  
  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = 0;			/* already mapped, uses CUSTOMbase */
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_DISPLAY;
  hw->hw_type	      = B_BUILTIN | D_BITMAP;
  hw++;
  
  /* and afterwards add Zorro II/III devices passed by the loader */
  
  for (sc = 0, cd = ConfigDev; sc < num_ConfigDev; sc++, cd++)
    {
      hw->hw_pa		  = cd->cd_BoardAddr;
      hw->hw_size	  = cd->cd_BoardSize;
      hw->hw_kva	  = 0;			/* XXX */
      hw->hw_manufacturer = cd->cd_Rom.er_Manufacturer;
      hw->hw_product	  = cd->cd_Rom.er_Product;
      
      switch (hw->hw_manufacturer)
        {
        case MANUF_CBM_1:
          switch (hw->hw_product)
            {
            case PROD_CBM_1_A2088:
              hw->hw_type = B_ZORROII | D_MISC;
              break;
              
            default:
              continue;
            }
            
        case MANUF_UNILOWELL:
          switch (hw->hw_product)
            {
            case PROD_UNILOWELL_A2410:
              hw->hw_type = B_ZORROII | D_BITMAP;
              break;
              
            default:
              continue;
            }
            
        default:
          continue;
	}

      hw++;
    }
}

#if 0
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
#endif

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

#if 0
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
#endif

/*
 * Configure swap space and related parameters.
 */
swapconf()
{
	register struct swdevt *swp;
	register int nblks;

	for (swp = swdevt; swp->sw_dev; swp++)
	  {
		if (bdevsw[major(swp->sw_dev)].d_psize) {
			nblks =
			  (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
			if (nblks != -1 &&
			    (swp->sw_nblks == 0 || swp->sw_nblks > nblks))
			      {
				swp->sw_nblks = nblks;
/*				printf ("swap: dev %x = %d\n", swp->sw_dev, nblks);*/
			      }
		}
	  }
	dumpconf();

	printf ("\n");
}

#define	DOSWAP			/* Change swdevt and dumpdev too */
u_long	bootdev;		/* should be dev_t, but not until 32 bits */

static	char devname[][2] = {
	0,0,		/* 0 = ct */
	0,0,		/* 1 = xx */
	'r','d',	/* 2 = rd */
	0,0,		/* 3 = sw */
	's','d',	/* 4 = sd */
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
	register struct amiga_ctlr *ac;
	register struct amiga_device *ad;
	int  majdev, mindev, unit, part, adaptor;
	dev_t temp, orootdev;
	struct swdevt *swp;

/*	printf ("setroot: boothowto = 0x%x, bootdev = 0x%x\n", boothowto, bootdev);*/

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
	for (ad = amiga_dinit; ad->amiga_driver; ad++)
		if (ad->amiga_driver->d_name[0] == devname[majdev][0] &&
		    ad->amiga_driver->d_name[1] == devname[majdev][1])
			break;
	if (ad->amiga_driver == 0)
		return;
	/*
	 * Next, find the controller of that type corresponding to
	 * the adaptor number.
	 */
	for (ac = amiga_cinit; ac->amiga_driver; ac++)
		if (ac->amiga_alive && ac->amiga_unit == adaptor &&
		    ac->amiga_driver == ad->amiga_cdriver)
			break;
	if (ac->amiga_driver == 0)
		return;
	/*
	 * Finally, find the device in question attached to that controller.
	 */
	for (ad = amiga_dinit; ad->amiga_driver; ad++)
		if (ad->amiga_alive && ad->amiga_slave == unit &&
		    ad->amiga_cdriver == ac->amiga_driver &&
		    ad->amiga_ctlr == ac->amiga_unit)
			break;
	if (ad->amiga_driver == 0)
		return;
	mindev = ad->amiga_unit;
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
}
