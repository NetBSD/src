/*	$NetBSD: cs4231.c,v 1.11 2003/02/01 13:23:28 martin Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cs4231.c,v 1.11 2003/02/01 13:23:28 martin Exp $");

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

/*---*/
#define CSAUDIO_DAC_LVL		0
#define CSAUDIO_LINE_IN_LVL	1
#define CSAUDIO_MONO_LVL	2
#define CSAUDIO_CD_LVL		3
#define CSAUDIO_OUTPUT_LVL	4
#define CSAUDIO_OUT_LVL		5
#define CSAUDIO_LINE_IN_MUTE	6
#define CSAUDIO_DAC_MUTE	7
#define CSAUDIO_CD_MUTE		8
#define CSAUDIO_MONO_MUTE	9
#define CSAUDIO_OUTPUT_MUTE	10
#define CSAUDIO_OUT_MUTE	11
#define CSAUDIO_REC_LVL		12
#define CSAUDIO_RECORD_SOURCE	13

#define CSAUDIO_INPUT_CLASS	14
#define CSAUDIO_MONITOR_CLASS	15
#define CSAUDIO_RECORD_CLASS	16

#ifdef AUDIO_DEBUG
int     cs4231_debug = 0;
#define DPRINTF(x)      if (cs4231_debug) printf x
#else
#define DPRINTF(x)
#endif

struct audio_device cs4231_device = {
	"cs4231",
	"x",
	"audio"
};


/* ad1848 sc_{read,write}reg */
static int	cs4231_read(struct ad1848_softc *, int);
static void	cs4231_write(struct ad1848_softc *, int, int);

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


void
cs4231_common_attach(sc, ioh)
	struct cs4231_softc *sc;
	bus_space_handle_t ioh;
{
	char *buf;
	int reg;

	sc->sc_ad1848.parent = sc;
	sc->sc_ad1848.sc_iot = sc->sc_bustag;
	sc->sc_ad1848.sc_ioh = ioh;
	sc->sc_ad1848.sc_readreg = cs4231_read;
	sc->sc_ad1848.sc_writereg = cs4231_write;

	sc->sc_playback.t_name = "playback";
	sc->sc_capture.t_name = "capture";

	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR,
			     NULL,
			     sc->sc_ad1848.sc_dev.dv_xname, "total");

	evcnt_attach_dynamic(&sc->sc_playback.t_intrcnt, EVCNT_TYPE_INTR,
			     &sc->sc_intrcnt,
			     sc->sc_ad1848.sc_dev.dv_xname, "playback");

	evcnt_attach_dynamic(&sc->sc_playback.t_ierrcnt, EVCNT_TYPE_INTR,
			     &sc->sc_intrcnt,
			     sc->sc_ad1848.sc_dev.dv_xname, "perrors");

	evcnt_attach_dynamic(&sc->sc_capture.t_intrcnt, EVCNT_TYPE_INTR,
			     &sc->sc_intrcnt,
			     sc->sc_ad1848.sc_dev.dv_xname, "capture");

	evcnt_attach_dynamic(&sc->sc_capture.t_ierrcnt, EVCNT_TYPE_INTR,
			     &sc->sc_intrcnt,
			     sc->sc_ad1848.sc_dev.dv_xname, "cerrors");

	/* put chip in native mode to access (extended) ID register */
	reg = ad_read(&sc->sc_ad1848, SP_MISC_INFO);
	ad_write(&sc->sc_ad1848, SP_MISC_INFO, reg | MODE2);

	/* read version numbers from I25 */
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
			sprintf(buf, "unknown rev: %x/%x", reg&0xe0, reg&7);
			sc->sc_ad1848.chip_name = buf;
		}
	}

	sc->sc_ad1848.mode = 2;	/* put ad1848 driver in `MODE 2' mode */
	ad1848_attach(&sc->sc_ad1848);
}

void *
cs4231_malloc(addr, direction, size, pool, flags)
	void *addr;
	int direction;
	size_t size;
	struct malloc_type *pool;
	int flags;
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
	struct malloc_type *pool;
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


/*
 * Set up transfer and return DMA address and byte count in paddr and psize
 * for bus dependent trigger_{in,out}put to load into the dma controller.
 */
int
cs4231_transfer_init(sc, t, paddr, psize, start, end, blksize, intr, arg)
	struct cs4231_softc *sc;
	struct cs_transfer *t;
	bus_addr_t *paddr;
	bus_size_t *psize;
	void *start, *end;
	int blksize;
	void (*intr)(void *);
	void *arg;
{
	struct cs_dma *p;
	vsize_t n;

	if (t->t_active) {
		printf("%s: %s already running\n",
		       sc->sc_ad1848.sc_dev.dv_xname, t->t_name);
		return (EINVAL);
	}

	t->t_intr = intr;
	t->t_arg = arg;

	for (p = sc->sc_dmas; p != NULL && p->addr != start; p = p->next)
		continue;
	if (p == NULL) {
		printf("%s: bad %s addr %p\n",
		       sc->sc_ad1848.sc_dev.dv_xname, t->t_name, start);
		return (EINVAL);
	}

	n = (char *)end - (char *)start;

	t->t_dma = p;		/* the DMA memory segment */
	t->t_segsz = n;		/* size of DMA segment */
	t->t_blksz = blksize;	/* do transfers in blksize chunks */

	if (n > t->t_blksz)
		n = t->t_blksz;

	t->t_cnt = n;

	/* for caller to load into dma controller */
	*paddr = t->t_dma->dmamap->dm_segs[0].ds_addr;
	*psize = n;

	DPRINTF(("%s: init %s: [%p..%p] %lu bytes %lu blocks;"
		 " dma at 0x%lx count %lu\n",
		 sc->sc_ad1848.sc_dev.dv_xname, t->t_name,
		 start, end, (u_long)t->t_segsz, (u_long)t->t_blksz,
		 (u_long)*paddr, (u_long)*psize));

	t->t_active = 1;
	return (0);
}

