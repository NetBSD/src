/*	$NetBSD: autoconf.c,v 1.4.8.1 2012/08/08 15:51:07 martin Exp $	*/

/*
 * Copyright (c) 1994 Christian E. Hopps
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.4.8.1 2012/08/08 15:51:07 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/cpu.h>

#include <amiga/amiga/cfdev.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/custom.h>

static void findroot(void);

u_long boot_partition;

int amiga_realconfig;

/*
 * called at boot time, configure all devices on system
 */
void
cpu_configure(void)
{

	/*
	 * this is the real thing baby (i.e. not console init)
	 */
	amiga_realconfig = 1;
	custom.intena = INTF_INTEN;

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

#ifdef DEBUG_KERNEL_START
	printf("survived autoconf, going to enable interrupts\n");
#endif

	genppc_cpu_configure();
	custom.intena = INTF_SETCLR | INTF_INTEN;

#ifdef DEBUG_KERNEL_START
	printf("survived configure...\n");
#endif
}

void
cpu_rootconf(void)
{

	findroot();
#ifdef DEBUG_KERNEL_START
	printf("survived findroot()\n");
#endif
	rootconf();
}

/*ARGSUSED*/
int
simple_devprint(void *auxp, const char *pnp)
{

	return QUIET;
}

int
matchname(const char *fp, const char *sp)
{
	int len;

	len = strlen(fp);
	if (strlen(sp) != len)
		return 0;
	if (bcmp(fp, sp, len) == 0)
		return 1;
	return 0;
}

/*
 * use config_search_ia to find appropriate device, then call that device
 * directly with NULL device variable storage.  A device can then
 * always tell the difference betwean the real and console init
 * by checking for NULL.
 */
int
amiga_config_found(cfdata_t pcfp, device_t pdp, void *auxp, cfprint_t pfn)
{
	struct device temp;
	cfdata_t cf;
	const struct cfattach *ca;

	if (amiga_realconfig)
		return config_found(pdp, auxp, pfn) != NULL;

	if (pdp == NULL) {
		memset(&temp, 0, sizeof temp);
		pdp = &temp;
	}

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
	cf = config_rootsearch(NULL, "mainbus", NULL);
	if (cf == NULL)
		panic("no mainbus");

	/*
	 * internal grf.
	 */
	amiga_config_found(cf, NULL, __UNCONST("grfcc"), NULL);

	/*
	 * zbus knows when its not for real and will
	 * only configure the appropriate hardware
	 */
	amiga_config_found(cf, NULL, __UNCONST("zbus"), NULL);
}

/*
 * The system will assign the "booted device" indicator (and thus
 * rootdev if rootspec is wildcarded) to the first partition 'a'
 * in preference of boot.  However, it does walk unit backwards
 * to remain compatible with the old Amiga method of picking the
 * last root found.
 */
#include <sys/fcntl.h>		/* XXXX and all that uses it */
#include <sys/proc.h>		/* XXXX and all that uses it */

#include "fd.h"
#include "sd.h"
#include "cd.h"
#include "wd.h"

#if NFD > 0
extern  struct cfdriver fd_cd;
extern	const struct bdevsw fd_bdevsw;
#endif
#if NSD > 0
extern  struct cfdriver sd_cd;
extern	const struct bdevsw sd_bdevsw;
#endif
#if NCD > 0
extern  struct cfdriver cd_cd;
extern	const struct bdevsw cd_bdevsw;
#endif
#if NWD > 0
extern  struct cfdriver wd_cd;
extern	const struct bdevsw wd_bdevsw;
#endif

struct cfdriver *genericconf[] = {
#if NFD > 0
	&fd_cd,
#endif
#if NSD > 0
	&sd_cd,
#endif
#if NWD > 0
	&wd_cd,
#endif
#if NCD > 0
	&cd_cd,
#endif
	NULL,
};

