/* $NetBSD: linux_sg.h,v 1.3 2005/12/11 12:20:19 christos Exp $ */

/*
 * Copyright (c) 2004 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _LINUX_SG_H
#define _LINUX_SG_H

/* sg driver version 3 */
#define LINUX_SG_EMULATED_HOST		_LINUX_IO('"', 0x03)
#define LINUX_SG_SET_TRANSFORM		_LINUX_IO('"', 0x04)
#define LINUX_SG_GET_TRANSFORM		_LINUX_IO('"', 0x05)
#define LINUX_SG_SET_RESERVED_SIZE	_LINUX_IO('"', 0x75)
#define LINUX_SG_GET_RESERVED_SIZE	_LINUX_IO('"', 0x72)
#define LINUX_SG_GET_SCSI_ID		_LINUX_IO('"', 0x76)
#define LINUX_SG_SET_FORCE_LOW_DMA	_LINUX_IO('"', 0x79)
#define LINUX_SG_GET_LOW_DMA		_LINUX_IO('"', 0x7a)
#define LINUX_SG_SET_FORCE_PACK_ID	_LINUX_IO('"', 0x7b)
#define LINUX_SG_GET_PACK_ID		_LINUX_IO('"', 0x7c)
#define LINUX_SG_GET_NUM_WAITING	_LINUX_IO('"', 0x7d)
#define LINUX_SG_GET_SG_TABLESIZE	_LINUX_IO('"', 0x7f)
#define LINUX_SG_GET_VERSION_NUM	_LINUX_IO('"', 0x82)
#define LINUX_SG_SCSI_RESET		_LINUX_IO('"', 0x84)
#define LINUX_SG_IO			_LINUX_IO('"', 0x85)
#define LINUX_SG_GET_REQUEST_TABLE	_LINUX_IO('"', 0x86)
#define LINUX_SG_SET_KEEP_ORPHAN	_LINUX_IO('"', 0x87)
#define LINUX_SG_GET_KEEP_ORPHAN	_LINUX_IO('"', 0x88)
#define LINUX_SG_GET_ACCESS_COUNT	_LINUX_IO('"', 0x89)
/* sg driver version 2 */
#define LINUX_SG_SET_TIMEOUT		_LINUX_IO('"', 0x01)
#define LINUX_SG_GET_TIMEOUT		_LINUX_IO('"', 0x02)
#define LINUX_SG_GET_COMMAND_Q		_LINUX_IO('"', 0x70)
#define LINUX_SG_SET_COMMAND_Q		_LINUX_IO('"', 0x71)
#define LINUX_SG_SET_DEBUG		_LINUX_IO('"', 0x7e)
#define LINUX_SG_NEXT_CMD_LEN		_LINUX_IO('"', 0x83)


struct linux_sg_io_hdr {
	int interface_id;
#define SG_DXFER_NONE		-1
#define SG_DXFER_TO_DEV		-2
#define SG_DXFER_FROM_DEV	-3
#define SG_DXFER_TO_FROM_DEV	-4
#define SG_DXFER_UNKNOWN	-5
	int dxfer_direction;
	unsigned char cmd_len;
	unsigned char mx_sb_len;
	unsigned short iovec_count;
	unsigned int dxfer_len;
	void *dxferp;
	unsigned char *cmdp;
	unsigned char *sbp;
	unsigned int timeout;
	unsigned int flags;
	int pack_id;
	void *usr_ptr;
	unsigned char status;
	unsigned char masked_status;
	unsigned char msg_status;
	unsigned char sb_len_wr;
	unsigned short host_status;
	unsigned short driver_status;
	int resid;
	unsigned int duration;
	unsigned int info;
};

#define SG_MAX_SENSE 16

/*
 * Host codes
 */
#define LINUX_DID_OK		0x00	/* OK */
#define LINUX_DID_NO_CONNECT	0x01	/* timeout during connect */
#define LINUX_DID_BUS_BUSY	0x02	/* timeout during command */
#define LINUX_DID_TIME_OUT	0x03	/* other timeout */
#define LINUX_DID_BAD_TARGET	0x04	/* bad target */
#define LINUX_DID_ABORT		0x05	/* abort */
#define LINUX_DID_PARITY	0x06	/* parity error */
#define LINUX_DID_ERROR		0x07	/* internal error */
#define LINUX_DID_RESET		0x08	/* reset by somebody */
#define LINUX_DID_BAD_INTR	0x09	/* unexpected interrupt */
#define LINUX_DID_PASSTHROUGH	0x0a	/* passthrough */
#define LINUX_DID_SOFT_ERROR	0x0b	/* low driver wants retry */
#define LINUX_DID_IMM_RETRY	0x0c	/* retry without decreasing retrycnt */

/*
 * Driver codes
 */
#define LINUX_DRIVER_BUSY	0x01
#define LINUX_DRIVER_SOFT	0x02
#define LINUX_DRIVER_MEDIA	0x03
#define LINUX_DRIVER_ERROR	0x04

#define LINUX_DRIVER_INVALID	0x05
#define LINUX_DRIVER_TIMEOUT	0x06
#define LINUX_DRIVER_HARD	0x07
#define LINUX_DRIVER_SENSE	0x08

#define LINUX_SUGGEST_RETRY	0x10
#define LINUX_SUGGEST_ABORT	0x20
#define LINUX_SUGGEST_REMAP	0x30
#define LINUX_SUGGEST_DIE	0x40
#define LINUX_SUGGEST_SENSE	0x80
#define LINUX_SUGGEST_IS_OK	0xff

#define LINUX_DRIVER_MASK	0x0f
#define LINUX_SUGGEST_MASK	0xf0

#endif /* !_LINUX_SG_H */
