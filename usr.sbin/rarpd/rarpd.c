/*	$NetBSD: rarpd.c,v 1.48 2003/05/15 14:50:02 itojun Exp $	*/

/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT(
    "@(#) Copyright (c) 1990 The Regents of the University of California.\n\
 All rights reserved.\n");
#endif /* not lint */

#ifndef lint
__RCSID("$NetBSD: rarpd.c,v 1.48 2003/05/15 14:50:02 itojun Exp $");
#endif


/*
 * rarpd - Reverse ARP Daemon
 *
 * Usage:	rarpd -a [-d|-f] [-l]
 *		rarpd [-d|-f] [-l] interface [...]
 */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#ifdef __NetBSD__
#include <net/if_ether.h>
#endif
#include <net/if_types.h>
#include <netinet/in.h>
#ifdef __NetBSD__
#include <netinet/if_inarp.h>
#else
#include <netinet/if_ether.h>
#endif

#include <arpa/inet.h>

#include <errno.h>
#include <dirent.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <util.h>
#include <ifaddrs.h>

#define FATAL		1	/* fatal error occurred */
#define NONFATAL	0	/* non fatal error occurred */

/*
 * The structure for each interface.
 */
struct if_info {
	int     ii_fd;		/* BPF file descriptor */
	u_char  ii_eaddr[6];	/* Ethernet address of this interface */
	u_int32_t  ii_ipaddr;	/* IP address of this interface */
	u_int32_t  ii_netmask;	/* subnet or net mask */
	char	*ii_name;	/* interface name */
	struct if_info *ii_alias;
	struct if_info *ii_next;
};
/*
 * The list of all interfaces that are being listened to.  rarp_loop()
 * "selects" on the descriptors in this list.
 */
struct if_info *iflist;

u_int32_t choose_ipaddr(u_int32_t **, u_int32_t, u_int32_t);
void	debug(const char *,...)
	__attribute__((__format__(__printf__, 1, 2)));
void	init_all(void);
void	init_one(char *, u_int32_t);
u_int32_t	ipaddrtonetmask(u_int32_t);
void	lookup_eaddr(char *, u_char *);
void	lookup_ipaddr(char *, u_int32_t *, u_int32_t *);
int	main(int, char **);
void	rarp_loop(void);
int	rarp_open(char *);
void	rarp_process(struct if_info *, u_char *);
void	rarp_reply(struct if_info *, struct ether_header *, u_int32_t,
		   struct hostent *);
void	rarperr(int, const char *,...)
	__attribute__((__format__(__printf__, 2, 3)));

#if defined(__NetBSD__)
#include "mkarp.h"
#else
void	update_arptab(u_char *, u_int32_t);
#endif

void	usage(void);

static int	bpf_open(void);
static int	rarp_check(u_char *, int);

#ifdef REQUIRE_TFTPBOOT
int	rarp_bootable(u_int32_t);
#endif

int     aflag = 0;		/* listen on "all" interfaces  */
int     dflag = 0;		/* print debugging messages */
int     fflag = 0;		/* don't fork */
int	lflag = 0;		/* log all replies */

