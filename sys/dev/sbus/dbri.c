/*	$NetBSD: dbri.c,v 1.9.2.2 2007/04/10 13:24:33 ad Exp $	*/

/*
 * Copyright (C) 1997 Rudolf Koenig (rfkoenig@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1998, 1999 Brent Baccala (baccala@freesoft.org)
 * Copyright (c) 2001, 2002 Jared D. McNeill <jmcneill@netbsd.org>
 * Copyright (c) 2005 Michael Lorenz <macallan@netbsd.org>
 * All rights reserved.
 *
 * This driver is losely based on a Linux driver written by Rudolf Koenig and
 * Brent Baccala who kindly gave their permission to use their code in a
 * BSD-licensed driver.
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
 *	This product includes software developed by Rudolf Koenig, Brent
 *      Baccala, Jared D. McNeill.
 * 4. Neither the name of the author nor the names of any contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dbri.c,v 1.9.2.2 2007/04/10 13:24:33 ad Exp $");

#include "audio.h"
#if NAUDIO > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/sbus/sbusvar.h>
#include <sparc/sparc/auxreg.h>
#include <machine/autoconf.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>

#include <dev/ic/cs4215reg.h>
#include <dev/ic/cs4215var.h>
#include <dev/sbus/dbrireg.h>
#include <dev/sbus/dbrivar.h>

#include "opt_sbus_dbri.h"

#define DBRI_ROM_NAME_PREFIX		"SUNW,DBRI"

#ifdef DBRI_DEBUG
# define DPRINTF aprint_normal
#else
# define DPRINTF while (0) printf
#endif

static const char *dbri_supported[] = {
	"e",
	"s3",
	""
};

enum ms {
	CHImaster,
	CHIslave
};

enum io {
	PIPEinput,
	PIPEoutput
};

/*
 * Function prototypes
 */

/* softc stuff */
static void	dbri_attach_sbus(struct device *, struct device *, void *);
static int	dbri_match_sbus(struct device *, struct cfdata *, void *);

static void	dbri_config_interrupts(struct device *);

/* interrupt handler */
static int	dbri_intr(void *);

/* supporting subroutines */
static int	dbri_init(struct dbri_softc *);
static int	dbri_reset(struct dbri_softc *);
static volatile u_int32_t *dbri_command_lock(struct dbri_softc *);
static void	dbri_command_send(struct dbri_softc *, volatile u_int32_t *);
static void	dbri_process_interrupt_buffer(struct dbri_softc *);
static void	dbri_process_interrupt(struct dbri_softc *, int32_t);

/* mmcodec subroutines */
static int	mmcodec_init(struct dbri_softc *);
static void	mmcodec_init_data(struct dbri_softc *);
static void	mmcodec_pipe_init(struct dbri_softc *);
static void	mmcodec_default(struct dbri_softc *);
static void	mmcodec_setgain(struct dbri_softc *, int);
static int	mmcodec_setcontrol(struct dbri_softc *);

/* chi subroutines */
static void	chi_reset(struct dbri_softc *, enum ms, int);

/* pipe subroutines */
static void	pipe_setup(struct dbri_softc *, int, int);
static void	pipe_reset(struct dbri_softc *, int);
static void	pipe_receive_fixed(struct dbri_softc *, int,
    volatile u_int32_t *);
static void	pipe_transmit_fixed(struct dbri_softc *, int, u_int32_t);

static void	pipe_ts_link(struct dbri_softc *, int, enum io, int, int, int);
static int	pipe_active(struct dbri_softc *, int);

/* audio(9) stuff */
static int	dbri_query_encoding(void *, struct audio_encoding *);
static int	dbri_set_params(void *, int, int, struct audio_params *,
    struct audio_params *,stream_filter_list_t *, stream_filter_list_t *);
static int	dbri_round_blocksize(void *, int, int, const audio_params_t *);
static int	dbri_halt_output(void *);
static int	dbri_halt_input(void *);
static int	dbri_getdev(void *, struct audio_device *);
static int	dbri_set_port(void *, mixer_ctrl_t *);
static int	dbri_get_port(void *, mixer_ctrl_t *);
static int	dbri_query_devinfo(void *, mixer_devinfo_t *);
static size_t	dbri_round_buffersize(void *, int, size_t);
static int	dbri_get_props(void *);
static int	dbri_open(void *, int);
static void	dbri_close(void *);

static void	setup_ring(struct dbri_softc *, int, int, int, int,
    void (*)(void *), void *);

static int	dbri_trigger_output(void *, void *, void *, int,
    void (*)(void *), void *, const struct audio_params *);
static int	dbri_trigger_input(void *, void *, void *, int,
    void (*)(void *), void *, const struct audio_params *);

static void	*dbri_malloc(void *, int, size_t, struct malloc_type *, int);
static void	dbri_free(void *, void *, struct malloc_type *);
static paddr_t	dbri_mappage(void *, void *, off_t, int);
static void	dbri_set_power(struct dbri_softc *, int);
static void	dbri_bring_up(struct dbri_softc *);
static void	dbri_powerhook(int, void *);

/* stupid support routines */
static u_int32_t	reverse_bytes(u_int32_t, int);

struct audio_device dbri_device = {
	"CS4215",
	"",
	"dbri"
};

struct audio_hw_if dbri_hw_if = {
	dbri_open,
	dbri_close,
	NULL,	/* drain */
	dbri_query_encoding,
	dbri_set_params,
	dbri_round_blocksize,
	NULL,	/* commit_settings */
	NULL,	/* init_output */
	NULL,	/* init_input */
	NULL,	/* start_output */
	NULL,	/* start_input */
	dbri_halt_output,
	dbri_halt_input,
	NULL,	/* speaker_ctl */
	dbri_getdev,
	NULL,	/* setfd */
	dbri_set_port,
	dbri_get_port,
	dbri_query_devinfo,
	dbri_malloc,
	dbri_free,
	dbri_round_buffersize,
	dbri_mappage,
	dbri_get_props,
	dbri_trigger_output,
	dbri_trigger_input
};

CFATTACH_DECL(dbri, sizeof(struct dbri_softc),
    dbri_match_sbus, dbri_attach_sbus, NULL, NULL);

#define DBRI_NFORMATS		7
static const struct audio_format dbri_formats[DBRI_NFORMATS] = {
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 2, AUFMT_STEREO, 8, {8000, 9600, 11025, 16000, 22050, 32000, 44100, 48000}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_ULAW, 8, 8,
	 2, AUFMT_STEREO, 8, {8000, 9600, 11025, 16000, 22050, 32000, 44100, 48000}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_ALAW, 8, 8,
	 2, AUFMT_STEREO, 8, {8000, 9600, 11025, 16000, 22050, 32000, 44100, 48000}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_ULINEAR, 8, 8,
	 2, AUFMT_STEREO, 8, {8000, 9600, 11025, 16000, 22050, 32000, 44100, 48000}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_ULAW, 8, 8,
	 1, AUFMT_STEREO, 8, {8000, 9600, 11025, 16000, 22050, 32000, 44100, 48000}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_ALAW, 8, 8,
	 1, AUFMT_STEREO, 8, {8000, 9600, 11025, 16000, 22050, 32000, 44100, 48000}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_ULINEAR, 8, 8,
	 1, AUFMT_STEREO, 8, {8000, 9600, 11025, 16000, 22050, 32000, 44100, 48000}},
};

enum {
	DBRI_OUTPUT_CLASS,
	DBRI_VOL_OUTPUT,
	DBRI_ENABLE_MONO,
	DBRI_ENABLE_HEADPHONE,
	DBRI_ENABLE_LINE,
	DBRI_MONITOR_CLASS,
	DBRI_VOL_MONITOR,
	DBRI_INPUT_CLASS,
	DBRI_INPUT_GAIN,
	DBRI_INPUT_SELECT,
	DBRI_RECORD_CLASS,
	DBRI_ENUM_LAST
};

/*
 * Autoconfig routines
 */
static int
dbri_match_sbus(struct device *parent, struct cfdata *match, void *aux)
{
	struct sbus_attach_args *sa = aux;
	char *ver;
	int i;

	if (strncmp(DBRI_ROM_NAME_PREFIX, sa->sa_name, 9))
		return (0);

	ver = &sa->sa_name[9];

	for (i = 0; dbri_supported[i][0] != '\0'; i++)
		if (strcmp(dbri_supported[i], ver) == 0)
			return (1);

	return (0);
}

