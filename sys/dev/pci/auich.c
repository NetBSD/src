/*	$NetBSD: auich.c,v 1.4 2001/10/03 00:04:51 augustss Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 2000 Michael Shalayeff
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
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	from OpenBSD: ich.c,v 1.3 2000/08/11 06:17:18 mickey Exp
 */

/* #define	ICH_DEBUG */
/*
 * AC'97 audio found on Intel 810/820/440MX chipsets.
 *	http://developer.intel.com/design/chipsets/datashts/290655.htm
 *	http://developer.intel.com/design/chipsets/manuals/298028.htm
 *
 * TODO:
 *
 *	- Probe codecs for supported sample rates.
 *
 *	- Add support for the microphone input.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>	/* for PAGE_SIZE */

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/auichreg.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>

#include <machine/bus.h>

#include <dev/ic/ac97reg.h>
#include <dev/ic/ac97var.h>

struct auich_dma {
	bus_dmamap_t map;
	caddr_t addr;
	bus_dma_segment_t segs[1];
	int nsegs;
	size_t size;
	struct auich_dma *next;
};

#define	DMAADDR(p)	((p)->map->dm_segs[0].ds_addr)
#define	KERNADDR(p)	((void *)((p)->addr))

struct auich_cdata {
	struct auich_dmalist ic_dmalist_pcmo[ICH_DMALIST_MAX];
	struct auich_dmalist ic_dmalist_pcmi[ICH_DMALIST_MAX];
	struct auich_dmalist ic_dmalist_mici[ICH_DMALIST_MAX];
};

#define	ICH_CDOFF(x)		offsetof(struct auich_cdata, x)
#define	ICH_PCMO_OFF(x)		ICH_CDOFF(ic_dmalist_pcmo[(x)])
#define	ICH_PCMI_OFF(x)		ICH_CDOFF(ic_dmalist_pcmi[(x)])
#define	ICH_MICI_OFF(x)		ICH_CDOFF(ic_dmalist_mici[(x)])

struct auich_softc {
	struct device sc_dev;
	void *sc_ih;

	audio_device_t sc_audev;

	bus_space_tag_t iot;
	bus_space_handle_t mix_ioh;
	bus_space_handle_t aud_ioh;
	bus_dma_tag_t dmat;

	struct ac97_codec_if *codec_if;
	struct ac97_host_if host_if;

	/* DMA scatter-gather lists. */
	bus_dmamap_t sc_cddmamap;
#define	sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	struct auich_cdata *sc_cdata;
#define	dmalist_pcmo	sc_cdata->ic_dmalist_pcmo
#define	dmalist_pcmi	sc_cdata->ic_dmalist_pcmi
#define	dmalist_mici	sc_cdata->ic_dmalist_mici

	int	ptr_pcmo,
		ptr_pcmi,
		ptr_mici;

	/* i/o buffer pointers */
	u_int32_t pcmo_start, pcmo_p, pcmo_end;
	int pcmo_blksize, pcmo_fifoe;

	u_int32_t pcmi_start, pcmi_p, pcmi_end;
	int pcmi_blksize, pcmi_fifoe;

	u_int32_t mici_start, mici_p, mici_end;
	int mici_blksize, mici_fifoe;

	struct auich_dma *sc_dmas;

	void (*sc_pintr)(void *);
	void *sc_parg;

	void (*sc_rintr)(void *);
	void *sc_rarg;
};

/* Debug */
#ifdef AUDIO_DEBUG
#define	DPRINTF(l,x)	do { if (auich_debug & (l)) printf x; } while(0)
int auich_debug = 0xfffe;
#define	ICH_DEBUG_CODECIO	0x0001
#define	ICH_DEBUG_DMA		0x0002
#define	ICH_DEBUG_PARAM		0x0004
#else
#define	DPRINTF(x,y)	/* nothing */
#endif

int	auich_match(struct device *, struct cfdata *, void *);
void	auich_attach(struct device *, struct device *, void *);
int	auich_intr(void *);

struct cfattach auich_ca = {
	sizeof(struct auich_softc), auich_match, auich_attach
};

