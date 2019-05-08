/*	$NetBSD: neo.c,v 1.53 2019/05/08 13:40:19 isaki Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: neo.c,v 1.53 2019/05/08 13:40:19 isaki Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/audioio.h>

#include <dev/audio/audio_if.h>

#include <dev/ic/ac97var.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/neoreg.h>
#include <dev/pci/neo-coeff.h>

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
 * might work with wss isa drivers. This might require some playing around
 * with your BIOS.
 *
 * The Neomagic 256 AV/ZX have 2 PCI I/O region descriptors. Both of
 * them describe a memory region. The frame buffer is the first region
 * and the register set is the second region.
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
 * Unlike many sound engines, the Neomagic does not support (as far as
 * we know :) the notion of interrupting every n bytes transferred,
 * unlike many DMA engines.  Instead, it allows you to specify one
 * location in each ring buffer (called the watermark). When the chip
 * passes that location while playing, it signals an interrupt.
 *
 * The ring buffer size is currently 16k. That is about 100ms of audio
 * at 44.1kHz/stero/16 bit. However, to keep the buffer full, interrupts
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
	device_t	dev;
	kmutex_t	lock;
	kmutex_t	intr_lock;

	bus_space_tag_t bufiot;
	bus_space_handle_t bufioh;

	bus_space_tag_t regiot;
	bus_space_handle_t regioh;

	uint32_t	type;
	void		*ih;

	void	(*pintr)(void *);	/* DMA completion intr handler */
	void	*parg;		/* arg for intr() */

	void	(*rintr)(void *);	/* DMA completion intr handler */
	void	*rarg;		/* arg for intr() */

	vaddr_t	buf_vaddr;
	vaddr_t	rbuf_vaddr;
	vaddr_t	pbuf_vaddr;
	int	pbuf_allocated;
	int	rbuf_allocated;

	bus_addr_t buf_pciaddr;
	bus_addr_t rbuf_pciaddr;
	bus_addr_t pbuf_pciaddr;

	uint32_t	ac97_base, ac97_status, ac97_busy;
	uint32_t	buftop, pbuf, rbuf, cbuf, acbuf;
	uint32_t	playint, recint, misc1int, misc2int;
	uint32_t	irsz, badintr;

	uint32_t	pbufsize;
	uint32_t	rbufsize;

	uint32_t	pblksize;
	uint32_t	rblksize;

	uint32_t	pwmark;
	uint32_t	rwmark;

	struct ac97_codec_if *codec_if;
	struct ac97_host_if host_if;
};

/* -------------------------------------------------------------------- */

/*
 * prototypes
 */

static int	nm_waitcd(struct neo_softc *);
static int	nm_loadcoeff(struct neo_softc *, int, int);
static int	nm_init(struct neo_softc *);

static int	neo_match(device_t, cfdata_t, void *);
static void	neo_attach(device_t, device_t, void *);
static int	neo_intr(void *);
static int	neo_rate2index(u_int);

static int	neo_query_format(void *, audio_format_query_t *);
static int	neo_set_format(void *, int,
			       const audio_params_t *, const audio_params_t *,
			       audio_filter_reg_t *, audio_filter_reg_t *);
static int	neo_round_blocksize(void *, int, int, const audio_params_t *);
static int	neo_trigger_output(void *, void *, void *, int,
				   void (*)(void *), void *,
				   const audio_params_t *);
static int	neo_trigger_input(void *, void *, void *, int,
				  void (*)(void *), void *,
				  const audio_params_t *);
static int	neo_halt_output(void *);
static int	neo_halt_input(void *);
static int	neo_getdev(void *, struct audio_device *);
static int	neo_mixer_set_port(void *, mixer_ctrl_t *);
static int	neo_mixer_get_port(void *, mixer_ctrl_t *);
static int	neo_attach_codec(void *, struct ac97_codec_if *);
static int	neo_read_codec(void *, uint8_t, uint16_t *);
static int	neo_write_codec(void *, uint8_t, uint16_t);
static int     neo_reset_codec(void *);
static enum ac97_host_flags neo_flags_codec(void *);
static int	neo_query_devinfo(void *, mixer_devinfo_t *);
static void *	neo_malloc(void *, int, size_t);
static void	neo_free(void *, void *, size_t);
static size_t	neo_round_buffersize(void *, int, size_t);
static int	neo_get_props(void *);
static void	neo_get_locks(void *, kmutex_t **, kmutex_t **);

CFATTACH_DECL_NEW(neo, sizeof(struct neo_softc),
    neo_match, neo_attach, NULL, NULL);

