/*	$NetBSD: autoconf.c,v 1.29.6.2 2021/05/08 15:51:30 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.29.6.2 2021/05/08 15:51:30 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <dev/cons.h>
#include <dev/pci/pcivar.h>
#include <dev/i2c/i2cvar.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/bootinfo.h>
#include <machine/pio.h>

static struct btinfo_rootdevice *bi_rdev;
static struct btinfo_bootpath *bi_path;
static struct btinfo_net *bi_net;
static struct btinfo_prodfamily *bi_pfam;
static struct btinfo_model *bi_model;

struct sandpoint_i2cdev {
	const char *	name;
	const char *	compat;
	uint32_t	model_mask;
	i2c_addr_t	addr;
};

static const struct sandpoint_i2cdev dlink_i2cdevs[] = {
[0] =	{
		.name = "strtc",
		.compat = "st,m41t80",
		.addr = 0x68,
	},
[1] =	{
		.name = NULL
	}
};

static const struct sandpoint_i2cdev iomega_i2cdevs[] = {
[0] =	{
		.name = "dsrtc",
		.compat = "dallas,ds1307",
		.addr = 0x68,
	},
[1] =	{
		.name = NULL
	}
};

static const struct sandpoint_i2cdev kurobox_i2cdevs[] = {
[0] =	{
		.name = "rs5c372rtc",
		.compat = "ricoh,rs5c372a",
		.addr = 0x32,
	},
[1] =	{
		.name = NULL
	}
};

static const struct sandpoint_i2cdev nhnas_i2cdevs[] = {
[0] =	{
		.name = "pcf8563rtc",
		.compat = "nxp,pcf8563",
		.addr = 0x51,
	},
[1] =	{
		.name = NULL
	}
};

static const struct sandpoint_i2cdev qnap_i2cdevs[] = {
[0] =	{
		.name = "s390rtc",
		.compat = "sii,s35390a",
		.addr = 0x30,
	},
[1] =	{
		.name = NULL
	}
};

static const struct sandpoint_i2cdev synology_i2cdevs[] = {
[0] =	{
		.name = "rs5c372rtc",
		.compat = "ricoh,rs5c372a",
		.addr = 0x32,
	},
[1] =	{
		.name = "lmtemp",
		.compat = "national,lm75",
		.addr = 0x48,
		.model_mask = BI_MODEL_THERMAL,
	},
[2] =	{
		.name = NULL
	}
};

static const struct device_compatible_entry sandpoint_i2c_compat[] = {
	{ .compat = "dlink",		.data = &dlink_i2cdevs },
	{ .compat = "iomega",		.data = &iomega_i2cdevs },
	{ .compat = "kurobox",		.data = &kurobox_i2cdevs },
	/* kurot4 has same i2c devices as kurobox */
	{ .compat = "kurot4",		.data = &kurobox_i2cdevs },
	{ .compat = "nhnas",		.data = &nhnas_i2cdevs },
	{ .compat = "qnap",		.data = &qnap_i2cdevs },
	{ .compat = "synology",		.data = &synology_i2cdevs },
	DEVICE_COMPAT_EOL
};

/*
 * We provide a device handle implementation for i2c device enumeration.
 */
static int
sandpoint_i2c_enumerate_devices(device_t dev, devhandle_t call_handle, void *v)
{
	struct i2c_enumerate_devices_args *args = v;
	const struct device_compatible_entry *dce;
	const struct sandpoint_i2cdev *i2cdev;
	prop_dictionary_t props;
	bool cbrv;

	KASSERT(bi_pfam != NULL);

	const char *fam_name = bi_pfam->name;
	dce = device_compatible_lookup(&fam_name, 1, sandpoint_i2c_compat);
	if (dce == NULL) {
		/* no i2c devices for this model. */
		return 0;
	}
	i2cdev = dce->data;
	KASSERT(i2cdev != NULL);

	for (; i2cdev->name != NULL; i2cdev++) {
		if (i2cdev->model_mask != 0) {
			KASSERT(bi_model != NULL);
			if ((i2cdev->model_mask & bi_model->flags) == 0) {
				/* skip this device. */
				continue;
			}
		}

		props = prop_dictionary_create();

		args->ia->ia_addr = i2cdev->addr;
		args->ia->ia_name = i2cdev->name;
		args->ia->ia_clist = i2cdev->compat;
		args->ia->ia_clist_size = strlen(i2cdev->compat) + 1;
		args->ia->ia_prop = props;
		/* no devhandle for child devices. */
		devhandle_invalidate(&args->ia->ia_devhandle);

		cbrv = args->callback(dev, args);

		prop_object_release(props);

		if (!cbrv) {
			break;
		}
	}

	return 0;
}

static device_call_t
sandpoint_i2c_devhandle_lookup_device_call(devhandle_t handle, const char *name,
    devhandle_t *call_handlep)
{
	if (strcmp(name, "i2c-enumerate-devices") == 0) {
		return sandpoint_i2c_enumerate_devices;
	}
	return NULL;
}

static const struct devhandle_impl sandpoint_i2c_devhandle_impl = {
	.type = DEVHANDLE_TYPE_PRIVATE,
	.lookup_device_call = sandpoint_i2c_devhandle_lookup_device_call,
};

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
	bi_model = lookup_bootinfo(BTINFO_MODEL);

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	genppc_cpu_configure();
}

void
cpu_rootconf(void)
{

	if (bi_path != NULL)
		booted_kernel = bi_path->bootpath;

	aprint_normal("boot device: %s\n",
	    booted_device ? device_xname(booted_device) : "<unknown>");
	rootconf();
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

			pd = prop_data_create_nocopy(bi_net->mac_address,
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
		/* For i2c device enumeration. */
		devhandle_t devhandle = {
			.impl = &sandpoint_i2c_devhandle_impl,
		};
		device_set_handle(dev, devhandle);
	}
}