static void
dbri_attach_sbus(struct device *parent, struct device *self, void *aux)
{
	struct dbri_softc *sc = (struct dbri_softc *)self;
	struct sbus_attach_args *sa = aux;
	bus_space_handle_t ioh;
	bus_size_t size;
	int error, rseg, pwr;
	char *ver = &sa->sa_name[9];

	sc->sc_iot = sa->sa_bustag;
	sc->sc_dmat = sa->sa_dmatag;
	sc->sc_powerstate = PWR_RESUME;

	pwr = prom_getpropint(sa->sa_node,"pwr-on-auxio",0);
	aprint_normal(": rev %s\n", ver);

	if (pwr) {
		/*
		 * we can control DBRI power via auxio and we're initially
		 * powered down
		 */

		sc->sc_have_powerctl = 1;
		sc->sc_powerstate = 0;
		dbri_set_power(sc, 1);
		powerhook_establish(self->dv_xname, dbri_powerhook, sc);
	} else {
		/* we can't control power so we're always up */
		sc->sc_have_powerctl = 0;
		sc->sc_powerstate = 1;
	}

	if (sa->sa_npromvaddrs)
		ioh = (bus_space_handle_t)sa->sa_promvaddrs[0];
	else {
		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
				 sa->sa_offset, sa->sa_size,
				 BUS_SPACE_MAP_LINEAR, /*0,*/ &ioh) != 0) {
			aprint_error("%s @ sbus: cannot map registers\n",
				self->dv_xname);
			return;
		}
	}

	sc->sc_ioh = ioh;

	size = sizeof(struct dbri_dma);

	/* get a DMA handle */
	if ((error = bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
				       BUS_DMA_NOWAIT, &sc->sc_dmamap)) != 0) {
		aprint_error("%s: DMA map create error %d\n", self->dv_xname, error);
		return;
	}

	/* allocate DMA buffer */
	if ((error = bus_dmamem_alloc(sc->sc_dmat, size, 0, 0, &sc->sc_dmaseg,
				      1, &rseg, BUS_DMA_NOWAIT)) != 0) {
		aprint_error("%s: DMA buffer alloc error %d\n",
		    self->dv_xname, error);
		return;
	}

	/* map DMA buffer into CPU addressable space */
	if ((error = bus_dmamem_map(sc->sc_dmat, &sc->sc_dmaseg, rseg, size,
				    &sc->sc_membase,
				    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		aprint_error("%s: DMA buffer map error %d\n",
		    self->dv_xname, error);
		return;
	}

	/* load the buffer */
	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap,
				     sc->sc_membase, size, NULL,
				     BUS_DMA_NOWAIT)) != 0) {
		aprint_error("%s: DMA buffer map load error %d\n",
		    self->dv_xname, error);
		bus_dmamem_unmap(sc->sc_dmat, sc->sc_membase, size);
		bus_dmamem_free(sc->sc_dmat, &sc->sc_dmaseg, rseg);
		return;
	}

	/* map the registers into memory */

	/* kernel virtual address of DMA buffer */
	sc->sc_dma = (struct dbri_dma *)sc->sc_membase;
	/* physical address of DMA buffer */
	sc->sc_dmabase = sc->sc_dmamap->dm_segs[0].ds_addr;
	sc->sc_bufsiz = size;

	sbus_establish(&sc->sc_sd, &sc->sc_dev);

	bus_intr_establish(sa->sa_bustag, sa->sa_pri, IPL_AUDIO, dbri_intr,
	    sc);

	sc->sc_locked = 0;
	sc->sc_desc_used = 0;
	sc->sc_open = 0;
	sc->sc_playing = 0;
	sc->sc_pmgrstate = PWR_RESUME;
	config_interrupts(self, &dbri_config_interrupts);

	return;
}

/*
 * lowlevel routine to switch power for the DBRI chip
 */
static void
dbri_set_power(struct dbri_softc *sc, int state)
{
	int s;

	if (sc->sc_have_powerctl == 0)
		return;
	if (sc->sc_powerstate == state)
		return;

	if (state) {
		DPRINTF("%s: waiting to power up... ", sc->sc_dev.dv_xname);
		s = splhigh();
		*AUXIO4M_REG |= (AUXIO4M_MMX);
		splx(s);
		delay(10000);
		DPRINTF("done (%02x)\n", *AUXIO4M_REG);
	} else {
		DPRINTF("%s: powering down\n", sc->sc_dev.dv_xname);
		s = splhigh();
		*AUXIO4M_REG &= ~AUXIO4M_MMX;
		splx(s);
		DPRINTF("done (%02x})\n", *AUXIO4M_REG);
	}
	sc->sc_powerstate = state;
}

/*
 * power up and re-initialize the chip
 */
static void
dbri_bring_up(struct dbri_softc *sc)
{

	if (sc->sc_have_powerctl == 0)
		return;
	if (sc->sc_powerstate == 1)
		return;

	/* ok, we really need to do something */
	dbri_set_power(sc, 1);

	/*
	 * re-initialize the chip but skip all the probing, don't overwrite
	 * any other settings either
	 */
	dbri_init(sc);
	mmcodec_setgain(sc, 1);
	mmcodec_pipe_init(sc);
	mmcodec_init_data(sc);
	mmcodec_setgain(sc, 0);
}

static void
dbri_config_interrupts(struct device *dev)
{
	struct dbri_softc *sc = (struct dbri_softc *)dev;

	dbri_init(sc);
	mmcodec_init(sc);
	
	/* Attach ourselves to the high level audio interface */
	audio_attach_mi(&dbri_hw_if, sc, &sc->sc_dev);

	/* power down until open() */
	dbri_set_power(sc, 0);
	return;
}

static int
dbri_intr(void *hdl)
{
	struct dbri_softc *sc = hdl;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int x;

	/* clear interrupt */
	x = bus_space_read_4(iot, ioh, DBRI_REG1);
	if (x & (DBRI_MRR | DBRI_MLE | DBRI_LBG | DBRI_MBE)) {
		u_int32_t tmp;

		if (x & DBRI_MRR)
			aprint_debug("%s: multiple ack error on sbus\n",
			    sc->sc_dev.dv_xname);
		if (x & DBRI_MLE)
			aprint_debug("%s: multiple late error on sbus\n",
			    sc->sc_dev.dv_xname);
		if (x & DBRI_LBG)
			aprint_debug("%s: lost bus grant on sbus\n",
			    sc->sc_dev.dv_xname);
		if (x & DBRI_MBE)
			aprint_debug("%s: burst error on sbus\n",
			    sc->sc_dev.dv_xname);

		/*
		 * Some of these errors disable the chip's circuitry.
		 * Re-enable the circuitry and keep on going.
		 */

		tmp = bus_space_read_4(iot, ioh, DBRI_REG0);
		tmp &= ~(DBRI_DISABLE_MASTER);
		bus_space_write_4(iot, ioh, DBRI_REG0, tmp);
	}

#if 0
	if (!x & 1)	/* XXX: DBRI_INTR_REQ */
		return (1);
#endif

	dbri_process_interrupt_buffer(sc);

	return (1);
}

static int
dbri_init(struct dbri_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t reg;
	volatile u_int32_t *cmd;
	bus_addr_t dmaaddr;
	int n;

	dbri_reset(sc);

	cmd = dbri_command_lock(sc);

	/* XXX: Initialize interrupt ring buffer */
	sc->sc_dma->intr[0] = (u_int32_t)sc->sc_dmabase + dbri_dma_off(intr, 0);
	sc->sc_irqp = 1;

	/* Initialize pipes */
	for (n = 0; n < DBRI_PIPE_MAX; n++)
		sc->sc_pipe[n].desc = sc->sc_pipe[n].next = -1;

	for (n = 1; n < DBRI_INT_BLOCKS; n++) {
		sc->sc_dma->intr[n] = 0;
	}

	/* Disable all SBus bursts */
	/* XXX 16 byte bursts cause errors, the rest works */
	reg = bus_space_read_4(iot, ioh, DBRI_REG0);

	/*reg &= ~(DBRI_BURST_4 | DBRI_BURST_8 | DBRI_BURST_16);*/
	reg |= (DBRI_BURST_4 | DBRI_BURST_8);
	bus_space_write_4(iot, ioh, DBRI_REG0, reg);

	/* setup interrupt queue */
	dmaaddr = (u_int32_t)sc->sc_dmabase + dbri_dma_off(intr, 0);
	*(cmd++) = DBRI_CMD(DBRI_COMMAND_IIQ, 0, 0);
	*(cmd++) = dmaaddr;

	dbri_command_send(sc, cmd);
	return (0);
}

