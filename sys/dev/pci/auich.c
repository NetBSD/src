/*	$NetBSD: auich.c,v 1.83 2004/12/11 17:48:56 cube Exp $	*/

/*-
 * Copyright (c) 2000, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and by Charles M. Hannum.
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


/* #define	AUICH_DEBUG */
/*
 * AC'97 audio found on Intel 810/820/440MX chipsets.
 *	http://developer.intel.com/design/chipsets/datashts/290655.htm
 *	http://developer.intel.com/design/chipsets/manuals/298028.htm
 * ICH3:http://www.intel.com/design/chipsets/datashts/290716.htm
 * ICH4:http://www.intel.com/design/chipsets/datashts/290744.htm
 * ICH5:http://www.intel.com/design/chipsets/datashts/252516.htm
 * AMD8111:
 *	http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/24674.pdf
 *	http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/25720.pdf
 *
 * TODO:
 *	- Add support for the dedicated microphone input.
 *
 * NOTE:
 *      - The 440MX B-stepping at running 100MHz has a hardware erratum.
 *        It causes PCI master abort and hangups until cold reboot.
 *        http://www.intel.com/design/chipsets/specupdt/245051.htm
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auich.c,v 1.83 2004/12/11 17:48:56 cube Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include <sys/sysctl.h>

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

	struct device *sc_audiodev;
	audio_device_t sc_audev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pt;
	bus_space_tag_t iot;
	bus_space_handle_t mix_ioh;
	bus_size_t mix_size;
	bus_space_handle_t aud_ioh;
	bus_size_t aud_size;
	bus_dma_tag_t dmat;

	struct ac97_codec_if *codec_if;
	struct ac97_host_if host_if;

	/* DMA scatter-gather lists. */
	bus_dmamap_t sc_cddmamap;
#define	sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	struct auich_cdata *sc_cdata;

	struct auich_ring {
		int qptr;
		struct auich_dmalist *dmalist;

		u_int32_t start, p, end;
		int blksize;

		void (*intr)(void *);
		void *arg;
	} pcmo, pcmi, mici;

	struct auich_dma *sc_dmas;

	/* SiS 7012 hack */
	int  sc_sample_shift;
	int  sc_sts_reg;
	/* 440MX workaround */
	int  sc_dmamap_flags;

	/* Power Management */
	void *sc_powerhook;
	int sc_suspend;

	/* sysctl */
	struct sysctllog *sc_log;
	uint32_t sc_ac97_clock;
	int sc_ac97_clock_mib;

#define AUICH_NFORMATS	3
	struct audio_format sc_formats[AUICH_NFORMATS];
	struct audio_encoding_set *sc_encodings;
};

/* Debug */
#ifdef AUICH_DEBUG
#define	DPRINTF(l,x)	do { if (auich_debug & (l)) printf x; } while(0)
int auich_debug = 0xfffe;
#define	ICH_DEBUG_CODECIO	0x0001
#define	ICH_DEBUG_DMA		0x0002
#define	ICH_DEBUG_INTR		0x0004
#else
#define	DPRINTF(x,y)	/* nothing */
#endif

static int	auich_match(struct device *, struct cfdata *, void *);
static void	auich_attach(struct device *, struct device *, void *);
static int	auich_detach(struct device *, int);
static int	auich_activate(struct device *, enum devact);
static int	auich_intr(void *);

CFATTACH_DECL(auich, sizeof(struct auich_softc),
    auich_match, auich_attach, auich_detach, auich_activate);

static int	auich_open(void *, int);
static void	auich_close(void *);
static int	auich_query_encoding(void *, struct audio_encoding *);
static int	auich_set_params(void *, int, int, struct audio_params *,
		    struct audio_params *);
static int	auich_round_blocksize(void *, int);
static int	auich_halt_output(void *);
static int	auich_halt_input(void *);
static int	auich_getdev(void *, struct audio_device *);
static int	auich_set_port(void *, mixer_ctrl_t *);
static int	auich_get_port(void *, mixer_ctrl_t *);
static int	auich_query_devinfo(void *, mixer_devinfo_t *);
static void	*auich_allocm(void *, int, size_t, struct malloc_type *, int);
static void	auich_freem(void *, void *, struct malloc_type *);
static size_t	auich_round_buffersize(void *, int, size_t);
static paddr_t	auich_mappage(void *, void *, off_t, int);
static int	auich_get_props(void *);
static int	auich_trigger_output(void *, void *, void *, int,
		    void (*)(void *), void *, struct audio_params *);
static int	auich_trigger_input(void *, void *, void *, int,
		    void (*)(void *), void *, struct audio_params *);

static int	auich_alloc_cdata(struct auich_softc *);

static int	auich_allocmem(struct auich_softc *, size_t, size_t,
		    struct auich_dma *);
static int	auich_freemem(struct auich_softc *, struct auich_dma *);

