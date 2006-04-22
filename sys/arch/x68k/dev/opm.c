/*	$NetBSD: opm.c,v 1.15.6.1 2006/04/22 11:38:08 simonb Exp $	*/

/*
 * Copyright (c) 1995 Masanobu Saitoh, Takuya Harakawa.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Masanobu Saitoh.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
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
 */

/*
 * Temporary implementation: not fully bus.h'fied.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: opm.c,v 1.15.6.1 2006/04/22 11:38:08 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <machine/opmreg.h>
#include <arch/x68k/dev/opmvar.h>
#include <arch/x68k/dev/intiovar.h>

struct opm_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bht;
	u_int8_t		sc_regs[0x100];
	struct opm_voice	sc_vdata[8];
};

struct opm_softc	*opm0;	/* XXX */

static int opm_match(struct device *, struct cfdata *, void *);
static void opm_attach(struct device *, struct device *, void *);

CFATTACH_DECL(opm, sizeof (struct opm_softc),
    opm_match, opm_attach, NULL, NULL);

static int 
opm_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct intio_attach_args *ia = aux;

	if (strcmp(ia->ia_name, "opm") != 0)
		return 0;

	if (ia->ia_addr == INTIOCF_ADDR_DEFAULT)
		ia->ia_addr = 0xe90000;
	ia->ia_size = 0x2000;
	if (intio_map_allocate_region (parent, ia, INTIO_MAP_TESTONLY))
		return 0;

	return 1;
}

static void 
opm_attach(struct device *parent, struct device *self, void *aux)
{
	struct opm_softc *sc = (struct opm_softc *)self;
	struct intio_attach_args *ia = aux;
	int r;

	printf ("\n");
	ia->ia_size = 0x2000;
	r = intio_map_allocate_region (parent, ia, INTIO_MAP_ALLOCATE);
#ifdef DIAGNOSTIC
	if (r)
		panic ("IO map for OPM corruption??");
#endif

	sc->sc_bst = ia->ia_bst;
	r = bus_space_map (sc->sc_bst,
			   ia->ia_addr, ia->ia_size,
			   BUS_SPACE_MAP_SHIFTED,
			   &sc->sc_bht);
#ifdef DIAGNOSTIC
	if (r)
		panic ("Cannot map IO space for OPM.");
#endif

	/* XXX device_unit() abuse */
	if (device_unit(&sc->sc_dev) == 0)
		opm0 = sc;	/* XXX */

	return;
}

void opm_set_volume(int, int);
void opm_set_key(int, int);
void opm_set_voice(int, struct opm_voice *);
void opm_set_voice_sub(int, struct opm_operator *);
inline static void writeopm(int, int);
inline static int readopm(int);
void opm_key_on(u_char);
void opm_key_off(u_char);
int opmopen(dev_t, int, int);
int opmclose(dev_t);

inline static void 
writeopm(int reg, int dat)
{
	while (bus_space_read_1 (opm0->sc_bst, opm0->sc_bht, OPM_DATA) & 0x80);
	bus_space_write_1 (opm0->sc_bst, opm0->sc_bht, OPM_REG, reg);
	while (bus_space_read_1 (opm0->sc_bst, opm0->sc_bht, OPM_DATA) & 0x80);
	bus_space_write_1 (opm0->sc_bst, opm0->sc_bht, OPM_DATA, dat);
	opm0->sc_regs[reg] = dat;
}

inline static int 
readopm(int reg)
{
	return opm0->sc_regs[reg];
}

#include "fd.h"
#include "vs.h"
#include "bell.h"

#if NVS > 0
void
adpcm_chgclk(u_char clk)
{
	writeopm(0x1b, (readopm(0x1b) & ~OPM1B_CT1MSK) | clk);
}
#endif

#if NFD > 0
void
fdc_force_ready(u_char rdy)
{
	writeopm(0x1b, (readopm(0x1b) & ~OPM1B_CT2MSK) | rdy);
}
#endif

#if NBELL > 0
void
opm_key_on(u_char channel)
{
	writeopm(0x08, opm0->sc_vdata[channel].sm << 3 | channel);
}

void
opm_key_off(u_char channel)
{
	writeopm(0x08, channel);
}

void 
opm_set_voice(int channel, struct opm_voice *voice)
{
	memcpy(&opm0->sc_vdata[channel], voice, sizeof(struct opm_voice));

	opm_set_voice_sub(0x40 + channel, &voice->m1);
	opm_set_voice_sub(0x48 + channel, &voice->m2);
	opm_set_voice_sub(0x50 + channel, &voice->c1);
	opm_set_voice_sub(0x58 + channel, &voice->c2);
	writeopm(0x20 + channel, 0xc0 | (voice->fb & 0x7) << 3 |
		 (voice->con & 0x7));
}

void 
opm_set_voice_sub(int reg, struct opm_operator *op)
{
	/* DT1/MUL */
	writeopm(reg, (op->dt1 & 0x7) << 3 | (op->mul & 0x7));

	/* TL */
	writeopm(reg + 0x20, op->tl & 0x7f);

	/* KS/AR */
	writeopm(reg + 0x40, (op->ks & 0x3) << 6 | (op->ar & 0x1f));

	/* AMS/D1R */
	writeopm(reg + 0x60, (op->ame & 0x1) << 7 | (op->d1r & 0x1f));

	/* DT2/D2R */
	writeopm(reg + 0x80, (op->dt2 & 0x3) << 6 | (op->d2r & 0x1f));

	/* D1L/RR */
	writeopm(reg + 0xa0, (op->d1l & 0xf) << 4 | (op->rr & 0xf));
}

void 
opm_set_volume(int channel, int volume)
{
	int value;

	switch (opm0->sc_vdata[channel].con) {
	case 7:
		value = opm0->sc_vdata[channel].m1.tl + volume;
		writeopm(0x60 + channel, ((value > 0x7f) ? 0x7f : value));
	case 6:
	case 5:
		value = opm0->sc_vdata[channel].m2.tl + volume;
		writeopm(0x68 + channel, ((value > 0x7f) ? 0x7f : value));
	case 4:
		value = opm0->sc_vdata[channel].c1.tl + volume;
		writeopm(0x70 + channel, ((value > 0x7f) ? 0x7f : value));
	case 3:
	case 2:
	case 1:
	case 0:
		value = opm0->sc_vdata[channel].c2.tl + volume;
		writeopm(0x78 + channel, ((value > 0x7f) ? 0x7f : value));
	}
}

void 
opm_set_key(int channel, int tone)
{
	writeopm(0x28 + channel, tone >> 8);
	writeopm(0x30 + channel, tone & 0xff);
}

/*ARGSUSED*/
int 
opmopen(dev_t dev, int flag, int mode)
{
	return 0;
}

/*ARGSUSED*/
int 
opmclose(dev_t dev)
{
	return 0;
}

#endif
