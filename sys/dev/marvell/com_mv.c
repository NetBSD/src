/*	$NetBSD: com_mv.c,v 1.6 2013/09/01 04:51:24 kiyohara Exp $	*/
/*
 * Copyright (c) 2007, 2010 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_mv.c,v 1.6 2013/09/01 04:51:24 kiyohara Exp $");

#include "opt_com.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/termios.h>

#include <dev/marvell/gtvar.h>
#include <dev/marvell/marvellreg.h>
#include <dev/marvell/marvellvar.h>

#include <dev/ic/comvar.h>

#include <prop/proplib.h>

#define MVUART_SIZE		0x80

#define MVUART_REG_USR		0x7c	/* XXXX: What is this??? */


static int mvuart_match(device_t, struct cfdata *, void *);
static void mvuart_attach(device_t, device_t, void *);

static int mvuart_intr(void *);

CFATTACH_DECL_NEW(mvuart_gt, sizeof(struct com_softc),
    mvuart_match, mvuart_attach, NULL, NULL);
CFATTACH_DECL_NEW(mvuart_mbus, sizeof(struct com_softc),
    mvuart_match, mvuart_attach, NULL, NULL);

#ifdef COM_REGMAP
#define MVUART_INIT_REGS(regs, tag, hdl, addr, size)			\
	do {								\
		int _i;							\
									\
		regs.cr_iot = tag;					\
		regs.cr_ioh = hdl;					\
		regs.cr_iobase = addr;					\
		regs.cr_nports = size;					\
		for (_i = 0; _i < __arraycount(regs.cr_map); _i++)	\
			regs.cr_map[_i] = com_std_map[_i] << 2;		\
	} while (0)
#define CSR_WRITE_1(r, o, v)    \
	bus_space_write_1((r)->cr_iot, (r)->cr_ioh, (r)->cr_map[o], v)
#define CSR_READ_1(r, o)        \
	bus_space_read_1((r)->cr_iot, (r)->cr_ioh, (r)->cr_map[o])
#else
#define MVUART_INIT_REGS(regs, tag, hdl, addr, size) \
	COM_INIT_REGS(regs, tag, hdl, addr)
#define CSR_WRITE_1(r, o, v)    \
        bus_space_write_1((r)->cr_iot, (r)->cr_ioh, o, v)
#define CSR_READ_1(r, o)        \
	bus_space_read_1((r)->cr_iot, (r)->cr_ioh, o)
#endif

struct {
	int model;
	int type;
} mvuart_extensions[] = {
	{ MARVELL_ARMADAXP_MV78130,	COM_TYPE_ARMADAXP },
	{ MARVELL_ARMADAXP_MV78160,	COM_TYPE_ARMADAXP },
	{ MARVELL_ARMADAXP_MV78230,	COM_TYPE_ARMADAXP },
	{ MARVELL_ARMADAXP_MV78260,	COM_TYPE_ARMADAXP },
	{ MARVELL_ARMADAXP_MV78460,	COM_TYPE_ARMADAXP },
};

/* ARGSUSED */
static int
mvuart_match(device_t parent, struct cfdata *match, void *aux)
{
	struct marvell_attach_args *mva = aux;
	struct com_regs regs;
	bus_space_handle_t ioh;

	if (strcmp(mva->mva_name, match->cf_name) != 0)
		return 0;
	if (mva->mva_offset == MVA_OFFSET_DEFAULT ||
	    mva->mva_irq == MVA_IRQ_DEFAULT)
		return 0;

	if (com_is_console(mva->mva_iot, mva->mva_addr + mva->mva_offset, NULL))
		goto console;

	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh, mva->mva_offset,
	    MVUART_SIZE, &ioh))
		return 0;
	MVUART_INIT_REGS(regs, mva->mva_iot, ioh, mva->mva_offset, MVUART_SIZE);
	if (!com_probe_subr(&regs))
		return 0;

console:
	mva->mva_size = MVUART_SIZE;
	return 1;
}

/* ARGSUSED */
static void
mvuart_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc *sc = device_private(self);
	struct marvell_attach_args *mva = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	prop_dictionary_t dict = device_properties(self);
	int i;

	sc->sc_dev = self;

	if (!prop_dictionary_get_uint32(dict, "frequency", &sc->sc_frequency)) {
		aprint_error(": no frequency property\n");
		return;
	}

	iot = mva->mva_iot;
	if (!com_is_console(iot, mva->mva_addr + mva->mva_offset, &ioh)) {
		if (bus_space_subregion(iot, mva->mva_ioh, mva->mva_offset,
		    mva->mva_size, &ioh)) {
			aprint_error(": can't map registers\n");
			return;
		}
	}
	MVUART_INIT_REGS(sc->sc_regs,
	    iot, ioh, mva->mva_addr + mva->mva_offset, mva->mva_size);

	for (i = 0; i < __arraycount(mvuart_extensions); i++)
		if (mva->mva_model == mvuart_extensions[i].model) {
			sc->sc_type = mvuart_extensions[i].type;
			break;
		}

	com_attach_subr(sc);

	if (sc->sc_type == COM_TYPE_ARMADAXP)
		marvell_intr_establish(mva->mva_irq, IPL_SERIAL,
		    mvuart_intr, sc);
	else
		marvell_intr_establish(mva->mva_irq, IPL_SERIAL, comintr, sc);
}

static int
mvuart_intr(void *arg)
{
	struct com_softc *sc = arg;
	struct com_regs *regsp = &sc->sc_regs;
	int timeout;
	uint8_t iir, v;

	if (!device_is_active(sc->sc_dev))
		return 0;

	KASSERT(regsp != NULL);

	mutex_spin_enter(&sc->sc_lock);
	iir = CSR_READ_1(regsp, COM_REG_IIR);
	if ((iir & IIR_BUSY) == IIR_BUSY) {
		/*
		 * XXXXX: What is this?  I don't found in Marvell datasheet.
		 *        Maybe workaround for BUG in UART.
		 */
		v = bus_space_read_1(regsp->cr_iot, regsp->cr_ioh,
		    MVUART_REG_USR);
		for (timeout = 10000; (v & 0x1) != 0; timeout--) {
			if (timeout <= 0) {
				aprint_error_dev(sc->sc_dev,
				    "timeout while waiting for BUSY interrupt "
				    "acknowledge\n");
				mutex_spin_exit(&sc->sc_lock);
				return 0;
			}
			v = bus_space_read_1(regsp->cr_iot, regsp->cr_ioh,
			    MVUART_REG_USR);
		}
		CSR_WRITE_1(regsp, COM_REG_LCR, sc->sc_lcr);
	}
	mutex_spin_exit(&sc->sc_lock);

	return comintr(arg);
}

#ifdef COM_REGMAP
int mvuart_cnattach(bus_space_tag_t, bus_addr_t, int, uint32_t, int);

int
mvuart_cnattach(bus_space_tag_t iot, bus_addr_t addr, int baud,
		uint32_t sysfreq, int mode)
{
	struct com_regs regs;

	MVUART_INIT_REGS(regs, iot, 0x0, addr, MVUART_SIZE);

	return comcnattach1(&regs, baud, sysfreq, COM_TYPE_16550_NOERS, mode);
}
#endif /* COM_REGMAP */
