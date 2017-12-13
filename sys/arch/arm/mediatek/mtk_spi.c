/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "locators.h"

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/locore.h>
#include <arm/mainbus/mainbus.h>

#include <arm/mediatek/mercury_reg.h>

#include <dev/spi/spivar.h>
#include <dev/fdt/fdtvar.h>

#define BIT(nr)			(1UL << (nr))

#define SPI_CFG0_REG                      0x0000
#define SPI_CFG1_REG                      0x0004
#define SPI_TX_SRC_REG                    0x0008
#define SPI_RX_DST_REG                    0x000c
#define SPI_TX_DATA_REG                   0x0010
#define SPI_RX_DATA_REG                   0x0014
#define SPI_CMD_REG                       0x0018
#define SPI_STATUS0_REG                   0x001c
#define SPI_STATUS1_REG                   0x0020
#define SPI_PAD_SEL_REG                   0x0024
#define SPI_CFG2_REG                      0x0028

#define SPI_CFG0_SCK_HIGH_OFFSET          0
#define SPI_CFG0_SCK_LOW_OFFSET           8
#define SPI_CFG0_CS_HOLD_OFFSET           16
#define SPI_CFG0_CS_SETUP_OFFSET          24
#define SPI_ADJUST_CFG0_SCK_LOW_OFFSET    16
#define SPI_ADJUST_CFG0_CS_HOLD_OFFSET    0
#define SPI_ADJUST_CFG0_CS_SETUP_OFFSET   16

#define SPI_CFG1_CS_IDLE_OFFSET           0
#define SPI_CFG1_PACKET_LOOP_OFFSET       8
#define SPI_CFG1_PACKET_LENGTH_OFFSET     16
#define SPI_CFG1_GET_TICK_DLY_OFFSET      30

#define SPI_CFG1_CS_IDLE_MASK             0xff
#define SPI_CFG1_PACKET_LOOP_MASK         0xff00
#define SPI_CFG1_PACKET_LENGTH_MASK       0x3ff0000

#define SPI_CMD_ACT                  BIT(0)
#define SPI_CMD_RESUME               BIT(1)
#define SPI_CMD_RST                  BIT(2)
#define SPI_CMD_PAUSE_EN             BIT(4)
#define SPI_CMD_DEASSERT             BIT(5)
#define SPI_CMD_SAMPLE_SEL           BIT(6)
#define SPI_CMD_CS_POL               BIT(7)
#define SPI_CMD_CPHA                 BIT(8)
#define SPI_CMD_CPOL                 BIT(9)
#define SPI_CMD_RX_DMA               BIT(10)
#define SPI_CMD_TX_DMA               BIT(11)
#define SPI_CMD_TXMSBF               BIT(12)
#define SPI_CMD_RXMSBF               BIT(13)
#define SPI_CMD_RX_ENDIAN            BIT(14)
#define SPI_CMD_TX_ENDIAN            BIT(15)
#define SPI_CMD_FINISH_IE            BIT(16)
#define SPI_CMD_PAUSE_IE             BIT(17)

#define	SPI_CPHA	0x01			/* clock phase */
#define	SPI_CPOL	0x02			/* clock polarity */

#define MTK_SPI_CLK_HZ	109*1000*1000

#define MTK_SPI_PAUSE_INT_STATUS 0x2

#define MTK_SPI_IDLE 0
#define MTK_SPI_PAUSED 1

#define MTK_SPI_MAX_FIFO_SIZE 32U
#define MTK_SPI_PACKET_SIZE 1024

#define MTK_DUMMY_DATA 0x0

struct mtkspi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_ios;
	void			*sc_intrh;
	struct spi_controller	sc_spi;
	SIMPLEQ_HEAD(,spi_transfer) sc_q;
	struct spi_transfer	*sc_transfer;
	struct spi_chunk	*sc_wchunk;
	struct spi_chunk	*sc_rchunk;
	uint32_t state; /* spi hw status */
};

struct mtk_chip_config {
	uint32_t tx_mlsb;
	uint32_t rx_mlsb;
};

struct mtk_chip_config chip_config = {
	.tx_mlsb = 1,
	.rx_mlsb = 1,
};

#define MTK_SPI_DEBUG_MESSAGE

#ifdef MTK_SPI_DEBUG_MESSAGE
#define mtk_spi_debug(_message...)  do { aprint_normal(_message); } while (0)
#else
#define printf_debug(_message...)  do {} while (0)
#endif

