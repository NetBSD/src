/*	$NetBSD: slstats.c,v 1.11 1998/07/06 07:50:20 mrg Exp $	*/

/*
 * print serial line IP statistics:
 *	slstats [-i interval] [-v] [interface] [system [core]]
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: slstats.c,v 1.11 1998/07/06 07:50:20 mrg Exp $");
#endif

#define INET

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <net/slcompress.h>
#include <net/if_slvar.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct nlist nl[] = {
#define N_SOFTC 0
	{ "_sl_softc" },
	{ "" },
};

extern	char *__progname;	/* from crt0.o */

char	*kernel;		/* kernel for namelist */
char	*kmemf;			/* memory file */

kvm_t	*kd;

int	vflag;
unsigned interval = 5;
int	unit;

void	catchalarm __P((int));
void	intpr __P((void));
int	main __P((int, char **));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char errbuf[_POSIX2_LINE_MAX];
	gid_t egid = getegid();
	int ch;

	setegid(getgid());
	while ((ch = getopt(argc, argv, "i:M:N:v")) != -1) {
		switch (ch) {
		case 'i':
			interval = atoi(optarg);
			if (interval <= 0)
				usage();
			break;

		case 'M':
			kmemf = optarg;
			break;

		case 'N':
			kernel = optarg;
			break;

		case 'v':
			++vflag;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();

	while (argc--) {
		if (isdigit(*argv[0])) {
			unit = atoi(*argv);
			if (unit < 0)
				usage();
			continue;
		}

		/* Fall to here, we have bogus arguments. */
		usage();
	}

	/*
	 * Discard setgid privileges.  If not the running kernel, we toss
	 * them away totally so that bad guys can't print interesting stuff
	 * from kernel memory, otherwise switch back to kmem for the
	 * duration of the kvm_openfiles() call.
	 */
	if (kmemf != NULL || kernel != NULL)
		(void)setgid(getgid());
	else
		(void)setegid(egid);

	memset(errbuf, 0, sizeof(errbuf));
	if ((kd = kvm_openfiles(kernel, kmemf, NULL, O_RDONLY, errbuf)) == NULL)
		errx(1, "can't open kvm: %s", errbuf);

	/* get rid of it now anyway */
	if (kmemf == NULL && kernel == NULL)
		setgid(getgid());

	if (kvm_nlist(kd, nl) < 0 || nl[0].n_type == 0)
		errx(1, "%s: SLIP symbols not in namelist",
		    kernel == NULL ? _PATH_UNIX : kernel);

	intpr();
	exit(0);
}

#define V(offset) ((line % 20)? sc->offset - osc->offset : sc->offset)
#define AMT (sizeof(*sc) - 2 * sizeof(sc->sc_comp.tstate))

void
usage()
{

	(void)fprintf(stderr, "usage: %s [-M core] [-N system] [-i interval] %s",
	    __progname, "[-v] [unit]\n");
	exit(1);
}

u_char	signalled;			/* set if alarm goes off "early" */

/*
 * Print a running summary of interface statistics.
 * Repeat display every interval seconds, showing statistics
 * collected over that interval.  Assumes that interval is non-zero.
 * First line printed at top of screen is always cumulative.
 */
void
intpr()
{
	int line = 0;
	int oldmask;
	struct sl_softc *sc, *osc;
	u_long addr;

	addr = nl[N_SOFTC].n_value + unit * sizeof(struct sl_softc);
	sc = (struct sl_softc *)malloc(AMT);
	osc = (struct sl_softc *)malloc(AMT);
	memset((char *)osc, 0, AMT);

	while (1) {
		if (kvm_read(kd, addr, (char *)sc, AMT) != AMT)
			errx(1, "kvm_read: %s", kvm_geterr(kd));

		(void)signal(SIGALRM, catchalarm);
		signalled = 0;
		(void)alarm(interval);

		if ((line % 20) == 0) {
			(void)printf("%8.8s %6.6s %6.6s %6.6s %6.6s",
				"IN", "PACK", "COMP", "UNCOMP", "ERR");
			if (vflag)
				printf(" %6.6s %6.6s", "TOSS", "IP");
			(void)printf(" | %8.8s %6.6s %6.6s %6.6s %6.6s",
				"OUT", "PACK", "COMP", "UNCOMP", "IP");
			if (vflag)
				(void)printf(" %6.6s %6.6s", "SEARCH", "MISS");
			(void)putchar('\n');
		}
		(void)printf("%8lu %6ld %6u %6u %6u",
			V(sc_if.if_ibytes),
			(long)V(sc_if.if_ipackets),
			V(sc_comp.sls_compressedin),
			V(sc_comp.sls_uncompressedin),
			V(sc_comp.sls_errorin));
		if (vflag)
			(void)printf(" %6u %6lu",
				V(sc_comp.sls_tossed),
				V(sc_if.if_ipackets) -
				  V(sc_comp.sls_compressedin) -
				  V(sc_comp.sls_uncompressedin) -
				  V(sc_comp.sls_errorin));
		(void)printf(" | %8lu %6ld %6u %6u %6lu",
			V(sc_if.if_obytes),
			V(sc_if.if_opackets),
			V(sc_comp.sls_compressed),
			V(sc_comp.sls_packets) - V(sc_comp.sls_compressed),
			V(sc_if.if_opackets) - V(sc_comp.sls_packets));
		if (vflag)
			(void)printf(" %6u %6u",
				V(sc_comp.sls_searches),
				V(sc_comp.sls_misses));

		(void)putchar('\n');
		fflush(stdout);
		line++;
		oldmask = sigblock(sigmask(SIGALRM));
		if (!signalled)
			sigpause(0);
		sigsetmask(oldmask);
		signalled = 0;
		(void)alarm(interval);
		memmove((char *)osc, (char *)sc, AMT);
	}
}

/*
 * Called if an interval expires before sidewaysintpr has completed a loop.
 * Sets a flag to not wait for the alarm.
 */
void
catchalarm(dummy)
	int dummy;
{

	signalled = 1;
}
