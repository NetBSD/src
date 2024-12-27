/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/pfvar.h>
#include <arpa/inet.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <altq/altq.h>
#include <altq/altq_cbq.h>
#include <altq/altq_priq.h>
#include <altq/altq_hfsc.h>
#include "npfctl.h"

union class_stats {
	class_stats_t		cbq_stats;
	struct priq_classstats	priq_stats;
	struct hfsc_classstats	hfsc_stats;
};
#define AVGN_MAX	8
#define STAT_INTERVAL	5
struct queue_stats {
	union class_stats	 data;
	int			 avgn;
	double			 avg_bytes;
	double			 avg_packets;
	uint64_t		 prev_bytes;
	uint64_t		 prev_packets;
};
struct npf_altq_node {
	struct npf_altq		 altq;
	struct npf_altq_node	*next;
	struct npf_altq_node	*children;
	struct queue_stats	 qstats;
};
int			 npfctl_update_qstats(int, struct npf_altq_node **);
void			 npfctl_insert_altq_node(struct npf_altq_node **,
			    const struct npf_altq, const struct queue_stats);
struct npf_altq_node	*npfctl_find_altq_node(struct npf_altq_node *,
			    const char *, const char *);
void			 npfctl_print_altq_node(int, const struct npf_altq_node *, unsigned);
void			 print_cbqstats(struct queue_stats);
void			 print_priqstats(struct queue_stats);
void			 print_hfscstats(struct queue_stats);
void			 npfctl_free_altq_node(struct npf_altq_node *);
void			 npfctl_print_altq_nodestat(const struct npf_altq_node *);
void			 update_avg(struct npf_altq_node *);

int
npfctl_show_altq(int fd)
{
	struct npf_altq_node	*root = NULL, *node;
	int			 nodes;

	if ((nodes = npfctl_update_qstats(fd, &root)) < 0)
		return -1;
	if (nodes == 0)
		printf("No queue in use\n");
	for (node = root; node != NULL; node = node->next) {
		if (node->altq.ifname == NULL)
			continue;

		npfctl_print_altq_node(fd, node, 0);
	}
    /* be verbose */
	while ( nodes > 0) {
		printf("\n");
		fflush(stdout);
		sleep(STAT_INTERVAL);
		if ((nodes = npfctl_update_qstats(fd, &root)) == -1)
			return -1;
		for (node = root; node != NULL; node = node->next) {
			if (node->altq.ifname == NULL)
				continue;
			npfctl_print_altq_node(fd, node, 0);
		}
	}
	npfctl_free_altq_node(root);
	return 0;
}

int
npfctl_update_qstats(int fd, struct npf_altq_node **root)
{
	struct npf_altq_node	*node;
	struct npfioc_altq	 pa;
	struct npfioc_qstats	 pq;
	uint32_t		 mnr, nr;
	struct queue_stats	 qstats;
	static	uint32_t	 last_ticket;
	memset(&pa, 0, sizeof(pa));
	memset(&pq, 0, sizeof(pq));
	memset(&qstats, 0, sizeof(qstats));
	if (ioctl(fd, IOC_NPF_GET_ALTQS, &pa)) {
		warn("IOC_NPF_GET_ALTQS");
		return -1;
	}
	/* if a new set is found, start over */
	if (pa.ticket != last_ticket && *root != NULL) {
		npfctl_free_altq_node(*root);
		*root = NULL;
	}
	last_ticket = pa.ticket;
	mnr = pa.nr;
	for (nr = 0; nr < mnr; ++nr) {
		pa.nr = nr;
		if (ioctl(fd, IOC_NPF_GET_ALTQ, &pa)) {
			warn("IOC_NPF_GET_ALTQ");
			return -1;
		}
		if (pa.altq.qid > 0) {
			pq.nr = nr;
			pq.ticket = pa.ticket;
			pq.buf = &qstats.data;
			pq.nbytes = sizeof(qstats.data);
			if (ioctl(fd, IOC_NPF_GET_QSTATS, &pq)) {
				warn("IOC_NPF_GET_QSTATS");
				return -1;
			}
			if ((node = npfctl_find_altq_node(*root, pa.altq.qname,
			    pa.altq.ifname)) != NULL) {
				memcpy(&node->qstats.data, &qstats.data,
				    sizeof(qstats.data));
				update_avg(node);
			} else {
				npfctl_insert_altq_node(root, pa.altq, qstats);
			}
		}
	}
	return mnr;
}

