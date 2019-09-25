/* $NetBSD: if_bwfm_sdio.c,v 1.6 2019/09/25 16:21:14 mlelstv Exp $ */
/* $OpenBSD: if_bwfm_sdio.c,v 1.1 2017/10/11 17:19:50 patrick Exp $ */
/*
 * Copyright (c) 2010-2016 Broadcom Corporation
 * Copyright (c) 2016,2017 Patrick Wildt <patrick@blueri.se>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/endian.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/mutex.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <netinet/in.h>

#include <dev/firmload.h>

#include <net80211/ieee80211_var.h>

#include <dev/sdmmc/sdmmcdevs.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <dev/ic/bwfmvar.h>
#include <dev/ic/bwfmreg.h>
#include <dev/sdmmc/if_bwfm_sdio.h>

#ifdef BWFM_DEBUG
#define DPRINTF(x)	do { if (bwfm_debug > 0) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (bwfm_debug >= (n)) printf x; } while (0)
static int bwfm_debug = 2;
#else
#define DPRINTF(x)	do { ; } while (0)
#define DPRINTFN(n, x)	do { ; } while (0)
#endif

#define DEVNAME(sc)	device_xname((sc)->sc_sc.sc_dev)

enum bwfm_sdio_clkstate {
	CLK_NONE,
	CLK_SDONLY,
	CLK_PENDING,
	CLK_AVAIL
};

struct bwfm_sdio_softc {
	struct bwfm_softc	sc_sc;
	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;

	bool			sc_bwfm_attached;

	struct sdmmc_function	**sc_sf;
	size_t			sc_sf_size;

	uint32_t		sc_bar0;
	enum bwfm_sdio_clkstate	sc_clkstate;
	bool			sc_sr_enabled;
	bool			sc_alp_only;
	bool			sc_sleeping;

	struct sdmmc_task	sc_task;
	bool			sc_task_queued;

	uint8_t			sc_tx_seq;
	uint8_t			sc_tx_max_seq;
	int			sc_tx_count;
	MBUFQ_HEAD()		sc_tx_queue;

	struct mbuf		*sc_rxctl_queue;
	kcondvar_t		sc_rxctl_cv;

	void			*sc_ih;
	struct bwfm_core	*sc_cc;

	char			*sc_bounce_buf;
	size_t			sc_bounce_size;

	uint32_t		sc_console_addr;
	char			*sc_console_buf;
	size_t			sc_console_buf_size;
	uint32_t		sc_console_readidx;
};

int		bwfm_sdio_match(device_t, cfdata_t, void *);
void		bwfm_sdio_attach(device_t, struct device *, void *);
int		bwfm_sdio_detach(device_t, int);
void		bwfm_sdio_attachhook(device_t);

void		bwfm_sdio_backplane(struct bwfm_sdio_softc *, uint32_t);
uint8_t		bwfm_sdio_read_1(struct bwfm_sdio_softc *, uint32_t);
uint32_t	bwfm_sdio_read_4(struct bwfm_sdio_softc *, uint32_t);
void		bwfm_sdio_write_1(struct bwfm_sdio_softc *, uint32_t,
		     uint8_t);
void		bwfm_sdio_write_4(struct bwfm_sdio_softc *, uint32_t,
		     uint32_t);

uint32_t	bwfm_sdio_dev_read(struct bwfm_sdio_softc *, uint32_t);
void		bwfm_sdio_dev_write(struct bwfm_sdio_softc *, uint32_t,
		     uint32_t);

uint32_t	bwfm_sdio_buscore_read(struct bwfm_softc *, uint32_t);
void		bwfm_sdio_buscore_write(struct bwfm_softc *, uint32_t,
		     uint32_t);
int		bwfm_sdio_buscore_prepare(struct bwfm_softc *);
void		bwfm_sdio_buscore_activate(struct bwfm_softc *, uint32_t);

int		bwfm_sdio_buf_read(struct bwfm_sdio_softc *,
		    struct sdmmc_function *, uint32_t, char *, size_t);
int		bwfm_sdio_buf_write(struct bwfm_sdio_softc *,
		    struct sdmmc_function *, uint32_t, char *, size_t);

struct mbuf	*bwfm_sdio_newbuf(void);
void		bwfm_qput(struct mbuf **, struct mbuf *);
struct mbuf	*bwfm_qget(struct mbuf **);

uint32_t	bwfm_sdio_ram_read_write(struct bwfm_sdio_softc *,
		    uint32_t, char *, size_t, int);
uint32_t	bwfm_sdio_frame_read_write(struct bwfm_sdio_softc *,
		    char *, size_t, int);

int		bwfm_sdio_intr(void *);
void		bwfm_sdio_task(void *);
void		bwfm_sdio_task1(struct bwfm_sdio_softc *);

int		bwfm_nvram_convert(u_char *, size_t, size_t *);
int		bwfm_sdio_load_microcode(struct bwfm_sdio_softc *,
		    u_char *, size_t, u_char *, size_t);
void		bwfm_sdio_clkctl(struct bwfm_sdio_softc *,
		    enum bwfm_sdio_clkstate, bool);
void		bwfm_sdio_htclk(struct bwfm_sdio_softc *, bool, bool);

int		bwfm_sdio_bus_sleep(struct bwfm_sdio_softc *, bool, bool);
void		bwfm_sdio_readshared(struct bwfm_sdio_softc *);

int		bwfm_sdio_txcheck(struct bwfm_softc *);
int		bwfm_sdio_txdata(struct bwfm_softc *, struct mbuf **);
int		bwfm_sdio_txctl(struct bwfm_softc *, char *, size_t);
int		bwfm_sdio_rxctl(struct bwfm_softc *, char *, size_t *);

int		bwfm_sdio_tx_ok(struct bwfm_sdio_softc *);
void		bwfm_sdio_tx_frames(struct bwfm_sdio_softc *);
void		bwfm_sdio_tx_ctrlframe(struct bwfm_sdio_softc *,
		    struct mbuf *);
void		bwfm_sdio_tx_dataframe(struct bwfm_sdio_softc *,
		    struct mbuf *);

void            bwfm_sdio_rx_frames(struct bwfm_sdio_softc *);
void            bwfm_sdio_rx_glom(struct bwfm_sdio_softc *,
		    uint16_t *, int, uint16_t *);

#ifdef BWFM_DEBUG 
void		bwfm_sdio_debug_console(struct bwfm_sdio_softc *);
#endif 

struct bwfm_bus_ops bwfm_sdio_bus_ops = {
	.bs_init = NULL,
	.bs_stop = NULL,
	.bs_txcheck = bwfm_sdio_txcheck,
	.bs_txdata = bwfm_sdio_txdata,
	.bs_txctl = bwfm_sdio_txctl,
	.bs_rxctl = bwfm_sdio_rxctl,
};

struct bwfm_buscore_ops bwfm_sdio_buscore_ops = {
	.bc_read = bwfm_sdio_buscore_read,
	.bc_write = bwfm_sdio_buscore_write,
	.bc_prepare = bwfm_sdio_buscore_prepare,
	.bc_reset = NULL,
	.bc_setup = NULL,
	.bc_activate = bwfm_sdio_buscore_activate,
};

CFATTACH_DECL_NEW(bwfm_sdio, sizeof(struct bwfm_sdio_softc),
    bwfm_sdio_match, bwfm_sdio_attach, bwfm_sdio_detach, NULL);

static const struct bwfm_sdio_product {
	uint32_t	manufacturer;
	uint32_t	product;
	const char	*cisinfo[4];
} bwfm_sdio_products[] = {
	{
		SDMMC_VENDOR_BROADCOM,
		SDMMC_PRODUCT_BROADCOM_BCM4330, 
		SDMMC_CIS_BROADCOM_BCM4330
	},
	{
		SDMMC_VENDOR_BROADCOM,
		SDMMC_PRODUCT_BROADCOM_BCM4334, 
		SDMMC_CIS_BROADCOM_BCM4334
	},
	{
		SDMMC_VENDOR_BROADCOM,
		SDMMC_PRODUCT_BROADCOM_BCM43143, 
		SDMMC_CIS_BROADCOM_BCM43143
	},
	{
		SDMMC_VENDOR_BROADCOM,
		SDMMC_PRODUCT_BROADCOM_BCM43430, 
		SDMMC_CIS_BROADCOM_BCM43430
	},
};

int
bwfm_sdio_match(device_t parent, cfdata_t match, void *aux)
{
	struct sdmmc_attach_args *saa = aux;
	struct sdmmc_function *sf = saa->sf;
	struct sdmmc_cis *cis;
	const struct bwfm_sdio_product *bsp;
	int i;

	/* Not SDIO. */
	if (sf == NULL)
		return 0;

	cis = &sf->sc->sc_fn0->cis;
	for (i = 0; i < __arraycount(bwfm_sdio_products); ++i) {
		bsp = &bwfm_sdio_products[i];
		if (cis->manufacturer == bsp->manufacturer &&
		    cis->product == bsp->product)
			break;
	}
	if (i >= __arraycount(bwfm_sdio_products))
		return 0;

	/* We need both functions, but ... */
	if (sf->sc->sc_function_count <= 1)
		return 0;

	/* ... only attach for one. */
	if (sf->number != 1)
		return 0;

	return 1;
}

