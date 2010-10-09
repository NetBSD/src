/*	$NetBSD: sdmmc_mem.c,v 1.1.4.6 2010/10/09 03:32:24 yamt Exp $	*/
/*	$OpenBSD: sdmmc_mem.c,v 1.10 2009/01/09 10:55:22 jsg Exp $	*/

/*
 * Copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
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

/*-
 * Copyright (c) 2007-2010 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Routines for SD/MMC memory cards. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sdmmc_mem.c,v 1.1.4.6 2010/10/09 03:32:24 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/sdmmc/sdmmcvar.h>

#ifdef SDMMC_DEBUG
#define DPRINTF(s)	do { printf s; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s)	do {} while (/*CONSTCOND*/0)
#endif

static int sdmmc_mem_sd_init(struct sdmmc_softc *, struct sdmmc_function *);
static int sdmmc_mem_mmc_init(struct sdmmc_softc *, struct sdmmc_function *);
static int sdmmc_mem_send_cid(struct sdmmc_softc *, sdmmc_response *);
static int sdmmc_mem_send_csd(struct sdmmc_softc *, struct sdmmc_function *,
    sdmmc_response *);
static int sdmmc_mem_send_scr(struct sdmmc_softc *, struct sdmmc_function *,
    uint32_t scr[2]);
static int sdmmc_mem_decode_scr(struct sdmmc_softc *, struct sdmmc_function *);
static int sdmmc_mem_send_cxd_data(struct sdmmc_softc *, int, void *, size_t);
static int sdmmc_set_bus_width(struct sdmmc_function *, int);
static int sdmmc_mem_sd_switch(struct sdmmc_function *, int, int, int, void *);
static int sdmmc_mem_mmc_switch(struct sdmmc_function *, uint8_t, uint8_t,
    uint8_t);
static int sdmmc_mem_spi_read_ocr(struct sdmmc_softc *, uint32_t, uint32_t *);
static int sdmmc_mem_single_read_block(struct sdmmc_function *, uint32_t,
    u_char *, size_t);
static int sdmmc_mem_single_write_block(struct sdmmc_function *, uint32_t,
    u_char *, size_t);
static int sdmmc_mem_read_block_subr(struct sdmmc_function *, uint32_t,
    u_char *, size_t);
static int sdmmc_mem_write_block_subr(struct sdmmc_function *, uint32_t,
    u_char *, size_t);

/*
 * Initialize SD/MMC memory cards and memory in SDIO "combo" cards.
 */
int
sdmmc_mem_enable(struct sdmmc_softc *sc)
{
	uint32_t host_ocr;
	uint32_t card_ocr;
	uint32_t ocr = 0;
	int error;

	SDMMC_LOCK(sc);

	/* Set host mode to SD "combo" card or SD memory-only. */
	SET(sc->sc_flags, SMF_SD_MODE|SMF_MEM_MODE);

	if (ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE))
		sdmmc_spi_chip_initialize(sc->sc_spi_sct, sc->sc_sch);

	/* Reset memory (*must* do that before CMD55 or CMD1). */
	sdmmc_go_idle_state(sc);

	/* Check SD Ver.2 */
	error = sdmmc_mem_send_if_cond(sc, 0x1aa, &card_ocr);
	if (error == 0 && card_ocr == 0x1aa)
		SET(ocr, MMC_OCR_HCS);

	/*
	 * Read the SD/MMC memory OCR value by issuing CMD55 followed
	 * by ACMD41 to read the OCR value from memory-only SD cards.
	 * MMC cards will not respond to CMD55 or ACMD41 and this is
	 * how we distinguish them from SD cards.
	 */
mmc_mode:
	error = sdmmc_mem_send_op_cond(sc,
	  ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE) ? ocr : 0, &card_ocr);
	if (error) {
		if (ISSET(sc->sc_flags, SMF_SD_MODE) &&
		    !ISSET(sc->sc_flags, SMF_IO_MODE)) {
			/* Not a SD card, switch to MMC mode. */
			DPRINTF(("%s: switch to MMC mode\n", SDMMCDEVNAME(sc)));
			CLR(sc->sc_flags, SMF_SD_MODE);
			goto mmc_mode;
		}
		if (!ISSET(sc->sc_flags, SMF_SD_MODE)) {
			DPRINTF(("%s: couldn't read memory OCR\n",
			    SDMMCDEVNAME(sc)));
			goto out;
		} else {
			/* Not a "combo" card. */
			CLR(sc->sc_flags, SMF_MEM_MODE);
			error = 0;
			goto out;
		}
	}
	if (ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
		/* get card OCR */
		error = sdmmc_mem_spi_read_ocr(sc, ocr, &card_ocr);
		if (error) {
			DPRINTF(("%s: couldn't read SPI memory OCR\n",
			    SDMMCDEVNAME(sc)));
			goto out;
		}
	}

	/* Set the lowest voltage supported by the card and host. */
	host_ocr = sdmmc_chip_host_ocr(sc->sc_sct, sc->sc_sch);
	error = sdmmc_set_bus_power(sc, host_ocr, card_ocr);
	if (error) {
		DPRINTF(("%s: couldn't supply voltage requested by card\n",
		    SDMMCDEVNAME(sc)));
		goto out;
	}
	host_ocr &= card_ocr;
	host_ocr |= ocr;

	/* Send the new OCR value until all cards are ready. */
	error = sdmmc_mem_send_op_cond(sc, host_ocr, NULL);
	if (error) {
		DPRINTF(("%s: couldn't send memory OCR\n", SDMMCDEVNAME(sc)));
		goto out;
	}

out:
	SDMMC_UNLOCK(sc);

	return error;
}

/*
 * Read the CSD and CID from all cards and assign each card a unique
 * relative card address (RCA).  CMD2 is ignored by SDIO-only cards.
 */
