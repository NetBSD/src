/* $NetBSD: ascaudio.c,v 1.2 2024/10/13 12:34:56 nat Exp $ */

/*-
 * Copyright (c) 2017, 2023 Nathanial Sloss <nathanialsloss@yahoo.com.au>
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

/* Based on pad(4) and asc(4) */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ascaudio.c,v 1.2 2024/10/13 12:34:56 nat Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/audioio.h>
#include <sys/module.h>
#include <sys/atomic.h>

#include <uvm/uvm_extern.h>

#include <dev/audio/audio_if.h>
#include <dev/audio/audiovar.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/viareg.h>

#include <mac68k/obio/ascaudiovar.h>
#include <mac68k/obio/ascreg.h>
#include <mac68k/obio/obiovar.h>

#define	MAC68K_ASCAUDIO_BASE		0x50f14000
#define	MAC68K_IIFX_ASCAUDIO_BASE	0x50f10000
#define	MAC68K_ASCAUDIO_LEN		0x1000

#define BUFSIZE 			32768
#define PLAYBLKSIZE			8192
#define RECBLKSIZE			8192

static int	ascaudiomatch(device_t, cfdata_t, void *);
static void	ascaudioattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ascaudio, sizeof(struct ascaudio_softc),
    ascaudiomatch, ascaudioattach, NULL, NULL);

extern struct cfdriver ascaudio_cd;

dev_type_open(ascaudioopen);
dev_type_close(ascaudioclose);
dev_type_read(ascaudioread);
dev_type_write(ascaudiowrite);
dev_type_ioctl(ascaudioioctl);

