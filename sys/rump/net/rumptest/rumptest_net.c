/*	$NetBSD: rumptest_net.c,v 1.17 2010/01/26 17:52:21 pooka Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by then
 * Finnish Cultural Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/module.h>
#include <sys/mbuf.h>
#include <sys/time.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>

#include <arpa/inet.h>
#include <net/bpf.h>
#include <net/ethertypes.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

#define DEST_ADDR "204.152.190.12"	/* www.NetBSD.org */
#define DEST_PORT 80			/* take a wild guess */

static in_addr_t youraddr;
static in_addr_t myaddr;

#ifdef FULL_NETWORK_STACK
#define IFNAME "virt0" /* XXX: hardcoded */
#else
#define IFNAME "sockin0"
#endif

#ifdef FULL_NETWORK_STACK
/*
 * If we are running with the full networking stack, configure
 * virtual interface.  For this to currently work, you *must* have
 * tap0 bridged with your main networking interface.  Essentially
 * the following steps are required:
 *
 * # ifconfig tap0 create
 * # ifconfig tap0 up
 * # ifconfig bridge0 create
 * # brconfig bridge0 add tap0 add yourrealif0
 * # brconfig bridge0 up
 *
 * The usability is likely to be improved later.
 */
#define MYADDR "10.181.181.0"
#define MYBCAST "10.181.181.255"
#define MYMASK "255.255.255.0"
#define MYGW "10.181.181.1"