void
findroot(void)
{
	struct disk *dkp;
	struct partition *pp;
	device_t*devs;
	int i, maj, unit;
	const struct bdevsw *bdp;

#if NSD > 0
	/*
	 * If we have the boot partition offset (boot_partition), try
	 * to locate the device corresponding to that partition.
	 */
#ifdef DEBUG_KERNEL_START
	printf("Boot partition offset is %ld\n", boot_partition);
#endif
	if (boot_partition != 0) {

		for (unit = 0; unit < sd_cd.cd_ndevs; ++unit) {
#ifdef DEBUG_KERNEL_START
			printf("probing for sd%d\n", unit);
#endif
			if (sd_cd.cd_devs[unit] == NULL)
				continue;

			/*
			 * Find the disk corresponding to the current
			 * device.
			 */
			devs = (device_t*)sd_cd.cd_devs;
			if ((dkp = disk_find(devs[unit]->dv_xname)) == NULL)
				continue;

			if (dkp->dk_driver == NULL ||
			    dkp->dk_driver->d_strategy == NULL)
				continue;
			bdp = &sd_bdevsw;
			maj = bdevsw_lookup_major(bdp);
			if ((*bdp->d_open)(MAKEDISKDEV(maj, unit, RAW_PART),
			    FREAD | FNONBLOCK, 0, curlwp))
				continue;
			(*bdp->d_close)(MAKEDISKDEV(maj, unit, RAW_PART),
			    FREAD | FNONBLOCK, 0, curlwp);
			pp = &dkp->dk_label->d_partitions[0];
			for (i = 0; i < dkp->dk_label->d_npartitions;
			    i++, pp++) {
#ifdef DEBUG_KERNEL_START
				printf("sd%d%c type %d offset %d size %d\n",
					unit, i+'a', pp->p_fstype,
					pp->p_offset, pp->p_size);
#endif
				if (pp->p_size == 0 ||
				    (pp->p_fstype != FS_BSDFFS &&
				    pp->p_fstype != FS_SWAP))
					continue;
				if (pp->p_offset == boot_partition) {
					if (booted_device == NULL) {
						booted_device = devs[unit];
						booted_partition = i;
					} else
						printf("Ambiguous boot device\n");
				}
			}
		}
	}
	if (booted_device != NULL)
		return;		/* we found the boot device */
#endif

	for (i = 0; genericconf[i] != NULL; i++) {
		for (unit = genericconf[i]->cd_ndevs - 1; unit >= 0; unit--) {
			if (genericconf[i]->cd_devs[unit] == NULL)
				continue;

			/*
			 * Find the disk structure corresponding to the
			 * current device.
			 */
			devs = (device_t*)genericconf[i]->cd_devs;
			if ((dkp = disk_find(devs[unit]->dv_xname)) == NULL)
				continue;

			if (dkp->dk_driver == NULL ||
			    dkp->dk_driver->d_strategy == NULL)
				continue;

			bdp = NULL;
#if NFD > 0
			if (fd_bdevsw.d_strategy == dkp->dk_driver->d_strategy)
				bdp = &fd_bdevsw;
#endif
#if NSD > 0
			if (sd_bdevsw.d_strategy == dkp->dk_driver->d_strategy)
				bdp = &sd_bdevsw;
#endif
#if NWD > 0
			if (wd_bdevsw.d_strategy == dkp->dk_driver->d_strategy)
				bdp = &wd_bdevsw;
#endif
#if NCD > 0
			if (cd_bdevsw.d_strategy == dkp->dk_driver->d_strategy)
				bdp = &cd_bdevsw;
#endif
#ifdef DIAGNOSTIC
			if (bdp == NULL)
				panic("findroot: impossible");
#endif
			maj = bdevsw_lookup_major(bdp);

			/* Open disk; forces read of disklabel. */
			if ((*bdp->d_open)(MAKEDISKDEV(maj,
			    unit, 0), FREAD|FNONBLOCK, 0, &lwp0))
				continue;
			(void)(*bdp->d_close)(MAKEDISKDEV(maj,
			    unit, 0), FREAD|FNONBLOCK, 0, &lwp0);

			pp = &dkp->dk_label->d_partitions[0];
			if (pp->p_size != 0 && pp->p_fstype == FS_BSDFFS) {
				booted_device = devs[unit];
				booted_partition = 0;
				return;
			}
		}
	}
}

/*
 * Try to determine, if this machine is an A3000, which has a builtin
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
is_a3000(void)
{
	/* this is a dirty kludge.. but how do you do this RIGHT ? :-) */
	extern long boot_fphystart;
	short sc;

	if ((machineid >> 16) == 3000)
		return 1;			/* It's an A3000 */
	if (machineid >> 16)
		return 0;			/* It's not an A3000 */
	/* Machine type is unknown, so try to guess it */
	/* where is fastram on the A4000 ?? */
	/* if fastram is below 0x07000000, assume it's not an A3000 */
	if (boot_fphystart < 0x07000000)
		return 0;
	/*
	 * OK, fastram starts at or above 0x07000000, check specific
	 * machines
	 */
	for (sc = 0; sc < ncfdev; sc++) {
		switch (cfdev[sc].rom.manid) {
		case 2026:		/* Progressive Peripherals, Inc */
			switch (cfdev[sc].rom.prodid) {
			case 0:		/* PPI Mercury - A3000 */
			case 1:		/* PP&S A3000 '040 */
				return 1;
			case 150:	/* PPI Zeus - it's an A2000 */
			case 105:	/* PP&S A2000 '040 */
			case 187:	/* PP&S A500 '040 */
				return 0;
			}
			break;

		case 2112:			/* IVS */
			switch (cfdev[sc].rom.prodid) {
			case 242:
				return 0;	/* A2000 accelerator? */
			}
			break;
		}
	}
	return a3000_flag;		/* XXX let flag tell now */
}

int
is_a4000(void)
{

	if ((machineid >> 16) == 4000)
		return 1;		/* It's an A4000 */
	if ((machineid >> 16) == 1200)
		return 0;		/* It's an A1200, so not A4000 */
	/* Do I need this any more? */
	if ((custom.deniseid & 0xff) == 0xf8)
		return 1;
#ifdef DEBUG
	if (a4000_flag)
		printf("Denise ID = %04x\n", (unsigned short)custom.deniseid);
#endif
	if (machineid >> 16)
		return 0;		/* It's not an A4000 */
	return a4000_flag;		/* Machine type not set */
}

int
is_a1200(void)
{

	if ((machineid >> 16) == 1200)
		return 1;		/* It's an A1200 */
	return 0;			/* Machine type not set */
}