void
sdmmc_mem_scan(struct sdmmc_softc *sc)
{
	sdmmc_response resp;
	struct sdmmc_function *sf;
	uint16_t next_rca;
	int error;
	int retry;

	SDMMC_LOCK(sc);

	/*
	 * CMD2 is a broadcast command understood by SD cards and MMC
	 * cards.  All cards begin to respond to the command, but back
	 * off if another card drives the CMD line to a different level.
	 * Only one card will get its entire response through.  That
	 * card remains silent once it has been assigned a RCA.
	 */
	for (retry = 0; retry < 100; retry++) {
		error = sdmmc_mem_send_cid(sc, &resp);
		if (error) {
			if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE) &&
			    error == ETIMEDOUT) {
				/* No more cards there. */
				break;
			}
			DPRINTF(("%s: couldn't read CID\n", SDMMCDEVNAME(sc)));
			break;
		}

		/* In MMC mode, find the next available RCA. */
		next_rca = 1;
		if (!ISSET(sc->sc_flags, SMF_SD_MODE)) {
			SIMPLEQ_FOREACH(sf, &sc->sf_head, sf_list)
				next_rca++;
		}

		/* Allocate a sdmmc_function structure. */
		sf = sdmmc_function_alloc(sc);
		sf->rca = next_rca;

		/*
		 * Remember the CID returned in the CMD2 response for
		 * later decoding.
		 */
		memcpy(sf->raw_cid, resp, sizeof(sf->raw_cid));

		/*
		 * Silence the card by assigning it a unique RCA, or
		 * querying it for its RCA in the case of SD.
		 */
		if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
			if (sdmmc_set_relative_addr(sc, sf) != 0) {
				aprint_error_dev(sc->sc_dev,
				    "couldn't set mem RCA\n");
				sdmmc_function_free(sf);
				break;
			}
		}

		/*
		 * If this is a memory-only card, the card responding
		 * first becomes an alias for SDIO function 0.
		 */
		if (sc->sc_fn0 == NULL)
			sc->sc_fn0 = sf;

		SIMPLEQ_INSERT_TAIL(&sc->sf_head, sf, sf_list);

		/* only one function in SPI mode */
		if (ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE))
			break;
	}

	if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE))
		/* Go to Data Transfer Mode, if possible. */
		sdmmc_chip_bus_rod(sc->sc_sct, sc->sc_sch, 0);

	/*
	 * All cards are either inactive or awaiting further commands.
	 * Read the CSDs and decode the raw CID for each card.
	 */
	SIMPLEQ_FOREACH(sf, &sc->sf_head, sf_list) {
		error = sdmmc_mem_send_csd(sc, sf, &resp);
		if (error) {
			SET(sf->flags, SFF_ERROR);
			continue;
		}

		if (sdmmc_decode_csd(sc, resp, sf) != 0 ||
		    sdmmc_decode_cid(sc, sf->raw_cid, sf) != 0) {
			SET(sf->flags, SFF_ERROR);
			continue;
		}

#ifdef SDMMC_DEBUG
		printf("%s: CID: ", SDMMCDEVNAME(sc));
		sdmmc_print_cid(&sf->cid);
#endif
	}

	SDMMC_UNLOCK(sc);
}

int
sdmmc_decode_csd(struct sdmmc_softc *sc, sdmmc_response resp,
    struct sdmmc_function *sf)
{
	/* TRAN_SPEED(2:0): transfer rate exponent */
	static const int speed_exponent[8] = {
		100 *    1,	/* 100 Kbits/s */
		  1 * 1000,	/*   1 Mbits/s */
		 10 * 1000,	/*  10 Mbits/s */
		100 * 1000,	/* 100 Mbits/s */
		         0,
		         0,
		         0,
		         0,
	};
	/* TRAN_SPEED(6:3): time mantissa */
	static const int speed_mantissa[16] = {
		0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80,
	};
	struct sdmmc_csd *csd = &sf->csd;
	int e, m;

	if (ISSET(sc->sc_flags, SMF_SD_MODE)) {
		/*
		 * CSD version 1.0 corresponds to SD system
		 * specification version 1.0 - 1.10. (SanDisk, 3.5.3)
		 */
		csd->csdver = SD_CSD_CSDVER(resp);
		switch (csd->csdver) {
		case SD_CSD_CSDVER_2_0:
			DPRINTF(("%s: SD Ver.2.0\n", SDMMCDEVNAME(sc)));
			SET(sf->flags, SFF_SDHC);
			csd->capacity = SD_CSD_V2_CAPACITY(resp);
			csd->read_bl_len = SD_CSD_V2_BL_LEN;
			csd->ccc = SD_CSD_CCC(resp);
			break;

		case SD_CSD_CSDVER_1_0:
			DPRINTF(("%s: SD Ver.1.0\n", SDMMCDEVNAME(sc)));
			csd->capacity = SD_CSD_CAPACITY(resp);
			csd->read_bl_len = SD_CSD_READ_BL_LEN(resp);
			break;

		default:
			aprint_error_dev(sc->sc_dev,
			    "unknown SD CSD structure version 0x%x\n",
			    csd->csdver);
			return 1;
		}

		csd->mmcver = SD_CSD_MMCVER(resp);
		csd->write_bl_len = SD_CSD_WRITE_BL_LEN(resp);
		csd->r2w_factor = SD_CSD_R2W_FACTOR(resp);
		e = SD_CSD_SPEED_EXP(resp);
		m = SD_CSD_SPEED_MANT(resp);
		csd->tran_speed = speed_exponent[e] * speed_mantissa[m] / 10;
	} else {
		csd->csdver = MMC_CSD_CSDVER(resp);
		if (csd->csdver != MMC_CSD_CSDVER_1_0 &&
		    csd->csdver != MMC_CSD_CSDVER_2_0) {
			aprint_error_dev(sc->sc_dev,
			    "unknown MMC CSD structure version 0x%x\n",
			    csd->csdver);
			return 1;
		}

		csd->mmcver = MMC_CSD_MMCVER(resp);
		csd->capacity = MMC_CSD_CAPACITY(resp);
		csd->read_bl_len = MMC_CSD_READ_BL_LEN(resp);
		csd->write_bl_len = MMC_CSD_WRITE_BL_LEN(resp);
		csd->r2w_factor = MMC_CSD_R2W_FACTOR(resp);
		e = MMC_CSD_TRAN_SPEED_EXP(resp);
		m = MMC_CSD_TRAN_SPEED_MANT(resp);
		csd->tran_speed = speed_exponent[e] * speed_mantissa[m] / 10;
	}
	if ((1 << csd->read_bl_len) > SDMMC_SECTOR_SIZE)
		csd->capacity *= (1 << csd->read_bl_len) / SDMMC_SECTOR_SIZE;

#ifdef SDMMC_DUMP_CSD
	sdmmc_print_csd(resp, csd);
#endif

	return 0;
}

int
sdmmc_decode_cid(struct sdmmc_softc *sc, sdmmc_response resp,
    struct sdmmc_function *sf)
{
	struct sdmmc_cid *cid = &sf->cid;

