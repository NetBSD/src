/*	$NetBSD: snapper.c,v 1.17 2007/03/25 23:38:22 macallan Exp $	*/
/*	Id: snapper.c,v 1.11 2002/10/31 17:42:13 tsubai Exp	*/
/*     Id: i2s.c,v 1.12 2005/01/15 14:32:35 tsubai Exp         */
/*-
 * Copyright (c) 2002, 2003 Tsubai Masanari.  All rights reserved.
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

/*
 * Datasheet is available from
 * http://www.ti.com/sc/docs/products/analog/tas3004.html
 */

#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/auconv.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>
#include <dev/ofw/openfirm.h>
#include <macppc/dev/dbdma.h>

#include <uvm/uvm_extern.h>
#include <dev/i2c/i2cvar.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

#include <macppc/dev/deqvar.h>

#ifdef SNAPPER_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

struct snapper_softc {
	struct device sc_dev;
	int sc_flags;
	int sc_node;

	void (*sc_ointr)(void *);	/* dma completion intr handler */
	void *sc_oarg;			/* arg for sc_ointr() */
	int sc_opages;			/* # of output pages */

	void (*sc_iintr)(void *);	/* dma completion intr handler */
	void *sc_iarg;			/* arg for sc_iintr() */
	int sc_ipages;			/* # of input pages */

	u_int sc_record_source;		/* recording source mask */
	u_int sc_output_mask;		/* output source mask */

	u_char *sc_reg;
	i2c_addr_t sc_deqaddr;
	i2c_tag_t sc_i2c;

	int sc_rate;                    /* current sampling rate */
	int sc_bitspersample;

	u_int sc_vol_l;
	u_int sc_vol_r;
	u_int sc_treble;
	u_int sc_bass;
	u_int mixer[6]; /* s1_l, s2_l, an_l, s1_r, s2_r, an_r */

	dbdma_regmap_t *sc_odma;
	dbdma_regmap_t *sc_idma;
	unsigned char	dbdma_cmdspace[sizeof(struct dbdma_command) * 40 + 15];
	struct dbdma_command *sc_odmacmd;
	struct dbdma_command *sc_idmacmd;
};

int snapper_match(struct device *, struct cfdata *, void *);
void snapper_attach(struct device *, struct device *, void *);
void snapper_defer(struct device *);
int snapper_intr(void *);
void snapper_close(void *);
int snapper_query_encoding(void *, struct audio_encoding *);
int snapper_set_params(void *, int, int, audio_params_t *,
    audio_params_t *, stream_filter_list_t *, stream_filter_list_t *);
int snapper_round_blocksize(void *, int, int, const audio_params_t *);
int snapper_halt_output(void *);
int snapper_halt_input(void *);
int snapper_getdev(void *, struct audio_device *);
int snapper_set_port(void *, mixer_ctrl_t *);
int snapper_get_port(void *, mixer_ctrl_t *);
int snapper_query_devinfo(void *, mixer_devinfo_t *);
size_t snapper_round_buffersize(void *, int, size_t);
paddr_t snapper_mappage(void *, void *, off_t, int);
int snapper_get_props(void *);
int snapper_trigger_output(void *, void *, void *, int, void (*)(void *),
    void *, const audio_params_t *);
int snapper_trigger_input(void *, void *, void *, int, void (*)(void *),
    void *, const audio_params_t *);
void snapper_set_volume(struct snapper_softc *, int, int);
int snapper_set_rate(struct snapper_softc *);
void snapper_set_treble(struct snapper_softc *, int);
void snapper_set_bass(struct snapper_softc *, int);
void snapper_write_mixers(struct snapper_softc *);

int tas3004_write(struct snapper_softc *, u_int, const void *);
static int gpio_read(char *);
static void gpio_write(char *, int);
void snapper_mute_speaker(struct snapper_softc *, int);
void snapper_mute_headphone(struct snapper_softc *, int);
int snapper_cint(void *);
int tas3004_init(struct snapper_softc *);
void snapper_init(struct snapper_softc *, int);

struct cfattach snapper_ca = {
	"snapper", {}, sizeof(struct snapper_softc),
	snapper_match, snapper_attach
};

const struct audio_hw_if snapper_hw_if = {
	NULL,			/* open */
	snapper_close,
	NULL,
	snapper_query_encoding,
	snapper_set_params,
	snapper_round_blocksize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	snapper_halt_output,
	snapper_halt_input,
	NULL,
	snapper_getdev,
	NULL,
	snapper_set_port,
	snapper_get_port,
	snapper_query_devinfo,
	NULL,
	NULL,
	snapper_round_buffersize,
	snapper_mappage,
	snapper_get_props,
	snapper_trigger_output,
	snapper_trigger_input,
	NULL
};

struct audio_device snapper_device = {
	"SNAPPER",
	"",
	"snapper"
};

const uint8_t snapper_basstab[] = {
	0x96,	/* -18dB */
	0x94,	/* -17dB */
	0x92,	/* -16dB */
	0x90,	/* -15dB */
	0x8e,	/* -14dB */
	0x8c,	/* -13dB */
	0x8a,	/* -12dB */
	0x88,	/* -11dB */
	0x86,	/* -10dB */
	0x84,	/* -9dB */
	0x82,	/* -8dB */
	0x80,	/* -7dB */
	0x7e,	/* -6dB */
	0x7c,	/* -5dB */
	0x7a,	/* -4dB */
	0x78,	/* -3dB */
	0x76,	/* -2dB */
	0x74,	/* -1dB */
	0x72,	/* 0dB */
	0x6f,	/* 1dB */
	0x6d,	/* 2dB */
	0x6a,	/* 3dB */
	0x67,	/* 4dB */
	0x65,	/* 5dB */
	0x62,	/* 6dB */
	0x5f,	/* 7dB */
	0x5b,	/* 8dB */
	0x55,	/* 9dB */
	0x4f,	/* 10dB */
	0x49,	/* 11dB */
	0x43,	/* 12dB */
	0x3b,	/* 13dB */
	0x33,	/* 14dB */
	0x29,	/* 15dB */
	0x1e,	/* 16dB */
	0x11,	/* 17dB */
	0x01,	/* 18dB */
};

