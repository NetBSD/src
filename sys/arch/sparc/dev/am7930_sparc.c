/*	$NetBSD: am7930_sparc.c,v 1.43 1999/01/13 04:19:08 abs Exp $	*/

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

#define AUDIO_ROM_NAME "audio"

#ifdef AUDIO_DEBUG
#define DPRINTF(x)      if (am7930debug) printf x
#else
#define DPRINTF(x)
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

/* forward declarations */
void	am7930_sparc_onopen __P((struct am7930_softc *sc));
void	am7930_sparc_onclose __P((struct am7930_softc *sc));

/* autoconfiguration driver */
void	am7930attach_mainbus __P((struct device *, struct device *, void *));
int	am7930match_mainbus __P((struct device *, struct cfdata *, void *));
void	am7930attach_sbus __P((struct device *, struct device *, void *));
int	am7930match_sbus __P((struct device *, struct cfdata *, void *));

void	am7930_sparc_attach(struct am7930_softc *sc, int);


struct cfattach audioamd_mainbus_ca = {
	sizeof(struct am7930_softc), am7930match_mainbus, am7930attach_mainbus
};

struct cfattach audioamd_sbus_ca = {
	sizeof(struct am7930_softc), am7930match_sbus, am7930attach_sbus
};

/*
 * Define our interface to the higher level audio driver.
 */
int	am7930_start_output __P((void *, void *, int, void (*)(void *),
				  void *));
int	am7930_start_input __P((void *, void *, int, void (*)(void *),
				 void *));
int	am7930_set_port __P((void *, mixer_ctrl_t *));
int	am7930_get_port __P((void *, mixer_ctrl_t *));
int	am7930_query_devinfo __P((void *, mixer_devinfo_t *));

void	am7930_sparc_w16 __P((volatile struct am7930 *amd, u_int16_t val));

struct audio_hw_if sa_hw_if = {
	am7930_open,
	am7930_close,
	0,
	am7930_query_encoding,
	am7930_set_params,
	am7930_round_blocksize,		/* XXX md? */
	am7930_commit_settings,
	0,
	0,
	am7930_start_output,		/* XXX md? */
	am7930_start_input,		/* XXX md? */
	am7930_halt_output,		/* XXX md? */
	am7930_halt_input,		/* XXX md? */
	0,
	am7930_getdev,
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

/* autoconfig routines */


int
am7930match_mainbus(parent, cf, aux)
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
am7930match_sbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(AUDIO_ROM_NAME, sa->sa_name) == 0);
}

/*
 * Audio chip found.
 */
void
am7930attach_mainbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	struct am7930_softc *sc = (struct am7930_softc *)self;
	bus_space_handle_t bh;

	sc->sc_bustag = ma->ma_bustag;

	if (bus_space_map2(
			ma->ma_bustag,
			ma->ma_iospace,
			ma->ma_paddr,
			sizeof(struct am7930),
			BUS_SPACE_MAP_LINEAR,
			0,
			&bh) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}
	sc->sc_amd = (volatile struct am7930 *)bh;
	am7930_sparc_attach(sc, ma->ma_pri);
}

void
am7930attach_sbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sbus_attach_args *sa = aux;
	struct am7930_softc *sc = (struct am7930_softc *)self;
	bus_space_handle_t bh;

	sc->sc_bustag = sa->sa_bustag;

	if (sbus_bus_map(
			sa->sa_bustag,
			sa->sa_slot,
			sa->sa_offset,
			sizeof(struct am7930),
			0, 0,
			&bh) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}
	sc->sc_amd = (volatile struct am7930 *)bh;
	am7930_sparc_attach(sc, sa->sa_pri);
}

