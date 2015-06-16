/* $NetBSD: pfs.c,v 1.2 2015/06/16 23:04:14 christos Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: pfs.c,v 1.2 2015/06/16 23:04:14 christos Exp $");
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <net/if.h>
#include <netinet/in.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <net/pfvar.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "parser.h"

__dead static void usage(void);
static int setlock(int, int, int);
static int get_states(int, int, struct pfioc_states*);
static int dump_states_binary(int, int, const char*);
static int restore_states_binary(int, int, const char*);
static int dump_states_ascii(int, int, const char*);
static int restore_states_ascii(int, int, const char*);
static char* print_host(const struct pfsync_state_host *h, sa_family_t, char*, size_t);
static void print_peer(const struct pfsync_state_peer *peer, uint8_t, FILE*);
static int print_states(int, int, FILE*);
static void display_states(const struct pfioc_states*, int, FILE*);
static int test_ascii_dump(int, const char*, const char*);

static char pf_device[] = "/dev/pf";

__dead static void
usage(void) 
{
	fprintf(stderr, 
			"usage : %s [-v] [-u | -l | -w <filename> | -r <filename> |\n"
			"			[ -W <filename> | -R <filename> ]\n",
			getprogname());
	exit(EXIT_FAILURE);
}

/*
 * The state table must be locked before calling this function
 * Return the number of state in case of success, -1 in case of failure
 * ps::ps_buf must be freed by user after use (in case of success)
 */
static int
get_states(int fd, int verbose __unused, struct pfioc_states* ps)
{
	memset(ps, 0, sizeof(*ps));
	ps->ps_len = 0;
	char* inbuf;

	// ask the kernel how much memory we need to allocate
	if (ioctl(fd, DIOCGETSTATES, ps) == -1) {
		err(EXIT_FAILURE, "DIOCGETSTATES");
	}	

	/* no state */
	if (ps->ps_len == 0)
		return 0;

	inbuf = malloc(ps->ps_len);
	if (inbuf == NULL)
		err(EXIT_FAILURE, NULL);

	ps->ps_buf = inbuf;

	// really retrieve the different states
	if (ioctl(fd, DIOCGETSTATES, ps) == -1) { 
		free(ps->ps_buf);
		err(EXIT_FAILURE, "DIOCGETSTATES");
	}

	return (ps->ps_len / sizeof(struct pfsync_state));
}

static int
dump_states_binary(int fd, int verbose, const char* filename)
{
	int wfd; 
	struct pfioc_states ps;
	struct pfsync_state *p = NULL;
	int nb_states;
	int i;
	int error = 0;
	int errno_saved = 0;

	wfd = open(filename, O_WRONLY|O_TRUNC|O_CREAT, 0600);
	if (wfd == -1)
		err(EXIT_FAILURE, "Cannot open %s", filename);

	nb_states = get_states(fd, verbose, &ps);
	if (nb_states <= 0) {
		close(wfd);
		return nb_states;
	}

	/*
	 * In the file, write the number of states, then store the different states
	 * When we will switch to text format, we probably don't care any more about the len
	 */
	if (write(wfd, &nb_states, sizeof(nb_states)) != sizeof(nb_states)) {
		error = EXIT_FAILURE;
		errno_saved = errno;
		goto done;
	}

	p = ps.ps_states;
	for (i = 0; i < nb_states; i++) {
		if (write(wfd, &p[i], sizeof(*p)) != sizeof(*p)) {
			error = EXIT_FAILURE; 
			errno_saved = errno;
			goto done;
		}
	}

done:
	free(p);
	close(wfd);
	// close can't modify errno
	if (error) {
		errno = errno_saved;
		err(error, NULL);
	}

	return 0;
}

static int
restore_states_binary(int fd, int verbose __unused, const char* filename)
{
	int rfd;
	struct pfioc_states ps;
	struct pfsync_state *p;
	int nb_states;
	int errno_saved = 0;
	int i;

	rfd = open(filename, O_RDONLY, 0600);
	if (rfd == -1)
		err(EXIT_FAILURE, "Cannot open %s", filename);

	if (read(rfd, &nb_states, sizeof(nb_states)) != sizeof(nb_states)) {
		errno_saved = errno;
		close(rfd);
		errno = errno_saved;
		err(EXIT_FAILURE, NULL);
	}

	ps.ps_len = nb_states * sizeof(struct pfsync_state);
	ps.ps_states = malloc(ps.ps_len);
	if (ps.ps_states == NULL) {
		errno_saved = errno;
		close(rfd);
		errno = errno_saved;
		err(EXIT_FAILURE, NULL);
	}

	p = ps.ps_states;

	for (i = 0; i < nb_states; i++) {
		if (read(rfd, &p[i], sizeof(*p)) != sizeof(*p)) {
			errno_saved = errno;
			close(rfd);
			free(ps.ps_states);
			errno = errno_saved;
			err(EXIT_FAILURE, NULL);
		}
	}

	if (ioctl(fd, DIOCADDSTATES, &ps) == -1) {
		errno_saved = errno;
		close(rfd);
		free(ps.ps_states);
		errno = errno_saved;
		err(EXIT_FAILURE, "DIOCADDSTATES");
	}

	free(ps.ps_states);
	close(rfd);
	return 0;
}