void
bwfm_sdio_attach(device_t parent, device_t self, void *aux)
{
	struct bwfm_sdio_softc *sc = device_private(self);
	struct sdmmc_attach_args *saa = aux;
	struct sdmmc_function *sf = saa->sf;
	struct bwfm_core *core;
	uint32_t reg;

	sc->sc_sc.sc_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_rxctl_cv, "bwfmctl");
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_NONE);

	sdmmc_init_task(&sc->sc_task, bwfm_sdio_task, sc);
	sc->sc_task_queued = false;

	sc->sc_bounce_size = 64 * 1024;
	sc->sc_bounce_buf = kmem_alloc(sc->sc_bounce_size, KM_SLEEP);
	sc->sc_tx_seq = 0xff;
	MBUFQ_INIT(&sc->sc_tx_queue);
	sc->sc_rxctl_queue = NULL;

	sc->sc_sf_size = (sf->sc->sc_function_count + 1)
	    * sizeof(struct sdmmc_function *);
	sc->sc_sf = kmem_zalloc(sc->sc_sf_size, KM_SLEEP);

	/* Copy all function pointers. */
	SIMPLEQ_FOREACH(sf, &saa->sf->sc->sf_head, sf_list) {
		sc->sc_sf[sf->number] = sf;
	}

	sdmmc_io_set_blocklen(sc->sc_sf[1]->sc, sc->sc_sf[1], 64);
	sdmmc_io_set_blocklen(sc->sc_sf[2]->sc, sc->sc_sf[2], 512);

        /* Enable Function 1. */
        if (sdmmc_io_function_enable(sc->sc_sf[1]) != 0) {
                printf("%s: cannot enable function 1\n", DEVNAME(sc));
                return;
        }

        DPRINTF(("%s: F1 signature read @0x18000000=%x\n", DEVNAME(sc),
            bwfm_sdio_read_4(sc, 0x18000000)));

	/* Force PLL off */
	bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR,
	    BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HW_CLKREQ_OFF |
	    BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL_REQ);

	sc->sc_sc.sc_buscore_ops = &bwfm_sdio_buscore_ops;
	if (bwfm_chip_attach(&sc->sc_sc) != 0) {
		aprint_error_dev(self, "cannot attach chip\n");
		return;
	}

	sc->sc_cc = bwfm_chip_get_core(&sc->sc_sc, BWFM_AGENT_CORE_CHIPCOMMON);
	if (sc->sc_cc == NULL) {
		aprint_error_dev(self, "cannot find chipcommon core\n");
		return;
	}

	core = bwfm_chip_get_core(&sc->sc_sc, BWFM_AGENT_CORE_SDIO_DEV);
	if (core->co_rev >= 12) {
		reg = bwfm_sdio_read_1(sc, BWFM_SDIO_FUNC1_SLEEPCSR);
		if ((reg & BWFM_SDIO_FUNC1_SLEEPCSR_KSO) == 0) {
			reg |= BWFM_SDIO_FUNC1_SLEEPCSR_KSO;
			bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_SLEEPCSR, reg);
		}
	}

	/* TODO: drive strength */

	bwfm_sdio_write_1(sc, BWFM_SDIO_CCCR_CARDCTRL,
	    bwfm_sdio_read_1(sc, BWFM_SDIO_CCCR_CARDCTRL) |
	    BWFM_SDIO_CCCR_CARDCTRL_WLANRESET);

	core = bwfm_chip_get_pmu(&sc->sc_sc);
	bwfm_sdio_write_4(sc, core->co_base + BWFM_CHIP_REG_PMUCONTROL,
	    bwfm_sdio_read_4(sc, core->co_base + BWFM_CHIP_REG_PMUCONTROL) |
	    (BWFM_CHIP_REG_PMUCONTROL_RES_RELOAD <<
	     BWFM_CHIP_REG_PMUCONTROL_RES_SHIFT));

	sdmmc_io_function_disable(sc->sc_sf[2]);

	bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR, 0);
	sc->sc_clkstate = CLK_SDONLY;

	config_mountroot(self, bwfm_sdio_attachhook);
}