const struct cdevsw ascaudio_cdevsw = {
	.d_open = ascaudioopen,
	.d_close = ascaudioclose,
	.d_read = ascaudioread,
	.d_write = ascaudiowrite,
	.d_ioctl = ascaudioioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static int	ascaudio_query_format(void *, struct audio_format_query *);
static int	ascaudio_set_format(void *, int,
		    const audio_params_t *, const audio_params_t *,
		    audio_filter_reg_t *, audio_filter_reg_t *);
static int	ascaudio_start_output(void *, void *, int,
				    void (*)(void *), void *);
static int	ascaudio_start_input(void *, void *, int,
				   void (*)(void *), void *);
static int	ascaudio_halt(void *);
static int	ascaudio_set_port(void *, mixer_ctrl_t *);
static int	ascaudio_get_port(void *, mixer_ctrl_t *);
static int	ascaudio_getdev(void *, struct audio_device *);
static int	ascaudio_query_devinfo(void *, mixer_devinfo_t *);
static int	ascaudio_get_props(void *);
static int
	    ascaudio_round_blocksize(void *, int, int, const audio_params_t *);
static void	ascaudio_get_locks(void *, kmutex_t **, kmutex_t **);
static void 	ascaudio_intr(void *);
static int 	ascaudio_intr_est(void *);
static void	ascaudio_intr_enable(void);
static void	ascaudio_done_output(void *);
static void	ascaudio_done_input(void *);

static const struct audio_hw_if ascaudio_hw_if = {
	.query_format	 = ascaudio_query_format,
	.set_format	 = ascaudio_set_format,
	.start_output	 = ascaudio_start_output,
	.start_input	 = ascaudio_start_input,
	.halt_output	 = ascaudio_halt,
	.halt_input	 = ascaudio_halt,
	.set_port	 = ascaudio_set_port,
	.get_port	 = ascaudio_get_port,
	.getdev		 = ascaudio_getdev,
	.query_devinfo	 = ascaudio_query_devinfo,
	.get_props 	 = ascaudio_get_props,
	.round_blocksize = ascaudio_round_blocksize,
	.get_locks	 = ascaudio_get_locks,
};

#define EASC_VER	0xb0
#define EASC_VER2	0xbb

enum {
	ASC_OUTPUT_CLASS,
	ASC_INPUT_CLASS,
	ASC_OUTPUT_MASTER_VOLUME,
	ASC_INPUT_DAC_VOLUME,
	ASC_ENUM_LAST,
};

static int
ascaudiomatch(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oa = (struct obio_attach_args *)aux;
	bus_addr_t addr;
	bus_space_handle_t bsh;
	int rval = 0;

	if (oa->oa_addr != (-1))
		addr = (bus_addr_t)oa->oa_addr;
	else if (current_mac_model->machineid == MACH_MACTV)
		return 0;
	else if (current_mac_model->machineid == MACH_MACIIFX)
		addr = (bus_addr_t)MAC68K_IIFX_ASCAUDIO_BASE;
	else
		addr = (bus_addr_t)MAC68K_ASCAUDIO_BASE;

	if (bus_space_map(oa->oa_tag, addr, MAC68K_ASCAUDIO_LEN, 0, &bsh))
		return (0);

	if (mac68k_bus_space_probe(oa->oa_tag, bsh, 0, 1)) {
		rval = 1;
	} else
		rval = 0;

	bus_space_unmap(oa->oa_tag, bsh, MAC68K_ASCAUDIO_LEN);

	return rval;
}

static void
ascaudioattach(device_t parent, device_t self, void *aux)
{
	struct ascaudio_softc *sc = device_private(self);
	struct obio_attach_args *oa = (struct obio_attach_args *)aux;
	bus_addr_t addr;
	uint8_t tmp;

	sc->sc_dev = self;
	sc->sc_tag = oa->oa_tag;

	if (oa->oa_addr != (-1))
		addr = (bus_addr_t)oa->oa_addr;
	else if (current_mac_model->machineid == MACH_MACIIFX)
		addr = (bus_addr_t)MAC68K_IIFX_ASCAUDIO_BASE;
	else
		addr = (bus_addr_t)MAC68K_ASCAUDIO_BASE;
	if (bus_space_map(sc->sc_tag, addr, MAC68K_ASCAUDIO_LEN, 0,
	    &sc->sc_handle)) {
		printf(": can't map memory space\n");
		return;
	}

	/* Pull in the options flags. */ 
	sc->sc_options = ((device_cfdata(self)->cf_flags) &
			    ASCAUDIO_OPTIONS_MASK);

	sc->sc_playbuf = kmem_alloc(BUFSIZE, KM_SLEEP);
	sc->sc_recbuf = kmem_alloc(BUFSIZE, KM_SLEEP);
	sc->sc_rptr = sc->sc_recbuf;
	sc->sc_getptr = sc->sc_recbuf;
	sc->sc_wptr = sc->sc_playbuf;
	sc->sc_putptr = sc->sc_playbuf;

	bus_space_write_1(sc->sc_tag, sc->sc_handle, ASCMODE, MODESTOP);

	sc->sc_ver = bus_space_read_1(oa->oa_tag, sc->sc_handle, 0x800);

	if (sc->sc_options & HIGHQUALITY) {
		tmp = bus_space_read_1(sc->sc_tag, sc->sc_handle, ASCRATE);
		switch (tmp) {
			case 2:
				sc->sc_rate = 22050;
				break;
			case 3:
				sc->sc_rate = 44100;
				break;
			default:
				sc->sc_rate = 22254;
				break;
		}

		tmp = bus_space_read_1(sc->sc_tag, sc->sc_handle, ASCTRL);
		if (tmp & STEREO)
			sc->sc_speakers = 2;
		else
			sc->sc_speakers = 1;

	} else {
		__USE(tmp);
		sc->sc_rate = 22254;
		sc->sc_speakers = 1;
	}

	if (sc->sc_options & LOWQUALITY) {
		sc->sc_slowcpu = true;
		if (sc->sc_slowcpu)
			sc->sc_rate /= 2;
	}

	if (sc->sc_ver == EASC_VER2)
		sc->sc_ver = EASC_VER;

	if (sc->sc_ver != EASC_VER)
		printf(": Apple Sound Chip");
	else
		printf(": Enhanced Apple Sound Chip");
	if (oa->oa_addr != (-1))
		printf(" at %x", oa->oa_addr);
	printf("\n");

	bus_space_write_1(sc->sc_tag, sc->sc_handle, ASCMODE, MODESTOP);

	if (mac68k_machine.aux_interrupts) {
		intr_establish(ascaudio_intr_est, sc, ASCIRQ);
	} else {
		via2_register_irq(VIA2_ASC, ascaudio_intr, sc);
	}
	ascaudio_intr_enable();

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_HIGH);
	callout_init(&sc->sc_pcallout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_pcallout, ascaudio_done_output, sc);
	callout_init(&sc->sc_rcallout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_rcallout, ascaudio_done_input, sc);

	sc->sc_vol = 255;

	sc->sc_audiodev = audio_attach_mi(&ascaudio_hw_if, sc, sc->sc_dev);

	if (!pmf_device_register(sc->sc_dev, NULL, NULL))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");


	if (sc->sc_ver != EASC_VER)
		return;

	if (sc->sc_options & HIGHQUALITY)
		tmp = CDQUALITY;
	else
		tmp = MACDEFAULTS;

	bus_space_write_1(sc->sc_tag, sc->sc_handle, FIFOCTRLA, tmp);
	bus_space_write_1(sc->sc_tag, sc->sc_handle, FIFOCTRLB, tmp);
	
}

