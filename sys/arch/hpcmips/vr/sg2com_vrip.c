/*	$NetBSD: sg2com_vrip.c,v 1.4.2.1 2002/01/08 00:25:06 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 TAKEMRUA Shin. All rights reserved.
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

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/sg2comreg.h>
#include <hpcmips/vr/sg2com_vripvar.h>

#include <dev/ic/comvar.h>
#include <dev/ic/comreg.h>

#include "locators.h"

#define SG2COM_VRIPDEBUG
#ifdef SG2COM_VRIPDEBUG
int	sg2com_vrip_debug = 0;
#define	DPRINTF(arg) if (sg2com_vrip_debug) printf arg;
#define	VPRINTF(arg) if (sg2com_vrip_debug || bootverbose) printf arg;
#else
#define	DPRINTF(arg)
#define	VPRINTF(arg) if (bootverbose) printf arg;
#endif

struct sg2com_vrip_softc {
	struct com_softc	sc_com;
	struct bus_space_tag	sc_iot;
};
static struct bus_space_tag sg2com_vrip_cniotx;
static bus_space_tag_t sg2com_vrip_cniot = &sg2com_vrip_cniotx;

static int sg2com_vrip_probe(struct device *, struct cfdata *, void *);
static void sg2com_vrip_attach(struct device *, struct device *, void *);
static int sg2com_vrip_common_probe(bus_space_tag_t, int);
void sg2com_vrip_iot_init(bus_space_tag_t iot, bus_space_tag_t base);
bus_space_protos(bs_notimpl);
bus_space_protos(bs_through);
bus_space_protos(sg2com_vrip);

struct cfattach sg2com_vrip_ca = {
	sizeof(struct sg2com_vrip_softc), sg2com_vrip_probe,
	sg2com_vrip_attach
};

struct bus_space_ops sg2com_vrip_bs_ops = {
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

	/* read (single) */
	sg2com_vrip_bs_r_1,
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
	sg2com_vrip_bs_w_1,
	bs_notimpl_bs_w_2,
	bs_notimpl_bs_w_4,
	bs_notimpl_bs_w_8,

	/* write multiple */
	sg2com_vrip_bs_wm_1,
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
sg2com_vrip_cndb_attach(bus_space_tag_t iot, int iobase, int rate,
    int frequency, tcflag_t cflag, int kgdb)
{

	DPRINTF(("sg2com_vrip_cndb_attach()\n"));
	sg2com_vrip_iot_init(sg2com_vrip_cniot, iot);
	if (!sg2com_vrip_common_probe(sg2com_vrip_cniot, iobase)) {
		DPRINTF(("sg2com_vrip_cndb_attach(): probe failed\n"));
		return (ENOTTY);
	}
	DPRINTF(("sg2com_vrip_cndb_attach(): probe succeeded\n"));
#ifdef KGDB
	if (kgdb)
		return (com_kgdb_attach(sg2com_vrip_cniot, iobase, rate,
		    frequency, cflag));
	else
#endif
		return (comcnattach(sg2com_vrip_cniot, iobase, rate,
		    frequency, cflag));
}

static int
sg2com_vrip_common_probe(bus_space_tag_t iot, int iobase)
{
	bus_space_handle_t ioh;
	int rv;

	if (bus_space_map(iot, iobase, 1, 0, &ioh)) {
		printf(": can't map i/o space\n");
		return 0;
	}
	rv = comprobe1(iot, ioh);
	bus_space_unmap(iot, ioh, 1);

	return (rv);
}

static int
sg2com_vrip_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct vrip_attach_args *va = aux;
	bus_space_tag_t iot = va->va_iot;
	int res;
	
	if (sg2com_vrip_cniot->bs_base == iot &&
	    com_is_console(sg2com_vrip_cniot->bs_base, va->va_addr, 0)) {
		/*
		 *  We have alredy probed.
		 */
		res = 1;
	} else {
		static struct bus_space_tag tmpiot;

		sg2com_vrip_iot_init(&tmpiot, iot);
		res = sg2com_vrip_common_probe(&tmpiot, va->va_addr);
	}

	DPRINTF((res ? ": found COM ports\n" : ": can't probe COM device\n"));

	if (res) {
		va->va_size = COM_NPORTS;
	}

	return (res);
}


static void
sg2com_vrip_attach(struct device *parent, struct device *self, void *aux)
{
	struct sg2com_vrip_softc *vsc = (void *)self;
	struct com_softc *sc = &vsc->sc_com;
	struct vrip_attach_args *va = aux;
	bus_space_tag_t iot = &vsc->sc_iot;
	bus_space_handle_t ioh;

	sg2com_vrip_iot_init(iot, va->va_iot);
	if (bus_space_map(iot, va->va_addr, 1, 0, &ioh)) {
		printf(": can't map bus space\n");
		return;
	}
	sc->sc_iobase = va->va_addr;
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	sc->enable = NULL;
	sc->disable = NULL;

	sc->sc_frequency = SG2COM_FREQ;
	com_attach_subr(sc);
#if 0
	vrip_intr_establish(va->va_vc, va->va_intr, IPL_TTY, comintr, self);
#else
	printf("XXX, sg2com: intrrupt line is unknown\n");
#endif
}

/*
 * bus stuff (registershalf word aligned)
 */
void
sg2com_vrip_iot_init(bus_space_tag_t iot, bus_space_tag_t base)
{

	iot->bs_base = base;
	iot->bs_ops = sg2com_vrip_bs_ops; /* structure assignment */
	iot->bs_ops.bs_r_1 = sg2com_vrip_bs_r_1;
	iot->bs_ops.bs_w_1 = sg2com_vrip_bs_w_1;
	iot->bs_ops.bs_wm_1 = sg2com_vrip_bs_wm_1;
}

u_int8_t
sg2com_vrip_bs_r_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset)
{
	return bus_space_read_1(t->bs_base, bsh, offset * 2);
}

void
sg2com_vrip_bs_w_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t value)
{
	bus_space_write_1(t->bs_base, bsh, offset * 2, value);
}

void
sg2com_vrip_bs_wm_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int8_t *addr, bus_size_t count)
{
	bus_space_write_multi_1(t->bs_base, bsh, offset * 2, addr, count);
}
