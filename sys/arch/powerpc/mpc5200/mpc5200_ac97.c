/*	$NetBSD: mpc5200_ac97.c,v 1.2 2026/07/01 00:03:18 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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
 * Driver for the Freescale MPC5200B on-chip AC97 controller (a PSC in AC97
 * mode), written with the assumption that it's connected to a codec that
 * can be driver through the ac97(4) interface. 
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpc5200_ac97.c,v 1.2 2026/07/01 00:03:18 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/audioio.h>

#include <dev/audio/audio_if.h>

#include <dev/ic/ac97var.h>

#include <dev/ofw/openfirm.h>

#include <machine/intr.h>

#include <powerpc/mpc5200/obiovar.h>
#include <powerpc/mpc5200/mpc5200reg.h>
#include <powerpc/mpc5200/pscreg.h>
#include <powerpc/mpc5200/mpc5200_ac97reg.h>
#include <powerpc/mpc5200/mpc5200_ac97var.h>
#include <powerpc/mpc5200/bestcommvar.h>

/*
 * BestComm wiring for the PSC2 transmit path.
 */
#define	MPCAC97_TX_TASK		12
#define	MPCAC97_TX_INITIATOR	12
#define	MPCAC97_TX_PRIO		5	/* SDMA initiator priority (0-7) */
#define	AC97_FIFO_TX		PSC_TB	/* DMA target: Tx Buffer (0x0c) */

#define	MPCAC97_RATE		48000	/* AC97 fixed sample rate */
#define	MPCAC97_FRAME_SIZE	8	/* 2 ch * 32-bit slot word */
#define	MPCAC97_MAX_NBLK	256	/* SDMA descriptor-ring sanity cap */

/*
 * Tx FIFO "almost empty" threshold
 */
#define	MPCAC97_TFALARM		0x100
#define	MPCAC97_TFGRAN		0x04

/* Codec register-access poll limits. */
#define	AC97_CMD_TIMEOUT	1000	/* x10us  = 10ms for a command frame */
#define	AC97_READY_TIMEOUT	1000	/* x1ms   = 1s for codec power-up */

#define	AC97_REG_POWERDOWN	0x26	/* powerdown ctrl/status (AC97_REG_POWER) */
#define	AC97_POWER_READY	0x000f	/* ADC+DAC+ANL+REF ready bits */

static int	mpcac97_match(device_t, cfdata_t, void *);
static void	mpcac97_attach(device_t, device_t, void *);

static void	mpcac97_init_psc(struct mpcac97_softc *);
static int	mpcac97_intr(void *);

/* ac97(4) host interface. */
static int	mpcac97_ac97_attach(void *, struct ac97_codec_if *);
static int	mpcac97_ac97_read(void *, uint8_t, uint16_t *);
static int	mpcac97_ac97_write(void *, uint8_t, uint16_t);
static int	mpcac97_ac97_reset(void *);
static enum ac97_host_flags mpcac97_ac97_flags(void *);

/* audio(4) hardware interface. */
static int	mpcac97_open(void *, int);
static void	mpcac97_close(void *);
static int	mpcac97_query_format(void *, audio_format_query_t *);
static int	mpcac97_set_format(void *, int, const audio_params_t *,
		    const audio_params_t *, audio_filter_reg_t *,
		    audio_filter_reg_t *);
static int	mpcac97_round_blocksize(void *, int, int,
		    const audio_params_t *);
static size_t	mpcac97_round_buffersize(void *, int, size_t);
static void	*mpcac97_allocm(void *, int, size_t);
static void	mpcac97_freem(void *, void *, size_t);
static int	mpcac97_get_props(void *);
static int	mpcac97_trigger_output(void *, void *, void *, int,
		    void (*)(void *), void *, const audio_params_t *);
static int	mpcac97_halt_output(void *);
static int	mpcac97_getdev(void *, struct audio_device *);
static int	mpcac97_set_port(void *, mixer_ctrl_t *);
static int	mpcac97_get_port(void *, mixer_ctrl_t *);
static int	mpcac97_query_devinfo(void *, mixer_devinfo_t *);
static void	mpcac97_get_locks(void *, kmutex_t **, kmutex_t **);

CFATTACH_DECL_NEW(mpcaudio, sizeof(struct mpcac97_softc),
    mpcac97_match, mpcac97_attach, NULL, NULL);

static const struct bestcomm_bd_layout mpcac97_tx_layout =
    BESTCOMM_LAYOUT_GEN_TX;

/*
 * Data-routing-descriptor offsets
 */
