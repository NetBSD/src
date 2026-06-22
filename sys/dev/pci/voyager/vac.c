/* $NetBSD: vac.c,v 1.2 2026/06/22 15:12:59 rkujawa Exp $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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
 * Driver for the SM502 AC'97 audio interface
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vac.c,v 1.2 2026/06/22 15:12:59 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/sysctl.h>
#include <sys/audioio.h>

#include <dev/audio/audio_if.h>

#include <dev/ic/ac97var.h>
#include <dev/ic/ac97reg.h>

#include <dev/ic/sm502reg.h>
#include <dev/pci/voyagervar.h>

/* PCM playback format: AC'97 fixed 48 kHz, 16-bit, stereo. */
#define	VAC_RATE	48000
#define	VAC_CHANNELS	2
#define	VAC_PRECISION	16
#define	VAC_FRAMESIZE	(VAC_CHANNELS * (VAC_PRECISION / 8))	/* 4 bytes */

/* poll when waiting on the codec */
#define	VAC_CODEC_TIMEOUT	1000

/*
 * 8051 polled protocol, via 4KB dual-port SRAM 
 *
 * Two PCM buffers selected by command parity
 */
#define	VAC_DP_BASE		SM502_uC_SRAM_DUALPORT
#define	VAC_DP_H_CMD		0x00	/* u8  host: command sequence */
#define	VAC_DP_H_DONE		0x01	/* u8  8051: last finished command */
#define	VAC_DP_H_ALIVE		0x03	/* u8  8051: VAC_DP_ALIVE_MAGIC when up */
#define	VAC_DP_H_FC0L		0x04	/* u16 host: buf0 frame count, LE */
#define	VAC_DP_H_FC0H		0x05
#define	VAC_DP_H_FC1L		0x06	/* u16 host: buf1 frame count, LE */
#define	VAC_DP_H_FC1H		0x07
#define	VAC_DP_BUF0		0x10
#define	VAC_DP_MAXFRAMES	504	/* per buffer; 504*4=0x7e0, both fit 4KB */
#define	VAC_DP_BUFBYTES		(VAC_DP_MAXFRAMES * VAC_FRAMESIZE)
#define	VAC_DP_BUF1		(VAC_DP_BUF0 + VAC_DP_BUFBYTES)	/* 0x7f0 */
#define	VAC_DP_ALIVE_MAGIC	0x51

struct vac_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_regh;	/* BAR 0x14: MMIO registers */
	void			*sc_parent;	/* voyager softc, for gpio/power */

	kmutex_t		sc_lock;	/* thread context */
	kmutex_t		sc_intr_lock;	/* "interrupt" context */

	struct ac97_host_if	sc_ac97_host;
	struct ac97_codec_if	*sc_ac97_codec;
	device_t		sc_audiodev;

	/* MI playback ring (filled by the audio layer, consumed by the feeder) */
	bool			sc_active;	/* playback in progress */
	uint8_t			*sc_pstart;
	uint8_t			*sc_pend;
	uint8_t			*sc_pcur;
	int			sc_pblksize;
	void			(*sc_pint)(void *);
	void			*sc_pintarg;

	/* 8051 interface */
	callout_t		sc_feed_ch;	/* polls H_DONE, refills buffers */
	int			sc_feed_ticks;	/* callout period */
	uint8_t			*sc_dpstage;	/* staging for one dual-port buf */
	uint8_t			sc_cmd_seq;	/* last command we queued */
	uint8_t			sc_done_seen;	/* last H_DONE we acted on */
	bool			sc_8051_ok;	/* feeder firmware responded */
	int			sc_underruns;	/* 8051 ran dry while playing */

	struct sysctllog	*sc_sysctllog;
};

#define	VAC_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_memt, (sc)->sc_regh, (reg))
#define	VAC_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_memt, (sc)->sc_regh, (reg), (val))
#define	VAC_DP_R8(sc, off) \
	bus_space_read_1((sc)->sc_memt, (sc)->sc_regh, VAC_DP_BASE + (off))
#define	VAC_DP_W8(sc, off, v) \
	bus_space_write_1((sc)->sc_memt, (sc)->sc_regh, VAC_DP_BASE + (off), (v))