int
ascaudioopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct ascaudio_softc *sc;

	sc = device_lookup_private(&ascaudio_cd, ASCAUDIOUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if (sc->sc_open)
		return (EBUSY);
	sc->sc_open = 1;

	return (0);
}

int
ascaudioclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct ascaudio_softc *sc;

	sc = device_lookup_private(&ascaudio_cd, ASCAUDIOUNIT(dev));
	sc->sc_open = 0;

	return (0);
}

int
ascaudioread(dev_t dev, struct uio *uio, int ioflag)
{
	return (ENXIO);
}

int
ascaudiowrite(dev_t dev, struct uio *uio, int ioflag)
{
	return (ENXIO);
}

int
ascaudioioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error;
#ifdef notyet
	struct ascaudio_softc *sc;
	int unit = ASCAUDIOUNIT(dev);

	sc = device_lookup_private(&ascaudio_cd, unit);
#endif
	error = 0;

	switch (cmd) {
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

#define ASCAUDIO_NFORMATS	2
static int
ascaudio_query_format(void *opaque, struct audio_format_query *ae)
{
	struct ascaudio_softc *sc = opaque;

	const struct audio_format asc_formats[ASCAUDIO_NFORMATS] = {
	      { .mode		= AUMODE_PLAY,
		.encoding	= AUDIO_ENCODING_SLINEAR_LE,
		.validbits	= 8,
		.precision	= 8,
		.channels	= sc->sc_speakers,
		.channel_mask	= sc->sc_speakers == 2 ? AUFMT_STEREO :
		    AUFMT_MONAURAL,
		.frequency_type	= 1,
		.frequency	= { sc->sc_rate }, },
	      { .mode		= AUMODE_RECORD,
		.encoding	= AUDIO_ENCODING_SLINEAR_LE,
		.validbits	= 8,
		.precision	= 8,
		.channels	= 1,
		.channel_mask	= AUFMT_MONAURAL,
		.frequency_type	= 1,
		.frequency	= { 11025 }, }
	};
			
	return audio_query_format(asc_formats, ASCAUDIO_NFORMATS, ae);
}

static int
ascaudio_set_format(void *opaque, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct ascaudio_softc *sc = opaque;

	KASSERT(mutex_owned(&sc->sc_lock));

	return 0;
}

static int
ascaudio_start_output(void *opaque, void *block, int blksize,
    void (*intr)(void *), void *intrarg)
{
	struct ascaudio_softc *sc;
	uint8_t *loc, tmp;
	int total;

	sc = (struct ascaudio_softc *)opaque;
	if (!sc)
		return (ENODEV);

	sc->sc_pintr = intr;
	sc->sc_pintrarg = intrarg;


	loc = block;
 	if (bus_space_read_1(sc->sc_tag, sc->sc_handle, ASCMODE) !=
								 MODEFIFO) {
		bus_space_write_1(sc->sc_tag, sc->sc_handle, ASCMODE, MODESTOP);

		if (sc->sc_ver == EASC_VER) {
			/* disable half interrupts channel a */
			bus_space_write_1(sc->sc_tag, sc->sc_handle, IRQA,
			    DISABLEHALFIRQ);
			/* Disable half interrupts channel b */
			bus_space_write_1(sc->sc_tag, sc->sc_handle, IRQB,
			    DISABLEHALFIRQ);
		}

		bus_space_write_1(sc->sc_tag, sc->sc_handle, ASCTEST, 0);
		bus_space_write_1(sc->sc_tag, sc->sc_handle, FIFOPARAM,
		    CLEARFIFO);
		tmp = 0;
		bus_space_write_1(sc->sc_tag, sc->sc_handle, APLAYREC, tmp);

		if (sc->sc_ver == EASC_VER) {
			/* enable interrupts channel b */
			bus_space_write_1(sc->sc_tag, sc->sc_handle, IRQB, 0);
		}
	}

	/* set the volume */
	tmp = sc->sc_vol >> 5;
	/* set volume for channel b left and right speakers */
	if (sc->sc_ver == EASC_VER) {
 		bus_space_write_1(sc->sc_tag, sc->sc_handle, A_LEFT_VOL, tmp);
 		bus_space_write_1(sc->sc_tag, sc->sc_handle, B_LEFT_VOL, tmp);
 		bus_space_write_1(sc->sc_tag, sc->sc_handle, A_RIGHT_VOL, tmp);
 		bus_space_write_1(sc->sc_tag, sc->sc_handle, B_RIGHT_VOL, tmp);
	} else
		bus_space_write_1(sc->sc_tag, sc->sc_handle, INTVOL, tmp << 5);

	total = blksize;
	if (sc->sc_putptr + blksize > sc->sc_playbuf + BUFSIZE)
		total = sc->sc_playbuf + BUFSIZE - sc->sc_putptr;

	memcpy(sc->sc_putptr, loc, total);
	sc->sc_putptr += total;
	loc += total;

	total = blksize - total;
	if (total) {
		sc->sc_putptr = sc->sc_playbuf;
		memcpy(sc->sc_playbuf, loc, total);
		sc->sc_putptr += total;
	}

	sc->sc_avail += blksize;
	if (sc->sc_avail > BUFSIZE)
		sc->sc_avail = BUFSIZE;

	/* start fifo playback */
	bus_space_write_1(sc->sc_tag, sc->sc_handle, ASCMODE, MODEFIFO);

	return 0;
}

static int
ascaudio_start_input(void *opaque, void *block, int blksize,
    void (*intr)(void *), void *intrarg)
{
	struct ascaudio_softc *sc;
	uint8_t tmp;
	int total;

	sc = (struct ascaudio_softc *)opaque;
	if (!sc)
		return (ENODEV);


	uint8_t *loc;
	loc = block;

	sc->sc_rintr = intr;
	sc->sc_rintrarg = intrarg;

 	if (bus_space_read_1(sc->sc_tag, sc->sc_handle, ASCMODE) !=
								 MODEFIFO) {
		bus_space_write_1(sc->sc_tag, sc->sc_handle, ASCMODE, MODESTOP);

		if (sc->sc_ver == EASC_VER) {
			/* disable half interrupts channel a */
			bus_space_write_1(sc->sc_tag, sc->sc_handle, IRQA,
			    DISABLEHALFIRQ);
			/* Disable half interrupts channel b */
			bus_space_write_1(sc->sc_tag, sc->sc_handle, IRQB,
			    DISABLEHALFIRQ);
		}

		bus_space_write_1(sc->sc_tag, sc->sc_handle, ASCTEST, 0);
		bus_space_write_1(sc->sc_tag, sc->sc_handle, FIFOPARAM,
		    CLEARFIFO);
		tmp = RECORDA;
		bus_space_write_1(sc->sc_tag, sc->sc_handle, APLAYREC, tmp);
		bus_space_write_1(sc->sc_tag, sc->sc_handle, INTVOL, 0xa0);

		if (sc->sc_ver == EASC_VER) {
			/* enable interrupts channel a */
			bus_space_write_1(sc->sc_tag, sc->sc_handle, IRQA, 0);
		}

		/* start fifo playback */
		bus_space_write_1(sc->sc_tag, sc->sc_handle, ASCMODE, MODEFIFO);

		return 0;
	}

	/* set the volume */
	tmp = sc->sc_vol >> 5;
	/* set volume for channel b left and right speakers */
	if (sc->sc_ver == EASC_VER) {
 		bus_space_write_1(sc->sc_tag, sc->sc_handle, A_LEFT_VOL, tmp);
 		bus_space_write_1(sc->sc_tag, sc->sc_handle, B_LEFT_VOL, tmp);
 		bus_space_write_1(sc->sc_tag, sc->sc_handle, A_RIGHT_VOL, tmp);
 		bus_space_write_1(sc->sc_tag, sc->sc_handle, B_RIGHT_VOL, tmp);
	} else
		bus_space_write_1(sc->sc_tag, sc->sc_handle, INTVOL, tmp << 5);

	total = blksize;
	if (sc->sc_getptr + blksize > sc->sc_recbuf + BUFSIZE)
		total = sc->sc_recbuf + BUFSIZE - sc->sc_getptr;

	memcpy(loc, sc->sc_getptr, total);
	sc->sc_getptr += total;
	loc += total;

	if (sc->sc_getptr >= sc->sc_recbuf + BUFSIZE)
		sc->sc_getptr = sc->sc_recbuf;

	total = blksize - total;
	if (total) {
		memcpy(loc, sc->sc_getptr, total);
		sc->sc_getptr += total;
	}

	sc->sc_recavail -= blksize;

	return 0;
}

static int
ascaudio_halt(void *opaque)
{
	ascaudio_softc_t *sc;

	sc = (ascaudio_softc_t *)opaque;

	KASSERT(mutex_owned(&sc->sc_lock));


	sc->sc_pintr = NULL;
	sc->sc_pintrarg = NULL;
	sc->sc_rintr = NULL;
	sc->sc_rintrarg = NULL;

	sc->sc_avail = 0;
	sc->sc_recavail = 0;

	callout_halt(&sc->sc_pcallout, &sc->sc_lock);
	callout_halt(&sc->sc_rcallout, &sc->sc_lock);

	bus_space_write_1(sc->sc_tag, sc->sc_handle, ASCMODE, MODESTOP);

	bus_space_write_1(sc->sc_tag, sc->sc_handle, FIFOPARAM, CLEARFIFO);

	sc->sc_rptr = sc->sc_recbuf;
	sc->sc_getptr = sc->sc_recbuf;
	sc->sc_wptr = sc->sc_playbuf;
	sc->sc_putptr = sc->sc_playbuf;

	if (sc->sc_ver != EASC_VER)
		return 0;

	/* disable half interrupts channel a */
	bus_space_write_1(sc->sc_tag, sc->sc_handle, IRQA, DISABLEHALFIRQ);
	/* disable half interrupts channel b */
	bus_space_write_1(sc->sc_tag, sc->sc_handle, IRQB, DISABLEHALFIRQ);

	return 0;
}

static int
ascaudio_getdev(void *opaque, struct audio_device *ret)
{
	strlcpy(ret->name, "Apple ASC Audio", sizeof(ret->name));
	strlcpy(ret->version, osrelease, sizeof(ret->version));
	strlcpy(ret->config, "ascaudio", sizeof(ret->config));

	return 0;
}

static int
ascaudio_set_port(void *opaque, mixer_ctrl_t *mc)
{
	struct ascaudio_softc *sc = opaque;

	KASSERT(mutex_owned(&sc->sc_lock));

	switch (mc->dev) {
	case ASC_OUTPUT_MASTER_VOLUME:
	case ASC_INPUT_DAC_VOLUME:
		if (mc->un.value.num_channels != 1)
			return EINVAL;
		sc->sc_vol = mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		return 0;
	}

	return ENXIO;
}

static int
ascaudio_get_port(void *opaque, mixer_ctrl_t *mc)
{
	struct ascaudio_softc *sc = opaque;

	KASSERT(mutex_owned(&sc->sc_lock));

	switch (mc->dev) {
	case ASC_OUTPUT_MASTER_VOLUME:
	case ASC_INPUT_DAC_VOLUME:
		if (mc->un.value.num_channels != 1)
			return EINVAL;
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_vol;
		return 0;
	}

	return ENXIO;
}

static int
ascaudio_query_devinfo(void *opaque, mixer_devinfo_t *di)
{
	ascaudio_softc_t *sc __diagused;

	sc = (ascaudio_softc_t *)opaque;

	KASSERT(mutex_owned(&sc->sc_lock));

	switch (di->index) {
	case ASC_OUTPUT_CLASS:
		di->mixer_class = ASC_OUTPUT_CLASS;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case ASC_INPUT_CLASS:
		di->mixer_class = ASC_INPUT_CLASS;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case ASC_OUTPUT_MASTER_VOLUME:
		di->mixer_class = ASC_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmaster);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 1;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case ASC_INPUT_DAC_VOLUME:
		di->mixer_class = ASC_INPUT_CLASS;
		strcpy(di->label.name, AudioNdac);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 1;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	}

	return ENXIO;
}

