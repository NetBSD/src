/*	$NetBSD: neo.c,v 1.9.2.1 2001/10/11 00:02:11 fvdl Exp $	*/

/*
 * Copyright (c) 1999 Cameron Grant <gandalf@vilnya.demon.co.uk>
 * All rights reserved.
 *
 * Derived from the public domain Linux driver
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHERIN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THEPOSSIBILITY OF
 * SUCH DAMAGE.
 *
 * FreeBSD: src/sys/dev/sound/pci/neomagic.c,v 1.8 2000/03/20 15:30:50 cg Exp
 * OpenBSD: neo.c,v 1.4 2000/07/19 09:04:37 csapuntz Exp
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/neoreg.h>
#include <dev/pci/neo-coeff.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>

#include <dev/ic/ac97var.h>


/* -------------------------------------------------------------------- */
/* 
 * As of 04/13/00, public documentation on the Neomagic 256 is not available.
 * These comments were gleaned by looking at the driver carefully.
 *
 * The Neomagic 256 AV/ZX chips provide both video and audio capabilities
 * on one chip. About 2-6 megabytes of memory are associated with
 * the chip. Most of this goes to video frame buffers, but some is used for
 * audio buffering
 *
 * Unlike most PCI audio chips, the Neomagic chip does not rely on DMA.
 * Instead, the chip allows you to carve out two ring buffers out of its
 * memory. However you carve this and how much you can carve seems to be
 * voodoo. The algorithm is in nm_init.
 *
 * Most Neomagic audio chips use the AC-97 codec interface. However, there 
 * seem to be a select few chips 256AV chips that do not support AC-97.
 * This driver does not support them but there are rumors that it
 * mgiht work with wss isa drivers. This might require some playing around
 * with your BIOS.
 *
 * The Neomagic 256 AV/ZX have 2 PCI I/O region descriptors. Both of
 * them describe a memory region. The frame buffer is the first region
 * and the register set is the secodn region.
 *
 * The register manipulation logic is taken from the Linux driver,
 * which is in the public domain.
 *
 * The Neomagic is even nice enough to map the AC-97 codec registers into
 * the register space to allow direct manipulation. Watch out, accessing
 * AC-97 registers on the Neomagic requires great delicateness, otherwise
 * the thing will hang the PCI bus, rendering your system frozen.
 *
 * For one, it seems the Neomagic status register that reports AC-97
 * readiness should NOT be polled more often than once each 1ms.
 *
 * Also, writes to the AC-97 register space may take order 40us to
 * complete.
 *
 * Unlike many sound engines, the Neomagic does not support (as fas as
 * we know :) the notion of interrupting every n bytes transferred,
 * unlike many DMA engines.  Instead, it allows you to specify one
 * location in each ring buffer (called the watermark). When the chip
 * passes that location while playing, it signals an interrupt.
 * 
 * The ring buffer size is currently 16k. That is about 100ms of audio
 * at 44.1khz/stero/16 bit. However, to keep the buffer full, interrupts
 * are generated more often than that, so 20-40 interrupts per second
 * should not be unexpected. Increasing BUFFSIZE should help minimize
 * of glitches due to drivers that spend to much time looping at high
 * privelege levels as well as the impact of badly written audio
 * interface clients.
 *
 * TO-DO list:
 *    Figure out interaction with video stuff (look at Xfree86 driver?)
 *
 *    Figure out how to shrink that huge table neo-coeff.h
 */

#define	NM_BUFFSIZE	16384

/* device private data */
struct neo_softc {
	struct          device dev;

	bus_space_tag_t bufiot;
	bus_space_handle_t  bufioh;

	bus_space_tag_t regiot;
	bus_space_handle_t  regioh;

	u_int32_t 	type;
	void            *ih;

	void	(*pintr)(void *);	/* dma completion intr handler */
	void	*parg;		/* arg for intr() */

	void	(*rintr)(void *);	/* dma completion intr handler */
	void	*rarg;		/* arg for intr() */

	vaddr_t	buf_vaddr;
	vaddr_t	rbuf_vaddr;
	vaddr_t	pbuf_vaddr;
	int	pbuf_allocated;
	int	rbuf_allocated;

	bus_addr_t buf_pciaddr;
	bus_addr_t rbuf_pciaddr;
	bus_addr_t pbuf_pciaddr;

