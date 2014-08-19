/*	$NetBSD: rfcomm_sppd.c,v 1.15.8.1 2014/08/20 00:05:03 tls Exp $	*/

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
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2009 The NetBSD Foundation, Inc.\
  Copyright (c) 2007 Iain Hibbert.\
  Copyright (c) 2006 Itronix, Inc.\
  Copyright (c) 2003 Maksim Yevmenkin m_evmenkin@yahoo.com.\
  All rights reserved.");
__RCSID("$NetBSD: rfcomm_sppd.c,v 1.15.8.1 2014/08/20 00:05:03 tls Exp $");

#include <sys/param.h>

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
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include <netbt/rfcomm.h>

static int open_tty(const char *);
static int open_client(bdaddr_t *, bdaddr_t *, int, uintmax_t, const char *);
static int open_server(bdaddr_t *, uint16_t, uint8_t, int, const char *);
static void copy_data(int, int);
static int service_search(const bdaddr_t *, const bdaddr_t *, uint16_t,
    uintmax_t *, uintmax_t *);
static void sighandler(int);
static void usage(void) __attribute__((__noreturn__));
static void reset_tio(void);

static sig_atomic_t done;	/* got a signal */
static struct termios tio;	/* stored termios for reset on exit */

static const struct service {
	const char *	name;
	const char *	description;
	uint16_t	class;
} services[] = {
	{ "DUN",	"Dialup Networking",
	    SDP_SERVICE_CLASS_DIALUP_NETWORKING		},
	{ "LAN",	"LAN access using PPP",
	    SDP_SERVICE_CLASS_LAN_ACCESS_USING_PPP	},
	{ "SP",		"Serial Port",
	    SDP_SERVICE_CLASS_SERIAL_PORT		},
	{ NULL,		NULL,		0		}
};