int	auich_open(void *, int);
void	auich_close(void *);
int	auich_query_encoding(void *, struct audio_encoding *);
int	auich_set_params(void *, int, int, struct audio_params *,
	    struct audio_params *);
int	auich_round_blocksize(void *, int);
int	auich_halt_output(void *);
int	auich_halt_input(void *);
int	auich_getdev(void *, struct audio_device *);
int	auich_set_port(void *, mixer_ctrl_t *);
int	auich_get_port(void *, mixer_ctrl_t *);
int	auich_query_devinfo(void *, mixer_devinfo_t *);
void	*auich_allocm(void *, int, size_t, int, int);
void	auich_freem(void *, void *, int);
size_t	auich_round_buffersize(void *, int, size_t);
paddr_t	auich_mappage(void *, void *, off_t, int);
int	auich_get_props(void *);
int	auich_trigger_output(void *, void *, void *, int, void (*)(void *),
	    void *, struct audio_params *);
int	auich_trigger_input(void *, void *, void *, int, void (*)(void *),
	    void *, struct audio_params *);

int	auich_alloc_cdata(struct auich_softc *);

int	auich_allocmem(struct auich_softc *, size_t, size_t,
	    struct auich_dma *);
int	auich_freemem(struct auich_softc *, struct auich_dma *);

struct audio_hw_if auich_hw_if = {
	auich_open,
	auich_close,
	NULL,			/* drain */
	auich_query_encoding,
	auich_set_params,
	auich_round_blocksize,
	NULL,			/* commit_setting */
	NULL,			/* init_output */
	NULL,			/* init_input */
	NULL,			/* start_output */
	NULL,			/* start_input */
	auich_halt_output,
	auich_halt_input,
	NULL,			/* speaker_ctl */
	auich_getdev,
	NULL,			/* getfd */
	auich_set_port,
	auich_get_port,
	auich_query_devinfo,
	auich_allocm,
	auich_freem,
	auich_round_buffersize,
	auich_mappage,
	auich_get_props,
	auich_trigger_output,
	auich_trigger_input,
	NULL,			/* dev_ioctl */
};

int	auich_attach_codec(void *, struct ac97_codec_if *);
int	auich_read_codec(void *, u_int8_t, u_int16_t *);
int	auich_write_codec(void *, u_int8_t, u_int16_t);
void	auich_reset_codec(void *);

static const struct auich_devtype {
	int	product;
	const char *name;
	const char *shortname;
} auich_devices[] = {
	{ PCI_PRODUCT_INTEL_82801AA_ACA,
	    "i82801AA (ICH) AC-97 Audio",	"ICH" },
	{ PCI_PRODUCT_INTEL_82801AB_ACA,
	    "i82801AB (ICH0) AC-97 Audio",	"ICH0" },
	{ PCI_PRODUCT_INTEL_82801BA_ACA,
	    "i82801BA (ICH2) AC-97 Audio",	"ICH2" },
	{ PCI_PRODUCT_INTEL_82440MX_ACA,
	    "i82440MX AC-97 Audio",		"440MX" },

	{ 0,
	    NULL,			NULL },
};

static const struct auich_devtype *
auich_lookup(struct pci_attach_args *pa)
{
	const struct auich_devtype *d;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return (NULL);

	for (d = auich_devices; d->name != NULL; d++) {
		if (PCI_PRODUCT(pa->pa_id) == d->product)
			return (d);
	}

	return (NULL);
}

int
auich_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (auich_lookup(pa) != NULL)
		return (1);

	return (0);
}

