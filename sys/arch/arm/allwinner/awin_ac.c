/* $NetBSD: awin_ac.c,v 1.12 2014/09/06 23:04:10 jmcneill Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "locators.h"
#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_ac.c,v 1.12 2014/09/06 23:04:10 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#define AWINAC_TX_TRIG_LEVEL	0xf
#define AWINAC_RX_TRIG_LEVEL	0x7
#define AWINAC_DRQ_CLR_CNT	0x3
#define AWINAC_INIT_VOL		0x3b

#define AC_DAC_DPC		0x00
#define  DAC_DPC_EN_DA		__BIT(31)
#define  DAC_DPC_MODQU		__BITS(28,25)
#define  DAC_DPC_DWA		__BIT(24)
#define  DAC_DPC_HPF_EN		__BIT(18)
#define  DAC_DPC_DVOL		__BITS(17,12)
#define AC_DAC_FIFOC		0x04
#define  DAC_FIFOC_FS		__BITS(31,29)
#define   DAC_FS_48KHZ		0
#define   DAC_FS_32KHZ		1
#define   DAC_FS_24KHZ		2
#define   DAC_FS_16KHZ		3
#define   DAC_FS_12KHZ		4
#define   DAC_FS_8KHZ		5
#define   DAC_FS_192KHZ		6
#define   DAC_FS_96KHZ		7
#define  DAC_FIFOC_FIR_VER	__BIT(28)
#define  DAC_FIFOC_SEND_LASAT	__BIT(26)
#define  DAC_FIFOC_FIFO_MODE	__BITS(25,24)
#define   FIFO_MODE_24_31_8	0
#define   FIFO_MODE_16_31_16	0
#define   FIFO_MODE_16_15_0	1
#define  DAC_FIFOC_DRQ_CLR_CNT	__BITS(22,21)
#define  DAC_FIFOC_TX_TRIG_LEVEL __BITS(14,8)
#define  DAC_FIFOC_ADDA_LOOP_EN	__BIT(7)
#define  DAC_FIFOC_MONO_EN	__BIT(6)
#define  DAC_FIFOC_TX_SAMPLE_BITS __BIT(5)
#define  DAC_FIFOC_DRQ_EN	__BIT(4)
#define  DAC_FIFOC_IRQ_EN	__BIT(3)
#define  DAC_FIFOC_FIFO_UNDERRUN_IRQ_EN __BIT(2)
#define  DAC_FIFOC_FIFO_OVERRUN_IRQ_EN __BIT(1)
#define  DAC_FIFOC_FIFO_FLUSH	__BIT(0)
#define AC_DAC_FIFOS		0x08
#define  DAC_FIFOS_TX_EMPTY	__BIT(23)
#define  DAC_FIFOS_TXE_CNT	__BITS(22,8)
#define  DAC_FIFOS_TXE_INT	__BIT(3)
#define  DAC_FIFOS_TXU_INT	__BIT(2)
#define  DAC_FIFOS_TXO_INT	__BIT(1)
#define  DAC_FIFOS_INT_MASK	__BITS(3,1)
#define AC_DAC_TXDATA		0x0c
#define AC_DAC_ACTL		0x10
#define  DAC_ACTL_DACAREN	__BIT(31)
#define  DAC_ACTL_DACALEN	__BIT(30)
#define  DAC_ACTL_MIXEN		__BIT(29)
#define  DAC_ACTL_LNG		__BIT(26)
#define  DAC_ACTL_FMG		__BITS(25,23)
#define  DAC_ACTL_MICG		__BITS(22,20)
#define  DAC_ACTL_LLNS		__BIT(19)
#define  DAC_ACTL_RLNS		__BIT(18)
#define  DAC_ACTL_LFMS		__BIT(17)
#define  DAC_ACTL_RFMS		__BIT(16)
#define  DAC_ACTL_LDACLMIXS	__BIT(15)
#define  DAC_ACTL_RDACRMIXS	__BIT(14)
#define  DAC_ACTL_LDACRMIXS	__BIT(13)
#define  DAC_ACTL_MIC1LS	__BIT(12)
#define  DAC_ACTL_MIC1RS	__BIT(11)
#define  DAC_ACTL_MIC2LS	__BIT(10)
#define  DAC_ACTL_MIC2RS	__BIT(9)
#define  DAC_ACTL_DACPAS	__BIT(8)
#define  DAC_ACTL_MIXPAS	__BIT(7)
#define  DAC_ACTL_PAMUTE	__BIT(6)
#define  DAC_ACTL_PAVOL		__BITS(5,0)
#define AC_DAC_TUNE		0x14
#define AC_ADC_FIFOC		0x1c
#define  ADC_FIFOC_FS		__BITS(31,29)
#define   ADC_FS_48KHZ		0
#define   ADC_FS_32KHZ		1
#define   ADC_FS_24KHZ		2
#define   ADC_FS_16KHZ		3
#define   ADC_FS_12KHZ		4
#define   DAC_FS_8KHZ		5
#define  ADC_FIFOC_EN_AD	__BIT(28)
#define  ADC_FIFOC_RX_FIFO_MODE	__BIT(24)
#define  ADC_FIFOC_RX_TRIG_LEVEL __BITS(12,8)
#define  ADC_FIFOC_MONO_EN	__BIT(7)
#define  ADC_FIFOC_RX_SAMPLE_BITS __BIT(6)
#define  ADC_FIFOC_DRQ_EN	__BIT(4)
#define  ADC_FIFOC_IRQ_EN	__BIT(3)
#define  ADC_FIFOC_OVERRUN_IRQ_EN __BIT(2)
#define  ADC_FIFOC_FIFO_FLUSH	__BIT(1)
#define AC_ADC_FIFOS		0x20
#define  ADC_FIFOS_RXA		__BIT(23)
#define  ADC_FIFOS_RXA_CNT	__BITS(13,8)
#define  ADC_FIFOS_RXA_INT	__BIT(3)
#define  ADC_FIFOS_RXO_INT	__BIT(1)
#define AC_ADC_RXDATA		0x24
#define AC_ADC_ACTL		0x28
#define  ADC_ACTL_ADCREN	__BIT(31)
#define  ADC_ACTL_ADCLEN	__BIT(30)
#define  ADC_ACTL_PREG1EN	__BIT(29)
#define  ADC_ACTL_PREG2EN	__BIT(28)
#define  ADC_ACTL_VMICEN	__BIT(27)
#define  ADC_ACTL_ADCG		__BITS(22,20)
#define  ADC_ACTL_ADCIS		__BITS(19,17)
#define  ADC_ACTL_LNRDF		__BIT(16)
#define  ADC_ACTL_LNPREG	__BIT(15)
#define  ADC_ACTL_LHPOUTN	__BIT(11)
#define  ADC_ACTL_RHPOUTN	__BIT(10)
#define  ADC_ACTL_DITHER	__BIT(8)
#define  ADC_ACTL_DITHER_CLK_SELECT __BITS(7,6)
#define  ADC_ACTL_PA_EN		__BIT(4)
#define  ADC_ACTL_DDE		__BIT(3)
#define  ADC_ACTL_COMPTEN	__BIT(2)
#define  ADC_ACTL_PTDBS		__BITS(1,0)
#define AC_DAC_CNT		0x30
#define AC_ADC_CNT		0x34
#define AC_DAC_CAL		0x38
#define AC_MIC_PHONE_CAL	0x3c

struct awinac_dma {
	LIST_ENTRY(awinac_dma)	dma_list;
	bus_dmamap_t		dma_map;
	void			*dma_addr;
	size_t			dma_size;
	bus_dma_segment_t	dma_segs[1];
	int			dma_nsegs;
};

struct awinac_softc {
	device_t		sc_dev;
	device_t		sc_audiodev;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;

	LIST_HEAD(, awinac_dma)	sc_dmalist;

	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;

	struct audio_format	sc_format;
	struct audio_encoding_set *sc_encodings;

	audio_params_t		sc_pparam;
	struct awin_dma_channel *sc_pdma;
	void			(*sc_pint)(void *);
	void			*sc_pintarg;
	bus_addr_t		sc_pstart;
	bus_addr_t		sc_pend;
	bus_addr_t		sc_pcur;
	int			sc_pblksize;

	audio_params_t		sc_rparam;
	struct awin_dma_channel *sc_rdma;
	void			(*sc_rint)(void *);
	void			*sc_rintarg;
	bus_addr_t		sc_rstart;
	bus_addr_t		sc_rend;
	bus_addr_t		sc_rcur;
	int			sc_rblksize;

	struct awin_gpio_pindata sc_pactrl_gpio;
	bool			sc_has_pactrl_gpio;
};

static int	awinac_match(device_t, cfdata_t, void *);
static void	awinac_attach(device_t, device_t, void *);
static int	awinac_rescan(device_t, const char *, const int *);
static void	awinac_childdet(device_t, device_t);

static void	awinac_init(struct awinac_softc *);
static int	awinac_allocdma(struct awinac_softc *, size_t, size_t,
				struct awinac_dma *);
static void	awinac_freedma(struct awinac_softc *, struct awinac_dma *);

static void	awinac_pint(void *);
static int	awinac_play(struct awinac_softc *);
static void	awinac_rint(void *);
static int	awinac_rec(struct awinac_softc *);

#if defined(DDB)
void		awinac_dump_regs(void);
#endif

static int	awinac_open(void *, int);
static void	awinac_close(void *);
static int	awinac_drain(void *);
static int	awinac_query_encoding(void *, struct audio_encoding *);
static int	awinac_set_params(void *, int, int,
				  audio_params_t *,
				  audio_params_t *,
				  stream_filter_list_t *,
				  stream_filter_list_t *);
static int	awinac_commit_settings(void *);
static int	awinac_halt_output(void *);
static int	awinac_halt_input(void *);
static int	awinac_set_port(void *, mixer_ctrl_t *);
static int	awinac_get_port(void *, mixer_ctrl_t *);
static int	awinac_query_devinfo(void *, mixer_devinfo_t *);
static void *	awinac_allocm(void *, int, size_t);
static void	awinac_freem(void *, void *, size_t);
static paddr_t	awinac_mappage(void *, void *, off_t, int);
static int	awinac_getdev(void *, struct audio_device *);
static int	awinac_get_props(void *);
static int	awinac_round_blocksize(void *, int, int,
				       const audio_params_t *);
static size_t	awinac_round_buffersize(void *, int, size_t);
static int	awinac_trigger_output(void *, void *, void *, int,
				      void (*)(void *), void *,
				      const audio_params_t *);
static int	awinac_trigger_input(void *, void *, void *, int,
				     void (*)(void *), void *,
				     const audio_params_t *);
static void	awinac_get_locks(void *, kmutex_t **, kmutex_t **);

static const struct audio_hw_if awinac_hw_if = {
	.open = awinac_open,
	.close = awinac_close,
	.drain = awinac_drain,
	.query_encoding = awinac_query_encoding,
	.set_params = awinac_set_params,
	.commit_settings = awinac_commit_settings,
	.halt_output = awinac_halt_output,
	.halt_input = awinac_halt_input,
	.allocm = awinac_allocm,
	.freem = awinac_freem,
	.mappage = awinac_mappage,
	.getdev = awinac_getdev,
	.set_port = awinac_set_port,
	.get_port = awinac_get_port,
	.query_devinfo = awinac_query_devinfo,
	.get_props = awinac_get_props,
	.round_blocksize = awinac_round_blocksize,
	.round_buffersize = awinac_round_buffersize,
	.trigger_output = awinac_trigger_output,
	.trigger_input = awinac_trigger_input,
	.get_locks = awinac_get_locks,
};

enum {
	AC_OUTPUT_CLASS,
	AC_INPUT_CLASS,
	AC_OUTPUT_MASTER_VOLUME,
	AC_INPUT_DAC_VOLUME,
	AC_ENUM_LAST
};

CFATTACH_DECL2_NEW(awin_ac, sizeof(struct awinac_softc),
	awinac_match, awinac_attach, NULL, NULL,
	awinac_rescan, awinac_childdet);

#define AC_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define AC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static int
awinac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AWINIOCF_PORT];

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	if (port != AWINIOCF_PORT_DEFAULT && port != loc->loc_port)
		return 0;

	return 1;
}

static void
awinac_attach(device_t parent, device_t self, void *aux)
{
	struct awinac_softc * const sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	prop_dictionary_t cfg = device_properties(self);
	const char *pin_name;
	int error;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	LIST_INIT(&sc->sc_dmalist);
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	aprint_naive("\n");
	aprint_normal(": CODEC\n");

	if (prop_dictionary_get_cstring_nocopy(cfg, "pactrl-gpio", &pin_name)) {
		if (!awin_gpio_pin_reserve(pin_name, &sc->sc_pactrl_gpio)) {
			aprint_error_dev(self,
			    "failed to reserve GPIO \"%s\"\n", pin_name);
		} else {
			sc->sc_has_pactrl_gpio = true;
		}
	}

	awin_pll2_enable();
	awin_reg_set_clear(sc->sc_bst, aio->aio_ccm_bsh,
	    AWIN_APB0_GATING_REG, AWIN_APB0_GATING_ADDA, 0);
	awin_reg_set_clear(sc->sc_bst, aio->aio_ccm_bsh,
	    AWIN_AUDIO_CODEC_CLK_REG, AWIN_CLK_ENABLE, 0);

	if (sc->sc_has_pactrl_gpio)
		awin_gpio_pindata_write(&sc->sc_pactrl_gpio, 0);

	awinac_init(sc);

	sc->sc_pdma = awin_dma_alloc(AWIN_DMA_TYPE_NDMA, awinac_pint, sc);
	if (sc->sc_pdma == NULL) {
		aprint_error_dev(self, "couldn't allocate play DMA channel\n");
		return;
	}
	sc->sc_rdma = awin_dma_alloc(AWIN_DMA_TYPE_NDMA, awinac_rint, sc);
	if (sc->sc_rdma == NULL) {
		aprint_error_dev(self, "couldn't allocate rec DMA channel\n");
		return;
	}

	sc->sc_format.mode = AUMODE_PLAY|AUMODE_RECORD;
	sc->sc_format.encoding = AUDIO_ENCODING_SLINEAR_LE;
	sc->sc_format.validbits = 16;
	sc->sc_format.precision = 16;
	sc->sc_format.channels = 2;
	sc->sc_format.channel_mask = AUFMT_STEREO;
	sc->sc_format.frequency_type = 0;
	sc->sc_format.frequency[0] = sc->sc_format.frequency[1] = 48000;

	error = auconv_create_encodings(&sc->sc_format, 1, &sc->sc_encodings);
	if (error) {
		aprint_error_dev(self, "couldn't create encodings (error=%d)\n",
		    error);
		return;
	}

	awinac_rescan(self, NULL, NULL);
}

static int
awinac_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct awinac_softc *sc = device_private(self);

	if (ifattr_match(ifattr, "audiobus") && sc->sc_audiodev == NULL) {
		if (sc->sc_encodings == NULL)
			return EIO;
		sc->sc_audiodev = audio_attach_mi(&awinac_hw_if,
		    sc, sc->sc_dev);
	}

	return 0;
}

static void
awinac_childdet(device_t self, device_t child)
{
	struct awinac_softc *sc = device_private(self);

	if (sc->sc_audiodev == child)
		sc->sc_audiodev = NULL;
}

static void
awinac_init(struct awinac_softc *sc)
{
	uint32_t val;

	val = AC_READ(sc, AC_DAC_DPC);
	val |= DAC_DPC_EN_DA;
	AC_WRITE(sc, AC_DAC_DPC, val);

	val = AC_READ(sc, AC_DAC_ACTL);
	val |= DAC_ACTL_PAMUTE;
	val &= ~DAC_ACTL_PAVOL;
	val |= __SHIFTIN(AWINAC_INIT_VOL, DAC_ACTL_PAVOL);
	AC_WRITE(sc, AC_DAC_ACTL, val);

	val = AC_READ(sc, AC_ADC_ACTL);
	val |= ADC_ACTL_PA_EN;
	AC_WRITE(sc, AC_ADC_ACTL, val);

	val = AC_READ(sc, AC_DAC_FIFOC);
	val &= ~DAC_FIFOC_IRQ_EN;
	val &= ~DAC_FIFOC_DRQ_EN;
	val &= ~DAC_FIFOC_FIFO_UNDERRUN_IRQ_EN;
	val &= ~DAC_FIFOC_FIFO_OVERRUN_IRQ_EN;
	AC_WRITE(sc, AC_DAC_FIFOC, val);

	val = AC_READ(sc, AC_ADC_FIFOC);
	val &= ~ADC_FIFOC_DRQ_EN;
	val &= ~ADC_FIFOC_IRQ_EN;
	val &= ~ADC_FIFOC_OVERRUN_IRQ_EN;
	AC_WRITE(sc, AC_ADC_FIFOC, val);

	AC_WRITE(sc, AC_DAC_FIFOS, AC_READ(sc, AC_DAC_FIFOS));
	AC_WRITE(sc, AC_ADC_FIFOS, AC_READ(sc, AC_ADC_FIFOS));
}

static int
awinac_allocdma(struct awinac_softc *sc, size_t size, size_t align,
    struct awinac_dma *dma)
{
	int error;

	dma->dma_size = size;
	error = bus_dmamem_alloc(sc->sc_dmat, dma->dma_size, align, 0,
	    dma->dma_segs, 1, &dma->dma_nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;

	error = bus_dmamem_map(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs,
	    dma->dma_size, &dma->dma_addr, BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (error)
		goto free;

	error = bus_dmamap_create(sc->sc_dmat, dma->dma_size, dma->dma_nsegs,
	    dma->dma_size, 0, BUS_DMA_WAITOK, &dma->dma_map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(sc->sc_dmat, dma->dma_map, dma->dma_addr,
	    dma->dma_size, NULL, BUS_DMA_WAITOK);
	if (error)
		goto destroy;

	return 0;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
unmap:
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
free:
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs);

	return error;
}

static void
awinac_freedma(struct awinac_softc *sc, struct awinac_dma *dma)
{
	bus_dmamap_unload(sc->sc_dmat, dma->dma_map);
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs);
}

static void
awinac_pint(void *priv)
{
	struct awinac_softc *sc = priv;

	mutex_enter(&sc->sc_intr_lock);
	if (sc->sc_pint == NULL) {
		mutex_exit(&sc->sc_intr_lock);
		return;
	}
	sc->sc_pint(sc->sc_pintarg);
	mutex_exit(&sc->sc_intr_lock);

	awinac_play(sc);
}

static int
awinac_play(struct awinac_softc *sc)
{
	int error;

	error = awin_dma_transfer(sc->sc_pdma,
	    sc->sc_pcur, AWIN_CORE_PBASE + AWIN_AC_OFFSET + AC_DAC_TXDATA,
	    sc->sc_pblksize);
	if (error) {
		device_printf(sc->sc_dev, "failed to transfer DMA; error %d\n",
		    error);
		return error;
	}

	sc->sc_pcur += sc->sc_pblksize;
	if (sc->sc_pcur >= sc->sc_pend)
		sc->sc_pcur = sc->sc_pstart;

	return 0;
}

static void
awinac_rint(void *priv)
{
	struct awinac_softc *sc = priv;

	mutex_enter(&sc->sc_intr_lock);
	if (sc->sc_rint == NULL) {
		mutex_exit(&sc->sc_intr_lock);
		return;
	}
	sc->sc_rint(sc->sc_rintarg);
	mutex_exit(&sc->sc_intr_lock);

	awinac_rec(sc);
}

static int
awinac_rec(struct awinac_softc *sc)
{
	int error;

	error = awin_dma_transfer(sc->sc_rdma,
	    AWIN_CORE_PBASE + AWIN_AC_OFFSET + AC_ADC_RXDATA, sc->sc_rcur,
	    sc->sc_rblksize);
	if (error) {
		device_printf(sc->sc_dev, "failed to transfer DMA; error %d\n",
		    error);
		return error;
	}

	sc->sc_rcur += sc->sc_rblksize;
	if (sc->sc_rcur >= sc->sc_rend)
		sc->sc_rcur = sc->sc_rstart;

	return 0;
}

static int
awinac_open(void *priv, int flags)
{
	struct awinac_softc *sc = priv;

	if (sc->sc_has_pactrl_gpio)
		awin_gpio_pindata_write(&sc->sc_pactrl_gpio, 1);

	return 0;
}

static void
awinac_close(void *priv)
{
	struct awinac_softc *sc = priv;

	if (sc->sc_has_pactrl_gpio)
		awin_gpio_pindata_write(&sc->sc_pactrl_gpio, 0);
}

static int
awinac_drain(void *priv)
{
	struct awinac_softc *sc = priv;
	uint32_t val;

	val = AC_READ(sc, AC_DAC_FIFOC);
	val |= DAC_FIFOC_FIFO_FLUSH;
	AC_WRITE(sc, AC_DAC_FIFOC, val);

	val = AC_READ(sc, AC_ADC_FIFOC);
	val |= ADC_FIFOC_FIFO_FLUSH;
	AC_WRITE(sc, AC_ADC_FIFOC, val);

	return 0;
}

static int
awinac_query_encoding(void *priv, struct audio_encoding *ae)
{
	struct awinac_softc *sc = priv;

	return auconv_query_encoding(sc->sc_encodings, ae);
}

static int
awinac_set_params(void *priv, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct awinac_softc *sc = priv;
	int index;

	if (play && (setmode & AUMODE_PLAY)) {
		index = auconv_set_converter(&sc->sc_format, 1,
		    AUMODE_PLAY, play, true, pfil);
		if (index < 0)
			return EINVAL;
		sc->sc_pparam = pfil->req_size > 0 ?
		    pfil->filters[0].param :
		    *play;
	}
	if (rec && (setmode & AUMODE_RECORD)) {
		index = auconv_set_converter(&sc->sc_format, 1,
		    AUMODE_RECORD, rec, true, rfil);
		if (index < 0)
			return EINVAL;
		sc->sc_rparam = rfil->req_size > 0 ?
		    rfil->filters[0].param :
		    *rec;
	}

	return 0;
}

static int
awinac_commit_settings(void *priv)
{
	struct awinac_softc *sc = priv;
	uint32_t val;

	if (sc->sc_pparam.sample_rate != 48000)
		return EINVAL;
	if (sc->sc_pparam.validbits != 16 && sc->sc_pparam.validbits != 24)
		return EINVAL;
	if (sc->sc_rparam.sample_rate != 48000)
		return EINVAL;
	if (sc->sc_rparam.validbits != 16 && sc->sc_rparam.validbits != 24)
		return EINVAL;

	val = AC_READ(sc, AC_DAC_FIFOC);
	val &= ~DAC_FIFOC_FIR_VER;
	val &= ~DAC_FIFOC_FS;
	val |= __SHIFTIN(DAC_FS_48KHZ, DAC_FIFOC_FS);
	val &= ~DAC_FIFOC_SEND_LASAT;
	val &= ~DAC_FIFOC_FIFO_MODE;
	val |= __SHIFTIN(FIFO_MODE_16_15_0, DAC_FIFOC_FIFO_MODE);
	val &= ~DAC_FIFOC_DRQ_CLR_CNT;
	val |= __SHIFTIN(AWINAC_DRQ_CLR_CNT, DAC_FIFOC_DRQ_CLR_CNT);
	val &= ~DAC_FIFOC_TX_TRIG_LEVEL;
	val |= __SHIFTIN(AWINAC_TX_TRIG_LEVEL, DAC_FIFOC_TX_TRIG_LEVEL);
	val &= ~DAC_FIFOC_ADDA_LOOP_EN;
	val &= ~DAC_FIFOC_MONO_EN;
	if (sc->sc_pparam.validbits == 16) {
		val &= ~DAC_FIFOC_TX_SAMPLE_BITS;
	} else if (sc->sc_pparam.validbits == 24) {
		val |= DAC_FIFOC_TX_SAMPLE_BITS;
	}
	AC_WRITE(sc, AC_DAC_FIFOC, val);

	val = AC_READ(sc, AC_ADC_FIFOC);
	val |= ADC_FIFOC_EN_AD;
	val |= ADC_FIFOC_RX_FIFO_MODE;
	val &= ~ADC_FIFOC_FS;
	val |= __SHIFTIN(ADC_FS_48KHZ, ADC_FIFOC_FS);
	val &= ~ADC_FIFOC_RX_TRIG_LEVEL;
	val |= __SHIFTIN(AWINAC_RX_TRIG_LEVEL, ADC_FIFOC_RX_TRIG_LEVEL);
	val &= ~ADC_FIFOC_MONO_EN;
	if (sc->sc_rparam.validbits == 16) {
		val &= ~ADC_FIFOC_RX_SAMPLE_BITS;
	} else {
		val |= ADC_FIFOC_RX_SAMPLE_BITS;
	}
	AC_WRITE(sc, AC_ADC_FIFOC, val);

	return 0;
}

static int
awinac_halt_output(void *priv)
{
	struct awinac_softc *sc = priv;
	uint32_t val;

	awin_dma_halt(sc->sc_pdma);

	val = AC_READ(sc, AC_DAC_ACTL);
	val &= ~DAC_ACTL_DACAREN;
	val &= ~DAC_ACTL_DACALEN;
	val &= ~DAC_ACTL_DACPAS;
	AC_WRITE(sc, AC_DAC_ACTL, val);

	val = AC_READ(sc, AC_DAC_FIFOC);
	val &= ~DAC_FIFOC_DRQ_EN;
	AC_WRITE(sc, AC_DAC_FIFOC, val);

	sc->sc_pint = NULL;
	sc->sc_pintarg = NULL;

	return 0;
}

static int
awinac_halt_input(void *priv)
{
	struct awinac_softc *sc = priv;
	uint32_t val;

	awin_dma_halt(sc->sc_rdma);

	val = AC_READ(sc, AC_ADC_ACTL);
	val &= ~ADC_ACTL_ADCREN;
	val &= ~ADC_ACTL_ADCLEN;
	AC_WRITE(sc, AC_ADC_ACTL, val);

	val = AC_READ(sc, AC_ADC_FIFOC);
	val &= ~ADC_FIFOC_DRQ_EN;
	AC_WRITE(sc, AC_ADC_FIFOC, val);

	sc->sc_rint = NULL;
	sc->sc_rintarg = NULL;
	
	return 0;
}

static int
awinac_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct awinac_softc *sc = priv;
	uint32_t val;
	int nvol;

	switch (mc->dev) {
	case AC_OUTPUT_MASTER_VOLUME:
	case AC_INPUT_DAC_VOLUME:
		nvol = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] >> 2;
		val = AC_READ(sc, AC_DAC_ACTL);
		val &= ~DAC_ACTL_PAVOL;
		val |= __SHIFTIN(nvol, DAC_ACTL_PAVOL);
		AC_WRITE(sc, AC_DAC_ACTL, val);
		return 0;
	}

	return ENXIO;
}

static int
awinac_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct awinac_softc *sc = priv;
	uint32_t val;
	int nvol;

	switch (mc->dev) {
	case AC_OUTPUT_MASTER_VOLUME:
	case AC_INPUT_DAC_VOLUME:
		val = AC_READ(sc, AC_DAC_ACTL);
		nvol = __SHIFTOUT(val, DAC_ACTL_PAVOL) << 2;
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = nvol;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = nvol;
		return 0;
	}

	return ENXIO;
}

static int
awinac_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	switch (di->index) {
	case AC_OUTPUT_CLASS:
		di->mixer_class = AC_OUTPUT_CLASS;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case AC_INPUT_CLASS:
		di->mixer_class = AC_INPUT_CLASS;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case AC_OUTPUT_MASTER_VOLUME:
		di->mixer_class = AC_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmaster);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case AC_INPUT_DAC_VOLUME:
		di->mixer_class = AC_INPUT_CLASS;
		strcpy(di->label.name, AudioNdac);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	}

	return ENXIO;
}

static void *
awinac_allocm(void *priv, int dir, size_t size)
{
	struct awinac_softc *sc = priv;
	struct awinac_dma *dma;
	int error;

	dma = kmem_alloc(sizeof(*dma), KM_SLEEP);
	if (dma == NULL)
		return NULL;

	error = awinac_allocdma(sc, size, 16, dma);
	if (error) {
		kmem_free(dma, sizeof(*dma));
		device_printf(sc->sc_dev, "couldn't allocate DMA memory (%d)\n",
		    error);
		return NULL;
	}

	LIST_INSERT_HEAD(&sc->sc_dmalist, dma, dma_list);

	return dma->dma_addr;
}

static void
awinac_freem(void *priv, void *addr, size_t size)
{
	struct awinac_softc *sc = priv;
	struct awinac_dma *dma;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list) {
		if (dma->dma_addr == addr) {
			awinac_freedma(sc, dma);
			LIST_REMOVE(dma, dma_list);
			kmem_free(dma, sizeof(*dma));
			break;
		}
	}
}

static paddr_t
awinac_mappage(void *priv, void *addr, off_t off, int prot)
{
	struct awinac_softc *sc = priv;
	struct awinac_dma *dma;

	if (off < 0)
		return -1;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list) {
		if (dma->dma_addr == addr) {
			return bus_dmamem_mmap(sc->sc_dmat, dma->dma_segs,
			    dma->dma_nsegs, off, prot, BUS_DMA_WAITOK);
		}
	}

	return -1;
}

static int
awinac_getdev(void *priv, struct audio_device *audiodev)
{
	snprintf(audiodev->name, sizeof(audiodev->name), "AllWinner");
	snprintf(audiodev->version, sizeof(audiodev->version), "CODEC");
	snprintf(audiodev->config, sizeof(audiodev->config), "awinac");
	return 0;
}

static int
awinac_get_props(void *priv)
{
	return AUDIO_PROP_PLAYBACK|AUDIO_PROP_CAPTURE|
	       AUDIO_PROP_INDEPENDENT|AUDIO_PROP_MMAP|
	       AUDIO_PROP_FULLDUPLEX;
}

static int
awinac_round_blocksize(void *priv, int bs, int mode,
    const audio_params_t *params)
{
	return bs & ~3;
}

static size_t
awinac_round_buffersize(void *priv, int direction, size_t bufsize)
{
	return bufsize;
}

static int
awinac_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct awinac_softc *sc = priv;
	struct awinac_dma *dma;
	bus_addr_t pstart;
	bus_size_t psize;
	uint32_t val, dmacfg;
	int error;

	pstart = 0;
	psize = (uintptr_t)end - (uintptr_t)start;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list) {
		if (dma->dma_addr == start) {
			pstart = dma->dma_map->dm_segs[0].ds_addr;
			break;
		}
	}
	if (pstart == 0) {
		device_printf(sc->sc_dev, "bad addr %p\n", start);
		return EINVAL;
	}

	val = AC_READ(sc, AC_DAC_FIFOC);
	val |= DAC_FIFOC_FIFO_FLUSH;
	AC_WRITE(sc, AC_DAC_FIFOC, val);

	val = AC_READ(sc, AC_DAC_ACTL);
	val |= DAC_ACTL_DACAREN;
	val |= DAC_ACTL_DACALEN;
	val |= DAC_ACTL_DACPAS;
	AC_WRITE(sc, AC_DAC_ACTL, val);

	sc->sc_pint = intr;
	sc->sc_pintarg = intrarg;
	sc->sc_pstart = sc->sc_pcur = pstart;
	sc->sc_pend = sc->sc_pstart + psize;
	sc->sc_pblksize = blksize;

	dmacfg = 0;
	dmacfg |= __SHIFTIN(AWIN_DMA_CTL_DATA_WIDTH_16,
			    AWIN_DMA_CTL_DST_DATA_WIDTH);
	dmacfg |= __SHIFTIN(AWIN_DMA_CTL_BURST_LEN_4,
			    AWIN_DMA_CTL_DST_BURST_LEN);
	dmacfg |= __SHIFTIN(AWIN_DMA_CTL_DATA_WIDTH_16,
			    AWIN_DMA_CTL_SRC_DATA_WIDTH);
	dmacfg |= __SHIFTIN(AWIN_DMA_CTL_BURST_LEN_4,
			    AWIN_DMA_CTL_SRC_BURST_LEN);
	dmacfg |= AWIN_DMA_CTL_BC_REMAINING;
	dmacfg |= AWIN_NDMA_CTL_DST_ADDR_NOINCR;
	dmacfg |= __SHIFTIN(AWIN_NDMA_CTL_DRQ_CODEC,
			    AWIN_DMA_CTL_DST_DRQ_TYPE);
	dmacfg |= __SHIFTIN(AWIN_NDMA_CTL_DRQ_SDRAM,
			    AWIN_DMA_CTL_SRC_DRQ_TYPE);
	awin_dma_set_config(sc->sc_pdma, dmacfg);

	val = AC_READ(sc, AC_DAC_FIFOC);
	val |= DAC_FIFOC_DRQ_EN;
	AC_WRITE(sc, AC_DAC_FIFOC, val);

	error = awinac_play(sc);
	if (error)
		awinac_halt_output(sc);

	return error;
}

static int
awinac_trigger_input(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct awinac_softc *sc = priv;
	struct awinac_dma *dma;
	bus_addr_t rstart;
	bus_size_t rsize;
	uint32_t val, dmacfg;
	int error;

	rstart = 0;
	rsize = (uintptr_t)end - (uintptr_t)start;

	LIST_FOREACH(dma, &sc->sc_dmalist, dma_list) {
		if (dma->dma_addr == start) {
			rstart = dma->dma_map->dm_segs[0].ds_addr;
			break;
		}
	}
	if (rstart == 0) {
		device_printf(sc->sc_dev, "bad addr %p\n", start);
		return EINVAL;
	}

	val = AC_READ(sc, AC_ADC_FIFOC);
	val |= ADC_FIFOC_FIFO_FLUSH;
	AC_WRITE(sc, AC_ADC_FIFOC, val);

	val = AC_READ(sc, AC_ADC_ACTL);
	val |= ADC_ACTL_ADCREN;
	val |= ADC_ACTL_ADCLEN;
	AC_WRITE(sc, AC_ADC_ACTL, val);

	sc->sc_rint = intr;
	sc->sc_rintarg = intrarg;
	sc->sc_rstart = sc->sc_rcur = rstart;
	sc->sc_rend = sc->sc_rstart + rsize;
	sc->sc_rblksize = blksize;

	dmacfg = 0;
	dmacfg |= __SHIFTIN(AWIN_DMA_CTL_DATA_WIDTH_16,
			    AWIN_DMA_CTL_DST_DATA_WIDTH);
	dmacfg |= __SHIFTIN(AWIN_DMA_CTL_BURST_LEN_4,
			    AWIN_DMA_CTL_DST_BURST_LEN);
	dmacfg |= __SHIFTIN(AWIN_DMA_CTL_DATA_WIDTH_16,
			    AWIN_DMA_CTL_SRC_DATA_WIDTH);
	dmacfg |= __SHIFTIN(AWIN_DMA_CTL_BURST_LEN_4,
			    AWIN_DMA_CTL_SRC_BURST_LEN);
	dmacfg |= AWIN_DMA_CTL_BC_REMAINING;
	dmacfg |= AWIN_NDMA_CTL_SRC_ADDR_NOINCR;
	dmacfg |= __SHIFTIN(AWIN_NDMA_CTL_DRQ_SDRAM,
			    AWIN_DMA_CTL_DST_DRQ_TYPE);
	dmacfg |= __SHIFTIN(AWIN_NDMA_CTL_DRQ_CODEC,
			    AWIN_DMA_CTL_SRC_DRQ_TYPE);
	awin_dma_set_config(sc->sc_rdma, dmacfg);

	val = AC_READ(sc, AC_ADC_FIFOC);
	val |= ADC_FIFOC_DRQ_EN;
	AC_WRITE(sc, AC_ADC_FIFOC, val);

	error = awinac_rec(sc);
	if (error)
		awinac_halt_input(sc);

	return error;
}

static void
awinac_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct awinac_softc *sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

#if defined(DDB)
void
awinac_dump_regs(void)
{
	struct awinac_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("awinac", 0);
	if (dev == NULL)
		return;
	sc = device_private(dev);

	printf("DAC_DPC:       %08X\n", AC_READ(sc, AC_DAC_DPC));
	printf("DAC_FIFOC:     %08X\n", AC_READ(sc, AC_DAC_FIFOC));
	printf("DAC_FIFOS:     %08X\n", AC_READ(sc, AC_DAC_FIFOS));
	printf("DAC_TXDATA:    ...\n");
	printf("DAC_ACTL:      %08X\n", AC_READ(sc, AC_DAC_ACTL));
	printf("DAC_TUNE:      %08X\n", AC_READ(sc, AC_DAC_TUNE));
	printf("ADC_FIFOC:     %08X\n", AC_READ(sc, AC_ADC_FIFOC));
	printf("ADC_FIFOS:     %08X\n", AC_READ(sc, AC_ADC_FIFOS));
	printf("ADC_RXDATA:    ...\n");
	printf("ADC_ACTL:      %08X\n", AC_READ(sc, AC_ADC_ACTL));
	printf("DAC_CNT:       %08X\n", AC_READ(sc, AC_DAC_CNT));
	printf("ADC_CNT:       %08X\n", AC_READ(sc, AC_ADC_CNT));
	printf("DAC_CAL:       %08X\n", AC_READ(sc, AC_DAC_CAL));
	printf("MIC_PHONE_CAL: %08X\n", AC_READ(sc, AC_MIC_PHONE_CAL));
}
#endif
