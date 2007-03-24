/*	$NetBSD: aica.c,v 1.12.14.2 2007/03/24 14:54:36 yamt Exp $	*/

/*
 * Copyright (c) 2003 SHIMIZU Ryo <ryo@misakimix.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aica.c,v 1.12.14.2 2007/03/24 14:54:36 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/audioio.h>

#include <dev/audio_if.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>

#include <machine/bus.h>
#include <machine/sysasicvar.h>

#include <dreamcast/dev/g2/g2busvar.h>
#include <dreamcast/dev/g2/aicavar.h>
#include <dreamcast/dev/microcode/aica_armcode.h>

#define	AICA_REG_ADDR	0x00700000
#define	AICA_RAM_START	0x00800000
#define	AICA_RAM_SIZE	0x00200000
#define	AICA_NCHAN	64
#define	AICA_TIMEOUT	0x1800

struct aica_softc {
	struct device		sc_dev;		/* base device */
	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_aica_regh;
	bus_space_handle_t	sc_aica_memh;

	/* audio property */
	int			sc_open;
	int			sc_encodings;
	int			sc_precision;
	int			sc_channels;
	int			sc_rate;
	void			(*sc_intr)(void *);
	void			*sc_intr_arg;

	int			sc_output_master;
	int			sc_output_gain[2];
#define	AICA_VOLUME_LEFT	0
#define	AICA_VOLUME_RIGHT	1

	/* work for output */
	void			*sc_buffer;
	void			*sc_buffer_start;
	void			*sc_buffer_end;
	int			sc_blksize;
	int			sc_nextfill;
};

const struct {
	const char *name;
	int	encoding;
	int	precision;
} aica_encodings[] = {
	{AudioEadpcm,		AUDIO_ENCODING_ADPCM,		4},
	{AudioEslinear,		AUDIO_ENCODING_SLINEAR,		8},
	{AudioEulinear,		AUDIO_ENCODING_ULINEAR,		8},
	{AudioEmulaw,		AUDIO_ENCODING_ULAW,		8},
	{AudioEalaw,		AUDIO_ENCODING_ALAW,		8},
	{AudioEslinear_be,	AUDIO_ENCODING_SLINEAR_BE,	16},
	{AudioEslinear_le,	AUDIO_ENCODING_SLINEAR_LE,	16},
	{AudioEulinear_be,	AUDIO_ENCODING_ULINEAR_BE,	16},
	{AudioEulinear_le,	AUDIO_ENCODING_ULINEAR_LE,	16},
};

#define AICA_NFORMATS	5
static const struct audio_format aica_formats[AICA_NFORMATS] = {
	{NULL, AUMODE_PLAY, AUDIO_ENCODING_ADPCM, 4, 4,
	 1, AUFMT_MONAURAL, 0, {1, 65536}},
	{NULL, AUMODE_PLAY, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 1, AUFMT_MONAURAL, 0, {1, 65536}},
	{NULL, AUMODE_PLAY, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 2, AUFMT_STEREO, 0, {1, 65536}},
	{NULL, AUMODE_PLAY, AUDIO_ENCODING_SLINEAR_LE, 8, 8,
	 1, AUFMT_MONAURAL, 0, {1, 65536}},
	{NULL, AUMODE_PLAY, AUDIO_ENCODING_SLINEAR_LE, 8, 8,
	 2, AUFMT_STEREO, 0, {1, 65536}},
};

int aica_match(struct device *, struct cfdata *, void *);
void aica_attach(struct device *, struct device *, void *);
int aica_print(void *, const char *);

CFATTACH_DECL(aica, sizeof(struct aica_softc), aica_match, aica_attach,
    NULL, NULL);

const struct audio_device aica_device = {
	"Dreamcast Sound",
	"",
	"aica"
};

inline static void aica_g2fifo_wait(void);
void aica_enable(struct aica_softc *);
void aica_disable(struct aica_softc *);
void aica_memwrite(struct aica_softc *, bus_size_t, uint32_t *, int);
void aica_ch2p16write(struct aica_softc *, bus_size_t, uint16_t *, int);
void aica_ch2p8write(struct aica_softc *, bus_size_t, uint8_t *, int);
void aica_command(struct aica_softc *, uint32_t);
void aica_sendparam(struct aica_softc *, uint32_t, int, int);
void aica_play(struct aica_softc *, int, int, int, int);
void aica_fillbuffer(struct aica_softc *);

