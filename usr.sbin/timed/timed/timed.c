/*	$NetBSD: timed.c,v 1.26 2018/02/04 09:01:13 mrg Exp $	*/

/*-
 * Copyright (c) 1985, 1993 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1985, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)timed.c	8.2 (Berkeley) 3/26/95";
#else
__RCSID("$NetBSD: timed.c,v 1.26 2018/02/04 09:01:13 mrg Exp $");
#endif
#endif /* not lint */

#define TSPTYPES
#include "globals.h"
#include <net/if.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <setjmp.h>
#include "pathnames.h"
#include <math.h>
#include <sys/types.h>
#include <sys/times.h>
#include <util.h>
#include <ifaddrs.h>
#include <err.h>

#ifdef HAVENIS
#include <netgroup.h>
#endif

int trace = 0;
int sock, sock_raw = -1;
int status = 0;
u_short sequence;			/* sequence number */
long delay1;
long delay2;

int nslavenets;				/* nets were I could be a slave */
int nmasternets;			/* nets were I could be a master */
int nignorednets;			/* ignored nets */
int nnets;				/* nets I am connected to */

FILE *fd;				/* trace file FD */

jmp_buf jmpenv;

struct netinfo *nettab = 0;
struct netinfo *slavenet;
int Mflag;
int justquit = 0;
int debug;

static struct nets {
	char	 *name;
	in_addr_t net;
	struct nets *next;
} *nets = 0;

struct hosttbl hosttbl[NHOSTS+1];	/* known hosts */

static struct goodhost {		/* hosts that we trust */
	char	name[MAXHOSTNAMELEN+1];
	struct goodhost *next;
	char	perm;
} *goodhosts;

static char *goodgroup;			/* net group of trusted hosts */
static void checkignorednets(void);
static void pickslavenet(struct netinfo *);
static void add_good_host(const char*,char);


/*
 * The timedaemons synchronize the clocks of hosts in a local area network.
 * One daemon runs as master, all the others as slaves. The master
 * performs the task of computing clock differences and sends correction
 * values to the slaves.
 * Slaves start an election to choose a new master when the latter disappears
 * because of a machine crash, network partition, or when killed.
 * A resolution protocol is used to kill all but one of the masters
 * that happen to exist in segments of a partitioned network when the
 * network partition is fixed.
 *
 * Authors: Riccardo Gusella & Stefano Zatti
 */

