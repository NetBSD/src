/*	$NetBSD: s3c2440_sdi.c,v 1.1.10.2 2017/12/03 11:35:55 jdolecek Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: s3c2440_sdi.c,v 1.1.10.2 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/malloc.h> /* For M_NOWAIT*/

#include <sys/mutex.h>
#include <sys/condvar.h>

#include <sys/bus.h>
#include <machine/cpu.h>

#include <arm/s3c2xx0/s3c24x0var.h>
#include <arm/s3c2xx0/s3c2440var.h>
#include <arm/s3c2xx0/s3c24x0reg.h>
#include <arm/s3c2xx0/s3c2440reg.h>
#include <arm/s3c2xx0/s3c2440_dma.h>

//#include <arm/s3c2xx0/s3c2440_sdi.h>

#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <uvm/uvm_extern.h>
/*#define SSSDI_DEBUG*/
#ifdef SSSDI_DEBUG
#define DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

struct sssdi_softc {
	device_t dev;

	bus_space_tag_t iot;

	bus_space_handle_t ioh;
	bus_space_handle_t card_ioh; /* Card detect I/O*/

	device_t sdmmc;

	uint32_t caps;

	int width;   /* Transfer width */
	void *sc_ih; /* SSSDI Interrupt handler */

	struct kmutex intr_mtx;
	struct kcondvar intr_cv;
	uint32_t intr_status; /* Set by the interrupt handler */

	dmac_xfer_t	sc_xfer;

	bus_dma_segment_t	sc_dr;
};

/* Basic driver stuff */
static int   sssdi_match(device_t, cfdata_t, void *);
static void  sssdi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sssdi, sizeof(struct sssdi_softc), sssdi_match, sssdi_attach,
	      NULL, NULL);

/* SD/MMC chip functions */
static int      sssdi_host_reset(sdmmc_chipset_handle_t);
static uint32_t sssdi_host_ocr(sdmmc_chipset_handle_t);
static int      sssdi_maxblklen(sdmmc_chipset_handle_t);
static int      sssdi_card_detect(sdmmc_chipset_handle_t);
static int      sssdi_write_protect(sdmmc_chipset_handle_t);
static int      sssdi_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int      sssdi_bus_clock(sdmmc_chipset_handle_t, int);
static int      sssdi_bus_width(sdmmc_chipset_handle_t, int);
static int	sssdi_bus_rod(sdmmc_chipset_handle_t, int);
static void     sssdi_exec_command(sdmmc_chipset_handle_t, struct sdmmc_command *);
static void     sssdi_card_enable_intr(sdmmc_chipset_handle_t, int);
static void     sssdi_card_intr_ack(sdmmc_chipset_handle_t);

/* Interrupt Handlers */
int sssdi_intr(void *arg);
int sssdi_intr_card(void *arg);

/* Interrupt helper functions */
static void sssdi_enable_intr(struct sssdi_softc *, uint32_t );
void sssdi_disable_intr(struct sssdi_softc *sc, uint32_t i);
void sssdi_clear_intr(struct sssdi_softc *sc);
static int sssdi_wait_intr(struct sssdi_softc *sc, uint32_t mask, int timeout);

/* Programmed I/O transfer helpers */
void sssdi_perform_pio_read(struct sssdi_softc *sc, struct sdmmc_command *cmd);
void sssdi_perform_pio_write(struct sssdi_softc *sc, struct sdmmc_command *cmd);

/* Interrupt helper defines */
#define SDI_CMD_SENT SDIINTMASK_CMD_SENT
#define SDI_CMD_TIMEOUT SDIINTMASK_CMD_TIMEOUT
#define SDI_RESP_FIN SDIINTMASK_RESP
#define SDI_FIFO_RX_FULL SDIINTMASK_RF_FULL
#define SDI_FIFO_RX_LAST SDIINTMASK_RF_LAST
#define SDI_FIFO_TX_EMPTY SDIINTMASK_TF_EMPTY
#define SDI_DATA_FIN SDIINTMASK_DATA_FIN
#define SDI_DATA_TIMEOUT SDIINTMASK_DATA_TIMEOUT

/* Constants */
#define SDI_DMA_WAIT_TIME       5000 /* ms */
#define SDI_CMD_WAIT_TIME       5000 /* ms */

/* SDMMC function structure */
struct sdmmc_chip_functions sssdi_functions = {
	/* host controller reset */
	.host_reset = sssdi_host_reset,