static int mtkspi_match(device_t parent, cfdata_t cf, void *aux);
static void mtkspi_attach(device_t parent, device_t self, void *aux);
static int mtkspi_intr(void *cookie);
static int mtkspi_configure(void *cookie, int slave, int mode, int speed);
static int mtkspi_transfer(void *cookie, struct spi_transfer *st);
static void mtkspi_sched(struct mtkspi_softc *sc);

static const char * const compatible[] = {
	"mediatek,mercury-spi",
	NULL
};

CFATTACH_DECL_NEW(mtk_spi, sizeof(struct mtkspi_softc),
	mtkspi_match, mtkspi_attach, NULL, NULL);

static void mtkspi_dump_register(void *cookie)
{
#ifdef MTK_SPI_DEBUG_MESSAGE
	struct mtkspi_softc * const sc = cookie;

	mtk_spi_debug("SPI_CFG0_REG 0x%x\n", bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CFG0_REG));
	mtk_spi_debug("SPI_CFG1_REG 0x%x\n", bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CFG1_REG));
	mtk_spi_debug("SPI_TX_SRC_REG 0x%x\n", bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_TX_SRC_REG));
	mtk_spi_debug("SPI_RX_DST_REG 0x%x\n", bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_RX_DST_REG));
	mtk_spi_debug("SPI_CMD_REG 0x%x\n", bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG));
	mtk_spi_debug("SPI_STATUS1_REG 0x%x\n", bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_STATUS1_REG));
#endif
}

static void mtk_spis_dump_data(const char *name, const char *data, uint32_t size)
{
#ifdef MTK_SPI_DEBUG_MESSAGE
	uint32_t i;

	mtk_spi_debug("%s:", name);
	for (i = 0; i < size; i++) {
		if (!(i % 6))
			mtk_spi_debug("\n");
		mtk_spi_debug("%x ", data[i]);
	}
	mtk_spi_debug("\n");
#endif
}

static void mtkspi_reset(struct mtkspi_softc *sc)
{
	uint32_t reg_val;

	/* set the software reset bit in SPI_CMD_REG. */
	reg_val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG);
	reg_val |= SPI_CMD_RST;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG, reg_val);

	reg_val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG);
	reg_val &= ~SPI_CMD_RST;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG, reg_val);
}

static int mtkspi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void mtkspi_attach(device_t parent, device_t self, void *aux)
{
	struct spibus_attach_args sba;
	struct fdt_attach_args * const faa = aux;
	struct mtkspi_softc * const sc = device_private(self);
	bus_space_handle_t bsh;
	bus_space_tag_t bst;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	bst = faa->faa_bst;
	sc->sc_dev = self;

	/* initialize the queue */
	SIMPLEQ_INIT(&sc->sc_q);

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(bst, addr, size, 0, &bsh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	sc->sc_ioh = bsh;
	sc->sc_iot = bst;

	mtkspi_reset(sc);

	//mtk_intpol_set(loc->loc_intr, IST_LEVEL_LOW);
	//intr_establish(loc->loc_intr, IPL_BIO, IST_LEVEL, mtkspi_intr, sc);

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_intrh = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_BIO,
		FDT_INTR_MPSAFE, mtkspi_intr, sc);
	if (sc->sc_intrh == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
			intrstr);
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_spi.sct_cookie = sc;
	sc->sc_spi.sct_configure = mtkspi_configure;
	sc->sc_spi.sct_transfer = mtkspi_transfer;
	sc->sc_spi.sct_nslaves = 1;

	sba.sba_controller = &sc->sc_spi;

	(void) config_found_ia(self, "spibus", &sba, spibus_print);

	return;
}

static inline unsigned long div_round_up(unsigned int n, unsigned int d)
{
	return (n + d - 1) / d;
}

static void mtkspi_recv(struct mtkspi_softc *sc)
{
	uint32_t i, remainder, reg_val = 0;
	struct spi_chunk *chunk = sc->sc_rchunk;
	struct spi_transfer	*st = sc->sc_transfer;

	if (!st->st_chunks->chunk_read) {
		sc->sc_rchunk = NULL;
		return;
	}

	if (chunk) {
		mtk_spi_debug("\n[%s]=======line %d ==========\n", __func__, __LINE__);
		for (i = 0; i < chunk->chunk_rresid; i++) {
			if (i % 4 == 0)
				reg_val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_RX_DATA_REG);
			*chunk->chunk_rptr++ = (reg_val >> ((i % 4) *8)) & 0xff;
		}

		remainder = chunk->chunk_rresid % 4;
		if (remainder > 0) {
			reg_val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_RX_DATA_REG);
			memcpy(chunk->chunk_rptr, &reg_val, remainder);
		}
		mtk_spis_dump_data("spim rx data is", chunk->chunk_read, chunk->chunk_count);
	}

	sc->sc_rchunk = sc->sc_rchunk->chunk_next;

	return;
}

