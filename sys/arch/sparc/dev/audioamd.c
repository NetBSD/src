/*	$NetBSD: audioamd.c,v 1.3.2.1 2000/06/22 17:03:54 minoura Exp $	*/
/*	NetBSD: am7930_sparc.c,v 1.44 1999/03/14 22:29:00 jonathan Exp 	*/

/*
 * Copyright (c) 1995 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "audio.h"
#if NAUDIO > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/ic/am7930reg.h>
#include <dev/ic/am7930var.h>
#include <sparc/dev/audioamdvar.h>

#define AUDIO_ROM_NAME "audio"

#ifdef AUDIO_DEBUG
#define DPRINTF(x)      if (am7930debug) printf x
#define DPRINTFN(n,x)   if (am7930debug>(n)) printf x
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif	/* AUDIO_DEBUG */


/* interrupt interfaces */
#ifdef AUDIO_C_HANDLER
int	am7930hwintr __P((void *));
#if defined(SUN4M)
#define AUDIO_SET_SWINTR do {		\
	if (CPU_ISSUN4M)		\
		raise(0, 4);		\
	else				\
		ienab_bis(IE_L4);	\
} while(0);
#else
#define AUDIO_SET_SWINTR ienab_bis(IE_L4)
#endif /* defined(SUN4M) */
#else
struct auio *auiop;
#endif /* AUDIO_C_HANDLER */
int	am7930swintr __P((void *));

/*
 * interrupt-handler status 
 */
struct am7930_intrhand {
	int	(*ih_fun) __P((void *));
	void	*ih_arg;
};

struct audioamd_softc {
	struct am7930_softc sc_am7930;	/* glue to MI code */

	bus_space_tag_t sc_bt;		/* bus cookie */
	bus_space_handle_t sc_bh;	/* device registers */

	struct am7930_intrhand	sc_ih;	/* interrupt vector (hw or sw)  */
	void	(*sc_rintr)(void*);	/* input completion intr handler */
	void	*sc_rarg;		/* arg for sc_rintr() */
	void	(*sc_pintr)(void*);	/* output completion intr handler */
	void	*sc_parg;		/* arg for sc_pintr() */

	/* sc_au is special in that the hardware interrupt handler uses it */
	struct  auio sc_au;		/* recv and xmit buffers, etc */
#define sc_intrcnt	sc_au.au_intrcnt	/* statistics */
};

void	audioamd_mainbus_attach __P((struct device *,
		struct device *, void *));
int	audioamd_mainbus_match __P((struct device *, struct cfdata *, void *));
void	audioamd_sbus_attach __P((struct device *, struct device *, void *));
int	audioamd_sbus_match __P((struct device *, struct cfdata *, void *));
void	audioamd_attach(struct audioamd_softc *sc, int);

struct cfattach audioamd_mainbus_ca = {
	sizeof(struct audioamd_softc),
	audioamd_mainbus_match,
	audioamd_mainbus_attach
};

struct cfattach audioamd_sbus_ca = {
	sizeof(struct audioamd_softc),
	audioamd_sbus_match,
	audioamd_sbus_attach
};

/*
 * Define our interface into the am7930 MI driver.
 */

u_int8_t	audioamd_codec_iread __P((struct am7930_softc *, int));
u_int16_t	audioamd_codec_iread16 __P((struct am7930_softc *, int));
u_int8_t	audioamd_codec_dread __P((struct audioamd_softc *, int));
void	audioamd_codec_iwrite __P((struct am7930_softc *, int, u_int8_t));
void	audioamd_codec_iwrite16 __P((struct am7930_softc *, int, u_int16_t));
void	audioamd_codec_dwrite __P((struct audioamd_softc *, int, u_int8_t));
void	audioamd_onopen __P((struct am7930_softc *sc));
void	audioamd_onclose __P((struct am7930_softc *sc));

struct am7930_glue audioamd_glue = {
	audioamd_codec_iread,
	audioamd_codec_iwrite,
	audioamd_codec_iread16,
	audioamd_codec_iwrite16,
	audioamd_onopen,
	audioamd_onclose,
	0,
	0,
	0,
};

/*
 * Define our interface to the higher level audio driver.
 */
int	audioamd_start_output __P((void *, void *, int, void (*)(void *),
				  void *));
int	audioamd_start_input __P((void *, void *, int, void (*)(void *),
				 void *));
int	audioamd_getdev __P((void *, struct audio_device *));

struct audio_hw_if sa_hw_if = {
	am7930_open,
	am7930_close,
	0,
	am7930_query_encoding,
	am7930_set_params,
	am7930_round_blocksize,
	am7930_commit_settings,
	0,
	0,
	audioamd_start_output,		/* md */
	audioamd_start_input,		/* md */
	am7930_halt_output,
	am7930_halt_input,
	0,
	audioamd_getdev,
	0,
	am7930_set_port,
	am7930_get_port,
	am7930_query_devinfo,
	0,
	0,
	0,
        0,
	am7930_get_props,
};