void
bwfm_sdio_attachhook(device_t self)
{
	struct bwfm_sdio_softc *sc = device_private(self);
	struct bwfm_softc *bwfm = &sc->sc_sc;
	firmware_handle_t fwh;
	const char *name, *nvname;
	u_char *ucode, *nvram;
	size_t size, nvlen, nvsize;
	uint32_t reg, clk;
	int error;

	DPRINTF(("%s: chip 0x%08x rev %u\n", DEVNAME(sc),
	    bwfm->sc_chip.ch_chip, bwfm->sc_chip.ch_chiprev));
	switch (bwfm->sc_chip.ch_chip) {
	case BRCM_CC_4330_CHIP_ID:
		name = "brcmfmac4330-sdio.bin";
		nvname = "brcmfmac4330-sdio.txt";
		break;
	case BRCM_CC_4334_CHIP_ID:
		name = "brcmfmac4334-sdio.bin";
		nvname = "brcmfmac4334-sdio.txt";
		break;
	case BRCM_CC_4345_CHIP_ID:
		name = "brcmfmac43455-sdio.bin";
		nvname = "brcmfmac43455-sdio.txt";
		break;
	case BRCM_CC_43340_CHIP_ID:
		name = "brcmfmac43340-sdio.bin";
		nvname = "brcmfmac43340-sdio.txt";
		break;
	case BRCM_CC_4335_CHIP_ID:
		if (bwfm->sc_chip.ch_chiprev < 2) {
			name = "brcmfmac4335-sdio.bin";
			nvname = "brcmfmac4335-sdio.txt";
		} else {
			name = "brcmfmac4339-sdio.bin";
			nvname = "brcmfmac4339-sdio.txt";
		}
		break;
	case BRCM_CC_4339_CHIP_ID:
		name = "brcmfmac4339-sdio.bin";
		nvname = "brcmfmac4339-sdio.txt";
		break;
	case BRCM_CC_43430_CHIP_ID:
		if (bwfm->sc_chip.ch_chiprev == 0) {
			name = "brcmfmac43430a0-sdio.bin";
			nvname = "brcmfmac43430a0-sdio.txt";
		} else {
			name = "brcmfmac43430-sdio.bin";
			nvname = "brcmfmac43430-sdio.txt";
		}
		break;
	case BRCM_CC_4356_CHIP_ID:
		name = "brcmfmac4356-sdio.bin";
		nvname = "brcmfmac4356-sdio.txt";
		break;
	default:
		printf("%s: unknown firmware for chip %s\n",
		    DEVNAME(sc), bwfm->sc_chip.ch_name);
		goto err;
	}

	if (firmware_open("if_bwfm", name, &fwh) != 0) {
		printf("%s: failed firmware_open of file %s\n",
		    DEVNAME(sc), name);
		goto err;
	}
	size = firmware_get_size(fwh);
	ucode = firmware_malloc(size);
	if (ucode == NULL) {
		printf("%s: failed firmware_open of file %s\n",
		    DEVNAME(sc), name);
		firmware_close(fwh);
		goto err;
	}
	error = firmware_read(fwh, 0, ucode, size);
	firmware_close(fwh);
	if (error != 0) {
		printf("%s: failed to read firmware (error %d)\n",
		    DEVNAME(sc), error);
		goto err1;
	}

	if (firmware_open("if_bwfm", nvname, &fwh) != 0) {
		printf("%s: failed firmware_open of file %s\n",
		    DEVNAME(sc), nvname);
		goto err1;
	}
	nvlen = firmware_get_size(fwh);
	nvram = firmware_malloc(nvlen);
	if (nvram == NULL) {
		printf("%s: failed firmware_open of file %s\n",
		    DEVNAME(sc), name);
		firmware_close(fwh);
		goto err1;
	}
	error = firmware_read(fwh, 0, nvram, nvlen);
	firmware_close(fwh);
	if (error != 0) {
		printf("%s: failed to read firmware (error %d)\n",
		    DEVNAME(sc), error);
		goto err2;
	}

	if (bwfm_nvram_convert(nvram, nvlen, &nvsize)) {
		printf("%s: failed to convert nvram\n", DEVNAME(sc));
		goto err2;
	}

	sc->sc_alp_only = true;
	if (bwfm_sdio_load_microcode(sc, ucode, size, nvram, nvsize) != 0) {
		printf("%s: could not load microcode\n",
		    DEVNAME(sc));
		goto err2;
	}
	sc->sc_alp_only = false;

	firmware_free(nvram, nvlen);
	firmware_free(ucode, size);

	bwfm_sdio_clkctl(sc, CLK_AVAIL, false);
	if (sc->sc_clkstate != CLK_AVAIL) {
		printf("%s: could not access clock\n",
		    DEVNAME(sc));
		goto err;
	}

	clk = bwfm_sdio_read_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR);
	bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR,
	    clk | BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HT);

	bwfm_sdio_dev_write(sc, SDPCMD_TOSBMAILBOXDATA,
	    SDPCM_PROT_VERSION << SDPCM_PROT_VERSION_SHIFT);
	if (sdmmc_io_function_enable(sc->sc_sf[2])) {
                printf("%s: cannot enable function 2\n", DEVNAME(sc));
                goto err;
	}

	bwfm_sdio_dev_write(sc, SDPCMD_HOSTINTMASK,
	    SDPCMD_INTSTATUS_HMB_SW_MASK | SDPCMD_INTSTATUS_CHIPACTIVE);
	bwfm_sdio_write_1(sc, BWFM_SDIO_WATERMARK, 8);

	if (bwfm_chip_sr_capable(bwfm)) {
		reg = bwfm_sdio_read_1(sc, BWFM_SDIO_FUNC1_WAKEUPCTRL);
		reg |= BWFM_SDIO_FUNC1_WAKEUPCTRL_HTWAIT;
		bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_WAKEUPCTRL, reg);
		bwfm_sdio_write_1(sc, BWFM_SDIO_CCCR_CARDCAP,
		    BWFM_SDIO_CCCR_CARDCAP_CMD14_SUPPORT |
		    BWFM_SDIO_CCCR_CARDCAP_CMD14_EXT);
		bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR,
		    BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HT);
		sc->sc_sr_enabled = 1;
	} else {
		bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR, clk);
	}

	sc->sc_ih = sdmmc_intr_establish(sc->sc_sc.sc_dev->dv_parent,
		bwfm_sdio_intr, sc, DEVNAME(sc));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "could not establish interrupt\n");
		bwfm_sdio_clkctl(sc, CLK_NONE, false);
		return;
	}
	sdmmc_intr_enable(sc->sc_sf[1]);

	sc->sc_sc.sc_bus_ops = &bwfm_sdio_bus_ops;
	sc->sc_sc.sc_proto_ops = &bwfm_proto_bcdc_ops;
	bwfm_attach(&sc->sc_sc);
	sc->sc_bwfm_attached = true;

	return;

err2:
	firmware_free(nvram, nvlen);
err1:
	firmware_free(ucode, size);
err:
	return;
}

int
bwfm_sdio_detach(struct device *self, int flags)
{
	struct bwfm_sdio_softc *sc = (struct bwfm_sdio_softc *)self;

#ifdef BWFM_DEBUG
	bwfm_sdio_debug_console(sc);
#endif

	if (sc->sc_ih) {
		sdmmc_intr_disable(sc->sc_sf[1]);
		sdmmc_intr_disestablish(sc->sc_ih);
	}
	if (sc->sc_bwfm_attached)
		bwfm_detach(&sc->sc_sc, flags);

	kmem_free(sc->sc_sf, sc->sc_sf_size);
	kmem_free(sc->sc_bounce_buf, sc->sc_bounce_size);

	mutex_destroy(&sc->sc_intr_lock);
	cv_destroy(&sc->sc_rxctl_cv);
	mutex_destroy(&sc->sc_lock);

	return 0;
}

void
bwfm_sdio_backplane(struct bwfm_sdio_softc *sc, uint32_t addr)
{
	uint32_t bar0 = addr & ~BWFM_SDIO_SB_OFT_ADDR_MASK;

	if (sc->sc_bar0 == bar0)
		return;

	bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_SBADDRLOW,
	    (bar0 >>  8) & 0xff);
	bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_SBADDRMID,
	    (bar0 >> 16) & 0xff);
	bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_SBADDRHIGH,
	    (bar0 >> 24) & 0xff);
	sc->sc_bar0 = bar0;
}

uint8_t
bwfm_sdio_read_1(struct bwfm_sdio_softc *sc, uint32_t addr)
{
	struct sdmmc_function *sf;
	uint8_t rv;

	/*
	 * figure out how to read the register based on address range
	 * 0x00 ~ 0x7FF: function 0 CCCR and FBR
	 * 0x10000 ~ 0x1FFFF: function 1 miscellaneous registers
	 * The rest: function 1 silicon backplane core registers
	 */
	if ((addr & ~0x7ff) == 0)
		sf = sc->sc_sf[0];
	else
		sf = sc->sc_sf[1];

	rv = sdmmc_io_read_1(sf, addr);
	return rv;
}

uint32_t
bwfm_sdio_read_4(struct bwfm_sdio_softc *sc, uint32_t addr)
{
	struct sdmmc_function *sf;
	uint32_t rv;

	bwfm_sdio_backplane(sc, addr);

	addr &= BWFM_SDIO_SB_OFT_ADDR_MASK;
	addr |= BWFM_SDIO_SB_ACCESS_2_4B_FLAG;

	/*
	 * figure out how to read the register based on address range
	 * 0x00 ~ 0x7FF: function 0 CCCR and FBR
	 * 0x10000 ~ 0x1FFFF: function 1 miscellaneous registers
	 * The rest: function 1 silicon backplane core registers
	 */
	if ((addr & ~0x7ff) == 0)
		sf = sc->sc_sf[0];
	else
		sf = sc->sc_sf[1];

	rv = sdmmc_io_read_4(sf, addr);
	return rv;
}

void
bwfm_sdio_write_1(struct bwfm_sdio_softc *sc, uint32_t addr, uint8_t data)
{
	struct sdmmc_function *sf;

	/*
	 * figure out how to read the register based on address range
	 * 0x00 ~ 0x7FF: function 0 CCCR and FBR
	 * 0x10000 ~ 0x1FFFF: function 1 miscellaneous registers
	 * The rest: function 1 silicon backplane core registers
	 */
	if ((addr & ~0x7ff) == 0)
		sf = sc->sc_sf[0];
	else
		sf = sc->sc_sf[1];

	sdmmc_io_write_1(sf, addr, data);
}

void
bwfm_sdio_write_4(struct bwfm_sdio_softc *sc, uint32_t addr, uint32_t data)
{
	struct sdmmc_function *sf;

	bwfm_sdio_backplane(sc, addr);

	addr &= BWFM_SDIO_SB_OFT_ADDR_MASK;
	addr |= BWFM_SDIO_SB_ACCESS_2_4B_FLAG;

	/*
	 * figure out how to read the register based on address range
	 * 0x00 ~ 0x7FF: function 0 CCCR and FBR
	 * 0x10000 ~ 0x1FFFF: function 1 miscellaneous registers
	 * The rest: function 1 silicon backplane core registers
	 */
	if ((addr & ~0x7ff) == 0)
		sf = sc->sc_sf[0];
	else
		sf = sc->sc_sf[1];

	sdmmc_io_write_4(sf, addr, data);
}