	u_int32_t 	ac97_base, ac97_status, ac97_busy;
	u_int32_t	buftop, pbuf, rbuf, cbuf, acbuf;
	u_int32_t	playint, recint, misc1int, misc2int;
	u_int32_t	irsz, badintr;

        u_int32_t       pbufsize;
        u_int32_t       rbufsize;

	u_int32_t       pblksize;
	u_int32_t       rblksize;

        u_int32_t       pwmark;
        u_int32_t       rwmark;

	struct ac97_codec_if *codec_if;
	struct ac97_host_if host_if;

	void		*powerhook;
};

/* -------------------------------------------------------------------- */

/*
 * prototypes
 */

static int	nm_waitcd(struct neo_softc *sc);
static int	nm_loadcoeff(struct neo_softc *sc, int dir, int num);
static int	nm_init(struct neo_softc *);

int	neo_match(struct device *, struct cfdata *, void *);
void	neo_attach(struct device *, struct device *, void *);
int	neo_intr(void *);

int	neo_open(void *, int);
void	neo_close(void *);
int	neo_query_encoding(void *, struct audio_encoding *);
int	neo_set_params(void *, int, int, struct audio_params *,
	    struct audio_params *);
int	neo_round_blocksize(void *, int);
int	neo_trigger_output(void *, void *, void *, int, void (*)(void *),
	    void *, struct audio_params *);
int	neo_trigger_input(void *, void *, void *, int, void (*)(void *),
	    void *, struct audio_params *);
int	neo_halt_output(void *);
int	neo_halt_input(void *);
int	neo_getdev(void *, struct audio_device *);
int	neo_mixer_set_port(void *, mixer_ctrl_t *);
int	neo_mixer_get_port(void *, mixer_ctrl_t *);
int     neo_attach_codec(void *sc, struct ac97_codec_if *);
int	neo_read_codec(void *sc, u_int8_t a, u_int16_t *d);
int	neo_write_codec(void *sc, u_int8_t a, u_int16_t d);
void    neo_reset_codec(void *sc);
enum ac97_host_flags neo_flags_codec(void *sc);
int	neo_query_devinfo(void *, mixer_devinfo_t *);
void   *neo_malloc(void *, int, size_t, int, int);
void	neo_free(void *, void *, int);
size_t	neo_round_buffersize(void *, int, size_t);
paddr_t	neo_mappage(void *, void *, off_t, int);
int	neo_get_props(void *);
void	neo_set_mixer(struct neo_softc *sc, int a, int d);
void	neo_power(int why, void *arg);

struct cfattach neo_ca = {
	sizeof(struct neo_softc), neo_match, neo_attach
};

struct audio_device neo_device = {
	"NeoMagic 256",
	"",
	"neo"
};

/* The actual rates supported by the card. */
static const int samplerates[9] = {
	8000,
	11025,
	16000,
	22050,
	24000,
	32000,
	44100,
	48000,
	99999999
};

/* -------------------------------------------------------------------- */

struct audio_hw_if neo_hw_if = {
	neo_open,
	neo_close,
	NULL,				/* drain */
	neo_query_encoding,
	neo_set_params,
	neo_round_blocksize,
	NULL,				/* commit_setting */
	NULL,				/* init_output */
	NULL,				/* init_input */
	NULL,				/* start_output */
	NULL,				/* start_input */
	neo_halt_output,
	neo_halt_input,
	NULL,				/* speaker_ctl */
	neo_getdev,
	NULL,				/* getfd */
	neo_mixer_set_port,
	neo_mixer_get_port,
	neo_query_devinfo,
	neo_malloc,
	neo_free,
	neo_round_buffersize,
	neo_mappage,
	neo_get_props,
	neo_trigger_output,
	neo_trigger_input,
	NULL,
};

/* -------------------------------------------------------------------- */

#define	nm_rd_1(sc, regno)						\
	bus_space_read_1((sc)->regiot, (sc)->regioh, (regno))

#define	nm_rd_2(sc, regno)						\
	bus_space_read_2((sc)->regiot, (sc)->regioh, (regno))

#define	nm_rd_4(sc, regno)						\
	bus_space_read_4((sc)->regiot, (sc)->regioh, (regno))

#define	nm_wr_1(sc, regno, val)						\
	bus_space_write_1((sc)->regiot, (sc)->regioh, (regno), (val))

#define	nm_wr_2(sc, regno, val)						\
	bus_space_write_2((sc)->regiot, (sc)->regioh, (regno), (val))

