/*	$NetBSD: ftp.c,v 1.19 2010/11/26 18:58:43 christos Exp $	*/
/*	$KAME: ftp.c,v 1.23 2003/08/19 21:20:33 itojun Exp $	*/

/*
 * Copyright (C) 1997 and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <ctype.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "faithd.h"

static char rbuf[MSS];
static int passivemode = 0;
static int wport4 = -1;			/* listen() to active */
static int wport6 = -1;			/* listen() to passive */
static int port4 = -1;			/* active: inbound  passive: outbound */
static int port6 = -1;			/* active: outbound  passive: inbound */
static struct sockaddr_storage data4;	/* server data address */
static struct sockaddr_storage data6;	/* client data address */
static int epsvall = 0;

enum state { NONE, LPRT, EPRT, LPSV, EPSV };

static int ftp_activeconn(void);
static int ftp_passiveconn(void);
static ssize_t ftp_copy(int, int);
static ssize_t ftp_copyresult(int, int, enum state);
static ssize_t ftp_copycommand(int, int, enum state *);

void
ftp_relay(int ctl6, int ctl4)
{
	struct pollfd pfd[6];
	ssize_t error;
	enum state state = NONE;

	syslog(LOG_INFO, "starting ftp control connection");

	for (;;) {
		pfd[0].fd = ctl4;
		pfd[0].events = POLLIN;
		pfd[1].fd = ctl6;
		pfd[1].events = POLLIN;
		if (0 <= port4) {
			pfd[2].fd = port4;
			pfd[2].events = POLLIN;
		} else
			pfd[2].fd = -1;
		if (0 <= port6) {
			pfd[3].fd = port6;
			pfd[3].events = POLLIN;
		} else
			pfd[3].fd = -1;
#if 0
		if (0 <= wport4) {
			pfd[4].fd = wport4;
			pfd[4].events = POLLIN;
		} else
			pfd[4].fd = -1;
		if (0 <= wport6) {
			pfd[5].fd = wport4;
			pfd[5].events = POLLIN;
		} else
			pfd[5].fd = -1;
#else
		pfd[4].fd = pfd[5].fd = -1;
		pfd[4].events = pfd[5].events = 0;
#endif
		error = poll(pfd, (unsigned int)(sizeof(pfd) / sizeof(pfd[0])),
		    FAITH_TIMEOUT * 1000);
		if (error == -1) {
			exit_failure("poll: %s", strerror(errno));
		}
		else if (error == 0)
			exit_failure("connection timeout");

		/*
		 * The order of the following checks does (slightly) matter.
		 * It is important to visit all checks (do not use "continue"),
		 * otherwise some of the pipe may become full and we cannot
		 * relay correctly.
		 */
		if (pfd[1].revents & POLLIN)
		{
			/*
			 * copy control connection from the client.
			 * command translation is necessary.
			 */
			error = ftp_copycommand(ctl6, ctl4, &state);

			if (error < 0)
				goto bad;
			else if (error == 0) {
				(void)close(ctl4);
				(void)close(ctl6);
				exit_success("terminating ftp control connection");
				/*NOTREACHED*/
			}
		}
		if (pfd[0].revents & POLLIN)
		{
			/*
			 * copy control connection from the server
			 * translation of result code is necessary.
			 */
			error = ftp_copyresult(ctl4, ctl6, state);

			if (error < 0)
				goto bad;
			else if (error == 0) {
				(void)close(ctl4);
				(void)close(ctl6);
				exit_success("terminating ftp control connection");
				/*NOTREACHED*/
			}
		}
		if (0 <= port4 && 0 <= port6 && (pfd[2].revents & POLLIN))
		{
			/*
			 * copy data connection.
			 * no special treatment necessary.
			 */
			if (pfd[2].revents & POLLIN)
				error = ftp_copy(port4, port6);
			switch (error) {
			case -1:
				goto bad;
			case 0:
				if (port4 >= 0) {
					(void)close(port4);
					port4 = -1;
				}
				if (port6 >= 0) {
					(void)close(port6);
					port6 = -1;
				}
				syslog(LOG_INFO, "terminating data connection");
				break;
			default:
				break;
			}
		}
		if (0 <= port4 && 0 <= port6 && (pfd[3].revents & POLLIN))
		{
			/*
			 * copy data connection.
			 * no special treatment necessary.
			 */
			if (pfd[3].revents & POLLIN)
				error = ftp_copy(port6, port4);
			switch (error) {
			case -1:
				goto bad;
			case 0:
				if (port4 >= 0) {
					(void)close(port4);
					port4 = -1;
				}
				if (port6 >= 0) {
					(void)close(port6);
					port6 = -1;
				}
				syslog(LOG_INFO, "terminating data connection");
				break;
			default:
				break;
			}
		}
#if 0
		if (wport4 && (pfd[4].revents & POLLIN))
		{
			/*
			 * establish active data connection from the server.
			 */
			ftp_activeconn();
		}
		if (wport4 && (pfd[5].revents & POLLIN))
		{
			/*
			 * establish passive data connection from the client.
			 */
			ftp_passiveconn();
		}
#endif
	}

 bad:
	exit_failure("%s", strerror(errno));
}