int
main(int argc, char *argv[])
{
	int on;
	int ret;
	int nflag, iflag;
	struct timeval ntime;
	struct servent *srvp;
	struct netinfo *ntp;
	struct netinfo *ntip;
	struct netinfo *savefromnet;
	struct netent *nentp;
	struct nets *nt;
	struct sockaddr_in server;
	uint16_t port;
	int c;
	extern char *optarg;
	extern int optind, opterr;
	struct ifaddrs *ifap, *ifa;

#define	IN_MSG "-i and -n make no sense together\n"
#ifdef HAVENIS
#define USAGE "[-dtM] [-i net|-n net] [-F host1 host2 ...] [-G netgp]\n"
#else
#define USAGE "[-dtM] [-i net|-n net] [-F host1 host2 ...]\n"
#endif /* HAVENIS */

	ntip = NULL;

	on = 1;
	nflag = OFF;
	iflag = OFF;

	opterr = 0;
	while ((c = getopt(argc, argv, "Mtdn:i:F:G:P:")) != -1) {
		switch (c) {
		case 'M':
			Mflag = 1;
			break;

		case 't':
			trace = 1;
			break;

		case 'n':
			if (iflag)
				errx(EXIT_FAILURE, "%s", IN_MSG);
			nflag = ON;
			addnetname(optarg);
			break;

		case 'i':
			if (nflag)
				errx(EXIT_FAILURE, "%s", IN_MSG);
			iflag = ON;
			addnetname(optarg);
			break;

		case 'F':
			add_good_host(optarg,1);
			while (optind < argc && argv[optind][0] != '-')
				add_good_host(argv[optind++], 1);
			break;

		case 'd':
			debug = 1;
			break;
		case 'G':
			if (goodgroup != 0)
				errx(EXIT_FAILURE, "timed: only one net group");
			goodgroup = optarg;
			break;
		default:
			errx(EXIT_FAILURE, "%s", USAGE);
			break;
		}
	}
	if (optind < argc)
		errx(EXIT_FAILURE, "%s", USAGE);

	/* If we care about which machine is the master, then we must
	 *	be willing to be a master
	 */
	if (0 != goodgroup || 0 != goodhosts)
		Mflag = 1;

	if (gethostname(hostname, sizeof(hostname)) < 0)
		err(EXIT_FAILURE, "gethostname");

	hostname[sizeof(hostname) - 1] = '\0';
	self.l_bak = &self;
	self.l_fwd = &self;
	self.h_bak = &self;
	self.h_fwd = &self;
	self.head = 1;
	self.good = 1;

	if (goodhosts != 0)		/* trust ourself */
		add_good_host(hostname,1);

	srvp = getservbyname("timed", "udp");
	if (srvp == NULL)
		errx(EXIT_FAILURE, "unknown service 'timed/udp'");

	port = srvp->s_port;
	(void)memset(&server, 0, sizeof(server));
	server.sin_port = srvp->s_port;
	server.sin_family = AF_INET;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		err(EXIT_FAILURE, "socket");

	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) 
		err(EXIT_FAILURE, "setsockopt");

	if (bind(sock, (struct sockaddr*)(void *)&server, sizeof(server))) {
		if (errno == EADDRINUSE)
			errx(EXIT_FAILURE, "time daemon already running");
		else
			err(EXIT_FAILURE, "bind");
	}

	/* choose a unique seed for random number generation */
	(void)gettimeofday(&ntime, 0);
	srandom((unsigned long)(ntime.tv_sec + ntime.tv_usec));

	sequence = (u_short)random();     /* initial seq number */

	/* rounds kernel variable time to multiple of 5 ms. */
	ntime.tv_sec = 0;
	ntime.tv_usec = -((ntime.tv_usec/1000) % 5) * 1000;
	(void)adjtime(&ntime, (struct timeval *)0);

	for (nt = nets; nt; nt = nt->next) {
		nentp = getnetbyname(nt->name);
		if (nentp == 0) {
			nt->net = inet_network(nt->name);
			if (nt->net != INADDR_NONE)
				nentp = getnetbyaddr(nt->net, AF_INET);
		}
		if (nentp != 0)
			nt->net = nentp->n_net;
		else if (nt->net == INADDR_NONE)
			errx(EXIT_FAILURE, "unknown net %s", nt->name);
		else if (nt->net == INADDR_ANY)
			errx(EXIT_FAILURE, "bad net %s", nt->name);
		else
			warnx("warning: %s unknown in /etc/networks",
				nt->name);

		if (0 == (nt->net & 0xff000000))
		    nt->net <<= 8;
		if (0 == (nt->net & 0xff000000))
		    nt->net <<= 8;
		if (0 == (nt->net & 0xff000000))
		    nt->net <<= 8;
	}
	if (getifaddrs(&ifap) != 0)
		err(EXIT_FAILURE, "get interface configuration");

	ntp = NULL;
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;
		if (!ntp)
			ntp = malloc(sizeof(struct netinfo));
		(void)memset(ntp, 0, sizeof(*ntp));
		ntp->my_addr=((struct sockaddr_in *)(void *)ifa->ifa_addr)->sin_addr;
		ntp->status = NOMASTER;

		if ((ifa->ifa_flags & IFF_UP) == 0)
			continue;
		if ((ifa->ifa_flags & IFF_BROADCAST) == 0 &&
		    (ifa->ifa_flags & IFF_POINTOPOINT) == 0) {
			continue;
		}

		ntp->mask = ((struct sockaddr_in *)(void *)
		    ifa->ifa_netmask)->sin_addr.s_addr;

		if (ifa->ifa_flags & IFF_BROADCAST) {
			ntp->dest_addr = *(struct sockaddr_in *)(void *)ifa->ifa_broadaddr;
			/* What if the broadcast address is all ones?
			 * So we cannot just mask ntp->dest_addr.  */
			ntp->net = ntp->my_addr;
			ntp->net.s_addr &= ntp->mask;
		} else {
			ntp->dest_addr = *(struct sockaddr_in *)(void *)ifa->ifa_dstaddr;
			ntp->net = ntp->dest_addr.sin_addr;
		}

		ntp->dest_addr.sin_port = port;

		for (nt = nets; nt; nt = nt->next) {
			if (ntohl(ntp->net.s_addr) == nt->net)
				break;
		}
		if ((nflag && !nt) || (iflag && nt))
			continue;

		ntp->next = NULL;
		if (nettab == NULL) {
			nettab = ntp;
		} else {
			ntip->next = ntp;
		}
		ntip = ntp;
		ntp = NULL;
	}
	freeifaddrs(ifap);
	if (ntp)
		(void) free(ntp);
	if (nettab == NULL)
		errx(EXIT_FAILURE, "no network usable");


	/* microseconds to delay before responding to a broadcast */
	delay1 = casual(1L, 100*1000L);

	/* election timer delay in secs. */
	delay2 = casual((long)MINTOUT, (long)MAXTOUT);


	if (!debug) {
		daemon(debug, 0);
		pidfile(NULL);
	}

	if (trace)
		traceon();
	openlog("timed", LOG_PID, LOG_DAEMON);

	/*
	 * keep returning here
	 */
	ret = setjmp(jmpenv);
	savefromnet = fromnet;
	setstatus();

	if (Mflag) {
		switch (ret) {

		case 0:
			checkignorednets();
			pickslavenet(0);
			break;
		case 1:
			/* Just lost our master */
			if (slavenet != 0)
				slavenet->status = election(slavenet);
			if (!slavenet || slavenet->status == MASTER) {
				checkignorednets();
				pickslavenet(0);
			} else {
				makeslave(slavenet);	/* prune extras */
			}
			break;

		case 2:
			/* Just been told to quit */
			justquit = 1;
			pickslavenet(savefromnet);
			break;
		}

		setstatus();
		if (!(status & MASTER) && sock_raw != -1) {
			/* sock_raw is not being used now */
			(void)close(sock_raw);
			sock_raw = -1;
		}

		if (status == MASTER)
			master();
		else
			slave();

	} else {
		if (sock_raw != -1) {
			(void)close(sock_raw);
			sock_raw = -1;
		}

		if (ret) {
			/* we just lost our master or were told to quit */
			justquit = 1;
		}
		for (ntp = nettab; ntp != NULL; ntp = ntp->next) {
			if (ntp->status == MASTER) {
				rmnetmachs(ntp);
				ntp->status = NOMASTER;
			}
		}
		checkignorednets();
		pickslavenet(0);
		setstatus();

		slave();
	}
	/* NOTREACHED */
	return(0);
}


