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
 *	$Id: autoconf.c,v 1.15 1994/04/10 21:30:44 chopps Exp $
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

#include <vm/vm.h>		/* XXXX Kludge for IVS Vector - mlh */

#include <machine/vmparam.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <amiga/dev/device.h>

#include <amiga/amiga/configdev.h>
#include <amiga/amiga/custom.h>

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold;		    /* if 1, still working on cold-start */
int	dkn;		    /* number of iostat dk numbers assigned so far */
int	cpuspeed = MHZ_8;   /* relative cpu speed */
struct	amiga_hw sc_table[MAXCTLRS];

/* set up in amiga_init.c */
extern caddr_t ZORRO2ADDR;

/* maps a zorro2 and/or A3000 builtin address into the mapped kva address */
#define zorro2map(pa) ((caddr_t) ((u_int)ZORRO2ADDR - ZORRO2BASE + (u_int)pa))

/* tests whether the address lies in our zorro2 space */
#define iszorro2kva(kva) \
    ((u_int)kva >= (u_int)ZORRO2ADDR && \
    (u_int)kva < ((u_int)ZORRO2ADDR + (u_int)ZORRO2TOP - (u_int)ZORRO2BASE))

#define iszorro2pa(pa) ((u_int)pa >= ZORRO2BASE && (u_int)pa <= ZORRO2TOP)

#ifdef DEBUG
int	acdebug = 0;
#endif

void setroot __P((void));
void swapconf __P((void));
void configure __P((void));
void find_devs __P((void));
void find_slaves __P((struct amiga_ctlr *));
void find_busslaves __P((struct amiga_ctlr *, int));

/*
 * Determine mass storage and memory configuration for a machine.
 */
void
configure()
{
	register struct amiga_hw *hw;
	int found;
	
	/* XXXX I HATE IT XXXX */
	custom.intena = INTF_INTEN;

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
		if (!found && acdebug) 
			printf("unconfigured device %d/%d\n",
			    hw->hw_manufacturer, hw->hw_product);
		
#endif
	}
	
#if GENERIC
	if ((boothowto & RB_ASKNAME) == 0)
		setroot();
	setconf();
#else
	setroot();
#endif
	swapconf();
	cold = 0;

	custom.intena = INTF_SETCLR | INTF_INTEN;
}

#define dr_type(d, s) (strcmp((d)->d_name, (s)) == 0)

#define same_hw_ctlr(hw, ac) \
    (HW_ISFLOPPY(hw) && dr_type((ac)->amiga_driver, "floppy") || \
    HW_ISSCSI(hw) && (dr_type((ac)->amiga_driver, "a3000scsi") \
    || dr_type((ac)->amiga_driver, "a2091scsi") \
    || dr_type((ac)->amiga_driver, "GVPIIscsi") \
    || dr_type((ac)->amiga_driver, "Zeusscsi") \
    || dr_type((ac)->amiga_driver, "Magnumscsi") \
    || dr_type((ac)->amiga_driver, "Mlhscsi") \
    || dr_type((ac)->amiga_driver, "Csa12gscsi") \
    || dr_type((ac)->amiga_driver, "Suprascsi") \
    || dr_type((ac)->amiga_driver, "IVSscsi")))

int
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
	if (0 == (*ac->amiga_driver->d_init)(ac)) 
		ac->amiga_addr = oaddr;
	else {
		ac->amiga_alive = 1;
		printf("%s%d", ac->amiga_driver->d_name, ac->amiga_unit);
		printf(" [%d/%d]", hw->hw_manufacturer, hw->hw_product);
		if (ac->amiga_flags)
			printf(", flags 0x%x", ac->amiga_flags);
		printf("\n");
		find_slaves(ac);
	}
	return(1);
}

