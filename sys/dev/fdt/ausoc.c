/* $NetBSD: ausoc.c,v 1.3.2.2 2018/05/21 04:36:05 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ausoc.c,v 1.3.2.2 2018/05/21 04:36:05 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/audio_dai.h>

#include <dev/fdt/fdtvar.h>

static const char *compatible[] = { "simple-audio-card", NULL };

struct ausoc_link {
	const char		*link_name;

	audio_dai_tag_t		link_cpu;
	audio_dai_tag_t		link_codec;
	audio_dai_tag_t		*link_aux;
	u_int			link_naux;

	u_int			link_mclk_fs;

	kmutex_t		link_lock;
	kmutex_t		link_intr_lock;
};

struct ausoc_softc {
	device_t		sc_dev;
	int			sc_phandle;
	const char		*sc_name;

	struct ausoc_link	*sc_link;
	u_int			sc_nlink;
};

static void
ausoc_close(void *priv)
{
	struct ausoc_link * const link = priv;
	u_int aux;

	for (aux = 0; aux < link->link_naux; aux++)
		audio_dai_close(link->link_aux[aux]);
	audio_dai_close(link->link_codec);
	audio_dai_close(link->link_cpu);
}

static int
ausoc_open(void *priv, int flags)
{
	struct ausoc_link * const link = priv;
	u_int aux;
	int error;

	error = audio_dai_open(link->link_cpu, flags);
	if (error)
		goto failed;

	error = audio_dai_open(link->link_codec, flags);
	if (error)
		goto failed;

	for (aux = 0; aux < link->link_naux; aux++) {
		error = audio_dai_open(link->link_aux[aux], flags);
		if (error)
			goto failed;
	}

	return 0;

failed:
	ausoc_close(priv);
	return error;
}

static int
ausoc_drain(void *priv)
{
	struct ausoc_link * const link = priv;

	return audio_dai_drain(link->link_cpu);
}

static int
ausoc_query_encoding(void *priv, struct audio_encoding *ae)
{
	struct ausoc_link * const link = priv;

	return audio_dai_query_encoding(link->link_cpu, ae);
}

static int
ausoc_set_params(void *priv, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct ausoc_link * const link = priv;
	int error;

	error = audio_dai_set_params(link->link_cpu, setmode,
	    usemode, play, rec, pfil, rfil);
	if (error)
		return error;

	return audio_dai_set_params(link->link_codec, setmode,
	    usemode, play, rec, pfil, rfil);
}

static int
ausoc_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct ausoc_link * const link = priv;

	return audio_dai_set_port(link->link_codec, mc);
}

static int
ausoc_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct ausoc_link * const link = priv;

	return audio_dai_get_port(link->link_codec, mc);
}

static int
ausoc_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	struct ausoc_link * const link = priv;

	return audio_dai_query_devinfo(link->link_codec, di);
}

static void *
ausoc_allocm(void *priv, int dir, size_t size)
{
	struct ausoc_link * const link = priv;

	return audio_dai_allocm(link->link_cpu, dir, size);
}

static void
ausoc_freem(void *priv, void *addr, size_t size)
{
	struct ausoc_link * const link = priv;

	return audio_dai_freem(link->link_cpu, addr, size);
}

static paddr_t
ausoc_mappage(void *priv, void *addr, off_t off, int prot)
{
	struct ausoc_link * const link = priv;

	return audio_dai_mappage(link->link_cpu, addr, off, prot);
}

static int
ausoc_getdev(void *priv, struct audio_device *adev)
{
	struct ausoc_link * const link = priv;

	/* Defaults */
	snprintf(adev->name, sizeof(adev->name), "%s", link->link_name);
	snprintf(adev->version, sizeof(adev->version), "");
	snprintf(adev->config, sizeof(adev->config), "ausoc");

	/* Codec can override */
	(void)audio_dai_getdev(link->link_codec, adev);

	return 0;
}

static int
ausoc_get_props(void *priv)
{
	struct ausoc_link * const link = priv;

	return audio_dai_get_props(link->link_cpu);
}

static int
ausoc_round_blocksize(void *priv, int bs, int mode,
    const audio_params_t *params)
{
	struct ausoc_link * const link = priv;

	return audio_dai_round_blocksize(link->link_cpu, bs, mode, params);
}

static size_t
ausoc_round_buffersize(void *priv, int dir, size_t bufsize)
{
	struct ausoc_link * const link = priv;

	return audio_dai_round_buffersize(link->link_cpu, dir, bufsize);
}

