/*	$NetBSD: isibootd.c,v 1.3 2012/04/02 09:01:30 nisimura Exp $	*/
/*	Id: isiboot.c,v 1.2 1999/12/26 14:33:33 nisimura Exp 	*/

/*-
 * Copyright (c) 2000, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/endian.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#include <err.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <paths.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#define	TRACE(l, x) if ((l) <= dbg) printf x

/*
 * Integrated Solutions Inc. "ISIBOOT" boot enet protocol.
 *
 * Following data format depends on m68k order, and aligned harmful
 * to RISC processors.
 */
#define	ISIBOOT_FRAMETYPE	0x80df
#define	ISIBOOT_FRAMELEN	1468
struct frame {
	uint8_t dst[ETHER_ADDR_LEN];
	uint8_t src[ETHER_ADDR_LEN];
	uint16_t type;
	uint16_t pad_0;
	uint16_t seqno;
	uint8_t opcode;
	uint8_t pad_1;
	uint8_t pos[4];
	uint8_t siz[4];
	uint8_t data[ISIBOOT_FRAMELEN - 28];
} __packed;

struct station {
	int 	fd;
	char	name[MAXHOSTNAMELEN];
	char	ifname[IFNAMSIZ];
	uint8_t addr[ETHER_ADDR_LEN];
} station;

struct session {
	struct session *next;
	int state;
	FILE *file;
	uint8_t addr[ETHER_ADDR_LEN];
} *activelist, *freelist;
#define	NEWPOOL 10

#define	WAITING	0	/* implicit state after receiving the first frame */
#define	OPENING	1	/* waiting for OPEN after CONNECT is received */
#define	TRANSFER 2	/* data transferring state after OPEN is well done */
static __unused const char *state[] = { "WAITING", "OPENING", "TRANSFER" };

#define	CONNECT	0
#define	OPEN	1
#define	READ	2
#define	CLOSE	4
static __unused const char *op[] =
    { "CONNECT", "OPEN", "READ", "WRITE", "CLOSE", "FIND" };

static void createbpfport(char *, uint8_t **, size_t *, struct station *);
static struct session *search(uint8_t *);
static void closedown(struct session *);
static void makepool(void);
static char *etheraddr(uint8_t *);
static int pickif(char *, uint8_t *);
static __dead void usage(void);

#define	ISIBOOT_FRAME(buf)	((buf) + ((struct bpf_hdr *)(buf))->bh_hdrlen)

#define	PATH_DEFBOOTDIR	"/tftpboot"

