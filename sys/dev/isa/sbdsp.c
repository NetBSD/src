/*	$NetBSD: sbdsp.c,v 1.54 1997/05/27 23:37:53 augustss Exp $	*/

/*
 * Copyright (c) 1991-1993 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 *
 */

/*
 * SoundBlaster Pro code provided by John Kohl, based on lots of
 * information he gleaned from Steve Haehnichen <steve@vigra.com>'s
 * SBlast driver for 386BSD and DOS driver code from Daniel Sachs
 * <sachs@meibm15.cen.uiuc.edu>.
 * Lots of rewrites by Lennart Augustsson <augustss@cs.chalmers.se>
 * with information from SB "Hardware Programming Guide" and the
 * Linux drivers.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/pio.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isa/sbreg.h>
#include <dev/isa/sbdspvar.h>

#ifdef AUDIO_DEBUG
extern void Dprintf __P((const char *, ...));
#define DPRINTF(x)	if (sbdspdebug) Dprintf x
int	sbdspdebug = 0;
#else
#define DPRINTF(x)
#endif

#ifndef SBDSP_NPOLL
#define SBDSP_NPOLL 3000
#endif

struct {
	int wdsp;
	int rdsp;
	int wmidi;
} sberr;

/*
 * Time constant routines follow.  See SBK, section 12.
 * Although they don't come out and say it (in the docs),
 * the card clearly uses a 1MHz countdown timer, as the
 * low-speed formula (p. 12-4) is:
 *	tc = 256 - 10^6 / sr
 * In high-speed mode, the constant is the upper byte of a 16-bit counter,
 * and a 256MHz clock is used:
 *	tc = 65536 - 256 * 10^ 6 / sr
 * Since we can only use the upper byte of the HS TC, the two formulae
 * are equivalent.  (Why didn't they say so?)  E.g.,
 * 	(65536 - 256 * 10 ^ 6 / x) >> 8 = 256 - 10^6 / x
 *
 * The crossover point (from low- to high-speed modes) is different
 * for the SBPRO and SB20.  The table on p. 12-5 gives the following data:
 *
 *				SBPRO			SB20
 *				-----			--------
 * input ls min			4	KHz		4	KHz
 * input ls max			23	KHz		13	KHz
 * input hs max			44.1	KHz		15	KHz
 * output ls min		4	KHz		4	KHz
 * output ls max		23	KHz		23	KHz
 * output hs max		44.1	KHz		44.1	KHz
 */
/* XXX Should we round the tc?
#define SB_RATE_TO_TC(x) (((65536 - 256 * 1000000 / (x)) + 128) >> 8)
*/
#define SB_RATE_TO_TC(x) (256 - 1000000 / (x))
#define SB_TC_TO_RATE(tc) (1000000 / (256 - (tc)))

struct sbmode {
	short	model;
	u_char	channels;
	u_char	precision;
	u_short	lowrate, highrate;
	u_char	cmd;
	short	cmdchan;
};
static struct sbmode sbpmodes[] = {
 { SB_1,    1,  8,  4000, 22727, SB_DSP_WDMA      },
 { SB_20,   1,  8,  4000, 22727, SB_DSP_WDMA_LOOP },
 { SB_2x,   1,  8,  4000, 22727, SB_DSP_WDMA_LOOP },
 { SB_2x,   1,  8, 22727, 45454, SB_DSP_HS_OUTPUT },
 { SB_PRO,  1,  8,  4000, 22727, SB_DSP_WDMA_LOOP },
 { SB_PRO,  1,  8, 22727, 45454, SB_DSP_HS_OUTPUT },
 { SB_PRO,  2,  8, 11025, 22727, SB_DSP_HS_OUTPUT },
 { SB_JAZZ, 1,  8,  4000, 22727, SB_DSP_WDMA_LOOP, SB_DSP_RECORD_MONO },
 { SB_JAZZ, 1,  8, 22727, 45454, SB_DSP_HS_OUTPUT, SB_DSP_RECORD_MONO },
 { SB_JAZZ, 2,  8, 11025, 22727, SB_DSP_HS_OUTPUT, SB_DSP_RECORD_STEREO },
 { SB_JAZZ, 1, 16,  4000, 22727, SB_DSP_WDMA_LOOP, JAZZ16_RECORD_MONO },
 { SB_JAZZ, 1, 16, 22727, 45454, SB_DSP_HS_OUTPUT, JAZZ16_RECORD_MONO },
 { SB_JAZZ, 2, 16, 11025, 22727, SB_DSP_HS_OUTPUT, JAZZ16_RECORD_STEREO },
 { SB_16,   1,  8,  5000, 45000, SB_DSP16_WDMA_8  },
 { SB_16,   2,  8,  5000, 45000, SB_DSP16_WDMA_8  },
 { SB_16,   1, 16,  5000, 45000, SB_DSP16_WDMA_16 },
 { SB_16,   2, 16,  5000, 45000, SB_DSP16_WDMA_16 },
 { -1 }
};
static struct sbmode sbrmodes[] = {
 { SB_1,    1,  8,  4000, 12987, SB_DSP_RDMA      },
 { SB_20,   1,  8,  4000, 12987, SB_DSP_RDMA_LOOP },
 { SB_2x,   1,  8,  4000, 12987, SB_DSP_RDMA_LOOP },
 { SB_2x,   1,  8, 12987, 14925, SB_DSP_HS_INPUT  },
 { SB_PRO,  1,  8,  4000, 22727, SB_DSP_RDMA_LOOP, SB_DSP_RECORD_MONO },
 { SB_PRO,  1,  8, 22727, 45454, SB_DSP_HS_INPUT,  SB_DSP_RECORD_MONO },
 { SB_PRO,  2,  8, 11025, 22727, SB_DSP_HS_INPUT,  SB_DSP_RECORD_STEREO },
 { SB_JAZZ, 1,  8,  4000, 22727, SB_DSP_RDMA_LOOP, SB_DSP_RECORD_MONO },
 { SB_JAZZ, 1,  8, 22727, 45454, SB_DSP_HS_INPUT,  SB_DSP_RECORD_MONO },
 { SB_JAZZ, 2,  8, 11025, 22727, SB_DSP_HS_INPUT,  SB_DSP_RECORD_STEREO },
 { SB_JAZZ, 1, 16,  4000, 22727, SB_DSP_RDMA_LOOP, JAZZ16_RECORD_MONO },
 { SB_JAZZ, 1, 16, 22727, 45454, SB_DSP_HS_INPUT,  JAZZ16_RECORD_MONO },
 { SB_JAZZ, 2, 16, 11025, 22727, SB_DSP_HS_INPUT,  JAZZ16_RECORD_STEREO },
 { SB_16,   1,  8,  5000, 45000, SB_DSP16_RDMA_8  },
 { SB_16,   2,  8,  5000, 45000, SB_DSP16_RDMA_8  },
 { SB_16,   1, 16,  5000, 45000, SB_DSP16_RDMA_16 },
 { SB_16,   2, 16,  5000, 45000, SB_DSP16_RDMA_16 },
 { -1 }
};

void	sbversion __P((struct sbdsp_softc *));
void	sbdsp_jazz16_probe __P((struct sbdsp_softc *));
void	sbdsp_set_mixer_gain __P((struct sbdsp_softc *sc, int port));
int	sbdsp16_wait __P((struct sbdsp_softc *));
void	sbdsp_to __P((void *));
void	sbdsp_pause __P((struct sbdsp_softc *));
int	sbdsp_set_timeconst __P((struct sbdsp_softc *, int));
int	sbdsp16_set_rate __P((struct sbdsp_softc *, int, int));
int	sbdsp_set_in_ports __P((struct sbdsp_softc *, int));
void	sbdsp_set_ifilter __P((void *, int));
int	sbdsp_get_ifilter __P((void *));

