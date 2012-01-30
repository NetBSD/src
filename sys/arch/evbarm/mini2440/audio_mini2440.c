/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>
#include <sys/audioio.h>

#include <sys/bus.h>

#include <dev/audio_if.h>


#include <dev/ic/uda1341var.h>

#include <arch/arm/s3c2xx0/s3c2440reg.h>
#include <arch/arm/s3c2xx0/s3c2440var.h>

#include <arch/arm/s3c2xx0/s3c2440_dma.h>
#include <arch/arm/s3c2xx0/s3c2440_i2s.h>

/*#define AUDIO_MINI2440_DEBUG*/

#ifdef AUDIO_MINI2440_DEBUG
#define DPRINTF(x) do {printf x; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

struct uda_softc {
	device_t		sc_dev;
	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;

	struct uda1341_softc	sc_uda1341;

	s3c2440_i2s_buf_t	sc_play_buf;
	s3c2440_i2s_buf_t	sc_rec_buf;

	void			*sc_i2s_handle;

	bool			sc_open;
};

int	uda_ssio_open(void *, int);
void	uda_ssio_close(void *);
int	uda_ssio_set_params(void *, int, int, audio_params_t *, audio_params_t *,
		       stream_filter_list_t *, stream_filter_list_t *);
int	uda_ssio_round_blocksize(void *, int, int, const audio_params_t *);
int	uda_ssio_start_output(void *, void *, int, void (*)(void *),
			      void *);
int	uda_ssio_start_input(void *, void *, int, void (*)(void *),
			      void *);
int	uda_ssio_halt_output(void *);
int	uda_ssio_halt_input(void *);
int	uda_ssio_getdev(void *, struct audio_device *ret);
void*	uda_ssio_allocm(void *, int, size_t);
void	uda_ssio_freem(void *, void *, size_t);
size_t	uda_ssio_round_buffersize(void *, int, size_t);
int	uda_ssio_getprops(void *);
void	uda_ssio_get_locks(void *, kmutex_t**, kmutex_t**);

struct audio_hw_if uda1341_hw_if = {
	uda_ssio_open,
	uda_ssio_close,
	NULL,
	uda1341_query_encodings,
	uda_ssio_set_params,
	uda_ssio_round_blocksize,
	NULL,				/* commit_settings*/
	NULL,
	NULL,
	uda_ssio_start_output,
	uda_ssio_start_input,
	uda_ssio_halt_output,
	uda_ssio_halt_input,
	NULL,
	uda_ssio_getdev,
	NULL,
	uda1341_set_port,
	uda1341_get_port,
	uda1341_query_devinfo,
	uda_ssio_allocm,
	uda_ssio_freem,
	uda_ssio_round_buffersize,
	NULL,				/* mappage */
	uda_ssio_getprops,
	NULL,
	NULL,
	NULL,
	uda_ssio_get_locks
};

static struct audio_device uda1341_device = {
	"MINI2240-UDA1341",
	"0.1",
	"uda_ssio"
};

void uda_ssio_l3_write(void *,int mode, int value);

int uda_ssio_match(device_t, cfdata_t, void*);
void uda_ssio_attach(device_t, device_t, void*);

CFATTACH_DECL_NEW(udassio, sizeof(struct uda_softc),
	      uda_ssio_match, uda_ssio_attach, NULL, NULL);

int
uda_ssio_match(device_t parent, cfdata_t match, void *aux)
{
	DPRINTF(("%s\n", __func__));
	/* Not quite sure how we can detect the UDA1341 chip */
	return 1;
}

