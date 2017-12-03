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

/*
 * All SD/MMC code is taken from various files in sys/dev/sdmmc
 */
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

#include <machine/limits.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/disklabel.h>

#include <netinet/in.h>

#include <lib/libsa/stand.h>

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/iodesc.h>

#include <dev/sdmmc/sdmmcreg.h>
#include "dev_sdmmc.h"
#include "s3csdi.h"

#define SET(t, f)       ((t) |= (f))
#define ISSET(t, f)     ((t) & (f))
#define CLR(t, f)       ((t) &= ~(f))

//#define SDMMC_DEBUG
#ifdef SDMMC_DEBUG
#define DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

/* SD/MMC device driver structure */
struct sdifdv {
	char*			name;
	int			(*match)(unsigned);
	void*			(*init)(unsigned, uint32_t*);
	int			(*host_ocr)(void*);
	int			(*bus_clock)(void*, int);
	int			(*bus_power)(void*, int);
	int			(*bus_width)(void*, int);
	void			(*exec_cmd)(void*, struct sdmmc_command*);
	int			(*get_max_bus_clock)(void*);
	void*			priv;
};

struct sdmmc_softc;

/* Structure used for of->f_devdata */
struct sdmmc_part {
	struct sdmmc_softc	*sc;
	struct partition	*part;
};

/* SD/MMC driver structure */
struct sdmmc_softc {
	uint32_t		flags;
	uint32_t		caps;
	uint16_t		rca;		/* relative card address */
	sdmmc_response		raw_cid;	/* temp. storage for decoding */
	uint32_t		raw_scr[2];
	struct sdmmc_csd	csd;		/* decoded CSD value */
	struct sdmmc_cid	cid;		/* decoded CID value */
	struct sdmmc_scr	scr;
	int			busclk;
	struct sdifdv		*sdifdv;
	struct disklabel	sc_label;
	int			npartitions;
	struct sdmmc_part	partitions[MAXPARTITIONS];
};

static struct sdifdv vnifdv[] = {
	{"S3C SD/MMC", s3csd_match, s3csd_init, s3csd_host_ocr,
	 s3csd_bus_clock, s3csd_bus_power, s3csd_bus_width, s3csd_exec_cmd,
	 s3csd_get_max_bus_clock}
};
static int nnifdv = sizeof(vnifdv)/sizeof(vnifdv[0]);

static struct sdmmc_softc sdmmc_softc;
static uint8_t sdmmc_initialized = FALSE;

extern time_t getsecs();
extern time_t getusecs();
extern void usleep(int);

/* Local functions */
static int sdmmc_getdisklabel(struct sdmmc_softc *sc);
static int sdmmc_init(unsigned int tag);
static int sdmmc_enable(struct sdmmc_softc*);

static int sdmmc_mem_send_if_cond(struct sdmmc_softc*, uint32_t, uint32_t*);
static int sdmmc_mmc_command(struct sdmmc_softc*, struct sdmmc_command*);
static void sdmmc_go_idle_state(struct sdmmc_softc*);
static int sdmmc_mem_send_op_cond(struct sdmmc_softc*, uint32_t, uint32_t *);
static int sdmmc_set_bus_power(struct sdmmc_softc*, uint32_t, uint32_t);
static int sdmmc_app_command(struct sdmmc_softc*, uint16_t,
			     struct sdmmc_command*);
static int sdmmc_mmc_command(struct sdmmc_softc*, struct sdmmc_command*);
static int sdmmc_scan(struct sdmmc_softc*);
static void sdmmc_mem_scan(struct sdmmc_softc*);
static int sdmmc_set_relative_addr(struct sdmmc_softc*);
static int sdmmc_mem_send_cid(struct sdmmc_softc*, sdmmc_response*);

static int sdmmc_mem_send_csd(struct sdmmc_softc*, sdmmc_response*);
static int sdmmc_decode_csd(struct sdmmc_softc*, sdmmc_response);
static int sdmmc_decode_cid(struct sdmmc_softc*, sdmmc_response);

static int sdmmc_mem_read_block(struct sdmmc_softc*, uint32_t, u_char*, size_t);
static int sdmmc_select_card(struct sdmmc_softc*);
static int sdmmc_mem_set_blocklen(struct sdmmc_softc*);

static int sdmmc_mem_send_scr(struct sdmmc_softc*, uint32_t[2]);
static int sdmmc_mem_decode_scr(struct sdmmc_softc*);
static int sdmmc_set_bus_width(struct sdmmc_softc*, int);
static int sdmmc_mem_sd_switch(struct sdmmc_softc *, int, int, int, void*);

#ifdef SDMMC_DEBUG
static void sdmmc_dump_data(const char*, void*, size_t);
static void sdmmc_print_cid(struct sdmmc_cid*);
static void sdmmc_dump_command(struct sdmmc_softc*, struct sdmmc_command*);
#endif

int
sdmmc_open(struct open_file *of, ...)
{
	va_list ap;
	int unit __unused, part;

	va_start(ap, of);
	unit = va_arg(ap, u_int); /* Not used for now */
	part = va_arg(ap, u_int);
	va_end(ap);

	/* Simply try to initialize SD mem sub system. */
	if( !sdmmc_init(0) ) {
		return 1;
	}

	of->f_devdata = (void*)&sdmmc_softc.partitions[part];

	return 0;
}