	/* host capabilities */
	.host_ocr = sssdi_host_ocr,
	.host_maxblklen = sssdi_maxblklen,

	/* card detection */
	.card_detect = sssdi_card_detect,

	/* write protect */
	.write_protect = sssdi_write_protect,

	/* bus power, clock frequency and width */
	.bus_power = sssdi_bus_power,
	.bus_clock = sssdi_bus_clock,
	.bus_width = sssdi_bus_width,
	.bus_rod = sssdi_bus_rod,

	/* command execution */
	.exec_command = sssdi_exec_command,

	/* card interrupt */
	.card_enable_intr = sssdi_card_enable_intr,
	.card_intr_ack = sssdi_card_intr_ack,
};

int
sssdi_match(device_t parent, cfdata_t match, void *aux)
{
/*	struct s3c2xx0_attach_args *sa = aux;*/

	/* Not sure how to match here, maybe CPU type? */
	return 1;
}

void
sssdi_attach(device_t parent, device_t self, void *aux)
{
	struct sssdi_softc *sc = device_private(self);
	struct s3c2xx0_attach_args *sa = (struct s3c2xx0_attach_args *)aux;
	struct sdmmcbus_attach_args saa;
	bus_space_tag_t iot = sa->sa_iot;
	uint32_t data;

	sc->dev = self;
	sc->iot = iot;

	if (bus_space_map(iot, S3C2440_SDI_BASE, S3C2440_SDI_SIZE, 0, &sc->ioh) ) {
		printf(": failed to map registers");
		return;
	}

	if (bus_space_map(iot, S3C2440_GPIO_BASE, S3C2440_GPIO_SIZE, 0, &sc->card_ioh) ) {
		printf(": failed to map GPIO memory for card detection");
		return;
	}

	/* Set GPG8 to EINT[16], as it is the card detect line. */
	data = bus_space_read_4(sc->iot, sc->card_ioh, GPIO_PGCON);
	data = GPIO_SET_FUNC(data, 8, 0x2);
	bus_space_write_4(sc->iot, sc->card_ioh, GPIO_PGCON, data);

	/* Set GPH8 to input, as it is used to detect write protection. */
	data = bus_space_read_4(sc->iot, sc->card_ioh, GPIO_PHCON);
	data = GPIO_SET_FUNC(data, 8, 0x00);
	bus_space_write_4(sc->iot, sc->card_ioh, GPIO_PHCON, data);

	mutex_init(&sc->intr_mtx, MUTEX_DEFAULT, IPL_SDMMC);

	cv_init(&sc->intr_cv, "s3c2440_sdiintr");
	sc->intr_status = 0;
	sc->caps = SMC_CAPS_4BIT_MODE | SMC_CAPS_DMA | SMC_CAPS_MULTI_SEG_DMA;

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &sssdi_functions;
	saa.saa_sch = sc;
	saa.saa_dmat = sa->sa_dmat;
	saa.saa_clkmin = s3c2xx0_softc->sc_pclk / 256;
	saa.saa_clkmax = s3c2xx0_softc->sc_pclk / 1; /* PCLK/1 or PCLK/2 depending on how the spec is read */
	saa.saa_caps = sc->caps;

	/* Attach our interrupt handler */
	sc->sc_ih = s3c24x0_intr_establish(S3C2410_INT_SDI, IPL_SDMMC, IST_EDGE_RISING, sssdi_intr, sc);

	/* Attach interrupt handler to detect change in card status */
	s3c2440_extint_establish(16, IPL_SDMMC, IST_EDGE_BOTH, sssdi_intr_card, sc);

	data = bus_space_read_4(s3c2xx0_softc->sc_iot, s3c2xx0_softc->sc_clkman_ioh, CLKMAN_CLKCON);
	bus_space_write_4(s3c2xx0_softc->sc_iot, s3c2xx0_softc->sc_clkman_ioh, CLKMAN_CLKCON, data | CLKCON_SDI);

	(void) sssdi_host_reset(sc);

	printf("\n");

	/* Attach to the generic SD/MMC bus */
	/* Is it a good idea to get the private parts of sdmmc ? */
	sc->sdmmc = config_found(sc->dev, &saa, NULL);

	sc->sc_xfer = s3c2440_dmac_allocate_xfer(M_NOWAIT);
	sc->sc_dr.ds_addr = S3C2440_SDI_BASE+SDI_DAT_LI_W;
	sc->sc_dr.ds_len = 4;
}

