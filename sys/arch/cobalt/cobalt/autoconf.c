/*	$NetBSD: autoconf.c,v 1.7 2000/06/06 18:00:13 soren Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/cpu.h>

struct device *booted_device;
int booted_partition;

static void	findroot(void);

int		cpuspeed = 100;		/* Until we know more precisely. */

void
cpu_configure()
{
	(void)splhigh();

	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");

	_splnone();
}

void
cpu_rootconf()
{
	findroot();

	printf("boot device: %s\n",
		booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

extern char	bootstring[];
extern int	netboot;

static void
findroot(void)
{
	struct device *dv;

	if (booted_device)
		return;

	if ((booted_device == NULL) && netboot == 0)
		for (dv = alldevs.tqh_first; dv != NULL;
		     dv = dv->dv_list.tqe_next)
			if (dv->dv_class == DV_DISK &&
			    !strcmp(dv->dv_cfdata->cf_driver->cd_name, "wd"))
				    booted_device = dv;

	/*
	 * XXX Match up MBR boot specification with BSD disklabel for root?
	 */
	booted_partition = 0;

	return;
}

void
device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	if ((booted_device == NULL) && (netboot == 1))
		if (dev->dv_class == DV_IFNET)
			booted_device = dev;
}
