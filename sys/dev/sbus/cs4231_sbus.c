/*	$NetBSD: cs4231_sbus.c,v 1.26 2003/02/27 13:30:39 pk Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: cs4231_sbus.c,v 1.26 2003/02/27 13:30:39 pk Exp $");

#include "audio.h"
#if NAUDIO > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>

#include <dev/sbus/sbusvar.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/ic/ad1848reg.h>
#include <dev/ic/cs4231reg.h>
#include <dev/ic/ad1848var.h>
#include <dev/ic/cs4231var.h>

#include <dev/ic/apcdmareg.h>

#ifdef AUDIO_DEBUG
int cs4231_sbus_debug = 0;
#define DPRINTF(x)      if (cs4231_sbus_debug) printf x
#else
#define DPRINTF(x)
#endif

/* where APC DMA registers are located */
#define CS4231_APCDMA_OFFSET	16

/* interrupt enable bits except those specific for playback/capture */
#define APC_ENABLE		(APC_EI | APC_IE | APC_EIE)

struct cs4231_sbus_softc {
	struct cs4231_softc sc_cs4231;

	struct sbusdev sc_sd;			/* sbus device */
	bus_space_tag_t sc_bt;			/* DMA controller tag */
	bus_space_handle_t sc_bh;		/* DMA controller registers */
};


static int	cs4231_sbus_match(struct device *, struct cfdata *, void *);
static void	cs4231_sbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(audiocs_sbus, sizeof(struct cs4231_sbus_softc),
    cs4231_sbus_match, cs4231_sbus_attach, NULL, NULL);

/* audio_hw_if methods specific to apc dma */
static int	cs4231_sbus_trigger_output(void *, void *, void *, int,
					   void (*)(void *), void *,
					   struct audio_params *);
static int	cs4231_sbus_trigger_input(void *, void *, void *, int,
					  void (*)(void *), void *,
					  struct audio_params *);
static int	cs4231_sbus_halt_output(void *);
static int	cs4231_sbus_halt_input(void *);

struct audio_hw_if audiocs_sbus_hw_if = {
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
	cs4231_sbus_halt_output,
	cs4231_sbus_halt_input,
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
	cs4231_sbus_trigger_output,
	cs4231_sbus_trigger_input,
	NULL,			/* dev_ioctl */
};


#ifdef AUDIO_DEBUG
static void	cs4231_sbus_regdump(char *, struct cs4231_sbus_softc *);
#endif

static int	cs4231_sbus_intr(void *);



static int
cs4231_sbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	if (strcmp(sa->sa_name, AUDIOCS_PROM_NAME) != 0)
		return (0);

	/*
	 * Some machine have a SUNW,CS4231 node, but no hardware.
	 * It seems these can be identified by a `serial' device type.
	 */
	if (strcmp(PROM_getpropstring(sa->sa_node, "device_type"),
		   "serial") == 0)
		return (0);
	return (1);
}


static void
cs4231_sbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cs4231_sbus_softc *sbsc = (struct cs4231_sbus_softc *)self;
	struct cs4231_softc *sc = &sbsc->sc_cs4231;
	struct sbus_attach_args *sa = aux;
	bus_space_handle_t bh;

	sbsc->sc_bt = sc->sc_bustag = sa->sa_bustag;
	sc->sc_dmatag = sa->sa_dmatag;

	/*
	 * Map my registers in, if they aren't already in virtual
	 * address space.
	 */
	if (sa->sa_npromvaddrs) {
		sbus_promaddr_to_handle(sa->sa_bustag,
			sa->sa_promvaddrs[0], &bh);
	} else {
		if (sbus_bus_map(sa->sa_bustag,	sa->sa_slot,
			sa->sa_offset, sa->sa_size, 0, &bh) != 0) {
			printf("%s @ sbus: cannot map registers\n",
				self->dv_xname);
			return;
		}
	}

	bus_space_subregion(sa->sa_bustag, bh, CS4231_APCDMA_OFFSET,
		sizeof(struct apc_dma), &sbsc->sc_bh);

	cs4231_common_attach(sc, bh);
	printf("\n");

	sbus_establish(&sbsc->sc_sd, &sc->sc_ad1848.sc_dev);

	/* Establish interrupt channel */
	if (sa->sa_nintr)
		bus_intr_establish(sa->sa_bustag,
				   sa->sa_pri, IPL_AUDIO,
				   cs4231_sbus_intr, sbsc);

	audio_attach_mi(&audiocs_sbus_hw_if, sbsc, &sc->sc_ad1848.sc_dev);
}