#ifdef AUDIO_DEBUG
void sb_printsc __P((struct sbdsp_softc *));

void
sb_printsc(sc)
	struct sbdsp_softc *sc;
{
	int i;
    
	printf("open %d dmachan %d/%d/%d iobase 0x%x irq %d\n",
	    (int)sc->sc_open, sc->dmachan, sc->sc_drq8, sc->sc_drq16,
	    sc->sc_iobase, sc->sc_irq);
	printf("irate %d itc %x orate %d otc %x\n",
	    sc->sc_irate, sc->sc_itc,
	    sc->sc_orate, sc->sc_otc);
	printf("outport %u inport %u spkron %u nintr %lu\n",
	    sc->out_port, sc->in_port, sc->spkr_state, sc->sc_interrupts);
	printf("intr %p arg %p\n",
	    sc->sc_intr, sc->sc_arg);
	printf("gain:");
	for (i = 0; i < SB_NDEVS; i++)
		printf(" %u,%u", sc->gain[i][SB_LEFT], sc->gain[i][SB_RIGHT]);
	printf("\n");
}
#endif /* AUDIO_DEBUG */

/*
 * Probe / attach routines.
 */

/*
 * Probe for the soundblaster hardware.
 */
int
sbdsp_probe(sc)
	struct sbdsp_softc *sc;
{

	if (sbdsp_reset(sc) < 0) {
		DPRINTF(("sbdsp: couldn't reset card\n"));
		return 0;
	}
	/* if flags set, go and probe the jazz16 stuff */
	if (sc->sc_dev.dv_cfdata->cf_flags & 1)
		sbdsp_jazz16_probe(sc);
	else
		sbversion(sc);
	if (sc->sc_model == SB_UNK) {
		/* Unknown SB model found. */
		DPRINTF(("sbdsp: unknown SB model found\n"));
		return 0;
	}
	return 1;
}

/*
 * Try add-on stuff for Jazz16.
 */
void
sbdsp_jazz16_probe(sc)
	struct sbdsp_softc *sc;
{
	static u_char jazz16_irq_conf[16] = {
	    -1, -1, 0x02, 0x03,
	    -1, 0x01, -1, 0x04,
	    -1, 0x02, 0x05, -1,
	    -1, -1, -1, 0x06};
	static u_char jazz16_drq_conf[8] = {
	    -1, 0x01, -1, 0x02,
	    -1, 0x03, -1, 0x04};

	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh;

	sbversion(sc);

	DPRINTF(("jazz16 probe\n"));

	if (bus_space_map(iot, JAZZ16_CONFIG_PORT, 1, 0, &ioh)) {
		DPRINTF(("bus map failed\n"));
		return;
	}

	if (jazz16_drq_conf[sc->sc_drq8] == (u_char)-1 ||
	    jazz16_irq_conf[sc->sc_irq] == (u_char)-1) {
		DPRINTF(("drq/irq check failed\n"));
		goto done;		/* give up, we can't do it. */
	}

	bus_space_write_1(iot, ioh, 0, JAZZ16_WAKEUP);
	delay(10000);			/* delay 10 ms */
	bus_space_write_1(iot, ioh, 0, JAZZ16_SETBASE);
	bus_space_write_1(iot, ioh, 0, sc->sc_iobase & 0x70);

	if (sbdsp_reset(sc) < 0) {
		DPRINTF(("sbdsp_reset check failed\n"));
		goto done;		/* XXX? what else could we do? */
	}

	if (sbdsp_wdsp(sc, JAZZ16_READ_VER)) {
		DPRINTF(("read16 setup failed\n"));
		goto done;
	}

	if (sbdsp_rdsp(sc) != JAZZ16_VER_JAZZ) {
		DPRINTF(("read16 failed\n"));
		goto done;
	}

	/* XXX set both 8 & 16-bit drq to same channel, it works fine. */
	sc->sc_drq16 = sc->sc_drq8;
	if (sbdsp_wdsp(sc, JAZZ16_SET_DMAINTR) ||
	    sbdsp_wdsp(sc, (jazz16_drq_conf[sc->sc_drq16] << 4) |
		jazz16_drq_conf[sc->sc_drq8]) ||
	    sbdsp_wdsp(sc, jazz16_irq_conf[sc->sc_irq])) {
		DPRINTF(("sbdsp: can't write jazz16 probe stuff\n"));
	} else {
		DPRINTF(("jazz16 detected!\n"));
		sc->sc_model = SB_JAZZ;
		sc->sc_mixer_model = SBM_CT1345; /* XXX really? */
	}

done:
	bus_space_unmap(iot, ioh, 1);
}

/*
 * Attach hardware to driver, attach hardware driver to audio
 * pseudo-device driver .
 */
void
sbdsp_attach(sc)
	struct sbdsp_softc *sc;
{
	struct audio_params xparams;
        int i;
        u_int v;

        sbdsp_set_params(sc, AUMODE_RECORD, &audio_default, &xparams);
        sbdsp_set_params(sc, AUMODE_PLAY,   &audio_default, &xparams);

	sbdsp_set_in_port(sc, SB_MIC_VOL);
	sbdsp_set_out_port(sc, SB_MASTER_VOL);

	if (sc->sc_mixer_model != SBM_NONE) {
        	/* Reset the mixer.*/
		sbdsp_mix_write(sc, SBP_MIX_RESET, SBP_MIX_RESET);
                /* And set our own default values */
		for (i = 0; i < SB_NDEVS; i++) {
			switch(i) {
			case SB_MIC_VOL:
			case SB_LINE_IN_VOL:
				v = 0;
				break;
			case SB_BASS:
			case SB_TREBLE:
				v = SB_ADJUST_GAIN(sc, AUDIO_MAX_GAIN/2);
				break;
			default:
				v = SB_ADJUST_GAIN(sc, AUDIO_MAX_GAIN * 3 / 4);
				break;
			}
			sc->gain[i][SB_LEFT] = sc->gain[i][SB_RIGHT] = v;
			sbdsp_set_mixer_gain(sc, i);
		}
		sc->in_filter = 0;	/* no filters turned on, please */
	}

	printf(": dsp v%d.%02d%s\n",
	       SBVER_MAJOR(sc->sc_version), SBVER_MINOR(sc->sc_version),
	       sc->sc_model == SB_JAZZ ? ": <Jazz16>" : "");
}

void
sbdsp_mix_write(sc, mixerport, val)
	struct sbdsp_softc *sc;
	int mixerport;
	int val;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

	s = splaudio();
	bus_space_write_1(iot, ioh, SBP_MIXER_ADDR, mixerport);
	delay(20);
	bus_space_write_1(iot, ioh, SBP_MIXER_DATA, val);
	delay(30);
	splx(s);
}

int
sbdsp_mix_read(sc, mixerport)
	struct sbdsp_softc *sc;
	int mixerport;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int val;
	int s;

	s = splaudio();
	bus_space_write_1(iot, ioh, SBP_MIXER_ADDR, mixerport);
	delay(20);
	val = bus_space_read_1(iot, ioh, SBP_MIXER_DATA);
	delay(30);
	splx(s);
	return val;
}

/*
 * Various routines to interface to higher level audio driver
 */

int
sbdsp_query_encoding(addr, fp)
	void *addr;
	struct audio_encoding *fp;
{
	struct sbdsp_softc *sc = addr;
	int emul;

	emul = ISSB16CLASS(sc) ? 0 : AUDIO_ENCODINGFLAG_EMULATED;

