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
#ifndef _SDIF_H_
#define _SDIF_H_

#include <sys/types.h>
#include <sys/queue.h>

/* Delay function used by SD/MMC drivers */
void sdmmc_delay(int);

#define	SDMMC_SECTOR_SIZE_SB	9
#define	SDMMC_SECTOR_SIZE	(1 << SDMMC_SECTOR_SIZE_SB)	/* =512 */

struct sdmmc_csd {
	int	csdver;		/* CSD structure format */
	u_int	mmcver;		/* MMC version (for CID format) */
	int	capacity;	/* total number of sectors */
	int	read_bl_len;	/* block length for reads */
	int	write_bl_len;	/* block length for writes */
	int	r2w_factor;
	int	tran_speed;	/* transfer speed (kbit/s) */
	int	ccc;		/* Card Command Class for SD */
	/* ... */
};

struct sdmmc_cid {
	int	mid;		/* manufacturer identification number */
	int	oid;		/* OEM/product identification number */
	char	pnm[8];		/* product name (MMC v1 has the longest) */
	int	rev;		/* product revision */
	int	psn;		/* product serial number */
	int	mdt;		/* manufacturing date */
};

struct sdmmc_scr {
	int	sd_spec;
	int	bus_width;
};

typedef uint32_t sdmmc_response[4];

struct sdmmc_command {
	uint16_t	 c_opcode;	/* SD or MMC command index */
	uint32_t	 c_arg;		/* SD/MMC command argument */
	sdmmc_response	 c_resp;	/* response buffer */
	/*bus_dmamap_t	 c_dmamap;*/
	void		*c_data;	/* buffer to send or read into */
	int		 c_datalen;	/* length of data buffer */
	int		 c_blklen;	/* block length */
	int		 c_flags;	/* see below */
#define SCF_ITSDONE	(1U << 0)		/* command is complete */
#define SCF_RSP_PRESENT	(1U << 1)
#define SCF_RSP_BSY	(1U << 2)
#define SCF_RSP_136	(1U << 3)
#define SCF_RSP_CRC	(1U << 4)
#define SCF_RSP_IDX	(1U << 5)
#define SCF_CMD_READ	(1U << 6)	/* read command (data expected) */
/* non SPI */
#define SCF_CMD_AC	(0U << 8)
#define SCF_CMD_ADTC	(1U << 8)
#define SCF_CMD_BC	(2U << 8)
#define SCF_CMD_BCR	(3U << 8)
#define SCF_CMD_MASK	(3U << 8)
/* SPI */
#define SCF_RSP_SPI_S1	(1U << 10)
#define SCF_RSP_SPI_S2	(1U << 11)
#define SCF_RSP_SPI_B4	(1U << 12)
#define SCF_RSP_SPI_BSY	(1U << 13)
/* response types */
#define SCF_RSP_R0	0	/* none */
#define SCF_RSP_R1	(SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX)
#define SCF_RSP_R1B	(SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX|SCF_RSP_BSY)
#define SCF_RSP_R2	(SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_136)
#define SCF_RSP_R3	(SCF_RSP_PRESENT)
#define SCF_RSP_R4	(SCF_RSP_PRESENT)
#define SCF_RSP_R5	(SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX)
#define SCF_RSP_R5B	(SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX|SCF_RSP_BSY)
#define SCF_RSP_R6	(SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX)
#define SCF_RSP_R7	(SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX)
/* SPI */
#define SCF_RSP_SPI_R1	(SCF_RSP_SPI_S1)
#define SCF_RSP_SPI_R1B	(SCF_RSP_SPI_S1|SCF_RSP_SPI_BSY)
#define SCF_RSP_SPI_R2	(SCF_RSP_SPI_S1|SCF_RSP_SPI_S2)
#define SCF_RSP_SPI_R3	(SCF_RSP_SPI_S1|SCF_RSP_SPI_B4)
#define SCF_RSP_SPI_R4	(SCF_RSP_SPI_S1|SCF_RSP_SPI_B4)
#define SCF_RSP_SPI_R5	(SCF_RSP_SPI_S1|SCF_RSP_SPI_S2)
#define SCF_RSP_SPI_R7	(SCF_RSP_SPI_S1|SCF_RSP_SPI_B4)
	int		 c_error;	/* errno value on completion */

	/* Host controller owned fields for data xfer in progress */
	int c_resid;			/* remaining I/O */
	u_char *c_buf;			/* remaining data */
};

/*
 * Decoded PC Card 16 based Card Information Structure (CIS),
 * per card (function 0) and per function (1 and greater).
 */
struct sdmmc_cis {
	uint16_t	 manufacturer;
#define SDMMC_VENDOR_INVALID	0xffff
	uint16_t	 product;
#define SDMMC_PRODUCT_INVALID	0xffff
	uint8_t		 function;
#define SDMMC_FUNCTION_INVALID	0xff
	u_char		 cis1_major;
	u_char		 cis1_minor;
	char		 cis1_info_buf[256];
	char		*cis1_info[4];
};

#define SMF_INITED		0x0001
#define SMF_SD_MODE		0x0002	/* host in SD mode (MMC otherwise) */
#define SMF_IO_MODE		0x0004	/* host in I/O mode (SD mode only) */
#define SMF_MEM_MODE		0x0008	/* host in memory mode (SD or MMC) */
#define SMF_CARD_PRESENT	0x4000	/* card presence noticed */
#define SMF_CARD_ATTACHED	0x8000	/* card driver(s) attached */
#define SMF_CARD_SDHC		0x0010  /* card is sdhc */

#define SMC_CAPS_AUTO_STOP	0x0001	/* send CMD12 automagically by host */
#define SMC_CAPS_4BIT_MODE	0x0002	/* 4-bits data bus width */
#define SMC_CAPS_DMA		0x0004	/* DMA transfer */
#define SMC_CAPS_SPI_MODE	0x0008	/* SPI mode */
#define SMC_CAPS_POLL_CARD_DET	0x0010	/* Polling card detect */
#define SMC_CAPS_SINGLE_ONLY	0x0020	/* only single read/write */

#endif
