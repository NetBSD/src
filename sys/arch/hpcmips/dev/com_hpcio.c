/*	$NetBSD: com_hpcio.c,v 1.2.2.3 2002/06/23 17:36:50 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 TAKEMRUA Shin. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
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
 */

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/termios.h>

#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/dev/com_hpciovar.h>

#include <dev/hpc/hpciovar.h>
#include <dev/ic/comvar.h>
#include <dev/ic/comreg.h>

#include "locators.h"

#define COM_HPCIODEBUG
#ifdef COM_HPCIODEBUG
int	com_hpcio_debug = 0;
#define	DPRINTF(arg...) do { if (com_hpcio_debug) printf(arg); } while (0)
#else
#define	DPRINTF(arg...) do {} while (0)
#endif

#define COM_HPCIO_BYTE_ALIGNMENT	0
#define COM_HPCIO_HALFWORD_ALIGNMENT	1

struct com_hpcio_softc {
	struct com_softc	hsc_com;
	struct bus_space_tag	hsc_iot;
	struct hpcio_chip	*hsc_hc;
};
static struct bus_space_tag com_hpcio_cniotx;
static bus_space_tag_t com_hpcio_cniot = &com_hpcio_cniotx;
static int com_hpcio_cniobase;

static int com_hpcio_probe(struct device *, struct cfdata *, void *);
static void com_hpcio_attach(struct device *, struct device *, void *);
static int com_hpcio_common_probe(bus_space_tag_t, int, int *);
void com_hpcio_iot_init(bus_space_tag_t iot, bus_space_tag_t base);
bus_space_protos(bs_notimpl);
bus_space_protos(bs_through);
bus_space_protos(com_hpcio);

struct cfattach com_hpcio_ca = {
	sizeof(struct com_hpcio_softc), com_hpcio_probe, com_hpcio_attach
};

struct bus_space_ops com_hpcio_bs_ops = {
	/* mapping/unmapping */
	bs_through_bs_map,
	bs_through_bs_unmap,
	bs_through_bs_subregion,

	/* allocation/deallocation */
	bs_through_bs_alloc,
	bs_through_bs_free,

	/* get kernel virtual address */
	bs_through_bs_vaddr, /* there is no linear mapping */

	/* Mmap bus space for user */
	bs_through_bs_mmap,

	/* barrier */
	bs_through_bs_barrier,

	/* probe */
	bs_through_bs_peek,
	bs_through_bs_poke,

	/* read (single) */
	com_hpcio_bs_r_1,
	bs_notimpl_bs_r_2,
	bs_notimpl_bs_r_4,
	bs_notimpl_bs_r_8,

	/* read multiple */
	bs_notimpl_bs_rm_1,
	bs_notimpl_bs_rm_2,
	bs_notimpl_bs_rm_4,
	bs_notimpl_bs_rm_8,

	/* read region */
	bs_notimpl_bs_rr_1,
	bs_notimpl_bs_rr_2,
	bs_notimpl_bs_rr_4,
	bs_notimpl_bs_rr_8,

	/* write (single) */
	com_hpcio_bs_w_1,
	bs_notimpl_bs_w_2,
	bs_notimpl_bs_w_4,
	bs_notimpl_bs_w_8,

	/* write multiple */
	com_hpcio_bs_wm_1,
	bs_notimpl_bs_wm_2,
	bs_notimpl_bs_wm_4,
	bs_notimpl_bs_wm_8,

	/* write region */
	bs_notimpl_bs_wr_1,
	bs_notimpl_bs_wr_2,
	bs_notimpl_bs_wr_4,
	bs_notimpl_bs_wr_8,

#ifdef BUS_SPACE_HAS_REAL_STREAM_METHODS
	/* read stream (single) */
	bs_notimpl_bs_rs_1,
	bs_notimpl_bs_rs_2,
	bs_notimpl_bs_rs_4,
	bs_notimpl_bs_rs_8,

	/* read multiple stream */
	bs_notimpl_bs_rms_1,
	bs_notimpl_bs_rms_2,
	bs_notimpl_bs_rms_4,
	bs_notimpl_bs_rms_8,

	/* read region stream */
	bs_notimpl_bs_rrs_1,
	bs_notimpl_bs_rrs_2,
	bs_notimpl_bs_rrs_4,
	bs_notimpl_bs_rrs_8,

	/* write stream (single) */
	bs_notimpl_bs_ws_1,
	bs_notimpl_bs_ws_2,
	bs_notimpl_bs_ws_4,
	bs_notimpl_bs_ws_8,

	/* write multiple stream */
	bs_notimpl_bs_wms_1,
	bs_notimpl_bs_wms_2,
	bs_notimpl_bs_wms_4,
	bs_notimpl_bs_wms_8,

	/* write region stream */
	bs_notimpl_bs_wrs_1,
	bs_notimpl_bs_wrs_2,
	bs_notimpl_bs_wrs_4,
	bs_notimpl_bs_wrs_8,
#endif /* BUS_SPACE_HAS_REAL_STREAM_METHODS */

	/* set multi */
	bs_notimpl_bs_sm_1,
	bs_notimpl_bs_sm_2,
	bs_notimpl_bs_sm_4,
	bs_notimpl_bs_sm_8,

	/* set region */
	bs_notimpl_bs_sr_1,
	bs_notimpl_bs_sr_2,
	bs_notimpl_bs_sr_4,
	bs_notimpl_bs_sr_8,

	/* copy */
	bs_notimpl_bs_c_1,
	bs_notimpl_bs_c_2,
	bs_notimpl_bs_c_4,
	bs_notimpl_bs_c_8,
};