	switch (fp->index) {
	case 0:
		strcpy(fp->name, AudioEulinear);
		fp->encoding = AUDIO_ENCODING_ULINEAR;
		fp->precision = 8;
		fp->flags = 0;
		return 0;
	case 1:
		strcpy(fp->name, AudioEmulaw);
		fp->encoding = AUDIO_ENCODING_ULAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return 0;
	case 2:
		strcpy(fp->name, AudioElinear);
		fp->encoding = AUDIO_ENCODING_LINEAR;
		fp->precision = 8;
		fp->flags = emul;
		return 0;
        }
        if (!ISSB16CLASS(sc) && sc->sc_model != SB_JAZZ)
		return EINVAL;

        switch(fp->index) {
        case 3:
		strcpy(fp->name, AudioElinear_le);
		fp->encoding = AUDIO_ENCODING_LINEAR_LE;
		fp->precision = 16;
		fp->flags = 0;
		return 0;
	case 4:
		strcpy(fp->name, AudioEulinear_le);
		fp->encoding = AUDIO_ENCODING_ULINEAR_LE;
		fp->precision = 16;
		fp->flags = emul;
		return 0;
	case 5:
		strcpy(fp->name, AudioElinear_be);
		fp->encoding = AUDIO_ENCODING_LINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return 0;
	case 6:
		strcpy(fp->name, AudioEulinear_be);
		fp->encoding = AUDIO_ENCODING_ULINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return 0;
	default:
		return EINVAL;
	}
	return 0;
}

int
sbdsp_set_params(addr, mode, p, q)
	void *addr;
	int mode;
	struct audio_params *p, *q;
{
	struct sbdsp_softc *sc = addr;
	struct sbmode *m;
	u_int rate, tc = 1, bmode = -1;
	void (*swcode) __P((void *, u_char *buf, int cnt));

	for(m = mode == AUMODE_PLAY ? sbpmodes : sbrmodes; 
	    m->model != -1; m++) {
		if (sc->sc_model == m->model &&
		    p->channels == m->channels &&
		    p->precision == m->precision &&
		    p->sample_rate >= m->lowrate && 
		    p->sample_rate < m->highrate)
			break;
	}
	if (m->model == -1)
		return EINVAL;
	rate = p->sample_rate;
	swcode = 0;
	if (m->model == SB_16) {
		switch (p->encoding) {
		case AUDIO_ENCODING_LINEAR_BE:
			if (p->precision == 16)
				swcode = swap_bytes;
			/* fall into */
		case AUDIO_ENCODING_LINEAR_LE:
			bmode = 0x10;
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
			if (p->precision == 16)
				swcode = swap_bytes;
			/* fall into */
		case AUDIO_ENCODING_ULINEAR_LE:
			bmode = 0;
			break;
		case AUDIO_ENCODING_ULAW:
			swcode = mode == AUMODE_PLAY ? 
				mulaw_to_ulinear8 : ulinear8_to_mulaw;
			bmode = 0;
			break;
		default:
			return EINVAL;
		}
		if (p->channels == 2)
			bmode |= 0x20;
	} else if (m->model == SB_JAZZ && m->precision == 16) {
		switch (p->encoding) {
		case AUDIO_ENCODING_LINEAR_LE:
			break;
		case AUDIO_ENCODING_ULINEAR_LE:
			swcode = change_sign16;
			break;
		case AUDIO_ENCODING_LINEAR_BE:
			swcode = swap_bytes;
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
			swcode = mode == AUMODE_PLAY ?
			  swap_bytes_change_sign16 : change_sign16_swap_bytes;
			break;
		case AUDIO_ENCODING_ULAW:
			swcode = mode == AUMODE_PLAY ? 
				mulaw_to_ulinear8 : ulinear8_to_mulaw;
			break;
		default:
			return EINVAL;
		}
	} else {
		switch (p->encoding) {
		case AUDIO_ENCODING_LINEAR_BE:
		case AUDIO_ENCODING_LINEAR_LE:
			swcode = change_sign8;
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
		case AUDIO_ENCODING_ULINEAR_LE:
			break;
		case AUDIO_ENCODING_ULAW:
			swcode = mode == AUMODE_PLAY ? 
				mulaw_to_ulinear8 : ulinear8_to_mulaw;
			break;
		default:
			return EINVAL;
		}
		tc = SB_RATE_TO_TC(p->sample_rate * p->channels);
		p->sample_rate = SB_TC_TO_RATE(tc) / p->channels;
	}

	if (mode == AUMODE_PLAY) {
		sc->sc_orate = rate;
		sc->sc_otc = tc;
		sc->sc_omodep = m;
		sc->sc_obmode = bmode;
	} else {
		sc->sc_irate = rate;
		sc->sc_itc = tc;
		sc->sc_imodep = m;
		sc->sc_ibmode = bmode;
	}

	p->sw_code = swcode;

	/* Update setting for the other mode. */
	q->encoding = p->encoding;
	q->channels = p->channels;
	q->precision = p->precision;

	/*
	 * XXX
	 * Should wait for chip to be idle.
	 */
	sc->sc_dmadir = SB_DMA_NONE;

	DPRINTF(("set_params: model=%d, rate=%ld, prec=%d, chan=%d, enc=%d -> tc=%02x, cmd=%02x, bmode=%02x, cmdchan=%02x, swcode=%p\n", 
		 sc->sc_model, p->sample_rate, p->precision, p->channels,
		 p->encoding, tc, m->cmd, bmode, m->cmdchan, swcode));

	return 0;
}

void
sbdsp_set_ifilter(addr, which)
	void *addr;
	int which;
{
	struct sbdsp_softc *sc = addr;
	int mixval;

	mixval = sbdsp_mix_read(sc, SBP_INFILTER) & ~SBP_IFILTER_MASK;
	switch (which) {
	case 0:
		mixval |= SBP_FILTER_OFF;
		break;
	case SB_TREBLE:
		mixval |= SBP_FILTER_ON | SBP_IFILTER_HIGH;
		break;
	case SB_BASS:
		mixval |= SBP_FILTER_ON | SBP_IFILTER_LOW; 
		break;
	default:
		return;
	}
	sc->in_filter = mixval & SBP_IFILTER_MASK;
	sbdsp_mix_write(sc, SBP_INFILTER, mixval);
}

int
sbdsp_get_ifilter(addr)
	void *addr;
{
	struct sbdsp_softc *sc = addr;
	
	sc->in_filter =
		sbdsp_mix_read(sc, SBP_INFILTER) & SBP_IFILTER_MASK;
	switch (sc->in_filter) {
	case SBP_FILTER_ON|SBP_IFILTER_HIGH:
		return SB_TREBLE;
	case SBP_FILTER_ON|SBP_IFILTER_LOW:
		return SB_BASS;
	default:
		return 0;
	}
}

int
sbdsp_set_out_port(addr, port)
	void *addr;
	int port;
{
	struct sbdsp_softc *sc = addr;
	
	sc->out_port = port; /* Just record it */

	return 0;
}

int
sbdsp_get_out_port(addr)
	void *addr;
{
	struct sbdsp_softc *sc = addr;

	return sc->out_port;
}


int
sbdsp_set_in_port(addr, port)
	void *addr;
	int port;
{
	return sbdsp_set_in_ports(addr, 1 << port);
}