static void
configure_interface(void)
{
        struct sockaddr_in *sin;
	struct sockaddr_in sinstore;
	struct ifaliasreq ia;
	ssize_t len;
	struct {
		struct rt_msghdr m_rtm;
		uint8_t m_space;
	} m_rtmsg;
#define rtm m_rtmsg.m_rtm
	uint8_t *bp = &m_rtmsg.m_space;
	int s, rv;

	if ((rv = rump_pub_virtif_create(0)) != 0) {
		printf("could not configure interface %d\n", rv);
		exit(1);
	}

	/* get a socket for configuring the interface */
	s = rump_sys_socket(PF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		err(1, "configuration socket");

	srandom(time(NULL));
	myaddr = inet_addr(MYADDR);
	myaddr = htonl(ntohl(myaddr) + (random() % 126 + 1));

	/* fill out struct ifaliasreq */
	memset(&ia, 0, sizeof(ia));
	strcpy(ia.ifra_name, IFNAME);
	sin = (struct sockaddr_in *)&ia.ifra_addr;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_addr.s_addr = myaddr;
	printf("using address %s\n", inet_ntoa(sin->sin_addr));

	sin = (struct sockaddr_in *)&ia.ifra_broadaddr;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_addr.s_addr = inet_addr(MYBCAST);

	sin = (struct sockaddr_in *)&ia.ifra_mask;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_addr.s_addr = inet_addr(MYMASK);

	/* toss to the configuration socket and see what it thinks */
	rv = rump_sys_ioctl(s, SIOCAIFADDR, &ia);
	if (rv)
		err(1, "SIOCAIFADDR");
	rump_sys_close(s);

	/* open routing socket and configure our default router */
	s = rump_sys_socket(PF_ROUTE, SOCK_RAW, 0);
	if (s == -1)
		err(1, "routing socket");

	/* create routing message */
        memset(&m_rtmsg, 0, sizeof(m_rtmsg));
        rtm.rtm_type = RTM_ADD;
        rtm.rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
        rtm.rtm_version = RTM_VERSION;
        rtm.rtm_seq = 2;
        rtm.rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;

	/* dst */
        memset(&sinstore, 0, sizeof(sinstore));
        sinstore.sin_family = AF_INET;
        sinstore.sin_len = sizeof(sinstore);
        memcpy(bp, &sinstore, sizeof(sinstore));
        bp += sizeof(sinstore);

	/* gw */
        memset(&sinstore, 0, sizeof(sinstore));
        sinstore.sin_family = AF_INET;
        sinstore.sin_len = sizeof(sinstore);
        sinstore.sin_addr.s_addr = inet_addr(MYGW);
        memcpy(bp, &sinstore, sizeof(sinstore));
        bp += sizeof(sinstore);

	/* netmask */
        memset(&sinstore, 0, sizeof(sinstore));
        sinstore.sin_family = AF_INET;
        sinstore.sin_len = sizeof(sinstore);
        memcpy(bp, &sinstore, sizeof(sinstore));
        bp += sizeof(sinstore);

        len = bp - (uint8_t *)&m_rtmsg;
        rtm.rtm_msglen = len;

	/* stuff that to the routing socket and wait for happy days */
	if (rump_sys_write(s, &m_rtmsg, len) != len)
		err(1, "routing incomplete");
	rump_sys_close(s);
}
#endif /* FULL_NETWORK_STACK */

static void
dump_ether(uint8_t *data, size_t caplen)
{
	char fmt[64];
	struct ether_header *ehdr;
	struct ip *ip;
	struct tcphdr *tcph;

	ehdr = (void *)data;
	switch (ntohs(ehdr->ether_type)) {
	case ETHERTYPE_ARP:
		printf("ARP\n");
		break;
	case ETHERTYPE_IP:
		printf("IP, ");
		ip = (void *)((uint8_t *)ehdr + sizeof(*ehdr));
		printf("version %d, proto ", ip->ip_v);
		if (ip->ip_p == IPPROTO_TCP)
			printf("TCP");
		else if (ip->ip_p == IPPROTO_UDP)
			printf("UDP");
		else
			printf("unknown");
		printf("\n");

		/*
		 * if it's the droids we're looking for,
		 * print segment contents.
		 */
		if (ip->ip_src.s_addr != youraddr ||
		    ip->ip_dst.s_addr != myaddr ||
		    ip->ip_p != IPPROTO_TCP)
			break;
		tcph = (void *)((uint8_t *)ip + (ip->ip_hl<<2));
		if (ntohs(tcph->th_sport) != 80)
			break;

		printf("requested data:\n");
		sprintf(fmt, "%%%ds\n",
		    ntohs(ip->ip_len) - (ip->ip_hl<<2));
		printf(fmt, (char *)tcph + (tcph->th_off<<2));

		break;
	case ETHERTYPE_IPV6:
		printf("IPv6\n");
		break;
	default:
		printf("unknown type 0x%04x\n",
		    ntohs(ehdr->ether_type));
		break;
	}
}

/* in luck we trust (i.e. true story on how an os works) */
static void
dump_nodlt(uint8_t *data, size_t caplen)
{
	char buf[32768];

	/* +4 = skip AF */
	strvisx(buf, (const char *)data+4, caplen-4, 0);
	printf("%s\n", buf);
}

static void
dobpfread(void)
{
	struct bpf_program bpf_prog;
	struct bpf_insn bpf_ins;
	struct bpf_hdr *bhdr;
	void *buf;
	struct ifreq ifr;
	int bpfd, modfd;
	u_int bpflen, x, dlt;
	ssize_t n;

	bpfd = rump_sys_open("/dev/bpf", O_RDWR);

	/* fail?  try to load kernel module */
	if (bpfd == -1) {
		modctl_load_t ml;

		/* XXX: struct stat size */
		modfd = open("./bpf.kmod", O_RDONLY);
		if (modfd == -1)
			err(1, "no bpf, no bpf kmod");
		close(modfd);

		rump_pub_etfs_register("/bpf.kmod",
		    "./bpf.kmod", RUMP_ETFS_REG);
		ml.ml_filename = "/bpf.kmod";
		ml.ml_flags = 0;
		ml.ml_props = NULL;
		ml.ml_propslen = 0;

		if (rump_sys_modctl(MODCTL_LOAD, &ml) == -1)
			err(1, "load bpf module");
		/* XXX: I "know" it's 256 XXX */
		rump_sys_mknod("/dev/bpf", 0777 | S_IFCHR, makedev(256,0));

		bpfd = rump_sys_open("/dev/bpf", O_RDWR);
		if (bpfd == -1)
			err(1, "open bpf");
	}

	if (rump_sys_ioctl(bpfd, BIOCGBLEN, &bpflen) == -1)
		err(1, "BIOCGBLEN");

	buf = malloc(bpflen);
	if (buf == NULL)
		err(1, "malloc bpfbuf");

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, IFNAME);

	if (rump_sys_ioctl(bpfd, BIOCSETIF, &ifr) == -1)
		err(1, "BIOCSETIF");

	/* accept all packets up to 9000 bytes */
	memset(&bpf_ins, 0, sizeof(bpf_ins));
	bpf_ins.code = BPF_RET + BPF_K;
	bpf_ins.k = 9000;
	bpf_prog.bf_len = 1;
	bpf_prog.bf_insns = &bpf_ins;
	if (rump_sys_ioctl(bpfd, BIOCSETF, &bpf_prog) == -1)
		err(1, "BIOCSETF");

	/* we want all packets delivered immediately */
	x = 1;
	if (rump_sys_ioctl(bpfd, BIOCIMMEDIATE, &x) == -1)
		err(1, "BIOCIMMEDIATE");

	if (rump_sys_ioctl(bpfd, BIOCGDLT, &dlt) == -1)
		err(1, "BIOCGDLT");
	
	for (;;) {

		memset(buf, 0, bpflen);
		n = rump_sys_read(bpfd, buf, bpflen);
		if (n == 0) {
			printf("EOF\n");
			exit(0);
		} else if (n == -1) {
			err(1, "read");
		}

		bhdr = buf;
		while (bhdr->bh_caplen) {
			uint8_t *data;

			printf("got packet, caplen %d\n", bhdr->bh_caplen);
			data = (void *)((uint8_t *)bhdr + bhdr->bh_hdrlen);

			if (dlt == DLT_NULL)
				dump_nodlt(data, bhdr->bh_caplen);
			else
				dump_ether(data, bhdr->bh_caplen);

			bhdr = (void *)((uint8_t *)bhdr +
			    BPF_WORDALIGN(bhdr->bh_hdrlen + bhdr->bh_caplen));
		}
	}
}

