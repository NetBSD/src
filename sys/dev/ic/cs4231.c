/*	$NetBSD: cs4231.c,v 1.4.8.1 2001/10/11 00:02:03 fvdl Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include "audio.h"
#if NAUDIO > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/ic/ad1848reg.h>
#include <dev/ic/cs4231reg.h>
#include <dev/ic/ad1848var.h>
#include <dev/ic/cs4231var.h>
#include <dev/ic/apcdmareg.h>

/*---*/
#define CSAUDIO_DAC_LVL		0
#define CSAUDIO_LINE_IN_LVL	1
#define CSAUDIO_MONO_LVL	2
#define CSAUDIO_CD_LVL		3
#define CSAUDIO_MONITOR_LVL	4
#define CSAUDIO_OUT_LVL		5
#define CSAUDIO_LINE_IN_MUTE	6
#define CSAUDIO_DAC_MUTE	7
#define CSAUDIO_CD_MUTE		8
#define CSAUDIO_MONO_MUTE	9
#define CSAUDIO_MONITOR_MUTE	10
#define CSAUDIO_REC_LVL		11
#define CSAUDIO_RECORD_SOURCE	12

#define CSAUDIO_INPUT_CLASS	13
#define CSAUDIO_OUTPUT_CLASS	14
#define CSAUDIO_RECORD_CLASS	15
#define CSAUDIO_MONITOR_CLASS	16

#ifdef AUDIO_DEBUG
int     cs4231debug = 0;
#define DPRINTF(x)      if (cs4231debug) printf x
#else
#define DPRINTF(x)
#endif

struct audio_device cs4231_device = {
	"cs4231",
	"x",
	"audio"
};


/*
 * Define our interface to the higher level audio driver.
 */
int	cs4231_open __P((void *, int));
void	cs4231_close __P((void *));
size_t	cs4231_round_buffersize __P((void *, int, size_t));
int	cs4231_round_blocksize __P((void *, int));
int	cs4231_halt_output __P((void *));
int	cs4231_halt_input __P((void *));
int	cs4231_getdev __P((void *, struct audio_device *));
int	cs4231_set_port __P((void *, mixer_ctrl_t *));
int	cs4231_get_port __P((void *, mixer_ctrl_t *));
int	cs4231_query_devinfo __P((void *, mixer_devinfo_t *));
int	cs4231_get_props __P((void *));

void   *cs4231_malloc __P((void *, int, size_t, int, int));
void	cs4231_free __P((void *, void *, int));
int	cs4231_trigger_output __P((void *, void *, void *, int,
				   void (*)(void *), void *,
				   struct audio_params *));
int	cs4231_trigger_input __P((void *, void *, void *, int,
				  void (*)(void *), void *,
				  struct audio_params *));

#ifdef AUDIO_DEBUG
static void	cs4231_regdump __P((char *, struct cs4231_softc *));
#endif

int
cs4231_read(sc, index)
	struct ad1848_softc	*sc;
	int			index;
{
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, (index << 2));
}

void
cs4231_write(sc, index, value)
	struct ad1848_softc	*sc;
	int			index, value;
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, (index << 2), value);
}

struct audio_hw_if audiocs_hw_if = {
	cs4231_open,
	cs4231_close,
	0,
	ad1848_query_encoding,
	ad1848_set_params,
	cs4231_round_blocksize,
	ad1848_commit_settings,
	0,
	0,
	NULL,
	NULL,
	cs4231_halt_output,
	cs4231_halt_input,
	0,
	cs4231_getdev,
	0,
	cs4231_set_port,
	cs4231_get_port,
	cs4231_query_devinfo,
	cs4231_malloc,
	cs4231_free,
	cs4231_round_buffersize,
        0,
	cs4231_get_props,
	cs4231_trigger_output,
	cs4231_trigger_input,
	NULL,
};


#ifdef AUDIO_DEBUG
static void
cs4231_regdump(label, sc)
	char *label;
	struct cs4231_softc *sc;
{
	char bits[128];
	volatile struct apc_dma *dma = sc->sc_dmareg;

