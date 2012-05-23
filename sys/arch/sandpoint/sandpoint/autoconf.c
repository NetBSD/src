/*	$NetBSD: autoconf.c,v 1.23.2.2 2012/05/23 10:07:48 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.23.2.2 2012/05/23 10:07:48 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <dev/cons.h>
#include <dev/pci/pcivar.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/bootinfo.h>
#include <machine/pio.h>

static struct btinfo_rootdevice *bi_rdev;
static struct btinfo_bootpath *bi_path;
static struct btinfo_net *bi_net;
static struct btinfo_prodfamily *bi_pfam;

struct i2cdev {
	const char *family;
	const char *name;
	int addr;
};

static struct i2cdev rtcmodel[] = {
    { "dlink",    "strtc",      0x68 },
    { "iomega",   "dsrtc",      0x68 },
    { "kurobox",  "rs5c372rtc", 0x32 },
    { "kurot4",   "rs5c372rtc", 0x32 },
    { "nhnas",    "pcf8563rtc", 0x51 },
    { "qnap",     "s390rtc",    0x30 },
    { "synology", "rs5c372rtc", 0x32 },
};

static void add_i2c_child_devices(device_t, const char *);

/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure(void)
{

	bi_rdev = lookup_bootinfo(BTINFO_ROOTDEVICE);
	bi_path = lookup_bootinfo(BTINFO_BOOTPATH);
	bi_net = lookup_bootinfo(BTINFO_NET);
	bi_pfam = lookup_bootinfo(BTINFO_PRODFAMILY);

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	genppc_cpu_configure();
}

char *booted_kernel; /* should be a genuine filename */

void
cpu_rootconf(void)
{

	if (bi_path != NULL)
		booted_kernel = bi_path->bootpath;

	aprint_normal("boot device: %s\n",
	    booted_device ? device_xname(booted_device) : "<unknown>");
	setroot(booted_device, booted_partition);
}

void
device_register(device_t dev, void *aux)
{
	struct pci_attach_args *pa;
	static device_t boot_parent = NULL, net_parent = NULL;
	static pcitag_t boot_tag, net_tag;
	pcitag_t tag;

	if (device_is_a(dev, "skc")) {
		pa = aux;
		if (bi_rdev != NULL && bi_rdev->cookie == pa->pa_tag) {
			boot_parent = dev;
			boot_tag = pa->pa_tag;
		}
		if (bi_net != NULL && bi_net->cookie == pa->pa_tag) {
			net_parent = dev;
			net_tag = pa->pa_tag;
		}
	}
	else if (device_class(dev) == DV_IFNET) {
		if (device_is_a(device_parent(dev), "pci")) {
			pa = aux;
			tag = pa->pa_tag;
		} else if (device_parent(dev) == boot_parent)
			tag = boot_tag;
		else if (device_parent(dev) == net_parent)
			tag = net_tag;
		else
			tag = 0;

		if (bi_rdev != NULL && device_is_a(dev, bi_rdev->devname)
		    && bi_rdev->cookie == tag)
			booted_device = dev;

		if (bi_net != NULL && device_is_a(dev, bi_net->devname)
		    && bi_net->cookie == tag) {
			prop_data_t pd;

			pd = prop_data_create_data_nocopy(bi_net->mac_address,
			    ETHER_ADDR_LEN);
			KASSERT(pd != NULL);
			if (prop_dictionary_set(device_properties(dev),
			    "mac-address", pd) == false)
				printf("WARNING: unable to set mac-addr "
				    "property for %s\n", device_xname(dev));
			prop_object_release(pd);
			bi_net = NULL;	/* do it just once */
		}
	}
	else if (bi_rdev != NULL && device_class(dev) == DV_DISK
	    && device_is_a(dev, bi_rdev->devname)
	    && device_unit(dev) == (bi_rdev->cookie >> 8)) {
		booted_device = dev;
		booted_partition = bi_rdev->cookie & 0xff;
	}
	else if (device_is_a(dev, "ociic") && bi_pfam != NULL) {
		add_i2c_child_devices(dev, bi_pfam->name);
	}
}

static void
add_i2c_child_devices(device_t self, const char *family)
{
	struct i2cdev *rtc;
	prop_dictionary_t pd;
	prop_array_t pa;
	int i;

	rtc = NULL;
	for (i = 0; i < (int)(sizeof(rtcmodel)/sizeof(rtcmodel[0])); i++) {
		if (strcmp(family, rtcmodel[i].family) == 0) {
			rtc = &rtcmodel[i];
			goto found;
		}
	}
	return;

 found:
	pd = prop_dictionary_create();
	pa = prop_array_create();
	prop_dictionary_set_cstring_nocopy(pd, "name", rtc->name);
	prop_dictionary_set_uint32(pd, "addr", rtc->addr);
	prop_array_add(pa, pd);
	prop_dictionary_set(device_properties(self), "i2c-child-devices", pa);
	prop_object_release(pd);
	prop_object_release(pa);
}
