/*	$NetBSD: traceroute6.c,v 1.44.12.1 2018/05/02 07:20:28 pgoyette Exp $	*/
/*	$KAME: traceroute6.c,v 1.67 2004/01/25 03:24:39 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if 0
#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)traceroute.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */
#else
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: traceroute6.c,v 1.44.12.1 2018/05/02 07:20:28 pgoyette Exp $");
#endif
#endif

/*
 * traceroute host  - trace the route ip packets follow going to "host".
 *
 * Attempt to trace the route an ip packet would follow to some
 * internet host.  We find out intermediate hops by launching probe
 * packets with a small ttl (time to live) then listening for an
 * icmp "time exceeded" reply from a gateway.  We start our probes
 * with a ttl of one and increase by one until we get an icmp "port
 * unreachable" (which means we got to "host") or hit a max (which
 * defaults to 30 hops & can be changed with the -m flag).  Three
 * probes (change with -q flag) are sent at each ttl setting and a
 * line is printed showing the ttl, address of the gateway and
 * round trip time of each probe.  If the probe answers come from
 * different gateways, the address of each responding system will
 * be printed.  If there is no response within a 5 sec. timeout
 * interval (changed with the -w flag), a "*" is printed for that
 * probe.
 *
 * Probe packets are UDP format.  We don't want the destination
 * host to process them so the destination port is set to an
 * unlikely value (if some clod on the destination is using that
 * value, it can be changed with the -p flag).
 *
 * A sample use might be:
 *
 *     [yak 71]% traceroute nis.nsf.net.
 *     traceroute to nis.nsf.net (35.1.1.48), 30 hops max, 56 byte packet
 *      1  helios.ee.lbl.gov (128.3.112.1)  19 ms  19 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  39 ms  19 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  39 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  39 ms  40 ms  39 ms
 *      5  ccn-nerif22.Berkeley.EDU (128.32.168.22)  39 ms  39 ms  39 ms
 *      6  128.32.197.4 (128.32.197.4)  40 ms  59 ms  59 ms
 *      7  131.119.2.5 (131.119.2.5)  59 ms  59 ms  59 ms
 *      8  129.140.70.13 (129.140.70.13)  99 ms  99 ms  80 ms
 *      9  129.140.71.6 (129.140.71.6)  139 ms  239 ms  319 ms
 *     10  129.140.81.7 (129.140.81.7)  220 ms  199 ms  199 ms
 *     11  nic.merit.edu (35.1.1.48)  239 ms  239 ms  239 ms
 *
 * Note that lines 2 & 3 are the same.  This is due to a buggy
 * kernel on the 2nd hop system -- lbl-csam.arpa -- that forwards
 * packets with a zero ttl.
 *
 * A more interesting example is:
 *
 *     [yak 72]% traceroute allspice.lcs.mit.edu.
 *     traceroute to allspice.lcs.mit.edu (18.26.0.115), 30 hops max
 *      1  helios.ee.lbl.gov (128.3.112.1)  0 ms  0 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  19 ms  19 ms  19 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  19 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  19 ms  39 ms  39 ms
 *      5  ccn-nerif22.Berkeley.EDU (128.32.168.22)  20 ms  39 ms  39 ms
 *      6  128.32.197.4 (128.32.197.4)  59 ms  119 ms  39 ms
 *      7  131.119.2.5 (131.119.2.5)  59 ms  59 ms  39 ms
 *      8  129.140.70.13 (129.140.70.13)  80 ms  79 ms  99 ms
 *      9  129.140.71.6 (129.140.71.6)  139 ms  139 ms  159 ms
 *     10  129.140.81.7 (129.140.81.7)  199 ms  180 ms  300 ms
 *     11  129.140.72.17 (129.140.72.17)  300 ms  239 ms  239 ms
 *     12  * * *
 *     13  128.121.54.72 (128.121.54.72)  259 ms  499 ms  279 ms
 *     14  * * *
 *     15  * * *
 *     16  * * *
 *     17  * * *
 *     18  ALLSPICE.LCS.MIT.EDU (18.26.0.115)  339 ms  279 ms  279 ms
 *
 * (I start to see why I'm having so much trouble with mail to
 * MIT.)  Note that the gateways 12, 14, 15, 16 & 17 hops away
 * either don't send ICMP "time exceeded" messages or send them
 * with a ttl too small to reach us.  14 - 17 are running the
 * MIT C Gateway code that doesn't send "time exceeded"s.  God
 * only knows what's going on with 12.
 *
 * The silent gateway 12 in the above may be the result of a bug in
 * the 4.[23]BSD network code (and its derivatives):  4.x (x <= 3)
 * sends an unreachable message using whatever ttl remains in the
 * original datagram.  Since, for gateways, the remaining ttl is
 * zero, the icmp "time exceeded" is guaranteed to not make it back
 * to us.  The behavior of this bug is slightly more interesting
 * when it appears on the destination system:
 *
 *      1  helios.ee.lbl.gov (128.3.112.1)  0 ms  0 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  19 ms  39 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  19 ms  39 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  39 ms  40 ms  19 ms
 *      5  ccn-nerif35.Berkeley.EDU (128.32.168.35)  39 ms  39 ms  39 ms
 *      6  csgw.Berkeley.EDU (128.32.133.254)  39 ms  59 ms  39 ms
 *      7  * * *
 *      8  * * *
 *      9  * * *
 *     10  * * *
 *     11  * * *
 *     12  * * *
 *     13  rip.Berkeley.EDU (128.32.131.22)  59 ms !  39 ms !  39 ms !
 *
 * Notice that there are 12 "gateways" (13 is the final
 * destination) and exactly the last half of them are "missing".
 * What's really happening is that rip (a Sun-3 running Sun OS3.5)
 * is using the ttl from our arriving datagram as the ttl in its
 * icmp reply.  So, the reply will time out on the return path
 * (with no notice sent to anyone since icmp's aren't sent for
 * icmp's) until we probe with a ttl that's at least twice the path
 * length.  I.e., rip is really only 7 hops away.  A reply that
 * returns with a ttl of 1 is a clue this problem exists.
 * Traceroute prints a "!" after the time if the ttl is <= 1.
 * Since vendors ship a lot of obsolete (DEC's Ultrix, Sun 3.x) or
 * non-standard (HPUX) software, expect to see this problem
 * frequently and/or take care picking the target host of your
 * probes.
 *
 * Other possible annotations after the time are !H, !N, !P (got a host,
 * network or protocol unreachable, respectively), !S or !F (source
 * route failed or fragmentation needed -- neither of these should
 * ever occur and the associated gateway is busted if you see one).  If
 * almost all the probes result in some kind of unreachable, traceroute
 * will give up and exit.
 *
 * Notes
 * -----
 * This program must be run by root or be setuid.  (I suggest that
 * you *don't* make it setuid -- casual use could result in a lot
 * of unnecessary traffic on our poor, congested nets.)
 *
 * This program requires a kernel mod that does not appear in any
 * system available from Berkeley:  A raw ip socket using proto
 * IPPROTO_RAW must interpret the data sent as an ip datagram (as
 * opposed to data to be wrapped in a ip datagram).  See the README
 * file that came with the source to this program for a description
 * of the mods I made to /sys/netinet/raw_ip.c.  Your mileage may
 * vary.  But, again, ANY 4.x (x < 4) BSD KERNEL WILL HAVE TO BE
 * MODIFIED TO RUN THIS PROGRAM.
 *
 * The udp port usage may appear bizarre (well, ok, it is bizarre).
 * The problem is that an icmp message only contains 8 bytes of
 * data from the original datagram.  8 bytes is the size of a udp
 * header so, if we want to associate replies with the original
 * datagram, the necessary information must be encoded into the
 * udp header (the ip id could be used but there's no way to
 * interlock with the kernel's assignment of ip id's and, anyway,
 * it would have taken a lot more kernel hacking to allow this
 * code to set the ip id).  So, to allow two or more users to
 * use traceroute simultaneously, we use this task's pid as the
 * source port (the high bit is set to move the port number out
 * of the "likely" range).  To keep track of which probe is being
 * replied to (so times and/or hop counts don't get confused by a
 * reply that was delayed in transit), we increment the destination
 * port number before each probe.
 *
 * Don't use this as a coding example.  I was trying to find a
 * routing problem and this code sort-of popped out after 48 hours
 * without sleep.  I was amazed it ever compiled, much less ran.
 *
 * I stole the idea for this program from Steve Deering.  Since
 * the first release, I've learned that had I attended the right
 * IETF working group meetings, I also could have stolen it from Guy
 * Almes or Matt Mathis.  I don't know (or care) who came up with
 * the idea first.  I envy the originators' perspicacity and I'm
 * glad they didn't keep the idea a secret.
 *
 * Tim Seaver, Ken Adelman and C. Philip Wood provided bug fixes and/or
 * enhancements to the original distribution.
 *
 * I've hacked up a round-trip-route version of this that works by
 * sending a loose-source-routed udp datagram through the destination
 * back to yourself.  Unfortunately, SO many gateways botch source
 * routing, the thing is almost worthless.  Maybe one day...
 *
 *  -- Van Jacobson (van@helios.ee.lbl.gov)
 *     Tue Dec 20 03:50:13 PST 1988
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <netdb.h>
#include <stdio.h>
#include <err.h>
#ifdef HAVE_POLL
#include <poll.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/udp.h>

#ifdef IPSEC
#include <net/route.h>
#include <netipsec/ipsec.h>
#endif

#include "as.h"

#define DUMMY_PORT 10010

#define	MAXPACKET	65535	/* max ip packet size */