int
sdmmc_close(struct open_file *f)
{
	return (0);
}

int
sdmmc_get_fstype(void *p)  {
	struct sdmmc_part *part = (struct sdmmc_part*)p;

	return part->part->p_fstype;
}


int
sdmmc_strategy(void *d, int f, daddr_t b, size_t s, void *buf, size_t *r)
{
	struct sdmmc_part *part = (struct sdmmc_part*)d;
	unsigned int offset;
	switch(f) {
	case F_READ:
		offset = part->part->p_offset + b;
		*r = s;
		if(sdmmc_mem_read_block(part->sc, offset, buf, s) == 0)
			return 0;
		else
			return EIO;
	default:
		printf("Unsupported operation\n");
		break;
	}
	return (EIO);
}

int
sdmmc_getdisklabel(struct sdmmc_softc *sc)
{
	char *msg;
	int sector, i, n;
	size_t rsize;
	struct mbr_partition *dp, *bsdp;
	struct disklabel *lp;
	/*uint8_t *buf = wd->sc_buf;*/
	uint8_t buf[DEV_BSIZE];

	lp = &sc->sc_label;
	memset(lp, 0, sizeof(struct disklabel));

	sector = 0;
	if (sdmmc_strategy(&sc->partitions[0], F_READ, MBR_BBSECTOR, DEV_BSIZE,
			   buf, &rsize))
		return EOFFSET;

	dp = (struct mbr_partition *)(buf + MBR_PART_OFFSET);
	bsdp = NULL;
	for (i = 0; i < MBR_PART_COUNT; i++, dp++) {
		if (dp->mbrp_type == MBR_PTYPE_NETBSD) {
			bsdp = dp;
			break;
		}
	}
	if (!bsdp) {
		/* generate fake disklabel */
		lp->d_secsize = DEV_BSIZE;
		/*lp->d_ntracks = wd->sc_params.atap_heads;
		lp->d_nsectors = wd->sc_params.atap_sectors;
		lp->d_ncylinders = wd->sc_params.atap_cylinders;*/
		lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
		lp->d_type = DKTYPE_FLASH;
		/*strncpy(lp->d_typename, (char *)wd->sc_params.atap_model, 16);*/
		strncpy(lp->d_packname, "fictitious", 16);
		/*if (wd->sc_capacity > UINT32_MAX)
			lp->d_secperunit = UINT32_MAX;
		else
		lp->d_secperunit = wd->sc_capacity;*/
		lp->d_rpm = 3600;
		lp->d_interleave = 1;
		lp->d_flags = 0;
		lp->d_partitions[RAW_PART].p_offset = 0;
		lp->d_partitions[RAW_PART].p_size =
			lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
		lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
		lp->d_magic = DISKMAGIC;
		lp->d_magic2 = DISKMAGIC;
		lp->d_checksum = dkcksum(lp);

		dp = (struct mbr_partition *)(buf + MBR_PART_OFFSET);
		n = 'e' - 'a';
		for (i = 0; i < MBR_PART_COUNT; i++, dp++) {
			if (dp->mbrp_type == MBR_PTYPE_UNUSED)
				continue;
			lp->d_partitions[n].p_offset = bswap32(dp->mbrp_start);
			lp->d_partitions[n].p_size = bswap32(dp->mbrp_size);
			switch (dp->mbrp_type) {
			case MBR_PTYPE_FAT12:
			case MBR_PTYPE_FAT16S:
			case MBR_PTYPE_FAT16B:
			case MBR_PTYPE_FAT32:
			case MBR_PTYPE_FAT32L:
			case MBR_PTYPE_FAT16L:
				lp->d_partitions[n].p_fstype = FS_MSDOS;
				break;
			case MBR_PTYPE_LNXEXT2:
				lp->d_partitions[n].p_fstype = FS_EX2FS;
				break;
			default:
				lp->d_partitions[n].p_fstype = FS_OTHER;
				break;
			}
			n += 1;
		}
		lp->d_npartitions = n;
	}
	else {
		sector = bsdp->mbrp_start;
		if (sdmmc_strategy(&sc->partitions[0], F_READ,
				   sector + LABELSECTOR, DEV_BSIZE,
				   buf, &rsize))
			return EOFFSET;
		msg = getdisklabel((char *)buf + LABELOFFSET, &sc->sc_label);
		if (msg != NULL)
			printf("getdisklabel: %s\n", msg);
	}
	/*DPRINTF(("label info: d_secsize %d, d_nsectors %d, d_ncylinders %d,"
		 "d_ntracks %d, d_secpercyl %d\n",
		 wd->sc_label.d_secsize,
		 wd->sc_label.d_nsectors,
		 wd->sc_label.d_ncylinders,
		 wd->sc_label.d_ntracks,
		 wd->sc_label.d_secpercyl));*/

	return 0;
}

void
sdmmc_delay(int us) {
	usleep(us);
}