/* intr */
int aica_intr(void *);

/* for audio */
int aica_open(void *, int);
void aica_close(void *);
int aica_query_encoding(void *, struct audio_encoding *);
int aica_set_params(void *, int, int, audio_params_t *,
    audio_params_t *, stream_filter_list_t *, stream_filter_list_t *);
int aica_round_blocksize(void *, int, int, const audio_params_t *);
size_t aica_round_buffersize(void *, int, size_t);
int aica_trigger_output(void *, void *, void *, int, void (*)(void *), void *,
    const audio_params_t *);
int aica_trigger_input(void *, void *, void *, int, void (*)(void *), void *,
    const audio_params_t *);
int aica_halt_output(void *);
int aica_halt_input(void *);
int aica_getdev(void *, struct audio_device *);
int aica_set_port(void *, mixer_ctrl_t *);
int aica_get_port(void *, mixer_ctrl_t *);
int aica_query_devinfo(void *, mixer_devinfo_t *);
void aica_encode(int, int, int, int, u_char *, u_short **);
int aica_get_props(void *);

const struct audio_hw_if aica_hw_if = {
	aica_open,
	aica_close,
	NULL,				/* aica_drain */
	aica_query_encoding,
	aica_set_params,
	aica_round_blocksize,
	NULL,				/* aica_commit_setting */
	NULL,				/* aica_init_output */
	NULL,				/* aica_init_input */
	NULL,				/* aica_start_output */
	NULL,				/* aica_start_input */
	aica_halt_output,
	aica_halt_input,
	NULL,				/* aica_speaker_ctl */
	aica_getdev,
	NULL,				/* aica_setfd */
	aica_set_port,
	aica_get_port,
	aica_query_devinfo,
	NULL,				/* aica_allocm */
	NULL,				/* aica_freem */

	aica_round_buffersize,		/* aica_round_buffersize */

	NULL,				/* aica_mappage */
	aica_get_props,
	aica_trigger_output,
	aica_trigger_input,
	NULL,				/* aica_dev_ioctl */
};

int
aica_match(struct device *parent, struct cfdata *cf, void *aux)
{
	static int aica_matched = 0;

	if (aica_matched)
		return 0;

	aica_matched = 1;
	return 1;
}

void
aica_attach(struct device *parent, struct device *self, void *aux)
{
	struct aica_softc *sc;
	struct g2bus_attach_args *ga;
	int i;

	sc = (struct aica_softc *)self;
	ga = aux;
	sc->sc_memt = ga->ga_memt;

	if (bus_space_map(sc->sc_memt, AICA_REG_ADDR, 0x3000, 0,
	    &sc->sc_aica_regh) != 0) {
		printf(": can't map AICA register space\n");
		return;
	}

	if (bus_space_map(sc->sc_memt, AICA_RAM_START, AICA_RAM_SIZE, 0,
	    &sc->sc_aica_memh) != 0) {
		printf(": can't map AICA memory space\n");
		return;
	}

	printf(": ARM7 Sound Processing Unit\n");

	aica_disable(sc);

	for (i = 0; i < AICA_NCHAN; i++)
		bus_space_write_4(sc->sc_memt,sc->sc_aica_regh, i << 7,
		    ((bus_space_read_4(sc->sc_memt, sc->sc_aica_regh, i << 7)
		    & ~0x4000) | 0x8000));

	/* load microcode, and clear memory */
	bus_space_set_region_4(sc->sc_memt, sc->sc_aica_memh,
	    0, 0, AICA_RAM_SIZE);

	aica_memwrite(sc, 0, aica_armcode, sizeof(aica_armcode));

	aica_enable(sc);

	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname,
	    sysasic_intr_string(IPL_BIO));
	sysasic_intr_establish(SYSASIC_EVENT_AICA, IPL_BIO, SYSASIC_IRL9,
	    aica_intr, sc);

	audio_attach_mi(&aica_hw_if, sc, &sc->sc_dev);

	/* init parameters */
	sc->sc_output_master = 255;
	sc->sc_output_gain[AICA_VOLUME_LEFT]  = 255;
	sc->sc_output_gain[AICA_VOLUME_RIGHT] = 255;
}

void
aica_enable(struct aica_softc *sc)
{

	bus_space_write_4(sc->sc_memt, sc->sc_aica_regh, 0x28a8, 24);
	bus_space_write_4(sc->sc_memt, sc->sc_aica_regh, 0x2c00,
	    bus_space_read_4(sc->sc_memt, sc->sc_aica_regh, 0x2c00) & ~1);
}

