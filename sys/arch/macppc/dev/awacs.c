/*	$NetBSD: awacs.c,v 1.43.14.1 2016/10/05 20:55:31 skrll Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
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
__KERNEL_RCSID(0, "$NetBSD: awacs.c,v 1.43.14.1 2016/10/05 20:55:31 skrll Exp $");

#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <dev/auconv.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>

#include <uvm/uvm_extern.h>
#include <machine/autoconf.h>
#include <machine/pio.h>

#include <dev/ofw/openfirm.h>
#include <macppc/dev/dbdma.h>

#include <dev/i2c/sgsmixvar.h>
#include "sgsmix.h"
#include "opt_awacs.h"

#ifdef AWACS_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

/* sc_flags values */
#define AWACS_CAP_BSWAP		0x0001

struct awacs_softc {
	device_t sc_dev;
	int sc_flags;
	bus_space_tag_t sc_tag;
	bus_space_handle_t	sc_regh;
	bus_space_handle_t	sc_idmah;
	bus_space_handle_t	sc_odmah;
	void (*sc_ointr)(void *);	/* DMA completion intr handler */
	void *sc_oarg;			/* arg for sc_ointr() */
	int sc_opages;			/* # of output pages */

	void (*sc_iintr)(void *);	/* DMA completion intr handler */
	void *sc_iarg;			/* arg for sc_iintr() */

	uint32_t sc_record_source;	/* recording source mask */
	uint32_t sc_output_mask;	/* output mask */
	uint32_t sc_headphones_mask;	/* which reading of the gpio means */
	uint32_t sc_headphones_in;	/* headphones are present */

	int sc_screamer;
	int sc_have_perch;
	int vol_l, vol_r;
	int sc_bass, sc_treble;
	lwp_t *sc_thread;
	kcondvar_t sc_event;
	int sc_output_wanted;
	int sc_need_parallel_output;
#if NSGSMIX > 0
	device_t sc_sgsmix;
#endif

	u_int sc_codecctl0;
	u_int sc_codecctl1;
	u_int sc_codecctl2;
	u_int sc_codecctl4;
	u_int sc_codecctl5;
	u_int sc_codecctl6;
	u_int sc_codecctl7;
	u_int sc_soundctl;

	struct dbdma_regmap *sc_odma;
	struct dbdma_regmap *sc_idma;
	struct dbdma_command *sc_odmacmd;
	struct dbdma_command *sc_idmacmd;

#define AWACS_NFORMATS	2
	struct audio_format sc_formats[AWACS_NFORMATS];

	kmutex_t sc_lock;
	kmutex_t sc_intr_lock;
};

static int awacs_match(device_t, struct cfdata *, void *);
static void awacs_attach(device_t, device_t, void *);
static int awacs_intr(void *);
static int awacs_status_intr(void *);

static void awacs_close(void *);
static int awacs_query_encoding(void *, struct audio_encoding *);
static int awacs_set_params(void *, int, int, audio_params_t *, audio_params_t *,
		     stream_filter_list_t *, stream_filter_list_t *);

static int awacs_round_blocksize(void *, int, int, const audio_params_t *);
static int awacs_trigger_output(void *, void *, void *, int, void (*)(void *),
			 void *, const audio_params_t *);
static int awacs_trigger_input(void *, void *, void *, int, void (*)(void *),
			void *, const audio_params_t *);
static int awacs_halt_output(void *);
static int awacs_halt_input(void *);
static int awacs_getdev(void *, struct audio_device *);
static int awacs_set_port(void *, mixer_ctrl_t *);
static int awacs_get_port(void *, mixer_ctrl_t *);
static int awacs_query_devinfo(void *, mixer_devinfo_t *);
static size_t awacs_round_buffersize(void *, int, size_t);
static paddr_t awacs_mappage(void *, void *, off_t, int);
static int awacs_get_props(void *);
static void awacs_get_locks(void *, kmutex_t **, kmutex_t **);

static inline u_int awacs_read_reg(struct awacs_softc *, int);
static inline void awacs_write_reg(struct awacs_softc *, int, int);
static void awacs_write_codec(struct awacs_softc *, int);

void awacs_set_volume(struct awacs_softc *, int, int);
static void awacs_set_speaker_volume(struct awacs_softc *, int, int);
static void awacs_set_ext_volume(struct awacs_softc *, int, int);
static void awacs_set_loopthrough_volume(struct awacs_softc *, int, int);
static int awacs_set_rate(struct awacs_softc *, const audio_params_t *);
static void awacs_select_output(struct awacs_softc *, int);
static int awacs_check_headphones(struct awacs_softc *);
static void awacs_thread(void *);

#if NSGSMIX > 0
static void awacs_set_bass(struct awacs_softc *, int);
static void awacs_set_treble(struct awacs_softc *, int);
#endif
static int awacs_setup_sgsmix(device_t);

CFATTACH_DECL_NEW(awacs, sizeof(struct awacs_softc),
    awacs_match, awacs_attach, NULL, NULL);

const struct audio_hw_if awacs_hw_if = {
	NULL,			/* open */
	awacs_close,
	NULL,
	awacs_query_encoding,
	awacs_set_params,
	awacs_round_blocksize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	awacs_halt_output,
	awacs_halt_input,
	NULL,
	awacs_getdev,
	NULL,
	awacs_set_port,
	awacs_get_port,
	awacs_query_devinfo,
	NULL,
	NULL,
	awacs_round_buffersize,
	awacs_mappage,
	awacs_get_props,
	awacs_trigger_output,
	awacs_trigger_input,
	NULL,
	awacs_get_locks,
};

struct audio_device awacs_device = {
	"AWACS",
	"",
	"awacs"
};

