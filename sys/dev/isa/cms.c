/* $NetBSD: cms.c,v 1.1.8.1 2001/10/08 20:11:05 nathanw Exp $ */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/select.h>

#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/audiovar.h>

#include <sys/midiio.h>
#include <dev/midi_if.h>
#include <dev/midivar.h>
#include <dev/midisynvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/cmsreg.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (cmsdebug) printf x
int	cmsdebug = 0;
#else
#define DPRINTF(x)
#endif

struct cms_softc {
	struct midi_softc sc_mididev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	/* shadow registers for each chip */
	u_int8_t sc_shadowregs[32*2];
	midisyn sc_midisyn;
};

int	cms_probe __P((struct device *, struct cfdata *, void *));
void	cms_attach __P((struct device *, struct device *, void *));

struct cfattach cms_ca = {
	sizeof(struct cms_softc), cms_probe, cms_attach,
};

int	cms_open __P((midisyn *, int));
void	cms_close __P((midisyn *));
void	cms_on __P((midisyn *, u_int32_t, u_int32_t, u_int32_t));
void	cms_off	__P((midisyn *, u_int32_t, u_int32_t, u_int32_t));

struct midisyn_methods midi_cms_hw = {
	cms_open,		/* open */
	cms_close,		/* close */
	0,			/* ioctl */
	0,			/* allocv */
	cms_on,			/* noteon */
	cms_off,		/* noteoff */
	0,			/* keypres */
	0,			/* ctlchg */
	0,			/* pgmchg */
	0,			/* chnpres */
	0,			/* pitchb */
	0,			/* sysex */
};

static void cms_reset __P((struct cms_softc *));

static char cms_note_table[] = {
	/* A  */ 3,
	/* A# */ 31,
	/* B  */ 58,
	/* C  */ 83,
	/* C# */ 107,
	/* D  */ 130,
	/* D# */ 151,
	/* E  */ 172,
	/* F  */ 191,
	/* F# */ 209,
	/* G  */ 226,
	/* G# */ 242,
};

#define NOTE_TO_OCTAVE(note) (((note)-CMS_FIRST_NOTE)/12)
#define NOTE_TO_COUNT(note) cms_note_table[(((note)-CMS_FIRST_NOTE)%12)]

int
cms_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int found = 0;
	int i;

	DPRINTF(("cms_probe():\n"));

	iot = ia->ia_iot;

	if (ia->ia_iobase == IOBASEUNK)
		return 0;

	if (bus_space_map(iot, ia->ia_iobase, CMS_IOSIZE, 0, &ioh))
		return 0;

	bus_space_write_1(iot, ioh, CMS_WREG, 0xaa);
	if (bus_space_read_1(iot, ioh, CMS_RREG) != 0xaa)
		goto out;

	for (i = 0; i < 8; i++) {
		if (bus_space_read_1(iot, ioh, CMS_MREG) != 0x7f)
			goto out;
	}
	found = 1;

	ia->ia_iosize = CMS_IOSIZE;

out:
	bus_space_unmap(iot, ioh, CMS_IOSIZE);

	return found;
}


