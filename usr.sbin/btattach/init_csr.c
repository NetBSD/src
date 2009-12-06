/*	$NetBSD: init_csr.c,v 1.2 2009/12/06 12:29:48 kiyohara Exp $	*/

/*-
 * Copyright (c) 2008 Iain Hibbert
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
 * init information in this file gleaned from hciattach(8)
 * command from BlueZ for Linux - see http://www.bluez.org/
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: init_csr.c,v 1.2 2009/12/06 12:29:48 kiyohara Exp $");

#include <bluetooth.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <dev/bluetooth/bcsp.h>

#include "btattach.h"

struct bccmd {
	uint8_t	chanid;

	struct {
		uint16_t type;
		uint16_t length;
		uint16_t seqno;
		uint16_t varid;
		uint16_t status;
		uint16_t payload[4];
	} message;
} __attribute__ ((__packed__));

#define CSR_BCCMD_FIRST		(1<<6)
#define CSR_BCCMD_LAST		(1<<7)

#define CSR_BCCMD_MESSAGE_TYPE_GETREQ	0x0000
#define CSR_BCCMD_MESSAGE_TYPE_GETRESP	0x0001
#define CSR_BCCMD_MESSAGE_TYPE_SETREQ	0x0002

#define CSR_BCCMD_MESSAGE_VARID_CONFIG_UART		0x6802
#define CSR_BCCMD_MESSAGE_VARID_CONFIG_UART_STOPB	0x2000
#define CSR_BCCMD_MESSAGE_VARID_CONFIG_UART_PARENB	0x4000
#define CSR_BCCMD_MESSAGE_VARID_CONFIG_UART_PARODD	0x8000

#define CSR_BCCMD_MESSAGE_STATUS_OK			0x0000
#define CSR_BCCMD_MESSAGE_STATUS_NO_SUCH_VARID		0x0001
#define CSR_BCCMD_MESSAGE_STATUS_TOO_BIG		0x0002
#define CSR_BCCMD_MESSAGE_STATUS_NO_VALUE		0x0003
#define CSR_BCCMD_MESSAGE_STATUS_BAD_REQ		0x0004
#define CSR_BCCMD_MESSAGE_STATUS_NO_ACCESS		0x0005
#define CSR_BCCMD_MESSAGE_STATUS_READ_ONLY		0x0006
#define CSR_BCCMD_MESSAGE_STATUS_WRITE_ONLY		0x0007
#define CSR_BCCMD_MESSAGE_STATUS_ERROR			0x0008
#define CSR_BCCMD_MESSAGE_STATUS_PERMISION_DENIED	0x0009

void
init_csr(int fd, unsigned int speed)
{
	struct bccmd cmd;

	/* setup BlueCore command packet */
	memset(&cmd, 0, sizeof(cmd));

	cmd.chanid = CSR_BCCMD_LAST | CSR_BCCMD_FIRST | BCSP_CHANNEL_BCCMD;

	cmd.message.type = htole16(CSR_BCCMD_MESSAGE_TYPE_SETREQ);
	cmd.message.length = htole16(sizeof(cmd.message) >> 1);
	cmd.message.seqno = htole16(0);
	cmd.message.varid = htole16(CSR_BCCMD_MESSAGE_VARID_CONFIG_UART);
	cmd.message.status = htole16(CSR_BCCMD_MESSAGE_STATUS_OK);

	/* Value = (baud rate / 244.140625) | no parity | 1 stop bit. */
	cmd.message.payload[0] = htole16((speed * 64 + 7812) / 15625);

	uart_send_cmd(fd, HCI_CMD_CSR_EXTN, &cmd, sizeof(cmd));
	uart_recv_cc(fd, HCI_CMD_CSR_EXTN, NULL, 0);
	/* assume it succeeded? */
}
