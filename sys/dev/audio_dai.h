/* $NetBSD: audio_dai.h,v 1.2.2.2 2018/05/21 04:36:04 pgoyette Exp $ */

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

#ifndef _DEV_AUDIO_DAI_H
#define _DEV_AUDIO_DAI_H

#include <dev/audio_if.h>

#define	AUDIO_DAI_FORMAT_MASK		__BITS(3,0)
#define	AUDIO_DAI_FORMAT_I2S		0
#define	AUDIO_DAI_FORMAT_RJ		1
#define	AUDIO_DAI_FORMAT_LJ		2
#define	AUDIO_DAI_FORMAT_DSPA		3
#define	AUDIO_DAI_FORMAT_DSPB		4
#define	AUDIO_DAI_FORMAT_AC97		5
#define	AUDIO_DAI_FORMAT_PDM		6

#define	AUDIO_DAI_POLARITY_MASK		__BITS(5,4)
#define	AUDIO_DAI_POLARITY_NB_NF	0
#define	AUDIO_DAI_POLARITY_NB_IF	1
#define	AUDIO_DAI_POLARITY_IB_NF	2
#define	AUDIO_DAI_POLARITY_IB_IF	3
#define	AUDIO_DAI_POLARITY_F(n)		(((n) & 0x1) != 0)
#define	AUDIO_DAI_POLARITY_B(n)		(((n) & 0x2) != 0)

#define	AUDIO_DAI_CLOCK_MASK		__BITS(9,8)
#define	AUDIO_DAI_CLOCK_CBM_CFM		0
#define	AUDIO_DAI_CLOCK_CBS_CFM		1
#define	AUDIO_DAI_CLOCK_CBM_CFS		2
#define	AUDIO_DAI_CLOCK_CBS_CFS		3

#define	AUDIO_DAI_CLOCK_IN		0
#define	AUDIO_DAI_CLOCK_OUT		1

#define	AUDIO_DAI_JACK_HP		0
#define	AUDIO_DAI_JACK_MIC		1

typedef struct audio_dai_device {
	int	(*dai_set_sysclk)(struct audio_dai_device *, u_int, int);
	int	(*dai_set_format)(struct audio_dai_device *, u_int);
	int	(*dai_add_device)(struct audio_dai_device *, struct audio_dai_device *);
	int	(*dai_jack_detect)(struct audio_dai_device *, u_int, int);

	const struct audio_hw_if *dai_hw_if;		/* audio driver callbacks */

	device_t 		dai_dev;		/* device */
	void			*dai_priv;		/* driver private data */
} *audio_dai_tag_t;

static inline device_t
audio_dai_device(audio_dai_tag_t dai)
{
	return dai->dai_dev;
}

static inline void *
audio_dai_private(audio_dai_tag_t dai)
{
	return dai->dai_priv;
}

static inline int
audio_dai_set_sysclk(audio_dai_tag_t dai, u_int rate, int dir)
{
	if (!dai->dai_set_sysclk)
		return 0;

	return dai->dai_set_sysclk(dai, rate, dir);
}

static inline int
audio_dai_set_format(audio_dai_tag_t dai, u_int format)
{
	if (!dai->dai_set_format)
		return 0;

	return dai->dai_set_format(dai, format);
}

static inline int
audio_dai_add_device(audio_dai_tag_t dai, audio_dai_tag_t aux)
{
	if (!dai->dai_add_device)
		return 0;

	return dai->dai_add_device(dai, aux);
}

static inline int
audio_dai_jack_detect(audio_dai_tag_t dai, u_int jack, bool present)
{
	if (!dai->dai_jack_detect)
		return 0;

	return dai->dai_jack_detect(dai, jack, present);
}

static inline int
audio_dai_open(audio_dai_tag_t dai, int flags)
{
	if (!dai->dai_hw_if->open)
		return 0;
	return dai->dai_hw_if->open(dai->dai_priv, flags);
}

static inline void
audio_dai_close(audio_dai_tag_t dai)
{
	if (!dai->dai_hw_if->close)
		return;
	dai->dai_hw_if->close(dai->dai_priv);
}

static inline int
audio_dai_drain(audio_dai_tag_t dai)
{
	if (!dai->dai_hw_if->drain)
		return 0;
	return dai->dai_hw_if->drain(dai->dai_priv);
}

static inline int
audio_dai_query_encoding(audio_dai_tag_t dai, audio_encoding_t *ae)
{
	if (!dai->dai_hw_if->query_encoding)
		return 0;
	return dai->dai_hw_if->query_encoding(dai->dai_priv, ae);
}