static void	auich_powerhook(int, void *);
static int	auich_set_rate(struct auich_softc *, int, u_long);
static int	auich_sysctl_verify(SYSCTLFN_ARGS);
static void	auich_finish_attach(struct device *);
static void	auich_calibrate(struct auich_softc *);

static int	auich_attach_codec(void *, struct ac97_codec_if *);
static int	auich_read_codec(void *, u_int8_t, u_int16_t *);
static int	auich_write_codec(void *, u_int8_t, u_int16_t);
static int	auich_reset_codec(void *);

const struct audio_hw_if auich_hw_if = {
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

#define AUICH_FORMATS_4CH	1
#define AUICH_FORMATS_6CH	2
static const struct audio_format auich_formats[AUICH_NFORMATS] = {
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 2, AUFMT_STEREO, 0, {8000, 48000}},
	{NULL, AUMODE_PLAY, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 4, AUFMT_SURROUND4, 0, {8000, 48000}},
	{NULL, AUMODE_PLAY, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 6, AUFMT_DOLBY_5_1, 0, {8000, 48000}},
};

#define PCI_ID_CODE0(v, p)	PCI_ID_CODE(PCI_VENDOR_##v, PCI_PRODUCT_##v##_##p)
#define PCIID_ICH		PCI_ID_CODE0(INTEL, 82801AA_ACA)
#define PCIID_ICH0		PCI_ID_CODE0(INTEL, 82801AB_ACA)
#define PCIID_ICH2		PCI_ID_CODE0(INTEL, 82801BA_ACA)
#define PCIID_440MX		PCI_ID_CODE0(INTEL, 82440MX_ACA)
#define PCIID_ICH3		PCI_ID_CODE0(INTEL, 82801CA_AC)
#define PCIID_ICH4		PCI_ID_CODE0(INTEL, 82801DB_AC)
#define PCIID_ICH5		PCI_ID_CODE0(INTEL, 82801EB_AC)
#define PCIID_ICH6		PCI_ID_CODE0(INTEL, 82801FB_AC)
#define PCIID_SIS7012		PCI_ID_CODE0(SIS, 7012_AC)
#define PCIID_NFORCE		PCI_ID_CODE0(NVIDIA, NFORCE_MCP_AC)
#define PCIID_NFORCE2		PCI_ID_CODE0(NVIDIA, NFORCE2_MCPT_AC)
#define PCIID_NFORCE3		PCI_ID_CODE0(NVIDIA, NFORCE3_MCPT_AC)
#define PCIID_NFORCE3_250	PCI_ID_CODE0(NVIDIA, NFORCE3_250_MCPT_AC)
#define PCIID_AMD768		PCI_ID_CODE0(AMD, PBC768_AC)
#define PCIID_AMD8111		PCI_ID_CODE0(AMD, PBC8111_AC)

static const struct auich_devtype {
	pcireg_t	id;
	const char	*name;
	const char	*shortname;	/* must be less than 11 characters */
} auich_devices[] = {
	{ PCIID_ICH,	"i82801AA (ICH) AC-97 Audio",	"ICH" },
	{ PCIID_ICH0,	"i82801AB (ICH0) AC-97 Audio",	"ICH0" },
	{ PCIID_ICH2,	"i82801BA (ICH2) AC-97 Audio",	"ICH2" },
	{ PCIID_440MX,	"i82440MX AC-97 Audio",		"440MX" },
	{ PCIID_ICH3,	"i82801CA (ICH3) AC-97 Audio",	"ICH3" },
	{ PCIID_ICH4,	"i82801DB/DBM (ICH4/ICH4M) AC-97 Audio", "ICH4" },
	{ PCIID_ICH5,	"i82801EB (ICH5) AC-97 Audio",	"ICH5" },
	{ PCIID_ICH6,	"i82801FB (ICH6) AC-97 Audio",	"ICH6" },
	{ PCIID_SIS7012, "SiS 7012 AC-97 Audio",	"SiS7012" },
	{ PCIID_NFORCE,	"nForce MCP AC-97 Audio",	"nForce" },
	{ PCIID_NFORCE2, "nForce2 MCP-T AC-97 Audio",	"nForce2" },
	{ PCIID_NFORCE3, "nForce3 MCP-T AC-97 Audio",	"nForce3" },
	{ PCIID_NFORCE3_250, "nForce3 250 MCP-T AC-97 Audio", "nForce3" },
	{ PCIID_AMD768,	"AMD768 AC-97 Audio",		"AMD768" },
	{ PCIID_AMD8111,"AMD8111 AC-97 Audio",		"AMD8111" },
	{ 0,		NULL,				NULL },
};

static const struct auich_devtype *
auich_lookup(struct pci_attach_args *pa)
{
	const struct auich_devtype *d;

	for (d = auich_devices; d->name != NULL; d++) {
		if (pa->pa_id == d->id)
			return (d);
	}

	return (NULL);
}

static int
auich_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (auich_lookup(pa) != NULL)
		return (1);

	return (0);
}

