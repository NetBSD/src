/*	$NetBSD: iris_scsicmd.h,v 1.1 2019/01/12 16:44:47 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Naruaki Etomi
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

/*
 * Silicon Graphics "IRIS" series MIPS processors machine bootloader.
 */

/*
 * scsipi_xfer status flags
 */
#define XS_STS_DONE		0x00000001

/* SCSI Commands */
#define CMD_TEST_UNIT_READY		0x00
#define CMD_REQUEST_SENSE		0x03
#define CMD_INQUIRY			0x12
#define CMD_SEND_DIAGNOSTIC		0x1D

#define CMD_REWIND			0x01
#define CMD_REZERO			0x01
#define CMD_FORMAT_UNIT			0x04
#define CMD_READ_BLOCK_LIMITS		0x05
#define CMD_REASSIGN_BLOCKS		0x07
#define CMD_READ			0x08
#define CMD_WRITE			0x0A
#define CMD_WRITE_FILEMARK		0x10
#define CMD_SPACE			0x11
#define CMD_MODE_SELECT			0x15
#define CMD_RELEASE_UNIT		0x17
#define CMD_ERASE			0x19
#define CMD_MODE_SENSE			0x1A
#define CMD_LOADUNLOAD			0x1B
#define CMD_RECEIVE_DIAG		0x1C
#define CMD_SEND_DIAG			0x1D
#define CMD_P_A_MEDIA_REMOVAL		0x1E
#define CMD_READ_CAPACITY		0x25
#define CMD_READ_EXT			0x28
#define CMD_WRITE_EXT			0x2A
#define CMD_READ_DEFECT_DATA		0x37
#define 	SD_MANUFAC_DEFECTS	0x14000000
#define 	SD_GROWN_DEFECTS	0x0c000000
#define CMD_READ_BUFFER			0x3B
#define CMD_WRITE_BUFFER		0x3C
#define CMD_READ_FULL			0xF0
#define CMD_MEDIA_TEST			0xF1
#define CMD_ACCESS_LOG			0xF2
#define CMD_WRITE_FULL			0xFC
#define CMD_MANAGE_PRIMARY		0xFD
#define CMD_EXECUTE_DATA		0xFE

/* command descriptor blocks */

struct scsi_cdb6 {
	uint8_t	cmd;		/* command code */
	uint8_t	lun:  3,	/* logical unit on ctlr */
		lbah: 5;	/* msb of read/write logical block addr */
	uint8_t	lbam;		/* middle byte of l.b.a. */
	uint8_t	lbal;		/* lsb of l.b.a. */
	uint8_t	len;		/* transfer length */
	uint8_t	xtra;
};

struct scsi_cdb10 {
	uint8_t	cmd;		/* command code */
	uint8_t	lun: 3,		/* logical unit on ctlr */
		   : 4,
		rel: 1;		/* l.b.a. is relative addr if =1 */
	uint8_t	lbah;		/* msb of read/write logical block addr */
	uint8_t	lbahm;		/* high middle byte of l.b.a. */
	uint8_t	lbalm;		/* low middle byte of l.b.a. */
	uint8_t	lbal;		/* lsb of l.b.a. */
	uint8_t	reserved;
	uint8_t	lenh;		/* msb transfer length */
	uint8_t	lenl;		/* lsb transfer length */
	uint8_t	xtra;
};
