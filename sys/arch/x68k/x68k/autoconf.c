/*	$NetBSD: autoconf.c,v 1.43 2005/06/30 17:03:54 drochner Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.43 2005/06/30 17:03:54 drochner Exp $");

#include "opt_compat_netbsd.h"
#include "scsibus.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/malloc.h>
#include <machine/cpu.h>
#include <x68k/x68k/iodevice.h>
#include <machine/bootinfo.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

void configure(void);
static void findroot(void);
void mbattach(struct device *, struct device *, void *);
int mbmatch(struct device *, struct cfdata *, void *);
int x68k_config_found(struct cfdata *, struct device *, void *, cfprint_t);

static struct device *scsi_find(dev_t);
static struct device *find_dev_byname(const char *);

int x68k_realconfig;

#include <sys/kernel.h>

/*
 * called at boot time, configure all devices on system
 */
void
cpu_configure(void)
{
	x68k_realconfig = 1;

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/* Turn on interrupts */
	(void) spl0();
}

void
cpu_rootconf(void)
{
	findroot();

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

/*
 * use config_search_ia to find appropriate device, then call that device
 * directly with NULL device variable storage.  A device can then 
 * always tell the difference between the real and console init 
 * by checking for NULL.
 */
int
x68k_config_found(struct cfdata *pcfp, struct device *pdp, void *auxp,
    cfprint_t pfn)
{
	struct device temp;
	struct cfdata *cf;
	const struct cfattach *ca;

	if (x68k_realconfig)
		return(config_found(pdp, auxp, pfn) != NULL);

	if (pdp == NULL)
		pdp = &temp;

	/* XXX Emulate 'struct device' of mainbus for cfparent_match() */
	pdp->dv_cfdata = pcfp;
	pdp->dv_cfdriver = config_cfdriver_lookup(pcfp->cf_name);
	pdp->dv_unit = 0;
	if ((cf = config_search_ia((cfmatch_t)NULL, pdp, NULL, auxp)) != NULL) {
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
config_console(void)
{	
	struct cfdata *cf;

	config_init();

	/*
	 * we need mainbus' cfdata.
	 */
	cf = config_rootsearch(NULL, "mainbus", NULL);
	if (cf == NULL)
		panic("no mainbus");
	x68k_config_found(cf, NULL, __UNCONST("intio"), NULL);
	x68k_config_found(cf, NULL, __UNCONST("grfbus"), NULL);
}

dev_t	bootdev = 0;

static void
findroot(void)
{
	int majdev, unit, part;
	const char *name;
	char buf[32];

	if (booted_device)
		return;

	if (boothowto & RB_ASKNAME)
		return;		/* Don't bother looking */

	if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;

	majdev = B_TYPE(bootdev);
	if (X68K_BOOT_DEV_IS_SCSI(majdev)) {
		/*
		 * SCSI device
		 */
		if ((booted_device = scsi_find(bootdev)) != NULL)
			booted_partition = B_X68K_SCSI_PART(bootdev);
		return;
	}
	name = devsw_blk2name(majdev);
	if (name == NULL)
		return;

	part = B_PARTITION(bootdev);
	unit = B_UNIT(bootdev);

	sprintf(buf, "%s%d", name, unit);

	if ((booted_device = find_dev_byname(buf)) != NULL)
		booted_partition = part;
}

static const char *const name_netif[] = { X68K_BOOT_NETIF_STRINGS };

void
device_register(struct device *dev, void *aux)
{
	int majdev;
	char tname[16];

	/*
	 * Handle network interfaces here, the attachment information is
	 * not available driver independantly later.
	 * For disks, there is nothing useful available at attach time.
	 */
	if (dev->dv_class == DV_IFNET) {
		majdev = B_TYPE(bootdev);
		if (X68K_BOOT_DEV_IS_NETIF(majdev)) {
			sprintf(tname, "%s%d",
				name_netif[255 - majdev], B_UNIT(bootdev));
			if (!strcmp(tname, dev->dv_xname))
				goto found;
		}
	}
	return;

found:
	if (booted_device) {
		/* XXX should be a "panic()" */
		printf("warning: double match for boot device (%s, %s)\n",
		       booted_device->dv_xname, dev->dv_xname);
		return;
	}
	booted_device = dev;
}

static const char *const name_scsiif[] = { X68K_BOOT_SCSIIF_STRINGS };

static struct device *
scsi_find(dev_t bdev)
{
#if defined(NSCSIBUS) && NSCSIBUS > 0
	int ifid;
	char tname[16];
	struct device *scsibus;
	struct scsibus_softc *sbsc;
	struct scsipi_periph *periph;

	ifid = B_X68K_SCSI_IF(bdev);
	if (ifid >= sizeof name_scsiif/sizeof name_scsiif[0] ||
					!name_scsiif[ifid]) {
#if defined(COMPAT_09) || defined(COMPAT_10) || defined(COMPAT_11) ||	\
    defined(COMPAT_12) || defined(COMPAT_13)
		/*
		 * old boot didn't pass interface type
		 * try "scsibus0"
		 */
		printf("warning: scsi_find: can't get boot interface -- "
		       "update boot loader\n");
		scsibus = find_dev_byname("scsibus0");
#else
		/* can't determine interface type */
		return NULL;
#endif
	} else {
		/*
		 * search for the scsibus whose parent is
		 * the specified SCSI interface
		 */
		sprintf(tname, "%s%d",
			name_scsiif[ifid], B_X68K_SCSI_IF_UN(bdev));

		for (scsibus = TAILQ_FIRST(&alldevs); scsibus;
					scsibus = TAILQ_NEXT(scsibus, dv_list))
			if (scsibus->dv_parent
			    && !strcmp(tname, scsibus->dv_parent->dv_xname))
				break;
	}
	if (!scsibus)
		return NULL;
	sbsc = (struct scsibus_softc *) scsibus;
	periph = scsipi_lookup_periph(sbsc->sc_channel,
	    B_X68K_SCSI_ID(bdev), B_X68K_SCSI_LUN(bdev));

	return periph ? periph->periph_dev : NULL;
#else
	return NULL;
#endif /* NSCSIBUS > 0 */
}

/*
 * Given a device name, find its struct device
 * XXX - Move this to some common file?
 */
static struct device *
find_dev_byname(const char *name)
{
	struct device *dv;

	for (dv = TAILQ_FIRST(&alldevs); dv; dv = TAILQ_NEXT(dv, dv_list))
		if (!strcmp(dv->dv_xname, name))
			break;

	return dv;
}

/* 
 * mainbus driver 
 */
CFATTACH_DECL(mainbus, sizeof(struct device),
    mbmatch, mbattach, NULL, NULL);

static int mb_attached;

int
mbmatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{

	if (mb_attached)
		return 0;

	return 1;
}

/*
 * "find" all the things that should be there.
 */
void
mbattach(struct device *pdp, struct device *dp, void *auxp)
{

	mb_attached = 1;

	printf("\n");

	config_found(dp, __UNCONST("intio")  , NULL);
	config_found(dp, __UNCONST("grfbus") , NULL);
	config_found(dp, __UNCONST("par")    , NULL);
	config_found(dp, __UNCONST("com")    , NULL);
	config_found(dp, __UNCONST("com")    , NULL);
	config_found(dp, __UNCONST("*")      , NULL);
}