static void
printstats(void)
{
	struct mbstat mbstat;
	int ctl[] = { CTL_KERN, KERN_MBUF, MBUF_STATS };
	int totalmbuf = 0;
	size_t mbslen = sizeof(struct mbstat);
	unsigned i;

	if (rump_sys___sysctl(ctl, __arraycount(ctl), &mbstat, &mbslen,
	    NULL, 0) == -1)
		return;

	printf("  mbuf count:\n");
	for (i = 0; i < __arraycount(mbstat.m_mtypes); i++) {
		if (mbstat.m_mtypes[i] == 0)
			continue;
		printf("%s (%d) mbuf count %d\n",
		    i == MT_DATA ? "data" : "unknown", i, mbstat.m_mtypes[i]);
		totalmbuf += mbstat.m_mtypes[i];
	}
	printf("total mbufs: %d\n", totalmbuf);
}

int
main(int argc, char *argv[])
{
	char buf[65536];
	struct sockaddr_in sin;
	struct timeval tv;
	ssize_t n;
	size_t off;
	int s;

	if (rump_init())
		errx(1, "rump_init failed");

#ifdef FULL_NETWORK_STACK
	configure_interface();
#endif

	s = rump_sys_socket(PF_INET, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "can't open socket");

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (rump_sys_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
	    &tv, sizeof(tv)) == -1)
		err(1, "setsockopt");

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(DEST_PORT);
	sin.sin_addr.s_addr = youraddr = inet_addr(DEST_ADDR);

	if (rump_sys_connect(s, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		err(1, "connect failed");
	}

	printstats();

	printf("connected\n");

	strcpy(buf, "GET / HTTP/1.0\n\n");
	n = rump_sys_write(s, buf, strlen(buf));
	if (n != (ssize_t)strlen(buf))
		err(1, "wrote only %zd vs. %zu\n",
		    n, strlen(buf));

	if (argc > 1)
		dobpfread();

	/* wait for mbufs to accumulate.  hacky, but serves purpose.  */
	sleep(1);
	printstats();
	sleep(1);
	
	memset(buf, 0, sizeof(buf));
	for (off = 0; off < sizeof(buf) && n > 0;) {
		n = rump_sys_read(s, buf+off, sizeof(buf)-off);
		if (n > 0)
			off += n;
	}
	printf("read %zd (max %zu):\n", off, sizeof(buf));
	printf("%s", buf);

	rump_sys_close(s);
	sleep(1);

	return 0;
}
