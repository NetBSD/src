/*	$NetBSD: init_swave.c,v 1.1.10.1 2009/05/13 19:20:19 jym Exp $	*/

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
__RCSID("$NetBSD: init_swave.c,v 1.1.10.1 2009/05/13 19:20:19 jym Exp $");

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "btattach.h"

#define HCI_CMD_SWAVE_PARAM_ACCESS_SET \
	HCI_OPCODE(HCI_OGF_VENDOR, 0x00b)

typedef struct {
	uint8_t		sub_command;
	uint8_t		param_type;
	uint8_t		param_length;
	uint8_t		transport_flow;
	uint8_t		transport_type;
	uint8_t		transport_rate;
} __attribute__ ((__packed__)) hci_param_access_set_cp;

typedef struct {
	uint8_t			d0;	/* ? */
	hci_param_access_set_cp	cp;
} __attribute__ ((__packed__)) hci_param_access_set_rp;
	
void
init_swave(int fd, unsigned int speed)
{
	hci_param_access_set_cp cp;
	hci_param_access_set_rp rp;
	size_t n;

	cp.sub_command    = 0x01; /* set */
	cp.param_type     = 0x11; /* HCI Transport */
	cp.param_length   = 0x03;
	cp.transport_flow = 0x01; /* flow control */
	cp.transport_type = 0x01; /* UART */

	switch(speed) {
	case B19200:	cp.transport_rate = 0x03;	break;
	case B38400:	cp.transport_rate = 0x02;	break;
	case B57600:	cp.transport_rate = 0x01;	break;
	case B115200:	cp.transport_rate = 0x00;	break;
	default:
		errx(EXIT_FAILURE, "invalid speed for swave: %u\n", speed);
	}

	uart_send_cmd(fd, HCI_CMD_SWAVE_PARAM_ACCESS_SET, &cp, sizeof(cp));

	/*
	 * we wait for a HCI_EVENT_VENDOR response with "0x0b" in the
	 * first byte (possibly corresponding to the OGF?), and the
	 * response should contain our settings as confirmation.
	 */
	do {
		n = uart_recv_ev(fd, HCI_EVENT_VENDOR, &rp, sizeof(rp));
	} while (n != sizeof(rp) || rp.d0 != 0x0b);

	if (memcmp(&cp, &rp.cp, sizeof(cp)))
		errx(EXIT_FAILURE, "param access set failed");

	/*
	 * now send a soft reset to make those parameters used
	 */
	uart_send_cmd(fd, HCI_CMD_RESET, NULL, 0);
	/* assume it succeeded? */

	/* bluez also says there will be a confirmation packet
	 * on the new baud rate - ignore that for now
	 */
}
