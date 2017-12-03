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
#include "s3csdi.h"

#include <arm/s3c2xx0/s3c2440reg.h>

#include <lib/libsa/stand.h>

#include <machine/int_mwgwtypes.h>
#include <machine/limits.h>

#include <dev/sdmmc/sdmmcreg.h>

#define SDI_REG(reg) (*(volatile uint32_t*)(S3C2440_SDI_BASE+reg))

//#define SSSDI_DEBUG
#ifdef SSSDI_DEBUG
#define DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

struct s3csdi_softc {
	int width;
};

extern int pclk;

static void sssdi_perform_pio_read(struct sdmmc_command *cmd);
//static void sssdi_perform_pio_write(struct sdmmc_command *cmd);

static struct s3csdi_softc s3csdi_softc;

int
s3csd_match(unsigned int tag)
{
	printf("Found S3C2440 SD/MMC\n");
	return 1;
}

void*
s3csd_init(unsigned int tag, uint32_t *caps)
{
	uint32_t data;

	*caps = SMC_CAPS_4BIT_MODE;

	DPRINTF(("CLKCON: 0x%X\n", *(volatile uint32_t*)(S3C2440_CLKMAN_BASE + CLKMAN_CLKCON)));

	DPRINTF(("SDI_INT_MASK: 0x%X\n", SDI_REG(SDI_INT_MASK)));

	SDI_REG(SDI_INT_MASK) = 0x0;
	SDI_REG(SDI_DTIMER) = 0x007FFFFF;

	SDI_REG(SDI_CON) &= ~SDICON_ENCLK;

	SDI_REG(SDI_CON) = SDICON_SD_RESET | SDICON_CTYP_SD;

	/* Set GPG8 to input such that we can check if there is a card present
	 */
	data = *(volatile uint32_t*)(S3C2440_GPIO_BASE+GPIO_PGCON);
	data = GPIO_SET_FUNC(data, 8, 0x00);
	*(volatile uint32_t*)(S3C2440_GPIO_BASE+GPIO_PGCON) = data;

	/* Check if a card is present */
	data = *(volatile uint32_t*)(S3C2440_GPIO_BASE+GPIO_PGDAT);
	if ( (data & (1<<8)) == (1<<8)) {
		printf("No card detected\n");
		/* Pin 8 is low when no card is inserted */
		return 0;
	}
	printf("Card detected\n");

	s3csdi_softc.width = 1;

	/* We have no private data to return, but 0 signals error */
	return (void*)0x01;
}

int
s3csd_bus_clock(void *priv, int freq)
{
	int div;
	int clock_set = 0;
	int control;
	int clk = pclk/1000; /*Peripheral bus clock in KHz*/

	/* Round peripheral bus clock down to nearest MHz */
	clk = (clk / 1000) * 1000;

	control = SDI_REG(SDI_CON);
	SDI_REG(SDI_CON) = control & ~SDICON_ENCLK;


	/* If the frequency is zero just keep the clock disabled */
	if (freq == 0)
		return 0;

	for (div = 1; div <= 256; div++) {
		if ( clk / div <= freq) {
			DPRINTF(("Using divisor %d: %d/%d = %d\n", div, clk,
				 div, clk/div));
			clock_set = 1;
			SDI_REG(SDI_PRE) = div-1;
			break;
		}
	}

	if (clock_set) {
		SDI_REG(SDI_CON) = control | SDICON_ENCLK;
		if (div-1 != SDI_REG(SDI_PRE)) {
			return 1;
		}

		sdmmc_delay(74000/freq);
		/* Wait for 74 SDCLK */
		/* 1/freq is the length of a clock cycle,
		   so we have to wait 1/freq * 74 .
		   74000 / freq should express the delay in us.
		 */
		return 0;
	} else {
		return 1;
	}
}

#define SSSDI_TRANSFER_NONE  0
#define SSSDI_TRANSFER_READ  1
#define SSSDI_TRANSFER_WRITE 2