const uint8_t snapper_mixer_gain[178][3] = {
	{ 0x7f, 0x17, 0xaf }, /* 18.0 dB */
	{ 0x77, 0xfb, 0xaa }, /* 17.5 dB */
	{ 0x71, 0x45, 0x75 }, /* 17.0 dB */
	{ 0x6a, 0xef, 0x5d }, /* 16.5 dB */
	{ 0x64, 0xf4, 0x03 }, /* 16.0 dB */
	{ 0x5f, 0x4e, 0x52 }, /* 15.5 dB */
	{ 0x59, 0xf9, 0x80 }, /* 15.0 dB */
	{ 0x54, 0xf1, 0x06 }, /* 14.5 dB */
	{ 0x50, 0x30, 0xa1 }, /* 14.0 dB */
	{ 0x4b, 0xb4, 0x46 }, /* 13.5 dB */
	{ 0x47, 0x78, 0x28 }, /* 13.0 dB */
	{ 0x43, 0x78, 0xb0 }, /* 12.5 dB */
	{ 0x3f, 0xb2, 0x78 }, /* 12.0 dB */
	{ 0x3c, 0x22, 0x4c }, /* 11.5 dB */
	{ 0x38, 0xc5, 0x28 }, /* 11.0 dB */
	{ 0x35, 0x98, 0x2f }, /* 10.5 dB */
	{ 0x32, 0x98, 0xb0 }, /* 10.0 dB */
	{ 0x2f, 0xc4, 0x20 }, /* 9.5 dB */
	{ 0x2d, 0x18, 0x18 }, /* 9.0 dB */
	{ 0x2a, 0x92, 0x54 }, /* 8.5 dB */
	{ 0x28, 0x30, 0xaf }, /* 8.0 dB */
	{ 0x25, 0xf1, 0x25 }, /* 7.5 dB */
	{ 0x23, 0xd1, 0xcd }, /* 7.0 dB */
	{ 0x21, 0xd0, 0xd9 }, /* 6.5 dB */
	{ 0x1f, 0xec, 0x98 }, /* 6.0 dB */
	{ 0x1e, 0x23, 0x6d }, /* 5.5 dB */
	{ 0x1c, 0x73, 0xd5 }, /* 5.0 dB */
	{ 0x1a, 0xdc, 0x61 }, /* 4.5 dB */
	{ 0x19, 0x5b, 0xb8 }, /* 4.0 dB */
	{ 0x17, 0xf0, 0x94 }, /* 3.5 dB */
	{ 0x16, 0x99, 0xc0 }, /* 3.0 dB */
	{ 0x15, 0x56, 0x1a }, /* 2.5 dB */
	{ 0x14, 0x24, 0x8e }, /* 2.0 dB */
	{ 0x13, 0x04, 0x1a }, /* 1.5 dB */
	{ 0x11, 0xf3, 0xc9 }, /* 1.0 dB */
	{ 0x10, 0xf2, 0xb4 }, /* 0.5 dB */
	{ 0x10, 0x00, 0x00 }, /* 0.0 dB */
	{ 0x0f, 0x1a, 0xdf }, /* -0.5 dB */
	{ 0x0e, 0x42, 0x90 }, /* -1.0 dB */
	{ 0x0d, 0x76, 0x5a }, /* -1.5 dB */
	{ 0x0c, 0xb5, 0x91 }, /* -2.0 dB */
	{ 0x0b, 0xff, 0x91 }, /* -2.5 dB */
	{ 0x0b, 0x53, 0xbe }, /* -3.0 dB */
	{ 0x0a, 0xb1, 0x89 }, /* -3.5 dB */
	{ 0x0a, 0x18, 0x66 }, /* -4.0 dB */
	{ 0x09, 0x87, 0xd5 }, /* -4.5 dB */
	{ 0x08, 0xff, 0x59 }, /* -5.0 dB */
	{ 0x08, 0x7e, 0x80 }, /* -5.5 dB */
	{ 0x08, 0x04, 0xdc }, /* -6.0 dB */
	{ 0x07, 0x92, 0x07 }, /* -6.5 dB */
	{ 0x07, 0x25, 0x9d }, /* -7.0 dB */
	{ 0x06, 0xbf, 0x44 }, /* -7.5 dB */
	{ 0x06, 0x5e, 0xa5 }, /* -8.0 dB */
	{ 0x06, 0x03, 0x6e }, /* -8.5 dB */
	{ 0x05, 0xad, 0x50 }, /* -9.0 dB */
	{ 0x05, 0x5c, 0x04 }, /* -9.5 dB */
	{ 0x05, 0x0f, 0x44 }, /* -10.0 dB */
	{ 0x04, 0xc6, 0xd0 }, /* -10.5 dB */
	{ 0x04, 0x82, 0x68 }, /* -11.0 dB */
	{ 0x04, 0x41, 0xd5 }, /* -11.5 dB */
	{ 0x04, 0x04, 0xde }, /* -12.0 dB */
	{ 0x03, 0xcb, 0x50 }, /* -12.5 dB */
	{ 0x03, 0x94, 0xfa }, /* -13.0 dB */
	{ 0x03, 0x61, 0xaf }, /* -13.5 dB */
	{ 0x03, 0x31, 0x42 }, /* -14.0 dB */
	{ 0x03, 0x03, 0x8a }, /* -14.5 dB */
	{ 0x02, 0xd8, 0x62 }, /* -15.0 dB */
	{ 0x02, 0xaf, 0xa3 }, /* -15.5 dB */
	{ 0x02, 0x89, 0x2c }, /* -16.0 dB */
	{ 0x02, 0x64, 0xdb }, /* -16.5 dB */
	{ 0x02, 0x42, 0x93 }, /* -17.0 dB */
	{ 0x02, 0x22, 0x35 }, /* -17.5 dB */
	{ 0x02, 0x03, 0xa7 }, /* -18.0 dB */
	{ 0x01, 0xe6, 0xcf }, /* -18.5 dB */
	{ 0x01, 0xcb, 0x94 }, /* -19.0 dB */
	{ 0x01, 0xb1, 0xde }, /* -19.5 dB */
	{ 0x01, 0x99, 0x99 }, /* -20.0 dB */
	{ 0x01, 0x82, 0xaf }, /* -20.5 dB */
	{ 0x01, 0x6d, 0x0e }, /* -21.0 dB */
	{ 0x01, 0x58, 0xa2 }, /* -21.5 dB */
	{ 0x01, 0x45, 0x5b }, /* -22.0 dB */
	{ 0x01, 0x33, 0x28 }, /* -22.5 dB */
	{ 0x01, 0x21, 0xf9 }, /* -23.0 dB */
	{ 0x01, 0x11, 0xc0 }, /* -23.5 dB */
	{ 0x01, 0x02, 0x70 }, /* -24.0 dB */
	{ 0x00, 0xf3, 0xfb }, /* -24.5 dB */
	{ 0x00, 0xe6, 0x55 }, /* -25.0 dB */
	{ 0x00, 0xd9, 0x73 }, /* -25.5 dB */
	{ 0x00, 0xcd, 0x49 }, /* -26.0 dB */
	{ 0x00, 0xc1, 0xcd }, /* -26.5 dB */
	{ 0x00, 0xb6, 0xf6 }, /* -27.0 dB */
	{ 0x00, 0xac, 0xba }, /* -27.5 dB */
	{ 0x00, 0xa3, 0x10 }, /* -28.0 dB */
	{ 0x00, 0x99, 0xf1 }, /* -28.5 dB */
	{ 0x00, 0x91, 0x54 }, /* -29.0 dB */
	{ 0x00, 0x89, 0x33 }, /* -29.5 dB */
	{ 0x00, 0x81, 0x86 }, /* -30.0 dB */
	{ 0x00, 0x7a, 0x48 }, /* -30.5 dB */
	{ 0x00, 0x73, 0x70 }, /* -31.0 dB */
	{ 0x00, 0x6c, 0xfb }, /* -31.5 dB */
	{ 0x00, 0x66, 0xe3 }, /* -32.0 dB */
	{ 0x00, 0x61, 0x21 }, /* -32.5 dB */
	{ 0x00, 0x5b, 0xb2 }, /* -33.0 dB */
	{ 0x00, 0x56, 0x91 }, /* -33.5 dB */
	{ 0x00, 0x51, 0xb9 }, /* -34.0 dB */
	{ 0x00, 0x4d, 0x27 }, /* -34.5 dB */
	{ 0x00, 0x48, 0xd6 }, /* -35.0 dB */
	{ 0x00, 0x44, 0xc3 }, /* -35.5 dB */
	{ 0x00, 0x40, 0xea }, /* -36.0 dB */
	{ 0x00, 0x3d, 0x49 }, /* -36.5 dB */
	{ 0x00, 0x39, 0xdb }, /* -37.0 dB */
	{ 0x00, 0x36, 0x9e }, /* -37.5 dB */
	{ 0x00, 0x33, 0x90 }, /* -38.0 dB */
	{ 0x00, 0x30, 0xae }, /* -38.5 dB */
	{ 0x00, 0x2d, 0xf5 }, /* -39.0 dB */
	{ 0x00, 0x2b, 0x63 }, /* -39.5 dB */
	{ 0x00, 0x28, 0xf5 }, /* -40.0 dB */
	{ 0x00, 0x26, 0xab }, /* -40.5 dB */
	{ 0x00, 0x24, 0x81 }, /* -41.0 dB */
	{ 0x00, 0x22, 0x76 }, /* -41.5 dB */
	{ 0x00, 0x20, 0x89 }, /* -42.0 dB */
	{ 0x00, 0x1e, 0xb7 }, /* -42.5 dB */
	{ 0x00, 0x1c, 0xff }, /* -43.0 dB */
	{ 0x00, 0x1b, 0x60 }, /* -43.5 dB */
	{ 0x00, 0x19, 0xd8 }, /* -44.0 dB */
	{ 0x00, 0x18, 0x65 }, /* -44.5 dB */
	{ 0x00, 0x17, 0x08 }, /* -45.0 dB */
	{ 0x00, 0x15, 0xbe }, /* -45.5 dB */
	{ 0x00, 0x14, 0x87 }, /* -46.0 dB */
	{ 0x00, 0x13, 0x61 }, /* -46.5 dB */
	{ 0x00, 0x12, 0x4b }, /* -47.0 dB */
	{ 0x00, 0x11, 0x45 }, /* -47.5 dB */
	{ 0x00, 0x10, 0x4e }, /* -48.0 dB */
	{ 0x00, 0x0f, 0x64 }, /* -48.5 dB */
	{ 0x00, 0x0e, 0x88 }, /* -49.0 dB */
	{ 0x00, 0x0d, 0xb8 }, /* -49.5 dB */
	{ 0x00, 0x0c, 0xf3 }, /* -50.0 dB */
	{ 0x00, 0x0c, 0x3a }, /* -50.5 dB */
	{ 0x00, 0x0b, 0x8b }, /* -51.0 dB */
	{ 0x00, 0x0a, 0xe5 }, /* -51.5 dB */
	{ 0x00, 0x0a, 0x49 }, /* -52.0 dB */
	{ 0x00, 0x09, 0xb6 }, /* -52.5 dB */
	{ 0x00, 0x09, 0x2b }, /* -53.0 dB */
	{ 0x00, 0x08, 0xa8 }, /* -53.5 dB */
	{ 0x00, 0x08, 0x2c }, /* -54.0 dB */
	{ 0x00, 0x07, 0xb7 }, /* -54.5 dB */
	{ 0x00, 0x07, 0x48 }, /* -55.0 dB */
	{ 0x00, 0x06, 0xe0 }, /* -55.5 dB */
	{ 0x00, 0x06, 0x7d }, /* -56.0 dB */
	{ 0x00, 0x06, 0x20 }, /* -56.5 dB */
	{ 0x00, 0x05, 0xc9 }, /* -57.0 dB */
	{ 0x00, 0x05, 0x76 }, /* -57.5 dB */
	{ 0x00, 0x05, 0x28 }, /* -58.0 dB */
	{ 0x00, 0x04, 0xde }, /* -58.5 dB */
	{ 0x00, 0x04, 0x98 }, /* -59.0 dB */
	{ 0x00, 0x04, 0x56 }, /* -59.5 dB */
	{ 0x00, 0x04, 0x18 }, /* -60.0 dB */
	{ 0x00, 0x03, 0xdd }, /* -60.5 dB */
	{ 0x00, 0x03, 0xa6 }, /* -61.0 dB */
	{ 0x00, 0x03, 0x72 }, /* -61.5 dB */
	{ 0x00, 0x03, 0x40 }, /* -62.0 dB */
	{ 0x00, 0x03, 0x12 }, /* -62.5 dB */
	{ 0x00, 0x02, 0xe6 }, /* -63.0 dB */
	{ 0x00, 0x02, 0xbc }, /* -63.5 dB */
	{ 0x00, 0x02, 0x95 }, /* -64.0 dB */
	{ 0x00, 0x02, 0x70 }, /* -64.5 dB */
	{ 0x00, 0x02, 0x4d }, /* -65.0 dB */
	{ 0x00, 0x02, 0x2c }, /* -65.5 dB */
	{ 0x00, 0x02, 0x0d }, /* -66.0 dB */
	{ 0x00, 0x01, 0xf0 }, /* -66.5 dB */
	{ 0x00, 0x01, 0xd4 }, /* -67.0 dB */
	{ 0x00, 0x01, 0xba }, /* -67.5 dB */
	{ 0x00, 0x01, 0xa1 }, /* -68.0 dB */
	{ 0x00, 0x01, 0x8a }, /* -68.5 dB */
	{ 0x00, 0x01, 0x74 }, /* -69.0 dB */
	{ 0x00, 0x01, 0x5f }, /* -69.5 dB */
	{ 0x00, 0x01, 0x4b }, /* -70.0 dB */
	{ 0x00, 0x00, 0x00 }  /* Mute */
};