	if (ISSET(sc->sc_flags, SMF_SD_MODE)) {
		cid->mid = SD_CID_MID(resp);
		cid->oid = SD_CID_OID(resp);
		SD_CID_PNM_CPY(resp, cid->pnm);
		cid->rev = SD_CID_REV(resp);
		cid->psn = SD_CID_PSN(resp);
		cid->mdt = SD_CID_MDT(resp);
	} else {
		switch(sf->csd.mmcver) {
		case MMC_CSD_MMCVER_1_0:
		case MMC_CSD_MMCVER_1_4:
			cid->mid = MMC_CID_MID_V1(resp);
			MMC_CID_PNM_V1_CPY(resp, cid->pnm);
			cid->rev = MMC_CID_REV_V1(resp);
			cid->psn = MMC_CID_PSN_V1(resp);
			cid->mdt = MMC_CID_MDT_V1(resp);
			break;
		case MMC_CSD_MMCVER_2_0:
		case MMC_CSD_MMCVER_3_1:
		case MMC_CSD_MMCVER_4_0:
			cid->mid = MMC_CID_MID_V2(resp);
			cid->oid = MMC_CID_OID_V2(resp);
			MMC_CID_PNM_V2_CPY(resp, cid->pnm);
			cid->psn = MMC_CID_PSN_V2(resp);
			break;
		default:
			aprint_error_dev(sc->sc_dev, "unknown MMC version %d\n",
			    sf->csd.mmcver);
			return 1;
		}
	}
	return 0;
}

void
sdmmc_print_cid(struct sdmmc_cid *cid)
{

	printf("mid=0x%02x oid=0x%04x pnm=\"%s\" rev=0x%02x psn=0x%08x"
	    " mdt=%03x\n", cid->mid, cid->oid, cid->pnm, cid->rev, cid->psn,
	    cid->mdt);
}

#ifdef SDMMC_DUMP_CSD
void
sdmmc_print_csd(sdmmc_response resp, struct sdmmc_csd *csd)
{

	printf("csdver = %d\n", csd->csdver);
	printf("mmcver = %d\n", csd->mmcver);
	printf("capacity = %08x\n", csd->capacity);
	printf("read_bl_len = %d\n", csd->read_bl_len);
	printf("write_cl_len = %d\n", csd->write_bl_len);
	printf("r2w_factor = %d\n", csd->r2w_factor);
	printf("tran_speed = %d\n", csd->tran_speed);
}
#endif

/*
 * Initialize a SD/MMC memory card.
 */
int
sdmmc_mem_init(struct sdmmc_softc *sc, struct sdmmc_function *sf)
{
	int error = 0;

	SDMMC_LOCK(sc);

	if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
		error = sdmmc_select_card(sc, sf);
		if (error)
			goto out;
	}

	if (!ISSET(sf->flags, SFF_SDHC)) {
		error = sdmmc_mem_set_blocklen(sc, sf);
		if (error)
			goto out;
	}

	if (ISSET(sc->sc_flags, SMF_SD_MODE))
		error = sdmmc_mem_sd_init(sc, sf);
	else
		error = sdmmc_mem_mmc_init(sc, sf);

out:
	SDMMC_UNLOCK(sc);

	return error;
}

/*
 * Get or set the card's memory OCR value (SD or MMC).
 */
int
sdmmc_mem_send_op_cond(struct sdmmc_softc *sc, uint32_t ocr, uint32_t *ocrp)
{
	struct sdmmc_command cmd;
	int error;
	int retry;

	/* Don't lock */

	/*
	 * If we change the OCR value, retry the command until the OCR
	 * we receive in response has the "CARD BUSY" bit set, meaning
	 * that all cards are ready for identification.
	 */
	for (retry = 0; retry < 100; retry++) {
		memset(&cmd, 0, sizeof(cmd));
		cmd.c_arg = !ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE) ?
		    ocr : (ocr & MMC_OCR_HCS);
		cmd.c_flags = SCF_CMD_BCR | SCF_RSP_R3 | SCF_RSP_SPI_R1;

		if (ISSET(sc->sc_flags, SMF_SD_MODE)) {
			cmd.c_opcode = SD_APP_OP_COND;
			error = sdmmc_app_command(sc, NULL, &cmd);
		} else {
			cmd.c_opcode = MMC_SEND_OP_COND;
			error = sdmmc_mmc_command(sc, &cmd);
		}
		if (error)
			break;

		if (ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
			if (!ISSET(MMC_SPI_R1(cmd.c_resp), R1_SPI_IDLE))
				break;
		} else {
			if (ISSET(MMC_R3(cmd.c_resp), MMC_OCR_MEM_READY) ||
			    ocr == 0)
				break;
		}

		error = ETIMEDOUT;
		sdmmc_delay(10000);
	}
	if (error == 0 &&
	    ocrp != NULL &&
	    !ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE))
		*ocrp = MMC_R3(cmd.c_resp);
	DPRINTF(("%s: sdmmc_mem_send_op_cond: error=%d, ocr=%#x\n",
	    SDMMCDEVNAME(sc), error, MMC_R3(cmd.c_resp)));
	return error;
}

int
sdmmc_mem_send_if_cond(struct sdmmc_softc *sc, uint32_t ocr, uint32_t *ocrp)
{
	struct sdmmc_command cmd;
	int error;

	/* Don't lock */

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_arg = ocr;
	cmd.c_flags = SCF_CMD_BCR | SCF_RSP_R7 | SCF_RSP_SPI_R7;
	cmd.c_opcode = SD_SEND_IF_COND;

	error = sdmmc_mmc_command(sc, &cmd);
	if (error == 0 && ocrp != NULL) {
		if (ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
			*ocrp = MMC_SPI_R7(cmd.c_resp);
		} else {
			*ocrp = MMC_R7(cmd.c_resp);
		}
		DPRINTF(("%s: sdmmc_mem_send_if_cond: error=%d, ocr=%#x\n",
		    SDMMCDEVNAME(sc), error, *ocrp));
	}
	return error;
}

/*
 * Set the read block length appropriately for this card, according to
 * the card CSD register value.
 */
int
sdmmc_mem_set_blocklen(struct sdmmc_softc *sc, struct sdmmc_function *sf)
{
	struct sdmmc_command cmd;
	int error;

	/* Don't lock */

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_SET_BLOCKLEN;
	cmd.c_arg = SDMMC_SECTOR_SIZE;
	cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1 | SCF_RSP_SPI_R1;

	error = sdmmc_mmc_command(sc, &cmd);

	DPRINTF(("%s: sdmmc_mem_set_blocklen: read_bl_len=%d sector_size=%d\n",
	    SDMMCDEVNAME(sc), 1 << sf->csd.read_bl_len, SDMMC_SECTOR_SIZE));

	return error;
}