#define AWACS_NFORMATS		2
#define AWACS_FORMATS_LE	0
static const struct audio_format awacs_formats[AWACS_NFORMATS] = {
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 2, AUFMT_STEREO, 8, 
	 {7350, 8820, 11025, 14700, 17640, 22050, 29400, 44100}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 2, AUFMT_STEREO, 8, 
	 {7350, 8820, 11025, 14700, 17640, 22050, 29400, 44100}},
};

/* register offset */
#define AWACS_SOUND_CTRL	0x00
#define AWACS_CODEC_CTRL	0x10
#define AWACS_CODEC_STATUS	0x20
#define AWACS_CLIP_COUNT	0x30
#define AWACS_BYTE_SWAP		0x40

/* sound control */
#define AWACS_INPUT_SUBFRAME0	0x00000001
#define AWACS_INPUT_SUBFRAME1	0x00000002
#define AWACS_INPUT_SUBFRAME2	0x00000004
#define AWACS_INPUT_SUBFRAME3	0x00000008

#define AWACS_OUTPUT_SUBFRAME0	0x00000010
#define AWACS_OUTPUT_SUBFRAME1	0x00000020
#define AWACS_OUTPUT_SUBFRAME2	0x00000040
#define AWACS_OUTPUT_SUBFRAME3	0x00000080

#define AWACS_RATE_44100	0x00000000
#define AWACS_RATE_29400	0x00000100
#define AWACS_RATE_22050	0x00000200
#define AWACS_RATE_17640	0x00000300
#define AWACS_RATE_14700	0x00000400
#define AWACS_RATE_11025	0x00000500
#define AWACS_RATE_8820		0x00000600
#define AWACS_RATE_7350		0x00000700
#define AWACS_RATE_MASK		0x00000700

#define AWACS_ERROR		0x00000800
#define AWACS_PORTCHG		0x00001000
#define AWACS_INTR_ERROR	0x00002000	/* interrupt on error */
#define AWACS_INTR_PORTCHG	0x00004000	/* interrupt on port change */

#define AWACS_STATUS_SUBFRAME	0x00018000	/* mask */

/* codec control */
#define AWACS_CODEC_ADDR0	0x00000000
#define AWACS_CODEC_ADDR1	0x00001000
#define AWACS_CODEC_ADDR2	0x00002000
#define AWACS_CODEC_ADDR4	0x00004000
#define AWACS_CODEC_ADDR5	0x00005000
#define AWACS_CODEC_ADDR6	0x00006000
#define AWACS_CODEC_ADDR7	0x00007000
#define AWACS_CODEC_EMSEL0	0x00000000
#define AWACS_CODEC_EMSEL1	0x00400000
#define AWACS_CODEC_EMSEL2	0x00800000
#define AWACS_CODEC_EMSEL4	0x00c00000
#define AWACS_CODEC_BUSY	0x01000000

/* cc0 */
#define AWACS_DEFAULT_CD_GAIN	0x000000bb
#define AWACS_INPUT_CD		0x00000200
#define AWACS_INPUT_LINE	0x00000400
#define AWACS_INPUT_MICROPHONE	0x00000800
#define AWACS_INPUT_MASK	0x00000e00

/* cc1 */
#define AWACS_LOOP_THROUGH	0x00000040
#define AWACS_MUTE_SPEAKER	0x00000080
#define AWACS_MUTE_HEADPHONE	0x00000200
#define AWACS_PARALLEL_OUTPUT	0x00000c00

/* output */
#define OUTPUT_SPEAKER		1
#define OUTPUT_HEADPHONES	2

/* codec status */

static const char *screamer[] = {"screamer", NULL};

/* 
 * list machines that have the headphone detect GPIO reversed here.
 * so far the only known case is the PowerBook 3400c and similar machines
 */
static const char *detect_reversed[] = {"AAPL,3400/2400",
					"AAPL,3500",
					NULL};

static const char *use_gpio4[] = {	"PowerMac3,1",
					"PowerMac3,2",
					"PowerMac3,3",
					NULL};

/*
 * list of machines that do not require AWACS_PARALLEL_OUTPUT
 */
static const char *no_parallel_output[] = {	"PowerBook3,1",
						NULL};

static int
awacs_match(device_t parent, struct cfdata *match, void *aux)
{
	struct confargs *ca;

	ca = aux;

	if (strcmp(ca->ca_name, "awacs") == 0 ||
	    strcmp(ca->ca_name, "davbus") == 0)
		return 100;

	if (ca->ca_nreg < 24 || ca->ca_nintr < 12)
		return 0;

	if (strcmp(ca->ca_name, "i2s") == 0)
		return 1;

	return 0;
}