#define SNAPPER_NFORMATS	2
static const struct audio_format snapper_formats[SNAPPER_NFORMATS] = {
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 2, AUFMT_STEREO, 3, {32000, 44100, 48000}},
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_BE, 24, 24,
	 2, AUFMT_STEREO, 3, {32000, 44100, 48000}},
};

static u_char *amp_mute;
static u_char *headphone_mute;
static u_char *audio_hw_reset;
static u_char *headphone_detect;
static int headphone_detect_active;


/* I2S registers */
#define I2S_INT		0x00
#define I2S_FORMAT	0x10
#define I2S_FRAMECOUNT	0x40
#define I2S_FRAMEMATCH	0x50
#define I2S_WORDSIZE	0x60

/* I2S_INT register definitions */
#define I2S_INT_CLKSTOPPEND 0x01000000  /* clock-stop interrupt pending */

/* FCR(0x3c) bits */
#define I2S0CLKEN      0x1000
#define I2S0EN         0x2000
#define I2S1CLKEN      0x080000
#define I2S1EN         0x100000
#define FCR3C_BITMASK "\020\25I2S1EN\24I2S1CLKEN\16I2S0EN\15I2S0CLKEN"

/* TAS3004 registers */
#define DEQ_MCR1	0x01	/* Main control register 1 (1byte) */
#define DEQ_DRC		0x02	/* Dynamic range compression (6bytes?) */
#define DEQ_VOLUME	0x04	/* Volume (6bytes) */
#define DEQ_TREBLE	0x05	/* Treble control (1byte) */
#define DEQ_BASS	0x06	/* Bass control (1byte) */
#define DEQ_MIXER_L	0x07	/* Mixer left gain (9bytes) */
#define DEQ_MIXER_R	0x08	/* Mixer right gain (9bytes) */
#define DEQ_LB0		0x0a	/* Left biquad 0 (15bytes) */
#define DEQ_LB1		0x0b	/* Left biquad 1 (15bytes) */
#define DEQ_LB2		0x0c	/* Left biquad 2 (15bytes) */
#define DEQ_LB3		0x0d	/* Left biquad 3 (15bytes) */
#define DEQ_LB4		0x0e	/* Left biquad 4 (15bytes) */
#define DEQ_LB5		0x0f	/* Left biquad 5 (15bytes) */
#define DEQ_LB6		0x10	/* Left biquad 6 (15bytes) */
#define DEQ_RB0		0x13	/* Right biquad 0 (15bytes) */
#define DEQ_RB1		0x14	/* Right biquad 1 (15bytes) */
#define DEQ_RB2		0x15	/* Right biquad 2 (15bytes) */
#define DEQ_RB3		0x16	/* Right biquad 3 (15bytes) */
#define DEQ_RB4		0x17	/* Right biquad 4 (15bytes) */
#define DEQ_RB5		0x18	/* Right biquad 5 (15bytes) */
#define DEQ_RB6		0x19	/* Right biquad 6 (15bytes) */
#define DEQ_LLB		0x21	/* Left loudness biquad (15bytes) */
#define DEQ_RLB		0x22	/* Right loudness biquad (15bytes) */
#define DEQ_LLB_GAIN	0x23	/* Left loudness biquad gain (3bytes) */
#define DEQ_RLB_GAIN	0x24	/* Right loudness biquad gain (3bytes) */
#define DEQ_ACR		0x40	/* Analog control register (1byte) */
#define DEQ_MCR2	0x43	/* Main control register 2 (1byte) */

#define DEQ_MCR1_FL	0x80	/* Fast load */
#define DEQ_MCR1_SC	0x40	/* SCLK frequency */
#define  DEQ_MCR1_SC_32	0x00	/*  32fs */
#define  DEQ_MCR1_SC_64	0x40	/*  64fs */
#define DEQ_MCR1_SM	0x30	/* Output serial port mode */
#define  DEQ_MCR1_SM_L	0x00	/*  Left justified */
#define  DEQ_MCR1_SM_R	0x10	/*  Right justified */
#define  DEQ_MCR1_SM_I2S 0x20	/*  I2S */
#define DEQ_MCR1_W	0x03	/* Serial port word length */
#define  DEQ_MCR1_W_16	0x00	/*  16 bit */
#define  DEQ_MCR1_W_18	0x01	/*  18 bit */
#define  DEQ_MCR1_W_20	0x02	/*  20 bit */
#define  DEQ_MCR1_W_24	0x03	/*  20 bit */

#define DEQ_MCR2_DL	0x80	/* Download */
#define DEQ_MCR2_AP	0x02	/* All pass mode */

#define DEQ_ACR_ADM	0x80	/* ADC output mode */
#define DEQ_ACR_LRB	0x40	/* Select B input */
#define DEQ_ACR_DM	0x0c	/* De-emphasis control */
#define  DEQ_ACR_DM_OFF	0x00	/*  off */
#define  DEQ_ACR_DM_48	0x04	/*  fs = 48kHz */
#define  DEQ_ACR_DM_44	0x08	/*  fs = 44.1kHz */
#define DEQ_ACR_INP	0x02	/* Analog input select */
#define  DEQ_ACR_INP_A	0x00	/*  A */
#define  DEQ_ACR_INP_B	0x02	/*  B */
#define DEQ_ACR_APD	0x01	/* Analog power down */