#ifdef AUDIO_DEBUG
static void
cs4231_sbus_regdump(label, sc)
	char *label;
	struct cs4231_sbus_softc *sc;
{
	char bits[128];
	volatile struct apc_dma *dma = NULL;

	printf("cs4231regdump(%s): regs:", label);
	printf("dmapva: 0x%x; ",
		bus_space_read_4(sc->sc_bh, sc->sc_bh, &dma->dmapva));
	printf("dmapc: 0x%x; ",
		bus_space_read_4(sc->sc_bh, sc->sc_bh, &dma->dmapc));
	printf("dmapnva: 0x%x; ",
		bus_space_read_4(sc->sc_bh, sc->sc_bh, &dma->dmapnva));
	printf("dmapnc: 0x%x\n",
		bus_space_read_4(sc->sc_bh, sc->sc_bh, &dma->dmapnc));
	printf("dmacva: 0x%x; ", 
		bus_space_read_4(sc->sc_bh, sc->sc_bh, &dma->dmacva));
	printf("dmacc: 0x%x; ", 
		bus_space_read_4(sc->sc_bh, sc->sc_bh, &dma->dmacc));
	printf("dmacnva: 0x%x; ", 
		bus_space_read_4(sc->sc_bh, sc->sc_bh, &dma->dmacnva));
	printf("dmacnc: 0x%x\n", 
		bus_space_read_4(sc->sc_bh, sc->sc_bh, &dma->dmacnc));

	printf("apc_dmacsr=%s\n",
		bitmask_snprintf(
			bus_space_read_4(sc->sc_bh, sc->sc_bh, &dma->dmacsr),
				APC_BITS, bits, sizeof(bits)));

	ad1848_dump_regs(&sc->sc_cs4231.sc_ad1848);
}
#endif /* AUDIO_DEBUG */


static int
cs4231_sbus_trigger_output(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr)(void *);
	void *arg;
	struct audio_params *param;
{
	struct cs4231_sbus_softc *sbsc = addr;
	struct cs4231_softc *sc = &sbsc->sc_cs4231;
	struct cs_transfer *t = &sc->sc_playback;
	volatile struct apc_dma *dma = NULL;
	u_int32_t csr;
	bus_addr_t dmaaddr;
	bus_size_t dmasize;
	int ret;
#ifdef AUDIO_DEBUG
	char bits[128];
#endif

	ret = cs4231_transfer_init(sc, t, &dmaaddr, &dmasize,
				   start, end, blksize, intr, arg);
	if (ret != 0)
		return (ret);

	DPRINTF(("trigger_output: was: %x %d, %x %d\n",
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapva), 
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapc),
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapnva),
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapnc)));

	/* load first block */
	bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapnva, dmaaddr);
	bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapnc, dmasize);

	DPRINTF(("trigger_output: 1st: %x %d, %x %d\n",
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapva),
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapc),
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapnva),
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapnc)));

	csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr);
	DPRINTF(("trigger_output: csr=%s\n",
		 bitmask_snprintf(csr, APC_BITS, bits, sizeof(bits))));
	if ((csr & PDMA_GO) == 0 || (csr & APC_PPAUSE) != 0) {
		int cfg;

		csr &= ~(APC_PPAUSE | APC_PMIE | APC_INTR_MASK);
		bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr, csr);

		csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr);
		csr &= ~APC_INTR_MASK;
		csr |= APC_ENABLE | APC_PIE | APC_PMIE | PDMA_GO;
		bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr, csr);

		ad_write(&sc->sc_ad1848, SP_LOWER_BASE_COUNT, 0xff);
		ad_write(&sc->sc_ad1848, SP_UPPER_BASE_COUNT, 0xff);

		cfg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
		ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG,
			 (cfg | PLAYBACK_ENABLE));
	} else {
		DPRINTF(("trigger_output: already: csr=%s\n",
			 bitmask_snprintf(csr, APC_BITS, bits, sizeof(bits))));
	}

	/* load next block if we can */
	csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr);
	if (csr & APC_PD) {
		cs4231_transfer_advance(t, &dmaaddr, &dmasize);
		bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapnva, dmaaddr);
		bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapnc, dmasize);

		DPRINTF(("trigger_output: 2nd: %x %d, %x %d\n",
		    bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapva),
		    bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapc),
		    bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapnva),
		    bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmapnc)));
	}

	return (0);
}