int
bwfm_sdio_buf_read(struct bwfm_sdio_softc *sc, struct sdmmc_function *sf,
    uint32_t reg, char *data, size_t size)
{
	int err;

	KASSERT(((vaddr_t)data & 0x3) == 0);
	KASSERT((size & 0x3) == 0);

	if (sf == sc->sc_sf[1])
		err = sdmmc_io_read_region_1(sf, reg, data, size);
	else
		err = sdmmc_io_read_multi_1(sf, reg, data, size);

	if (err)
		printf("%s: error %d\n", __func__, err);

	return err;
}

int
bwfm_sdio_buf_write(struct bwfm_sdio_softc *sc, struct sdmmc_function *sf,
    uint32_t reg, char *data, size_t size)
{
	int err;

	KASSERT(((vaddr_t)data & 0x3) == 0);
	KASSERT((size & 0x3) == 0);

	err = sdmmc_io_write_region_1(sf, reg, data, size);

	if (err)
		printf("%s: error %d\n", __func__, err);

	return err;
}

uint32_t
bwfm_sdio_ram_read_write(struct bwfm_sdio_softc *sc, uint32_t reg,
    char *data, size_t left, int write)
{
	uint32_t sbaddr, sdaddr, off;
	size_t size;
	int err;

	err = off = 0;
	while (left > 0) {
		sbaddr = reg + off;
		bwfm_sdio_backplane(sc, sbaddr);

		sdaddr = sbaddr & BWFM_SDIO_SB_OFT_ADDR_MASK;
		size = ulmin(left, (BWFM_SDIO_SB_OFT_ADDR_PAGE - sdaddr));
		sdaddr |= BWFM_SDIO_SB_ACCESS_2_4B_FLAG;

		if (write) {
			memcpy(sc->sc_bounce_buf, data + off, size);
			if (roundup(size, 4) != size)
				memset(sc->sc_bounce_buf + size, 0,
				    roundup(size, 4) - size);
			err = bwfm_sdio_buf_write(sc, sc->sc_sf[1], sdaddr,
			    sc->sc_bounce_buf, roundup(size, 4));
		} else {
			err = bwfm_sdio_buf_read(sc, sc->sc_sf[1], sdaddr,
			    sc->sc_bounce_buf, roundup(size, 4));
			memcpy(data + off, sc->sc_bounce_buf, size);
		}
		if (err)
			break;

		off += size;
		left -= size;
	}

	return err;
}

uint32_t
bwfm_sdio_frame_read_write(struct bwfm_sdio_softc *sc,
    char *data, size_t size, int write)
{
	uint32_t addr;
	int err;

	addr = sc->sc_cc->co_base;
	bwfm_sdio_backplane(sc, addr);

	addr &= BWFM_SDIO_SB_OFT_ADDR_MASK;
	addr |= BWFM_SDIO_SB_ACCESS_2_4B_FLAG;

	if (write)
		err = bwfm_sdio_buf_write(sc, sc->sc_sf[2], addr, data, size);
	else
		err = bwfm_sdio_buf_read(sc, sc->sc_sf[2], addr, data, size);

	return err;
}

uint32_t
bwfm_sdio_dev_read(struct bwfm_sdio_softc *sc, uint32_t reg)
{
	struct bwfm_core *core;
	uint32_t val;

	core = bwfm_chip_get_core(&sc->sc_sc, BWFM_AGENT_CORE_SDIO_DEV);
	val = bwfm_sdio_read_4(sc, core->co_base + reg);
	/* TODO: Workaround for 4335/4339 */

	return val;
}

void
bwfm_sdio_dev_write(struct bwfm_sdio_softc *sc, uint32_t reg, uint32_t val)
{
	struct bwfm_core *core;

	core = bwfm_chip_get_core(&sc->sc_sc, BWFM_AGENT_CORE_SDIO_DEV);
	bwfm_sdio_write_4(sc, core->co_base + reg, val);
}

uint32_t
bwfm_sdio_buscore_read(struct bwfm_softc *bwfm, uint32_t reg)
{
	struct bwfm_sdio_softc *sc = (void *)bwfm;
	uint32_t val;

	mutex_enter(&sc->sc_lock);
	val = bwfm_sdio_read_4(sc, reg);
	/* TODO: Workaround for 4335/4339 */
	mutex_exit(&sc->sc_lock);

	return val;
}

void
bwfm_sdio_buscore_write(struct bwfm_softc *bwfm, uint32_t reg, uint32_t val)
{
	struct bwfm_sdio_softc *sc = (void *)bwfm;

	mutex_enter(&sc->sc_lock);
	bwfm_sdio_write_4(sc, reg, val);
	mutex_exit(&sc->sc_lock);
}

int
bwfm_sdio_buscore_prepare(struct bwfm_softc *bwfm)
{
	struct bwfm_sdio_softc *sc = (void *)bwfm;
	uint8_t clkval, clkset, clkmask;
	int i, error = 0;

	mutex_enter(&sc->sc_lock);

	clkset = BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL_REQ |
	    BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HW_CLKREQ_OFF;
	bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR, clkset);

	clkmask = BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL |
	    BWFM_SDIO_FUNC1_CHIPCLKCSR_HT_AVAIL;
	clkval = bwfm_sdio_read_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR);

	if ((clkval & ~clkmask) != clkset) {
		printf("%s: wrote 0x%02x read 0x%02x\n", DEVNAME(sc),
		    clkset, clkval);
		error = 1;
		goto done;
	}

	for (i = 1000; i > 0; i--) {
		clkval = bwfm_sdio_read_1(sc,
		    BWFM_SDIO_FUNC1_CHIPCLKCSR);
		if (clkval & clkmask)
			break;
	}
	if (i == 0) {
		printf("%s: timeout on ALPAV wait, clkval 0x%02x\n",
		    DEVNAME(sc), clkval);
		error = 1;
		goto done;
	}

	clkset = BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HW_CLKREQ_OFF |
	    BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_ALP;
	bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR, clkset);
	delay(65);

	bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_SDIOPULLUP, 0);

done:
	mutex_exit(&sc->sc_lock);

	return error;
}

void
bwfm_sdio_buscore_activate(struct bwfm_softc *bwfm, uint32_t rstvec)
{
	struct bwfm_sdio_softc *sc = (void *)bwfm;
	struct bwfm_core *core;

	core = bwfm_chip_get_core(&sc->sc_sc, BWFM_AGENT_CORE_SDIO_DEV);
	bwfm_sdio_buscore_write(&sc->sc_sc,
	    core->co_base + BWFM_SDPCMD_INTSTATUS, 0xFFFFFFFF);

	mutex_enter(&sc->sc_lock);
	if (rstvec)
		bwfm_sdio_ram_read_write(sc, 0, (char *)&rstvec,
		    sizeof(rstvec), 1);
	mutex_exit(&sc->sc_lock);
}

struct mbuf *
bwfm_sdio_newbuf(void)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return NULL;

	MCLGET(m, M_DONTWAIT);
	if (!(m->m_flags & M_EXT)) {
		m_freem(m);
		return NULL;
	}

	m->m_len = m->m_pkthdr.len = MCLBYTES;
	return m;
}

struct mbuf *
bwfm_qget(struct mbuf **q)
{
	struct mbuf *m = NULL;

	if (*q != NULL) {
		m = *q;
		*q = m->m_next;
		m->m_next = NULL;
	}

	return m;
}

void
bwfm_qput(struct mbuf **q, struct mbuf *m)
{

	if (*q == NULL)
		*q = m;
	else
		m_cat(*q, m);
}

int
bwfm_sdio_txcheck(struct bwfm_softc *bwfm)
{
	struct bwfm_sdio_softc *sc = (void *)bwfm;
	int error = 0;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_tx_count >= 64)
		error = ENOBUFS;
	mutex_exit(&sc->sc_lock);

	return error;
}