void
auich_attach(struct device *parent, struct device *self, void *aux)
{
	struct auich_softc *sc = (struct auich_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_intr_handle_t ih;
	bus_size_t mix_size, aud_size;
	pcireg_t csr;
	const char *intrstr;
	const struct auich_devtype *d;

	d = auich_lookup(pa);
	if (d == NULL)
		panic("auich_attach: impossible");

	printf(": %s\n", d->name);

	if (pci_mapreg_map(pa, ICH_NAMBAR, PCI_MAPREG_TYPE_IO, 0,
			   &sc->iot, &sc->mix_ioh, NULL, &mix_size)) {
		printf("%s: can't map codec i/o space\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (pci_mapreg_map(pa, ICH_NABMBAR, PCI_MAPREG_TYPE_IO, 0,
			   &sc->iot, &sc->aud_ioh, NULL, &aud_size)) {
		printf("%s: can't map device i/o space\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->dmat = pa->pa_dmat;

	/* enable bus mastering */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: can't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_AUDIO,
	    auich_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: can't establish interrupt", sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	sprintf(sc->sc_audev.name, "%s AC97", d->shortname);
	sprintf(sc->sc_audev.version, "0x%02x", PCI_REVISION(pa->pa_class));
	strcpy(sc->sc_audev.config, sc->sc_dev.dv_xname);

	/* Set up DMA lists. */
	sc->ptr_pcmo = sc->ptr_pcmi = sc->ptr_mici = 0;
	auich_alloc_cdata(sc);

	DPRINTF(ICH_DEBUG_DMA, ("auich_attach: lists %p %p %p\n",
	    sc->dmalist_pcmo, sc->dmalist_pcmi, sc->dmalist_mici));

	/* Reset codec and AC'97 */
	auich_reset_codec(sc);

	sc->host_if.arg = sc;
	sc->host_if.attach = auich_attach_codec;
	sc->host_if.read = auich_read_codec;
	sc->host_if.write = auich_write_codec;
	sc->host_if.reset = auich_reset_codec;

	if (ac97_attach(&sc->host_if) != 0)
		return;

	audio_attach_mi(&auich_hw_if, sc, &sc->sc_dev);
}

int
auich_read_codec(void *v, u_int8_t reg, u_int16_t *val)
{
	struct auich_softc *sc = v;
	int i;

	/* wait for an access semaphore */
	for (i = ICH_SEMATIMO; i-- &&
	    bus_space_read_1(sc->iot, sc->aud_ioh, ICH_CAS) & 1; DELAY(1));

	if (i > 0) {
		*val = bus_space_read_2(sc->iot, sc->mix_ioh, reg);
		DPRINTF(ICH_DEBUG_CODECIO,
		    ("auich_read_codec(%x, %x)\n", reg, *val));

		return 0;
	} else {
		DPRINTF(ICH_DEBUG_CODECIO,
		    ("%s: read_codec timeout\n", sc->sc_dev.dv_xname));
		return -1;
	}
}

int
auich_write_codec(void *v, u_int8_t reg, u_int16_t val)
{
	struct auich_softc *sc = v;
	int i;

	DPRINTF(ICH_DEBUG_CODECIO, ("auich_write_codec(%x, %x)\n", reg, val));

	/* wait for an access semaphore */
	for (i = ICH_SEMATIMO; i-- &&
	    bus_space_read_1(sc->iot, sc->aud_ioh, ICH_CAS) & 1; DELAY(1));

	if (i > 0) {
		bus_space_write_2(sc->iot, sc->mix_ioh, reg, val);
		return 0;
	} else {
		DPRINTF(ICH_DEBUG_CODECIO,
		    ("%s: write_codec timeout\n", sc->sc_dev.dv_xname));
		return -1;
	}
}

int
auich_attach_codec(void *v, struct ac97_codec_if *cif)
{
	struct auich_softc *sc = v;

	sc->codec_if = cif;
	return 0;
}

void
auich_reset_codec(void *v)
{
	struct auich_softc *sc = v;

	bus_space_write_4(sc->iot, sc->aud_ioh, ICH_GCTRL, 0);
	DELAY(10);
	bus_space_write_4(sc->iot, sc->aud_ioh, ICH_GCTRL, ICH_CRESET);
}

int
auich_open(void *v, int flags)
{

	return 0;
}

void
auich_close(void *v)
{
	struct auich_softc *sc = v;

	auich_halt_output(sc);
	auich_halt_input(sc);

	sc->sc_pintr = NULL;
	sc->sc_rintr = NULL;
}

int
auich_query_encoding(void *v, struct audio_encoding *aep)
{
	switch (aep->index) {
#if 0 /* XXX Not until we emulate it. */
	case 0:
		strcpy(aep->name, AudioEulinear);
		aep->encoding = AUDIO_ENCODING_ULINEAR;
		aep->precision = 8;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
#endif
	case 1:
		strcpy(aep->name, AudioEmulaw);
		aep->encoding = AUDIO_ENCODING_ULAW;
		aep->precision = 8;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 2:
		strcpy(aep->name, AudioEalaw);
		aep->encoding = AUDIO_ENCODING_ALAW;
		aep->precision = 8;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 3:
		strcpy(aep->name, AudioEslinear);
		aep->encoding = AUDIO_ENCODING_SLINEAR;
		aep->precision = 8;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 4:
		strcpy(aep->name, AudioEslinear_le);
		aep->encoding = AUDIO_ENCODING_SLINEAR_LE;
		aep->precision = 16;
		aep->flags = 0;
		return (0);
	case 5:
		strcpy(aep->name, AudioEulinear_le);
		aep->encoding = AUDIO_ENCODING_ULINEAR_LE;
		aep->precision = 16;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 6:
		strcpy(aep->name, AudioEslinear_be);
		aep->encoding = AUDIO_ENCODING_SLINEAR_BE;
		aep->precision = 16;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 7:
		strcpy(aep->name, AudioEulinear_be);
		aep->encoding = AUDIO_ENCODING_ULINEAR_BE;
		aep->precision = 16;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	default:
		return (EINVAL);
	}
}

int
auich_set_params(void *v, int setmode, int usemode, struct audio_params *play,
    struct audio_params *rec)
{
	struct auich_softc *sc = v;
	struct audio_params *p;
	int mode;
	u_int16_t val, rate, inout;

	for (mode = AUMODE_RECORD; mode != -1;
	     mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = mode == AUMODE_PLAY ? play : rec;
		if (p == NULL)
			continue;

		inout = mode == AUMODE_PLAY ? ICH_PM_PCMO : ICH_PM_PCMI;

		/*
		 * XXX NEED TO DETERMINE WHICH RATES THE CODEC SUPPORTS!
		 */
		if (p->sample_rate != 48000)
			return (EINVAL);

		p->factor = 1;
		p->sw_code = NULL;
		switch (p->encoding) {
		case AUDIO_ENCODING_SLINEAR_BE:
			if (p->precision == 16)
				p->sw_code = swap_bytes;
			else {
				if (mode == AUMODE_PLAY)
					p->sw_code = linear8_to_linear16_le;
				else
					p->sw_code = linear16_to_linear8_le;
			}
			break;

		case AUDIO_ENCODING_SLINEAR_LE:
			if (p->precision != 16) {
				if (mode == AUMODE_PLAY)
					p->sw_code = linear8_to_linear16_le;
				else
					p->sw_code = linear16_to_linear8_le;
			}
			break;

		case AUDIO_ENCODING_ULINEAR_BE:
			if (p->precision == 16) {
				if (mode == AUMODE_PLAY)
					p->sw_code =
					    swap_bytes_change_sign16_le;
				else
					p->sw_code =
					    change_sign16_swap_bytes_le;
			} else {
				/*
				 * XXX ulinear8_to_slinear16_le
				 */
				return (EINVAL);
			}
			break;

		case AUDIO_ENCODING_ULINEAR_LE:
			if (p->precision == 16)
				p->sw_code = change_sign16_le;
			else {
				/*
				 * XXX ulinear8_to_slinear16_le
				 */
				return (EINVAL);
			}
			break;

		case AUDIO_ENCODING_ULAW:
			if (mode == AUMODE_PLAY) {
				p->factor = 2;
				p->sw_code = mulaw_to_slinear16_le;
			} else {
				/*
				 * XXX slinear16_le_to_mulaw
				 */
				return (EINVAL);
			}
			break;

		case AUDIO_ENCODING_ALAW:
			if (mode == AUMODE_PLAY) {
				p->factor = 2;
				p->sw_code = alaw_to_slinear16_le;
			} else {
				/*
				 * XXX slinear16_le_to_alaw
				 */
				return (EINVAL);
			}
			break;

		default:
			return (EINVAL);
		}

		auich_read_codec(sc, AC97_REG_POWER, &val);
		auich_write_codec(sc, AC97_REG_POWER, val | inout);

		auich_write_codec(sc, AC97_REG_PCM_FRONT_DAC_RATE,
		    p->sample_rate);
		auich_read_codec(sc, AC97_REG_PCM_FRONT_DAC_RATE, &rate);
		p->sample_rate = rate;

		auich_write_codec(sc, AC97_REG_POWER, val);
	}

	return (0);
}

int
auich_round_blocksize(void *v, int blk)
{

	return (blk & ~0x3f);		/* keep good alignment */
}

int
auich_halt_output(void *v)
{
	struct auich_softc *sc = v;

	DPRINTF(ICH_DEBUG_DMA, ("%s: halt_output\n", sc->sc_dev.dv_xname));

	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMO + ICH_CTRL, ICH_RR);

	return (0);
}

int
auich_halt_input(void *v)
{
	struct auich_softc *sc = v;

	DPRINTF(ICH_DEBUG_DMA,
	    ("%s: halt_input\n", sc->sc_dev.dv_xname));

	/* XXX halt both unless known otherwise */

	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_CTRL, ICH_RR);
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_MICI + ICH_CTRL, ICH_RR);

	return (0);
}

