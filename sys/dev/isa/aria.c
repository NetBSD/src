/*	$NetBSD: aria.c,v 1.10 2001/10/03 00:04:50 augustss Exp $	*/

/*-
 * Copyright (c) 1995, 1996, 1998 Roland C. Dowdeswell.  All rights reserved.
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
 *      This product includes software developed by Roland C. Dowdeswell.
 * 4. The name of the authors may not be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * TODO:
 *  o   Test the driver on cards other than a single
 *      Prometheus Aria 16.
 *  o   Look into where aria_prometheus_kludge() belongs.
 *  o   Add some dma code.  It accomplishes its goal by
 *      direct IO at the moment.
 *  o   Different programs should be able to open the device
 *      with O_RDONLY and O_WRONLY at the same time.  But I
 *      do not see support for this in /sys/dev/audio.c, so
 *	I cannot effectively code it.
 *  o   We should nicely deal with the cards that can do mulaw
 *      and alaw output.
 *  o   Rework the mixer interface.
 *       o   Deal with the lvls better.  We need to do better mapping
 *           between logarithmic scales and the one byte that
 *           we are passed.
 *       o   Deal better with cards that have no mixer.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/buf.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>

#include <dev/mulaw.h>
#include <dev/isa/isavar.h>
#include <i386/isa/icu.h>

#include <dev/isa/ariareg.h>

#define FREAD 1
#define FWRITE 2

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	printf x
int	ariadebug = 0;
#else
#define DPRINTF(x)
#endif

struct aria_mixdev_info {
	u_char	num_channels;
	u_char	level[2];
	u_char	mute;
};

struct aria_mixmaster {
	u_char num_channels;
	u_char level[2];
	u_char treble[2];
	u_char bass[2];
};

struct aria_softc {
	struct	device sc_dev;		/* base device */
	void	*sc_ih;			/* interrupt vectoring */
	bus_space_tag_t sc_iot;		/* Tag on 'da bus. */
	bus_space_handle_t sc_ioh;	/* Handle of iospace */
	isa_chipset_tag_t sc_ic;	/* ISA chipset info */

	u_short	sc_open;		/* reference count of open calls */
	u_short sc_play;		/* non-paused play chans 2**chan */
	u_short sc_record;		/* non-paused record chans 2**chan */
/* XXX -- keep this? */
	u_short sc_gain[2];		/* left/right gain (play) */

	u_long	sc_rate;		/* Sample rate for input and output */
	u_int	sc_encoding;		/* audio encoding -- ulaw/linear */
	int	sc_chans;		/* # of channels */
	int	sc_precision;		/* # bits per sample */

	u_long	sc_interrupts;		/* number of interrupts taken */
	void	(*sc_rintr)(void*);	/* record transfer completion intr handler */
	void	(*sc_pintr)(void*);	/* play transfer completion intr handler */
	void	*sc_rarg;		/* arg for sc_rintr() */
	void	*sc_parg;		/* arg for sc_pintr() */

	int	sc_blocksize;		/* literal dio block size */
	void	*sc_rdiobuffer;		/* record: where the next samples should be */
	void	*sc_pdiobuffer;		/* play:   where the next samples are */

	u_short sc_hardware;		/* bit field of hardware present */
#define ARIA_TELEPHONE	0x0001		/* has telephone input */
#define ARIA_MIXER	0x0002		/* has SC18075 digital mixer */
#define ARIA_MODEL	0x0004		/* is SC18025 (=0) or SC18026 (=1) */

	struct aria_mixdev_info aria_mix[6];
	struct aria_mixmaster ariamix_master;
	u_char	aria_mix_source;

	int	sc_sendcmd_err;
};

int	ariaprobe __P((struct device *, struct cfdata *, void *));
void	ariaattach __P((struct device *, struct device *, void *));
void	ariaclose __P((void *));
int	ariaopen __P((void *, int));
int	ariareset __P((bus_space_tag_t, bus_space_handle_t));
int	aria_reset __P((struct aria_softc *));
int	aria_getdev __P((void *, struct audio_device *));

void	aria_do_kludge __P((bus_space_tag_t, bus_space_handle_t, 
			    bus_space_handle_t, 
			    u_short, u_short, u_short, u_short));
void	aria_prometheus_kludge __P((struct isa_attach_args *, 
				    bus_space_handle_t));

int	aria_query_encoding __P((void *, struct audio_encoding *));
int	aria_round_blocksize __P((void *, int));
int	aria_speaker_ctl __P((void *, int));
int	aria_commit_settings __P((void *));
int	aria_set_params __P((void *, int, int, 
			     struct audio_params *, struct audio_params *));
int	aria_get_props __P((void *));

int	aria_start_output __P((void *, void *, int, 
			       void (*) __P((void *)), void*));
int	aria_start_input __P((void *, void *, int, 
			      void (*) __P((void *)), void*));

int	aria_halt_input __P((void *));
int	aria_halt_output __P((void *));

int	aria_sendcmd __P((struct aria_softc *, u_short, int, int, int));

u_short	aria_getdspmem __P((struct aria_softc *, u_short));
void	aria_putdspmem __P((struct aria_softc *, u_short, u_short));

int	aria_intr __P((void *));
short	ariaversion __P((struct aria_softc *));

void	aria_set_mixer __P((struct aria_softc *, int));

void	aria_mix_write __P((struct aria_softc *, int, int));
int	aria_mix_read __P((struct aria_softc *, int));

int	aria_mixer_set_port __P((void *, mixer_ctrl_t *));
int	aria_mixer_get_port __P((void *, mixer_ctrl_t *));
int	aria_mixer_query_devinfo __P((void *, mixer_devinfo_t *));

struct cfattach aria_ca = {
	sizeof(struct aria_softc), ariaprobe, ariaattach
};

/* XXX temporary test for 1.3 */
#ifndef AudioNaux
/* 1.3 */
struct cfdriver aria_cd = {
	NULL, "aria", DV_DULL
};
#endif

struct audio_device aria_device = {
	"Aria 16(se)",
	"x",
	"aria"
};

/*
 * Define our interface to the higher level audio driver.
 */

struct audio_hw_if aria_hw_if = {
	ariaopen,
	ariaclose,
	NULL,
	aria_query_encoding,
	aria_set_params,
	aria_round_blocksize,
	aria_commit_settings,
	NULL,
	NULL,
	aria_start_output,
	aria_start_input,
	aria_halt_input,
	aria_halt_output,
	NULL,
	aria_getdev,
	NULL,
	aria_mixer_set_port,
	aria_mixer_get_port,
	aria_mixer_query_devinfo,
	NULL,
	NULL,
	NULL,
	NULL,
	aria_get_props,
	NULL,
	NULL,
	NULL,
};

