/*      $NetBSD: sv.c,v 1.12.2.1 2001/08/24 00:10:22 nathanw Exp $ */
/*      $OpenBSD: sv.c,v 1.2 1998/07/13 01:50:15 csapuntz Exp $ */

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*
 * Copyright (c) 1998 Constantine Paul Sapuntzakis
 * All rights reserved
 *
 * Author: Constantine Paul Sapuntzakis (csapuntz@cvs.openbsd.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The author's name or those of the contributors may be used to
 *    endorse or promote products derived from this software without 
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS
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
 * S3 SonicVibes driver
 *   Heavily based on the eap driver by Lennart Augustsson
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>

#include <dev/ic/i8237reg.h>
#include <dev/pci/svreg.h>
#include <dev/pci/svvar.h>

#include <machine/bus.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (svdebug) printf x
#define DPRINTFN(n,x)	if (svdebug>(n)) printf x
int	svdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

int	sv_match __P((struct device *, struct cfdata *, void *));
void	sv_attach __P((struct device *, struct device *, void *));
int	sv_intr __P((void *));

struct sv_dma {
	bus_dmamap_t map;
	caddr_t addr;
	bus_dma_segment_t segs[1];
	int nsegs;
	size_t size;
	struct sv_dma *next;
};
#define DMAADDR(p) ((p)->map->dm_segs[0].ds_addr)
#define KERNADDR(p) ((void *)((p)->addr))

struct cfattach sv_ca = {
	sizeof(struct sv_softc), sv_match, sv_attach
};

struct audio_device sv_device = {
	"S3 SonicVibes",
	"",
	"sv"
};

#define ARRAY_SIZE(foo)  ((sizeof(foo)) / sizeof(foo[0]))

int	sv_allocmem __P((struct sv_softc *, size_t, size_t, int, struct sv_dma *));
int	sv_freemem __P((struct sv_softc *, struct sv_dma *));

int	sv_open __P((void *, int));
void	sv_close __P((void *));
int	sv_query_encoding __P((void *, struct audio_encoding *));
int	sv_set_params __P((void *, int, int, struct audio_params *, struct audio_params *));
int	sv_round_blocksize __P((void *, int));
int	sv_trigger_output __P((void *, void *, void *, int, void (*)(void *),
	    void *, struct audio_params *));
int	sv_trigger_input __P((void *, void *, void *, int, void (*)(void *),
	    void *, struct audio_params *));
int	sv_halt_output __P((void *));
int	sv_halt_input __P((void *));
int	sv_getdev __P((void *, struct audio_device *));
int	sv_mixer_set_port __P((void *, mixer_ctrl_t *));
int	sv_mixer_get_port __P((void *, mixer_ctrl_t *));
int	sv_query_devinfo __P((void *, mixer_devinfo_t *));
void   *sv_malloc __P((void *, int, size_t, int, int));
void	sv_free __P((void *, void *, int));
size_t	sv_round_buffersize __P((void *, int, size_t));
paddr_t	sv_mappage __P((void *, void *, off_t, int));
int	sv_get_props __P((void *));

#ifdef AUDIO_DEBUG
void    sv_dumpregs __P((struct sv_softc *sc));
#endif

struct audio_hw_if sv_hw_if = {
	sv_open,
	sv_close,
	NULL,
	sv_query_encoding,
	sv_set_params,
	sv_round_blocksize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	sv_halt_output,
	sv_halt_input,
	NULL,
	sv_getdev,
	NULL,
	sv_mixer_set_port,
	sv_mixer_get_port,
	sv_query_devinfo,
	sv_malloc,
	sv_free,
	sv_round_buffersize,
	sv_mappage,
	sv_get_props,
	sv_trigger_output,
	sv_trigger_input,
};


static u_int8_t sv_read __P((struct sv_softc *, u_int8_t));
static u_int8_t sv_read_indirect __P((struct sv_softc *, u_int8_t));
static void sv_write __P((struct sv_softc *, u_int8_t, u_int8_t ));
static void sv_write_indirect __P((struct sv_softc *, u_int8_t, u_int8_t ));
static void sv_init_mixer __P((struct sv_softc *));

static void sv_defer __P((struct device *self));

static void
sv_write (sc, reg, val)
	struct sv_softc *sc;
	u_int8_t reg, val;
     
{
	DPRINTFN(8,("sv_write(0x%x, 0x%x)\n", reg, val));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, reg, val);
}

static u_int8_t
sv_read(sc, reg)
	struct sv_softc *sc;
	u_int8_t reg;
     
{
	u_int8_t val;

	val = bus_space_read_1(sc->sc_iot, sc->sc_ioh, reg);
	DPRINTFN(8,("sv_read(0x%x) = 0x%x\n", reg, val));
	return val;
}

static u_int8_t
sv_read_indirect(sc, reg)
	struct sv_softc *sc;
	u_int8_t reg;
{
	u_int8_t val;
	int s = splaudio();

	sv_write(sc, SV_CODEC_IADDR, reg & SV_IADDR_MASK);
	val = sv_read(sc, SV_CODEC_IDATA);
	splx(s);
	return (val);
}

static void
sv_write_indirect(sc, reg, val)
	struct sv_softc *sc;
	u_int8_t reg, val;
{
	u_int8_t iaddr = reg & SV_IADDR_MASK;
	int s = splaudio();

	if (reg == SV_DMA_DATA_FORMAT)
		iaddr |= SV_IADDR_MCE;

	sv_write(sc, SV_CODEC_IADDR, iaddr);
	sv_write(sc, SV_CODEC_IDATA, val);
	splx(s);
}

int
sv_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_S3 &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_S3_SONICVIBES)
		return (1);
	
	return (0);
}

int pci_alloc_io __P((pci_chipset_tag_t pc, pcitag_t pt,
		      int pcioffs,
		      bus_space_tag_t iot, bus_size_t size,
		      bus_size_t align, bus_size_t bound, int flags,
		      bus_space_handle_t *ioh));

