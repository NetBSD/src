/*	$NetBSD: lmcctl.c,v 1.6 2001/07/19 00:28:50 itojun Exp $	*/

/*-
 * Copyright (c) 1997-1999 LAN Media Corporation (LMC)
 * All rights reserved.  www.lanmedia.com
 *
 * This code is written by Michael Graff <graff@vix.com> for LMC.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. All marketing or advertising materials mentioning features or
 *    use of this software must display the following acknowledgement:
 *      This product includes software developed by LAN Media Corporation
 *      and its contributors.
 * 4. Neither the name of LAN Media Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY LAN MEDIA CORPORATION AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>

#include <netinet/in.h>

#include <dev/pci/if_lmc_types.h>
#include <dev/pci/if_lmcioctl.h>

#include "extern.h"

static void dumpdata(char *, lmc_ctl_t *);

void usage(char *);

#define DEFAULT_INTERFACE "lmc0"

void
usage(char *s)
{
	fprintf(stderr,
		"Usage: %s [-i interface] [-l speed] [-cCeEsKkSoO]\n", s);
	fprintf(stderr,
		"\tinterface is the interface name, default '%s'\n",
		DEFAULT_INTERFACE);
	fprintf(stderr, "\t-l sets line speed [T1]\n");
	fprintf(stderr, "\t-c sets external clock source [T1, HSSI]\n");
	fprintf(stderr, "\t-C sets internal clock source [T1, HSSI]\n");
	fprintf(stderr, "\t-e selects 16 bit CRC\n");
	fprintf(stderr, "\t-E selects 32 bit CRC\n");
#if defined(__NetBSD__) || defined(__FreeBSD__)
	fprintf(stderr, "\t-k enables Cisco HDLC keepalive protocol\n");
	fprintf(stderr, "\t-K disables Cisco HDLC keepalive protocol\n");
#endif
	fprintf(stderr, "\t-s turns DS3 scrambler off [DS3]\n");
	fprintf(stderr, "\t-S turns DS3 scrambler on [DS3]\n");
	fprintf(stderr, "\t-o sets DS3 cable length < 100 feet [DS3]\n");
	fprintf(stderr, "\t-O sets DS3 cable length > 100 feet [DS3]\n");
}

int
main(int argc, char **argv)
{
	lmc_ctl_t	ctl;
	int		fd;
	struct ifreq	ifr;
	int		ch;
	char	       *ifname = DEFAULT_INTERFACE;
	lmc_ctl_t	wanted;
	int		flag_c; /* clock source external, internal */
	int		flag_l; /* line speed */
	int		flag_s; /* Scrambler on, off */
	int		flag_o;	/* cable length < 100, > 100 */
	int		flag_e; /* crc 16, 32 */
	int		flag_k; /* HDLC keepalive */
	int		just_print;

	flag_c = 0;
	flag_l = 0;
	flag_s = 0;
	flag_o = 0;
	flag_e = 0;
	flag_k = 0;
	just_print = 1;

	while ((ch = getopt(argc, argv, "i:l:cCsSoOeEkK")) != -1) {
		switch (ch) {
		case 'i':
			ifname = optarg;
			break;
		case 'l':
			flag_l = 1;
			just_print = 0;
			wanted.clock_rate = atoi(optarg);
			break;
		case 's':
			flag_s = 1;
			just_print = 0;
			wanted.scrambler_onoff = LMC_CTL_OFF;
			break;
		case 'S':
			flag_s = 1;
			just_print = 0;
			wanted.scrambler_onoff = LMC_CTL_ON;
			break;
		case 'c':
			flag_c = 1;
			just_print = 0;
			wanted.clock_source = LMC_CTL_CLOCK_SOURCE_EXT;
			break;
		case 'C':
			flag_c = 1;
			just_print = 0;
			wanted.clock_source = LMC_CTL_CLOCK_SOURCE_INT;
			break;
		case 'o':
			flag_o = 1;
			just_print = 0;
			wanted.cable_length = LMC_CTL_CABLE_LENGTH_LT_100FT;
			break;
		case 'O':
			flag_o = 1;
			just_print = 0;
			wanted.cable_length = LMC_CTL_CABLE_LENGTH_GT_100FT;
			break;
		case 'e':
			flag_e = 1;
			just_print = 0;
			wanted.crc_length = LMC_CTL_CRC_LENGTH_16;
			break;
		case 'E':
			flag_e = 1;
			just_print = 0;
			wanted.crc_length = LMC_CTL_CRC_LENGTH_32;
			break;
		case 'k':
#if defined(__NetBSD__) || defined(__FreeBSD__)
			flag_k = 1;
			just_print = 0;
			wanted.keepalive_onoff = LMC_CTL_ON;
#else
			fprintf(stderr, "-k flag ignored on this system\n");
#endif
			break;
		case 'K':
#if defined(__NetBSD__) || defined(__FreeBSD__)
			flag_k = 1;
			just_print = 0;
			wanted.keepalive_onoff = LMC_CTL_OFF;
#else
			fprintf(stderr, "-K flag ignored on this system\n");
#endif
			break;
		case 'h':
		case '?':
			usage(argv[0]);
			exit(0);
		}
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		fprintf(stderr, "socket: %s\n", strerror(errno));
		exit(1);
	}

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	ifr.ifr_data = (caddr_t)&ctl;

	/*
	 * Fetch current settings
	 */
	if (ioctl(fd, LMCIOCGINFO, &ifr) < 0) {
		fprintf(stderr, "ioctl %s LMCIOCGINFO: %s\n",
			ifr.ifr_name, strerror(errno));
		exit(1);
	}

	/*
	 * If none of the flags are set, print status
	 */
	if (just_print) {
		dumpdata(ifname, &ctl);
		exit(0);
	}

	if (flag_c)
		ctl.clock_source = wanted.clock_source;
	if (flag_l) {
		lmc_av9110_freq(wanted.clock_rate, &wanted.cardspec.ssi);
		if (wanted.cardspec.ssi.f == 0) {
			printf("Unable to calculate requested rate.\n");
			exit(1);
		}
		if (wanted.cardspec.ssi.exact == 0)
			printf("Unable to calculate exact frequency,"
			       " using approximation %u\n",
			       wanted.cardspec.ssi.f);
		ctl.clock_rate = wanted.clock_rate;
		ctl.cardspec.ssi = wanted.cardspec.ssi;
		printf("rate: %u\n", ctl.cardspec.ssi.f);
#if 0
		{
			u_int32_t f;
			lmc_av9110_t *av;

			av = &wanted.cardspec.ssi;

			printf("m == %u, v == %u, n == %u, r == %u, x == %u\n",
			       av->m, av->v, av->n, av->r, av->x);

			f = (20000000 / av->m) * (av->v ? 8 : 1) * av->n;
			printf("fvco == %u\n", f);
			if (av->r == 1)
				f /= 2;
			else if (av->r == 2)
				f /= 4;
			else if (av->r == 3)
				f /= 8;
			printf("fclk == %u (%u)\n", f, f/16);
			if (av->x == 1)
				f /= 2;
			else if (av->x == 2)
				f /= 4;
			else if (av->x == 3)
				f /= 8;
			printf("fclkx == %u (%u)\n", f, f/16);
		}
#endif
	}
	if (flag_s)
		ctl.scrambler_onoff = wanted.scrambler_onoff;
	if (flag_o)
		ctl.cable_length = wanted.cable_length;
	if (flag_e)
		ctl.crc_length = wanted.crc_length;