static int
sdmmc_mem_sd_init(struct sdmmc_softc *sc, struct sdmmc_function *sf)
{
	struct {
		int v;
		int freq;
	} switch_group0_functions [] = {
		/* Default/SDR12 */
		{ MMC_OCR_1_7V_1_8V | MMC_OCR_1_8V_1_9V |
		  MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V,	 25000 },

		/* High-Speed/SDR25 */
		{ MMC_OCR_1_7V_1_8V | MMC_OCR_1_8V_1_9V |
		  MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V,	 50000 },

		/* SDR50 */
		{ MMC_OCR_1_7V_1_8V | MMC_OCR_1_8V_1_9V,	100000 },

		/* SDR104 */
		{ MMC_OCR_1_7V_1_8V | MMC_OCR_1_8V_1_9V,	208000 },

		/* DDR50 */
		{ MMC_OCR_1_7V_1_8V | MMC_OCR_1_8V_1_9V,	 50000 },
	};
	int host_ocr, support_func, best_func, error, g, i;
	char status[64];

	error = sdmmc_mem_send_scr(sc, sf, sf->raw_scr);
	if (error) {
		aprint_error_dev(sc->sc_dev, "SD_SEND_SCR send failed.\n");
		return error;
	}
	error = sdmmc_mem_decode_scr(sc, sf);
	if (error)
		return error;

	if (ISSET(sc->sc_caps, SMC_CAPS_4BIT_MODE) &&
	    ISSET(sf->scr.bus_width, SCR_SD_BUS_WIDTHS_4BIT)) {
		error = sdmmc_set_bus_width(sf, 4);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "can't change bus width (%d bit)\n", 4);
			return error;
		}
		sf->width = 4;
	} else
		sf->width = 1;

	if (sf->scr.sd_spec >= SCR_SD_SPEC_VER_1_10 &&
	    ISSET(sf->csd.ccc, SD_CSD_CCC_SWITCH)) {
		error = sdmmc_mem_sd_switch(sf, 0, 1, 0, status);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "switch func mode 0 failed\n");
			return error;
		}

		host_ocr = sdmmc_chip_host_ocr(sc->sc_sct, sc->sc_sch);
		support_func = SFUNC_STATUS_GROUP(status, 1);
		best_func = 0;
		for (i = 0, g = 1;
		    i < __arraycount(switch_group0_functions); i++, g <<= 1) {
			if (!(switch_group0_functions[i].v & host_ocr))
				continue;
			if (g & support_func)
				best_func = i;
		}
		if (best_func != 0) {
			error =
			    sdmmc_mem_sd_switch(sf, 1, 1, best_func, status);
			if (error) {
				aprint_error_dev(sc->sc_dev,
				    "switch func mode 1 failed:"
				    " group 1 function %d(0x%2x)\n",
				    best_func, support_func);
				return error;
			}
			sf->csd.tran_speed =
			    switch_group0_functions[best_func].freq;

			/* Wait 400KHz x 8 clock */
			delay(1);
			if (sc->sc_busclk > sf->csd.tran_speed)
				sc->sc_busclk = sf->csd.tran_speed;

			error = sdmmc_chip_bus_clock(sc->sc_sct, sc->sc_sch,
			    sc->sc_busclk);
			if (error) {
				aprint_error_dev(sc->sc_dev,
				    "can't change bus clock\n");
				return error;
			}
		} else
			if (sc->sc_busclk > sf->csd.tran_speed)
				sc->sc_busclk = sf->csd.tran_speed;
	}

	return 0;
}

static int
sdmmc_mem_mmc_init(struct sdmmc_softc *sc, struct sdmmc_function *sf)
{
	int width, value, hs_timing, error;
	char ext_csd[512];

	if (sf->csd.mmcver >= MMC_CSD_MMCVER_4_0) {
		error = sdmmc_mem_send_cxd_data(sc,
		    MMC_SEND_EXT_CSD, ext_csd, sizeof(ext_csd));
		if (error) {
			aprint_error_dev(sc->sc_dev, "can't read EXT_CSD\n");
			return error;
		}
		if (ext_csd[EXT_CSD_STRUCTURE] > EXT_CSD_STRUCTURE_VER_1_2) {
			aprint_error_dev(sc->sc_dev,
			    "unrecognised future version\n");
			return error;
		}
		hs_timing = 0;
		switch (ext_csd[EXT_CSD_CARD_TYPE]) {
		case EXT_CSD_CARD_TYPE_26M:
			sf->csd.tran_speed = 26000;	/* 26MHz */
			break;

		case EXT_CSD_CARD_TYPE_52M | EXT_CSD_CARD_TYPE_26M:
			sf->csd.tran_speed = 52000;	/* 52MHz */
			hs_timing = 1;

			error = sdmmc_mem_mmc_switch(sf, EXT_CSD_CMD_SET_NORMAL,
			    EXT_CSD_HS_TIMING, hs_timing);
			if (error) {
				aprint_error_dev(sc->sc_dev,
				    "can't change high speed\n");
				return error;
			}
			break;

		default:
			aprint_error_dev(sc->sc_dev,
			    "unknwon CARD_TYPE: 0x%x\n",
			    ext_csd[EXT_CSD_CARD_TYPE]);
			return error;
		}
		if (sc->sc_busclk > sf->csd.tran_speed)
			sc->sc_busclk = sf->csd.tran_speed;
		error =
		    sdmmc_chip_bus_clock(sc->sc_sct, sc->sc_sch, sc->sc_busclk);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "can't change bus clock\n");
			return error;
		}
		if (hs_timing) {
			error = sdmmc_mem_send_cxd_data(sc,
			    MMC_SEND_EXT_CSD, ext_csd, sizeof(ext_csd));
			if (error) {
				aprint_error_dev(sc->sc_dev,
				    "can't re-read EXT_CSD\n");
				return error;
			}
			if (ext_csd[EXT_CSD_HS_TIMING] != hs_timing) {
				aprint_error_dev(sc->sc_dev,
				    "HS_TIMING set failed\n");
				return EINVAL;
			}
		}

		if (ISSET(sc->sc_caps, SMC_CAPS_8BIT_MODE)) {
			width = 8;
			value = EXT_CSD_BUS_WIDTH_8;
		} else if (ISSET(sc->sc_caps, SMC_CAPS_4BIT_MODE)) {
			width = 4;
			value = EXT_CSD_BUS_WIDTH_4;
		} else {
			width = 1;
			value = EXT_CSD_BUS_WIDTH_1;
		}

		if (width != 1) {
			error = sdmmc_mem_mmc_switch(sf, EXT_CSD_CMD_SET_NORMAL,
			    EXT_CSD_BUS_WIDTH, value);
			if (error == 0)
				error = sdmmc_chip_bus_width(sc->sc_sct,
				    sc->sc_sch, width);
			else {
				DPRINTF(("%s: can't change bus width"
				    " (%d bit)\n", SDMMCDEVNAME(sc), width));
				return error;
			}

			/* XXXX: need bus test? (using by CMD14 & CMD19) */
		}
		sf->width = width;
	} else {
		if (sc->sc_busclk > sf->csd.tran_speed)
			sc->sc_busclk = sf->csd.tran_speed;
		error =
		    sdmmc_chip_bus_clock(sc->sc_sct, sc->sc_sch, sc->sc_busclk);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "can't change bus clock\n");
			return error;
		}
		sf->width = 1;
	}

	return 0;
}