static int
ausoc_halt_output(void *priv)
{
	struct ausoc_link * const link = priv;
	u_int n;

	for (n = 0; n < link->link_naux; n++)
		audio_dai_halt(link->link_aux[n], AUMODE_PLAY);

	audio_dai_halt(link->link_codec, AUMODE_PLAY);

	return audio_dai_halt(link->link_cpu, AUMODE_PLAY);
}

static int
ausoc_halt_input(void *priv)
{
	struct ausoc_link * const link = priv;
	u_int n;

	for (n = 0; n < link->link_naux; n++)
		audio_dai_halt(link->link_aux[n], AUMODE_RECORD);

	audio_dai_halt(link->link_codec, AUMODE_RECORD);

	return audio_dai_halt(link->link_cpu, AUMODE_RECORD);
}

static int
ausoc_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct ausoc_link * const link = priv;
	u_int n, rate;
	int error;

	if (link->link_mclk_fs) {
		rate = params->sample_rate * link->link_mclk_fs;
		error = audio_dai_set_sysclk(link->link_codec, rate,
		    AUDIO_DAI_CLOCK_IN);
		if (error)
			goto failed;
		error = audio_dai_set_sysclk(link->link_cpu, rate,
		    AUDIO_DAI_CLOCK_OUT);
		if (error)
			goto failed;
	}

	for (n = 0; n < link->link_naux; n++) {
		error = audio_dai_trigger(link->link_aux[n], start, end,
		    blksize, intr, intrarg, params, AUMODE_PLAY);
		if (error)
			goto failed;
	}
	error = audio_dai_trigger(link->link_codec, start, end, blksize,
	    intr, intrarg, params, AUMODE_PLAY);
	if (error)
		goto failed;

	return audio_dai_trigger(link->link_cpu, start, end, blksize,
	    intr, intrarg, params, AUMODE_PLAY);

failed:
	ausoc_halt_output(priv);
	return error;
}

static int
ausoc_trigger_input(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct ausoc_link * const link = priv;
	u_int n, rate;
	int error;

	if (link->link_mclk_fs) {
		rate = params->sample_rate * link->link_mclk_fs;
		error = audio_dai_set_sysclk(link->link_codec, rate,
		    AUDIO_DAI_CLOCK_IN);
		if (error)
			goto failed;
		error = audio_dai_set_sysclk(link->link_cpu, rate,
		    AUDIO_DAI_CLOCK_OUT);
		if (error)
			goto failed;
	}

	for (n = 0; n < link->link_naux; n++) {
		error = audio_dai_trigger(link->link_aux[n], start, end,
		    blksize, intr, intrarg, params, AUMODE_RECORD);
		if (error)
			goto failed;
	}
	error = audio_dai_trigger(link->link_codec, start, end, blksize,
	    intr, intrarg, params, AUMODE_RECORD);
	if (error)
		goto failed;

	return audio_dai_trigger(link->link_cpu, start, end, blksize,
	    intr, intrarg, params, AUMODE_RECORD);

failed:
	ausoc_halt_input(priv);
	return error;
}

static void
ausoc_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct ausoc_link * const link = priv;

	return audio_dai_get_locks(link->link_cpu, intr, thread);
}

static const struct audio_hw_if ausoc_hw_if = {
	.open = ausoc_open,
	.close = ausoc_close,
	.drain = ausoc_drain,
	.query_encoding = ausoc_query_encoding,
	.set_params = ausoc_set_params,
	.allocm = ausoc_allocm,
	.freem = ausoc_freem,
	.mappage = ausoc_mappage,
	.getdev = ausoc_getdev,
	.set_port = ausoc_set_port,
	.get_port = ausoc_get_port,
	.query_devinfo = ausoc_query_devinfo,
	.get_props = ausoc_get_props,
	.round_blocksize = ausoc_round_blocksize,
	.round_buffersize = ausoc_round_buffersize,
	.trigger_output = ausoc_trigger_output,
	.trigger_input = ausoc_trigger_input,
	.halt_output = ausoc_halt_output,
	.halt_input = ausoc_halt_input,
	.get_locks = ausoc_get_locks,
};

