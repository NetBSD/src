/*	$NetBSD: sdvar.h,v 1.1.2.2 2010/10/22 07:21:08 uebayasi Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _STAND_SDVAR_H
#define _STAND_SDVAR_H

#include <dev/ic/siopreg.h>
#include <dev/pci/pcireg.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsi_spc.h>

#include <sys/disklabel.h>
#include <sys/bootblock.h>


/* tables used by SCRIPT */
typedef struct scr_table {
	uint32_t count;
	uint32_t addr;
} __packed scr_table_t;

/* Number of scatter/gather entries */
#define SIOP_NSG	(0x10000/0x1000 + 1)	/* XXX PAGE_SIZE */
/*
 * This structure interfaces the SCRIPT with the driver; it describes a full
 * transfer.
 * If you change something here, don't forget to update offsets in {s,es}iop.ss
 */
struct siop_common_xfer {
	uint8_t msg_out[16];	/* 0 */
	uint8_t msg_in[16];	/* 16 */
	uint32_t status;	/* 32 */
	uint32_t pad1;		/* 36 */
	uint32_t id;		/* 40 */
	uint32_t pad2;		/* 44 */
	scr_table_t t_msgin;	/* 48 */
	scr_table_t t_extmsgin;	/* 56 */
	scr_table_t t_extmsgdata; /* 64 */
	scr_table_t t_msgout;	/* 72 */
	scr_table_t cmd;	/* 80 */
	scr_table_t t_status;	/* 88 */
	scr_table_t data[SIOP_NSG]; /* 96 */
} __packed;

/* status can hold the SCSI_* status values, and 2 additional values: */
#define SCSI_SIOP_NOCHECK	0xfe	/* don't check the scsi status */
#define SCSI_SIOP_NOSTATUS	0xff	/* device didn't report status */


/*
 * xfer description of the script: tables and reselect script
 * In struct siop_common_cmd siop_xfer will point to this.
 */
struct siop_xfer {
	struct siop_common_xfer siop_tables;
	/* uint32_t resel[sizeof(load_dsa) / sizeof(load_dsa[0])]; */
	uint32_t resel[25];
} __packed;


#define SIOP_SCRIPT_SIZE	4096
#define SIOP_TABLE_SIZE		4096
#define SIOP_SCSI_COMMAND_SIZE	4096
#define SIOP_SCSI_DATA_SIZE	4096

struct scsi_xfer {
	int target;
	int lun;
	struct scsipi_generic *cmd;
	int cmdlen;
	u_char *data;
	int datalen;
	int resid;
	scsipi_xfer_result_t error;
	uint8_t status;				/* SCSI status */
	int xs_status;
};

struct sd_softc;
struct siop_adapter {
	int id;
	u_long addr;				/* register map address */
	int clock_div;
	uint32_t *script;			/* script addr */
	struct siop_xfer *xfer;			/* xfer addr */
	struct scsipi_generic *cmd;		/* SCSI command buffer */
	struct scsi_request_sense *sense;	/* SCSI sense buffer */
	u_char *data;				/* SCSI data buffer */

	struct scsi_xfer *xs;

	int currschedslot;		/* current scheduler slot */

	int sel_t;			/* selected target */
	struct sd_softc *sd;
};

struct sd_softc {
	int sc_part;
	int sc_lun;
	int sc_target;
	int sc_bus;

	int sc_type;
	int sc_cap;
	int sc_flags;
#define FLAGS_MEDIA_LOADED	(1 << 0)
#define FLAGS_REMOVABLE		(1 << 1)

	struct disk_parms {
		u_long	heads;		/* number of heads */
		u_long	cyls;		/* number of cylinders */
		u_long	sectors;	/* number of sectors/track */
		u_long	blksize;	/* number of bytes/sector */
		u_long	rot_rate;	/* rotational rate, in RPM */
		u_int64_t disksize;	/* total number sectors */
		u_int64_t disksize512;	/* total number sectors */
	} sc_params;
	struct disklabel sc_label;
};

int	siop_init(int, int, int);
int	scsi_inquire(struct sd_softc *, int, void *);
int	scsi_mode_sense(struct sd_softc *, int, int,
			struct scsi_mode_parameter_header_6 *, int);
int	scsi_command(struct sd_softc *, void *, int, void *, int);


#define	SDGP_RESULT_OK		0	/* parameters obtained */
#define	SDGP_RESULT_OFFLINE	1	/* no media, or otherwise losing */
#define	SDGP_RESULT_UNFORMATTED	2	/* unformatted media (max params) */

#define ERESTART	-1

#endif /* _STAND_SDVAR_H */