int
sssdi_host_reset(sdmmc_chipset_handle_t sch)
{
	struct sssdi_softc *sc = (struct sssdi_softc*)sch;

	/* Note that we do not enable the clock just yet. */
	bus_space_write_4(sc->iot, sc->ioh, SDI_CON, SDICON_SD_RESET |
			  SDICON_CTYP_SD | SDICON_RCV_IO_INT);
	/*	bus_space_write_4(sc->iot, sc->ioh, SDI_CMD_STA, SDICMDSTA_RSP_CRC | SDICMDSTA_CMD_SENT |
		SDICMDSTA_CMD_TIMEOUT | SDICMDSTA_RSP_FIN);*/

	sssdi_clear_intr(sc);
	sssdi_enable_intr(sc, SDI_CMD_SENT | SDI_CMD_TIMEOUT | SDI_DATA_TIMEOUT
			  | SDI_RESP_FIN);

	return 0;
}

uint32_t
sssdi_host_ocr(sdmmc_chipset_handle_t sch)
{
	/* This really ought to be made configurable, I guess... */
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V;
}

int
sssdi_maxblklen(sdmmc_chipset_handle_t sch)
{
	/* The S3C2440 user's manual mentions 4095 as a maximum */
	return 4095;
}

int
sssdi_card_detect(sdmmc_chipset_handle_t sch)
{
	struct sssdi_softc *sc = (struct sssdi_softc*)sch;
	uint32_t data;

	DPRINTF(("sssdi_card_detect\n"));

	data = bus_space_read_4(sc->iot, sc->card_ioh, GPIO_PGDAT);

	/* GPIO Port G, pin 8 is high when card is inserted. */
	if ( (data & (1<<8)) == 0) {
		return 1; /* Card Present */
	} else {
		return 0; /* No Card */
	}
}

int
sssdi_write_protect(sdmmc_chipset_handle_t sch)
{
	struct sssdi_softc *sc = (struct sssdi_softc*)sch;
	uint32_t data;

	data = bus_space_read_4(sc->iot, sc->card_ioh, GPIO_PHDAT);


	/* If GPIO Port H Pin 8 is high, the card is write protected. */
	if ( (data & (1<<8)) ) {
		return 1; /* Write protected */
	} else {
		return 0; /* Writable */
	}
}

int
sssdi_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	/* Do nothing, we can't adjust the bus power */
	return 0;
}

int
sssdi_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct sssdi_softc *sc = (struct sssdi_softc*)sch;
	int div;
	int clock_set = 0;
	int control;
	int pclk = s3c2xx0_softc->sc_pclk/1000; /*Peripheral bus clock in KHz*/

	/* Round peripheral bus clock down to nearest MHz */
	pclk = (pclk / 1000) * 1000;

	control = bus_space_read_4(sc->iot, sc->ioh, SDI_CON);
	bus_space_write_4(sc->iot, sc->ioh, SDI_CON, control & ~SDICON_ENCLK);

	DPRINTF(("sssdi_bus_clock (freq: %d KHz)\n", freq));

	/* If the frequency is zero just keep the clock disabled */
	if (freq == 0)
		return 0;

	for (div = 1; div <= 256; div++) {
		if ( pclk / div <= freq) {
			DPRINTF(("Using divisor %d: %d/%d = %d\n", div, pclk,
				 div, pclk/div));
			clock_set = 1;
			bus_space_write_1(sc->iot, sc->ioh, SDI_PRE, div-1);
			break;
		}
	}

	if (clock_set) {
		bus_space_write_4(sc->iot, sc->ioh,
				  SDI_CON, control | SDICON_ENCLK);
		if (div-1 == bus_space_read_4(sc->iot, sc->ioh, SDI_PRE)) {
			/* Clock successfully set, TODO: how do we fail?! */
		}

		/* We do not need to wait here, as the sdmmc code will do that
		   for us. */
		return 0;
	} else {
		return 1;
	}
}

int
sssdi_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct sssdi_softc *sc = (struct sssdi_softc*)sch;

	sc->width = width;
	return 0;
}

int
sssdi_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return -1;
}

#define SSSDI_TRANSFER_NONE  0
#define SSSDI_TRANSFER_READ  1
#define SSSDI_TRANSFER_WRITE 2

void
sssdi_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct sssdi_softc *sc = (struct sssdi_softc*)sch;
	uint32_t cmd_control;
	int status = 0;
