/*	$NetBSD: auich.c,v 1.36 2003/02/01 06:23:38 thorpej Exp $	*/

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

/*
 * Copyright (c) 2000 Katsurajima Naoto <raven@katsurajima.seya.yokohama.jp>
 * Copyright (c) 2001 Cameron Grant <cg@freebsd.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHERIN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THEPOSSIBILITY OF
 * SUCH DAMAGE.
 *
 * auich_calibrate() was from FreeBSD: ich.c,v 1.22 2002/06/27 22:36:01 scottl Exp 
 */


/* #define	ICH_DEBUG */
/*
 * AC'97 audio found on Intel 810/820/440MX chipsets.
 *	http://developer.intel.com/design/chipsets/datashts/290655.htm
 *	http://developer.intel.com/design/chipsets/manuals/298028.htm
 * ICH3:http://www.intel.com/design/chipsets/datashts/290716.htm
 * ICH4:http://www.intel.com/design/chipsets/datashts/290744.htm
 *
 * TODO:
 *	- Add support for the dedicated microphone input.
 *	- 4ch/6ch support.
 *
 * NOTE:
 *      - The 440MX B-stepping at running 100MHz has a hardware erratum.
 *        It causes PCI master abort and hangups until cold reboot.
 *        http://www.intel.com/design/chipsets/specupdt/245051.htm
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auich.c,v 1.36 2003/02/01 06:23:38 thorpej Exp $");

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

#ifdef DIAGNOSTIC
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pt;
#endif
	int sc_ignore_codecready;
	/* SiS 7012 hack */
	int  sc_sample_size;
	int  sc_sts_reg;
	/* 440MX workaround */
	int  sc_dmamap_flags;

	void (*sc_pintr)(void *);
	void *sc_parg;

	void (*sc_rintr)(void *);
	void *sc_rarg;

	/* Power Management */
	void *sc_powerhook;
	int sc_suspend;
	u_int16_t ext_status;
};

#define IS_FIXED_RATE(codec)	!((codec)->vtbl->get_extcaps(codec) \
				  & AC97_EXT_AUDIO_VRA)

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

CFATTACH_DECL(auich, sizeof(struct auich_softc),
    auich_match, auich_attach, NULL, NULL);

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
void	*auich_allocm(void *, int, size_t, struct malloc_type *, int);
void	auich_freem(void *, void *, struct malloc_type *);
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

void	auich_powerhook(int, void *);
int	auich_set_rate(struct auich_softc *, int, u_long);
void	auich_calibrate(struct device *);


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
	int	vendor;
	int	product;
	const char *name;
	const char *shortname;
} auich_devices[] = {
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801AA_ACA,
	    "i82801AA (ICH) AC-97 Audio",	"ICH" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801AB_ACA,
	    "i82801AB (ICH0) AC-97 Audio",	"ICH0" }, /* i810-L */
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801BA_ACA,
	    "i82801BA (ICH2) AC-97 Audio",	"ICH2" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82440MX_ACA,
	    "i82440MX AC-97 Audio",		"440MX" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801CA_AC,
	    "i82801CA (ICH3) AC-97 Audio",	"ICH3" }, /* i830Mx i845MP/MZ*/
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801DB_AC,
	    "i82801DB (ICH4) AC-97 Audio",	"ICH4" }, /* i845E i845Gx */
	{ PCI_VENDOR_SIS, PCI_PRODUCT_SIS_7012_AC,
	    "SiS 7012 AC-97 Audio",		"SiS7012" },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_NFORCE_MCP_AC,
	    "nForce MCP AC-97 Audio",		"nForce-MCP" },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_NFORCE2_MCPT_AC,
	    "nForce2 MCP-T AC-97 Audio",	"nForce-MCP-T" },
	{ PCI_VENDOR_AMD, PCI_PRODUCT_AMD_PBC768_AC,
	    "AMD768 AC-97 Audio",		"AMD768" },
	{ PCI_VENDOR_AMD, PCI_PRODUCT_AMD_PBC8111_AC,
	    "AMD8111 AC-97 Audio",		"AMD8111" },
	{ 0,
	    NULL,				NULL },
};