static int
ftp_activeconn()
{
	socklen_t n;
	int error;
	struct pollfd pfd[1];
	struct sockaddr *sa;

	/* get active connection from server */
	pfd[0].fd = wport4;
	pfd[0].events = POLLIN;
	n = sizeof(data4);
	if (poll(pfd, (unsigned int)(sizeof(pfd) / sizeof(pfd[0])),
	    120000) == 0 ||
	    (port4 = accept(wport4, (void *)&data4, &n)) < 0)
	{
		(void)close(wport4);
		wport4 = -1;
		syslog(LOG_INFO, "active mode data connection failed");
		return -1;
	}

	/* ask active connection to client */
	sa = (void *)&data6;
	port6 = socket(sa->sa_family, SOCK_STREAM, 0);
	if (port6 == -1) {
		(void)close(port4);
		(void)close(wport4);
		port4 = wport4 = -1;
		syslog(LOG_INFO, "active mode data connection failed");
		return -1;
	}
	error = connect(port6, sa, (socklen_t)sa->sa_len);
	if (error < 0) {
		(void)close(port6);
		(void)close(port4);
		(void)close(wport4);
		port6 = port4 = wport4 = -1;
		syslog(LOG_INFO, "active mode data connection failed");
		return -1;
	}

	syslog(LOG_INFO, "active mode data connection established");
	return 0;
}

static int
ftp_passiveconn()
{
	socklen_t len;
	int error;
	struct pollfd pfd[1];
	struct sockaddr *sa;

	/* get passive connection from client */
	pfd[0].fd = wport6;
	pfd[0].events = POLLIN;
	len = sizeof(data6);
	if (poll(pfd, (unsigned int)(sizeof(pfd) / sizeof(pfd[0])),
	    120000) == 0 ||
	    (port6 = accept(wport6, (void *)&data6, &len)) < 0)
	{
		(void)close(wport6);
		wport6 = -1;
		syslog(LOG_INFO, "passive mode data connection failed");
		return -1;
	}

	/* ask passive connection to server */
	sa = (void *)&data4;
	port4 = socket(sa->sa_family, SOCK_STREAM, 0);
	if (port4 == -1) {
		(void)close(wport6);
		(void)close(port6);
		wport6 = port6 = -1;
		syslog(LOG_INFO, "passive mode data connection failed");
		return -1;
	}
	error = connect(port4, sa, (socklen_t)sa->sa_len);
	if (error < 0) {
		(void)close(wport6);
		(void)close(port4);
		(void)close(port6);
		wport6 = port4 = port6 = -1;
		syslog(LOG_INFO, "passive mode data connection failed");
		return -1;
	}

	syslog(LOG_INFO, "passive mode data connection established");
	return 0;
}