static int
sdmmc_mem_send_cid(struct sdmmc_softc *sc, sdmmc_response *resp)
{
	struct sdmmc_command cmd;
	int error;

	if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
		memset(&cmd, 0, sizeof cmd);
		cmd.c_opcode = MMC_ALL_SEND_CID;
		cmd.c_flags = SCF_CMD_BCR | SCF_RSP_R2;

		error = sdmmc_mmc_command(sc, &cmd);
	} else {
		error = sdmmc_mem_send_cxd_data(sc, MMC_SEND_CID, &cmd.c_resp,
		    sizeof(cmd.c_resp));
	}

#ifdef SDMMC_DEBUG
	sdmmc_dump_data("CID", cmd.c_resp, sizeof(cmd.c_resp));
#endif
	if (error == 0 && resp != NULL)
		memcpy(resp, &cmd.c_resp, sizeof(*resp));
	return error;
}

static int
sdmmc_mem_send_csd(struct sdmmc_softc *sc, struct sdmmc_function *sf,
    sdmmc_response *resp)
{
	struct sdmmc_command cmd;
	int error;

	if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
		memset(&cmd, 0, sizeof cmd);
		cmd.c_opcode = MMC_SEND_CSD;
		cmd.c_arg = MMC_ARG_RCA(sf->rca);
		cmd.c_flags = SCF_CMD_AC | SCF_RSP_R2;

		error = sdmmc_mmc_command(sc, &cmd);
	} else {
		error = sdmmc_mem_send_cxd_data(sc, MMC_SEND_CSD, &cmd.c_resp,
		    sizeof(cmd.c_resp));
	}

#ifdef SDMMC_DEBUG
	sdmmc_dump_data("CSD", cmd.c_resp, sizeof(cmd.c_resp));
#endif
	if (error == 0 && resp != NULL)
		memcpy(resp, &cmd.c_resp, sizeof(*resp));
	return error;
}

static int
sdmmc_mem_send_scr(struct sdmmc_softc *sc, struct sdmmc_function *sf,
    uint32_t scr[2])
{
	struct sdmmc_command cmd;
	bus_dma_segment_t ds[1];
	void *ptr = NULL;
	int datalen = 8;
	int rseg;
	int error = 0;

	/* Don't lock */

	if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
		error = bus_dmamem_alloc(sc->sc_dmat, datalen, PAGE_SIZE, 0,
		    ds, 1, &rseg, BUS_DMA_NOWAIT);
		if (error)
			goto out;
		error = bus_dmamem_map(sc->sc_dmat, ds, 1, datalen, &ptr,
		    BUS_DMA_NOWAIT);
		if (error)
			goto dmamem_free;
		error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, ptr, datalen,
		    NULL, BUS_DMA_NOWAIT|BUS_DMA_STREAMING|BUS_DMA_READ);
		if (error)
			goto dmamem_unmap;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
		    BUS_DMASYNC_PREREAD);
	} else {
		ptr = malloc(datalen, M_DEVBUF, M_NOWAIT | M_ZERO);
		if (ptr == NULL)
			goto out;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_data = ptr;
	cmd.c_datalen = datalen;
	cmd.c_blklen = datalen;
	cmd.c_arg = 0;
	cmd.c_flags = SCF_CMD_ADTC | SCF_CMD_READ | SCF_RSP_R1 | SCF_RSP_SPI_R1;
	cmd.c_opcode = SD_APP_SEND_SCR;
	if (ISSET(sc->sc_caps, SMC_CAPS_DMA))
		cmd.c_dmamap = sc->sc_dmap;

	error = sdmmc_app_command(sc, sf, &cmd);
	if (error == 0) {
		if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
			    BUS_DMASYNC_POSTREAD);
		}
		memcpy(scr, ptr, datalen);
	}

out:
	if (ptr != NULL) {
		if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
			bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);
dmamem_unmap:
			bus_dmamem_unmap(sc->sc_dmat, ptr, datalen);
dmamem_free:
			bus_dmamem_free(sc->sc_dmat, ds, rseg);
		} else {
			free(ptr, M_DEVBUF);
		}
	}
	DPRINTF(("%s: sdmem_mem_send_scr: error = %d\n", SDMMCDEVNAME(sc),
	    error));

#ifdef SDMMC_DEBUG
	if (error == 0)
		sdmmc_dump_data("SCR", scr, 8);
#endif
	return error;
}

static int
sdmmc_mem_decode_scr(struct sdmmc_softc *sc, struct sdmmc_function *sf)
{
	sdmmc_response resp;
	int ver;

	memset(resp, 0, sizeof(resp));
	/*
	 * Change the raw-scr received from the DMA stream to resp.
	 */
	resp[0] = be32toh(sf->raw_scr[1]);
	resp[1] = be32toh(sf->raw_scr[0]) >> 8;

	ver = SCR_STRUCTURE(resp);
	sf->scr.sd_spec = SCR_SD_SPEC(resp);
	sf->scr.bus_width = SCR_SD_BUS_WIDTHS(resp);

	DPRINTF(("%s: sdmmc_mem_decode_scr: spec=%d, bus width=%d\n",
	    SDMMCDEVNAME(sc), sf->scr.sd_spec, sf->scr.bus_width));

	if (ver != 0) {
		DPRINTF(("%s: unknown structure version: %d\n",
		    SDMMCDEVNAME(sc), ver));
		return EINVAL;
	}
	return 0;
}