static void
auich_attach(struct device *parent, struct device *self, void *aux)
{
	struct auich_softc *sc = (struct auich_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_intr_handle_t ih;
	pcireg_t v;
	const char *intrstr;
	const struct auich_devtype *d;
	struct sysctlnode *node;
	int err, node_mib, i;

	aprint_naive(": Audio controller\n");

	d = auich_lookup(pa);
	if (d == NULL)
		panic("auich_attach: impossible");

	sc->sc_pc = pa->pa_pc;
	sc->sc_pt = pa->pa_tag;

	aprint_normal(": %s\n", d->name);

	if (d->id == PCIID_ICH4 || d->id == PCIID_ICH5 || d->id == PCIID_ICH6) {
		/*
		 * Use native mode for ICH4/ICH5/ICH6
		 */
		if (pci_mapreg_map(pa, ICH_MMBAR, PCI_MAPREG_TYPE_MEM, 0,
				   &sc->iot, &sc->mix_ioh, NULL, &sc->mix_size)) {
			v = pci_conf_read(pa->pa_pc, pa->pa_tag, ICH_CFG);
			pci_conf_write(pa->pa_pc, pa->pa_tag, ICH_CFG,
				       v | ICH_CFG_IOSE);
			if (pci_mapreg_map(pa, ICH_NAMBAR, PCI_MAPREG_TYPE_IO,
					   0, &sc->iot, &sc->mix_ioh, NULL,
					   &sc->mix_size)) {
				aprint_error("%s: can't map codec i/o space\n",
					     sc->sc_dev.dv_xname);
				return;
			}
		}
		if (pci_mapreg_map(pa, ICH_MBBAR, PCI_MAPREG_TYPE_MEM, 0,
				   &sc->iot, &sc->aud_ioh, NULL, &sc->aud_size)) {
			v = pci_conf_read(pa->pa_pc, pa->pa_tag, ICH_CFG);
			pci_conf_write(pa->pa_pc, pa->pa_tag, ICH_CFG,
				       v | ICH_CFG_IOSE);
			if (pci_mapreg_map(pa, ICH_NABMBAR, PCI_MAPREG_TYPE_IO,
					   0, &sc->iot, &sc->aud_ioh, NULL,
					   &sc->aud_size)) {
				aprint_error("%s: can't map device i/o space\n",
					     sc->sc_dev.dv_xname);
				return;
			}
		}
	} else {
		if (pci_mapreg_map(pa, ICH_NAMBAR, PCI_MAPREG_TYPE_IO, 0,
				   &sc->iot, &sc->mix_ioh, NULL, &sc->mix_size)) {
			aprint_error("%s: can't map codec i/o space\n",
				     sc->sc_dev.dv_xname);
			return;
		}
		if (pci_mapreg_map(pa, ICH_NABMBAR, PCI_MAPREG_TYPE_IO, 0,
				   &sc->iot, &sc->aud_ioh, NULL, &sc->aud_size)) {
			aprint_error("%s: can't map device i/o space\n",
				     sc->sc_dev.dv_xname);
			return;
		}
	}
	sc->dmat = pa->pa_dmat;

	/* enable bus mastering */
	v = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    v | PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_BACKTOBACK_ENABLE);

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

	snprintf(sc->sc_audev.name, MAX_AUDIO_DEV_LEN, "%s AC97", d->shortname);
	snprintf(sc->sc_audev.version, MAX_AUDIO_DEV_LEN,
		 "0x%02x", PCI_REVISION(pa->pa_class));
	strlcpy(sc->sc_audev.config, sc->sc_dev.dv_xname, MAX_AUDIO_DEV_LEN);

	/* SiS 7012 needs special handling */
	if (d->id == PCIID_SIS7012) {
		sc->sc_sts_reg = ICH_PICB;
		sc->sc_sample_shift = 0;
		/* Un-mute output. From Linux. */
		bus_space_write_4(sc->iot, sc->aud_ioh, ICH_SIS_NV_CTL,
		    bus_space_read_4(sc->iot, sc->aud_ioh, ICH_SIS_NV_CTL) |
		    ICH_SIS_CTL_UNMUTE);
	} else {
		sc->sc_sts_reg = ICH_STS;
		sc->sc_sample_shift = 1;
	}

	/* Workaround for a 440MX B-stepping erratum */
	sc->sc_dmamap_flags = BUS_DMA_COHERENT;
	if (d->id == PCIID_440MX) {
		sc->sc_dmamap_flags |= BUS_DMA_NOCACHE;
		printf("%s: DMA bug workaround enabled\n", sc->sc_dev.dv_xname);
	}

	/* Set up DMA lists. */
	sc->pcmo.qptr = sc->pcmi.qptr = sc->mici.qptr = 0;
	auich_alloc_cdata(sc);

	DPRINTF(ICH_DEBUG_DMA, ("auich_attach: lists %p %p %p\n",
	    sc->pcmo.dmalist, sc->pcmi.dmalist, sc->mici.dmalist));

	sc->host_if.arg = sc;
	sc->host_if.attach = auich_attach_codec;
	sc->host_if.read = auich_read_codec;
	sc->host_if.write = auich_write_codec;
	sc->host_if.reset = auich_reset_codec;

	if (ac97_attach(&sc->host_if) != 0)
		return;

	/* setup audio_format */
	memcpy(sc->sc_formats, auich_formats, sizeof(auich_formats));
	if (!AC97_IS_4CH(sc->codec_if))
		AUFMT_INVALIDATE(&sc->sc_formats[AUICH_FORMATS_4CH]);
	if (!AC97_IS_6CH(sc->codec_if))
		AUFMT_INVALIDATE(&sc->sc_formats[AUICH_FORMATS_6CH]);
	if (AC97_IS_FIXED_RATE(sc->codec_if)) {
		for (i = 0; i < AUICH_NFORMATS; i++) {
			sc->sc_formats[i].frequency_type = 1;
			sc->sc_formats[i].frequency[0] = 48000;
		}
	}

	if (0 != auconv_create_encodings(sc->sc_formats, AUICH_NFORMATS,
					 &sc->sc_encodings)) {
		return;
	}

	/* Watch for power change */
	sc->sc_suspend = PWR_RESUME;
	sc->sc_powerhook = powerhook_establish(auich_powerhook, sc);

	config_interrupts(self, auich_finish_attach);

	/* sysctl setup */
	if (AC97_IS_FIXED_RATE(sc->codec_if))
		return;
	err = sysctl_createv(&sc->sc_log, 0, NULL, NULL, 0,
			     CTLTYPE_NODE, "hw", NULL, NULL, 0, NULL, 0,
			     CTL_HW, CTL_EOL);
	if (err != 0)
		goto sysctl_err;
	err = sysctl_createv(&sc->sc_log, 0, NULL, &node, 0,
			     CTLTYPE_NODE, sc->sc_dev.dv_xname, NULL, NULL, 0,
			     NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);
	if (err != 0)
		goto sysctl_err;
	node_mib = node->sysctl_num;
	/* passing the sc address instead of &sc->sc_ac97_clock */
	err = sysctl_createv(&sc->sc_log, 0, NULL, &node, CTLFLAG_READWRITE,
			     CTLTYPE_INT, "ac97rate",
			     SYSCTL_DESCR("AC'97 codec link rate"),
			     auich_sysctl_verify, 0, sc, 0,
			     CTL_HW, node_mib, CTL_CREATE, CTL_EOL);
	if (err != 0)
		goto sysctl_err;
	sc->sc_ac97_clock_mib = node->sysctl_num;

	return;

 sysctl_err:
	printf("%s: failed to add sysctl nodes. (%d)\n",
	       sc->sc_dev.dv_xname, err);
	return;			/* failure of sysctl is not fatal. */
}

