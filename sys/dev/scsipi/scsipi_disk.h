/*	$NetBSD: scsipi_disk.h,v 1.9 2003/09/17 23:33:43 mycroft Exp $	*/

/*
 * SCSI and SCSI-like interfaces description
 */

/*
 * Some lines of this file come from a file of the name "scsi.h"
 * distributed by OSF as part of mach2.5,
 *  so the following disclaimer has been kept.
 *
 * Copyright 1990 by Open Software Foundation,
 * Grenoble, FRANCE
 *
 * 		All Rights Reserved
 *
 *   Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OSF or Open Software
 * Foundation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.
 *
 *   OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Largely written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

#ifndef _DEV_SCSIPI_SCSIPI_DISK_H_
#define _DEV_SCSIPI_SCSIPI_DISK_H_

/*
 * SCSI command format
 */

#define	READ_BIG		0x28
#define WRITE_BIG		0x2a
struct scsipi_rw_big {
	u_int8_t opcode;
	u_int8_t byte2;
#define	SRWB_RELADDR	0x01
#define	SRWB_FUA	0x08	/* force unit access */
#define	SRWB_DPO	0x10	/* disable page out */
	u_int8_t addr[4];
	u_int8_t reserved;
	u_int8_t length[2];
	u_int8_t control;
} __attribute__((packed));

#define	READ_CAPACITY		0x25
struct scsipi_read_capacity {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t addr[4];
	u_int8_t unused[3];
	u_int8_t control;
} __attribute__((packed));

/* DATAs definitions for the above commands */

struct scsipi_read_cap_data {
	u_int8_t addr[4];
	u_int8_t length[4];
} __attribute__((packed));

#define READ_FORMAT_CAPACITIES	0x23
struct scsipi_read_format_capacities {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t reserved1[5];
	u_int8_t length[2];
	u_int8_t reserved2[3];
} __attribute__((packed));

struct scsipi_capacity_list_header {
	u_int8_t reserved[3];
	u_int8_t length;
} __attribute__((packed));

struct scsipi_capacity_descriptor {
	u_int8_t nblks[4];
	u_int8_t byte5;
	u_int8_t blklen[3];
} __attribute__((packed));

/* codes only valid in the current/maximum capacity descriptor */
#define	SCSIPI_CAP_DESC_CODE_MASK		0x3
#define	SCSIPI_CAP_DESC_CODE_RESERVED		0x0
#define	SCSIPI_CAP_DESC_CODE_UNFORMATTED	0x1
#define	SCSIPI_CAP_DESC_CODE_FORMATTED		0x2
#define	SCSIPI_CAP_DESC_CODE_NONE		0x3

#endif /* _DEV_SCSIPI_SCSIPI_DISK_H_ */
