/*	$NetBSD: trans_relay.c,v 1.1.1.1 2004/03/28 08:56:26 martti Exp $	*/

/*
 * Sample program to be used as a transparent proxy.
 *
 * Must be executed with permission enough to do an ioctl on /dev/ipl
 * or equivalent.  This is just a sample and is only alpha quality.
 * - Darren Reed (8 April 1996)
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#if !defined(__SVR4) && !defined(__svr4__)
#include <strings.h>
#else
#include <sys/byteorder.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#if defined(sun) && (defined(__svr4__) || defined(__SVR4))
# include <sys/ioccom.h>
# include <sys/sysmacros.h>
#endif
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <ctype.h>
#include "netinet/ip_compat.h"
#include "netinet/ip_fil.h"
#include "netinet/ip_proxy.h"
#include "netinet/ip_nat.h"


#define	RELAY_BUFSZ	8192

char	ibuff[RELAY_BUFSZ];
char	obuff[RELAY_BUFSZ];

int relay(ifd, ofd, rfd)
int ifd, ofd, rfd;
{
	fd_set	rfds, wfds;
	char	*irh, *irt, *rrh, *rrt;
	char	*iwh, *iwt, *rwh, *rwt;
	int	nfd, n, rw;

	irh = irt = ibuff;
	iwh = iwt = obuff;
	nfd = ifd;
	if (nfd < ofd)
		nfd = ofd;
	if (nfd < rfd)
		nfd = rfd;

	while (1) {
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		if (irh > irt)
			FD_SET(rfd, &wfds);
		if (irh < (ibuff + RELAY_BUFSZ))
			FD_SET(ifd, &rfds);
		if (iwh > iwt)
			FD_SET(ofd, &wfds);
		if (iwh < (obuff + RELAY_BUFSZ))
			FD_SET(rfd, &rfds);

		switch ((n = select(nfd + 1, &rfds, &wfds, NULL, NULL)))
		{
		case -1 :
		case 0 :
			return -1;
		default :
			if (FD_ISSET(ifd, &rfds)) {
				rw = read(ifd, irh, ibuff + RELAY_BUFSZ - irh);
				if (rw == -1)
					return -1;
				if (rw == 0)
					return 0;
				irh += rw;
				n--;
			}
			if (n && FD_ISSET(ofd, &wfds)) {
				rw = write(ofd, iwt, iwh  - iwt);
				if (rw == -1)
					return -1;
				iwt += rw;
				n--;
			}
			if (n && FD_ISSET(rfd, &rfds)) {
				rw = read(rfd, iwh, obuff + RELAY_BUFSZ - iwh);
				if (rw == -1)
					return -1;
				if (rw == 0)
					return 0;
				iwh += rw;
				n--;
			}
			if (n && FD_ISSET(rfd, &wfds)) {
				rw = write(rfd, irt, irh  - irt);
				if (rw == -1)
					return -1;
				irt += rw;
				n--;
			}
			if (irh == irt)
				irh = irt = ibuff;
			if (iwh == iwt)
				iwh = iwt = obuff;
		}
	}
}

main(argc, argv)
int argc;
char *argv[];
{
	struct	sockaddr_in	sin;
	natlookup_t	nl, *nlp = &nl;
	nattrans_t	nattn, *nattnp = &nattn;
	int	nfd, fd, sl = sizeof(sl), se;

	openlog(argv[0], LOG_PID|LOG_NDELAY, LOG_DAEMON);
	if ((nfd = open(IPL_NAT, O_RDWR)) == -1) {
		se = errno;
		perror("open");
		errno = se;
		syslog(LOG_ERR, "open: %m\n");
		exit(-1);
	}

	bzero(&nl, sizeof(nl));
	bzero(&nattn, sizeof(nattn));
	nl.nl_flags = IPN_TCP;

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sl = sizeof(sin);
	if (getsockname(0, (struct sockaddr *)&sin, &sl) == -1) {
		se = errno;
		perror("getsockname");
		errno = se;
		syslog(LOG_ERR, "getsockname: %m\n");
		exit(-1);
	} else {
		nl.nl_inip.s_addr = sin.sin_addr.s_addr;
		nl.nl_inport = sin.sin_port;
	}

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sl = sizeof(sin);
	if (getpeername(0, (struct sockaddr *)&sin, &sl) == -1) {
		se = errno;
		perror("getpeername");
		errno = se;
		syslog(LOG_ERR, "getpeername: %m\n");
		exit(-1);
	} else {
		nl.nl_outip.s_addr = sin.sin_addr.s_addr;
		nl.nl_outport = sin.sin_port;
	}

	if (ioctl(nfd, SIOCGNATL, &nlp) == -1) {
		se = errno;
		perror("ioctl");
		errno = se;
		syslog(LOG_ERR, "ioctl: %m\n");
		exit(-1);
	}

	/*
	 * This is to work out which outgoing IP address will be used.
	 */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		se = errno;
		perror("socket");
		errno = se;
		syslog(LOG_ERR, "socket: %m\n");
		exit(-1);
	}

	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr = nl.nl_realip;
	sin.sin_port = nl.nl_realport;
	if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		se = errno;
		perror("connect");
		errno = se;
		syslog(LOG_ERR, "connect: %m\n");
		exit(-1);
	}

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sl = sizeof(sin);
	if (getsockname(fd, (struct sockaddr *)&sin, &sl) == -1) {
		se = errno;
		perror("getsockname");
		errno = se;
		syslog(LOG_ERR, "getsockname: %m\n");
		exit(-1);
	} else {
		nattn.tn_dip = nl.nl_realip;
		nattn.tn_lip = sin.sin_addr;
		nattn.tn_sip = nl.nl_outip;
		nattn.tn_dport = nl.nl_realport;
		nattn.tn_sport = nl.nl_outport;
	}
	close(fd);
	fd = -1;

	/*
	 * Now create the real outgoing socket.
	 */
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		se = errno;
		perror("socket");
		errno = se;
		syslog(LOG_ERR, "socket: %m\n");
		exit(-1);
	}

	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr = nattn.tn_lip;
	sin.sin_port = nl.nl_outport;
	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) == 0) {
		nattn.tn_lport = sin.sin_port;
		syslog(LOG_INFO, "from %s,%hu",
			inet_ntoa(nl.nl_outip), nl.nl_outport);
		syslog(LOG_INFO, "to %s,%hu",
			inet_ntoa(nl.nl_realip), nl.nl_realport);
		syslog(LOG_INFO, "via %s,%hu",
			inet_ntoa(nattn.tn_lip), nattn.tn_lport);

		if (ioctl(nfd, SIOCTRANS, &nattnp) == -1) {
			se = errno;
			perror("ioctl(SIOCTRANS)");
			errno = se;
			syslog(LOG_ERR, "ioctl(SIOCTRANS):%d %m\n", errno);
			exit(-1);
		}
	} else {
		se = errno;
		perror("bind");
		errno = se;
		syslog(LOG_ERR, "bind: %m\n");
		exit(-1);
	}
	close(nfd);

	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = nl.nl_realport;
	sin.sin_addr = nl.nl_realip;
	sl = sizeof(sin);
	syslog(LOG_NOTICE, "connecting to %s,%d\n", inet_ntoa(sin.sin_addr),
		ntohs(sin.sin_port));
	sleep(3);
	if (connect(fd, (struct sockaddr *)&sin, sl) == -1) {
		se = errno;
		perror("connect");
		errno = se;
		syslog(LOG_ERR, "connect: %m\n");
		exit(-1);
	}

	(void) ioctl(fd, F_SETFL, ioctl(fd, F_GETFL, 0)|O_NONBLOCK);
	(void) ioctl(0, F_SETFL, ioctl(fd, F_GETFL, 0)|O_NONBLOCK);
	(void) ioctl(1, F_SETFL, ioctl(fd, F_GETFL, 0)|O_NONBLOCK);

	syslog(LOG_NOTICE, "connected to %s,%d\n", inet_ntoa(sin.sin_addr),
		ntohs(sin.sin_port));
	if (relay(0, 1, fd) == -1) {
		se = errno;
		perror("relay");
		errno = se;
		syslog(LOG_ERR, "relay: %m\n");
		exit(-1);
	}
	exit(0);
}