static int
auich_activate(struct device *self, enum devact act)
{
	struct auich_softc *sc;
	int ret;

	sc = (struct auich_softc *)self;
	ret = 0;
	switch (act) {
	case DVACT_ACTIVATE:
		return EOPNOTSUPP;
	case DVACT_DEACTIVATE:
		if (sc->sc_audiodev != NULL)
			ret = config_deactivate(sc->sc_audiodev);
		return ret;
	}
	return EOPNOTSUPP;
}

static int
auich_detach(struct device *self, int flags)
{
	struct auich_softc *sc;

	sc = (struct auich_softc *)self;

	/* audio */
	if (sc->sc_audiodev != NULL)
		config_detach(sc->sc_audiodev, flags);

	/* sysctl */
	sysctl_teardown(&sc->sc_log);

	/* audio_encoding_set */
	auconv_delete_encodings(sc->sc_encodings);

	/* ac97 */
	if (sc->codec_if != NULL)
		sc->codec_if->vtbl->detach(sc->codec_if);

	/* PCI */
	if (sc->sc_ih != NULL)
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
	if (sc->mix_size != 0)
		bus_space_unmap(sc->iot, sc->mix_ioh, sc->mix_size);
	if (sc->aud_size != 0)
		bus_space_unmap(sc->iot, sc->aud_ioh, sc->aud_size);
	return 0;
}

static int
auich_sysctl_verify(SYSCTLFN_ARGS)
{
	int error, tmp;
	struct sysctlnode node;
	struct auich_softc *sc;

	node = *rnode;
	sc = rnode->sysctl_data;
	tmp = sc->sc_ac97_clock;
	node.sysctl_data = &tmp;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (node.sysctl_num == sc->sc_ac97_clock_mib) {
		if (tmp < 48000 || tmp > 96000)
			return EINVAL;
		sc->sc_ac97_clock = tmp;
	}

	return 0;
}