/* Initialize the SD/MMC subsystem. Return 1 on success, and 0 on error.
   In case of error, errno will be set to a sane value.
 */
int
sdmmc_init(unsigned int tag)
{
	struct sdifdv *dv;
	int n;
	int error;
	struct sdmmc_softc *sc = &sdmmc_softc;
	char status[64];

	if (sdmmc_initialized) {
		printf("SD/MMC already initialized\n");
		return 1;
	}

	for (n = 0; n < nnifdv; n++) {
		dv = &vnifdv[n];
		if ((*dv->match)(tag) > 0)
			goto found;
	}
	errno = ENODEV;
	return 0;
 found:
	sc->caps = 0;
	/* Init should return NULL if no card is present. */
	sc->sdifdv->priv = (*dv->init)(tag, &sc->caps);
	if (sc->sdifdv->priv == NULL) {
		/* We expect that the device initialization sets
		   errno properly */
		return 0;
	}

	sc->flags = 0;
	sc->sdifdv = dv;

	/* Perfom SD-card initialization. */
	if( sdmmc_enable(sc) ) {
		printf("Failed to enable SD interface\n");
		errno = EIO;
		return 0;
	}
	sc->busclk = sc->sdifdv->get_max_bus_clock(sc->sdifdv->priv);

	if (sdmmc_scan(sc)) {
		printf("No functions\n");
		errno = EIO;
		return 0;
	}

	if (sdmmc_select_card(sc)) {
		printf("Failed to select card\n");
		errno = EIO;
		return 0;
	}

	if (!ISSET(sc->flags, SMF_CARD_SDHC)) {
		sdmmc_mem_set_blocklen(sc);
	}

	/* change bus width if supported */
	if (ISSET(sc->flags, SMF_SD_MODE) ) {
		error = sdmmc_mem_send_scr(sc, sc->raw_scr);
		if (error) {
			DPRINTF(("SD_SEND_SCR send failed.\n"));
			errno = EIO;
			return 0;
		}
		error = sdmmc_mem_decode_scr(sc);
		if (error) {
			errno = EIO;
			return 0;
		}

		if (ISSET(sc->caps, SMC_CAPS_4BIT_MODE) &&
		    ISSET(sc->scr.bus_width, SCR_SD_BUS_WIDTHS_4BIT)) {
			error = sdmmc_set_bus_width(sc, 4);
			if (error) {
				DPRINTF(("can't change bus width"
				    " (%d bit)\n", 4));
				errno = EIO;
				return 0;
			}
		}

#if 1
		if (sc->scr.sd_spec >= SCR_SD_SPEC_VER_1_10 &&
		    ISSET(sc->csd.ccc, SD_CSD_CCC_SWITCH)) {
			DPRINTF(("switch func mode 0\n"));
			error = sdmmc_mem_sd_switch(sc, 0, 1, 0, status);
			if (error) {
				printf("switch func mode 0 failed\n");
				errno = error;
				return 0;
			}
		}
#endif
		sc->sdifdv->bus_clock(sc->sdifdv->priv, sc->busclk);
	}

	/* Prepare dummy partition[0] entry used by sdmmc_getdisklabel() */
	sc->partitions[0].sc = sc;
	sc->partitions[0].part->p_offset = 0;

	if(sdmmc_getdisklabel(sc)) {
		errno = EOFFSET;
		return 0;
	}

	sc->npartitions = sc->sc_label.d_npartitions;
	for(n=0; n<sc->sc_label.d_npartitions; n++) {
		sc->partitions[n].part = &sc->sc_label.d_partitions[n];
		sc->partitions[n].sc = sc;
	}

	sdmmc_initialized = TRUE;

	return 1;
}

int
sdmmc_enable(struct sdmmc_softc *sc)
{
	uint32_t card_ocr;
	uint32_t ocr = 0;
	uint32_t host_ocr;
	int error;

	/* 1. Set the maximum power supported by bus */
	/* For now, we expect the init function to set the maximum
	   voltage. And if that is not supported by the SD-card we
	   just cannot work with it.
	 */

	sc->busclk = 400;
	/* 2. Clock bus at minimum frequency */
	sc->sdifdv->bus_clock(sc->sdifdv->priv, 400);

	/* We expect that the above call has performed any waiting needed.*/

	/* Initialize SD/MMC memory card(s), which is the only thing
	   we support.
	 */

	/* Set host mode to SD "combo" card or SD memory-only. */
	SET(sc->flags, SMF_SD_MODE|SMF_MEM_MODE);

	sdmmc_go_idle_state(sc);

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
	  ISSET(sc->caps, SMC_CAPS_SPI_MODE) ? ocr : 0, &card_ocr);
	if (error) {
		if (ISSET(sc->flags, SMF_SD_MODE) &&
		    !ISSET(sc->flags, SMF_IO_MODE)) {
			/* Not a SD card, switch to MMC mode. */
			DPRINTF(("Switch to MMC mode\n"));
			CLR(sc->flags, SMF_SD_MODE);
			goto mmc_mode;
		}
		if (!ISSET(sc->flags, SMF_SD_MODE)) {
			DPRINTF(("couldn't read memory OCR\n"));
			goto out;
		} else {
			/* Not a "combo" card. */
			CLR(sc->flags, SMF_MEM_MODE);
			error = 0;
			goto out;
		}
	}