void
aica_disable(struct aica_softc *sc)
{

	bus_space_write_4(sc->sc_memt, sc->sc_aica_regh, 0x2c00,
	    bus_space_read_4(sc->sc_memt, sc->sc_aica_regh, 0x2c00) | 1);
}

inline static void
aica_g2fifo_wait(void)
{
	int i;

	i = AICA_TIMEOUT;
	while (--i > 0)
		if ((*(volatile uint32_t *)0xa05f688c) & 0x11)
			break;
}

void
aica_memwrite(struct aica_softc *sc, bus_size_t offset, uint32_t *src, int len)
{
	int n;

	KASSERT((offset & 3) == 0);
	n = (len + 3) / 4;	/* uint32_t * n (aligned) */

	aica_g2fifo_wait();
	bus_space_write_region_4(sc->sc_memt, sc->sc_aica_memh,
	    offset, src, n);
}

void
aica_ch2p16write(struct aica_softc *sc, bus_size_t offset, uint16_t *src,
    int len)
{
	union {
		uint32_t w[8];
		uint16_t s[16];
	} buf;
	uint16_t *p;
	int i;

	KASSERT((offset & 3) == 0);

	while (len >= 32) {
		p = buf.s;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;

		aica_g2fifo_wait();
		bus_space_write_region_4(sc->sc_memt, sc->sc_aica_memh,
		    offset, buf.w , 32 / 4);

		offset += sizeof(uint16_t) * 16;
		len -= 32;
	}

	if (len / 2 > 0) {
		p = buf.s;
		for (i = 0; i < len / 2; i++) {
			*p++ = *src++; src++;
		}

		aica_g2fifo_wait();
		bus_space_write_region_4(sc->sc_memt, sc->sc_aica_memh,
		    offset, buf.w, len / 4);
	}
}

void
aica_ch2p8write(struct aica_softc *sc, bus_size_t offset, uint8_t *src,
    int len)
{
	uint32_t buf[8];
	uint8_t *p;
	int i;

	KASSERT((offset & 3) == 0);
	while (len >= 32) {
		p = (uint8_t *)buf;

		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;
		*p++ = *src++; src++;

		aica_g2fifo_wait();
		bus_space_write_region_4(sc->sc_memt, sc->sc_aica_memh,
		    offset, buf, 32 / 4);

		offset += 32;
		len -= 32;
	}

	if (len) {
		p = (uint8_t *)buf;
		for (i = 0; i < len; i++)
			*p++ = *src++; src++;

		aica_g2fifo_wait();
		bus_space_write_region_4(sc->sc_memt, sc->sc_aica_memh,
		    offset, buf, len / 4);
	}
}

int
aica_open(void *addr, int flags)
{
	struct aica_softc *sc;

	sc = addr;
	if (sc->sc_open)
		return EBUSY;

	sc->sc_intr = NULL;
	sc->sc_open = 1;

	return 0;
}

void
aica_close(void *addr)
{
	struct aica_softc *sc;

	sc = addr;
	sc->sc_open = 0;
	sc->sc_intr = NULL;
}

int
aica_query_encoding(void *addr, struct audio_encoding *fp)
{
	if (fp->index >= sizeof(aica_encodings) / sizeof(aica_encodings[0]))
		return EINVAL;

	strcpy(fp->name, aica_encodings[fp->index].name);
	fp->encoding = aica_encodings[fp->index].encoding;
	fp->precision = aica_encodings[fp->index].precision;
	fp->flags = 0;

	return 0;
}

int
aica_set_params(void *addr, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct aica_softc *sc;
	const audio_params_t *hw;
	int i;

	i = auconv_set_converter(aica_formats, AICA_NFORMATS,
				 AUMODE_PLAY, play, false, pfil);
	if (i < 0)
		return EINVAL;
	hw = pfil->req_size > 0 ? &pfil->filters[0].param : play;
	sc = addr;
	sc->sc_precision = hw->precision;
	sc->sc_channels = hw->channels;
	sc->sc_rate = hw->sample_rate;
	sc->sc_encodings = hw->encoding;
	return 0;
}