static void
auich_finish_attach(struct device *self)
{
	struct auich_softc *sc = (void *)self;

	if (!AC97_IS_FIXED_RATE(sc->codec_if))
		auich_calibrate(sc);

	sc->sc_audiodev = audio_attach_mi(&auich_hw_if, sc, &sc->sc_dev);
}

#define ICH_CODECIO_INTERVAL	10
static int
auich_read_codec(void *v, u_int8_t reg, u_int16_t *val)
{
	struct auich_softc *sc = v;
	int i;
	uint32_t status;

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
			DPRINTF(ICH_DEBUG_CODECIO,
			    ("%s: read_codec error\n", sc->sc_dev.dv_xname));
			return -1;
		}
		return 0;
	} else {
		DPRINTF(ICH_DEBUG_CODECIO,
		    ("%s: read_codec timeout\n", sc->sc_dev.dv_xname));
		return -1;
	}
}

static int
auich_write_codec(void *v, u_int8_t reg, u_int16_t val)
{
	struct auich_softc *sc = v;
	int i;

	DPRINTF(ICH_DEBUG_CODECIO, ("auich_write_codec(%x, %x)\n", reg, val));
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

static int
auich_attach_codec(void *v, struct ac97_codec_if *cif)
{
	struct auich_softc *sc = v;

	sc->codec_if = cif;
	return 0;
}

static int
auich_reset_codec(void *v)
{
	struct auich_softc *sc = v;
	int i;
	uint32_t control, status;

	control = bus_space_read_4(sc->iot, sc->aud_ioh, ICH_GCTRL);
	control &= ~(ICH_ACLSO | ICH_PCM246_MASK);
	control |= (control & ICH_CRESET) ? ICH_WRESET : ICH_CRESET;
	bus_space_write_4(sc->iot, sc->aud_ioh, ICH_GCTRL, control);

	for (i = 500000; i >= 0; i--) {
		status = bus_space_read_4(sc->iot, sc->aud_ioh, ICH_GSTS);
		if (status & (ICH_PCR | ICH_SCR | ICH_S2CR))
			break;
		DELAY(1);
	}
	if (i <= 0) {
		printf("%s: auich_reset_codec: time out\n", sc->sc_dev.dv_xname);
		return ETIMEDOUT;
	}
#ifdef DEBUG
	if (status & ICH_SCR)
		printf("%s: The 2nd codec is ready.\n",
		       sc->sc_dev.dv_xname);
	if (status & ICH_S2CR)
		printf("%s: The 3rd codec is ready.\n",
		       sc->sc_dev.dv_xname);
#endif
	return 0;
}

static int
auich_open(void *v, int flags)
{
	return 0;
}

static void
auich_close(void *v)
{
}

static int
auich_query_encoding(void *v, struct audio_encoding *aep)
{
	struct auich_softc *sc;

	sc = (struct auich_softc *)v;
	return auconv_query_encoding(sc->sc_encodings, aep);
}

static int
auich_set_rate(struct auich_softc *sc, int mode, u_long srate)
{
	int ret;
	u_long ratetmp;

	sc->codec_if->vtbl->set_clock(sc->codec_if, sc->sc_ac97_clock);
	ratetmp = srate;
	if (mode == AUMODE_RECORD)
		return sc->codec_if->vtbl->set_rate(sc->codec_if,
		    AC97_REG_PCM_LR_ADC_RATE, &ratetmp);
	ret = sc->codec_if->vtbl->set_rate(sc->codec_if,
	    AC97_REG_PCM_FRONT_DAC_RATE, &ratetmp);
	if (ret)
		return ret;
	ratetmp = srate;
	ret = sc->codec_if->vtbl->set_rate(sc->codec_if,
	    AC97_REG_PCM_SURR_DAC_RATE, &ratetmp);
	if (ret)
		return ret;
	ratetmp = srate;
	ret = sc->codec_if->vtbl->set_rate(sc->codec_if,
	    AC97_REG_PCM_LFE_DAC_RATE, &ratetmp);
	return ret;
}

static int
auich_set_params(void *v, int setmode, int usemode, struct audio_params *play,
    struct audio_params *rec)
{
	struct auich_softc *sc = v;
	struct audio_params *p;
	int mode, index;
	u_int32_t control;

	for (mode = AUMODE_RECORD; mode != -1;
	     mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = mode == AUMODE_PLAY ? play : rec;
		if (p == NULL)
			continue;

		if (p->sample_rate <  8000 ||
		    p->sample_rate > 48000)
			return (EINVAL);

		index = auconv_set_converter(sc->sc_formats, AUICH_NFORMATS,
					     mode, p, TRUE);
		if (index < 0)
			return EINVAL;
		if (sc->sc_formats[index].frequency_type != 1
		    && auich_set_rate(sc, mode, p->hw_sample_rate))
			return EINVAL;
		if (mode == AUMODE_PLAY) {
			control = bus_space_read_4(sc->iot, sc->aud_ioh, ICH_GCTRL);
			control &= ~ICH_PCM246_MASK;
			if (p->hw_channels == 4) {
				control |= ICH_PCM4;
			} else if (p->hw_channels == 6) {
				control |= ICH_PCM6;
			}
			bus_space_write_4(sc->iot, sc->aud_ioh, ICH_GCTRL, control);
		}
	}

	return (0);
}

static int
auich_round_blocksize(void *v, int blk)
{

	return (blk & ~0x3f);		/* keep good alignment */
}

static int
auich_halt_output(void *v)
{
	struct auich_softc *sc = v;

	DPRINTF(ICH_DEBUG_DMA, ("%s: halt_output\n", sc->sc_dev.dv_xname));

	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMO + ICH_CTRL, ICH_RR);
	sc->pcmo.intr = NULL;

	return (0);
}