static void
awacs_attach(device_t parent, device_t self, void *aux)
{
	struct awacs_softc *sc;
	struct confargs *ca = aux;
	int cirq, oirq, iirq, cirq_type, oirq_type, iirq_type;
	int len = -1, perch;
	int root_node;
	char compat[256];

	sc = device_private(self);
	sc->sc_dev = self;
	sc->sc_tag = ca->ca_tag;

	if (bus_space_map(sc->sc_tag, ca->ca_baseaddr + ca->ca_reg[0],
	    ca->ca_reg[1], 0, &sc->sc_regh) != 0)
		printf("couldn't map codec registers\n");
	if (bus_space_map(sc->sc_tag, ca->ca_baseaddr + ca->ca_reg[2],
	    ca->ca_reg[3], BUS_SPACE_MAP_LINEAR, &sc->sc_odmah) != 0)
		printf("couldn't map DMA out registers\n");
	if (bus_space_map(sc->sc_tag, ca->ca_baseaddr + ca->ca_reg[4],
	    ca->ca_reg[5], BUS_SPACE_MAP_LINEAR, &sc->sc_idmah) != 0)
		printf("couldn't map DMA in registers\n");

	sc->sc_odma = bus_space_vaddr(sc->sc_tag, sc->sc_odmah);
	sc->sc_idma = bus_space_vaddr(sc->sc_tag, sc->sc_idmah);
	sc->sc_odmacmd = dbdma_alloc(20 * sizeof(struct dbdma_command), NULL);
	sc->sc_idmacmd = dbdma_alloc(20 * sizeof(struct dbdma_command), NULL);

	if (strcmp(ca->ca_name, "i2s") == 0) {
		int node, intr[6];

		node = OF_child(ca->ca_node);
		if (node == 0) {
			printf("no i2s-a child\n");
			return;
		}
		if (OF_getprop(node, "interrupts", intr, sizeof(intr)) == -1) {
			printf("no interrupt property\n");
			return;
		}

		cirq = intr[0];
		oirq = intr[2];
		iirq = intr[4];
		cirq_type = intr[1] ? IST_LEVEL : IST_EDGE;
		oirq_type = intr[3] ? IST_LEVEL : IST_EDGE;
		iirq_type = intr[5] ? IST_LEVEL : IST_EDGE;
	} else if (ca->ca_nintr == 24) {
		cirq = ca->ca_intr[0];
		oirq = ca->ca_intr[2];
		iirq = ca->ca_intr[4];
		cirq_type = ca->ca_intr[1] ? IST_LEVEL : IST_EDGE;
		oirq_type = ca->ca_intr[3] ? IST_LEVEL : IST_EDGE;
		iirq_type = ca->ca_intr[5] ? IST_LEVEL : IST_EDGE;
	} else {
		cirq = ca->ca_intr[0];
		oirq = ca->ca_intr[1];
		iirq = ca->ca_intr[2];
		cirq_type = oirq_type = iirq_type = IST_EDGE;
	}

	intr_establish(cirq, cirq_type, IPL_BIO, awacs_status_intr, sc);
	intr_establish(oirq, oirq_type, IPL_AUDIO, awacs_intr, sc);
	intr_establish(iirq, iirq_type, IPL_AUDIO, awacs_intr, sc);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_AUDIO);

	cv_init(&sc->sc_event, "awacs_wait");

	/* check if the chip is a screamer */
	sc->sc_screamer = (of_compatible(ca->ca_node, screamer) != -1);
	if (!sc->sc_screamer) {
		/* look for 'sound' child node */
		int sound_node;

		sound_node = OF_child(ca->ca_node);
		while ((sound_node != 0) && (!sc->sc_screamer)) {

			sc->sc_screamer = 
			    (of_compatible(sound_node, screamer) != -1);
			sound_node = OF_peer(sound_node);
		}
	}

	if (sc->sc_screamer) {
		printf(" Screamer");
	}

	printf(": irq %d,%d,%d\n", cirq, oirq, iirq);

	sc->vol_l = 0;
	sc->vol_r = 0;

	memcpy(&sc->sc_formats, awacs_formats, sizeof(awacs_formats));

	/* XXX Uni-North based models don't have byteswap capability. */
	if (OF_finddevice("/uni-n") == -1) {

		sc->sc_flags |= AWACS_CAP_BSWAP;
	} else {

		AUFMT_INVALIDATE(&sc->sc_formats[AWACS_FORMATS_LE]);
	}

	sc->sc_soundctl = AWACS_INPUT_SUBFRAME0 | AWACS_OUTPUT_SUBFRAME0 |
		AWACS_RATE_44100 | AWACS_INTR_PORTCHG;
	awacs_write_reg(sc, AWACS_SOUND_CTRL, sc->sc_soundctl);

	sc->sc_codecctl0 = AWACS_CODEC_ADDR0 | AWACS_CODEC_EMSEL0;
	sc->sc_codecctl1 = AWACS_CODEC_ADDR1 | AWACS_CODEC_EMSEL0;
	sc->sc_codecctl2 = AWACS_CODEC_ADDR2 | AWACS_CODEC_EMSEL0;
	sc->sc_codecctl4 = AWACS_CODEC_ADDR4 | AWACS_CODEC_EMSEL0;
	sc->sc_codecctl5 = AWACS_CODEC_ADDR5 | AWACS_CODEC_EMSEL0;
	sc->sc_codecctl6 = AWACS_CODEC_ADDR6 | AWACS_CODEC_EMSEL0;
	sc->sc_codecctl7 = AWACS_CODEC_ADDR7 | AWACS_CODEC_EMSEL0;

	sc->sc_codecctl0 |= AWACS_INPUT_CD | AWACS_DEFAULT_CD_GAIN;
	awacs_write_codec(sc, sc->sc_codecctl0);

	/* Set loopthrough for external mixer on beige G3 */
	sc->sc_codecctl1 |= AWACS_LOOP_THROUGH;

        printf("%s: ", device_xname(sc->sc_dev));

	/*
	 * all(?) awacs have GPIOs to detect if there's something plugged into
	 * the headphone jack. The other GPIOs are either used for other jacks
	 * ( the PB3400c's microphone jack for instance ) or unused.
	 * The problem is that there are at least three different ways how
	 * those GPIOs are wired to the actual jacks.
	 * For now we bother only with headphone detection
	 */
	perch = OF_finddevice("/perch");
	root_node = OF_finddevice("/");
	if (of_compatible(root_node, detect_reversed) != -1) {

		/* 0x02 is for the microphone jack, high active */
		/*
		 * for some reason the gpio for the headphones jack is low
		 * active on the PB3400 and similar machines
		 */
		sc->sc_headphones_mask = 0x8;
		sc->sc_headphones_in = 0x0;
	} else if ((perch != -1) ||
	    (of_compatible(root_node, use_gpio4) != -1)) {
		/*
		 * this is for the beige G3's 'personality card' which uses
		 * yet another wiring of the headphone detect GPIOs
		 * some G4s use it as well
		 */
		sc->sc_headphones_mask = 0x04;
		sc->sc_headphones_in = 0x04;
	} else {
		/* while on most machines it's high active as well */
		sc->sc_headphones_mask = 0x8;
		sc->sc_headphones_in = 0x8;
	}

	if (of_compatible(root_node, no_parallel_output) != -1)
		sc->sc_need_parallel_output = 0;
	else {
		sc->sc_need_parallel_output = 1;
		sc->sc_codecctl1 |= AWACS_PARALLEL_OUTPUT;
	}
	
	if (awacs_check_headphones(sc)) {

                /* default output to headphones */
                printf("headphones\n");
                sc->sc_output_mask = OUTPUT_HEADPHONES;
        } else {

                /* default output to speakers */
                printf("speaker\n");
                sc->sc_output_mask = OUTPUT_SPEAKER;
        }
	sc->sc_output_wanted = sc->sc_output_mask;
	awacs_select_output(sc, sc->sc_output_mask);

	delay(100);
	if (sc->sc_screamer) {
		awacs_write_codec(sc, sc->sc_codecctl6);
		awacs_write_codec(sc, sc->sc_codecctl5);
		delay(2);
		awacs_write_codec(sc, sc->sc_codecctl1);
		awacs_write_codec(sc, sc->sc_codecctl7);
	}

	/* default input from CD */
	sc->sc_record_source = 1 << 0;
	sc->sc_codecctl0 &= ~AWACS_INPUT_MASK;
	sc->sc_codecctl0 |= AWACS_INPUT_CD;
	awacs_write_codec(sc, sc->sc_codecctl0);

	/* Enable interrupts and looping mode. */
	/* XXX ... */

	sc->sc_codecctl1 |= AWACS_LOOP_THROUGH;
	if (sc->sc_need_parallel_output)
		sc->sc_codecctl1 |= AWACS_PARALLEL_OUTPUT;
	awacs_write_codec(sc, sc->sc_codecctl1);