int
com_hpcio_cndb_attach(bus_space_tag_t iot, int iobase, int rate,
    int frequency, tcflag_t cflag, int kgdb)
{
	int alignment;

	DPRINTF("com_hpcio_cndb_attach()\n");
	if (!com_hpcio_common_probe(iot, iobase, &alignment)) {
		DPRINTF("com_hpcio_cndb_attach(): probe failed\n");
		return (ENOTTY);
	}
	if (alignment == COM_HPCIO_HALFWORD_ALIGNMENT) {
		DPRINTF("com_hpcio_cndb_attach(): half word aligned\n");
		com_hpcio_iot_init(&com_hpcio_cniotx, iot);
		com_hpcio_cniot = &com_hpcio_cniotx;
	} else {
		com_hpcio_cniot = iot;
	}
	com_hpcio_cniobase = iobase;
	DPRINTF("com_hpcio_cndb_attach(): probe succeeded\n");
#ifdef KGDB
	if (kgdb)
		return (com_kgdb_attach(com_hpcio_cniot, iobase, rate,
		    frequency, cflag));
	else
#endif
		return (comcnattach(com_hpcio_cniot, iobase, rate,
		    frequency, cflag));
}

static int
com_hpcio_common_probe(bus_space_tag_t iot, int iobase, int *alignment)
{
	bus_space_handle_t ioh;
	static struct bus_space_tag tmpiot;
	int rv;

	/*
	 * try byte aligned register
	 */
	*alignment = COM_HPCIO_BYTE_ALIGNMENT;
	if (bus_space_map(iot, iobase, 1, 0, &ioh))
		return 0;
	rv = comprobe1(iot, ioh);
	bus_space_unmap(iot, ioh, 1);

	if (rv != 0)
		return (rv);

	/*
	 * try half word aligned register
	 */
	*alignment = COM_HPCIO_HALFWORD_ALIGNMENT;
	com_hpcio_iot_init(&tmpiot, iot);
	if (bus_space_map(&tmpiot, iobase, 1, 0, &ioh))
		return 0;
	rv = comprobe1(&tmpiot, ioh);
	bus_space_unmap(&tmpiot, ioh, 1);

	return (rv);
}

static int
com_hpcio_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hpcio_attach_args *haa = aux;
	bus_space_tag_t iot = haa->haa_iot;
	int addr, alignment;

	if (cf->cf_loc[HPCIOIFCF_PLATFORM] != HPCIOIFCF_PLATFORM_DEFAULT) {
		platid_mask_t mask;

		mask = PLATID_DEREF(cf->cf_loc[HPCIOIFCF_PLATFORM]);
		if (!platid_match(&platid, &mask))
			return (0); /* platform id didn't match */
	}

	if ((addr = cf->cf_loc[HPCIOIFCF_ADDR]) == HPCIOIFCF_ADDR_DEFAULT)
		return (0); /* address wasn't specified */

	return com_hpcio_common_probe(iot, addr, &alignment);
}


static void
com_hpcio_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_hpcio_softc *hsc = (void *)self;
	struct com_softc *sc = &hsc->hsc_com;
	struct hpcio_attach_args *haa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int addr, port, mode, alignment, *loc;

	loc = sc->sc_dev.dv_cfdata->cf_loc;
	addr = loc[HPCIOIFCF_ADDR];
	printf(" addr %x", addr);
	if ((com_hpcio_cniot == haa->haa_iot ||
	    com_hpcio_cniot->bs_base == haa->haa_iot) &&
	    com_hpcio_cniobase == addr &&
	    com_is_console(com_hpcio_cniot, addr, 0)) {
		iot = com_hpcio_cniot;
		if (com_hpcio_cniot->bs_base == haa->haa_iot)
			printf(", half word aligned");
	} else {
		com_hpcio_common_probe(haa->haa_iot, addr, &alignment);
		if (alignment == COM_HPCIO_HALFWORD_ALIGNMENT) {
			printf(", half word aligned");
			iot = &hsc->hsc_iot;
			com_hpcio_iot_init(iot, haa->haa_iot);
		} else {
			iot = haa->haa_iot;
		}
	}
	if (bus_space_map(iot, addr, 1, 0, &ioh)) {
		printf(": can't map bus space\n");
		return;
	}
	sc->sc_iobase = addr;
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	sc->enable = NULL;
	sc->disable = NULL;

	sc->sc_frequency = COM_FREQ;
	com_attach_subr(sc);

	hsc->hsc_hc = (*haa->haa_getchip)(haa->haa_sc, loc[HPCIOIFCF_IOCHIP]);
	port = loc[HPCIOIFCF_PORT];
	mode = HPCIO_INTR_LEVEL | HPCIO_INTR_HIGH;
	hpcio_intr_establish(hsc->hsc_hc, port, mode, comintr, sc);
}

/*
 * bus stuff (registershalf word aligned)
 */
void
com_hpcio_iot_init(bus_space_tag_t iot, bus_space_tag_t base)
{

	iot->bs_base = base;
	iot->bs_ops = com_hpcio_bs_ops; /* structure assignment */
	iot->bs_ops.bs_r_1 = com_hpcio_bs_r_1;
	iot->bs_ops.bs_w_1 = com_hpcio_bs_w_1;
	iot->bs_ops.bs_wm_1 = com_hpcio_bs_wm_1;
}

u_int8_t
com_hpcio_bs_r_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset)
{
	return bus_space_read_1(t->bs_base, bsh, offset * 2);
}

void
com_hpcio_bs_w_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t value)
{
	bus_space_write_1(t->bs_base, bsh, offset * 2, value);
}

void
com_hpcio_bs_wm_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int8_t *addr, bus_size_t count)
{
	bus_space_write_multi_1(t->bs_base, bsh, offset * 2, addr, count);
}