void
uda_ssio_attach(device_t parent, device_t self, void *aux)
{
	/*	struct s3c2xx0_attach_args *sa = aux;*/
	struct uda_softc *sc = device_private(self);
	struct s3c2xx0_softc *s3sc = s3c2xx0_softc; /* Shortcut */
	struct s3c2440_i2s_attach_args *aa = aux;
	uint32_t reg;

	sc->sc_dev = self;

	sc->sc_play_buf = NULL;
	sc->sc_i2s_handle = aa->i2sa_handle;
	sc->sc_open = false;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	s3c2440_i2s_set_intr_lock(aa->i2sa_handle, &sc->sc_intr_lock);

	/* arch/arm/s3c2xx0/s3c2440.c initializes the I2S subsystem for us */

	/* Setup GPIO pins to output for L3 communication.
	   GPB3 (L3DATA) will have to be switched to input when reading
	   from the L3 bus.

	   GPB2 - L3MODE
	   GPB3 - L3DATA
	   GPB4 - L3CLOCK
	   TODO: Make this configurable
	*/
	reg = bus_space_read_4(s3sc->sc_iot, s3sc->sc_gpio_ioh, GPIO_PBCON);
	reg = GPIO_SET_FUNC(reg, 2, 1);
	reg = GPIO_SET_FUNC(reg, 3, 1);
	reg = GPIO_SET_FUNC(reg, 4, 1);
	bus_space_write_4(s3sc->sc_iot, s3sc->sc_gpio_ioh, GPIO_PBCON, reg);

	reg = bus_space_read_4(s3sc->sc_iot, s3sc->sc_gpio_ioh, GPIO_PBDAT);
	reg = GPIO_SET_DATA(reg, 4, 1);
	reg = GPIO_SET_DATA(reg, 3, 0);
	reg = GPIO_SET_DATA(reg, 2, 1);
	bus_space_write_4(s3sc->sc_iot, s3sc->sc_gpio_ioh, GPIO_PBDAT, reg);

	printf("\n");

	/* uda1341_attach resets the uda1341 sc, so it has to be called before
	   attributes are set on the sc.*/
	uda1341_attach(&sc->sc_uda1341);

	/* Configure the UDA1341 Codec */
	sc->sc_uda1341.parent = sc;
	sc->sc_uda1341.sc_l3_write = uda_ssio_l3_write;
	sc->sc_uda1341.sc_bus_format = UDA1341_BUS_MSB;

	/* Configure I2S controller */
	s3c2440_i2s_set_bus_format(sc->sc_i2s_handle, S3C2440_I2S_BUS_MSB);
	// Attach
	audio_attach_mi(&uda1341_hw_if, &sc->sc_uda1341, self);
}

int
uda_ssio_open(void *handle, int flags)
{
	struct uda1341_softc *uc = handle;
	struct uda_softc *sc = uc->parent;
	int retval;

	DPRINTF(("%s\n", __func__));

	if (sc->sc_open)
		return EBUSY;

	/* We only support write operations */
	if (!(flags & FREAD) && !(flags & FWRITE))
		return EINVAL;

	/* We can't do much more at this point than to
	   ask the UDA1341 codec to initialize itself
	   (for an unknown system clock)
	*/
	retval = uda1341_open(handle, flags);
	if (retval != 0) {
		return retval;
	}

	sc->sc_open = true;

	return 0; /* SUCCESS */
}

void
uda_ssio_close(void *handle)
{
	struct uda1341_softc *uc = handle;
	struct uda_softc *sc = uc->parent;
	DPRINTF(("%s\n", __func__));

	uda1341_close(handle);
	sc->sc_open = false;
}