#define PCI_IO_ALLOC_LOW 0xa000
#define PCI_IO_ALLOC_HIGH 0xb000
int
pci_alloc_io(pc, pt, pcioffs, iot, size, align, bound, flags, ioh)
	pci_chipset_tag_t pc;
	pcitag_t pt;
	int pcioffs;
	bus_space_tag_t iot;
	bus_size_t size;
	bus_size_t align;
	bus_size_t bound;
	int flags;
	bus_space_handle_t *ioh;
{
	bus_addr_t addr;
	int error;

	error = bus_space_alloc(iot, PCI_IO_ALLOC_LOW, PCI_IO_ALLOC_HIGH,
				size, align, bound, flags, &addr, ioh);
	if (error)
		return(error);

	pci_conf_write(pc, pt, pcioffs, addr);
	return (0);
}

/*
 * Allocate IO addresses when all other configuration is done.
 */
void
sv_defer(self)
	struct device *self;
{
	struct sv_softc *sc = (struct sv_softc *)self;
	pci_chipset_tag_t pc = sc->sc_pa.pa_pc;
	pcitag_t pt = sc->sc_pa.pa_tag;
	pcireg_t dmaio;

	DPRINTF(("sv_defer: %p\n", sc));
	if (pci_alloc_io(pc, pt, SV_DMAA_CONFIG_OFF, 
			  sc->sc_iot, SV_DMAA_SIZE, SV_DMAA_ALIGN, 0,
			  0, &sc->sc_dmaa_ioh)) {
		printf("sv_attach: cannot allocate DMA A range\n");
		return;
	}
	dmaio = pci_conf_read(pc, pt, SV_DMAA_CONFIG_OFF);
	DPRINTF(("sv_attach: addr a dmaio=0x%lx\n", (u_long)dmaio));
	pci_conf_write(pc, pt, SV_DMAA_CONFIG_OFF, 
		       dmaio | SV_DMA_CHANNEL_ENABLE | SV_DMAA_EXTENDED_ADDR);

	if (pci_alloc_io(pc, pt, SV_DMAC_CONFIG_OFF, 
			  sc->sc_iot, SV_DMAC_SIZE, SV_DMAC_ALIGN, 0,
			  0, &sc->sc_dmac_ioh)) {
		printf("sv_attach: cannot allocate DMA C range\n");
		return;
	}
	dmaio = pci_conf_read(pc, pt, SV_DMAC_CONFIG_OFF);
	DPRINTF(("sv_attach: addr c dmaio=0x%lx\n", (u_long)dmaio));
	pci_conf_write(pc, pt, SV_DMAC_CONFIG_OFF, 
		       dmaio | SV_DMA_CHANNEL_ENABLE);

	sc->sc_dmaset = 1;
}

void
sv_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sv_softc *sc = (struct sv_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t pt = pa->pa_tag;
	pci_intr_handle_t ih;
	pcireg_t csr;
	char const *intrstr;
	u_int8_t reg;
	struct audio_attach_args arg;
	
	printf ("\n");
	
	/* Map I/O registers */
	if (pci_mapreg_map(pa, SV_ENHANCED_PORTBASE_SLOT,
			   PCI_MAPREG_TYPE_IO, 0,
			   &sc->sc_iot, &sc->sc_ioh, NULL, NULL)) {
		printf("%s: can't map enhanced i/o space\n", 
		       sc->sc_dev.dv_xname);
		return;
	}
	if (pci_mapreg_map(pa, SV_FM_PORTBASE_SLOT,
			   PCI_MAPREG_TYPE_IO, 0,
			   &sc->sc_opliot, &sc->sc_oplioh, NULL, NULL)) {
		printf("%s: can't map FM i/o space\n", sc->sc_dev.dv_xname);
		return;
	}
	if (pci_mapreg_map(pa, SV_MIDI_PORTBASE_SLOT,
			   PCI_MAPREG_TYPE_IO, 0,
			   &sc->sc_midiiot, &sc->sc_midiioh, NULL, NULL)) {
		printf("%s: can't map MIDI i/o space\n", sc->sc_dev.dv_xname);
		return;
	}
	DPRINTF(("sv: IO ports: enhanced=0x%x, OPL=0x%x, MIDI=0x%x\n",
		 (int)sc->sc_ioh, (int)sc->sc_oplioh, (int)sc->sc_midiioh));

#ifdef alpha
	/* XXX Force allocation through the SGMAP. */
	sc->sc_dmatag = alphabus_dma_get_tag(pa->pa_dmat, ALPHA_BUS_ISA);
#else
	sc->sc_dmatag = pa->pa_dmat;
#endif

	pci_conf_write(pc, pt, SV_DMAA_CONFIG_OFF, SV_DMAA_EXTENDED_ADDR);
	pci_conf_write(pc, pt, SV_DMAC_CONFIG_OFF, 0);

	/* Enable the device. */
	csr = pci_conf_read(pc, pt, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pc, pt, PCI_COMMAND_STATUS_REG,
		       csr | PCI_COMMAND_MASTER_ENABLE);

	sv_write_indirect(sc, SV_ANALOG_POWER_DOWN_CONTROL, 0);
	sv_write_indirect(sc, SV_DIGITAL_POWER_DOWN_CONTROL, 0);
	
	/* initialize codec registers */
	reg = sv_read(sc, SV_CODEC_CONTROL);
	reg |= SV_CTL_RESET;
	sv_write(sc, SV_CODEC_CONTROL, reg);
	delay(50);

	reg = sv_read(sc, SV_CODEC_CONTROL);
	reg &= ~SV_CTL_RESET;
	reg |= SV_CTL_INTA | SV_CTL_ENHANCED;

	/* This write clears the reset */
	sv_write(sc, SV_CODEC_CONTROL, reg);
	delay(50);

	/* This write actually shoves the new values in */
	sv_write(sc, SV_CODEC_CONTROL, reg);

	DPRINTF(("sv_attach: control=0x%x\n", sv_read(sc, SV_CODEC_CONTROL)));

	/* Enable DMA interrupts */
	reg = sv_read(sc, SV_CODEC_INTMASK);
	reg &= ~(SV_INTMASK_DMAA | SV_INTMASK_DMAC);
	reg |= SV_INTMASK_UD | SV_INTMASK_SINT | SV_INTMASK_MIDI;
	sv_write(sc, SV_CODEC_INTMASK, reg);

	sv_read(sc, SV_CODEC_STATUS);
	
	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_AUDIO, sv_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		       sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);
	printf("%s: rev %d", sc->sc_dev.dv_xname, 
	       sv_read_indirect(sc, SV_REVISION_LEVEL));
	if (sv_read(sc, SV_CODEC_CONTROL) & SV_CTL_MD1)
		printf(", reverb SRAM present");
	if (!(sv_read_indirect(sc, SV_WAVETABLE_SOURCE_SELECT) & SV_WSS_WT0))
		printf(", wavetable ROM present");
	printf("\n");
	
	sv_init_mixer(sc);
	
	audio_attach_mi(&sv_hw_if, sc, &sc->sc_dev);

	arg.type = AUDIODEV_TYPE_OPL;
	arg.hwif = 0;
	arg.hdl = 0;
	(void)config_found(&sc->sc_dev, &arg, audioprint);

	sc->sc_pa = *pa;	/* for deferred setup */
	config_defer(self, sv_defer);
}