int
auich_getdev(void *v, struct audio_device *adp)
{
	struct auich_softc *sc = v;

	*adp = sc->sc_audev;
	return (0);
}

int
auich_set_port(void *v, mixer_ctrl_t *cp)
{
	struct auich_softc *sc = v;

	return (sc->codec_if->vtbl->mixer_set_port(sc->codec_if, cp));
}

int
auich_get_port(void *v, mixer_ctrl_t *cp)
{
	struct auich_softc *sc = v;

	return (sc->codec_if->vtbl->mixer_get_port(sc->codec_if, cp));
}

int
auich_query_devinfo(void *v, mixer_devinfo_t *dp)
{
	struct auich_softc *sc = v;

	return (sc->codec_if->vtbl->query_devinfo(sc->codec_if, dp));
}

void *
auich_allocm(void *v, int direction, size_t size, int pool, int flags)
{
	struct auich_softc *sc = v;
	struct auich_dma *p;
	int error;

	if (size > (ICH_DMALIST_MAX * ICH_DMASEG_MAX))
		return (NULL);

	p = malloc(sizeof(*p), pool, flags);
	if (p == NULL)
		return (NULL);
	memset(p, 0, sizeof(*p));

	error = auich_allocmem(sc, size, 0, p);
	if (error) {
		free(p, pool);
		return (NULL);
	}

	p->next = sc->sc_dmas;
	sc->sc_dmas = p;

	return (KERNADDR(p));
}