int
bwfm_sdio_txdata(struct bwfm_softc *bwfm, struct mbuf **mp)
{
	struct bwfm_sdio_softc *sc = (void *)bwfm;

	if (sc->sc_tx_count >= 64) {
		printf("%s: tx count limit reached\n",DEVNAME(sc));
		return ENOBUFS;
	}

	mutex_enter(&sc->sc_lock);
	sc->sc_tx_count++;
	MBUFQ_ENQUEUE(&sc->sc_tx_queue, *mp);
	mutex_exit(&sc->sc_lock);

	bwfm_sdio_intr(sc);

	return 0;
}

int
bwfm_sdio_txctl(struct bwfm_softc *bwfm, char *buf, size_t len)
{
	struct bwfm_sdio_softc *sc = (void *)bwfm;
	struct mbuf *m;

	KASSERT(len <= MCLBYTES);

	MGET(m, M_DONTWAIT, MT_CONTROL);
	if (m == NULL)
		goto fail;
	if (len > MLEN) {
		MCLGET(m, M_DONTWAIT);
		if (!(m->m_flags & M_EXT)) {
			m_freem(m);
			goto fail;
		}
	}
	memcpy(mtod(m, char *), buf, len);
	m->m_len = len;

	mutex_enter(&sc->sc_lock);
	MBUFQ_ENQUEUE(&sc->sc_tx_queue, m);
	mutex_exit(&sc->sc_lock);

	bwfm_sdio_intr(sc);

	return 0;

fail:
	return ENOBUFS;
}

int
bwfm_nvram_convert(u_char *buf, size_t len, size_t *newlenp)
{
	u_char *src, *dst, *end = buf + len;
	bool skip = false;
	size_t count = 0, pad;
	uint32_t token;

	for (src = buf, dst = buf; src != end; ++src) {
		if (*src == '\n') {
			if (count > 0)
				*dst++ = '\0';
			count = 0;
			skip = false;
			continue;
		}
		if (skip)
			continue;
		if (*src == '#' && count == 0) {
			skip = true;
			continue;
		}
		if (*src == '\r')
			continue;
		*dst++ = *src;
		++count;
	}

	count = dst - buf;
	pad = roundup(count + 1, 4) - count;

	if (count + pad + sizeof(token) > len)
		return 1;

	memset(dst, 0, pad);
	count += pad;
	dst += pad;

	token = (count / 4) & 0xffff;
	token |= ~token << 16;
	token = htole32(token);

	memcpy(dst, &token, sizeof(token));
	count += sizeof(token);

	*newlenp = count ;

	return 0;
}

int
bwfm_sdio_load_microcode(struct bwfm_sdio_softc *sc, u_char *ucode, size_t size,
    u_char *nvram, size_t nvlen)
{
	struct bwfm_softc *bwfm = &sc->sc_sc;
	char *verify = NULL;
	int err = 0;

	bwfm_sdio_clkctl(sc, CLK_AVAIL, false);

	DPRINTF(("ucode %zu bytes to 0x%08lx\n", size,
		(u_long)bwfm->sc_chip.ch_rambase));
	/* Upload firmware */
	err = bwfm_sdio_ram_read_write(sc, bwfm->sc_chip.ch_rambase,
	    ucode, size, 1);
	if (err)
		goto out;

	/* Verify firmware */
	verify = kmem_zalloc(size, KM_SLEEP);
	err = bwfm_sdio_ram_read_write(sc, bwfm->sc_chip.ch_rambase,
	    verify, size, 0);
	if (err || memcmp(verify, ucode, size)) {
		printf("%s: firmware verification failed\n",
		    DEVNAME(sc));
		kmem_free(verify, size);
		goto out;
	}
	kmem_free(verify, size);

	DPRINTF(("nvram %zu bytes to 0x%08lx\n", nvlen,
	    (u_long)bwfm->sc_chip.ch_rambase + bwfm->sc_chip.ch_ramsize
	    - nvlen));
	/* Upload nvram */
	err = bwfm_sdio_ram_read_write(sc, bwfm->sc_chip.ch_rambase +
	    bwfm->sc_chip.ch_ramsize - nvlen, nvram, nvlen, 1);
	if (err)
		goto out;

	/* Verify nvram */
	verify = kmem_zalloc(nvlen, KM_SLEEP);
	err = bwfm_sdio_ram_read_write(sc, bwfm->sc_chip.ch_rambase +
	    bwfm->sc_chip.ch_ramsize - nvlen, verify, nvlen, 0);
	if (err || memcmp(verify, nvram, nvlen)) {
		printf("%s: nvram verification failed\n",
		    DEVNAME(sc));
		kmem_free(verify, nvlen);
		goto out;
	}
	kmem_free(verify, nvlen);

	DPRINTF(("Reset core 0x%08x\n", *(uint32_t *)ucode));
	/* Load reset vector from firmware and kickstart core. */
	bwfm_chip_set_active(bwfm, *(uint32_t *)ucode);

out:
	bwfm_sdio_clkctl(sc, CLK_SDONLY, false);
	return err;
}

void
bwfm_sdio_clkctl(struct bwfm_sdio_softc *sc, enum bwfm_sdio_clkstate newstate,
    bool pendok)
{
	enum bwfm_sdio_clkstate oldstate;

	oldstate = sc->sc_clkstate;
	if (oldstate == newstate)
		return;

	switch (newstate) {
	case CLK_AVAIL:
		if (oldstate == CLK_NONE)
			sc->sc_clkstate = CLK_SDONLY; /* XXX */
		bwfm_sdio_htclk(sc, true, pendok);
		break;
	case CLK_SDONLY:
		if (oldstate == CLK_NONE)
			sc->sc_clkstate = newstate;
		else if (oldstate == CLK_AVAIL)
			bwfm_sdio_htclk(sc, false, false);
		else
			printf("%s: clkctl %d -> %d\n", DEVNAME(sc),
				sc->sc_clkstate, newstate);
		break;
	case CLK_NONE:
		if (oldstate == CLK_AVAIL)
			bwfm_sdio_htclk(sc, false, false);
		sc->sc_clkstate = newstate;
		break;
	default:
		break;
	}

	DPRINTF(("%s: %d -> %d = %d\n", DEVNAME(sc), oldstate, newstate,
	    sc->sc_clkstate));
}

void
bwfm_sdio_htclk(struct bwfm_sdio_softc *sc, bool on, bool pendok)
{
	uint32_t clkctl, devctl, req;
	int i;

	if (sc->sc_sr_enabled) {
		if (on)
			sc->sc_clkstate = CLK_AVAIL;
		else
			sc->sc_clkstate = CLK_SDONLY;
		return;
	}

	if (on) {
		if (sc->sc_alp_only)
			req = BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL_REQ;
		else
			req = BWFM_SDIO_FUNC1_CHIPCLKCSR_HT_AVAIL_REQ;
		bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR, req);

		clkctl = bwfm_sdio_read_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR);
		if (!BWFM_SDIO_FUNC1_CHIPCLKCSR_CLKAV(clkctl, sc->sc_alp_only)
		    && pendok) {
			devctl = bwfm_sdio_read_1(sc, BWFM_SDIO_DEVICE_CTL);
			devctl |= BWFM_SDIO_DEVICE_CTL_CA_INT_ONLY;
			bwfm_sdio_write_1(sc, BWFM_SDIO_DEVICE_CTL, devctl);
			sc->sc_clkstate = CLK_PENDING;
			return;
		} else if (sc->sc_clkstate == CLK_PENDING) {
			devctl = bwfm_sdio_read_1(sc, BWFM_SDIO_DEVICE_CTL);
			devctl &= ~BWFM_SDIO_DEVICE_CTL_CA_INT_ONLY;
			bwfm_sdio_write_1(sc, BWFM_SDIO_DEVICE_CTL, devctl);
		}

		for (i = 0; i < 5000; i++) {
			if (BWFM_SDIO_FUNC1_CHIPCLKCSR_CLKAV(clkctl,
			    sc->sc_alp_only))
				break;
			clkctl = bwfm_sdio_read_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR
);
			delay(1000);
		}
		if (!BWFM_SDIO_FUNC1_CHIPCLKCSR_CLKAV(clkctl, sc->sc_alp_only))
{
			printf("%s: HT avail timeout\n", DEVNAME(sc));
			return;
		}

		sc->sc_clkstate = CLK_AVAIL;
	} else {
		if (sc->sc_clkstate == CLK_PENDING) {
			devctl = bwfm_sdio_read_1(sc, BWFM_SDIO_DEVICE_CTL);
			devctl &= ~BWFM_SDIO_DEVICE_CTL_CA_INT_ONLY;
			bwfm_sdio_write_1(sc, BWFM_SDIO_DEVICE_CTL, devctl);
		}
		sc->sc_clkstate = CLK_SDONLY;
		bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR, 0);
	}
}