/*
 * Embedded 8051 firmware
 * assemble from vac8051.asm with naken_asm (-type bin)
 */
static const uint8_t vac8051_fw[] = {
	0x90, 0x30, 0x03, 0x74, 0x51, 0xf0, 0x90, 0x30, 0x01, 0xe4, 0xf0, 0x7f,
	0x00, 0x90, 0x30, 0x00, 0xe0, 0xfe, 0x6f, 0x60, 0xf8, 0xee, 0x54, 0x01,
	0x60, 0x10, 0x90, 0x30, 0x06, 0xe0, 0xfc, 0x90, 0x30, 0x07, 0xe0, 0xfd,
	0x7a, 0xf0, 0x7b, 0x37, 0x80, 0x0e, 0x90, 0x30, 0x04, 0xe0, 0xfc, 0x90,
	0x30, 0x05, 0xe0, 0xfd, 0x7a, 0x10, 0x7b, 0x30, 0x78, 0x00, 0x79, 0x00,
	0x90, 0x91, 0x45, 0xe0, 0x54, 0x08, 0x60, 0x04, 0xd9, 0xf6, 0xd8, 0xf4,
	0x8a, 0x82, 0x8b, 0x83, 0xe0, 0xf5, 0x20, 0xa3, 0xe0, 0xf5, 0x21, 0xa3,
	0xe0, 0xf5, 0x24, 0xa3, 0xe0, 0xf5, 0x25, 0xa3, 0xaa, 0x82, 0xab, 0x83,
	0x90, 0x91, 0x00, 0xe4, 0xf0, 0xa3, 0x74, 0x98, 0xf0, 0xe5, 0x20, 0xc4,
	0xf5, 0x22, 0xe5, 0x21, 0xc4, 0xf5, 0x23, 0x90, 0x91, 0x0c, 0xe5, 0x22,
	0x54, 0xf0, 0xf0, 0xa3, 0xe5, 0x22, 0x54, 0x0f, 0xf5, 0x26, 0xe5, 0x23,
	0x54, 0xf0, 0x45, 0x26, 0xf0, 0xa3, 0xe5, 0x23, 0x54, 0x0f, 0xf0, 0xa3,
	0xe4, 0xf0, 0xe5, 0x24, 0xc4, 0xf5, 0x22, 0xe5, 0x25, 0xc4, 0xf5, 0x23,
	0x90, 0x91, 0x10, 0xe5, 0x22, 0x54, 0xf0, 0xf0, 0xa3, 0xe5, 0x22, 0x54,
	0x0f, 0xf5, 0x26, 0xe5, 0x23, 0x54, 0xf0, 0x45, 0x26, 0xf0, 0xa3, 0xe5,
	0x23, 0x54, 0x0f, 0xf0, 0xa3, 0xe4, 0xf0, 0xec, 0x70, 0x01, 0x1d, 0x1c,
	0xec, 0x4d, 0x60, 0x03, 0x02, 0x00, 0x38, 0x90, 0x30, 0x01, 0xee, 0xf0,
	0xff, 0x02, 0x00, 0x0d
};

static int	vac_match(device_t, cfdata_t, void *);
static void	vac_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(vac, sizeof(struct vac_softc),
    vac_match, vac_attach, NULL, NULL);

static const struct audio_format vac_format = {
	.mode = AUMODE_PLAY,
	.encoding = AUDIO_ENCODING_SLINEAR_LE,
	.validbits = VAC_PRECISION,
	.precision = VAC_PRECISION,
	.channels = VAC_CHANNELS,
	.channel_mask = AUFMT_STEREO,
	.frequency_type = 1,
	.frequency = { VAC_RATE }
};

/*
 * AC'97 codec register access.
 */