	printf("cs4231regdump(%s): regs:", label);
	printf("dmapva: 0x%x; ", dma->dmapva);
	printf("dmapc: 0x%x; ", dma->dmapc);
	printf("dmapnva: 0x%x; ", dma->dmapnva);
	printf("dmapnc: 0x%x\n", dma->dmapnc);
	printf("dmacva: 0x%x; ", dma->dmacva);
	printf("dmacc: 0x%x; ", dma->dmacc);
	printf("dmacnva: 0x%x; ", dma->dmacnva);
	printf("dmacnc: 0x%x\n", dma->dmacnc);

	printf("apc_dmacsr=%s\n",
		bitmask_snprintf(dma->dmacsr, APC_BITS, bits, sizeof(bits)) );

	ad1848_dump_regs(&sc->sc_ad1848);
}
#endif

void
cs4231_init(sc)
	struct cs4231_softc *sc;
{
	char *buf;
#if 0
	volatile struct apc_dma *dma = sc->sc_dmareg;
#endif
	int reg;

#if 0
	dma->dmacsr = APC_CODEC_PDN;
	delay(20);
	dma->dmacsr &= ~APC_CODEC_PDN;
#endif
	/* First, put chip in native mode */
	reg = ad_read(&sc->sc_ad1848, SP_MISC_INFO);
	ad_write(&sc->sc_ad1848, SP_MISC_INFO, reg | MODE2);

	/* Read version numbers from I25 */
	reg = ad_read(&sc->sc_ad1848, CS_VERSION_ID);
	switch (reg & (CS_VERSION_NUMBER | CS_VERSION_CHIPID)) {
	case 0xa0:
		sc->sc_ad1848.chip_name = "CS4231A";
		break;
	case 0x80:
		sc->sc_ad1848.chip_name = "CS4231";
		break;
	case 0x82:
		sc->sc_ad1848.chip_name = "CS4232";
		break;
	default:
		if ((buf = malloc(32, M_TEMP, M_NOWAIT)) != NULL) {
			sprintf(buf, "unknown rev: %x/%x", reg&0xe, reg&7);
			sc->sc_ad1848.chip_name = buf;
		}
	}
}

void *
cs4231_malloc(addr, direction, size, pool, flags)
	void *addr;
	int direction;
	size_t size;
	int pool, flags;
{
	struct cs4231_softc *sc = addr;
	bus_dma_tag_t dmatag = sc->sc_dmatag;
	struct cs_dma *p;

	p = malloc(sizeof(*p), pool, flags);
	if (p == NULL)
		return (NULL);

	/* Allocate a DMA map */
	if (bus_dmamap_create(dmatag, size, 1, size, 0,
				BUS_DMA_NOWAIT, &p->dmamap) != 0)
		goto fail1;

	/* Allocate DMA memory */
	p->size = size;
	if (bus_dmamem_alloc(dmatag, size, 64*1024, 0,
				p->segs, sizeof(p->segs)/sizeof(p->segs[0]),
				&p->nsegs, BUS_DMA_NOWAIT) != 0)
		goto fail2;

	/* Map DMA memory into kernel space */
	if (bus_dmamem_map(dmatag, p->segs, p->nsegs, p->size, 
			   &p->addr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT) != 0)
		goto fail3;

	/* Load the buffer */
	if (bus_dmamap_load(dmatag, p->dmamap,
			    p->addr, size, NULL, BUS_DMA_NOWAIT) != 0)
		goto fail4;

	p->next = sc->sc_dmas;
	sc->sc_dmas = p;
	return (p->addr);

fail4:
	bus_dmamem_unmap(dmatag, p->addr, p->size);
fail3:
	bus_dmamem_free(dmatag, p->segs, p->nsegs);
fail2:
	bus_dmamap_destroy(dmatag, p->dmamap);
fail1:
	free(p, pool);
	return (NULL);
}

