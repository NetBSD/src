/*	$NetBSD: snapper.c,v 1.8.6.1 2006/04/22 11:37:41 simonb Exp $	*/
/*	Id: snapper.c,v 1.11 2002/10/31 17:42:13 tsubai Exp	*/

/*-
 * Copyright (c) 2002 Tsubai Masanari.  All rights reserved.
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

	u_int sc_record_source;		/* recording source mask */
	u_int sc_output_mask;		/* output source mask */

	u_char *sc_reg;
	i2c_addr_t sc_deqaddr;
	i2c_tag_t sc_i2c;

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
int snapper_set_rate(struct snapper_softc *, u_int);
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

#define SNAPPER_NFORMATS	1
static const struct audio_format snapper_formats[SNAPPER_NFORMATS] = {
	{NULL, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 2, AUFMT_STEREO, 3, {8000, 44100, 48000}},
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
	int soundbus, soundchip;
	char compat[32];

	ca = aux;
	if (strcmp(ca->ca_name, "i2s") != 0)
		return 0;

	if ((soundbus = OF_child(ca->ca_node)) == 0 ||
	    (soundchip = OF_child(soundbus)) == 0)
		return 0;

	bzero(compat, sizeof compat);
	OF_getprop(soundchip, "compatible", compat, sizeof compat);

	if (strcmp(compat, "snapper") != 0)
		return 0;

	return 1;
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
	/* intr_establish(iirq, iirq_type, IPL_AUDIO, snapper_intr, sc); */

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
		if (p->sample_rate < 4000 || p->sample_rate > 50000)
			return EINVAL;

		fil = mode == AUMODE_PLAY ? pfil : rfil;
		if (auconv_set_converter(snapper_formats, SNAPPER_NFORMATS,
					 mode, p, TRUE, fil) < 0)
			return EINVAL;
		if (fil->req_size > 0)
			p = &fil->filters[0].param;
	}

	/* Set the speed. p points HW encoding. */
	if (snapper_set_rate(sc, p->sample_rate))
		return EINVAL;

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
		case 1 << 0: /* CD */
		case 1 << 1: /* microphone */
		case 1 << 2: /* line in */
			/* XXX TO BE DONE */
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
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO]=sc->sc_treble;
		return 0;
	case SNAPPER_BASS:
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO]=sc->sc_bass;
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
		dip->un.s.num_mem = 3;
		strcpy(dip->un.s.member[0].label.name, AudioNcd);
		dip->un.s.member[0].mask = 1 << 0;
		strcpy(dip->un.s.member[1].label.name, AudioNmicrophone);
		dip->un.s.member[1].mask = 1 << 1;
		strcpy(dip->un.s.member[2].label.name, AudioNline);
		dip->un.s.member[2].mask = 1 << 2;
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
		strcpy(dip->label.name, "Digi2");
		dip->type = AUDIO_MIXER_VALUE;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		dip->un.v.num_channels = 2;
		return 0;
	case SNAPPER_ANALOG:
		dip->mixer_class = SNAPPER_MONITOR_CLASS;
		strcpy(dip->label.name, "Analog");
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

	DPRINTF("trigger_output %p %p 0x%x\n", start, end, bsize);
	sc = h;
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

	printf("snapper_trigger_input called\n");
	return 1;
}

void
snapper_set_volume(struct snapper_softc *sc, int left, int right)
{
	u_char vol[6];

	sc->sc_vol_l = left;
	sc->sc_vol_r = right;

#if 0
	left <<= 8;	/* XXX for now */
	right <<= 8;

	vol[0] = left >> 16;
	vol[1] = left >> 8;
	vol[2] = left;
	vol[3] = right >> 16;
	vol[4] = right >> 8;
	vol[5] = right;
#else
	/* 0x07ffff is LOUD, 0x000000 is mute */
	vol[0]=0/*(left>>5)&0xff*/;		/* upper 3 bits */
	vol[1]=(left/*<<3*/)&0xff;	/* lower 5 bits */
	vol[2]=0;
	vol[3]=0/*(right>>5)&0xff*/;		/* upper 3 bits */
	vol[4]=(right/*<<3*/)&0xff;	/* lower 5 bits */
	vol[5]=0;
#endif
	tas3004_write(sc, DEQ_VOLUME, vol);
}

void snapper_set_treble(struct snapper_softc *sc, int stuff)
{
	uint8_t reg;
	if((stuff>=0) && (stuff<=255) && (sc->sc_treble!=stuff)) {
		reg=snapper_basstab[(stuff>>3)+2];
		sc->sc_treble=stuff;
		tas3004_write(sc, DEQ_TREBLE,&reg);
	}
}

void snapper_set_bass(struct snapper_softc *sc, int stuff)
{
	uint8_t reg;
	if((stuff>=0) && (stuff<=255) && (stuff!=sc->sc_bass)) {
		reg=snapper_basstab[(stuff>>3)+2];
		sc->sc_bass=stuff;
		tas3004_write(sc, DEQ_BASS,&reg);
	}
}