static void mtkspi_done(void *cookie)
{
	struct spi_transfer	*st;
	struct mtkspi_softc * const sc = cookie;

	/* called from interrupt handler */
	if ((st = sc->sc_transfer) != NULL) {
		mtk_spi_debug("\n[%s]=======line %d ==========\n", __func__, __LINE__);
		sc->sc_transfer = NULL;
		spi_done(st, 0);
	}

	/* make sure we clear these bits out */
	sc->sc_wchunk = sc->sc_rchunk = NULL;
}

static int mtkspi_intr(void *cookie)
{
	uint32_t reg_val;
	struct mtkspi_softc * const sc = cookie;

	reg_val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_STATUS0_REG);
	if (reg_val & MTK_SPI_PAUSE_INT_STATUS)
		sc->state = MTK_SPI_PAUSED;
	else
		sc->state = MTK_SPI_IDLE;

	mtkspi_recv(sc);

	mtkspi_sched(sc);


	if (sc->sc_rchunk == NULL)
		mtk_spi_debug("sc->sc_rchunk == NULL\n");
	if (sc->sc_wchunk == NULL)
		mtk_spi_debug("sc->sc_wchunk == NULL\n");

	if ((sc->sc_rchunk == NULL) && (sc->sc_wchunk == NULL)) {
		mtk_spi_debug("\n[%s]=======line %d ==========\n", __func__, __LINE__);
		reg_val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG);
		reg_val &= ~SPI_CMD_PAUSE_EN;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG, reg_val);
		sc->state = MTK_SPI_IDLE;
		mtkspi_reset(sc);
		mtkspi_done(sc);
	}

	mtk_spi_debug("\n[%s]=======line %d ==========\n", __func__, __LINE__);

	return 0;
}


static void mtkspi_config_speed(void *cookie, int speed)
{
	uint32_t div, sck_time, cs_time, reg_val = 0;
	struct mtkspi_softc * const sc = cookie;

	if (speed < MTK_SPI_CLK_HZ / 2)
		div = div_round_up(MTK_SPI_CLK_HZ, speed);
	else
		div = 1;

	sck_time = (div + 1) / 2;
	cs_time = sck_time * 2;

	reg_val |= (((sck_time - 1) & 0xff)
		   << SPI_CFG0_SCK_HIGH_OFFSET);
	reg_val |= (((sck_time - 1) & 0xff) << SPI_CFG0_SCK_LOW_OFFSET);
	reg_val |= (((cs_time - 1) & 0xff) << SPI_CFG0_CS_HOLD_OFFSET);
	reg_val |= (((cs_time - 1) & 0xff) << SPI_CFG0_CS_SETUP_OFFSET);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CFG0_REG, reg_val);

	reg_val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CFG1_REG);
	reg_val &= ~SPI_CFG1_CS_IDLE_MASK;
	reg_val |= (((cs_time - 1) & 0xff) << SPI_CFG1_CS_IDLE_OFFSET);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CFG1_REG, reg_val);
}

static int mtkspi_configure(void *cookie, int slave, int mode, int speed)
{
	struct mtkspi_softc * const sc = cookie;
	uint32_t reg_val, cpol, cpha;

	if (slave > 1)
		return EINVAL;

	if (speed <= 0)
		return EINVAL;

	cpha = mode & SPI_CPHA ? 1 : 0;
	cpol = mode & SPI_CPOL ? 1 : 0;

	reg_val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG);

	if (cpha)
		reg_val |= SPI_CMD_CPHA;
	else
		reg_val &= ~SPI_CMD_CPHA;
	if (cpol)
		reg_val |= SPI_CMD_CPOL;
	else
		reg_val &= ~SPI_CMD_CPOL;

	/* set the mlsbx and mlsbtx */
	if (chip_config.tx_mlsb)
		reg_val |= SPI_CMD_TXMSBF;
	else
		reg_val &= ~SPI_CMD_TXMSBF;
	if (chip_config.rx_mlsb)
		reg_val |= SPI_CMD_RXMSBF;
	else
		reg_val &= ~SPI_CMD_RXMSBF;

		/* set the tx/rx endian */
#if 1//def __LITTLE_ENDIAN
		reg_val &= ~SPI_CMD_TX_ENDIAN;
		reg_val &= ~SPI_CMD_RX_ENDIAN;
#else
		reg_val |= SPI_CMD_TX_ENDIAN;
		reg_val |= SPI_CMD_RX_ENDIAN;
