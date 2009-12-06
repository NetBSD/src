/*	$NetBSD: init_unistone.c,v 1.1 2009/12/06 12:55:46 kiyohara Exp $	*/
/*
 * Copyright (c) 2009 KIYOHARA Takashi
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
/*
 * init information in this file gleaned from hciattach(8)
 * command from Gumstix's patch for BlueZ.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: init_unistone.c,v 1.1 2009/12/06 12:55:46 kiyohara Exp $");

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>

#include <unistd.h>
#include <sys/ioctl.h>

#include "btattach.h"


#define HCI_CMD_INFINEON_SET_UART_BAUDRATE	\
        HCI_OPCODE(HCI_OGF_VENDOR, 0x006)


static int infineon_manufacturer_mode(int, int);

static int
infineon_manufacturer_mode(int fd, int enable)
{
	hci_status_rp rp;
	uint8_t cmd[2];
	int n;

	cmd[0] = enable;
	cmd[1] = 0;	/* No reset */

	uart_send_cmd(fd, 0xfc11, cmd, sizeof(cmd));
	n = uart_recv_cc(fd, 0xfc11, &rp, sizeof(rp));
	if (n != sizeof(rp) || rp.status != 0x00)
		errx(EXIT_FAILURE, "Manufacturer mode %s failed",
		    enable ? "enable" : "disable");

	return 0;
}

void
init_unistone(int fd, unsigned int speed)
{
	hci_command_status_ep cs;
	struct termios tio;
	uint8_t rate, v[2];
	int n;

	switch(speed) {
	case B9600:	rate = 0x00;	break;
	case B19200:	rate = 0x01;	break;
	case B38400:	rate = 0x02;	break;
	case B57600:	rate = 0x03;	break;
	case B115200:	rate = 0x04;	break;
	case B230400:	rate = 0x05;	break;
	case B460800:	rate = 0x06;	break;
	case B921600:	rate = 0x07;	break;
#if 0
	case B1843200:	rate = 0x08;	break;
#endif
	default:
		errx(EXIT_FAILURE, "invalid speed for infineon unistone: %u\n",
		    speed);
	}

	if (tcgetattr(fd, &tio) != 0)
		err(EXIT_FAILURE, "can't get baud rate");

	infineon_manufacturer_mode(fd, 1);

	uart_send_cmd(fd, HCI_CMD_INFINEON_SET_UART_BAUDRATE, &rate,
	    sizeof(rate));

	n = uart_recv_ev(fd, HCI_EVENT_COMMAND_STATUS, &cs, sizeof(cs));
	if (n != sizeof(cs) ||
	    cs.status != 0x00 ||
	    cs.opcode != HCI_CMD_INFINEON_SET_UART_BAUDRATE)
		errx(EXIT_FAILURE, "Set_UART_Baudrate failed\n");

	if (cfsetspeed(&tio, speed) != 0 ||
	    tcsetattr(fd, TCSANOW, &tio) != 0)
		err(EXIT_FAILURE, "can't change baud rate");

	n = uart_recv_ev(fd, HCI_EVENT_VENDOR, &v, sizeof(v));
	if (n != sizeof(v) ||
	    v[0] != 0x12 ||
	    v[1] != 0x00)
		errx(EXIT_FAILURE, "Set_UART_Baudrate not complete\n");

	infineon_manufacturer_mode(fd, 0);
}