#if notyet
int
bwfm_sdio_bus_sleep(struct bwfm_sdio_softc *sc, bool sleep, bool pendok)
{
	uint32_t clkctl;

	if (sc->sleeping == sleep)
		return 0;

	if (sc->sc_sr_enabled) {
		if (sleep) {
			clkctl = bwfm_sdio_read_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR);
			if ((clkctl & BWFM_SDIO_FUNC1_CHIPCLKCSR_CSR_MASK) == 0)
				bwfm_sdio_write_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR, BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL_REQ);
		}
		/* kso_ctrl(sc, sleep) */
	}

	if (sleep) {
		if (!sc->sc_sr_enabled)
			bwfm_sdio_clkctl(sc, CLK_NONE, pendok);
	} else {
		bwfm_sdio_clkctl(sc, CLK_AVAIL, pendok);
	}

	sc->sleeping = sleep;

	return 0;
}
#endif

void
bwfm_sdio_readshared(struct bwfm_sdio_softc *sc)
{
	struct bwfm_softc *bwfm = &sc->sc_sc;
	struct bwfm_sdio_sdpcm sdpcm;
	uint32_t addr, shaddr;
	int err;

	bwfm_sdio_clkctl(sc, CLK_AVAIL, false);
	if (sc->sc_clkstate != CLK_AVAIL)
		return;

	shaddr = bwfm->sc_chip.ch_rambase + bwfm->sc_chip.ch_ramsize - 4;
	if (!bwfm->sc_chip.ch_rambase && sc->sc_sr_enabled)
		shaddr -= bwfm->sc_chip.ch_srsize;

	err = bwfm_sdio_ram_read_write(sc, shaddr, (char *)&addr,
	    sizeof(addr), 0);
	if (err)
		return;

	addr = le32toh(addr);
	if (addr == 0 || ((~addr >> 16) & 0xffff) == (addr & 0xffff))
		return;

	err = bwfm_sdio_ram_read_write(sc, addr, (char *)&sdpcm,
	    sizeof(sdpcm), 0);
	if (err)
		return;

	sc->sc_console_addr = le32toh(sdpcm.console_addr);
}

int
bwfm_sdio_intr(void *v)
{
	struct bwfm_sdio_softc *sc = (void *)v;

        DPRINTF(("%s: sdio_intr\n", DEVNAME(sc)));

	mutex_enter(&sc->sc_intr_lock);
	if (!sdmmc_task_pending(&sc->sc_task))
		sdmmc_add_task(sc->sc_sf[1]->sc, &sc->sc_task);
	sc->sc_task_queued = true;
	mutex_exit(&sc->sc_intr_lock);
	return 1;
}

void
bwfm_sdio_task(void *v)
{
	struct bwfm_sdio_softc *sc = (void *)v;

	mutex_enter(&sc->sc_intr_lock);
	while (sc->sc_task_queued) {
		sc->sc_task_queued = false;
		mutex_exit(&sc->sc_intr_lock);

		mutex_enter(&sc->sc_lock);
		bwfm_sdio_task1(sc);
#ifdef BWFM_DEBUG
		bwfm_sdio_debug_console(sc);
#endif
		mutex_exit(&sc->sc_lock);

		mutex_enter(&sc->sc_intr_lock);
	}
	mutex_exit(&sc->sc_intr_lock);
}

void
bwfm_sdio_task1(struct bwfm_sdio_softc *sc)
{
	uint32_t clkctl, devctl, intstat, hostint;

	if (!sc->sc_sr_enabled && sc->sc_clkstate == CLK_PENDING) {
		clkctl = bwfm_sdio_read_1(sc, BWFM_SDIO_FUNC1_CHIPCLKCSR);
		if (BWFM_SDIO_FUNC1_CHIPCLKCSR_HTAV(clkctl)) {
			devctl = bwfm_sdio_read_1(sc, BWFM_SDIO_DEVICE_CTL);
			devctl &= ~BWFM_SDIO_DEVICE_CTL_CA_INT_ONLY;
			bwfm_sdio_write_1(sc, BWFM_SDIO_DEVICE_CTL, devctl);
			sc->sc_clkstate = CLK_AVAIL;
		}
	}

	intstat = bwfm_sdio_dev_read(sc, BWFM_SDPCMD_INTSTATUS);
        DPRINTF(("%s: intstat 0x%" PRIx32 "\n", DEVNAME(sc), intstat));
	intstat &= (SDPCMD_INTSTATUS_HMB_SW_MASK|SDPCMD_INTSTATUS_CHIPACTIVE);
	/* XXX fc state */
	if (intstat)
		bwfm_sdio_dev_write(sc, BWFM_SDPCMD_INTSTATUS, intstat);

	if (intstat & SDPCMD_INTSTATUS_HMB_HOST_INT) {
		hostint = bwfm_sdio_dev_read(sc, SDPCMD_TOHOSTMAILBOXDATA);
        	DPRINTF(("%s: hostint 0x%" PRIx32 "\n", DEVNAME(sc), hostint));
		bwfm_sdio_dev_write(sc, SDPCMD_TOSBMAILBOX,
		    SDPCMD_TOSBMAILBOX_INT_ACK);
		if (hostint & SDPCMD_TOHOSTMAILBOXDATA_NAKHANDLED)
			intstat |= SDPCMD_INTSTATUS_HMB_FRAME_IND;
		if (hostint & SDPCMD_TOHOSTMAILBOXDATA_DEVREADY ||
		    hostint & SDPCMD_TOHOSTMAILBOXDATA_FWREADY)
			bwfm_sdio_readshared(sc);
	}

	/* FIXME: Might stall if we don't when not set. */
	if (intstat & SDPCMD_INTSTATUS_HMB_FRAME_IND)
		bwfm_sdio_rx_frames(sc);

	if (MBUFQ_FIRST(&sc->sc_tx_queue))
		bwfm_sdio_tx_frames(sc);
}

int
bwfm_sdio_tx_ok(struct bwfm_sdio_softc *sc)
{
	return (uint8_t)(sc->sc_tx_max_seq - sc->sc_tx_seq) != 0 &&
	    ((uint8_t)(sc->sc_tx_max_seq - sc->sc_tx_seq) & 0x80) == 0;
}

void    
bwfm_sdio_tx_frames(struct bwfm_sdio_softc *sc)
{
	struct mbuf *m;
	struct ifnet *ifp = sc->sc_sc.sc_ic.ic_ifp;
	bool ifstart = false;
	int i;

	if (!bwfm_sdio_tx_ok(sc))
		return;

	i = uimin((uint8_t)(sc->sc_tx_max_seq - sc->sc_tx_seq), 32);
	while (i--) {
		MBUFQ_DEQUEUE(&sc->sc_tx_queue, m);
		if (m == NULL)
			break;

		if (m->m_type == MT_CONTROL)
			bwfm_sdio_tx_ctrlframe(sc, m);
		else {
			bwfm_sdio_tx_dataframe(sc, m);  
			ifp->if_opackets++;
			ifstart = true;
		}

		m_freem(m);
	}

	if (ifstart) {
		ifp->if_flags &= ~IFF_OACTIVE;
		if_schedule_deferred_start(ifp);
	}
}