static int
auich_halt_input(void *v)
{
	struct auich_softc *sc = v;

	DPRINTF(ICH_DEBUG_DMA,
	    ("%s: halt_input\n", sc->sc_dev.dv_xname));

	/* XXX halt both unless known otherwise */

	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_CTRL, ICH_RR);
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_MICI + ICH_CTRL, ICH_RR);
	sc->pcmi.intr = NULL;

	return (0);
}

static int
auich_getdev(void *v, struct audio_device *adp)
{
	struct auich_softc *sc = v;

	*adp = sc->sc_audev;
	return (0);
}

static int
auich_set_port(void *v, mixer_ctrl_t *cp)
{
	struct auich_softc *sc = v;

	return (sc->codec_if->vtbl->mixer_set_port(sc->codec_if, cp));
}

static int
auich_get_port(void *v, mixer_ctrl_t *cp)
{
	struct auich_softc *sc = v;

	return (sc->codec_if->vtbl->mixer_get_port(sc->codec_if, cp));
}

static int
auich_query_devinfo(void *v, mixer_devinfo_t *dp)
{
	struct auich_softc *sc = v;

	return (sc->codec_if->vtbl->query_devinfo(sc->codec_if, dp));
}

static void *
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

static void
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

static size_t
auich_round_buffersize(void *v, int direction, size_t size)
{

	if (size > (ICH_DMALIST_MAX * ICH_DMASEG_MAX))
		size = ICH_DMALIST_MAX * ICH_DMASEG_MAX;

	return size;
}

static paddr_t
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

static int
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
	if (!AC97_IS_FIXED_RATE(sc->codec_if))
		props |= AUDIO_PROP_MMAP;
	return props;
}