int
main(int argc, char *argv[])
{
	int cc, dbg, dflag;
	size_t iolen;
	uint32_t pos, siz;
	size_t nread;
	char *ifname, *p;
	const char *bootwd, *servername, *filename;
	uint8_t *iobuf;
	struct session *cp;
	struct frame *fp;
	struct pollfd pollfd;
	char clientname[MAXHOSTNAMELEN + 1];
	struct hostent *clientent;

	ifname = NULL;
	bootwd = PATH_DEFBOOTDIR;
	dbg = 0;
	dflag = 0;
	while ((cc = getopt(argc, argv, "i:s:d:")) != -1) {
		switch (cc) {
		case 'i':
			ifname = optarg;
			break;
		case 's':
			bootwd = optarg;
			break;
		case 'd':
			dflag = 1;
			dbg = atoi(optarg);
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argv += optind;
	argc -= optind;

	if (geteuid() != 0)
		warnx("WARNING: run by non root priviledge");

	memset(station.name, 0, sizeof(station.name));
	gethostname(station.name, sizeof(station.name) - 1);
	if ((p = strchr(station.name, '.')) != NULL)
		*p = '\0';

	createbpfport(ifname, &iobuf, &iolen, &station);

	TRACE(1, ("Using interface: %s (%s)\n",
	    station.ifname, etheraddr(station.addr)));

	if (!dflag) {
		if (daemon(0, 0))
			err(EXIT_FAILURE, "can not start daemon");
#ifdef __NetBSD__
		pidfile(NULL);
#endif
	}

	if (chdir(bootwd) < 0)
		err(EXIT_FAILURE, "can not chdir to %s", bootwd);

	pollfd.fd = station.fd;
	pollfd.events = POLLIN;
	for (;;) {
		poll(&pollfd, 1, INFTIM);
		read(pollfd.fd, iobuf, iolen);	/* returns 1468 */
		fp = (struct frame *)ISIBOOT_FRAME(iobuf);

		/* ignore own TX packets */
		if (memcmp(fp->src, station.addr, ETHER_ADDR_LEN) == 0)
			continue;

		/* check if the received Ethernet address is in ethers(5) */
		if (ether_ntohost(clientname, (struct ether_addr *)fp->src)) {
			TRACE(3, ("'%s' is not in ethers(5)\n",
			    etheraddr(fp->src)));
			continue;	
		}
		/* check if the client has a valid hostname */
		clientname[sizeof(clientname) - 1] = '\0';
		clientent = gethostbyname(clientname);
		if (clientent == NULL || clientent->h_addrtype != AF_INET) {
			TRACE(3, ("'%s' is not a valid host\n", clientname));
			continue;	
		}

		cp = search(fp->src);
		TRACE(2, ("[%s] ", etheraddr(fp->src)));
		switch (cp->state) {
		case WAITING:
			if (fp->opcode != CONNECT) {
				TRACE(2, ("not connected\n"));
				continue;
			}
			/* check if specified servername is mine */
			fp->data[sizeof(fp->data) - 1] = '\0';
			servername = (char *)fp->data;
			if (strcmp(servername, station.name) != 0) {
				TRACE(3, ("'%s' not for me\n", servername));
				continue;
			}
			cp->state = OPENING;
			TRACE(2, ("new connection\n"));
			break;
		case OPENING:
			if (fp->opcode != OPEN)
				goto aborting;	/* out of phase */

			/* don't allow files outside the specified dir */
			fp->data[sizeof(fp->data) - 1] = '\0';
			filename = strrchr((char *)fp->data, '/');
			if (filename != NULL)
				filename++;
			else
				filename = (char *)fp->data;

			cp->file = fopen(filename, "r");
			if (cp->file == NULL) {
				TRACE(1, ("failed to open '%s'\n", filename));
				goto closedown;	/* no such file */
			}
			cp->state = TRANSFER;
			TRACE(2, ("open '%s'\n", filename));
			break;
		case TRANSFER:
			if (fp->opcode == CLOSE) {
				TRACE(2, ("connection closed\n"));
				goto closedown;	/* close request */
			}
			if (fp->opcode != READ)
				goto aborting;	/* out of phase */
			siz = be32dec(fp->siz);
			pos = be32dec(fp->pos);
			nread = siz;
			if (nread > sizeof(fp->data) ||
			    fseek(cp->file, pos, 0L) < 0 ||
			    fread(fp->data, 1, nread, cp->file) < nread) {
				be32enc(fp->siz, 0); /* corrupted file */
			}
			TRACE(3, ("%u@%u\n", siz, pos));
			break;
 aborting:
			TRACE(1, ("out of phase\n"));
 closedown:
			closedown(cp);
			fp->opcode = CLOSE;
			break;
		}
		memcpy(fp->dst, fp->src, ETHER_ADDR_LEN);
		memcpy(fp->src, station.addr, ETHER_ADDR_LEN);
		write(pollfd.fd, fp, ISIBOOT_FRAMELEN);
	}
	/* NOTREACHED */
}

struct session *
search(uint8_t *client)
{
	struct session *cp;

	for (cp = activelist; cp; cp = cp->next) {
		if (memcmp(client, cp->addr, ETHER_ADDR_LEN) == 0)
			return cp;
	}
	if (freelist == NULL)
		makepool();
	cp = freelist;
	freelist = cp->next;	
	cp->next = activelist;
	activelist = cp;

	cp->state = WAITING;
	cp->file = NULL;
	memcpy(cp->addr, client, ETHER_ADDR_LEN);
	return cp;
}

void
closedown(struct session *cp)
{
	struct session *cpp;

	cpp = activelist;
	if (cpp == cp)
		activelist = cp->next;
	else {
		do {
			if (cpp->next == cp)
				break;
		} while (NULL != (cpp = cpp->next)); /* should never happen */
		cpp->next = cp->next;
	}
	cp->next = freelist;
	freelist = cp;

	if (cp->file != NULL)
		fclose(cp->file);
	cp->file = NULL;
	memset(cp->addr, 0, ETHER_ADDR_LEN);
}

void
makepool(void)
{
	struct session *cp;
	int n;

	freelist = calloc(NEWPOOL, sizeof(struct session));
	if (freelist == NULL)
		err(EXIT_FAILURE, "Can't allocate pool");
	cp = freelist;
	for (n = 0; n < NEWPOOL - 1; n++) {
		cp->next = cp + 1;
		cp++;
	}
}

char *
etheraddr(uint8_t *e)
{
	static char address[sizeof("xx:xx:xx:xx:xx:xx")];

	snprintf(address, sizeof(address), "%02x:%02x:%02x:%02x:%02x:%02x",
	    e[0], e[1], e[2], e[3], e[4], e[5]);
	return address;
}

static struct bpf_insn bpf_insn[] = {
	{ BPF_LD|BPF_H|BPF_ABS,  0, 0, offsetof(struct frame, type) },
	{ BPF_JMP|BPF_JEQ|BPF_K, 0, 1, ISIBOOT_FRAMETYPE },
	{ BPF_RET|BPF_K,         0, 0, ISIBOOT_FRAMELEN },
	{ BPF_RET|BPF_K,         0, 0, 0x0 }
};
static struct bpf_program bpf_pgm = {
	sizeof(bpf_insn) / sizeof(bpf_insn[0]),
	bpf_insn
};

void
createbpfport(char *ifname, uint8_t **iobufp, size_t *iolenp,
    struct station *st)
{
	struct ifreq ifr;
	int fd;
	u_int type;
	size_t buflen;
	uint8_t dladdr[ETHER_ADDR_LEN], *buf;
#ifdef BIOCIMMEDIATE
	u_int flag;
#endif
#ifndef _PATH_BPF
	char devbpf[PATH_MAX];
	int n;
#endif

#ifdef _PATH_BPF
	fd = open(_PATH_BPF, O_RDWR, 0);
#else
	n = 0;
	do {
		snprintf(devbpf, sizeof(devbpf), "/dev/bpf%d", n++);
		fd = open(devbpf, O_RDWR, 0);
	} while (fd == -1 && errno == EBUSY);
#endif
	if (fd == -1)
		err(EXIT_FAILURE, "No bpf device available");
	memset(&ifr, 0, sizeof(ifr));
	if (ifname != NULL)
		strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (pickif(ifr.ifr_name, dladdr) < 0)
		errx(EXIT_FAILURE,
		    "No network interface available: %s\n", ifr.ifr_name);

	ioctl(fd, BIOCSETIF, &ifr);
	ioctl(fd, BIOCGDLT, &type);	/* XXX - should check whether EN10MB */
#ifdef BIOCIMMEDIATE
	flag = 1;
	ioctl(fd, BIOCIMMEDIATE, &flag);
#endif
	ioctl(fd, BIOCGBLEN, &buflen);
	ioctl(fd, BIOCSETF, &bpf_pgm);

	buf = malloc(buflen);
	if (buf == NULL)
		err(EXIT_FAILURE, "Can't allocate buffer");
	*iobufp = buf;
	*iolenp = buflen;
	st->fd = fd;
	strlcpy(st->ifname, ifr.ifr_name, sizeof(st->ifname));
	memcpy(st->addr, dladdr, ETHER_ADDR_LEN);
}

int
pickif(char *xname, uint8_t *dladdr)
{
#define	MATCH(x, v) ((v) == ((v) & (x)))
#ifndef CLLADDR
#define CLLADDR(s) ((const char *)((s)->sdl_data + (s)->sdl_nlen))
#endif
	int s, error;
	struct ifaddrs *ifaddrs, *ifa;
	const struct sockaddr_dl *sdl;

	error = -1;
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		return error;
	if (getifaddrs(&ifaddrs) == -1)
		goto out;

	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_LINK) {
			if (MATCH(ifa->ifa_flags, IFF_UP | IFF_BROADCAST)) {
				sdl = (const struct sockaddr_dl *)ifa->ifa_addr;
				if (xname[0] == '\0') {
					strlcpy(xname, ifa->ifa_name,
					    IFNAMSIZ);
					memcpy(dladdr, CLLADDR(sdl),
					    ETHER_ADDR_LEN);
					error = 0;
					break;
				} else if (strcmp(xname, ifa->ifa_name) == 0) {
					memcpy(dladdr, CLLADDR(sdl),
					    ETHER_ADDR_LEN);
					error = 0;
					break;
				}
			}
		}
	}
	freeifaddrs(ifaddrs);
 out:
	close(s);
	return error;
#undef MATCH
}

void
usage(void)
{

	fprintf(stderr,
	    "usage: %s [-d tracelevel] [-i interface] [-s directory]\n",
	    getprogname());
	exit(0);
}