#ifdef SSSDI_DEBUG
	uint32_t data_status;
#endif
	int transfer = SSSDI_TRANSFER_NONE;
	dmac_xfer_t xfer;

	/* Reset all status registers prior to sending a command */
	bus_space_write_4(sc->iot, sc->ioh, SDI_DAT_FSTA, 0xFFFFFFFF);
	bus_space_write_4(sc->iot, sc->ioh, SDI_DAT_STA, 0xFFFFFFFF);
	bus_space_write_4(sc->iot, sc->ioh, SDI_CMD_STA, 0xFFFFFFFF);

	/* Set the argument */
	bus_space_write_4(sc->iot, sc->ioh, SDI_CMD_ARG, cmd->c_arg);

	/* Prepare the value for the command control register */
	cmd_control = (cmd->c_opcode & SDICMDCON_CMD_MASK) |
	  SDICMDCON_HOST_CMD | SDICMDCON_CMST;
	if (cmd->c_flags & SCF_RSP_PRESENT)
		cmd_control |= SDICMDCON_WAIT_RSP;
	if (cmd->c_flags & SCF_RSP_136)
		cmd_control |= SDICMDCON_LONG_RSP;

	if (cmd->c_datalen > 0 && cmd->c_data != NULL) {
		/* TODO: Ensure that the above condition matches the semantics
		         of SDICMDCON_WITH_DATA*/
		DPRINTF(("DATA, datalen: %d, blk_size: %d\n", cmd->c_datalen,
			 cmd->c_blklen));
		cmd_control |= SDICMDCON_WITH_DATA;
	}

	/* Unfortunately we have to set the ABORT_CMD bit when using CMD12 and
	   CMD52.
	   CMD12 is MMC_STOP_TRANSMISSION. I currently do not know what CMD52
	   is, but it is related to SDIO.
	 */
	if (cmd->c_opcode == MMC_STOP_TRANSMISSION) {
		cmd_control |= SDICMDCON_ABORT_CMD;
	}

	/* Prepare SDI for data transfer */
	bus_space_write_4(sc->iot, sc->ioh, SDI_BSIZE, cmd->c_blklen);

	/* Set maximum transfer timeout */
	bus_space_write_4(sc->iot, sc->ioh, SDI_DTIMER, 0x007FFFFF);

	/* Set the timeout as low as possible to trigger timeouts for debugging purposes */
	/*bus_space_write_4(sc->iot, sc->ioh, SDI_DTIMER, 0x00005000);*/

	if ( (cmd->c_flags & SCF_CMD_READ) &&
	     (cmd_control & SDICMDCON_WITH_DATA)) {
		uint32_t data_control;
		DPRINTF(("Reading %d bytes\n", cmd->c_datalen));
		transfer = SSSDI_TRANSFER_READ;

		data_control = SDIDATCON_DATMODE_RECEIVE | SDIDATCON_RACMD |
		  SDIDATCON_DTST | SDIDATCON_BLKMODE |
		  ((cmd->c_datalen / cmd->c_blklen) & SDIDATCON_BLKNUM_MASK) |
		  SDIDATCON_DATA_WORD;

		if (sc->caps & SMC_CAPS_DMA) {
			data_control |= SDIDATCON_ENDMA;
			xfer = sc->sc_xfer;
			xfer->dx_desc[DMAC_DESC_SRC].xd_bus_type = DMAC_BUS_TYPE_PERIPHERAL;
			xfer->dx_desc[DMAC_DESC_SRC].xd_increment = FALSE;
			xfer->dx_desc[DMAC_DESC_SRC].xd_nsegs = 1;
			xfer->dx_desc[DMAC_DESC_SRC].xd_dma_segs = &sc->sc_dr;

			xfer->dx_desc[DMAC_DESC_DST].xd_bus_type = DMAC_BUS_TYPE_SYSTEM;
			xfer->dx_desc[DMAC_DESC_DST].xd_increment = TRUE;
			xfer->dx_desc[DMAC_DESC_DST].xd_nsegs = cmd->c_dmamap->dm_nsegs;
			xfer->dx_desc[DMAC_DESC_DST].xd_dma_segs = cmd->c_dmamap->dm_segs;

			/* Let the SD/MMC peripheral control the DMA transfer */
			xfer->dx_peripheral = DMAC_PERIPH_SDI;
			xfer->dx_xfer_width = DMAC_XFER_WIDTH_32BIT;
		}
		if (sc->width == 4) {
			data_control |= SDIDATCON_WIDEBUS;
		}

		bus_space_write_4(sc->iot, sc->ioh, SDI_DAT_CON, data_control);
	} else if (cmd_control & SDICMDCON_WITH_DATA) {
		/* Write data */

		uint32_t data_control;
		DPRINTF(("Writing %d bytes\n", cmd->c_datalen));
		DPRINTF(("Requesting %d blocks\n",
			 cmd->c_datalen / cmd->c_blklen));
		transfer = SSSDI_TRANSFER_WRITE;
		data_control = SDIDATCON_DATMODE_TRANSMIT | SDIDATCON_BLKMODE |
		  SDIDATCON_TARSP | SDIDATCON_DTST |
		  ((cmd->c_datalen / cmd->c_blklen) & SDIDATCON_BLKNUM_MASK) |
		  SDIDATCON_DATA_WORD;

		if (sc->caps & SMC_CAPS_DMA) {
			data_control |= SDIDATCON_ENDMA;
			xfer = sc->sc_xfer;

			xfer->dx_desc[DMAC_DESC_DST].xd_bus_type = DMAC_BUS_TYPE_PERIPHERAL;
			xfer->dx_desc[DMAC_DESC_DST].xd_increment = FALSE;
			xfer->dx_desc[DMAC_DESC_DST].xd_nsegs = 1;
			xfer->dx_desc[DMAC_DESC_DST].xd_dma_segs = &sc->sc_dr;

			xfer->dx_desc[DMAC_DESC_SRC].xd_bus_type = DMAC_BUS_TYPE_SYSTEM;
			xfer->dx_desc[DMAC_DESC_SRC].xd_increment = TRUE;
			xfer->dx_desc[DMAC_DESC_SRC].xd_nsegs = cmd->c_dmamap->dm_nsegs;
			xfer->dx_desc[DMAC_DESC_SRC].xd_dma_segs = cmd->c_dmamap->dm_segs;

			/* Let the SD/MMC peripheral control the DMA transfer */
			xfer->dx_peripheral = DMAC_PERIPH_SDI;
			xfer->dx_xfer_width = DMAC_XFER_WIDTH_32BIT;
		}
		if (sc->width == 4) {
			data_control |= SDIDATCON_WIDEBUS;
		}

		bus_space_write_4(sc->iot, sc->ioh, SDI_DAT_CON, data_control);
	}

	/* Send command to SDI */
	bus_space_write_4(sc->iot, sc->ioh, SDI_CMD_CON, cmd_control);

	/* Wait for command sent acknowledgement, timeout set to 5000ms */
	status = sssdi_wait_intr(sc, SDI_CMD_SENT | SDI_CMD_TIMEOUT, mstohz(SDI_CMD_WAIT_TIME));

	if (status & SDI_CMD_TIMEOUT) {
		DPRINTF(("Timeout waiting for command acknowledgement\n"));
		cmd->c_error = ETIMEDOUT;
		goto out;
	} else if (status & SDICMDSTA_CMD_SENT) {
		/* Interrupt handler has acknowledged already, we do not need
		   to do anything further here */
	}

	if (!(cmd_control & SDICMDCON_WAIT_RSP)) {
		cmd->c_flags |= SCF_ITSDONE;
		goto out;
	}

	DPRINTF(("waiting for response\n"));

	status = sssdi_wait_intr(sc, SDI_RESP_FIN | SDI_DATA_TIMEOUT, 100);
	if (status & SDI_CMD_TIMEOUT || status & SDI_DATA_TIMEOUT) {
		cmd->c_error = ETIMEDOUT;
		DPRINTF(("Timeout waiting for response\n"));
		goto out;
	}
	DPRINTF(("Got Response\n"));


	if (cmd->c_flags & SCF_RSP_136 ) {
		uint32_t w[4];

		/* We store the response least significant word first */
		w[0] = bus_space_read_4(sc->iot, sc->ioh, SDI_RSP3);
		w[1] = bus_space_read_4(sc->iot, sc->ioh, SDI_RSP2);
		w[2] = bus_space_read_4(sc->iot, sc->ioh, SDI_RSP1);
		w[3] = bus_space_read_4(sc->iot, sc->ioh, SDI_RSP0);

		/* The sdmmc subsystem expects that the response is delivered
		   without the lower 8 bits (CRC + '1' bit) */
		cmd->c_resp[0] = (w[0] >> 8) | ((w[1] & 0xFF) << 24);
		cmd->c_resp[1] = (w[1] >> 8) | ((w[2] & 0XFF) << 24);
		cmd->c_resp[2] = (w[2] >> 8) | ((w[3] & 0XFF) << 24);
		cmd->c_resp[3] = (w[3] >> 8);

	} else {
		cmd->c_resp[0] = bus_space_read_4(sc->iot, sc->ioh, SDI_RSP0);
		cmd->c_resp[1] = bus_space_read_4(sc->iot, sc->ioh, SDI_RSP1);
	}

	DPRINTF(("Response: %X %X %X %X\n",
		 cmd->c_resp[0],
		 cmd->c_resp[1],
		 cmd->c_resp[2],
		 cmd->c_resp[3]));

	status = bus_space_read_4(sc->iot, sc->ioh, SDI_DAT_CNT);

	DPRINTF(("Remaining bytes of current block: %d\n",
		 SDIDATCNT_BLK_CNT(status)));
	DPRINTF(("Remaining Block Number          : %d\n",
		 SDIDATCNT_BLK_NUM_CNT(status)));