#ifdef AUDIO_DEBUG
void
sv_dumpregs(sc)
	struct sv_softc *sc;
{
	int idx;

#if 0
	for (idx = 0; idx < 0x50; idx += 4)
		printf ("%02x = %x\n", idx, 
			pci_conf_read(pa->pa_pc, pa->pa_tag, idx));
#endif

	for (idx = 0; idx < 6; idx++)
		printf ("REG %02x = %02x\n", idx, sv_read(sc, idx));
	
	for (idx = 0; idx < 0x32; idx++)
		printf ("IREG %02x = %02x\n", idx, sv_read_indirect(sc, idx));

	for (idx = 0; idx < 0x10; idx++)
		printf ("DMA %02x = %02x\n", idx, 
			bus_space_read_1(sc->sc_iot, sc->sc_dmaa_ioh, idx));
}
#endif

int
sv_intr(p)
	void *p;
{
	struct sv_softc *sc = p;
	u_int8_t intr;

	intr = sv_read(sc, SV_CODEC_STATUS);
	DPRINTFN(5,("sv_intr: intr=0x%x\n", intr));

	if (!(intr & (SV_INTSTATUS_DMAA | SV_INTSTATUS_DMAC))) 
		return (0);

	if (intr & SV_INTSTATUS_DMAA) {
		if (sc->sc_pintr)
			sc->sc_pintr(sc->sc_parg);
	}

	if (intr & SV_INTSTATUS_DMAC) {
		if (sc->sc_rintr)
			sc->sc_rintr(sc->sc_rarg);
	}
	
	return (1);
}

int
sv_allocmem(sc, size, align, direction, p)
	struct sv_softc *sc;
	size_t size;
	size_t align;
	int direction;
	struct sv_dma *p;
{
	int error;

	p->size = size;
	error = bus_dmamem_alloc(sc->sc_dmatag, p->size, align, 0,
				 p->segs, ARRAY_SIZE(p->segs),
				 &p->nsegs, BUS_DMA_NOWAIT);
	if (error)
		return (error);

	error = bus_dmamem_map(sc->sc_dmatag, p->segs, p->nsegs, p->size, 
			       &p->addr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error)
		goto free;

	error = bus_dmamap_create(sc->sc_dmatag, p->size, 1, p->size,
				  0, BUS_DMA_NOWAIT, &p->map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(sc->sc_dmatag, p->map, p->addr, p->size, NULL, 
				BUS_DMA_NOWAIT |
                                (direction == AUMODE_RECORD) ? BUS_DMA_READ : BUS_DMA_WRITE);
	if (error)
		goto destroy;
	DPRINTF(("sv_allocmem: pa=%lx va=%lx pba=%lx\n",
	    (long)p->segs[0].ds_addr, (long)KERNADDR(p), (long)DMAADDR(p)));
	return (0);

destroy:
	bus_dmamap_destroy(sc->sc_dmatag, p->map);
unmap:
	bus_dmamem_unmap(sc->sc_dmatag, p->addr, p->size);
free:
	bus_dmamem_free(sc->sc_dmatag, p->segs, p->nsegs);
	return (error);
}

int
sv_freemem(sc, p)
	struct sv_softc *sc;
	struct sv_dma *p;
{
	bus_dmamap_unload(sc->sc_dmatag, p->map);
	bus_dmamap_destroy(sc->sc_dmatag, p->map);
	bus_dmamem_unmap(sc->sc_dmatag, p->addr, p->size);
	bus_dmamem_free(sc->sc_dmatag, p->segs, p->nsegs);
	return (0);
}

int
sv_open(addr, flags)
	void *addr;
	int flags;
{
	struct sv_softc *sc = addr;

	DPRINTF(("sv_open\n"));
	if (!sc->sc_dmaset)
		return (ENXIO);
	sc->sc_pintr = 0;
	sc->sc_rintr = 0;

	return (0);
}

/*
 * Close function is called at splaudio().
 */
void
sv_close(addr)
	void *addr;
{
	struct sv_softc *sc = addr;
    
	DPRINTF(("sv_close\n"));
	sv_halt_output(sc);
	sv_halt_input(sc);

	sc->sc_pintr = 0;
	sc->sc_rintr = 0;
}

int
sv_query_encoding(addr, fp)
	void *addr;
	struct audio_encoding *fp;
{
	switch (fp->index) {
	case 0:
		strcpy(fp->name, AudioEulinear);
		fp->encoding = AUDIO_ENCODING_ULINEAR;
		fp->precision = 8;
		fp->flags = 0;
		return (0);
	case 1:
		strcpy(fp->name, AudioEmulaw);
		fp->encoding = AUDIO_ENCODING_ULAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 2:
		strcpy(fp->name, AudioEalaw);
		fp->encoding = AUDIO_ENCODING_ALAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 3:
		strcpy(fp->name, AudioEslinear);
		fp->encoding = AUDIO_ENCODING_SLINEAR;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 4:
		strcpy(fp->name, AudioEslinear_le);
		fp->encoding = AUDIO_ENCODING_SLINEAR_LE;
		fp->precision = 16;
		fp->flags = 0;
		return (0);
	case 5:
		strcpy(fp->name, AudioEulinear_le);
		fp->encoding = AUDIO_ENCODING_ULINEAR_LE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 6:
		strcpy(fp->name, AudioEslinear_be);
		fp->encoding = AUDIO_ENCODING_SLINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 7:
		strcpy(fp->name, AudioEulinear_be);
		fp->encoding = AUDIO_ENCODING_ULINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	default:
		return (EINVAL);
	}
}

int
sv_set_params(addr, setmode, usemode, play, rec)
	void *addr;
	int setmode, usemode;
	struct audio_params *play, *rec;
{
	struct sv_softc *sc = addr;
	struct audio_params *p = NULL;
	int mode;
	u_int32_t val;
	