#ifndef HAVE_GETIPNODEBYNAME
#define getipnodebyname(x, y, z, u)	gethostbyname2((x), (y))
#define freehostent(x)
#endif

/*
 * format of a (udp) probe packet.
 */
struct tv32 {
	u_int32_t tv32_sec;
	u_int32_t tv32_usec;
};

struct opacket {
	u_char seq;		/* sequence number of this packet */
	u_char hops;		/* hop limit of the packet */
	u_char pad[2];
	struct tv32 tv;		/* time packet left */
} __packed;

static u_char	packet[512];		/* last inbound (icmp) packet */
static struct opacket	*outpacket;	/* last output (udp) packet */

static ssize_t	wait_for_reply(int, struct msghdr *);
#if defined(IPSEC) && defined(IPSEC_POLICY_IPSEC)
static int	setpolicy(int, const char *);
#endif
static void send_probe(int, u_long);
static struct udphdr *get_udphdr(struct ip6_hdr *, u_char *);
static double deltaT(struct timeval *, struct timeval *);
static const char *pr_type(int);
static int packet_ok(struct msghdr *, ssize_t, int);
static void print(struct msghdr *, int);
static const char *inetname(struct sockaddr *);
static void usage(void) __attribute__((__noreturn__));

static int rcvsock;		/* receive (icmp) socket file descriptor */
static int sndsock;		/* send (udp) socket file descriptor */

