/*	$NetBSD: cs4231_ebus.c,v 1.2 2002/03/12 06:00:42 uwe Exp $ */

/*
 * Copyright (c) 2002 Valeriy E. Ushakov
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/ic/ad1848reg.h>
#include <dev/ic/cs4231reg.h>
#include <dev/ic/ad1848var.h>
#include <dev/ic/cs4231var.h>

#ifdef AUDIO_DEBUG
int cs4231_ebus_debug = 0;
#define DPRINTF(x)	if (cs4231_ebus_debug) printf x
#else
#define DPRINTF(x)
#endif


struct cs4231_ebus_softc {
	struct cs4231_softc sc_cs4231;

	volatile struct	ebus_dmac_reg	*sc_pdmareg; /* playback DMA */
	volatile struct	ebus_dmac_reg	*sc_rdmareg; /* record DMA */
};


void	cs4231_ebus_attach(struct device *, struct device *, void *);
int	cs4231_ebus_match(struct device *, struct cfdata *, void *);

struct cfattach audiocs_ebus_ca = {
	sizeof(struct cs4231_ebus_softc), cs4231_ebus_match, cs4231_ebus_attach
};


/* audio_hw_if methods specific to ebus dma */
static int	cs4231_ebus_trigger_output(void *, void *, void *, int,
					   void (*)(void *), void *,
					   struct audio_params *);
static int	cs4231_ebus_trigger_input(void *, void *, void *, int,
					  void (*)(void *), void *,
					  struct audio_params *);
static int	cs4231_ebus_halt_output(void *);
static int	cs4231_ebus_halt_input(void *);

struct audio_hw_if audiocs_ebus_hw_if = {
	cs4231_open,
	cs4231_close,
	NULL,			/* drain */
	ad1848_query_encoding,
	ad1848_set_params,
	cs4231_round_blocksize,
	ad1848_commit_settings,
	NULL,			/* init_output */
	NULL,			/* init_input */
	NULL,			/* start_output */
	NULL,			/* start_input */
	cs4231_ebus_halt_output,
	cs4231_ebus_halt_input,
	NULL,			/* speaker_ctl */
	cs4231_getdev,
	NULL,			/* setfd */
	cs4231_set_port,
	cs4231_get_port,
	cs4231_query_devinfo,
	cs4231_malloc,
	cs4231_free,
	cs4231_round_buffersize,
	NULL,			/* mappage */
	cs4231_get_props,
	cs4231_ebus_trigger_output,
	cs4231_ebus_trigger_input,
	NULL,			/* dev_ioctl */
};

#ifdef AUDIO_DEBUG
static void	cs4231_ebus_regdump(char *, struct cs4231_ebus_softc *);
#endif

static int	cs4231_ebus_dma_reset(volatile struct ebus_dmac_reg *);
static int	cs4231_ebus_trigger_transfer(struct cs4231_softc *,
			struct cs_transfer *, volatile struct ebus_dmac_reg *,
			int, void *, void *, int, void (*)(void *), void *,
			struct audio_params *);
static void	cs4231_ebus_dma_advance(struct cs_transfer *,
					volatile struct ebus_dmac_reg *);
static int	cs4231_ebus_dma_intr(struct cs_transfer *,
				     volatile struct ebus_dmac_reg *);
static int	cs4231_ebus_intr(void *);


int
cs4231_ebus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ebus_attach_args *ea = aux;

	if (strcmp(ea->ea_name, AUDIOCS_PROM_NAME) == 0)
		return (1);
#ifdef __sparc__		/* XXX: Krups */
	if (strcmp(ea->ea_name, "sound") == 0)
		return (1);
#endif

	return (0);
}


void
cs4231_ebus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cs4231_ebus_softc *ebsc = (struct cs4231_ebus_softc *)self;
	struct cs4231_softc *sc = &ebsc->sc_cs4231;
	struct ebus_attach_args *ea = aux;
	bus_space_handle_t bh;
	int i;

	sc->sc_bustag = ea->ea_bustag;
	sc->sc_dmatag = ea->ea_dmatag;

	/*
	 * These are the register we get from the prom:
	 *	- CS4231 registers
	 *	- Playback EBus DMA controller
	 *	- Capture EBus DMA controller
	 *	- AUXIO audio register (codec powerdown)
	 *
	 * Map my registers in, if they aren't already in virtual
	 * address space.
	 */
	if (ea->ea_nvaddr) {
		bh = (bus_space_handle_t)ea->ea_vaddr[0];
	} else {
		if (bus_space_map(ea->ea_bustag,
				  EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
				  ea->ea_reg[0].size,
				  BUS_SPACE_MAP_LINEAR,
				  &bh) != 0)
		{
			printf("%s: unable to map registers\n",
			       self->dv_xname);
			return;
		}
	}

	/* XXX: map playback DMA registers (we just know where they are) */
	if (bus_space_map(ea->ea_bustag,
			  BUS_ADDR(0x14, 0x702000), /* XXX: magic num */
			  sizeof(struct ebus_dmac_reg),
			  BUS_SPACE_MAP_LINEAR,
			  (bus_space_handle_t *)&ebsc->sc_pdmareg) != 0)
	{
		printf("%s: unable to map playback DMA registers\n",
		       self->dv_xname);
		return;
	}

	/* XXX: map capture DMA registers (we just know where they are) */
	if (bus_space_map(ea->ea_bustag,
			  BUS_ADDR(0x14, 0x704000), /* XXX: magic num */
			  sizeof(struct ebus_dmac_reg),
			  BUS_SPACE_MAP_LINEAR,
			  (bus_space_handle_t *)&ebsc->sc_rdmareg) != 0)
	{
		printf("%s: unable to map capture DMA registers\n",
		       self->dv_xname);
		return;
	}

	/* establish interrupt channels */
	for (i = 0; i < ea->ea_nintr; ++i)
		bus_intr_establish(ea->ea_bustag,
				   ea->ea_intr[i], IPL_AUDIO, 0,
				   cs4231_ebus_intr, ebsc);

	cs4231_common_attach(sc, bh);
	printf("\n");

	/* XXX: todo: move to cs4231_common_attach, pass hw_if as arg? */
	audio_attach_mi(&audiocs_ebus_hw_if, sc, &sc->sc_ad1848.sc_dev);
}