	/*
	 * This device only has one clock, so make the sample rates match.
	 */
	if (play->sample_rate != rec->sample_rate &&
	    usemode == (AUMODE_PLAY | AUMODE_RECORD)) {
		if (setmode == AUMODE_PLAY) {
			rec->sample_rate = play->sample_rate;
			setmode |= AUMODE_RECORD;
		} else if (setmode == AUMODE_RECORD) {
			play->sample_rate = rec->sample_rate;
			setmode |= AUMODE_PLAY;
		} else
			return (EINVAL);
	}

	for (mode = AUMODE_RECORD; mode != -1; 
	     mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = mode == AUMODE_PLAY ? play : rec;

		if (p->sample_rate < 2000 || p->sample_rate > 48000 ||
		    (p->precision != 8 && p->precision != 16) ||
		    (p->channels != 1 && p->channels != 2))
			return (EINVAL);

		p->factor = 1;
		p->sw_code = 0;
		switch (p->encoding) {
		case AUDIO_ENCODING_SLINEAR_BE:
			if (p->precision == 16)
				p->sw_code = swap_bytes;
			else
				p->sw_code = change_sign8;
			break;
		case AUDIO_ENCODING_SLINEAR_LE:
			if (p->precision != 16)
				p->sw_code = change_sign8;
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
			if (p->precision == 16) {
				if (mode == AUMODE_PLAY)
					p->sw_code = swap_bytes_change_sign16_le;
				else
					p->sw_code = change_sign16_swap_bytes_le;
			}
			break;
		case AUDIO_ENCODING_ULINEAR_LE:
			if (p->precision == 16)
				p->sw_code = change_sign16_le;
			break;
		case AUDIO_ENCODING_ULAW:
			if (mode == AUMODE_PLAY) {
				p->factor = 2;
				p->sw_code = mulaw_to_slinear16_le;
			} else
				p->sw_code = ulinear8_to_mulaw;
			break;
		case AUDIO_ENCODING_ALAW:
			if (mode == AUMODE_PLAY) {
				p->factor = 2;
				p->sw_code = alaw_to_slinear16_le;
			} else
				p->sw_code = ulinear8_to_alaw;
			break;
		default:
			return (EINVAL);
		}
	}
	
	val = p->sample_rate * 65536 / 48000;
	/*
	 * If the sample rate is exactly 48KHz, the fraction would overflow the
	 * register, so we have to bias it.  This causes a little clock drift.
	 * The drift is below normal crystal tolerance (.0001%), so although
	 * this seems a little silly, we can pretty much ignore it.
	 * (I tested the output speed with values of 1-20, just to be sure this
	 * register isn't *supposed* to have a bias.  It isn't.)
	 * - mycroft
	 */
	if (val > 65535)
		val = 65535;

	sv_write_indirect(sc, SV_PCM_SAMPLE_RATE_0, val & 0xff);
	sv_write_indirect(sc, SV_PCM_SAMPLE_RATE_1, val >> 8);

#define F_REF 24576000

#define ABS(x) (((x) < 0) ? (-x) : (x))

	if (setmode & AUMODE_RECORD) {
		/* The ADC reference frequency (f_out) is 512 * sample rate */

		/* f_out is dervied from the 24.576MHZ crystal by three values:
		   M & N & R. The equation is as follows:

		   f_out = (m + 2) * f_ref / ((n + 2) * (2 ^ a))

		   with the constraint that:

		   80 MhZ < (m + 2) / (n + 2) * f_ref <= 150Mhz
		   and n, m >= 1
		*/

		int  goal_f_out = 512 * rec->sample_rate;
		int  a, n, m, best_n = 0, best_m = 0, best_error = 10000000;
		int  pll_sample;
		int  error;

		for (a = 0; a < 8; a++) {
			if ((goal_f_out * (1 << a)) >= 80000000)
				break;
		}
	  
		/* a != 8 because sample_rate >= 2000 */

		for (n = 33; n > 2; n--) {
			m = (goal_f_out * n * (1 << a)) / F_REF;
			if ((m > 257) || (m < 3))
				continue;
 
			pll_sample = (m * F_REF) / (n * (1 << a));
			pll_sample /= 512;

			/* Threshold might be good here */
			error = pll_sample - rec->sample_rate;
			error = ABS(error);
	    
			if (error < best_error) {
				best_error = error;
				best_n = n;
				best_m = m;
				if (error == 0) break;
			}
		}

		best_n -= 2;
		best_m -= 2;
	  
		sv_write_indirect(sc, SV_ADC_PLL_M, best_m);
		sv_write_indirect(sc, SV_ADC_PLL_N, 
				  best_n | (a << SV_PLL_R_SHIFT));
	}