static int
dbri_reset(struct dbri_softc *sc)
{
	int bail = 0;

	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	bus_space_write_4(iot, ioh, DBRI_REG0, DBRI_SOFT_RESET);
	while ((bus_space_read_4(iot, ioh, DBRI_REG0) & DBRI_SOFT_RESET) &&
	    (bail < 100000)) {
		bail++;
		delay(10);
	}
	if (bail == 100000) aprint_error("%s: reset timed out\n",
	    sc->sc_dev.dv_xname);
	return (0);
}

static volatile u_int32_t *
dbri_command_lock(struct dbri_softc *sc)
{

	if (sc->sc_locked)
		aprint_debug("%s: command buffer locked\n", sc->sc_dev.dv_xname);

	sc->sc_locked++;

	return (&sc->sc_dma->command[0]);
}

static void
dbri_command_send(struct dbri_softc *sc, volatile u_int32_t *cmd)
{
	bus_space_handle_t ioh = sc->sc_ioh;
	bus_space_tag_t iot = sc->sc_iot;
	int maxloops = 1000000;
	int x;

	x = splaudio();

	sc->sc_locked--;

	if (sc->sc_locked != 0) {
		aprint_error("%s: command buffer improperly locked\n",
		    sc->sc_dev.dv_xname);
	} else if ((cmd - &sc->sc_dma->command[0]) >= DBRI_NUM_COMMANDS - 1) {
		aprint_error("%s: command buffer overflow\n", sc->sc_dev.dv_xname);
	} else {
		*(cmd++) = DBRI_CMD(DBRI_COMMAND_PAUSE, 0, 0);
		*(cmd++) = DBRI_CMD(DBRI_COMMAND_WAIT, 1, 0);
		sc->sc_waitseen = 0;
		bus_space_write_4(iot, ioh, DBRI_REG8, sc->sc_dmabase);
		while ((--maxloops) > 0 &&
		    (bus_space_read_4(iot, ioh, DBRI_REG0)
		     & DBRI_COMMAND_VALID)) {
			bus_space_barrier(iot, ioh, DBRI_REG0, 4,
					  BUS_SPACE_BARRIER_READ);
			delay(1000);
		}

		if (maxloops == 0) {
			aprint_error("%s: chip never completed command buffer\n",
			    sc->sc_dev.dv_xname);
		} else {

			DPRINTF("%s: command completed\n",
			    sc->sc_dev.dv_xname);

			while ((--maxloops) > 0 && (!sc->sc_waitseen))
				dbri_process_interrupt_buffer(sc);
			if (maxloops == 0) {
				aprint_error("%s: chip never acked WAIT\n",
				    sc->sc_dev.dv_xname);
			}
		}
	}

	splx(x);

	return;
}

static void
dbri_process_interrupt_buffer(struct dbri_softc *sc)
{
	int32_t i;

	while ((i = sc->sc_dma->intr[sc->sc_irqp]) != 0) {
		sc->sc_dma->intr[sc->sc_irqp] = 0;
		sc->sc_irqp++;

		if (sc->sc_irqp == DBRI_INT_BLOCKS)
			sc->sc_irqp = 1;
		else if ((sc->sc_irqp & (DBRI_INT_BLOCKS - 1)) == 0)
			sc->sc_irqp++;

		dbri_process_interrupt(sc, i);
	}

	return;
}

static void
dbri_process_interrupt(struct dbri_softc *sc, int32_t i)
{
#if 0
	const int liu_states[] = { 1, 0, 8, 3, 4, 5, 6, 7 };
#endif
	int val = DBRI_INTR_GETVAL(i);
	int channel = DBRI_INTR_GETCHAN(i);
	int command = DBRI_INTR_GETCMD(i);
	int code = DBRI_INTR_GETCODE(i);
#if 0
	int rval = DBRI_INTR_GETRVAL(i);
#endif
	if (channel == DBRI_INTR_CMD && command == DBRI_COMMAND_WAIT)
		sc->sc_waitseen++;

	switch (code) {
	case DBRI_INTR_XCMP:	/* transmission complete */
	{
		int td;
		struct dbri_desc *dd;

		td = sc->sc_pipe[channel].desc;
		dd = &sc->sc_desc[td];

		if (dd->callback != NULL)
			dd->callback(dd->callback_args);
		break;
	}
	case DBRI_INTR_FXDT:		/* fixed data change */
		DPRINTF("dbri_intr: Fixed data change (%d: %x)\n", channel,
		    val);

		if (sc->sc_pipe[channel].sdp & DBRI_SDP_MSB)
			val = reverse_bytes(val, sc->sc_pipe[channel].length);
		if (sc->sc_pipe[channel].prec)
			*(sc->sc_pipe[channel].prec) = val;
#ifndef DBRI_SPIN
		DPRINTF("%s: wakeup %p\n", sc->sc_dev.dv_xname, sc);
		wakeup(sc);
#endif
		break;
	case DBRI_INTR_SBRI:
		DPRINTF("dbri_intr: SBRI\n");
		break;
	case DBRI_INTR_BRDY:
	{
		/* XXX no input (yet) */
#if 0
		int rd = sc->sc_pipe[channel].desc;
		u_int32_t status;

		DPRINTF("dbri_intr: BRDY\n");
		if (rd < 0 || rd >= DBRI_NUM_DESCRIPTORS) {
			aprint_error("%s: invalid rd on pipe\n",
			    sc->sc_dev.dv_xname);
			break;
		}

		sc->sc_desc[rd].busy = 0;
		sc->sc_pipe[channel].desc = sc->sc_desc[rd].next;
		status = sc->sc_dma->desc[rd].word1;
#endif
		/* XXX: callback ??? */

		break;
	}
	case DBRI_INTR_UNDR:
	{
		volatile u_int32_t *cmd;
		int td = sc->sc_pipe[channel].desc;

		DPRINTF("%s: DBRI_INTR_UNDR\n", sc->sc_dev.dv_xname);

		sc->sc_dma->desc[td].status = 0;

		cmd = dbri_command_lock(sc);
		*(cmd++) = DBRI_CMD(DBRI_COMMAND_SDP, 0,
				    sc->sc_pipe[channel].sdp |
				    DBRI_SDP_VALID_POINTER |
				    DBRI_SDP_CLEAR |
				    DBRI_SDP_2SAME);
		*(cmd++) = sc->sc_dmabase + dbri_dma_off(desc, td);
		dbri_command_send(sc, cmd);
		break;
	}
	case DBRI_INTR_CMDI:
		break;
	default:

		DPRINTF("%s: unknown interrupt code %d\n",
		    sc->sc_dev.dv_xname, code);
		break;
	}

	return;
}

/*
 * mmcodec stuff
 */

static int
mmcodec_init(struct dbri_softc *sc)
{
	bus_space_handle_t ioh = sc->sc_ioh;
	bus_space_tag_t iot = sc->sc_iot;
	u_int32_t reg2;
	int bail;

	reg2 = bus_space_read_4(iot, ioh, DBRI_REG2);
	DPRINTF("mmcodec_init: PIO reads %x\n", reg2);

	if (reg2 & DBRI_PIO2) {
		aprint_normal("%s: onboard CS4215 detected\n",
		    sc->sc_dev.dv_xname);
		sc->sc_mm.onboard = 1;
	}

	if (reg2 & DBRI_PIO0) {
		aprint_normal("%s: speakerbox detected\n",
		    sc->sc_dev.dv_xname);
		sc->sc_mm.onboard = 0;
	}

	if ((reg2 & DBRI_PIO2) && (reg2 & DBRI_PIO0)) {
		aprint_normal("%s: using speakerbox\n",
		    sc->sc_dev.dv_xname);
		bus_space_write_4(iot, ioh, DBRI_REG2, DBRI_PIO2_ENABLE);
		sc->sc_mm.onboard = 0;
	}

	if (!(reg2 & (DBRI_PIO0|DBRI_PIO2))) {
		aprint_normal("%s: no mmcodec found\n", sc->sc_dev.dv_xname);
		return -1;
	}

	sc->sc_version = 0xff;

	mmcodec_pipe_init(sc);
	mmcodec_default(sc);

	sc->sc_mm.offset = sc->sc_mm.onboard ? 0 : 8;

	/* 
	 * mmcodec_setcontrol() sometimes fails right after powerup
	 * so we just try again until we either get a useful response or run
	 * out of time
	 */
	bail = 0;
	while (mmcodec_setcontrol(sc) == -1 || sc->sc_version == 0xff) {

		bail++;
		if (bail > 100) {
			DPRINTF("%s: cs4215 probe failed at offset %d\n",
		    	    sc->sc_dev.dv_xname, sc->sc_mm.offset);
			return (-1);
		}
		delay(10000);
	}

	aprint_normal("%s: cs4215 ver %d found at offset %d\n",
	    sc->sc_dev.dv_xname, sc->sc_version & 0xf, sc->sc_mm.offset);

	/* set some sane defaults for mmcodec_init_data */
	sc->sc_params.channels = 2;
	sc->sc_params.precision = 16;

	mmcodec_init_data(sc);

	return (0);
}