void
cms_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cms_softc *sc = (struct cms_softc *)self;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	midisyn *ms;
	struct audio_attach_args arg;

	printf("\n");

	DPRINTF(("cms_attach():\n"));

	iot = ia->ia_iot;

	if (bus_space_map(iot, ia->ia_iobase, CMS_IOSIZE, 0, &ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	/* now let's reset the chips */
	cms_reset(sc);

	ms = &sc->sc_midisyn;
	ms->mets = &midi_cms_hw;
	strcpy(ms->name, "Creative Music System");
	ms->nvoice = CMS_NVOICES;
	ms->flags = MS_DOALLOC;
	ms->data = sc;

	/* use the synthesiser */
	midisyn_attach(&sc->sc_mididev, ms);

	/* now attach the midi device to the synthesiser */
	arg.type = AUDIODEV_TYPE_MIDI;
	arg.hwif = sc->sc_mididev.hw_if;
	arg.hdl = sc->sc_mididev.hw_hdl;
	config_found((struct device *)&sc->sc_mididev, &arg, 0);
}


int
cms_open(ms,flag)
	midisyn *ms;
	int flag;
{
	struct cms_softc *sc = (struct cms_softc *)ms->data;

	cms_reset(sc);

	return 0;
}

void
cms_close(ms)
	midisyn *ms;
{
	struct cms_softc *sc = (struct cms_softc *)ms->data;

	cms_reset(sc);
}

void
cms_on(ms, chan, note, vel)
	midisyn *ms;
	u_int32_t chan;
	u_int32_t note;
	u_int32_t vel;
{
	struct cms_softc *sc = (struct cms_softc *)ms->data;
	int chip = CHAN_TO_CHIP(chan);
	int voice = CHAN_TO_VOICE(chan);
	u_int8_t octave;
	u_int8_t count;
	u_int8_t reg;
	u_int8_t vol;

	if (note < CMS_FIRST_NOTE)
		return;

	octave = NOTE_TO_OCTAVE(note);
	count = NOTE_TO_COUNT(note);

	DPRINTF(("chip=%d voice=%d octave=%d count=%d offset=%d shift=%d\n",
		chip, voice, octave, count, OCTAVE_OFFSET(voice),
		OCTAVE_SHIFT(voice)));

	/* write the count */
	CMS_WRITE(sc, chip, CMS_IREG_FREQ0 + voice, count);

	/* select the octave */
	reg = CMS_READ(sc, chip, CMS_IREG_OCTAVE_1_0 + OCTAVE_OFFSET(voice));
	reg &= ~(0x0f<<OCTAVE_SHIFT(voice));
	reg |= ((octave&0x7)<<OCTAVE_SHIFT(voice));
	CMS_WRITE(sc, chip, CMS_IREG_OCTAVE_1_0 + OCTAVE_OFFSET(voice), reg);

	/* set the volume */
	vol = (vel>>3)&0x0f;
	CMS_WRITE(sc, chip, CMS_IREG_VOL0 + voice, ((vol<<4)|vol));

	/* enable the voice */
	reg = CMS_READ(sc, chip, CMS_IREG_FREQ_CTL);
	reg |= (1<<voice);
	CMS_WRITE(sc, chip, CMS_IREG_FREQ_CTL, reg);	
}

void
cms_off(ms, chan, note, vel)
	midisyn *ms;
	u_int32_t chan;
	u_int32_t note;
	u_int32_t vel;
{
	struct cms_softc *sc = (struct cms_softc *)ms->data;
	int chip = CHAN_TO_CHIP(chan);
	int voice = CHAN_TO_VOICE(chan);
	u_int8_t reg;

	if (note < CMS_FIRST_NOTE)
		return;

	/* disable the channel */
	reg = CMS_READ(sc, chip, CMS_IREG_FREQ_CTL);
	reg &= ~(1<<voice);
	CMS_WRITE(sc, chip, CMS_IREG_FREQ_CTL, reg);	
}

static void
cms_reset(sc)
	struct cms_softc *sc;
{
	int i;

	DPRINTF(("cms_reset():\n"));

	for (i = 0; i < 6; i++) {
		CMS_WRITE(sc, 0, CMS_IREG_VOL0+i, 0x00);
		CMS_WRITE(sc, 1, CMS_IREG_VOL0+i, 0x00);

		CMS_WRITE(sc, 0, CMS_IREG_FREQ0+i, 0x00);
		CMS_WRITE(sc, 1, CMS_IREG_FREQ0+i, 0x00);
	}

	for (i = 0; i < 3; i++) {
		CMS_WRITE(sc, 0, CMS_IREG_OCTAVE_1_0+i, 0x00);	
		CMS_WRITE(sc, 1, CMS_IREG_OCTAVE_1_0+i, 0x00);	
	}

	CMS_WRITE(sc, 0, CMS_IREG_FREQ_CTL, 0x00);
	CMS_WRITE(sc, 1, CMS_IREG_FREQ_CTL, 0x00);

	CMS_WRITE(sc, 0, CMS_IREG_NOISE_CTL, 0x00);
	CMS_WRITE(sc, 1, CMS_IREG_NOISE_CTL, 0x00);

	CMS_WRITE(sc, 0, CMS_IREG_NOISE_BW, 0x00);
	CMS_WRITE(sc, 1, CMS_IREG_NOISE_BW, 0x00);

	/*
	 * These registers don't appear to be useful, but must be
	 * cleared otherwise certain voices don't work properly
	 */
	CMS_WRITE(sc, 0, 0x18, 0x00);
	CMS_WRITE(sc, 1, 0x18, 0x00);
	CMS_WRITE(sc, 0, 0x19, 0x00);
	CMS_WRITE(sc, 1, 0x19, 0x00);

	CMS_WRITE(sc, 0, CMS_IREG_SYS_CTL, CMS_IREG_SYS_RESET);
	CMS_WRITE(sc, 1, CMS_IREG_SYS_CTL, CMS_IREG_SYS_RESET);

	CMS_WRITE(sc, 0, CMS_IREG_SYS_CTL, CMS_IREG_SYS_ENBL);
	CMS_WRITE(sc, 1, CMS_IREG_SYS_CTL, CMS_IREG_SYS_ENBL);
}
