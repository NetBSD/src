/*
 * print PPP interface statistics:
 *      pppstats [-v] [-c count] [-w wait] [-M core] [-N system ] [interface]
 * print SLIP interface statistics:
 *      slstats [-v] [-c count] [-w wait] [-M core] [-N system ] [interface]
 *
 *
 *	Brad Parker (brad@cayman.com) 6/92
 *
 * from the original "slstats" by Van Jacobson
 *
 * Copyright (c) 1989, 1990, 1991, 1992 Regents of the University of
 * California. All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	Van Jacobson (van@ee.lbl.gov), Dec 31, 1989:
 *	- Initial distribution.
 */

#ifndef lint
/*static char rcsid[] =
    "@(#) $Header: /cvsroot/src/usr.sbin/pppd/pppstats/Attic/pppstats.c,v 1.8 1994/11/15 07:20:54 glass Exp $ (LBL)";*/
static char rcsid[] = "$Id: pppstats.c,v 1.8 1994/11/15 07:20:54 glass Exp $";
#endif

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#define	VJC	1		/* XXX */
#define INET			/* XXX */

#include <net/slcompress.h>
#include <net/if_slvar.h>
#include <net/if_ppp.h>

#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define V(offset) ((line != 0) ? sc->offset - osc->offset : sc->offset)

#ifdef PPP
#define STRUCT			struct ppp_softc
#define NLIST_ENTRY		"_ppp_softc"
#define INTERFACE_PREFIX 	"ppp"
#else
#define STRUCT			struct sl_softc
#define NLIST_ENTRY 		"_sl_softc"
#define INTERFACE_PREFIX 	"sl"
#endif

struct nlist nl[] = {
#define N_SOFTC 0
        { NLIST_ENTRY },
	"",
};

kvm_t	*kd;
char	*progname, interface[IFNAMSIZ];
int	count, infinite, interval, signalled, unit, vflag;

void
usage()
{
	fprintf(stderr,
		"usage: %s [-v] [-c count] [-w wait] [-M core] [-N system ] [interface]\n",
		progname);
	exit(1);
}

/*
 * Called if an interval expires before intpr has completed a loop.
 * Sets a flag to not wait for the alarm.
 */
void
catchalarm()
{
	signalled = 1;
}

/*
 * Print a running summary of interface statistics.
 * Repeat display every interval seconds, showing statistics
 * collected over that interval.  Assumes that interval is non-zero.
 * First line printed at top of screen is always cumulative.
 */
void
intpr()
{
	STRUCT 	*sc, *osc;
	off_t 	addr;
	int 	oldmask, line = 0;

	addr = nl[N_SOFTC].n_value + unit * sizeof(STRUCT);
	sc = (STRUCT *) malloc(sizeof(STRUCT));
	osc = (STRUCT *) malloc(sizeof(STRUCT));
	memset(osc, 0, sizeof(STRUCT));

	while (1) {
		if (kvm_read(kd, addr, sc,
			     sizeof(STRUCT)) != sizeof(STRUCT))
			errx(1, "reading statistics: %s", kvm_geterr(kd));
		
		(void)signal(SIGALRM, catchalarm);
		signalled = 0;
		(void)alarm(interval);

		if ((line % 20) == 0) {
			printf("%8.8s %6.6s %6.6s %6.6s %6.6s",
				"IN", "PACK", "COMP", "UNCOMP", "ERR");
			if (vflag)
				printf(" %6.6s %6.6s", "TOSS", "IP");
			printf(" | %8.8s %6.6s %6.6s %6.6s %6.6s",
				"OUT", "PACK", "COMP", "UNCOMP", "IP");
			if (vflag)
				printf(" %6.6s %6.6s", "SEARCH", "MISS");
			putchar('\n');
		}
		printf("%8u %6d %6u %6u %6u",
		       V(sc_if.if_ibytes),
		       V(sc_if.if_ipackets),
		       V(sc_comp.sls_compressedin),
		       V(sc_comp.sls_uncompressedin),
		       V(sc_comp.sls_errorin));
		if (vflag)
			printf(" %6u %6u",
			       V(sc_comp.sls_tossed),
			       V(sc_if.if_ipackets) -
			       V(sc_comp.sls_compressedin) -
			       V(sc_comp.sls_uncompressedin) -
			       V(sc_comp.sls_errorin));
		printf(" | %8u %6d %6u %6u %6u",
			V(sc_if.if_obytes),
			V(sc_if.if_opackets),
			V(sc_comp.sls_compressed),
			V(sc_comp.sls_packets) - V(sc_comp.sls_compressed),
			V(sc_if.if_opackets) - V(sc_comp.sls_packets));
		if (vflag)
			printf(" %6u %6u",
				V(sc_comp.sls_searches),
				V(sc_comp.sls_misses));

		putchar('\n');
		fflush(stdout);
		line++;

		count--;
		if (!infinite && !count)
			break;
		oldmask = sigblock(sigmask(SIGALRM));
		if (signalled == 0)
			sigpause(0);
		sigsetmask(oldmask);
		signalled = 0;
		(void)alarm(interval);
		memcpy(osc, sc, sizeof(STRUCT));
	}
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char 	errbuf[_POSIX2_LINE_MAX], tmp[IFNAMSIZ];
	struct 	ifreq ifr;
	char 	*kernel, *core;
	int 	c, s;

	kernel = core = NULL;
	strcpy(interface, INTERFACE_PREFIX);
	strcat(interface, "0");
	if ((progname = strrchr(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		++progname;

	while ((c = getopt(argc, argv, ":c:vw:M:N:")) != -1) {
		switch (c) {
		case 'c':
			count = atoi(optarg);
			if (count <= 0)
				usage();
			break;
		case 'v':
			++vflag;
			break;
		case 'w':
			interval = atoi(optarg);
			if (interval <= 0)
				usage();
			break;
		case 'M':
			core = optarg;
			break;
		case 'N':
			kernel = optarg;
			break;
		case ':':
			fprintf(stderr,
				"option -%c requires an argument\n", optopt);
			usage();
			break;
		case '?':
			fprintf(stderr, "unrecognized option: -%c\n", optopt);
			usage();
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (!interval && count)
		interval = 5;
	if (interval && !count)
		infinite = 1;
	if (!interval && !count)
		count = 1;

	if (argc > 1)
		usage();
	if (argc > 0) {
		if (strlen(argv[0]) > sizeof(interface))
			errx(1, "invalid interface specified %s", interface);
		strcpy(interface, argv[0]);
	}

	strcpy(tmp, INTERFACE_PREFIX);
	strcat(tmp, "%d");
	if (sscanf(interface, tmp, &unit) != 1)
		errx(1, "invalid interface '%s' specified", interface);
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		err(1, "creating socket");
	strcpy(ifr.ifr_name, interface);
	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		errx(1, "unable to confirm existence of interface '%s'",
		    interface);
	close(s);

	kd = kvm_openfiles(kernel, core, NULL, O_RDONLY, errbuf);
	if (kd == NULL)
		errx(1, "%s", errbuf);
	if (kvm_nlist(kd, nl) != 0)
		errx(1, "%s", kvm_geterr(kd));

	intpr();
	exit(0);
}