static int
vac_ac97_read(void *priv, uint8_t reg, uint16_t *val)
{
	struct vac_softc * const sc = priv;
	uint32_t addr, rx;
	int i;

	addr = ((uint32_t)reg << SM502_AC97_ADDR_SHIFT) & SM502_AC97_ADDR_MASK;

	/* Mark frame + slot 1 valid and issue a read of the register. */
	VAC_WRITE(sc, SM502_AC97_TX_TAG,
	    SM502_AC97_FRAME_VALID | SM502_AC97_S1_VALID);
	VAC_WRITE(sc, SM502_AC97_TX_ADDR, addr | SM502_AC97_READ);

	/* Wait for the codec to echo the register index back in RX slot 1. */
	for (i = 0; i < VAC_CODEC_TIMEOUT; i++) {
		rx = VAC_READ(sc, SM502_AC97_RX_ADDR);
		if ((rx & SM502_AC97_ADDR_MASK) == addr)
			break;
		DELAY(1);
	}
	if (i == VAC_CODEC_TIMEOUT)
		return 1;

	*val = (VAC_READ(sc, SM502_AC97_RX_DATA) & SM502_AC97_DATA_MASK) >> 4;
	return 0;
}

static int
vac_ac97_write(void *priv, uint8_t reg, uint16_t val)
{
	struct vac_softc * const sc = priv;
	uint32_t addr;

	addr = ((uint32_t)reg << SM502_AC97_ADDR_SHIFT) & SM502_AC97_ADDR_MASK;

	/* Mark frame + slots 1 (address) and 2 (data) valid. */
	VAC_WRITE(sc, SM502_AC97_TX_TAG,
	    SM502_AC97_FRAME_VALID | SM502_AC97_S1_VALID | SM502_AC97_S2_VALID);
	VAC_WRITE(sc, SM502_AC97_TX_DATA,
	    ((uint32_t)val << 4) & SM502_AC97_DATA_MASK);
	VAC_WRITE(sc, SM502_AC97_TX_ADDR, addr);

	DELAY(100);
	return 0;
}

static int
vac_ac97_reset(void *priv)
{
	struct vac_softc * const sc = priv;
	uint32_t reg;
	int i;

	aprint_debug_dev(sc->sc_dev, "ac97 control at reset entry 0x%08x\n",
	    VAC_READ(sc, SM502_AC97_CONTROL));

	/*
	 * Cold reset the codec with the controller enabled: assert RESET#
	 * (COLD_RESET) for well over the 1us minimum, then release it.
	 */
	VAC_WRITE(sc, SM502_AC97_CONTROL,
	    SM502_AC97_ENABLE | SM502_AC97_COLD_RESET);
	DELAY(20);
	VAC_WRITE(sc, SM502_AC97_CONTROL, SM502_AC97_ENABLE);

	/*
	 * Wait (up to ~250ms) for the codec to start the bit clock and the
	 * link to reach the active state.
	 */
	for (i = 0; i < 2500; i++) {
		reg = VAC_READ(sc, SM502_AC97_CONTROL);
		if ((reg & SM502_AC97_BCLK_RUNNING) != 0 &&
		    (reg & SM502_AC97_STATUS_MASK) == SM502_AC97_STATUS_ON) {
			aprint_debug_dev(sc->sc_dev,
			    "AC-link up (control 0x%08x)\n", reg);
			return 0;
		}
		DELAY(100);
	}

	aprint_error_dev(sc->sc_dev,
	    "AC-link did not come ready (control 0x%08x)\n", reg);
	return 1;
}

static int
vac_ac97_attach(void *priv, struct ac97_codec_if *codec)
{
	struct vac_softc * const sc = priv;

	sc->sc_ac97_codec = codec;
	return 0;
}

static void
vac_unmute(struct vac_softc *sc)
{
	struct ac97_codec_if * const codec = sc->sc_ac97_codec;
	static const struct {
		const char *class;
		const char *device;
	} ports[] = {
		{ AudioCoutputs, AudioNmaster },
		{ AudioCinputs,  AudioNdac },
	};
	mixer_ctrl_t mc;
	int i, idx;

	mutex_enter(&sc->sc_lock);
	for (i = 0; i < (int)__arraycount(ports); i++) {
		idx = codec->vtbl->get_portnum_by_name(codec,
		    ports[i].class, ports[i].device, NULL);
		if (idx >= 0) {
			memset(&mc, 0, sizeof(mc));
			mc.dev = idx;
			mc.type = AUDIO_MIXER_VALUE;
			mc.un.value.num_channels = 2;
			mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT] = 200;
			mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = 200;
			(void)codec->vtbl->mixer_set_port(codec, &mc);
		}
		idx = codec->vtbl->get_portnum_by_name(codec,
		    ports[i].class, ports[i].device, AudioNmute);
		if (idx >= 0) {
			memset(&mc, 0, sizeof(mc));
			mc.dev = idx;
			mc.type = AUDIO_MIXER_ENUM;
			mc.un.ord = 0;		/* off = not muted */
			(void)codec->vtbl->mixer_set_port(codec, &mc);
		}
	}
	mutex_exit(&sc->sc_lock);
}