struct audio_device audioamd_device = {
	"am7930",
	"x",
	"audioamd"
};


int
audioamd_mainbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	if (CPU_ISSUN4)
		return (0);
	return (strcmp(AUDIO_ROM_NAME, ma->ma_name) == 0);
}

int
audioamd_sbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(AUDIO_ROM_NAME, sa->sa_name) == 0);
}

void
audioamd_mainbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	struct audioamd_softc *sc = (struct audioamd_softc *)self;
	bus_space_handle_t bh;

	sc->sc_bt = ma->ma_bustag;

	if (bus_space_map2(
			ma->ma_bustag,
			ma->ma_iospace,
			ma->ma_paddr,
			AM7930_DREG_SIZE,
			BUS_SPACE_MAP_LINEAR,
			0,
			&bh) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}
	sc->sc_bh = bh;
	audioamd_attach(sc, ma->ma_pri);
}


void
audioamd_sbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sbus_attach_args *sa = aux;
	struct audioamd_softc *sc = (struct audioamd_softc *)self;
	bus_space_handle_t bh;

	sc->sc_bt = sa->sa_bustag;

	if (sbus_bus_map(
			sa->sa_bustag,
			sa->sa_slot,
			sa->sa_offset,
			AM7930_DREG_SIZE,
			0, 0,
			&bh) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}
	sc->sc_bh = bh;
	audioamd_attach(sc, sa->sa_pri);
}

void
audioamd_attach(sc, pri)
	struct audioamd_softc *sc;
	int pri;
{

	printf(" softpri %d\n", PIL_AUSOFT);

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_am7930.sc_glue = &audioamd_glue;

	am7930_init(&sc->sc_am7930, AUDIOAMD_POLL_MODE);

#ifndef AUDIO_C_HANDLER
	auiop = &sc->sc_au;
	(void)bus_intr_establish(sc->sc_bt, pri,
				 BUS_INTR_ESTABLISH_FASTTRAP,
				 (int (*) __P((void *)))amd7930_trap, NULL);
#else
	(void)bus_intr_establish(sc->sc_bt, pri, 0,
				 am7930hwintr, sc);
#endif
	(void)bus_intr_establish(sc->sc_bt, PIL_AUSOFT,
				 BUS_INTR_ESTABLISH_SOFTINTR,
				 am7930swintr, sc);

	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    sc->sc_am7930.sc_dev.dv_xname, "intr");

	audio_attach_mi(&sa_hw_if, sc, &sc->sc_am7930.sc_dev);
}


void
audioamd_onopen(sc)
	struct am7930_softc *sc;
{
	struct audioamd_softc *mdsc = (struct audioamd_softc *)sc;

	/* reset pdma state */
	mdsc->sc_rintr = 0;
	mdsc->sc_rarg = 0;
	mdsc->sc_pintr = 0;
	mdsc->sc_parg = 0;

	mdsc->sc_au.au_rdata = 0;
	mdsc->sc_au.au_pdata = 0;
}


void
audioamd_onclose(sc)
	struct am7930_softc *sc;
{
	/* On sparc, just do the chipset-level halt. */
	am7930_halt_input(sc);
	am7930_halt_output(sc);
}

int
audioamd_start_output(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr) __P((void *));
	void *arg;
{
	struct audioamd_softc *sc = addr;

	DPRINTFN(1, ("sa_start_output: cc=%d %p (%p)\n", cc, intr, arg));

	if (!sc->sc_am7930.sc_locked) {
		audioamd_codec_iwrite(&sc->sc_am7930,
			AM7930_IREG_INIT, AM7930_INIT_PMS_ACTIVE);
		sc->sc_am7930.sc_locked = 1;
		DPRINTF(("sa_start_output: started intrs.\n"));
	}
	sc->sc_pintr = intr;
	sc->sc_parg = arg;
#ifndef AUDIO_C_HANDLER
	sc->sc_au.au_bt = sc->sc_bt;
	sc->sc_au.au_bh = sc->sc_bh;
#endif
	sc->sc_au.au_pdata = p;
	sc->sc_au.au_pend = (char *)p + cc - 1;
	return(0);
}

int
audioamd_start_input(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr) __P((void *));
	void *arg;
{
	struct audioamd_softc *sc = addr;

	DPRINTFN(1, ("sa_start_input: cc=%d %p (%p)\n", cc, intr, arg));

	if (!sc->sc_am7930.sc_locked) {
		audioamd_codec_iwrite(&sc->sc_am7930,
			AM7930_IREG_INIT, AM7930_INIT_PMS_ACTIVE);
		sc->sc_am7930.sc_locked = 1;
		DPRINTF(("sa_start_input: started intrs.\n"));
	}
	sc->sc_rintr = intr;
	sc->sc_rarg = arg;
#ifndef AUDIO_C_HANDLER
	sc->sc_au.au_bt = sc->sc_bt;
	sc->sc_au.au_bh = sc->sc_bh;
#endif
	sc->sc_au.au_rdata = p;
	sc->sc_au.au_rend = (char *)p + cc -1;
	return(0);
}