#define	nm_wr_4(sc, regno, val)						\
	bus_space_write_4((sc)->regiot, (sc)->regioh, (regno), (val))

#define	nm_rdbuf_4(sc, regno)						\
	bus_space_read_4((sc)->bufiot, (sc)->bufioh, (regno))

#define	nm_wrbuf_1(sc, regno, val)					\
	bus_space_write_1((sc)->bufiot, (sc)->bufioh, (regno), (val))

/* ac97 codec */
static int
nm_waitcd(struct neo_softc *sc)
{
	int cnt = 10;
	int fail = 1;

	while (cnt-- > 0) {
		if (nm_rd_2(sc, sc->ac97_status) & sc->ac97_busy)
			DELAY(100);
		else {
		        fail = 0;
			break;
		}
	}
	return (fail);
}


static void
nm_ackint(struct neo_softc *sc, u_int32_t num)
{

	switch (sc->type) {
	case PCI_PRODUCT_NEOMAGIC_NMMM256AV_AU:
		nm_wr_2(sc, NM_INT_REG, num << 1);
		break;

	case PCI_PRODUCT_NEOMAGIC_NMMM256ZX_AU:
		nm_wr_4(sc, NM_INT_REG, num);
		break;
	}
}

static int
nm_loadcoeff(struct neo_softc *sc, int dir, int num)
{
	int ofs, sz, i;
	u_int32_t addr;

	addr = (dir == AUMODE_PLAY)? 0x01c : 0x21c;
	if (dir == AUMODE_RECORD)
		num += 8;
	sz = coefficientSizes[num];
	ofs = 0;
	while (num-- > 0)
		ofs+= coefficientSizes[num];
	for (i = 0; i < sz; i++)
		nm_wrbuf_1(sc, sc->cbuf + i, coefficients[ofs + i]);
	nm_wr_4(sc, addr, sc->cbuf);
	if (dir == AUMODE_PLAY)
		sz--;
	nm_wr_4(sc, addr + 4, sc->cbuf + sz);
	return 0;
}

/* The interrupt handler */
int
neo_intr(void *p)
{
	struct neo_softc *sc = (struct neo_softc *)p;
	int status, x, active;
	int rv = 0;

	active = (sc->pintr || sc->rintr);
	status = (sc->irsz == 2) ?
	    nm_rd_2(sc, NM_INT_REG) :
	    nm_rd_4(sc, NM_INT_REG);

	if (status & sc->playint) {
		status &= ~sc->playint;

		sc->pwmark += sc->pblksize;
		sc->pwmark %= sc->pbufsize;

		nm_wr_4(sc, NM_PBUFFER_WMARK, sc->pbuf + sc->pwmark);

		nm_ackint(sc, sc->playint);

		if (sc->pintr)
			(*sc->pintr)(sc->parg);

		rv = 1;
	}
	if (status & sc->recint) {
		status &= ~sc->recint;

		sc->rwmark += sc->rblksize;
		sc->rwmark %= sc->rbufsize;

		nm_ackint(sc, sc->recint);
		if (sc->rintr)
			(*sc->rintr)(sc->rarg);

		rv = 1;
	}
	if (status & sc->misc1int) {
		status &= ~sc->misc1int;
		nm_ackint(sc, sc->misc1int);
		x = nm_rd_1(sc, 0x400);
		nm_wr_1(sc, 0x400, x | 2);
		printf("%s: misc int 1\n", sc->dev.dv_xname);
		rv = 1;
	}
	if (status & sc->misc2int) {
		status &= ~sc->misc2int;
		nm_ackint(sc, sc->misc2int);
		x = nm_rd_1(sc, 0x400);
		nm_wr_1(sc, 0x400, x & ~2);
		printf("%s: misc int 2\n", sc->dev.dv_xname);
		rv = 1;
	}
	if (status) {
		status &= ~sc->misc2int;
		nm_ackint(sc, sc->misc2int);
		printf("%s: unknown int\n", sc->dev.dv_xname);
		rv = 1;
	}

	return (rv);
}

/* -------------------------------------------------------------------- */

/*
 * Probe and attach the card
 */