/*
 * Probe / attach routines.
 */

/*
 * Probe for the aria hardware.
 */
int
ariaprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	bus_space_handle_t ioh;
	struct isa_attach_args *ia = aux;

	if (!ARIA_BASE_VALID(ia->ia_iobase)) {
		printf("aria: configured iobase %d invalid\n", ia->ia_iobase);
		return 0;
	}
		
	if (!ARIA_IRQ_VALID(ia->ia_irq)) {
		printf("aria: configured irq %d invalid\n", ia->ia_irq);
		return 0;
	}

	if (bus_space_map(ia->ia_iot, ia->ia_iobase, ARIADSP_NPORT, 0, &ioh)) {
		DPRINTF(("aria: aria probe failed\n"));
		return 0;
	}

	if (cf->cf_flags & 1)
		aria_prometheus_kludge(ia, ioh);

	if (ariareset(ia->ia_iot, ioh) != 0) {
		DPRINTF(("aria: aria probe failed\n"));
		bus_space_unmap(ia->ia_iot, ioh,  ARIADSP_NPORT);
		return 0;
	}

	bus_space_unmap(ia->ia_iot, ioh,  ARIADSP_NPORT);

	ia->ia_iosize = ARIADSP_NPORT;
	DPRINTF(("aria: aria probe succeeded\n"));
	return 1;
}

/*
 * I didn't call this a kludge for
 * nothing.  This is cribbed from
 * ariainit, the author of that
 * disassembled some code to discover
 * how to set up the initial values of
 * the card.  Without this, the card
 * is dead. (It will not respond to _any_
 * input at all.)
 *
 * ariainit can be found (ftp) at:
 * ftp://ftp.wi.leidenuniv.nl/pub/audio/aria/programming/contrib/ariainit.zip
 * currently.
 */

void
aria_prometheus_kludge(ia, ioh1)
	struct isa_attach_args *ia;
	bus_space_handle_t ioh1;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_short	end;

	DPRINTF(("aria: begin aria_prometheus_kludge\n"));

/* Begin Config Sequence */

	iot = ia->ia_iot;
	bus_space_map(iot, 0x200, 8, 0, &ioh);

        bus_space_write_1(iot, ioh, 4, 0x4c);
        bus_space_write_1(iot, ioh, 5, 0x42);
        bus_space_write_1(iot, ioh, 6, 0x00);
        bus_space_write_2(iot, ioh, 0, 0x0f);
        bus_space_write_1(iot, ioh, 1, 0x00);
        bus_space_write_2(iot, ioh, 0, 0x02);
        bus_space_write_1(iot, ioh, 1, ia->ia_iobase>>2);

/* 
 * These next three lines set up the iobase
 * and the irq; and disable the drq.  
 */

	aria_do_kludge(iot, ioh, ioh1, 0x111, ((ia->ia_iobase-0x280)>>2)+0xA0,
		       0xbf, 0xa0);
	aria_do_kludge(iot, ioh, ioh1, 0x011, ia->ia_irq-6, 0xf8, 0x00);
	aria_do_kludge(iot, ioh, ioh1, 0x011, 0x00, 0xef, 0x00);

/* The rest of these lines just disable everything else */

	aria_do_kludge(iot, ioh, ioh1, 0x113, 0x00, 0x88, 0x00);
	aria_do_kludge(iot, ioh, ioh1, 0x013, 0x00, 0xf8, 0x00);
	aria_do_kludge(iot, ioh, ioh1, 0x013, 0x00, 0xef, 0x00);
	aria_do_kludge(iot, ioh, ioh1, 0x117, 0x00, 0x88, 0x00);
	aria_do_kludge(iot, ioh, ioh1, 0x017, 0x00, 0xff, 0x00);

/* End Sequence */

	bus_space_write_1(iot, ioh, 0, 0x0f);
	end = bus_space_read_1(iot, ioh1, 0);
	bus_space_write_2(iot, ioh, 0, 0x0f);
	bus_space_write_1(iot, ioh, 1, end|0x80);
	bus_space_read_1(iot, ioh, 0);

	bus_space_unmap(iot, ioh, 8);
/*
 * This delay is necessary for some reason,
 * at least it would crash, and sometimes not
 * probe properly if it did not exist.
 */
	delay(1000000);
}

void
aria_do_kludge(iot, ioh, ioh1, func, bits, and, or)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_space_handle_t ioh1;
	u_short func;
	u_short bits;
	u_short and;
	u_short or;
{
	u_int i;
	if (func & 0x100) {
		func &= ~0x100;
		if (bits) {
			bus_space_write_2(iot, ioh, 0, func-1);
			bus_space_write_1(iot, ioh, 1, bits);
		}
	} else
		or |= bits;

	bus_space_write_1(iot, ioh, 0, func);
	i = bus_space_read_1(iot, ioh1, 0);
	bus_space_write_2(iot, ioh, 0, func);
	bus_space_write_1(iot, ioh, 1, (i&and) | or);
}

/*
 * Attach hardware to driver, attach hardware driver to audio
 * pseudo-device driver.
 */
void
ariaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	bus_space_handle_t ioh;
	struct aria_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	u_short i;

	if (bus_space_map(ia->ia_iot, ia->ia_iobase, ARIADSP_NPORT, 0, &ioh))
		panic("%s: can map io port range", self->dv_xname);

	sc->sc_iot = ia->ia_iot;
	sc->sc_ioh = ioh;
	sc->sc_ic = ia->ia_ic;

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
			IPL_AUDIO, aria_intr, sc);

	DPRINTF(("isa_intr_establish() returns (%x)\n", (unsigned) sc->sc_ih));

	i = aria_getdspmem(sc, ARIAA_HARDWARE_A);

	sc->sc_hardware  = 0;
	sc->sc_hardware |= ((i>>13)&0x01)==1 ? ARIA_TELEPHONE:0;
	sc->sc_hardware |= (((i>>5)&0x07))==0x04 ? ARIA_MIXER:0;
	sc->sc_hardware |= (aria_getdspmem(sc, ARIAA_MODEL_A)>=1)?ARIA_MODEL:0;

	sc->sc_open       = 0;
	sc->sc_play       = 0;
	sc->sc_record     = 0;
	sc->sc_rate       = 7875;
	sc->sc_chans      = 1;
	sc->sc_blocksize  = 1024;
	sc->sc_precision  = 8;
        sc->sc_rintr      = 0;
        sc->sc_rarg       = 0;
        sc->sc_pintr      = 0;
        sc->sc_parg       = 0;
	sc->sc_gain[0]       = 127;
	sc->sc_gain[1]       = 127;

	for (i=0; i<6; i++) {
		if (i == ARIAMIX_TEL_LVL)
			sc->aria_mix[i].num_channels = 1;
		else
			sc->aria_mix[i].num_channels = 2;
		sc->aria_mix[i].level[0] = 127;
		sc->aria_mix[i].level[1] = 127;
	}

	sc->ariamix_master.num_channels = 2;
	sc->ariamix_master.level[0] = 222;
	sc->ariamix_master.level[1] = 222;
	sc->ariamix_master.bass[0] = 127;
	sc->ariamix_master.bass[1] = 127;
	sc->ariamix_master.treble[0] = 127;
	sc->ariamix_master.treble[1] = 127;
	sc->aria_mix_source = 0;

	aria_commit_settings(sc);

	printf(": dsp %s", (ARIA_MODEL&sc->sc_hardware)?"SC18026":"SC18025");
	if (ARIA_TELEPHONE&sc->sc_hardware)
		printf(", tel");
	if (ARIA_MIXER&sc->sc_hardware)
		printf(", SC18075 mixer");
	printf("\n");

	sprintf(aria_device.version, "%s", 
		ARIA_MODEL & sc->sc_hardware ? "SC18026" : "SC18025");

	audio_attach_mi(&aria_hw_if, (void *)sc, &sc->sc_dev);
}