static inline int
audio_dai_set_params(audio_dai_tag_t dai, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	if (!dai->dai_hw_if->set_params)
		return 0;
	return dai->dai_hw_if->set_params(dai->dai_priv, setmode,
	    usemode, play, rec, pfil, rfil);
}

static inline int
audio_dai_round_blocksize(audio_dai_tag_t dai, int bs, int mode,
    const audio_params_t *params)
{
	if (!dai->dai_hw_if->round_blocksize)
		return bs;
	return dai->dai_hw_if->round_blocksize(dai->dai_priv, bs,
	    mode, params);
}

static inline int
audio_dai_commit_settings(audio_dai_tag_t dai)
{
	if (!dai->dai_hw_if->commit_settings)
		return 0;
	return dai->dai_hw_if->commit_settings(dai->dai_priv);
}

static inline int
audio_dai_halt(audio_dai_tag_t dai, int dir)
{
	switch (dir) {
	case AUMODE_PLAY:
		if (!dai->dai_hw_if->halt_output)
			return 0;
		return dai->dai_hw_if->halt_output(dai->dai_priv);
	case AUMODE_RECORD:
		if (!dai->dai_hw_if->halt_input)
			return 0;
		return dai->dai_hw_if->halt_input(dai->dai_priv);
	default:
		return EINVAL;
	}
}

static inline int
audio_dai_trigger(audio_dai_tag_t dai, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params,
    int dir)
{
	switch (dir) {
	case AUMODE_PLAY:
		if (!dai->dai_hw_if->trigger_output)
			return 0;
		return dai->dai_hw_if->trigger_output(dai->dai_priv, start,
		    end, blksize, intr, intrarg, params);
	case AUMODE_RECORD:
		if (!dai->dai_hw_if->trigger_input)
			return 0;
		return dai->dai_hw_if->trigger_input(dai->dai_priv, start,
		    end, blksize, intr, intrarg, params);
	default:
		return EINVAL;
	}
}

static inline void *
audio_dai_allocm(audio_dai_tag_t dai, int dir, size_t size)
{
	if (!dai->dai_hw_if->allocm)
		return NULL;
	return dai->dai_hw_if->allocm(dai->dai_priv, dir, size);
}

static inline void
audio_dai_freem(audio_dai_tag_t dai, void *addr, size_t size)
{
	if (!dai->dai_hw_if->freem)
		return;
	dai->dai_hw_if->freem(dai->dai_priv, addr, size);
}

static inline size_t
audio_dai_round_buffersize(audio_dai_tag_t dai, int dir, size_t bufsize)
{
	if (!dai->dai_hw_if->round_buffersize)
		return bufsize;
	return dai->dai_hw_if->round_buffersize(dai->dai_priv, dir, bufsize);
}

static inline paddr_t
audio_dai_mappage(audio_dai_tag_t dai, void *addr, off_t off, int prot)
{
	if (!dai->dai_hw_if->mappage)
		return -1;
	return dai->dai_hw_if->mappage(dai->dai_priv, addr, off, prot);
}

static inline int
audio_dai_get_props(audio_dai_tag_t dai)
{
	if (!dai->dai_hw_if->get_props)
		return 0;
	return dai->dai_hw_if->get_props(dai->dai_priv);
}

static inline int
audio_dai_getdev(audio_dai_tag_t dai, struct audio_device *adev)
{
	if (!dai->dai_hw_if->getdev)
		return ENXIO;
	return dai->dai_hw_if->getdev(dai->dai_priv, adev);
}

static inline int
audio_dai_get_port(audio_dai_tag_t dai, mixer_ctrl_t *mc)
{
	if (!dai->dai_hw_if->get_port)
		return ENXIO;
	return dai->dai_hw_if->get_port(dai->dai_priv, mc);
}

static inline int
audio_dai_set_port(audio_dai_tag_t dai, mixer_ctrl_t *mc)
{
	if (!dai->dai_hw_if->set_port)
		return ENXIO;
	return dai->dai_hw_if->set_port(dai->dai_priv, mc);
}

static inline int
audio_dai_query_devinfo(audio_dai_tag_t dai, mixer_devinfo_t *di)
{
	if (!dai->dai_hw_if->query_devinfo)
		return ENXIO;
	return dai->dai_hw_if->query_devinfo(dai->dai_priv, di);
}

static inline void
audio_dai_get_locks(audio_dai_tag_t dai, kmutex_t **intr, kmutex_t **thread)
{
	if (!dai->dai_hw_if->get_locks) {
		*intr = NULL;
		*thread = NULL;
		return;
	}

	dai->dai_hw_if->get_locks(dai->dai_priv, intr, thread);
}

#endif /* _DEV_AUDIO_DAI_H */