/*
 * MI audio interface.
 */

static int
vac_query_format(void *priv, audio_format_query_t *afp)
{

	return audio_query_format(&vac_format, 1, afp);
}

static int
vac_set_format(void *priv, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{

	/* Only one format; nothing to do. */
	return 0;
}

static int
vac_getdev(void *priv, struct audio_device *ad)
{

	snprintf(ad->name, sizeof(ad->name), "SM502");
	snprintf(ad->version, sizeof(ad->version), "AC97");
	snprintf(ad->config, sizeof(ad->config), "vac");
	return 0;
}

static int
vac_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct vac_softc * const sc = priv;

	return sc->sc_ac97_codec->vtbl->mixer_set_port(sc->sc_ac97_codec, mc);
}

static int
vac_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct vac_softc * const sc = priv;

	return sc->sc_ac97_codec->vtbl->mixer_get_port(sc->sc_ac97_codec, mc);
}

static int
vac_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	struct vac_softc * const sc = priv;

	return sc->sc_ac97_codec->vtbl->query_devinfo(sc->sc_ac97_codec, di);
}

static int
vac_get_props(void *priv)
{

	return AUDIO_PROP_PLAYBACK;
}

static int
vac_round_blocksize(void *priv, int blk, int mode,
    const audio_params_t *param)
{

	/*
	 * Play one MI block per dual-port buffer
	 */
	if (blk > VAC_DP_BUFBYTES)
		blk = VAC_DP_BUFBYTES;
	blk &= ~(VAC_FRAMESIZE - 1);
	if (blk < VAC_FRAMESIZE)
		blk = VAC_FRAMESIZE;
	return blk;
}

/*
 * Queue one block from the MI ring 
 */
static void
vac_dp_queue(struct vac_softc *sc)
{
	const uint8_t cmd = sc->sc_cmd_seq + 1;
	uint8_t *p = sc->sc_pcur;
	bus_size_t base;
	int n, i;

	n = sc->sc_pblksize / VAC_FRAMESIZE;
	if (n > VAC_DP_MAXFRAMES)
		n = VAC_DP_MAXFRAMES;

	/* Copy PCM into the staging buffer (raw 16-bit LE), handling wrap. */
	for (i = 0; i < n; i++) {
		uint8_t * const d = sc->sc_dpstage + i * VAC_FRAMESIZE;

		d[0] = p[0];	/* left low */
		d[1] = p[1];	/* left high */
		d[2] = p[2];	/* right low */
		d[3] = p[3];	/* right high */
		p += VAC_FRAMESIZE;
		if (p >= sc->sc_pend)
			p = sc->sc_pstart;
	}
	sc->sc_pcur = p;

	base = VAC_DP_BASE + ((cmd & 1) ? VAC_DP_BUF1 : VAC_DP_BUF0);
	bus_space_write_region_1(sc->sc_memt, sc->sc_regh, base,
	    sc->sc_dpstage, n * VAC_FRAMESIZE);

	if (cmd & 1) {
		VAC_DP_W8(sc, VAC_DP_H_FC1L, n & 0xff);
		VAC_DP_W8(sc, VAC_DP_H_FC1H, (n >> 8) & 0xff);
	} else {
		VAC_DP_W8(sc, VAC_DP_H_FC0L, n & 0xff);
		VAC_DP_W8(sc, VAC_DP_H_FC0H, (n >> 8) & 0xff);
	}

	/* The data and count must be visible before the command bump. */
	bus_space_barrier(sc->sc_memt, sc->sc_regh, VAC_DP_BASE, 0x1000,
	    BUS_SPACE_BARRIER_WRITE);
	VAC_DP_W8(sc, VAC_DP_H_CMD, cmd);
	sc->sc_cmd_seq = cmd;
}