int
main(int argc, char *argv[])
{
	struct termios		t;
	bdaddr_t		laddr, raddr;
	struct pollfd		pfd[2];
	const char		*service;
	char			*ep, *tty;
	int			n, lm, rfcomm, tty_in, tty_out;
	uint16_t		psm;
	uint8_t			channel;

	setprogname(argv[0]);
	bdaddr_copy(&laddr, BDADDR_ANY);
	bdaddr_copy(&raddr, BDADDR_ANY);
	service = "SP";
	tty = NULL;
	channel = RFCOMM_CHANNEL_ANY;
	psm = L2CAP_PSM_RFCOMM;
	lm = 0;

	/* Parse command line options */
	while ((n = getopt(argc, argv, "a:c:d:hm:p:s:t:")) != -1) {
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
			if (*ep != '\0'
			    || channel < RFCOMM_CHANNEL_MIN
			    || channel > RFCOMM_CHANNEL_MAX)
				errx(EXIT_FAILURE, "Invalid channel: %s",
				    optarg);

			break;

		case 'd': /* local device address */
			if (!bt_devaddr(optarg, &laddr))
				err(EXIT_FAILURE, "%s", optarg);

			break;

		case 'm': /* Link Mode */
			if (strcasecmp(optarg, "auth") == 0)
				lm = RFCOMM_LM_AUTH;
			else if (strcasecmp(optarg, "encrypt") == 0)
				lm = RFCOMM_LM_ENCRYPT;
			else if (strcasecmp(optarg, "secure") == 0)
				lm = RFCOMM_LM_SECURE;
			else
				errx(EXIT_FAILURE, "Unknown mode: %s", optarg);

			break;

		case 'p': /* PSM */
			psm = strtoul(optarg, &ep, 0);
			if (*ep != '\0' || L2CAP_PSM_INVALID(psm))
				errx(EXIT_FAILURE, "Invalid PSM: %s", optarg);

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
	 *	cannot have remote address if channel was given
	 */
	if (channel != RFCOMM_CHANNEL_ANY && !bdaddr_any(&raddr))
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
	if (!bdaddr_any(&raddr))
		rfcomm = open_client(&laddr, &raddr, lm, psm, service);
	else
		rfcomm = open_server(&laddr, psm, channel, lm, service);

	/*
	 * now we are ready to go, so either detach or maybe turn
	 * off some input processing, so that rfcomm_sppd can
	 * be used directly with stdio
	 */
	if (tty == NULL) {
		if (tcgetattr(tty_in, &t) != -1) {
			tio = t;
			t.c_lflag &= ~(ECHO | ICANON);
			t.c_iflag &= ~(ICRNL);

			if (tio.c_lflag != t.c_lflag ||
			    tio.c_iflag != t.c_iflag) {
				if (tcsetattr(tty_in, TCSANOW, &t) == -1)
					err(EXIT_FAILURE, "tcsetattr");

				atexit(reset_tio);
			}
		}
	} else {
		if (daemon(0, 0) == -1)
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

	pfd[0].fd = tty_in;
	pfd[1].fd = rfcomm;
	pfd[0].events = POLLIN|POLLRDNORM;
	pfd[1].events = POLLIN|POLLRDNORM;

	while (!done) {
		if (poll(pfd, 2, INFTIM) == -1) {
			if (errno == EINTR)
				continue;

			syslog(LOG_ERR, "poll error: %m");
		}
		if (pfd[0].revents & (POLLIN|POLLRDNORM))
			copy_data(tty_in, rfcomm);

		if (pfd[1].revents & (POLLIN|POLLRDNORM))
			copy_data(rfcomm, tty_out);
	}

	syslog(LOG_INFO, "Completed on %s", (tty ? tty : "stdio"));
	return EXIT_SUCCESS;
}

static int
open_tty(const char *tty)
{
	char		 pty[PATH_MAX], *slash;
	struct group	*gr = NULL;
	gid_t		 ttygid;
	int		 master;

	/*
	 * Construct master PTY name. The slave tty name must be less than
	 * PATH_MAX characters in length, must contain '/' character and
	 * must not end with '/'.
	 */
	if (strlcpy(pty, tty, sizeof(pty)) >= sizeof(pty))
		errx(EXIT_FAILURE, "Tty name too long `%s'", tty);

	slash = strrchr(pty, '/');
	if (slash == NULL || slash[1] == '\0')
		errx(EXIT_FAILURE, "Invalid tty `%s'", tty);

	slash[1] = 'p';
	if (strcmp(pty, tty) == 0)
		errx(EXIT_FAILURE, "Master and slave tty are the same (%s)",
		    tty);

	if ((master = open(pty, O_RDWR)) == -1)
		err(EXIT_FAILURE, "Cannot open `%s'", pty);

	/*
	 * Slave TTY
	 */
	if ((gr = getgrnam("tty")) != NULL)
		ttygid = gr->gr_gid;
	else
		ttygid = (gid_t)-1;

	if (chown(tty, getuid(), ttygid) == -1)
		err(EXIT_FAILURE, "Cannot chown `%s'", pty);
	if (chmod(tty, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
		err(EXIT_FAILURE, "Cannot chmod `%s'", pty);
	if (revoke(tty) == -1)
		err(EXIT_FAILURE, "Cannot revoke `%s'", pty);

	return master;
}

static int
open_client(bdaddr_t *laddr, bdaddr_t *raddr, int lm, uintmax_t psm,
    const char *service)
{
	struct sockaddr_bt sa;
	const struct service *s;
	struct linger l;
	char *ep;
	int fd;
	uintmax_t channel;

	for (s = services ; ; s++) {
		if (s->name == NULL) {
			errno = 0;
			channel = strtoul(service, &ep, 10);
			if (service == ep || *ep != '\0')
				errx(EXIT_FAILURE, "Unknown service `%s'",
				    service);
			if (channel == ULONG_MAX && errno == ERANGE)
				err(EXIT_FAILURE, "Service `%s'",
				    service);

			break;
		}

		if (strcasecmp(s->name, service) == 0) {
			if (service_search(laddr, raddr, s->class, &psm,
			    &channel) == -1)
				err(EXIT_FAILURE, "%s", s->name);

			break;
		}
	}

	if (channel < RFCOMM_CHANNEL_MIN || channel > RFCOMM_CHANNEL_MAX)
		errx(EXIT_FAILURE, "Invalid channel %"PRIuMAX, channel);

	if (L2CAP_PSM_INVALID(psm))
		errx(EXIT_FAILURE, "Invalid PSM 0x%04"PRIxMAX, psm);

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sa.bt_bdaddr, laddr);

	fd = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (fd == -1)
		err(EXIT_FAILURE, "socket()");

	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		err(EXIT_FAILURE, "bind(%s)", bt_ntoa(laddr, NULL));

	memset(&l, 0, sizeof(l));
	l.l_onoff = 1;
	l.l_linger = 5;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) == -1)
		err(EXIT_FAILURE, "linger()");

	if (setsockopt(fd, BTPROTO_RFCOMM, SO_RFCOMM_LM, &lm, sizeof(lm)) == -1)
		err(EXIT_FAILURE, "link mode");

	sa.bt_psm = psm;
	sa.bt_channel = channel;
	bdaddr_copy(&sa.bt_bdaddr, raddr);

	if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		err(EXIT_FAILURE, "connect(%s, 0x%04"PRIxMAX", %"PRIuMAX")",
		    bt_ntoa(raddr, NULL), psm, channel);

	return fd;
}

static int
open_server(bdaddr_t *laddr, uint16_t psm, uint8_t channel, int lm,
    const char *service)
{
	uint8_t	buffer[256];
	struct sockaddr_bt sa;
	const struct service *s;
	struct linger l;
	socklen_t len;
	sdp_session_t ss;
	sdp_data_t rec;
	int sv, fd;

	for (s = services; ; s++) {
		if (s->name == NULL)
			usage();

		if (strcasecmp(s->name, service) == 0)
			break;
	}

	/* Open server socket */
	sv = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (sv == -1)
		err(EXIT_FAILURE, "socket()");

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	sa.bt_psm = psm;
	sa.bt_channel = channel;
	bdaddr_copy(&sa.bt_bdaddr, laddr);
	if (bind(sv, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		err(EXIT_FAILURE, "bind(%s, 0x%04x, %d)",
		    bt_ntoa(laddr, NULL), psm, channel);

	if (setsockopt(sv, BTPROTO_RFCOMM, SO_RFCOMM_LM, &lm, sizeof(lm)) == -1)
		err(EXIT_FAILURE, "link mode");

	if (listen(sv, 1) == -1)
		err(EXIT_FAILURE, "listen()");

	len = sizeof(sa);
	if (getsockname(sv, (struct sockaddr *)&sa, &len) == -1)
		err(EXIT_FAILURE, "getsockname()");
	if (len != sizeof(sa))
		errx(EXIT_FAILURE, "getsockname()");

	/* Build SDP record */
	rec.next = buffer;
	rec.end = buffer + sizeof(buffer);

	sdp_put_uint16(&rec, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(&rec, 0x00000000);

	sdp_put_uint16(&rec, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(&rec, 3);
	sdp_put_uuid16(&rec, s->class);

	len = (psm == L2CAP_PSM_RFCOMM ? 0 : 3);

	sdp_put_uint16(&rec, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(&rec, 12 + len);
	sdp_put_seq(&rec, 3 + len);
	sdp_put_uuid16(&rec, SDP_UUID_PROTOCOL_L2CAP);
	if (len > 0)
		sdp_put_uint16(&rec, psm);
	sdp_put_seq(&rec, 5);
	sdp_put_uuid16(&rec, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(&rec, sa.bt_channel);

	sdp_put_uint16(&rec, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(&rec, 3);
	sdp_put_uuid16(&rec, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(&rec, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(&rec, 9);
	sdp_put_uint16(&rec, 0x656e);	/* "en" */
	sdp_put_uint16(&rec, 106);	/* UTF-8 */
	sdp_put_uint16(&rec, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	if (s->class == SDP_SERVICE_CLASS_LAN_ACCESS_USING_PPP) {
		sdp_put_uint16(&rec, SDP_ATTR_SERVICE_AVAILABILITY);
		sdp_put_uint8(&rec, 0x00);
	}

	sdp_put_uint16(&rec, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(&rec, 8);
	sdp_put_seq(&rec, 6);
	sdp_put_uuid16(&rec, s->class);
	sdp_put_uint16(&rec, 0x0100);	/* v1.0 */

	sdp_put_uint16(&rec, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID
	    + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(&rec, s->description, -1);

	if (s->class == SDP_SERVICE_CLASS_DIALUP_NETWORKING) {
		sdp_put_uint16(&rec, SDP_ATTR_AUDIO_FEEDBACK_SUPPORT);
		sdp_put_bool(&rec, false);
	}

#if 0
	if (s->class == SDP_SERVICE_CLASS_LAN_ACCESS_USING_PPP) {
		sdp_put_uint16(&rec, SDP_ATTR_IP_SUBNET);	/* TODO */
		sdp_put_str(&rec, "0.0.0.0/0", -1);
	}
#endif

	rec.end = rec.next;
	rec.next = buffer;

	/* Register service with SDP server */
	ss = sdp_open_local(NULL);
	if (ss == NULL)
		err(EXIT_FAILURE, "sdp_open_local");

	if (!sdp_record_insert(ss, laddr, NULL, &rec))
		err(EXIT_FAILURE, "sdp_record_insert");

	/* Accept client connection */
	len = sizeof(sa);
	fd = accept(sv, (struct sockaddr *)&sa, &len);
	if (fd == -1)
		err(EXIT_FAILURE, "accept");

	memset(&l, 0, sizeof(l));
	l.l_onoff = 1;
	l.l_linger = 5;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) == -1)
		err(EXIT_FAILURE, "linger()");

	close(sv);
	return fd;
}

static void
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

static int
service_search(bdaddr_t const *laddr, bdaddr_t const *raddr,
    uint16_t class, uintmax_t *psm, uintmax_t *channel)
{
	uint8_t		buffer[6];	/* SSP (3 bytes) + AIL (3 bytes) */
	sdp_session_t	ss;
	sdp_data_t	ail, ssp, rsp, rec, value, pdl, seq;
	uint16_t	attr;
	bool		rv;

	seq.next = buffer;
	seq.end = buffer + sizeof(buffer);

	/*
	 * build ServiceSearchPattern (3 bytes)
	 */
	ssp.next = seq.next;
	sdp_put_uuid16(&seq, class);
	ssp.end = seq.next;

	/*
	 * build AttributeIDList (3 bytes)
	 */
	ail.next = seq.next;
	sdp_put_uint16(&seq, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	ail.end = seq.next;

	ss = sdp_open(laddr, raddr);
	if (ss == NULL)
		return -1;

	rv = sdp_service_search_attribute(ss, &ssp, &ail, &rsp);
	if (!rv) {
		sdp_close(ss);
		return -1;
	}

	/*
	 * The response will be a list of records that matched our
	 * ServiceSearchPattern, where each record is a sequence
	 * containing a single ProtocolDescriptorList attribute and
	 * value
	 *
	 *	seq
	 *	  uint16	ProtocolDescriptorList
	 *	  value
	 *	seq
	 *	  uint16	ProtocolDescriptorList
	 *	  value
	 *
	 * If the ProtocolDescriptorList describes a single stack,
	 * the attribute value takes the form of a single Data Element
	 * Sequence where each member is a protocol descriptor.
	 *
	 *	seq
	 *	  list
	 *
	 * If it is possible for more than one kind of protocol
	 * stack to be used to gain access to the service, the
	 * ProtocolDescriptorList takes the form of a Data Element
	 * Alternative where each member is a Data Element Sequence
	 * describing an alternative protocol stack.
	 *
	 *	alt
	 *	  seq
	 *	    list
	 *	  seq
	 *	    list
	 *
	 * Each protocol stack description contains a sequence for each
	 * protocol, where each sequence contains the protocol UUID as
	 * the first element, and any ProtocolSpecificParameters. We are
	 * interested in the L2CAP psm if provided, and the RFCOMM channel
	 * number, stored as parameter#1 in each case.
	 *
	 *	seq
	 *	  uuid		L2CAP
	 *	  uint16	psm
	 *	seq
	 *	  uuid		RFCOMM
	 *	  uint8		channel
	 */

	rv = false;
	while (!rv && sdp_get_seq(&rsp, &rec)) {
		if (!sdp_get_attr(&rec, &attr, &value)
		    || attr != SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST)
			continue;

		sdp_get_alt(&value, &value);	/* strip any alt container */
		while (!rv && sdp_get_seq(&value, &pdl)) {
			*psm = L2CAP_PSM_RFCOMM;
			if (sdp_get_seq(&pdl, &seq)
			    && sdp_match_uuid16(&seq, SDP_UUID_PROTOCOL_L2CAP)
			    && (sdp_get_uint(&seq, psm) || true)
			    && sdp_get_seq(&pdl, &seq)
			    && sdp_match_uuid16(&seq, SDP_UUID_PROTOCOL_RFCOMM)
			    && sdp_get_uint(&seq, channel))
				rv = true;
		}
	}

	sdp_close(ss);
	if (rv)
		return 0;
	errno = ENOATTR;
	return -1;
}

static void
sighandler(int s)
{

	done++;
}

static void
reset_tio(void)
{

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio);
}

static void
usage(void)
{
	const char *cmd = getprogname();
	const struct service *s;

	fprintf(stderr, "Usage: %s [-d device] [-m mode] [-p psm] [-s service]"
	    " [-t tty]\n"
	    "       %*s {-a bdaddr | [-c channel]}\n"
	    "\n"
	    "Where:\n"
	    "\t-a bdaddr    remote device address\n"
	    "\t-c channel   local RFCOMM channel\n"
	    "\t-d device    local device address\n"
	    "\t-m mode      link mode\n"
	    "\t-p psm       protocol/service multiplexer\n"
	    "\t-s service   service class\n"
	    "\t-t tty       run in background using pty\n"
	    "\n", cmd, (int)strlen(cmd), "");

	fprintf(stderr, "Known service classes:\n");
	for (s = services ; s->name != NULL ; s++)
		fprintf(stderr, "\t%-13s%s\n", s->name, s->description);

	exit(EXIT_FAILURE);
}