#if 0 /* SPI NOT SUPPORTED */
	if (ISSET(ssc->caps, SMC_CAPS_SPI_MODE)) {
		/* get card OCR */
		error = sdmmc_mem_spi_read_ocr(sc, ocr, &card_ocr);
		if (error) {
			DPRINTF(("%s: couldn't read SPI memory OCR\n",
			    SDMMCDEVNAME(sc)));
			goto out;
		}
	}
#endif

	/* Set the lowest voltage supported by the card and host. */
	host_ocr = sc->sdifdv->host_ocr(sc->sdifdv->priv);
	error = sdmmc_set_bus_power(sc, host_ocr, card_ocr);
	if (error) {
		DPRINTF(("Couldn't supply voltage requested by card\n"));
		goto out;
	}
	host_ocr &= card_ocr;
	host_ocr |= ocr;

	/* Send the new OCR value until all cards are ready. */
	error = sdmmc_mem_send_op_cond(sc, host_ocr, NULL);
	if (error) {
		DPRINTF(("Couldn't send memory OCR\n"));
		goto out;
	}

out:
	return error;
}

int
sdmmc_mem_send_if_cond(struct sdmmc_softc *sc, uint32_t ocr, uint32_t *ocrp)
{
	struct sdmmc_command cmd;
	int error;

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_arg = ocr;
	cmd.c_flags = SCF_CMD_BCR | SCF_RSP_R7 | SCF_RSP_SPI_R7;
	cmd.c_opcode = SD_SEND_IF_COND;

	error = sdmmc_mmc_command(sc, &cmd);
	if (error == 0 && ocrp != NULL) {
		*ocrp = MMC_R7(cmd.c_resp);
	}

	return error;
}