/*
 * Pseudo-DMA support: either C or locore assember.
 */

#ifdef AUDIO_C_HANDLER
int
am7930hwintr(sc)
	struct audioamd_softc *au0;
{
	struct auio *au = &sc->sc_au;
	u_int8_t *d, *e;
	int k;

	/* clear interrupt */
	k = audioamd_codec_dread(sc, AM7930_DREG_IR);

	/* receive incoming data */
	d = au->au_rdata;
	e = au->au_rend;
	if (d && d <= e) {
		*d = audioamd_codec_dread(sc, AM7930_DREG_BBRB);
		au->au_rdata++;
		if (d == e) {
			DPRINTFN(1, ("am7930hwintr: swintr(r) requested"));
			AUDIO_SET_SWINTR;
		}
	}

	/* send outgoing data */
	d = au->au_pdata;
	e = au->au_pend;
	if (d && d <= e) {
		audioamd_codec_dwrite(sc, AM7930_DREG_BBTB, *d);
		au->au_pdata++;
		if (d == e) {
			DPRINTFN(1, ("am7930hwintr: swintr(p) requested"));
			AUDIO_SET_SWINTR;
		}
	}

	*(au->au_intrcnt)++;
	return (1);
}
#endif /* AUDIO_C_HANDLER */

int
am7930swintr(sc0)
	void *sc0;
{
	struct audioamd_softc *sc = sc0;
	struct auio *au;
	int s, ret = 0;

	DPRINTFN(1, ("audiointr: sc=%p\n", sc););

	au = &sc->sc_au;
	s = splaudio();
	if (au->au_rdata > au->au_rend && sc->sc_rintr != NULL) {
		splx(s);
		ret = 1;
		(*sc->sc_rintr)(sc->sc_rarg);
		s = splaudio();
	}
	if (au->au_pdata > au->au_pend && sc->sc_pintr != NULL) {
		splx(s);
		ret = 1;
		(*sc->sc_pintr)(sc->sc_parg);
	} else
		splx(s);
	return (ret);
}


/* indirect write */
void
audioamd_codec_iwrite(sc, reg, val)
	struct am7930_softc *sc;
	int reg;
	u_int8_t val;
{
	struct audioamd_softc *mdsc = (struct audioamd_softc *)sc;

	audioamd_codec_dwrite(mdsc, AM7930_DREG_CR, reg);
	audioamd_codec_dwrite(mdsc, AM7930_DREG_DR, val);
}

void
audioamd_codec_iwrite16(sc, reg, val)
	struct am7930_softc *sc;
	int reg;
	u_int16_t val;
{
	struct audioamd_softc *mdsc = (struct audioamd_softc *)sc;

	audioamd_codec_dwrite(mdsc, AM7930_DREG_CR, reg);
	audioamd_codec_dwrite(mdsc, AM7930_DREG_DR, val);
	audioamd_codec_dwrite(mdsc, AM7930_DREG_DR, val>>8);
}


/* indirect read */
u_int8_t
audioamd_codec_iread(sc, reg)
	struct am7930_softc *sc;
	int reg;
{
	struct audioamd_softc *mdsc = (struct audioamd_softc *)sc;

	audioamd_codec_dwrite(mdsc, AM7930_DREG_CR, reg);
	return (audioamd_codec_dread(mdsc, AM7930_DREG_DR));
}

u_int16_t
audioamd_codec_iread16(sc, reg)
	struct am7930_softc *sc;
	int reg;
{
	struct audioamd_softc *mdsc = (struct audioamd_softc *)sc;
	u_int8_t lo, hi;

	audioamd_codec_dwrite(mdsc, AM7930_DREG_CR, reg);
	lo = audioamd_codec_dread(mdsc, AM7930_DREG_DR);
	hi = audioamd_codec_dread(mdsc, AM7930_DREG_DR);
	return ((hi << 8) | lo);
}

/* direct read */
u_int8_t
audioamd_codec_dread(sc, reg)
	struct audioamd_softc *sc;
	int reg;
{
	return (bus_space_read_1(sc->sc_bt, sc->sc_bh, reg));
}

/* direct write */
void
audioamd_codec_dwrite(sc, reg, val)
	struct audioamd_softc *sc;
	int reg;
	u_int8_t val;
{
	bus_space_write_1(sc->sc_bt, sc->sc_bh, reg, val);
}

int
audioamd_getdev(addr, retp)
	void *addr;
	struct audio_device *retp;
{

	*retp = audioamd_device;
	return (0);
}

#endif /* NAUDIO > 0 */