static const struct auich_devtype *
auich_lookup(struct pci_attach_args *pa)
{
	const struct auich_devtype *d;

	for (d = auich_devices; d->name != NULL; d++) {
		if (PCI_VENDOR(pa->pa_id) == d->vendor
			&& PCI_PRODUCT(pa->pa_id) == d->product)
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
	u_int32_t status;

	aprint_naive(": Audio controller\n");

	d = auich_lookup(pa);
	if (d == NULL)
		panic("auich_attach: impossible");

#ifdef DIAGNOSTIC
	sc->sc_pc = pa->pa_pc;
	sc->sc_pt = pa->pa_tag;
#endif

	aprint_normal(": %s\n", d->name);

	if (pci_mapreg_map(pa, ICH_NAMBAR, PCI_MAPREG_TYPE_IO, 0,
			   &sc->iot, &sc->mix_ioh, NULL, &mix_size)) {
		aprint_error("%s: can't map codec i/o space\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (pci_mapreg_map(pa, ICH_NABMBAR, PCI_MAPREG_TYPE_IO, 0,
			   &sc->iot, &sc->aud_ioh, NULL, &aud_size)) {
		aprint_error("%s: can't map device i/o space\n",
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
		aprint_error("%s: can't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_AUDIO,
	    auich_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("%s: can't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	sprintf(sc->sc_audev.name, "%s AC97", d->shortname);
	sprintf(sc->sc_audev.version, "0x%02x", PCI_REVISION(pa->pa_class));
	strcpy(sc->sc_audev.config, sc->sc_dev.dv_xname);

	/* SiS 7012 needs special handling */
	if (d->vendor == PCI_VENDOR_SIS
	    && d->product == PCI_PRODUCT_SIS_7012_AC) {
		sc->sc_sts_reg = ICH_PICB;
		sc->sc_sample_size = 1;
	} else {
		sc->sc_sts_reg = ICH_STS;
		sc->sc_sample_size = 2;
	}
	/* nForce MCP quirk */
	if (d->vendor == PCI_VENDOR_NVIDIA
	    && d->product == PCI_PRODUCT_NVIDIA_NFORCE_MCP_AC) {
		sc->sc_ignore_codecready = TRUE;
	}
	/* Workaround for a 440MX B-stepping erratum */
	sc->sc_dmamap_flags = BUS_DMA_COHERENT;
	if (d->vendor == PCI_VENDOR_INTEL
	    && d->product == PCI_PRODUCT_INTEL_82440MX_ACA) {
		sc->sc_dmamap_flags |= BUS_DMA_NOCACHE;
		printf("%s: DMA bug workaround enabled\n", sc->sc_dev.dv_xname);
	}

	/* Set up DMA lists. */
	sc->ptr_pcmo = sc->ptr_pcmi = sc->ptr_mici = 0;
	auich_alloc_cdata(sc);

	DPRINTF(ICH_DEBUG_DMA, ("auich_attach: lists %p %p %p\n",
	    sc->dmalist_pcmo, sc->dmalist_pcmi, sc->dmalist_mici));

	/* Reset codec and AC'97 */
	auich_reset_codec(sc);
	status = bus_space_read_4(sc->iot, sc->aud_ioh, ICH_GSTS);
	if (!(status & ICH_PCR)) { /* reset failure */
		if (d->vendor == PCI_VENDOR_INTEL
		    && d->product == PCI_PRODUCT_INTEL_82801DB_AC) {
			/* MSI 845G Max never return ICH_PCR */
			sc->sc_ignore_codecready = TRUE;
		} else {
			return;
		}
	}
	/* Print capabilities though there are no supports for now */
	if ((status & ICH_SAMPLE_CAP) == ICH_POM20)
		aprint_normal("%s: 20 bit precision support\n",
		    sc->sc_dev.dv_xname);
	if ((status & ICH_CHAN_CAP) == ICH_PCM4)
		aprint_normal("%s: 4ch PCM output support\n",
		    sc->sc_dev.dv_xname);
	if ((status & ICH_CHAN_CAP) == ICH_PCM6)
		aprint_normal("%s: 6ch PCM output support\n",
		    sc->sc_dev.dv_xname);

	sc->host_if.arg = sc;
	sc->host_if.attach = auich_attach_codec;
	sc->host_if.read = auich_read_codec;
	sc->host_if.write = auich_write_codec;
	sc->host_if.reset = auich_reset_codec;

	if (ac97_attach(&sc->host_if) != 0)
		return;

	audio_attach_mi(&auich_hw_if, sc, &sc->sc_dev);

	/* Watch for power change */
	sc->sc_suspend = PWR_RESUME;
	sc->sc_powerhook = powerhook_establish(auich_powerhook, sc);

	if (!IS_FIXED_RATE(sc->codec_if)) {
		config_interrupts(self, auich_calibrate);
	}
}

#define ICH_CODECIO_INTERVAL	10
int
auich_read_codec(void *v, u_int8_t reg, u_int16_t *val)
{
	struct auich_softc *sc = v;
	int i;
	uint32_t status;

	status = bus_space_read_4(sc->iot, sc->aud_ioh, ICH_GSTS);
	if (!sc->sc_ignore_codecready && !(status & ICH_PCR)) {
		printf("auich_read_codec: codec is not ready (0x%x)\n", status);
		*val = 0xffff;
		return -1;
	}
	/* wait for an access semaphore */
	for (i = ICH_SEMATIMO / ICH_CODECIO_INTERVAL; i-- &&
	    bus_space_read_1(sc->iot, sc->aud_ioh, ICH_CAS) & 1;
	    DELAY(ICH_CODECIO_INTERVAL));

	if (i > 0) {
		*val = bus_space_read_2(sc->iot, sc->mix_ioh, reg);
		DPRINTF(ICH_DEBUG_CODECIO,
		    ("auich_read_codec(%x, %x)\n", reg, *val));
		status = bus_space_read_4(sc->iot, sc->aud_ioh, ICH_GSTS);
		if (status & ICH_RCS) {
			bus_space_write_4(sc->iot, sc->aud_ioh, ICH_GSTS,
					  status & ~(ICH_SRI|ICH_PRI|ICH_GSCI));
			*val = 0xffff;
		}
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
	if (!sc->sc_ignore_codecready
	    && !(bus_space_read_4(sc->iot, sc->aud_ioh, ICH_GSTS) & ICH_PCR)) {
		printf("auich_write_codec: codec is not ready.");
		return -1;
	}
	/* wait for an access semaphore */
	for (i = ICH_SEMATIMO / ICH_CODECIO_INTERVAL; i-- &&
	    bus_space_read_1(sc->iot, sc->aud_ioh, ICH_CAS) & 1;
	    DELAY(ICH_CODECIO_INTERVAL));

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
	int i;
	uint32_t control;

	control = bus_space_read_4(sc->iot, sc->aud_ioh, ICH_GCTRL);
	control &= ~(ICH_ACLSO | ICH_PCM246_MASK);
	control |= (control & ICH_CRESET) ? ICH_WRESET : ICH_CRESET;
	bus_space_write_4(sc->iot, sc->aud_ioh, ICH_GCTRL, control);

	for (i = 500000; i-- &&
	       !(bus_space_read_4(sc->iot, sc->aud_ioh, ICH_GSTS) & ICH_PCR);
	     DELAY(1));					/*       or ICH_SCR? */
	if (i <= 0)
		printf("%s: auich_reset_codec: time out\n", sc->sc_dev.dv_xname);
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
	case 0:
		strcpy(aep->name, AudioEulinear);
		aep->encoding = AUDIO_ENCODING_ULINEAR;
		aep->precision = 8;
		aep->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
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
auich_set_rate(struct auich_softc *sc, int mode, u_long srate)
{
	int reg;
	u_long ratetmp;

	ratetmp = srate;
	reg = mode == AUMODE_PLAY
		? AC97_REG_PCM_FRONT_DAC_RATE : AC97_REG_PCM_LR_ADC_RATE;
	return sc->codec_if->vtbl->set_rate(sc->codec_if, reg, &ratetmp);
}

int
auich_set_params(void *v, int setmode, int usemode, struct audio_params *play,
    struct audio_params *rec)
{
	struct auich_softc *sc = v;
	struct audio_params *p;
	int mode;

	for (mode = AUMODE_RECORD; mode != -1;
	     mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = mode == AUMODE_PLAY ? play : rec;
		if (p == NULL)
			continue;

		if ((p->sample_rate !=  8000) &&
		    (p->sample_rate != 11025) &&
		    (p->sample_rate != 16000) &&
		    (p->sample_rate != 22050) &&
		    (p->sample_rate != 32000) &&
		    (p->sample_rate != 44100) &&
		    (p->sample_rate != 48000))
			return (EINVAL);

		p->factor = 1;
		if (p->precision == 8)
			p->factor *= 2;

		p->sw_code = NULL;
		/* setup hardware formats */
		p->hw_encoding = AUDIO_ENCODING_SLINEAR_LE;
		p->hw_precision = 16;

		/* If monaural is requested, aurateconv expands a monaural
		 * stream to stereo. */
		if (p->channels < 2)
			p->hw_channels = 2;

		switch (p->encoding) {
		case AUDIO_ENCODING_SLINEAR_BE:
			if (p->precision == 16) {
				p->sw_code = swap_bytes;
			} else {
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
				if (mode == AUMODE_PLAY)
					p->sw_code =
					    ulinear8_to_slinear16_le;
				else
					p->sw_code =
					    slinear16_to_ulinear8_le;
			}
			break;

		case AUDIO_ENCODING_ULINEAR_LE:
			if (p->precision == 16) {
				p->sw_code = change_sign16_le;
			} else {
				if (mode == AUMODE_PLAY)
					p->sw_code =
					    ulinear8_to_slinear16_le;
				else
					p->sw_code =
					    slinear16_to_ulinear8_le;
			}
			break;

		case AUDIO_ENCODING_ULAW:
			if (mode == AUMODE_PLAY) {
				p->sw_code = mulaw_to_slinear16_le;
			} else {
				p->sw_code = slinear16_to_mulaw_le;
			}
			break;

		case AUDIO_ENCODING_ALAW:
			if (mode == AUMODE_PLAY) {
				p->sw_code = alaw_to_slinear16_le;
			} else {
				p->sw_code = slinear16_to_alaw_le;
			}
			break;

		default:
			return (EINVAL);
		}

		if (IS_FIXED_RATE(sc->codec_if)) {
			p->hw_sample_rate = AC97_SINGLE_RATE;
			/* If hw_sample_rate is changed, aurateconv works. */
		} else {
			if (auich_set_rate(sc, mode, p->sample_rate))
				return EINVAL;
		}
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
auich_allocm(void *v, int direction, size_t size, struct malloc_type *pool,
    int flags)
{
	struct auich_softc *sc = v;
	struct auich_dma *p;
	int error;

	if (size > (ICH_DMALIST_MAX * ICH_DMASEG_MAX))
		return (NULL);

	p = malloc(sizeof(*p), pool, flags|M_ZERO);
	if (p == NULL)
		return (NULL);

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
auich_freem(void *v, void *ptr, struct malloc_type *pool)
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
	struct auich_softc *sc = v;
	int props;

	props = AUDIO_PROP_INDEPENDENT | AUDIO_PROP_FULLDUPLEX;
	/*
	 * Even if the codec is fixed-rate, set_param() succeeds for any sample
	 * rate because of aurateconv.  Applications can't know what rate the
	 * device can process in the case of mmap().
	 */
	if (!IS_FIXED_RATE(sc->codec_if))
		props |= AUDIO_PROP_MMAP;
	return props;
}

int
auich_intr(void *v)
{
	struct auich_softc *sc = v;
	int ret = 0, sts, gsts, i, qptr;

#ifdef DIAGNOSTIC
	int csts;
#endif

#ifdef DIAGNOSTIC
	csts = pci_conf_read(sc->sc_pc, sc->sc_pt, PCI_COMMAND_STATUS_REG);
	if (csts & PCI_STATUS_MASTER_ABORT) {
		printf("auich_intr: PCI master abort\n");
	}
#endif

	gsts = bus_space_read_2(sc->iot, sc->aud_ioh, ICH_GSTS);
	DPRINTF(ICH_DEBUG_DMA, ("auich_intr: gsts=0x%x\n", gsts));

	if (gsts & ICH_POINT) {
		sts = bus_space_read_2(sc->iot, sc->aud_ioh, ICH_PCMO+sc->sc_sts_reg);
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
				q->len = (sc->pcmo_blksize / sc->sc_sample_size) | ICH_DMAF_IOC;
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
		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_PCMO + sc->sc_sts_reg,
		    sts & (ICH_LVBCI | ICH_CELV | ICH_BCIS | ICH_FIFOE));
		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_GSTS, ICH_POINT);
		ret++;
	}

	if (gsts & ICH_PIINT) {
		sts = bus_space_read_2(sc->iot, sc->aud_ioh, ICH_PCMI+sc->sc_sts_reg);
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
				q->len = (sc->pcmi_blksize / sc->sc_sample_size) | ICH_DMAF_IOC;
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
		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_PCMI + sc->sc_sts_reg,
		    sts & (ICH_LVBCI | ICH_CELV | ICH_BCIS | ICH_FIFOE));
		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_GSTS, ICH_POINT);
		ret++;
	}

	if (gsts & ICH_MIINT) {
		sts = bus_space_read_2(sc->iot, sc->aud_ioh, ICH_MICI+sc->sc_sts_reg);
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
#ifdef DIAGNOSTIC
	int csts;
#endif

	DPRINTF(ICH_DEBUG_DMA,
	    ("auich_trigger_output(%p, %p, %d, %p, %p, %p)\n",
	    start, end, blksize, intr, arg, param));

	sc->sc_pintr = intr;
	sc->sc_parg = arg;
#ifdef DIAGNOSTIC
	csts = pci_conf_read(sc->sc_pc, sc->sc_pt, PCI_COMMAND_STATUS_REG);
	if (csts & PCI_STATUS_MASTER_ABORT) {
		printf("auich_trigger_output: PCI master abort\n");
	}
#endif

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
	q->len = (blksize / sc->sc_sample_size) | ICH_DMAF_IOC;
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
#ifdef DIAGNOSTIC
	int csts;
#endif

	DPRINTF(ICH_DEBUG_DMA,
	    ("auich_trigger_input(%p, %p, %d, %p, %p, %p)\n",
	    start, end, blksize, intr, arg, param));

	sc->sc_rintr = intr;
	sc->sc_rarg = arg;

#ifdef DIAGNOSTIC
	csts = pci_conf_read(sc->sc_pc, sc->sc_pt, PCI_COMMAND_STATUS_REG);
	if (csts & PCI_STATUS_MASTER_ABORT) {
		printf("auich_trigger_input: PCI master abort\n");
	}
#endif

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
	q->len = (blksize / sc->sc_sample_size) | ICH_DMAF_IOC;
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
			       &p->addr, BUS_DMA_NOWAIT|sc->sc_dmamap_flags);
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
				    sc->sc_dmamap_flags)) != 0) {
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

void
auich_powerhook(int why, void *addr)
{
	struct auich_softc *sc = (struct auich_softc *)addr;

	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		/* Power down */
		DPRINTF(1, ("%s: power down\n", sc->sc_dev.dv_xname));
		sc->sc_suspend = why;
		auich_read_codec(sc, AC97_REG_EXT_AUDIO_CTRL, &sc->ext_status);
		break;

	case PWR_RESUME:
		/* Wake up */
		DPRINTF(1, ("%s: power resume\n", sc->sc_dev.dv_xname));
		if (sc->sc_suspend == PWR_RESUME) {
			printf("%s: resume without suspend.\n",
			    sc->sc_dev.dv_xname);
			sc->sc_suspend = why;
			return;
		}
		sc->sc_suspend = why;
		auich_reset_codec(sc);
		DELAY(1000);
		(sc->codec_if->vtbl->restore_ports)(sc->codec_if);
		auich_write_codec(sc, AC97_REG_EXT_AUDIO_CTRL, sc->ext_status);
		break;

	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
}


/* -------------------------------------------------------------------- */
/* Calibrate card (some boards are overclocked and need scaling) */

void
auich_calibrate(struct device *self)
{
	struct auich_softc *sc;
	struct timeval t1, t2;
	u_int8_t ociv, nciv;
	u_int32_t wait_us, actual_48k_rate, bytes, ac97rate;
	void *temp_buffer;
	struct auich_dma *p;

	sc = (struct auich_softc*)self;
	/*
	 * Grab audio from input for fixed interval and compare how
	 * much we actually get with what we expect.  Interval needs
	 * to be sufficiently short that no interrupts are
	 * generated.
	 */

	/* Setup a buffer */
	bytes = 16000;
	temp_buffer = auich_allocm(sc, AUMODE_RECORD, bytes, M_DEVBUF, M_WAITOK);
	for (p = sc->sc_dmas; p && KERNADDR(p) != temp_buffer; p = p->next)
		;
	if (p == NULL) {
		printf("auich_calibrate: bad address %p\n", temp_buffer);
		return;
	}
	sc->dmalist_pcmi[0].base = DMAADDR(p);
	sc->dmalist_pcmi[0].len = (bytes / sc->sc_sample_size) | ICH_DMAF_IOC;

	/*
	 * our data format is stereo, 16 bit so each sample is 4 bytes.
	 * assuming we get 48000 samples per second, we get 192000 bytes/sec.
	 * we're going to start recording with interrupts disabled and measure
	 * the time taken for one block to complete.  we know the block size,
	 * we know the time in microseconds, we calculate the sample rate:
	 *
	 * actual_rate [bps] = bytes / (time [s] * 4)
	 * actual_rate [bps] = (bytes * 1000000) / (time [us] * 4)
	 * actual_rate [Hz] = (bytes * 250000) / time [us]
	 */

	/* prepare */
	ociv = bus_space_read_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_CIV);
	nciv = ociv;
	bus_space_write_4(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_BDBAR,
			  sc->sc_cddma + ICH_PCMI_OFF(0));
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_LVI,
			  (0 - 1) & ICH_LVI_MASK);

	/* start */
	microtime(&t1);
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_CTRL, ICH_RPBM);

	/* wait */
	while (nciv == ociv) {
		microtime(&t2);
		if (t2.tv_sec - t1.tv_sec > 1)
			break;
		nciv = bus_space_read_1(sc->iot, sc->aud_ioh,
					ICH_PCMI + ICH_CIV);
	}
	microtime(&t2);

	/* stop */
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_CTRL, 0);

	/* reset */
	DELAY(100);
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_CTRL, ICH_RR);

	/* turn time delta into us */
	wait_us = ((t2.tv_sec - t1.tv_sec) * 1000000) + t2.tv_usec - t1.tv_usec;

	auich_freem(sc, temp_buffer, M_DEVBUF);

	if (nciv == ociv) {
		printf("%s: ac97 link rate calibration timed out after %d us\n",
		       sc->sc_dev.dv_xname, wait_us);
		return;
	}

	actual_48k_rate = (bytes * 250000U) / wait_us;

	if (actual_48k_rate <= 48500)
		ac97rate = 48000;
	else
		ac97rate = actual_48k_rate;

	printf("%s: measured ac97 link rate at %d Hz",
	       sc->sc_dev.dv_xname, actual_48k_rate);
	if (ac97rate != actual_48k_rate)
		printf(", will use %d Hz", ac97rate);
	printf("\n");

	sc->codec_if->vtbl->set_clock(sc->codec_if, ac97rate);
}
