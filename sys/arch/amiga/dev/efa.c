/*	$NetBSD: efa.c,v 1.15 2017/10/20 07:06:06 jdolecek Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * Driver for FastATA 1200 EIDE controller, manufactured by ELBOX Computer. 
 *
 * Gayle-related stuff inspired by wdc_amiga.c written by Michael L. Hitch
 * and Aymeric Vincent. 
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/kthread.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <sys/bswap.h>

#include <amiga/amiga/cia.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/gayle.h>
#include <amiga/dev/zbusvar.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <amiga/dev/efareg.h>
#include <amiga/dev/efavar.h>

#define EFA_32BIT_IO 1
/* #define EFA_NO_INTR 1 */
/* #define EFA_DEBUG 1 */

int		efa_probe(device_t, cfdata_t, void *);
void		efa_attach(device_t, device_t, void *);
int		efa_intr(void *);
int		efa_intr_soft(void *arg);
static void	efa_set_opts(struct efa_softc *sc);
static bool	efa_mapbase(struct efa_softc *sc);
static bool	efa_mapreg_gayle(struct efa_softc *sc);
static bool	efa_mapreg_native(struct efa_softc *sc);
static void	efa_fata_subregion_pio0(struct wdc_regs *wdr_fata);
static void	efa_fata_subregion_pion(struct wdc_regs *wdr_fata, bool data32);
static void	efa_setup_channel(struct ata_channel *chp);
static void	efa_attach_channel(struct efa_softc *sc, int i);
static void	efa_select_regset(struct efa_softc *sc, int chnum, 
		    uint8_t piomode);
static void	efa_poll_kthread(void *arg);
static bool	efa_compare_status(void);
#ifdef EFA_DEBUG
static void	efa_debug_print_regmapping(struct wdc_regs *wdr_fata);
#endif /* EFA_DEBUG */

CFATTACH_DECL_NEW(efa, sizeof(struct efa_softc),
    efa_probe, efa_attach, NULL, NULL);

#define PIO_NSUPP		0xFFFFFFFF

static const bus_addr_t		pio_offsets[] = 
    { FATA1_PIO0_OFF, PIO_NSUPP, PIO_NSUPP, FATA1_PIO3_OFF, FATA1_PIO4_OFF, 
      FATA1_PIO5_OFF };
static const unsigned int	wdr_offsets_pio0[] = 
    { FATA1_PIO0_OFF_DATA, FATA1_PIO0_OFF_ERROR, FATA1_PIO0_OFF_SECCNT,
      FATA1_PIO0_OFF_SECTOR, FATA1_PIO0_OFF_CYL_LO, FATA1_PIO0_OFF_CYL_HI,
      FATA1_PIO0_OFF_SDH, FATA1_PIO0_OFF_COMMAND };
static const unsigned int	wdr_offsets_pion[] =
    { FATA1_PION_OFF_DATA, FATA1_PION_OFF_ERROR, FATA1_PION_OFF_SECCNT,
      FATA1_PION_OFF_SECTOR, FATA1_PION_OFF_CYL_LO, FATA1_PION_OFF_CYL_HI,
      FATA1_PION_OFF_SDH, FATA1_PION_OFF_COMMAND };

int
efa_probe(device_t parent, cfdata_t cfp, void *aux)
{
	/*
	 * FastATA 1200 uses portions of Gayle IDE interface, and efa driver 
	 * can't coexist with wdc_amiga. Match "wdc" on an A1200, because 
	 * FastATA 1200 does not autoconfigure. 
	 */
	if (!matchname(aux, "wdc") || !is_a1200())
		return(0);

	if (!efa_compare_status())
		return(0);

#ifdef EFA_DEBUG
	aprint_normal("efa_probe succeeded\n");
#endif /* EFA_DEBUG */

	return 100;
}

