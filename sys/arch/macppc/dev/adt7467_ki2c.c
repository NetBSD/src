/*	$NetBSD: adt7467_ki2c.c,v 1.2.80.1 2008/05/16 02:22:44 yamt Exp $	*/

/*-
 * Copyright (C) 2005 Michael Lorenz
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * a driver for the ADT7467 environmental controller found in the iBook G4 
 * and probably other Apple machines 
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adt7467_ki2c.c,v 1.2.80.1 2008/05/16 02:22:44 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <uvm/uvm_extern.h>

#include <dev/ofw/openfirm.h>
#include <macppc/dev/ki2cvar.h>

#include <dev/i2c/adt7467var.h>

static void adt7467_ki2c_attach(device_t, device_t, void *);
static int adt7467_ki2c_match(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(adt7467_ki2c, sizeof(struct adt7467c_softc),
    adt7467_ki2c_match, adt7467_ki2c_attach, NULL, NULL);

int
adt7467_ki2c_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ki2c_confargs *ka = aux;
	char compat[32];
	
	if (strcmp(ka->ka_name, "fan") != 0)
		return 0;

	memset(compat, 0, sizeof(compat));
	OF_getprop(ka->ka_node, "compatible", compat, sizeof(compat));
	if (strcmp(compat, "adt7467") != 0)
		return 0;
	
	return 1;
}

void
adt7467_ki2c_attach(device_t parent, device_t self, void *aux)
{
	struct adt7467c_softc *sc = device_private(self);
	struct ki2c_confargs *ka = aux;
	int node, rev, stp;
	uint8_t reg, buf;

	node = ka->ka_node;
	sc->sc_node = node;
	sc->parent = parent;
	sc->address = ka->ka_addr & 0xfe;

	iic_acquire_bus(ka->ka_tag, 0);
	reg = 0x3f;
	iic_exec(ka->ka_tag, I2C_OP_READ, ka->ka_addr & 0xfe, &reg, 1,
	    &buf, 1, 0);
	rev = (buf & 0xf0) >> 4;
	stp = (buf & 0x0f);
	iic_release_bus(ka->ka_tag, 0);

	aprint_normal(" ADT7467 thermal monitor and fan controller, "
	    "rev. %d.%d\n", rev, stp);
	sc->sc_i2c = ka->ka_tag;
	adt7467c_setup(self);
}