#if NSGSMIX > 0
	sc->sc_sgsmix = NULL;
#endif
	sc->sc_have_perch = 0;
	if (perch != -1) {

		len = OF_getprop(perch, "compatible", compat, 255);
		if (len > 0) {
			printf("%s: found '%s' personality card\n",
			    device_xname(sc->sc_dev), compat);
			sc->sc_have_perch = 1;
			config_finalize_register(sc->sc_dev,
			    awacs_setup_sgsmix);
		}
	}

	/* Set initial volume[s] */
	awacs_set_volume(sc, 144, 144);
	awacs_set_loopthrough_volume(sc, 0, 0);

	audio_attach_mi(&awacs_hw_if, sc, sc->sc_dev);
	
	if (kthread_create(PRI_NONE, 0, NULL, awacs_thread, sc,
	    &sc->sc_thread, "%s", "awacs") != 0) {
		printf("awacs: unable to create event kthread");
	}
	pmf_device_register(sc->sc_dev, NULL, NULL);
}

static int
awacs_setup_sgsmix(device_t cookie)
{
	struct awacs_softc *sc = device_private(cookie);
#if NSGSMIX > 0
	device_t dv;
	deviter_t di;
#endif

	if (!sc->sc_have_perch)
		return 0;
#if NSGSMIX > 0
	/* look for sgsmix */
	for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST);
	     dv != NULL;
	     dv = deviter_next(&di)) {
		if (device_is_a(dv, "sgsmix")) {
			sc->sc_sgsmix = dv;
			break;
		}
	}
	deviter_release(&di);
	if (sc->sc_sgsmix == NULL)
		return 0;

	printf("%s: using %s\n", device_xname(sc->sc_dev),
	    device_xname(sc->sc_sgsmix));

	sc->sc_codecctl1 &= ~AWACS_MUTE_HEADPHONE;
	awacs_write_codec(sc, sc->sc_codecctl1);

	awacs_select_output(sc, sc->sc_output_mask);
	awacs_set_volume(sc, sc->vol_l, sc->vol_r);
	awacs_set_bass(sc, 128);
	awacs_set_treble(sc, 128);
	cv_signal(&sc->sc_event);	
#endif
	return 0;
}


static inline u_int
awacs_read_reg(struct awacs_softc *sc, int reg)
{

	return bus_space_read_4(sc->sc_tag, sc->sc_regh, reg);
}

static inline void
awacs_write_reg(struct awacs_softc *sc, int reg, int val)
{

	bus_space_write_4(sc->sc_tag, sc->sc_regh, reg, val);
}

static void
awacs_write_codec(struct awacs_softc *sc, int value)
{

	do {
		delay(100);
	} while (awacs_read_reg(sc, AWACS_CODEC_CTRL) & AWACS_CODEC_BUSY);

	awacs_write_reg(sc, AWACS_CODEC_CTRL, value);

	do {
		delay(100);
	} while (awacs_read_reg(sc, AWACS_CODEC_CTRL) & AWACS_CODEC_BUSY);
}