void
cs4231_free(addr, ptr, pool)
	void *addr;
	void *ptr;
	int pool;
{
	struct cs4231_softc *sc = addr;
	bus_dma_tag_t dmatag = sc->sc_dmatag;
	struct cs_dma *p, **pp;

	for (pp = &sc->sc_dmas; (p = *pp) != NULL; pp = &(*pp)->next) {
		if (p->addr != ptr)
			continue;
		bus_dmamap_unload(dmatag, p->dmamap);
		bus_dmamem_unmap(dmatag, p->addr, p->size);
		bus_dmamem_free(dmatag, p->segs, p->nsegs);
		bus_dmamap_destroy(dmatag, p->dmamap);
		*pp = p->next;
		free(p, pool);
		return;
	}
	printf("cs4231_free: rogue pointer\n");
}

int
cs4231_open(addr, flags)
	void *addr;
	int flags;
{
	struct cs4231_softc *sc = addr;
#if 0
	struct apc_dma *dma = sc->sc_dmareg;
#endif

	DPRINTF(("sa_open: unit %p\n", sc));

	if (sc->sc_open)
		return (EBUSY);
	sc->sc_open = 1;
	sc->sc_locked = 0;
	sc->sc_rintr = 0;
	sc->sc_rarg = 0;
	sc->sc_pintr = 0;
	sc->sc_parg = 0;
#if 1
	/*No interrupts from ad1848 */
	ad_write(&sc->sc_ad1848, SP_PIN_CONTROL, 0);
#endif
#if 0
	dma->dmacsr = APC_RESET;
	delay(10);
	dma->dmacsr = 0;
	delay(10);
#endif
	ad1848_reset(&sc->sc_ad1848);

	DPRINTF(("saopen: ok -> sc=%p\n", sc));
	return (0);
}

void
cs4231_close(addr)
	void *addr;
{
	struct cs4231_softc *sc = addr;

	DPRINTF(("sa_close: sc=%p\n", sc));
	/*
	 * halt i/o, clear open flag, and done.
	 */
	cs4231_halt_input(sc);
	cs4231_halt_output(sc);
	sc->sc_open = 0;

	DPRINTF(("sa_close: closed.\n"));
}

size_t
cs4231_round_buffersize(addr, direction, size)
	void *addr;
	int direction;
	size_t size;
{
#if 0
	if (size > APC_MAX)
		size = APC_MAX;
#endif
	return (size);
}

int
cs4231_round_blocksize(addr, blk)
	void *addr;
	int blk;
{
	return (blk & -4);
}

int
cs4231_getdev(addr, retp)
        void *addr;
        struct audio_device *retp;
{
        *retp = cs4231_device;
        return (0);
}

static ad1848_devmap_t csmapping[] = {
	{ CSAUDIO_DAC_LVL, AD1848_KIND_LVL, AD1848_AUX1_CHANNEL },
	{ CSAUDIO_LINE_IN_LVL, AD1848_KIND_LVL, AD1848_LINE_CHANNEL }, 
	{ CSAUDIO_MONO_LVL, AD1848_KIND_LVL, AD1848_MONO_CHANNEL },
	{ CSAUDIO_CD_LVL, AD1848_KIND_LVL, AD1848_AUX2_CHANNEL },
	{ CSAUDIO_MONITOR_LVL, AD1848_KIND_LVL, AD1848_MONITOR_CHANNEL },
	{ CSAUDIO_OUT_LVL, AD1848_KIND_LVL, AD1848_DAC_CHANNEL },
	{ CSAUDIO_DAC_MUTE, AD1848_KIND_MUTE, AD1848_AUX1_CHANNEL },
	{ CSAUDIO_LINE_IN_MUTE, AD1848_KIND_MUTE, AD1848_LINE_CHANNEL },
	{ CSAUDIO_MONO_MUTE, AD1848_KIND_MUTE, AD1848_MONO_CHANNEL },
	{ CSAUDIO_CD_MUTE, AD1848_KIND_MUTE, AD1848_AUX2_CHANNEL },
	{ CSAUDIO_MONITOR_MUTE, AD1848_KIND_MUTE, AD1848_MONITOR_CHANNEL },
	{ CSAUDIO_REC_LVL, AD1848_KIND_RECORDGAIN, -1 },
	{ CSAUDIO_RECORD_SOURCE, AD1848_KIND_RECORDSOURCE, -1 }
};