int
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

		/*
		 * XXX: A graphics device that was found as part of the
		 * console init will have the amiga_addr field already set
		 * (i.e. no longer the select code).  Gotta perform a
		 * slightly different check for an exact match.
		 */
		if (HW_ISDEV(hw, D_BITMAP) && iszorro2kva(ad->amiga_addr)) {
			if (ad->amiga_addr == hw->hw_kva) {
				match_d = ad;
				break;
			}
			continue;
		}
		sc = (int) ad->amiga_addr;
		/*
		 * Exact match; all done.
		 */
		if (sc > 0 && 
		    sc == ((hw->hw_manufacturer << 16) | hw->hw_product)) {
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
	ad->amiga_serno = hw->hw_serno;
	ad->amiga_size = hw->hw_size;
	
	if (0 == (*ad->amiga_driver->d_init)(ad))
		ad->amiga_addr = oaddr;
	else {
		ad->amiga_alive = 1;
		printf("%s%d", ad->amiga_driver->d_name, ad->amiga_unit);
		printf(" [%d/%d]", hw->hw_manufacturer, hw->hw_product);
		if (ad->amiga_flags)
			printf(", flags 0x%x", ad->amiga_flags);
		printf("\n");
	}
	return(1);
}

void
find_slaves(ac)
	struct amiga_ctlr *ac;
{
	if (dr_type(ac->amiga_driver, "floppy"))
		find_busslaves(ac, 4);
	else if (dr_type(ac->amiga_driver, "a3000scsi")
            || dr_type(ac->amiga_driver, "a2091scsi")
	    || dr_type(ac->amiga_driver, "GVPIIscsi")
	    || dr_type(ac->amiga_driver, "Zeusscsi")
	    || dr_type(ac->amiga_driver, "Magnumscsi")
	    || dr_type(ac->amiga_driver, "Mlhscsi")
	    || dr_type(ac->amiga_driver, "Csa12gscsi")
	    || dr_type(ac->amiga_driver, "Suprascsi")
	    || dr_type(ac->amiga_driver, "IVSscsi"))
		find_busslaves(ac, 7);
}

/*
 */