static char*
print_host(const struct pfsync_state_host *h, sa_family_t af, char* buf, 
		size_t size_buf)
{
	uint16_t port;
	char	buf_addr[48];

	port = ntohs(h->port);
	if (inet_ntop(af, &(h->addr) , buf_addr, sizeof(buf_addr)) == NULL) {
		strcpy(buf_addr, "?");
	} 

	snprintf(buf, size_buf, "%s:[%d]", buf_addr, port);
	return buf;
}

static void
print_peer(const struct pfsync_state_peer* peer, uint8_t proto, FILE* f)
{
	if (proto == IPPROTO_TCP) { 
		if (peer->state < TCP_NSTATES)
			fprintf(f, "state %s", tcpstates[peer->state]);

		if (peer->seqdiff != 0) 
			fprintf(f, " seq [%" PRIu32 ":%" PRIu32 ",%" PRIu32"]",
					peer->seqlo, peer->seqhi, peer->seqdiff);
		else 
			fprintf(f, " seq [%" PRIu32 ":%" PRIu32 "]",
					peer->seqlo, peer->seqhi);

		if (peer->mss != 0) 
			fprintf(f, " max_win %" PRIu16 " mss %" PRIu16 " wscale %" PRIu8, 
					peer->max_win, peer->mss, peer->wscale);
		else
			fprintf(f, " max_win %" PRIu16 " wscale %" PRIu8, peer->max_win, 
					peer->wscale);
				
	} else {
		if (proto == IPPROTO_UDP) {
			const char *mystates[] = PFUDPS_NAMES;
			if (peer->state < PFUDPS_NSTATES)
				fprintf(f, "state %s", mystates[peer->state]);
		} else if (proto == IPPROTO_ICMP || proto == IPPROTO_ICMPV6) { 
			fprintf(f, " state %" PRIu8, peer->state);
		} else {
			const char *mystates[] = PFOTHERS_NAMES;
			if (peer->state < PFOTHERS_NSTATES)
				fprintf(f, " state %s", mystates[peer->state]);
		}
	}

	if (peer->scrub.scrub_flag == PFSYNC_SCRUB_FLAG_VALID) {
		fprintf(f, " scrub flags %" PRIu16 "ttl %" PRIu8 "mod %"PRIu32,
				peer->scrub.pfss_flags, peer->scrub.pfss_ttl, peer->scrub.pfss_ts_mod);
	} else {
		fprintf(f, " no-scrub");
	}
}

static void 
display_states(const struct pfioc_states *ps, int verbose __unused, FILE* f)
{
	struct pfsync_state *p = NULL;
	struct pfsync_state_peer *src, *dst;
	struct protoent *proto;
	int nb_states;
	int i;
	uint64_t id;

	p = ps->ps_states;
	nb_states = ps->ps_len / sizeof(struct pfsync_state);

	for (i = 0; i < nb_states; i++, p++) {
		fprintf(f, "state %s ", p->direction == PF_OUT ? "out" : "in");
		fprintf(f, "on %s ", p->ifname);

		if ((proto = getprotobynumber(p->proto)) != NULL)
			fprintf(f, "proto %s ", proto->p_name);
		else
			fprintf(f, "proto %u ", p->proto);


		if (PF_ANEQ(&p->lan.addr, &p->gwy.addr, p->af) || 
				(p->lan.port != p->gwy.port)) {
					
			char buf1[64], buf2[64], buf3[64];
			fprintf(f, "from %s to %s using %s", 
					print_host(&p->lan, p->af, buf1, sizeof(buf1)),
					print_host(&p->ext, p->af, buf2, sizeof(buf2)),
					print_host(&p->gwy, p->af, buf3, sizeof(buf3)));
		} else {
			char buf1[64], buf2[64];
			fprintf(f, "from %s to %s", 
					print_host(&p->lan, p->af, buf1, sizeof(buf1)),
					print_host(&p->ext, p->af, buf2, sizeof(buf2)));
		}

		memcpy(&id, p->id, sizeof(p->id));
		fprintf(f, " id %" PRIu64 " cid %" PRIu32 " expire %" PRIu32 " timeout %" PRIu8,
				id , p->creatorid, p->expire, p->timeout);

		if (p->direction == PF_OUT) {
			src = &p->src;
			dst = &p->dst;
		} else {
			src = &p->dst;
			dst = &p->src;
		}

		fprintf(f, " src "); 
		print_peer(src, p->proto, f);
		fprintf(f, " dst ");
		print_peer(dst, p->proto, f);

		fprintf(f, "\n");
	}
}