/*
 * Various routines to interface to higher level audio driver
 */

int
ariaopen(addr, flags)
	void *addr;
	int flags;
{
	struct aria_softc *sc = addr;

	DPRINTF(("ariaopen() called\n"));
    
	if (!sc)
		return ENXIO;
	if ((flags&FREAD) && (sc->sc_open & ARIAR_OPEN_RECORD))
		return ENXIO;
	if ((flags&FWRITE) && (sc->sc_open & ARIAR_OPEN_PLAY))
		return ENXIO;
    
	if (flags&FREAD)
		sc->sc_open |= ARIAR_OPEN_RECORD;
	if (flags&FWRITE)
		sc->sc_open |= ARIAR_OPEN_PLAY;
	sc->sc_play  = 0;
	sc->sc_record= 0;
	sc->sc_rintr = 0;
	sc->sc_rarg  = 0;
	sc->sc_pintr = 0;
	sc->sc_parg  = 0;

	return 0;
}

int
aria_getdev(addr, retp)
	void *addr;
	struct audio_device *retp;
{
	*retp = aria_device;
	return 0;
}

/*
 * Various routines to interface to higher level audio driver
 */

int
aria_query_encoding(addr, fp)
    void *addr;
    struct audio_encoding *fp;
{
	struct aria_softc *sc = addr;

	switch (fp->index) {
		case 0:
			strcpy(fp->name, AudioEmulaw);
			fp->encoding = AUDIO_ENCODING_ULAW;
			fp->precision = 8;
			if ((ARIA_MODEL&sc->sc_hardware) == 0)
				fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 1:
			strcpy(fp->name, AudioEalaw);
			fp->encoding = AUDIO_ENCODING_ALAW;
			fp->precision = 8;
			if ((ARIA_MODEL&sc->sc_hardware) == 0)
				fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 2:
			strcpy(fp->name, AudioEslinear);
			fp->encoding = AUDIO_ENCODING_SLINEAR;
			fp->precision = 8;
			fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 3:
			strcpy(fp->name, AudioEslinear_le);
			fp->encoding = AUDIO_ENCODING_SLINEAR_LE;
			fp->precision = 16;
			fp->flags = 0;
			break;
		case 4:
			strcpy(fp->name, AudioEslinear_be);
			fp->encoding = AUDIO_ENCODING_SLINEAR_BE;
			fp->precision = 16;
			fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 5:
			strcpy(fp->name, AudioEulinear);
			fp->encoding = AUDIO_ENCODING_ULINEAR;
			fp->precision = 8;
			fp->flags = 0;
			break;
		case 6:
			strcpy(fp->name, AudioEulinear_le);
			fp->encoding = AUDIO_ENCODING_ULINEAR_LE;
			fp->precision = 16;
			fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 7:
			strcpy(fp->name, AudioEulinear_be);
			fp->encoding = AUDIO_ENCODING_ULINEAR_BE;
			fp->precision = 16;
			fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		default:
			return(EINVAL);
		/*NOTREACHED*/
	}

	return (0);
}

/*
 * Store blocksize in bytes.
 */

int
aria_round_blocksize(addr, blk)
	void *addr;
	int blk;
{
	int i;
#if 0 /* XXX -- this is being a tad bit of a problem... */
	for (i=64; i<1024; i*=2)
		if (blk <= i)
			break;
#else
	i = 1024;
#endif
	return(i);
}

int
aria_get_props(addr)
	void *addr;
{
	return AUDIO_PROP_FULLDUPLEX;
}

int
aria_set_params(addr, setmode, usemode, p, r)
	void *addr;
	int setmode, usemode;
	struct audio_params *p, *r;
{
	struct aria_softc *sc = addr;

	switch(p->encoding) {
	case AUDIO_ENCODING_ULAW:
	case AUDIO_ENCODING_ALAW:
	case AUDIO_ENCODING_SLINEAR:
	case AUDIO_ENCODING_SLINEAR_LE:
	case AUDIO_ENCODING_SLINEAR_BE:
	case AUDIO_ENCODING_ULINEAR:
	case AUDIO_ENCODING_ULINEAR_LE:
	case AUDIO_ENCODING_ULINEAR_BE:
		break;
	default:
		return (EINVAL);
	}

	if (p->sample_rate <= 9450)
		p->sample_rate = 7875;
	else if (p->sample_rate <= 13387)
		p->sample_rate = 11025;
	else if (p->sample_rate <= 18900)
		p->sample_rate = 15750;
	else if (p->sample_rate <= 26775)
		p->sample_rate = 22050;
	else if (p->sample_rate <= 37800)
		p->sample_rate = 31500;
	else
		p->sample_rate = 44100;

	sc->sc_encoding = p->encoding;
	sc->sc_precision = p->precision;
	sc->sc_chans = p->channels;
	sc->sc_rate = p->sample_rate;

	switch(p->encoding) {
	case AUDIO_ENCODING_ULAW:
		if ((ARIA_MODEL&sc->sc_hardware) == 0) {
			p->sw_code = mulaw_to_ulinear8;
			r->sw_code = ulinear8_to_mulaw;
		}
		break;
	case AUDIO_ENCODING_ALAW:
		if ((ARIA_MODEL&sc->sc_hardware) == 0) {
			p->sw_code = alaw_to_ulinear8;
			r->sw_code = ulinear8_to_alaw;
		}
		break;
	case AUDIO_ENCODING_SLINEAR:
		p->sw_code = r->sw_code = change_sign8;
		break;
	case AUDIO_ENCODING_ULINEAR_LE:
		p->sw_code = r->sw_code = change_sign16_le;
		break;
	case AUDIO_ENCODING_SLINEAR_BE:
		p->sw_code = r->sw_code = swap_bytes;
		break;
	case AUDIO_ENCODING_ULINEAR_BE:
		p->sw_code = r->sw_code = swap_bytes_change_sign16_le;
		break;
	}

