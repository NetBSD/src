/*	$NetBSD: autoconf.c,v 1.65 2009/11/05 18:13:07 dyoung Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.65 2009/11/05 18:13:07 dyoung Exp $");

#include "opt_compat_netbsd.h"
#include "scsibus.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <machine/cpu.h>
#include <machine/bootinfo.h>
#include <machine/autoconf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

static void findroot(void);
static device_t scsi_find(dev_t);

int x68k_realconfig;

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
	    booted_device ? device_xname(booted_device) : "<unknown>");

	setroot(booted_device, booted_partition);
}

void
config_console(void)
{	
	mfp_config_console();
	grf_config_console();
	ite_config_console();
}

uint32_t bootdev = 0;

static void
findroot(void)
{
	int majdev, unit, part;
	const char *name;

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

	if ((booted_device = device_find_by_driver_unit(name, unit)) != NULL)
		booted_partition = part;
}

static const char *const name_netif[] = { X68K_BOOT_NETIF_STRINGS };

void
device_register(device_t dev, void *aux)
{
	int majdev;
	char tname[16];

	/*
	 * Handle network interfaces here, the attachment information is
	 * not available driver independently later.
	 * For disks, there is nothing useful available at attach time.
	 */
	if (device_class(dev) == DV_IFNET) {
		majdev = B_TYPE(bootdev);
		if (X68K_BOOT_DEV_IS_NETIF(majdev)) {
			sprintf(tname, "%s%d",
				name_netif[255 - majdev], B_UNIT(bootdev));
			if (!strcmp(tname, device_xname(dev)))
				goto found;
		}
	}
	return;

found:
	if (booted_device) {
		/* XXX should be a "panic()" */
		printf("warning: double match for boot device (%s, %s)\n",
		       device_xname(booted_device), device_xname(dev));
		return;
	}
	booted_device = dev;
}

static const char *const name_scsiif[] = { X68K_BOOT_SCSIIF_STRINGS };

static device_t
scsi_find(dev_t bdev)
{
#if defined(NSCSIBUS) && NSCSIBUS > 0
	int ifid;
	char tname[16];
	device_t scsibus;
	deviter_t di;
	struct scsibus_softc *sbsc;
	struct scsipi_periph *periph;

	ifid = B_X68K_SCSI_IF(bdev);
	if (ifid >= sizeof name_scsiif/sizeof name_scsiif[0] ||
					!name_scsiif[ifid]) {
#ifdef COMPAT_13
		/*
		 * old boot didn't pass interface type
		 * try "scsibus0"
		 */
		printf("warning: scsi_find: can't get boot interface -- "
		       "update boot loader\n");
		scsibus = device_find_by_xname("scsibus0");
#else
		/* can't determine interface type */
		return NULL;
#endif
	} else {
		/*
		 * search for the scsibus whose parent is
		 * the specified SCSI interface
		 */
		sprintf(tname, "%s%" PRIu64,
			name_scsiif[ifid], B_X68K_SCSI_IF_UN(bdev));

		for (scsibus = deviter_first(&di, DEVITER_F_ROOT_FIRST);
		     scsibus != NULL;
		     scsibus = deviter_next(&di)) {
			if (device_parent(scsibus)
			    && strcmp(tname, device_xname(device_parent(scsibus))) == 0)
				break;
		}
		deviter_release(&di);
	}
	if (scsibus == NULL)
		return NULL;
	sbsc = device_private(scsibus);
	periph = scsipi_lookup_periph(sbsc->sc_channel,
	    B_X68K_SCSI_ID(bdev), B_X68K_SCSI_LUN(bdev));

	return periph ? periph->periph_dev : NULL;
#else
	return NULL;
#endif /* NSCSIBUS > 0 */
}
