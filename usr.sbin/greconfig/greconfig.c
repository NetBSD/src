/*
 * (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Heiko W.Rupp <hwr@pilhuhn.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * greconfig - frontend to set/query tunnel endpoints
 *
 * $NetBSD: greconfig.c,v 1.5.2.1 1999/12/27 18:37:41 wrstuden Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>

#include <netdb.h>

#include <net/if.h>

#include <netinet/in.h>

#include <arpa/inet.h>

/* from sys/sockio.h */
#define GRESADDRS       _IOW('i', 101, struct ifreq)
#define GRESADDRD       _IOW('i', 102, struct ifreq)   
#define GREGADDRS       _IOWR('i', 103, struct ifreq)
#define GREGADDRD       _IOWR('i', 104, struct ifreq)
#define GRESPROTO	_IOW('i' , 105, struct ifreq)
#define GREGPROTO	_IOWR('i', 106, struct ifreq)


void usage(void);
void name2sa(char *name,struct sockaddr **sa);
char *sa2name(struct sockaddr *sa);

int verbose;

int
main(int argc, char **argv)
{
	int i, s, err;
	char *dst, *src, *inf;
	struct ifreq ifr;
	struct sockaddr *sa;
	int pflag=0;
	u_char proto = 47;

	dst = src = inf = NULL;
	verbose = 0;

	while ((i = getopt(argc, argv, "d:i:p:s:v")) != -1) 
		switch(i) {
			case 'd':
				dst = optarg;
				break;
			case 'i':
				inf = optarg;
				break;
			case 'p':
				proto = atoi(optarg);
				pflag++;
				break;
			case 's':
				src = optarg;
				break;
			case 'v':
				verbose++;
				break;
			default:
				usage();
				exit(1); 
		}

	if (inf == NULL) {
		usage();
		exit(2);
	}

	if (strncmp("gre", inf, 3) != 0) {
		usage();
		exit(3);
	}

	if ((proto != 47) && (proto != 55)) {
		usage();
		exit(4);
	}

	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("socket() failed:");
		exit(5);
	}
	if (pflag) {	/* IPPROTO_GRE is default in kernel */
		strncpy(ifr.ifr_name, inf, sizeof(ifr.ifr_name));
		ifr.ifr_flags =  proto;
		if (verbose)
			printf("Setting tunnel protocol to proto %d\n", proto);
		err = ioctl(s, GRESPROTO, (caddr_t)&ifr);
		if (err < 0)
			perror("GRESPROTO");
	}
	if (src != NULL) {
		strncpy(ifr.ifr_name, inf, sizeof(ifr.ifr_name));
		name2sa(src, &sa);
		ifr.ifr_addr = *sa;
		if (verbose)
			printf("Setting source address to %s...\n",
			       sa2name(sa));
		err = ioctl(s, GRESADDRS, (caddr_t)&ifr);
		if (err < 0)
			perror("GRESADDRS");
	}
	if (dst != NULL) {
		strncpy(ifr.ifr_name, inf, sizeof(ifr.ifr_name));
		name2sa(dst, &sa);
		ifr.ifr_addr = *sa;
		if (verbose)
			printf("Setting destination address to %s...\n",
			       sa2name(sa));
		err = ioctl(s, GRESADDRD, (caddr_t)&ifr);
		if (err < 0)
			perror("GRESADDRD");
	}
	if (src == NULL && dst == NULL && !pflag) {
		strncpy(ifr.ifr_name, inf, sizeof(ifr.ifr_name));
		err = ioctl(s, GREGADDRS, (caddr_t)&ifr);
		if (err < 0)
			perror("GREGADDRS");
		else 
			printf("%s -> ", sa2name(&ifr.ifr_addr));
		strncpy(ifr.ifr_name, inf, sizeof(ifr.ifr_name));
		err = ioctl(s, GREGADDRD, (caddr_t)&ifr);
		if (err < 0)
			perror("GREGADDRD");
		else
			printf("%s, ", sa2name(&ifr.ifr_addr));
		err = ioctl(s, GREGPROTO, (caddr_t)&ifr);
		if (err < 0)
			perror("GREGPROTO");
		else
			printf("running IP-Proto %d\n", ifr.ifr_flags);
	}
	close(s);

	exit(0);
}

void
usage(void)
{
	fprintf(stderr,
		"greconfig -i unit [-d dst] [-s src] [-p proto] [-v]\n");
	fprintf(stderr, "unit is gre<n>, proto either 47 or 55\n");
}

void
name2sa(char *name, struct sockaddr **sa)
{
	struct hostent *hp;
	struct sockaddr_in *si;
	static struct sockaddr_in s;

	hp = gethostbyname(name);

	bzero(&s, sizeof(struct sockaddr_in));
	s.sin_family = hp->h_addrtype;
	if (hp->h_addrtype != AF_INET)
		errx(5, "Only internet addresses allowed, not %s\n", name);
	bcopy(hp->h_addr, &s.sin_addr, hp->h_length);
	si = &s;

	*sa = (struct sockaddr *)si;
}

char *
sa2name(struct sockaddr *sa)
{
	struct sockaddr_in *si;

	si = ((struct sockaddr_in *)(sa));

	return (inet_ntoa(si->sin_addr));
}
