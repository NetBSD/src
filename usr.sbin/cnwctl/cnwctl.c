/*	$NetBSD: cnwctl.c,v 1.5 1999/12/01 03:40:51 sommerfeld Exp $	*/

/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this notice is retained,
 * the conditions in the following notices are met, and terms applying
 * to contributors in the following notices also apply to Berkeley
 * Software Design, Inc.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by
 *	Berkeley Software Design, Inc.
 * 4. Neither the name of the Berkeley Software Design, Inc. nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      PAO2 Id: cnwctl.c,v 1.1.1.1 1997/12/11 14:46:06 itojun Exp
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>

#include <dev/pcmcia/if_cnwioctl.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	int c, domain, i, key, rate, sflag, Sflag, skt;
	char *e, *interface;
	struct ifreq ifr;
        struct cnwistats cnwis, onwis;
        struct cnwstatus cnws;
	struct ttysize ts;

	domain = -1;
	key = -1;
	Sflag = sflag = 0;
	rate = 0;
	interface = "cnw0";

	while ((c = getopt(argc, argv, "d:i:k:sS")) != EOF)
		switch (c) {
		case 'd':
			domain = strtol(optarg, &e, 0);
			if (e == optarg || *e || domain & ~ 0x1ff)
				errx(1, "%s: invalid domain", optarg);
			break;
		case 'i':
			interface = optarg;
			break;
		case 'k':
			key = strtol(optarg, &e, 0);
			if (e == optarg || *e || key & ~ 0xffff)
				errx(1, "%s: invalid scramble key", optarg);
			break;
		case 'S':
			++Sflag;
			break;
		case 's':
			++sflag;
			break;
		default: usage:
			fprintf(stderr, "usage: cnwctl [-i interface] [-d domain] [-k key] [-sS [rate]]\n");
			exit(1);
		}

	switch (argc - optind) {
	case 0:
		break;
	case 1:
		if (sflag == 0 && Sflag == 0)
			goto usage;
		if (sflag && Sflag)
			errx(1, "only one of -s and -S may be specified with a rate");
		rate = strtol(argv[optind], &e, 0);
		if (e == optarg || *e || rate < 1)
			errx(1, "%s: invalid rate", optarg);
		break;
	default:
		goto usage;
	}

        if ((skt = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                err(1, "socket(AF_INET, SOCK_DGRAM)");

	if (key >= 0) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));
		ifr.ifr_key = key;
		if (ioctl(skt, SIOCSCNWKEY, (caddr_t)&ifr) < 0)
			err(1, "SIOCSCNWKEY");
	}

	if (domain >= 0) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));
		ifr.ifr_domain = domain;
		if (ioctl(skt, SIOCSCNWDOMAIN, (caddr_t)&ifr) < 0)
			err(1, "SIOCSCNWDOMAIN");
	}

	if (sflag == 0 && Sflag == 0)
		exit (0);

	if (Sflag) {
        	memset(&cnws, 0, sizeof(cnws));
		strncpy(cnws.ifr.ifr_name, interface,
		    sizeof(cnws.ifr.ifr_name));
		if (ioctl(skt, SIOCGCNWSTATUS, (caddr_t)&cnws) < 0)
			err(1, "SIOCGCNWSTATUS");
	}

	if (sflag) {
		memset(&cnwis, 0, sizeof(cnwis));
		strncpy(cnwis.ifr.ifr_name, interface,
		    sizeof(cnwis.ifr.ifr_name));
		if (ioctl(skt, SIOCGCNWSTATS, (caddr_t)&cnwis) < 0)
			err(1, "SIOCGCNWSTATS");
		cnwis.stats.nws_txretries[0] = 0;

		for (i = 1; i < 16; ++i)
			cnwis.stats.nws_txretries[0] +=
			    cnwis.stats.nws_txretries[i] * i;
	}

	if (rate == 0 && sflag) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));
		if (ioctl(skt, SIOCGCNWDOMAIN, (caddr_t)&ifr) < 0)
			err(1, "SIOCGCNWDOMAIN");
		printf("%s:\n     0x%03x domain\n", interface, ifr.ifr_domain);
		printf("%10llu rx\n", (unsigned long long)cnwis.stats.nws_rx);
		printf("%10llu rxoverflow\n",
		    (unsigned long long)cnwis.stats.nws_rxoverflow);
		printf("%10llu rxoverrun\n",
		    (unsigned long long)cnwis.stats.nws_rxoverrun);
		printf("%10llu rxcrcerror\n",
		    (unsigned long long)cnwis.stats.nws_rxcrcerror);
		printf("%10llu rxframe\n",
		    (unsigned long long)cnwis.stats.nws_rxframe);
		printf("%10llu rxerrors\n", 
		    (unsigned long long)cnwis.stats.nws_rxerrors);
		printf("%10llu rxavail\n",
		    (unsigned long long)cnwis.stats.nws_rxavail);

		printf("%10llu tx\n", (unsigned long long)cnwis.stats.nws_tx);
		printf("%10llu txokay\n",
		    (unsigned long long)cnwis.stats.nws_txokay);
		printf("%10llu txabort\n",
		    (unsigned long long)cnwis.stats.nws_txabort);
		printf("%10llu txlostcd\n",
		    (unsigned long long)cnwis.stats.nws_txlostcd);
		printf("%10llu txerrors\n",
		    (unsigned long long)cnwis.stats.nws_txerrors);
		printf("%10llu txretries\n",
		    (unsigned long long)cnwis.stats.nws_txretries[0]);
		for (i = 1; i < 16; ++i)
			if (cnwis.stats.nws_txretries[i])
				printf("%10s %10llu %dx retries\n", "",
				    (unsigned long long)
				    cnwis.stats.nws_txretries[i],
				    i);
	}

	if (rate == 0 && Sflag) {
		printf("      0x%02x link integrity field\n",
		    cnws.data[0x4e]);
		printf("      0x%02x connection quality\n",
		    cnws.data[0x54]);
		printf("      0x%02x spu\n",
		    cnws.data[0x55]);
		printf("      0x%02x link quality\n",
		    cnws.data[0x56]);
		printf("      0x%02x hhc\n",
		    cnws.data[0x58]);
		printf("      0x%02x mhs\n",
		    cnws.data[0x6b]);
		printf(" %04x %04x revision\n",
		    *(u_short *)&cnws.data[0x66], *(u_short *)&cnws.data[0x68]);
		printf("        %c%c id\n",
		    cnws.data[0x6e], cnws.data[0x6f]);
	}

	if (rate == 0)
		exit (0);

	if (ioctl(0, TIOCGWINSZ, &ts) < 0)
		ts.ts_lines = 24;
	c = 0;

	if (Sflag) for (;;) {
		if (c-- == 0) {
			printf("lif  cq spu  lq hhc mhs\n");
			c = ts.ts_lines - 3;
		}
		printf(" %02x  %02x  %02x  %02x  %02x  %02x\n",
                    cnws.data[0x4e],
                    cnws.data[0x54],
                    cnws.data[0x55],
                    cnws.data[0x56],
                    cnws.data[0x58],
		    cnws.data[0x6b]);
		fflush(stdout);
		if (ioctl(skt, SIOCGCNWSTATUS, (caddr_t)&cnws) < 0)
			err(1, "SIOCGCNWSTATUS");
		sleep (rate);
	}

	for (;;) {
		if (c-- == 0) {
			printf("%10s %10s %10s %10s %10s %10s\n",
			    "tx-request", "tx-okay", "tx-error", "tx-retry",
			    "rx", "rx-error");
			c = ts.ts_lines - 3;
			memset(&onwis, 0, sizeof(onwis));
		}
		printf("%10llu ", (unsigned long long)
		    (cnwis.stats.nws_tx - onwis.stats.nws_tx));
		printf("%10llu ", (unsigned long long)
		    (cnwis.stats.nws_txokay - onwis.stats.nws_txokay));
		printf("%10llu ", (unsigned long long)
		    (cnwis.stats.nws_txerrors - onwis.stats.nws_txerrors));
		printf("%10llu ", (unsigned long long)
		    (cnwis.stats.nws_txretries[0] -
			onwis.stats.nws_txretries[0]));
		printf("%10llu ", (unsigned long long)
		    (cnwis.stats.nws_rx - onwis.stats.nws_rx));
		printf("%10llu\n", (unsigned long long)
		    (cnwis.stats.nws_rxerrors - onwis.stats.nws_rxerrors));
		fflush(stdout);
		sleep (rate);
		onwis = cnwis;

		if (ioctl(skt, SIOCGCNWSTATS, (caddr_t)&cnwis) < 0)
			err(1, "SIOCGCNWSTATS");
		cnwis.stats.nws_txretries[0] = 0;
		for (i = 1; i < 16; ++i)
			cnwis.stats.nws_txretries[0] +=
			    cnwis.stats.nws_txretries[i] * i;
	}
}