static int
awacs_intr(void *v)
{
	struct awacs_softc *sc;
	struct dbdma_command *cmd;
	int count;
	int status;

	sc = v;
	mutex_spin_enter(&sc->sc_intr_lock);
	cmd = sc->sc_odmacmd;
	count = sc->sc_opages;
	/* Fill used buffer(s). */
	while (count-- > 0) {
		/* if DBDMA_INT_ALWAYS */
		if (in16rb(&cmd->d_command) & 0x30) {	/* XXX */
			status = in16rb(&cmd->d_status);
			cmd->d_status = 0;
			if (status)	/* status == 0x8400 */
				if (sc->sc_ointr)
					(*sc->sc_ointr)(sc->sc_oarg);
		}
		cmd++;
	}
	mutex_spin_exit(&sc->sc_intr_lock);

	return 1;
}

/*
 * Close function is called at splaudio().
 */
static void
awacs_close(void *h)
{
	struct awacs_softc *sc;

	sc = h;
	awacs_halt_output(sc);
	awacs_halt_input(sc);

	sc->sc_ointr = 0;
	sc->sc_iintr = 0;
}

static int
awacs_query_encoding(void *h, struct audio_encoding *ae)
{
	struct awacs_softc *sc;

	sc = h;
	ae->flags = AUDIO_ENCODINGFLAG_EMULATED;

	switch (ae->index) {
	case 0:
		strcpy(ae->name, AudioEslinear);
		ae->encoding = AUDIO_ENCODING_SLINEAR;
		ae->precision = 16;
		ae->flags = 0;
		return 0;
	case 1:
		strcpy(ae->name, AudioEslinear_be);
		ae->encoding = AUDIO_ENCODING_SLINEAR_BE;
		ae->precision = 16;
		ae->flags = 0;
		return 0;
	case 2:
		strcpy(ae->name, AudioEslinear_le);
		ae->encoding = AUDIO_ENCODING_SLINEAR_LE;
		ae->precision = 16;
		if (sc->sc_flags & AWACS_CAP_BSWAP)
			ae->flags = 0;
		return 0;
	case 3:
		strcpy(ae->name, AudioEulinear_be);
		ae->encoding = AUDIO_ENCODING_ULINEAR_BE;
		ae->precision = 16;
		return 0;
	case 4:
		strcpy(ae->name, AudioEulinear_le);
		ae->encoding = AUDIO_ENCODING_ULINEAR_LE;
		ae->precision = 16;
		return 0;
	case 5:
		strcpy(ae->name, AudioEmulaw);
		ae->encoding = AUDIO_ENCODING_ULAW;
		ae->precision = 8;
		return 0;
	case 6:
		strcpy(ae->name, AudioEalaw);
		ae->encoding = AUDIO_ENCODING_ALAW;
		ae->precision = 8;
		return 0;
	default:
		return EINVAL;
	}
}

static int
awacs_set_params(void *h, int setmode, int usemode,
		 audio_params_t *play, audio_params_t *rec,
		 stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct awacs_softc *sc;
	audio_params_t *p;
	stream_filter_list_t *fil;
	int mode, i;

	sc = h;
	p = NULL;
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
		fil = mode == AUMODE_PLAY ? pfil : rfil;
		switch (p->sample_rate) {
		case 48000:	/* aurateconv */
		case 44100:
		case 29400:
		case 22050:
		case 17640:
		case 14700:
		case 11025:
		case 8820:
		case 8000:	/* aurateconv */
		case 7350:
			break;
		default:
			return EINVAL;
		}
		awacs_write_reg(sc, AWACS_BYTE_SWAP, 0);
		i = auconv_set_converter(sc->sc_formats, AWACS_NFORMATS,
					 mode, p, true, fil);
		if (i < 0)
			return EINVAL;
		if (i == AWACS_FORMATS_LE)
			awacs_write_reg(sc, AWACS_BYTE_SWAP, 1);
		if (fil->req_size > 0)
			p = &fil->filters[0].param;
		if (awacs_set_rate(sc, p))
			return EINVAL;
	}
	return 0;
}

static int
awacs_round_blocksize(void *h, int size, int mode, const audio_params_t *param)
{

	if (size < PAGE_SIZE)
		size = PAGE_SIZE;
	return size & ~PGOFSET;
}

static int
awacs_halt_output(void *h)
{
	struct awacs_softc *sc;

	sc = h;
	dbdma_stop(sc->sc_odma);
	dbdma_reset(sc->sc_odma);
	return 0;
}

static int
awacs_halt_input(void *h)
{
	struct awacs_softc *sc;

	sc = h;
	dbdma_stop(sc->sc_idma);
	dbdma_reset(sc->sc_idma);
	return 0;
}

static int
awacs_getdev(void *h, struct audio_device *retp)
{

	*retp = awacs_device;
	return 0;
}

enum {
	AWACS_MONITOR_CLASS,
	AWACS_OUTPUT_CLASS,
	AWACS_RECORD_CLASS,
	AWACS_OUTPUT_SELECT,
	AWACS_VOL_MASTER,
	AWACS_INPUT_SELECT,
	AWACS_VOL_INPUT,
	AWACS_VOL_MONITOR,
	AWACS_BASS,
	AWACS_TREBLE,
	AWACS_ENUM_LAST
};