	return (0);
}

int
sv_round_blocksize(addr, blk)
	void *addr;
	int blk;
{
	return (blk & -32);	/* keep good alignment */
}

int
sv_trigger_output(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct sv_softc *sc = addr;
	struct sv_dma *p;
	u_int8_t mode;
	int dma_count;

	DPRINTFN(1, ("sv_trigger_output: sc=%p start=%p end=%p blksize=%d intr=%p(%p)\n", 
	    addr, start, end, blksize, intr, arg));
	sc->sc_pintr = intr;
	sc->sc_parg = arg;

	mode = sv_read_indirect(sc, SV_DMA_DATA_FORMAT);
	mode &= ~(SV_DMAA_FORMAT16 | SV_DMAA_STEREO);
	if (param->precision * param->factor == 16)
		mode |= SV_DMAA_FORMAT16;
	if (param->channels == 2)
		mode |= SV_DMAA_STEREO;
	sv_write_indirect(sc, SV_DMA_DATA_FORMAT, mode);

	for (p = sc->sc_dmas; p && KERNADDR(p) != start; p = p->next)
		;
	if (!p) {
		printf("sv_trigger_output: bad addr %p\n", start);
		return (EINVAL);
	}

	dma_count = ((char *)end - (char *)start) - 1;
	DPRINTF(("sv_trigger_output: dma start loop input addr=%x cc=%d\n", 
	    (int)DMAADDR(p), dma_count));

	bus_space_write_4(sc->sc_iot, sc->sc_dmaa_ioh, SV_DMA_ADDR0,
			  DMAADDR(p));
	bus_space_write_4(sc->sc_iot, sc->sc_dmaa_ioh, SV_DMA_COUNT0,
			  dma_count);
	bus_space_write_1(sc->sc_iot, sc->sc_dmaa_ioh, SV_DMA_MODE,
			  DMA37MD_READ | DMA37MD_LOOP);

	DPRINTF(("sv_trigger_output: current addr=%x\n",
	    bus_space_read_4(sc->sc_iot, sc->sc_dmaa_ioh, SV_DMA_ADDR0)));

	dma_count = blksize - 1;

	sv_write_indirect(sc, SV_DMAA_COUNT1, dma_count >> 8);
	sv_write_indirect(sc, SV_DMAA_COUNT0, dma_count & 0xFF);

	mode = sv_read_indirect(sc, SV_PLAY_RECORD_ENABLE);
	sv_write_indirect(sc, SV_PLAY_RECORD_ENABLE, mode | SV_PLAY_ENABLE);

	return (0);
}

int
sv_trigger_input(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct sv_softc *sc = addr;
	struct sv_dma *p;
	u_int8_t mode;
	int dma_count;

	DPRINTFN(1, ("sv_trigger_input: sc=%p start=%p end=%p blksize=%d intr=%p(%p)\n", 
	    addr, start, end, blksize, intr, arg));
	sc->sc_rintr = intr;
	sc->sc_rarg = arg;

	mode = sv_read_indirect(sc, SV_DMA_DATA_FORMAT);
	mode &= ~(SV_DMAC_FORMAT16 | SV_DMAC_STEREO);
	if (param->precision * param->factor == 16)
		mode |= SV_DMAC_FORMAT16;
	if (param->channels == 2)
		mode |= SV_DMAC_STEREO;
	sv_write_indirect(sc, SV_DMA_DATA_FORMAT, mode);

	for (p = sc->sc_dmas; p && KERNADDR(p) != start; p = p->next)
		;
	if (!p) {
		printf("sv_trigger_input: bad addr %p\n", start);
		return (EINVAL);
	}

	dma_count = (((char *)end - (char *)start) >> 1) - 1;
	DPRINTF(("sv_trigger_input: dma start loop input addr=%x cc=%d\n", 
	    (int)DMAADDR(p), dma_count));

	bus_space_write_4(sc->sc_iot, sc->sc_dmac_ioh, SV_DMA_ADDR0,
			  DMAADDR(p));
	bus_space_write_4(sc->sc_iot, sc->sc_dmac_ioh, SV_DMA_COUNT0,
			  dma_count);
	bus_space_write_1(sc->sc_iot, sc->sc_dmac_ioh, SV_DMA_MODE,
			  DMA37MD_WRITE | DMA37MD_LOOP);

	DPRINTF(("sv_trigger_input: current addr=%x\n",
	    bus_space_read_4(sc->sc_iot, sc->sc_dmac_ioh, SV_DMA_ADDR0)));

	dma_count = (blksize >> 1) - 1;

	sv_write_indirect(sc, SV_DMAC_COUNT1, dma_count >> 8);
	sv_write_indirect(sc, SV_DMAC_COUNT0, dma_count & 0xFF);

	mode = sv_read_indirect(sc, SV_PLAY_RECORD_ENABLE);
	sv_write_indirect(sc, SV_PLAY_RECORD_ENABLE, mode | SV_RECORD_ENABLE);

	return (0);
}

int
sv_halt_output(addr)
	void *addr;
{
	struct sv_softc *sc = addr;
	u_int8_t mode;
	
	DPRINTF(("sv: sv_halt_output\n"));
	mode = sv_read_indirect(sc, SV_PLAY_RECORD_ENABLE);
	sv_write_indirect(sc, SV_PLAY_RECORD_ENABLE, mode & ~SV_PLAY_ENABLE);

	return (0);
}

int
sv_halt_input(addr)
	void *addr;
{
	struct sv_softc *sc = addr;
	u_int8_t mode;
    
	DPRINTF(("sv: sv_halt_input\n"));
	mode = sv_read_indirect(sc, SV_PLAY_RECORD_ENABLE);
	sv_write_indirect(sc, SV_PLAY_RECORD_ENABLE, mode & ~SV_RECORD_ENABLE);

	return (0);
}

int
sv_getdev(addr, retp)
	void *addr;
	struct audio_device *retp;
{
	*retp = sv_device;
	return (0);
}


/*
 * Mixer related code is here
 *
 */

#define SV_INPUT_CLASS 0
#define SV_OUTPUT_CLASS 1
#define SV_RECORD_CLASS 2

#define SV_LAST_CLASS 2

static const char *mixer_classes[] = 
	{ AudioCinputs, AudioCoutputs, AudioCrecord };

static const struct {
	u_int8_t   l_port;
	u_int8_t   r_port;
	u_int8_t   mask;
	u_int8_t   class;
	const char *audio;
} ports[] = {
  { SV_LEFT_AUX1_INPUT_CONTROL, SV_RIGHT_AUX1_INPUT_CONTROL, SV_AUX1_MASK,
    SV_INPUT_CLASS, "aux1" },
  { SV_LEFT_CD_INPUT_CONTROL, SV_RIGHT_CD_INPUT_CONTROL, SV_CD_MASK, 
    SV_INPUT_CLASS, AudioNcd },
  { SV_LEFT_LINE_IN_INPUT_CONTROL, SV_RIGHT_LINE_IN_INPUT_CONTROL, SV_LINE_IN_MASK,
    SV_INPUT_CLASS, AudioNline },
  { SV_MIC_INPUT_CONTROL, 0, SV_MIC_MASK, SV_INPUT_CLASS, AudioNmicrophone },
  { SV_LEFT_SYNTH_INPUT_CONTROL, SV_RIGHT_SYNTH_INPUT_CONTROL, 
    SV_SYNTH_MASK, SV_INPUT_CLASS, AudioNfmsynth },
  { SV_LEFT_AUX2_INPUT_CONTROL, SV_RIGHT_AUX2_INPUT_CONTROL, SV_AUX2_MASK,
    SV_INPUT_CLASS, "aux2" },
  { SV_LEFT_PCM_INPUT_CONTROL, SV_RIGHT_PCM_INPUT_CONTROL, SV_PCM_MASK,
    SV_INPUT_CLASS, AudioNdac },
  { SV_LEFT_MIXER_OUTPUT_CONTROL, SV_RIGHT_MIXER_OUTPUT_CONTROL, 
    SV_MIXER_OUT_MASK, SV_OUTPUT_CLASS, AudioNmaster }
};


static const struct {
	int idx;
	const char *name;
} record_sources[] = {
	{ SV_REC_CD, AudioNcd },
	{ SV_REC_DAC, AudioNdac },
	{ SV_REC_AUX2, "aux2" },
	{ SV_REC_LINE, AudioNline },
	{ SV_REC_AUX1, "aux1" },
	{ SV_REC_MIC, AudioNmicrophone },
	{ SV_REC_MIXER, AudioNmixerout }
};


#define SV_DEVICES_PER_PORT 2
#define SV_FIRST_MIXER (SV_LAST_CLASS + 1)
#define SV_LAST_MIXER (SV_DEVICES_PER_PORT * (ARRAY_SIZE(ports)) + SV_LAST_CLASS)
#define SV_RECORD_SOURCE (SV_LAST_MIXER + 1)
#define SV_MIC_BOOST (SV_LAST_MIXER + 2)
#define SV_RECORD_GAIN (SV_LAST_MIXER + 3)
#define SV_SRS_MODE (SV_LAST_MIXER + 4)

int 
sv_query_devinfo(addr, dip)
	void *addr;
	mixer_devinfo_t *dip;
{
	int i;

	/* It's a class */
	if (dip->index <= SV_LAST_CLASS) {
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = dip->index;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, 
		       mixer_classes[dip->index]);
		return (0);
	}