#ifdef SSSDI_DEBUG
	data_status = bus_space_read_4(sc->iot, sc->ioh, SDI_DAT_STA);
	printf("SDI Data Status Register Before xfer: 0x%X\n", data_status);
#endif
	if (transfer == SSSDI_TRANSFER_READ) {
		DPRINTF(("Waiting for transfer to complete\n"));

		if (sc->sc_xfer != NULL ) {
			int dma_error = 0;
			/* It might not be very efficient to delay the start of
			   the DMA transfer until now, but it works :-).
			 */
			s3c2440_dmac_start_xfer(sc->sc_xfer);

			/* Wait until the transfer has completed, timeout is
			   500ms */
			dma_error = s3c2440_dmac_wait_xfer(sc->sc_xfer, mstohz(SDI_DMA_WAIT_TIME));
			if (dma_error != 0) {
				//s3c2440_dma_xfer_abort(sc->dma_xfer, mstohz(100)); /* XXX: Handle timeout during abort */
				cmd->c_error = dma_error;
				DPRINTF(("DMA xfer failed: %d\n", dma_error));
				goto out;
			}
		} else {
			DPRINTF(("PIO READ\n"));
			sssdi_perform_pio_read(sc, cmd);
		}
	} else if (transfer == SSSDI_TRANSFER_WRITE) {
		DPRINTF(("Waiting for WRITE transfer to complete\n"));

		if (sc->sc_xfer != NULL) {
			int dma_error = 0;
			s3c2440_dmac_start_xfer(sc->sc_xfer);

			dma_error = s3c2440_dmac_wait_xfer(sc->sc_xfer, mstohz(SDI_DMA_WAIT_TIME));
			if (dma_error != 0) {
				//s3c2440_dma_xfer_abort(sc->dma_xfer, mstohz(100)); /* XXX: Handle timeout during abort*/
				cmd->c_error = dma_error;
				DPRINTF(("DMA xfer failed: %d\n", dma_error));
				goto out;
			}
		} else {
			DPRINTF(("PIO WRITE\n"));
			sssdi_perform_pio_write(sc, cmd);
		}

		if (cmd->c_error == ETIMEDOUT)
			goto out;

		DPRINTF(("Waiting for transfer to complete\n"));
		status = sssdi_wait_intr(sc, SDI_DATA_FIN | SDI_DATA_TIMEOUT, 1000);
		if (status & SDI_CMD_TIMEOUT || status & SDI_DATA_TIMEOUT) {
			cmd->c_error = ETIMEDOUT;
			DPRINTF(("Timeout waiting for data to complete\n"));
			goto out;
		}
		DPRINTF(("Done\n"));

	}


	/* Response has been received, and any data transfer needed has been
	   performed */
	cmd->c_flags |= SCF_ITSDONE;

 out:

#ifdef SSSDI_DEBUG
	data_status = bus_space_read_4(sc->iot, sc->ioh, SDI_DAT_STA);
	printf("SDI Data Status Register after execute: 0x%X\n", data_status);
#endif

	/* Clear status register. Their are cleared on the
	   next sssdi_exec_command  */
	bus_space_write_4(sc->iot, sc->ioh, SDI_CMD_STA, 0xFFFFFFFF);
	bus_space_write_4(sc->iot, sc->ioh, SDI_DAT_CON, 0x0);
}

void sssdi_perform_pio_read(struct sssdi_softc *sc, struct sdmmc_command *cmd)
{
	uint32_t fifo_status;
	int count;
	uint32_t written;
	uint32_t *dest = (uint32_t*)cmd->c_data;

	written = 0;

	while (written < cmd->c_datalen ) {
		/* Wait until the FIFO is full or has the final data.
		   In the latter case it might not get filled. */
		sssdi_wait_intr(sc, SDI_FIFO_RX_FULL | SDI_FIFO_RX_LAST, 1000);

		fifo_status = bus_space_read_4(sc->iot, sc->ioh, SDI_DAT_FSTA);
		count = SDIDATFSTA_FFCNT(fifo_status);

		for(int i=0; i<count; i+=4) {
			uint32_t buf;

			buf = bus_space_read_4(sc->iot, sc->ioh, SDI_DAT_LI_W);
			*dest = buf;
			written += 4;
			dest++;
		}
	}
}