void
bwfm_sdio_tx_ctrlframe(struct bwfm_sdio_softc *sc, struct mbuf *m)
{
	struct bwfm_sdio_hwhdr *hwhdr;
	struct bwfm_sdio_swhdr *swhdr;
	size_t len, roundto;
	
	len = sizeof(*hwhdr) + sizeof(*swhdr) + m->m_len;

	/* Zero-pad to either block-size or 4-byte alignment. */
	if (len > 512 && (len % 512) != 0)
		roundto = 512;
	else
		roundto = 4;

	KASSERT(roundup(len, roundto) <= sc->sc_bounce_size);
 
	hwhdr = (void *)sc->sc_bounce_buf;
	hwhdr->frmlen = htole16(len);
	hwhdr->cksum = htole16(~len);
	
	swhdr = (void *)&hwhdr[1];
	swhdr->seqnr = sc->sc_tx_seq++;
	swhdr->chanflag = BWFM_SDIO_SWHDR_CHANNEL_CONTROL;
	swhdr->nextlen = 0;
	swhdr->dataoff = sizeof(*hwhdr) + sizeof(*swhdr);
	swhdr->maxseqnr = 0;
	
	m_copydata(m, 0, m->m_len, &swhdr[1]);
	
	if (roundup(len, roundto) != len)
		memset(sc->sc_bounce_buf + len, 0,
		    roundup(len, roundto) - len);
	
	bwfm_sdio_frame_read_write(sc, sc->sc_bounce_buf,
	    roundup(len, roundto), 1);
}

void
bwfm_sdio_tx_dataframe(struct bwfm_sdio_softc *sc, struct mbuf *m)
{
	struct bwfm_sdio_hwhdr *hwhdr;
	struct bwfm_sdio_swhdr *swhdr;
	struct bwfm_proto_bcdc_hdr *bcdc;
	size_t len, roundto;

	len = sizeof(*hwhdr) + sizeof(*swhdr) + sizeof(*bcdc)
	    + m->m_pkthdr.len;

	/* Zero-pad to either block-size or 4-byte alignment. */
	if (len > 512 && (len % 512) != 0)
		roundto = 512;
	else
		roundto = 4;

	KASSERT(roundup(len, roundto) <= sc->sc_bounce_size);

	hwhdr = (void *)sc->sc_bounce_buf;
	hwhdr->frmlen = htole16(len);
	hwhdr->cksum = htole16(~len);

	swhdr = (void *)&hwhdr[1];
	swhdr->seqnr = sc->sc_tx_seq++;
	swhdr->chanflag = BWFM_SDIO_SWHDR_CHANNEL_DATA;
	swhdr->nextlen = 0;
	swhdr->dataoff = sizeof(*hwhdr) + sizeof(*swhdr);
	swhdr->maxseqnr = 0;

	bcdc = (void *)&swhdr[1];
	bcdc->data_offset = 0;
	bcdc->priority = WME_AC_BE;
	bcdc->flags = BWFM_BCDC_FLAG_VER(BWFM_BCDC_FLAG_PROTO_VER);
	bcdc->flags2 = 0;

	m_copydata(m, 0, m->m_pkthdr.len, &bcdc[1]);

	if (roundup(len, roundto) != len)
		memset(sc->sc_bounce_buf + len, 0,
		    roundup(len, roundto) - len);

	bwfm_sdio_frame_read_write(sc, sc->sc_bounce_buf,
	    roundup(len, roundto), 1);

	sc->sc_tx_count--;
}

int
bwfm_sdio_rxctl(struct bwfm_softc *bwfm, char *buf, size_t *lenp)
{
	struct bwfm_sdio_softc *sc = (void *)bwfm;
	struct mbuf *m;
	int err = 0;

	mutex_enter(&sc->sc_lock);
	while ((m = bwfm_qget(&sc->sc_rxctl_queue)) == NULL) {
		err = cv_timedwait(&sc->sc_rxctl_cv, &sc->sc_lock,
		                   mstohz(5000));
		if (err == EWOULDBLOCK)
			break;
	}
	mutex_exit(&sc->sc_lock);

	if (err)
		return 1;

	if (m->m_len < *lenp) {
		m_freem(m);
		return 1;
	}

	*lenp = m->m_len;
	m_copydata(m, 0, m->m_len, buf);
	m_freem(m);
	return 0;
}

void
bwfm_sdio_rx_frames(struct bwfm_sdio_softc *sc)
{       
	struct bwfm_sdio_hwhdr *hwhdr;
	struct bwfm_sdio_swhdr *swhdr;
	struct bwfm_proto_bcdc_hdr *bcdc;
	uint16_t *sublen, nextlen = 0;  
	struct mbuf *m;
	size_t flen, off, hoff;
	char *data;
	int nsub;

	hwhdr = (struct bwfm_sdio_hwhdr *)sc->sc_bounce_buf;
	swhdr = (struct bwfm_sdio_swhdr *)&hwhdr[1];
	data = (char *)&swhdr[1];
	
	for (;;) {
		/* If we know the next size, just read ahead. */
		if (nextlen) {
			if (bwfm_sdio_frame_read_write(sc, sc->sc_bounce_buf,
			    nextlen, 0))
				break;
		} else {
			if (bwfm_sdio_frame_read_write(sc, sc->sc_bounce_buf,
			    sizeof(*hwhdr) + sizeof(*swhdr), 0))
				break; 
		}
	
		hwhdr->frmlen = le16toh(hwhdr->frmlen);
		hwhdr->cksum = le16toh(hwhdr->cksum);
	
		if (hwhdr->frmlen == 0 && hwhdr->cksum == 0)
			break;

		if ((hwhdr->frmlen ^ hwhdr->cksum) != 0xffff) {
			printf("%s: checksum error\n", DEVNAME(sc));
			break;  
		}

		if (hwhdr->frmlen < sizeof(*hwhdr) + sizeof(*swhdr)) {
			printf("%s: length error\n", DEVNAME(sc));
			break;
		} 

		if (nextlen && hwhdr->frmlen > nextlen) {
			printf("%s: read ahead length error (%u > %u)\n",
			    DEVNAME(sc), hwhdr->frmlen, nextlen);
			break;
		}

		sc->sc_tx_max_seq = swhdr->maxseqnr;

		flen = hwhdr->frmlen - (sizeof(*hwhdr) + sizeof(*swhdr));
		if (flen == 0) {
			nextlen = swhdr->nextlen << 4;
			continue;
		}

		if (!nextlen) {
			KASSERT(roundup(flen, 4) <= sc->sc_bounce_size -
			    (sizeof(*hwhdr) + sizeof(*swhdr)));
			if (bwfm_sdio_frame_read_write(sc, data,
			    roundup(flen, 4), 0))
				break;
		}

		if (swhdr->dataoff < (sizeof(*hwhdr) + sizeof(*swhdr))) {
			printf("%s: data offset %u in header\n",
			    DEVNAME(sc), swhdr->dataoff);
			break;
		}

		off = swhdr->dataoff - (sizeof(*hwhdr) + sizeof(*swhdr));
		if (off > flen) {
			printf("%s: offset %zu beyond end %zu\n",
			    DEVNAME(sc), off, flen);
			break;
		}

		switch (swhdr->chanflag & BWFM_SDIO_SWHDR_CHANNEL_MASK) {
		case BWFM_SDIO_SWHDR_CHANNEL_CONTROL:
			m = bwfm_sdio_newbuf();
			if (m == NULL)
				break;
			if (flen - off > m->m_len) {
				printf("%s: ctl bigger than anticipated\n",
				    DEVNAME(sc));
				m_freem(m);
				break;
			}
			m->m_len = m->m_pkthdr.len = flen - off;
			memcpy(mtod(m, char *), data + off, flen - off);
			bwfm_qput(&sc->sc_rxctl_queue, m);
			cv_broadcast(&sc->sc_rxctl_cv);
			nextlen = swhdr->nextlen << 4;
			break;
		case BWFM_SDIO_SWHDR_CHANNEL_EVENT:
		case BWFM_SDIO_SWHDR_CHANNEL_DATA:
			m = bwfm_sdio_newbuf();
			if (m == NULL)
				break;
			if (flen - off > m->m_len) {
				printf("%s: frame bigger than anticipated\n",
				    DEVNAME(sc));
				m_freem(m);
				break;
			}
			m->m_len = m->m_pkthdr.len = flen - off;
			memcpy(mtod(m, char *), data + off, flen - off);
			bcdc = mtod(m, struct bwfm_proto_bcdc_hdr *);
			hoff = sizeof(*bcdc) + ((size_t)bcdc->data_offset << 2);
			if (m->m_len < hoff) {
				printf("%s: short bcdc packet %d < %zu\n",
				    DEVNAME(sc), m->m_len, hoff);
				m_freem(m);
				break;
			}
			m_adj(m, hoff);
			/* don't pass empty packet to stack */
			if (m->m_len == 0) {
				m_freem(m);
				break;
			}
			bwfm_rx(&sc->sc_sc, m);
			nextlen = swhdr->nextlen << 4;
			break;
		case BWFM_SDIO_SWHDR_CHANNEL_GLOM:
			if ((flen % sizeof(uint16_t)) != 0) {
				printf("%s: odd length (%zu) glom table\n",
				    DEVNAME(sc), flen);
				break;
			}
			nsub = flen / sizeof(uint16_t);
			sublen = kmem_zalloc(nsub * sizeof(uint16_t), KM_SLEEP);
			memcpy(sublen, data, nsub * sizeof(uint16_t));
			bwfm_sdio_rx_glom(sc, sublen, nsub, &nextlen);
			kmem_free(sublen, nsub * sizeof(uint16_t));
			break;
		default:
			printf("%s: unknown channel\n", DEVNAME(sc));
			break;
		}
	}
}