	return 0;
}

/*
 * This is where all of the twiddling goes on.
 */

int
aria_commit_settings(addr)
	void *addr;
{
        struct aria_softc *sc = addr;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	static u_char tones[16] = 
	    { 7, 6, 5, 4, 3, 2, 1, 0, 8, 9, 10, 11, 12, 13, 14, 15 };
	u_short format;
	u_short left, right;
	u_short samp;
	u_char i;

	DPRINTF(("aria_commit_settings\n"));

	switch (sc->sc_rate) {
	case  7875: format = 0x00; samp = 0x60; break;
	case 11025: format = 0x00; samp = 0x40; break;
	case 15750: format = 0x10; samp = 0x60; break;
	case 22050: format = 0x10; samp = 0x40; break;
	case 31500: format = 0x10; samp = 0x20; break;
	case 44100: format = 0x20; samp = 0x00; break;
	default:    format = 0x00; samp = 0x40; break;/* XXX can we get here? */
	}

	if ((ARIA_MODEL&sc->sc_hardware) != 0) {
		format |= (sc->sc_encoding==AUDIO_ENCODING_ULAW)?0x06:0x00;
		format |= (sc->sc_encoding==AUDIO_ENCODING_ALAW)?0x08:0x00;
	}
		
	format |= (sc->sc_precision==16)?0x02:0x00;
	format |= (sc->sc_chans==2)?1:0;
	samp |= bus_space_read_2(iot, ioh, ARIADSP_STATUS) & ~0x60;

	aria_sendcmd(sc, ARIADSPC_FORMAT, format, -1, -1);
	bus_space_write_2(iot, ioh, ARIADSP_CONTROL, samp);

	if (sc->sc_hardware&ARIA_MIXER) {
		for (i = 0; i < 6; i++)
			aria_set_mixer(sc, i);
 
		if (sc->sc_chans==2) {
			aria_sendcmd(sc, ARIADSPC_CHAN_VOL, ARIAR_PLAY_CHAN, 
				     ((sc->sc_gain[0]+sc->sc_gain[1])/2)<<7,
				     -1);
			aria_sendcmd(sc, ARIADSPC_CHAN_PAN, ARIAR_PLAY_CHAN, 
				     (sc->sc_gain[0]-sc->sc_gain[1])/4+0x40,
				     -1);
		} else {
			aria_sendcmd(sc, ARIADSPC_CHAN_VOL, ARIAR_PLAY_CHAN, 
				     sc->sc_gain[0]<<7, -1);
			aria_sendcmd(sc, ARIADSPC_CHAN_PAN, ARIAR_PLAY_CHAN, 
				     0x40, -1);
		}

		aria_sendcmd(sc, ARIADSPC_MASMONMODE, 
			     sc->ariamix_master.num_channels != 2, -1, -1);

		aria_sendcmd(sc, ARIADSPC_MIXERVOL, 0x0004, 
			     sc->ariamix_master.level[0] << 7, 
			     sc->ariamix_master.level[1] << 7);

		/* Convert treble/bass from byte to soundcard style */

		left  = (tones[(sc->ariamix_master.treble[0]>>4)&0x0f]<<8) | 
			 tones[(sc->ariamix_master.bass[0]>>4)&0x0f];
		right = (tones[(sc->ariamix_master.treble[1]>>4)&0x0f]<<8) | 
			 tones[(sc->ariamix_master.bass[1]>>4)&0x0f];

		aria_sendcmd(sc, ARIADSPC_TONE, left, right, -1);
	}

	aria_sendcmd(sc, ARIADSPC_BLOCKSIZE, sc->sc_blocksize/2, -1, -1);

/*
 * If we think that the card is recording or playing, start it up again here.
 * Some of the previous commands turn the channels off.
 */

	if (sc->sc_record&(1<<ARIAR_RECORD_CHAN))
		aria_sendcmd(sc, ARIADSPC_START_REC, ARIAR_RECORD_CHAN, -1,-1);

	if (sc->sc_play&(1<<ARIAR_PLAY_CHAN))
		aria_sendcmd(sc, ARIADSPC_START_PLAY, ARIAR_PLAY_CHAN, -1, -1);

	return(0);
}

void
aria_set_mixer(sc, i)
        struct aria_softc *sc;	
	int i;
{
	u_char source;
	switch(i) {
	case ARIAMIX_MIC_LVL:     source = 0x0001; break;
	case ARIAMIX_CD_LVL:      source = 0x0002; break;
	case ARIAMIX_LINE_IN_LVL: source = 0x0008; break;
	case ARIAMIX_TEL_LVL:     source = 0x0020; break;
	case ARIAMIX_AUX_LVL:     source = 0x0010; break;
	case ARIAMIX_DAC_LVL:     source = 0x0004; break;
	default:                  source = 0x0000; break;
	}
	
	if (source != 0x0000 && source != 0x0004) {
		if (sc->aria_mix[i].mute == 1)
			aria_sendcmd(sc, ARIADSPC_INPMONMODE, source, 3, -1);
		else
			aria_sendcmd(sc, ARIADSPC_INPMONMODE, source, 
				     sc->aria_mix[i].num_channels != 2, -1); 
		
		aria_sendcmd(sc, ARIADSPC_INPMONMODE, 0x8000|source, 
			     sc->aria_mix[i].num_channels != 2, -1);
		aria_sendcmd(sc, ARIADSPC_MIXERVOL, source, 
			     sc->aria_mix[i].level[0] << 7, 
			     sc->aria_mix[i].level[1] << 7);
	}
	
	if (sc->aria_mix_source == i) {
		aria_sendcmd(sc, ARIADSPC_ADCSOURCE, source, -1, -1);
		
		if (sc->sc_open & ARIAR_OPEN_RECORD)
			aria_sendcmd(sc, ARIADSPC_ADCCONTROL, 1, -1, -1);
		else 
			aria_sendcmd(sc, ARIADSPC_ADCCONTROL, 0, -1, -1);
	}
}