static struct audio_device neo_device = {
	"NeoMagic 256",
	"",
	"neo"
};

/*
 * The frequency list in this format is also referred from neo_rate2index().
 * So don't rearrange or delete entries.
 */
static const struct audio_format neo_formats[] = {
	{
		.mode		= AUMODE_PLAY | AUMODE_RECORD,
		.encoding	= AUDIO_ENCODING_SLINEAR_LE,
		.validbits	= 16,
		.precision	= 16,
		.channels	= 2,
		.channel_mask	= AUFMT_STEREO,
		.frequency_type	= 8,
		.frequency	=
		    { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000 },
	},
};
#define NEO_NFORMATS	__arraycount(neo_formats)

/* -------------------------------------------------------------------- */

static const struct audio_hw_if neo_hw_if = {
	.query_format		= neo_query_format,
	.set_format		= neo_set_format,
	.round_blocksize	= neo_round_blocksize,
	.halt_output		= neo_halt_output,
	.halt_input		= neo_halt_input,
	.getdev			= neo_getdev,
	.set_port		= neo_mixer_set_port,
	.get_port		= neo_mixer_get_port,
	.query_devinfo		= neo_query_devinfo,
	.allocm			= neo_malloc,
	.freem			= neo_free,
	.round_buffersize	= neo_round_buffersize,
	.get_props		= neo_get_props,
	.trigger_output		= neo_trigger_output,
	.trigger_input		= neo_trigger_input,
	.get_locks		= neo_get_locks,
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
	int cnt;
	int fail;

	cnt  = 10;
	fail = 1;
	while (cnt-- > 0) {
		if (nm_rd_2(sc, sc->ac97_status) & sc->ac97_busy)
			DELAY(100);
		else {
			fail = 0;
			break;
		}
	}
	return fail;
}

static void
nm_ackint(struct neo_softc *sc, uint32_t num)
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
	uint32_t addr;

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
static int
neo_intr(void *p)
{
	struct neo_softc *sc;
	int status, x;
	int rv;

	sc = (struct neo_softc *)p;
	mutex_spin_enter(&sc->intr_lock);

	rv = 0;
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
		nm_wr_4(sc, NM_RBUFFER_WMARK, sc->rbuf + sc->rwmark);
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
		printf("%s: misc int 1\n", device_xname(sc->dev));
		rv = 1;
	}
	if (status & sc->misc2int) {
		status &= ~sc->misc2int;
		nm_ackint(sc, sc->misc2int);
		x = nm_rd_1(sc, 0x400);
		nm_wr_1(sc, 0x400, x & ~2);
		printf("%s: misc int 2\n", device_xname(sc->dev));
		rv = 1;
	}
	if (status) {
		status &= ~sc->misc2int;
		nm_ackint(sc, sc->misc2int);
		printf("%s: unknown int\n", device_xname(sc->dev));
		rv = 1;
	}

	mutex_spin_exit(&sc->intr_lock);
	return rv;
}

/* -------------------------------------------------------------------- */

/*
 * Probe and attach the card
 */

static int
nm_init(struct neo_softc *sc)
{
	uint32_t ofs, i;

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

static int
neo_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa;
	pcireg_t subdev;

	pa = aux;
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_NEOMAGIC)
		return 0;

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
				return 0;
			}
			break;

		case PCI_VENDOR_HP:
			switch (PCI_PRODUCT(subdev)) {
			case 0x0007:
				return 0;
			}
			break;

		case PCI_VENDOR_IBM:
			switch (PCI_PRODUCT(subdev)) {
			case 0x00dd:
				return 0;
			}
			break;
		}
		return 1;

	case PCI_PRODUCT_NEOMAGIC_NMMM256ZX_AU:
		return 1;
	}

	return 0;
}

static bool
neo_resume(device_t dv, const pmf_qual_t *qual)
{
	struct neo_softc *sc = device_private(dv);

	mutex_enter(&sc->lock);
	mutex_spin_enter(&sc->intr_lock);
	nm_init(sc);
	mutex_spin_exit(&sc->intr_lock);
	sc->codec_if->vtbl->restore_ports(sc->codec_if);
	mutex_exit(&sc->lock);

	return true;
}