void
efa_attach(device_t parent, device_t self, void *aux)
{
	int i; 
	struct efa_softc *sc = device_private(self);

	aprint_normal(": ELBOX FastATA 1200\n");

	gayle_init();

	efa_set_opts(sc);

	if (!efa_mapbase(sc)) {
		aprint_error_dev(self, "couldn't map base addresses\n");
		return;
	}
	if (!efa_mapreg_gayle(sc)) {
		aprint_error_dev(self, "couldn't map Gayle registers\n");
		return;
	}
	if (!efa_mapreg_native(sc)) {
		aprint_error_dev(self, "couldn't map FastATA regsters\n");
		return;
	}

	sc->sc_wdcdev.sc_atac.atac_pio_cap = 5; 
	sc->sc_wdcdev.sc_atac.atac_nchannels = FATA1_CHANNELS;
	sc->sc_wdcdev.sc_atac.atac_set_modes = efa_setup_channel;
	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_cap = ATAC_CAP_DATA16;
	sc->sc_wdcdev.wdc_maxdrives = 2;

	if (sc->sc_32bit_io)
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA32;

	/*
	 * The following should work for polling mode, but it does not.
	 * if (sc->sc_no_intr) 
	 *	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_NOIRQ;
	 */

	wdc_allocate_regs(&sc->sc_wdcdev);

	for (i = 0; i < FATA1_CHANNELS; i++) 
		efa_attach_channel(sc, i);

	if (sc->sc_no_intr) {
		sc->sc_fata_softintr = softint_establish(SOFTINT_BIO,
		    (void (*)(void *))efa_intr_soft, sc);
		if (sc->sc_fata_softintr == NULL) {
			aprint_error_dev(self, "couldn't create soft intr\n");
			return;
		}
		if (kthread_create(PRI_NONE, 0, NULL, efa_poll_kthread, sc,
		    NULL, "efa")) {
			aprint_error_dev(self, "couldn't create kthread\n");
			return;
		}
	} else {
		sc->sc_isr.isr_intr = efa_intr;
		sc->sc_isr.isr_arg = sc;
		sc->sc_isr.isr_ipl = 2;
		add_isr (&sc->sc_isr);
		gayle_intr_enable_set(GAYLE_INT_IDE);
	}

}

static void
efa_attach_channel(struct efa_softc *sc, int chnum) 
{
#ifdef EFA_DEBUG
	device_t self;

	self = sc->sc_wdcdev.sc_atac.atac_dev;
#endif /* EFA_DEBUG */

	sc->sc_chanlist[chnum] = &sc->sc_ports[chnum].chan;

	sc->sc_ports[chnum].chan.ch_channel = chnum;
	sc->sc_ports[chnum].chan.ch_atac = &sc->sc_wdcdev.sc_atac;

	if (!sc->sc_32bit_io)
		efa_select_regset(sc, chnum, 0); /* Start in PIO0. */
	else
		efa_select_regset(sc, chnum, 3); 

	wdc_init_shadow_regs(CHAN_TO_WDC_REGS(&sc->sc_ports[chnum].chan));

	wdcattach(&sc->sc_ports[chnum].chan);	

#ifdef EFA_DEBUG
	aprint_normal_dev(self, "done init for channel %d\n", chnum);
#endif

}

/* TODO: convert to callout(9) */ 
static void
efa_poll_kthread(void *arg)
{
	struct efa_softc *sc = arg;

	for (;;) {
		/* TODO: actually check if interrupt status register is set */
		softint_schedule(sc->sc_fata_softintr);
		/* TODO: convert to kpause */
		tsleep(arg, PWAIT, "efa_poll", hz);
	}
}

static void
efa_set_opts(struct efa_softc *sc)
{
	device_t self;

	self = sc->sc_wdcdev.sc_atac.atac_dev;

#ifdef EFA_32BIT_IO 
	sc->sc_32bit_io = true;	
#else
	sc->sc_32bit_io = false;
#endif /* EFA_32BIT_IO */

#ifdef EFA_NO_INTR
	sc->sc_no_intr = true;		/* XXX: not yet! */
#else
	sc->sc_no_intr = false;
#endif /* EFA_NO_INTR */

	if (sc->sc_no_intr)
		aprint_verbose_dev(self, "hardware interrupt disabled\n");

	if (sc->sc_32bit_io)
		aprint_verbose_dev(self, "32-bit I/O enabled\n");
}