void
auich_freem(void *v, void *ptr, int pool)
{
	struct auich_softc *sc = v;
	struct auich_dma *p, **pp;

	for (pp = &sc->sc_dmas; (p = *pp) != NULL; pp = &p->next) {
		if (KERNADDR(p) == ptr) {
			auich_freemem(sc, p);
			*pp = p->next;
			free(p, pool);
			return;
		}
	}
}

size_t
auich_round_buffersize(void *v, int direction, size_t size)
{

	if (size > (ICH_DMALIST_MAX * ICH_DMASEG_MAX))
		size = ICH_DMALIST_MAX * ICH_DMASEG_MAX;

	return size;
}

paddr_t
auich_mappage(void *v, void *mem, off_t off, int prot)
{
	struct auich_softc *sc = v;
	struct auich_dma *p;

	if (off < 0)
		return (-1);

	for (p = sc->sc_dmas; p && KERNADDR(p) != mem; p = p->next)
		;
	if (!p)
		return (-1);
	return (bus_dmamem_mmap(sc->dmat, p->segs, p->nsegs,
	    off, prot, BUS_DMA_WAITOK));
}

int
auich_get_props(void *v)
{

	return (AUDIO_PROP_MMAP | AUDIO_PROP_INDEPENDENT |
		AUDIO_PROP_FULLDUPLEX);
}