void
bwfm_sdio_rx_glom(struct bwfm_sdio_softc *sc, uint16_t *sublen, int nsub,
    uint16_t *nextlen)
{
	struct bwfm_sdio_hwhdr hwhdr;
	struct bwfm_sdio_swhdr swhdr;
	struct bwfm_proto_bcdc_hdr *bcdc;
	struct mbuf *m, *m0;
	size_t flen, off, hoff;
	int i;

	if (nsub == 0)
		return;

	m0 = NULL;
	for (i = 0; i < nsub; i++) {
		m = bwfm_sdio_newbuf();
		if (m == NULL) {
			m_freem(m0);
			return;
		}
		bwfm_qput(&m0, m);
		if (le16toh(sublen[i]) > m->m_len) {
			m_freem(m0);
			return;
		}
		if (bwfm_sdio_frame_read_write(sc, mtod(m, char *),
		    le16toh(sublen[i]), 0)) {
			m_freem(m0);
			return;
		}
		m->m_len = m->m_pkthdr.len = le16toh(sublen[i]);
	}

	if (m0->m_len >= sizeof(hwhdr) + sizeof(swhdr)) {
		m_copydata(m0, 0, sizeof(hwhdr), &hwhdr);
		m_copydata(m0, sizeof(hwhdr), sizeof(swhdr), &swhdr);

		/* TODO: Verify actual superframe header */

		/* remove superframe header */
		if (m0->m_len >= swhdr.dataoff)
			m_adj(m0, swhdr.dataoff);
	}

	*nextlen = 0;
	while ((m = bwfm_qget(&m0)) != NULL) {
		if (m->m_len < sizeof(hwhdr) + sizeof(swhdr)) {
			printf("%s: tiny mbuf %d < %zu\n", DEVNAME(sc),
			    m->m_len, sizeof(hwhdr) + sizeof(swhdr));
			goto drop;
		}

		m_copydata(m, 0, sizeof(hwhdr), &hwhdr);
		m_copydata(m, sizeof(hwhdr), sizeof(swhdr), &swhdr);

		hwhdr.frmlen = le16toh(hwhdr.frmlen);
		hwhdr.cksum = le16toh(hwhdr.cksum);

		if (hwhdr.frmlen == 0 && hwhdr.cksum == 0)
			goto drop;

		if ((hwhdr.frmlen ^ hwhdr.cksum) != 0xffff) {
			printf("%s: checksum error\n", DEVNAME(sc));
			goto drop;
		}


		if (hwhdr.frmlen < sizeof(hwhdr) + sizeof(swhdr)) {
			printf("%s: length error\n", DEVNAME(sc));
			goto drop;
		}

		flen = hwhdr.frmlen - (sizeof(hwhdr) + sizeof(swhdr));
		if (flen == 0)
			goto drop;

		if (hwhdr.frmlen > m->m_len) {
			printf("%s: short mbuf %d < %zu\n",
			    DEVNAME(sc),m->m_len,flen);
			goto drop;
		}

		if (swhdr.dataoff < (sizeof(hwhdr) + sizeof(swhdr))) {
			printf("%s: data offset %u in header\n",
			    DEVNAME(sc), swhdr.dataoff);
			goto drop;
		}

		off = swhdr.dataoff - (sizeof(hwhdr) + sizeof(swhdr));
		if (off > flen) {
			printf("%s: offset %zu beyond end %zu\n",
			    DEVNAME(sc), off, flen);
			goto drop;
		}

		m_adj(m, (int)hwhdr.frmlen - m->m_len);
		*nextlen = swhdr.nextlen << 4;

		switch (swhdr.chanflag & BWFM_SDIO_SWHDR_CHANNEL_MASK) {
		case BWFM_SDIO_SWHDR_CHANNEL_CONTROL:
			printf("%s: control channel not allowed in glom\n",
			    DEVNAME(sc));
			goto drop;
		case BWFM_SDIO_SWHDR_CHANNEL_EVENT:
		case BWFM_SDIO_SWHDR_CHANNEL_DATA:
			m_adj(m, swhdr.dataoff);
			bcdc = mtod(m, struct bwfm_proto_bcdc_hdr *);
			hoff = sizeof(*bcdc) + ((size_t)bcdc->data_offset << 2);
			if (m->m_len < hoff) {
				printf("%s: short bcdc packet %d < %zu\n",
				    DEVNAME(sc), m->m_len, hoff);
				m_freem(m);
				break;
			}
			m_adj(m, hoff);
			/* don't pass empty packet to stack */
			if (m->m_len == 0) {
				m_freem(m);
				break;
			}
			bwfm_rx(&sc->sc_sc, m);
			break;
		case BWFM_SDIO_SWHDR_CHANNEL_GLOM:
			printf("%s: glom not allowed in glom\n",
			    DEVNAME(sc));
			goto drop;
		default:
			printf("%s: unknown channel\n", DEVNAME(sc));
			goto drop;
		}

		continue;
drop:
		printf("rx dropped %p len %d\n",mtod(m, char *),m->m_pkthdr.len);
		m_free(m);
	}
}

#ifdef BWFM_DEBUG
void
bwfm_sdio_debug_console(struct bwfm_sdio_softc *sc)
{
	struct bwfm_sdio_console c;
	uint32_t newidx;
	int err;

	if (!sc->sc_console_addr)
		return;

	err = bwfm_sdio_ram_read_write(sc, sc->sc_console_addr,
	    (char *)&c, sizeof(c), 0);
	if (err)
		return; 
 
	c.log_buf = le32toh(c.log_buf);
	c.log_bufsz = le32toh(c.log_bufsz);
	c.log_idx = le32toh(c.log_idx);

	if (sc->sc_console_buf == NULL) {
		sc->sc_console_buf = malloc(c.log_bufsz, M_DEVBUF,
		    M_WAITOK|M_ZERO);
		sc->sc_console_buf_size = c.log_bufsz;
	}

	newidx = c.log_idx;
	if (newidx >= sc->sc_console_buf_size)
		return;

	err = bwfm_sdio_ram_read_write(sc, c.log_buf, sc->sc_console_buf,
	    sc->sc_console_buf_size, 0);
	if (err)
		return;

	if (newidx != sc->sc_console_readidx)
		DPRINTFN(3, ("BWFM CONSOLE: "));
	while (newidx != sc->sc_console_readidx) {
		uint8_t ch = sc->sc_console_buf[sc->sc_console_readidx];
		sc->sc_console_readidx++;
		if (sc->sc_console_readidx == sc->sc_console_buf_size)
			sc->sc_console_readidx = 0;
		if (ch == '\r')
			continue;
		DPRINTFN(3, ("%c", ch));
	}
}
#endif