int
efa_intr_soft(void *arg) 
{
	int ret = 0;
	struct efa_softc *sc = (struct efa_softc *)arg;

	/* TODO: check which channel needs servicing */
	/* 
	uint8_t fataintreq;
	fataintreq = bus_space_read_1(sc->sc_ports[0].wdr[piom].cmd_iot, 
	sc->sc_ports[chnum].intst[piom], 0);
	*/

	ret = wdcintr(&sc->sc_ports[0].chan);
	ret = wdcintr(&sc->sc_ports[1].chan);

	return ret;
}

int
efa_intr(void *arg)
{
	struct efa_softc *sc = (struct efa_softc *)arg;
	int r1, r2, ret;
	uint8_t intreq;

	intreq = gayle_intr_status();
	ret = 0;

	if (intreq & GAYLE_INT_IDE) {
		gayle_intr_ack(0x7C | (intreq & 0x03));
		/* How to check which channel caused interrupt?
		 * Interrupt status register is not very useful here. */
		r1 = wdcintr(&sc->sc_ports[0].chan);
		r2 = wdcintr(&sc->sc_ports[1].chan);
		ret = r1 | r2;
	}

	return ret;
}

static bool
efa_mapbase(struct efa_softc *sc) 
{
	static struct bus_space_tag fata_cmd_iot;
	static struct bus_space_tag gayle_cmd_iot;
	int i, j;
#ifdef EFA_DEBUG
	device_t self;

	self = sc->sc_wdcdev.sc_atac.atac_dev;
#endif /* EFA_DEBUG */
	
	gayle_cmd_iot.base = (bus_addr_t) ztwomap(GAYLE_IDE_BASE + 2);
	gayle_cmd_iot.absm = &amiga_bus_stride_4swap;
	fata_cmd_iot.base = (bus_addr_t) ztwomap(FATA1_BASE);
	fata_cmd_iot.absm = &amiga_bus_stride_4swap;

#ifdef EFA_DEBUG
	aprint_normal_dev(self, "Gayle %x -> %x, FastATA %x -> %x\n",
	    GAYLE_IDE_BASE, gayle_cmd_iot.base, FATA1_BASE, fata_cmd_iot.base);
#endif

	if (!gayle_cmd_iot.base)
		return false;
	if (!fata_cmd_iot.base)
		return false;

	sc->sc_gayle_wdc_regs.cmd_iot = &gayle_cmd_iot;
	sc->sc_gayle_wdc_regs.ctl_iot = &gayle_cmd_iot;

	for (i = 0; i < FATA1_CHANNELS; i++) {
		for (j = 0; j < PIO_COUNT; j++) {
			sc->sc_ports[i].wdr[j].cmd_iot = &fata_cmd_iot;
			sc->sc_ports[i].wdr[j].data32iot = &fata_cmd_iot;
			sc->sc_ports[i].wdr[j].ctl_iot = &gayle_cmd_iot;
		}
	}

	return true;
}


/* Gayle IDE register mapping, we need it anyway. */
static bool 
efa_mapreg_gayle(struct efa_softc *sc)
{
	int i;

	struct wdc_regs *wdr = &sc->sc_gayle_wdc_regs;

	if (bus_space_map(wdr->cmd_iot, 0, 0x40, 0,
	    &wdr->cmd_baseioh)) {
		return false;
	}

	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
		    wdr->cmd_baseioh, i, i == 0 ? 4 : 1,
		    &wdr->cmd_iohs[i]) != 0) {

			bus_space_unmap(wdr->cmd_iot,
			    wdr->cmd_baseioh, 0x40);
			return false;
		}
	}

	if (bus_space_subregion(wdr->cmd_iot,
	    wdr->cmd_baseioh, 0x406, 1, &wdr->ctl_ioh))
		return false;

	return true;
}

