/*	$NetBSD: btattach.c,v 1.2.6.1 2009/05/13 19:20:19 jym Exp $	*/

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

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2008 Iain Hibbert.  All rights reserved.");
__RCSID("$NetBSD: btattach.c,v 1.2.6.1 2009/05/13 19:20:19 jym Exp $");

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/uio.h>

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <util.h>

#include "btattach.h"

static void sighandler(int);
static void usage(void);

static int sigcount = 0;	/* signals received */
static int opt_debug = 0;	/* global? */

const struct devtype types[] = {
    {
	.name = "bcsp",
	.line = "bcsp",
	.descr = "Generic BlueCore Serial Protocol",
	.cflag = CRTSCTS,
	.speed = B57600,
    },
    {
	.name = "bcm2035",
	.line = "btuart",
	.descr = "Broadcom BCM2035",
	.init = &init_bcm2035,
	.speed = B115200,
    },
    {
	.name = "bgb2xx",
	.line = "btuart",
	.descr = "Philips BGB2xx module",
	.init = &init_bgb2xx,
	.cflag = CRTSCTS,
	.speed = B115200,
    },
    {
	.name = "btuart",
	.line = "btuart",
	.descr = "Generic UART (the default)",
    },
    {
	.name = "csr",
	.line = "btuart",
	.descr = "CSR Casira serial adaptor",
	.init = &init_csr,
	.cflag = CRTSCTS,
	.speed = B57600,
    },
    {
	.name = "digi",
	.line = "btuart",
	.descr = "Digianswer based cards",
	.init = &init_digi,
	.cflag = CRTSCTS,
	.speed = B9600,
    },
    {
	.name = "ericsson",
	.line = "btuart",
	.descr = "Ericsson based modules",
	.init = &init_ericsson,
	.cflag = CRTSCTS,
	.speed = B57600,
    },
    {
	.name = "st",
	.line = "btuart",
	.descr = "ST Microelectronics minikits based on STLC2410/STLC2415",
	.init = &init_st,
	.cflag = CRTSCTS,
	.speed = B57600,
    },
    {
	.name = "stlc2500",
	.descr = "ST Microelectronics minikits based on STLC2500",
	.init = &init_stlc2500,
	.cflag = CRTSCTS,
	.speed = B115200,
    },
    {
	.name = "swave",
	.line = "btuart",
	.descr = "Silicon Wave kits",
	.init = &init_swave,
	.cflag = CRTSCTS,
	.speed = B57600,
    },
    {
	.name = "texas",
	.line = "btuart",
	.descr = "Texas Instruments",
	.cflag = CRTSCTS,
	.speed = B115200,
    },
};

