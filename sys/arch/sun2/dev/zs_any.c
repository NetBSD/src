/*	$NetBSD: zs_any.c,v 1.12 2005/06/19 20:00:28 thorpej Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs_any.c,v 1.12 2005/06/19 20:00:28 thorpej Exp $");

#include "opt_kgdb.h"

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
#include <sun2/dev/cons.h>
#endif

#include <sun2/sun2/machdep.h>
#include <dev/ic/z8530reg.h>
#include <dev/sun/kbd_reg.h>

/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int	zs_any_match(struct device *, struct cfdata *, void *);
static void	zs_any_attach(struct device *, struct device *, void *);

CFATTACH_DECL(zs_obio, sizeof(struct zsc_softc),
    zs_any_match, zs_any_attach, NULL, NULL);

CFATTACH_DECL(zs_obmem, sizeof(struct zsc_softc),
    zs_any_match, zs_any_attach, NULL, NULL);

CFATTACH_DECL(zs_mbmem, sizeof(struct zsc_softc),
    zs_any_match, zs_any_attach, NULL, NULL);

/*
 * Is the zs chip present?
 */
static int 
zs_any_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;
	int matched;

	/* Make sure there is something there... */
	if (bus_space_map(ma->ma_bustag, ma->ma_paddr, sizeof(struct zsdevice), 
			  0, &bh))
		return (0);
	matched = (bus_space_peek_1(ma->ma_bustag, bh, 0, NULL) == 0);
	bus_space_unmap(ma->ma_bustag, bh, sizeof(struct zsdevice));
	if (!matched)
		return (0);

	/* Default interrupt priority (always splbio==2) */
	if (ma->ma_pri == -1)
		ma->ma_pri = ZSHARD_PRI;

	return (1);
}

/*
 * Attach a found zs.
 */
static void 
zs_any_attach(struct device *parent, struct device *self, void *aux)
{
	struct zsc_softc *zsc = (void *) self;
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;

        zsc->zsc_bustag = ma->ma_bustag;
        zsc->zsc_dmatag = ma->ma_dmatag;
        zsc->zsc_promunit = self->dv_unit;
        zsc->zsc_node = 0;
        
	/* Map in the device. */
	if (bus_space_map(ma->ma_bustag, ma->ma_paddr, sizeof(struct zsdevice), 
			  BUS_SPACE_MAP_LINEAR, &bh))
		panic("zs_any_attach: can't map");

	/* This is where we break the bus_space(9) abstraction: */
	zs_attach(zsc, (void *)bh, ma->ma_pri);
}

int 
zs_console_flags(int promunit, int node, int channel)
{
	int cookie, flags = 0;

	/*
	 * Use `promunit' and `channel' to derive the PROM
	 * stdio handles that correspond to this device.
	 */
	if (promunit == 0)
		cookie = PROMDEV_TTYA + channel;
	else if (promunit == 1 && channel == 0)
		cookie = PROMDEV_KBD;
	else
		cookie = -1;

	/*
	 * We have the console keyboard only if it's a Sun-2 
	 * keyboard or better. (i.e., not a Sun-1 parallel kbd).
	 */
	if (cookie == prom_stdin() &&
	    (cookie != PROMDEV_KBD || prom_kbdid() >= KB_SUN2))
		flags |= ZS_HWFLAG_CONSOLE_INPUT;

	/*
	 * Prevent the keyboard from matching the output device
	 * (note that PROMDEV_KBD == PROMDEV_SCREEN == 0!).
	 */
	if (cookie != PROMDEV_KBD && cookie == prom_stdout())
		flags |= ZS_HWFLAG_CONSOLE_OUTPUT;

	return (flags);
}

#ifdef	KGDB
extern	int sun68k_find_prom_map(bus_addr_t, bus_type_t, int,
	    bus_space_handle_t *);

/*
 * Find a zs mapped by the PROM.  Currently this only works to find
 * zs0 on obio.
 */
void *
zs_find_prom(int unit)
{
	bus_addr_t zs0_phys;
	bus_space_handle_t bh;

	if (unit != 0)
		return (NULL);

	/*
	 * The physical address of zs0 is model-dependent.
	 */
	zs0_phys = (cpu_machine_id == ID_SUN2_120 ? 0x002000 : 0x7f2000);
	if (sun68k_find_prom_map(zs0_phys, PMAP_OBIO, sizeof(struct zsdevice),
	    &bh))
		return (NULL);

	return (bh);
}
#endif	/* KGDB */