/* Native FastATA register mapping, suitable for PIO modes 0 to 5. */
static bool
efa_mapreg_native(struct efa_softc *sc) 
{
	struct wdc_regs *wdr_gayle = &sc->sc_gayle_wdc_regs;
	struct wdc_regs *wdr_fata;
	int i,j;
#ifdef EFA_DEBUG
	device_t self;

	self = sc->sc_wdcdev.sc_atac.atac_dev;
#endif /* EFA_DEBUG */

	for (i = 0; i < FATA1_CHANNELS; i++) {

		for (j = 0; j < PIO_COUNT; j++) {

			wdr_fata = &sc->sc_ports[i].wdr[j];
			sc->sc_ports[i].mode_ok[j] = false;

			if (pio_offsets[j] == PIO_NSUPP) {
#ifdef EFA_DEBUG
				aprint_normal_dev(self, 
				    "Skipping mapping for PIO mode %x\n", j);
#endif
				continue;
			}

			if (bus_space_map(wdr_fata->cmd_iot, 
			    pio_offsets[j] + FATA1_CHAN_SIZE * i, 
			    FATA1_CHAN_SIZE, 0, &wdr_fata->cmd_baseioh)) {
			    return false;
			}
#ifdef EFA_DEBUG
			aprint_normal_dev(self, 
			    "Chan %x PIO mode %x mapped %x -> %x\n",
			    i, j, (bus_addr_t) kvtop((void*) 
			    wdr_fata->cmd_baseioh), (unsigned int) 
			    wdr_fata->cmd_baseioh);
#endif

			sc->sc_ports[i].mode_ok[j] = true;

			if (j == 0)
				efa_fata_subregion_pio0(wdr_fata);
			else {
				if (sc->sc_32bit_io) 
					efa_fata_subregion_pion(wdr_fata, 
					    true);
				else 
					efa_fata_subregion_pion(wdr_fata, 
					    false);

				bus_space_subregion(wdr_fata->cmd_iot,
				    wdr_fata->cmd_baseioh, FATA1_PION_OFF_INTST,
				    1, &sc->sc_ports[i].intst[j]);
			}

			/* No 32-bit register for PIO0 ... */
			if (j == 0 && sc->sc_32bit_io)
				sc->sc_ports[i].mode_ok[j] = false;
			
			wdr_fata->ctl_ioh = wdr_gayle->ctl_ioh; 
		};
	}
	return true;
}


static void
efa_fata_subregion_pio0(struct wdc_regs *wdr_fata) 
{
	int i;

	for (i = 0; i < WDC_NREG; i++) 
		bus_space_subregion(wdr_fata->cmd_iot,
		    wdr_fata->cmd_baseioh, wdr_offsets_pio0[i], 
		    i == 0 ? 4 : 1, &wdr_fata->cmd_iohs[i]);
}

static void
efa_fata_subregion_pion(struct wdc_regs *wdr_fata, bool data32) 
{
	int i;

	if (data32)
		bus_space_subregion(wdr_fata->cmd_iot, wdr_fata->cmd_baseioh, 
		    FATA1_PION_OFF_DATA32, 8, &wdr_fata->data32ioh);

	for (i = 0; i < WDC_NREG; i++) 
		bus_space_subregion(wdr_fata->cmd_iot,
		    wdr_fata->cmd_baseioh, wdr_offsets_pion[i], 
		    i == 0 ? 4 : 1, &wdr_fata->cmd_iohs[i]);
}

static void
efa_setup_channel(struct ata_channel *chp)
{
	int drive, chnum;
	uint8_t mode; 
	struct atac_softc *atac; 
	struct ata_drive_datas *drvp;
	struct efa_softc *sc;
	int ipl;
#ifdef EFA_DEBUG
	device_t self;
#endif /* EFA_DEBUG */

	chnum = chp->ch_channel;
	atac = chp->ch_atac;

	sc = device_private(atac->atac_dev);

	mode = 5; /* start with fastest possible setting */

#ifdef EFA_DEBUG
	self = sc->sc_wdcdev.sc_atac.atac_dev;
	aprint_normal_dev(self, "efa_setup_channel for ch %d\n",
	    chnum);
#endif /* EFA_DEBUG */

	/* We might be in the middle of something... so raise IPL. */
	ipl = splvm();

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];

		if (drvp->drive_type == ATA_DRIVET_NONE)
			continue; /* nothing to see here */

		if (drvp->PIO_cap < mode)
			mode = drvp->PIO_cap;

		/* TODO: check if sc_ports->mode_ok */