#endif

	/* set finish and pause interrupt always enable */
	reg_val |= SPI_CMD_FINISH_IE | SPI_CMD_PAUSE_IE;

	/* disable dma mode */
	reg_val &= ~(SPI_CMD_TX_DMA | SPI_CMD_RX_DMA);

	/* disable deassert mode */
	reg_val &= ~SPI_CMD_DEASSERT;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG, reg_val);

	mtkspi_config_speed(cookie, speed);

	return 0;
}

static void mtkspi_setup_packet(struct mtkspi_softc *sc, uint32_t trans_length)
{
	uint32_t packet_size, packet_loop, reg_val;

	packet_size = MIN(trans_length, MTK_SPI_PACKET_SIZE);
	packet_loop = trans_length / packet_size;

	mtk_spi_debug("%s: packet_size[%d] packet_loop[%d]\n",
				__FUNCTION__, packet_size, packet_loop);

	reg_val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CFG1_REG);
	reg_val &= ~(SPI_CFG1_PACKET_LENGTH_MASK | SPI_CFG1_PACKET_LOOP_MASK);
	reg_val |= (packet_size - 1) << SPI_CFG1_PACKET_LENGTH_OFFSET;
	reg_val |= (packet_loop - 1) << SPI_CFG1_PACKET_LOOP_OFFSET;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CFG1_REG, reg_val);
}

static void mtkspi_enable_transfer(struct mtkspi_softc *sc)
{
	uint32_t cmd;

	cmd = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG);
	if (sc->state == MTK_SPI_IDLE)
		cmd |= SPI_CMD_ACT;
	else
		cmd |= SPI_CMD_RESUME;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG, cmd);
}

static void mtkspi_prepare_transfer(struct mtkspi_softc *sc)
{
	uint32_t i, cnt, len = 0;
	const uint32_t *outb;
	struct spi_chunk *chunk = sc->sc_wchunk;

	if (sc->sc_wchunk->chunk_wresid)
		len = sc->sc_wchunk->chunk_wresid;
	if (sc->sc_rchunk->chunk_rresid)
		len = sc->sc_rchunk->chunk_rresid;

	cnt = div_round_up(len, sizeof(uint32_t));
	mtkspi_setup_packet(sc, chunk->chunk_count);


	if (chunk->chunk_write) { /* tx */
		outb = (const uint32_t *)chunk->chunk_wptr;
		mtk_spis_dump_data("spi outb is", (const uint8_t *)outb, chunk->chunk_wresid);
		for (i = 0; i < cnt; i++)
			bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_TX_DATA_REG, outb[i]);
	} else {
		/* mtk spi controller need transmit in full-duplex for RX, BUT can tx only */
		for (i = 0; i < cnt; i++)
			bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_TX_DATA_REG, MTK_DUMMY_DATA);
	}

	/* advance to next transfer */
	sc->sc_wchunk = sc->sc_wchunk->chunk_next;

	return;
}

static void mtkspi_sched(struct mtkspi_softc *sc)
{
	struct spi_transfer	*st;

	mtk_spi_debug("\n[%s]=======line %d ==========\n", __func__, __LINE__);
	while ((st = spi_transq_first(&sc->sc_q)) != NULL) {
		aprint_normal("%s: st=%p\n", __FUNCTION__, st);

		/* remove the item */
		spi_transq_dequeue(&sc->sc_q);

		/* note that we are working on it */
		sc->sc_transfer = st;

		/* setup chunks */
		sc->sc_rchunk = sc->sc_wchunk = st->st_chunks;

		mtkspi_prepare_transfer(sc);

		mtk_spi_debug("=================%s %d\n", __FUNCTION__, __LINE__);
		mtkspi_dump_register(sc);

		/* now kick the master start to get the chip running */
		mtkspi_enable_transfer(sc);

		return;
	}
}

static int mtkspi_transfer(void *cookie, struct spi_transfer *st)
{
	struct mtkspi_softc * const sc = cookie;
	int s, cmd;

	mtk_spi_debug("\n[%s]=======line %d==========\n", __func__, __LINE__);

	s = splbio();
	spi_transq_enqueue(&sc->sc_q, st);

	sc->state = MTK_SPI_IDLE;
	/* set pause mode */
	cmd = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG);
	cmd |= SPI_CMD_PAUSE_EN;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CMD_REG, cmd);

	mtkspi_sched(sc);

	splx(s);

	mtk_spi_debug("\n[%s]=======line %d==========\n", __func__, __LINE__);
	return 0;
}

