/*	$NetBSD: autoconf.c,v 1.46 2003/07/15 01:19:42 lukem Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman
 * Copyright (c) 1994 Christian E. Hopps
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.46 2003/07/15 01:19:42 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <machine/disklabel.h>
#include <machine/cpu.h>
#include <atari/atari/device.h>

static void findroot __P((void));
void mbattach __P((struct device *, struct device *, void *));
int mbprint __P((void *, const char *));
int mbmatch __P((struct device *, struct cfdata *, void *));

struct device *booted_device;
int booted_partition;

int atari_realconfig;
#include <sys/kernel.h>

/*
 * called at boot time, configure all devices on system
 */
void
cpu_configure()
{
	extern int atari_realconfig;
	
	atari_realconfig = 1;

	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");
}

void
cpu_rootconf()
{
	findroot();
	setroot(booted_device, booted_partition);
}

/*ARGSUSED*/
int
simple_devprint(auxp, pnp)
	void *auxp;
	const char *pnp;
{
	return(QUIET);
}

/*
 * use config_search to find appropriate device, then call that device
 * directly with NULL device variable storage.  A device can then 
 * always tell the difference between the real and console init 
 * by checking for NULL.
 */
int
atari_config_found(pcfp, pdp, auxp, pfn)
	struct cfdata *pcfp;
	struct device *pdp;
	void *auxp;
	cfprint_t pfn;
{
	struct device temp;
	struct cfdata *cf;
	const struct cfattach *ca;
	extern int	atari_realconfig;

	if (atari_realconfig)
		return(config_found(pdp, auxp, pfn) != NULL);

	memset(&temp, 0, sizeof(temp));
	if (pdp == NULL)
		pdp = &temp;

	pdp->dv_cfdata = pcfp;
	pdp->dv_cfdriver = config_cfdriver_lookup(pcfp->cf_name);
	pdp->dv_unit = pcfp->cf_unit;

	if ((cf = config_search((cfmatch_t)NULL, pdp, auxp)) != NULL) {
		ca = config_cfattach_lookup(cf->cf_name, cf->cf_atname);
		if (ca != NULL) {
			(*ca->ca_attach)(pdp, NULL, auxp);
			pdp->dv_cfdata = NULL;
			return(1);
		}
	}
	pdp->dv_cfdata = NULL;
	return(0);
}

/*
 * this function needs to get enough configured to do a console
 * basically this means start attaching the grfxx's that support 
 * the console. Kinda hacky but it works.
 */
void
config_console()
{	
	struct cfdata *cf;

	config_init();

	/*
	 * we need mainbus' cfdata.
	 */
	cf = config_rootsearch(NULL, "mainbus", "mainbus");
	if (cf == NULL)
		panic("no mainbus");

	/*
	 * Note: The order of the 'atari_config_found()' calls is
	 * important! On the Hades, the 'pci-side' of the config does
	 * some setup for the 'grf-side'. This make it possible to use
	 * a PCI card for both wscons and grfabs.
	 */
	atari_config_found(cf, NULL, "pcib"  , NULL);
	atari_config_found(cf, NULL, "isab"  , NULL);
	atari_config_found(cf, NULL, "grfbus", NULL);
}

/*
 * The system will assign the "booted device" indicator (and thus
 * rootdev if rootspec is wildcarded) to the first partition 'a'
 * in preference of boot.
 */
#include <sys/fcntl.h>		/* XXXX and all that uses it */
#include <sys/proc.h>		/* XXXX and all that uses it */

#include "fd.h"
#include "sd.h"
#include "cd.h"
#include "wd.h"

#if NWD > 0
extern	struct cfdriver wd_cd;
#endif
#if NSD > 0
extern	struct cfdriver sd_cd;  
#endif
#if NCD > 0
extern	struct cfdriver cd_cd;
#endif
#if NFD > 0
extern	struct cfdriver fd_cd;
#endif

struct cfdriver *genericconf[] = {
#if NWD > 0
	&wd_cd,
#endif
#if NSD > 0
	&sd_cd,
#endif
#if NCD > 0
	&cd_cd,
#endif
#if NFD > 0
	&fd_cd,
#endif
	NULL,
};

void
findroot(void)
{
	struct disk *dkp;
	struct partition *pp;
	struct device **devs;
	const struct bdevsw *bdev;
	int i, maj, unit;

	if (boothowto & RB_ASKNAME)
		return;		/* Don't bother looking */

	for (i = 0; genericconf[i] != NULL; i++) {
		for (unit = 0; unit < genericconf[i]->cd_ndevs; unit++) {
			if (genericconf[i]->cd_devs[unit] == NULL)
				continue;

			/*
			 * Find the disk structure corresponding to the
			 * current device.
			 */
			devs = (struct device **)genericconf[i]->cd_devs;
			if ((dkp = disk_find(devs[unit]->dv_xname)) == NULL)
				continue;

			if (dkp->dk_driver == NULL ||
			    dkp->dk_driver->d_strategy == NULL)
				continue;
			
			maj = devsw_name2blk(genericconf[i]->cd_name, NULL, 0);
			if (maj == -1)
				continue;
			bdev = bdevsw_lookup(makedev(maj, 0));
#ifdef DIAGNOSTIC
			if (bdev == NULL)
				panic("findroot: impossible");
#endif
			if (bdev == NULL ||
			    bdev->d_strategy != dkp->dk_driver->d_strategy)
				continue;

			/* Open disk; forces read of disklabel. */
			if ((*bdev->d_open)(MAKEDISKDEV(maj,
			    unit, 0), FREAD|FNONBLOCK, 0, &proc0))
				continue;
			(void)(*bdev->d_close)(MAKEDISKDEV(maj,
			    unit, 0), FREAD|FNONBLOCK, 0, &proc0);
			
			pp = &dkp->dk_label->d_partitions[booted_partition];
			if (pp->p_size != 0 && pp->p_fstype == FS_BSDFFS) {
				booted_device = devs[unit];
				return;
			}
		}
	}
}

/* 
 * mainbus driver 
 */
CFATTACH_DECL(mainbus, sizeof(struct device),
    mbmatch, mbattach, NULL, NULL);

int
mbmatch(pdp, cfp, auxp)
	struct device	*pdp;
	struct cfdata	*cfp;
	void		*auxp;
{
	if (cfp->cf_unit > 0)
		return(0);
	/*
	 * We are always here
	 */
	return(1);
}

/*
 * "find" all the things that should be there.
 */
void
mbattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	printf ("\n");
	config_found(dp, "clock"   , simple_devprint);
	config_found(dp, "grfbus"  , simple_devprint);
	config_found(dp, "kbd"     , simple_devprint);
	config_found(dp, "fdc"     , simple_devprint);
	config_found(dp, "ser"     , simple_devprint);
	config_found(dp, "zs"      , simple_devprint);
	config_found(dp, "ncrscsi" , simple_devprint);
	config_found(dp, "nvr"     , simple_devprint);
	config_found(dp, "lpt"     , simple_devprint);
	config_found(dp, "wdc"     , simple_devprint);
	config_found(dp, "isab"    , simple_devprint);
	config_found(dp, "pcib"    , simple_devprint);
	config_found(dp, "avmebus" , simple_devprint);
}

int
mbprint(auxp, pnp)
	void *auxp;
	const char *pnp;
{
	if (pnp)
		aprint_normal("%s at %s", (char *)auxp, pnp);
	return(UNCONF);
}
