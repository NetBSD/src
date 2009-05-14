/* $NetBSD: haltwo.c,v 1.16 2009/05/14 01:06:15 macallan Exp $ */

/*
 * Copyright (c) 2003 Ilpo Ruotsalainen
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * 
 * <<Id: LICENSE_GC,v 1.1 2001/10/01 23:24:05 cgd Exp>>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: haltwo.c,v 1.16 2009/05/14 01:06:15 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/audioio.h>
#include <sys/malloc.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>
#include <dev/mulaw.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/sysconf.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>

#include <sgimips/hpc/haltworeg.h>
#include <sgimips/hpc/haltwovar.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)      printf x
#else
#define DPRINTF(x)
#endif

static int haltwo_query_encoding(void *, struct audio_encoding *);
static int haltwo_set_params(void *, int, int, audio_params_t *,
	audio_params_t *, stream_filter_list_t *, stream_filter_list_t *);
static int haltwo_round_blocksize(void *, int, int, const audio_params_t *);
static int haltwo_halt_output(void *);
static int haltwo_halt_input(void *);
static int haltwo_getdev(void *, struct audio_device *);
static int haltwo_set_port(void *, mixer_ctrl_t *);
static int haltwo_get_port(void *, mixer_ctrl_t *);
static int haltwo_query_devinfo(void *, mixer_devinfo_t *);
static void *haltwo_malloc(void *, int, size_t, struct malloc_type *, int);
static void haltwo_free(void *, void *, struct malloc_type *);
static int haltwo_get_props(void *);
static int haltwo_trigger_output(void *, void *, void *, int, void (*)(void *),
	void *, const audio_params_t *);
static int haltwo_trigger_input(void *, void *, void *, int, void (*)(void *),
	void *, const audio_params_t *);
static void haltwo_shutdown(void *);

static const struct audio_hw_if haltwo_hw_if = {
	NULL, /* open */
	NULL, /* close */
	NULL, /* drain */
	haltwo_query_encoding,
	haltwo_set_params,
	haltwo_round_blocksize,
	NULL, /* commit_settings */
	NULL, /* init_output */
	NULL, /* init_input */
	NULL, /* start_output */
	NULL, /* start_input */
	haltwo_halt_output,
	haltwo_halt_input,
	NULL, /* speaker_ctl */
	haltwo_getdev,
	NULL, /* setfd */
	haltwo_set_port,
	haltwo_get_port,
	haltwo_query_devinfo,
	haltwo_malloc,
	haltwo_free,
	NULL, /* round_buffersize */
	NULL, /* mappage */
	haltwo_get_props,
	haltwo_trigger_output,
	haltwo_trigger_input,
	NULL  /* dev_ioctl */
};

static const struct audio_device haltwo_device = {
	"HAL2",
	"",
	"haltwo"
};

static int  haltwo_match(struct device *, struct cfdata *, void *);
static void haltwo_attach(struct device *, struct device *, void *);
static int  haltwo_intr(void *);

CFATTACH_DECL(haltwo, sizeof(struct haltwo_softc),
    haltwo_match, haltwo_attach, NULL, NULL);

