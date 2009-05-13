/*	$NetBSD: init_ericsson.c,v 1.1.10.1 2009/05/13 19:20:19 jym Exp $	*/

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
__RCSID("$NetBSD: init_ericsson.c,v 1.1.10.1 2009/05/13 19:20:19 jym Exp $");

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>

#include "btattach.h"

void
init_ericsson(int fd, unsigned int speed)
{
	uint8_t rate;

	switch(speed) {
	case B460800:	rate = 0x00;	break;
	case B230400:	rate = 0x01;	break;
	case B115200:	rate = 0x02;	break;
	case B57600:	rate = 0x03;	break;
	case B28800:	rate = 0x04;	break;
	case B14400:	rate = 0x05;	break;
	case B7200:	rate = 0x06;	break;
#if defined(B3600)
	case B3600:	rate = 0x07;	break;
#endif
	case B1800:	rate = 0x08;	break;
#if defined(B900)
	case B900:	rate = 0x09;	break;
#endif
#if defined(B153600)
	case B153600:	rate = 0x10;	break;
#endif
	case B76800:	rate = 0x11;	break;
	case B38400:	rate = 0x12;	break;
	case B19200:	rate = 0x13;	break;
	case B9600:	rate = 0x14;	break;
	case B4800:	rate = 0x15;	break;
	case B2400:	rate = 0x16;	break;
	case B1200:	rate = 0x17;	break;
	case B600:	rate = 0x18;	break;
	case B300:	rate = 0x19;	break;
	case B921600:	rate = 0x20;	break;
	case 200000:	rate = 0x25;	break;
	case 300000:	rate = 0x27;	break;
	case 400000:	rate = 0x2b;	break;
	default:
		errx(EXIT_FAILURE, "invalid speed for ericsson: %u\n", speed);
	}

	uart_send_cmd(fd, HCI_CMD_ERICSSON_SET_UART_BAUD_RATE, &rate, sizeof(rate));
	/* wait for response? */
	/* assume it succeeded? */
}
