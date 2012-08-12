/*	$NetBSD: npfstream.c,v 1.3 2012/08/12 03:35:14 rmind Exp $	*/

/*
 * NPF stream processor.
 *
 * Public Domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <err.h>
#include <pcap.h>

#include <arpa/inet.h>

#include <net/if.h>
#include <net/ethertypes.h>
#include <net/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <rump/rump.h>

#include "npftest.h"

static struct in_addr	initial_ip;
static int		snd_packet_no = 0;
static int		rcv_packet_no = 0;

static void
process_tcpip(const void *data, size_t len, FILE *fp, unsigned idx)
{
	const struct ether_header *eth = data;
	const struct ip *ip;
	const struct tcphdr *th;
	unsigned hlen, tcpdlen;
	int error, packetno;
	tcp_seq seq;
	bool forw;

	if (ntohs(eth->ether_type) != ETHERTYPE_IP) {
		ip = (const struct ip *)((const char *)data + 4);
	} else {
		ip = (const struct ip *)(eth + 1);
	}
	hlen = ip->ip_hl << 2;
	th = (const struct tcphdr *)((const uint8_t *)ip + hlen);

	tcpdlen = ntohs(ip->ip_len) - hlen - (th->th_off << 2);
	if (th->th_flags & TH_SYN) {
		tcpdlen++;
	}
	if (th->th_flags & TH_FIN) {
		tcpdlen++;
	}
	seq = ntohl(th->th_seq);

	if (snd_packet_no == 0) {
		memcpy(&initial_ip, &ip->ip_src, sizeof(struct in_addr));
	}

	forw = (initial_ip.s_addr == ip->ip_src.s_addr);
	packetno = forw ? ++snd_packet_no : ++rcv_packet_no;

	int64_t result[11];
	memset(result, 0, sizeof(result));

	len = ntohs(ip->ip_len);
	error = rumpns_npf_test_handlepkt(ip, len, idx, forw, result);

	fprintf(fp, "%s%2x %5d %3d %11u %11u %11u %11u %12lx",
	    forw ? ">" : "<", (th->th_flags & (TH_SYN | TH_ACK | TH_FIN)),
	    packetno, error, (u_int)seq, (u_int)ntohl(th->th_ack),
	    tcpdlen, ntohs(th->th_win), (uintptr_t)result[0]);

	for (unsigned i = 1; i < __arraycount(result); i++) {
		fprintf(fp, "%11" PRIu64 " ", result[i]);
	}
	fputs("\n", fp);
}

int
process_stream(const char *input, const char *output, unsigned idx)
{
	pcap_t *pcap;
	char pcap_errbuf[PCAP_ERRBUF_SIZE];
	struct pcap_pkthdr *phdr;
	const uint8_t *data;
	FILE *fp;

	pcap = pcap_open_offline(input, pcap_errbuf);
	if (pcap == NULL) {
		errx(EXIT_FAILURE, "pcap_open_offline failed: %s", pcap_errbuf);
	}
	fp = output ? fopen(output, "w") : stdout;
	if (fp == NULL) {
		err(EXIT_FAILURE, "fopen");
	}
	fprintf(fp, "#FL %5s %3s %11s %11s %11s %11s %11s %11s %11s "
	    "%11s %11s %11s %5s %11s %11s %11s %5s\n",
	    "No", "Err", "Seq", "Ack", "TCP Len", "Win",
	    "Stream", "RetVal", "State",
	    "F.END", "F.MAXEND", "F.MAXWIN", "F.WSC",
	    "T.END", "T.MAXEND", "T.MAXWIN", "T.WSC");
	while (pcap_next_ex(pcap, &phdr, &data) > 0) {
		if (phdr->len != phdr->caplen) {
			warnx("process_stream: truncated packet");
		}
		process_tcpip(data, phdr->caplen, fp, idx);
	}
	pcap_close(pcap);
	fclose(fp);

	return 0;
}