static int
sdmmc_mem_send_cxd_data(struct sdmmc_softc *sc, int opcode, void *data,
    size_t datalen)
{
	struct sdmmc_command cmd;
	bus_dma_segment_t ds[1];
	void *ptr = NULL;
	int rseg;
	int error = 0;

	if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
		error = bus_dmamem_alloc(sc->sc_dmat, datalen, PAGE_SIZE, 0, ds,
		    1, &rseg, BUS_DMA_NOWAIT);
		if (error)
			goto out;
		error = bus_dmamem_map(sc->sc_dmat, ds, 1, datalen, &ptr,
		    BUS_DMA_NOWAIT);
		if (error)
			goto dmamem_free;
		error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, ptr, datalen,
		    NULL, BUS_DMA_NOWAIT|BUS_DMA_STREAMING|BUS_DMA_READ);
		if (error)
			goto dmamem_unmap;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
		    BUS_DMASYNC_PREREAD);
	} else {
		ptr = malloc(datalen, M_DEVBUF, M_NOWAIT | M_ZERO);
		if (ptr == NULL)
			goto out;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_data = ptr;
	cmd.c_datalen = datalen;
	cmd.c_blklen = datalen;
	cmd.c_opcode = opcode;
	cmd.c_arg = 0;
	cmd.c_flags = SCF_CMD_ADTC | SCF_CMD_READ | SCF_RSP_SPI_R1;
	if (opcode == MMC_SEND_EXT_CSD)
		SET(cmd.c_flags, SCF_RSP_R1);
	else
		SET(cmd.c_flags, SCF_RSP_R2);
	if (ISSET(sc->sc_caps, SMC_CAPS_DMA))
		cmd.c_dmamap = sc->sc_dmap;

	error = sdmmc_mmc_command(sc, &cmd);
	if (error == 0) {
		if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
			    BUS_DMASYNC_POSTREAD);
		}
		memcpy(data, ptr, datalen);
	}

out:
	if (ptr != NULL) {
		if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
			bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);
dmamem_unmap:
			bus_dmamem_unmap(sc->sc_dmat, ptr, datalen);
dmamem_free:
			bus_dmamem_free(sc->sc_dmat, ds, rseg);
		} else {
			free(ptr, M_DEVBUF);
		}
	}
	return error;
}

static int
sdmmc_set_bus_width(struct sdmmc_function *sf, int width)
{
	struct sdmmc_softc *sc = sf->sc;
	struct sdmmc_command cmd;
	int error;

	if (ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE))
		return ENODEV;

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = SD_APP_SET_BUS_WIDTH;
	cmd.c_flags = SCF_RSP_R1 | SCF_CMD_AC;

	switch (width) {
	case 1:
		cmd.c_arg = SD_ARG_BUS_WIDTH_1;
		break;

	case 4:
		cmd.c_arg = SD_ARG_BUS_WIDTH_4;
		break;

	default:
		return EINVAL;
	}

	error = sdmmc_app_command(sc, sf, &cmd);
	if (error == 0)
		error = sdmmc_chip_bus_width(sc->sc_sct, sc->sc_sch, width);
	return error;
}

static int
sdmmc_mem_sd_switch(struct sdmmc_function *sf, int mode, int group,
    int function, void *status)
{
	struct sdmmc_softc *sc = sf->sc;
	struct sdmmc_command cmd;
	bus_dma_segment_t ds[1];
	void *ptr = NULL;
	int gsft, rseg, error = 0;
	const int statlen = 64;

	if (sf->scr.sd_spec >= SCR_SD_SPEC_VER_1_10 &&
	    !ISSET(sf->csd.ccc, SD_CSD_CCC_SWITCH))
		return EINVAL;

	if (group <= 0 || group > 6 ||
	    function < 0 || function > 16)
		return EINVAL;

	gsft = (group - 1) << 2;

	if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
		error = bus_dmamem_alloc(sc->sc_dmat, statlen, PAGE_SIZE, 0, ds,
		    1, &rseg, BUS_DMA_NOWAIT);
		if (error)
			goto out;
		error = bus_dmamem_map(sc->sc_dmat, ds, 1, statlen, &ptr,
		    BUS_DMA_NOWAIT);
		if (error)
			goto dmamem_free;
		error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, ptr, statlen,
		    NULL, BUS_DMA_NOWAIT|BUS_DMA_STREAMING|BUS_DMA_READ);
		if (error)
			goto dmamem_unmap;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, statlen,
		    BUS_DMASYNC_PREREAD);
	} else {
		ptr = malloc(statlen, M_DEVBUF, M_NOWAIT | M_ZERO);
		if (ptr == NULL)
			goto out;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_data = ptr;
	cmd.c_datalen = statlen;
	cmd.c_blklen = statlen;
	cmd.c_opcode = SD_SEND_SWITCH_FUNC;
	cmd.c_arg =
	    (!!mode << 31) | (function << gsft) | (0x00ffffff & ~(0xf << gsft));
	cmd.c_flags = SCF_CMD_ADTC | SCF_CMD_READ | SCF_RSP_R1 | SCF_RSP_SPI_R1;
	if (ISSET(sc->sc_caps, SMC_CAPS_DMA))
		cmd.c_dmamap = sc->sc_dmap;

	error = sdmmc_mmc_command(sc, &cmd);
	if (error == 0) {
		if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, statlen,
			    BUS_DMASYNC_POSTREAD);
		}
		memcpy(status, ptr, statlen);
	}

out:
	if (ptr != NULL) {
		if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
			bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);
dmamem_unmap:
			bus_dmamem_unmap(sc->sc_dmat, ptr, statlen);
dmamem_free:
			bus_dmamem_free(sc->sc_dmat, ds, rseg);
		} else {
			free(ptr, M_DEVBUF);
		}
	}
	return error;
}

static int
sdmmc_mem_mmc_switch(struct sdmmc_function *sf, uint8_t set, uint8_t index,
    uint8_t value)
{
	struct sdmmc_softc *sc = sf->sc;
	struct sdmmc_command cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_SWITCH;
	cmd.c_arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
	    (index << 16) | (value << 8) | set;
	cmd.c_flags = SCF_RSP_SPI_R1B | SCF_RSP_R1B | SCF_CMD_AC;

	return sdmmc_mmc_command(sc, &cmd);
}

/*
 * SPI mode function
 */
static int
sdmmc_mem_spi_read_ocr(struct sdmmc_softc *sc, uint32_t hcs, uint32_t *card_ocr)
{
	struct sdmmc_command cmd;
	int error;

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_READ_OCR;
	cmd.c_arg = hcs ? MMC_OCR_HCS : 0;
	cmd.c_flags = SCF_RSP_SPI_R3;

	error = sdmmc_mmc_command(sc, &cmd);
	if (error == 0 && card_ocr != NULL)
		*card_ocr = cmd.c_resp[1];
	DPRINTF(("%s: sdmmc_mem_spi_read_ocr: error=%d, ocr=%#x\n",
	    SDMMCDEVNAME(sc), error, cmd.c_resp[1]));
	return error;
}

/*
 * read/write function
 */