void
npfctl_insert_altq_node(struct npf_altq_node **root,
    const struct npf_altq altq, const struct queue_stats qstats)
{
	struct npf_altq_node	*node;
	node = calloc(1, sizeof(*node));
	if (node == NULL)
		err(EXIT_FAILURE, "npfctl_insert_altq_node: calloc");
	memcpy(&node->altq, &altq, sizeof(altq));
	memcpy(&node->qstats, &qstats, sizeof(qstats));
	node->next = node->children = NULL;
	if (*root == NULL)
		*root = node;
	else if (!altq.parent[0]) {
		struct npf_altq_node	*prev = *root;
		while (prev->next != NULL)
			prev = prev->next;
		prev->next = node;
	} else {
		struct npf_altq_node	*parent;
		parent = npfctl_find_altq_node(*root, altq.parent, altq.ifname);
		if (parent == NULL)
			errx(1, "parent %s not found", altq.parent);
		if (parent->children == NULL)
			parent->children = node;
		else {
			struct npf_altq_node *prev = parent->children;
			while (prev->next != NULL)
				prev = prev->next;
			prev->next = node;
		}
	}
	update_avg(node);
}

struct npf_altq_node *
npfctl_find_altq_node(struct npf_altq_node *root, const char *qname,
    const char *ifname)
{
	struct npf_altq_node	*node, *child;
	for (node = root; node != NULL; node = node->next) {
		if (!strcmp(node->altq.qname, qname)
		    && !(strcmp(node->altq.ifname, ifname)))
			return node;
		if (node->children != NULL) {
			child = npfctl_find_altq_node(node->children, qname,
			    ifname);
			if (child != NULL)
				return child;
		}
	}
	return NULL;
}

void
npfctl_print_altq_node(int fd, const struct npf_altq_node *node, unsigned level)
{
	const struct npf_altq_node	*child;
	if (node == NULL)
		return;
	print_altq(&node->altq, level, NULL, NULL);
	if (node->children != NULL) {
		printf("{");
		for (child = node->children; child != NULL;
		    child = child->next) {
			printf("%s", child->altq.qname);
			if (child->next != NULL)
				printf(", ");
		}
		printf("}");
	}
	printf("\n");
    /* be verbose */
	npfctl_print_altq_nodestat(node);
//	if (opts & PF_OPT_DEBUG)
//		printf("  [ qid=%u ifname=%s ifbandwidth=%s ]\n",
//		    node->altq.qid, node->altq.ifname,
//		    rate2str((double)(node->altq.ifbandwidth)));
	for (child = node->children; child != NULL;
	    child = child->next)
		npfctl_print_altq_node(fd, child, level + 1);
}
void
npfctl_print_altq_nodestat( const struct npf_altq_node *a)
{
	if (a->altq.qid == 0)
		return;
	switch (a->altq.scheduler) {
	case ALTQT_CBQ:
		print_cbqstats(a->qstats);
		break;
	case ALTQT_PRIQ:
		print_priqstats(a->qstats);
		break;
	case ALTQT_HFSC:
		print_hfscstats(a->qstats);
		break;
	}
}

void
print_cbqstats(struct queue_stats cur)
{
	printf("  [ pkts: %10llu  bytes: %10llu  "
	    "dropped pkts: %6llu bytes: %6llu ]\n",
	    (unsigned long long)cur.data.cbq_stats.xmit_cnt.packets,
	    (unsigned long long)cur.data.cbq_stats.xmit_cnt.bytes,
	    (unsigned long long)cur.data.cbq_stats.drop_cnt.packets,
	    (unsigned long long)cur.data.cbq_stats.drop_cnt.bytes);
	printf("  [ qlength: %3d/%3d  borrows: %6u  suspends: %6u ]\n",
	    cur.data.cbq_stats.qcnt, cur.data.cbq_stats.qmax,
	    cur.data.cbq_stats.borrows, cur.data.cbq_stats.delays);
	if (cur.avgn < 2)
		return;
	printf("  [ measured: %7.1f packets/s, %s/s ]\n",
	    cur.avg_packets / STAT_INTERVAL,
	    rate2str((8 * cur.avg_bytes) / STAT_INTERVAL));
}