static int
auich_intr(void *v)
{
	struct auich_softc *sc = v;
	int ret = 0, gsts;

#ifdef DIAGNOSTIC
	int csts;
#endif

#ifdef DIAGNOSTIC
	csts = pci_conf_read(sc->sc_pc, sc->sc_pt, PCI_COMMAND_STATUS_REG);
	if (csts & PCI_STATUS_MASTER_ABORT) {
		printf("auich_intr: PCI master abort\n");
	}
#endif

	gsts = bus_space_read_4(sc->iot, sc->aud_ioh, ICH_GSTS);
	DPRINTF(ICH_DEBUG_INTR, ("auich_intr: gsts=0x%x\n", gsts));

	if (gsts & ICH_POINT) {
		int sts;

		sts = bus_space_read_2(sc->iot, sc->aud_ioh,
		    ICH_PCMO + sc->sc_sts_reg);
		DPRINTF(ICH_DEBUG_INTR,
		    ("auich_intr: osts=0x%x\n", sts));

		if (sts & ICH_FIFOE)
			printf("%s: fifo underrun\n", sc->sc_dev.dv_xname);

		if (sts & ICH_BCIS) {
			struct auich_dmalist *q;
			int blksize, qptr, i;

			blksize = sc->pcmo.blksize;
			qptr = sc->pcmo.qptr;
			i = bus_space_read_1(sc->iot, sc->aud_ioh,
			    ICH_PCMO + ICH_CIV);

			while (qptr != i) {
				q = &sc->pcmo.dmalist[qptr];

				q->base = sc->pcmo.p;
				q->len = (blksize >> sc->sc_sample_shift) |
				    ICH_DMAF_IOC;
				DPRINTF(ICH_DEBUG_INTR,
				    ("auich_intr: %p, %p = %x @ 0x%x\n",
				    &sc->pcmo.dmalist[i], q, q->len, q->base));

				sc->pcmo.p += blksize;
				if (sc->pcmo.p >= sc->pcmo.end)
					sc->pcmo.p = sc->pcmo.start;

				qptr = (qptr + 1) & ICH_LVI_MASK;
				if (sc->pcmo.intr)
					sc->pcmo.intr(sc->pcmo.arg);
			}

			sc->pcmo.qptr = qptr;
			bus_space_write_1(sc->iot, sc->aud_ioh,
			    ICH_PCMO + ICH_LVI, (qptr - 1) & ICH_LVI_MASK);
		}

		/* int ack */
		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_PCMO +
		    sc->sc_sts_reg, sts & (ICH_BCIS | ICH_FIFOE));
		bus_space_write_4(sc->iot, sc->aud_ioh, ICH_GSTS, ICH_POINT);
		ret++;
	}

	if (gsts & ICH_PIINT) {
		int sts;

		sts = bus_space_read_2(sc->iot, sc->aud_ioh,
		    ICH_PCMI + sc->sc_sts_reg);
		DPRINTF(ICH_DEBUG_INTR,
		    ("auich_intr: ists=0x%x\n", sts));

		if (sts & ICH_FIFOE)
			printf("%s: fifo overrun\n", sc->sc_dev.dv_xname);

		if (sts & ICH_BCIS) {
			struct auich_dmalist *q;
			int blksize, qptr, i;

			blksize = sc->pcmi.blksize;
			qptr = sc->pcmi.qptr;
			i = bus_space_read_1(sc->iot, sc->aud_ioh,
			    ICH_PCMI + ICH_CIV);

			while (qptr != i) {
				q = &sc->pcmi.dmalist[qptr];

				q->base = sc->pcmi.p;
				q->len = (blksize >> sc->sc_sample_shift) |
				    ICH_DMAF_IOC;
				DPRINTF(ICH_DEBUG_INTR,
				    ("auich_intr: %p, %p = %x @ 0x%x\n",
				    &sc->pcmi.dmalist[i], q, q->len, q->base));

				sc->pcmi.p += blksize;
				if (sc->pcmi.p >= sc->pcmi.end)
					sc->pcmi.p = sc->pcmi.start;

				qptr = (qptr + 1) & ICH_LVI_MASK;
				if (sc->pcmi.intr)
					sc->pcmi.intr(sc->pcmi.arg);
			}

			sc->pcmi.qptr = qptr;
			bus_space_write_1(sc->iot, sc->aud_ioh,
			    ICH_PCMI + ICH_LVI, (qptr - 1) & ICH_LVI_MASK);
		}

		/* int ack */
		bus_space_write_2(sc->iot, sc->aud_ioh, ICH_PCMI +
		    sc->sc_sts_reg, sts & (ICH_BCIS | ICH_FIFOE));
		bus_space_write_4(sc->iot, sc->aud_ioh, ICH_GSTS, ICH_PIINT);
		ret++;
	}

	if (gsts & ICH_MIINT) {
		int sts;

		sts = bus_space_read_2(sc->iot, sc->aud_ioh,
		    ICH_MICI + sc->sc_sts_reg);
		DPRINTF(ICH_DEBUG_INTR,
		    ("auich_intr: ists=0x%x\n", sts));

		if (sts & ICH_FIFOE)
			printf("%s: fifo overrun\n", sc->sc_dev.dv_xname);

		/* TODO mic input DMA */

		bus_space_write_4(sc->iot, sc->aud_ioh, ICH_GSTS, ICH_MIINT);
	}

	return ret;
}

static int
auich_trigger_output(void *v, void *start, void *end, int blksize,
    void (*intr)(void *), void *arg, struct audio_params *param)
{
	struct auich_softc *sc = v;
	struct auich_dmalist *q;
	struct auich_dma *p;
	size_t size;
	int qptr;
#ifdef DIAGNOSTIC
	int csts;
#endif

	DPRINTF(ICH_DEBUG_DMA,
	    ("auich_trigger_output(%p, %p, %d, %p, %p, %p)\n",
	    start, end, blksize, intr, arg, param));

	sc->pcmo.intr = intr;
	sc->pcmo.arg = arg;
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
	sc->pcmo.start = DMAADDR(p);
	sc->pcmo.p = sc->pcmo.start;
	sc->pcmo.end = sc->pcmo.start + size;
	sc->pcmo.blksize = blksize;

	for (qptr = 0; qptr < ICH_DMALIST_MAX; qptr++) {
		q = &sc->pcmo.dmalist[qptr];

		q->base = sc->pcmo.p;
		q->len = (blksize >> sc->sc_sample_shift) | ICH_DMAF_IOC;

		sc->pcmo.p += blksize;
		if (sc->pcmo.p >= sc->pcmo.end)
			sc->pcmo.p = sc->pcmo.start;
	}

	sc->pcmo.qptr = qptr = 0;
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMO + ICH_LVI,
	    (qptr - 1) & ICH_LVI_MASK);

	bus_space_write_4(sc->iot, sc->aud_ioh, ICH_PCMO + ICH_BDBAR,
	    sc->sc_cddma + ICH_PCMO_OFF(0));
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMO + ICH_CTRL,
	    ICH_IOCE | ICH_FEIE | ICH_RPBM);

	return (0);
}

static int
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
	int qptr;
#ifdef DIAGNOSTIC
	int csts;