/* read */
static int
sdmmc_mem_single_read_block(struct sdmmc_function *sf, uint32_t blkno,
    u_char *data, size_t datalen)
{
	struct sdmmc_softc *sc __unused = sf->sc;
	int error = 0;
	int i;

	KASSERT((datalen % SDMMC_SECTOR_SIZE) == 0);
	KASSERT(!ISSET(sc->sc_caps, SMC_CAPS_DMA));

	for (i = 0; i < datalen / SDMMC_SECTOR_SIZE; i++) {
		error = sdmmc_mem_read_block_subr(sf, blkno + i,
		    data + i * SDMMC_SECTOR_SIZE, SDMMC_SECTOR_SIZE);
		if (error)
			break;
	}
	return error;
}

static int
sdmmc_mem_read_block_subr(struct sdmmc_function *sf, uint32_t blkno,
    u_char *data, size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	struct sdmmc_command cmd;
	int error, bbuf, seg, off, len, num;

	if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
		error = sdmmc_select_card(sc, sf);
		if (error)
			goto out;
	}

	bbuf = 0;
	num = 0;
	seg = off = len = 0;
retry:
	memset(&cmd, 0, sizeof(cmd));
	cmd.c_data = data;
	cmd.c_datalen = datalen;
	cmd.c_blklen = SDMMC_SECTOR_SIZE;
	cmd.c_opcode = (cmd.c_datalen / cmd.c_blklen) > 1 ?
	    MMC_READ_BLOCK_MULTIPLE : MMC_READ_BLOCK_SINGLE;
	cmd.c_arg = blkno;
	if (!ISSET(sf->flags, SFF_SDHC))
		cmd.c_arg <<= SDMMC_SECTOR_SIZE_SB;
	cmd.c_flags = SCF_CMD_ADTC | SCF_CMD_READ | SCF_RSP_R1 | SCF_RSP_SPI_R1;
	if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
		cmd.c_dmamap = sc->sc_dmap;
		if (!ISSET(sc->sc_caps, SMC_CAPS_MULTI_SEG_DMA)) {
			len = sc->sc_dmap->dm_segs[seg].ds_len - off;
			len &= ~(SDMMC_SECTOR_SIZE - 1);
			cmd.c_datalen = len;
			cmd.c_dmaseg = seg;
			cmd.c_dmaoff = off;
			bbuf = 0;
			if (len == 0) {
				/* Use bounce buffer */
				bus_dmamap_sync(sc->sc_dmat, sf->bbuf_dmap,
				    0, SDMMC_SECTOR_SIZE, BUS_DMASYNC_PREREAD);
				cmd.c_datalen = SDMMC_SECTOR_SIZE;
				cmd.c_dmamap = sf->bbuf_dmap;
				cmd.c_dmaseg = 0;
				cmd.c_dmaoff = 0;
				bbuf = 1;
				len = SDMMC_SECTOR_SIZE;
			}
			cmd.c_opcode = (cmd.c_datalen / cmd.c_blklen) > 1 ?
			    MMC_READ_BLOCK_MULTIPLE : MMC_READ_BLOCK_SINGLE;
		}
	}

	error = sdmmc_mmc_command(sc, &cmd);
	if (error)
		goto out;

	if (!ISSET(sc->sc_caps, SMC_CAPS_AUTO_STOP)) {
		if (cmd.c_opcode == MMC_READ_BLOCK_MULTIPLE) {
			memset(&cmd, 0, sizeof cmd);
			cmd.c_opcode = MMC_STOP_TRANSMISSION;
			cmd.c_arg = MMC_ARG_RCA(sf->rca);
			cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1B | SCF_RSP_SPI_R1B;
			error = sdmmc_mmc_command(sc, &cmd);
			if (error)
				goto out;
		}
	}

	if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
		do {
			memset(&cmd, 0, sizeof(cmd));
			cmd.c_opcode = MMC_SEND_STATUS;
			if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE))
				cmd.c_arg = MMC_ARG_RCA(sf->rca);
			cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1 | SCF_RSP_SPI_R2;
			error = sdmmc_mmc_command(sc, &cmd);
			if (error)
				break;
			/* XXX time out */
		} while (!ISSET(MMC_R1(cmd.c_resp), MMC_R1_READY_FOR_DATA));
	}

	if (ISSET(sc->sc_caps, SMC_CAPS_DMA) &&
	    !ISSET(sc->sc_caps, SMC_CAPS_MULTI_SEG_DMA)) {
		bus_dma_segment_t *dm_segs = sc->sc_dmap->dm_segs;

		if (bbuf) {
			bus_dmamap_sync(sc->sc_dmat, sf->bbuf_dmap,
			    0, SDMMC_SECTOR_SIZE, BUS_DMASYNC_POSTREAD);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, num,
			    SDMMC_SECTOR_SIZE, BUS_DMASYNC_POSTREAD);
			memcpy(data, sf->bbuf, SDMMC_SECTOR_SIZE);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, num,
			    SDMMC_SECTOR_SIZE, BUS_DMASYNC_PREREAD);
		}
		num += len;
		data += len;
		datalen -= len;
		blkno += (len / SDMMC_SECTOR_SIZE);

		while (off + len >= dm_segs[seg].ds_len) {
			len -= dm_segs[seg++].ds_len;
			off = 0;
		}
		off += len;

		if (seg < sc->sc_dmap->dm_nsegs)
			goto retry;
	}

out:
	return error;
}

int
sdmmc_mem_read_block(struct sdmmc_function *sf, uint32_t blkno, u_char *data,
    size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	int error;

	SDMMC_LOCK(sc);

	if (ISSET(sc->sc_caps, SMC_CAPS_SINGLE_ONLY)) {
		error = sdmmc_mem_single_read_block(sf, blkno, data, datalen);
		goto out;
	}

	if (!ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
		error = sdmmc_mem_read_block_subr(sf, blkno, data, datalen);
		goto out;
	}

	/* DMA transfer */
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, data, datalen, NULL,
	    BUS_DMA_NOWAIT|BUS_DMA_READ);
	if (error)
		goto out;

#ifdef SDMMC_DEBUG
	for (int i = 0; i < sc->sc_dmap->dm_nsegs; i++) {
		printf("seg#%d: addr=%#lx, size=%#lx\n", i,
		    (u_long)sc->sc_dmap->dm_segs[i].ds_addr,
		    (u_long)sc->sc_dmap->dm_segs[i].ds_len);
	}
#endif

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
	    BUS_DMASYNC_PREREAD);

	error = sdmmc_mem_read_block_subr(sf, blkno, data, datalen);
	if (error)
		goto unload;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
	    BUS_DMASYNC_POSTREAD);
unload:
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);

out:
	SDMMC_UNLOCK(sc);

	return error;
}