static int
cs4231_sbus_halt_output(addr)
	void *addr;
{
	struct cs4231_sbus_softc *sbsc = addr;
	struct cs4231_softc *sc = &sbsc->sc_cs4231;
	volatile struct apc_dma *dma = NULL;
	u_int32_t csr;
	int cfg;
#ifdef AUDIO_DEBUG
	char bits[128];
#endif

	sc->sc_playback.t_active = 0;

	csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr);
	DPRINTF(("halt_output: csr=%s\n",
		 bitmask_snprintf(csr, APC_BITS, bits, sizeof(bits))));

	csr &= ~APC_INTR_MASK;	/* do not clear interrupts accidentally */
	csr |= APC_PPAUSE;	/* pause playback (let current complete) */
	bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr, csr);
	
	/* let the curernt transfer complete */
	if (csr & PDMA_GO)
		do {
			csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, 
				(vaddr_t)&dma->dmacsr);
			DPRINTF(("halt_output: csr=%s\n",
				 bitmask_snprintf(csr, APC_BITS,
						  bits, sizeof(bits))));
		} while ((csr & APC_PM) == 0);

	cfg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG,(cfg & ~PLAYBACK_ENABLE));

	return (0);
}


/* NB: we don't enable APC_CMIE and won't use APC_CM */
static int
cs4231_sbus_trigger_input(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr)(void *);
	void *arg;
	struct audio_params *param;
{
	struct cs4231_sbus_softc *sbsc = addr;
	struct cs4231_softc *sc = &sbsc->sc_cs4231;
	struct cs_transfer *t = &sc->sc_capture;
	volatile struct apc_dma *dma = NULL;
	u_int32_t csr;
	bus_addr_t dmaaddr;
	bus_size_t dmasize;
	int ret;
#ifdef AUDIO_DEBUG
	char bits[128];
#endif

	ret = cs4231_transfer_init(sc, t, &dmaaddr, &dmasize,
				   start, end, blksize, intr, arg);
	if (ret != 0)
		return (ret);

	csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr);
	DPRINTF(("trigger_input: csr=%s\n",
		 bitmask_snprintf(csr, APC_BITS, bits, sizeof(bits))));
	DPRINTF(("trigger_input: was: %x %d, %x %d\n",
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacva), 
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacc),
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacnva),
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacnc)));

	/* supply first block */
	bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacnva, dmaaddr);
	bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacnc, dmasize);

	DPRINTF(("trigger_input: 1st: %x %d, %x %d\n",
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacva), 
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacc),
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacnva),
		bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacnc)));

	csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr);
	if ((csr & CDMA_GO) == 0 || (csr & APC_CPAUSE) != 0) {
		int cfg;

		csr &= ~(APC_CPAUSE | APC_CMIE | APC_INTR_MASK);
		bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr, csr);

		csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr);
		csr &= ~APC_INTR_MASK;
		csr |= APC_ENABLE | APC_CIE | CDMA_GO;
		bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr, csr);

		ad_write(&sc->sc_ad1848, CS_LOWER_REC_CNT, 0xff);
		ad_write(&sc->sc_ad1848, CS_UPPER_REC_CNT, 0xff);

		cfg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
		ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG,
			 (cfg | CAPTURE_ENABLE));
	} else {
		DPRINTF(("trigger_input: already: csr=%s\n",
			 bitmask_snprintf(csr, APC_BITS, bits, sizeof(bits))));
	}

	/* supply next block if we can */
	csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr);
	if (csr & APC_CD) {
		cs4231_transfer_advance(t, &dmaaddr, &dmasize);
		bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacnva, dmaaddr);
		bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacnc, dmasize);
		DPRINTF(("trigger_input: 2nd: %x %d, %x %d\n",
		    bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacva),
		    bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacc),
		    bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacnva),
		    bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacnc)));
	}

	return (0);
}


