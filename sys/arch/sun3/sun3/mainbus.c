/*	$NetBSD: mainbus.c,v 1.17.40.1 2014/08/20 00:03:26 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.17.40.1 2014/08/20 00:03:26 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>

static int	main_match(device_t, cfdata_t, void *);
static void	main_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mainbus, 0,
    main_match, main_attach, NULL, NULL);

/*
 * Probe for the mainbus; always succeeds.
 */
static int
main_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

/*
 * Do "direct" configuration for the bus types on mainbus.
 * This controls the order of autoconfig for important things
 * used early.  For example, idprom is used by Ether drivers.
 */
static void
main_attach(device_t parent, device_t self, void *args)
{
	struct confargs ca;
	int i;

	aprint_normal("\n");

	ca.ca_bustag = &mainbus_space_tag;
	ca.ca_dmatag = &mainbus_dma_tag;
	ca.ca_name = NULL;
	ca.ca_paddr = -1;
	ca.ca_intpri = -1;
	ca.ca_intvec = -1;

	for (i = 0; i < BUS__NTYPES; i++) {
		ca.ca_bustype = i;
		(void)config_found(self, &ca, NULL);
	}
}