#endif

	DPRINTF(ICH_DEBUG_DMA,
	    ("auich_trigger_input(%p, %p, %d, %p, %p, %p)\n",
	    start, end, blksize, intr, arg, param));

	sc->pcmi.intr = intr;
	sc->pcmi.arg = arg;

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
	sc->pcmi.start = DMAADDR(p);
	sc->pcmi.p = sc->pcmi.start;
	sc->pcmi.end = sc->pcmi.start + size;
	sc->pcmi.blksize = blksize;

	for (qptr = 0; qptr < ICH_DMALIST_MAX; qptr++) {
		q = &sc->pcmi.dmalist[qptr];

		q->base = sc->pcmi.p;
		q->len = (blksize >> sc->sc_sample_shift) | ICH_DMAF_IOC;

		sc->pcmi.p += blksize;
		if (sc->pcmi.p >= sc->pcmi.end)
			sc->pcmi.p = sc->pcmi.start;
	}

	sc->pcmi.qptr = qptr = 0;
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_LVI,
	    (qptr - 1) & ICH_LVI_MASK);

	bus_space_write_4(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_BDBAR,
	    sc->sc_cddma + ICH_PCMI_OFF(0));
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_CTRL,
	    ICH_IOCE | ICH_FEIE | ICH_RPBM);

	return (0);
}

static int
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

static int
auich_freemem(struct auich_softc *sc, struct auich_dma *p)
{

	bus_dmamap_unload(sc->dmat, p->map);
	bus_dmamap_destroy(sc->dmat, p->map);
	bus_dmamem_unmap(sc->dmat, p->addr, p->size);
	bus_dmamem_free(sc->dmat, p->segs, p->nsegs);
	return (0);
}

static int
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

	sc->pcmo.dmalist = sc->sc_cdata->ic_dmalist_pcmo;
	sc->pcmi.dmalist = sc->sc_cdata->ic_dmalist_pcmi;
	sc->mici.dmalist = sc->sc_cdata->ic_dmalist_mici;

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

static void
auich_powerhook(int why, void *addr)
{
	struct auich_softc *sc = (struct auich_softc *)addr;

	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		/* Power down */
		DPRINTF(1, ("%s: power down\n", sc->sc_dev.dv_xname));
		sc->sc_suspend = why;
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
		break;

	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
}

/*
 * Calibrate card (some boards are overclocked and need scaling)
 */
static void
auich_calibrate(struct auich_softc *sc)
{
	struct timeval t1, t2;
	uint8_t ociv, nciv;
	uint64_t wait_us;
	uint32_t actual_48k_rate, bytes, ac97rate;
	void *temp_buffer;
	struct auich_dma *p;
	u_long rate;

	/*
	 * Grab audio from input for fixed interval and compare how
	 * much we actually get with what we expect.  Interval needs
	 * to be sufficiently short that no interrupts are
	 * generated.
	 */

	/* Force the codec to a known state first. */
	sc->codec_if->vtbl->set_clock(sc->codec_if, 48000);
	rate = sc->sc_ac97_clock = 48000;
	sc->codec_if->vtbl->set_rate(sc->codec_if, AC97_REG_PCM_LR_ADC_RATE,
	    &rate);

	/* Setup a buffer */
	bytes = 64000;
	temp_buffer = auich_allocm(sc, AUMODE_RECORD, bytes, M_DEVBUF, M_WAITOK);

	for (p = sc->sc_dmas; p && KERNADDR(p) != temp_buffer; p = p->next)
		;
	if (p == NULL) {
		printf("auich_calibrate: bad address %p\n", temp_buffer);
		return;
	}
	sc->pcmi.dmalist[0].base = DMAADDR(p);
	sc->pcmi.dmalist[0].len = (bytes >> sc->sc_sample_shift);

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
	bus_space_write_4(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_BDBAR,
			  sc->sc_cddma + ICH_PCMI_OFF(0));
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_LVI,
			  (0 - 1) & ICH_LVI_MASK);

	/* start */
	microtime(&t1);
	bus_space_write_1(sc->iot, sc->aud_ioh, ICH_PCMI + ICH_CTRL, ICH_RPBM);

	/* wait */
	nciv = ociv;
	do {
		microtime(&t2);
		if (t2.tv_sec - t1.tv_sec > 1)
			break;
		nciv = bus_space_read_1(sc->iot, sc->aud_ioh,
					ICH_PCMI + ICH_CIV);
	} while (nciv == ociv);
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
		printf("%s: ac97 link rate calibration timed out after %"
		       PRIu64 " us\n", sc->sc_dev.dv_xname, wait_us);
		return;
	}

	actual_48k_rate = (bytes * UINT64_C(250000)) / wait_us;

	if (actual_48k_rate < 50000)
		ac97rate = 48000;
	else
		ac97rate = ((actual_48k_rate + 500) / 1000) * 1000;

	printf("%s: measured ac97 link rate at %d Hz",
	       sc->sc_dev.dv_xname, actual_48k_rate);
	if (ac97rate != actual_48k_rate)
		printf(", will use %d Hz", ac97rate);
	printf("\n");

	sc->sc_ac97_clock = ac97rate;
}