static int
nm_init(struct neo_softc *sc)
{
	u_int32_t ofs, i;

	switch (sc->type) {
	case PCI_PRODUCT_NEOMAGIC_NMMM256AV_AU:
		sc->ac97_base = NM_MIXER_OFFSET;
		sc->ac97_status = NM_MIXER_STATUS_OFFSET;
		sc->ac97_busy = NM_MIXER_READY_MASK;

		sc->buftop = 2560 * 1024;

		sc->irsz = 2;
		sc->playint = NM_PLAYBACK_INT;
		sc->recint = NM_RECORD_INT;
		sc->misc1int = NM_MISC_INT_1;
		sc->misc2int = NM_MISC_INT_2;
		break;

	case PCI_PRODUCT_NEOMAGIC_NMMM256ZX_AU:
		sc->ac97_base = NM_MIXER_OFFSET;
		sc->ac97_status = NM2_MIXER_STATUS_OFFSET;
		sc->ac97_busy = NM2_MIXER_READY_MASK;

		sc->buftop = (nm_rd_2(sc, 0xa0b) ? 6144 : 4096) * 1024;

		sc->irsz = 4;
		sc->playint = NM2_PLAYBACK_INT;
		sc->recint = NM2_RECORD_INT;
		sc->misc1int = NM2_MISC_INT_1;
		sc->misc2int = NM2_MISC_INT_2;
		break;
#ifdef DIAGNOSTIC
	default:
		panic("nm_init: impossible");
#endif
	}

	sc->badintr = 0;
	ofs = sc->buftop - 0x0400;
	sc->buftop -= 0x1400;

 	if ((nm_rdbuf_4(sc, ofs) & NM_SIG_MASK) == NM_SIGNATURE) {
		i = nm_rdbuf_4(sc, ofs + 4);
		if (i != 0 && i != 0xffffffff)
			sc->buftop = i;
	}

	sc->cbuf = sc->buftop - NM_MAX_COEFFICIENT;
	sc->rbuf = sc->cbuf - NM_BUFFSIZE;
	sc->pbuf = sc->rbuf - NM_BUFFSIZE;
	sc->acbuf = sc->pbuf - (NM_TOTAL_COEFF_COUNT * 4);

	sc->buf_vaddr = (vaddr_t) bus_space_vaddr(sc->bufiot, sc->bufioh);
	sc->rbuf_vaddr = sc->buf_vaddr + sc->rbuf;
	sc->pbuf_vaddr = sc->buf_vaddr + sc->pbuf;

	sc->rbuf_pciaddr = sc->buf_pciaddr + sc->rbuf;
	sc->pbuf_pciaddr = sc->buf_pciaddr + sc->pbuf;

	nm_wr_1(sc, 0, 0x11);
	nm_wr_1(sc, NM_RECORD_ENABLE_REG, 0);
	nm_wr_2(sc, 0x214, 0);

	return 0;
}

int
neo_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;
	pcireg_t subdev;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_NEOMAGIC)
		return (0);

	subdev = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_NEOMAGIC_NMMM256AV_AU:
		/*
		 * We have to weed-out the non-AC'97 versions of
		 * the chip (i.e. the ones that are known to work
		 * in WSS emulation mode), as they won't work with
		 * this driver.
		 */
		switch (PCI_VENDOR(subdev)) {
		case PCI_VENDOR_DELL:
			switch (PCI_PRODUCT(subdev)) {
			case 0x008f:
				return (0);
			}
			break;

		case PCI_VENDOR_HP:
			switch (PCI_PRODUCT(subdev)) {
			case 0x0007:
				return (0);
			}
			break;

		case PCI_VENDOR_IBM:
			switch (PCI_PRODUCT(subdev)) {
			case 0x00dd:
				return (0);
			}
			break;
		}
		return (1);

	case PCI_PRODUCT_NEOMAGIC_NMMM256ZX_AU:
		return (1);
	}

	return (0);
}

void
neo_power(int why, void *addr)
{
	struct neo_softc *sc = (struct neo_softc *)addr;

	if (why == PWR_RESUME) {
		nm_init(sc);
		(sc->codec_if->vtbl->restore_ports)(sc->codec_if);
	}
}