void
ariaclose(addr)
	void *addr;
{
        struct aria_softc *sc = addr;

	DPRINTF(("aria_close sc=0x%x\n", (unsigned) sc));

        sc->sc_rintr = 0;
        sc->sc_pintr = 0;
	sc->sc_rdiobuffer = 0;
	sc->sc_pdiobuffer = 0;

	if (sc->sc_play&(1<<ARIAR_PLAY_CHAN) &&
	    sc->sc_open & ARIAR_OPEN_PLAY) {
		aria_sendcmd(sc, ARIADSPC_STOP_PLAY, ARIAR_PLAY_CHAN, -1, -1);
		sc->sc_play &= ~(1<<ARIAR_PLAY_CHAN);
	}

	if (sc->sc_record&(1<<ARIAR_RECORD_CHAN) &&
	    sc->sc_open & ARIAR_OPEN_RECORD) {
		aria_sendcmd(sc, ARIADSPC_STOP_REC, ARIAR_RECORD_CHAN, -1, -1);
		sc->sc_record &= ~(1<<ARIAR_RECORD_CHAN);
	}

	sc->sc_open = 0;

	if (aria_reset(sc) != 0) {
		delay(500);
		aria_reset(sc);
	}
}

/*
 * Reset the hardware.
 */

int ariareset(iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{
	struct aria_softc tmp, *sc = &tmp;

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	return aria_reset(sc);
}

int
aria_reset(sc)
	struct aria_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int fail=0;
	int i;

	bus_space_write_2(iot, ioh, ARIADSP_CONTROL, 
			  ARIAR_ARIA_SYNTH | ARIAR_SR22K|ARIAR_DSPINTWR);
	aria_putdspmem(sc, 0x6102, 0);

	fail |= aria_sendcmd(sc, ARIADSPC_SYSINIT, 0x0000, 0x0000, 0x0000);

	for (i=0; i < ARIAR_NPOLL; i++)
		if (aria_getdspmem(sc, ARIAA_TASK_A) == 1)
			break;

	bus_space_write_2(iot, ioh, ARIADSP_CONTROL, 
			  ARIAR_ARIA_SYNTH|ARIAR_SR22K | ARIAR_DSPINTWR |
			  ARIAR_PCINTWR);
	fail |= aria_sendcmd(sc, ARIADSPC_MODE, ARIAV_MODE_NO_SYNTH,-1,-1);

	return (fail);
}

/*
 * Lower-level routines
 */

void
aria_putdspmem(sc, loc, val)
	struct aria_softc *sc;
	u_short loc;
	u_short val;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	bus_space_write_2(iot, ioh, ARIADSP_DMAADDRESS, loc);
	bus_space_write_2(iot, ioh, ARIADSP_DMADATA, val);
}

u_short
aria_getdspmem(sc, loc)
	struct aria_softc *sc;
	u_short loc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	bus_space_write_2(iot, ioh, ARIADSP_DMAADDRESS, loc);
	return bus_space_read_2(iot, ioh, ARIADSP_DMADATA);
}

/*
 * aria_sendcmd()
 *  each full DSP command is unified into this
 *  function.
 */

#define ARIASEND(data, flag) \
	for (i = ARIAR_NPOLL; \
	     (bus_space_read_2(iot, ioh, ARIADSP_STATUS) & ARIAR_BUSY) && i>0; \
	     i--) \
		; \
	if (bus_space_read_2(iot, ioh, ARIADSP_STATUS) & ARIAR_BUSY) \
		fail |= flag; \
	bus_space_write_2(iot, ioh, ARIADSP_WRITE, (u_short)data)

int
aria_sendcmd(sc, command, arg1, arg2, arg3)
	struct aria_softc *sc;
	u_short command;
	int arg1;
	int arg2;
	int arg3;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i, fail = 0;

	ARIASEND(command, 1);
	if (arg1 != -1) {
		ARIASEND(arg1, 2);
	}
	if (arg2 != -1) {
		ARIASEND(arg2, 4);
	}
	if (arg3 != -1) {
		ARIASEND(arg3, 8);
	}
	ARIASEND(ARIADSPC_TERM, 16);
	
	if (fail) {
		sc->sc_sendcmd_err++;
#ifdef AUDIO_DEBUG
		DPRINTF(("aria_sendcmd: failure=(%d) cmd=(0x%x) fail=(0x%x)\n",
			 sc->sc_sendcmd_err, command, fail));
#endif
		return -1;
	}

	return 0;
}
#undef ARIASEND

int
aria_halt_input(addr)
	void *addr;
{
	struct aria_softc *sc = addr;

	DPRINTF(("aria_halt_input\n"));

	if (sc->sc_record & (1<<0)) {
		aria_sendcmd(sc, ARIADSPC_STOP_REC, 0, -1, -1);
		sc->sc_record &= ~(1<<0);
	}

	return(0);
}

int
aria_halt_output(addr)
	void *addr;
{
	struct aria_softc *sc = addr;

	DPRINTF(("aria_halt_output\n"));

	if (sc->sc_play & (1<<1)) {
		aria_sendcmd(sc, ARIADSPC_STOP_PLAY, 1, -1, -1);
		sc->sc_play &= ~(1<<1);
	}

	return(0);
}

/*
 * Here we just set up the buffers.  If we receive
 * an interrupt without these set, it is ignored.
 */

int
aria_start_input(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr) __P((void *));
	void *arg;
{
	struct aria_softc *sc = addr;

	DPRINTF(("aria_start_input %d @ %x\n", cc, (unsigned) p));

	if (cc != sc->sc_blocksize) {
		DPRINTF(("aria_start_input reqsize %d not sc_blocksize %d\n",
			cc, sc->sc_blocksize));
		return EINVAL;
	}

	sc->sc_rarg = arg;
	sc->sc_rintr = intr;
	sc->sc_rdiobuffer = p;

	if (!(sc->sc_record&(1<<ARIAR_RECORD_CHAN))) {
		aria_sendcmd(sc, ARIADSPC_START_REC, ARIAR_RECORD_CHAN, -1,-1);
		sc->sc_record |= (1<<ARIAR_RECORD_CHAN);
	}

	return 0;
}

int
aria_start_output(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr) __P((void *));
	void *arg;
{
	struct aria_softc *sc = addr;

	DPRINTF(("aria_start_output %d @ %x\n", cc, (unsigned) p));

	if (cc != sc->sc_blocksize) {
		DPRINTF(("aria_start_output reqsize %d not sc_blocksize %d\n",
			cc, sc->sc_blocksize));
		return EINVAL;
	}

	sc->sc_parg = arg;
	sc->sc_pintr = intr;
	sc->sc_pdiobuffer = p;

	if (!(sc->sc_play&(1<<ARIAR_PLAY_CHAN))) {
		aria_sendcmd(sc, ARIADSPC_START_PLAY, ARIAR_PLAY_CHAN, -1, -1);
		sc->sc_play |= (1<<ARIAR_PLAY_CHAN);
	}

	return 0;
}