int
main(int argc, char **argv)
{
	int     op;

	/* All error reporting is done through syslogs. */
	openlog("rarpd", LOG_PID, LOG_DAEMON);

	opterr = 0;
	while ((op = getopt(argc, argv, "adfl")) != -1) {
		switch (op) {
		case 'a':
			++aflag;
			break;

		case 'd':
			++dflag;
			break;

		case 'f':
			++fflag;
			break;

		case 'l':
			++lflag;
			break;

		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if ((aflag && argc != 0) || (!aflag && argc == 0))
		usage();

	if ((!fflag) && (!dflag)) {
		if (daemon(0, 0))
			rarperr(FATAL, "daemon");
		pidfile(NULL);
	}

	if (aflag)
		init_all();
	else {
		while (argc--)
			init_one(*argv++, INADDR_ANY);
	}

	rarp_loop();
	/* NOTREACHED */
	return (0);
}

/*
 * Add 'ifname' to the interface list.  Lookup its IP address and network
 * mask and Ethernet address, and open a BPF file for it.
 */
void
init_one(char *ifname, u_int32_t ipaddr)
{
	struct if_info *h;
	struct if_info *p;
	int fd;

	for (h = iflist; h != NULL; h = h->ii_next) {
		if (!strcmp(h->ii_name, ifname))
			break;
	}
	if (h == NULL) {
		fd = rarp_open(ifname);
		if (fd < 0)
			return;
	} else {
		fd = h->ii_fd;
	}

	p = (struct if_info *)malloc(sizeof(*p));
	if (p == 0) {
		rarperr(FATAL, "malloc: %s", strerror(errno));
		/* NOTREACHED */
	}
	p->ii_name = strdup(ifname);
	if (p->ii_name == 0) {
		rarperr(FATAL, "malloc: %s", strerror(errno));
		/* NOTREACHED */
	}
	if (h != NULL) {
		p->ii_alias = h->ii_alias;
		h->ii_alias = p;
	} else {
		p->ii_next = iflist;
		iflist = p;
	}

	p->ii_fd = fd;
	p->ii_ipaddr = ipaddr;
	lookup_eaddr(ifname, p->ii_eaddr);
	lookup_ipaddr(ifname, &p->ii_ipaddr, &p->ii_netmask);
}

/*
 * Initialize all "candidate" interfaces that are in the system
 * configuration list.  A "candidate" is up, not loopback and not
 * point to point.
 */
void
init_all(void)
{
	struct ifaddrs *ifap, *ifa, *p;

	if (getifaddrs(&ifap) != 0) {
		rarperr(FATAL, "getifaddrs: %s", strerror(errno));
		/* NOTREACHED */
	}

	p = NULL;
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
#define SIN(s)	((struct sockaddr_in *) (s))
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;
		if (p && !strcmp(p->ifa_name, ifa->ifa_name) &&
		    SIN(p->ifa_addr)->sin_addr.s_addr == SIN(ifa->ifa_addr)->sin_addr.s_addr)
			continue;
		p = ifa;
		if ((ifa->ifa_flags &
		     (IFF_UP | IFF_LOOPBACK | IFF_POINTOPOINT)) != IFF_UP)
			continue;
		init_one(ifa->ifa_name, SIN(ifa->ifa_addr)->sin_addr.s_addr);
#undef	SIN
	}
	freeifaddrs(ifap);
}

void
usage(void)
{
	(void) fprintf(stderr, "usage: rarpd -a [-d|-f] [-l]\n");
	(void) fprintf(stderr, "       rarpd [-d|-f] [-l] interface [...]\n");
	exit(1);
}

static int
bpf_open(void)
{
	int     fd;
	int     n = 0;
	char    device[sizeof "/dev/bpf000"];

	/* Go through all the minors and find one that isn't in use. */
	do {
		(void)sprintf(device, "/dev/bpf%d", n++);
		fd = open(device, O_RDWR);
	} while (fd < 0 && errno == EBUSY);

	if (fd < 0) {
		rarperr(FATAL, "%s: %s", device, strerror(errno));
		/* NOTREACHED */
	}
	return fd;
}
/*
 * Open a BPF file and attach it to the interface named 'device'.
 * Set immediate mode, and set a filter that accepts only RARP requests.
 */
int
rarp_open(char *device)
{
	int     fd;
	struct ifreq ifr;
	u_int   dlt;
	int     immediate;

	static struct bpf_insn insns[] = {
		BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 12),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ETHERTYPE_REVARP, 0, 3),
		BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 20),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ARPOP_REVREQUEST, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, 
		    sizeof(struct arphdr) + 
		    2 * ETHER_ADDR_LEN + 2 * sizeof(struct in_addr) +
		    sizeof(struct ether_header)),
		BPF_STMT(BPF_RET | BPF_K, 0),
	};
	static struct bpf_program filter = {
		sizeof insns / sizeof(insns[0]),
		insns
	};

	fd = bpf_open();

	/* Set immediate mode so packets are processed as they arrive. */
	immediate = 1;
	if (ioctl(fd, BIOCIMMEDIATE, &immediate) < 0) {
		rarperr(FATAL, "BIOCIMMEDIATE: %s", strerror(errno));
		/* NOTREACHED */
	}
	(void)strncpy(ifr.ifr_name, device, sizeof ifr.ifr_name - 1);
	ifr.ifr_name[sizeof ifr.ifr_name - 1] = '\0';
	if (ioctl(fd, BIOCSETIF, (caddr_t) & ifr) < 0) {
		if (aflag) {	/* for -a skip non-ethernet interfaces */
			close(fd);
			return(-1);
		}
		rarperr(FATAL, "BIOCSETIF: %s", strerror(errno));
		/* NOTREACHED */
	}
	/* Check that the data link layer is an Ethernet; this code won't work
	 * with anything else. */
	if (ioctl(fd, BIOCGDLT, (caddr_t) & dlt) < 0) {
		rarperr(FATAL, "BIOCGDLT: %s", strerror(errno));
		/* NOTREACHED */
	}
	if (dlt != DLT_EN10MB) {
		if (aflag) {	/* for -a skip non-ethernet interfaces */
			close(fd);
			return(-1);
		}
		rarperr(FATAL, "%s is not an ethernet", device);
		/* NOTREACHED */
	}
	/* Set filter program. */
	if (ioctl(fd, BIOCSETF, (caddr_t) & filter) < 0) {
		rarperr(FATAL, "BIOCSETF: %s", strerror(errno));
		/* NOTREACHED */
	}
	return fd;
}
/*
 * Perform various sanity checks on the RARP request packet.  Return
 * false on failure and log the reason.
 */