void
sssdi_perform_pio_write(struct sssdi_softc *sc, struct sdmmc_command *cmd)
{
	uint32_t status;
	uint32_t fifo_status;
	int count;
	uint32_t written;
	uint32_t *dest = (uint32_t*)cmd->c_data;

	written = 0;

	while (written < cmd->c_datalen ) {
		/* Wait until the FIFO is full or has the final data.
		   In the latter case it might not get filled. */
		DPRINTF(("Waiting for FIFO to become empty\n"));
		status = sssdi_wait_intr(sc, SDI_FIFO_TX_EMPTY, 1000);

		fifo_status = bus_space_read_4(sc->iot, sc->ioh, SDI_DAT_FSTA);
		DPRINTF(("PIO Write FIFO Status: 0x%X\n", fifo_status));
		count = 64-SDIDATFSTA_FFCNT(fifo_status);

		status = bus_space_read_4(sc->iot, sc->ioh, SDI_DAT_CNT);
		DPRINTF(("Remaining bytes of current block: %d\n",
			 SDIDATCNT_BLK_CNT(status)));
		DPRINTF(("Remaining Block Number          : %d\n",
			 SDIDATCNT_BLK_NUM_CNT(status)));


		status = bus_space_read_4(sc->iot,sc->ioh, SDI_DAT_STA);
		DPRINTF(("PIO Write Data Status: 0x%X\n", status));

		if (status & SDIDATSTA_DATA_TIMEOUT) {
			cmd->c_error = ETIMEDOUT;
			/* Acknowledge the timeout*/
			bus_space_write_4(sc->iot, sc->ioh, SDI_DAT_STA,
					  SDIDATSTA_DATA_TIMEOUT);
			printf("%s: Data timeout\n", device_xname(sc->dev));
			break;
		}

		DPRINTF(("Filling FIFO with %d bytes\n", count));
		for(int i=0; i<count; i+=4) {
			bus_space_write_4(sc->iot, sc->ioh, SDI_DAT_LI_W, *dest);
			written += 4;
			dest++;
		}
	}
}


void
sssdi_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	printf("sssdi_card_enable_intr not implemented\n");
}

void
sssdi_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	printf("sssdi_card_intr_ack not implemented\n");
}

