/*	$NetBSD: autoconf.c,v 1.61.8.1 2012/07/05 17:36:31 riz Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.61.8.1 2012/07/05 17:36:31 riz Exp $");

#include "opt_md.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/malloc.h>
#include <machine/disklabel.h>
#include <machine/cpu.h>
#include <atari/atari/device.h>

#if defined(MEMORY_DISK_HOOKS)
#include <dev/md.h>
#endif

#include "ioconf.h"

static void findroot(void);
int mbmatch(device_t, cfdata_t, void *);
void mbattach(device_t, device_t, void *);
#if 0
int mbprint(void *, const char *);
#endif

int atari_realconfig;
#include <sys/kernel.h>

/*
 * called at boot time, configure all devices on system
 */
void
cpu_configure(void)
{

	atari_realconfig = 1;

	init_sicallback();

	if (config_rootfound("mainbus", __UNCONST("mainbus")) == NULL)
		panic("no mainbus found");
}

void
cpu_rootconf(void)
{

	findroot();
#if defined(MEMORY_DISK_HOOKS)
	/*
	 * XXX
	 * quick hacks for atari's traditional "auto-load from floppy on open"
	 * installation md(4) ramdisk.
	 * See sys/arch/atari/dev/md_root.c for details.
	 */
#define RAMD_NDEV	3	/* XXX */

	if ((boothowto & RB_ASKNAME) != 0) {
		int md_major, i;
		dev_t md_dev;
		cfdata_t cf;
		struct md_softc *sc;

		md_major = devsw_name2blk("md", NULL, 0);
		if (md_major >= 0) {
			for (i = 0; i < RAMD_NDEV; i++) {
				md_dev = MAKEDISKDEV(md_major, i, RAW_PART);
				cf = malloc(sizeof(*cf), M_DEVBUF,
				    M_ZERO|M_WAITOK);
				if (cf == NULL)
					break;	/* XXX */
				cf->cf_name = md_cd.cd_name;
				cf->cf_atname = md_cd.cd_name;
				cf->cf_unit = i;
				cf->cf_fstate = FSTATE_STAR;
				/* XXX mutex */
				sc = device_private(config_attach_pseudo(cf));
				if (sc == NULL)
					break;	/* XXX */
			}
		}
	}
#endif
	setroot(booted_device, booted_partition);
}

/*ARGSUSED*/
int
simple_devprint(void *auxp, const char *pnp)
{

	return QUIET;
}

/*
 * use config_search_ia to find appropriate device, then call that device
 * directly with NULL device variable storage.  A device can then 
 * always tell the difference between the real and console init 
 * by checking for NULL.
 */
int
atari_config_found(cfdata_t pcfp, device_t pdp, void *auxp, cfprint_t pfn)
{
	struct device temp;
	cfdata_t cf;
	const struct cfattach *ca;

	if (atari_realconfig)
		return config_found(pdp, auxp, pfn) != NULL;

	memset(&temp, 0, sizeof(temp));
	if (pdp == NULL)
		pdp = &temp;

	pdp->dv_cfdata = pcfp;
	pdp->dv_cfdriver = config_cfdriver_lookup(pcfp->cf_name);
	pdp->dv_unit = pcfp->cf_unit;

	if ((cf = config_search_ia(NULL, pdp, NULL, auxp)) != NULL) {
		ca = config_cfattach_lookup(cf->cf_name, cf->cf_atname);
		if (ca != NULL) {
			(*ca->ca_attach)(pdp, NULL, auxp);
			pdp->dv_cfdata = NULL;
			return 1;
		}
	}
	pdp->dv_cfdata = NULL;
	return 0;
}

/*
 * this function needs to get enough configured to do a console
 * basically this means start attaching the grfxx's that support 
 * the console. Kinda hacky but it works.
 */
void
config_console(void)
{	
	cfdata_t cf;

	config_init();

	/*
	 * we need mainbus' cfdata.
	 */
	cf = config_rootsearch(NULL, "mainbus", __UNCONST("mainbus"));
	if (cf == NULL)
		panic("no mainbus");

	/*
	 * Note: The order of the 'atari_config_found()' calls is
	 * important! On the Hades, the 'pci-side' of the config does
	 * some setup for the 'grf-side'. This make it possible to use
	 * a PCI card for both wscons and grfabs.
	 */
	atari_config_found(cf, NULL, __UNCONST("pcib")  , NULL);
	atari_config_found(cf, NULL, __UNCONST("isab")  , NULL);
	atari_config_found(cf, NULL, __UNCONST("grfbus"), NULL);
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
#include "ioconf.h"

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
	device_t *devs;
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
			devs = (device_t *)genericconf[i]->cd_devs;
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
			    unit, 0), FREAD|FNONBLOCK, 0, &lwp0))
				continue;
			(void)(*bdev->d_close)(MAKEDISKDEV(maj,
			    unit, 0), FREAD|FNONBLOCK, 0, &lwp0);
			
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
CFATTACH_DECL_NEW(mainbus, 0,
    mbmatch, mbattach, NULL, NULL);

static int mb_attached;

int
mbmatch(device_t parent, cfdata_t cf, void *aux)
{

	if (mb_attached)
		return 0;
	/*
	 * We are always here
	 */
	return 1;
}

/*
 * "find" all the things that should be there.
 */
void
mbattach(device_t parent, device_t self, void *aux)
{

	mb_attached = 1;

	printf ("\n");
	config_found(self, __UNCONST("clock")   , simple_devprint);
	config_found(self, __UNCONST("grfbus")  , simple_devprint);
	config_found(self, __UNCONST("kbd")     , simple_devprint);
	config_found(self, __UNCONST("fdc")     , simple_devprint);
	config_found(self, __UNCONST("ser")     , simple_devprint);
	config_found(self, __UNCONST("zs")      , simple_devprint);
	config_found(self, __UNCONST("ncrscsi") , simple_devprint);
	config_found(self, __UNCONST("nvr")     , simple_devprint);
	config_found(self, __UNCONST("lpt")     , simple_devprint);
	config_found(self, __UNCONST("wdc")     , simple_devprint);
	config_found(self, __UNCONST("ne")      , simple_devprint);
	config_found(self, __UNCONST("isab")    , simple_devprint);
	config_found(self, __UNCONST("pcib")    , simple_devprint);
	config_found(self, __UNCONST("avmebus") , simple_devprint);
}

#if 0
int
mbprint(void *aux, const char *pnp)
{

	if (pnp)
		aprint_normal("%s at %s", (char *)aux, pnp);
	return UNCONF;
}
#endif