int
sbdsp_set_in_ports(sc, mask)
	struct sbdsp_softc *sc;
	int mask;
{
	int bitsl, bitsr;
	int sbport;
	int i;

	DPRINTF(("sbdsp_set_in_ports: model=%d, mask=%x\n",
		 sc->sc_mixer_model, mask));

	switch(sc->sc_mixer_model) {
	case SBM_NONE:
		return EINVAL;
	case SBM_CT1335:
		if (mask != (1 << SB_MIC_VOL))
			return EINVAL;
		break;
	case SBM_CT1345:
		switch (mask) {
		case 1 << SB_MIC_VOL:
			sbport = SBP_FROM_MIC;
			break;
		case 1 << SB_LINE_IN_VOL:
			sbport = SBP_FROM_LINE;
			break;
		case 1 << SB_CD_VOL:
			sbport = SBP_FROM_CD;
			break;
		default:
			return EINVAL;
		}
		sbdsp_mix_write(sc, SBP_RECORD_SOURCE,
		    SBP_RECORD_FROM(sbport, SBP_FILTER_OFF, SBP_IFILTER_HIGH));
		break;
	case SBM_CT1745:
		if (mask & ~((1<<SB_MIDI_VOL) | (1<<SB_LINE_IN_VOL) |
			     (1<<SB_CD_VOL) | (1<<SB_MIC_VOL)))
			return EINVAL;
		bitsr = 0;
		if (mask & SB_MIDI_VOL)    bitsr |= SBP_MIDI_SRC_R;
		if (mask & SB_LINE_IN_VOL) bitsr |= SBP_LINE_SRC_R;
		if (mask & SB_CD_VOL)      bitsr |= SBP_CD_SRC_R;
		bitsl = SB_SRC_R_TO_L(bitsr);
		if (mask & SB_MIC_VOL) {
			bitsl |= SBP_MIC_SRC;
			bitsr |= SBP_MIC_SRC;
		}
		sbdsp_mix_write(sc, SBP_RECORD_SOURCE_L, bitsl);
		sbdsp_mix_write(sc, SBP_RECORD_SOURCE_R, bitsr);
		break;
	}

	sc->in_mask = mask;

	/* XXX 
	 * We have to fake a single port since the upper layer
	 * expects one.
	 */
	for(i = 0; i < SB_NPORT; i++) {
		if (mask & (1 << i)) {
			sc->in_port = i;
			break;
		}
	}
	return 0;
}

int
sbdsp_get_in_port(addr)
	void *addr;
{
	struct sbdsp_softc *sc = addr;

	return sc->in_port;
}


int
sbdsp_speaker_ctl(addr, newstate)
	void *addr;
	int newstate;
{
	struct sbdsp_softc *sc = addr;

	if ((newstate == SPKR_ON) &&
	    (sc->spkr_state == SPKR_OFF)) {
		sbdsp_spkron(sc);
		sc->spkr_state = SPKR_ON;
	}
	if ((newstate == SPKR_OFF) &&
	    (sc->spkr_state == SPKR_ON)) {
		sbdsp_spkroff(sc);
		sc->spkr_state = SPKR_OFF;
	}
	return 0;
}

int
sbdsp_round_blocksize(addr, blk)
	void *addr;
	int blk;
{
	if (blk > NBPG/3)
		blk = NBPG/3;	/* XXX allow at least 3 blocks */

	/* Round to a multiple of the biggest sample size. */
	blk &= -4;

	return blk;
}

int
sbdsp_commit_settings(addr)
	void *addr;
{
	return 0;
}

int
sbdsp_open(sc, dev, flags)
	struct sbdsp_softc *sc;
	dev_t dev;
	int flags;
{
        DPRINTF(("sbdsp_open: sc=%p\n", sc));

	if (sc->sc_open != 0 || sbdsp_reset(sc) != 0)
		return ENXIO;

	sc->sc_open = 1;
	sc->sc_mintr = 0;
	if (ISSBPRO(sc) &&
	    sbdsp_wdsp(sc, SB_DSP_RECORD_MONO) < 0) {
		DPRINTF(("sbdsp_open: can't set mono mode\n"));
		/* we'll readjust when it's time for DMA. */
	}

	/*
	 * Leave most things as they were; users must change things if
	 * the previous process didn't leave it they way they wanted.
	 * Looked at another way, it's easy to set up a configuration
	 * in one program and leave it for another to inherit.
	 */
	DPRINTF(("sbdsp_open: opened\n"));

	return 0;
}

void
sbdsp_close(addr)
	void *addr;
{
	struct sbdsp_softc *sc = addr;

        DPRINTF(("sbdsp_close: sc=%p\n", sc));

	sc->sc_open = 0;
	sbdsp_spkroff(sc);
	sc->spkr_state = SPKR_OFF;
	sc->sc_intr = 0;
	sc->sc_mintr = 0;
	sbdsp_haltdma(sc);

	DPRINTF(("sbdsp_close: closed\n"));
}

/*
 * Lower-level routines
 */

/*
 * Reset the card.
 * Return non-zero if the card isn't detected.
 */
int
sbdsp_reset(sc)
	struct sbdsp_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	sc->sc_intr = 0;
	if (sc->sc_dmadir != SB_DMA_NONE) {
		isa_dmaabort(sc->dmachan);
		sc->sc_dmadir = SB_DMA_NONE;
	}

	/*
	 * See SBK, section 11.3.
	 * We pulse a reset signal into the card.
	 * Gee, what a brilliant hardware design.
	 */
	bus_space_write_1(iot, ioh, SBP_DSP_RESET, 1);
	delay(10);
	bus_space_write_1(iot, ioh, SBP_DSP_RESET, 0);
	delay(30);
	if (sbdsp_rdsp(sc) != SB_MAGIC)
		return -1;

	return 0;
}

int
sbdsp16_wait(sc)
	struct sbdsp_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;
	u_char x;

	for (i = SBDSP_NPOLL; --i >= 0; ) {
		x = bus_space_read_1(iot, ioh, SBP_DSP_WSTAT);
		delay(10);
		if ((x & SB_DSP_BUSY) == 0)
			continue;
		return 0;
	}
	++sberr.wdsp;
	return -1;
}

/*
 * Write a byte to the dsp.
 * XXX We are at the mercy of the card as we use a
 * polling loop and wait until it can take the byte.
 */
int
sbdsp_wdsp(sc, v)
	struct sbdsp_softc *sc;
	int v;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;
	u_char x;

	for (i = SBDSP_NPOLL; --i >= 0; ) {
		x = bus_space_read_1(iot, ioh, SBP_DSP_WSTAT);
		delay(10);
		if ((x & SB_DSP_BUSY) != 0)
			continue;
		bus_space_write_1(iot, ioh, SBP_DSP_WRITE, v);
		delay(10);
		return 0;
	}
	++sberr.wdsp;
	return -1;
}

/*
 * Read a byte from the DSP, using polling.
 */
int
sbdsp_rdsp(sc)
	struct sbdsp_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;
	u_char x;

	for (i = SBDSP_NPOLL; --i >= 0; ) {
		x = bus_space_read_1(iot, ioh, SBP_DSP_RSTAT);
		delay(10);
		if ((x & SB_DSP_READY) == 0)
			continue;
		x = bus_space_read_1(iot, ioh, SBP_DSP_READ);
		delay(10);
		return x;
	}
	++sberr.rdsp;
	return -1;
}

/*
 * Doing certain things (like toggling the speaker) make
 * the SB hardware go away for a while, so pause a little.
 */
void
sbdsp_to(arg)
	void *arg;
{
	wakeup(arg);
}

void
sbdsp_pause(sc)
	struct sbdsp_softc *sc;
{
	extern int hz;

	timeout(sbdsp_to, sbdsp_to, hz/8);
	(void)tsleep(sbdsp_to, PWAIT, "sbpause", 0);
}