void
neo_attach(struct device *parent, struct device *self, void *aux)
{
	struct neo_softc *sc = (struct neo_softc *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	char const *intrstr;
	pci_intr_handle_t ih;
	pcireg_t csr;
	int error;

	sc->type = PCI_PRODUCT(pa->pa_id);

	printf(": NeoMagic 256%s audio\n",
	    sc->type == PCI_PRODUCT_NEOMAGIC_NMMM256AV_AU ? "AV" : "ZX");

	/* Map I/O register */
	if (pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_MEM, 0,
			   &sc->bufiot, &sc->bufioh, &sc->buf_pciaddr, NULL)) {
		printf("%s: can't map buffer\n", sc->dev.dv_xname);
		return;
	}

	if (pci_mapreg_map(pa, PCI_MAPREG_START + 4, PCI_MAPREG_TYPE_MEM, 0,
			   &sc->regiot, &sc->regioh, NULL, NULL)) {
		printf("%s: can't map registers\n", sc->dev.dv_xname);
		return;
	}

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->dev.dv_xname);
		return;
	}

	intrstr = pci_intr_string(pc, ih);
	sc->ih = pci_intr_establish(pc, ih, IPL_AUDIO, neo_intr, sc);

	if (sc->ih == NULL) {
		printf("%s: couldn't establish interrupt",
		       sc->dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interruping at %s\n", sc->dev.dv_xname, intrstr);

	if ((error = nm_init(sc)) != 0)
		return;

	/* Enable the device. */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
		       csr | PCI_COMMAND_MASTER_ENABLE);

	sc->host_if.arg = sc;

	sc->host_if.attach = neo_attach_codec;
	sc->host_if.read   = neo_read_codec;
	sc->host_if.write  = neo_write_codec;
	sc->host_if.reset  = neo_reset_codec;
	sc->host_if.flags  = neo_flags_codec;

	if ((error = ac97_attach(&sc->host_if)) != 0)
		return;

	sc->powerhook = powerhook_establish(neo_power, sc);

	audio_attach_mi(&neo_hw_if, sc, &sc->dev);
}

int
neo_read_codec(void *v, u_int8_t a, u_int16_t *d)
{
	struct neo_softc *sc = v;
	
	if (!nm_waitcd(sc)) {
		*d = nm_rd_2(sc, sc->ac97_base + a);
		DELAY(1000);
		return 0;
	}

	return (ENXIO);
}


int
neo_write_codec(void *v, u_int8_t a, u_int16_t d)
{
	struct neo_softc *sc = v;
	int cnt = 3;

	if (!nm_waitcd(sc)) {
		while (cnt-- > 0) {
			nm_wr_2(sc, sc->ac97_base + a, d);
			if (!nm_waitcd(sc)) {
				DELAY(1000);
				return (0);
			}
		}
	}

        return (ENXIO);
}

int
neo_attach_codec(void *v, struct ac97_codec_if *codec_if)
{
	struct neo_softc *sc = v;

	sc->codec_if = codec_if;
	return (0);
}

void
neo_reset_codec(void *v)
{
	struct neo_softc *sc = v;

	nm_wr_1(sc, 0x6c0, 0x01);
	nm_wr_1(sc, 0x6cc, 0x87);
	nm_wr_1(sc, 0x6cc, 0x80);
	nm_wr_1(sc, 0x6cc, 0x00);
}

enum ac97_host_flags
neo_flags_codec(void *v)
{

	return (AC97_HOST_DONT_READ);
}

int
neo_open(void *addr, int flags)
{

	return (0);
}

/*
 * Close function is called at splaudio().
 */
void
neo_close(void *addr)
{
	struct neo_softc *sc = addr;
    
	neo_halt_output(sc);
	neo_halt_input(sc);

	sc->pintr = 0;
	sc->rintr = 0;
}

int
neo_query_encoding(void *addr, struct audio_encoding *fp)
{

	switch (fp->index) {
	case 0:
		strcpy(fp->name, AudioEulinear);
		fp->encoding = AUDIO_ENCODING_ULINEAR;
		fp->precision = 8;
		fp->flags = 0;
		return (0);
	case 1:
		strcpy(fp->name, AudioEmulaw);
		fp->encoding = AUDIO_ENCODING_ULAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 2:
		strcpy(fp->name, AudioEalaw);
		fp->encoding = AUDIO_ENCODING_ALAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 3:
		strcpy(fp->name, AudioEslinear);
		fp->encoding = AUDIO_ENCODING_SLINEAR;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 4:
		strcpy(fp->name, AudioEslinear_le);
		fp->encoding = AUDIO_ENCODING_SLINEAR_LE;
		fp->precision = 16;
		fp->flags = 0;
		return (0);
	case 5:
		strcpy(fp->name, AudioEulinear_le);
		fp->encoding = AUDIO_ENCODING_ULINEAR_LE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 6:
		strcpy(fp->name, AudioEslinear_be);
		fp->encoding = AUDIO_ENCODING_SLINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 7:
		strcpy(fp->name, AudioEulinear_be);
		fp->encoding = AUDIO_ENCODING_ULINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	default:
		return (EINVAL);
	}
}