#ifdef AUDIO_DEBUG
static void
cs4231_ebus_regdump(label, ebsc)
	char *label;
	struct cs4231_ebus_softc *ebsc;
{
	/* char bits[128]; */

	printf("cs4231regdump(%s): regs:", label);
	/* XXX: dump ebus dma and aux registers */
	ad1848_dump_regs(&ebsc->sc_cs4231.sc_ad1848);
}
#endif /* AUDIO_DEBUG */


/* XXX: nothing CS4231-specific in this code... */
static int
cs4231_ebus_dma_reset(dmac)
	volatile struct ebus_dmac_reg *dmac;
{
	int timo;

	dmac->dcsr = EBDMA_RESET | EBDMA_TC; /* also clear TC, just in case */

	for (timo = 50000; timo != 0; --timo)
		if ((dmac->dcsr & (EBDMA_CYC_PEND | EBDMA_DRAIN)) == 0)
			break;

	if (timo == 0) {
		printf("cs4231_ebus_dma_reset: dcsr = %x, reset timed out\n",
		       dmac->dcsr);
		return (ETIMEDOUT);
	}

	dmac->dcsr &= ~EBDMA_RESET;
	return (0);
}


static void
cs4231_ebus_dma_advance(t, dmac)
	struct cs_transfer *t;
	volatile struct ebus_dmac_reg *dmac;
{
	bus_addr_t dmaaddr;
	bus_size_t dmasize;

	cs4231_transfer_advance(t, &dmaaddr, &dmasize);
	dmac->dbcr = (u_int32_t)dmasize;
	dmac->dacr = (u_int32_t)dmaaddr;
}


/*
 * Trigger transfer "t" using DMA controller "dmac".
 * "iswrite" defines direction of the transfer.
 */
static int
cs4231_ebus_trigger_transfer(sc, t, dmac, iswrite,
			     start, end, blksize,
			     intr, arg, param)
	struct cs4231_softc *sc;
	struct cs_transfer *t;
	volatile struct ebus_dmac_reg *dmac;
	int iswrite;
	void *start, *end;
	int blksize;
	void (*intr)(void *);
	void *arg;
	struct audio_params *param;
{
	bus_addr_t dmaaddr;
	bus_size_t dmasize;
	int ret;

	ret = cs4231_transfer_init(sc, t, &dmaaddr, &dmasize,
				   start, end, blksize, intr, arg);
	if (ret != 0)
		return (ret);

	ret = cs4231_ebus_dma_reset(dmac);
	if (ret != 0)
		return (ret);

	dmac->dcsr |= EBDMA_EN_NEXT | (iswrite ? EBDMA_WRITE : 0)
		| EBDMA_EN_DMA | EBDMA_EN_CNT | EBDMA_INT_EN;

	/* first load: goes to DACR/DBCR */
	dmac->dbcr = (u_int32_t)dmasize;
	dmac->dacr = (u_int32_t)dmaaddr;

	/* next load: goes to DNAR/DNBR */
	cs4231_ebus_dma_advance(t, dmac);

	return (0);
}


static int
cs4231_ebus_trigger_output(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr)(void *);
	void *arg;
	struct audio_params *param;
{
	struct cs4231_ebus_softc *ebsc = addr;
	struct cs4231_softc *sc = &ebsc->sc_cs4231;
	struct cs_transfer *t = &sc->sc_playback;
	volatile struct ebus_dmac_reg *dma = ebsc->sc_pdmareg;
	int cfg;
	int ret;

	ret = cs4231_ebus_trigger_transfer(sc, t, dma, 0,
					   start, end, blksize,
					   intr, arg,
					   param);
	if (ret != 0)
		return (ret);

	ad_write(&sc->sc_ad1848, SP_LOWER_BASE_COUNT, 0xff);
	ad_write(&sc->sc_ad1848, SP_UPPER_BASE_COUNT, 0xff);

	cfg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG, (cfg | PLAYBACK_ENABLE));

	return (0);
}