void
sdmmc_go_idle_state(struct sdmmc_softc *sc)
{
	struct sdmmc_command cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_GO_IDLE_STATE;
	cmd.c_flags = SCF_CMD_BC | SCF_RSP_R0 | SCF_RSP_SPI_R1;

	(void)sdmmc_mmc_command(sc, &cmd);
}
int
sdmmc_mem_send_op_cond(struct sdmmc_softc *sc, uint32_t ocr, uint32_t *ocrp)
{
	struct sdmmc_command cmd;
	int error;
	int retry;


	/*
	 * If we change the OCR value, retry the command until the OCR
	 * we receive in response has the "CARD BUSY" bit set, meaning
	 * that all cards are ready for identification.
	 */
	for (retry = 0; retry < 100; retry++) {
		memset(&cmd, 0, sizeof(cmd));
		cmd.c_arg = !ISSET(sc->caps, SMC_CAPS_SPI_MODE) ?
		    ocr : (ocr & MMC_OCR_HCS);
		cmd.c_flags = SCF_CMD_BCR | SCF_RSP_R3 | SCF_RSP_SPI_R1;

		if (ISSET(sc->flags, SMF_SD_MODE)) {
			cmd.c_opcode = SD_APP_OP_COND;
			error = sdmmc_app_command(sc, 0, &cmd);
		} else {
			cmd.c_opcode = MMC_SEND_OP_COND;
			error = sdmmc_mmc_command(sc, &cmd);
		}
		if (error)
			break;

		if (ISSET(sc->caps, SMC_CAPS_SPI_MODE)) {
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
	    !ISSET(sc->caps, SMC_CAPS_SPI_MODE))
		*ocrp = MMC_R3(cmd.c_resp);
	DPRINTF(("sdmmc_mem_send_op_cond: error=%d, ocr=%x\n",
		 error, MMC_R3(cmd.c_resp)));
	return error;
}

/*
 * Set the lowest bus voltage supported by the card and the host.
 */
int
sdmmc_set_bus_power(struct sdmmc_softc *sc, uint32_t host_ocr,
		    uint32_t card_ocr)
{
	uint32_t bit;

	/* Mask off unsupported voltage levels and select the lowest. */
	DPRINTF(("host_ocr=%x ", host_ocr));
	host_ocr &= card_ocr;
	for (bit = 4; bit < 23; bit++) {
		if (ISSET(host_ocr, (1 << bit))) {
			host_ocr &= (3 << bit);
			break;
		}
	}
	DPRINTF(("card_ocr=%x new_ocr=%x\n", card_ocr, host_ocr));

	if (host_ocr == 0 ||
	    sc->sdifdv->bus_power(sc->sdifdv->priv, host_ocr) != 0)
		return 1;
	return 0;
}

int
sdmmc_app_command(struct sdmmc_softc *sc, uint16_t rca,
		  struct sdmmc_command *cmd)
{
	struct sdmmc_command acmd;
	int error;

	memset(&acmd, 0, sizeof(acmd));
	acmd.c_opcode = MMC_APP_CMD;
	if (rca != 0) {
		acmd.c_arg = rca << 16;
		acmd.c_flags = SCF_CMD_AC | SCF_RSP_R1 | SCF_RSP_SPI_R1;
	} else {
		acmd.c_arg = 0;
		acmd.c_flags = SCF_CMD_BCR | SCF_RSP_R1 | SCF_RSP_SPI_R1;
	}

	error = sdmmc_mmc_command(sc, &acmd);
	if (error == 0) {
		if (!ISSET(sc->caps, SMC_CAPS_SPI_MODE) &&
		    !ISSET(MMC_R1(acmd.c_resp), MMC_R1_APP_CMD)) {
			/* Card does not support application commands. */
			error = ENODEV;
		} else {
			error = sdmmc_mmc_command(sc, cmd);
		}
	}
	DPRINTF(("sdmmc_app_command: done (error=%d)\n", error));
	return error;
}

void
sdmmc_dump_command(struct sdmmc_softc *sc, struct sdmmc_command *cmd)
{
	int i;

	printf("cmd %u arg=%x data=%p dlen=%d flags=%x (error %d)\n",
	    cmd->c_opcode, cmd->c_arg, cmd->c_data,
	    cmd->c_datalen, cmd->c_flags, cmd->c_error);

	if (cmd->c_error )
		return;

	printf("resp=");
	if (ISSET(cmd->c_flags, SCF_RSP_136))
		for (i = 0; i < sizeof cmd->c_resp; i++)
			printf("%02x ", ((uint8_t *)cmd->c_resp)[i]);
	else if (ISSET(cmd->c_flags, SCF_RSP_PRESENT))
		for (i = 0; i < 4; i++)
			printf("%02x ", ((uint8_t *)cmd->c_resp)[i]);
	else
		printf("none");
	printf("\n");
}

int
sdmmc_mmc_command(struct sdmmc_softc *sc, struct sdmmc_command *cmd)
{
	int error;

	DPRINTF(("sdmmc_mmc_command: cmd=%d, arg=%x, flags=%x\n",
		 cmd->c_opcode, cmd->c_arg, cmd->c_flags));

#if 0
#if defined(DIAGNOSTIC) || defined(SDMMC_DEBUG)
	if (cmd->c_data && !ISSET(sc->caps, SMC_CAPS_SPI_MODE)) {
		if (sc->sc_card == NULL)
			panic("%s: deselected card\n", DEVNAME(sc));
	}
#endif
#endif

	sc->sdifdv->exec_cmd(sc->sdifdv->priv, cmd);

#ifdef SDMMC_DEBUG

	sdmmc_dump_command(sc, cmd);

#endif

	error = cmd->c_error;

	DPRINTF(("sdmmc_mmc_command: error=%d\n", error));

	return error;
}

/*
 * Scan for I/O functions and memory cards on the bus, allocating a
 * sdmmc_function structure for each.
 */
int
sdmmc_scan(struct sdmmc_softc *sc)
{

#if 0 /* SPI NOT SUPPORTED */
	if (!ISSET(sc->caps, SMC_CAPS_SPI_MODE)) {
		/* Scan for I/O functions. */
		if (ISSET(sc->sc_flags, SMF_IO_MODE))
			sdmmc_io_scan(sc);
	}
#endif

	/* Scan for memory cards on the bus. */
	if (ISSET(sc->flags, SMF_MEM_MODE))
		sdmmc_mem_scan(sc);

	DPRINTF(("Bus clock speed: %d\n", sc->busclk));
	return sc->sdifdv->bus_clock(sc->sdifdv->priv, sc->busclk);
}

/*
 * Read the CSD and CID from all cards and assign each card a unique
 * relative card address (RCA).  CMD2 is ignored by SDIO-only cards.
 */
void
sdmmc_mem_scan(struct sdmmc_softc *sc)
{
	sdmmc_response resp;
	//struct sdmmc_function *sf;
	//	uint16_t next_rca;
	int error;
	int retry;

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
			if (!ISSET(sc->caps, SMC_CAPS_SPI_MODE) &&
			    error == ETIMEDOUT) {
				/* No more cards there. */
				break;
			}
			DPRINTF(("Couldn't read CID\n"));
			break;
		}

		/* In MMC mode, find the next available RCA. */
		/*next_rca = 1;
		if (!ISSET(dv->flags, SMF_SD_MODE)) {
			SIMPLEQ_FOREACH(sf, &sc->sf_head, sf_list)
				next_rca++;
				}*/

		/* Allocate a sdmmc_function structure. */
		/*sf = sdmmc_function_alloc(sc);
		  sf->rca = next_rca;*/

		/*
		 * Remember the CID returned in the CMD2 response for
		 * later decoding.
		 */
		memcpy(sc->raw_cid, resp, sizeof(sc->raw_cid));

		/*
		 * Silence the card by assigning it a unique RCA, or
		 * querying it for its RCA in the case of SD.
		 */
		if (!ISSET(sc->caps, SMC_CAPS_SPI_MODE)) {
			if (sdmmc_set_relative_addr(sc) != 0) {
				DPRINTF(("couldn't set mem RCA\n"));
				break;
			}
		}

		/*
		 * If this is a memory-only card, the card responding
		 * first becomes an alias for SDIO function 0.
		 */
		/*if (sc->sc_fn0 == NULL)
			sc->sc_fn0 = sf;

			SIMPLEQ_INSERT_TAIL(&sc->sf_head, sf, sf_list);*/

		/* only one function in SPI mode */
		/*if (ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE))
		  break;*/
	}

	/*
	 * All cards are either inactive or awaiting further commands.
	 * Read the CSDs and decode the raw CID for each card.
	 */
	/*	SIMPLEQ_FOREACH(sf, &sc->sf_head, sf_list) {*/
	error = sdmmc_mem_send_csd(sc, &resp);
	if (error) {
		/*SET(sf->flags, SFF_ERROR);
		  continue;*/
	}

	if (sdmmc_decode_csd(sc, resp) != 0 ||
	    sdmmc_decode_cid(sc, sc->raw_cid) != 0) {
		/*SET(sf->flags, SFF_ERROR);
		  continue;*/
	}

