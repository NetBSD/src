/* $NetBSD: autoconf.c,v 1.13.2.2 2014/08/20 00:03:10 tls Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.13.2.2 2014/08/20 00:03:10 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/disklabel.h>
#include <machine/cpu.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <luna68k/luna68k/isr.h>

static uint booted_adpt, booted_ctlr, booted_unit, booted_part;

/*
 * Determine mass storage and memory configuration for a machine.
 */
void
cpu_configure(void)
{

	booted_device = NULL;	/* set by device drivers (if found) */

	booted_adpt = B_ADAPTOR(bootdev);
	booted_ctlr = B_CONTROLLER(bootdev);
	booted_unit = B_UNIT(bootdev);
	booted_part = B_PARTITION(bootdev);

	(void)splhigh();
	isrinit();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("autoconfig failed, no root");

	spl0();
}

void
device_register(device_t dev, void *aux)
{
	static device_t booted_controller;
	static bool bootdev_found;

	/*
	 * Check booted device.
	 */
	if (bootdev_found)
		return;

	if (booted_adpt == LUNA68K_BOOTADPT_LANCE &&
	    device_is_a(dev, "le")) {
		struct mainbus_attach_args *ma = aux;

		if (booted_ctlr == 0 &&
		    ma->ma_addr == LUNA68K_BOOTADPT_LANCE0_PA) {
			booted_device = dev;
			bootdev_found = true;
		}
		return;
	}

	if (booted_adpt == LUNA68K_BOOTADPT_SPC &&
	    device_is_a(dev, "spc")) {
		struct mainbus_attach_args *ma = aux;

		if ((booted_ctlr == 0 &&
		     (u_int)ma->ma_addr == LUNA68K_BOOTADPT_SPC0_PA) ||
		    (booted_ctlr == 1 &&
		     (u_int)ma->ma_addr == LUNA68K_BOOTADPT_SPC1_PA)) {
			booted_controller = dev;
		}
		return;
	}

	if (booted_controller != NULL &&
	    (device_is_a(dev, "sd") || device_is_a(dev, "cd"))) {
		struct scsipibus_attach_args *sa = aux;
		device_t parent = device_parent(dev);

		if (device_parent(parent) != booted_controller)
			return;
		if (booted_unit != sa->sa_periph->periph_target)
			return;
		booted_device = dev;
		booted_partition = booted_part;
		bootdev_found = true;
		return;
	}
}

void
cpu_rootconf(void)
{

	printf("boot device: %s\n",
		(booted_device) ? device_xname(booted_device) : "<unknown>");

	rootconf();
}
