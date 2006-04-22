/* $Id: com_arbus.c,v 1.1.8.2 2006/04/22 11:37:42 simonb Exp $ */
/*-
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_arbus.c,v 1.1.8.2 2006/04/22 11:37:42 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/ttydefaults.h>
#include <sys/types.h>

#include <machine/bus.h>

#include <dev/cons.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <mips/cpuregs.h>
#include <mips/atheros/include/arbusvar.h>
#include <mips/atheros/include/ar531xreg.h>

struct com_arbus_softc {
	struct com_softc sc_com;
};

static bus_space_tag_t com_arbus_get_bus_space_tag(void);
static int com_arbus_match(struct device *, struct cfdata *, void *);
static void com_arbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(com_arbus, sizeof(struct com_arbus_softc),
    com_arbus_match, com_arbus_attach, NULL, NULL);

#if 0
#ifdef	TTYDEF_SPEED
#define	COM_ARBUS_BAUD	TTYDEF_SPEED
#else
#define	COM_ARBUS_BAUD	115200
#endif
#endif
#define	COM_ARBUS_BAUD	115200

int	com_arbus_baud = COM_ARBUS_BAUD;

#define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

int
com_arbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct arbus_attach_args	*aa = aux;
	bus_space_handle_t		bsh;
	int				rv;

	if (com_is_console(com_arbus_get_bus_space_tag(), aa->aa_addr, NULL)) {
		return 1;
	}

	if (bus_space_map(aa->aa_bst, aa->aa_addr, aa->aa_size,
		0, &bsh))
		return 0;

	rv = comprobe1(aa->aa_bst, bsh);
	bus_space_unmap(aa->aa_bst, bsh, aa->aa_size);

	return rv;
}

void
com_arbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_arbus_softc *arsc = (void *)self;
	struct com_softc *sc = &arsc->sc_com;
	struct arbus_attach_args *aa = aux;

	sc->sc_iot = com_arbus_get_bus_space_tag();
	sc->sc_iobase = aa->aa_addr;
	sc->sc_frequency = curcpu()->ci_cpu_freq / 4;

	if (!com_is_console(sc->sc_iot, sc->sc_iobase, &sc->sc_ioh) &&
	    bus_space_map(sc->sc_iot, sc->sc_iobase, aa->aa_size, 0,
		&sc->sc_ioh) != 0) {
		printf(": can't map registers\n");
		return;
	}

	com_attach_subr(sc);

	if (aa->aa_irq >= 0) {
		arbus_intr_establish(aa->aa_irq, comintr, sc);
	}
}

/*
 * This hideousness is to cope with the fact that the 16550 registers need
 * a stride.  Yech!
 */
static struct mips_bus_space com_arbus_space;

static void
com_arbus_write_1(void *v, bus_space_handle_t h, bus_size_t off, uint8_t val)
{
	bus_space_write_1((bus_space_tag_t)v, h, (off * 4) + 3, val);
}

static uint8_t
com_arbus_read_1(void *v, bus_space_handle_t h, bus_size_t off)
{
	return bus_space_read_1((bus_space_tag_t)v, h, (off * 4) + 3);
}

static void
com_arbus_read_multi_1(void *v, bus_space_handle_t h, bus_size_t off,
    uint8_t *data, bus_size_t cnt)
{
	while (cnt-- > 0) {
		*data++ = com_arbus_read_1(v, h, off);
	}
}

static void
com_arbus_write_multi_1(void *v, bus_space_handle_t h, bus_size_t off,
    const uint8_t *data, bus_size_t cnt)
{
	while (cnt-- > 0) {
		com_arbus_write_1(v, h, off, *data++);
	}
}

static void
com_arbus_barrier(void *v, bus_space_handle_t h, bus_size_t o, bus_size_t l,
    int f)
{
	bus_space_barrier((bus_space_tag_t)v, h, o, l, f);
}

static int
com_arbus_map(void *v, bus_addr_t addr, bus_size_t size, int flag,
    bus_space_handle_t *bshp, int acct)
{
	return bus_space_map((bus_space_tag_t)v, addr, size, flag, bshp);
}

static void
com_arbus_unmap(void *v, bus_space_handle_t bsh, bus_size_t size, int acct)
{
	bus_space_unmap((bus_space_tag_t)v, bsh, size);
}

bus_space_tag_t
com_arbus_get_bus_space_tag(void)
{
	static int done = 0;
	if (!done) {
		com_arbus_space.bs_cookie = arbus_get_bus_space_tag();
		com_arbus_space.bs_w_1 = com_arbus_write_1;
		com_arbus_space.bs_r_1 = com_arbus_read_1;
		com_arbus_space.bs_wm_1 = com_arbus_write_multi_1;
		com_arbus_space.bs_rm_1 = com_arbus_read_multi_1;
		com_arbus_space.bs_barrier = com_arbus_barrier;
		com_arbus_space.bs_map = com_arbus_map;
		com_arbus_space.bs_unmap = com_arbus_unmap;
		done++;
	}
	return &com_arbus_space;
}

void
com_arbus_cnattach(bus_addr_t addr)
{
	bus_space_tag_t		bst;
	bus_space_handle_t	bsh;
	uint32_t		sysfreq;
	extern void		mdputs(const char *);

	bst = com_arbus_get_bus_space_tag();
	if (bus_space_map(bst, addr, 0x1000, 0, &bsh))
		return;

	sysfreq = curcpu()->ci_cpu_freq / 4;

	comcnattach(bst, addr, com_arbus_baud, sysfreq,
	    COM_TYPE_NORMAL, CONMODE);
}