static const uint16_t mpcac97_tx_drd[] = {
	0x04, 0x0c, 0x10, 0x14, 0x20, 0x28, 0x30, 0x34
};

static const struct audio_format mpcac97_formats[] = {
	{
		.mode		= AUMODE_PLAY,
		.encoding	= AUDIO_ENCODING_SLINEAR_BE,
		.validbits	= 16,
		.precision	= 32,
		.channels	= 2,
		.channel_mask	= AUFMT_STEREO,
		.frequency_type	= 1,
		.frequency	= { MPCAC97_RATE },
	},
};

static const struct audio_hw_if mpcac97_hw_if = {
	.open		= mpcac97_open,
	.close		= mpcac97_close,
	.query_format	= mpcac97_query_format,
	.set_format	= mpcac97_set_format,
	.round_blocksize = mpcac97_round_blocksize,
	.round_buffersize = mpcac97_round_buffersize,
	.allocm		= mpcac97_allocm,
	.freem		= mpcac97_freem,
	.get_props	= mpcac97_get_props,
	.trigger_output	= mpcac97_trigger_output,
	.halt_output	= mpcac97_halt_output,
	.getdev		= mpcac97_getdev,
	.set_port	= mpcac97_set_port,
	.get_port	= mpcac97_get_port,
	.query_devinfo	= mpcac97_query_devinfo,
	.get_locks	= mpcac97_get_locks,
};

/* Register accessors over the big-endian SoC bus_space tag. */
static inline uint8_t
RD1(struct mpcac97_softc *sc, bus_size_t o)
{
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, o);
}
static inline uint16_t
RD2(struct mpcac97_softc *sc, bus_size_t o)
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, o);
}
static inline uint32_t
RD4(struct mpcac97_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, o);
}
static inline void
WR1(struct mpcac97_softc *sc, bus_size_t o, uint8_t v)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, o, v);
}
static inline void
WR2(struct mpcac97_softc *sc, bus_size_t o, uint16_t v)
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, o, v);
}
static inline void
WR4(struct mpcac97_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, o, v);
}

static int
mpcac97_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oba = aux;
	char compat[40];
	int len;

	if (strcmp(oba->obio_name, "sound") == 0)
		return 1;

	len = OF_getprop(oba->obio_node, "compatible", compat, sizeof(compat));
	if (len > 0 &&
	    (strcmp(compat, "mpc5200-psc-ac97") == 0 ||
	     strcmp(compat, "mpc5200b-psc-ac97") == 0))
		return 1;

	return 0;
}

static void
mpcac97_attach(device_t parent, device_t self, void *aux)
{
	struct mpcac97_softc *sc = device_private(self);
	struct obio_attach_args *oba = aux;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_iot = oba->obio_bst;
	sc->sc_dmat = oba->obio_dmat;

	aprint_naive("\n");
	aprint_normal(": MPC5200 AC97 controller\n");

	if (!bestcomm_available()) {
		aprint_error_dev(self, "BestComm SDMA not available\n");
		return;
	}

	/*
	 * XXX: we should not depend on the firmware here?
	 */
	addr = oba->obio_addr != 0 ? oba->obio_addr :
	    (MPC5200_MBAR_DEFAULT + MPC5200_REG_PSC2);
	size = oba->obio_size != 0 ? oba->obio_size : MPC5200_PSC_SIZE;
	sc->sc_pa = addr;
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	/*
	 * XXX: The hardware interrupt lock is at IPL_NET, not usual IPL_AUDIO!
	 * The completion interrupt is a leaf on the shared BestComm SDMA cascade
	 * (along with FEC), and every leaf on that cascade MUST run at the
	 * cascade's own IPL!!! Otherwise, enjoy hard lockups.
	 */
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_NET);

	/* Put PSC2 into normal AC97 mode and enable the transceiver. */
	mpcac97_init_psc(sc);

	/*
	 * Attach the codec-independent AC97 layer.
	 */
	sc->sc_host_if.arg = sc;
	sc->sc_host_if.attach = mpcac97_ac97_attach;
	sc->sc_host_if.read = mpcac97_ac97_read;
	sc->sc_host_if.write = mpcac97_ac97_write;
	sc->sc_host_if.reset = mpcac97_ac97_reset;
	sc->sc_host_if.flags = mpcac97_ac97_flags;
	aprint_debug_dev(self, "calling ac97_attach\n");
	if (ac97_attach(&sc->sc_host_if, self, &sc->sc_lock) != 0) {
		aprint_error_dev(self, "no AC97 codec found\n");
		goto fail;
	}
	aprint_debug_dev(self, "ac97_attach returned ok\n");

	/*
	 * The codec's SPDIF control is lock-gated
	 */
	mutex_enter(&sc->sc_lock);
	sc->sc_codec->vtbl->unlock(sc->sc_codec);
	mutex_exit(&sc->sc_lock);
	aprint_debug_dev(self, "spdif unlocked\n");

	/*
	 * Route the BestComm transmit-task completion event to our block
	 * handler.
	 */
	sc->sc_txih = intr_establish(bestcomm_task_irq(MPCAC97_TX_TASK),
	    IST_LEVEL, IPL_NET, mpcac97_intr, sc);
	if (sc->sc_txih == NULL) {
		aprint_error_dev(self, "can't establish SDMA interrupt\n");
		goto fail;
	}
	aprint_debug_dev(self, "SDMA intr established\n");

	sc->sc_audiodev = audio_attach_mi(&mpcac97_hw_if, sc, self);
	aprint_debug_dev(self, "audio_attach_mi done\n");
	return;

