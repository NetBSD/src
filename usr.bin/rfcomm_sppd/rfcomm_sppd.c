/*	$NetBSD: rfcomm_sppd.c,v 1.6 2007/03/31 07:14:44 plunky Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
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
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * rfcomm_sppd.c
 *
 * Copyright (c) 2007 Iain Hibbert
 * Copyright (c) 2003 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2007 Iain Hibbert\n"
	    "@(#) Copyright (c) 2006 Itronix, Inc.\n"
	    "@(#) Copyright (c) 2003 Maksim Yevmenkin <m_evmenkin@yahoo.com>\n"
	    "All rights reserved.\n");
__RCSID("$NetBSD: rfcomm_sppd.c,v 1.6 2007/03/31 07:14:44 plunky Exp $");

#include <bluetooth.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <paths.h>
#include <sdp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include "rfcomm_sdp.h"

#define max(a, b)	((a) > (b) ? (a) : (b))

int open_tty(const char *);
int open_client(bdaddr_t *, bdaddr_t *, const char *);
int open_server(bdaddr_t *, uint8_t, const char *);
void copy_data(int, int);
void sighandler(int);
void usage(void);
void reset_tio(void);

int done;		/* got a signal */
struct termios tio;	/* stored termios for reset on exit */

struct service {
	const char	*name;
	const char	*description;
	uint16_t	class;
	int		pdulen;
} services[] = {
	{ "DUN",	"Dialup Networking",
	  SDP_SERVICE_CLASS_DIALUP_NETWORKING,
	  sizeof(struct sdp_dun_profile)
	},
	{ "LAN",	"Lan access using PPP",
	  SDP_SERVICE_CLASS_LAN_ACCESS_USING_PPP,
	  sizeof(struct sdp_lan_profile)
	},
	{ "SP",		"Serial Port",
	  SDP_SERVICE_CLASS_SERIAL_PORT,
	  sizeof(struct sdp_sp_profile)
	},
	{ NULL,		NULL,
	  0,
	  0
	}
};