static int
cs4231_sbus_halt_input(addr)
	void *addr;
{
	struct cs4231_sbus_softc *sbsc = addr;
	struct cs4231_softc *sc = &sbsc->sc_cs4231;
	volatile struct apc_dma *dma = NULL;
	u_int32_t csr;
	int cfg;
#ifdef AUDIO_DEBUG
	char bits[128];
#endif

	sc->sc_capture.t_active = 0;

	csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr);
	DPRINTF(("halt_input: csr=%s\n",
		 bitmask_snprintf(csr, APC_BITS, bits, sizeof(bits))));

	csr &= ~APC_INTR_MASK;	/* do not clear interrupts accidentally */
	csr |= APC_CPAUSE;
	bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr, csr);

	/* let the curernt transfer complete */
	if (csr & CDMA_GO)
		do {
			csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh,
				(vaddr_t)&dma->dmacsr);
			DPRINTF(("halt_input: csr=%s\n",
				 bitmask_snprintf(csr, APC_BITS,
						  bits, sizeof(bits))));
		} while ((csr & APC_CM) == 0);

	cfg = ad_read(&sc->sc_ad1848, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad1848, SP_INTERFACE_CONFIG, (cfg & ~CAPTURE_ENABLE));

	return (0);
}


static int
cs4231_sbus_intr(arg)
	void *arg;
{
	struct cs4231_sbus_softc *sbsc = arg;
	struct cs4231_softc *sc = &sbsc->sc_cs4231;
	volatile struct apc_dma *dma = NULL;
	u_int32_t csr;
	int status;
	bus_addr_t dmaaddr;
	bus_size_t dmasize;
	int served;
#if defined(AUDIO_DEBUG) || defined(DIAGNOSTIC)
	char bits[128];
#endif

	csr = bus_space_read_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr);
	if ((csr & APC_INTR_MASK) == 0)	/* any interrupt pedning? */
		return (0);

	/* write back DMA status to clear interrupt */
	bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh, (vaddr_t)&dma->dmacsr, csr);
	++sc->sc_intrcnt.ev_count;
	served = 0;

#ifdef AUDIO_DEBUG
	if (cs4231_sbus_debug > 1)
		cs4231_sbus_regdump("audiointr", sbsc);
#endif

	status = ADREAD(&sc->sc_ad1848, AD1848_STATUS);
	DPRINTF(("%s: status: %s\n", sc->sc_ad1848.sc_dev.dv_xname,
		bitmask_snprintf(status, AD_R2_BITS, bits, sizeof(bits))));
	if (status & INTERRUPT_STATUS) {
#ifdef AUDIO_DEBUG
		int reason;

		reason = ad_read(&sc->sc_ad1848, CS_IRQ_STATUS);
		DPRINTF(("%s: i24: %s\n", sc->sc_ad1848.sc_dev.dv_xname,
		  bitmask_snprintf(reason, CS_I24_BITS, bits, sizeof(bits))));
#endif
		/* clear ad1848 interrupt */
		ADWRITE(&sc->sc_ad1848, AD1848_STATUS, 0);
	}
	
	if (csr & APC_CI) {
		if (csr & APC_CD) { /* can supply new block */
			struct cs_transfer *t = &sc->sc_capture;

			cs4231_transfer_advance(t, &dmaaddr, &dmasize);
			bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh,
				(vaddr_t)&dma->dmacnva, dmaaddr);
			bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh,
				(vaddr_t)&dma->dmacnc, dmasize);

			if (t->t_intr != NULL)
				(*t->t_intr)(t->t_arg);
			++t->t_intrcnt.ev_count;
			served = 1;
		}
	}

	if (csr & APC_PMI) {
		if (!sc->sc_playback.t_active)
			served = 1; /* draining in halt_output() */
	}

	if (csr & APC_PI) {
		if (csr & APC_PD) { /* can load new block */
			struct cs_transfer *t = &sc->sc_playback;

			if (t->t_active) {
				cs4231_transfer_advance(t, &dmaaddr, &dmasize);
				bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh,
					(vaddr_t)&dma->dmapnva, dmaaddr);
				bus_space_write_4(sbsc->sc_bt, sbsc->sc_bh,
					(vaddr_t)&dma->dmapnc, dmasize);
			}

			if (t->t_intr != NULL)
				(*t->t_intr)(t->t_arg);
			++t->t_intrcnt.ev_count;
			served = 1;
		}
	}

	/* got an interrupt we don't know how to handle */
	if (!served) {
#ifdef DIAGNOSTIC
		printf("%s: unhandled csr=%s\n", sc->sc_ad1848.sc_dev.dv_xname,
		       bitmask_snprintf(csr, APC_BITS, bits, sizeof(bits)));
#endif
		/* evcnt? */
	}

	return (1);
}

#endif /* NAUDIO > 0 */