int
auich_intr(void *v)
{
	struct auich_softc *sc = v;
	int ret = 0, sts, gsts, i, qptr;

	gsts = bus_space_read_2(sc->iot, sc->aud_ioh, ICH_GSTS);
	DPRINTF(ICH_DEBUG_DMA, ("auich_intr: gsts=0x%x\n", gsts));

	if (gsts & ICH_POINT) {
		sts = bus_space_read_2(sc->iot, sc->aud_ioh, ICH_PCMO+ICH_STS);
		DPRINTF(ICH_DEBUG_DMA,
		    ("auich_intr: osts=0x%x\n", sts));

		if (sts & ICH_FIFOE) {
			printf("%s: fifo underrun # %u\n",
			    sc->sc_dev.dv_xname, ++sc->pcmo_fifoe);
		}

		i = bus_space_read_1(sc->iot, sc->aud_ioh, ICH_PCMO + ICH_CIV);
		if (sts & (ICH_LVBCI | ICH_CELV)) {
			struct auich_dmalist *q;

			qptr = sc->ptr_pcmo;

			while (qptr != i) {
				q = &sc->dmalist_pcmo[qptr];

				q->base = sc->pcmo_p;
				q->len = (sc->pcmo_blksize / 2) | ICH_DMAF_IOC;
				DPRINTF(ICH_DEBUG_DMA,
				    ("auich_intr: %p, %p = %x @ 0x%x\n",
				    &sc->dmalist_pcmo[i], q,
				    sc->pcmo_blksize / 2, sc->pcmo_p));

				sc->pcmo_p += sc->pcmo_blksize;
				if (sc->pcmo_p >= sc->pcmo_end)
					sc->pcmo_p = sc->pcmo_start;

				if (++qptr == ICH_DMALIST_MAX)
					qptr = 0;
			}

			sc->ptr_pcmo = qptr;
			bus_space_write_1(sc->iot, sc->aud_ioh,
			    ICH_PCMO + ICH_LVI,
			    (sc->ptr_pcmo - 1) & ICH_LVI_MASK);
		}

		if (sts & ICH_BCIS && sc->sc_pintr)
			sc->sc_pintr(sc->sc_parg);

		/* int ack */
		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_PCMO + ICH_STS,
		    sts & (ICH_LVBCI | ICH_CELV | ICH_BCIS | ICH_FIFOE));
		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_GSTS, ICH_POINT);
		ret++;
	}

	if (gsts & ICH_PIINT) {
		sts = bus_space_read_2(sc->iot, sc->aud_ioh, ICH_PCMI+ICH_STS);
		DPRINTF(ICH_DEBUG_DMA,
		    ("auich_intr: ists=0x%x\n", sts));

		if (sts & ICH_FIFOE) {
			printf("%s: fifo overrun # %u\n",
			    sc->sc_dev.dv_xname, ++sc->pcmi_fifoe);
		}

		i = bus_space_read_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_CIV);
		if (sts & (ICH_LVBCI | ICH_CELV)) {
			struct auich_dmalist *q;

			qptr = sc->ptr_pcmi;

			while (qptr != i) {
				q = &sc->dmalist_pcmi[qptr];

				q->base = sc->pcmi_p;
				q->len = (sc->pcmi_blksize / 2) | ICH_DMAF_IOC;
				DPRINTF(ICH_DEBUG_DMA,
				    ("auich_intr: %p, %p = %x @ 0x%x\n",
				    &sc->dmalist_pcmi[i], q,
				    sc->pcmi_blksize / 2, sc->pcmi_p));

				sc->pcmi_p += sc->pcmi_blksize;
				if (sc->pcmi_p >= sc->pcmi_end)
					sc->pcmi_p = sc->pcmi_start;

				if (++qptr == ICH_DMALIST_MAX)
					qptr = 0;
			}

			sc->ptr_pcmi = qptr;
			bus_space_write_1(sc->iot, sc->aud_ioh,
			    ICH_PCMI + ICH_LVI,
			    (sc->ptr_pcmi - 1) & ICH_LVI_MASK);
		}

		if (sts & ICH_BCIS && sc->sc_rintr)
			sc->sc_rintr(sc->sc_rarg);

		/* int ack */
		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_STS,
		    sts & (ICH_LVBCI | ICH_CELV | ICH_BCIS | ICH_FIFOE));
		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_GSTS, ICH_POINT);
		ret++;
	}

	if (gsts & ICH_MIINT) {
		sts = bus_space_read_2(sc->iot, sc->aud_ioh, ICH_MICI+ICH_STS);
		DPRINTF(ICH_DEBUG_DMA,
		    ("auich_intr: ists=0x%x\n", sts));
		if (sts & ICH_FIFOE)
			printf("%s: fifo overrun\n", sc->sc_dev.dv_xname);

		/* TODO mic input dma */

		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_GSTS, ICH_MIINT);
	}

	return ret;
}