fail:
	if (sc->sc_txih != NULL)
		intr_disestablish(sc->sc_txih);
	mutex_destroy(&sc->sc_lock);
	mutex_destroy(&sc->sc_intr_lock);
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
}

/*
 * Configure PSC2 for "normal" AC97 mode.
 */
static void
mpcac97_init_psc(struct mpcac97_softc *sc)
{

	WR1(sc, PSC_CR, CMD_RESET_RX);
	WR1(sc, PSC_CR, CMD_RESET_TX);
	WR1(sc, PSC_CR, CMD_RESET_ERR);
	WR1(sc, PSC_CR, CMD_RESET_MR);

	WR4(sc, PSC_SICR, SICR_NORMAL_AC97);
	WR4(sc, PSC_AC97SLOTS, AC97SLOTS_TX_STEREO);

	WR2(sc, PSC_TFALARM, MPCAC97_TFALARM);
	WR1(sc, PSC_TFCNTL, MPCAC97_TFGRAN);

	WR2(sc, PSC_IMR, 0);	/* no PSC interrupts; BestComm drives playback */
}

/*
 * BestComm transmit-task completion handler.
 */
static int
mpcac97_intr(void *arg)
{
	struct mpcac97_softc *sc = arg;
	int handled = 0;

	mutex_enter(&sc->sc_intr_lock);
	if (!sc->sc_txon) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}

	while ((bestcomm_bd_status(&sc->sc_txring, sc->sc_curblk) &
	    BESTCOMM_BD_READY) == 0) {
		u_int blk = sc->sc_curblk;

		bestcomm_bd_post(&sc->sc_txring, blk,
		    sc->sc_blkbase + blk * sc->sc_blksize, sc->sc_blksize, 0);
		sc->sc_curblk = (blk + 1) % sc->sc_nblk;
		if (sc->sc_pintr != NULL)
			(*sc->sc_pintr)(sc->sc_pintrarg);
		handled = 1;
	}

	/* Restart the task if it ran the ring dry between completions. */
	if (handled)
		bestcomm_task_start(MPCAC97_TX_TASK);

	mutex_exit(&sc->sc_intr_lock);
	return handled;
}

/*
 * ac97(4) host interface.
 */
static int
mpcac97_ac97_attach(void *arg, struct ac97_codec_if *codec)
{
	struct mpcac97_softc *sc = arg;

	sc->sc_codec = codec;
	return 0;
}

static int
mpcac97_ac97_read(void *arg, uint8_t reg, uint16_t *valp)
{
	struct mpcac97_softc *sc = arg;
	int i;

	/* Drop any stale response, then issue the read command. */
	(void)RD4(sc, PSC_AC97DATA);
	WR4(sc, PSC_AC97CMD, AC97CMD_READ | AC97CMD_INDEX(reg));

	for (i = 0; i < AC97_CMD_TIMEOUT; i++) {
		if (RD2(sc, PSC_SR) & AC97_SR_DATA_VALID) {
			*valp = AC97DATA_VALUE(RD4(sc, PSC_AC97DATA));
			return 0;
		}
		DELAY(10);
	}

	*valp = 0xffff;
	return ETIMEDOUT;
}

static int
mpcac97_ac97_write(void *arg, uint8_t reg, uint16_t val)
{
	struct mpcac97_softc *sc = arg;
	int i;

	WR4(sc, PSC_AC97CMD, AC97CMD_WRITE(reg, val));

	/* CMD_SEND is set by the write and cleared once the frame is sent. */
	for (i = 0; i < AC97_CMD_TIMEOUT; i++) {
		if ((RD2(sc, PSC_SR) & AC97_SR_CMD_SEND) == 0)
			return 0;
		DELAY(10);
	}

	return ETIMEDOUT;
}