static void
mmcodec_init_data(struct dbri_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t tmp;
	int data_width;

	tmp = bus_space_read_4(iot, ioh, DBRI_REG0);
	tmp &= ~(DBRI_CHI_ACTIVATE);	/* disable CHI */
	bus_space_write_4(iot, ioh, DBRI_REG0, tmp);

	/* switch CS4215 to data mode - set PIO3 to 1 */
	tmp = DBRI_PIO_ENABLE_ALL | DBRI_PIO1 | DBRI_PIO3;

	/* XXX */
	tmp |= (sc->sc_mm.onboard ? DBRI_PIO0 : DBRI_PIO2);

	bus_space_write_4(iot, ioh, DBRI_REG2, tmp);
	chi_reset(sc, CHIslave, 128);

	data_width = sc->sc_params.channels * sc->sc_params.precision;
	pipe_ts_link(sc, 20, PIPEoutput, 16, 32, sc->sc_mm.offset + 32);
	pipe_ts_link(sc, 4, PIPEoutput, 16, data_width, sc->sc_mm.offset);
	pipe_ts_link(sc, 6, PIPEinput, 16, data_width, sc->sc_mm.offset);
	pipe_ts_link(sc, 21, PIPEinput, 16, 16, sc->sc_mm.offset + 40);

	mmcodec_setgain(sc, 0);

	tmp = bus_space_read_4(iot, ioh, DBRI_REG0);
	tmp |= DBRI_CHI_ACTIVATE;
	bus_space_write_4(iot, ioh, DBRI_REG0, tmp);

	return;
}

static void
mmcodec_pipe_init(struct dbri_softc *sc)
{

	pipe_setup(sc, 4, DBRI_SDP_MEM | DBRI_SDP_TO_SER | DBRI_SDP_MSB);
	pipe_setup(sc, 20, DBRI_SDP_FIXED | DBRI_SDP_TO_SER | DBRI_SDP_MSB);
	pipe_setup(sc, 6, DBRI_SDP_MEM | DBRI_SDP_FROM_SER | DBRI_SDP_MSB);
	pipe_setup(sc, 21, DBRI_SDP_FIXED | DBRI_SDP_FROM_SER | DBRI_SDP_MSB);

	pipe_setup(sc, 17, DBRI_SDP_FIXED | DBRI_SDP_TO_SER | DBRI_SDP_MSB);
	pipe_setup(sc, 18, DBRI_SDP_FIXED | DBRI_SDP_FROM_SER | DBRI_SDP_MSB);
	pipe_setup(sc, 19, DBRI_SDP_FIXED | DBRI_SDP_FROM_SER | DBRI_SDP_MSB);

	sc->sc_mm.status = 0;

	pipe_receive_fixed(sc, 18, &sc->sc_mm.status);
	pipe_receive_fixed(sc, 19, &sc->sc_mm.version);

	return;
}

static void
mmcodec_default(struct dbri_softc *sc)
{
	struct cs4215_state *mm = &sc->sc_mm;

	/*
	 * no action, memory resetting only
	 *
	 * data time slots 5-8
	 * speaker, line and headphone enable. set gain to half.
	 * input is mic
	 */
	mm->d.bdata[0] = sc->sc_latt = 0x20 | CS4215_HE | CS4215_LE;
	mm->d.bdata[1] = sc->sc_ratt = 0x20 | CS4215_SE;
	sc->sc_linp = 128;
	sc->sc_rinp = 128;
	sc->sc_monitor = 0;
	sc->sc_input = 1;	/* line */
	mm->d.bdata[2] = (CS4215_LG((sc->sc_linp >> 4)) & 0x0f) |
	    ((sc->sc_input == 2) ? CS4215_IS : 0) | CS4215_PIO0 | CS4215_PIO1;
	mm->d.bdata[3] = (CS4215_RG((sc->sc_rinp >> 4) & 0x0f)) |
	    CS4215_MA(15 - ((sc->sc_monitor >> 4) & 0x0f));
	

	/*
	 * control time slots 1-4
	 *
	 * 0: default I/O voltage scale
	 * 1: 8 bit ulaw, 8kHz, mono, high pass filter disabled
	 * 2: serial enable, CHI master, 128 bits per frame, clock 1
	 * 3: tests disabled
	 */
	mm->c.bcontrol[0] = CS4215_RSRVD_1 | CS4215_MLB;
	mm->c.bcontrol[1] = CS4215_DFR_ULAW | CS4215_FREQ[0].csval;
	mm->c.bcontrol[2] = CS4215_XCLK | CS4215_BSEL_128 | CS4215_FREQ[0].xtal;
	mm->c.bcontrol[3] = 0;

	return;
}

static void
mmcodec_setgain(struct dbri_softc *sc, int mute)
{
	if (mute) {
		/* disable all outputs, max. attenuation */
		sc->sc_mm.d.bdata[0] = sc->sc_latt | 63;
		sc->sc_mm.d.bdata[1] = sc->sc_ratt | 63;
	} else {

		sc->sc_mm.d.bdata[0] = sc->sc_latt;
		sc->sc_mm.d.bdata[1] = sc->sc_ratt;
	}

	/* input stuff */
	sc->sc_mm.d.bdata[2] = CS4215_LG((sc->sc_linp >> 4) & 0x0f) |
	    ((sc->sc_input == 2) ? CS4215_IS : 0) | CS4215_PIO0 | CS4215_PIO1;
	sc->sc_mm.d.bdata[3] = (CS4215_RG((sc->sc_rinp >> 4)) & 0x0f) |
	    (CS4215_MA(15 - ((sc->sc_monitor >> 4) & 0x0f)));

	if (sc->sc_powerstate == 0)
		return;
	pipe_transmit_fixed(sc, 20, sc->sc_mm.d.ldata);

	DPRINTF("mmcodec_setgain: %08x\n", sc->sc_mm.d.ldata);
	/* give the chip some time to execute the command */
	delay(250);

	return;
}

static int
mmcodec_setcontrol(struct dbri_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t val;
	u_int32_t tmp;
#if DBRI_SPIN
	int i;
#endif

	/*
	 * Temporarily mute outputs and wait 125 us to make sure that it
	 * happens. This avoids clicking noises.
	 */
	mmcodec_setgain(sc, 1);
	delay(125);

	/* enable control mode */
	val = DBRI_PIO_ENABLE_ALL | DBRI_PIO1;	/* was PIO1 */

	/* XXX */
	val |= (sc->sc_mm.onboard ? DBRI_PIO0 : DBRI_PIO2);

	bus_space_write_4(iot, ioh, DBRI_REG2, val);

	delay(34);

	/*
	 * in control mode, the cs4215 is the slave device, so the
	 * DBRI must act as the CHI master.
	 *
	 * in data mode, the cs4215 must be the CHI master to insure
	 * that the data stream is in sync with its codec
	 */
	tmp = bus_space_read_4(iot, ioh, DBRI_REG0);
	tmp &= ~DBRI_COMMAND_CHI;
	bus_space_write_4(iot, ioh, DBRI_REG0, tmp);

	chi_reset(sc, CHImaster, 128);

	/* control mode */
	pipe_ts_link(sc, 17, PIPEoutput, 16, 32, sc->sc_mm.offset);
	pipe_ts_link(sc, 18, PIPEinput, 16, 8, sc->sc_mm.offset);
	pipe_ts_link(sc, 19, PIPEinput, 16, 8, sc->sc_mm.offset + 48);

	/* wait for the chip to echo back CLB as zero */
	sc->sc_mm.c.bcontrol[0] &= ~CS4215_CLB;
	pipe_transmit_fixed(sc, 17, sc->sc_mm.c.lcontrol);

	tmp = bus_space_read_4(iot, ioh, DBRI_REG0);
	tmp |= DBRI_CHI_ACTIVATE;
	bus_space_write_4(iot, ioh, DBRI_REG0, tmp);

#if DBRI_SPIN
	i = 1024;
	while (((sc->sc_mm.status & 0xe4) != 0x20) && --i) {
		delay(125);
	}

	if (i == 0) {
		DPRINTF("%s: cs4215 didn't respond to CLB (0x%02x)\n",
		    sc->sc_dev.dv_xname, sc->sc_mm.status);
		return (-1);
	}
#else
	while ((sc->sc_mm.status & 0xe4) != 0x20) {
		DPRINTF("%s: tsleep %p\n", sc->sc_dev.dv_xname, sc);
		tsleep(sc, PCATCH | PZERO, "dbrifxdt", 0);
	}
#endif

	/* copy the version information before it becomes unreadable again */
	sc->sc_version = sc->sc_mm.version;

	/* terminate cs4215 control mode */
	sc->sc_mm.c.bcontrol[0] |= CS4215_CLB;
	pipe_transmit_fixed(sc, 17, sc->sc_mm.c.lcontrol);

	/* two frames of control info @ 8kHz frame rate = 250us delay */
	delay(250);

	mmcodec_setgain(sc, 0);

	return (0);

}