/* suppress an upstart, untrustworthy, self-appointed master
 */
void
suppress(struct sockaddr_in *addr, char *name, struct netinfo *net)
{
	struct sockaddr_in tgt;
	char tname[MAXHOSTNAMELEN];
	struct tsp msg;
	static struct timeval wait;

	if (trace)
		fprintf(fd, "suppress: %s\n", name);
	tgt = *addr;
	(void)strlcpy(tname, name, sizeof(tname));

	while (0 != readmsg(TSP_ANY, ANYADDR, &wait, net)) {
		if (trace)
			fprintf(fd, "suppress:\tdiscarded packet from %s\n",
				    name);
	}

	syslog(LOG_NOTICE, "suppressing false master %s", tname);
	msg.tsp_type = TSP_QUIT;
	set_tsp_name(&msg, hostname);
	(void)acksend(&msg, &tgt, tname, TSP_ACK, 0, 1);
}

void
lookformaster(struct netinfo *ntp)
{
	struct tsp resp, conflict, *answer;
	struct timeval ntime;
	char mastername[MAXHOSTNAMELEN];
	struct sockaddr_in masteraddr;

	get_goodgroup(0);
	ntp->status = SLAVE;

	/* look for master */
	resp.tsp_type = TSP_MASTERREQ;
	set_tsp_name(&resp, hostname);
	answer = acksend(&resp, &ntp->dest_addr, ANYADDR,
			 TSP_MASTERACK, ntp, 0);
	if (answer != 0 && !good_host_name(answer->tsp_name)) {
		suppress(&from, answer->tsp_name, ntp);
		ntp->status = NOMASTER;
		answer = 0;
	}
	if (answer == 0) {
		/*
		 * Various conditions can cause conflict: races between
		 * two just started timedaemons when no master is
		 * present, or timedaemons started during an election.
		 * A conservative approach is taken.  Give up and became a
		 * slave, postponing election of a master until first
		 * timer expires.
		 */
		ntime.tv_sec = ntime.tv_usec = 0;
		answer = readmsg(TSP_MASTERREQ, ANYADDR, &ntime, ntp);
		if (answer != 0) {
			if (!good_host_name(answer->tsp_name)) {
				suppress(&from, answer->tsp_name, ntp);
				ntp->status = NOMASTER;
			}
			return;
		}

		ntime.tv_sec = ntime.tv_usec = 0;
		answer = readmsg(TSP_MASTERUP, ANYADDR, &ntime, ntp);
		if (answer != 0) {
			if (!good_host_name(answer->tsp_name)) {
				suppress(&from, answer->tsp_name, ntp);
				ntp->status = NOMASTER;
			}
			return;
		}

		ntime.tv_sec = ntime.tv_usec = 0;
		answer = readmsg(TSP_ELECTION, ANYADDR, &ntime, ntp);
		if (answer != 0) {
			if (!good_host_name(answer->tsp_name)) {
				suppress(&from, answer->tsp_name, ntp);
				ntp->status = NOMASTER;
			}
			return;
		}

		if (Mflag)
			ntp->status = MASTER;
		else
			ntp->status = NOMASTER;
		return;
	}

	ntp->status = SLAVE;
	get_tsp_name(answer, mastername, sizeof(mastername));
	masteraddr = from;

	/*
	 * If network has been partitioned, there might be other
	 * masters; tell the one we have just acknowledged that
	 * it has to gain control over the others.
	 */
	ntime.tv_sec = 0;
	ntime.tv_usec = 300000;
	answer = readmsg(TSP_MASTERACK, ANYADDR, &ntime, ntp);
	/*
	 * checking also not to send CONFLICT to ack'ed master
	 * due to duplicated MASTERACKs
	 */
	if (answer != NULL &&
	    strcmp(answer->tsp_name, mastername) != 0) {
		conflict.tsp_type = TSP_CONFLICT;
		set_tsp_name(&conflict, hostname);
		if (!acksend(&conflict, &masteraddr, mastername,
			     TSP_ACK, 0, 0)) {
			syslog(LOG_ERR,
			       "error on sending TSP_CONFLICT");
		}
	}
}