static ssize_t
ftp_copy(int src, int dst)
{
	int error, atmark;
	ssize_t n;

	/* OOB data handling */
	error = ioctl(src, SIOCATMARK, &atmark);
	if (error != -1 && atmark == 1) {
		n = read(src, rbuf, 1);
		if (n == -1)
			goto bad;
		(void)send(dst, rbuf, (size_t)n, MSG_OOB);
#if 0
		n = read(src, rbuf, sizeof(rbuf));
		if (n == -1)
			goto bad;
		(void)write(dst, rbuf, (size_t)n);
		return n;
#endif
	}

	n = read(src, rbuf, sizeof(rbuf));
	switch (n) {
	case -1:
	case 0:
		return n;
	default:
		(void)write(dst, rbuf, (size_t)n);
		return n;
	}

 bad:
	exit_failure("%s", strerror(errno));
	/*NOTREACHED*/
	return 0;	/* to make gcc happy */
}

static ssize_t
ftp_copyresult(int src, int dst, enum state state)
{
	int error, atmark;
	ssize_t n;
	socklen_t len;
	char *param;
	int code;
	char *a, *p;
	int i;

	/* OOB data handling */
	error = ioctl(src, SIOCATMARK, &atmark);
	if (error != -1 && atmark == 1) {
		n = read(src, rbuf, 1);
		if (n == -1)
			goto bad;
		(void)send(dst, rbuf, (size_t)n, MSG_OOB);
#if 0
		n = read(src, rbuf, sizeof(rbuf));
		if (n == -1)
			goto bad;
		(void)write(dst, rbuf, (size_t)n);
		return n;
#endif
	}

	n = read(src, rbuf, sizeof(rbuf));
	if (n <= 0)
		return n;
	rbuf[n] = '\0';

	/*
	 * parse argument
	 */
	p = rbuf;
	for (i = 0; i < 3; i++) {
		if (!isdigit((unsigned char)*p)) {
			/* invalid reply */
			(void)write(dst, rbuf, (size_t)n);
			return n;
		}
		p++;
	}
	if (!isspace((unsigned char)*p)) {
		/* invalid reply */
		(void)write(dst, rbuf, (size_t)n);
		return n;
	}
	code = atoi(rbuf);
	param = p;
	/* param points to first non-command token, if any */
	while (*param && isspace((unsigned char)*param))
		param++;
	if (!*param)
		param = NULL;

	switch (state) {
	case NONE:
		if (!passivemode && rbuf[0] == '1') {
			if (ftp_activeconn() < 0) {
				n = snprintf(rbuf, sizeof(rbuf),
				    "425 Cannot open data connetion\r\n");
				if (n < 0 || n >= (int)sizeof(rbuf))
					n = 0;
			}
		}
		if (n)
			(void)write(dst, rbuf, (size_t)n);
		return n;
	case LPRT:
	case EPRT:
		/* expecting "200 PORT command successful." */
		if (code == 200) {
			p = strstr(rbuf, "PORT");
			if (p) {
				p[0] = (state == LPRT) ? 'L' : 'E';
				p[1] = 'P';
			}
		} else {
			(void)close(wport4);
			wport4 = -1;
		}
		(void)write(dst, rbuf, (size_t)n);
		return n;
	case LPSV:
	case EPSV:
		/*
		 * expecting "227 Entering Passive Mode (x,x,x,x,x,x,x)"
		 * (in some cases result comes without paren)
		 */
		if (code != 227) {
passivefail0:
			(void)close(wport6);
			wport6 = -1;
			(void)write(dst, rbuf, (size_t)n);
			return n;
		}

	    {
		unsigned int ho[4], po[2];
		struct sockaddr_in *sin;
		struct sockaddr_in6 *sin6;
		u_short port;

		/*
		 * PASV result -> LPSV/EPSV result
		 */
		p = param;
		while (*p && *p != '(' && !isdigit((unsigned char)*p))	/*)*/
			p++;
		if (!*p)
			goto passivefail0;	/*XXX*/
		if (*p == '(')	/*)*/
			p++;
		n = sscanf(p, "%u,%u,%u,%u,%u,%u",
			&ho[0], &ho[1], &ho[2], &ho[3], &po[0], &po[1]);
		if (n != 6)
			goto passivefail0;	/*XXX*/

		/* keep PORT parameter */
		memset(&data4, 0, sizeof(data4));
		sin = (void *)&data4;
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = 0;
		for (n = 0; n < 4; n++) {
			sin->sin_addr.s_addr |= htonl(((uint32_t)(ho[n] & 0xff)
			    << (int)((3 - n) * 8)));
		}
		sin->sin_port = htons(((po[0] & 0xff) << 8) | (po[1] & 0xff));

		/* get ready for passive data connection */
		memset(&data6, 0, sizeof(data6));
		sin6 = (void *)&data6;
		sin6->sin6_len = sizeof(*sin6);
		sin6->sin6_family = AF_INET6;
		wport6 = socket(sin6->sin6_family, SOCK_STREAM, 0);
		if (wport6 == -1) {
passivefail:
			return dprintf(src,
			    "500 could not translate from PASV\r\n");
		}
#ifdef IPV6_FAITH
	    {
		int on = 1;
		error = setsockopt(wport6, IPPROTO_IPV6, IPV6_FAITH,
			&on, (socklen_t)sizeof(on));
		if (error == -1)
			exit_failure("setsockopt(IPV6_FAITH): %s", strerror(errno));
	    }
#endif
		error = bind(wport6, (void *)sin6, (socklen_t)sin6->sin6_len);
		if (error == -1) {
			(void)close(wport6);
			wport6 = -1;
			goto passivefail;
		}
		error = listen(wport6, 1);
		if (error == -1) {
			(void)close(wport6);
			wport6 = -1;
			goto passivefail;
		}

		/* transmit LPSV or EPSV */
		/*
		 * addr from dst, port from wport6
		 */
		len = sizeof(data6);
		error = getsockname(wport6, (void *)&data6, &len);
		if (error == -1) {
			(void)close(wport6);
			wport6 = -1;
			goto passivefail;
		}
		sin6 = (void *)&data6;
		port = sin6->sin6_port;

		len = sizeof(data6);
		error = getsockname(dst, (void *)&data6, &len);
		if (error == -1) {
			(void)close(wport6);
			wport6 = -1;
			goto passivefail;
		}
		sin6 = (void *)&data6;
		sin6->sin6_port = port;

		if (state == LPSV) {
			a = (void *)&sin6->sin6_addr;
			p = (void *)&sin6->sin6_port;
			passivemode = 1;
			return dprintf(dst,
			    "228 Entering Long Passive Mode (%d,%d,%d,%d,%d,%d,"
			    "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\r\n",
			    6, 16, UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
			    UC(a[4]), UC(a[5]), UC(a[6]), UC(a[7]),
			    UC(a[8]), UC(a[9]), UC(a[10]), UC(a[11]),
			    UC(a[12]), UC(a[13]), UC(a[14]), UC(a[15]),
			    2, UC(p[0]), UC(p[1]));
		} else {
			passivemode = 1;
			return dprintf(dst,
			    "229 Entering Extended Passive Mode (|||%d|)\r\n",
			    ntohs(sin6->sin6_port));
		}
	    }
	}

 bad:
	exit_failure("%s", strerror(errno));
	/*NOTREACHED*/
	return 0;	/* to make gcc happy */
}