int
aica_round_blocksize(void *addr, int blk, int mode, const audio_params_t *param)
{
	struct aica_softc *sc;

	sc = addr;
	switch (sc->sc_precision) {
	case 4:
		if (sc->sc_channels == 1)
			return AICA_DMABUF_SIZE / 4;
		else
			return AICA_DMABUF_SIZE / 2;
		break;
	case 8:
		if (sc->sc_channels == 1)
			return AICA_DMABUF_SIZE / 2;
		else
			return AICA_DMABUF_SIZE;
		break;
	case 16:
		if (sc->sc_channels == 1)
			return AICA_DMABUF_SIZE;
		else
			return AICA_DMABUF_SIZE * 2;
		break;
	default:
		break;
	}

	return AICA_DMABUF_SIZE / 4;
}

size_t
aica_round_buffersize(void *addr, int dir, size_t bufsize)
{

	if (dir == AUMODE_PLAY)
		return 65536;

	return 512;	/* XXX: AUMINBUF */
}

void
aica_command(struct aica_softc *sc, uint32_t command)
{

	bus_space_write_4(sc->sc_memt, sc->sc_aica_memh,
	    AICA_ARM_CMD_COMMAND, command);
	bus_space_write_4(sc->sc_memt,sc->sc_aica_memh, AICA_ARM_CMD_SERIAL,
	    bus_space_read_4(sc->sc_memt, sc->sc_aica_memh,
	    AICA_ARM_CMD_SERIAL) + 1);
}

void
aica_sendparam(struct aica_softc *sc, uint32_t command,
    int32_t lparam, int32_t rparam)
{

	bus_space_write_4(sc->sc_memt,sc->sc_aica_memh,
	    AICA_ARM_CMD_LPARAM, lparam);
	bus_space_write_4(sc->sc_memt,sc->sc_aica_memh,
	    AICA_ARM_CMD_RPARAM, rparam);

	aica_command(sc, command);
}

void
aica_play(struct aica_softc *sc, int blksize, int channel, int rate, int prec)
{

	bus_space_write_4(sc->sc_memt,sc->sc_aica_memh,
	    AICA_ARM_CMD_BLOCKSIZE, blksize);
	bus_space_write_4(sc->sc_memt,sc->sc_aica_memh,
	    AICA_ARM_CMD_CHANNEL, channel);
	bus_space_write_4(sc->sc_memt,sc->sc_aica_memh,
	    AICA_ARM_CMD_RATE, rate);
	bus_space_write_4(sc->sc_memt,sc->sc_aica_memh,
	    AICA_ARM_CMD_PRECISION, prec);

	aica_command(sc, AICA_COMMAND_PLAY);
}

void
aica_fillbuffer(struct aica_softc *sc)
{

	if (sc->sc_channels == 2) {
		if (sc->sc_precision == 16) {
			aica_ch2p16write(sc,
			    AICA_DMABUF_LEFT + sc->sc_nextfill,
			    (uint16_t *)sc->sc_buffer + 0, sc->sc_blksize / 2);
			aica_ch2p16write(sc,
			    AICA_DMABUF_RIGHT + sc->sc_nextfill,
			    (uint16_t *)sc->sc_buffer + 1, sc->sc_blksize / 2);
		} else if (sc->sc_precision == 8) {
			aica_ch2p8write(sc, AICA_DMABUF_LEFT + sc->sc_nextfill,
			    (uint8_t *)sc->sc_buffer + 0, sc->sc_blksize / 2);
			aica_ch2p8write(sc, AICA_DMABUF_RIGHT + sc->sc_nextfill,
			    (uint8_t *)sc->sc_buffer + 1, sc->sc_blksize / 2);
		}
	} else {
		aica_memwrite(sc, AICA_DMABUF_MONO + sc->sc_nextfill,
		    sc->sc_buffer, sc->sc_blksize);
	}

	sc->sc_buffer = (int8_t *)sc->sc_buffer + sc->sc_blksize;
	if (sc->sc_buffer >= sc->sc_buffer_end)
		sc->sc_buffer = sc->sc_buffer_start;

	sc->sc_nextfill ^= sc->sc_blksize / sc->sc_channels;
}

int
aica_intr(void *arg)
{
	struct aica_softc *sc;

	sc = arg;
	aica_fillbuffer(sc);

	/* call audio interrupt handler (audio_pint()) */
	if (sc->sc_open && sc->sc_intr != NULL) {
		(*(sc->sc_intr))(sc->sc_intr_arg);
	}

	/* clear SPU interrupt */
	bus_space_write_4(sc->sc_memt, sc->sc_aica_regh, 0x28bc, 0x20);
	return 1;
}