void
am7930_sparc_attach(sc, pri)
	struct am7930_softc *sc;
	int pri;
{

	printf(" softpri %d\n", PIL_AUSOFT);

	sc->sc_au.au_amd = sc->sc_amd;
	am7930_init(sc);

	sc->sc_wam16  = am7930_sparc_w16;
	sc->sc_onopen = am7930_sparc_onopen;
	sc->sc_onclose = am7930_sparc_onclose;

#ifndef AUDIO_C_HANDLER
	auiop = &sc->sc_au;
	(void)bus_intr_establish(sc->sc_bustag, pri,
				 BUS_INTR_ESTABLISH_FASTTRAP,
				 (int (*) __P((void *)))amd7930_trap, NULL);
#else
	(void)bus_intr_establish(sc->sc_bustag, pri, 0,
				 am7930hwintr, &sc->sc_au);
#endif
	(void)bus_intr_establish(sc->sc_bustag, PIL_AUSOFT,
				 BUS_INTR_ESTABLISH_SOFTINTR,
				 am7930swintr, sc);

	evcnt_attach(&sc->sc_dev, "intr", &sc->sc_intrcnt);

	audio_attach_mi(&sa_hw_if, sc, &sc->sc_dev);
}


/*
 *  16-bit register write, big-endian mapping.
 */
void
am7930_sparc_w16(amd, val)
	volatile struct am7930 *amd;
	u_int16_t val;
{
	amd->dr = val;
	amd->dr = val >> 8;
}


/*
 * MD attach-dependent middle layer:
 * move bytes to/from MAP chip.
 */

void
am7930_sparc_onopen(sc)
	struct am7930_softc *sc;
{

	/* reset pdma state */
	sc->sc_rintr = 0;
	sc->sc_rarg = 0;
	sc->sc_pintr = 0;
	sc->sc_parg = 0;

	sc->sc_au.au_rdata = 0;
	sc->sc_au.au_pdata = 0;
}


void
am7930_sparc_onclose(sc)
	struct am7930_softc *sc;
{
	/* On sparc, just do the chipset-level halt. */
	am7930_halt_input(sc);
	am7930_halt_output(sc);
}

int
am7930_start_output(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr) __P((void *));
	void *arg;
{
	register struct am7930_softc *sc = addr;

#ifdef AUDIO_DEBUG
	if (am7930debug > 1)
		printf("sa_start_output: cc=%d %p (%p)\n", cc, intr, arg);
#endif

	if (!sc->sc_locked) {
		register volatile struct am7930 *amd;

		amd = sc->sc_au.au_amd;
		amd->cr = AMDR_INIT;
		amd->dr = AMD_INIT_PMS_ACTIVE;
		sc->sc_locked = 1;
		DPRINTF(("sa_start_output: started intrs.\n"));
	}
	sc->sc_pintr = intr;
	sc->sc_parg = arg;
	sc->sc_au.au_pdata = p;
	sc->sc_au.au_pend = (char *)p + cc - 1;
	return(0);
}

/* ARGSUSED */
int
am7930_start_input(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr) __P((void *));
	void *arg;
{
	register struct am7930_softc *sc = addr;

#ifdef AUDIO_DEBUG
	if (am7930debug > 1)
		printf("sa_start_input: cc=%d %p (%p)\n", cc, intr, arg);
#endif

	if (!sc->sc_locked) {
		register volatile struct am7930 *amd;

		amd = sc->sc_au.au_amd;
		amd->cr = AMDR_INIT;
		amd->dr = AMD_INIT_PMS_ACTIVE;
		sc->sc_locked = 1;
		DPRINTF(("sa_start_input: started intrs.\n"));
	}
	sc->sc_rintr = intr;
	sc->sc_rarg = arg;
	sc->sc_au.au_rdata = p;
	sc->sc_au.au_rend = (char *)p + cc -1;
	return(0);
}
/*
 * halt routines: on Sparc, just use MI chipset halt.
 */


/*
 * Pseudo-DMA support: either C or locore assember.
 */

