/*	$NetBSD: autoconf.c,v 1.1.2.6 2012/01/04 16:17:52 matt Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.1.2.6 2012/01/04 16:17:52 matt Exp $");

#define _MIPS_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <mips/rmi/rmixlvar.h>

static void	findroot(void);
static void	rmixl_bus_dma_init(void);

void
cpu_configure()
{
	rmixl_bus_dma_init();

	rmixl_fmn_init();

	intr_init();

	/* Kick off autoconfiguration. */
	(void)splhigh();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/*
	 * Hardware interrupts will be enabled in
	 * sys/arch/mips/mips/mips3_clockintr.c:mips3_initclocks()
	 * to avoid hardclock(9) by CPU INT5 before softclockintr is
	 * initialized in initclocks().
	 */
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
	if (booted_device)
		return;

	if ((booted_device == NULL) && netboot == 0) {
		device_t dv;
		deviter_t di;

		for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST);
		     dv != NULL;
		     dv = deviter_next(&di)) {
			if (device_class(dv) == DV_DISK
			    && (device_is_a(dv, "wd")
				|| device_is_a(dv, "sd")
				|| device_is_a(dv, "ld")))
				booted_device = dv;
		}
		deviter_release(&di);
	}

	/*
	 * XXX Match up MBR boot specification with BSD disklabel for root?
	 */
	booted_partition = 0;
}

static void
addprop_integer(device_t dev, const char *name, uint32_t val)
{
	prop_number_t pn;
	pn = prop_number_create_integer(val);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(device_properties(dev), name, pn) == false) {
		printf("WARNING: unable to set %s property for %s",
		    name, device_xname(dev));
	}
	prop_object_release(pn);
}

void
device_register(device_t dev, void *aux)
{

	if (booted_device == NULL
	    && netboot == 1
	    && device_class(dev) == DV_IFNET)
		booted_device = dev;

	if (device_is_a(dev, "com")) {
		device_t parent = device_parent(dev);
		if (device_is_a(parent, "mainbus")) {
			addprop_integer(dev, "frequency", 133333333);
		} if (device_is_a(parent, "pci")) {
			struct pci_attach_args * const pa = aux;
			int bus;
			pci_decompose_tag(pa->pa_pc, pa->pa_tag, &bus,
			    NULL, NULL);
			if (bus == 0) {
				addprop_integer(dev, "frequency", 133333333);
			}
		} else if (device_is_a(parent, "obio")) {
			addprop_integer(dev, "frequency", 66666666);
		}
	}
}

static void
rmixl_bus_dma_init(void)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	int error;

	/* 64bit dmatag was staticly initialized */

	/* dma space for addr < 4GB */
	if (rcp->rc_dmat32 == NULL) {
		error = bus_dmatag_subregion(rcp->rc_dmat64,
		    0, (bus_addr_t)1 << 32, &rcp->rc_dmat32, 0);
		if (error)
			panic("%s: failed to create 32bit dma tag: %d",
			    __func__, error);
	}

	/* dma space for addr < 512MB (KSEG0/1) */
	if (rcp->rc_dmat29 == NULL) {
		error = bus_dmatag_subregion(rcp->rc_dmat32,
		    0, (bus_addr_t)1 << 29, &rcp->rc_dmat29, 0);
		if (error)
			panic("%s: failed to create 29bit dma tag: %d",
			    __func__, error);
	}
}