static int
mpcac97_ac97_reset(void *arg)
{
	struct mpcac97_softc *sc = arg;
	uint16_t val = 0xffff;
	int i, rderr = ETIMEDOUT;

	/*
	 * Cold-reset the external codec by pulsing RES low then high
	 */
	WR1(sc, PSC_CR, CMD_TX_DISABLE | CMD_RX_DISABLE);
	WR1(sc, PSC_OP1, AC97_OP_RES);	/* RES -> low */
	DELAY(1000);
	WR1(sc, PSC_OP0, AC97_OP_RES);	/* RES -> high */
	DELAY(1000);
	WR1(sc, PSC_CR, CMD_TX_ENABLE | CMD_RX_ENABLE);

	/* Wait for the codec's analog/digital subsections to power up. */
	for (i = 0; i < AC97_READY_TIMEOUT; i++) {
		rderr = mpcac97_ac97_read(sc, AC97_REG_POWERDOWN, &val);
		if (rderr == 0 &&
		    (val & AC97_POWER_READY) == AC97_POWER_READY) {
			aprint_debug_dev(sc->sc_dev,
			    "codec ready after ~%dms (powerdown=0x%04x)\n",
			    i, val);
			return 0;
		}
		DELAY(1000);
	}

	/*
	 * Rare, not-reliably-reproducible cold-boot race: the STAC9766 is
	 * occasionally not ready within AC97_READY_TIMEOUT. Try to hunt this
	 * down.
	 */
	aprint_error_dev(sc->sc_dev,
	    "codec not ready after reset: last read %s, powerdown=0x%04x, "
	    "sr=0x%04x\n",
	    rderr ? "TIMED OUT (AC-link silent)" : "ok-but-not-ready",
	    val, RD2(sc, PSC_SR));
	return 0;	/* best effort: let ac97_attach probe the codec ID */
}

static enum ac97_host_flags
mpcac97_ac97_flags(void *arg)
{

	return 0;
}

/*
 * audio(4) hardware interface.
 */
static int
mpcac97_open(void *v, int flags)
{
	struct mpcac97_softc *sc = v;

	/*
	 * Lock the codec's SPDIF control for the duration the device is open
	 */
	sc->sc_codec->vtbl->lock(sc->sc_codec);
	return 0;
}

static void
mpcac97_close(void *v)
{
	struct mpcac97_softc *sc = v;

	sc->sc_codec->vtbl->unlock(sc->sc_codec);
}

static int
mpcac97_query_format(void *v, audio_format_query_t *afp)
{

	return audio_query_format(mpcac97_formats,
	    __arraycount(mpcac97_formats), afp);
}

static int
mpcac97_set_format(void *v, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{

	/*
	 * Only the single advertised hardware format is offered
	 */
	return 0;
}

static int
mpcac97_round_blocksize(void *v, int bs, int mode, const audio_params_t *p)
{

	/* Keep a block a whole number of 8-byte stereo DMA frames. */
	bs &= ~(MPCAC97_FRAME_SIZE - 1);
	if (bs < MPCAC97_FRAME_SIZE)
		bs = MPCAC97_FRAME_SIZE;
	return bs;
}

static size_t
mpcac97_round_buffersize(void *v, int mode, size_t size)
{

	/*
	 * The audio layer sizes the hardware buffer as a whole number of
	 * blocks; keep it intact.
	 */
	return size;
}

static void *
mpcac97_allocm(void *v, int direction, size_t size)
{
	struct mpcac97_softc *sc = v;
	int rseg;

	if (direction != AUMODE_PLAY)	/* playback only */
		return NULL;
	if (sc->sc_dmakva != NULL)	/* one play buffer */
		return NULL;

	if (bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, &sc->sc_dmaseg,
	    1, &rseg, BUS_DMA_WAITOK) != 0)
		return NULL;
	sc->sc_dmanseg = rseg;
	if (bus_dmamem_map(sc->sc_dmat, &sc->sc_dmaseg, rseg, size,
	    &sc->sc_dmakva, BUS_DMA_WAITOK | BUS_DMA_COHERENT) != 0)
		goto free;
	if (bus_dmamap_create(sc->sc_dmat, size, 1, size, 0, BUS_DMA_WAITOK,
	    &sc->sc_dmamap) != 0)
		goto unmap;
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap, sc->sc_dmakva, size,
	    NULL, BUS_DMA_WAITOK) != 0)
		goto destroy;

	sc->sc_dmaphys = sc->sc_dmamap->dm_segs[0].ds_addr;
	sc->sc_dmasize = size;
	return sc->sc_dmakva;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmamap);
	sc->sc_dmamap = NULL;
