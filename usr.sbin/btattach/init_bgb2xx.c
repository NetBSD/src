/*	$NetBSD: init_bgb2xx.c,v 1.1 2008/04/15 11:17:48 plunky Exp $	*/

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
__RCSID("$NetBSD: init_bgb2xx.c,v 1.1 2008/04/15 11:17:48 plunky Exp $");

#include <bluetooth.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>

#include "btattach.h"

#define HCI_CMD_ERICSSON_READ_REVISION_INFORMATION	\
	HCI_OPCODE(HCI_OGF_VENDOR, 0x00f)

#define HCI_CMD_ST_STORE_IN_NVDS			\
	HCI_OPCODE(HCI_OGF_VENDOR, 0x022)

typedef struct {
	uint8_t		d0;	/* ? */
	uint8_t		d1;	/* ? */
	bdaddr_t	bdaddr;
} __attribute__ ((__packed__)) hci_store_in_nvds_cp;

static const char *default_bdaddr = "bd:b2:10:00:ab:ba";

void
init_bgb2xx(int fd, unsigned int speed)
{
	hci_store_in_nvds_cp cp;

	/* bluez also reads revision information from device */

	/* don't seem to be able to set the speed */

	cp.d0 = 0xfe;
	cp.d1 = 0x06;
	bt_aton(default_bdaddr, &cp.bdaddr);

	uart_send_cmd(fd, HCI_CMD_ST_STORE_IN_NVDS, &cp, sizeof(cp));
	uart_recv_cc(fd, HCI_CMD_ST_STORE_IN_NVDS, NULL, 0);
	/* assume it succeeded? */

	uart_send_cmd(fd, HCI_CMD_RESET, NULL, 0);
	uart_recv_cc(fd, HCI_CMD_RESET, NULL, 0);
	/* assume it succeeded? */
}
