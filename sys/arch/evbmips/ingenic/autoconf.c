/* $NetBSD: autoconf.c,v 1.2 2015/10/08 17:51:15 macallan Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.2 2015/10/08 17:51:15 macallan Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <net/if_ether.h>

static uint8_t enaddr[ETHER_ADDR_LEN];
static int have_enaddr = false;
void ingenic_set_enaddr(uint8_t *);

/*
 * Configure all devices on system
 */     
void
cpu_configure(void)
{

	intr_init();

	/* Kick off autoconfiguration. */
	(void)splhigh();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");
}

void
cpu_rootconf(void)
{

	rootconf();
}

void
device_register(device_t dev, void *aux)
{
	if (device_is_a(dev, "dme") && have_enaddr) {
		prop_dictionary_t dict;
		prop_data_t blob;
		
		dict = device_properties(dev);

		blob = prop_data_create_data(enaddr, ETHER_ADDR_LEN);
		prop_dictionary_set(dict, "mac-address", blob);
		prop_object_release(blob);
	}
#ifdef notyet
	(*platformsw->apsw_device_register)(dev, aux);
#endif
}

void
ingenic_set_enaddr(uint8_t *goop)
{
	memcpy(enaddr, goop, ETHER_ADDR_LEN);
	have_enaddr = true;
}