static struct msghdr rcvmhdr;
static struct iovec rcviov[2];
static int rcvhlim;
static struct in6_pktinfo *rcvpktinfo;

static struct sockaddr_in6 Src, Dst, Rcv;
static u_long datalen;			/* How much data */
#define	ICMP6ECHOLEN	8

static char *source;
static char *hostname;

static u_long nprobes = 3;
static u_long first_hop = 1;
static u_long max_hops = 30;
static u_int16_t srcport;
static u_int16_t port = 32768+666;/* start udp dest port # for probe packets */
static u_int16_t ident;
static int options;		/* socket options */
static int verbose;
static int waittime = 5;	/* time to wait for response (in seconds) */
static int nflag;		/* print addresses numerically */
static int useicmp;
static int lflag;		/* print both numerical address & hostname */
static int as_path;		/* print as numbers for each hop */
static char *as_server = NULL;
static void *asn;

int
main(int argc, char *argv[])
{
	int mib[4] = { CTL_NET, PF_INET6, IPPROTO_IPV6, IPV6CTL_DEFHLIM };
	char hbuf[NI_MAXHOST], src0[NI_MAXHOST], *ep;
	int ch, i, on = 1, seq, rcvcmsglen, error;
	u_long minlen;
	struct addrinfo hints, *res;
	static u_char *rcvcmsgbuf;
	u_long probe, hops, lport;
	size_t size;

	/*
	 * Receive ICMP
	 */
	if ((rcvsock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0)
		err(5, "socket(ICMPv6)");

	/* revoke privs */
	seteuid(getuid());
	setuid(getuid());

	size = sizeof(i);
	(void) sysctl(mib, sizeof(mib)/sizeof(mib[0]), &i, &size, NULL, 0);
	max_hops = i;

	/* specify to tell receiving interface */
	if (setsockopt(rcvsock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on,
	    sizeof(on)) < 0)
		err(1, "setsockopt(IPV6_RECVPKTINFO)");

	/* specify to tell value of hoplimit field of received IP6 hdr */
	if (setsockopt(rcvsock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on,
	    sizeof(on)) < 0)
		err(1, "setsockopt(IPV6_RECVHOPLIMIT)");

	seq = 0;

	while ((ch = getopt(argc, argv, "aA:df:Ilm:np:q:rs:w:v")) != -1)
		switch (ch) {
		case 'a':
			as_path = 1;
			break;
		case 'A':
			as_path = 1;
			as_server = optarg;
			break;
		case 'd':
			options |= SO_DEBUG;
			break;
		case 'f':
			ep = NULL;
			errno = 0;
			first_hop = strtoul(optarg, &ep, 0);
			if (errno || !*optarg || *ep|| first_hop > 255)
				errx(1, "Invalid min hoplimit `%s'", optarg);
			break;
		case 'I':
			useicmp++;
			/* same as ping6 */
			ident = htons(arc4random() & 0xffff);
			break;
		case 'l':
			lflag++;
			break;
		case 'm':
			ep = NULL;
			errno = 0;
			max_hops = strtoul(optarg, &ep, 0);
			if (errno || !*optarg || *ep || max_hops > 255)
				errx(1, "Invalid max hoplimit `%s'", optarg);
			break;
		case 'n':
			nflag++;
			break;
		case 'p':
			ep = NULL;
			errno = 0;
			lport = strtoul(optarg, &ep, 0);
			if (errno || !*optarg || *ep)
				errx(1, "Invalid port `%s'", optarg);
			if (lport == 0 || lport != (lport & 0xffff))
				errx(1, "Port `%s' out of range", optarg);
			port = lport & 0xffff;
			break;
		case 'q':
			ep = NULL;
			errno = 0;
			nprobes = strtoul(optarg, &ep, 0);
			if (errno || !*optarg || *ep)
				errx(1, "Invalid nprobes `%s'", optarg);
			if (nprobes < 1)
				errx(1, "nprobes `%s' must be > 0", optarg);
			break;
		case 'r':
			options |= SO_DONTROUTE;
			break;
		case 's':
			/*
			 * set the ip source address of the outbound
			 * probe (e.g., on a multi-homed host).
			 */
			source = optarg;
			break;
		case 'v':
			verbose++;
			break;
		case 'w':
			ep = NULL;
			errno = 0;
			waittime = strtoul(optarg, &ep, 0);
			if (errno || !*optarg || *ep)
				errx(1, "Invalid wait time `%s'", optarg);
			if (waittime <= 1)
				errx(1, "Wait `%s' must be > 1 sec", optarg);
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (max_hops < first_hop)
		errx(1, "max hoplimit `%lu' must be larger than "
		    "first hoplimit `%lu'", max_hops, first_hop);

	if (argc < 1 || argc > 2)
		usage();

#if 1
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
#else
	setlinebuf(stdout);
#endif

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET6;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMPV6;
	hints.ai_flags = AI_CANONNAME;
	error = getaddrinfo(*argv, NULL, &hints, &res);
	if (error)
		errx(1, "%s", gai_strerror(error));
	if (res->ai_addrlen != sizeof(Dst))
		errx(1, "size of sockaddr mismatch");
	memcpy(&Dst, res->ai_addr, res->ai_addrlen);
	hostname = res->ai_canonname ? strdup(res->ai_canonname) : *argv;
	if (!hostname)
		err(1, NULL);
	if (res->ai_next) {
		if (getnameinfo(res->ai_addr, res->ai_addrlen, hbuf,
		    sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0)
			strlcpy(hbuf, "?", sizeof(hbuf));
		warnx("`%s' has multiple addresses; using `%s'",
		    hostname, hbuf);
	}

	if (*++argv) {
		ep = NULL;
		errno = 0;
		datalen = strtoul(*argv, &ep, 0);
		if (errno || *ep)
			errx(1, "Invalid packet length `%s'", *argv);
	}
	if (useicmp)
		minlen = ICMP6ECHOLEN + sizeof(struct tv32);
	else
		minlen = sizeof(struct opacket);
	if (datalen < minlen)
		datalen = minlen;
	else if (datalen >= MAXPACKET)
		errx(1, "Packet size must be %lu <= s < %lu",
		    minlen, (u_long)MAXPACKET);
	outpacket = (struct opacket *)malloc((unsigned)datalen);
	if (!outpacket)
		err(1, NULL);
	memset(outpacket, 0, datalen);

	/* initialize msghdr for receiving packets */
	rcviov[0].iov_base = (caddr_t)packet;
	rcviov[0].iov_len = sizeof(packet);
	rcvmhdr.msg_name = (caddr_t)&Rcv;
	rcvmhdr.msg_namelen = sizeof(Rcv);
	rcvmhdr.msg_iov = rcviov;
	rcvmhdr.msg_iovlen = 1;
	rcvcmsglen = CMSG_SPACE(sizeof(struct in6_pktinfo)) +
	    CMSG_SPACE(sizeof(int));
	if ((rcvcmsgbuf = malloc(rcvcmsglen)) == NULL)
		err(1, NULL);
	rcvmhdr.msg_control = (caddr_t) rcvcmsgbuf;
	rcvmhdr.msg_controllen = rcvcmsglen;

	if (options & SO_DEBUG)
		(void) setsockopt(rcvsock, SOL_SOCKET, SO_DEBUG,
		    (char *)&on, sizeof(on));
	if (options & SO_DONTROUTE)
		(void) setsockopt(rcvsock, SOL_SOCKET, SO_DONTROUTE,
		    (char *)&on, sizeof(on));

#if defined(IPSEC) && defined(IPSEC_POLICY_IPSEC)
	if (setpolicy(rcvsock, "in bypass") < 0)
		errx(1, "%s", ipsec_strerror());
	if (setpolicy(rcvsock, "out bypass") < 0)
		errx(1, "%s", ipsec_strerror());
#endif

	/*
	 * Send UDP or ICMP
	 */
	if (useicmp) {
		sndsock = rcvsock;
	} else {
		if ((sndsock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
			err(5, "socket(SOCK_DGRAM)");
	}

	i = datalen;
	if (setsockopt(sndsock, SOL_SOCKET, SO_SNDBUF, (char *)&i,
	    sizeof(i)) < 0)
		err(6, "setsockopt(SO_SNDBUF)");

	if (options & SO_DEBUG)
		(void) setsockopt(sndsock, SOL_SOCKET, SO_DEBUG,
		    (char *)&on, sizeof(on));
	if (options & SO_DONTROUTE)
		(void) setsockopt(sndsock, SOL_SOCKET, SO_DONTROUTE,
		    (char *)&on, sizeof(on));

#if defined(IPSEC) && defined(IPSEC_POLICY_IPSEC)
	if (setpolicy(sndsock, "in bypass") < 0)
		errx(1, "%s", ipsec_strerror());
	if (setpolicy(sndsock, "out bypass") < 0)
		errx(1, "%s", ipsec_strerror());
#endif

	/*
	 * Source selection
	 */
	memset(&Src, 0, sizeof(Src));
	if (source) {
		struct addrinfo hints0, *res0;
		int error0;

		memset(&hints0, 0, sizeof(hints0));
		hints0.ai_family = AF_INET6;
		hints0.ai_socktype = SOCK_DGRAM;	/*dummy*/
		hints0.ai_flags = AI_NUMERICHOST;
		error0 = getaddrinfo(source, "0", &hints0, &res0);
		if (error0)
			errx(1, "Cannot get address for `%s' (%s)", source,
			    gai_strerror(error0));
		if (res0->ai_addrlen > sizeof(Src))
			errx(1, "Bad incompatible address length");
		memcpy(&Src, res0->ai_addr, res0->ai_addrlen);
		freeaddrinfo(res0);
	} else {
		struct sockaddr_in6 Nxt;
		int dummy;
		socklen_t len;

		Nxt = Dst;
		Nxt.sin6_port = htons(DUMMY_PORT);

		if ((dummy = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
			err(1, "socket");
		if (connect(dummy, (struct sockaddr *)&Nxt, Nxt.sin6_len) < 0)
			err(1, "connect");
		len = sizeof(Src);
		if (getsockname(dummy, (struct sockaddr *)&Src, &len) < 0)
			err(1, "getsockname");
		if ((error = getnameinfo((struct sockaddr *)&Src, Src.sin6_len,
		    src0, sizeof(src0), NULL, 0, NI_NUMERICHOST)))
			errx(1, "getnameinfo failed for source (%s)",
			    gai_strerror(error));
		source = src0;
		close(dummy);
	}

	Src.sin6_port = htons(0);
	if (bind(sndsock, (struct sockaddr *)&Src, Src.sin6_len) < 0)
		err(1, "bind");

	{
		socklen_t len;

		len = sizeof(Src);
		if (getsockname(sndsock, (struct sockaddr *)&Src, &len) < 0)
			err(1, "bind");
		srcport = ntohs(Src.sin6_port);
	}

	if (as_path) {
		asn = as_setup(as_server);
		if (asn == NULL) {
			warnx("as_setup failed, AS# lookups disabled");
			(void)fflush(stderr);
			as_path = 0;
		}
	 }

	/*
	 * Message to users
	 */
	if (getnameinfo((struct sockaddr *)&Dst, Dst.sin6_len, hbuf,
	    sizeof(hbuf), NULL, 0, NI_NUMERICHOST))
		strlcpy(hbuf, "(invalid)", sizeof(hbuf));
	fprintf(stderr, "traceroute6");
	fprintf(stderr, " to %s (%s)", hostname, hbuf);
	if (source)
		fprintf(stderr, " from %s", source);
	fprintf(stderr, ", %lu hops max, %lu byte packets\n",
	    max_hops, datalen);
	(void) fflush(stderr);

	if (first_hop > 1)
		printf("Skipping %lu intermediate hops\n", first_hop - 1);

	/*
	 * Main loop
	 */
	for (hops = first_hop; hops <= max_hops; ++hops) {
		struct in6_addr lastaddr;
		int got_there = 0;
		u_long unreachable = 0;

		printf("%2lu ", hops);
		memset(&lastaddr, 0, sizeof(lastaddr));
		for (probe = 0; probe < nprobes; ++probe) {
			int cc;
			struct timeval t1, t2;

			(void) gettimeofday(&t1, NULL);
			if (!useicmp && htons((in_port_t)(port + seq + 1)) == 0)
				seq++;
			send_probe(++seq, hops);
			while ((cc = wait_for_reply(rcvsock, &rcvmhdr))) {
				(void) gettimeofday(&t2, NULL);
				if ((i = packet_ok(&rcvmhdr, cc, seq))) {
					if (!IN6_ARE_ADDR_EQUAL(&Rcv.sin6_addr,
					    &lastaddr)) {
						print(&rcvmhdr, cc);
						lastaddr = Rcv.sin6_addr;
					}
					printf("  %g ms", deltaT(&t1, &t2));
					switch (i - 1) {
					case ICMP6_DST_UNREACH_NOROUTE:
						++unreachable;
						printf(" !N");
						break;
					case ICMP6_DST_UNREACH_ADMIN:
						++unreachable;
						printf(" !X");
						break;
					case ICMP6_DST_UNREACH_NOTNEIGHBOR:
						++unreachable;
						printf(" !S");
						break;
					case ICMP6_DST_UNREACH_ADDR:
						++unreachable;
						printf(" !H");
						break;
					case ICMP6_DST_UNREACH_NOPORT:
						if (rcvhlim >= 0 &&
						    rcvhlim <= 1)
							printf(" !");
						++got_there;
						break;
					}
					break;
				}
			}
			if (cc == 0)
				printf(" *");
			(void) fflush(stdout);
		}
		putchar('\n');
		if (got_there ||
		    (unreachable > 0 && unreachable >= ((nprobes + 1) / 2))) {
			exit(0);
		}
	}

	if (as_path)
		as_shutdown(asn);

	exit(0);
}

static ssize_t
wait_for_reply(int sock, struct msghdr *mhdr)
{
#ifdef HAVE_POLL
	struct pollfd pfd[1];
	ssize_t cc = 0;

	pfd[0].fd = sock;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;

	if (poll(pfd, 1, waittime * 1000) > 0)
		cc = recvmsg(rcvsock, mhdr, 0);

	return cc;
#else
	fd_set *fdsp;
	struct timeval wait;
	ssize_t cc = 0;
	int fdsn;

	fdsn = howmany(sock + 1, NFDBITS) * sizeof(fd_mask);
	if ((fdsp = (fd_set *)malloc(fdsn)) == NULL)
		err(1, "malloc");
	memset(fdsp, 0, fdsn);
	FD_SET(sock, fdsp);
	wait.tv_sec = waittime; wait.tv_usec = 0;

	if (select(sock+1, fdsp, (fd_set *)0, (fd_set *)0, &wait) > 0)
		cc = recvmsg(rcvsock, mhdr, 0);

	free(fdsp);
	return cc;
#endif
}

#if defined(IPSEC) && defined(IPSEC_POLICY_IPSEC)
static int
setpolicy(int so, const char *policy)
{
	char *buf;

	/*
	 * do not raise error even if setsockopt fails, kernel may have ipsec
	 * turned off.
	 */
	buf = ipsec_set_policy(policy, strlen(policy));
	if (buf == NULL) {
		warnx("%s", ipsec_strerror());
		return -1;
	}
	(void)setsockopt(so, IPPROTO_IPV6, IPV6_IPSEC_POLICY,
	    buf, ipsec_get_policylen(buf));

	free(buf);

	return 0;
}
#endif

static void
send_probe(int seq, u_long hops)
{
	struct timeval tv;
	struct tv32 tv32;
	int i;

	i = hops;
	if (setsockopt(sndsock, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
	    (char *)&i, sizeof(i)) < 0)
		warn("setsockopt IPV6_UNICAST_HOPS");

	Dst.sin6_port = htons(port + seq);
	(void) gettimeofday(&tv, NULL);
	tv32.tv32_sec = htonl(tv.tv_sec);
	tv32.tv32_usec = htonl(tv.tv_usec);

	if (useicmp) {
		struct icmp6_hdr *icp = (struct icmp6_hdr *)outpacket;

		icp->icmp6_type = ICMP6_ECHO_REQUEST;
		icp->icmp6_code = 0;
		icp->icmp6_cksum = 0;
		icp->icmp6_id = ident;
		icp->icmp6_seq = htons(seq);
		memcpy(((u_int8_t *)outpacket + ICMP6ECHOLEN), &tv32,
		    sizeof(tv32));
	} else {
		struct opacket *op = outpacket;

		op->seq = seq;
		op->hops = hops;
		memcpy(&op->tv, &tv32, sizeof tv32);
	}

	i = sendto(sndsock, (char *)outpacket, datalen , 0,
	    (struct sockaddr *)&Dst, Dst.sin6_len);
	if (i < 0 || i != (int)datalen)  {
		if (i < 0)
			warnx("sendto");
		printf("traceroute6: wrote %s %lu chars, ret=%d\n",
		    hostname, datalen, i);
		(void) fflush(stdout);
	}
}

static double
deltaT(struct timeval *t1p, struct timeval *t2p)
{
	double dt;

	dt = (double)(t2p->tv_sec - t1p->tv_sec) * 1000.0 +
	    (double)(t2p->tv_usec - t1p->tv_usec) / 1000.0;
	return dt;
}

/*
 * Convert an ICMP "type" field to a printable string.
 */
static const char *
pr_type(int t0)
{
	u_char t = t0 & 0xff;
	const char *cp;

	switch (t) {
	case ICMP6_DST_UNREACH:
		cp = "Destination Unreachable";
		break;
	case ICMP6_PACKET_TOO_BIG:
		cp = "Packet Too Big";
		break;
	case ICMP6_TIME_EXCEEDED:
		cp = "Time Exceeded";
		break;
	case ICMP6_PARAM_PROB:
		cp = "Parameter Problem";
		break;
	case ICMP6_ECHO_REQUEST:
		cp = "Echo Request";
		break;
	case ICMP6_ECHO_REPLY:
		cp = "Echo Reply";
		break;
	case ICMP6_MEMBERSHIP_QUERY:
		cp = "Group Membership Query";
		break;
	case ICMP6_MEMBERSHIP_REPORT:
		cp = "Group Membership Report";
		break;
	case ICMP6_MEMBERSHIP_REDUCTION:
		cp = "Group Membership Reduction";
		break;
	case ND_ROUTER_SOLICIT:
		cp = "Router Solicitation";
		break;
	case ND_ROUTER_ADVERT:
		cp = "Router Advertisement";
		break;
	case ND_NEIGHBOR_SOLICIT:
		cp = "Neighbor Solicitation";
		break;
	case ND_NEIGHBOR_ADVERT:
		cp = "Neighbor Advertisement";
		break;
	case ND_REDIRECT:
		cp = "Redirect";
		break;
	default:
		cp = "Unknown";
		break;
	}
	return cp;
}

static int
packet_ok(struct msghdr *mhdr, ssize_t cc, int seq)
{
	struct icmp6_hdr *icp;
	struct sockaddr_in6 *from = (struct sockaddr_in6 *)mhdr->msg_name;
	u_char type, code;
	char *buf = (char *)mhdr->msg_iov[0].iov_base;
	struct cmsghdr *cm;
	int *hlimp;
	char hbuf[NI_MAXHOST];

	if (cc < (ssize_t)sizeof(struct icmp6_hdr)) {
		if (verbose) {
			if (getnameinfo((struct sockaddr *)from, from->sin6_len,
			    hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0)
				strlcpy(hbuf, "invalid", sizeof(hbuf));
			printf("data too short (%zd bytes) from %s\n", cc,
			    hbuf);
		}
		return 0;
	}
	icp = (struct icmp6_hdr *)buf;

	/* get optional information via advanced API */
	rcvpktinfo = NULL;
	hlimp = NULL;
	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(mhdr); cm;
	    cm = (struct cmsghdr *)CMSG_NXTHDR(mhdr, cm)) {
		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_PKTINFO &&
		    cm->cmsg_len ==
		    CMSG_LEN(sizeof(struct in6_pktinfo)))
			rcvpktinfo = (struct in6_pktinfo *)(CMSG_DATA(cm));

		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_HOPLIMIT &&
		    cm->cmsg_len == CMSG_LEN(sizeof(int)))
			hlimp = (int *)CMSG_DATA(cm);
	}
	if (rcvpktinfo == NULL || hlimp == NULL) {
		warnx("failed to get received hop limit or packet info");
#if 0
		return 0;
#else
		rcvhlim = 0;	/*XXX*/
#endif
	}
	else
		rcvhlim = *hlimp;

	type = icp->icmp6_type;
	code = icp->icmp6_code;
	if ((type == ICMP6_TIME_EXCEEDED && code == ICMP6_TIME_EXCEED_TRANSIT)
	    || type == ICMP6_DST_UNREACH) {
		struct ip6_hdr *hip;
		struct udphdr *up;

		hip = (struct ip6_hdr *)(icp + 1);
		if ((up = get_udphdr(hip, (u_char *)(buf + cc))) == NULL) {
			if (verbose)
				warnx("failed to get upper layer header");
			return 0;
		}
		if (useicmp &&
		    ((struct icmp6_hdr *)up)->icmp6_id == ident &&
		    ((struct icmp6_hdr *)up)->icmp6_seq == htons(seq))
			return type == ICMP6_TIME_EXCEEDED ? -1 : code + 1;
		else if (!useicmp &&
		    up->uh_sport == htons(srcport) &&
		    up->uh_dport == htons(port + seq))
			return type == ICMP6_TIME_EXCEEDED ? -1 : code + 1;
	} else if (useicmp && type == ICMP6_ECHO_REPLY) {
		if (icp->icmp6_id == ident &&
		    icp->icmp6_seq == htons(seq))
			return ICMP6_DST_UNREACH_NOPORT + 1;
	}
	if (verbose) {
		char sbuf[NI_MAXHOST+1], dbuf[INET6_ADDRSTRLEN];
		u_int8_t *p;
		ssize_t i;

		if (getnameinfo((struct sockaddr *)from, from->sin6_len,
		    sbuf, sizeof(sbuf), NULL, 0, NI_NUMERICHOST) != 0)
			strlcpy(sbuf, "invalid", sizeof(sbuf));
		printf("\n%zd bytes from %s to %s", cc, sbuf,
		    rcvpktinfo ? inet_ntop(AF_INET6, &rcvpktinfo->ipi6_addr,
		    dbuf, sizeof(dbuf)) : "?");
		printf(": icmp type %d (%s) code %d\n", type, pr_type(type),
		    icp->icmp6_code);
		p = (u_int8_t *)(icp + 1);
#define WIDTH	16
		for (i = 0; i < cc; i++) {
			if (i % WIDTH == 0)
				printf("%04zx:", i);
			if (i % 4 == 0)
				printf(" ");
			printf("%02x", p[i]);
			if (i % WIDTH == WIDTH - 1)
				printf("\n");
		}
		if (cc % WIDTH != 0)
			printf("\n");
	}
	return 0;
}

/*
 * Increment pointer until find the UDP or ICMP header.
 */
static struct udphdr *
get_udphdr(struct ip6_hdr *ip6, u_char *lim)
{
	u_char *cp = (u_char *)ip6, nh;
	int hlen;

	if (cp + sizeof(*ip6) >= lim)
		return NULL;

	nh = ip6->ip6_nxt;
	cp += sizeof(struct ip6_hdr);

	while (lim - cp >= 8) {
		switch (nh) {
		case IPPROTO_ESP:
		case IPPROTO_TCP:
			return NULL;
		case IPPROTO_ICMPV6:
			return useicmp ? (struct udphdr *)cp : NULL;
		case IPPROTO_UDP:
			return useicmp ? NULL : (struct udphdr *)cp;
		case IPPROTO_FRAGMENT:
			hlen = sizeof(struct ip6_frag);
			nh = ((struct ip6_frag *)cp)->ip6f_nxt;
			break;
		case IPPROTO_AH:
			hlen = (((struct ip6_ext *)cp)->ip6e_len + 2) << 2;
			nh = ((struct ip6_ext *)cp)->ip6e_nxt;
			break;
		default:
			hlen = (((struct ip6_ext *)cp)->ip6e_len + 1) << 3;
			nh = ((struct ip6_ext *)cp)->ip6e_nxt;
			break;
		}

		cp += hlen;
	}

	return NULL;
}

static void
print(struct msghdr *mhdr, int cc)
{
	struct sockaddr_in6 *from = (struct sockaddr_in6 *)mhdr->msg_name;
	char hbuf[NI_MAXHOST];

	if (getnameinfo((struct sockaddr *)from, from->sin6_len,
	    hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0)
		strlcpy(hbuf, "invalid", sizeof(hbuf));
	if (as_path)
		printf(" [AS%u]", as_lookup(asn, hbuf, AF_INET6));
	if (nflag)
		printf(" %s", hbuf);
	else if (lflag)
		printf(" %s (%s)", inetname((struct sockaddr *)from), hbuf);
	else
		printf(" %s", inetname((struct sockaddr *)from));

	if (verbose) {
		printf(" %d bytes of data to %s", cc,
		    rcvpktinfo ?  inet_ntop(AF_INET6, &rcvpktinfo->ipi6_addr,
		    hbuf, sizeof(hbuf)) : "?");
	}
}

/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give
 * numeric value, otherwise try for symbolic name.
 */
static const char *
inetname(struct sockaddr *sa)
{
	static char line[NI_MAXHOST], domain[MAXHOSTNAMELEN + 1];
	static int first = 1;
	char *cp;

	if (first && !nflag) {
		first = 0;
		if (gethostname(domain, sizeof(domain)) == 0 &&
		    (cp = strchr(domain, '.')))
			(void) strlcpy(domain, cp + 1, sizeof(domain));
		else
			domain[0] = 0;
	}
	cp = NULL;
	if (!nflag) {
		if (getnameinfo(sa, sa->sa_len, line, sizeof(line), NULL, 0,
		    NI_NAMEREQD) == 0) {
			if ((cp = strchr(line, '.')) &&
			    !strcmp(cp + 1, domain))
				*cp = 0;
			cp = line;
		}
	}
	if (cp)
		return cp;

	if (getnameinfo(sa, sa->sa_len, line, sizeof(line), NULL, 0,
	    NI_NUMERICHOST) != 0)
		strlcpy(line, "invalid", sizeof(line));
	return line;
}

static void
usage(void)
{

	fprintf(stderr,
"Usage: %s [-adIlnrv] [-A as_server] [-f firsthop] [-m hoplimit]\n"
"\t[-p port] [-q probes] [-s src] [-w waittime] target [datalen]\n",
	getprogname());
	exit(1);
}