int
auich_trigger_output(void *v, void *start, void *end, int blksize,
    void (*intr)(void *), void *arg, struct audio_params *param)
{
	struct auich_softc *sc = v;
	struct auich_dmalist *q;
	struct auich_dma *p;
	size_t size;

	DPRINTF(ICH_DEBUG_DMA,
	    ("auich_trigger_output(%p, %p, %d, %p, %p, %p)\n",
	    start, end, blksize, intr, arg, param));

	sc->sc_pintr = intr;
	sc->sc_parg = arg;

	for (p = sc->sc_dmas; p && KERNADDR(p) != start; p = p->next)
		;
	if (!p) {
		printf("auich_trigger_output: bad addr %p\n", start);
		return (EINVAL);
	}

	size = (size_t)((caddr_t)end - (caddr_t)start);

	/*
	 * The logic behind this is:
	 * setup one buffer to play, then LVI dump out the rest
	 * to the scatter-gather chain.
	 */
	sc->pcmo_start = DMAADDR(p);
	sc->pcmo_p = sc->pcmo_start + blksize;
	sc->pcmo_end = sc->pcmo_start + size;
	sc->pcmo_blksize = blksize;

	sc->ptr_pcmo = 0;
	q = &sc->dmalist_pcmo[sc->ptr_pcmo];
	q->base = sc->pcmo_start;
	q->len = (blksize / 2) | ICH_DMAF_IOC;
	if (++sc->ptr_pcmo == ICH_DMALIST_MAX)
		sc->ptr_pcmo = 0;

	bus_space_write_4(sc->iot, sc->aud_ioh, ICH_PCMO + ICH_BDBAR,
	    sc->sc_cddma + ICH_PCMO_OFF(0));
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMO + ICH_CTRL,
	    ICH_IOCE | ICH_FEIE | ICH_LVBIE | ICH_RPBM);
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMO + ICH_LVI,
	    (sc->ptr_pcmo - 1) & ICH_LVI_MASK);

	return (0);
}

int
auich_trigger_input(v, start, end, blksize, intr, arg, param)
	void *v;
	void *start, *end;
	int blksize;
	void (*intr)(void *);
	void *arg;
	struct audio_params *param;
{
	struct auich_softc *sc = v;
	struct auich_dmalist *q;
	struct auich_dma *p;
	size_t size;

	DPRINTF(ICH_DEBUG_DMA,
	    ("auich_trigger_input(%p, %p, %d, %p, %p, %p)\n",
	    start, end, blksize, intr, arg, param));

	sc->sc_rintr = intr;
	sc->sc_rarg = arg;

	for (p = sc->sc_dmas; p && KERNADDR(p) != start; p = p->next)
		;
	if (!p) {
		printf("auich_trigger_input: bad addr %p\n", start);
		return (EINVAL);
	}

	size = (size_t)((caddr_t)end - (caddr_t)start);