static void
neo_attach(device_t parent, device_t self, void *aux)
{
	struct neo_softc *sc;
	struct pci_attach_args *pa;
	pci_chipset_tag_t pc;
	char const *intrstr;
	pci_intr_handle_t ih;
	pcireg_t csr;
	int error;
	char intrbuf[PCI_INTRSTR_LEN];

	sc = device_private(self);
	pa = aux;
	pc = pa->pa_pc;

	sc->type = PCI_PRODUCT(pa->pa_id);

	printf(": NeoMagic 256%s audio\n",
	    sc->type == PCI_PRODUCT_NEOMAGIC_NMMM256AV_AU ? "AV" : "ZX");

	/* Map I/O register */
	if (pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_MEM, 0,
			   &sc->bufiot, &sc->bufioh, &sc->buf_pciaddr, NULL)) {
		aprint_error_dev(self, "can't map buffer\n");
		return;
	}

	if (pci_mapreg_map(pa, PCI_MAPREG_START + 4, PCI_MAPREG_TYPE_MEM,
	    BUS_SPACE_MAP_LINEAR, &sc->regiot, &sc->regioh, NULL, NULL)) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		aprint_error_dev(self, "couldn't map interrupt\n");
		return;
	}

	mutex_init(&sc->lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->intr_lock, MUTEX_DEFAULT, IPL_AUDIO);

	intrstr = pci_intr_string(pc, ih, intrbuf, sizeof(intrbuf));
	sc->ih = pci_intr_establish_xname(pc, ih, IPL_AUDIO, neo_intr, sc,
	    device_xname(self));

	if (sc->ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		mutex_destroy(&sc->lock);
		mutex_destroy(&sc->intr_lock);
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	mutex_spin_enter(&sc->intr_lock);
	error = nm_init(sc);
	mutex_spin_exit(&sc->intr_lock);
	if (error != 0) {
		mutex_destroy(&sc->lock);
		mutex_destroy(&sc->intr_lock);
		return;
	}

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

	if (ac97_attach(&sc->host_if, self, &sc->lock) != 0) {
		mutex_destroy(&sc->lock);
		mutex_destroy(&sc->intr_lock);
		return;
	}

	if (!pmf_device_register(self, NULL, neo_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	audio_attach_mi(&neo_hw_if, sc, self);
}

static int
neo_read_codec(void *v, uint8_t a, uint16_t *d)
{
	struct neo_softc *sc;

	sc = v;
	if (!nm_waitcd(sc)) {
		*d = nm_rd_2(sc, sc->ac97_base + a);
		DELAY(1000);
		return 0;
	}

	return ENXIO;
}


static int
neo_write_codec(void *v, u_int8_t a, u_int16_t d)
{
	struct neo_softc *sc;
	int cnt;

	sc = v;
	cnt = 3;
	if (!nm_waitcd(sc)) {
		while (cnt-- > 0) {
			nm_wr_2(sc, sc->ac97_base + a, d);
			if (!nm_waitcd(sc)) {
				DELAY(1000);
				return 0;
			}
		}
	}

	return ENXIO;
}

static int
neo_attach_codec(void *v, struct ac97_codec_if *codec_if)
{
	struct neo_softc *sc;

	sc = v;
	sc->codec_if = codec_if;
	return 0;
}

static int
neo_reset_codec(void *v)
{
	struct neo_softc *sc;

	sc = v;
	nm_wr_1(sc, 0x6c0, 0x01);
	nm_wr_1(sc, 0x6cc, 0x87);
	nm_wr_1(sc, 0x6cc, 0x80);
	nm_wr_1(sc, 0x6cc, 0x00);
	return 0;
}

static enum ac97_host_flags
neo_flags_codec(void *v)
{

	return AC97_HOST_DONT_READ;
}

static int
neo_query_format(void *addr, audio_format_query_t *afp)
{

	return audio_query_format(neo_formats, NEO_NFORMATS, afp);
}

/* Return index number of sample_rate */
static int
neo_rate2index(u_int sample_rate)
{
	int i;

	for (i = 0; i < neo_formats[0].frequency_type; i++) {
		if (sample_rate == neo_formats[0].frequency[i])
			return i;
	}

	/* NOTREACHED */
	panic("neo_formats.frequency mismatch?");
}

/* Todo: don't commit settings to card until we've verified all parameters */
static int
neo_set_format(void *addr, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct neo_softc *sc;
	const audio_params_t *p;
	uint32_t base;
	uint8_t x;
	int mode;

	sc = addr;
	for (mode = AUMODE_RECORD; mode != -1;
	     mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = mode == AUMODE_PLAY ? play : rec;

		x = neo_rate2index(p->sample_rate);
		nm_loadcoeff(sc, mode, x);

		x <<= 4;
		x &= NM_RATE_MASK;
		x |= NM_RATE_BITS_16;
		x |= NM_RATE_STEREO;

		base = (mode == AUMODE_PLAY)?
		    NM_PLAYBACK_REG_OFFSET : NM_RECORD_REG_OFFSET;
		nm_wr_1(sc, base + NM_RATE_REG_OFFSET, x);
	}

	return 0;
}

static int
neo_round_blocksize(void *addr, int blk, int mode,
    const audio_params_t *param)
{

	/* The number of blocks must be 3 or greater. */
	return NM_BUFFSIZE / 4;
}

static int
neo_trigger_output(void *addr, void *start, void *end, int blksize,
    void (*intr)(void *), void *arg, const audio_params_t *param)
{
	struct neo_softc *sc;
	int ssz;

	sc = addr;
	sc->pintr = intr;
	sc->parg = arg;

	ssz = (param->precision == 16) ? 2 : 1;
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

	return 0;
}

static int
neo_trigger_input(void *addr, void *start, void *end, int blksize,
    void (*intr)(void *), void *arg, const audio_params_t *param)
{
	struct neo_softc *sc;
	int ssz;

	sc = addr;
	sc->rintr = intr;
	sc->rarg = arg;

	ssz = (param->precision == 16) ? 2 : 1;
	if (param->channels == 2)
		ssz <<= 1;

	sc->rbufsize = ((char*)end - (char *)start);
	sc->rblksize = blksize;
	sc->rwmark = blksize;

	/*
	 * XXX Doesn't it need to subtract ssz from BUFFER_END like
	 * trigger_output()?
	 */
	nm_wr_4(sc, NM_RBUFFER_START, sc->rbuf);
	nm_wr_4(sc, NM_RBUFFER_END, sc->rbuf + sc->rbufsize);
	nm_wr_4(sc, NM_RBUFFER_CURRP, sc->rbuf);
	nm_wr_4(sc, NM_RBUFFER_WMARK, sc->rbuf + sc->rwmark);
	nm_wr_1(sc, NM_RECORD_ENABLE_REG, NM_RECORD_FREERUN |
	    NM_RECORD_ENABLE_FLAG);

	return 0;
}

static int
neo_halt_output(void *addr)
{
	struct neo_softc *sc;

	sc = (struct neo_softc *)addr;
	nm_wr_1(sc, NM_PLAYBACK_ENABLE_REG, 0);
	nm_wr_2(sc, NM_AUDIO_MUTE_REG, NM_AUDIO_MUTE_BOTH);
	sc->pintr = 0;

	return 0;
}

static int
neo_halt_input(void *addr)
{
	struct neo_softc *sc;

	sc = (struct neo_softc *)addr;
	nm_wr_1(sc, NM_RECORD_ENABLE_REG, 0);
	sc->rintr = 0;

	return 0;
}

static int
neo_getdev(void *addr, struct audio_device *retp)
{

	*retp = neo_device;
	return 0;
}

static int
neo_mixer_set_port(void *addr, mixer_ctrl_t *cp)
{
	struct neo_softc *sc;

	sc = addr;
	return sc->codec_if->vtbl->mixer_set_port(sc->codec_if, cp);
}

static int
neo_mixer_get_port(void *addr, mixer_ctrl_t *cp)
{
	struct neo_softc *sc;

	sc = addr;
	return sc->codec_if->vtbl->mixer_get_port(sc->codec_if, cp);
}

static int
neo_query_devinfo(void *addr, mixer_devinfo_t *dip)
{
	struct neo_softc *sc;

	sc = addr;
	return sc->codec_if->vtbl->query_devinfo(sc->codec_if, dip);
}

static void *
neo_malloc(void *addr, int direction, size_t size)
{
	struct neo_softc *sc;
	void *rv;

	sc = addr;
	rv = NULL;
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

	return rv;
}

static void
neo_free(void *addr, void *ptr, size_t size)
{
	struct neo_softc *sc;
	vaddr_t v;

	sc = addr;
	v = (vaddr_t)ptr;
	if (v == sc->pbuf_vaddr)
		sc->pbuf_allocated = 0;
	else if (v == sc->rbuf_vaddr)
		sc->rbuf_allocated = 0;
	else
		printf("neo_free: bad address %p\n", ptr);
}

static size_t
neo_round_buffersize(void *addr, int direction, size_t size)
{

	return NM_BUFFSIZE;
}

static int
neo_get_props(void *addr)
{

	return AUDIO_PROP_INDEPENDENT | AUDIO_PROP_MMAP |
	    AUDIO_PROP_FULLDUPLEX;
}

static void
neo_get_locks(void *addr, kmutex_t **intr, kmutex_t **thread)
{
	struct neo_softc *sc;

	sc = addr;
	*intr = &sc->intr_lock;
	*thread = &sc->lock;
}