/*
 * Process an interrupt.  This should be a
 * request (from the card) to write or read
 * samples.
 */
int
aria_intr(arg)
	void *arg;
{
	struct  aria_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_short *pdata = sc->sc_pdiobuffer;
	u_short *rdata = sc->sc_rdiobuffer;
	u_short address;

#if 0 /*  XXX --  BAD BAD BAD (That this is #define'd out */
	DPRINTF(("Checking to see if this is our intr\n"));

	if ((inw(iobase) & 1) != 0x1) 
		return 0;  /* not for us */
#endif

	sc->sc_interrupts++;

	DPRINTF(("aria_intr\n"));

	if ((sc->sc_open & ARIAR_OPEN_PLAY) && (pdata!=NULL)) {
		DPRINTF(("aria_intr play=(%x)\n", (unsigned) pdata));
		address = 0x8000 - 2*(sc->sc_blocksize);
		address+= aria_getdspmem(sc, ARIAA_PLAY_FIFO_A);
		bus_space_write_2(iot, ioh, ARIADSP_DMAADDRESS, address);
		bus_space_write_multi_2(iot, ioh, ARIADSP_DMADATA, pdata, 
					sc->sc_blocksize / 2);
		if (sc->sc_pintr != NULL)
			(*sc->sc_pintr)(sc->sc_parg);
	}

	if ((sc->sc_open & ARIAR_OPEN_RECORD) && (rdata!=NULL)) {
		DPRINTF(("aria_intr record=(%x)\n", (unsigned) rdata));
		address = 0x8000 - (sc->sc_blocksize);
		address+= aria_getdspmem(sc, ARIAA_REC_FIFO_A);
		bus_space_write_2(iot, ioh, ARIADSP_DMAADDRESS, address);
		bus_space_read_multi_2(iot, ioh, ARIADSP_DMADATA, rdata, 
				       sc->sc_blocksize / 2);
		if (sc->sc_rintr != NULL)
			(*sc->sc_rintr)(sc->sc_rarg);
	}

	aria_sendcmd(sc, ARIADSPC_TRANSCOMPLETE, -1, -1, -1);

	return 1;
}

int
aria_mixer_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct aria_softc *sc = addr;
	int error = EINVAL;

	DPRINTF(("aria_mixer_set_port\n"));

	/* This could be done better, no mixer still has some controls. */
	if (!(ARIA_MIXER & sc->sc_hardware))
		return ENXIO;

	if (cp->type == AUDIO_MIXER_VALUE) {
		mixer_level_t *mv = &cp->un.value;
		switch (cp->dev) {
		case ARIAMIX_MIC_LVL:
			if (mv->num_channels == 1 || mv->num_channels == 2) {
				sc->aria_mix[ARIAMIX_MIC_LVL].num_channels = 
					mv->num_channels;
				sc->aria_mix[ARIAMIX_MIC_LVL].level[0] = 
					mv->level[0];
				sc->aria_mix[ARIAMIX_MIC_LVL].level[1] = 
					mv->level[1];
				error = 0;
			}
			break;
	
		case ARIAMIX_LINE_IN_LVL:
			if (mv->num_channels == 1 || mv->num_channels == 2) {
				sc->aria_mix[ARIAMIX_LINE_IN_LVL].num_channels=
					mv->num_channels;
				sc->aria_mix[ARIAMIX_LINE_IN_LVL].level[0] = 
					mv->level[0];
				sc->aria_mix[ARIAMIX_LINE_IN_LVL].level[1] = 
					mv->level[1];
				error = 0;
			}
			break;
	
		case ARIAMIX_CD_LVL:
			if (mv->num_channels == 1 || mv->num_channels == 2) {
				sc->aria_mix[ARIAMIX_CD_LVL].num_channels = 
					mv->num_channels;
				sc->aria_mix[ARIAMIX_CD_LVL].level[0] = 
					mv->level[0];
				sc->aria_mix[ARIAMIX_CD_LVL].level[1] = 
					mv->level[1];
				error = 0;
			}
			break;
	
		case ARIAMIX_TEL_LVL:
			if (mv->num_channels == 1) {
				sc->aria_mix[ARIAMIX_TEL_LVL].num_channels = 
					mv->num_channels;
				sc->aria_mix[ARIAMIX_TEL_LVL].level[0] = 
					mv->level[0];
				error = 0;
			}
			break;
	
		case ARIAMIX_DAC_LVL:
			if (mv->num_channels == 1 || mv->num_channels == 2) {
				sc->aria_mix[ARIAMIX_DAC_LVL].num_channels = 
					mv->num_channels;
				sc->aria_mix[ARIAMIX_DAC_LVL].level[0] = 
					mv->level[0];
				sc->aria_mix[ARIAMIX_DAC_LVL].level[1] = 
					mv->level[1];
				error = 0;
			}
			break;

		case ARIAMIX_AUX_LVL:
			if (mv->num_channels == 1 || mv->num_channels == 2) {
				sc->aria_mix[ARIAMIX_AUX_LVL].num_channels = 
					mv->num_channels;
				sc->aria_mix[ARIAMIX_AUX_LVL].level[0] = 
					mv->level[0];
				sc->aria_mix[ARIAMIX_AUX_LVL].level[1] = 
					mv->level[1];
				error = 0;
			}
			break;
	
		case ARIAMIX_MASTER_LVL:
			if (mv->num_channels == 1 || mv->num_channels == 2) {
				sc->ariamix_master.num_channels = 
					mv->num_channels;
				sc->ariamix_master.level[0] = mv->level[0];
				sc->ariamix_master.level[1] = mv->level[1];
				error = 0;
			}
			break;
	
		case ARIAMIX_MASTER_TREBLE:
			if (mv->num_channels == 2) {
				sc->ariamix_master.treble[0] = 
					mv->level[0] == 0 ? 1 : mv->level[0];
				sc->ariamix_master.treble[1] = 
					mv->level[1] == 0 ? 1 : mv->level[1];
				error = 0;
			}
			break;
		case ARIAMIX_MASTER_BASS:
			if (mv->num_channels == 2) {
				sc->ariamix_master.bass[0] = 
					mv->level[0] == 0 ? 1 : mv->level[0];
				sc->ariamix_master.bass[1] = 
					mv->level[1] == 0 ? 1 : mv->level[1];
				error = 0;
			}
			break;
		case ARIAMIX_OUT_LVL:
			if (mv->num_channels == 1 || mv->num_channels == 2) {
				sc->sc_gain[0] = mv->level[0];
				sc->sc_gain[1] = mv->level[1];
				error = 0;
			}
			break;
		default:
		}
	}

	if (cp->type == AUDIO_MIXER_ENUM)
		switch(cp->dev) {
		case ARIAMIX_RECORD_SOURCE:
			if (cp->un.ord>=0 && cp->un.ord<=6) {
				sc->aria_mix_source = cp->un.ord;
				error = 0;
			}
			break;

		case ARIAMIX_MIC_MUTE:
			if (cp->un.ord == 0 || cp->un.ord == 1) {
				sc->aria_mix[ARIAMIX_MIC_LVL].mute =cp->un.ord;
				error = 0;
			}
			break;

		case ARIAMIX_LINE_IN_MUTE:
			if (cp->un.ord == 0 || cp->un.ord == 1) {
				sc->aria_mix[ARIAMIX_LINE_IN_LVL].mute = 
					cp->un.ord;
				error = 0;
			}
			break;

		case ARIAMIX_CD_MUTE:
			if (cp->un.ord == 0 || cp->un.ord == 1) {
				sc->aria_mix[ARIAMIX_CD_LVL].mute = cp->un.ord;
				error = 0;
			}
			break;

		case ARIAMIX_DAC_MUTE:
			if (cp->un.ord == 0 || cp->un.ord == 1) {
				sc->aria_mix[ARIAMIX_DAC_LVL].mute =cp->un.ord;
				error = 0;
			}
			break;

		case ARIAMIX_AUX_MUTE:
			if (cp->un.ord == 0 || cp->un.ord == 1) {
				sc->aria_mix[ARIAMIX_AUX_LVL].mute =cp->un.ord;
				error = 0;
			}
			break;

		case ARIAMIX_TEL_MUTE:
			if (cp->un.ord == 0 || cp->un.ord == 1) {
				sc->aria_mix[ARIAMIX_TEL_LVL].mute =cp->un.ord;
				error = 0;
			}
			break;

		default:
			/* NOTREACHED */
			return ENXIO;
		}

	return(error);
}