/*
 * CHI combo
 */
static void
chi_reset(struct dbri_softc *sc, enum ms ms, int bpf)
{
	volatile u_int32_t *cmd;
	int val;
	int clockrate, divisor;

	cmd = dbri_command_lock(sc);

	/* set CHI anchor: pipe 16 */
	val = DBRI_DTS_VI | DBRI_DTS_INS | DBRI_DTS_PRVIN(16) | DBRI_PIPE(16);
	*(cmd++) = DBRI_CMD(DBRI_COMMAND_DTS, 0, val);
	*(cmd++) = DBRI_TS_ANCHOR | DBRI_TS_NEXT(16);
	*(cmd++) = 0;

	val = DBRI_DTS_VO | DBRI_DTS_INS | DBRI_DTS_PRVOUT(16) | DBRI_PIPE(16);
	*(cmd++) = DBRI_CMD(DBRI_COMMAND_DTS, 0, val);
	*(cmd++) = 0;
	*(cmd++) = DBRI_TS_ANCHOR | DBRI_TS_NEXT(16);

	sc->sc_pipe[16].sdp = 1;
	sc->sc_pipe[16].next = 16;
	sc->sc_chi_pipe_in = 16;
	sc->sc_chi_pipe_out = 16;

	switch (ms) {
	case CHIslave:
		*(cmd++) = DBRI_CMD(DBRI_COMMAND_CHI, 0, DBRI_CHI_CHICM(0));
		break;
	case CHImaster:
		clockrate = bpf * 8;
		divisor = 12288 / clockrate;

		if (divisor > 255 || divisor * clockrate != 12288)
			aprint_error("%s: illegal bits-per-frame %d\n",
			    sc->sc_dev.dv_xname, bpf);

		*(cmd++) = DBRI_CMD(DBRI_COMMAND_CHI, 0,
		    DBRI_CHI_CHICM(divisor) | DBRI_CHI_FD | DBRI_CHI_BPF(bpf));
		break;
	default:
		aprint_error("%s: unknown value for ms!\n", sc->sc_dev.dv_xname);
		break;
	}

	sc->sc_chi_bpf = bpf;

	/* CHI data mode */
	*(cmd++) = DBRI_CMD(DBRI_COMMAND_PAUSE, 0, 0);
	*(cmd++) = DBRI_CMD(DBRI_COMMAND_CDM, 0,
	    DBRI_CDM_XCE | DBRI_CDM_XEN | DBRI_CDM_REN);

	dbri_command_send(sc, cmd);

	return;
}

/*
 * pipe stuff
 */
static void
pipe_setup(struct dbri_softc *sc, int pipe, int sdp)
{
	DPRINTF("pipe setup: %d\n", pipe);
	if (pipe < 0 || pipe >= DBRI_PIPE_MAX) {
		aprint_error("%s: illegal pipe number %d\n", sc->sc_dev.dv_xname,
		    pipe);
		return;
	}

	if ((sdp & 0xf800) != sdp)
		aprint_error("%s: strange SDP value %d\n", sc->sc_dev.dv_xname, sdp);

	if (DBRI_SDP_MODE(sdp) == DBRI_SDP_FIXED &&
	    !(sdp & DBRI_SDP_TO_SER))
		sdp |= DBRI_SDP_CHANGE;

	sdp |= DBRI_PIPE(pipe);

	sc->sc_pipe[pipe].sdp = sdp;
	sc->sc_pipe[pipe].desc = -1;

	pipe_reset(sc, pipe);

	return;
}

static void
pipe_reset(struct dbri_softc *sc, int pipe)
{
	struct dbri_desc *dd;
	int sdp;
	int desc;
	volatile u_int32_t *cmd;

	if (pipe < 0 || pipe >= DBRI_PIPE_MAX) {
		aprint_error("%s: illegal pipe number %d\n", sc->sc_dev.dv_xname,
		    pipe);
		return;
	}

	sdp = sc->sc_pipe[pipe].sdp;
	if (sdp == 0) {
		aprint_error("%s: can not reset uninitialized pipe %d\n",
		    sc->sc_dev.dv_xname, pipe);
		return;
	}

	cmd = dbri_command_lock(sc);
	*(cmd++) = DBRI_CMD(DBRI_COMMAND_SDP, 0,
	    sdp | DBRI_SDP_CLEAR | DBRI_SDP_VALID_POINTER);
	*(cmd++) = 0;
	dbri_command_send(sc, cmd);

	desc = sc->sc_pipe[pipe].desc;

	dd = &sc->sc_desc[desc];

	dd->busy = 0;

#if 0
	if (dd->callback)
		(*dd->callback)(dd->callback_args);
#endif

	sc->sc_pipe[pipe].desc = -1;

	return;
}

static void
pipe_receive_fixed(struct dbri_softc *sc, int pipe, volatile u_int32_t *prec)
{

	if (pipe < DBRI_PIPE_MAX / 2 || pipe >= DBRI_PIPE_MAX) {
		aprint_error("%s: illegal pipe number %d\n",
		    sc->sc_dev.dv_xname, pipe);
		return;
	}

	if (DBRI_SDP_MODE(sc->sc_pipe[pipe].sdp) != DBRI_SDP_FIXED) {
		aprint_error("%s: non-fixed pipe %d\n", sc->sc_dev.dv_xname,
		    pipe);
		return;
	}

	if (sc->sc_pipe[pipe].sdp & DBRI_SDP_TO_SER) {
		aprint_error("%s: can not receive on transmit pipe %d\b",
		    sc->sc_dev.dv_xname, pipe);
		return;
	}

	sc->sc_pipe[pipe].prec = prec;

	return;
}

static void
pipe_transmit_fixed(struct dbri_softc *sc, int pipe, u_int32_t data)
{
	volatile u_int32_t *cmd;

	if (pipe < DBRI_PIPE_MAX / 2 || pipe >= DBRI_PIPE_MAX) {
		aprint_error("%s: illegal pipe number %d\n",
		    sc->sc_dev.dv_xname, pipe);
		return;
	}

	if (DBRI_SDP_MODE(sc->sc_pipe[pipe].sdp) == 0) {
		aprint_error("%s: uninitialized pipe %d\n",
		    sc->sc_dev.dv_xname, pipe);
		return;
	}

	if (DBRI_SDP_MODE(sc->sc_pipe[pipe].sdp) != DBRI_SDP_FIXED) {
		aprint_error("%s: non-fixed pipe %d\n", sc->sc_dev.dv_xname,
		    pipe);
		return;
	}

	if (!(sc->sc_pipe[pipe].sdp & DBRI_SDP_TO_SER)) {
		aprint_error("%s: called on receive pipe %d\n",
		    sc->sc_dev.dv_xname, pipe);
		return;
	}

	if (sc->sc_pipe[pipe].sdp & DBRI_SDP_MSB)
		data = reverse_bytes(data, sc->sc_pipe[pipe].length);

	cmd = dbri_command_lock(sc);
	*(cmd++) = DBRI_CMD(DBRI_COMMAND_SSP, 0, pipe);
	*(cmd++) = data;

	dbri_command_send(sc, cmd);

	return;
}