static int
ausoc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static struct {
	const char *name;
	u_int fmt;
} ausoc_dai_formats[] = {
	{ "i2s",	AUDIO_DAI_FORMAT_I2S },
	{ "right_j",	AUDIO_DAI_FORMAT_RJ },
	{ "left_j",	AUDIO_DAI_FORMAT_LJ },
	{ "dsp_a",	AUDIO_DAI_FORMAT_DSPA },
	{ "dsp_b",	AUDIO_DAI_FORMAT_DSPB },
	{ "ac97",	AUDIO_DAI_FORMAT_AC97 },
	{ "pdm",	AUDIO_DAI_FORMAT_PDM },
};

static int
ausoc_link_format(struct ausoc_softc *sc, struct ausoc_link *link, int phandle,
    int dai_phandle, bool single_link, u_int *format)
{
	const char *format_prop = single_link ?
	    "simple-audio-card,format" : "format";
	const char *frame_master_prop = single_link ?
	    "simple-audio-card,frame-master" : "frame-master";
	const char *bitclock_master_prop = single_link ?
	    "simple-audio-card,bitclock-master" : "bitclock-master";
	const char *bitclock_inversion_prop = single_link ?
	    "simple-audio-card,bitclock-inversion" : "bitclock-inversion";
	const char *frame_inversion_prop = single_link ?
	    "simple-audio-card,frame-inversion" : "frame-inversion";

	u_int fmt, pol, clk;
	const char *s;
	u_int n;

	s = fdtbus_get_string(phandle, format_prop);
	if (s) {
		for (n = 0; n < __arraycount(ausoc_dai_formats); n++) {
			if (strcmp(s, ausoc_dai_formats[n].name) == 0) {
				fmt = ausoc_dai_formats[n].fmt;
				break;
			}
		}
		if (n == __arraycount(ausoc_dai_formats))
			return EINVAL;
	} else {
		fmt = AUDIO_DAI_FORMAT_I2S;
	}

	const bool frame_master =
	    dai_phandle == fdtbus_get_phandle(phandle, frame_master_prop);
	const bool bitclock_master =
	    dai_phandle == fdtbus_get_phandle(phandle, bitclock_master_prop);
	if (frame_master) {
		clk = bitclock_master ?
		    AUDIO_DAI_CLOCK_CBM_CFM : AUDIO_DAI_CLOCK_CBS_CFM;
	} else {
		clk = bitclock_master ?
		    AUDIO_DAI_CLOCK_CBM_CFS : AUDIO_DAI_CLOCK_CBS_CFS;
	}

	const bool bitclock_inversion = of_hasprop(phandle, bitclock_inversion_prop);
	const bool frame_inversion = of_hasprop(phandle, frame_inversion_prop);
	if (bitclock_inversion) {
		pol = frame_inversion ?
		    AUDIO_DAI_POLARITY_IB_IF : AUDIO_DAI_POLARITY_IB_NF;
	} else {
		pol = frame_inversion ?
		    AUDIO_DAI_POLARITY_NB_IF : AUDIO_DAI_POLARITY_NB_NF;
	}

	*format = __SHIFTIN(fmt, AUDIO_DAI_FORMAT_MASK) |
		  __SHIFTIN(pol, AUDIO_DAI_POLARITY_MASK) |
		  __SHIFTIN(clk, AUDIO_DAI_CLOCK_MASK);

	return 0;
}