void snapper_write_mixers(struct snapper_softc *sc)
{
	uint8_t regs[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	regs[0] = (sc->mixer[0] >> 4) & 0xff;
	regs[1] = (sc->mixer[0] << 4) & 0xff;
	regs[3] = (sc->mixer[1] >> 4) & 0xff;
	regs[4] = (sc->mixer[1] << 4) & 0xff;
	regs[5] = (sc->mixer[2] >> 4) & 0xff;
	regs[7] = (sc->mixer[2] << 4) & 0xff;
	tas3004_write(sc, DEQ_MIXER_L, regs);
	regs[0] = (sc->mixer[3] >> 4) & 0xff;
	regs[1] = (sc->mixer[3] << 4) & 0xff;
	regs[3] = (sc->mixer[4] >> 4) & 0xff;
	regs[4] = (sc->mixer[4] << 4) & 0xff;
	regs[5] = (sc->mixer[5] >> 4) & 0xff;
	regs[7] = (sc->mixer[5] << 4) & 0xff;
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

// rate = fs = LRCLK
// SCLK = 64*LRCLK (I2S)
// MCLK = 256fs (typ. -- changeable)

// MCLK = clksrc / mdiv
// SCLK = MCLK / sdiv
// rate = SCLK / 64    ( = LRCLK = fs)

int
snapper_set_rate(struct snapper_softc *sc, u_int rate)
{
	u_int reg;
	int MCLK;
	int clksrc, mdiv, sdiv;
	int mclk_fs;

	reg = 0;
	switch (rate) {
	case 8000:
		clksrc = 18432000;		/* 18MHz */
		reg = CLKSRC_18MHz;
		mclk_fs = 256;
		break;

	case 44100:
		clksrc = 45158400;		/* 45MHz */
		reg = CLKSRC_45MHz;
		mclk_fs = 256;
		break;

	case 48000:
		clksrc = 49152000;		/* 49MHz */
		reg = CLKSRC_49MHz;
		mclk_fs = 256;
		break;

	default:
		return EINVAL;
	}

	MCLK = rate * mclk_fs;
	mdiv = clksrc / MCLK;			// 4
	sdiv = mclk_fs / 64;			// 4

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
	DPRINTF("I2SSetDataWordSizeReg 0x%08x -> 0x%08x\n",
	    in32rb(sc->sc_reg + I2S_WORDSIZE), 0x02000200);
	out32rb(sc->sc_reg + I2S_WORDSIZE, 0x02000200);

	DPRINTF("I2SSetSerialFormatReg 0x%x -> 0x%x\n",
	    in32rb(sc->sc_reg + I2S_FORMAT), reg);
	out32rb(sc->sc_reg + I2S_FORMAT, reg);

	return 0;
}

/*#define DEQaddr 0x6a*/

const struct tas3004_reg tas3004_initdata = {
	{ DEQ_MCR1_SC_64 | DEQ_MCR1_SM_I2S | DEQ_MCR1_W_20 },	/* MCR1 */
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
	{ 0xc0 },							/* ACR - right channel of input B is the microphone */
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

#ifdef DEBUG_SNAPPER
	printf("reg: %x, %d %d\n",reg,size,((char*)data)[0]);
#endif
#if 0
	ki2c_setmode(sc->sc_i2c, 8); /* std+sub mode */
	
	if (ki2c_write(sc->sc_i2c, DEQaddr, reg, data, size))
		return -1;
#endif
	/* ugly, but for now... */
	regblock[0] = reg;
	memcpy(&regblock[1], data, size);
	iic_acquire_bus(sc->sc_i2c, 0);
	iic_exec(sc->sc_i2c, I2C_OP_WRITE, sc->sc_deqaddr, &regblock, size + 1,
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

/* FCR(0x3c) bits */
#define I2S0CLKEN	0x1000
#define I2S0EN		0x2000
#define I2S1CLKEN	0x080000
#define I2S1EN		0x100000

#define FCR3C_BITMASK "\020\25I2S1EN\24I2S1CLKEN\16I2S0EN\15I2S0CLKEN"

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
		/* printf("0x%x %s %s\n", gpio, name, audio_gpio); */

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

	/* "sample-rates" (44100, 48000) */
	snapper_set_rate(sc, 44100);

	/* Enable headphone interrupt? */
	*headphone_detect |= 0x80;
	__asm volatile ("eieio");

	/* i2c_set_port(port); */

#if 0
	/* Enable I2C interrupts. */
#define IER 4
#define I2C_INT_DATA 0x01
#define I2C_INT_ADDR 0x02
#define I2C_INT_STOP 0x04
	ki2c_writereg(sc->sc_i2c, IER,I2C_INT_DATA|I2C_INT_ADDR|I2C_INT_STOP);
#endif

	if (tas3004_init(sc))
		return;

	/* Update headphone status. */
	snapper_cint(sc);

	snapper_set_volume(sc, 80, 80);
	
	sc->sc_bass = 128;
	sc->sc_treble = 128;
	
	/* We mute the analog input for now */
	sc->mixer[0] = 80;
	sc->mixer[1] = 80;
	sc->mixer[2] = 0;
	sc->mixer[3] = 80;
	sc->mixer[4] = 80;
	sc->mixer[5] = 0;
	snapper_write_mixers(sc);
}
