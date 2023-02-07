/*	$NetBSD: init_bcm43xx.c,v 1.6 2023/02/07 20:45:44 mlelstv Exp $	*/

/*-
 * Copyright (c) 2017 Nathanial Sloss <nathanialsloss@yahoo.com.au>
 * All rights reserved.
 *
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
__RCSID("$NetBSD: init_bcm43xx.c,v 1.6 2023/02/07 20:45:44 mlelstv Exp $");

#include <sys/param.h>

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "btattach.h"
#include "firmload.h"

#define HCI_CMD_BCM43XX_SET_UART_BAUD_RATE	\
	HCI_OPCODE(HCI_OGF_VENDOR, 0x018)

#define HCI_CMD_BCM43XX_SET_BDADDR		\
	HCI_OPCODE(HCI_OGF_VENDOR, 0x006)

#define HCI_CMD_BCM43XX_SET_CLOCK		\
	HCI_OPCODE(HCI_OGF_VENDOR, 0x045)

#define HCI_CMD_43XXFWDN 			\
	HCI_OPCODE(HCI_OGF_VENDOR, 0x02e)

#define HCI_CMD_GET_LOCAL_NAME			0x0c14

#define BCM43XX_CLK_48	1
#define BCM43XX_CLK_24	2

static int
bcm43xx_get_local_name(int fd, char *name, size_t namelen)
{
	char buf[256];
	size_t len;

	memset(buf, 0, sizeof(buf));

	uart_send_cmd(fd, HCI_CMD_GET_LOCAL_NAME, NULL, 0);
	len = uart_recv_cc(fd, HCI_CMD_GET_LOCAL_NAME, buf, sizeof(buf));
	if (len == 0)
		return EIO;

	strlcpy(name, &buf[1], MIN(len - 1, namelen));

	if (strlen(name) == 0)
		return EIO;

	return 0;
}

void
init_bcm43xx(int fd, unsigned int speed)
{
	uint8_t rate[6], clock;
	uint8_t fw_buf[1024];
	int fwfd, fw_len;
	uint8_t resp[7];
	uint16_t fw_cmd;
	char local_name[256];
	char fw[260];

	memset(rate, 0, sizeof(rate));
	memset(local_name, 0, sizeof(local_name));

	if (uart_send_cmd(fd, HCI_CMD_RESET, NULL, 0))
		return;
	uart_recv_cc(fd, HCI_CMD_RESET, &resp, sizeof(resp));
	/* assume it succeeded? */

	if (bcm43xx_get_local_name(fd, local_name, sizeof(local_name)) != 0) {
		fprintf(stderr, "Couldn't read local name\n");
		return;
	}
	snprintf(fw, sizeof(fw), "%s.hcd", local_name);

	fwfd = firmware_open("bcm43xx", fw);
	if (fwfd < 0) {
		fprintf(stderr, "Unable to open firmware bcm43xx/%s: %s\n",
		    fw, strerror(errno));
		return;
	}

	uart_send_cmd(fd, HCI_CMD_43XXFWDN, NULL, 0);
	uart_recv_cc(fd, HCI_CMD_43XXFWDN, &resp, sizeof(resp));
		sleep(1);

	while (read(fwfd, &fw_buf[1], 3) == 3) {
		fw_buf[0] = HCI_CMD_PKT;
		fw_len = fw_buf[3];
		if (read(fwfd, &fw_buf[4], fw_len) != fw_len)
			break;
		fw_cmd = fw_buf[2] << 8 | fw_buf[1];
		uart_send_cmd(fd, fw_cmd, &fw_buf[4], fw_len);
		uart_recv_cc(fd, fw_cmd, &resp, sizeof(resp));
	}
	
	close(fwfd);
		
	sleep(4);
	uart_send_cmd(fd, HCI_CMD_RESET, NULL, 0);
	uart_recv_cc(fd, HCI_CMD_RESET, &resp, sizeof(resp));
	/* assume it succeeded? */

	if (speed >= 3000000) 
		clock = BCM43XX_CLK_48;
	else
		clock = BCM43XX_CLK_24;

	uart_send_cmd(fd, HCI_CMD_BCM43XX_SET_CLOCK, &clock, sizeof(clock));
	uart_recv_cc(fd, HCI_CMD_BCM43XX_SET_CLOCK, &resp, sizeof(resp));

	rate[2] = speed;
	rate[3] = speed >> 8;
	rate[4] = speed >> 16;
	rate[5] = speed >> 24;

	uart_send_cmd(fd, HCI_CMD_BCM43XX_SET_UART_BAUD_RATE, &rate, sizeof(rate));
	uart_recv_cc(fd, HCI_CMD_BCM43XX_SET_UART_BAUD_RATE, &resp, sizeof(resp));
}