void
s3csd_exec_cmd(void *priv, struct sdmmc_command *cmd)
{
	uint32_t cmd_control;
	int status = 0;
	uint32_t data_status;
	int transfer = SSSDI_TRANSFER_NONE;

	DPRINTF(("s3csd_exec_cmd\n"));

	SDI_REG(SDI_DAT_FSTA) = 0xFFFFFFFF;
	SDI_REG(SDI_DAT_STA) = 0xFFFFFFFF;
	SDI_REG(SDI_CMD_STA) = 0xFFFFFFFF;

	SDI_REG(SDI_CMD_ARG) = cmd->c_arg;

	cmd_control = (cmd->c_opcode & SDICMDCON_CMD_MASK) |
	  SDICMDCON_HOST_CMD | SDICMDCON_CMST;
	if (cmd->c_flags & SCF_RSP_PRESENT)
		cmd_control |= SDICMDCON_WAIT_RSP;
	if (cmd->c_flags & SCF_RSP_136)
		cmd_control |= SDICMDCON_LONG_RSP;

	if (cmd->c_datalen > 0 && cmd->c_data != NULL) {
		/* TODO: Ensure that the above condition matches the semantics
		         of SDICMDCON_WITH_DATA*/
		DPRINTF(("DATA, datalen: %d, blk_size: %d, offset: %d\n", cmd->c_datalen,
			 cmd->c_blklen, cmd->c_arg));
		cmd_control |= SDICMDCON_WITH_DATA;
	}

	if (cmd->c_opcode == MMC_STOP_TRANSMISSION) {
		cmd_control |= SDICMDCON_ABORT_CMD;
	}

	SDI_REG(SDI_DTIMER) = 0x007FFFFF;
	SDI_REG(SDI_BSIZE) = cmd->c_blklen;

	if ( (cmd->c_flags & SCF_CMD_READ) &&
	     (cmd_control & SDICMDCON_WITH_DATA)) {
		uint32_t data_control;
		DPRINTF(("Reading %d bytes\n", cmd->c_datalen));
		transfer = SSSDI_TRANSFER_READ;

		data_control = SDIDATCON_DATMODE_RECEIVE | SDIDATCON_RACMD |
		  SDIDATCON_DTST | SDIDATCON_BLKMODE |
		  ((cmd->c_datalen / cmd->c_blklen) & SDIDATCON_BLKNUM_MASK) |
		  SDIDATCON_DATA_WORD;


		if (s3csdi_softc.width == 4) {
			data_control |= SDIDATCON_WIDEBUS;
		}

		SDI_REG(SDI_DAT_CON) = data_control;
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

/*		if (sc->width == 4) {
			data_control |= SDIDATCON_WIDEBUS;
			}*/

		SDI_REG(SDI_DAT_CON) = data_control;
	}

	DPRINTF(("SID_CMD_CON: 0x%X\n", cmd_control));
	/* Send command to SDI */
	SDI_REG(SDI_CMD_CON) = cmd_control;
	DPRINTF(("Status before cmd sent: 0x%X\n", SDI_REG(SDI_CMD_STA)));
	DPRINTF(("Waiting for command being sent\n"));
	while( !(SDI_REG(SDI_CMD_STA) & SDICMDSTA_CMD_SENT));
	DPRINTF(("Command has been sent\n"));

	//SDI_REG(SDI_CMD_STA) |= SDICMDSTA_CMD_SENT;

	if (!(cmd_control & SDICMDCON_WAIT_RSP)) {
		SDI_REG(SDI_CMD_STA) |= SDICMDSTA_CMD_SENT;
		cmd->c_flags |= SCF_ITSDONE;
		goto out;
	}

	DPRINTF(("waiting for response\n"));
	while(1) {
		status = SDI_REG(SDI_CMD_STA);
		if (status & SDICMDSTA_RSP_FIN) {
			break;
		}
		if (status & SDICMDSTA_CMD_TIMEOUT) {
			break;
		}
	}

	DPRINTF(("Status: 0x%X\n", status));
	if (status & SDICMDSTA_CMD_TIMEOUT) {
		cmd->c_error = ETIMEDOUT;
		DPRINTF(("Timeout waiting for response\n"));
		goto out;
	}
	DPRINTF(("Got Response\n"));

	if (cmd->c_flags & SCF_RSP_136 ) {
		uint32_t w[4];

		/* We store the response least significant word first */
		w[0] = SDI_REG(SDI_RSP3);
		w[1] = SDI_REG(SDI_RSP2);
		w[2] = SDI_REG(SDI_RSP1);
		w[3] = SDI_REG(SDI_RSP0);

		/* The sdmmc subsystem expects that the response is delivered
		   without the lower 8 bits (CRC + '1' bit) */
		cmd->c_resp[0] = (w[0] >> 8) | ((w[1] & 0xFF) << 24);
		cmd->c_resp[1] = (w[1] >> 8) | ((w[2] & 0XFF) << 24);
		cmd->c_resp[2] = (w[2] >> 8) | ((w[3] & 0XFF) << 24);
		cmd->c_resp[3] = (w[3] >> 8);

	} else {
		cmd->c_resp[0] = SDI_REG(SDI_RSP0);
		cmd->c_resp[1] = SDI_REG(SDI_RSP1);
	}

	DPRINTF(("Response: %X %X %X %X\n",
		 cmd->c_resp[0],
		 cmd->c_resp[1],
		 cmd->c_resp[2],
		 cmd->c_resp[3]));

	status = SDI_REG(SDI_DAT_CNT);

	DPRINTF(("Remaining bytes of current block: %d\n",
		 SDIDATCNT_BLK_CNT(status)));
	DPRINTF(("Remaining Block Number          : %d\n",
		 SDIDATCNT_BLK_NUM_CNT(status)));

	data_status = SDI_REG(SDI_DAT_STA);

	DPRINTF(("SDI Data Status Register Before xfer: 0x%X\n", data_status));

	if (data_status & SDIDATSTA_DATA_TIMEOUT) {
		cmd->c_error = ETIMEDOUT;
		DPRINTF(("Timeout waiting for data\n"));
		goto out;
	}


	if (transfer == SSSDI_TRANSFER_READ) {
		DPRINTF(("Waiting for transfer to complete\n"));

		sssdi_perform_pio_read(cmd);
	} else if (transfer == SSSDI_TRANSFER_WRITE) {

/*		DPRINTF(("PIO WRITE\n"));
		sssdi_perform_pio_write(sc, cmd);

		if (cmd->c_error == ETIMEDOUT)
		goto out;*/
	}


	/* Response has been received, and any data transfer needed has been
	   performed */
	cmd->c_flags |= SCF_ITSDONE;

 out:

	data_status = SDI_REG(SDI_DAT_STA);
	DPRINTF(("SDI Data Status Register after execute: 0x%X\n", data_status));

	/* Clear status register. Their are cleared on the
	   next sssdi_exec_command  */
	SDI_REG(SDI_CMD_STA) =  0xFFFFFFFF;
	SDI_REG(SDI_DAT_CON) =  0x0;
}

