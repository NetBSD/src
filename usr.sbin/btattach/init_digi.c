/*	$NetBSD: init_digi.c,v 1.1.10.1 2009/05/13 19:20:19 jym Exp $	*/

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
__RCSID("$NetBSD: init_digi.c,v 1.1.10.1 2009/05/13 19:20:19 jym Exp $");

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>

#include "btattach.h"

#define HCI_CMD_DIGIANSWER_SET_UART_BAUD_RATE \
	HCI_OPCODE(HCI_OGF_VENDOR, 0x007)

void
init_digi(int fd, unsigned int speed)
{
	uint8_t rate;

	switch (speed) {
	case B57600:	rate = 0x08;	break;
	case B115200:	rate = 0x09;	break;
	default:
		errx(EXIT_FAILURE, "invalid speed for digi: %u\n", speed);
	}

	uart_send_cmd(fd, HCI_CMD_DIGIANSWER_SET_UART_BAUD_RATE, &rate, sizeof(rate));
	/* wait for response? */
	/* assume it succeeded? */
}