static ssize_t
ftp_copycommand(int src, int dst, enum state *state)
{
	int error, atmark;
	ssize_t n;
	socklen_t len;
	unsigned int af, hal, ho[16], pal, po[2];
	char *a, *p, *q;
	char cmd[5], *param;
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	enum state nstate;
	char ch;
	int i;

	/* OOB data handling */
	error = ioctl(src, SIOCATMARK, &atmark);
	if (error != -1 && atmark == 1) {
		n = read(src, rbuf, 1);
		if (n == -1)
			goto bad;
		(void)send(dst, rbuf, (size_t)n, MSG_OOB);
#if 0
		n = read(src, rbuf, sizeof(rbuf));
		if (n == -1)
			goto bad;
		(void)write(dst, rbuf, (size_t)n);
		return n;
#endif
	}

	n = read(src, rbuf, sizeof(rbuf));
	if (n <= 0)
		return n;
	rbuf[n] = '\0';

	if (n < 4) {
		(void)write(dst, rbuf, (size_t)n);
		return n;
	}

	/*
	 * parse argument
	 */
	p = rbuf;
	q = cmd;
	for (i = 0; i < 4; i++) {
		if (!isalpha((unsigned char)*p)) {
			/* invalid command */
			(void)write(dst, rbuf, (size_t)n);
			return n;
		}
		*q++ = islower((unsigned char)*p) ? toupper((unsigned char)*p) : *p;
		p++;
	}
	if (!isspace((unsigned char)*p)) {
		/* invalid command */
		(void)write(dst, rbuf, (size_t)n);
		return n;
	}
	*q = '\0';
	param = p;
	/* param points to first non-command token, if any */
	while (*param && isspace((unsigned char)*param))
		param++;
	if (!*param)
		param = NULL;

	*state = NONE;

	if (strcmp(cmd, "LPRT") == 0 && param) {
		/*
		 * LPRT -> PORT
		 */
		nstate = LPRT;

		(void)close(wport4);
		(void)close(wport6);
		(void)close(port4);
		(void)close(port6);
		wport4 = wport6 = port4 = port6 = -1;

		if (epsvall) {
			return dprintf(src, "501 %s disallowed in EPSV ALL\r\n",
			    cmd);
		}

		n = sscanf(param,
		    "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		    "%u,%u,%u", &af, &hal, &ho[0], &ho[1], &ho[2], &ho[3],
		    &ho[4], &ho[5], &ho[6], &ho[7],
		    &ho[8], &ho[9], &ho[10], &ho[11],
		    &ho[12], &ho[13], &ho[14], &ho[15],
		    &pal, &po[0], &po[1]);
		if (n != 21 || af != 6 || hal != 16|| pal != 2) {
			return dprintf(src,
			    "501 illegal parameter to LPRT\r\n");
		}

		/* keep LPRT parameter */
		memset(&data6, 0, sizeof(data6));
		sin6 = (void *)&data6;
		sin6->sin6_len = sizeof(*sin6);
		sin6->sin6_family = AF_INET6;
		for (n = 0; n < 16; n++)
			sin6->sin6_addr.s6_addr[n] = ho[n];
		sin6->sin6_port = htons(((po[0] & 0xff) << 8) | (po[1] & 0xff));

sendport:
		/* get ready for active data connection */
		len = sizeof(data4);
		error = getsockname(dst, (void *)&data4, &len);
		if (error == -1) {
lprtfail:
			return dprintf(src,
			    "500 could not translate to PORT\r\n");
		}
		if (((struct sockaddr *)(void *)&data4)->sa_family != AF_INET)
			goto lprtfail;
		sin = (void *)&data4;
		sin->sin_port = 0;
		wport4 = socket(sin->sin_family, SOCK_STREAM, 0);
		if (wport4 == -1)
			goto lprtfail;
		error = bind(wport4, (void *)sin, (socklen_t)sin->sin_len);
		if (error == -1) {
			(void)close(wport4);
			wport4 = -1;
			goto lprtfail;
		}
		error = listen(wport4, 1);
		if (error == -1) {
			(void)close(wport4);
			wport4 = -1;
			goto lprtfail;
		}

		/* transmit PORT */
		len = sizeof(data4);
		error = getsockname(wport4, (void *)&data4, &len);
		if (error == -1) {
			(void)close(wport4);
			wport4 = -1;
			goto lprtfail;
		}
		if (((struct sockaddr *)(void *)&data4)->sa_family != AF_INET) {
			(void)close(wport4);
			wport4 = -1;
			goto lprtfail;
		}
		sin = (void *)&data4;
		a = (void *)&sin->sin_addr;
		p = (void *)&sin->sin_port;
		*state = nstate;
		passivemode = 0;
		return dprintf(dst, "PORT %d,%d,%d,%d,%d,%d\r\n",
		    UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
	} else if (strcmp(cmd, "EPRT") == 0 && param) {
		/*
		 * EPRT -> PORT
		 */
		char *afp, *hostp, *portp;
		struct addrinfo hints, *res;

		nstate = EPRT;

		(void)close(wport4);
		(void)close(wport6);
		(void)close(port4);
		(void)close(port6);
		wport4 = wport6 = port4 = port6 = -1;

		if (epsvall) {
			return dprintf(src, "501 %s disallowed in EPSV ALL\r\n",
			    cmd);
		}

		p = param;
		ch = *p++;	/* boundary character */
		afp = p;
		while (*p && *p != ch)
			p++;
		if (!*p) {
eprtparamfail:
			return dprintf(src,
			    "501 illegal parameter to EPRT\r\n");
		}
		*p++ = '\0';
		hostp = p;
		while (*p && *p != ch)
			p++;
		if (!*p)
			goto eprtparamfail;
		*p++ = '\0';
		portp = p;
		while (*p && *p != ch)
			p++;
		if (!*p)
			goto eprtparamfail;
		*p++ = '\0';

		n = sscanf(afp, "%d", &af);
		if (n != 1 || af != 2) {
			return dprintf(src,
			    "501 unsupported address family to EPRT\r\n");
		}
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		error = getaddrinfo(hostp, portp, &hints, &res);
		if (error) {
			return dprintf(src,
			    "501 EPRT: %s\r\n", gai_strerror(error));
		}
		if (res->ai_next) {
			freeaddrinfo(res);
			return dprintf(src,
			    "501 EPRT: %s resolved to multiple addresses\r\n",
			    hostp);
		}

		memcpy(&data6, res->ai_addr, res->ai_addrlen);

		freeaddrinfo(res);
		goto sendport;
	} else if (strcmp(cmd, "LPSV") == 0 && !param) {
		/*
		 * LPSV -> PASV
		 */
		nstate = LPSV;

		(void)close(wport4);
		(void)close(wport6);
		(void)close(port4);
		(void)close(port6);
		wport4 = wport6 = port4 = port6 = -1;

		if (epsvall) {
			return dprintf(src, "501 %s disallowed in EPSV ALL\r\n",
			    cmd);
		}

		*state = LPSV;
		passivemode = 0;	/* to be set to 1 later */
		/* transmit PASV */
		return dprintf(dst, "PASV\r\n");
	} else if (strcmp(cmd, "EPSV") == 0 && !param) {
		/*
		 * EPSV -> PASV
		 */
		(void)close(wport4);
		(void)close(wport6);
		(void)close(port4);
		(void)close(port6);
		wport4 = wport6 = port4 = port6 = -1;

		*state = EPSV;
		passivemode = 0;	/* to be set to 1 later */
		return dprintf(dst, "PASV\r\n");
	} else if (strcmp(cmd, "EPSV") == 0 && param &&
	    strncasecmp(param, "ALL", 3) == 0 &&
	    isspace((unsigned char)param[3])) {
		/*
		 * EPSV ALL
		 */
		epsvall = 1;
		return dprintf(src, "200 EPSV ALL command successful.\r\n");
	} else if (strcmp(cmd, "PORT") == 0 || strcmp(cmd, "PASV") == 0) {
		/*
		 * reject PORT/PASV
		 */
		return dprintf(src, "502 %s not implemented.\r\n", cmd);
	} else if (passivemode
		&& (strcmp(cmd, "STOR") == 0
		 || strcmp(cmd, "STOU") == 0
		 || strcmp(cmd, "RETR") == 0
		 || strcmp(cmd, "LIST") == 0
		 || strcmp(cmd, "NLST") == 0
		 || strcmp(cmd, "APPE") == 0)) {
		/*
		 * commands with data transfer.  need to care about passive
		 * mode data connection.
		 */

		*state = NONE;
		if (ftp_passiveconn() < 0) {
			return dprintf(src,
			    "425 Cannot open data connetion\r\n");
		} else {
			/* simply relay the command */
			return write(dst, rbuf, (size_t)n);
		}
	} else {
		/* simply relay it */
		*state = NONE;
		return write(dst, rbuf, (size_t)n);
	}

 bad:
	exit_failure("%s", strerror(errno));
	/*NOTREACHED*/
	return 0;	/* to make gcc happy */
}