#if defined(__NetBSD__) || defined(__FreeBSD__)
	if (flag_k)
		ctl.keepalive_onoff = wanted.keepalive_onoff;
#endif

	if (ioctl(fd, LMCIOCSINFO, &ifr) < 0) {
		fprintf(stderr, "ioctl %s LMCIOCSINFO: %s\n",
			ifr.ifr_name, strerror(errno));
		exit(1);
	}

	exit(0);
}

char *clock_sources[] = {
	"External/Line",
	"Internal"
};

static void
print_clocking(lmc_ctl_t *ctl)
{
	char *source;

	if (ctl->clock_source > 1)
		source = "Unknown Value";
	else
		source = clock_sources[ctl->clock_source];

	printf("\tClock source: %s\n", source);

	if (ctl->cardtype == LMC_CTL_CARDTYPE_LMC1000)
		printf("\tClock rate: %u\n", ctl->clock_rate);

	printf("\tApproximate detected rate: %u\n", ctl->ticks * 4096);
}

char *lmc_t1_cables[] = {
	"V.10/RS423", "EIA530A", "reserved", "X.21", "V.35",
	"EIA449/EIA530/V.36", "V.28/EIA232", "none", NULL
};

static void
print_t1_cable(lmc_ctl_t *ctl)
{
	char *type;

	if (ctl->cable_type > 7)
		type = "Invalid cable type";
	else
		type = lmc_t1_cables[ctl->cable_type];

	printf("\tCable type: %s\n", type);
}

static void
print_protocol(lmc_ctl_t *ctl)
{
	printf("\tHDLC Keepalive: ");
#if defined(__NetBSD__) || defined(__FreeBSD__)
	if (ctl->keepalive_onoff)
		printf("on\n");
	else
		printf("off\n");
#endif
#ifdef linux
	printf("passive\n");
#endif
#ifdef __bsdi__
	printf("on\n");
#endif
}

static void
dumpdata(char *name, lmc_ctl_t *ctl)
{
	/*
	 * Dump the data
	 */
	switch(ctl->cardtype) {
	case LMC_CTL_CARDTYPE_LMC5200:
		printf("%s: Lan Media Corporation LMC5200 (HSSI)\n", name);
		print_clocking(ctl);
		print_protocol(ctl);
		break;
	case LMC_CTL_CARDTYPE_LMC5245:
		printf("%s: Lan Media Corporation LMC5245 (DS3)\n", name);
		print_clocking(ctl);
		printf("\tCable length: %s than 100 feet\n",
		       (ctl->cable_length == LMC_CTL_CABLE_LENGTH_LT_100FT
			? "less" : "more"));
		printf("\tScrambler: %s\n",
		       (ctl->scrambler_onoff ? "on" : "off"));
		print_protocol(ctl);
		break;
	case LMC_CTL_CARDTYPE_LMC1000:
		printf("%s: Lan Media Corporation LMC1000 (SSI/V.35)\n", name);
		print_clocking(ctl);
		print_t1_cable(ctl);
		print_protocol(ctl);
		break;
	case LMC_CTL_CARDTYPE_LMC1200:
		printf("%s: Lan Media Corperation LMC1200 (T1/E1)\n", name);
		print_clocking(ctl);
		print_protocol(ctl);
		printf("\tLine protocol: %s\n",
		    ctl->circuit_type == LMC_CTL_CIRCUIT_TYPE_T1 ? "T1" : "E1");
		break;
	default:
		printf("%s: Unknown card type: %d\n", name, ctl->cardtype);
	}

	printf("\tCRC length: %d\n", ctl->crc_length);
}