#ifdef SDMMC_DEBUG
	printf("CID: ");
	sdmmc_print_cid(&sc->cid);
#endif
		/*	}*/
}

/*
 * Retrieve (SD) or set (MMC) the relative card address (RCA).
 */
int
sdmmc_set_relative_addr(struct sdmmc_softc *sc)
{
	struct sdmmc_command cmd;
	int error;

	/* Don't lock */

	if (ISSET(sc->caps, SMC_CAPS_SPI_MODE))
		return EIO;

	memset(&cmd, 0, sizeof(cmd));
	if (ISSET(sc->flags, SMF_SD_MODE)) {
		cmd.c_opcode = SD_SEND_RELATIVE_ADDR;
		cmd.c_flags = SCF_CMD_BCR | SCF_RSP_R6;
	} else {
		cmd.c_opcode = MMC_SET_RELATIVE_ADDR;
		cmd.c_arg = MMC_ARG_RCA(sc->rca);
		cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1;
	}
	error = sdmmc_mmc_command(sc, &cmd);
	if (error)
		return error;

	if (ISSET(sc->flags, SMF_SD_MODE))
		sc->rca = SD_R6_RCA(cmd.c_resp);

	return 0;
}

int
sdmmc_mem_send_cid(struct sdmmc_softc *sc, sdmmc_response *resp)
{
	struct sdmmc_command cmd;
	int error;


	memset(&cmd, 0, sizeof cmd);
	cmd.c_opcode = MMC_ALL_SEND_CID;
	cmd.c_flags = SCF_CMD_BCR | SCF_RSP_R2;

	error = sdmmc_mmc_command(sc, &cmd);

#ifdef SDMMC_DEBUG
	sdmmc_dump_data("CID", cmd.c_resp, sizeof(cmd.c_resp));
#endif
	if (error == 0 && resp != NULL)
		memcpy(resp, &cmd.c_resp, sizeof(*resp));
	return error;
}

void
sdmmc_dump_data(const char *title, void *ptr, size_t size)
{
	char buf[16];
	uint8_t *p = ptr;
	int i, j;

	printf("sdmmc_dump_data: %s\n", title ? title : "");
	printf("--------+--------------------------------------------------+------------------+\n");
	printf("offset  | +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +a +b +c +d +e +f | data             |\n");
	printf("--------+--------------------------------------------------+------------------+\n");
	for (i = 0; i < (int)size; i++) {
		if ((i % 16) == 0) {
			printf("%08x| ", i);
		} else if ((i % 16) == 8) {
			printf(" ");
		}

		printf("%02x ", p[i]);
		buf[i % 16] = p[i];

		if ((i % 16) == 15) {
			printf("| ");
			for (j = 0; j < 16; j++) {
				if (buf[j] >= 0x20 && buf[j] <= 0x7e) {
					printf("%c", buf[j]);
				} else {
					printf(".");
				}
			}
			printf(" |\n");
		}
	}
	if ((i % 16) != 0) {
		j = (i % 16);
		for (; j < 16; j++) {
			printf("   ");
			if ((j % 16) == 8) {
				printf(" ");
			}
		}

		printf("| ");
		for (j = 0; j < (i % 16); j++) {
			if (buf[j] >= 0x20 && buf[j] <= 0x7e) {
				printf("%c", buf[j]);
			} else {
				printf(".");
			}
		}
		for (; j < 16; j++) {
			printf(" ");
		}
		printf(" |\n");
	}
	printf("--------+--------------------------------------------------+------------------+\n");
}

int
sdmmc_mem_send_csd(struct sdmmc_softc *sc, sdmmc_response *resp)
{
	struct sdmmc_command cmd;
	int error;

	memset(&cmd, 0, sizeof cmd);
	cmd.c_opcode = MMC_SEND_CSD;
	cmd.c_arg = MMC_ARG_RCA(sc->rca);
	cmd.c_flags = SCF_CMD_AC | SCF_RSP_R2;

	error = sdmmc_mmc_command(sc, &cmd);

#ifdef SDMMC_DEBUG
	sdmmc_dump_data("CSD", cmd.c_resp, sizeof(cmd.c_resp));
#endif
	if (error == 0 && resp != NULL)
		memcpy(resp, &cmd.c_resp, sizeof(*resp));
	return error;
}