/*
 * based on the current network configuration, set the status, and count
 * networks;
 */
void
setstatus(void)
{
	struct netinfo *ntp;

	status = 0;
	nmasternets = nslavenets = nnets = nignorednets = 0;
	if (trace)
		fprintf(fd, "Net status:\n");
	for (ntp = nettab; ntp != NULL; ntp = ntp->next) {
		switch ((int)ntp->status) {
		case MASTER:
			nmasternets++;
			break;
		case SLAVE:
			nslavenets++;
			break;
		case NOMASTER:
		case IGNORE:
			nignorednets++;
			break;
		}
		if (trace) {
			fprintf(fd, "\t%-16s", inet_ntoa(ntp->net));
			switch ((int)ntp->status) {
			case NOMASTER:
				fprintf(fd, "NOMASTER\n");
				break;
			case MASTER:
				fprintf(fd, "MASTER\n");
				break;
			case SLAVE:
				fprintf(fd, "SLAVE\n");
				break;
			case IGNORE:
				fprintf(fd, "IGNORE\n");
				break;
			default:
				fprintf(fd, "invalid state %d\n",
					(int)ntp->status);
				break;
			}
		}
		nnets++;
		status |= ntp->status;
	}
	status &= ~IGNORE;
	if (trace)
		fprintf(fd,
		    "\tnets=%d masters=%d slaves=%d ignored=%d delay2=%ld\n",
		    nnets, nmasternets, nslavenets, nignorednets, (long)delay2);
}

void
makeslave(struct netinfo *net)
{
	struct netinfo *ntp;

	for (ntp = nettab; ntp != NULL; ntp = ntp->next) {
		if (ntp->status == SLAVE && ntp != net)
			ntp->status = IGNORE;
	}
	slavenet = net;
}

/*
 * Try to become master over ignored nets..
 */
static void
checkignorednets(void)
{
	struct netinfo *ntp;

	for (ntp = nettab; ntp != NULL; ntp = ntp->next) {
		if (!Mflag && ntp->status == SLAVE)
			break;

		if (ntp->status == IGNORE || ntp->status == NOMASTER) {
			lookformaster(ntp);
			if (!Mflag && ntp->status == SLAVE)
				break;
		}
	}
}

/*
 * choose a good network on which to be a slave
 *	The ignored networks must have already been checked.
 *	Take a hint about for a good network.
 */