void
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
			else if (0 == dr_type(ac->amiga_driver,
			    ad->amiga_cdriver->d_name))
				continue;
			else if (ad->amiga_ctlr >= 0 &&
			    ad->amiga_ctlr != ac->amiga_unit)
				continue;
			else if (ad->amiga_slave >= 0 && ad->amiga_slave != s)
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
			if (ad->amiga_slave == s &&
			    ad->amiga_ctlr == ac->amiga_unit) {
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
			    match_s->amiga_ctlr < 0 &&
			    match_s->amiga_slave < 0) {
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
		 * Found a match.  We need to set
		 * amiga_ctlr/amiga_slave properly for the init
		 * routines but we also need to remember all the old
		 * values in case this doesn't pan out.
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
				       ad->amiga_driver->d_name,
				       ad->amiga_unit,
				       ac->amiga_driver->d_name,
				       ad->amiga_ctlr,ad->amiga_slave);
				if (ad->amiga_flags)
					printf(" flags 0x%x",ad->amiga_flags);
				printf("\n");
				
				ad->amiga_alive = 1;
				if (ad->amiga_dk && dkn < DK_NDRIVE)
					ad->amiga_dk = dkn++;
				else
					ad->amiga_dk = -1;
				rescan = 1;
				/*
				 * The init on this unit suceeded, so
				 * we need to mark the same
				 * device/unit on other hardware
				 * controllers as "alive" since we
				 * can't reuse the same unit for that
				 * device driver. mlh
				 */
				for (ad = amiga_dinit; ad->amiga_driver;
				     ad++) {
					if (ad->amiga_driver ==
					    match_s->amiga_driver &&
					    ad->amiga_unit ==
					    match_s->amiga_unit)
						ad->amiga_alive = 2;
				}
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
				for (ad = amiga_dinit; ad->amiga_driver;
				    ad++) {
					if (ad->amiga_alive)
						continue;
					else if (match_s->amiga_alive == 1) {
						if (ad->amiga_flags ==
						    match_s->amiga_flags)
							ad->amiga_alive = -1;
					} else { /* 2 */
						if (ad->amiga_driver ==
						    match_s->amiga_driver)
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

int
same_hw_device(hw, ad)
	struct amiga_hw *hw;
	struct amiga_device *ad;
{
	int found;

	found = 0;

	switch (hw->hw_type & ~B_MASK) {
	case C_FLOPPY:
		found = dr_type(ad->amiga_driver, "floppy");
		break;
	case C_SCSI:
		found = (dr_type(ad->amiga_driver, "a3000scsi")
			 || dr_type(ad->amiga_driver, "a2091scsi")
			 || dr_type(ad->amiga_driver, "GVPIIscsi")
			 || dr_type(ad->amiga_driver, "Zeusscsi")
			 || dr_type(ad->amiga_driver, "Magnumscsi")
			 || dr_type(ad->amiga_driver, "Mlhscsi")
			 || dr_type(ad->amiga_driver, "Csa12gscsi")
			 || dr_type(ad->amiga_driver, "Suprascsi")
			 || dr_type(ad->amiga_driver, "IVSscsi"));
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
	case D_CLOCK:
		found = dr_type(ad->amiga_driver, "rtclock");
		break;
	case D_PPORT:
		found = dr_type(ad->amiga_driver, "par");
		break;
	case D_FLOP:
		found = dr_type(ad->amiga_driver, "fd");
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
void
find_devs()
{
  extern int num_ConfigDev;
  extern struct ConfigDev *ConfigDev;
  short sc;
  u_char *id_reg;
  register caddr_t addr;
  register struct amiga_hw *hw;
  int didmap, sctop;
  struct ConfigDev *cd;

#if 0
  /*
   * Initialize IO resource map for iomap().
   */
  rminit(extiomap, (long)EIOMAPSIZE, (long)1, "extio", EIOMAPSIZE/16);
#endif
  hw = sc_table;
  
  /* first enter builtin devices */
  
  if (is_a4000 ())
    {
      /* The A4000 appears to use the same realtime clock as the A3000.
         Add the IDE controller when that information becomes available.
	 */
  
      hw->hw_pa	      	  = (caddr_t) 0xdc0000;
      hw->hw_size	  = NBPG;
      hw->hw_kva	  = zorro2map (0xdc0000);
      hw->hw_manufacturer = MANUF_BUILTIN;
      hw->hw_product      = PROD_BUILTIN_CLOCK;
      hw->hw_type	  = B_BUILTIN | D_CLOCK;
      hw->hw_serno	  = 0;
      hw++;
    }
  else if (is_a3000 ())
    {
      /* hm, this doesn't belong here... */
      volatile u_char *magic_reset_reg = zorro2map (0xde0002);
      /* this bit makes the next reset look like a powerup reset, Amiga
	 Unix sets this bit, and perhaps it will enable 16M machines to
	 boot again... */
      *magic_reset_reg   |= 0x80;

      hw->hw_pa		  = (caddr_t) 0xdd0000;
      hw->hw_size	  = NBPG;
      hw->hw_kva	  = zorro2map (0xdd0000);
      hw->hw_manufacturer = MANUF_BUILTIN;
      hw->hw_product      = PROD_BUILTIN_SCSI;
      hw->hw_type	  = B_BUILTIN | C_SCSI;
      hw->hw_serno	  = 0;
      hw++;
  
      hw->hw_pa	      	  = (caddr_t) 0xdc0000;
      hw->hw_size	  = NBPG;
      hw->hw_kva	  = zorro2map (0xdc0000);
      hw->hw_manufacturer = MANUF_BUILTIN;
      hw->hw_product      = PROD_BUILTIN_CLOCK;
      hw->hw_type	  = B_BUILTIN | D_CLOCK;
      hw->hw_serno	  = 0;
      hw++;
    }
  else
    {
      /* what about other Amigas? Oh well.. */
      hw->hw_pa	      	  = (caddr_t) 0xdc0000;
      hw->hw_size	  = NBPG;
      hw->hw_kva	  = zorro2map (0xdc0000);
      hw->hw_manufacturer = MANUF_BUILTIN;
      hw->hw_product      = PROD_BUILTIN_CLOCK2;
      hw->hw_type	  = B_BUILTIN | D_CLOCK;
      hw->hw_serno	  = 0;
      hw++;
    }

  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = (caddr_t) CUSTOMbase;
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_FLOPPY;
  hw->hw_type	      = B_BUILTIN | C_FLOPPY;
  hw->hw_serno	      = 0;
  hw++;
  
  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = (caddr_t) CUSTOMbase;
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_KEYBOARD;
  hw->hw_type	      = B_BUILTIN | D_KEYBOARD;
  hw->hw_serno	      = 0;
  hw++;
  
  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = (caddr_t) CUSTOMbase;
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_PPORT;
  hw->hw_type	      = B_BUILTIN | D_PPORT;
  hw->hw_serno	      = 0;
  hw++;
  
  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = (caddr_t)CUSTOMbase;
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_FLOP;
  hw->hw_type	      = B_BUILTIN | D_FLOP;
  hw->hw_serno	      = 0;
  hw++;

  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = (caddr_t) CUSTOMbase;
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_DISPLAY;
  hw->hw_type	      = B_BUILTIN | D_BITMAP;
  hw->hw_serno	      = 0;
  hw++;

  hw->hw_pa	      = 0;
  hw->hw_size	      = 0;
  hw->hw_kva	      = (caddr_t) CUSTOMbase;
  hw->hw_manufacturer = MANUF_BUILTIN;
  hw->hw_product      = PROD_BUILTIN_RS232;
  hw->hw_type	      = B_BUILTIN | D_COMMSER;
  hw->hw_serno	      = 0;
  hw++;

  /* and afterwards add Zorro II/III devices passed by the loader */
  
  for (sc = 0, cd = ConfigDev; sc < num_ConfigDev; sc++, cd++)
    {
      hw->hw_pa		  = cd->cd_BoardAddr;
      hw->hw_size	  = cd->cd_BoardSize;
      /* ADD ZORRO3 SUPPORT HERE !! */
      hw->hw_kva	  = iszorro2pa(cd->cd_BoardAddr) ? zorro2map (cd->cd_BoardAddr) : 0;
      hw->hw_manufacturer = cd->cd_Rom.er_Manufacturer;
      hw->hw_product	  = cd->cd_Rom.er_Product;
      hw->hw_serno	  = cd->cd_Rom.er_SerialNumber;
      
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
          break;

	case MANUF_CBM_2:
	  switch (hw->hw_product)            
	    {
	    case PROD_CBM_2_A2091:
	      hw->hw_type = B_ZORROII | C_SCSI;
	      break;

	    case PROD_CBM_2_A2065:
	      hw->hw_type = B_ZORROII | D_LAN;
              /* the ethernet board uses bytes 1 to 3 for ID, set byte 0 to indicate
                 whether Commodore or Ameristar board. */
              hw->hw_serno = (hw->hw_serno & 0x00ffffff) | 0x01000000;
	      break;

	    default:
	      continue;
	    }
	  break;
	  
	case MANUF_AMERISTAR:
	  switch (hw->hw_product)
	    {
	    case PROD_AMERISTAR_ETHER:
	      hw->hw_type = B_ZORROII | D_LAN;
              /* the ethernet board uses bytes 1 to 3 for ID, set byte 0 to indicate
                 whether Commodore or Ameristar board. */
              hw->hw_serno = (hw->hw_serno & 0x00ffffff) | 0x02000000;
	      break;
	      
	    default:
	      continue;
	    }
	  break;

        case MANUF_UNILOWELL:
          switch (hw->hw_product)
            {
            case PROD_UNILOWELL_A2410:
              hw->hw_type = B_ZORROII | D_BITMAP;
              break;
              
            default:
              continue;
            }
          break;

	case MANUF_MACROSYSTEM:
	  switch (hw->hw_product)
	    {
	    case PROD_MACROSYSTEM_RETINA:
	      hw->hw_type = B_ZORROII | D_BITMAP;
	      break;
	      
	    default:
	      continue;
	    }
	  break;

	case MANUF_GVP:
	  switch (hw->hw_product)
	    {
	    case PROD_GVP_SERIES_I:
	      /* XXX need to pass product code to driver */
	      hw->hw_type = B_ZORROII | C_SCSI;
	      break;

	    case PROD_GVP_SERIES_II:
	      /* Figure out what kind of board this is */
	      id_reg = (u_char *)hw->hw_kva + 0x8001;
	      switch (*id_reg & 0xf8)
		{
		case PROD_GVP_X_GF40_SCSI:
		case PROD_GVP_X_COMBO4_SCSI:
		case PROD_GVP_X_GF30_SCSI:
		case PROD_GVP_X_COMBO3_SCSI:
		case PROD_GVP_X_SCSI_II:
		  hw->hw_type = B_ZORROII | C_SCSI;
		  break;
		case PROD_GVP_X_IOEXTEND:
		  hw->hw_type = B_ZORROII | D_COMMSER;
		  break;
		}
	      break;

	    case PROD_GVP_IV24:
	      hw->hw_type = B_ZORROII | D_BITMAP;
	      break;

	    default:
	      continue;
	    }
	  break;

	case MANUF_PPI:
	  switch (hw->hw_product)
	    {
	    case PROD_PPI_ZEUS:
	      hw->hw_type = B_ZORROII | C_SCSI;
	      break;

	    default:
	      continue;
	    }
	  break;

	case MANUF_CSA:
	  switch (hw->hw_product)
	    {
	    case PROD_CSA_MAGNUM:
	    case PROD_CSA_12G:
	      hw->hw_type = B_ZORROII | C_SCSI;
	      break;

	    default:
	      continue;
	    }
	  break;

	case MANUF_SUPRA:
	  switch (hw->hw_product)
	    {
	    /* XXXX need to distinguish different controllers with a single driver */
	    case PROD_SUPRA_WORDSYNC_2:
	      hw->hw_type = B_ZORROII | C_SCSI;
	      break;

	    default:
	      continue;
	    }
	  break;

	case MANUF_IVS:
	  switch (hw->hw_product)
	    {
	    /* XXXX need to distinguish different controllers with a single driver */
	    case PROD_IVS_VECTOR:
	      /*
	       * XXXX Ouch! board addresss isn't Zorro II or Zorro III!
	       * XXXX Kludge it up until I can do it better (MLH).
	       */
	      {
		/* XXXX pa 0x00f00000 shouldn't be used for anything */
		if (pmap_extract(kernel_pmap, zorro2map(0x00f00000)) == 0x00f00000) {
		  physaccess(zorro2map(0x00f00000),cd->cd_BoardAddr,
		    0x10000, PG_W|PG_CI);
		  hw->hw_kva = zorro2map(0x00f00000) + ((int)cd->cd_BoardAddr & PGOFSET);
#ifdef DEBUG
		  printf("IVS Vector: mapped to %x kva %x\n",
		    zorro2map(0x00f00000), hw->hw_kva);
#endif
		}
		else
		  printf("Unable to map IVS Vector SCSI\n");
	      }
	      /* fall through */
	    case PROD_IVS_TRUMPCARD:
	      hw->hw_product = PROD_IVS_VECTOR;	/* another kludge */
	      hw->hw_type = B_ZORROII | C_SCSI;
	      break;

	    default:
	      continue;
	    }
	  break;

	case MANUF_HACKER:
	  switch (hw->hw_product)
	    {
	    case PROD_HACKER_MLH:
	      hw->hw_type = B_ZORROII | C_SCSI;
	      break;

	    default:
	      continue;
	    }
	  break;

        default:
          continue;
	}

      hw++;
    }
}

/*
 * Configure swap space and related parameters.
 */
void
swapconf()
{
	register struct swdevt *swp;
	register int nblks;

	for (swp = swdevt; swp->sw_dev; swp++) {
		if (bdevsw[major(swp->sw_dev)].d_psize) {
			nblks =
			    bdevsw[major(swp->sw_dev)].d_psize(swp->sw_dev);
			if (nblks != -1 &&
			    (swp->sw_nblks == 0 || swp->sw_nblks > nblks)) {
				swp->sw_nblks = nblks;
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
void
setroot()
{
	register struct amiga_ctlr *ac;
	register struct amiga_device *ad;
	int  majdev, mindev, unit, part, adaptor;
	dev_t temp, orootdev;
	struct swdevt *swp;

#ifdef DEBUG
	if (acdebug > 1)
		printf("setroot: boothowto = 0x%x, bootdev = 0x%x\n",
			boothowto, bootdev);
#endif

	if (boothowto & RB_DFLTROOT ||
	    (bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC) {
#ifdef DEBUG
		if (acdebug > 1)
			printf("returning due to: bad boothowto\n");
#endif
		return;
	}
	majdev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
	if (majdev > sizeof(devname) / sizeof(devname[0])) {
#ifdef DEBUG
		if (acdebug > 1)
			printf("returning due to: majdev(%d) > maxdevs(%d)\n",
			    majdev, sizeof(devname) / sizeof(devname[0]));
#endif
		return;
	}

	adaptor = (bootdev >> B_ADAPTORSHIFT) & B_ADAPTORMASK;
	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;

	/*
	 * First, find the controller type which support this device.
	 * Can have more than one controller for the same device, with
	 * just one of them configured, so test for
	 * ad->amiga_cdriver != 0 too.
	 */
	for (ad = amiga_dinit; ad->amiga_driver; ad++) {
		if (ad->amiga_driver->d_name[0] != devname[majdev][0] 
		    || ad->amiga_driver->d_name[1] != devname[majdev][1])
			continue;

		/*
		 * Next, find the controller of that type corresponding to
		 * the adaptor number.
		 */
		for (ac = amiga_cinit; ac->amiga_driver; ac++)
			if (ac->amiga_alive && ac->amiga_unit == adaptor &&
			    ac->amiga_driver == ad->amiga_cdriver)
				goto found_it;
	}

	/* could also place after test, but I'd like to be on the safe side */
found_it:
	if (ad->amiga_driver == 0) {
#ifdef DEBUG
		if (acdebug > 1)
			printf("returning due to: amiga_driver == 0\n");
#endif
		return;
	}

	/*
	 * Finally, find the device in question attached to that controller.
	 */
	for (ad = amiga_dinit; ad->amiga_driver; ad++)
		if (ad->amiga_alive && ad->amiga_slave == unit &&
		    ad->amiga_cdriver == ac->amiga_driver &&
		    ad->amiga_ctlr == ac->amiga_unit)
			break;
	if (ad->amiga_driver == 0) {
#ifdef DEBUG
		if (acdebug > 1)
			printf("returning due to: no device\n");
#endif
		return;
	}
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
	if (rootdev == orootdev) {
#ifdef DEBUG
		if (acdebug > 1)
			printf("returning due to: new root == old root\n");
#endif
		return;
	}

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

/*
 * Try to determine, of this machine is an A3000, which has a builtin
 * realtime clock and scsi controller, so that this hardware is only
 * included as "configured" if this IS an A3000
 */

int a3000_flag = 1;		/* patchable */
#ifdef A4000
int a4000_flag = 1;		/* patchable - default to A4000 */
#else
int a4000_flag = 0;		/* patchable */
#endif

int
is_a3000()
{
	/* this is a dirty kludge.. but how do you do this RIGHT ? :-) */
	extern long orig_fastram_start;
	extern int num_ConfigDev;
	extern struct ConfigDev *ConfigDev;
	short sc;
	struct ConfigDev *cd;

	/* where is fastram on the A4000 ?? */
	/* if fastram is below 0x07000000, assume it's not an A3000 */
	if (orig_fastram_start < 0x07000000)
		return (0);

	/*
	 * OK, fastram starts at or above 0x07000000, check specific
	 * machines
	 */
	for (sc = 0, cd = ConfigDev; sc < num_ConfigDev; sc++, cd++) {
		switch (cd->cd_Rom.er_Manufacturer) {
		case MANUF_PPI:		/* Progressive Peripherals, Inc */
			switch (cd->cd_Rom.er_Product) {
			case PROD_PPI_MERCURY:	/* PPI Mercury - A3000 */
			case PROD_PPI_A3000_040:/* PP&S A3000 '040 */
				return (1);
			case PROD_PPI_ZEUS:	/* PPI Zeus - it's an A2000 */
			case PROD_PPI_A2000_040:/* PP&S A2000 '040 */
			case PROD_PPI_A500_040:	/* PP&S A500 '040 */
				return (0);
			}
			break;

		case MANUF_IVS:			/* IVS */
			switch (cd->cd_Rom.er_Product) {
			case PROD_IVS_VECTOR_ACC:
				return (0); /* A2000 accelerator? */
			}
			break;
		}
	}
	/* XXX assume it's an A3000 */
	return (a3000_flag);
}

int
is_a4000()
{
	return (a4000_flag);		/* XXX */
}