int
uda_ssio_set_params(void *handle, int setmode, int usemode,
		    audio_params_t *play, audio_params_t *rec,
		    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct uda1341_softc *uc = handle;
	struct uda_softc *sc = uc->parent;
	const struct audio_format *selected_format;
	audio_params_t *params;
	stream_filter_list_t *fil;
	int retval;

	DPRINTF(("%s: setmode: %d\n", __func__, setmode));
	DPRINTF(("%s: usemode: %d\n", __func__, usemode));

	if (setmode == 0)
		setmode = usemode;

	if (setmode & AUMODE_PLAY) {
		params = play;
		fil = pfil;
	} else if (setmode == AUMODE_RECORD) {
		params = rec;
		fil = rfil;
	} else {
		return EINVAL;
	}

	DPRINTF(("%s: %dHz, encoding: %d, precision: %d, channels: %d\n",
		 __func__, params->sample_rate, params->encoding, play->precision,
		 params->channels));

	if (params->sample_rate != 8000 &&
	    params->sample_rate != 11025 &&
	    params->sample_rate != 22050 &&
	    params->sample_rate != 32000 &&
	    params->sample_rate != 44100 &&
	    params->sample_rate != 48000) {
		return EINVAL;
	}

	retval = auconv_set_converter(uda1341_formats, UDA1341_NFORMATS,
				    setmode, params, true, fil);
	if (retval < 0) {
		printf("Could not find valid format\n");
		return EINVAL;
	}

	selected_format = &uda1341_formats[retval];

	if (setmode == AUMODE_PLAY) {
		s3c2440_i2s_set_direction(sc->sc_i2s_handle,
					  S3C2440_I2S_TRANSMIT);
	} else {
		s3c2440_i2s_set_direction(sc->sc_i2s_handle,
					  S3C2440_I2S_RECEIVE);
	}

	s3c2440_i2s_set_sample_rate(sc->sc_i2s_handle, params->sample_rate);
	s3c2440_i2s_set_sample_width(sc->sc_i2s_handle,
				     selected_format->precision);

	/* It is vital that sc_system_clock is set PRIOR to calling
	   uda1341_set_params. */
	switch (s3c2440_i2s_get_master_clock(sc->sc_i2s_handle)) {
	case 384:
		uc->sc_system_clock = UDA1341_CLOCK_384;
		break;
	case 256:
		uc->sc_system_clock = UDA1341_CLOCK_256;
		break;
	default:
		return EINVAL;
	}

	retval = uda1341_set_params(handle, setmode, usemode,
				    play, rec, pfil, rfil);
	if (retval != 0) {
		return retval;
	}

	/* Setup and enable I2S controller */
	retval = s3c2440_i2s_commit(sc->sc_i2s_handle);
	if (retval != 0) {
		printf("Failed to setup I2S controller\n");
		return retval;
	}

	return 0;
}

int
uda_ssio_round_blocksize(void *handle, int bs, int mode,
			 const audio_params_t *param)
{
	int out_bs;
	DPRINTF(("%s: %d\n", __func__, bs));

	out_bs =  (bs + 0x03) & ~0x03;
	DPRINTF(("%s: out_bs: %d\n", __func__, out_bs));
	return out_bs;
}

int
uda_ssio_start_output(void *handle, void *block, int bsize,
		      void (*intr)(void *), void *intrarg)
{
	struct uda1341_softc *uc = handle;
	struct uda_softc *sc = uc->parent;

	return s3c2440_i2s_output(sc->sc_play_buf, block, bsize, intr, intrarg);
}

int
uda_ssio_start_input(void *handle, void *block, int bsize,
		     void (*intr)(void *), void *intrarg)
{
	struct uda1341_softc *uc = handle;
	struct uda_softc *sc = uc->parent;

	return s3c2440_i2s_input(sc->sc_rec_buf, block, bsize, intr, intrarg);
}

int
uda_ssio_halt_output(void *handle)
{
	struct uda1341_softc *uc = handle;
	struct uda_softc *sc = uc->parent;

	return s3c2440_i2s_halt_output(sc->sc_play_buf);
}

int
uda_ssio_halt_input(void *handle)
{
	DPRINTF(("%s\n", __func__));
	return 0;
}

int
uda_ssio_getdev(void *handle, struct audio_device *ret)
{
	*ret = uda1341_device;
	return 0;
}