int
main(int argc, char *argv[])
{
	struct termios		t;
	bdaddr_t		laddr, raddr;
	fd_set			rdset;
	const char		*service;
	char			*ep, *tty;
	int			n, rfcomm, tty_in, tty_out;
	uint8_t			channel;

	bdaddr_copy(&laddr, BDADDR_ANY);
	bdaddr_copy(&raddr, BDADDR_ANY);
	service = "SP";
	tty = NULL;
	channel = 0;

	/* Parse command line options */
	while ((n = getopt(argc, argv, "a:c:d:hs:t:")) != -1) {
		switch (n) {
		case 'a': /* remote device address */
			if (!bt_aton(optarg, &raddr)) {
				struct hostent	*he = NULL;

				if ((he = bt_gethostbyname(optarg)) == NULL)
					errx(EXIT_FAILURE, "%s: %s", optarg,
					    hstrerror(h_errno));

				bdaddr_copy(&raddr, (bdaddr_t *)he->h_addr);
			}
			break;

		case 'c': /* RFCOMM channel */
			channel = strtoul(optarg, &ep, 10);
			if (*ep != '\0' || channel < 1 || channel > 30)
				errx(EXIT_FAILURE, "Invalid channel: %s", optarg);

			break;

		case 'd': /* local device address */
			if (!bt_devaddr(optarg, &laddr))
				err(EXIT_FAILURE, "%s", optarg);

			break;

		case 's': /* service class */
			service = optarg;
			break;

		case 't': /* Slave TTY name */
			if (optarg[0] != '/')
				asprintf(&tty, "%s%s", _PATH_DEV, optarg);
			else
				tty = optarg;

			break;

		case 'h':
		default:
			usage();
			/* NOT REACHED */
		}
	}

	/*
	 * validate options:
	 *	must have channel or remote address but not both
	 */
	if ((channel == 0 && bdaddr_any(&raddr))
	    || (channel != 0 && !bdaddr_any(&raddr)))
		usage();

	/*
	 * grab ttys before we start the bluetooth
	 */
	if (tty == NULL) {
		tty_in = STDIN_FILENO;
		tty_out = STDOUT_FILENO;
	} else {
		tty_in = open_tty(tty);
		tty_out = tty_in;
	}

	/* open RFCOMM */
	if (channel == 0)
		rfcomm = open_client(&laddr, &raddr, service);
	else
		rfcomm = open_server(&laddr, channel, service);

	/*
	 * now we are ready to go, so either detach or maybe turn
	 * off some input processing, so that rfcomm_sppd can
	 * be used directly with stdio
	 */
	if (tty == NULL) {
		if (tcgetattr(tty_in, &t) < 0)
			err(EXIT_FAILURE, "tcgetattr");

		memcpy(&tio, &t, sizeof(tio));
		t.c_lflag &= ~(ECHO | ICANON);
		t.c_iflag &= ~(ICRNL);

		if (memcmp(&tio, &t, sizeof(tio))) {
			if (tcsetattr(tty_in, TCSANOW, &t) < 0)
				err(EXIT_FAILURE, "tcsetattr");

			atexit(reset_tio);
		}
	} else {
		if (daemon(0, 0) < 0)
			err(EXIT_FAILURE, "daemon() failed");
	}

	/* catch signals */
	done = 0;
	(void)signal(SIGHUP, sighandler);
	(void)signal(SIGINT, sighandler);
	(void)signal(SIGPIPE, sighandler);
	(void)signal(SIGTERM, sighandler);

	openlog(getprogname(), LOG_PERROR | LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "Starting on %s...", (tty ? tty : "stdio"));

	n = max(tty_in, rfcomm) + 1;
	while (!done) {
		FD_ZERO(&rdset);
		FD_SET(tty_in, &rdset);
		FD_SET(rfcomm, &rdset);

		if (select(n, &rdset, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue;

			syslog(LOG_ERR, "select error: %m");
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(tty_in, &rdset))
			copy_data(tty_in, rfcomm);

		if (FD_ISSET(rfcomm, &rdset))
			copy_data(rfcomm, tty_out);
	}

	syslog(LOG_INFO, "Completed on %s", (tty ? tty : "stdio"));
	exit(EXIT_SUCCESS);
}

int
open_tty(const char *tty)
{
	char		 pty[PATH_MAX], *slash;
	struct group	*gr = NULL;
	gid_t		 ttygid;
	int		 master;

	/*
	 * Construct master PTY name. The slave tty name must be less then
	 * PATH_MAX characters in length, must contain '/' character and
	 * must not end with '/'.
	 */
	if (strlen(tty) >= sizeof(pty))
		errx(EXIT_FAILURE, ": tty name too long");

	strlcpy(pty, tty, sizeof(pty));
	slash = strrchr(pty, '/');
	if (slash == NULL || slash[1] == '\0')
		errx(EXIT_FAILURE, "%s: invalid tty", tty);

	slash[1] = 'p';
	if (strcmp(pty, tty) == 0)
		errx(EXIT_FAILURE, "Master and slave tty are the same (%s)", tty);

	if ((master = open(pty, O_RDWR, 0)) < 0)
		err(EXIT_FAILURE, "%s", pty);

	/*
	 * Slave TTY
	 */

	if ((gr = getgrnam("tty")) != NULL)
		ttygid = gr->gr_gid;
	else
		ttygid = (gid_t)-1;

	(void)chown(tty, getuid(), ttygid);
	(void)chmod(tty, S_IRUSR | S_IWUSR | S_IWGRP);
	(void)revoke(tty);

	return master;
}

int
open_client(bdaddr_t *laddr, bdaddr_t *raddr, const char *service)
{
	struct sockaddr_bt sa;
	struct service *s;
	struct linger l;
	char *ep;
	int fd;
	uint8_t channel;

	for (s = services ; ; s++) {
		if (s->name == NULL) {
			channel = strtoul(service, &ep, 10);
			if (*ep != '\0' || channel < 1 || channel > 30)
				errx(EXIT_FAILURE, "Invalid service: %s", service);

			break;
		}

		if (strcasecmp(s->name, service) == 0) {
			if (rfcomm_channel_lookup(laddr, raddr, s->class, &channel, &errno) < 0)
				err(EXIT_FAILURE, "%s", s->name);

			break;
		}
	}

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sa.bt_bdaddr, laddr);

	fd = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (fd < 0)
		err(EXIT_FAILURE, "socket()");

	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		err(EXIT_FAILURE, "bind(%s)", bt_ntoa(laddr, NULL));

	memset(&l, 0, sizeof(l));
	l.l_onoff = 1;
	l.l_linger = 5;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0)
		err(EXIT_FAILURE, "linger()");

	sa.bt_channel = channel;
	bdaddr_copy(&sa.bt_bdaddr, raddr);

	if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		err(EXIT_FAILURE, "connect(%s, %d)", bt_ntoa(raddr, NULL),
						     channel);

	return fd;
}