/*
 * Poll the 8051's done counter
 */
static void
vac_feed_callout(void *arg)
{
	struct vac_softc * const sc = arg;
	uint8_t done;
	int n;

	mutex_enter(&sc->sc_intr_lock);
	if (!sc->sc_active) {
		mutex_exit(&sc->sc_intr_lock);
		return;
	}

	done = VAC_DP_R8(sc, VAC_DP_H_DONE);
	if (sc->sc_cmd_seq == done)
		sc->sc_underruns++;	/* 8051 caught up: nothing queued */

	n = (uint8_t)(done - sc->sc_done_seen);
	sc->sc_done_seen = done;
	while (n-- > 0 && sc->sc_active && sc->sc_pint != NULL)
		sc->sc_pint(sc->sc_pintarg);

	while ((uint8_t)(sc->sc_cmd_seq - done) <= 1 && sc->sc_active)
		vac_dp_queue(sc);

	callout_schedule(&sc->sc_feed_ch, sc->sc_feed_ticks);
	mutex_exit(&sc->sc_intr_lock);
}

static int
vac_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct vac_softc * const sc = priv;
	int i;

	/*
	 * audio calls trigger_output with both sc_lock and sc_intr_lock held 
	 */
	KASSERT(mutex_owned(&sc->sc_intr_lock));
	KASSERT(mutex_owned(&sc->sc_lock));

	if (!sc->sc_8051_ok)
		return ENXIO;

	/*
	 * Restart the 8051 cleanly for this playback
	 */
	VAC_WRITE(sc, SM502_uC_RESET, 0);
	for (i = 0; i < 16; i++)
		VAC_DP_W8(sc, i, 0);
	VAC_WRITE(sc, SM502_uC_RESET, SM502_uC_ENABLE);
	for (i = 0; i < 5000 &&
	    VAC_DP_R8(sc, VAC_DP_H_ALIVE) != VAC_DP_ALIVE_MAGIC; i++)
		DELAY(10);

	sc->sc_pstart = start;
	sc->sc_pend = end;
	sc->sc_pcur = start;
	sc->sc_pblksize = blksize;
	sc->sc_pint = intr;
	sc->sc_pintarg = intrarg;

	sc->sc_cmd_seq = 0;
	sc->sc_done_seen = 0;
	sc->sc_underruns = 0;
	sc->sc_active = true;

	/* Prime both buffers, then poll for completion from the callout. */
	vac_dp_queue(sc);
	vac_dp_queue(sc);
	callout_schedule(&sc->sc_feed_ch, sc->sc_feed_ticks);

	return 0;
}

static int
vac_halt_output(void *priv)
{
	struct vac_softc * const sc = priv;

	/*
	 * called with both sc_lock and sc_intr_lock held
	 */
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	sc->sc_active = false;
	sc->sc_pint = NULL;
	callout_stop(&sc->sc_feed_ch);

	/* Park the 8051 so the host codec/mixer path is live while idle. */
	VAC_WRITE(sc, SM502_uC_RESET, 0);
	return 0;
}

static void
vac_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct vac_softc * const sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static const struct audio_hw_if vac_hw_if = {
	.query_format = vac_query_format,
	.set_format = vac_set_format,
	.getdev = vac_getdev,
	.set_port = vac_set_port,
	.get_port = vac_get_port,
	.query_devinfo = vac_query_devinfo,
	.get_props = vac_get_props,
	.round_blocksize = vac_round_blocksize,
	.trigger_output = vac_trigger_output,
	.halt_output = vac_halt_output,
	.get_locks = vac_get_locks,
};