struct tas3004_reg {
	u_char MCR1[1];
	u_char DRC[6];
	u_char VOLUME[6];
	u_char TREBLE[1];
	u_char BASS[1];
	u_char MIXER_L[9];
	u_char MIXER_R[9];
	u_char LB0[15];
	u_char LB1[15];
	u_char LB2[15];
	u_char LB3[15];
	u_char LB4[15];
	u_char LB5[15];
	u_char LB6[15];
	u_char RB0[15];
	u_char RB1[15];
	u_char RB2[15];
	u_char RB3[15];
	u_char RB4[15];
	u_char RB5[15];
	u_char RB6[15];
	u_char LLB[15];
	u_char RLB[15];
	u_char LLB_GAIN[3];
	u_char RLB_GAIN[3];
	u_char ACR[1];
	u_char MCR2[1];
};

#define GPIO_OUTSEL	0xf0	/* Output select */
		/*	0x00	GPIO bit0 is output
			0x10	media-bay power
			0x20	reserved
			0x30	MPIC */

#define GPIO_ALTOE	0x08	/* Alternate output enable */
		/*	0x00	Use DDR
			0x08	Use output select */

#define GPIO_DDR	0x04	/* Data direction */
#define GPIO_DDR_OUTPUT	0x04	/* Output */
#define GPIO_DDR_INPUT	0x00	/* Input */

#define GPIO_LEVEL	0x02	/* Pin level (RO) */

#define	GPIO_DATA	0x01	/* Data */

int
snapper_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct confargs *ca;
	int soundbus, soundchip, soundcodec;
	char compat[32];

	ca = aux;
	if (strcmp(ca->ca_name, "i2s") != 0)
		return 0;

	if ((soundbus = OF_child(ca->ca_node)) == 0 ||
	    (soundchip = OF_child(soundbus)) == 0)
		return 0;

	bzero(compat, sizeof compat);
	OF_getprop(soundchip, "compatible", compat, sizeof compat);

	if (strcmp(compat, "snapper") == 0)
		return 1;
		
	if (OF_getprop(soundchip,"platform-tas-codec-ref",
	    &soundcodec, sizeof soundcodec) == sizeof soundcodec)
		return 1;

	return 0;
}

void
snapper_attach(struct device *parent, struct device *self, void *aux)
{
	struct snapper_softc *sc;
	struct confargs *ca;
	unsigned long v;
	int cirq, oirq, iirq, cirq_type, oirq_type, iirq_type;
	int soundbus, intr[6];

	sc = (struct snapper_softc *)self;
	ca = aux;

	v = (((unsigned long) &sc->dbdma_cmdspace[0]) + 0xf) & ~0xf;
	sc->sc_odmacmd = (struct dbdma_command *) v;
	sc->sc_idmacmd = sc->sc_odmacmd + 20;

#ifdef DIAGNOSTIC
	if ((vaddr_t)sc->sc_odmacmd & 0x0f) {
		printf(": bad dbdma alignment\n");
		return;
	}
#endif

	ca->ca_reg[0] += ca->ca_baseaddr;
	ca->ca_reg[2] += ca->ca_baseaddr;
	ca->ca_reg[4] += ca->ca_baseaddr;

	sc->sc_node = ca->ca_node;
	sc->sc_reg = (void *)ca->ca_reg[0];
	sc->sc_odma = (void *)ca->ca_reg[2];
	sc->sc_idma = (void *)ca->ca_reg[4];

	soundbus = OF_child(ca->ca_node);
	OF_getprop(soundbus, "interrupts", intr, sizeof intr);
	cirq = intr[0];
	oirq = intr[2];
	iirq = intr[4];
	cirq_type = intr[1] ? IST_LEVEL : IST_EDGE;
	oirq_type = intr[3] ? IST_LEVEL : IST_EDGE;
	iirq_type = intr[5] ? IST_LEVEL : IST_EDGE;

	/* intr_establish(cirq, cirq_type, IPL_AUDIO, snapper_intr, sc); */
	intr_establish(oirq, oirq_type, IPL_AUDIO, snapper_intr, sc);
	intr_establish(iirq, iirq_type, IPL_AUDIO, snapper_intr, sc);

	printf(": irq %d,%d,%d\n", cirq, oirq, iirq);

	config_interrupts(self, snapper_defer);
}

void
snapper_defer(struct device *dev)
{
	struct snapper_softc *sc;
	struct device *dv;
	struct deq_softc *deq;
	
	sc = (struct snapper_softc *)dev;
	/*
	for (dv = alldevs.tqh_first; dv; dv=dv->dv_list.tqe_next)
		if (strncmp(dv->dv_xname, "ki2c", 4) == 0 &&
		    strncmp(device_parent(dv)->dv_xname, "obio", 4) == 0)
			sc->sc_i2c = dv;
	*/
	for (dv = alldevs.tqh_first; dv; dv=dv->dv_list.tqe_next)
		if (strncmp(dv->dv_xname, "deq", 3) == 0 &&
		    strncmp(device_parent(dv)->dv_xname, "ki2c", 4) == 0) {
		    	deq=(struct deq_softc *)dv;
			sc->sc_i2c = deq->sc_i2c;
			sc->sc_deqaddr=deq->sc_address;
		}

	if (sc->sc_i2c == NULL) {
		printf("%s: unable to find i2c\n", sc->sc_dev.dv_xname);
		return;
	}

	/* XXX If i2c was failed to attach, what should we do? */

	audio_attach_mi(&snapper_hw_if, sc, &sc->sc_dev);

	/* ki2c_setmode(sc->sc_i2c, I2C_STDSUBMODE); */
	snapper_init(sc, sc->sc_node);
}

int
snapper_intr(void *v)
{
	struct snapper_softc *sc;
	struct dbdma_command *cmd;
	int count;
	int status;

	sc = v;
	cmd = sc->sc_odmacmd;
	count = sc->sc_opages;
	/* Fill used buffer(s). */
	while (count-- > 0) {
		if ((dbdma_ld16(&cmd->d_command) & 0x30) == 0x30) {
			status = dbdma_ld16(&cmd->d_status);
			cmd->d_status = 0;
			if (status)	/* status == 0x8400 */
				if (sc->sc_ointr)
					(*sc->sc_ointr)(sc->sc_oarg);
		}
		cmd++;
	}

	cmd = sc->sc_idmacmd;
	count = sc->sc_ipages;
	while (count-- > 0) {
		if ((dbdma_ld16(&cmd->d_command) & 0x30) == 0x30) {
			status = dbdma_ld16(&cmd->d_status);
			cmd->d_status = 0;
			if (status)	/* status == 0x8400 */
				if (sc->sc_iintr)
					(*sc->sc_iintr)(sc->sc_iarg);
		}
		cmd++;
	}
	    

	return 1;
}

/*
 * Close function is called at splaudio().
 */
void
snapper_close(void *h)
{
	struct snapper_softc *sc;

	sc = h;
	snapper_halt_output(sc);
	snapper_halt_input(sc);

	sc->sc_ointr = 0;
	sc->sc_iintr = 0;
}

int
snapper_query_encoding(void *h, struct audio_encoding *ae)
{

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
		DPRINTF("snapper_query_encoding: invalid encoding %d\n", ae->index);
		return EINVAL;
	}
}

int
snapper_set_params(void *h, int setmode, int usemode,
		   audio_params_t *play, audio_params_t *rec,
		   stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct snapper_softc *sc;
	audio_params_t *p;
	stream_filter_list_t *fil;
	int mode;

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
		if (p->sample_rate < 4000 || p->sample_rate > 50000) {
			DPRINTF("snapper_set_params: invalid rate %d\n", 
			    p->sample_rate);
			return EINVAL;
		}

		fil = mode == AUMODE_PLAY ? pfil : rfil;
		if (auconv_set_converter(snapper_formats, SNAPPER_NFORMATS,
					 mode, p, true, fil) < 0) {
			DPRINTF("snapper_set_params: auconv_set_converter failed\n");
			return EINVAL;
		}
		if (fil->req_size > 0)
			p = &fil->filters[0].param;
	}

	/* Set the speed. p points HW encoding. */
	if (p) {
		sc->sc_rate = p->sample_rate;
		sc->sc_bitspersample = p->precision;
	}
	return 0;
}

int
snapper_round_blocksize(void *h, int size, int mode,
			const audio_params_t *param)
{

	if (size < NBPG)
		size = NBPG;
	return size & ~PGOFSET;
}

int
snapper_halt_output(void *h)
{
	struct snapper_softc *sc;

	sc = h;
	dbdma_stop(sc->sc_odma);
	dbdma_reset(sc->sc_odma);
	return 0;
}

int
snapper_halt_input(void *h)
{
	struct snapper_softc *sc;

	sc = h;
	dbdma_stop(sc->sc_idma);
	dbdma_reset(sc->sc_idma);
	return 0;
}

