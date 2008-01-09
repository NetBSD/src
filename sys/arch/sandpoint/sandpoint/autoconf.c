/*	$NetBSD: autoconf.c,v 1.12.50.2 2008/01/09 01:48:36 matt Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)autoconf.c	7.1 (Berkeley) 5/9/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.12.50.2 2008/01/09 01:48:36 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <dev/pci/pcivar.h>

#include <machine/bootinfo.h>

static struct btinfo_rootdevice *bi_rdev;
static struct btinfo_bootpath *bi_path;

#include <dev/cons.h>
#include <machine/pio.h>

/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure()
{

	bi_rdev = lookup_bootinfo(BTINFO_ROOTDEVICE);
	bi_path = lookup_bootinfo(BTINFO_BOOTPATH);

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	genppc_cpu_configure();
}

char *booted_kernel; /* should be a genuine filename */

void
cpu_rootconf()
{

	if (bi_path != NULL)
		booted_kernel = bi_path->bootpath;

	aprint_normal("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");
	setroot(booted_device, booted_partition);
}

void
device_register(struct device *dev, void *aux)
{

	if (bi_rdev == NULL)
		return; /* no clue to determine */

	if (dev->dv_class == DV_IFNET) {
		if (device_is_a(dev, bi_rdev->devname)) {
			struct pci_attach_args *pa = aux;
			unsigned tag = (unsigned)pa->pa_tag;

			if (bi_rdev->cookie == tag)
				booted_device = dev;
		}
		return;
	}
	if (dev->dv_class == DV_DISK) {
		/* XXX add diskboot case later XXX */
	}
}

#if 0
void
findroot(void)
{
	int unit, part;
	struct device *dv;
	char buf[32];
	const char *name;

#if 0
	printf("howto %x bootdev %x ", boothowto, bootdev);
#endif

	if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;

	name = devsw_blk2name((bootdev >> B_TYPESHIFT) & B_TYPEMASK);
	if (name == NULL)
		return;

	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;

	sprintf(buf, "%s%d", name, unit);
	for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
	    dv = TAILQ_NEXT(dv, dv_list)) {
		if (strcmp(buf, dv->dv_xname) == 0) {
			booted_device = dv;
			booted_partition = part;
			return;
		}
	}
}
#endif