#ifdef AUDIO_C_HANDLER
int
am7930hwintr(au0)
	void *au0;
{
	register struct auio *au = au0;
	register volatile struct am7930 *amd = au->au_amd;
	register u_char *d, *e;
	register int k;

	k = amd->ir;		/* clear interrupt */

	/* receive incoming data */
	d = au->au_rdata;
	e = au->au_rend;
	if (d && d <= e) {
		*d = amd->bbrb;
		au->au_rdata++;
		if (d == e) {
#ifdef AUDIO_DEBUG
		        if (am7930debug > 1)
                		printf("am7930hwintr: swintr(r) requested");
#endif
			AUDIO_SET_SWINTR;
		}
	}

	/* send outgoing data */
	d = au->au_pdata;
	e = au->au_pend;
	if (d && d <= e) {
		amd->bbtb = *d;
		au->au_pdata++;
		if (d == e) {
#ifdef AUDIO_DEBUG
		        if (am7930debug > 1)
                		printf("am7930hwintr: swintr(p) requested");
#endif
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
	register struct am7930_softc *sc = sc0;
	register struct auio *au;
	register int s, ret = 0;

#ifdef AUDIO_DEBUG
	if (am7930debug > 1)
		printf("audiointr: sc=%p\n", sc);
#endif

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

/*
 * Attach-dependent channel set/query
 */
int
am7930_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	register struct am7930_softc *sc = addr;

	DPRINTF(("am7930_set_port: port=%d", cp->dev));

	if (cp->dev == SUNAUDIO_SOURCE || cp->dev == SUNAUDIO_OUTPUT) {
		if (cp->type != AUDIO_MIXER_ENUM)
			return(EINVAL);
	} else if (cp->type != AUDIO_MIXER_VALUE ||
					cp->un.value.num_channels != 1) {
		return(EINVAL);
	}

	switch(cp->dev) {
	    case SUNAUDIO_MIC_PORT:
		    sc->sc_rlevel = cp->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		    break;
	    case SUNAUDIO_SPEAKER:
	    case SUNAUDIO_HEADPHONES:
		    sc->sc_plevel = cp->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		    break;
	    case SUNAUDIO_MONITOR:
		    sc->sc_mlevel = cp->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		    break;
	    case SUNAUDIO_SOURCE:
		    if (cp->un.ord != SUNAUDIO_MIC_PORT)
			    return EINVAL;
		    break;
	    case SUNAUDIO_OUTPUT:
		    if (cp->un.ord != SUNAUDIO_SPEAKER &&
			cp->un.ord != SUNAUDIO_HEADPHONES)
			    return EINVAL;
			sc->sc_out_port = cp->un.ord;
		    break;
	    default:
		    return(EINVAL);
		    /* NOTREACHED */
	}
	return(0);
}

int
am7930_get_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	register struct am7930_softc *sc = addr;

	DPRINTF(("am7930_get_port: port=%d", cp->dev));

	if (cp->dev == SUNAUDIO_SOURCE || cp->dev == SUNAUDIO_OUTPUT) {
		if (cp->type != AUDIO_MIXER_ENUM)
			return(EINVAL);
	} else if (cp->type != AUDIO_MIXER_VALUE ||
					cp->un.value.num_channels != 1) {
		return(EINVAL);
	}

	switch(cp->dev) {
	    case SUNAUDIO_MIC_PORT:
		    cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_rlevel;
		    break;
	    case SUNAUDIO_SPEAKER:
	    case SUNAUDIO_HEADPHONES:
		    cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_plevel;
		    break;
	    case SUNAUDIO_MONITOR:
		    cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_mlevel;
		    break;
	    case SUNAUDIO_SOURCE:
		    cp->un.ord = SUNAUDIO_MIC_PORT;
		    break;
	    case SUNAUDIO_OUTPUT:
		    cp->un.ord = sc->sc_out_port;
		    break;
	    default:
		    return(EINVAL);
		    /* NOTREACHED */
	}
	return(0);
}


int
am7930_query_devinfo(addr, dip)
	void *addr;
	register mixer_devinfo_t *dip;
{
	switch(dip->index) {
	    case SUNAUDIO_MIC_PORT:
		    dip->type = AUDIO_MIXER_VALUE;
		    dip->mixer_class = SUNAUDIO_INPUT_CLASS;
		    dip->prev = dip->next = AUDIO_MIXER_LAST;
		    strcpy(dip->label.name, AudioNmicrophone);
		    dip->un.v.num_channels = 1;
		    strcpy(dip->un.v.units.name, AudioNvolume);
		    break;
	    case SUNAUDIO_SPEAKER:
		    dip->type = AUDIO_MIXER_VALUE;
		    dip->mixer_class = SUNAUDIO_OUTPUT_CLASS;
		    dip->prev = dip->next = AUDIO_MIXER_LAST;
		    strcpy(dip->label.name, AudioNspeaker);
		    dip->un.v.num_channels = 1;
		    strcpy(dip->un.v.units.name, AudioNvolume);
		    break;
	    case SUNAUDIO_HEADPHONES:
		    dip->type = AUDIO_MIXER_VALUE;
		    dip->mixer_class = SUNAUDIO_OUTPUT_CLASS;
		    dip->prev = dip->next = AUDIO_MIXER_LAST;
		    strcpy(dip->label.name, AudioNheadphone);
		    dip->un.v.num_channels = 1;
		    strcpy(dip->un.v.units.name, AudioNvolume);
		    break;
	    case SUNAUDIO_MONITOR:
		    dip->type = AUDIO_MIXER_VALUE;
		    dip->mixer_class = SUNAUDIO_MONITOR_CLASS;
		    dip->prev = dip->next = AUDIO_MIXER_LAST;
		    strcpy(dip->label.name, AudioNmonitor);
		    dip->un.v.num_channels = 1;
		    strcpy(dip->un.v.units.name, AudioNvolume);
		    break;
	    case SUNAUDIO_SOURCE:
		    dip->type = AUDIO_MIXER_ENUM;
		    dip->mixer_class = SUNAUDIO_RECORD_CLASS;
		    dip->next = dip->prev = AUDIO_MIXER_LAST;
		    strcpy(dip->label.name, AudioNsource);
		    dip->un.e.num_mem = 1;
		    strcpy(dip->un.e.member[0].label.name, AudioNmicrophone);
		    dip->un.e.member[0].ord = SUNAUDIO_MIC_PORT;
		    break;
	    case SUNAUDIO_OUTPUT:
		    dip->type = AUDIO_MIXER_ENUM;
		    dip->mixer_class = SUNAUDIO_MONITOR_CLASS;
		    dip->next = dip->prev = AUDIO_MIXER_LAST;
		    strcpy(dip->label.name, AudioNoutput);
		    dip->un.e.num_mem = 2;
		    strcpy(dip->un.e.member[0].label.name, AudioNspeaker);
		    dip->un.e.member[0].ord = SUNAUDIO_SPEAKER;
		    strcpy(dip->un.e.member[1].label.name, AudioNheadphone);
		    dip->un.e.member[1].ord = SUNAUDIO_HEADPHONES;
		    break;
	    case SUNAUDIO_INPUT_CLASS:
		    dip->type = AUDIO_MIXER_CLASS;
		    dip->mixer_class = SUNAUDIO_INPUT_CLASS;
		    dip->next = dip->prev = AUDIO_MIXER_LAST;
		    strcpy(dip->label.name, AudioCinputs);
		    break;
	    case SUNAUDIO_OUTPUT_CLASS:
		    dip->type = AUDIO_MIXER_CLASS;
		    dip->mixer_class = SUNAUDIO_OUTPUT_CLASS;
		    dip->next = dip->prev = AUDIO_MIXER_LAST;
		    strcpy(dip->label.name, AudioCoutputs);
		    break;
	    case SUNAUDIO_RECORD_CLASS:
		    dip->type = AUDIO_MIXER_CLASS;
		    dip->mixer_class = SUNAUDIO_RECORD_CLASS;
		    dip->next = dip->prev = AUDIO_MIXER_LAST;
		    strcpy(dip->label.name, AudioCrecord);
		    break;
	    case SUNAUDIO_MONITOR_CLASS:
		    dip->type = AUDIO_MIXER_CLASS;
		    dip->mixer_class = SUNAUDIO_MONITOR_CLASS;
		    dip->next = dip->prev = AUDIO_MIXER_LAST;
		    strcpy(dip->label.name, AudioCmonitor);
		    break;
	    default:
		    return ENXIO;
		    /*NOTREACHED*/
	}

	DPRINTF(("AUDIO_MIXER_DEVINFO: name=%s\n", dip->label.name));

	return(0);
}

#endif /* NAUDIO > 0 */
