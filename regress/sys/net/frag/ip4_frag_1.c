/*	$NetBSD: ip4_frag_1.c,v 1.2 2010/10/03 19:41:25 rmind Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Mindaugas Rasiukevicius <rmind at NetBSD org>.
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

/*
 * Test various cases of IPv4 reassembly.
 */

#include <sys/cdefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <err.h>

#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#define	IP_SIZE			sizeof(struct ip)
#define	ICMP_SIZE		offsetof(struct icmp, icmp_ip)

static void *
xalloc(size_t sz)
{
	void *ptr = calloc(1, sz);

	if (ptr == NULL) {
		err(EXIT_FAILURE, "calloc");
	}
	return ptr;
}

static int
create_socket(void)
{
	int s, val = 1;

	s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (s == -1) {
		err(EXIT_FAILURE, "socket");
	}
	if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val)) == -1) {
		err(EXIT_FAILURE, "setsockopt");
	}
	return s;
}

void *
create_ip_packet(in_addr_t target, int foff, size_t addlen)
{
	struct ip *ip;
	unsigned int n;
	uint8_t *data;
	int first;

	/* Header at zero offset - first fragment. */
	first = ((foff & IP_MF) == 0);
	if (first) {
		assert(addlen >= ICMP_SIZE);
		addlen -= ICMP_SIZE;
	}
	ip = xalloc(IP_SIZE + addlen);

	/*
	 * IP header.
	 * Note: ip_len and ip_off shall be in host byte order.
	 */
	ip->ip_v = IPVERSION;
	ip->ip_hl = 5;
	ip->ip_tos = 0;
	ip->ip_len = (5 << 2) + addlen;
	ip->ip_id = htons(1);
	ip->ip_off = foff;
	ip->ip_ttl = 64;
	ip->ip_p = IPPROTO_ICMP;
	ip->ip_sum = 0;

	ip->ip_src.s_addr = INADDR_ANY;
	ip->ip_dst.s_addr = target;

	/* Fill in some data. */
	data = (void *)(ip + 1);
	if (first) {
		struct icmp *icmp = (void *)data;
		icmp->icmp_type = ICMP_ECHO;
		icmp->icmp_code = 0;
		icmp->icmp_cksum = 0;
		n = ICMP_SIZE;
	} else {
		n = 0;
	}
	for (data += n; n < addlen; n++) {
		*data = (0xf & n);
		data++;
	}
	return (void *)ip;
}

void
send_packet(int s, void *p, in_addr_t target)
{
	struct ip *ip = (struct ip *)p;
	struct sockaddr_in addr;
	ssize_t ret;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = target;

	/* Note: total length was set host byte-order. */
	ret = sendto(s, p, ip->ip_len, 0,
	    (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret == -1) {
		err(EXIT_FAILURE, "sendto");
	}
	free(p);
}

void
test_case_0(int sock, in_addr_t target)
{
	void *p;

	/*
	 * Case 0: normal reassembly.
	 * => IP_STAT_FRAGMENTS
	 */

	p = create_ip_packet(target, 0 | IP_MF, 1480);
	send_packet(sock, p, target);

	p = create_ip_packet(target, 185, 1480);
	send_packet(sock, p, target);
}

void
test_case_1(int sock, in_addr_t target)
{
	void *p;

	/*
	 * Case 1: ip_len of the first fragment and ip_off + hlen of other
	 * fragments should be at least 68 (IP_MINFRAGSIZE - 1) bytes.
	 * => IP_STAT_BADFRAGS (malformed)
	 */

	/* 20 + 48 == 68, OK */
	p = create_ip_packet(target, 0 | IP_MF, 48);
	send_packet(sock, p, target);

	/* 20 + (5 * 8) = 60 < 68, ERR */
	p = create_ip_packet(target, 6 - 1, 8);
	send_packet(sock, p, target);
}

void
test_case_2(int sock, in_addr_t target)
{
	void *p;
	struct ip *ip;

	/*
	 * Case 2: mismatch of TOS.
	 * => IP_STAT_BADFRAGS (malformed)
	 */

	p = create_ip_packet(target, 0 | IP_MF, 48);
	ip = (struct ip *)p;
	ip->ip_tos = 123;
	send_packet(sock, p, target);

	p = create_ip_packet(target, 6, 48);
	send_packet(sock, p, target);
}

void
test_case_3(int sock, in_addr_t target)
{
	void *p;

	/*
	 * Case 3: data length is not multiple of 8 bytes.
	 * => IP_STAT_BADFRAGS (malformed)
	 */

	p = create_ip_packet(target, 0 | IP_MF, 48);
	send_packet(sock, p, target);

	/* (48 + 1) & 0x7 != 0, ERR */
	p = create_ip_packet(target, 6, 48 + 1);
	send_packet(sock, p, target);
}

void
test_case_4(int sock, in_addr_t target)
{
	void *p;

	/*
	 * Case 4: fragment covering previous (duplicate).
	 * => IP_STAT_FRAGDROPPED (dup, out of space)
	 */

	p = create_ip_packet(target, 0 | IP_MF, 48);
	send_packet(sock, p, target);

	p = create_ip_packet(target, 6 | IP_MF, 96);
	send_packet(sock, p, target);

	p = create_ip_packet(target, 6, 48);
	send_packet(sock, p, target);
}

void
test_case_5(int sock, in_addr_t target)
{
	void *p;
	int cnt, left, off;

	/*
	 * Case 5: construct packet larger than IP_MAXPACKET.
	 * => IP_STAT_TOOLONG (with length > max ip packet size)
	 */

	/*
	 * Assumptions:
	 * - 1500 MTU.  Minus IP header - 1480.
	 * - Bytes left: 65535 - (1480 * 44) = 415.
	 */
	cnt = IP_MAXPACKET / 1480;
	left = IP_MAXPACKET - (1480 * cnt);
	left = (left / 8) * 8;

	for (off = 0; cnt != 0; cnt--) {
		p = create_ip_packet(target, off | IP_MF, 1480);
		send_packet(sock, p, target);
		off += (1480 >> 3);
	}
	/* Add 8 bytes and thus cross IP_MAXPACKET limit. */
	p = create_ip_packet(target, off, left + 8);
	send_packet(sock, p, target);
}

void
test_case_6(int sock, in_addr_t target)
{
	struct ip *ip;
	void *p;

	/*
	 * Case 6: cause a fragment timeout.
	 * => IP_STAT_FRAGTIMEOUT (fragments dropped after timeout)
	 */

	p = create_ip_packet(target, 0 | IP_MF, 48);
	ip = (struct ip *)p;
	ip->ip_id = htons(321);
	send_packet(sock, p, target);
	/*
	 * Missing non-MF packet - timeout.
	 */
}

int
main(int argc, char **argv)
{
	struct hostent *he;
	in_addr_t target;
	int sock;

	if (argc < 2) {
		printf("%s: <target host>\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	he = gethostbyname(argv[1]);
	if (he == NULL) {
		err(EXIT_FAILURE, "gethostbyname");
	}
	memcpy(&target, he->h_addr, sizeof(target));

	sock = create_socket();

	test_case_0(sock, target);
	test_case_1(sock, target);
	test_case_2(sock, target);
	test_case_3(sock, target);
	test_case_4(sock, target);
	test_case_5(sock, target);
	test_case_6(sock, target);

	close(sock);

	return 0;
}
