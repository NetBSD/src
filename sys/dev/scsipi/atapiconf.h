/*	$NetBSD: atapiconf.h,v 1.1.2.1 1997/07/01 16:52:09 bouyer Exp $	*/

/*
 * Copyright (c) 1996 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SCSI_ATAPICONF_H
#define SCSI_ATAPICONF_H

#include <dev/scsipi/scsipiconf.h>

#undef ATAPI_DEBUG
#undef ATAPI_DEBUG2
#undef ATAPI_DEBUG_PROBE
#undef ATAPI_DEBUG_WDC

struct atapi_identify {
	struct	config_s {
		u_int8_t cmd_drq_rem;
#define ATAPI_PACKET_SIZE_MASK	0x03
#define ATAPI_PACKET_SIZE_12	0x00
#define ATAPI_PACKET_SIZE_16	0x01

#define ATAPI_DRQ_MASK		0x60
#define ATAPI_MICROPROCESSOR_DRQ 0x00
#define ATAPI_INTERRUPT_DRQ	0x20
#define ATAPI_ACCELERATED_DRQ	0x40

#define ATAPI_REMOVABLE		0x80

		u_int8_t device_type;
#define ATAPI_DEVICE_TYPE_MASK	0x1f
#define ATAPI_DEVICE_TYPE_DAD	0x00	/* direct access device */
					/* 0x1-0x4 reserved */
#define ATAPI_DEVICE_TYPE_CD	0x05	/* CD-ROM */
					/* 0x6 reserved */
#define ATAPI_DEVICE_TYPE_OMD	0x07	/* optical memory device */
					/* 0x8-0x1e reserved */
#define ATAPI_DEVICE_TYPE_UNKNOWN 0x1f

#define ATAPI_GC_PROTOCOL_MASK	0xc0	/* mask of protocol bits */
					/* 0x00 and 0x01 are ATA */
#define ATAPI_GC_PROTO_TYPE_ATAPI 0x80
#define ATAPI_GC_PROTO_TYPE_RESERVED 0xc0
	} config;				/* general configuration */

	u_int8_t	cylinders[2];
	u_int8_t	reserved1[2];
	u_int8_t	heads[2];
	u_int8_t	unf_bytes_per_track[2];
	u_int8_t	unf_bytes_per_sector[2];
	u_int8_t	sectors_per_track[2];
	u_int8_t	reserved2[6];
	char		serial_number[20];
	u_int8_t	buffer_type[2];
	u_int8_t	buffer_size[2];
	u_int8_t	ECC_bytes_available[2];
	char		firmware_revision[8];
	char		model[40];
	u_int8_t	sector_count[2];
	u_int8_t	double_word[2];		/* == 0 for CD-ROMs */

	struct capabilities_s {
		u_int8_t vendor;
		u_int8_t capflags;
#define ATAPI_CAP_DMA			0x01	/* DMA supported */
#define ATAPI_CAP_LBA			0x02	/* LBA supported */
#define ATAPI_IORDY_DISABLE		0x04	/* IORDY can be disabled */
#define ATAPI_IORDY			0x08	/* IORDY supported */
	} capabilities;

	u_int8_t	reserved3[2];
	u_int8_t	PIO_cycle_timing[2];
	u_int8_t	DMA_cycle_timing[2];
	u_int8_t	validity[2]; /* of words 54-58, 64-70 in this table */

#define ATAPI_VALID_FIRST	0x0	/* == 1 => words 54-58 are valid */
#define ATAPI_VALID_SECOND	0x1	/* == 1 => words 64-70 are valid */

	u_int8_t	current_chs[6];	/* cylinder/head/sector */
	u_int8_t	current_capacity[4];
	u_int8_t	reserved4[2];
	u_int8_t	user_addressable_sectors[4];
	u_int8_t	singleword_DMA_mode[2];

#define ATAPI_SW_DMA_MODE_AVAIL	0x00ff	/* Mode 0 is supported */
#define ATAPI_SW_DMA_MODE_ACTIVE 0xff00	/* which mode is active */

	u_int8_t	multiword_DMA_mode[2];

#define ATAPI_MW_DMA_MODE_AVAIL	0x00ff	/* Mode 0 is supported */
#define ATAPI_MW_DMA_MODE_ACTIVE 0xff00	/* which mode is active */

	u_int8_t	enhanced_PIO_mode[2];

#define ATAPI_ENHANCED_PIO_AVAIL 0x0001	/* PIO Mode 3 is supported */

	u_int8_t	blind_PIO_minimum_cycles[2];
	u_int8_t	mw_dma_tct[2]; /* multi-word DMA transfer cycle time */
	u_int8_t	min_PIO_tct_no_flow_control[2];
	u_int8_t	min_PIO_tct_with_flow_control[2];
	u_int8_t	reserved5[4];
	u_int8_t	reserved6[114];
	u_int8_t	vendor[64];	/* vendor unique */
	u_int8_t	reserved7[192];
};

struct atapibus_attach_args {
	struct scsipi_link *sa_sc_link;
	struct atapi_identify *sa_inqbuf;
};

int	wdc_atapi_get_params __P((struct scsipi_link *, u_int8_t,
	    struct atapi_identify *)); 
void atapi_print_addr __P((struct scsipi_link *));
int atapi_interpret_sense __P((struct scsipi_xfer *));
int atapi_scsipi_cmd __P((struct scsipi_link *, struct scsipi_generic *,
		int, u_char *, int, int, int, struct buf *, int));
#endif /* SCSI_ATAPICONF_H */