/*
 * In all the profiles we currently support registering, the channel
 * is the first octet in the PDU, and it seems all the rest can be
 * zero, so we just use an array of uint8_t big enough to store the
 * largest, currently LAN. See <sdp.h> for definitions..
 */
#define pdu_len		sizeof(struct sdp_lan_profile)

int
open_server(bdaddr_t *laddr, uint8_t channel, const char *service)
{
	struct sockaddr_bt sa;
	struct linger l;
	socklen_t len;
	void *ss;
	int sv, fd, n;
	uint8_t pdu[pdu_len];

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sa.bt_bdaddr, laddr);
	sa.bt_channel = channel;

	sv = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (sv < 0)
		err(EXIT_FAILURE, "socket()");

	if (bind(sv, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		err(EXIT_FAILURE, "bind(%s, %d)", bt_ntoa(laddr, NULL),
						  channel);

	if (listen(sv, 1) < 0)
		err(EXIT_FAILURE, "listen()");

	/* Register service with SDP server */
	for (n = 0 ; ; n++) {
		if (services[n].name == NULL)
			usage();

		if (strcasecmp(services[n].name, service) == 0)
			break;
	}

	memset(pdu, 0, pdu_len);
	pdu[0] = channel;

	ss = sdp_open_local(NULL);
	if (ss == NULL || (errno = sdp_error(ss)) != 0)
		err(EXIT_FAILURE, "sdp_open_local");

	if (sdp_register_service(ss, services[n].class, laddr,
		    pdu, services[n].pdulen, NULL) != 0) {
		errno = sdp_error(ss);
		err(EXIT_FAILURE, "sdp_register_service");
	}

	len = sizeof(sa);
	fd = accept(sv, (struct sockaddr *)&sa, &len);
	if (fd < 0)
		err(EXIT_FAILURE, "accept");

	memset(&l, 0, sizeof(l));
	l.l_onoff = 1;
	l.l_linger = 5;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0)
		err(EXIT_FAILURE, "linger()");

	close(sv);
	return fd;
}

void
copy_data(int src, int dst)
{
	static char	buf[BUFSIZ];
	ssize_t		nr, nw, off;

	while ((nr = read(src, buf, sizeof(buf))) == -1) {
		if (errno != EINTR) {
			syslog(LOG_ERR, "read failed: %m");
			exit(EXIT_FAILURE);
		}
	}

	if (nr == 0)	/* reached EOF */
		done++;

	for (off = 0 ; nr ; nr -= nw, off += nw) {
		if ((nw = write(dst, buf + off, (size_t)nr)) == -1) {
			syslog(LOG_ERR, "write failed: %m");
			exit(EXIT_FAILURE);
		}
	}
}

void
sighandler(int s)
{

	done++;
}

void
reset_tio(void)
{

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio);
}

void
usage(void)
{
	struct service *s;

	fprintf(stderr, "Usage: %s  [-d device] [-s service] [-t tty] -a bdaddr | -c channel\n"
			"\n"
			"Where:\n"
			"\t-a bdaddr    remote device address\n"
			"\t-c channel   local RFCOMM channel\n"
			"\t-d device    local device address\n"
			"\t-s service   service class\n"
			"\t-t tty       run in background using pty\n"
			"\n", getprogname());

	fprintf(stderr, "Known service classes:\n");
	for (s = services ; s->name != NULL ; s++)
		fprintf(stderr, "\t%-13s%s\n", s->name, s->description);

	exit(EXIT_FAILURE);
}