unmap:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_dmakva, size);
	sc->sc_dmakva = NULL;
free:
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dmaseg, rseg);
	return NULL;
}

static void
mpcac97_freem(void *v, void *ptr, size_t size)
{
	struct mpcac97_softc *sc = v;

	if (ptr != sc->sc_dmakva)
		return;

	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmamap);
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmamap);
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_dmakva, size);
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dmaseg, sc->sc_dmanseg);
	sc->sc_dmamap = NULL;
	sc->sc_dmakva = NULL;
	sc->sc_dmasize = 0;
}

static int
mpcac97_get_props(void *v)
{

	return AUDIO_PROP_PLAYBACK;
}

static int
mpcac97_trigger_output(void *v, void *start, void *end, int blksize,
    void (*intr)(void *), void *arg, const audio_params_t *param)
{
	struct mpcac97_softc *sc = v;
	bus_addr_t base;
	u_int i, nblk;

	nblk = ((char *)end - (char *)start) / blksize;
	if (nblk == 0 || nblk > MPCAC97_MAX_NBLK)
		return EINVAL;
	if ((char *)start < (char *)sc->sc_dmakva ||
	    (char *)start + nblk * blksize >
	    (char *)sc->sc_dmakva + sc->sc_dmasize)
		return EINVAL;

	base = sc->sc_dmaphys + ((char *)start - (char *)sc->sc_dmakva);

	/* (Re)build the BD ring: one descriptor per audio block. */
	bestcomm_bd_teardown(&sc->sc_txring);
	if (bestcomm_bd_setup(&sc->sc_txring, MPCAC97_TX_TASK,
	    &mpcac97_tx_layout, sc->sc_pa + AC97_FIFO_TX, nblk, blksize,
	    sizeof(uint32_t), MPCAC97_TX_INITIATOR, MPCAC97_TX_PRIO) != 0)
		return ENOMEM;

	/*
	 * GEN_TX_BD is a runtime-configurable task
	 */
	bestcomm_task_set_initiator(MPCAC97_TX_TASK, MPCAC97_TX_INITIATOR,
	    mpcac97_tx_drd, __arraycount(mpcac97_tx_drd));

	sc->sc_blkbase = base;
	sc->sc_blksize = blksize;
	sc->sc_nblk = nblk;
	sc->sc_curblk = 0;
	sc->sc_pintr = intr;
	sc->sc_pintrarg = arg;
	sc->sc_txon = true;

	for (i = 0; i < nblk; i++)
		bestcomm_bd_post(&sc->sc_txring, i, base + i * blksize,
		    blksize, 0);

	bestcomm_task_start(MPCAC97_TX_TASK);
	return 0;
}

static int
mpcac97_halt_output(void *v)
{
	struct mpcac97_softc *sc = v;

	sc->sc_txon = false;
	sc->sc_pintr = NULL;
	sc->sc_pintrarg = NULL;
	bestcomm_bd_teardown(&sc->sc_txring);
	return 0;
}

static int
mpcac97_getdev(void *v, struct audio_device *adp)
{

	strlcpy(adp->name, "MPC5200 AC97", sizeof(adp->name));
	strlcpy(adp->version, "", sizeof(adp->version));
	strlcpy(adp->config, "mpcac97", sizeof(adp->config));
	return 0;
}

static int
mpcac97_set_port(void *v, mixer_ctrl_t *mc)
{
	struct mpcac97_softc *sc = v;

	return sc->sc_codec->vtbl->mixer_set_port(sc->sc_codec, mc);
}

static int
mpcac97_get_port(void *v, mixer_ctrl_t *mc)
{
	struct mpcac97_softc *sc = v;

	return sc->sc_codec->vtbl->mixer_get_port(sc->sc_codec, mc);
}

static int
mpcac97_query_devinfo(void *v, mixer_devinfo_t *di)
{
	struct mpcac97_softc *sc = v;

	return sc->sc_codec->vtbl->query_devinfo(sc->sc_codec, di);
}

static void
mpcac97_get_locks(void *v, kmutex_t **intr, kmutex_t **thread)
{
	struct mpcac97_softc *sc = v;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}