#ifdef EFA_DEBUG
		aprint_normal_dev(self, "drive %d supports %d\n",
		    drive, drvp->PIO_cap);
#endif /* EFA_DEBUG */

		drvp->PIO_mode = mode; 
	}

	/* Change FastATA register set. */
	efa_select_regset(sc, chnum, mode);
	/* re-init shadow regs */
	wdc_init_shadow_regs(CHAN_TO_WDC_REGS(&sc->sc_ports[chnum].chan));

	splx(ipl);
}

static void 
efa_select_regset(struct efa_softc *sc, int chnum, uint8_t piomode) 
{
	struct wdc_softc *wdc;
#ifdef EFA_DEBUG
	device_t self;

	self = sc->sc_wdcdev.sc_atac.atac_dev;
#endif /* EFA_DEBUG */

	wdc = CHAN_TO_WDC(&sc->sc_ports[chnum].chan);	
	wdc->regs[chnum] = sc->sc_ports[chnum].wdr[piomode]; 

#ifdef EFA_DEBUG
	aprint_normal_dev(self, "switched ch %d to PIO %d\n",
	    chnum, piomode);

	efa_debug_print_regmapping(&wdc->regs[chnum]);
#endif /* EFA_DEBUG */
}

#ifdef EFA_DEBUG
static void 
efa_debug_print_regmapping(struct wdc_regs *wdr_fata) 
{ 
	int i;
	aprint_normal("base %x->%x", 
	    (bus_addr_t) kvtop((void*) wdr_fata->cmd_baseioh),
	    (bus_addr_t) wdr_fata->cmd_baseioh);
	for (i = 0; i < WDC_NREG; i++) {
		aprint_normal("reg %x, %x->%x, ", i, 
		    (bus_addr_t) kvtop((void*) wdr_fata->cmd_iohs[i]),
		    (bus_addr_t) wdr_fata->cmd_iohs[i]);
	}
	aprint_normal("\n");
}
#endif /* EFA_DEBUG */

/* Compare the values of (status) command register in PIO0, PIO3 sets. */
static bool
efa_compare_status(void) 
{
	uint8_t cmd0, cmd3;
	struct bus_space_tag fata_bst;
	bus_space_tag_t fata_iot;
	bus_space_handle_t cmd0_ioh, cmd3_ioh;
	bool rv;

	rv = false;

	fata_bst.base = (bus_addr_t) ztwomap(FATA1_BASE);
	fata_bst.absm = &amiga_bus_stride_4swap;

	fata_iot = &fata_bst;

	if (bus_space_map(fata_iot, pio_offsets[0], FATA1_CHAN_SIZE, 0, 
	    &cmd0_ioh))
		return false;
	if (bus_space_map(fata_iot, pio_offsets[3], FATA1_CHAN_SIZE, 0, 
	    &cmd3_ioh))
		return false;

#ifdef EFA_DEBUG
	aprint_normal("probing for FastATA at %x, %x: ", (bus_addr_t) cmd0_ioh,
	    (bus_addr_t) cmd3_ioh);
#endif /* EFA_DEBUG */

	cmd0 = bus_space_read_1(fata_iot, cmd0_ioh, FATA1_PIO0_OFF_COMMAND);
	cmd3 = bus_space_read_1(fata_iot, cmd3_ioh, FATA1_PION_OFF_COMMAND);

	if (cmd0 == cmd3)
		rv = true;

	if ( (cmd0 == 0xFF) || (cmd0 == 0x00) ) {
		/* Assume there's nothing there... */
		rv = false;
	}

#ifdef EFA_DEBUG
	aprint_normal("cmd0 %x, cmd3 %x\n", cmd0, cmd3);
#endif /* EFA_DEBUG */

	bus_space_unmap(fata_iot, pio_offsets[0], FATA1_CHAN_SIZE);
	bus_space_unmap(fata_iot, pio_offsets[3], FATA1_CHAN_SIZE);

	return rv;
}