int
aica_trigger_output(void *addr, void *start, void *end, int blksize,
    void (*intr)(void *), void *arg, const audio_params_t *param)
{
	struct aica_softc *sc;

	sc = addr;
	aica_command(sc, AICA_COMMAND_INIT);
	tsleep(aica_trigger_output, PWAIT, "aicawait", hz / 20);

	sc->sc_buffer_start = sc->sc_buffer = start;
	sc->sc_buffer_end = end;
	sc->sc_blksize = blksize;
	sc->sc_nextfill = 0;

	sc->sc_intr = intr;
	sc->sc_intr_arg = arg;

	/* fill buffers in advance */
	aica_intr(sc);
	aica_intr(sc);

	/* ...and start playing */
	aica_play(sc, blksize / sc->sc_channels, sc->sc_channels, sc->sc_rate,
	    sc->sc_precision);

	return 0;
}

int
aica_trigger_input(void *addr, void *start, void *end, int blksize,
    void (*intr)(void *), void *arg, const audio_params_t *param)
{

	return ENODEV;
}

int
aica_halt_output(void *addr)
{
	struct aica_softc *sc;

	sc = addr;
	aica_command(sc, AICA_COMMAND_STOP);
	return 0;
}

int
aica_halt_input(void *addr)
{

	return ENODEV;
}

int
aica_getdev(void *addr, struct audio_device *ret)
{

	*ret = aica_device;
	return 0;
}

int
aica_set_port(void *addr, mixer_ctrl_t *mc)
{
	struct aica_softc *sc;

	sc = addr;
	switch (mc->dev) {
	case AICA_MASTER_VOL:
		sc->sc_output_master =
		    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] & 0xff;
		aica_sendparam(sc, AICA_COMMAND_MVOL,
		    sc->sc_output_master, sc->sc_output_master);
		break;
	case AICA_OUTPUT_GAIN:
		sc->sc_output_gain[AICA_VOLUME_LEFT] =
		    mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT]  & 0xff;
		sc->sc_output_gain[AICA_VOLUME_RIGHT] =
		    mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] & 0xff;
		aica_sendparam(sc, AICA_COMMAND_VOL,
		    sc->sc_output_gain[AICA_VOLUME_LEFT],
		    sc->sc_output_gain[AICA_VOLUME_RIGHT]);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

int
aica_get_port(void *addr, mixer_ctrl_t *mc)
{
	struct aica_softc *sc;

	sc = addr;
	switch (mc->dev) {
	case AICA_MASTER_VOL:
		if (mc->un.value.num_channels != 1)
			return EINVAL;
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
		    L16TO256(L256TO16(sc->sc_output_master));
		break;
	case AICA_OUTPUT_GAIN:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
		    sc->sc_output_gain[AICA_VOLUME_LEFT];
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
		    sc->sc_output_gain[AICA_VOLUME_RIGHT];
		break;
	default:
		return EINVAL;
	}
	return 0;
}

int
aica_query_devinfo(void *addr, mixer_devinfo_t *md)
{

	switch (md->index) {
	case AICA_MASTER_VOL:
		md->type = AUDIO_MIXER_VALUE;
		md->mixer_class = AICA_OUTPUT_CLASS;
		md->prev = md->next = AUDIO_MIXER_LAST;
		strcpy(md->label.name, AudioNmaster);
		md->un.v.num_channels = 1;
		strcpy(md->un.v.units.name, AudioNvolume);
		return 0;
	case AICA_OUTPUT_GAIN:
		md->type = AUDIO_MIXER_VALUE;
		md->mixer_class = AICA_OUTPUT_CLASS;
		md->prev = md->next = AUDIO_MIXER_LAST;
		strcpy(md->label.name, AudioNoutput);
		md->un.v.num_channels = 2;
		strcpy(md->label.name, AudioNvolume);
		return 0;
	case AICA_OUTPUT_CLASS:
		md->type = AUDIO_MIXER_CLASS;
		md->mixer_class = AICA_OUTPUT_CLASS;
		md->next = md->prev = AUDIO_MIXER_LAST;
		strcpy(md->label.name, AudioCoutputs);
		return 0;
	}

	return ENXIO;
}

int
aica_get_props(void *addr)
{

	return 0;
}