/* Todo: don't commit settings to card until we've verified all parameters */
int
neo_set_params(void *addr, int setmode, int usemode, struct audio_params *play,
    struct audio_params *rec)
{
	struct neo_softc *sc = addr;
	u_int32_t base;
	u_int8_t x;
	int mode;
	struct audio_params *p;

	for (mode = AUMODE_RECORD; mode != -1; 
	     mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = mode == AUMODE_PLAY ? play : rec;

		if (p == NULL) continue;

		for (x = 0; x < 8; x++) {
			if (p->sample_rate <
			    (samplerates[x] + samplerates[x + 1]) / 2)
				break;
		}
		if (x == 8)
			return (EINVAL);

		p->sample_rate = samplerates[x];
		nm_loadcoeff(sc, mode, x);

		x <<= 4;
		x &= NM_RATE_MASK;
		if (p->precision == 16)
			x |= NM_RATE_BITS_16;
		if (p->channels == 2)
			x |= NM_RATE_STEREO;

		base = (mode == AUMODE_PLAY)? 
		    NM_PLAYBACK_REG_OFFSET : NM_RECORD_REG_OFFSET;
		nm_wr_1(sc, base + NM_RATE_REG_OFFSET, x);
		
		p->factor = 1;
		p->sw_code = 0;
		switch (p->encoding) {
		case AUDIO_ENCODING_SLINEAR_BE:
			if (p->precision == 16)
				p->sw_code = swap_bytes;
			else
				p->sw_code = change_sign8;
			break;
		case AUDIO_ENCODING_SLINEAR_LE:
			if (p->precision != 16)
				p->sw_code = change_sign8;
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
			if (p->precision == 16) {
				if (mode == AUMODE_PLAY)
					p->sw_code =
					    swap_bytes_change_sign16_le;
				else
					p->sw_code =
					    change_sign16_swap_bytes_le;
			}
			break;
		case AUDIO_ENCODING_ULINEAR_LE:
			if (p->precision == 16)
				p->sw_code = change_sign16_le;
			break;
		case AUDIO_ENCODING_ULAW:
			if (mode == AUMODE_PLAY) {
				p->factor = 2;
				p->sw_code = mulaw_to_slinear16_le;
			} else
				p->sw_code = ulinear8_to_mulaw;
			break;
		case AUDIO_ENCODING_ALAW:
			if (mode == AUMODE_PLAY) {
				p->factor = 2;
				p->sw_code = alaw_to_slinear16_le;
			} else
				p->sw_code = ulinear8_to_alaw;
			break;
		default:
			return (EINVAL);
		}
	}


	return (0);
}

int
neo_round_blocksize(void *addr, int blk)
{

	return (NM_BUFFSIZE / 2);	
}

int
neo_trigger_output(void *addr, void *start, void *end, int blksize,
    void (*intr)(void *), void *arg, struct audio_params *param)
{
	struct neo_softc *sc = addr;
	int ssz;

	sc->pintr = intr;
	sc->parg = arg;

	ssz = (param->precision * param->factor == 16) ? 2 : 1;
	if (param->channels == 2)
		ssz <<= 1;

	sc->pbufsize = ((char*)end - (char *)start);
	sc->pblksize = blksize;
	sc->pwmark = blksize;

	nm_wr_4(sc, NM_PBUFFER_START, sc->pbuf);
	nm_wr_4(sc, NM_PBUFFER_END, sc->pbuf + sc->pbufsize - ssz); 
	nm_wr_4(sc, NM_PBUFFER_CURRP, sc->pbuf);
	nm_wr_4(sc, NM_PBUFFER_WMARK, sc->pbuf + sc->pwmark);
	nm_wr_1(sc, NM_PLAYBACK_ENABLE_REG, NM_PLAYBACK_FREERUN |
	    NM_PLAYBACK_ENABLE_FLAG);
	nm_wr_2(sc, NM_AUDIO_MUTE_REG, 0);

	return (0);
}