static int nummap = sizeof(csmapping) / sizeof(csmapping[0]);


int
cs4231_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct ad1848_softc *ac = addr;

	DPRINTF(("cs4231_set_port: port=%d", cp->dev));
	return (ad1848_mixer_set_port(ac, csmapping, nummap, cp));
}

int
cs4231_get_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct ad1848_softc *ac = addr;

	DPRINTF(("cs4231_get_port: port=%d", cp->dev));
	return (ad1848_mixer_get_port(ac, csmapping, nummap, cp));
}

int
cs4231_get_props(addr)
	void *addr;
{
	return (AUDIO_PROP_FULLDUPLEX);
}

int
cs4231_query_devinfo(addr, dip)
	void *addr;
	mixer_devinfo_t *dip;
{

	switch(dip->index) {
#if 0
	case CSAUDIO_MIC_IN_LVL:	/* Microphone */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CSAUDIO_MIC_IN_MUTE;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;
#endif

	case CSAUDIO_MONO_LVL:	/* mono/microphone mixer */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CSAUDIO_MONO_MUTE;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case CSAUDIO_DAC_LVL:		/*  dacout */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CSAUDIO_DAC_MUTE;
		strcpy(dip->label.name, AudioNdac);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case CSAUDIO_LINE_IN_LVL:	/* line */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CSAUDIO_LINE_IN_MUTE;
		strcpy(dip->label.name, AudioNline);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case CSAUDIO_CD_LVL:		/* cd */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CSAUDIO_CD_MUTE;
		strcpy(dip->label.name, AudioNcd);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;


	case CSAUDIO_MONITOR_LVL:	/* monitor level */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_MONITOR_CLASS;
		dip->next = CSAUDIO_MONITOR_MUTE;
		dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmonitor);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case CSAUDIO_OUT_LVL:		/* cs4231 output volume: not useful? */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_MONITOR_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNoutput);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case CSAUDIO_LINE_IN_MUTE:
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = CSAUDIO_LINE_IN_LVL;
		dip->next = AUDIO_MIXER_LAST;
		goto mute;
		
	case CSAUDIO_DAC_MUTE:
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = CSAUDIO_DAC_LVL;
		dip->next = AUDIO_MIXER_LAST;
		goto mute;

	case CSAUDIO_CD_MUTE:
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = CSAUDIO_CD_LVL;
		dip->next = AUDIO_MIXER_LAST;
		goto mute;
		
	case CSAUDIO_MONO_MUTE:
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = CSAUDIO_MONO_LVL;
		dip->next = AUDIO_MIXER_LAST;
		goto mute;

	case CSAUDIO_MONITOR_MUTE:
		dip->mixer_class = CSAUDIO_OUTPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = CSAUDIO_MONITOR_LVL;
		dip->next = AUDIO_MIXER_LAST;
	mute:
		strcpy(dip->label.name, AudioNmute);
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		break;
		
	case CSAUDIO_REC_LVL:	/* record level */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CSAUDIO_RECORD_SOURCE;
		strcpy(dip->label.name, AudioNrecord);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case CSAUDIO_RECORD_SOURCE:
		dip->mixer_class = CSAUDIO_RECORD_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = CSAUDIO_REC_LVL;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNsource);
		dip->un.e.num_mem = 4;
		strcpy(dip->un.e.member[0].label.name, AudioNoutput);
		dip->un.e.member[0].ord = DAC_IN_PORT;
		strcpy(dip->un.e.member[1].label.name, AudioNmicrophone);
		dip->un.e.member[1].ord = MIC_IN_PORT;
		strcpy(dip->un.e.member[2].label.name, AudioNdac);
		dip->un.e.member[2].ord = AUX1_IN_PORT;
		strcpy(dip->un.e.member[3].label.name, AudioNline);
		dip->un.e.member[3].ord = LINE_IN_PORT;
		break;

	case CSAUDIO_INPUT_CLASS:		/* input class descriptor */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCinputs);
		break;

	case CSAUDIO_OUTPUT_CLASS:		/* output class descriptor */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = CSAUDIO_OUTPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCoutputs);
		break;

	case CSAUDIO_MONITOR_CLASS:		/* monitor class descriptor */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = CSAUDIO_MONITOR_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCmonitor);
		break;
		    
	case CSAUDIO_RECORD_CLASS:		/* record source class */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = CSAUDIO_RECORD_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCrecord);
		break;

	default:
		return ENXIO;
		/*NOTREACHED*/
	}
	DPRINTF(("AUDIO_MIXER_DEVINFO: name=%s\n", dip->label.name));

	return (0);
}