	if (dip->index >= SV_FIRST_MIXER &&
	    dip->index <= SV_LAST_MIXER) {
		int off = dip->index - SV_FIRST_MIXER;
		int mute = (off % SV_DEVICES_PER_PORT);
		int idx = off / SV_DEVICES_PER_PORT;
		
		dip->mixer_class = ports[idx].class;
		strcpy(dip->label.name, ports[idx].audio);
		
		if (!mute) {
			dip->type = AUDIO_MIXER_VALUE;
			dip->prev = AUDIO_MIXER_LAST;
			dip->next = dip->index + 1;

			if (ports[idx].r_port != 0)
				dip->un.v.num_channels = 2;
			else
				dip->un.v.num_channels = 1;
      
			strcpy(dip->un.v.units.name, AudioNvolume);
		} else {
			dip->type = AUDIO_MIXER_ENUM;
			dip->prev = dip->index - 1;
			dip->next = AUDIO_MIXER_LAST;
			
			strcpy(dip->label.name, AudioNmute);
			dip->un.e.num_mem = 2;
			strcpy(dip->un.e.member[0].label.name, AudioNoff);
			dip->un.e.member[0].ord = 0;
			strcpy(dip->un.e.member[1].label.name, AudioNon);
			dip->un.e.member[1].ord = 1;
		}
		
		return (0);
	}

	switch (dip->index) {
	case SV_RECORD_SOURCE:
		dip->mixer_class = SV_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = SV_RECORD_GAIN;
		strcpy(dip->label.name, AudioNsource);
		dip->type = AUDIO_MIXER_ENUM;
		
		dip->un.e.num_mem = ARRAY_SIZE(record_sources);
		for (i = 0; i < ARRAY_SIZE(record_sources); i++) {
			strcpy(dip->un.e.member[i].label.name, 
			       record_sources[i].name);
			dip->un.e.member[i].ord = record_sources[i].idx;
		}
		return (0);

	case SV_RECORD_GAIN:
		dip->mixer_class = SV_RECORD_CLASS;
		dip->prev = SV_RECORD_SOURCE;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, "gain");
		dip->type = AUDIO_MIXER_VALUE;
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return (0);
		
