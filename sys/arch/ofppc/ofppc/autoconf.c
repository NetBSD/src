/*	$NetBSD: autoconf.c,v 1.13 2003/07/15 02:46:32 lukem Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.13 2003/07/15 02:46:32 lukem Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>
#include <machine/platform.h>
#include <machine/powerpc.h>

static void canonicalize_bootpath(void);

extern char bootpath[];
char cbootpath[256];
struct device *booted_device;	/* boot device */
int booted_partition;		/* ...and partition on that device */

/*
 * Determine device configuration for a machine.
 */
void
cpu_configure()
{

	canonicalize_bootpath();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	(void)spl0();
}

/*
 * Setup root device.
 * Configure swap area.
 */
void
cpu_rootconf()
{

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

void
device_register(dev, aux)
	struct device *dev;
	void *aux;
{

	(*platform.device_register)(dev, aux);
}

static void
canonicalize_bootpath()
{
	int node;
	char *p;
	char last[32];

printf("bootpath %s\n", bootpath);
	/*
	 * Strip kernel name.  bootpath contains "OF-path"/"kernel".
	 *
	 * for example:
	 *   /pci/scsi/disk@0:0/netbsd		(FirePower)
	 */
	strcpy(cbootpath, bootpath);
	while ((node = OF_finddevice(cbootpath)) == -1) {
		if ((p = strrchr(cbootpath, '/')) == NULL)
			break;
		*p = 0;
	}

	if (node == -1) {
		/* Cannot canonicalize... use bootpath anyway. */
		strcpy(cbootpath, bootpath);

		return;
	}

	/*
	 * cbootpath is a valid OF path.  Use package-to-path to
	 * canonicalize pathname.
	 */

	/* Back up the last component for later use. */
	if ((p = strrchr(cbootpath, '/')) != NULL)
		strcpy(last, p + 1);
	else
		last[0] = 0;

	memset(cbootpath, 0, sizeof(cbootpath));
	OF_package_to_path(node, cbootpath, sizeof(cbootpath) - 1);

	/*
	 * At this point, cbootpath contains like:
	 * "/pci/scsi@1/disk"
	 *
	 * The last component may have no address... so append it.
	 */
	p = strrchr(cbootpath, '/');
	if (p != NULL && strchr(p, '@') == NULL) {
		/* Append it. */
		if ((p = strrchr(last, '@')) != NULL)
			strcat(cbootpath, p);
	}

	/*
	 * Strip off the partition again (saving it in booted_parition).
	 */
	if ((p = strrchr(cbootpath, ':')) != NULL) {
		*p++ = 0;
		booted_partition = *p - 'a';
	}
printf("cbootpath %s\n", cbootpath);
}