static void
ausoc_attach_link(struct ausoc_softc *sc, struct ausoc_link *link,
    int card_phandle, int link_phandle)
{
	const bool single_link = card_phandle == link_phandle;
	const char *cpu_prop = single_link ?
	    "simple-audio-card,cpu" : "cpu";
	const char *codec_prop = single_link ?
	    "simple-audio-card,codec" : "codec";
	const char *mclk_fs_prop = single_link ?
	    "simple-audio-card,mclk-fs" : "mclk-fs";
	const char *node_name = fdtbus_get_string(link_phandle, "name");
	u_int n, format;

	const int cpu_phandle = of_find_firstchild_byname(link_phandle, cpu_prop);
	if (cpu_phandle <= 0) {
		aprint_error_dev(sc->sc_dev, "missing %s prop on %s node\n",
		    cpu_prop, node_name);
		return;
	}

	link->link_cpu = fdtbus_dai_acquire(cpu_phandle, "sound-dai");
	if (!link->link_cpu) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't acquire cpu dai on %s node\n", node_name);
		return;
	}

	const int codec_phandle = of_find_firstchild_byname(link_phandle, codec_prop);
	if (codec_phandle <= 0) {
		aprint_error_dev(sc->sc_dev, "missing %s prop on %s node\n",
		    codec_prop, node_name);
		return;
	}

	link->link_codec = fdtbus_dai_acquire(codec_phandle, "sound-dai");
	if (!link->link_codec) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't acquire codec dai on %s node\n", node_name);
		return;
	}

	for (;;) {
		if (fdtbus_dai_acquire_index(card_phandle,
		    "simple-audio-card,aux-devs", link->link_naux) == NULL)
			break;
		link->link_naux++;
	}
	if (link->link_naux) {
		link->link_aux = kmem_zalloc(sizeof(audio_dai_tag_t) * link->link_naux, KM_SLEEP);
		for (n = 0; n < link->link_naux; n++) {
			link->link_aux[n] = fdtbus_dai_acquire_index(card_phandle,
			    "simple-audio-card,aux-devs", n);
			KASSERT(link->link_aux[n] != NULL);

			/* Attach aux devices to codec */
			audio_dai_add_device(link->link_codec, link->link_aux[n]);
		}
	}

	of_getprop_uint32(link_phandle, mclk_fs_prop, &link->link_mclk_fs);
	if (ausoc_link_format(sc, link, link_phandle, codec_phandle, single_link, &format) != 0) {
		aprint_error_dev(sc->sc_dev, "couldn't parse format properties\n");
		return;
	}
	if (audio_dai_set_format(link->link_cpu, format) != 0) {
		aprint_error_dev(sc->sc_dev, "couldn't set cpu format\n");
		return;
	}
	if (audio_dai_set_format(link->link_codec, format) != 0) {
		aprint_error_dev(sc->sc_dev, "couldn't set codec format\n");
		return;
	}

	aprint_normal_dev(sc->sc_dev, "codec: %s, cpu: %s",
	    device_xname(audio_dai_device(link->link_codec)),
	    device_xname(audio_dai_device(link->link_cpu)));
	for (n = 0; n < link->link_naux; n++) {
		if (n == 0)
			aprint_normal(", aux:");
		aprint_normal(" %s",
		    device_xname(audio_dai_device(link->link_aux[n])));
	}
	aprint_normal("\n");

	audio_attach_mi(&ausoc_hw_if, link, sc->sc_dev);
}

static void
ausoc_attach_cb(device_t self)
{
	struct ausoc_softc * const sc = device_private(self);
	const int phandle = sc->sc_phandle;
	const char *name;
	int child, n;
	size_t len;

	/*
	 * If the root node defines a cpu and codec, there is only one link. For
	 * cards with multiple links, there will be simple-audio-card,dai-link
	 * child nodes for each one.
	 */
	if (of_find_firstchild_byname(phandle, "simple-audio-card,cpu") > 0 &&
	    of_find_firstchild_byname(phandle, "simple-audio-card,codec") > 0) {
		sc->sc_nlink = 1;
		sc->sc_link = kmem_zalloc(sizeof(*sc->sc_link), KM_SLEEP);
		sc->sc_link[0].link_name = sc->sc_name;
		ausoc_attach_link(sc, &sc->sc_link[0], phandle, phandle);
	} else {
		for (child = OF_child(phandle); child; child = OF_peer(child)) {
			name = fdtbus_get_string(child, "name");
			len = strlen("simple-audio-card,dai-link");
			if (strncmp(name, "simple-audio-card,dai-link", len) != 0)
				continue;
			sc->sc_nlink++;
		}
		if (sc->sc_nlink == 0)
			return;
		sc->sc_link = kmem_zalloc(sizeof(*sc->sc_link) * sc->sc_nlink,
		    KM_SLEEP);
		for (child = OF_child(phandle), n = 0; child; child = OF_peer(child)) {
			name = fdtbus_get_string(child, "name");
			len = strlen("simple-audio-card,dai-link");
			if (strncmp(name, "simple-audio-card,dai-link", len) != 0)
				continue;
			sc->sc_link[n].link_name = sc->sc_name;
			ausoc_attach_link(sc, &sc->sc_link[n], phandle, child);
			n++;
		}
	}
}

static void
ausoc_attach(device_t parent, device_t self, void *aux)
{
	struct ausoc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_name = fdtbus_get_string(phandle, "simple-audio-card,name");
	if (!sc->sc_name)
		sc->sc_name = "SoC Audio";

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_name);

	/*
	 * Defer attachment until all other drivers are ready.
	 */
	config_defer(self, ausoc_attach_cb);
}

CFATTACH_DECL_NEW(ausoc, sizeof(struct ausoc_softc),
    ausoc_match, ausoc_attach, NULL, NULL);