static int
rarp_check(u_char *p, int len)
{
	struct ether_header *ep = (struct ether_header *) p;
#ifdef __NetBSD__
	struct arphdr *ap = (struct arphdr *) (p + sizeof(*ep));
#else
	struct ether_arp *ap = (struct ether_arp *) (p + sizeof(*ep));
#endif

	if (len < sizeof(*ep) + sizeof(*ap)) {
		rarperr(NONFATAL, "truncated request");
		return 0;
	}
#ifdef __NetBSD__
	/* now that we know the fixed part of the ARP hdr is there: */
	if (len < sizeof(*ap) + 2 * ap->ar_hln + 2 * ap->ar_pln) {
		rarperr(NONFATAL, "truncated request");
		return 0;
	}
#endif
	/* XXX This test might be better off broken out... */
#ifdef __FreeBSD__
	/* BPF (incorrectly) returns this in host order. */
	if (ep->ether_type != ETHERTYPE_REVARP ||
#else
	if (ntohs (ep->ether_type) != ETHERTYPE_REVARP ||
#endif
#ifdef __NetBSD__
	    ntohs (ap->ar_hrd) != ARPHRD_ETHER ||
	    ntohs (ap->ar_op) != ARPOP_REVREQUEST ||
	    ntohs (ap->ar_pro) != ETHERTYPE_IP ||
	    ap->ar_hln != 6 || ap->ar_pln != 4) {
#else
	    ntohs (ap->arp_hrd) != ARPHRD_ETHER ||
	    ntohs (ap->arp_op) != ARPOP_REVREQUEST ||
	    ntohs (ap->arp_pro) != ETHERTYPE_IP ||
	    ap->arp_hln != 6 || ap->arp_pln != 4) {
#endif
		rarperr(NONFATAL, "request fails sanity check");
		return 0;
	}
#ifdef __NetBSD__
	if (memcmp((char *) &ep->ether_shost, ar_sha(ap), 6) != 0) {
#else
	if (memcmp((char *) &ep->ether_shost, ap->arp_sha, 6) != 0) {
#endif
		rarperr(NONFATAL, "ether/arp sender address mismatch");
		return 0;
	}
#ifdef __NetBSD__
	if (memcmp(ar_sha(ap), ar_tha(ap), 6) != 0) {
#else
	if (memcmp((char *) &ap->arp_sha, (char *) &ap->arp_tha, 6) != 0) {
#endif
		rarperr(NONFATAL, "ether/arp target address mismatch");
		return 0;
	}
	return 1;
}

/*
 * Loop indefinitely listening for RARP requests on the
 * interfaces in 'iflist'.
 */
void
rarp_loop(void)
{
	u_char *buf, *bp, *ep;
	int     cc, fd;
	fd_set  fds, listeners;
	int     bufsize, maxfd = 0;
	struct if_info *ii;

	if (iflist == 0) {
		rarperr(FATAL, "no interfaces");
		/* NOTREACHED */
	}
	if (ioctl(iflist->ii_fd, BIOCGBLEN, (caddr_t) & bufsize) < 0) {
		rarperr(FATAL, "BIOCGBLEN: %s", strerror(errno));
		/* NOTREACHED */
	}
	buf = (u_char *) malloc((unsigned) bufsize);
	if (buf == 0) {
		rarperr(FATAL, "malloc: %s", strerror(errno));
		/* NOTREACHED */
	}
	/*
         * Find the highest numbered file descriptor for select().
         * Initialize the set of descriptors to listen to.
         */
	FD_ZERO(&fds);
	for (ii = iflist; ii; ii = ii->ii_next) {
		FD_SET(ii->ii_fd, &fds);
		if (ii->ii_fd > maxfd)
			maxfd = ii->ii_fd;
	}
	while (1) {
		listeners = fds;
		if (select(maxfd + 1, &listeners, (struct fd_set *) 0,
			(struct fd_set *) 0, (struct timeval *) 0) < 0) {
			rarperr(FATAL, "select: %s", strerror(errno));
			/* NOTREACHED */
		}
		for (ii = iflist; ii; ii = ii->ii_next) {
			fd = ii->ii_fd;
			if (!FD_ISSET(fd, &listeners))
				continue;
		again:
			cc = read(fd, (char *) buf, bufsize);
			/* Don't choke when we get ptraced */
			if (cc < 0 && errno == EINTR)
				goto again;
			/* Due to a SunOS bug, after 2^31 bytes, the file
			 * offset overflows and read fails with EINVAL.  The
			 * lseek() to 0 will fix things. */
			if (cc < 0) {
				if (errno == EINVAL &&
				    (lseek(fd, 0, SEEK_CUR) + bufsize) < 0) {
					(void)lseek(fd, 0, 0);
					goto again;
				}
				rarperr(FATAL, "read: %s", strerror(errno));
				/* NOTREACHED */
			}
			/* Loop through the packet(s) */
#define bhp ((struct bpf_hdr *)bp)
			bp = buf;
			ep = bp + cc;
			while (bp < ep) {
				int caplen, hdrlen;

				caplen = bhp->bh_caplen;
				hdrlen = bhp->bh_hdrlen;
				debug("received packet on %s", ii->ii_name);

				if (rarp_check(bp + hdrlen, caplen))
					rarp_process(ii, bp + hdrlen);
				bp += BPF_WORDALIGN(hdrlen + caplen);
			}
		}
	}
}

#ifdef REQUIRE_TFTPBOOT

#ifndef TFTP_DIR
#define TFTP_DIR "/tftpboot"
#endif

/*
 * True if this server can boot the host whose IP address is 'addr'.
 * This check is made by looking in the tftp directory for the
 * configuration file.
 */
int
rarp_bootable(u_int32_t addr)
{
	struct dirent *dent;
	DIR *d;
	char    ipname[9];
	static DIR *dd = 0;

	(void)sprintf(ipname, "%08X", addr);
	/* If directory is already open, rewind it.  Otherwise, open it. */
	if (d = dd)
		rewinddir(d);
	else {
		if (chdir(TFTP_DIR) == -1) {
			rarperr(FATAL, "chdir: %s", strerror(errno));
			/* NOTREACHED */
		}
		d = opendir(".");
		if (d == 0) {
			rarperr(FATAL, "opendir: %s", strerror(errno));
			/* NOTREACHED */
		}
		dd = d;
	}
	while (dent = readdir(d))
		if (strncmp(dent->d_name, ipname, 8) == 0)
			return 1;
	return 0;
}
#endif /* REQUIRE_TFTPBOOT */

/*
 * Given a list of IP addresses, 'alist', return the first address that
 * is on network 'net'; 'netmask' is a mask indicating the network portion
 * of the address.
 */
u_int32_t
choose_ipaddr(u_int32_t **alist, u_int32_t net, u_int32_t netmask)
{

	for (; *alist; ++alist) {
		if ((**alist & netmask) == net)
			return **alist;
	}
	return 0;
}
/*
 * Answer the RARP request in 'pkt', on the interface 'ii'.  'pkt' has
 * already been checked for validity.  The reply is overlaid on the request.
 */
void
rarp_process(struct if_info *ii, u_char *pkt)
{
	struct ether_header *ep;
	struct hostent *hp;
	u_int32_t  target_ipaddr = 0;
	char    ename[MAXHOSTNAMELEN + 1];
	struct	in_addr in;

	ep = (struct ether_header *) pkt;

	if (ether_ntohost(ename, (struct ether_addr *)&ep->ether_shost) != 0) {
		debug("no IP address for %s",
		    ether_ntoa((struct ether_addr *)&ep->ether_shost));
		return;
	}
	ename[sizeof(ename)-1] = '\0';

	if ((hp = gethostbyname(ename)) == 0) {
		debug("gethostbyname(%s) failed: %s", ename,
		    hstrerror(h_errno));
		return;
	}

	/* Choose correct address from list. */
	if (hp->h_addrtype != AF_INET) {
		rarperr(FATAL, "cannot handle non IP addresses");
		/* NOTREACHED */
	}
	for (;; ii = ii->ii_alias) {
		target_ipaddr = choose_ipaddr((u_int32_t **) hp->h_addr_list,
		    ii->ii_ipaddr & ii->ii_netmask, ii->ii_netmask);
		if (target_ipaddr != 0)
			break;
		if (ii->ii_alias == NULL)
			break;
	}

	if (target_ipaddr == 0) {
		in.s_addr = ii->ii_ipaddr & ii->ii_netmask;
		rarperr(NONFATAL, "cannot find %s on net %s",
		    ename, inet_ntoa(in));
		return;
	}
#ifdef REQUIRE_TFTPBOOT
	if (rarp_bootable(htonl(target_ipaddr)))
#endif
		rarp_reply(ii, ep, target_ipaddr, hp);
#ifdef REQUIRE_TFTPBOOT
	else
		debug("%08X not bootable", htonl(target_ipaddr));
#endif
}
/*
 * Lookup the ethernet address of the interface attached to the BPF
 * file descriptor 'fd'; return it in 'eaddr'.
 */
void
lookup_eaddr(char *ifname, u_char *eaddr)
{
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_dl *sdl;

	if (getifaddrs(&ifap) != 0) {
		rarperr(FATAL, "getifaddrs: %s", strerror(errno));
		/* NOTREACHED */
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		sdl = (struct sockaddr_dl *)ifa->ifa_addr;
		if (sdl->sdl_family != AF_LINK || sdl->sdl_type != IFT_ETHER ||
		    sdl->sdl_alen != 6)
			continue;
		if (!strcmp(ifa->ifa_name, ifname)) {
			memmove((caddr_t)eaddr, (caddr_t)LLADDR(sdl), 6);
			debug("%s: %x:%x:%x:%x:%x:%x",
			    ifa->ifa_name, eaddr[0], eaddr[1],
			    eaddr[2], eaddr[3], eaddr[4], eaddr[5]);
			freeifaddrs(ifap);
			return;
		}
	}
	rarperr(FATAL, "lookup_eaddr: Never saw interface `%s'!", ifname);
	freeifaddrs(ifap);
}
/*
 * Lookup the IP address and network mask of the interface named 'ifname'.
 */
void
lookup_ipaddr(char *ifname, u_int32_t *addrp, u_int32_t *netmaskp)
{
	int     fd;

	/* Use datagram socket to get IP address. */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		rarperr(FATAL, "socket: %s", strerror(errno));
		/* NOTREACHED */
	}
	if (*addrp == INADDR_ANY) {
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		(void)strncpy(ifr.ifr_name, ifname, sizeof ifr.ifr_name);
		if (ioctl(fd, SIOCGIFADDR, (char *) &ifr) < 0) {
			rarperr(FATAL, "SIOCGIFADDR: %s", strerror(errno));
			/* NOTREACHED */
		}
		*addrp = ((struct sockaddr_in *) & ifr.ifr_addr)->sin_addr.s_addr;
		if (ioctl(fd, SIOCGIFNETMASK, (char *) &ifr) < 0) {
			perror("SIOCGIFNETMASK");
			exit(1);
		}
		*netmaskp = ((struct sockaddr_in *) & ifr.ifr_addr)->sin_addr.s_addr;
	} else {
		struct ifaliasreq ifra;
		memset(&ifra, 0, sizeof(ifra));
		(void)strncpy(ifra.ifra_name, ifname, sizeof ifra.ifra_name);
		((struct sockaddr_in *) & ifra.ifra_addr)->sin_family = AF_INET;
		((struct sockaddr_in *) & ifra.ifra_addr)->sin_addr.s_addr = *addrp;
		if (ioctl(fd, SIOCGIFALIAS, (char *) &ifra) < 0) {
			rarperr(FATAL, "SIOCGIFALIAS: %s", strerror(errno));
			/* NOTREACHED */
		}
		*addrp = ((struct sockaddr_in *) & ifra.ifra_addr)->sin_addr.s_addr;
		*netmaskp = ((struct sockaddr_in *) & ifra.ifra_mask)->sin_addr.s_addr;
	}
	/* If SIOCGIFNETMASK didn't work, figure out a mask from the IP
	 * address class. */
	if (*netmaskp == 0)
		*netmaskp = ipaddrtonetmask(*addrp);

	(void)close(fd);
}
/*
 * Poke the kernel arp tables with the ethernet/ip address combinataion
 * given.  When processing a reply, we must do this so that the booting
 * host (i.e. the guy running rarpd), won't try to ARP for the hardware
 * address of the guy being booted (he cannot answer the ARP).
 */
#ifndef __NetBSD__
void
update_arptab(u_char *ep, u_int32_t ipaddr)
{
	struct arpreq request;
	struct sockaddr_in *sin;

	request.arp_flags = 0;
	sin = (struct sockaddr_in *) & request.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = ipaddr;
	request.arp_ha.sa_family = AF_UNSPEC;
	/* This is needed #if defined(COMPAT_43) && BYTE_ORDER != BIG_ENDIAN,
	   because AF_UNSPEC is zero and the kernel assumes that a zero
	   sa_family means that the real sa_family value is in sa_len.  */
	request.arp_ha.sa_len = 16; /* XXX */
	memmove((char *) request.arp_ha.sa_data, (char *)ep, 6);

#if 0
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (ioctl(s, SIOCSARP, (caddr_t) & request) < 0) {
		rarperr(NONFATAL, "SIOCSARP: %s", strerror(errno));
	}
	(void)close(s);
#endif
}
#endif

/*
 * Build a reverse ARP packet and sent it out on the interface.
 * 'ep' points to a valid ARPOP_REVREQUEST.  The ARPOP_REVREPLY is built
 * on top of the request, then written to the network.
 *
 * RFC 903 defines the ether_arp fields as follows.  The following comments
 * are taken (more or less) straight from this document.
 *
 * ARPOP_REVREQUEST
 *
 * arp_sha is the hardware address of the sender of the packet.
 * arp_spa is undefined.
 * arp_tha is the 'target' hardware address.
 *   In the case where the sender wishes to determine his own
 *   protocol address, this, like arp_sha, will be the hardware
 *   address of the sender.
 * arp_tpa is undefined.
 *
 * ARPOP_REVREPLY
 *
 * arp_sha is the hardware address of the responder (the sender of the
 *   reply packet).
 * arp_spa is the protocol address of the responder (see the note below).
 * arp_tha is the hardware address of the target, and should be the same as
 *   that which was given in the request.
 * arp_tpa is the protocol address of the target, that is, the desired address.
 *
 * Note that the requirement that arp_spa be filled in with the responder's
 * protocol is purely for convenience.  For instance, if a system were to use
 * both ARP and RARP, then the inclusion of the valid protocol-hardware
 * address pair (arp_spa, arp_sha) may eliminate the need for a subsequent
 * ARP request.
 */
void
rarp_reply(struct if_info *ii, struct ether_header *ep, u_int32_t ipaddr,
	   struct hostent *hp)
{
	int     n;
#ifdef __NetBSD__
	struct arphdr *ap = (struct arphdr *) (ep + 1);
#else
	struct ether_arp *ap = (struct ether_arp *) (ep + 1);
#endif

	int     len;

#ifdef __NetBSD__
	(void)mkarp(ar_sha(ap), ipaddr);
#else
	update_arptab((u_char *) & ap->arp_sha, ipaddr);
#endif

	/* Build the rarp reply by modifying the rarp request in place. */
#ifdef __FreeBSD__
	/* BPF (incorrectly) wants this in host order. */
	ep->ether_type = ETHERTYPE_REVARP;
#else
	ep->ether_type = htons(ETHERTYPE_REVARP);
#endif
#ifdef __NetBSD__
	ap->ar_hrd = htons(ARPHRD_ETHER);
	ap->ar_pro = htons(ETHERTYPE_IP);
	ap->ar_op = htons(ARPOP_REVREPLY);

	memmove((char *) &ep->ether_dhost, ar_sha(ap), 6);
	memmove((char *) &ep->ether_shost, (char *) ii->ii_eaddr, 6);
	memmove(ar_sha(ap), (char *) ii->ii_eaddr, 6);

	memmove(ar_tpa(ap), (char *) &ipaddr, 4);
	/* Target hardware is unchanged. */
	memmove(ar_spa(ap), (char *) &ii->ii_ipaddr, 4);

	len = sizeof(*ep) + sizeof(*ap) + 
	    2 * ap->ar_pln + 2 * ap->ar_hln;
#else
	ap->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	ap->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	ap->arp_op = htons(ARPOP_REVREPLY);

	memmove((char *) &ep->ether_dhost, (char *) &ap->arp_sha, 6);
	memmove((char *) &ep->ether_shost, (char *) ii->ii_eaddr, 6);
	memmove((char *) &ap->arp_sha, (char *) ii->ii_eaddr, 6);

	memmove((char *) ap->arp_tpa, (char *) &ipaddr, 4);
	/* Target hardware is unchanged. */
	memmove((char *) ap->arp_spa, (char *) &ii->ii_ipaddr, 4);

	len = sizeof(*ep) + sizeof(*ap);
#endif

	debug("%s asked; %s replied",
	    ether_ntoa((struct ether_addr *)ar_tha(ap)), hp->h_name);
	if (lflag)
		syslog(LOG_INFO, "%s asked; %s replied",
		    ether_ntoa((struct ether_addr *)ar_tha(ap)), hp->h_name);
	n = write(ii->ii_fd, (char *) ep, len);
	if (n != len) {
		rarperr(NONFATAL, "write: only %d of %d bytes written", n, len);
	}
}
/*
 * Get the netmask of an IP address.  This routine is used if
 * SIOCGIFNETMASK doesn't work.
 */
u_int32_t
ipaddrtonetmask(u_int32_t addr)
{

	if (IN_CLASSA(addr))
		return IN_CLASSA_NET;
	if (IN_CLASSB(addr))
		return IN_CLASSB_NET;
	if (IN_CLASSC(addr))
		return IN_CLASSC_NET;
	rarperr(FATAL, "unknown IP address class: %08X", addr);
	/* NOTREACHED */
	return(-1);
}

#include <stdarg.h>

void
rarperr(int fatal, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	if (dflag) {
		if (fatal)
			(void)fprintf(stderr, "rarpd: error: ");
		else
			(void)fprintf(stderr, "rarpd: warning: ");
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
		va_start(ap, fmt);
		(void)fprintf(stderr, "\n");
	}
	vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);
	if (fatal)
		exit(1);
	/* NOTREACHED */
}

void
debug(const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	if (dflag) {
		(void)fprintf(stderr, "rarpd: ");
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
		va_start(ap, fmt);
		(void)fprintf(stderr, "\n");
	}
	vsyslog(LOG_WARNING, fmt, ap);
	va_end(ap);
}
