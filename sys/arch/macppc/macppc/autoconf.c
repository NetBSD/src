/*	$NetBSD: autoconf.c,v 1.3 1998/06/24 15:13:43 tsubai Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/ofw/openfirm.h>

#include <machine/powerpc.h>
#include <machine/pio.h>
#include <machine/autoconf.h>

extern int cold;

void configure __P((void));
void findroot __P((void));

struct device *booted_device;	/* boot device */
int booted_partition;		/* ...and partition on that device */

struct devnametobdevmaj powermac_nam2blk[] = {
	{ "ofdisk",	0 },
	{ "sd",		4 },
	{ "md",		9 },
	{ NULL,		0 },
};

#define INT_ENABLE_REG (interrupt_reg + 0x24)
#define INT_CLEAR_REG  (interrupt_reg + 0x28)
extern u_char *interrupt_reg;

/*
 * Determine device configuration for a machine.
 */
void
configure()
{
	interrupt_reg = mapiodev(0xf3000000, NBPG);
	out32rb(INT_ENABLE_REG, 0);		/* disable all intr. */
	out32rb(INT_CLEAR_REG, 0xffffffff);	/* clear pending intr. */

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	(void)spl0();
	cold = 0;
}

/*
 * Setup root device.
 * Configure swap area.
 */
void
cpu_rootconf()
{
	findroot();

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition, powermac_nam2blk);
}

/*
 * Try to find the device we were booted from to set rootdev.
 */
void
findroot()
{
	int chosen, len;
	int targ;
	int lun = 0;	/* XXX */
	struct device *dv;
	char p[64];

	chosen = OF_finddevice("/chosen");
	if (chosen == -1)
		goto out;

	bzero(p, sizeof(p));
	len = OF_getprop(chosen, "bootpath", p, sizeof(p));
	if (len < 0 || len >= sizeof(p))
		goto out;

	/* XXX for now... */
	if (strncmp(p, "scsi/sd", 7) != 0)
		goto out;

	booted_partition = p[len - 2] - '0';
	targ = p[len - 4] - '0';

	for (dv = alldevs.tqh_first; dv; dv=dv->dv_list.tqe_next) {
		if (strncmp(dv->dv_xname, "scsibus0", 8) == 0) { /* XXX */
			struct scsibus_softc *sdv = (void *)dv;

			if (sdv->sc_link[targ][lun] == NULL)
				continue;
			booted_device = sdv->sc_link[targ][lun]->device_softc;
			break;
		}
	}

out:
	dk_cleanup();
}