int
neo_trigger_input(void *addr, void *start, void *end, int blksize,
    void (*intr)(void *), void *arg, struct audio_params *param)
{
	struct neo_softc *sc = addr;	
	int ssz;

	sc->rintr = intr;
	sc->rarg = arg;

	ssz = (param->precision * param->factor == 16) ? 2 : 1;
	if (param->channels == 2)
		ssz <<= 1;

	sc->rbufsize = ((char*)end - (char *)start);
	sc->rblksize = blksize;
	sc->rwmark = blksize;

	nm_wr_4(sc, NM_RBUFFER_START, sc->rbuf);
	nm_wr_4(sc, NM_RBUFFER_END, sc->rbuf + sc->rbufsize);
	nm_wr_4(sc, NM_RBUFFER_CURRP, sc->rbuf);
	nm_wr_4(sc, NM_RBUFFER_WMARK, sc->rbuf + sc->rwmark);
	nm_wr_1(sc, NM_RECORD_ENABLE_REG, NM_RECORD_FREERUN |
	    NM_RECORD_ENABLE_FLAG);

	return (0);
}

int
neo_halt_output(void *addr)
{
	struct neo_softc *sc = (struct neo_softc *)addr;

	nm_wr_1(sc, NM_PLAYBACK_ENABLE_REG, 0);
	nm_wr_2(sc, NM_AUDIO_MUTE_REG, NM_AUDIO_MUTE_BOTH);

	return (0);
}

int
neo_halt_input(void *addr)
{
	struct neo_softc *sc = (struct neo_softc *)addr;

	nm_wr_1(sc, NM_RECORD_ENABLE_REG, 0);

	return (0);
}

int
neo_getdev(void *addr, struct audio_device *retp)
{

	*retp = neo_device;
	return (0);
}

int
neo_mixer_set_port(void *addr, mixer_ctrl_t *cp)
{
	struct neo_softc *sc = addr;

	return ((sc->codec_if->vtbl->mixer_set_port)(sc->codec_if, cp));
}

int
neo_mixer_get_port(void *addr, mixer_ctrl_t *cp)
{
	struct neo_softc *sc = addr;

	return ((sc->codec_if->vtbl->mixer_get_port)(sc->codec_if, cp));
}

int
neo_query_devinfo(void *addr, mixer_devinfo_t *dip)
{
	struct neo_softc *sc = addr;

	return ((sc->codec_if->vtbl->query_devinfo)(sc->codec_if, dip));
}

void *
neo_malloc(void *addr, int direction, size_t size, int pool, int flags)
{
	struct neo_softc *sc = addr;
	void *rv = NULL;

	switch (direction) {
	case AUMODE_PLAY:
		if (sc->pbuf_allocated == 0) {
			rv = (void *) sc->pbuf_vaddr;
			sc->pbuf_allocated = 1;
		}
		break;

	case AUMODE_RECORD:
		if (sc->rbuf_allocated == 0) {
			rv = (void *) sc->rbuf_vaddr;
			sc->rbuf_allocated = 1;
		}
		break;
	}

	return (rv);
}

void
neo_free(void *addr, void *ptr, int pool)
{
	struct neo_softc *sc = addr;
	vaddr_t v = (vaddr_t) ptr;

	if (v == sc->pbuf_vaddr)
		sc->pbuf_allocated = 0;
	else if (v == sc->rbuf_vaddr)
		sc->rbuf_allocated = 0;
	else
		printf("neo_free: bad address %p\n", ptr);
}

size_t
neo_round_buffersize(void *addr, int direction, size_t size)
{

	return (NM_BUFFSIZE);
}

paddr_t
neo_mappage(void *addr, void *mem, off_t off, int prot)
{
	struct neo_softc *sc = addr;
	vaddr_t v = (vaddr_t) mem;
	bus_addr_t pciaddr;

	if (v == sc->pbuf_vaddr)
		pciaddr = sc->pbuf_pciaddr;
	else if (v == sc->rbuf_vaddr)
		pciaddr = sc->rbuf_pciaddr;
	else
		return (-1);

	return (bus_space_mmap(sc->bufiot, pciaddr, off, prot,
	    BUS_SPACE_MAP_LINEAR));
}

int
neo_get_props(void *addr)
{

	return (AUDIO_PROP_INDEPENDENT | AUDIO_PROP_MMAP |
                AUDIO_PROP_FULLDUPLEX);
}