void *
uda_ssio_allocm(void *handle, int direction, size_t size)
{
	struct uda1341_softc *uc = handle;
	struct uda_softc *sc = uc->parent;
	void *retval = NULL;

	DPRINTF(("%s\n", __func__));

	if (direction == AUMODE_PLAY ) {
		if (sc->sc_play_buf != NULL)
			return NULL;

		s3c2440_i2s_alloc(sc->sc_i2s_handle, direction, size, 0x00, &sc->sc_play_buf);
		DPRINTF(("%s: addr of ring buffer: %p\n", __func__, sc->sc_play_buf->i2b_addr));
		retval = sc->sc_play_buf->i2b_addr;
	} else if (direction == AUMODE_RECORD) {
		if (sc->sc_rec_buf != NULL)
			return NULL;

		s3c2440_i2s_alloc(sc->sc_i2s_handle, direction, size, 0x00, &sc->sc_rec_buf);
		DPRINTF(("%s: addr of ring buffer: %p\n", __func__, sc->sc_rec_buf->i2b_addr));
		retval = sc->sc_rec_buf->i2b_addr;
	}

	DPRINTF(("buffer: %p", retval));

	return retval;
}

void
uda_ssio_freem(void *handle, void *ptr, size_t size)
{
	struct uda1341_softc *uc = handle;
	struct uda_softc *sc = uc->parent;
	DPRINTF(("%s\n", __func__));

	if (ptr == sc->sc_play_buf->i2b_addr)
		s3c2440_i2s_free(sc->sc_play_buf);
	else if (ptr == sc->sc_rec_buf->i2b_addr)
		s3c2440_i2s_free(sc->sc_rec_buf);
}

size_t
uda_ssio_round_buffersize(void *handle, int direction, size_t bufsize)
{
	DPRINTF(("%s: %d\n", __func__, (int)bufsize));
	return bufsize;
}

int
uda_ssio_getprops(void *handle)
{
	return AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE;
}

void
uda_ssio_get_locks(void *handle, kmutex_t **intr, kmutex_t **thread)
{
	struct uda1341_softc *uc = handle;
	struct uda_softc *sc = uc->parent;
	//struct uda_softc *sc = handle;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

void
uda_ssio_l3_write(void *cookie, int mode, int value)
{
	struct s3c2xx0_softc *s3sc = s3c2xx0_softc; /* Shortcut */
	uint32_t reg;

	/* GPB2: L3MODE
	   GPB3: L2DATA
	   GPB4: L3CLOCK */
#define L3MODE	2
#define L3DATA	3
#define L3CLOCK 4
#define READ_GPIO() bus_space_read_4(s3sc->sc_iot, s3sc->sc_gpio_ioh, GPIO_PBDAT)
#define WRITE_GPIO(val) bus_space_write_4(s3sc->sc_iot, s3sc->sc_gpio_ioh, GPIO_PBDAT, val)

#define DELAY_TIME 1

	reg = READ_GPIO();
	reg = GPIO_SET_DATA(reg, L3CLOCK, 1);
	reg = GPIO_SET_DATA(reg, L3MODE, mode);
	reg = GPIO_SET_DATA(reg, L3DATA, 0);
	WRITE_GPIO(reg);

	if (mode == 1 ) {
		reg = READ_GPIO();
		reg = GPIO_SET_DATA(reg, L3MODE, 1);
		WRITE_GPIO(reg);
	}

	DELAY(1); /* L3MODE setup time: min 190ns */

	for(int i = 0; i<8; i++) {
		char bval = (value >> i) & 0x1;

		reg = READ_GPIO();
		reg = GPIO_SET_DATA(reg, L3CLOCK, 0);
		reg = GPIO_SET_DATA(reg, L3DATA, bval);
		WRITE_GPIO(reg);

		DELAY(DELAY_TIME);

		reg = READ_GPIO();
		reg = GPIO_SET_DATA(reg, L3CLOCK, 1);
		reg = GPIO_SET_DATA(reg, L3DATA, bval);
		WRITE_GPIO(reg);

		DELAY(DELAY_TIME);
	}

	reg = READ_GPIO();
	reg = GPIO_SET_DATA(reg, L3MODE, 1);
	reg = GPIO_SET_DATA(reg, L3CLOCK, 1);
	reg = GPIO_SET_DATA(reg, L3DATA, 0);
	WRITE_GPIO(reg);

#undef L3MODE
#undef L3DATA
#undef L3CLOCK
#undef DELAY_TIME
#undef READ_GPIO
#undef WRITE_GPIO
}