void
sssdi_perform_pio_read(struct sdmmc_command *cmd)
{
	uint32_t status;
	uint32_t fifo_status;
	int count;
	uint32_t written;
	uint8_t *dest = (uint8_t*)cmd->c_data;
	int i;

	written = 0;

	while (written < cmd->c_datalen ) {
		/* Wait until the FIFO is full or has the final data.
		   In the latter case it might not get filled. */
		//status = sssdi_wait_intr(sc, SDI_FIFO_RX_FULL | SDI_FIFO_RX_LAST, 1000);
		//printf("Waiting for FIFO (got %d / %d)\n", written, cmd->c_datalen);
		do {
			status = SDI_REG(SDI_DAT_FSTA);
		} while( !(status & SDIDATFSTA_RF_FULL) && !(status & SDIDATFSTA_RF_LAST));
		//printf("Done\n");

		fifo_status = SDI_REG(SDI_DAT_FSTA);
		count = SDIDATFSTA_FFCNT(fifo_status);

		//printf("Writing %d bytes to %p\n", count, dest);
		for(i=0; i<count; i+=4) {
			uint32_t buf;

			buf = SDI_REG(SDI_DAT_LI_W);
			*dest = (buf & 0xFF); dest++;
			*dest = (buf >> 8) & 0xFF; dest++;
			*dest = (buf >> 16) & 0xFF; dest++;
			*dest = (buf >> 24) & 0xFF; dest++;
			written += 4;
		}
	}
}

#if 0
void
sssdi_perform_pio_write(struct sdmmc_command *cmd)
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
#endif

int
s3csd_host_ocr(void *priv)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V;
}

int
s3csd_bus_power(void *priv, int ocr)
{
	return 0;
}

int
s3csd_bus_width(void *priv, int width)
{
	s3csdi_softc.width = width;
	return 0;
}

int
s3csd_get_max_bus_clock(void *priv)
{
	return pclk / 1;
}