int
sdmmc_decode_csd(struct sdmmc_softc *sc, sdmmc_response resp)
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
	struct sdmmc_csd *csd = &sc->csd;
	int e, m;

	if (ISSET(sc->flags, SMF_SD_MODE)) {
		/*
		 * CSD version 1.0 corresponds to SD system
		 * specification version 1.0 - 1.10. (SanDisk, 3.5.3)
		 */
		csd->csdver = SD_CSD_CSDVER(resp);
		switch (csd->csdver) {
		case SD_CSD_CSDVER_2_0:
			DPRINTF(("SD Ver.2.0\n"));
			SET(sc->flags, SMF_CARD_SDHC);
			csd->capacity = SD_CSD_V2_CAPACITY(resp);
			csd->read_bl_len = SD_CSD_V2_BL_LEN;
                        csd->ccc = SD_CSD_CCC(resp);
			break;

		case SD_CSD_CSDVER_1_0:
			DPRINTF(("SD Ver.1.0\n"));
			csd->capacity = SD_CSD_CAPACITY(resp);
			csd->read_bl_len = SD_CSD_READ_BL_LEN(resp);
			break;

		default:
			printf("unknown SD CSD structure version 0x%x\n",
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
		if (csd->csdver == MMC_CSD_CSDVER_1_0 ) {
			printf("unknown MMC CSD structure version 0x%x\n",
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


	if (sc->busclk > csd->tran_speed)
		sc->busclk = csd->tran_speed;

#ifdef SDMMC_DUMP_CSD
	sdmmc_print_csd(resp, csd);
#endif

	return 0;
}

int
sdmmc_decode_cid(struct sdmmc_softc *sc, sdmmc_response resp)
{
	struct sdmmc_cid *cid = &sc->cid;

	if (ISSET(sc->flags, SMF_SD_MODE)) {
		cid->mid = SD_CID_MID(resp);
		cid->oid = SD_CID_OID(resp);
		SD_CID_PNM_CPY(resp, cid->pnm);
		cid->rev = SD_CID_REV(resp);
		cid->psn = SD_CID_PSN(resp);
		cid->mdt = SD_CID_MDT(resp);
	} else {
		switch(sc->csd.mmcver) {
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
			printf("unknown MMC version %d\n",
			    sc->csd.mmcver);
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

int
sdmmc_mem_read_block(struct sdmmc_softc *sc, uint32_t blkno,
    u_char *data, size_t datalen)
{
	struct sdmmc_command cmd;
	int error;

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_data = data;
	cmd.c_datalen = datalen;
	cmd.c_blklen = SDMMC_SECTOR_SIZE;
	cmd.c_opcode = (cmd.c_datalen / cmd.c_blklen) > 1 ?
	    MMC_READ_BLOCK_MULTIPLE : MMC_READ_BLOCK_SINGLE;
	cmd.c_arg = blkno;
	if (!ISSET(sc->flags, SMF_CARD_SDHC))
	  cmd.c_arg <<= SDMMC_SECTOR_SIZE_SB;
	DPRINTF(("Reading block %d (%d)\n", blkno, cmd.c_arg));
	cmd.c_flags = SCF_CMD_ADTC | SCF_CMD_READ | SCF_RSP_R1 | SCF_RSP_SPI_R1;

	error = sdmmc_mmc_command(sc, &cmd);
	if (error)
		goto out;

	if (!ISSET(sc->caps, SMC_CAPS_AUTO_STOP)) {
		if (cmd.c_opcode == MMC_READ_BLOCK_MULTIPLE) {
			memset(&cmd, 0, sizeof cmd);
			cmd.c_opcode = MMC_STOP_TRANSMISSION;
			cmd.c_arg = MMC_ARG_RCA(sc->rca);
			cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1B | SCF_RSP_SPI_R1B;
			error = sdmmc_mmc_command(sc, &cmd);
			if (error)
				goto out;
		}
	}

	/*if (!ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE)) {*/
	do {
		memset(&cmd, 0, sizeof(cmd));
		cmd.c_opcode = MMC_SEND_STATUS;
		cmd.c_arg = MMC_ARG_RCA(sc->rca);
		cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1 | SCF_RSP_SPI_R2;
		error = sdmmc_mmc_command(sc, &cmd);
		if (error)
			break;
		/* XXX time out */
	} while (!ISSET(MMC_R1(cmd.c_resp), MMC_R1_READY_FOR_DATA));
		/*}*/

out:
	return error;
}

int
sdmmc_select_card(struct sdmmc_softc *sc)
{
	struct sdmmc_command cmd;
	int error;

	/* Don't lock */

	/*	if (ISSET(sc->sc_caps, SMC_CAPS_SPI_MODE))
		return EIO;*/

	/*if (sc->sc_card == sf
	 || (sf && sc->sc_card && sc->sc_card->rca == sf->rca)) {
		sc->sc_card = sf;
		return 0;
		}*/

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_SELECT_CARD;
	cmd.c_arg = (sc == NULL) ? 0 : MMC_ARG_RCA(sc->rca);
	cmd.c_flags = SCF_CMD_AC | ((sc == NULL) ? SCF_RSP_R0 : SCF_RSP_R1);
	error = sdmmc_mmc_command(sc, &cmd);
	/*if (error == 0 || sf == NULL)
	  sc->sc_card = sf;*/

	return error;
}

/*
 * Set the read block length appropriately for this card, according to
 * the card CSD register value.
 */
int
sdmmc_mem_set_blocklen(struct sdmmc_softc *sc)
{
	struct sdmmc_command cmd;
	int error;

	/* Don't lock */

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_SET_BLOCKLEN;
	cmd.c_arg = SDMMC_SECTOR_SIZE;
	cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1 | SCF_RSP_SPI_R1;

	error = sdmmc_mmc_command(sc, &cmd);

	DPRINTF(("sdmmc_mem_set_blocklen: read_bl_len=%d sector_size=%d\n",
		 1 << sc->csd.read_bl_len, SDMMC_SECTOR_SIZE));

	return error;
}

int
sdmmc_mem_send_scr(struct sdmmc_softc *sc, uint32_t scr[2])
{
	struct sdmmc_command cmd;
	void *ptr = NULL;
	int datalen = 8;
	int error = 0;

	ptr = alloc(datalen); //malloc(datalen, M_DEVBUF, M_NOWAIT | M_ZERO);
	if (ptr == NULL)
		goto out;

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_data = ptr;
	cmd.c_datalen = datalen;
	cmd.c_blklen = datalen;
	cmd.c_arg = 0;
	cmd.c_flags = SCF_CMD_ADTC | SCF_CMD_READ | SCF_RSP_R1 | SCF_RSP_SPI_R1;
	cmd.c_opcode = SD_APP_SEND_SCR;

	error = sdmmc_app_command(sc, sc->rca, &cmd);
	if (error == 0) {
		memcpy(scr, ptr, datalen);
	}

out:
	if (ptr != NULL) {
		dealloc(ptr, datalen);
	}
	DPRINTF(("sdmem_mem_send_scr: error = %d\n",
	    error));
	if (error)
		return error;
#ifdef SDMMC_DEBUG
	sdmmc_dump_data("SCR", scr, 8);
#endif
	return error;
}

int
sdmmc_mem_decode_scr(struct sdmmc_softc *sc)
{
	sdmmc_response resp;
	int ver;

	memset(resp, 0, sizeof(resp));
	/*resp[0] = sc->raw_scr[1];
	resp[1] = sc->raw_scr[0];*/
	/*
	 * Change the raw-scr received from the DMA stream to resp.
	 */
	resp[0] = be32toh(sc->raw_scr[1]) >> 8;		// LSW
	resp[1] = be32toh(sc->raw_scr[0]);		// MSW
	resp[0] |= (resp[1] & 0xff) << 24;
	resp[1] >>= 8;
	resp[0] = htole32(resp[0]);
	resp[1] = htole32(resp[1]);

	ver = SCR_STRUCTURE(resp);
	sc->scr.sd_spec = SCR_SD_SPEC(resp);
	sc->scr.bus_width = SCR_SD_BUS_WIDTHS(resp);

	DPRINTF(("sdmmc_mem_decode_scr: spec=%d, bus width=%d\n",
	    sc->scr.sd_spec, sc->scr.bus_width));

	if (ver != 0) {
		DPRINTF(("unknown structure version: %d\n",
		    ver));
		return EINVAL;
	}
	return 0;
}

int
sdmmc_set_bus_width(struct sdmmc_softc *sc, int width)
{
	struct sdmmc_command cmd;
	int error;

	if (ISSET(sc->caps, SMC_CAPS_SPI_MODE))
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

	error = sdmmc_app_command(sc, sc->rca, &cmd);
	if (error == 0)
		error = sc->sdifdv->bus_width(sc->sdifdv->priv, width);
	return error;
}

#if 1
static int
sdmmc_mem_sd_switch(struct sdmmc_softc *sc, int mode, int group,
    int function, void *status)
{
	struct sdmmc_command cmd;
	void *ptr = NULL;
	int gsft, error = 0;
	const int statlen = 64;

	if (sc->scr.sd_spec >= SCR_SD_SPEC_VER_1_10 &&
	    !ISSET(sc->csd.ccc, SD_CSD_CCC_SWITCH))
		return EINVAL;

	if (group <= 0 || group > 6 ||
	    function < 0 || function > 16)
		return EINVAL;

	gsft = (group - 1) << 2;

	ptr = alloc(statlen);
	if (ptr == NULL)
		goto out;

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_data = ptr;
	cmd.c_datalen = statlen;
	cmd.c_blklen = statlen;
	cmd.c_opcode = SD_SEND_SWITCH_FUNC;
	cmd.c_arg =
	    (!!mode << 31) | (function << gsft) | (0x00ffffff & ~(0xf << gsft));
	cmd.c_flags = SCF_CMD_ADTC | SCF_CMD_READ | SCF_RSP_R1 | SCF_RSP_SPI_R1;

	error = sdmmc_mmc_command(sc, &cmd);
	if (error == 0) {
		memcpy(status, ptr, statlen);
	}

out:
	if (ptr != NULL) {
		dealloc(ptr, statlen);
	}
	return error;
}
#endif