static int
ascaudio_get_props(void *opaque)
{

	return AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE |
	    AUDIO_PROP_INDEPENDENT;
}

static int
ascaudio_round_blocksize(void *opaque, int blksize, int mode,
    const audio_params_t *p)
{
	ascaudio_softc_t *sc __diagused;

	sc = (ascaudio_softc_t *)opaque;
	KASSERT(mutex_owned(&sc->sc_lock));

	if (mode == AUMODE_PLAY)
		return PLAYBLKSIZE;
	else
		return RECBLKSIZE;
}

static void
ascaudio_get_locks(void *opaque, kmutex_t **intr, kmutex_t **thread)
{
	ascaudio_softc_t *sc;

	sc = (ascaudio_softc_t *)opaque;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static int
ascaudio_intr_est(void *arg)
{
	ascaudio_intr(arg);

	return 0;
}

static void
ascaudio_intr(void *arg)
{
	struct ascaudio_softc *sc = arg;
	uint8_t status, val;
	bool again;
	int total, count, i;

	if (!sc)
		return;

	if (!sc->sc_pintr && !sc->sc_rintr)
		return;

	mutex_enter(&sc->sc_intr_lock);
	do {
		status = bus_space_read_1(sc->sc_tag, sc->sc_handle,
			    FIFOSTATUS);
		again = false;
		count = 0;
		if ((status & A_HALF) == 0)
			count = 0x200;
		if (count && ((status & A_FULL) == 0))
			count = 0x400;

		if (sc->sc_rintr && count) {
			total = count;
			if (sc->sc_rptr + count > sc->sc_recbuf + BUFSIZE)
				count = sc->sc_recbuf + BUFSIZE - sc->sc_rptr;

			while (total) {
				for (i = 0; i < count; i++) {
					val = bus_space_read_1(sc->sc_tag,
					    sc->sc_handle, FIFO_A);
					val ^= 0x80;
					*sc->sc_rptr++ = val;
				}
				if (sc->sc_rptr >= sc->sc_recbuf + BUFSIZE)
					sc->sc_rptr = sc->sc_recbuf;
				total -= count;
				sc->sc_recavail += count;
			}

			if (sc->sc_recavail > BUFSIZE)
				sc->sc_recavail = BUFSIZE;
		}

		count = 0;
		if (status &  B_FULL)
			count = 0x400;
		else if (status & B_HALF)
			count = 0x200;

		if (sc->sc_slowcpu)
			count /= 2;

		if (sc->sc_pintr && count) {
			if (sc->sc_avail < count) {
				if (sc->sc_pintr) {
					for (i = 0; i < 0x200; i++) {
						bus_space_write_1(sc->sc_tag,
						    sc->sc_handle, FIFO_A,
						    0x80);
						bus_space_write_1(sc->sc_tag,
						    sc->sc_handle, FIFO_B,
						    0x80);
					}
				} else {
					for (i = 0; i < 0x200; i++) {
						bus_space_write_1(sc->sc_tag,
						    sc->sc_handle, FIFO_B,
						    0x80);
					}
				}
			} else if (sc->sc_slowcpu) {
				for (i = 0; i < count; i++) {
					val = *sc->sc_wptr++;
					val ^= 0x80;
					bus_space_write_1(sc->sc_tag,
					    sc->sc_handle, FIFO_A, val);
					bus_space_write_1(sc->sc_tag,
					    sc->sc_handle, FIFO_B, val);
					bus_space_write_1(sc->sc_tag,
					    sc->sc_handle, FIFO_A, val);
					bus_space_write_1(sc->sc_tag,
					    sc->sc_handle, FIFO_B, val);
				}
				sc->sc_avail -= count;
				again = true;
			} else {
				for (i = 0; i < count; i++) {
					val = *sc->sc_wptr++;
					val ^= 0x80;
					bus_space_write_1(sc->sc_tag,
					    sc->sc_handle, FIFO_A, val);
					bus_space_write_1(sc->sc_tag,
					    sc->sc_handle, FIFO_B, val);
				}
				sc->sc_avail -= count;
				again = true;
			}
			if (sc->sc_wptr >= sc->sc_playbuf + BUFSIZE)
				sc->sc_wptr = sc->sc_playbuf;
		}

		if (sc->sc_pintr && (sc->sc_avail <= PLAYBLKSIZE))
			callout_schedule(&sc->sc_pcallout, 0);

		if (sc->sc_rintr && (sc->sc_recavail >= RECBLKSIZE))
			callout_schedule(&sc->sc_rcallout, 0);
	} while (again);
	mutex_exit(&sc->sc_intr_lock);
}
		
static void
ascaudio_intr_enable(void)
{
	int s;

	s = splhigh();
	if (VIA2 == VIA2OFF)
		via2_reg(vIER) = 0x80 | V2IF_ASC;
	else
		via2_reg(rIER) = 0x80 | V2IF_ASC;
	splx(s);
}

static void
ascaudio_done_output(void *arg)
{
	struct ascaudio_softc *sc = arg;

	mutex_enter(&sc->sc_intr_lock);
	if (sc->sc_pintr)
		(*sc->sc_pintr)(sc->sc_pintrarg);
	mutex_exit(&sc->sc_intr_lock);
}

static void
ascaudio_done_input(void *arg)
{
	struct ascaudio_softc *sc = arg;

	mutex_enter(&sc->sc_intr_lock);
	if (sc->sc_rintr)
		(*sc->sc_rintr)(sc->sc_rintrarg);
	mutex_exit(&sc->sc_intr_lock);
}

#ifdef _MODULE

MODULE(MODULE_CLASS_DRIVER, ascaudio, "audio");

static const struct cfiattrdata audiobuscf_iattrdata = {
	"audiobus", 0, { { NULL, NULL, 0 }, }
};
static const struct cfiattrdata * const ascaudio_attrs[] = {
	&audiobuscf_iattrdata, NULL
};

CFDRIVER_DECL(ascaudio, DV_DULL, ascaud_attrs);
extern struct cfattach ascaudio_ca;
static int ascaudioloc[] = { -1, -1 };

static struct cfdata ascaudio_cfdata[] = {
	{
		.cf_name = "ascaudio",
		.cf_atname = "ascaudio",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = ascaudioloc,
		.cf_flags = 0,
		.cf_pspec = NULL,
	},
	{ NULL, NULL, 0, 0, NULL, 0, NULL }
};

static int
ascaudio_modcmd(modcmd_t cmd, void *arg)
{
	devmajor_t cmajor = NODEVMAJOR, bmajor = NODEVMAJOR;
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = config_cfdriver_attach(&ascaudio_cd);
		if (error) {
			return error;
		}

		error = config_cfattach_attach(ascaudio_cd.cd_name, &ascaud_ca);
		if (error) {
			config_cfdriver_detach(&ascaudio_cd);
			aprint_error("%s: unable to register cfattach\n",
				ascaudio_cd.cd_name);

			return error;
		}

		error = config_cfdata_attach(ascaudio_cfdata, 1);
		if (error) {
			config_cfattach_detach(ascaudio_cd.cd_name, &ascaud_ca);
			config_cfdriver_detach(&ascaudio_cd);
			aprint_error("%s: unable to register cfdata\n",
				ascaudio_cd.cd_name);

			return error;
		}

		error = devsw_attach(ascaudio_cd.cd_name, NULL, &bmajor,
		    &ascaudio_cdevsw, &cmajor);
		if (error) {
			error = config_cfdata_detach(ascaudio_cfdata);
			if (error) {
				return error;
			}
			config_cfattach_detach(ascaudio_cd.cd_name, &ascaud_ca);
			config_cfdriver_detach(&ascaudio_cd);
			aprint_error("%s: unable to register devsw\n",
				ascaudio_cd.cd_name);

			return error;
		}

		(void)config_attach_pseudo(ascaudio_cfdata);

		return 0;
	case MODULE_CMD_FINI:
		error = config_cfdata_detach(ascaudio_cfdata);
		if (error) {
			return error;
		}

		config_cfattach_detach(ascaudio_cd.cd_name, &ascaud_ca);
		config_cfdriver_detach(&ascaudio_cd);
		devsw_detach(NULL, &ascaudio_cdevsw);

		return 0;
	default:
		return ENOTTY;
	}
}

#endif