/* write */
static int
sdmmc_mem_single_write_block(struct sdmmc_function *sf, uint32_t blkno,
    u_char *data, size_t datalen)
{
	struct sdmmc_softc *sc __unused = sf->sc;
	int error = 0;
	int i;

	KASSERT((datalen % SDMMC_SECTOR_SIZE) == 0);
	KASSERT(!ISSET(sc->sc_caps, SMC_CAPS_DMA));

	for (i = 0; i < datalen / SDMMC_SECTOR_SIZE; i++) {
		error = sdmmc_mem_write_block_subr(sf, blkno + i,
		    data + i * SDMMC_SECTOR_SIZE, SDMMC_SECTOR_SIZE);
		if (error)
			break;
	}
	return error;
}

static int
sdmmc_mem_write_block_subr(struct sdmmc_function *sf, uint32_t blkno,
    u_char *data, size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	struct sdmmc_command cmd;
	int error, bbuf, seg, off, len, num;

	if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
		error = sdmmc_select_card(sc, sf);
		if (error)
			goto out;
	}

	bbuf = 0;
	num = 0;
	seg = off = len = 0;
retry:
	memset(&cmd, 0, sizeof(cmd));
	cmd.c_data = data;
	cmd.c_datalen = datalen;
	cmd.c_blklen = SDMMC_SECTOR_SIZE;
	cmd.c_opcode = (cmd.c_datalen / cmd.c_blklen) > 1 ?
	    MMC_WRITE_BLOCK_MULTIPLE : MMC_WRITE_BLOCK_SINGLE;
	cmd.c_arg = blkno;
	if (!ISSET(sf->flags, SFF_SDHC))
		cmd.c_arg <<= SDMMC_SECTOR_SIZE_SB;
	cmd.c_flags = SCF_CMD_ADTC | SCF_RSP_R1;
	if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
		cmd.c_dmamap = sc->sc_dmap;
		if (!ISSET(sc->sc_caps, SMC_CAPS_MULTI_SEG_DMA)) {
			len = sc->sc_dmap->dm_segs[seg].ds_len - off;
			len &= ~(SDMMC_SECTOR_SIZE - 1);
			cmd.c_datalen = len;
			cmd.c_dmaseg = seg;
			cmd.c_dmaoff = off;
			bbuf = 0;
			if (len == 0) {
				/* Use bounce buffer */
				bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, num,
				    SDMMC_SECTOR_SIZE, BUS_DMASYNC_POSTWRITE);
				memcpy(sf->bbuf, data, SDMMC_SECTOR_SIZE);
				bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, num,
				    SDMMC_SECTOR_SIZE, BUS_DMASYNC_PREWRITE);
				bus_dmamap_sync(sc->sc_dmat, sf->bbuf_dmap, 0,
				    SDMMC_SECTOR_SIZE, BUS_DMASYNC_PREWRITE);
				cmd.c_datalen = SDMMC_SECTOR_SIZE;
				cmd.c_dmamap = sf->bbuf_dmap;
				cmd.c_dmaseg = 0;
				cmd.c_dmaoff = 0;
				bbuf = 1;
				len = SDMMC_SECTOR_SIZE;
			}
			cmd.c_opcode = (cmd.c_datalen / cmd.c_blklen) > 1 ?
			    MMC_WRITE_BLOCK_MULTIPLE : MMC_WRITE_BLOCK_SINGLE;
		}
	}

	error = sdmmc_mmc_command(sc, &cmd);
	if (error)
		goto out;

	if (!ISSET(sc->sc_caps, SMC_CAPS_AUTO_STOP)) {
		if (cmd.c_opcode == MMC_WRITE_BLOCK_MULTIPLE) {
			memset(&cmd, 0, sizeof(cmd));
			cmd.c_opcode = MMC_STOP_TRANSMISSION;
			cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1B | SCF_RSP_SPI_R1B;
			error = sdmmc_mmc_command(sc, &cmd);
			if (error)
				goto out;
		}
	}

	if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {
		do {
			memset(&cmd, 0, sizeof(cmd));
			cmd.c_opcode = MMC_SEND_STATUS;
			if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE))
				cmd.c_arg = MMC_ARG_RCA(sf->rca);
			cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1 | SCF_RSP_SPI_R2;
			error = sdmmc_mmc_command(sc, &cmd);
			if (error)
				break;
			/* XXX time out */
		} while (!ISSET(MMC_R1(cmd.c_resp), MMC_R1_READY_FOR_DATA));
	}

	if (ISSET(sc->sc_caps, SMC_CAPS_DMA) &&
	    !ISSET(sc->sc_caps, SMC_CAPS_MULTI_SEG_DMA)) {
		bus_dma_segment_t *dm_segs = sc->sc_dmap->dm_segs;

		if (bbuf)
			bus_dmamap_sync(sc->sc_dmat, sf->bbuf_dmap,
			    0, SDMMC_SECTOR_SIZE, BUS_DMASYNC_POSTWRITE);
		num += len;
		data += len;
		datalen -= len;
		blkno += (len / SDMMC_SECTOR_SIZE);

		while (off + len >= dm_segs[seg].ds_len) {
			len -= dm_segs[seg++].ds_len;
			off = 0;
		}
		off += len;

		if (seg < sc->sc_dmap->dm_nsegs)
			goto retry;
	}

out:
	return error;
}

int
sdmmc_mem_write_block(struct sdmmc_function *sf, uint32_t blkno, u_char *data,
    size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	int error;

	SDMMC_LOCK(sc);

	if (sdmmc_chip_write_protect(sc->sc_sct, sc->sc_sch)) {
		aprint_normal_dev(sc->sc_dev, "write-protected\n");
		error = EIO;
		goto out;
	}

	if (ISSET(sc->sc_caps, SMC_CAPS_SINGLE_ONLY)) {
		error = sdmmc_mem_single_write_block(sf, blkno, data, datalen);
		goto out;
	}

	if (!ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
		error = sdmmc_mem_write_block_subr(sf, blkno, data, datalen);
		goto out;
	}

	/* DMA transfer */
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, data, datalen, NULL,
	    BUS_DMA_NOWAIT|BUS_DMA_WRITE);
	if (error)
		goto out;

#ifdef SDMMC_DEBUG
	for (int i = 0; i < sc->sc_dmap->dm_nsegs; i++) {
		printf("seg#%d: addr=%#lx, size=%#lx\n", i,
		    (u_long)sc->sc_dmap->dm_segs[i].ds_addr,
		    (u_long)sc->sc_dmap->dm_segs[i].ds_len);
	}
#endif

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
	    BUS_DMASYNC_PREWRITE);

	error = sdmmc_mem_write_block_subr(sf, blkno, data, datalen);
	if (error)
		goto unload;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
	    BUS_DMASYNC_POSTWRITE);
unload:
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);

out:
	SDMMC_UNLOCK(sc);

	return error;
}