	case SV_MIC_BOOST:
		dip->mixer_class = SV_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, "micboost");
		goto on_off;
		
	case SV_SRS_MODE:
		dip->mixer_class = SV_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNspatial);
		
	on_off:
		dip->type = AUDIO_MIXER_ENUM;
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		return (0);
	}
	
	return (ENXIO);
}

int
sv_mixer_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct sv_softc *sc = addr;
	u_int8_t reg;
	int idx;

	if (cp->dev >= SV_FIRST_MIXER &&
	    cp->dev <= SV_LAST_MIXER) {
		int off = cp->dev - SV_FIRST_MIXER;
		int mute = (off % SV_DEVICES_PER_PORT);
		idx = off / SV_DEVICES_PER_PORT;
		
		if (mute) {
			if (cp->type != AUDIO_MIXER_ENUM) 
				return (EINVAL);

			reg = sv_read_indirect(sc, ports[idx].l_port);
			if (cp->un.ord) 
				reg |= SV_MUTE_BIT;
			else
				reg &= ~SV_MUTE_BIT;
			sv_write_indirect(sc, ports[idx].l_port, reg);
			
			if (ports[idx].r_port) {
				reg = sv_read_indirect(sc, ports[idx].r_port);
				if (cp->un.ord) 
					reg |= SV_MUTE_BIT;
				else
					reg &= ~SV_MUTE_BIT;
				sv_write_indirect(sc, ports[idx].r_port, reg);
			}
		} else {
			int  lval, rval;
			
			if (cp->type != AUDIO_MIXER_VALUE)
				return (EINVAL);
			
			if (cp->un.value.num_channels != 1 &&
			    cp->un.value.num_channels != 2)
				return (EINVAL);
			
			if (ports[idx].r_port == 0) {
				if (cp->un.value.num_channels != 1)
					return (EINVAL);
				lval = cp->un.value.level[AUDIO_MIXER_LEVEL_MONO];
				rval = 0; /* shut up GCC */
			} else {
				if (cp->un.value.num_channels != 2)
					return (EINVAL);
				
				lval = cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
				rval = cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
      }


			reg = sv_read_indirect(sc, ports[idx].l_port);
			reg &= ~(ports[idx].mask);
			lval = (AUDIO_MAX_GAIN - lval) * ports[idx].mask / 
				AUDIO_MAX_GAIN;
			reg |= lval;
			sv_write_indirect(sc, ports[idx].l_port, reg);

			if (ports[idx].r_port != 0) {
				reg = sv_read_indirect(sc, ports[idx].r_port);
				reg &= ~(ports[idx].mask);
				
				rval = (AUDIO_MAX_GAIN - rval) * ports[idx].mask /
					AUDIO_MAX_GAIN;
				reg |= rval;

				sv_write_indirect(sc, ports[idx].r_port, reg);
			}

			sv_read_indirect(sc, ports[idx].l_port);
		}

		return (0);
	}


	switch (cp->dev) {
	case SV_RECORD_SOURCE:
		if (cp->type != AUDIO_MIXER_ENUM)
			return (EINVAL);
		
		for (idx = 0; idx < ARRAY_SIZE(record_sources); idx++) {
			if (record_sources[idx].idx == cp->un.ord)
				goto found;
		}
    
		return (EINVAL);

	found:
		reg = sv_read_indirect(sc, SV_LEFT_ADC_INPUT_CONTROL);
		reg &= ~SV_REC_SOURCE_MASK;
		reg |= (((cp->un.ord) << SV_REC_SOURCE_SHIFT) & SV_REC_SOURCE_MASK);
		sv_write_indirect(sc, SV_LEFT_ADC_INPUT_CONTROL, reg);
		
		reg = sv_read_indirect(sc, SV_RIGHT_ADC_INPUT_CONTROL);
		reg &= ~SV_REC_SOURCE_MASK;
		reg |= (((cp->un.ord) << SV_REC_SOURCE_SHIFT) & SV_REC_SOURCE_MASK);
		sv_write_indirect(sc, SV_RIGHT_ADC_INPUT_CONTROL, reg);
		return (0);
		
	case SV_RECORD_GAIN:
	{
		int val;
		
		if (cp->type != AUDIO_MIXER_VALUE)
			return (EINVAL);
		
		if (cp->un.value.num_channels != 1)
			return (EINVAL);
		
		val = (cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] * SV_REC_GAIN_MASK) 
			/ AUDIO_MAX_GAIN;
		
		reg = sv_read_indirect(sc, SV_LEFT_ADC_INPUT_CONTROL);
		reg &= ~SV_REC_GAIN_MASK;
		reg |= val;
		sv_write_indirect(sc, SV_LEFT_ADC_INPUT_CONTROL, reg);
		
		reg = sv_read_indirect(sc, SV_RIGHT_ADC_INPUT_CONTROL);
		reg &= ~SV_REC_GAIN_MASK;
		reg |= val;
		sv_write_indirect(sc, SV_RIGHT_ADC_INPUT_CONTROL, reg);
	}
	return (0);

	case SV_MIC_BOOST:
		if (cp->type != AUDIO_MIXER_ENUM)
			return (EINVAL);
		
		reg = sv_read_indirect(sc, SV_LEFT_ADC_INPUT_CONTROL);
		if (cp->un.ord) {
			reg |= SV_MIC_BOOST_BIT;
		} else {
			reg &= ~SV_MIC_BOOST_BIT;
		}
		
		sv_write_indirect(sc, SV_LEFT_ADC_INPUT_CONTROL, reg);
		return (0);
		
	case SV_SRS_MODE:
		if (cp->type != AUDIO_MIXER_ENUM)
			return (EINVAL);
		
		reg = sv_read_indirect(sc, SV_SRS_SPACE_CONTROL);
		if (cp->un.ord) {
			reg &= ~SV_SRS_SPACE_ONOFF;
		} else {
			reg |= SV_SRS_SPACE_ONOFF;
		}
		
		sv_write_indirect(sc, SV_SRS_SPACE_CONTROL, reg);
		return (0);
	}
	
	return (EINVAL);
}