static void
setup_ring(struct dbri_softc *sc, int pipe, int which, int num, int blksz,
		void (*callback)(void *), void *callback_args)
{
	volatile u_int32_t *cmd;
	int x, i;
	int td;
	int td_first, td_last;
	bus_addr_t dmabuf, dmabase;
	struct dbri_desc *dd = &sc->sc_desc[which];

	td = 0;
	td_first = td_last = -1;

	if (pipe < 0 || pipe >= DBRI_PIPE_MAX / 2) {
		aprint_error("%s: illegal pipe number %d\n",
		    sc->sc_dev.dv_xname, pipe);
		return;
	}

	if (sc->sc_pipe[pipe].sdp == 0) {
		aprint_error("%s: uninitialized pipe %d\n",
		    sc->sc_dev.dv_xname, pipe);
		return;
	}

	if (!(sc->sc_pipe[pipe].sdp & DBRI_SDP_TO_SER)) {
		aprint_error("%s: called on receive pipe %d\n",
		    sc->sc_dev.dv_xname, pipe);
		return;
	}


	dmabuf = dd->dmabase;
	dmabase = sc->sc_dmabase;
	td = 0;

	for (i = 0; i < (num-1); i++) {

		sc->sc_dma->desc[i].flags = TX_BCNT(blksz)
		    | TX_EOF | TX_BINT;
		sc->sc_dma->desc[i].ba = dmabuf;
		sc->sc_dma->desc[i].nda = dmabase + dbri_dma_off(desc, i + 1);
		sc->sc_dma->desc[i].status = 0;

		td_last = td;
		dmabuf += blksz;
	}

	sc->sc_dma->desc[i].flags = TX_BCNT(blksz) | TX_EOF | TX_BINT;
	sc->sc_dma->desc[i].ba = dmabuf;
	sc->sc_dma->desc[i].nda = dmabase + dbri_dma_off(desc, 0);
	sc->sc_dma->desc[i].status = 0;

	dd->callback = callback;
	dd->callback_args = callback_args;

	x = splaudio();

	/* the pipe shouldn't be active */
	if (pipe_active(sc, pipe)) {
		aprint_error("pipe active (CDP)\n");
		/* pipe is already active */
#if 0
		td_last = sc->sc_pipe[pipe].desc;
		while (sc->sc_desc[td_last].next != -1)
			td_last = sc->sc_desc[td_last].next;

		sc->sc_desc[td_last].next = td_first;
		sc->sc_dma->desc[td_last].nda =
		    sc->sc_dmabase + dbri_dma_off(desc, td_first);

		cmd = dbri_command_lock(sc);
		*(cmd++) = DBRI_CMD(DBRI_COMMAND_CDP, 0, pipe);
		dbri_command_send(sc, cmd);
#endif
	} else {
		/*
		 * pipe isn't active - issue an SDP command to start our
		 * chain of TDs running
		 */
		sc->sc_pipe[pipe].desc = which;
		cmd = dbri_command_lock(sc);
		*(cmd++) = DBRI_CMD(DBRI_COMMAND_SDP, 0,
					sc->sc_pipe[pipe].sdp |
					DBRI_SDP_VALID_POINTER |
					DBRI_SDP_EVERY |
					DBRI_SDP_CLEAR);
		*(cmd++) = sc->sc_dmabase + dbri_dma_off(desc, 0);
		dbri_command_send(sc, cmd);
	}

	splx(x);

	return;
}

static void
pipe_ts_link(struct dbri_softc *sc, int pipe, enum io dir, int basepipe,
		int len, int cycle)
{
	volatile u_int32_t *cmd;
	int prevpipe, nextpipe;
	int val;

	if (pipe < 0 || pipe >= DBRI_PIPE_MAX ||
	    basepipe < 0 || basepipe >= DBRI_PIPE_MAX) {
		aprint_error("%s: illegal pipe numbers (%d, %d)\n",
		    sc->sc_dev.dv_xname, pipe, basepipe);
		return;
	}

	if (sc->sc_pipe[pipe].sdp == 0 || sc->sc_pipe[basepipe].sdp == 0) {
		aprint_error("%s: uninitialized pipe (%d, %d)\n",
		    sc->sc_dev.dv_xname, pipe, basepipe);
		return;
	}

	if (basepipe == 16 && dir == PIPEoutput && cycle == 0)
		cycle = sc->sc_chi_bpf;

	if (basepipe == pipe)
		prevpipe = nextpipe = pipe;
	else {
		if (basepipe == 16) {
			if (dir == PIPEinput) {
				prevpipe = sc->sc_chi_pipe_in;
			} else {
				prevpipe = sc->sc_chi_pipe_out;
			}
		} else
			prevpipe = basepipe;

		nextpipe = sc->sc_pipe[prevpipe].next;

		while (sc->sc_pipe[nextpipe].cycle < cycle &&
		    sc->sc_pipe[nextpipe].next != basepipe) {
			prevpipe = nextpipe;
			nextpipe = sc->sc_pipe[nextpipe].next;
		}
	}

	if (prevpipe == 16) {
		if (dir == PIPEinput) {
			sc->sc_chi_pipe_in = pipe;
		} else {
			sc->sc_chi_pipe_out = pipe;
		}
	} else
		sc->sc_pipe[prevpipe].next = pipe;

	sc->sc_pipe[pipe].next = nextpipe;
	sc->sc_pipe[pipe].cycle = cycle;
	sc->sc_pipe[pipe].length = len;

	cmd = dbri_command_lock(sc);

	switch (dir) {
	case PIPEinput:
		val = DBRI_DTS_VI | DBRI_DTS_INS | DBRI_DTS_PRVIN(prevpipe);
		val |= pipe;
		*(cmd++) = DBRI_CMD(DBRI_COMMAND_DTS, 0, val);
		*(cmd++) = DBRI_TS_LEN(len) | DBRI_TS_CYCLE(cycle) |
		    DBRI_TS_NEXT(nextpipe);
		*(cmd++) = 0;
		break;
	case PIPEoutput:
		val = DBRI_DTS_VO | DBRI_DTS_INS | DBRI_DTS_PRVOUT(prevpipe);
		val |= pipe;
		*(cmd++) = DBRI_CMD(DBRI_COMMAND_DTS, 0, val);
		*(cmd++) = 0;
		*(cmd++) = DBRI_TS_LEN(len) | DBRI_TS_CYCLE(cycle) |
		    DBRI_TS_NEXT(nextpipe);
		break;
	default:
		DPRINTF("%s: should not have happened!\n",
		    sc->sc_dev.dv_xname);
		break;
	}

	dbri_command_send(sc, cmd);

	return;
}

static int
pipe_active(struct dbri_softc *sc, int pipe)
{

	return (sc->sc_pipe[pipe].desc != -1);
}

/*
 * subroutines required to interface with audio(9)
 */

static int
dbri_query_encoding(void *hdl, struct audio_encoding *ae)
{

	switch (ae->index) {
	case 0:
		strcpy(ae->name, AudioEulinear);
		ae->encoding = AUDIO_ENCODING_ULINEAR;
		ae->precision = 8;
		ae->flags = 0;
		break;
	case 1:
		strcpy(ae->name, AudioEmulaw);
		ae->encoding = AUDIO_ENCODING_ULAW;
		ae->precision = 8;
		ae->flags = 0;
		break;
	case 2:
		strcpy(ae->name, AudioEalaw);
		ae->encoding = AUDIO_ENCODING_ALAW;
		ae->precision = 8;
		ae->flags = 0;
		break;
	case 3:
		strcpy(ae->name, AudioEslinear);
		ae->encoding = AUDIO_ENCODING_SLINEAR;
		ae->precision = 8;
		ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 4:
		strcpy(ae->name, AudioEslinear_le);
		ae->encoding = AUDIO_ENCODING_SLINEAR_LE;
		ae->precision = 16;
		ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 5:
		strcpy(ae->name, AudioEulinear_le);
		ae->encoding = AUDIO_ENCODING_ULINEAR_LE;
		ae->precision = 16;
		ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 6:
		strcpy(ae->name, AudioEslinear_be);
		ae->encoding = AUDIO_ENCODING_SLINEAR_BE;
		ae->precision = 16;
		ae->flags = 0;
		break;
	case 7:
		strcpy(ae->name, AudioEulinear_be);
		ae->encoding = AUDIO_ENCODING_ULINEAR_BE;
		ae->precision = 16;
		ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 8:
		strcpy(ae->name, AudioEslinear);
		ae->encoding = AUDIO_ENCODING_SLINEAR;
		ae->precision = 16;
		ae->flags = 0;
		break;
	default:
		return (EINVAL);
	}

	return (0);
}

/*
 * XXX: recording isn't supported - jmcneill
 */