#define haltwo_write(sc,type,off,val) \
	bus_space_write_4(sc->sc_st, sc->sc_##type##_sh, off, val)

#define haltwo_read(sc,type,off) \
	bus_space_read_4(sc->sc_st, sc->sc_##type##_sh, off)

static void
haltwo_write_indirect(struct haltwo_softc *sc, uint32_t ireg, uint16_t low,
		uint16_t high)
{

	haltwo_write(sc, ctl, HAL2_REG_CTL_IDR0, low);
	haltwo_write(sc, ctl, HAL2_REG_CTL_IDR1, high);
	haltwo_write(sc, ctl, HAL2_REG_CTL_IDR2, 0);
	haltwo_write(sc, ctl, HAL2_REG_CTL_IDR3, 0);
	haltwo_write(sc, ctl, HAL2_REG_CTL_IAR, ireg);

	while (haltwo_read(sc, ctl, HAL2_REG_CTL_ISR) & HAL2_ISR_TSTATUS)
		continue;
}

static void
haltwo_read_indirect(struct haltwo_softc *sc, uint32_t ireg, uint16_t *low,
		uint16_t *high)
{

	haltwo_write(sc, ctl, HAL2_REG_CTL_IAR,
	    ireg | HAL2_IAR_READ);

	while (haltwo_read(sc, ctl, HAL2_REG_CTL_ISR) & HAL2_ISR_TSTATUS)
		continue;

	if (low)
		*low = haltwo_read(sc, ctl, HAL2_REG_CTL_IDR0);

	if (high)
		*high = haltwo_read(sc, ctl, HAL2_REG_CTL_IDR1);
}

static int
haltwo_init_codec(struct haltwo_softc *sc, struct haltwo_codec *codec)
{
	int err;
	int rseg;
	size_t allocsz;

	allocsz = sizeof(struct hpc_dma_desc) * HALTWO_MAX_DMASEGS;
	KASSERT(allocsz <= PAGE_SIZE);

	err = bus_dmamem_alloc(sc->sc_dma_tag, allocsz, 0, 0, &codec->dma_seg,
	    1, &rseg, BUS_DMA_NOWAIT);
	if (err)
		goto out;

	err = bus_dmamem_map(sc->sc_dma_tag, &codec->dma_seg, rseg, allocsz,
	    (void **)&codec->dma_descs, BUS_DMA_NOWAIT);
	if (err)
		goto out_free;

	err = bus_dmamap_create(sc->sc_dma_tag, allocsz, 1, PAGE_SIZE, 0,
	    BUS_DMA_NOWAIT, &codec->dma_map);
	if (err)
		goto out_free;

	err = bus_dmamap_load(sc->sc_dma_tag, codec->dma_map, codec->dma_descs,
	    allocsz, NULL, BUS_DMA_NOWAIT);
	if (err)
		goto out_destroy;

	DPRINTF(("haltwo_init_codec: allocated %d descriptors (%d bytes)"
	    " at %p\n", HALTWO_MAX_DMASEGS, allocsz, codec->dma_descs));

	memset(codec->dma_descs, 0, allocsz);

	return 0;

out_destroy:
	bus_dmamap_destroy(sc->sc_dma_tag, codec->dma_map);
out_free:
	bus_dmamem_free(sc->sc_dma_tag, &codec->dma_seg, rseg);
out:
	DPRINTF(("haltwo_init_codec failed: %d\n",err));

	return err;
}

static void
haltwo_setup_dma(struct haltwo_softc *sc, struct haltwo_codec *codec,
		struct haltwo_dmabuf *dmabuf, size_t len, int blksize,
		void (*intr)(void *), void *intrarg)
{
	int i;
	bus_dma_segment_t *segp;
	struct hpc_dma_desc *descp;
	int next_intr;

	KASSERT(len % blksize == 0);

	next_intr = blksize;
	codec->intr = intr;
	codec->intr_arg = intrarg;

	segp = dmabuf->dma_map->dm_segs;
	descp = codec->dma_descs;

	/* Build descriptor chain for looping DMA, triggering interrupt every
	 * blksize bytes */
	for (i = 0; i < dmabuf->dma_map->dm_nsegs; i++) {
		descp->hpc3_hdd_bufptr = segp->ds_addr;
		descp->hpc3_hdd_ctl = segp->ds_len;

		KASSERT(next_intr >= segp->ds_len);

		if (next_intr == segp->ds_len) {
			/* Generate intr after this DMA buffer */
			descp->hpc3_hdd_ctl |= HPC3_HDD_CTL_INTR;
			next_intr = blksize;
		} else
			next_intr -= segp->ds_len;

		if (i < dmabuf->dma_map->dm_nsegs - 1)
			descp->hdd_descptr = codec->dma_seg.ds_addr +
			    sizeof(struct hpc_dma_desc) * (i + 1);
		else
			descp->hdd_descptr = codec->dma_seg.ds_addr;

		DPRINTF(("haltwo_setup_dma: hdd_bufptr = %x hdd_ctl = %x"
		    " hdd_descptr = %x\n", descp->hpc3_hdd_bufptr,
		    descp->hpc3_hdd_ctl, descp->hdd_descptr));

		segp++;
		descp++;
	}

	bus_dmamap_sync(sc->sc_dma_tag, codec->dma_map, 0,
	    codec->dma_map->dm_mapsize, BUS_DMASYNC_PREWRITE);
}

static int
haltwo_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hpc_attach_args *haa;
	uint32_t rev;

	haa = aux;
	if (strcmp(haa->ha_name, cf->cf_name))
		return 0;

	if ( platform.badaddr((void *)(vaddr_t)(haa->ha_sh + haa->ha_devoff),
	    sizeof(u_int32_t)) )
		return 0;

	if ( platform.badaddr(
	    (void *)(vaddr_t)(haa->ha_sh + haa->ha_devoff + HAL2_REG_CTL_REV),
	    sizeof(u_int32_t)) )
		return 0;

	rev = *(uint32_t *)MIPS_PHYS_TO_KSEG1(haa->ha_sh + haa->ha_devoff +
	    HAL2_REG_CTL_REV);

	/* This bit is inverted, the test is correct */
	if (rev & HAL2_REV_AUDIO_PRESENT_N)
		return 0;

	return 1;
}

static void
haltwo_attach(struct device *parent, struct device *self, void *aux)
{
	struct haltwo_softc *sc;
	struct hpc_attach_args *haa;
	uint32_t rev;

	sc = (void *)self;
	haa = aux;
	sc->sc_st = haa->ha_st;
	sc->sc_dma_tag = haa->ha_dmat;

	if (bus_space_subregion(haa->ha_st, haa->ha_sh, haa->ha_devoff,
	    HPC3_PBUS_CH0_DEVREGS_SIZE, &sc->sc_ctl_sh)) {
		aprint_error(": unable to map control registers\n");
		return;
	}

	if (bus_space_subregion(haa->ha_st, haa->ha_sh, HPC3_PBUS_CH2_DEVREGS,
	    HPC3_PBUS_CH2_DEVREGS_SIZE, &sc->sc_vol_sh)) {
		aprint_error(": unable to map volume registers\n");
		return;
	}

	if (bus_space_subregion(haa->ha_st, haa->ha_sh, haa->ha_dmaoff,
	    HPC3_PBUS_DMAREGS_SIZE, &sc->sc_dma_sh)) {
		aprint_error(": unable to map DMA registers\n");
		return;
	}

	haltwo_write(sc, ctl, HAL2_REG_CTL_ISR, 0);
	haltwo_write(sc, ctl, HAL2_REG_CTL_ISR,
	    HAL2_ISR_GLOBAL_RESET_N | HAL2_ISR_CODEC_RESET_N);
	haltwo_write_indirect(sc, HAL2_IREG_RELAY_C, HAL2_RELAY_C_STATE, 0);

	rev = haltwo_read(sc, ctl, HAL2_REG_CTL_REV);

	if (cpu_intr_establish(haa->ha_irq, IPL_AUDIO, haltwo_intr, sc)
	    == NULL) {
		aprint_error(": unable to establish interrupt\n");
		return;
	}

	aprint_naive(": Audio controller\n");

	aprint_normal(": HAL2 revision %d.%d.%d\n", (rev & 0x7000) >> 12,
	    (rev & 0x00F0) >> 4, rev & 0x000F);

	if (haltwo_init_codec(sc, &sc->sc_dac)) {
		aprint_error(
		    "haltwo_attach: unable to create DMA descriptor list\n");
		return;
	}

	/* XXX Magic PBUS CFGDMA values from Linux HAL2 driver XXX */
	bus_space_write_4(haa->ha_st, haa->ha_sh, HPC3_PBUS_CH0_CFGDMA,
	    0x8208844);
	bus_space_write_4(haa->ha_st, haa->ha_sh, HPC3_PBUS_CH1_CFGDMA,
	    0x8208844);

	/* Unmute output */
	/* XXX Add mute/unmute support to mixer ops? XXX */
	haltwo_write_indirect(sc, HAL2_IREG_DAC_C2, 0, 0);

	/* Set master volume to zero */
	sc->sc_vol_left = sc->sc_vol_right = 0;
	haltwo_write(sc, vol, HAL2_REG_VOL_LEFT, sc->sc_vol_left);
	haltwo_write(sc, vol, HAL2_REG_VOL_RIGHT, sc->sc_vol_right);

	audio_attach_mi(&haltwo_hw_if, sc, &sc->sc_dev);

	sc->sc_sdhook = shutdownhook_establish(haltwo_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		aprint_error_dev(self,
		    "WARNING: unable to establish shutdown hook\n");
}

static int
haltwo_intr(void *v)
{
	struct haltwo_softc *sc;
	int ret;

	sc = v;
	ret = 0;
	if (bus_space_read_4(sc->sc_st, sc->sc_dma_sh, HPC3_PBUS_CH0_CTL)
	    & HPC3_PBUS_DMACTL_IRQ) {
		sc->sc_dac.intr(sc->sc_dac.intr_arg);

		ret = 1;
	} else
		DPRINTF(("haltwo_intr: Huh?\n"));

	return ret;
}

static int
haltwo_query_encoding(void *v, struct audio_encoding *e)
{

	switch (e->index) {
	case 0:
		strcpy(e->name, AudioEslinear_le);
		e->encoding = AUDIO_ENCODING_SLINEAR_LE;
		e->precision = 16;
		e->flags = 0;
		break;

	case 1:
		strcpy(e->name, AudioEslinear_be);
		e->encoding = AUDIO_ENCODING_SLINEAR_BE;
		e->precision = 16;
		e->flags = 0;
		break;

	case 2:
		strcpy(e->name, AudioEmulaw);
		e->encoding = AUDIO_ENCODING_ULAW;
		e->precision = 8;
		e->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;

	default:
		return EINVAL;
	}

	return 0;
}

static int
haltwo_set_params(void *v, int setmode, int usemode,
		  audio_params_t *play, audio_params_t *rec,
		  stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	audio_params_t hw;
	struct haltwo_softc *sc;
	int master, inc, mod;
	uint16_t tmp;

	sc = v;
	if (play->sample_rate < 4000)
		play->sample_rate = 4000;
	if (play->sample_rate > 48000)
		play->sample_rate = 48000;

	if (44100 % play->sample_rate < 48000 % play->sample_rate)
		master = 44100;
	else
		master = 48000;

	/* HAL2 specification 3.1.2.21: Codecs should be driven with INC/MOD
	 * fractions equivalent to 4/N, where N is a positive integer. */
	inc = 4;
	mod = master * inc / play->sample_rate;

	/* Fixup upper layers idea of HW sample rate to the actual final rate */
	play->sample_rate = master * inc / mod;

	DPRINTF(("haltwo_set_params: master = %d inc = %d mod = %d"
	    " sample_rate = %ld\n", master, inc, mod,
	    play->sample_rate));

	hw = *play;
	switch (play->encoding) {
	case AUDIO_ENCODING_ULAW:
		if (play->precision != 8)
			return EINVAL;

		hw.encoding = AUDIO_ENCODING_SLINEAR_LE;
		pfil->append(pfil, mulaw_to_linear16, &hw);
		play = &hw;
		break;
	case AUDIO_ENCODING_SLINEAR_BE:
	case AUDIO_ENCODING_SLINEAR_LE:
		break;

	default:
		return EINVAL;
	}
	/* play points HW encoding */

	/* Setup samplerate to HW */
	haltwo_write_indirect(sc, HAL2_IREG_BRES1_C1,
	    master == 44100 ? 1 : 0, 0);
	/* XXX Documentation disagrees but this seems to work XXX */
	haltwo_write_indirect(sc, HAL2_IREG_BRES1_C2,
	    inc, 0xFFFF & (inc - mod - 1));

	/* Setup endianness to HW */
	haltwo_read_indirect(sc, HAL2_IREG_DMA_END, &tmp, NULL);
	if (play->encoding == AUDIO_ENCODING_SLINEAR_LE)
		tmp |= HAL2_DMA_END_CODECTX;
	else
		tmp &= ~HAL2_DMA_END_CODECTX;
	haltwo_write_indirect(sc, HAL2_IREG_DMA_END, tmp, 0);

	/* Set PBUS channel, Bresenham clock source, number of channels to HW */
	haltwo_write_indirect(sc, HAL2_IREG_DAC_C1,
	    (0 << HAL2_C1_DMA_SHIFT) |
	    (1 << HAL2_C1_CLKID_SHIFT) |
	    (play->channels << HAL2_C1_DATAT_SHIFT), 0);

	DPRINTF(("haltwo_set_params: hw_encoding = %d hw_channels = %d\n",
	    play->encoding, play->channels));

	return 0;
}

static int
haltwo_round_blocksize(void *v, int blocksize,
		       int mode, const audio_params_t *param)
{

	/* XXX Make this smarter and support DMA descriptor chaining XXX */
	/* XXX Rounding to nearest PAGE_SIZE might work? XXX */
	return PAGE_SIZE;
}

static int
haltwo_halt_output(void *v)
{
	struct haltwo_softc *sc;

	sc = v;
	/* Disable PBUS DMA */
	bus_space_write_4(sc->sc_st, sc->sc_dma_sh, HPC3_PBUS_CH0_CTL,
	    HPC3_PBUS_DMACTL_ACT_LD);

	return 0;
}

static int
haltwo_halt_input(void *v)
{

	return ENXIO;
}

static int
haltwo_getdev(void *v, struct audio_device *dev)
{

	*dev = haltwo_device;
	return 0;
}

static int
haltwo_set_port(void *v, mixer_ctrl_t *mc)
{
	struct haltwo_softc *sc;
	int lval, rval;

	if (mc->type != AUDIO_MIXER_VALUE)
		return EINVAL;

	if (mc->un.value.num_channels == 1)
		lval = rval = mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
	else if (mc->un.value.num_channels == 2) {
		lval = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		rval = mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
	} else
		return EINVAL;

	sc = v;
	switch (mc->dev) {
	case HALTWO_MASTER_VOL:
		sc->sc_vol_left = lval;
		sc->sc_vol_right = rval;

		haltwo_write(sc, vol, HAL2_REG_VOL_LEFT,
		    sc->sc_vol_left);
		haltwo_write(sc, vol, HAL2_REG_VOL_RIGHT,
		    sc->sc_vol_right);
		break;

	default:
		return EINVAL;
	}

	return 0;
}

static int
haltwo_get_port(void *v, mixer_ctrl_t *mc)
{
	struct haltwo_softc *sc;
	int l, r;

	switch (mc->dev) {
	case HALTWO_MASTER_VOL:
		sc = v;
		l = sc->sc_vol_left;
		r = sc->sc_vol_right;
		break;

	default:
		return EINVAL;
	}

	if (mc->un.value.num_channels == 1)
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = (l+r) / 2;
	else if (mc->un.value.num_channels == 2) {
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT]  = l;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = r;
	} else
		return EINVAL;

	return 0;
}

static int
haltwo_query_devinfo(void *v, mixer_devinfo_t *dev)
{

	switch (dev->index) {
	/* Mixer values */
	case HALTWO_MASTER_VOL:
		dev->type = AUDIO_MIXER_VALUE;
		dev->mixer_class = HALTWO_OUTPUT_CLASS;
		dev->prev = dev->next = AUDIO_MIXER_LAST;
		strcpy(dev->label.name, AudioNmaster);
		dev->un.v.num_channels = 2;
		dev->un.v.delta = 16;
		strcpy(dev->un.v.units.name, AudioNvolume);
		break;

	/* Mixer classes */
	case HALTWO_OUTPUT_CLASS:
		dev->type = AUDIO_MIXER_CLASS;
		dev->mixer_class = HALTWO_OUTPUT_CLASS;
		dev->next = dev->prev = AUDIO_MIXER_LAST;
		strcpy(dev->label.name, AudioCoutputs);
		break;

	default:
		return EINVAL;
	}

	return 0;
}

static int
haltwo_alloc_dmamem(struct haltwo_softc *sc, size_t size,
		struct haltwo_dmabuf *p)
{
	int err;

	p->size = size;

	/* XXX Check align/boundary XXX */
	/* XXX Pass flags and use them instead BUS_DMA_NOWAIT? XXX */
	err = bus_dmamem_alloc(sc->sc_dma_tag, p->size, 0, 0, p->dma_segs,
	    HALTWO_MAX_DMASEGS, &p->dma_segcount, BUS_DMA_NOWAIT);
	if (err)
		goto out;

	/* XXX BUS_DMA_COHERENT? XXX */
	err = bus_dmamem_map(sc->sc_dma_tag, p->dma_segs, p->dma_segcount,
	    p->size, &p->kern_addr, BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (err)
		goto out_free;

	/* XXX Just guessing ... XXX */
	err = bus_dmamap_create(sc->sc_dma_tag, p->size, HALTWO_MAX_DMASEGS,
	    PAGE_SIZE, 0, BUS_DMA_NOWAIT, &p->dma_map);
	if (err)
		goto out_free;

	err = bus_dmamap_load(sc->sc_dma_tag, p->dma_map, p->kern_addr,
	    p->size, NULL, BUS_DMA_NOWAIT);
	if (err)
		goto out_destroy;

	return 0;

out_destroy:
	bus_dmamap_destroy(sc->sc_dma_tag, p->dma_map);
out_free:
	bus_dmamem_free(sc->sc_dma_tag, p->dma_segs, p->dma_segcount);
out:
	DPRINTF(("haltwo_alloc_dmamem failed: %d\n",err));

	return err;
}

static void *
haltwo_malloc(void *v, int direction, size_t size, struct malloc_type *type,
		int flags)
{
	struct haltwo_softc *sc;
	struct haltwo_dmabuf *p;

	DPRINTF(("haltwo_malloc size = %d\n", size));
	sc = v;
	p = malloc(sizeof(struct haltwo_dmabuf), type, flags);
	if (!p)
		return 0;

	if (haltwo_alloc_dmamem(sc, size, p)) {
		free(p, type);
		return 0;
	}

	p->next = sc->sc_dma_bufs;
	sc->sc_dma_bufs = p;

	return p->kern_addr;
}

static void
haltwo_free(void *v, void *addr, struct malloc_type *type)
{
	struct haltwo_softc *sc;
	struct haltwo_dmabuf *p, **pp;

	sc = v;
	for (pp = &sc->sc_dma_bufs; (p = *pp) != NULL; pp = &p->next) {
		if (p->kern_addr == addr) {
			*pp = p->next;
			free(p, type);
			return;
		}
	}

	panic("haltwo_free: buffer not in list");
}

static int
haltwo_get_props(void *v)
{

	return 0;
}

static int
haltwo_trigger_output(void *v, void *start, void *end, int blksize,
		void (*intr)(void *), void *intrarg, const audio_params_t *param)
{
	struct haltwo_softc *sc;
	struct haltwo_dmabuf *p;
	uint16_t tmp;
	uint32_t ctrl;
	unsigned int fifobeg, fifoend, highwater;

	DPRINTF(("haltwo_trigger_output start = %p end = %p blksize = %d"
	    " param = %p\n", start, end, blksize, param));
	sc = v;
	for (p = sc->sc_dma_bufs; p != NULL; p = p->next)
		if (p->kern_addr == start)
			break;

	if (p == NULL) {
		printf("haltwo_trigger_output: buffer not in list\n");

		return EINVAL;
	}

	/* Disable PBUS DMA */
	bus_space_write_4(sc->sc_st, sc->sc_dma_sh, HPC3_PBUS_CH0_CTL,
	    HPC3_PBUS_DMACTL_ACT_LD);

	/* Disable HAL2 codec DMA */
	haltwo_read_indirect(sc, HAL2_IREG_DMA_PORT_EN, &tmp, NULL);
	haltwo_write_indirect(sc, HAL2_IREG_DMA_PORT_EN,
	    tmp & ~HAL2_DMA_PORT_EN_CODECTX, 0);

	haltwo_setup_dma(sc, &sc->sc_dac, p, (char *)end - (char *)start,
	    blksize, intr, intrarg);

	highwater = (param->channels * 4) >> 1;
	fifobeg = 0;
	fifoend = (param->channels * 8) >> 3;

	DPRINTF(("haltwo_trigger_output: hw_channels = %d highwater = %d"
	    " fifobeg = %d fifoend = %d\n", param->hw_channels, highwater,
	    fifobeg, fifoend));

	ctrl = HPC3_PBUS_DMACTL_RT
	    | HPC3_PBUS_DMACTL_ACT_LD
	    | (highwater << HPC3_PBUS_DMACTL_HIGHWATER_SHIFT)
	    | (fifobeg << HPC3_PBUS_DMACTL_FIFOBEG_SHIFT)
	    | (fifoend << HPC3_PBUS_DMACTL_FIFOEND_SHIFT);

	/* Using PBUS CH0 for DAC DMA */
	haltwo_write_indirect(sc, HAL2_IREG_DMA_DRV, 1, 0);

	/* HAL2 is ready for action, now setup PBUS for DMA transfer */
	bus_space_write_4(sc->sc_st, sc->sc_dma_sh, HPC3_PBUS_CH0_DP,
	    sc->sc_dac.dma_seg.ds_addr);
	bus_space_write_4(sc->sc_st, sc->sc_dma_sh, HPC3_PBUS_CH0_CTL,
	    ctrl | HPC3_PBUS_DMACTL_ACT);

	/* Both HAL2 and PBUS have been setup, now start it up */
	haltwo_read_indirect(sc, HAL2_IREG_DMA_PORT_EN, &tmp, NULL);
	haltwo_write_indirect(sc, HAL2_IREG_DMA_PORT_EN,
	    tmp | HAL2_DMA_PORT_EN_CODECTX, 0);

	return 0;
}

static int
haltwo_trigger_input(void *v, void *start, void *end, int blksize,
		void (*intr)(void *), void *intrarg, const audio_params_t *param)
{
	struct haltwo_softc *sc;
	struct haltwo_dmabuf *p;

	DPRINTF(("haltwo_trigger_input start = %p end = %p blksize = %d\n",
	    start, end, blksize));
	sc = v;
	for (p = sc->sc_dma_bufs; p != NULL; p = p->next)
		if (p->kern_addr == start)
			break;

	if (p == NULL) {
		printf("haltwo_trigger_input: buffer not in list\n");

		return EINVAL;
	}

#if 0
	haltwo_setup_dma(sc, &sc->sc_adc, p, (char *)end - (char *)start,
	    blksize, intr, intrarg);
#endif

	return ENXIO;
}

void
haltwo_shutdown(void *arg)
{
	struct haltwo_softc *sc = arg;

	haltwo_write(sc, ctl, HAL2_REG_CTL_ISR, 0);
	haltwo_write(sc, ctl, HAL2_REG_CTL_ISR,
	    HAL2_ISR_GLOBAL_RESET_N | HAL2_ISR_CODEC_RESET_N);
}