int
cs4231_trigger_output(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct cs4231_softc *sc = addr;
	struct cs_dma *p;
	volatile struct apc_dma *dma = sc->sc_dmareg;
	int csr;
	vsize_t n;

	if (sc->sc_locked != 0) {
		printf("cs4231_trigger_output: already running\n");
		return (EINVAL);
	}

	sc->sc_locked = 1;
	sc->sc_pintr = intr;
	sc->sc_parg = arg;

	for (p = sc->sc_dmas; p != NULL && p->addr != start; p = p->next)
		/*void*/;
	if (p == NULL) {
		printf("cs4231_trigger_output: bad addr %p\n", start);
		return (EINVAL);
	}

	n = (char *)end - (char *)start;

	/* XXX
	 * Do only `blksize' at a time, so audio_pint() is kept
	 * synchronous with us...
	 */
	/*XXX*/sc->sc_blksz = blksize;
	/*XXX*/sc->sc_nowplaying = p;
	/*XXX*/sc->sc_playsegsz = n;

	if (n > APC_MAX)
		n = APC_MAX;

	sc->sc_playcnt = n;

	DPRINTF(("trigger_out: start %p, end %p, size %lu; "
		 "dmaaddr 0x%lx, dmacnt %lu, segsize %lu\n",
		 start, end, (u_long)sc->sc_playsegsz, 
		 (u_long)p->dmamap->dm_segs[0].ds_addr,
		 (u_long)n, (u_long)p->size));

	csr = dma->dmacsr;
	dma->dmapnva = (u_long)p->dmamap->dm_segs[0].ds_addr;
	dma->dmapnc = (u_long)n;
	if ((csr & PDMA_GO) == 0 || (csr & APC_PPAUSE) != 0) {
		int reg;

		dma->dmacsr &= ~(APC_PIE|APC_PPAUSE);
		dma->dmacsr |= APC_EI|APC_IE|APC_PIE|APC_EIE|APC_PMIE|PDMA_GO;

		/* Start chip */

		/* Probably should just ignore this.. */
		ad_write(&sc->sc_ad1848, SP_LOWER_BASE_COUNT, 0xff);
		ad_write(&sc->sc_ad1848, SP_UPPER_BASE_COUNT, 0xff);

		reg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
		ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG,
			 (PLAYBACK_ENABLE|reg));
	}

	return (0);
}

int
cs4231_trigger_input(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	return (ENXIO);
}

int
cs4231_halt_output(addr)
	void *addr;
{
	struct cs4231_softc *sc = addr;
	volatile struct apc_dma *dma = sc->sc_dmareg;
	int reg;

	dma->dmacsr &= ~(APC_EI | APC_IE | APC_PIE | APC_EIE | PDMA_GO | APC_PMIE);
	reg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG, (reg & ~PLAYBACK_ENABLE));
	sc->sc_locked = 0;

	return (0);
}

int
cs4231_halt_input(addr)
	void *addr;
{
	struct cs4231_softc *sc = addr;
	int reg;

	reg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG, (reg & ~CAPTURE_ENABLE));
	sc->sc_locked = 0;

	return (0);
}


