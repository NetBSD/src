/*	$NetBSD: init_stlc2500.c,v 1.2 2009/12/06 12:31:07 kiyohara Exp $	*/

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
__RCSID("$NetBSD: init_stlc2500.c,v 1.2 2009/12/06 12:31:07 kiyohara Exp $");

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "btattach.h"

#define HCI_CMD_ERICSSON_READ_REVISION_INFO \
	HCI_OPCODE(HCI_OGF_VENDOR, 0x00f)

#define HCI_CMD_ST_STORE_IN_NVDS \
	HCI_OPCODE(HCI_OGF_VENDOR, 0x022)

typedef struct {
	uint8_t		d0;	/* ? */
	uint8_t		d1;	/* ? */
	bdaddr_t	bdaddr;
} __attribute__ ((__packed__)) hci_store_in_nvds_cp;

static const char *default_bdaddr = "00:80:e1:00:ab:ba";

#define HCI_CMD_ST_LOAD_FIRMWARE \
	HCI_OPCODE(HCI_OGF_VENDOR, 0x02e)

static int
firmload_stlc2500(int fd, uint16_t revision, const char *ext)
{
	uint8_t buf[0x100], seq;
	char *name;
	ssize_t n;
	int ff;

	/* we should look up hw.firmware.path and search for the file */

	asprintf(&name, "STLC2500_R%d_%02d_%s", (revision & 0xff), (revision >> 8), ext);
	ff = open(name, O_RDONLY, 0);
	if (ff < 0) {
		if (errno != ENOENT)
			err(EXIT_FAILURE, "%s", name);

		free(name);
		return -1;
	}

	for (seq = 0;; seq++) {
		n = read(ff, buf + 1, sizeof(buf) - 1);
		if (n < 0)
			err(EXIT_FAILURE, "%s", name);

		if (n == 0)
			break;

		buf[0] = seq;
		uart_send_cmd(fd, HCI_CMD_ST_LOAD_FIRMWARE, buf, (size_t)(n + 1));
		n = uart_recv_cc(fd, HCI_CMD_ST_LOAD_FIRMWARE, buf, 1);
		/* command complete ok? */

		if (n != 1 || buf[0] != seq)
			err(EXIT_FAILURE, "sequence mismatch");
	}

	close(ff);
	free(name);
	return 0;
}

void
init_stlc2500(int fd, unsigned int speed)
{
	hci_read_local_ver_rp rp;
	hci_store_in_nvds_cp cp;
	struct termios tio;
	int n;

	/* STLC2500 has an ericsson core */
	init_ericsson(fd, speed);

	if (tcgetattr(fd, &tio) != 0 ||
	    cfsetspeed(&tio, speed) != 0 ||
	    tcsetattr(fd, TCSANOW, &tio) != 0)
		err(EXIT_FAILURE, "can't change baud rate");

	uart_send_cmd(fd, HCI_CMD_READ_LOCAL_VER, NULL, 0);
	n = uart_recv_cc(fd, HCI_CMD_READ_LOCAL_VER, &rp, sizeof(rp));
	if (n != sizeof(rp) || rp.status != 0x00)
		errx(EXIT_FAILURE, "read local version command failed");

	if (firmload_stlc2500(fd, rp.hci_revision, "ptc") < 0)
		warn("no ROM patch file");
		
	if (firmload_stlc2500(fd, rp.hci_revision, "ssf") < 0)
		warn("no static settings file");

	cp.d0 = 0xfe;	/* ? */
	cp.d1 = 0x06;	/* ? */
	bt_aton(default_bdaddr, &cp.bdaddr);

	uart_send_cmd(fd, HCI_CMD_ST_STORE_IN_NVDS, &cp, sizeof(cp));
	uart_recv_cc(fd, HCI_CMD_ST_STORE_IN_NVDS, NULL, 0);
	/* command complete ok? */
	/* assume it succeeded? */

	uart_send_cmd(fd, HCI_CMD_RESET, NULL, 0);
	uart_recv_cc(fd, HCI_CMD_RESET, NULL, 0);
	/* assume it succeeded? */
}