/*
 * Turn on the speaker.  The SBK documention says this operation
 * can take up to 1/10 of a second.  Higher level layers should
 * probably let the task sleep for this amount of time after
 * calling here.  Otherwise, things might not work (because
 * sbdsp_wdsp() and sbdsp_rdsp() will probably timeout.)
 *
 * These engineers had their heads up their ass when
 * they designed this card.
 */
void
sbdsp_spkron(sc)
	struct sbdsp_softc *sc;
{
	(void)sbdsp_wdsp(sc, SB_DSP_SPKR_ON);
	sbdsp_pause(sc);
}

/*
 * Turn off the speaker; see comment above.
 */
void
sbdsp_spkroff(sc)
	struct sbdsp_softc *sc;
{
	(void)sbdsp_wdsp(sc, SB_DSP_SPKR_OFF);
	sbdsp_pause(sc);
}

/*
 * Read the version number out of the card.  Return major code
 * in high byte, and minor code in low byte.
 */
void
sbversion(sc)
	struct sbdsp_softc *sc;
{
	int v;

	sc->sc_model = SB_UNK;
	sc->sc_version = 0;
	if (sbdsp_wdsp(sc, SB_DSP_VERSION) < 0)
		return;
	v = sbdsp_rdsp(sc) << 8;
	v |= sbdsp_rdsp(sc);
	if (v < 0)
		return;
	sc->sc_version = v;
	switch(SBVER_MAJOR(v)) {
	case 1:
		sc->sc_mixer_model = SBM_NONE;
		sc->sc_model = SB_1;
		break;
	case 2:
		/* Some SB2 have a mixer, some don't. */
		sbdsp_mix_write(sc, SBP_1335_MASTER_VOL, 0x04);
		sbdsp_mix_write(sc, SBP_1335_MIDI_VOL,   0x06);
		/* Check if we can read back the mixer values. */
		if ((sbdsp_mix_read(sc, SBP_1335_MASTER_VOL) & 0x0e) == 0x04 &&
		    (sbdsp_mix_read(sc, SBP_1335_MIDI_VOL)   & 0x0e) == 0x06)
			sc->sc_mixer_model = SBM_CT1335;
		else
			sc->sc_mixer_model = SBM_NONE;
		if (SBVER_MINOR(v) == 0)
			sc->sc_model = SB_20;
		else
			sc->sc_model = SB_2x;
		break;
	case 3:
		sc->sc_mixer_model = SBM_CT1345;
		sc->sc_model = SB_PRO;
		break;
	case 4:
		sc->sc_mixer_model = SBM_CT1745;
		sc->sc_model = SB_16;
		break;
	}
}

/*
 * Halt a DMA in progress.  A low-speed transfer can be
 * resumed with sbdsp_contdma().
 */
int
sbdsp_haltdma(addr)
	void *addr;
{
	struct sbdsp_softc *sc = addr;

	DPRINTF(("sbdsp_haltdma: sc=%p\n", sc));

	sbdsp_reset(sc);
	return 0;
}

int
sbdsp_contdma(addr)
	void *addr;
{
	struct sbdsp_softc *sc = addr;

	DPRINTF(("sbdsp_contdma: sc=%p\n", sc));

	/* XXX how do we reinitialize the DMA controller state?  do we care? */
	(void)sbdsp_wdsp(sc, SB_DSP_CONT);
	return 0;
}

int
sbdsp_set_timeconst(sc, tc)
	struct sbdsp_softc *sc;
	int tc;
{
	DPRINTF(("sbdsp_set_timeconst: sc=%p tc=%d\n", sc, tc));

	if (sbdsp_wdsp(sc, SB_DSP_TIMECONST) < 0 ||
	    sbdsp_wdsp(sc, tc) < 0)
		return EIO;
	    
	return 0;
}

int
sbdsp16_set_rate(sc, cmd, rate)
	struct sbdsp_softc *sc;
	int cmd, rate;
{
	DPRINTF(("sbdsp16_set_rate: sc=%p rate=%d\n", sc, rate));

	if (sbdsp_wdsp(sc, cmd) < 0 ||
	    sbdsp_wdsp(sc, rate >> 8) < 0 ||
	    sbdsp_wdsp(sc, rate) < 0)
		return EIO;
	return 0;
}

int
sbdsp_dma_input(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr) __P((void *));
	void *arg;
{
	struct sbdsp_softc *sc = addr;
	int loop = sc->sc_model != SB_1;
	int stereo = sc->sc_imodep->channels == 2;
	int filter;
	
#ifdef AUDIO_DEBUG
	if (sbdspdebug > 1)
		Dprintf("sbdsp_dma_input: cc=%d 0x%x (0x%x)\n", cc, intr, arg);
#endif
#ifdef DIAGNOSTIC
	if (sc->sc_imodep->channels == 2 && (cc & 1)) {
		DPRINTF(("stereo record odd bytes (%d)\n", cc));
		return EIO;
	}
#endif

	if (sc->sc_dmadir != SB_DMA_IN) {
		if (ISSBPRO(sc)) {
			if (sbdsp_wdsp(sc, sc->sc_imodep->cmdchan) < 0)
				goto badmode;
			filter = stereo ? SBP_FILTER_OFF : sc->in_filter;
			sbdsp_mix_write(sc, SBP_INFILTER,
					(sbdsp_mix_read(sc, SBP_INFILTER) &
					 ~SBP_IFILTER_MASK) | filter);
		}

		if (ISSB16CLASS(sc)) {
			if (sbdsp16_set_rate(sc, SB_DSP16_INPUTRATE, 
					     sc->sc_irate))
				goto giveup;
		} else {
			if (sbdsp_set_timeconst(sc, sc->sc_itc))
				goto giveup;
		}

		sc->sc_dmadir = SB_DMA_IN;
		sc->dmaflags = DMAMODE_READ;
		if (loop)
			sc->dmaflags |= DMAMODE_LOOP;
	} else {
		/* If already started; just return. */
		if (loop)
			return 0;
	}

	sc->dmaaddr = p;
	sc->dmacnt = loop ? (NBPG/cc)*cc : cc;
	sc->dmachan = sc->sc_imodep->precision == 16 ? sc->sc_drq16 : sc->sc_drq8;
#ifdef AUDIO_DEBUG
	if (sbdspdebug > 1)
		Dprintf("sbdsp_dma_input: dmastart %x %p %d %d\n", 
			sc->dmaflags, sc->dmaaddr, sc->dmacnt, sc->dmachan);
#endif
	isa_dmastart(sc->dmaflags, sc->dmaaddr, sc->dmacnt, sc->dmachan);
	sc->sc_intr = intr;
	sc->sc_arg = arg;

	if (sc->sc_imodep->precision == 16)
		cc >>= 1;
	--cc;
	if (ISSB16CLASS(sc)) {
		if (sbdsp_wdsp(sc, sc->sc_imodep->cmd) < 0 || 
		    sbdsp_wdsp(sc, sc->sc_ibmode) < 0 ||
		    sbdsp16_wait(sc) ||
		    sbdsp_wdsp(sc, cc) < 0 ||
		    sbdsp_wdsp(sc, cc >> 8) < 0) {
			DPRINTF(("sbdsp_dma_input: SB16 DMA start failed\n"));
			goto giveup;
		}
	} else {
		if (loop) {
			DPRINTF(("sbdsp_dma_input: set blocksize=%d\n", cc));
			if (sbdsp_wdsp(sc, SB_DSP_BLOCKSIZE) < 0 ||
			    sbdsp_wdsp(sc, cc) < 0 ||
			    sbdsp_wdsp(sc, cc >> 8) < 0) {
				DPRINTF(("sbdsp_dma_input: SB2 DMA start failed\n"));
				goto giveup;
			}
			if (sbdsp_wdsp(sc, sc->sc_imodep->cmd) < 0) {
				DPRINTF(("sbdsp_dma_input: SB2 DMA restart failed\n"));
				goto giveup;
			}
		} else {
			if (sbdsp_wdsp(sc, sc->sc_imodep->cmd) < 0 ||
			    sbdsp_wdsp(sc, cc) < 0 ||
			    sbdsp_wdsp(sc, cc >> 8) < 0) {
				DPRINTF(("sbdsp_dma_input: SB1 DMA start failed\n"));
				goto giveup;
			}
		}
	}
	return 0;

giveup:
	sbdsp_reset(sc);
	return EIO;

badmode:
	DPRINTF(("sbdsp_dma_input: can't set mode\n"));
	return EIO;
}