static int
awacs_set_port(void *h, mixer_ctrl_t *mc)
{
	struct awacs_softc *sc;
	int l, r;

	DPRINTF("awacs_set_port dev = %d, type = %d\n", mc->dev, mc->type);
	sc = h;
	l = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
	r = mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];

	switch (mc->dev) {
	case AWACS_OUTPUT_SELECT:
		/* No change necessary? */
		if (mc->un.mask == sc->sc_output_mask)
			return 0;
		awacs_select_output(sc, mc->un.mask);
		return 0;

	case AWACS_VOL_MASTER:
		awacs_set_volume(sc, l, r);
		return 0;

	case AWACS_INPUT_SELECT:
		/* no change necessary? */
		if (mc->un.mask == sc->sc_record_source)
			return 0;
		switch (mc->un.mask) {
		case 1 << 0: /* CD */
			sc->sc_codecctl0 &= ~AWACS_INPUT_MASK;
			sc->sc_codecctl0 |= AWACS_INPUT_CD;
			awacs_write_codec(sc, sc->sc_codecctl0);
			break;
		case 1 << 1: /* microphone */
			sc->sc_codecctl0 &= ~AWACS_INPUT_MASK;
			sc->sc_codecctl0 |= AWACS_INPUT_MICROPHONE;
			awacs_write_codec(sc, sc->sc_codecctl0);
			break;
		case 1 << 2: /* line in */
			sc->sc_codecctl0 &= ~AWACS_INPUT_MASK;
			sc->sc_codecctl0 |= AWACS_INPUT_LINE;
			awacs_write_codec(sc, sc->sc_codecctl0);
			break;
		default: /* invalid argument */
			return EINVAL;
		}
		sc->sc_record_source = mc->un.mask;
		return 0;

	case AWACS_VOL_INPUT:
		sc->sc_codecctl0 &= ~0xff;
		sc->sc_codecctl0 |= (l & 0xf0) | (r >> 4);
		awacs_write_codec(sc, sc->sc_codecctl0);
		return 0;

	case AWACS_VOL_MONITOR:
		awacs_set_loopthrough_volume(sc, l, r);
		return 0;

#if NSGSMIX > 0
	case AWACS_BASS:
		awacs_set_bass(sc, l);
		return 0;

	case AWACS_TREBLE:
		awacs_set_treble(sc, l);
		return 0;
#endif
	}

	return ENXIO;
}

static int
awacs_get_port(void *h, mixer_ctrl_t *mc)
{
	struct awacs_softc *sc;
	int l, r, vol;

	sc = h;
	switch (mc->dev) {
	case AWACS_OUTPUT_SELECT:
		mc->un.mask = sc->sc_output_mask;
		return 0;

	case AWACS_VOL_MASTER:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = sc->vol_l;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = sc->vol_r;
		return 0;

	case AWACS_INPUT_SELECT:
		mc->un.mask = sc->sc_record_source;
		return 0;

	case AWACS_VOL_INPUT:
		vol = sc->sc_codecctl0 & 0xff;
		l = (vol & 0xf0);
		r = (vol & 0x0f) << 4;
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = l;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = r;
		return 0;

	case AWACS_VOL_MONITOR:
		vol = sc->sc_codecctl5 & 0x3cf;
		l = (vol & 0x3c0) >> 6;
		r = vol & 0xf;
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = (15 - l) << 4;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = (15 - r) << 4;
		return 0;

#if NSGSMIX > 0
	case AWACS_BASS:
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_bass;
		return 0;

	case AWACS_TREBLE:
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_treble;
		return 0;
#endif

	default:
		return ENXIO;
	}

	return 0;
}

static int
awacs_query_devinfo(void *h, mixer_devinfo_t *dip)
{
#if NSGSMIX > 0
	struct awacs_softc *sc = h;
#endif

	switch (dip->index) {

	case AWACS_OUTPUT_SELECT:
		dip->mixer_class = AWACS_OUTPUT_CLASS;
		strcpy(dip->label.name, AudioNoutput);
		dip->type = AUDIO_MIXER_SET;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.s.num_mem = 2;
		strcpy(dip->un.s.member[0].label.name, AudioNspeaker);
		dip->un.s.member[0].mask = 1 << 0;
		strcpy(dip->un.s.member[1].label.name, AudioNheadphone);
		dip->un.s.member[1].mask = 1 << 1;
		return 0;

	case AWACS_VOL_MASTER:
		dip->mixer_class = AWACS_OUTPUT_CLASS;
		strcpy(dip->label.name, AudioNmaster);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 2;
		dip->un.v.delta = 16;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case AWACS_VOL_MONITOR:
		dip->mixer_class = AWACS_MONITOR_CLASS;
		strcpy(dip->label.name, AudioNmonitor);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

#if NSGSMIX > 0
	case AWACS_BASS:
		if (sc->sc_sgsmix == NULL)
			return ENXIO;
		dip->mixer_class = AWACS_OUTPUT_CLASS;
		strcpy(dip->label.name, AudioNbass);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNbass);
		return 0;

	case AWACS_TREBLE:
		if (sc->sc_sgsmix == NULL)
			return ENXIO;
		dip->mixer_class = AWACS_OUTPUT_CLASS;
		strcpy(dip->label.name, AudioNtreble);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNtreble);
		return 0;
#endif

	case AWACS_INPUT_SELECT:
		dip->mixer_class = AWACS_RECORD_CLASS;
		strcpy(dip->label.name, AudioNsource);
		dip->type = AUDIO_MIXER_SET;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.s.num_mem = 3;
		strcpy(dip->un.s.member[0].label.name, AudioNcd);
		dip->un.s.member[0].mask = 1 << 0;
		strcpy(dip->un.s.member[1].label.name, AudioNmicrophone);
		dip->un.s.member[1].mask = 1 << 1;
		strcpy(dip->un.s.member[2].label.name, AudioNline);
		dip->un.s.member[2].mask = 1 << 2;
		return 0;

	case AWACS_VOL_INPUT:
		dip->mixer_class = AWACS_RECORD_CLASS;
		strcpy(dip->label.name, AudioNrecord);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case AWACS_MONITOR_CLASS:
		dip->mixer_class = AWACS_MONITOR_CLASS;
		strcpy(dip->label.name, AudioCmonitor);
		dip->type = AUDIO_MIXER_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		return 0;

	case AWACS_OUTPUT_CLASS:
		dip->mixer_class = AWACS_OUTPUT_CLASS;
		strcpy(dip->label.name, AudioCoutputs);
		dip->type = AUDIO_MIXER_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		return 0;

	case AWACS_RECORD_CLASS:
		dip->mixer_class = AWACS_RECORD_CLASS;
		strcpy(dip->label.name, AudioCrecord);
		dip->type = AUDIO_MIXER_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		return 0;
	}

	return ENXIO;
}