int
main(int argc, char *argv[])
{
	const struct devtype *type;
	struct termios tio;
	unsigned int init_speed, speed;
	tcflag_t cflag;
	int fd, ch, i;
	const char *name;
	char *ptr;

	init_speed = 0;
	cflag = CLOCAL;
	name = "btuart";

	while ((ch = getopt(argc, argv, "dfi:op")) != -1) {
		switch (ch) {
		case 'd':
			opt_debug++;
			break;

		case 'f':
			cflag |= CRTSCTS;
			break;

		case 'i':
			init_speed = strtoul(optarg, &ptr, 10);
			if (ptr[0] != '\0')
				errx(EXIT_FAILURE, "invalid speed: %s", optarg);

			break;

		case 'o':
			cflag |= (PARENB | PARODD);
			break;

		case 'p':
			cflag |= PARENB;
			break;

		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 3) {
		name = argv[0];
		argv++;
		argc--;
	}

	for (i = 0; ; i++) {
		if (i == __arraycount(types))
			errx(EXIT_FAILURE, "unknown type: %s", name);

		type = &types[i];
		if (strcasecmp(type->name, name) == 0)
			break;
	}

	if (argc != 2)
		usage();

	/* parse tty speed */
	speed = strtoul(argv[1], &ptr, 10);
	if (ptr[0] != '\0')
		errx(EXIT_FAILURE, "invalid speed: %s", argv[1]);

	if (init_speed == 0)
		init_speed = (type->speed ? type->speed : speed);

	/* open tty */
	if ((fd = open(argv[0], O_RDWR | O_NDELAY | O_EXLOCK, 0)) < 0)
		err(EXIT_FAILURE, "%s", argv[0]);

	/* setup tty */
	if (tcgetattr(fd, &tio) < 0)
		err(EXIT_FAILURE, "tcgetattr");

	cfmakeraw(&tio);
	tio.c_cflag |= (cflag | type->cflag);

	if (cfsetspeed(&tio, init_speed) < 0
	    || tcsetattr(fd, TCSANOW, &tio) < 0
	    || tcflush(fd, TCIOFLUSH) < 0)
		err(EXIT_FAILURE, "tty setup failed");

	/* initialize device */
	if (type->init != NULL)
		(*type->init)(fd, speed);

	if (cfsetspeed(&tio, speed) < 0
	    || tcsetattr(fd, TCSADRAIN, &tio) < 0)
		err(EXIT_FAILURE, "tty setup failed");

	/* start line discipline */
	if (ioctl(fd, TIOCSLINED, type->line) < 0)
		err(EXIT_FAILURE, "%s", type->line);

	if (opt_debug == 0 && daemon(0, 0) < 0)
		warn("detach failed!");

	/* store PID in "/var/run/btattach-{tty}.pid" */
	ptr = strrchr(argv[0], '/');
	asprintf(&ptr, "%s-%s", getprogname(), (ptr ? ptr + 1 : argv[0]));
	if (ptr == NULL || pidfile(ptr) < 0)
		warn("no pidfile");

	free(ptr);

	(void)signal(SIGHUP, sighandler);
	(void)signal(SIGINT, sighandler);
	(void)signal(SIGTERM, sighandler);
	(void)signal(SIGTSTP, sighandler);
	(void)signal(SIGUSR1, sighandler);
	(void)signal(SIGUSR2, sighandler);

	while (sigcount == 0)
		select(0, NULL, NULL, NULL, NULL);

	return EXIT_SUCCESS;
}

static void
usage(void)
{
	size_t i;

	fprintf(stderr,
		"Usage: %s [-dfop] [-i speed] [type] tty speed\n"
		"\n"
		"Where:\n"
		"\t-d          debug mode (no detach, dump io)\n"
		"\t-f          enable flow control\n"
		"\t-i speed    init speed\n"
		"\t-o          odd parity\n"
		"\t-p          even parity\n"
		"\n"
		"Known types:\n"
		"", getprogname());

	for (i = 0; i < __arraycount(types); i++)
		fprintf(stderr, "\t%-12s%s\n", types[i].name, types[i].descr);

	exit(EXIT_FAILURE);
}

static void
sighandler(int s)
{

	sigcount++;
}

static void
hexdump(uint8_t *ptr, size_t len)
{
	
	while (len--)
		printf(" %2.2x", *ptr++);
}

/*
 * send HCI comamnd
 */
void
uart_send_cmd(int fd, uint16_t opcode, void *buf, size_t len)
{
	struct iovec iov[2];
	hci_cmd_hdr_t hdr;

	hdr.type = HCI_CMD_PKT;
	hdr.opcode = htole16(opcode);
	hdr.length = len;

	iov[0].iov_base = &hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = buf;
	iov[1].iov_len = len;

	if (opt_debug) {
		printf("<<");
		hexdump(iov[0].iov_base, iov[0].iov_len);
		hexdump(iov[1].iov_base, iov[1].iov_len);
		printf("\n");
		fflush(stdout);
	}

	if (writev(fd, iov, __arraycount(iov)) < 0)
		err(EXIT_FAILURE, "writev");

	tcdrain(fd);
}

/*
 * get next character
 * store in iovec and inc counter if it fits
 */
static uint8_t
uart_getc(int fd, struct iovec *iov, int ioc, size_t *count)
{
	uint8_t ch, *b;
	ssize_t n;
	size_t off;

	n = read(fd, &ch, sizeof(ch));
	if (n < 0)
		err(EXIT_FAILURE, "read");

	if (n == 0)
		errx(EXIT_FAILURE, "eof");

	if (opt_debug)
		printf(" %2.2x", ch);

	off = *count;
	while (ioc > 0) {
		if (iov->iov_len > off) {
			b = iov->iov_base;
			b[off] = ch;
			*count += 1;
			break;
		}

		off -= iov->iov_len;
		iov++;
		ioc--;
	}

	return ch;
}

/*
 * read next packet, storing into iovec
 */
static size_t
uart_recv_pkt(int fd, struct iovec *iov, int ioc)
{
	size_t count, want;
	uint8_t type;

	if (opt_debug)
		printf(">>");

	count = 0;
	type = uart_getc(fd, iov, ioc, &count);
	switch(type) {
	case HCI_EVENT_PKT:
		(void)uart_getc(fd, iov, ioc, &count);	/* event */
		want = sizeof(hci_event_hdr_t);
		want += uart_getc(fd, iov, ioc, &count);
		break;

	case HCI_ACL_DATA_PKT:
		(void)uart_getc(fd, iov, ioc, &count);	/* handle LSB */
		(void)uart_getc(fd, iov, ioc, &count);	/* handle MSB */
		want = sizeof(hci_acldata_hdr_t);
		want += uart_getc(fd, iov, ioc, &count);	/* LSB */
		want += uart_getc(fd, iov, ioc, &count) << 8;	/* MSB */
		break;

	case HCI_SCO_DATA_PKT:
		(void)uart_getc(fd, iov, ioc, &count);	/* handle LSB */
		(void)uart_getc(fd, iov, ioc, &count);	/* handle MSB */
		want = sizeof(hci_scodata_hdr_t);
		want += uart_getc(fd, iov, ioc, &count);
		break;

	default: /* out of sync? */
		err(EXIT_FAILURE, "unknown packet type 0x%2.2x", type);
	}

	while (want-- > 0)
		(void)uart_getc(fd, iov, ioc, &count);

	if (opt_debug)
		printf("\n");

	return count;
}

/*
 * read next matching event packet to buffer
 */
size_t
uart_recv_ev(int fd, uint8_t event, void *buf, size_t len)
{
	struct iovec iov[2];
	hci_event_hdr_t hdr;
	size_t n;

	iov[0].iov_base = &hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = buf;
	iov[1].iov_len = len;

	for (;;) {
		n = uart_recv_pkt(fd, iov, __arraycount(iov));
		if (n < sizeof(hdr)
		    || hdr.type != HCI_EVENT_PKT
		    || hdr.event != event)
			continue;

		n -= sizeof(hdr);
		break;
	}

	return n;
}

/*
 * read next matching command_complete event to buffer
 */
size_t
uart_recv_cc(int fd, uint16_t opcode, void *buf, size_t len)
{
	struct iovec iov[3];
	hci_event_hdr_t hdr;
	hci_command_compl_ep cc;
	size_t n;

	iov[0].iov_base = &hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = &cc;
	iov[1].iov_len = sizeof(cc);
	iov[2].iov_base = buf;
	iov[2].iov_len = len;

	for (;;) {
		n = uart_recv_pkt(fd, iov, __arraycount(iov));
		if (n < sizeof(hdr)
		    || hdr.type != HCI_EVENT_PKT
		    || hdr.event != HCI_EVENT_COMMAND_COMPL)
			continue;

		n -= sizeof(hdr);
		if (n < sizeof(cc)
		    || cc.opcode != htole16(opcode))
			continue;

		n -= sizeof(cc);
		break;
	}

	return n;
}