int
sbdsp_dma_output(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr) __P((void *));
	void *arg;
{
	struct sbdsp_softc *sc = addr;
	int loop = sc->sc_model != SB_1;
	int stereo = sc->sc_omodep->channels == 2;
	int cmd;
	
#ifdef AUDIO_DEBUG
	if (sbdspdebug > 1)
		Dprintf("sbdsp_dma_output: cc=%d 0x%x (0x%x)\n", cc, intr, arg);
#endif
#ifdef DIAGNOSTIC
	if (sc->sc_omodep->channels == 2 && (cc & 1)) {
		DPRINTF(("stereo playback odd bytes (%d)\n", cc));
		return EIO;
	}
#endif

	if (sc->sc_dmadir != SB_DMA_OUT) {
		if (ISSBPRO(sc)) {
			/* make sure we re-set stereo mixer bit when we start
			   output. */
			sbdsp_mix_write(sc, SBP_STEREO,
			    (sbdsp_mix_read(sc, SBP_STEREO) & ~SBP_PLAYMODE_MASK) |
			    (stereo ?  SBP_PLAYMODE_STEREO : SBP_PLAYMODE_MONO));
			cmd = sc->sc_omodep->cmdchan;
			if (cmd && sbdsp_wdsp(sc, cmd) < 0)
				goto badmode;
		}

		if (ISSB16CLASS(sc)) {
			if (sbdsp16_set_rate(sc, SB_DSP16_OUTPUTRATE,
					     sc->sc_orate))
				goto giveup;
		} else {
			if (sbdsp_set_timeconst(sc, sc->sc_otc))
				goto giveup;
		}

		sc->sc_dmadir = SB_DMA_OUT;
		sc->dmaflags = DMAMODE_WRITE;
		if (loop)
			sc->dmaflags |= DMAMODE_LOOP;
	} else {
		/* Already started; just return. */
		if (loop)
			return 0;
	}

	sc->dmaaddr = p;
	sc->dmacnt = loop ? (NBPG/cc)*cc : cc;
	sc->dmachan = sc->sc_omodep->precision == 16 ? sc->sc_drq16 : sc->sc_drq8;
#ifdef AUDIO_DEBUG
	if (sbdspdebug > 1)
		Dprintf("sbdsp_dma_output: dmastart %x %p %d %d\n", 
			sc->dmaflags, sc->dmaaddr, sc->dmacnt, sc->dmachan);
#endif
	isa_dmastart(sc->dmaflags, sc->dmaaddr, sc->dmacnt, sc->dmachan);
	sc->sc_intr = intr;
	sc->sc_arg = arg;

	if (sc->sc_omodep->precision == 16)
		cc >>= 1;
	--cc;
	if (ISSB16CLASS(sc)) {
		if (sbdsp_wdsp(sc, sc->sc_omodep->cmd) < 0 || 
		    sbdsp_wdsp(sc, sc->sc_obmode) < 0 ||
		    sbdsp16_wait(sc) ||
		    sbdsp_wdsp(sc, cc) < 0 ||
		    sbdsp_wdsp(sc, cc >> 8) < 0) {
			DPRINTF(("sbdsp_dma_output: SB16 DMA start failed\n"));
			goto giveup;
		}
	} else {
		if (loop) {
			DPRINTF(("sbdsp_dma_output: set blocksize=%d\n", cc));
			if (sbdsp_wdsp(sc, SB_DSP_BLOCKSIZE) < 0 ||
			    sbdsp_wdsp(sc, cc) < 0 ||
			    sbdsp_wdsp(sc, cc >> 8) < 0) {
				DPRINTF(("sbdsp_dma_output: SB2 DMA blocksize failed\n"));
				goto giveup;
			}
			if (sbdsp_wdsp(sc, sc->sc_omodep->cmd) < 0) {
				DPRINTF(("sbdsp_dma_output: SB2 DMA start failed\n"));
				goto giveup;
			}
		} else {
			if (sbdsp_wdsp(sc, sc->sc_omodep->cmd) < 0 ||
			    sbdsp_wdsp(sc, cc) < 0 ||
			    sbdsp_wdsp(sc, cc >> 8) < 0) {
				DPRINTF(("sbdsp_dma_output: SB1 DMA start failed\n"));
				goto giveup;
			}
		}
	}
	return 0;

giveup:
	sbdsp_reset(sc);
	return EIO;

badmode:
	DPRINTF(("sbdsp_dma_output: can't set mode\n"));
	return EIO;
}

/*
 * Only the DSP unit on the sound blaster generates interrupts.
 * There are three cases of interrupt: reception of a midi byte
 * (when mode is enabled), completion of dma transmission, or 
 * completion of a dma reception.  The three modes are mutually
 * exclusive so we know a priori which event has occurred.
 */
int
sbdsp_intr(arg)
	void *arg;
{
	struct sbdsp_softc *sc = arg;
	int loop = sc->sc_model != SB_1;
	u_char irq;

#ifdef AUDIO_DEBUG
	if (sbdspdebug > 1)
		Dprintf("sbdsp_intr: intr=0x%x\n", sc->sc_intr);
#endif
	if (ISSB16CLASS(sc)) {
		irq = sbdsp_mix_read(sc, SBP_IRQ_STATUS);
		if ((irq & (SBP_IRQ_DMA8 | SBP_IRQ_DMA16)) == 0)
			return 0;
	} else {
		if (!loop && !isa_dmafinished(sc->dmachan))
			return 0;
		irq = SBP_IRQ_DMA8;
	}
	sc->sc_interrupts++;
	delay(10);		/* XXX why? */
#if 0
	if (sc->sc_mintr != 0) {
		x = sbdsp_rdsp(sc);
		(*sc->sc_mintr)(sc->sc_arg, x);
	} else
#endif
	if (sc->sc_intr != 0) {
		/* clear interrupt */
		if (irq & SBP_IRQ_DMA8)
			bus_space_read_1(sc->sc_iot, sc->sc_ioh, SBP_DSP_IRQACK8);
		if (irq & SBP_IRQ_DMA16)
			bus_space_read_1(sc->sc_iot, sc->sc_ioh, SBP_DSP_IRQACK16);
		if (!loop)
			isa_dmadone(sc->dmaflags, sc->dmaaddr, sc->dmacnt,
			    sc->dmachan);
		(*sc->sc_intr)(sc->sc_arg);
	} else {
		return 0;
	}
	return 1;
}

#if 0
/*
 * Enter midi uart mode and arrange for read interrupts
 * to vector to `intr'.  This puts the card in a mode
 * which allows only midi I/O; the card must be reset
 * to leave this mode.  Unfortunately, the card does not
 * use transmit interrupts, so bytes must be output
 * using polling.  To keep the polling overhead to a
 * minimum, output should be driven off a timer.
 * This is a little tricky since only 320us separate
 * consecutive midi bytes.
 */