int
aria_mixer_get_port(addr, cp)
    void *addr;
    mixer_ctrl_t *cp;
{
	struct aria_softc *sc = addr;
	int error = EINVAL;

	DPRINTF(("aria_mixer_get_port\n"));

	/* This could be done better, no mixer still has some controls. */
	if (!(ARIA_MIXER&sc->sc_hardware))
		return ENXIO;

	switch (cp->dev) {
	case ARIAMIX_MIC_LVL:
		if (cp->type == AUDIO_MIXER_VALUE) {
			cp->un.value.num_channels = 
				sc->aria_mix[ARIAMIX_MIC_LVL].num_channels;
			cp->un.value.level[0] = 
				sc->aria_mix[ARIAMIX_MIC_LVL].level[0];
			cp->un.value.level[1] = 
				sc->aria_mix[ARIAMIX_MIC_LVL].level[1];
			error = 0;
		}
		break;
			
	case ARIAMIX_LINE_IN_LVL:
		if (cp->type == AUDIO_MIXER_VALUE) {
			cp->un.value.num_channels = 
				sc->aria_mix[ARIAMIX_LINE_IN_LVL].num_channels;
			cp->un.value.level[0] = 
				sc->aria_mix[ARIAMIX_LINE_IN_LVL].level[0];
			cp->un.value.level[1] = 
				sc->aria_mix[ARIAMIX_LINE_IN_LVL].level[1];
			error = 0;
		}
		break;

	case ARIAMIX_CD_LVL:
		if (cp->type == AUDIO_MIXER_VALUE) {
			cp->un.value.num_channels = 
				sc->aria_mix[ARIAMIX_CD_LVL].num_channels;
			cp->un.value.level[0] = 
				sc->aria_mix[ARIAMIX_CD_LVL].level[0];
			cp->un.value.level[1] = 
				sc->aria_mix[ARIAMIX_CD_LVL].level[1];
			error = 0;
		}
		break;

	case ARIAMIX_TEL_LVL:
		if (cp->type == AUDIO_MIXER_VALUE) {
			cp->un.value.num_channels = 
				sc->aria_mix[ARIAMIX_TEL_LVL].num_channels;
			cp->un.value.level[0] = 
				sc->aria_mix[ARIAMIX_TEL_LVL].level[0];
			error = 0;
		}
		break;
	case ARIAMIX_DAC_LVL:
		if (cp->type == AUDIO_MIXER_VALUE) {
			cp->un.value.num_channels = 
				sc->aria_mix[ARIAMIX_DAC_LVL].num_channels;
			cp->un.value.level[0] = 
				sc->aria_mix[ARIAMIX_DAC_LVL].level[0];
			cp->un.value.level[1] = 
				sc->aria_mix[ARIAMIX_DAC_LVL].level[1];
			error = 0;
		}
		break;

	case ARIAMIX_AUX_LVL:
		if (cp->type == AUDIO_MIXER_VALUE) {
			cp->un.value.num_channels = 
				sc->aria_mix[ARIAMIX_AUX_LVL].num_channels;
			cp->un.value.level[0] = 
				sc->aria_mix[ARIAMIX_AUX_LVL].level[0];
			cp->un.value.level[1] = 
				sc->aria_mix[ARIAMIX_AUX_LVL].level[1];
			error = 0;
		}
		break;

	case ARIAMIX_MIC_MUTE:
		if (cp->type == AUDIO_MIXER_ENUM) {
			cp->un.ord = sc->aria_mix[ARIAMIX_MIC_LVL].mute;
			error = 0;
		}
		break;

	case ARIAMIX_LINE_IN_MUTE:
		if (cp->type == AUDIO_MIXER_ENUM) {
			cp->un.ord = sc->aria_mix[ARIAMIX_LINE_IN_LVL].mute;
			error = 0;
		}
		break;

	case ARIAMIX_CD_MUTE:
		if (cp->type == AUDIO_MIXER_ENUM) {
			cp->un.ord = sc->aria_mix[ARIAMIX_CD_LVL].mute;
			error = 0;
		}
		break;

	case ARIAMIX_DAC_MUTE:
		if (cp->type == AUDIO_MIXER_ENUM) {
			cp->un.ord = sc->aria_mix[ARIAMIX_DAC_LVL].mute;
			error = 0;
		}
		break;

	case ARIAMIX_AUX_MUTE:
		if (cp->type == AUDIO_MIXER_ENUM) {
			cp->un.ord = sc->aria_mix[ARIAMIX_AUX_LVL].mute;
			error = 0;
		}
		break;

	case ARIAMIX_TEL_MUTE:
		if (cp->type == AUDIO_MIXER_ENUM) {
			cp->un.ord = sc->aria_mix[ARIAMIX_TEL_LVL].mute;
			error = 0;
		}
		break;

	case ARIAMIX_MASTER_LVL:
		if (cp->type == AUDIO_MIXER_VALUE) {
			cp->un.value.num_channels = 
				sc->ariamix_master.num_channels;
			cp->un.value.level[0] = sc->ariamix_master.level[0];
			cp->un.value.level[1] = sc->ariamix_master.level[1];
			error = 0;
		}
		break;

	case ARIAMIX_MASTER_TREBLE:
		if (cp->type == AUDIO_MIXER_VALUE) {
			cp->un.value.num_channels = 2;
			cp->un.value.level[0] = sc->ariamix_master.treble[0];
			cp->un.value.level[1] = sc->ariamix_master.treble[1];
			error = 0;
		}
		break;

	case ARIAMIX_MASTER_BASS:
		if (cp->type == AUDIO_MIXER_VALUE) {
			cp->un.value.num_channels = 2;
			cp->un.value.level[0] = sc->ariamix_master.bass[0];
			cp->un.value.level[1] = sc->ariamix_master.bass[1];
			error = 0;
		}
		break;

	case ARIAMIX_OUT_LVL:
		if (cp->type == AUDIO_MIXER_VALUE) {
			cp->un.value.num_channels = sc->sc_chans;
			cp->un.value.level[0] = sc->sc_gain[0];
			cp->un.value.level[1] = sc->sc_gain[1];
			error = 0;
		}
		break;
	case ARIAMIX_RECORD_SOURCE:
		if (cp->type == AUDIO_MIXER_ENUM) {
			cp->un.ord = sc->aria_mix_source;
			error = 0;
		}
		break;

	default:
		return ENXIO;
		/* NOT REACHED */
	}

	return(error);
}