int
sssdi_intr(void *arg)
{
	struct sssdi_softc *sc = (struct sssdi_softc*)arg;
	uint32_t status;
	uint32_t ack_status;

	/* Start by dealing with Command Status */
	ack_status = 0;
	status = bus_space_read_4(sc->iot, sc->ioh, SDI_CMD_STA);

	if (status & SDICMDSTA_CMD_TIMEOUT) {
		ack_status |= SDICMDSTA_CMD_TIMEOUT;
		sc->intr_status |= SDI_CMD_TIMEOUT;
		/*sssdi_disable_intr(sc, SDI_CMD_TIMEOUT);*/
	}
	if (status & SDICMDSTA_CMD_SENT) {
		ack_status |= SDICMDSTA_CMD_SENT;
		sc->intr_status |= SDI_CMD_SENT;
		/*		sssdi_disable_intr(sc, SDI_CMD_SENT);*/
	}
	if (status & SDICMDSTA_RSP_FIN) {
		ack_status |= SDICMDSTA_RSP_FIN;
		sc->intr_status |= SDI_RESP_FIN;
		/*	sssdi_disable_intr(sc, SDI_RESP_FIN);*/
	}
	bus_space_write_4(sc->iot, sc->ioh, SDI_CMD_STA, ack_status);

	/* Next: FIFO Status */
	ack_status = 0;
	status = bus_space_read_4(sc->iot, sc->ioh, SDI_DAT_FSTA);
	if (status & SDIDATFSTA_RF_FULL) {
		ack_status |= SDIDATFSTA_RF_FULL;
		sc->intr_status |= SDI_FIFO_RX_FULL;
		sssdi_disable_intr(sc, SDI_FIFO_RX_FULL);
	}
	if (status & SDIDATFSTA_RF_LAST) {
		ack_status |= SDIDATFSTA_RF_LAST | SDIDATFSTA_RESET;
		sc->intr_status |= SDI_FIFO_RX_LAST;
		sssdi_disable_intr(sc, SDI_FIFO_RX_LAST);
	}
	if (status & SDIDATFSTA_TF_EMPTY) {
		ack_status |= SDIDATFSTA_TF_EMPTY;
		sc->intr_status |= SDI_FIFO_TX_EMPTY;
		sssdi_disable_intr(sc, SDI_FIFO_TX_EMPTY);
	}
	bus_space_write_4(sc->iot, sc->ioh, SDI_DAT_FSTA, ack_status);

	ack_status = 0;
	status = bus_space_read_4(sc->iot, sc->ioh, SDI_DAT_STA);
	if (status & SDIDATSTA_DATA_FIN) {
		DPRINTF(("sssdi_intr: DATA FINISHED\n"));
		ack_status |= SDIDATSTA_DATA_FIN;
		sc->intr_status |= SDI_DATA_FIN;
		sssdi_disable_intr(sc, SDI_DATA_FIN);
	}
	if (status & SDIDATSTA_DATA_TIMEOUT) {
		printf("sssdi_intr: DATA TIMEOUT\n");
		ack_status |= SDIDATSTA_DATA_TIMEOUT;
		sc->intr_status |= SDI_DATA_TIMEOUT;
		/* Data timeout interrupt is always enabled, thus
		   we do not disable it when we have received one. */
		/*sssdi_disable_intr(sc, SDI_DATA_TIMEOUT);*/

		if (sc->sc_xfer != NULL) {
			s3c2440_dmac_abort_xfer(sc->sc_xfer);
		}
	}
	bus_space_write_4(sc->iot, sc->ioh, SDI_DAT_STA, ack_status);

	mutex_enter(&sc->intr_mtx);
	cv_broadcast(&sc->intr_cv);
	mutex_exit(&sc->intr_mtx);

	return 1;
}

int
sssdi_intr_card(void *arg)
{
	struct sssdi_softc *sc = (struct sssdi_softc*)arg;

	/* TODO: If card was removed then abort any current command */

	sdmmc_needs_discover(sc->sdmmc);

	return 1; /* handled */
}

static void
sssdi_enable_intr(struct sssdi_softc *sc, uint32_t i)
{
	uint32_t v = bus_space_read_4(sc->iot, sc->ioh, SDI_INT_MASK);
	bus_space_write_4(sc->iot, sc->ioh, SDI_INT_MASK, v | i );
}

 void
sssdi_disable_intr(struct sssdi_softc *sc, uint32_t i)
{
	uint32_t v = bus_space_read_4(sc->iot, sc->ioh, SDI_INT_MASK);
	bus_space_write_4(sc->iot, sc->ioh, SDI_INT_MASK, v & ~i );
}

 void
sssdi_clear_intr(struct sssdi_softc *sc)
{
	bus_space_write_4(sc->iot, sc->ioh, SDI_INT_MASK, 0x0);
}

static int
sssdi_wait_intr(struct sssdi_softc *sc, uint32_t mask, int timeout)
{
	uint32_t status;

	/* Wait until the command has been sent */
	mutex_enter(&sc->intr_mtx);
	sssdi_enable_intr(sc, mask);
	status = sc->intr_status & mask;
	while(status == 0) {

		if (cv_timedwait(&sc->intr_cv, &sc->intr_mtx, timeout) ==
		    EWOULDBLOCK ) {
			DPRINTF(("Timed out waiting for interrupt from SDI controller\n"));
			status |= SDI_CMD_TIMEOUT;
			break;
		}

		status = sc->intr_status & mask;
	}

	sc->intr_status &= ~status;
	mutex_exit(&sc->intr_mtx);

	return status;
}