void
sbdsp_set_midi_mode(sc, intr, arg)
	struct sbdsp_softc *sc;
	void (*intr)();
	void *arg;
{

	sbdsp_wdsp(sc, SB_MIDI_UART_INTR);
	sc->sc_mintr = intr;
	sc->sc_intr = 0;
	sc->sc_arg = arg;
}

/*
 * Write a byte to the midi port, when in midi uart mode.
 */
void
sbdsp_midi_output(sc, v)
	struct sbdsp_softc *sc;
	int v;
{

	if (sbdsp_wdsp(sc, v) < 0)
		++sberr.wmidi;
}
#endif

int
sbdsp_setfd(addr, flag)
	void *addr;
	int flag;
{
	/* Can't do full-duplex */
	return ENOTTY;
}

void
sbdsp_set_mixer_gain(sc, port)
	struct sbdsp_softc *sc;
	int port;
{
	int src, gain;

	switch(sc->sc_mixer_model) {
	case SBM_NONE:
		return;
	case SBM_CT1335:
		gain = SB_1335_GAIN(sc->gain[port][SB_LEFT]);
		switch(port) {
		case SB_MASTER_VOL:
			src = SBP_1335_MASTER_VOL;
			break;
		case SB_MIDI_VOL:
			src = SBP_1335_MIDI_VOL;
			break;
		case SB_CD_VOL:
			src = SBP_1335_CD_VOL;
			break;
		case SB_VOICE_VOL:
			src = SBP_1335_VOICE_VOL;
			gain = SB_1335_MASTER_GAIN(sc->gain[port][SB_LEFT]);
			break;
		default:
			return;
		}
		sbdsp_mix_write(sc, src, gain);
		break;
	case SBM_CT1345:
		gain = SB_STEREO_GAIN(sc->gain[port][SB_LEFT],
				      sc->gain[port][SB_RIGHT]);
		switch (port) {
		case SB_MIC_VOL:
			src = SBP_MIC_VOL;
			gain = SB_MIC_GAIN(sc->gain[port][SB_LEFT]);
			break;
		case SB_MASTER_VOL:
			src = SBP_MASTER_VOL;
			break;
		case SB_LINE_IN_VOL:
			src = SBP_LINE_VOL;
			break;
		case SB_VOICE_VOL:
			src = SBP_VOICE_VOL;
			break;
		case SB_MIDI_VOL:
			src = SBP_MIDI_VOL;
			break;
		case SB_CD_VOL:
			src = SBP_CD_VOL;
			break;
		default:
			return;
		}
		sbdsp_mix_write(sc, src, gain);
		break;
	case SBM_CT1745:
		switch (port) {
		case SB_MIC_VOL:
			src = SB16P_MIC_L;
			break;
		case SB_MASTER_VOL:
			src = SB16P_MASTER_L;
			break;
		case SB_LINE_IN_VOL:
			src = SB16P_LINE_L;
			break;
		case SB_VOICE_VOL:
			src = SB16P_VOICE_L;
			break;
		case SB_MIDI_VOL:
			src = SB16P_MIDI_L;
			break;
		case SB_CD_VOL:
			src = SB16P_CD_L;
			break;
		case SB_INPUT_GAIN:
			src = SB16P_INPUT_GAIN_L;
			break;
		case SB_OUTPUT_GAIN:
			src = SB16P_OUTPUT_GAIN_L;
			break;
		case SB_TREBLE:
			src = SB16P_TREBLE_L;
			break;
		case SB_BASS:
			src = SB16P_BASS_L;
			break;
		case SB_PCSPEAKER:
			sbdsp_mix_write(sc, SB16P_PCSPEAKER, sc->gain[port][SB_LEFT]);
			return;
		default:
			return;
		}
		sbdsp_mix_write(sc, src, sc->gain[port][SB_LEFT]);
		sbdsp_mix_write(sc, SB16P_L_TO_R(src), sc->gain[port][SB_RIGHT]);
		break;
	}
}

int
sbdsp_mixer_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct sbdsp_softc *sc = addr;
	int lgain, rgain;
    
	DPRINTF(("sbdsp_mixer_set_port: port=%d num_channels=%d\n", cp->dev,
	    cp->un.value.num_channels));

	if (sc->sc_mixer_model == SBM_NONE)
		return EINVAL;

	switch (cp->dev) {
	case SB_TREBLE:
	case SB_BASS:
		if (sc->sc_mixer_model == SBM_CT1345) {
			if (cp->type != AUDIO_MIXER_ENUM)
				return EINVAL;
			switch (cp->dev) {
			case SB_TREBLE:
				sbdsp_set_ifilter(addr, cp->un.ord ? SB_TREBLE : 0);
				return 0;
			case SB_BASS:
				sbdsp_set_ifilter(addr, cp->un.ord ? SB_BASS : 0);
				return 0;
			}
		}
	case SB_PCSPEAKER:
	case SB_INPUT_GAIN:
	case SB_OUTPUT_GAIN:
		if (sc->sc_mixer_model != SBM_CT1745)
			return EINVAL;
	case SB_MIC_VOL:
	case SB_LINE_IN_VOL:
		if (sc->sc_mixer_model == SBM_CT1335)
			return EINVAL;
	case SB_VOICE_VOL:
	case SB_MIDI_VOL:
	case SB_CD_VOL:
	case SB_MASTER_VOL:
		if (cp->type != AUDIO_MIXER_VALUE)
			return EINVAL;

		/*
		 * All the mixer ports are stereo except for the microphone.
		 * If we get a single-channel gain value passed in, then we
		 * duplicate it to both left and right channels.
		 */

		switch (cp->dev) {
		case SB_MIC_VOL:
			if (cp->un.value.num_channels != 1)
				return EINVAL;

			lgain = rgain = SB_ADJUST_MIC_GAIN(sc, 
			  cp->un.value.level[AUDIO_MIXER_LEVEL_MONO]);
			break;
		case SB_PCSPEAKER:
			if (cp->un.value.num_channels != 1)
				return EINVAL;
			/* fall into */
		case SB_INPUT_GAIN:
		case SB_OUTPUT_GAIN:
			lgain = rgain = SB_ADJUST_2_GAIN(sc, 
			  cp->un.value.level[AUDIO_MIXER_LEVEL_MONO]);
			break;
		default:
			switch (cp->un.value.num_channels) {
			case 1:
				lgain = rgain = SB_ADJUST_GAIN(sc, 
				  cp->un.value.level[AUDIO_MIXER_LEVEL_MONO]);
				break;
			case 2:
				if (sc->sc_mixer_model == SBM_CT1335)
					return EINVAL;
				lgain = SB_ADJUST_GAIN(sc, 
				  cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT]);
				rgain = SB_ADJUST_GAIN(sc, 
				  cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT]);
				break;
			default:
				return EINVAL;
			}
			break;
		}
		sc->gain[cp->dev][SB_LEFT]  = lgain;
		sc->gain[cp->dev][SB_RIGHT] = rgain;

		sbdsp_set_mixer_gain(sc, cp->dev);
		break;

	case SB_RECORD_SOURCE:
		if (sc->sc_mixer_model == SBM_CT1745) {
			if (cp->type != AUDIO_MIXER_SET)
				return EINVAL;
			return sbdsp_set_in_ports(sc, cp->un.mask);
		} else {
			if (cp->type != AUDIO_MIXER_ENUM)
				return EINVAL;
			return sbdsp_set_in_port(sc, cp->un.ord);
		}
		break;

	case SB_AGC:
		if (sc->sc_mixer_model != SBM_CT1745 || cp->type != AUDIO_MIXER_ENUM)
			return EINVAL;
		sbdsp_mix_write(sc, SB16P_AGC, cp->un.ord & 1);
		break;

	default:
		return EINVAL;
	}

	return 0;
}