/* Load the firmware blob as 32-bit LE words at (h, base). */
static void
vac_load_fw(struct vac_softc *sc, bus_space_handle_t h, bus_size_t base)
{
	size_t i;

	for (i = 0; i < sizeof(vac8051_fw); i += 4) {
		uint8_t b0 = vac8051_fw[i];
		uint8_t b1 = (i + 1 < sizeof(vac8051_fw)) ? vac8051_fw[i + 1] : 0;
		uint8_t b2 = (i + 2 < sizeof(vac8051_fw)) ? vac8051_fw[i + 2] : 0;
		uint8_t b3 = (i + 3 < sizeof(vac8051_fw)) ? vac8051_fw[i + 3] : 0;

		bus_space_write_4(sc->sc_memt, h, base + i,
		    b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
	}
}

static void
vac_8051_bringup(struct vac_softc *sc)
{
	uint32_t v;
	uint8_t alive;
	int i;

	/* Hold the 8051 in reset... mode = SRAM on, AC97, fastest clock (div2). */
	VAC_WRITE(sc, SM502_uC_RESET, 0);
	v = VAC_READ(sc, SM502_uC_MODE_SELECT);
	v &= ~(SM502_uC_SRAM_DISABLE | SM502_uC_CODEC_I2S | SM502_uC_CLOCK_MASK);
	VAC_WRITE(sc, SM502_uC_MODE_SELECT, v);

	/* Load the feeder into the program SRAM (host R/W only while in reset). */
	vac_load_fw(sc, sc->sc_regh, SM502_uC_SRAM_PROG);

	/* Clear the dual-port protocol header before the 8051 starts. */
	for (i = 0; i < 16; i++)
		VAC_DP_W8(sc, i, 0);

	/* Release the 8051 and give it time to post its alive marker. */
	VAC_WRITE(sc, SM502_uC_RESET, SM502_uC_ENABLE);
	DELAY(2000);

	alive = VAC_DP_R8(sc, VAC_DP_H_ALIVE);
	sc->sc_8051_ok = (alive == VAC_DP_ALIVE_MAGIC);
	aprint_verbose_dev(sc->sc_dev, "8051 feeder %s (alive=0x%02x)\n",
	    sc->sc_8051_ok ? "loaded" : "not responding", alive);
	if (!sc->sc_8051_ok)
		aprint_error_dev(sc->sc_dev,
		    "8051 feeder did not start; playback unavailable\n");

	/* Park the 8051 while idle (keeps the host codec/mixer path live). */
	VAC_WRITE(sc, SM502_uC_RESET, 0);
}

#ifdef VAC_DEBUG
/*
 * Diagnostics: dump the AC97 link control + key codec registers (read while the
 * 8051 is idle; codec access is dead while it runs).
 */
static int
vac_sysctl_diag(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct vac_softc * const sc = node.sysctl_data;
	uint16_t r02 = 0, r04 = 0, r18 = 0, r26 = 0, r2a = 0, r2c = 0, r7a = 0;
	char buf[200];

	vac_ac97_read(sc, 0x02, &r02);	/* master volume */
	vac_ac97_read(sc, 0x04, &r04);	/* aux/headphone volume */
	vac_ac97_read(sc, 0x18, &r18);	/* PCM out volume */
	vac_ac97_read(sc, 0x26, &r26);	/* powerdown ctrl/status */
	vac_ac97_read(sc, 0x2a, &r2a);	/* extended audio ctrl (VRA) */
	vac_ac97_read(sc, 0x2c, &r2c);	/* PCM front DAC rate */
	vac_ac97_read(sc, 0x7a, &r7a);	/* ALC655 vendor (EAPD etc.) */

	snprintf(buf, sizeof(buf),
	    "acctl=%08x mode=%08x reset=%08x | "
	    "m02=%04x hp04=%04x pcm18=%04x pwr26=%04x ext2a=%04x rate2c=%04x ven7a=%04x",
	    VAC_READ(sc, SM502_AC97_CONTROL),
	    VAC_READ(sc, SM502_uC_MODE_SELECT),
	    VAC_READ(sc, SM502_uC_RESET),
	    r02, r04, r18, r26, r2a, r2c, r7a);
	node.sysctl_data = buf;
	node.sysctl_size = strlen(buf) + 1;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}
#endif /* VAC_DEBUG */

static void
vac_sysctl_setup(struct vac_softc *sc)
{
	const struct sysctlnode *node = NULL;

	if (sysctl_createv(&sc->sc_sysctllog, 0, NULL, &node,
	    0, CTLTYPE_NODE, "vac", SYSCTL_DESCR("SM502 AC97-link audio"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL) != 0 || node == NULL)
		return;

	sysctl_createv(&sc->sc_sysctllog, 0, NULL, NULL,
	    CTLFLAG_READONLY, CTLTYPE_INT, "underruns",
	    SYSCTL_DESCR("times the 8051 ran dry during the last playback"),
	    NULL, 0, &sc->sc_underruns, 0, CTL_HW, node->sysctl_num,
	    CTL_CREATE, CTL_EOL);
#ifdef VAC_DEBUG
	sysctl_createv(&sc->sc_sysctllog, 0, NULL, NULL,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "diag",
	    SYSCTL_DESCR("AC97 link + codec register dump (read while idle)"),
	    vac_sysctl_diag, 0, (void *)sc, 0, CTL_HW, node->sysctl_num,
	    CTL_CREATE, CTL_EOL);
#endif
}

static int
vac_match(device_t parent, cfdata_t match, void *aux)
{
	struct voyager_attach_args *vaa = aux;

	if (strcmp(vaa->vaa_name, "vac") == 0)
		return 100;
	return 0;
}

static void
vac_attach(device_t parent, device_t self, void *aux)
{
	struct vac_softc * const sc = device_private(self);
	struct voyager_attach_args *vaa = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_memt = vaa->vaa_tag;
	sc->sc_regh = vaa->vaa_regh;
	sc->sc_parent = device_private(parent);

	aprint_naive("\n");
	aprint_normal(": SM502 AC97-link\n");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_AUDIO);

	callout_init(&sc->sc_feed_ch, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_feed_ch, vac_feed_callout, sc);
	sc->sc_feed_ticks = mstohz(5);
	if (sc->sc_feed_ticks < 1)
		sc->sc_feed_ticks = 1;
	sc->sc_dpstage = kmem_alloc(VAC_DP_BUFBYTES, KM_SLEEP);

	aprint_debug_dev(self,
	    "power: ctrl 0x%08x cur-gate 0x%08x gate0 0x%08x gate1 0x%08x\n",
	    VAC_READ(sc, SM502_POWER_MODE_CONTROL),
	    VAC_READ(sc, SM502_CURRENT_GATE),
	    VAC_READ(sc, SM502_POWER_MODE0_GATE),
	    VAC_READ(sc, SM502_POWER_MODE1_GATE));

	VAC_WRITE(sc, SM502_POWER_MODE0_GATE,
	    VAC_READ(sc, SM502_POWER_MODE0_GATE) |
	    SM502_GATE_AUDIO_ENABLE | SM502_GATE_8051_ENABLE);
	VAC_WRITE(sc, SM502_POWER_MODE1_GATE,
	    VAC_READ(sc, SM502_POWER_MODE1_GATE) |
	    SM502_GATE_AUDIO_ENABLE | SM502_GATE_8051_ENABLE);
	DELAY(1000);

	/* Route the shared audio pins to the AC97-link (not GPIO). */
	voyager_control_gpio(sc->sc_parent, 0xffffffff, SM502_AUDIO_GPIO_MASK);

	/* Select AC97 mode for the link (CODEC_I2S clear). */
	VAC_WRITE(sc, SM502_uC_MODE_SELECT,
	    VAC_READ(sc, SM502_uC_MODE_SELECT) & ~SM502_uC_CODEC_I2S);

	sc->sc_ac97_host.arg = sc;
	sc->sc_ac97_host.attach = vac_ac97_attach;
	sc->sc_ac97_host.read = vac_ac97_read;
	sc->sc_ac97_host.write = vac_ac97_write;
	sc->sc_ac97_host.reset = vac_ac97_reset;

	error = ac97_attach(&sc->sc_ac97_host, self, &sc->sc_lock);
	if (error) {
		aprint_error_dev(self, "couldn't attach codec (%d)\n", error);
		return;
	}

	vac_sysctl_setup(sc);

	/* Load the 8051 binary. */
	vac_8051_bringup(sc);

	sc->sc_audiodev = audio_attach_mi(&vac_hw_if, sc, self);

	/* Unmute master + DAC now that the codec/mixer stack is up (8051 idle). */
	vac_unmute(sc);
}