void
print_priqstats(struct queue_stats cur)
{
	printf("  [ pkts: %10llu  bytes: %10llu  "
	    "dropped pkts: %6llu bytes: %6llu ]\n",
	    (unsigned long long)cur.data.priq_stats.xmitcnt.packets,
	    (unsigned long long)cur.data.priq_stats.xmitcnt.bytes,
	    (unsigned long long)cur.data.priq_stats.dropcnt.packets,
	    (unsigned long long)cur.data.priq_stats.dropcnt.bytes);
	printf("  [ qlength: %3d/%3d ]\n",
	    cur.data.priq_stats.qlength, cur.data.priq_stats.qlimit);
	if (cur.avgn < 2)
		return;
	printf("  [ measured: %7.1f packets/s, %s/s ]\n",
	    cur.avg_packets / STAT_INTERVAL,
	    rate2str((8 * cur.avg_bytes) / STAT_INTERVAL));
}

void
print_hfscstats(struct queue_stats cur)
{
	printf("  [ pkts: %10llu  bytes: %10llu  "
	    "dropped pkts: %6llu bytes: %6llu ]\n",
	    (unsigned long long)cur.data.hfsc_stats.xmit_cnt.packets,
	    (unsigned long long)cur.data.hfsc_stats.xmit_cnt.bytes,
	    (unsigned long long)cur.data.hfsc_stats.drop_cnt.packets,
	    (unsigned long long)cur.data.hfsc_stats.drop_cnt.bytes);
	printf("  [ qlength: %3d/%3d ]\n",
	    cur.data.hfsc_stats.qlength, cur.data.hfsc_stats.qlimit);
	if (cur.avgn < 2)
		return;
	printf("  [ measured: %7.1f packets/s, %s/s ]\n",
	    cur.avg_packets / STAT_INTERVAL,
	    rate2str((8 * cur.avg_bytes) / STAT_INTERVAL));
}

void
npfctl_free_altq_node(struct npf_altq_node *node)
{
	while (node != NULL) {
		struct npf_altq_node	*prev;
		if (node->children != NULL)
			npfctl_free_altq_node(node->children);
		prev = node;
		node = node->next;
		free(prev);
	}
}

void
update_avg(struct npf_altq_node *a)
{
	struct queue_stats	*qs;
	uint64_t		 b, p;
	int			 n;
	if (a->altq.qid == 0)
		return;
	qs = &a->qstats;
	n = qs->avgn;
	switch (a->altq.scheduler) {
	case ALTQT_CBQ:
		b = qs->data.cbq_stats.xmit_cnt.bytes;
		p = qs->data.cbq_stats.xmit_cnt.packets;
		break;
	case ALTQT_PRIQ:
		b = qs->data.priq_stats.xmitcnt.bytes;
		p = qs->data.priq_stats.xmitcnt.packets;
		break;
	case ALTQT_HFSC:
		b = qs->data.hfsc_stats.xmit_cnt.bytes;
		p = qs->data.hfsc_stats.xmit_cnt.packets;
		break;
	default:
		b = 0;
		p = 0;
		break;
	}
	if (n == 0) {
		qs->prev_bytes = b;
		qs->prev_packets = p;
		qs->avgn++;
		return;
	}
	if (b >= qs->prev_bytes)
		qs->avg_bytes = ((qs->avg_bytes * (n - 1)) +
		    (b - qs->prev_bytes)) / n;
	if (p >= qs->prev_packets)
		qs->avg_packets = ((qs->avg_packets * (n - 1)) +
		    (p - qs->prev_packets)) / n;
	qs->prev_bytes = b;
	qs->prev_packets = p;
	if (n < AVGN_MAX)
		qs->avgn++;
}