int
aria_mixer_query_devinfo(addr, dip)
	   void *addr;
	   mixer_devinfo_t *dip;
{

	struct aria_softc *sc = addr;

	DPRINTF(("aria_mixer_query_devinfo\n"));

	/* This could be done better, no mixer still has some controls. */
	if (!(ARIA_MIXER & sc->sc_hardware))
		return ENXIO;

	dip->prev = dip->next = AUDIO_MIXER_LAST;

	switch(dip->index) {
	case ARIAMIX_MIC_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ARIAMIX_INPUT_CLASS;
		dip->next = ARIAMIX_MIC_MUTE;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case ARIAMIX_LINE_IN_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ARIAMIX_INPUT_CLASS;
		dip->next = ARIAMIX_LINE_IN_MUTE;
		strcpy(dip->label.name, AudioNline);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case ARIAMIX_CD_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ARIAMIX_INPUT_CLASS;
		dip->next = ARIAMIX_CD_MUTE;
		strcpy(dip->label.name, AudioNcd);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case ARIAMIX_TEL_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ARIAMIX_INPUT_CLASS;
		dip->next = ARIAMIX_TEL_MUTE;
		strcpy(dip->label.name, "telephone");
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case ARIAMIX_DAC_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ARIAMIX_INPUT_CLASS;
		dip->next = ARIAMIX_DAC_MUTE;
		strcpy(dip->label.name, AudioNdac);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case ARIAMIX_AUX_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ARIAMIX_INPUT_CLASS;
		dip->next = ARIAMIX_AUX_MUTE;
		strcpy(dip->label.name, AudioNoutput);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case ARIAMIX_MIC_MUTE:
		dip->prev = ARIAMIX_MIC_LVL;
		goto mute;

	case ARIAMIX_LINE_IN_MUTE:
		dip->prev = ARIAMIX_LINE_IN_LVL;
		goto mute;
	
	case ARIAMIX_CD_MUTE:
		dip->prev = ARIAMIX_CD_LVL;
		goto mute;
	
	case ARIAMIX_DAC_MUTE:
		dip->prev = ARIAMIX_DAC_LVL;
		goto mute;

	case ARIAMIX_AUX_MUTE:
		dip->prev = ARIAMIX_AUX_LVL;
		goto mute;

	case ARIAMIX_TEL_MUTE:
		dip->prev = ARIAMIX_TEL_LVL;
		goto mute;

mute:
		dip->mixer_class = ARIAMIX_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		strcpy(dip->label.name, AudioNmute);
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		break;

	case ARIAMIX_MASTER_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ARIAMIX_OUTPUT_CLASS;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNvolume);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case ARIAMIX_MASTER_TREBLE:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ARIAMIX_EQ_CLASS;
		strcpy(dip->label.name, AudioNtreble);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNtreble);
		break;

	case ARIAMIX_MASTER_BASS:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ARIAMIX_EQ_CLASS;
		strcpy(dip->label.name, AudioNbass);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNbass);
		break;

	case ARIAMIX_OUT_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ARIAMIX_OUTPUT_CLASS;
		strcpy(dip->label.name, AudioNoutput);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case ARIAMIX_RECORD_SOURCE:
		dip->mixer_class = ARIAMIX_RECORD_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		strcpy(dip->label.name, AudioNsource);
		dip->un.e.num_mem = 6;
		strcpy(dip->un.e.member[0].label.name, AudioNoutput);
		dip->un.e.member[0].ord = ARIAMIX_AUX_LVL;
		strcpy(dip->un.e.member[1].label.name, AudioNmicrophone);
		dip->un.e.member[1].ord = ARIAMIX_MIC_LVL;
		strcpy(dip->un.e.member[2].label.name, AudioNdac);
		dip->un.e.member[2].ord = ARIAMIX_DAC_LVL;
		strcpy(dip->un.e.member[3].label.name, AudioNline);
		dip->un.e.member[3].ord = ARIAMIX_LINE_IN_LVL;
		strcpy(dip->un.e.member[4].label.name, AudioNcd);
		dip->un.e.member[4].ord = ARIAMIX_CD_LVL;
		strcpy(dip->un.e.member[5].label.name, "telephone");
		dip->un.e.member[5].ord = ARIAMIX_TEL_LVL;
		break;

	case ARIAMIX_INPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = ARIAMIX_INPUT_CLASS;
		strcpy(dip->label.name, AudioCinputs);
		break;

	case ARIAMIX_OUTPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = ARIAMIX_OUTPUT_CLASS;
		strcpy(dip->label.name, AudioCoutputs);
		break;

	case ARIAMIX_RECORD_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = ARIAMIX_RECORD_CLASS;
		strcpy(dip->label.name, AudioCrecord);
		break;

	case ARIAMIX_EQ_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = ARIAMIX_EQ_CLASS;
		strcpy(dip->label.name, AudioCequalization);
		break;

	default:
		return ENXIO;
		/*NOTREACHED*/
	}
	return 0;
}