static int
dbri_set_params(void *hdl, int setmode, int usemode,
		struct audio_params *play, struct audio_params *rec,
		stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct dbri_softc *sc = hdl;
	int rate;
	audio_params_t *p = NULL;
	stream_filter_list_t *fil;
	int mode;

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
			return EINVAL;
	}

	for (mode = AUMODE_RECORD; mode != -1;
	     mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = mode == AUMODE_PLAY ? play : rec;
		if (p->sample_rate < 4000 || p->sample_rate > 50000) {
			DPRINTF("dbri_set_params: invalid rate %d\n", 
			    p->sample_rate);
			return EINVAL;
		}

		fil = mode == AUMODE_PLAY ? pfil : rfil;
	DPRINTF("enc: %d rate: %d prec: %d chan: %d\n", p->encoding,
	    p->sample_rate, p->precision, p->channels); 
		if (auconv_set_converter(dbri_formats, DBRI_NFORMATS,
					 mode, p, true, fil) < 0) {
			DPRINTF("dbri_set_params: auconv_set_converter failed\n");
			return EINVAL;
		}
		if (fil->req_size > 0)
			p = &fil->filters[0].param;
	}

	if (p == NULL) {
		DPRINTF("dbri_set_params: no parameters to set\n");
		return 0;
	}

	DPRINTF("enc: %d rate: %d prec: %d chan: %d\n", p->encoding,
	    p->sample_rate, p->precision, p->channels); 

	for (rate = 0; CS4215_FREQ[rate].freq; rate++)
		if (CS4215_FREQ[rate].freq == p->sample_rate)
			break;

	if (CS4215_FREQ[rate].freq == 0)
		return (EINVAL);

	/* set frequency */
	sc->sc_mm.c.bcontrol[1] &= ~0x38;
	sc->sc_mm.c.bcontrol[1] |= CS4215_FREQ[rate].csval;
	sc->sc_mm.c.bcontrol[2] &= ~0x70;
	sc->sc_mm.c.bcontrol[2] |= CS4215_FREQ[rate].xtal;

	switch (p->encoding) {
	case AUDIO_ENCODING_ULAW:
		sc->sc_mm.c.bcontrol[1] &= ~3;
		sc->sc_mm.c.bcontrol[1] |= CS4215_DFR_ULAW;
		break;
	case AUDIO_ENCODING_ALAW:
		sc->sc_mm.c.bcontrol[1] &= ~3;
		sc->sc_mm.c.bcontrol[1] |= CS4215_DFR_ALAW;
		break;
	case AUDIO_ENCODING_ULINEAR:
		sc->sc_mm.c.bcontrol[1] &= ~3;
		if (p->precision == 8) {
			sc->sc_mm.c.bcontrol[1] |= CS4215_DFR_LINEAR8;
		} else {
			sc->sc_mm.c.bcontrol[1] |= CS4215_DFR_LINEAR16;
		}
		break;
	case AUDIO_ENCODING_SLINEAR_BE:
	case AUDIO_ENCODING_SLINEAR:
		sc->sc_mm.c.bcontrol[1] &= ~3;
		sc->sc_mm.c.bcontrol[1] |= CS4215_DFR_LINEAR16;
		break;
	}

	switch (p->channels) {
	case 1:
		sc->sc_mm.c.bcontrol[1] &= ~CS4215_DFR_STEREO;
		break;
	case 2:
		sc->sc_mm.c.bcontrol[1] |= CS4215_DFR_STEREO;
		break;
	}

	return (0);
}

static int
dbri_round_blocksize(void *hdl, int bs, int mode,
			const audio_params_t *param)
{

	/* DBRI DMA segment size, rounded town to 32bit alignment */
	return 0x1ffc;
}

static int
dbri_halt_output(void *hdl)
{
	struct dbri_softc *sc = hdl;

	sc->sc_playing = 0;
	pipe_reset(sc, 4);
	return (0);
}

static int
dbri_getdev(void *hdl, struct audio_device *ret)
{

	*ret = dbri_device;
	return (0);
}

static int
dbri_set_port(void *hdl, mixer_ctrl_t *mc)
{
	struct dbri_softc *sc = hdl;
	int latt = sc->sc_latt, ratt = sc->sc_ratt;

	switch (mc->dev) {
	    case DBRI_VOL_OUTPUT:	/* master volume */
		latt = (latt & 0xc0) | (63 -
		    min(mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] >> 2, 63));
		ratt = (ratt & 0xc0) | (63 -
		    min(mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] >> 2, 63));
		break;
	    case DBRI_ENABLE_MONO:	/* built-in speaker */
	    	if (mc->un.ord == 1) {
			ratt |= CS4215_SE;
		} else
			ratt &= ~CS4215_SE;
		break;
	    case DBRI_ENABLE_HEADPHONE:	/* headphones output */
	    	if (mc->un.ord == 1) {
			latt |= CS4215_HE;
		} else
			latt &= ~CS4215_HE;
		break;
	    case DBRI_ENABLE_LINE:	/* line out */
	    	if (mc->un.ord == 1) {
			latt |= CS4215_LE;
		} else
			latt &= ~CS4215_LE;
		break;
	    case DBRI_VOL_MONITOR:
		if (mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] == 
		    sc->sc_monitor)
			return 0;
		sc->sc_monitor = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		break;
	    case DBRI_INPUT_GAIN:
		sc->sc_linp = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		sc->sc_rinp = mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
		break;
	    case DBRI_INPUT_SELECT:
	    	if (mc->un.mask == sc->sc_input)
	    		return 0;
	    	sc->sc_input =  mc->un.mask;
	    	break;
	}

	sc->sc_latt = latt;
	sc->sc_ratt = ratt;

	mmcodec_setgain(sc, 0);

	return (0);
}

static int
dbri_get_port(void *hdl, mixer_ctrl_t *mc)
{
	struct dbri_softc *sc = hdl;

	switch (mc->dev) {
	    case DBRI_VOL_OUTPUT:	/* master volume */
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
		    (63 - (sc->sc_latt & 0x3f)) << 2;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
		    (63 - (sc->sc_ratt & 0x3f)) << 2;
		return (0);
	    case DBRI_ENABLE_MONO:	/* built-in speaker */
	    	mc->un.ord = (sc->sc_ratt & CS4215_SE) ? 1 : 0;
		return 0;
	    case DBRI_ENABLE_HEADPHONE:	/* headphones output */
	    	mc->un.ord = (sc->sc_latt & CS4215_HE) ? 1 : 0;
		return 0;
	    case DBRI_ENABLE_LINE:	/* line out */
	    	mc->un.ord = (sc->sc_latt & CS4215_LE) ? 1 : 0;
		return 0;
	    case DBRI_VOL_MONITOR:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = sc->sc_monitor;
		return 0;
	    case DBRI_INPUT_GAIN:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = sc->sc_linp;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = sc->sc_rinp;
		return 0;
	    case DBRI_INPUT_SELECT:
	    	mc->un.mask = sc->sc_input;
	    	return 0;
	}
	return (EINVAL);
}

static int
dbri_query_devinfo(void *hdl, mixer_devinfo_t *di)
{

	switch (di->index) {
	case DBRI_MONITOR_CLASS:
		di->mixer_class = DBRI_MONITOR_CLASS;
		strcpy(di->label.name, AudioCmonitor);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case DBRI_OUTPUT_CLASS:
		di->mixer_class = DBRI_OUTPUT_CLASS;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case DBRI_INPUT_CLASS:
		di->mixer_class = DBRI_INPUT_CLASS;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case DBRI_VOL_OUTPUT:	/* master volume */
		di->mixer_class = DBRI_OUTPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNmaster);
		di->type = AUDIO_MIXER_VALUE;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return (0);
	case DBRI_INPUT_GAIN:	/* input gain */
		di->mixer_class = DBRI_INPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNrecord);
		di->type = AUDIO_MIXER_VALUE;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return (0);
	case DBRI_VOL_MONITOR:	/* monitor volume */
		di->mixer_class = DBRI_MONITOR_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNmonitor);
		di->type = AUDIO_MIXER_VALUE;
		di->un.v.num_channels = 1;
		strcpy(di->un.v.units.name, AudioNvolume);
		return (0);
	case DBRI_ENABLE_MONO:	/* built-in speaker */
		di->mixer_class = DBRI_OUTPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNmono);
		di->type = AUDIO_MIXER_ENUM;
		di->un.e.num_mem = 2;
		strcpy(di->un.e.member[0].label.name, AudioNoff);
		di->un.e.member[0].ord = 0;
		strcpy(di->un.e.member[1].label.name, AudioNon);
		di->un.e.member[1].ord = 1;
		return (0);
	case DBRI_ENABLE_HEADPHONE:	/* headphones output */
		di->mixer_class = DBRI_OUTPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNheadphone);
		di->type = AUDIO_MIXER_ENUM;
		di->un.e.num_mem = 2;
		strcpy(di->un.e.member[0].label.name, AudioNoff);
		di->un.e.member[0].ord = 0;
		strcpy(di->un.e.member[1].label.name, AudioNon);
		di->un.e.member[1].ord = 1;
		return (0);
	case DBRI_ENABLE_LINE:	/* line out */
		di->mixer_class = DBRI_OUTPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNline);
		di->type = AUDIO_MIXER_ENUM;
		di->un.e.num_mem = 2;
		strcpy(di->un.e.member[0].label.name, AudioNoff);
		di->un.e.member[0].ord = 0;
		strcpy(di->un.e.member[1].label.name, AudioNon);
		di->un.e.member[1].ord = 1;
		return (0);
	case DBRI_INPUT_SELECT:
		di->mixer_class = DBRI_INPUT_CLASS;
		strcpy(di->label.name, AudioNsource);
		di->type = AUDIO_MIXER_SET;
		di->prev = di->next = AUDIO_MIXER_LAST;
		di->un.s.num_mem = 2;
		strcpy(di->un.s.member[0].label.name, AudioNline);
		di->un.s.member[0].mask = 1 << 0;
		strcpy(di->un.s.member[1].label.name, AudioNmicrophone);
		di->un.s.member[1].mask = 1 << 1;
		return 0;
	}

	return (ENXIO);
}