	/*
	 * The logic behind this is:
	 * setup one buffer to play, then LVI dump out the rest
	 * to the scatter-gather chain.
	 */
	sc->pcmi_start = DMAADDR(p);
	sc->pcmi_p = sc->pcmi_start + blksize;
	sc->pcmi_end = sc->pcmi_start + size;
	sc->pcmi_blksize = blksize;

	sc->ptr_pcmi = 0;
	q = &sc->dmalist_pcmi[sc->ptr_pcmi];
	q->base = sc->pcmi_start;
	q->len = (blksize / 2) | ICH_DMAF_IOC;
	if (++sc->ptr_pcmi == ICH_DMALIST_MAX)
		sc->ptr_pcmi = 0;

	bus_space_write_4(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_BDBAR,
	    sc->sc_cddma + ICH_PCMI_OFF(0));
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_CTRL,
	    ICH_IOCE | ICH_FEIE | ICH_LVBIE | ICH_RPBM);
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_LVI,
	    (sc->ptr_pcmi - 1) & ICH_LVI_MASK);

	return (0);
}

int
auich_allocmem(struct auich_softc *sc, size_t size, size_t align,
    struct auich_dma *p)
{
	int error;

	p->size = size;
	error = bus_dmamem_alloc(sc->dmat, p->size, align, 0,
				 p->segs, sizeof(p->segs)/sizeof(p->segs[0]),
				 &p->nsegs, BUS_DMA_NOWAIT);
	if (error)
		return (error);

	error = bus_dmamem_map(sc->dmat, p->segs, p->nsegs, p->size,
			       &p->addr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error)
		goto free;

	error = bus_dmamap_create(sc->dmat, p->size, 1, p->size,
				  0, BUS_DMA_NOWAIT, &p->map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(sc->dmat, p->map, p->addr, p->size, NULL,
				BUS_DMA_NOWAIT);
	if (error)
		goto destroy;
	return (0);

 destroy:
	bus_dmamap_destroy(sc->dmat, p->map);
 unmap:
	bus_dmamem_unmap(sc->dmat, p->addr, p->size);
 free:
	bus_dmamem_free(sc->dmat, p->segs, p->nsegs);
	return (error);
}

int
auich_freemem(struct auich_softc *sc, struct auich_dma *p)
{

	bus_dmamap_unload(sc->dmat, p->map);
	bus_dmamap_destroy(sc->dmat, p->map);
	bus_dmamem_unmap(sc->dmat, p->addr, p->size);
	bus_dmamem_free(sc->dmat, p->segs, p->nsegs);
	return (0);
}

int
auich_alloc_cdata(struct auich_softc *sc)
{
	bus_dma_segment_t seg;
	int error, rseg;

	/*
	 * Allocate the control data structure, and create and load the
	 * DMA map for it.
	 */
	if ((error = bus_dmamem_alloc(sc->dmat,
				      sizeof(struct auich_cdata),
				      PAGE_SIZE, 0, &seg, 1, &rseg, 0)) != 0) {
		printf("%s: unable to allocate control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_0;
	}

	if ((error = bus_dmamem_map(sc->dmat, &seg, rseg,
				    sizeof(struct auich_cdata),
				    (caddr_t *) &sc->sc_cdata,
				    BUS_DMA_COHERENT)) != 0) {
		printf("%s: unable to map control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_1;
	}

	if ((error = bus_dmamap_create(sc->dmat, sizeof(struct auich_cdata), 1,
				       sizeof(struct auich_cdata), 0, 0,
				       &sc->sc_cddmamap)) != 0) {
		printf("%s: unable to create control data DMA map, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->dmat, sc->sc_cddmamap,
				     sc->sc_cdata, sizeof(struct auich_cdata),
				     NULL, 0)) != 0) {
		printf("%s: unable tp load control data DMA map, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		goto fail_3;
	}

	return (0);

 fail_3:
	bus_dmamap_destroy(sc->dmat, sc->sc_cddmamap);
 fail_2:
	bus_dmamem_unmap(sc->dmat, (caddr_t) sc->sc_cdata,
	    sizeof(struct auich_cdata));
 fail_1:
	bus_dmamem_free(sc->dmat, &seg, rseg);
 fail_0:
	return (error);
}