int
sbdsp_mixer_get_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct sbdsp_softc *sc = addr;
    
	DPRINTF(("sbdsp_mixer_get_port: port=%d\n", cp->dev));

	if (sc->sc_mixer_model == SBM_NONE)
		return EINVAL;

	switch (cp->dev) {
	case SB_TREBLE:
	case SB_BASS:
		if (sc->sc_mixer_model == SBM_CT1345) {
			switch (cp->dev) {
			case SB_TREBLE:
				cp->un.ord = sbdsp_get_ifilter(addr) == SB_TREBLE;
				return 0;
			case SB_BASS:
				cp->un.ord = sbdsp_get_ifilter(addr) == SB_BASS;
				return 0;
			}
		}
	case SB_PCSPEAKER:
	case SB_INPUT_GAIN:
	case SB_OUTPUT_GAIN:
		if (sc->sc_mixer_model != SBM_CT1745)
			return EINVAL;
	case SB_MIC_VOL:
	case SB_LINE_IN_VOL:
		if (sc->sc_mixer_model == SBM_CT1335)
			return EINVAL;
	case SB_VOICE_VOL:
	case SB_MIDI_VOL:
	case SB_CD_VOL:
	case SB_MASTER_VOL:
		switch (cp->dev) {
		case SB_MIC_VOL:
		case SB_PCSPEAKER:
			if (cp->un.value.num_channels != 1)
				return EINVAL;
			/* fall into */
		default:
			switch (cp->un.value.num_channels) {
			case 1:
				cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = 
					sc->gain[cp->dev][SB_LEFT];
				break;
			case 2:
				cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = 
					sc->gain[cp->dev][SB_LEFT];
				cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = 
					sc->gain[cp->dev][SB_RIGHT];
				break;
			default:
				return EINVAL;
			}
			break;
		}
		break;

	case SB_RECORD_SOURCE:
		if (sc->sc_mixer_model == SBM_CT1745)
			cp->un.mask = sc->in_mask;
		else
			cp->un.ord = sc->in_port;
		break;

	case SB_AGC:
		if (sc->sc_mixer_model != SBM_CT1745)
			return EINVAL;
		cp->un.ord = sbdsp_mix_read(sc, SB16P_AGC);
		break;

	default:
		return EINVAL;
	}

	return 0;
}

int
sbdsp_mixer_query_devinfo(addr, dip)
	void *addr;
	mixer_devinfo_t *dip;
{
	struct sbdsp_softc *sc = addr;
	int chan, class;

	DPRINTF(("sbdsp_mixer_query_devinfo: model=%d index=%d\n", 
		 sc->sc_mixer_model, dip->index));

	if (sc->sc_mixer_model == SBM_NONE)
		return ENXIO;

	chan = sc->sc_mixer_model == SBM_CT1335 ? 1 : 2;
	class = sc->sc_mixer_model == SBM_CT1745 ? SB_INPUT_CLASS : SB_OUTPUT_CLASS;

	switch (dip->index) {
	case SB_MASTER_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = SB_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmaster);
		dip->un.v.num_channels = chan;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case SB_MIDI_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = class;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNfmsynth);
		dip->un.v.num_channels = chan;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case SB_CD_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = class;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNcd);
		dip->un.v.num_channels = chan;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case SB_VOICE_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = class;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNdac);
		dip->un.v.num_channels = chan;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case SB_OUTPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = SB_OUTPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCOutputs);
		return 0;
	}

	if (sc->sc_mixer_model == SBM_CT1335)
		return ENXIO;

	switch (dip->index) {
	case SB_MIC_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = class;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case SB_LINE_IN_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = class;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNline);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case SB_RECORD_SOURCE:
		dip->mixer_class = SB_RECORD_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNsource);
		if (sc->sc_mixer_model == SBM_CT1745) {
			dip->type = AUDIO_MIXER_SET;
			dip->un.s.num_mem = 4;
			strcpy(dip->un.s.member[0].label.name, AudioNmicrophone);
			dip->un.s.member[0].mask = 1 << SB_MIC_VOL;
			strcpy(dip->un.s.member[1].label.name, AudioNcd);
			dip->un.s.member[1].mask = 1 << SB_CD_VOL;
			strcpy(dip->un.s.member[2].label.name, AudioNline);
			dip->un.s.member[2].mask = 1 << SB_LINE_IN_VOL;
			strcpy(dip->un.s.member[3].label.name, AudioNfmsynth);
			dip->un.s.member[3].mask = 1 << SB_MIDI_VOL;
		} else {
			dip->type = AUDIO_MIXER_ENUM;
			dip->un.e.num_mem = 3;
			strcpy(dip->un.e.member[0].label.name, AudioNmicrophone);
			dip->un.e.member[0].ord = SB_MIC_VOL;
			strcpy(dip->un.e.member[1].label.name, AudioNcd);
			dip->un.e.member[1].ord = SB_CD_VOL;
			strcpy(dip->un.e.member[2].label.name, AudioNline);
			dip->un.e.member[2].ord = SB_LINE_IN_VOL;
		}
		return 0;

	case SB_BASS:
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNbass);
		if (sc->sc_mixer_model == SBM_CT1745) {
			dip->type = AUDIO_MIXER_VALUE;
			dip->mixer_class = SB_EQUALIZATION_CLASS;
			dip->un.v.num_channels = 2;
			strcpy(dip->un.v.units.name, AudioNbass);
		} else {
			dip->type = AUDIO_MIXER_ENUM;
			dip->mixer_class = SB_INPUT_CLASS;
			dip->un.e.num_mem = 2;
			strcpy(dip->un.e.member[0].label.name, AudioNoff);
			dip->un.e.member[0].ord = 0;
			strcpy(dip->un.e.member[1].label.name, AudioNon);
			dip->un.e.member[1].ord = 1;
		}
		return 0;
		
	case SB_TREBLE:
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNtreble);
		if (sc->sc_mixer_model == SBM_CT1745) {
			dip->type = AUDIO_MIXER_VALUE;
			dip->mixer_class = SB_EQUALIZATION_CLASS;
			dip->un.v.num_channels = 2;
			strcpy(dip->un.v.units.name, AudioNtreble);
		} else {
			dip->type = AUDIO_MIXER_ENUM;
			dip->mixer_class = SB_INPUT_CLASS;
			dip->un.e.num_mem = 2;
			strcpy(dip->un.e.member[0].label.name, AudioNoff);
			dip->un.e.member[0].ord = 0;
			strcpy(dip->un.e.member[1].label.name, AudioNon);
			dip->un.e.member[1].ord = 1;
		}
		return 0;
		
	case SB_RECORD_CLASS:			/* record source class */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = SB_RECORD_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCRecord);
		return 0;

	}

	if (sc->sc_mixer_model == SBM_CT1345)
		return ENXIO;

	switch(dip->index) {
	case SB_PCSPEAKER:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = SB_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, "pc_speaker");
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case SB_INPUT_GAIN:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = SB_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNinput);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case SB_OUTPUT_GAIN:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = SB_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNoutput);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case SB_AGC:
		dip->type = AUDIO_MIXER_ENUM;
		dip->mixer_class = SB_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, "AGC");
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		return 0;

	case SB_INPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = SB_INPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCInputs);
		return 0;

	case SB_EQUALIZATION_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = SB_EQUALIZATION_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCEqualization);
		return 0;
	}

	return ENXIO;
}