static size_t
dbri_round_buffersize(void *hdl, int dir, size_t bufsize)
{
#ifdef DBRI_BIG_BUFFER
	return 16*0x1ffc;	/* use ~128KB buffer */
#else
	return bufsize;
#endif
}

static int
dbri_get_props(void *hdl)
{

	return AUDIO_PROP_MMAP;
}

static int
dbri_trigger_output(void *hdl, void *start, void *end, int blksize,
		    void (*intr)(void *), void *intrarg,
		    const struct audio_params *param)
{
	struct dbri_softc *sc = hdl;
	unsigned long count, current, num;

	count = (unsigned long)(((char *)end - (char *)start));
	num = count / blksize;

	DPRINTF("trigger_output(%lx %lx) : %d %ld %ld\n",
	    (unsigned long)intr,
	    (unsigned long)intrarg, blksize, count, num);

	sc->sc_params = *param;

	mmcodec_setcontrol(sc);
	mmcodec_init_data(sc);
	current = 0;
	while ((current < sc->sc_desc_used) &&
	    (sc->sc_desc[current].buf != start))
	    	current++;

	if (current < sc->sc_desc_used) {
		setup_ring(sc, 4, current, num, blksize, intr, intrarg);
		sc->sc_playing = 1;
		return 0;
	}
	return EINVAL;
}

static int
dbri_halt_input(void *cookie)
{
	return 0;
}

static int
dbri_trigger_input(void *hdl, void *start, void *end, int blksize,
		    void (*intr)(void *), void *intrarg,
		    const struct audio_params *param)
{
#if notyet
	struct dbri_softc *sc = hdl;
	unsigned long count, current, num;

	count = (unsigned long)(((char *)end - (char *)start));
	num = count / blksize;

	DPRINTF("trigger_input(%lx %lx) : %d %ld %ld\n",
	    (unsigned long)intr,
	    (unsigned long)intrarg, blksize, count, num);

	sc->sc_params = *param;

	mmcodec_setcontrol(sc);
	mmcodec_init_data(sc);
	current = 0;
	while ((current < sc->sc_desc_used) &&
	    (sc->sc_desc[current].buf != start))
	    	current++;

	if (current < sc->sc_desc_used) {
		setup_ring(sc, 4, current, num, blksize, intr, intrarg);
		return 0;
	}
#endif
	return EINVAL;
}


static u_int32_t
reverse_bytes(u_int32_t b, int len)
{
	switch (len) {
	case 32:
		b = ((b & 0xffff0000) >> 16) | ((b & 0x0000ffff) << 16);
	case 16:
		b = ((b & 0xff00ff00) >>  8) | ((b & 0x00ff00ff) <<  8);
	case 8:
		b = ((b & 0xf0f0f0f0) >>  4) | ((b & 0x0f0f0f0f) <<  4);
	case 4:
		b = ((b & 0xcccccccc) >>  2) | ((b & 0x33333333) <<  2);
	case 2:
		b = ((b & 0xaaaaaaaa) >>  1) | ((b & 0x55555555) <<  1);
	case 1:
	case 0:
		break;
	default:
		DPRINTF("reverse_bytes: unsupported length\n");
	};

	return (b);
}

static void
*dbri_malloc(void *v, int dir, size_t s, struct malloc_type *mt, int flags)
{
	struct dbri_softc *sc = v;
	struct dbri_desc *dd = &sc->sc_desc[sc->sc_desc_used];
	int rseg;

	if (bus_dmamap_create(sc->sc_dmat, s, 1, s, 0, BUS_DMA_NOWAIT,
	    &dd->dmamap) == 0) {
		if (bus_dmamem_alloc(sc->sc_dmat, s, 0, 0, &dd->dmaseg,
		    1, &rseg, BUS_DMA_NOWAIT) == 0) {
			if (bus_dmamem_map(sc->sc_dmat, &dd->dmaseg, rseg, s,
			    &dd->buf, BUS_DMA_NOWAIT|BUS_DMA_COHERENT) == 0) {
				if (dd->buf!=NULL) {
					if (bus_dmamap_load(sc->sc_dmat,
					    dd->dmamap, dd->buf, s, NULL,
					    BUS_DMA_NOWAIT) == 0) {
						dd->len = s;
						dd->busy = 0;
						dd->callback = NULL;
						dd->dmabase =
						 dd->dmamap->dm_segs[0].ds_addr;
						DPRINTF("dbri_malloc: using buffer %d\n",
						    sc->sc_desc_used);
						sc->sc_desc_used++;
						return dd->buf;
					} else
						aprint_error("dbri_malloc: load failed\n");
				} else
					aprint_error("dbri_malloc: map returned NULL\n");
			} else
				aprint_error("dbri_malloc: map failed\n");
			bus_dmamem_free(sc->sc_dmat, &dd->dmaseg, rseg);
		} else
			aprint_error("dbri_malloc: malloc() failed\n");
		bus_dmamap_destroy(sc->sc_dmat, dd->dmamap);
	} else
		aprint_error("dbri_malloc: bus_dmamap_create() failed\n");
	return NULL;
}

static void
dbri_free(void *v, void *p, struct malloc_type *mt)
{
	free(p, mt);
}

static paddr_t
dbri_mappage(void *v, void *mem, off_t off, int prot)
{
	struct dbri_softc *sc = v;;
	int current;

	if (off < 0)
		return -1;

	current = 0;
	while ((current < sc->sc_desc_used) &&
	    (sc->sc_desc[current].buf != mem))
	    	current++;

	if (current < sc->sc_desc_used) {
		return bus_dmamem_mmap(sc->sc_dmat,
		    &sc->sc_desc[current].dmaseg, 1, off, prot, BUS_DMA_WAITOK);
	}

	return -1;
}

static int
dbri_open(void *cookie, int flags)
{
	struct dbri_softc *sc = cookie;

	dbri_bring_up(sc);
	sc->sc_open = 1;
	return 0;
}

static void
dbri_close(void *cookie)
{
	struct dbri_softc *sc = cookie;

	sc->sc_open = 0;
	dbri_set_power(sc, 0);
}

static void
dbri_powerhook(int why, void *cookie)
{
	struct dbri_softc *sc = cookie;

	if (why == sc->sc_pmgrstate)
		return;

	switch(why)
	{
		case PWR_SUSPEND:
			dbri_set_power(sc, 0);
			break;
		case PWR_RESUME:
			DPRINTF("resume: %d\n", sc->sc_open);
			sc->sc_pmgrstate = PWR_RESUME;
			if (sc->sc_open == 1) {
				dbri_bring_up(sc);
				if (sc->sc_playing) {
					volatile u_int32_t *cmd;
					int s;

					s = splaudio();
					cmd = dbri_command_lock(sc);
					*(cmd++) = DBRI_CMD(DBRI_COMMAND_SDP,
					    0, sc->sc_pipe[4].sdp |
					    DBRI_SDP_VALID_POINTER |
					    DBRI_SDP_EVERY | DBRI_SDP_CLEAR);
					*(cmd++) = sc->sc_dmabase +
					    dbri_dma_off(desc, 0);
					dbri_command_send(sc, cmd);
					splx(s);
				}
			}
			break;
		default:
			return;
	}
	sc->sc_pmgrstate = why;
}

#endif /* NAUDIO > 0 */