static int
cs4231_ebus_trigger_input(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr)(void *);
	void *arg;
	struct audio_params *param;
{
	struct cs4231_ebus_softc *ebsc = addr;
	struct cs4231_softc *sc = &ebsc->sc_cs4231;
	struct cs_transfer *t = &sc->sc_capture;
	volatile struct ebus_dmac_reg *dmac = ebsc->sc_rdmareg;
	int cfg;
	int ret;

	ret = cs4231_ebus_trigger_transfer(sc, t, dmac, 1,
					   start, end, blksize,
					   intr, arg,
					   param);
	if (ret != 0)
		return (ret);

	ad_write(&sc->sc_ad1848, CS_LOWER_REC_CNT, 0xff);
	ad_write(&sc->sc_ad1848, CS_UPPER_REC_CNT, 0xff);

	cfg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG, (cfg | CAPTURE_ENABLE));

	return (0);
}


static int
cs4231_ebus_halt_output(addr)
	void *addr;
{
	struct cs4231_ebus_softc *ebsc = (struct cs4231_ebus_softc *)addr;
	struct cs4231_softc *sc = &ebsc->sc_cs4231;
	int cfg;

	sc->sc_playback.t_active = 0;
	ebsc->sc_pdmareg->dcsr &= ~EBDMA_EN_DMA;

	cfg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG,(cfg & ~PLAYBACK_ENABLE));

	return (0);
}


static int
cs4231_ebus_halt_input(addr)
	void *addr;
{
	struct cs4231_ebus_softc *ebsc = (struct cs4231_ebus_softc *)addr;
	struct cs4231_softc *sc = &ebsc->sc_cs4231;
	int cfg;

	sc->sc_capture.t_active = 0;
	ebsc->sc_pdmareg->dcsr &= ~EBDMA_EN_DMA;

	cfg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG, (cfg & ~CAPTURE_ENABLE));

	return (0);
}


static int
cs4231_ebus_dma_intr(t, dmac)
	struct cs_transfer *t;
	volatile struct ebus_dmac_reg *dmac;
{
	u_int32_t csr;
#ifdef AUDIO_DEBUG
	char bits[128];
#endif

	/* read DMA status, clear TC bit by writing it back */
	csr = dmac->dcsr;
	dmac->dcsr = csr;
	DPRINTF(("audiocs: %s dcsr=%s\n", t->t_name,
		 bitmask_snprintf(csr, EBUS_DCSR_BITS, bits, sizeof(bits))));

	if (csr & EBDMA_ERR_PEND) {
		++t->t_ierrcnt.ev_count;
		printf("audiocs: %s DMA error, resetting\n", t->t_name);
		cs4231_ebus_dma_reset(dmac);
		/* how to notify audio(9)??? */
		return (1);
	}

	if ((csr & EBDMA_INT_PEND) == 0)
		return (0);

	++t->t_intrcnt.ev_count;

	if ((csr & EBDMA_TC) == 0) { /* can this happen? */
		printf("audiocs: %s INT_PEND but !TC\n", t->t_name);
		return (1);
	}

	if (!t->t_active)
		return (1);

	cs4231_ebus_dma_advance(t, dmac);

	/* call audio(9) framework while dma is chugging along */
	if (t->t_intr != NULL)
		(*t->t_intr)(t->t_arg);
	return (1);
}


static int
cs4231_ebus_intr(arg)
	void *arg;
{
	struct cs4231_ebus_softc *ebsc = (struct cs4231_ebus_softc *)arg;
	struct cs4231_softc *sc = &ebsc->sc_cs4231;
	int status;
	int ret;
#ifdef AUDIO_DEBUG
	char bits[128];
#endif

	status = ADREAD(&sc->sc_ad1848, AD1848_STATUS);

#ifdef AUDIO_DEBUG
	if (cs4231_ebus_debug > 1)
		cs4231_ebus_regdump("audiointr", ebsc);

	DPRINTF(("%s: status: %s\n", sc->sc_ad1848.sc_dev.dv_xname,
		 bitmask_snprintf(status, AD_R2_BITS, bits, sizeof(bits))));
#endif

	if (status & INTERRUPT_STATUS) {
#ifdef AUDIO_DEBUG
		int reason;

		reason = ad_read(&sc->sc_ad1848, CS_IRQ_STATUS);
		DPRINTF(("%s: i24: %s\n", sc->sc_ad1848.sc_dev.dv_xname,
		  bitmask_snprintf(reason, CS_I24_BITS, bits, sizeof(bits))));
#endif
		/* clear interrupt from ad1848 */
		ADWRITE(&sc->sc_ad1848, AD1848_STATUS, 0);
	}

	ret = 0;

	if (cs4231_ebus_dma_intr(&sc->sc_capture, ebsc->sc_rdmareg)) {
		++sc->sc_intrcnt.ev_count;
		ret = 1;
	}

	if (cs4231_ebus_dma_intr(&sc->sc_playback, ebsc->sc_pdmareg)) {
		++sc->sc_intrcnt.ev_count;
		ret = 1;
	}

	return (ret);
}
