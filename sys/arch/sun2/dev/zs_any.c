/*	$NetBSD: zs_any.c,v 1.1 2001/04/06 15:14:39 fredette Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and Matthew Fredette.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Zilog Z8530 Dual UART driver (machine-dependent part)
 *
 * Runs two serial lines per chip using slave drivers.
 * Plain tty/async lines use the zs_async slave.
 * Sun keyboard/mouse uses the zs_kbd/zs_ms slaves.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/z8530var.h>

#ifdef	KGDB
#include <uvm/uvm_extern.h>

#include <machine/idprom.h>
#include <machine/pmap.h>
#include <dev/sun/cons.h>
#endif

#include <sun2/sun2/machdep.h>
#include <dev/ic/z8530reg.h>

/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int	zs_any_match __P((struct device *, struct cfdata *, void *));
static void	zs_any_attach __P((struct device *, struct device *, void *));

struct cfattach zs_obio_ca = {
	sizeof(struct zsc_softc), zs_any_match, zs_any_attach
};

struct cfattach zs_obmem_ca = {
	sizeof(struct zsc_softc), zs_any_match, zs_any_attach
};

struct cfattach zs_mbmem_ca = {
	sizeof(struct zsc_softc), zs_any_match, zs_any_attach
};

/*
 * Is the zs chip present?
 */
static int
zs_any_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	/* Make sure there is something there... */
	if (!bus_space_probe(ca->ca_bustag, 0, ca->ca_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL))
		return (0);

	/* Default interrupt priority (always splbio==2) */
	if (ca->ca_intpri == -1)
		ca->ca_intpri = ZSHARD_PRI;

	return (1);
}

/*
 * Attach a found zs.
 */
static void
zs_any_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *) self;
	struct confargs *ca = aux;
	bus_space_handle_t bh;

        zsc->zsc_bustag = ca->ca_bustag;
        zsc->zsc_dmatag = ca->ca_dmatag;
        zsc->zsc_promunit = self->dv_unit;
        zsc->zsc_node = 0;
        
	/* Map in the device. */
	if (bus_space_map(ca->ca_bustag, ca->ca_paddr, sizeof(struct zsdevice), 
			  BUS_SPACE_MAP_LINEAR, &bh))
		panic("zs_any_attach: can't map");

	/* This is where we break the bus_space(9) abstraction: */
	zs_attach(zsc, (void *)bh, ca->ca_intpri);
}

#ifdef	KGDB
/*
 * Find a zs mapped by the PROM.  Currently this only works to find
 * zs0 on obio.
 */
void *
zs_find_prom(unit)
    int unit;
{
	bus_addr_t zs0_phys;
	bus_space_handle_t bh;

	if (unit != 0)
		return (NULL);

	/*
	 * The physical address of zs0 is model-dependent.
	 */
	zs0_phys = (cpu_machine_id == SUN2_MACH_120 ?
	    	    0x002000 : 0x7f2000);
	    
	if (sun2_find_prom_map(zs0_phys, PMAP_OBIO, sizeof(struct zsdevice), &bh))
		return (NULL);

	return ((void*) bh);
}
#endif	/* KGDB */