static size_t
awacs_round_buffersize(void *h, int dir, size_t size)
{

	if (size > 65536)
		size = 65536;
	return size;
}

static paddr_t
awacs_mappage(void *h, void *mem, off_t off, int prot)
{

	if (off < 0)
		return -1;
	return -1;	/* XXX */
}

static int
awacs_get_props(void *h)
{
	return AUDIO_PROP_FULLDUPLEX /* | AUDIO_PROP_MMAP */;
}

static int
awacs_trigger_output(void *h, void *start, void *end, int bsize,
		     void (*intr)(void *), void *arg,
		     const audio_params_t *param)
{
	struct awacs_softc *sc;
	struct dbdma_command *cmd;
	vaddr_t va;
	int i, len, intmode;

	DPRINTF("trigger_output %p %p 0x%x\n", start, end, bsize);
	sc = h;
	cmd = sc->sc_odmacmd;
	sc->sc_ointr = intr;
	sc->sc_oarg = arg;
	sc->sc_opages = ((char *)end - (char *)start) / PAGE_SIZE;

#ifdef DIAGNOSTIC
	if (sc->sc_opages > 16)
		panic("awacs_trigger_output");
#endif

	va = (vaddr_t)start;
	len = 0;
	for (i = sc->sc_opages; i > 0; i--) {
		len += PAGE_SIZE;
		if (len < bsize)
			intmode = DBDMA_INT_NEVER;
		else {
			len = 0;
			intmode = DBDMA_INT_ALWAYS;
		}

		DBDMA_BUILD(cmd, DBDMA_CMD_OUT_MORE, 0, PAGE_SIZE, vtophys(va),
			intmode, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
		va += PAGE_SIZE;
		cmd++;
	}

	DBDMA_BUILD(cmd, DBDMA_CMD_NOP, 0, 0, 0,
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_ALWAYS);
	out32rb(&cmd->d_cmddep, vtophys((vaddr_t)sc->sc_odmacmd));

	dbdma_start(sc->sc_odma, sc->sc_odmacmd);

	return 0;
}

static int
awacs_trigger_input(void *h, void *start, void *end, int bsize,
		    void (*intr)(void *), void *arg,
		    const audio_params_t *param)
{

	DPRINTF("awacs_trigger_input called\n");
	return 1;
}
  
static void
awacs_get_locks(void *opaque, kmutex_t **intr, kmutex_t **thread)
{
	struct awacs_softc *sc = opaque;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static void
awacs_select_output(struct awacs_softc *sc, int mask)
{

#if NSGSMIX > 0
	if (sc->sc_sgsmix) {
		if (mask & OUTPUT_HEADPHONES) {
			/* mute speakers */
			sgsmix_set_speaker_vol(sc->sc_sgsmix, 0, 0);
			sgsmix_set_headphone_vol(sc->sc_sgsmix,
			    sc->vol_l, sc->vol_r);
		}
		if (mask & OUTPUT_SPEAKER) {
			/* mute headphones */
			sgsmix_set_speaker_vol(sc->sc_sgsmix,
			    sc->vol_l, sc->vol_r);
			sgsmix_set_headphone_vol(sc->sc_sgsmix, 0, 0);
		}
	} else {
#endif
	sc->sc_codecctl1 |= AWACS_MUTE_SPEAKER | AWACS_MUTE_HEADPHONE;
	if ((sc->vol_l > 0) || (sc->vol_r > 0)) {
		if (mask & OUTPUT_SPEAKER)
			sc->sc_codecctl1 &= ~AWACS_MUTE_SPEAKER;
		if (mask & OUTPUT_HEADPHONES)
			sc->sc_codecctl1 &= ~AWACS_MUTE_HEADPHONE;
	}
	awacs_write_codec(sc, sc->sc_codecctl1);
#if NSGSMIX > 0
	}
#endif
	sc->sc_output_mask = mask;
}

static void
awacs_set_speaker_volume(struct awacs_softc *sc, int left, int right)
{

#if NSGSMIX > 0
	if (sc->sc_sgsmix) {
		if (sc->sc_output_mask & OUTPUT_SPEAKER)
			sgsmix_set_speaker_vol(sc->sc_sgsmix, left, right);
	} else
#endif
	{
		int lval;
		int rval;
		uint32_t codecctl = sc->sc_codecctl1;

		lval = 15 - ((left  & 0xf0) >> 4);
		rval = 15 - ((right & 0xf0) >> 4);
		DPRINTF("speaker_volume %d %d\n", lval, rval);

		sc->sc_codecctl4 &= ~0x3cf;
		sc->sc_codecctl4 |= (lval << 6) | rval;
		awacs_write_codec(sc, sc->sc_codecctl4);
		if ((left == 0) && (right == 0)) {
			/*
			 * max. attenuation doesn't mean silence so we need to
			 * mute the output channel here
			 */
			codecctl |= AWACS_MUTE_SPEAKER;
		} else if (sc->sc_output_mask & OUTPUT_SPEAKER) {
			codecctl &= ~AWACS_MUTE_SPEAKER;
		}

		if (codecctl != sc->sc_codecctl1) {

			sc->sc_codecctl1 = codecctl;
			awacs_write_codec(sc, sc->sc_codecctl1);
		}	
	}
}

static void
awacs_set_ext_volume(struct awacs_softc *sc, int left, int right)
{

#if NSGSMIX > 0
	if (sc->sc_sgsmix) {
		if (sc->sc_output_mask & OUTPUT_HEADPHONES)
			sgsmix_set_headphone_vol(sc->sc_sgsmix, left, right);
	} else
#endif
	{
		int lval;
		int rval;
		uint32_t codecctl = sc->sc_codecctl1;

		lval = 15 - ((left  & 0xf0) >> 4);
		rval = 15 - ((right & 0xf0) >> 4);
		DPRINTF("ext_volume %d %d\n", lval, rval);

		sc->sc_codecctl2 &= ~0x3cf;
		sc->sc_codecctl2 |= (lval << 6) | rval;
		awacs_write_codec(sc, sc->sc_codecctl2);

		if ((left == 0) && (right == 0)) {
			/*
			 * max. attenuation doesn't mean silence so we need to
			 * mute the output channel here
			 */
			codecctl |= AWACS_MUTE_HEADPHONE;
		} else if (sc->sc_output_mask & OUTPUT_HEADPHONES) {

			codecctl &= ~AWACS_MUTE_HEADPHONE;
		}

		if (codecctl != sc->sc_codecctl1) {

			sc->sc_codecctl1 = codecctl;
			awacs_write_codec(sc, sc->sc_codecctl1);
		}	
	}
}

void
awacs_set_volume(struct awacs_softc *sc, int left, int right)
{

	awacs_set_ext_volume(sc, left, right);
	awacs_set_speaker_volume(sc, left, right);

	sc->vol_l = left;
	sc->vol_r = right;
}

#if NSGSMIX > 0
static void
awacs_set_bass(struct awacs_softc *sc, int bass)
{

	if (sc->sc_bass == bass)
		return;

	sc->sc_bass = bass;
	if (sc->sc_sgsmix)
		sgsmix_set_bass_treble(sc->sc_sgsmix, sc->sc_bass, 
		    sc->sc_treble);
}

static void
awacs_set_treble(struct awacs_softc *sc, int treble)
{

	if (sc->sc_treble == treble)
		return;

	sc->sc_treble = treble;
	if (sc->sc_sgsmix)
		sgsmix_set_bass_treble(sc->sc_sgsmix, sc->sc_bass, 
		    sc->sc_treble);
}
#endif

void
awacs_set_loopthrough_volume(struct awacs_softc *sc, int left, int right)
{
	int lval;
	int rval;

	lval = 15 - ((left  & 0xff) >> 4);
	rval = 15 - ((right & 0xff) >> 4);
	DPRINTF("loopthrough_volume %d %d\n", lval, rval);

	sc->sc_codecctl5 &= ~0x3cf;
	sc->sc_codecctl5 |= (lval << 6) | rval;
	awacs_write_codec(sc, sc->sc_codecctl5);
}

int
awacs_set_rate(struct awacs_softc *sc, const audio_params_t *p)
{
	int c;

	switch (p->sample_rate) {
	case 44100:
		c = AWACS_RATE_44100;
		break;
	case 29400:
		c = AWACS_RATE_29400;
		break;
	case 22050:
		c = AWACS_RATE_22050;
		break;
	case 17640:
		c = AWACS_RATE_17640;
		break;
	case 14700:
		c = AWACS_RATE_14700;
		break;
	case 11025:
		c = AWACS_RATE_11025;
		break;
	case 8820:
		c = AWACS_RATE_8820;
		break;
	case 7350:
		c = AWACS_RATE_7350;
		break;
	default:
		return -1;
	}

	sc->sc_soundctl &= ~AWACS_RATE_MASK;
	sc->sc_soundctl |= c;
	awacs_write_reg(sc, AWACS_SOUND_CTRL, sc->sc_soundctl);

	return 0;
}

static int
awacs_check_headphones(struct awacs_softc *sc)
{
	uint32_t reg;
	reg = awacs_read_reg(sc, AWACS_CODEC_STATUS);
	DPRINTF("%s: codec status reg %08x\n", device_xname(sc->sc_dev), reg);
	return ((reg & sc->sc_headphones_mask) == sc->sc_headphones_in);
}

static int
awacs_status_intr(void *cookie)
{
	struct awacs_softc *sc = cookie;
	int mask;
	
	mutex_spin_enter(&sc->sc_intr_lock);
	mask = awacs_check_headphones(sc) ? OUTPUT_HEADPHONES : OUTPUT_SPEAKER;
	if (mask != sc->sc_output_mask) {

		sc->sc_output_wanted = mask;
		cv_signal(&sc->sc_event);
	}
	/* clear the interrupt */
	awacs_write_reg(sc, AWACS_SOUND_CTRL, sc->sc_soundctl | AWACS_PORTCHG);
	mutex_spin_exit(&sc->sc_intr_lock);
	return 1;
}

static void
awacs_thread(void *cookie)
{
	struct awacs_softc *sc = cookie;
	
	mutex_enter(&sc->sc_intr_lock);
	while (1) {
		cv_timedwait(&sc->sc_event, &sc->sc_intr_lock, hz);
		if (sc->sc_output_wanted == sc->sc_output_mask)
			continue;

		awacs_select_output(sc, sc->sc_output_wanted);
		DPRINTF("%s: switching to %s\n", device_xname(sc->sc_dev), 
		    (sc->sc_output_wanted & OUTPUT_SPEAKER) ?
		    "speaker" : "headphones");
	}
}