int
cs4231_intr(arg)
	void *arg;
{
	struct cs4231_softc *sc = arg;
	volatile struct apc_dma *dma = sc->sc_dmareg;
	struct cs_dma *p;
	int ret = 0;
	int csr;
	int reg, status;
#if defined(DEBUG) || defined(AUDIO_DEBUG)
	char bits[128];
#endif

#ifdef AUDIO_DEBUG
	if (cs4231debug > 1)
		cs4231_regdump("audiointr", sc);
#endif

	/* Read DMA status */
	csr = dma->dmacsr;
	DPRINTF((
	    "intr: csr=%s; dmapva=0x%lx,dmapc=%lu;dmapnva=0x%lx,dmapnc=%lu\n",
		bitmask_snprintf(csr, APC_BITS, bits, sizeof(bits)),
		(u_long)dma->dmapva, (u_long)dma->dmapc,
		(u_long)dma->dmapnva, (u_long)dma->dmapnc));

	status = ADREAD(&sc->sc_ad1848, AD1848_STATUS);
	DPRINTF(("%s: status: %s\n", sc->sc_ad1848.sc_dev.dv_xname,
		bitmask_snprintf(status, AD_R2_BITS, bits, sizeof(bits))));
	if (status & (INTERRUPT_STATUS | SAMPLE_ERROR)) {
		reg = ad_read(&sc->sc_ad1848, CS_IRQ_STATUS);
		DPRINTF(("%s: i24: %s\n", sc->sc_ad1848.sc_dev.dv_xname,
		       bitmask_snprintf(reg, CS_I24_BITS, bits, sizeof(bits))));

		if (reg & CS_IRQ_PI) {
			ad_write(&sc->sc_ad1848, SP_LOWER_BASE_COUNT, 0xff);
			ad_write(&sc->sc_ad1848, SP_UPPER_BASE_COUNT, 0xff);
		}
		/* Clear interrupt bit */
		ADWRITE(&sc->sc_ad1848, AD1848_STATUS, 0);
	}

	/* Write back DMA status (clears interrupt) */
	dma->dmacsr = csr;

	/*
	 * Simplistic.. if "play emtpy" is set advance to next chunk.
	 */
#if 1
	/* Ack all play interrupts*/
	if ((csr & (APC_PI|APC_PD|APC_PIE|APC_PMI)) != 0)
		ret = 1;
#endif
	if (csr & APC_PM) {
		u_long nextaddr, togo;

		p = sc->sc_nowplaying;

		togo = sc->sc_playsegsz - sc->sc_playcnt;
		if (togo == 0) {
			/* Roll over */
			nextaddr = (u_long)p->dmamap->dm_segs[0].ds_addr;
			sc->sc_playcnt = togo = APC_MAX;
		} else {
			nextaddr = dma->dmapnva + APC_MAX;
			if (togo > APC_MAX)
				togo = APC_MAX;
			sc->sc_playcnt += togo;
		}

		dma->dmapnva = nextaddr;
		dma->dmapnc = togo;

		if (sc->sc_pintr != NULL)
			(*sc->sc_pintr)(sc->sc_parg);

		ret = 1;
	}

	if (csr & APC_CI) {
		if (sc->sc_rintr != NULL) {
			ret = 1;
			(*sc->sc_rintr)(sc->sc_rarg);
		}
	}

#ifdef DEBUG
if (ret == 0) {
	printf(
	    "oops: csr=%s; dmapva=0x%lx,dmapc=%lu;dmapnva=0x%lx,dmapnc=%lu\n",
		bitmask_snprintf(csr, APC_BITS, bits, sizeof(bits)),
		(u_long)dma->dmapva, (u_long)dma->dmapc,
		(u_long)dma->dmapnva, (u_long)dma->dmapnc);
	ret = 1;
}
#endif

	return (ret);
}
#endif /* NAUDIO > 0 */