static void
pickslavenet(struct netinfo *ntp)
{
	if (slavenet != 0 && slavenet->status == SLAVE) {
		makeslave(slavenet);		/* prune extras */
		return;
	}

	if (ntp == 0 || ntp->status != SLAVE) {
		for (ntp = nettab; ntp != 0; ntp = ntp->next) {
			if (ntp->status == SLAVE)
				break;
		}
	}
	makeslave(ntp);
}

/*
 * returns a random number in the range [inf, sup]
 */
long
casual(long inf, long sup)
{
	double value;

	value = ((double)(random() & 0x7fffffff)) / (0x7fffffff*1.0);
	return(inf + (sup - inf)*value);
}

char *
date(void)
{
	struct	timeval tv;
	time_t t;

	(void)gettimeofday(&tv, (struct timezone *)0);
	t = tv.tv_sec;
	return (ctime(&t));
}

void
addnetname(char *name)
{
	struct nets **netlist = &nets;

	while (*netlist)
		netlist = &((*netlist)->next);
	*netlist = calloc(1, sizeof **netlist);
	if (*netlist == NULL)
		err(EXIT_FAILURE, "malloc failed");
	(*netlist)->name = name;
}

/* note a host as trustworthy */
static void
add_good_host(const char* name,
	      char perm)		/* 1=not part of the netgroup */
{
	struct goodhost *ghp;
	struct hostent *hentp;

	ghp = calloc(1, sizeof(*ghp));
	if (!ghp) {
		syslog(LOG_ERR, "malloc failed");
		exit(EXIT_FAILURE);
	}

	(void)strncpy(&ghp->name[0], name, sizeof(ghp->name));
	ghp->next = goodhosts;
	ghp->perm = perm;
	goodhosts = ghp;

	hentp = gethostbyname(name);
	if (NULL == hentp && perm)
		warnx("unknown host %s", name);
}


/* update our image of the net-group of trustworthy hosts
 */
void
get_goodgroup(int force)
{
# define NG_DELAY (30*60*CLK_TCK)	/* 30 minutes */
	static unsigned long last_update;
	static int firsttime;
	unsigned long new_update;
	struct goodhost *ghp, **ghpp;
#ifdef HAVENIS
	struct hosttbl *htp;
	const char *mach, *usr, *dom;
#endif
	struct tms tm;

	if (firsttime == 0) {
		last_update = -NG_DELAY;
		firsttime++;
	}

	/* if no netgroup, then we are finished */
	if (goodgroup == 0 || !Mflag)
		return;

	/* Do not chatter with the netgroup master too often.
	 */
	new_update = times(&tm);
	if (new_update < last_update + NG_DELAY
	    && !force)
		return;
	last_update = new_update;

	/* forget the old temporary entries */
	ghpp = &goodhosts;
	while (0 != (ghp = *ghpp)) {
		if (!ghp->perm) {
			*ghpp = ghp->next;
			free(ghp);
		} else {
			ghpp = &ghp->next;
		}
	}

#ifdef HAVENIS
	/* quit now if we are not one of the trusted masters
	 */
	if (!innetgr(goodgroup, &hostname[0], 0,0)) {
		if (trace)
			(void)fprintf(fd, "get_goodgroup: %s not in %s\n",
				      &hostname[0], goodgroup);
		return;
	}
	if (trace)
		(void)fprintf(fd, "get_goodgroup: %s in %s\n",
				  &hostname[0], goodgroup);

	/* mark the entire netgroup as trusted */
	(void)setnetgrent(goodgroup);
	while (getnetgrent(&mach,&usr,&dom)) {
		if (0 != mach)
			add_good_host(mach,0);
	}
	(void)endnetgrent();

	/* update list of slaves */
	for (htp = self.l_fwd; htp != &self; htp = htp->l_fwd) {
		htp->good = good_host_name(&htp->name[0]);
	}
#endif /* HAVENIS */
}


/* see if a machine is trustworthy
 */
int					/* 1=trust hp to change our date */
good_host_name(char *name)
{
	struct goodhost *ghp = goodhosts;
	char c;

	if (!ghp || !Mflag)		/* trust everyone if no one named */
		return 1;

	c = *name;
	do {
		if (c == ghp->name[0]
		    && !strcasecmp(name, ghp->name))
			return 1;	/* found him, so say so */
	} while (0 != (ghp = ghp->next));

	if (!strcasecmp(name,hostname))	/* trust ourself */
		return 1;

	return 0;			/* did not find him */
}