int
snapper_getdev(void *h, struct audio_device *retp)
{

	*retp = snapper_device;
	return 0;
}

enum {
	SNAPPER_MONITOR_CLASS,
	SNAPPER_OUTPUT_CLASS,
	SNAPPER_RECORD_CLASS,
	SNAPPER_OUTPUT_SELECT,
	SNAPPER_VOL_OUTPUT,
	SNAPPER_DIGI1,
	SNAPPER_DIGI2,
	SNAPPER_ANALOG,
	SNAPPER_INPUT_SELECT,
	SNAPPER_VOL_INPUT,
	SNAPPER_TREBLE,
	SNAPPER_BASS,
	SNAPPER_ENUM_LAST
};

int
snapper_set_port(void *h, mixer_ctrl_t *mc)
{
	struct snapper_softc *sc;
	int l, r;
	u_char data;

	DPRINTF("snapper_set_port dev = %d, type = %d\n", mc->dev, mc->type);
	sc = h;
	l = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
	r = mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];

	switch (mc->dev) {
	case SNAPPER_OUTPUT_SELECT:
		/* No change necessary? */
		if (mc->un.mask == sc->sc_output_mask)
			return 0;

		snapper_mute_speaker(sc, 1);
		snapper_mute_headphone(sc, 1);
		if (mc->un.mask & 1 << 0)
			snapper_mute_speaker(sc, 0);
		if (mc->un.mask & 1 << 1)
			snapper_mute_headphone(sc, 0);

		sc->sc_output_mask = mc->un.mask;
		return 0;

	case SNAPPER_VOL_OUTPUT:
		snapper_set_volume(sc, l, r);
		return 0;

	case SNAPPER_INPUT_SELECT:
		/* no change necessary? */
		if (mc->un.mask == sc->sc_record_source)
			return 0;
		switch (mc->un.mask) {
		case 1 << 0: /* microphone */
			/* Select right channel of B input */
			data = DEQ_ACR_ADM | DEQ_ACR_LRB | DEQ_ACR_INP_B;
			tas3004_write(sc, DEQ_ACR, &data);
			break;
		case 1 << 1: /* line in */
			/* Select both channels of A input */
			data = 0;
			tas3004_write(sc, DEQ_ACR, &data);
			break;
		default: /* invalid argument */
			return EINVAL;
		}
		sc->sc_record_source = mc->un.mask;
		return 0;

	case SNAPPER_VOL_INPUT:
		/* XXX TO BE DONE */
		return 0;

	case SNAPPER_BASS:
		snapper_set_bass(sc,l);
		return 0;
	case SNAPPER_TREBLE:
		snapper_set_treble(sc,l);
		return 0;
	case SNAPPER_DIGI1:
		sc->mixer[0]=l;
		sc->mixer[3]=r;
		snapper_write_mixers(sc);
		return 0;
	case SNAPPER_DIGI2:
		sc->mixer[1]=l;
		sc->mixer[4]=r;
		snapper_write_mixers(sc);
		return 0;
	case SNAPPER_ANALOG:
		sc->mixer[2]=l;
		sc->mixer[5]=r;
		snapper_write_mixers(sc);
		return 0;
	}	
	return ENXIO;
}

int
snapper_get_port(void *h, mixer_ctrl_t *mc)
{
	struct snapper_softc *sc;

	DPRINTF("snapper_get_port dev = %d, type = %d\n", mc->dev, mc->type);
	sc = h;
	switch (mc->dev) {
	case SNAPPER_OUTPUT_SELECT:
		mc->un.mask = sc->sc_output_mask;
		return 0;

	case SNAPPER_VOL_OUTPUT:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = sc->sc_vol_l;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = sc->sc_vol_r;
		return 0;

	case SNAPPER_INPUT_SELECT:
		mc->un.mask = sc->sc_record_source;
		return 0;

	case SNAPPER_VOL_INPUT:
		/* XXX TO BE DONE */
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = 0;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = 0;
		return 0;
	case SNAPPER_TREBLE:
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_treble;
		return 0;
	case SNAPPER_BASS:
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_bass;
		return 0;
	case SNAPPER_DIGI1:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = sc->mixer[0];
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = sc->mixer[3];
		return 0;
	case SNAPPER_DIGI2:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = sc->mixer[1];
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = sc->mixer[4];
		return 0;
	case SNAPPER_ANALOG:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = sc->mixer[2];
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = sc->mixer[5];
		return 0;
	default:
		return ENXIO;
	}

	return 0;
}

int
snapper_query_devinfo(void *h, mixer_devinfo_t *dip)
{
	switch (dip->index) {

	case SNAPPER_OUTPUT_SELECT:
		dip->mixer_class = SNAPPER_MONITOR_CLASS;
		strcpy(dip->label.name, AudioNoutput);
		dip->type = AUDIO_MIXER_SET;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.s.num_mem = 2;
		strcpy(dip->un.s.member[0].label.name, AudioNspeaker);
		dip->un.s.member[0].mask = 1 << 0;
		strcpy(dip->un.s.member[1].label.name, AudioNheadphone);
		dip->un.s.member[1].mask = 1 << 1;
		return 0;

	case SNAPPER_VOL_OUTPUT:
		dip->mixer_class = SNAPPER_MONITOR_CLASS;
		strcpy(dip->label.name, AudioNmaster);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case SNAPPER_INPUT_SELECT:
		dip->mixer_class = SNAPPER_RECORD_CLASS;
		strcpy(dip->label.name, AudioNsource);
		dip->type = AUDIO_MIXER_SET;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.s.num_mem = 2;
		strcpy(dip->un.s.member[0].label.name, AudioNmicrophone);
		dip->un.s.member[0].mask = 1 << 0;
		strcpy(dip->un.s.member[1].label.name, AudioNline);
		dip->un.s.member[1].mask = 1 << 1;
		return 0;

	case SNAPPER_VOL_INPUT:
		dip->mixer_class = SNAPPER_RECORD_CLASS;
		strcpy(dip->label.name, AudioNrecord);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case SNAPPER_MONITOR_CLASS:
		dip->mixer_class = SNAPPER_MONITOR_CLASS;
		strcpy(dip->label.name, AudioCmonitor);
		dip->type = AUDIO_MIXER_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		return 0;

	case SNAPPER_OUTPUT_CLASS:
		dip->mixer_class = SNAPPER_OUTPUT_CLASS;
		strcpy(dip->label.name, AudioCoutputs);
		dip->type = AUDIO_MIXER_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		return 0;

	case SNAPPER_RECORD_CLASS:
		dip->mixer_class = SNAPPER_RECORD_CLASS;
		strcpy(dip->label.name, AudioCrecord);
		dip->type = AUDIO_MIXER_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		return 0;

	case SNAPPER_TREBLE:
		dip->mixer_class = SNAPPER_MONITOR_CLASS;
		strcpy(dip->label.name, AudioNtreble);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 1;
		return 0;

	case SNAPPER_BASS:
		dip->mixer_class = SNAPPER_MONITOR_CLASS;
		strcpy(dip->label.name, AudioNbass);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 1;
		return 0;

	case SNAPPER_DIGI1:
		dip->mixer_class = SNAPPER_MONITOR_CLASS;
		strcpy(dip->label.name, AudioNdac);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 2;
		return 0;
	case SNAPPER_DIGI2:
		dip->mixer_class = SNAPPER_MONITOR_CLASS;
		strcpy(dip->label.name, AudioNline);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 2;
		return 0;
	case SNAPPER_ANALOG:
		dip->mixer_class = SNAPPER_MONITOR_CLASS;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 2;
		return 0;
	}

	return ENXIO;
}

size_t
snapper_round_buffersize(void *h, int dir, size_t size)
{

	if (size > 65536)
		size = 65536;
	return size;
}

paddr_t
snapper_mappage(void *h, void *mem, off_t off, int prot)
{

	if (off < 0)
		return -1;
	return -1;	/* XXX */
}

int
snapper_get_props(void *h)
{
	return AUDIO_PROP_FULLDUPLEX /* | AUDIO_PROP_MMAP */;
}