static int
print_states(int fd, int verbose, FILE* f)
{
	struct pfioc_states ps;
	int nb_states;

	nb_states = get_states(fd, verbose, &ps);
	if (nb_states <= 0) {
		return nb_states;
	}

	display_states(&ps, verbose, f);
					
	free(ps.ps_states);
	return 0;
}

static int
dump_states_ascii(int fd, int verbose, const char* filename)
{
	FILE *f;

	if (strcmp(filename, "-") == 0) {
		f = stdout;
	} else {
		f = fopen(filename, "w");
		if (f == NULL) 
			err(EXIT_FAILURE, "Can't open %s", filename);
	}

	print_states(fd, verbose, f);

	if (f != stdout)
		fclose(f);

	return 0;
}

static int
restore_states_ascii(int fd, int verbose __unused, const char* filename)
{
	FILE *f;
	struct pfioc_states ps;
	int errno_saved;

	f = fopen(filename, "r");
	if (f == NULL)
		err(EXIT_FAILURE, "Can't open %s", filename);

	parse(f, &ps);

	if (ioctl(fd, DIOCADDSTATES, &ps) == -1) {
		errno_saved = errno;
		fclose(f);
		free(ps.ps_states);
		errno = errno_saved;
		err(EXIT_FAILURE, "DIOCADDSTATES");
	}

	free(ps.ps_states);
	fclose(f);
	return 0;
}
			
static int
setlock(int fd, int verbose, int lock)
{
	if (verbose)
		printf("Turning lock %s\n", lock ? "on" : "off");

	if (ioctl(fd, DIOCSETLCK, &lock) == -1) 
		err(EXIT_FAILURE, "DIOCSETLCK");

	return 0;
}

static int
test_ascii_dump(int verbose, const char* file1, const char *file2)
{
	FILE *f1, *f2;
	struct pfioc_states ps;
	int errno_saved;
	
	f1 = fopen(file1, "r");
	if (f1 == NULL)
		err(EXIT_FAILURE, "Can't open %s", file1);


	f2 = fopen(file2, "w");
	if (f2 == NULL) {
		errno_saved = errno;
		fclose(f2);
		errno = errno_saved;
		err(EXIT_FAILURE, "Can't open %s", file2);
	}

	parse(f1, &ps);
	display_states(&ps, verbose, f2);

	free(ps.ps_states);
	fclose(f1);
	fclose(f2);

	return 0;
}

int main(int argc, char *argv[])
{
	setprogname(argv[0]);

	int lock = 0;
	int set = 0;
	int dump = 0;
	int restore = 0;
	int verbose = 0;
	int test = 0;
	bool binary = false;
	char* filename = NULL;
	char* filename2 = NULL;
	int error = 0;
	int fd;
	int c;

	while ((c = getopt(argc, argv, "ulvw:r:R:W:bt:o:")) != -1) 
		switch (c) {
		case 'u' : 
			lock = 0;
			set = 1;
			break;

		case 'l' :
			lock = 1;
			set = 1;
			break;

		case 'b':
			binary = true;
			break;

		case 'r':
			restore = 1;
			filename = optarg;
			break;

		case 'v':
			verbose=1;
			break;

		case 'w':
			dump=1;
			filename=optarg;
			break;

		case 'R':
			restore = 1;
			set = 1;
			filename = optarg;
			break;

		case 'W':
			dump = 1;
			set = 1;
			filename = optarg;
			break;

		case 't':
			test=1;
			filename = optarg;
			break;

		case 'o':
			filename2 = optarg; 
			break;

		case '?' :
		default:
			usage();
		}

	if (set == 0 && dump == 0 && restore == 0 && test == 0) 
		usage();

	if (dump == 1 && restore == 1)
		usage();

	if (test == 1) {
		if (filename2 == NULL) {
			fprintf(stderr, "-o <file> is required when using -t\n");
			err(EXIT_FAILURE, NULL);
		}
		error = test_ascii_dump(verbose, filename, filename2);
	} else {
		fd = open(pf_device, O_RDWR);
		if (fd == -1) 
			err(EXIT_FAILURE, "Cannot open %s", pf_device);

		if (set != 0 && dump == 0 && restore == 0)
			error = setlock(fd, verbose, lock);

		if (dump) {
			if (set) 
				error = setlock(fd, verbose, 1);

			if (binary) 
				error = dump_states_binary(fd, verbose, filename);
			else
				error = dump_states_ascii(fd, verbose, filename);

			if (set)
				error = setlock(fd, verbose, 0);
		}

		if (restore) {
			if (set) 
				error = setlock(fd, verbose, 1);

			if (binary) 
				error = restore_states_binary(fd, verbose, filename);
			else 
				error = restore_states_ascii(fd, verbose, filename);

			if (set)
				error = setlock(fd, verbose, 0);
		}

		close(fd);
	}

	return error;
}