int
sv_mixer_get_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct sv_softc *sc = addr;
	int val;
	u_int8_t reg;
	
	if (cp->dev >= SV_FIRST_MIXER &&
	    cp->dev <= SV_LAST_MIXER) {
		int off = cp->dev - SV_FIRST_MIXER;
		int mute = (off % 2);
		int idx = off / 2;
		
		if (mute) {
			if (cp->type != AUDIO_MIXER_ENUM) 
				return (EINVAL);
			
			reg = sv_read_indirect(sc, ports[idx].l_port);
			cp->un.ord = ((reg & SV_MUTE_BIT) ? 1 : 0);
		} else {
			if (cp->type != AUDIO_MIXER_VALUE)
				return (EINVAL);
			
			if (cp->un.value.num_channels != 1 &&
			    cp->un.value.num_channels != 2)
				return (EINVAL);
			
			if ((ports[idx].r_port == 0 &&
			     cp->un.value.num_channels != 1) ||
			    (ports[idx].r_port != 0 &&
			     cp->un.value.num_channels != 2))
				return (EINVAL);
			
			reg = sv_read_indirect(sc, ports[idx].l_port);
			reg &= ports[idx].mask;
			
			val = AUDIO_MAX_GAIN - ((reg * AUDIO_MAX_GAIN) / ports[idx].mask);
			
			if (ports[idx].r_port != 0) {
				cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = val;
				
				reg = sv_read_indirect(sc, ports[idx].r_port);
				reg &= ports[idx].mask;
				
				val = AUDIO_MAX_GAIN - ((reg * AUDIO_MAX_GAIN) / ports[idx].mask);
				cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = val;
			} else 
				cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = val;
		}

		return (0);
  }

	switch (cp->dev) {
	case SV_RECORD_SOURCE:
		if (cp->type != AUDIO_MIXER_ENUM)
			return (EINVAL);
		
		reg = sv_read_indirect(sc, SV_LEFT_ADC_INPUT_CONTROL);
		cp->un.ord = ((reg & SV_REC_SOURCE_MASK) >> SV_REC_SOURCE_SHIFT);
		
		return (0);
		
	case SV_RECORD_GAIN:
		if (cp->type != AUDIO_MIXER_VALUE)
			return (EINVAL);
		if (cp->un.value.num_channels != 1)
			return (EINVAL);
		
		reg = sv_read_indirect(sc, SV_LEFT_ADC_INPUT_CONTROL) & SV_REC_GAIN_MASK;
		cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = 
			(((unsigned int)reg) * AUDIO_MAX_GAIN) / SV_REC_GAIN_MASK;
		
		return (0);
		
	case SV_MIC_BOOST:
		if (cp->type != AUDIO_MIXER_ENUM)
			return (EINVAL);
		reg = sv_read_indirect(sc, SV_LEFT_ADC_INPUT_CONTROL);
		cp->un.ord = ((reg & SV_MIC_BOOST_BIT) ? 1 : 0);
		return (0);
		
		
	case SV_SRS_MODE:
		if (cp->type != AUDIO_MIXER_ENUM)
			return (EINVAL);
		reg = sv_read_indirect(sc, SV_SRS_SPACE_CONTROL);
		cp->un.ord = ((reg & SV_SRS_SPACE_ONOFF) ? 0 : 1);
		return (0);
	}
	
	return (EINVAL);
}


static void
sv_init_mixer(sc)
	struct sv_softc *sc;
{
	mixer_ctrl_t cp;
	int i;
	
	cp.type = AUDIO_MIXER_ENUM;
	cp.dev = SV_SRS_MODE;
	cp.un.ord = 0;

	sv_mixer_set_port(sc, &cp);

	for (i = 0; i < ARRAY_SIZE(ports); i++) {
		if (ports[i].audio == AudioNdac) {
			cp.type = AUDIO_MIXER_ENUM;
			cp.dev = SV_FIRST_MIXER + i * SV_DEVICES_PER_PORT + 1;
			cp.un.ord = 0;
			sv_mixer_set_port(sc, &cp);
			break;
		}
	}
}

void *
sv_malloc(addr, direction, size, pool, flags)
	void *addr;
	int direction;
	size_t size;
	int pool, flags;
{
	struct sv_softc *sc = addr;
	struct sv_dma *p;
	int error;

	p = malloc(sizeof(*p), pool, flags);
	if (!p)
		return (0);
	error = sv_allocmem(sc, size, 16, direction, p);
	if (error) {
		free(p, pool);
		return (0);
	}
	p->next = sc->sc_dmas;
	sc->sc_dmas = p;
	return (KERNADDR(p));
}

void
sv_free(addr, ptr, pool)
	void *addr;
	void *ptr;
	int pool;
{
	struct sv_softc *sc = addr;
	struct sv_dma **pp, *p;

	for (pp = &sc->sc_dmas; (p = *pp) != NULL; pp = &p->next) {
		if (KERNADDR(p) == ptr) {
			sv_freemem(sc, p);
			*pp = p->next;
			free(p, pool);
			return;
		}
	}
}

size_t
sv_round_buffersize(addr, direction, size)
	void *addr;
	int direction;
	size_t size;
{
	return (size);
}

paddr_t
sv_mappage(addr, mem, off, prot)
	void *addr;
	void *mem;
	off_t off;
	int prot;
{
	struct sv_softc *sc = addr;
	struct sv_dma *p;

	if (off < 0)
		return (-1);
	for (p = sc->sc_dmas; p && KERNADDR(p) != mem; p = p->next)
		;
	if (!p)
		return (-1);
	return (bus_dmamem_mmap(sc->sc_dmatag, p->segs, p->nsegs, 
				off, prot, BUS_DMA_WAITOK));
}

int
sv_get_props(addr)
	void *addr;
{
	return (AUDIO_PROP_MMAP | AUDIO_PROP_INDEPENDENT | AUDIO_PROP_FULLDUPLEX);
}