int
snapper_trigger_output(void *h, void *start, void *end, int bsize,
		       void (*intr)(void *), void *arg,
		       const audio_params_t *param)
{
	struct snapper_softc *sc;
	struct dbdma_command *cmd;
	vaddr_t va;
	int i, len, intmode;
	int res;

	DPRINTF("trigger_output %p %p 0x%x\n", start, end, bsize);
	sc = h;

	if ((res = snapper_set_rate(sc)) != 0)
		return res;

	cmd = sc->sc_odmacmd;
	sc->sc_ointr = intr;
	sc->sc_oarg = arg;
	sc->sc_opages = ((char *)end - (char *)start) / NBPG;

#ifdef DIAGNOSTIC
	if (sc->sc_opages > 16)
		panic("snapper_trigger_output");
#endif

	va = (vaddr_t)start;
	len = 0;
	for (i = sc->sc_opages; i > 0; i--) {
		len += NBPG;
		if (len < bsize)
			intmode = 0;
		else {
			len = 0;
			intmode = DBDMA_INT_ALWAYS;
		}

		DBDMA_BUILD(cmd, DBDMA_CMD_OUT_MORE, 0, NBPG, vtophys(va),
		    intmode, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
		cmd++;
		va += NBPG;
	}

	DBDMA_BUILD(cmd, DBDMA_CMD_NOP, 0, 0,
	    0/*vtophys((vaddr_t)sc->sc_odmacmd)*/, 0, DBDMA_WAIT_NEVER,
	    DBDMA_BRANCH_ALWAYS);

	dbdma_st32(&cmd->d_cmddep, vtophys((vaddr_t)sc->sc_odmacmd));

	dbdma_start(sc->sc_odma, sc->sc_odmacmd);

	return 0;
}

int
snapper_trigger_input(void *h, void *start, void *end, int bsize,
		      void (*intr)(void *), void *arg,
		      const audio_params_t *param)
{
	struct snapper_softc *sc;
	struct dbdma_command *cmd;
	vaddr_t va;
	int i, len, intmode;
	int res;

	DPRINTF("trigger_input %p %p 0x%x\n", start, end, bsize);
	sc = h;

	if ((res = snapper_set_rate(sc)) != 0)
		return res;

	cmd = sc->sc_idmacmd;
	sc->sc_iintr = intr;
	sc->sc_iarg = arg;
	sc->sc_ipages = ((char *)end - (char *)start) / NBPG;

#ifdef DIAGNOSTIC
	if (sc->sc_ipages > 16)
		panic("snapper_trigger_input");
#endif

	va = (vaddr_t)start;
	len = 0;
	for (i = sc->sc_ipages; i > 0; i--) {
		len += NBPG;
		if (len < bsize)
			intmode = 0;
		else {
			len = 0;
			intmode = DBDMA_INT_ALWAYS;
		}

		DBDMA_BUILD(cmd, DBDMA_CMD_IN_MORE, 0, NBPG, vtophys(va),
		    intmode, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
		cmd++;
		va += NBPG;
	}

	DBDMA_BUILD(cmd, DBDMA_CMD_NOP, 0, 0,
	    0/*vtophys((vaddr_t)sc->sc_odmacmd)*/, 0, DBDMA_WAIT_NEVER,
	    DBDMA_BRANCH_ALWAYS);

	dbdma_st32(&cmd->d_cmddep, vtophys((vaddr_t)sc->sc_idmacmd));

	dbdma_start(sc->sc_idma, sc->sc_idmacmd);

	return 0;
}

void
snapper_set_volume(struct snapper_softc *sc, int left, int right)
{
	u_char regs[6];
	int l, r;
	
	/*
	 * for some insane reason the gain table for master volume and the
	 * mixer channels is almost identical - just shifted by 4 bits
	 * so we use the mixer_gain table and bit-twiddle it... 
	 */
	if ((left >= 0) && (left < 256) && (right >= 0) && (right < 256)) {
		l = 177 - (left * 177 / 255);
		regs[0] =  (snapper_mixer_gain[l][0] >> 4);
		regs[1] = ((snapper_mixer_gain[l][0] & 0x0f) << 4) | 
			   (snapper_mixer_gain[l][1] >> 4);
		regs[2] = ((snapper_mixer_gain[l][1] & 0x0f) << 4) | 
			   (snapper_mixer_gain[l][2] >> 4);

		r = 177 - (right * 177 / 255);
		regs[3] =  (snapper_mixer_gain[r][0] >> 4);
		regs[4] = ((snapper_mixer_gain[r][0] & 0x0f) << 4) | 
			   (snapper_mixer_gain[r][1] >> 4);
		regs[5] = ((snapper_mixer_gain[r][1] & 0x0f) << 4) | 
			   (snapper_mixer_gain[r][2] >> 4);

		tas3004_write(sc, DEQ_VOLUME, regs);
		
		sc->sc_vol_l = left;
		sc->sc_vol_r = right;

		DPRINTF("%d %02x %02x %02x : %d %02x %02x %02x\n", l, regs[0], 	
		    regs[1], regs[2], r, regs[3], regs[4], regs[5]);
	}
}

void snapper_set_treble(struct snapper_softc *sc, int stuff)
{
	uint8_t reg;
	if ((stuff >= 0) && (stuff <= 255) && (sc->sc_treble != stuff)) {
		reg = snapper_basstab[(stuff >> 3) + 2];
		sc->sc_treble = stuff;
		tas3004_write(sc, DEQ_TREBLE, &reg);
	}
}

void snapper_set_bass(struct snapper_softc *sc, int stuff)
{
	uint8_t reg;
	if ((stuff >= 0) && (stuff <= 255) && (stuff != sc->sc_bass)) {
		reg = snapper_basstab[(stuff >> 3) + 2];
		sc->sc_bass = stuff;
		tas3004_write(sc, DEQ_BASS, &reg);
	}
}

void snapper_write_mixers(struct snapper_softc *sc)
{
	uint8_t regs[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	int i;

	/* Left channel of SDIN1 */
	i = 177 - (sc->mixer[0] * 177 / 255);
	regs[0] = snapper_mixer_gain[i][0];
	regs[1] = snapper_mixer_gain[i][1];
	regs[2] = snapper_mixer_gain[i][2];

	/* Left channel of SDIN2 */
	i = 177 - (sc->mixer[1] * 177 / 255);
	regs[3] = snapper_mixer_gain[i][0];
	regs[4] = snapper_mixer_gain[i][1];
	regs[5] = snapper_mixer_gain[i][2];

	/* Left channel of analog input */
	i = 177 - (sc->mixer[2] * 177 / 255);
	regs[6] = snapper_mixer_gain[i][0];
	regs[7] = snapper_mixer_gain[i][1];
	regs[8] = snapper_mixer_gain[i][2];

	tas3004_write(sc, DEQ_MIXER_L, regs);

	/* Right channel of SDIN1 */
	i = 177 - (sc->mixer[3] * 177 / 255);
	regs[0] = snapper_mixer_gain[i][0];
	regs[1] = snapper_mixer_gain[i][1];
	regs[2] = snapper_mixer_gain[i][2];

	/* Right channel of SDIN2 */
	i = 177 - (sc->mixer[4] * 177 / 255);
	regs[3] = snapper_mixer_gain[i][0];
	regs[4] = snapper_mixer_gain[i][1];
	regs[5] = snapper_mixer_gain[i][2];

	/* Right channel of analog input */
	i = 177 - (sc->mixer[5] * 177 / 255);
	regs[6] = snapper_mixer_gain[i][0];
	regs[7] = snapper_mixer_gain[i][1];
	regs[8] = snapper_mixer_gain[i][2];

	tas3004_write(sc, DEQ_MIXER_R, regs);
}

#define CLKSRC_49MHz	0x80000000	/* Use 49152000Hz Osc. */
#define CLKSRC_45MHz	0x40000000	/* Use 45158400Hz Osc. */
#define CLKSRC_18MHz	0x00000000	/* Use 18432000Hz Osc. */
#define MCLK_DIV	0x1f000000	/* MCLK = SRC / DIV */
#define  MCLK_DIV1	0x14000000	/*  MCLK = SRC */
#define  MCLK_DIV3	0x13000000	/*  MCLK = SRC / 3 */
#define  MCLK_DIV5	0x12000000	/*  MCLK = SRC / 5 */
#define SCLK_DIV	0x00f00000	/* SCLK = MCLK / DIV */
#define  SCLK_DIV1	0x00800000
#define  SCLK_DIV3	0x00900000
#define SCLK_MASTER	0x00080000	/* Master mode */
#define SCLK_SLAVE	0x00000000	/* Slave mode */
#define SERIAL_FORMAT	0x00070000
#define  SERIAL_SONY	0x00000000
#define  SERIAL_64x	0x00010000
#define  SERIAL_32x	0x00020000
#define  SERIAL_DAV	0x00040000
#define  SERIAL_SILICON	0x00050000

/*
 * rate = fs = LRCLK
 * SCLK = 64*LRCLK (I2S)
 * MCLK = 256fs (typ. -- changeable)
 *
 * MCLK = clksrc / mdiv
 * SCLK = MCLK / sdiv
 * rate = SCLK / 64    ( = LRCLK = fs)
 */

int
snapper_set_rate(struct snapper_softc *sc)
{
	u_int reg = 0, x;
	u_int rate = sc->sc_rate;
	uint32_t wordsize, ows;
	int MCLK;
	int clksrc, mdiv, sdiv;
	int mclk_fs;
	int timo;
	uint8_t mcr1;

	switch (rate) {
	case 44100:
		clksrc = 45158400;		/* 45MHz */
		reg = CLKSRC_45MHz;
		mclk_fs = 256;
		break;

	case 32000:
	case 48000:
		clksrc = 49152000;		/* 49MHz */
		reg = CLKSRC_49MHz;
		mclk_fs = 256;
		break;

	default:
		DPRINTF("snapper_set_rate: invalid rate %u\n", rate);
		return EINVAL;
	}

	MCLK = rate * mclk_fs;
	mdiv = clksrc / MCLK;			/* 4 */
	sdiv = mclk_fs / 64;			/* 4 */

	switch (mdiv) {
	case 1:
		reg |= MCLK_DIV1;
		break;
	case 3:
		reg |= MCLK_DIV3;
		break;
	case 5:
		reg |= MCLK_DIV5;
		break;
	default:
		reg |= ((mdiv / 2 - 1) << 24) & 0x1f000000;
		break;
	}

	switch (sdiv) {
	case 1:
		reg |= SCLK_DIV1;
		break;
	case 3:
		reg |= SCLK_DIV3;
		break;
	default:
		reg |= ((sdiv / 2 - 1) << 20) & 0x00f00000;
		break;
	}

	reg |= SCLK_MASTER;	/* XXX master mode */

	reg |= SERIAL_64x;

	/* stereo input and output */
	
	DPRINTF("precision: %d\n", sc->sc_bitspersample);
	switch(sc->sc_bitspersample) {
		case 16:
			wordsize = 0x02000200;
			mcr1 = DEQ_MCR1_SC_64 | DEQ_MCR1_SM_I2S | DEQ_MCR1_W_16;
			break;
		case 24:
			wordsize = 0x03000300;
			mcr1 = DEQ_MCR1_SC_64 | DEQ_MCR1_SM_I2S | DEQ_MCR1_W_24;
			break;
		default:
			printf("%s: unsupported sample size %d\n",
			    sc->sc_dev.dv_xname, sc->sc_bitspersample);
			return EINVAL;
	}

	ows = in32rb(sc->sc_reg + I2S_WORDSIZE);
	DPRINTF("I2SSetDataWordSizeReg 0x%08x -> 0x%08x\n",
	    ows, wordsize);
	if (ows != wordsize) {
		out32rb(sc->sc_reg + I2S_WORDSIZE, wordsize);
		tas3004_write(sc, DEQ_MCR1, &mcr1);
	}

	x = in32rb(sc->sc_reg + I2S_FORMAT);
	if (x == reg)
		return 0;        /* No change; do nothing. */

	DPRINTF("I2SSetSerialFormatReg 0x%x -> 0x%x\n",
	    in32rb(sc->sc_reg + I2S_FORMAT), reg);

	/* Clear CLKSTOPPEND. */
	out32rb(sc->sc_reg + I2S_INT, I2S_INT_CLKSTOPPEND);

	x = in32rb(0x8000003c);                /* FCR */
	x &= ~I2S0CLKEN;                /* XXX I2S0 */
	out32rb(0x8000003c, x);

	/* Wait until clock is stopped. */
	for (timo = 1000; timo > 0; timo--) {
		if (in32rb(sc->sc_reg + I2S_INT) & I2S_INT_CLKSTOPPEND)
			goto done;
		delay(1);
	}
	DPRINTF("snapper_set_rate: timeout\n");
done:
	out32rb(sc->sc_reg + I2S_FORMAT, reg);

	x = in32rb(0x8000003c);
	x |= I2S0CLKEN;
	out32rb(0x8000003c, x);

	return 0;
}

const struct tas3004_reg tas3004_initdata = {
	{ DEQ_MCR1_SC_64 | DEQ_MCR1_SM_I2S | DEQ_MCR1_W_16 },	/* MCR1 */
	{ 1, 0, 0, 0, 0, 0 },					/* DRC */
	{ 0, 0, 0, 0, 0, 0 },					/* VOLUME */
	{ 0x72 },						/* TREBLE */
	{ 0x72 },						/* BASS */
	{ 0x10, 0x00, 0x00, 0, 0, 0, 0, 0, 0 },			/* MIXER_L */
	{ 0x10, 0x00, 0x00, 0, 0, 0, 0, 0, 0 },			/* MIXER_R */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0, 0, 0 },						/* LLB_GAIN */
	{ 0, 0, 0 },						/* RLB_GAIN */
	{ DEQ_ACR_ADM | DEQ_ACR_LRB | DEQ_ACR_INP_B },		/* ACR - right channel of input B is the microphone */
	{ 2 }							/* MCR2 - AllPass mode since we don't use the equalizer anyway */
};

const char tas3004_regsize[] = {
	0,					/* 0x00 */
	sizeof tas3004_initdata.MCR1,		/* 0x01 */
	sizeof tas3004_initdata.DRC,		/* 0x02 */
	0,					/* 0x03 */
	sizeof tas3004_initdata.VOLUME,		/* 0x04 */
	sizeof tas3004_initdata.TREBLE,		/* 0x05 */
	sizeof tas3004_initdata.BASS,		/* 0x06 */
	sizeof tas3004_initdata.MIXER_L,	/* 0x07 */
	sizeof tas3004_initdata.MIXER_R,	/* 0x08 */
	0,					/* 0x09 */
	sizeof tas3004_initdata.LB0,		/* 0x0a */
	sizeof tas3004_initdata.LB1,		/* 0x0b */
	sizeof tas3004_initdata.LB2,		/* 0x0c */
	sizeof tas3004_initdata.LB3,		/* 0x0d */
	sizeof tas3004_initdata.LB4,		/* 0x0e */
	sizeof tas3004_initdata.LB5,		/* 0x0f */
	sizeof tas3004_initdata.LB6,		/* 0x10 */
	0,					/* 0x11 */
	0,					/* 0x12 */
	sizeof tas3004_initdata.RB0,		/* 0x13 */
	sizeof tas3004_initdata.RB1,		/* 0x14 */
	sizeof tas3004_initdata.RB2,		/* 0x15 */
	sizeof tas3004_initdata.RB3,		/* 0x16 */
	sizeof tas3004_initdata.RB4,		/* 0x17 */
	sizeof tas3004_initdata.RB5,		/* 0x18 */
	sizeof tas3004_initdata.RB6,		/* 0x19 */
	0,0,0,0, 0,0,
	0,					/* 0x20 */
	sizeof tas3004_initdata.LLB,		/* 0x21 */
	sizeof tas3004_initdata.RLB,		/* 0x22 */
	sizeof tas3004_initdata.LLB_GAIN,	/* 0x23 */
	sizeof tas3004_initdata.RLB_GAIN,	/* 0x24 */
	0,0,0,0, 0,0,0,0, 0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	sizeof tas3004_initdata.ACR,		/* 0x40 */
	0,					/* 0x41 */
	0,					/* 0x42 */
	sizeof tas3004_initdata.MCR2		/* 0x43 */
};

int
tas3004_write(struct snapper_softc *sc, u_int reg, const void *data)
{
	int size;
	static char regblock[sizeof(struct tas3004_reg)+1];
		
	KASSERT(reg < sizeof tas3004_regsize);
	size = tas3004_regsize[reg];
	KASSERT(size > 0);

	DPRINTF("reg: %x, %d %d\n", reg, size, ((const char*)data)[0]);

	regblock[0] = reg;
	memcpy(&regblock[1], data, size);
	iic_acquire_bus(sc->sc_i2c, 0);
	iic_exec(sc->sc_i2c, I2C_OP_WRITE, sc->sc_deqaddr, regblock, size + 1,
	    NULL, 0, 0);
	iic_release_bus(sc->sc_i2c, 0);
	
	return 0;
}

int
gpio_read(char *addr)
{

	if (*addr & GPIO_DATA)
		return 1;
	return 0;
}

void
gpio_write(char *addr, int val)
{
	u_int data;

	data = GPIO_DDR_OUTPUT;
	if (val)
		data |= GPIO_DATA;
	*addr = data;
	__asm volatile ("eieio");
}

#define headphone_active 0	/* XXX OF */
#define amp_active 0		/* XXX OF */

void
snapper_mute_speaker(struct snapper_softc *sc, int mute)
{
	u_int x;

	DPRINTF("ampmute %d --> ", gpio_read(amp_mute));

	if (mute)
		x = amp_active;		/* mute */
	else
		x = !amp_active;	/* unmute */
	if (x != gpio_read(amp_mute))
		gpio_write(amp_mute, x);

	DPRINTF("%d\n", gpio_read(amp_mute));
}

void
snapper_mute_headphone(struct snapper_softc *sc, int mute)
{
	u_int x;

	DPRINTF("headphonemute %d --> ", gpio_read(headphone_mute));

	if (mute)
		x = headphone_active;	/* mute */
	else
		x = !headphone_active;	/* unmute */
	if (x != gpio_read(headphone_mute))
		gpio_write(headphone_mute, x);

	DPRINTF("%d\n", gpio_read(headphone_mute));
}

int
snapper_cint(void *v)
{
	struct snapper_softc *sc;
	u_int sense;

	sc = v;
	sense = *headphone_detect;
	DPRINTF("headphone detect = 0x%x\n", sense);

	if (((sense & 0x02) >> 1) == headphone_detect_active) {
		DPRINTF("headphone is inserted\n");
		snapper_mute_speaker(sc, 1);
		snapper_mute_headphone(sc, 0);
		sc->sc_output_mask = 1 << 1;
	} else {
		DPRINTF("headphone is NOT inserted\n");
		snapper_mute_speaker(sc, 0);
		snapper_mute_headphone(sc, 1);
		sc->sc_output_mask = 1 << 0;
	}

	return 1;
}

#define reset_active 0	/* XXX OF */

#define DEQ_WRITE(sc, reg, addr) \
	if (tas3004_write(sc, reg, addr)) goto err

int
tas3004_init(struct snapper_softc *sc)
{

	/* No reset port.  Nothing to do. */
	if (audio_hw_reset == NULL)
		goto noreset;

	/* Reset TAS3004. */
	gpio_write(audio_hw_reset, !reset_active);	/* Negate RESET */
	delay(100000);				/* XXX Really needed? */

	gpio_write(audio_hw_reset, reset_active);	/* Assert RESET */
	delay(1);

	gpio_write(audio_hw_reset, !reset_active);	/* Negate RESET */
	delay(10000);

noreset:
	DEQ_WRITE(sc, DEQ_LB0, tas3004_initdata.LB0);
	DEQ_WRITE(sc, DEQ_LB1, tas3004_initdata.LB1);
	DEQ_WRITE(sc, DEQ_LB2, tas3004_initdata.LB2);
	DEQ_WRITE(sc, DEQ_LB3, tas3004_initdata.LB3);
	DEQ_WRITE(sc, DEQ_LB4, tas3004_initdata.LB4);
	DEQ_WRITE(sc, DEQ_LB5, tas3004_initdata.LB5);
	DEQ_WRITE(sc, DEQ_LB6, tas3004_initdata.LB6);
	DEQ_WRITE(sc, DEQ_RB0, tas3004_initdata.RB0);
	DEQ_WRITE(sc, DEQ_RB1, tas3004_initdata.RB1);
	DEQ_WRITE(sc, DEQ_RB1, tas3004_initdata.RB1);
	DEQ_WRITE(sc, DEQ_RB2, tas3004_initdata.RB2);
	DEQ_WRITE(sc, DEQ_RB3, tas3004_initdata.RB3);
	DEQ_WRITE(sc, DEQ_RB4, tas3004_initdata.RB4);
	DEQ_WRITE(sc, DEQ_RB5, tas3004_initdata.RB5);
	DEQ_WRITE(sc, DEQ_MCR1, tas3004_initdata.MCR1);
	DEQ_WRITE(sc, DEQ_MCR2, tas3004_initdata.MCR2);
	DEQ_WRITE(sc, DEQ_DRC, tas3004_initdata.DRC);
	DEQ_WRITE(sc, DEQ_VOLUME, tas3004_initdata.VOLUME);
	DEQ_WRITE(sc, DEQ_TREBLE, tas3004_initdata.TREBLE);
	DEQ_WRITE(sc, DEQ_BASS, tas3004_initdata.BASS);
	DEQ_WRITE(sc, DEQ_MIXER_L, tas3004_initdata.MIXER_L);
	DEQ_WRITE(sc, DEQ_MIXER_R, tas3004_initdata.MIXER_R);
	DEQ_WRITE(sc, DEQ_LLB, tas3004_initdata.LLB);
	DEQ_WRITE(sc, DEQ_RLB, tas3004_initdata.RLB);
	DEQ_WRITE(sc, DEQ_LLB_GAIN, tas3004_initdata.LLB_GAIN);
	DEQ_WRITE(sc, DEQ_RLB_GAIN, tas3004_initdata.RLB_GAIN);
	DEQ_WRITE(sc, DEQ_ACR, tas3004_initdata.ACR);

	return 0;
err:
	printf("tas3004_init: error\n");
	return -1;
}

void
snapper_init(struct snapper_softc *sc, int node)
{
	int gpio;
	int headphone_detect_intr, headphone_detect_intrtype;
#ifdef SNAPPER_DEBUG
	char fcr[32];

	bitmask_snprintf(in32rb(0x8000003c), FCR3C_BITMASK, fcr, sizeof fcr);
	printf("FCR(0x3c) 0x%s\n", fcr);
#endif
	headphone_detect_intr = -1;

	gpio = getnodebyname(OF_parent(node), "gpio");
	DPRINTF(" /gpio 0x%x\n", gpio);
	gpio = OF_child(gpio);
	while (gpio) {
		char name[64], audio_gpio[64];
		int intr[2];
		char *addr;

		bzero(name, sizeof name);
		bzero(audio_gpio, sizeof audio_gpio);
		addr = 0;
		OF_getprop(gpio, "name", name, sizeof name);
		OF_getprop(gpio, "audio-gpio", audio_gpio, sizeof audio_gpio);
		OF_getprop(gpio, "AAPL,address", &addr, sizeof addr);
		DPRINTF(" 0x%x %s %s\n", gpio, name, audio_gpio);

		/* gpio5 */
		if (strcmp(audio_gpio, "headphone-mute") == 0)
			headphone_mute = addr;
		/* gpio6 */
		if (strcmp(audio_gpio, "amp-mute") == 0)
			amp_mute = addr;
		/* extint-gpio15 */
		if (strcmp(audio_gpio, "headphone-detect") == 0) {
			headphone_detect = addr;
			OF_getprop(gpio, "audio-gpio-active-state",
			    &headphone_detect_active, 4);
			OF_getprop(gpio, "interrupts", intr, 8);
			headphone_detect_intr = intr[0];
			headphone_detect_intrtype = intr[1];
		}
		/* gpio11 (keywest-11) */
		if (strcmp(audio_gpio, "audio-hw-reset") == 0)
			audio_hw_reset = addr;
		gpio = OF_peer(gpio);
	}
	DPRINTF(" headphone-mute %p\n", headphone_mute);
	DPRINTF(" amp-mute %p\n", amp_mute);
	DPRINTF(" headphone-detect %p\n", headphone_detect);
	DPRINTF(" headphone-detect active %x\n", headphone_detect_active);
	DPRINTF(" headphone-detect intr %x\n", headphone_detect_intr);
	DPRINTF(" audio-hw-reset %p\n", audio_hw_reset);

	if (headphone_detect_intr != -1)
		intr_establish(headphone_detect_intr, IST_EDGE, IPL_AUDIO,
		    snapper_cint, sc);

	sc->sc_rate = 44100;	/* default rate */
	sc->sc_bitspersample = 16;

	/* Enable headphone interrupt? */
	*headphone_detect |= 0x80;
	__asm volatile ("eieio");

	/* i2c_set_port(port); */

	if (tas3004_init(sc))
		return;

	/* Update headphone status. */
	snapper_cint(sc);

	snapper_set_volume(sc, 128, 128);
	
	sc->sc_bass = 128;
	sc->sc_treble = 128;

	/* Record source defaults to microphone.  This reflects the
	 * default value for the ACR (see tas3004_initdata).
	 */
	sc->sc_record_source = 1 << 0;
	
	/* We mute the analog input for now */
	sc->mixer[0] = 80;
	sc->mixer[1] = 80;
	sc->mixer[2] = 0;
	sc->mixer[3] = 80;
	sc->mixer[4] = 80;
	sc->mixer[5] = 0;
	snapper_write_mixers(sc);
}