/*
 * Compute next DMA address/counter, update transfer status.
 */
void
cs4231_transfer_advance(t, paddr, psize)
	struct cs_transfer *t;
	bus_addr_t *paddr;
	bus_size_t *psize;
{
	bus_addr_t dmabase, nextaddr;
	bus_size_t togo;

	dmabase = t->t_dma->dmamap->dm_segs[0].ds_addr;

	togo = t->t_segsz - t->t_cnt;
	if (togo == 0) {	/* roll over */
		nextaddr = dmabase;
		t->t_cnt = togo = t->t_blksz;
	} else {
		nextaddr = dmabase + t->t_cnt;
		if (togo > t->t_blksz)
			togo = t->t_blksz;
		t->t_cnt += togo;
	}

	/* for caller to load into dma controller */
	*paddr = nextaddr;
	*psize = togo;
}


int
cs4231_open(addr, flags)
	void *addr;
	int flags;
{
	struct cs4231_softc *sc = addr;

	DPRINTF(("sa_open: unit %p\n", sc));

	if (sc->sc_open)
		return (EBUSY);

	sc->sc_open = 1;

	sc->sc_playback.t_active = 0;
	sc->sc_playback.t_intr = NULL;
	sc->sc_playback.t_arg = NULL;

	sc->sc_capture.t_active = 0;
	sc->sc_capture.t_intr = NULL;
	sc->sc_capture.t_arg = NULL;

	/* no interrupts from ad1848 */
	ad_write(&sc->sc_ad1848, SP_PIN_CONTROL, 0);
	ad1848_reset(&sc->sc_ad1848);

	DPRINTF(("sa_open: ok -> sc=%p\n", sc));
	return (0);
}

void
cs4231_close(addr)
	void *addr;
{
	struct cs4231_softc *sc = addr;

	DPRINTF(("sa_close: sc=%p\n", sc));

	/* audio(9) already called halt methods */
	sc->sc_open = 0;

	DPRINTF(("sa_close: closed.\n"));
}

size_t
cs4231_round_buffersize(addr, direction, size)
	void *addr;
	int direction;
	size_t size;
{

	return (size);
}

int
cs4231_round_blocksize(addr, blk)
	void *addr;
	int blk;
{

	return (blk & ~3);
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
	{ CSAUDIO_OUTPUT_LVL, AD1848_KIND_LVL, AD1848_MONITOR_CHANNEL },
	{ CSAUDIO_OUT_LVL, AD1848_KIND_LVL, AD1848_DAC_CHANNEL },
	{ CSAUDIO_DAC_MUTE, AD1848_KIND_MUTE, AD1848_AUX1_CHANNEL },
	{ CSAUDIO_LINE_IN_MUTE, AD1848_KIND_MUTE, AD1848_LINE_CHANNEL },
	{ CSAUDIO_MONO_MUTE, AD1848_KIND_MUTE, AD1848_MONO_CHANNEL },
	{ CSAUDIO_CD_MUTE, AD1848_KIND_MUTE, AD1848_AUX2_CHANNEL },
	{ CSAUDIO_OUTPUT_MUTE, AD1848_KIND_MUTE, AD1848_MONITOR_CHANNEL },
	{ CSAUDIO_OUT_MUTE, AD1848_KIND_MUTE, AD1848_OUT_CHANNEL },
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

	case CSAUDIO_MONO_LVL:	/* mono/microphone mixer */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CSAUDIO_MONO_MUTE;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 1;
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


	case CSAUDIO_OUTPUT_LVL:	/* monitor level */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_MONITOR_CLASS;
		dip->next = CSAUDIO_OUTPUT_MUTE;
		dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmonitor);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case CSAUDIO_OUT_LVL:		/* cs4231 output volume */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CSAUDIO_MONITOR_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmaster);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case CSAUDIO_OUT_MUTE: /* mute built-in speaker */
		dip->mixer_class = CSAUDIO_MONITOR_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = CSAUDIO_MONITOR_CLASS;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmono);
		/* names reversed, this is a "mute" value used as "mono enabled" */
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNon);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNoff);
		dip->un.e.member[1].ord = 1;
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

	case CSAUDIO_OUTPUT_MUTE:
		dip->mixer_class = CSAUDIO_MONITOR_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = CSAUDIO_OUTPUT_LVL;
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

	case CSAUDIO_MONITOR_CLASS:		/* output class descriptor */
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

#endif /* NAUDIO > 0 */
