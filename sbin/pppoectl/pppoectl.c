/*	$NetBSD: pppoectl.c,v 1.14 2003/09/06 19:35:35 martin Exp $	*/

/*
 * Copyright (c) 1997 Joerg Wunsch
 *
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
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPERS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE DEVELOPERS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * From: spppcontrol.c,v 1.3 1998/01/07 07:55:26 charnier Exp
 * From: ispppcontrol
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: pppoectl.c,v 1.14 2003/09/06 19:35:35 martin Exp $");
#endif


#include <sys/param.h>
#include <sys/callout.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_sppp.h>
#include <net/if_pppoe.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

static void usage(void);
static void print_error(const char *ifname, int error, const char * str);
static void print_vals(const char *ifname, int phase, struct spppauthcfg *sp,
	int lcp_timeout, time_t idle_timeout, int authfailures, 
	int max_auth_failures, u_int maxalive, time_t max_noreceive);
const char *phase_name(int phase);
const char *proto_name(int proto);
const char *authflags(int flags);
static void pppoectl_argument(char *arg);

int hz = 0;

int set_auth, set_lcp, set_idle_to, set_auth_failure, set_dns,
    clear_auth_failure_count, set_keepalive;
int maxalive = -1;
int max_noreceive = -1;
struct spppauthcfg spr;
struct sppplcpcfg lcp;
struct spppstatus status;
struct spppidletimeout timeout;
struct spppauthfailurestats authfailstats;
struct spppauthfailuresettings authfailset;
struct spppdnssettings dnssettings;
struct spppkeepalivesettings keepalivesettings;

int
main(int argc, char **argv)
{
	FILE *fp;
	int s, c;
	int errs = 0, verbose = 0, dump = 0, dns1 = 0, dns2 = 0;
	size_t len;
	const char *eth_if_name, *access_concentrator, *service;
	const char *ifname, *configname;
	char *line;
	int mib[2];
	struct clockinfo clockinfo;
	setprogname(argv[0]);

	eth_if_name = NULL;
	access_concentrator = NULL;
	service = NULL;
	configname = NULL;
	while ((c = getopt(argc, argv, "vde:f:s:a:n:")) != -1)
		switch (c) {
		case 'v':
			verbose++;
			break;

		case 'd':
			dump++;
			break;

		case 'e':
			eth_if_name = optarg;
			break;

		case 'f':
			configname = optarg;
			break;

		case 's':
			service = optarg;
			break;

		case 'a':
			access_concentrator = optarg;
			break;

		case 'n':
			if (strcmp(optarg, "1") == 0)
				dns1 = 1;
			else if (strcmp(optarg, "2") == 0)
				dns2 = 1;
			else {
				fprintf(stderr, "bad argument \"%s\" to -n (only 1 or two allowed)\n",
					optarg);
				errs++;
			}
			break;

		default:
			errs++;
			break;
		}
	argv += optind;
	argc -= optind;

	if (errs || argc < 1)
		usage();

	ifname = argv[0];

	/* use a random AF to create the socket */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(EX_UNAVAILABLE, "ifconfig: socket");

	argc--;
	argv++;

	if (eth_if_name) {
		struct pppoediscparms parms;
		int e;

		memset(&parms, 0, sizeof parms);
		strncpy(parms.ifname, ifname, sizeof(parms.ifname));
		strncpy(parms.eth_ifname, eth_if_name, sizeof(parms.eth_ifname));
		if (access_concentrator) {
			parms.ac_name = (char*)access_concentrator;
			parms.ac_name_len = strlen(access_concentrator);
		}
		if (service) {
			parms.service_name = (char*)service;
			parms.service_name_len = strlen(service);
		}

		e = ioctl(s, PPPOESETPARMS, &parms);
		if (e) 
			print_error(ifname, e, "PPPOESETPARMS");
		return 0;
	}

	if (dns1 || dns2) {
		/* print DNS addresses */
		int e;
		struct spppdnsaddrs addrs;
		memset(&addrs, 0, sizeof addrs);
		strncpy(addrs.ifname, ifname, sizeof addrs.ifname);
		e = ioctl(s, SPPPGETDNSADDRS, &addrs);
		if (e) 
			print_error(ifname, e, "SPPPGETDNSADDRS");
		if (dns1)
			printf("%d.%d.%d.%d\n",
				(addrs.dns[0] >> 24) & 0xff,
				(addrs.dns[0] >> 16) & 0xff,
				(addrs.dns[0] >> 8) & 0xff,
				addrs.dns[0] & 0xff);
		if (dns2)
			printf("%d.%d.%d.%d\n",
				(addrs.dns[1] >> 24) & 0xff,
				(addrs.dns[1] >> 16) & 0xff,
				(addrs.dns[1] >> 8) & 0xff,
				addrs.dns[1] & 0xff);
	}

	if (dump) {
		/* dump PPPoE session state */
		struct pppoeconnectionstate state;
		int e;
		
		memset(&state, 0, sizeof state);
		strncpy(state.ifname, ifname, sizeof state.ifname);
		e = ioctl(s, PPPOEGETSESSION, &state);
		if (e) 
			print_error(ifname, e, "PPPOEGETSESSION");

		printf("%s:\tstate = ", ifname);
		switch(state.state) {
		case PPPOE_STATE_INITIAL:
			printf("initial\n"); break;
		case PPPOE_STATE_PADI_SENT:
			printf("PADI sent\n"); break;
		case PPPOE_STATE_PADR_SENT:
			printf("PADR sent\n"); break;
		case PPPOE_STATE_SESSION:
			printf("session\n"); break;
		case PPPOE_STATE_CLOSING:
			printf("closing\n"); break;
		}
		printf("\tSession ID: 0x%x\n", state.session_id);
		printf("\tPADI retries: %d\n", state.padi_retry_no);
		printf("\tPADR retries: %d\n", state.padr_retry_no);
		
		return 0;
	}


	memset(&spr, 0, sizeof spr);
	strncpy(spr.ifname, ifname, sizeof spr.ifname);
	memset(&lcp, 0, sizeof lcp);
	strncpy(lcp.ifname, ifname, sizeof lcp.ifname);
	memset(&status, 0, sizeof status);
	strncpy(status.ifname, ifname, sizeof status.ifname);
	memset(&timeout, 0, sizeof timeout);
	strncpy(timeout.ifname, ifname, sizeof timeout.ifname);
	memset(&authfailstats, 0, sizeof &authfailstats);
	strncpy(authfailstats.ifname, ifname, sizeof authfailstats.ifname);
	memset(&authfailset, 0, sizeof authfailset);
	strncpy(authfailset.ifname, ifname, sizeof authfailset.ifname);
	memset(&dnssettings, 0, sizeof dnssettings);
	strncpy(dnssettings.ifname, ifname, sizeof dnssettings.ifname);
	memset(&keepalivesettings, 0, sizeof keepalivesettings);
	strncpy(keepalivesettings.ifname, ifname, sizeof keepalivesettings.ifname);

	mib[0] = CTL_KERN;
	mib[1] = KERN_CLOCKRATE;
	len = sizeof(clockinfo);
	if(sysctl(mib, 2, &clockinfo, &len, NULL, 0) == -1)
	{
		fprintf(stderr, "error, cannot sysctl kern.clockrate!\n");
		exit(1);
	}

	hz = clockinfo.hz;
		
	if (argc == 0 && !(dns1||dns2) && !configname) {
		/* list only mode */

		/* first pass, get name lenghts */
		if (ioctl(s, SPPPGETAUTHCFG, &spr) == -1)
			err(EX_OSERR, "SPPPGETAUTHCFG");
		/* now allocate buffers for strings */
		if (spr.myname_length)
			spr.myname = malloc(spr.myname_length);
		if (spr.hisname_length)
			spr.hisname = malloc(spr.hisname_length);
		/* second pass: get names too */
		if (ioctl(s, SPPPGETAUTHCFG, &spr) == -1)
			err(EX_OSERR, "SPPPGETAUTHCFG");

		if (ioctl(s, SPPPGETLCPCFG, &lcp) == -1)
			err(EX_OSERR, "SPPPGETLCPCFG");
		if (ioctl(s, SPPPGETSTATUS, &status) == -1)
			err(EX_OSERR, "SPPPGETSTATUS");
		if (ioctl(s, SPPPGETIDLETO, &timeout) == -1)
			err(EX_OSERR, "SPPPGETIDLETO");
		if (ioctl(s, SPPPGETAUTHFAILURES, &authfailstats) == -1)
			err(EX_OSERR, "SPPPGETAUTHFAILURES");
		if (ioctl(s, SPPPGETKEEPALIVE, &keepalivesettings) == -1)
			err(EX_OSERR, "SPPPGETKEEPALIVE");

		print_vals(ifname, status.phase, &spr, lcp.lcp_timeout,
		    timeout.idle_seconds, authfailstats.auth_failures,
		    authfailstats.max_failures,
		    keepalivesettings.maxalive,
		    keepalivesettings.max_noreceive);

		if (spr.hisname) free(spr.hisname);
		if (spr.myname) free(spr.myname);
		return 0;
	}

	/* first load the config file, then parse command line args */
	if (configname && (fp = fopen(configname, "r")))
		while ((line = fparseln(fp, NULL, NULL, NULL, 0)))
			pppoectl_argument(line);
       
	while (argc > 0) {
		pppoectl_argument(argv[0]);

		argv++;
		argc--;
	}

	if (set_auth) {
		if (ioctl(s, SPPPSETAUTHCFG, &spr) == -1)
			err(EX_OSERR, "SPPPSETAUTHCFG");
	}
	if (set_lcp) {
		if (ioctl(s, SPPPSETLCPCFG, &lcp) == -1)
			err(EX_OSERR, "SPPPSETLCPCFG");
	}
	if (set_idle_to) {
		if (ioctl(s, SPPPSETIDLETO, &timeout) == -1)
			err(EX_OSERR, "SPPPSETIDLETO");
	}
	if (set_auth_failure) {
		if (ioctl(s, SPPPSETAUTHFAILURE, &authfailset) == -1)
			err(EX_OSERR, "SPPPSETAUTHFAILURE");
	}
	if (clear_auth_failure_count && !(set_auth || set_auth_failure)) {
		/*
		 * We want to clear the auth failure count, but did not
		 * do that implicitly by setting authentication - so
		 * do a zero-effect auth setting change
		 */
		if (ioctl(s, SPPPGETAUTHFAILURES, &authfailstats) == -1)
			err(EX_OSERR, "SPPPGETAUTHFAILURES");
		authfailset.max_failures = authfailstats.max_failures;
		if (ioctl(s, SPPPSETAUTHFAILURE, &authfailset) == -1)
			err(EX_OSERR, "SPPPSETAUTHFAILURE");
	}
	if (set_dns) {
		if (ioctl(s, SPPPSETDNSOPTS, &dnssettings) == -1)
			err(EX_OSERR, "SPPPSETDNSOPTS");
	}
	if (set_keepalive) {
		if (ioctl(s, SPPPGETKEEPALIVE, &keepalivesettings) == -1)
			err(EX_OSERR, "SPPPGETKEEPALIVE");
		if (max_noreceive >= 0)
			keepalivesettings.max_noreceive = max_noreceive;
		if (maxalive >= 0)
			keepalivesettings.maxalive = maxalive;
		if (ioctl(s, SPPPSETKEEPALIVE, &keepalivesettings) == -1)
			err(EX_OSERR, "SPPPSETKEEPALIVE");
	}

	if (verbose) {
		if (ioctl(s, SPPPGETAUTHFAILURES, &authfailstats) == -1)
			err(EX_OSERR, "SPPPGETAUTHFAILURES");
		if (ioctl(s, SPPPGETKEEPALIVE, &keepalivesettings) == -1)
			err(EX_OSERR, "SPPPGETKEEPALIVE");
		print_vals(ifname, status.phase, &spr, lcp.lcp_timeout,
		    timeout.idle_seconds, authfailstats.auth_failures,
		    authfailstats.max_failures,
		    keepalivesettings.maxalive,
		    keepalivesettings.max_noreceive);
	}

	return 0;
}

static void
pppoectl_argument(char *arg)
{
	size_t off;
	const char *cp;

#define startswith(a,s) strncmp(a, s, (off = strlen(s))) == 0
	if (startswith(arg, "authproto=")) {
		cp = arg + off;
		if (strcmp(cp, "pap") == 0)
			spr.myauth =
				spr.hisauth = SPPP_AUTHPROTO_PAP;
		else if (strcmp(cp, "chap") == 0)
			spr.myauth = spr.hisauth = SPPP_AUTHPROTO_CHAP;
		else if (strcmp(cp, "none") == 0)
			spr.myauth = spr.hisauth = SPPP_AUTHPROTO_NONE;
		else
			errx(EX_DATAERR, "bad auth proto: %s", cp);
		set_auth = 1;
	} else if (startswith(arg, "myauthproto=")) {
		cp = arg + off;
		if (strcmp(cp, "pap") == 0)
			spr.myauth = SPPP_AUTHPROTO_PAP;
		else if (strcmp(cp, "chap") == 0)
			spr.myauth = SPPP_AUTHPROTO_CHAP;
		else if (strcmp(cp, "none") == 0)
			spr.myauth = SPPP_AUTHPROTO_NONE;
		else
			errx(EX_DATAERR, "bad auth proto: %s", cp);
		set_auth = 1;
	} else if (startswith(arg, "myauthname=")) {
		spr.myname = arg + off;
		spr.myname_length = strlen(spr.myname)+1;
		set_auth = 1;
	} else if (startswith(arg, "myauthsecret=") || startswith(arg, "myauthkey=")) {
		spr.mysecret = arg + off;
		spr.mysecret_length = strlen(spr.mysecret)+1;
		set_auth = 1;
	} else if (startswith(arg, "hisauthproto=")) {
		cp = arg + off;
		if (strcmp(cp, "pap") == 0)
			spr.hisauth = SPPP_AUTHPROTO_PAP;
		else if (strcmp(cp, "chap") == 0)
			spr.hisauth = SPPP_AUTHPROTO_CHAP;
		else if (strcmp(cp, "none") == 0)
			spr.hisauth = SPPP_AUTHPROTO_NONE;
		else
			errx(EX_DATAERR, "bad auth proto: %s", cp);
		set_auth = 1;
	} else if (startswith(arg, "hisauthname=")) {
		spr.hisname = arg + off;
		spr.hisname_length = strlen(spr.hisname)+1;
		set_auth = 1;
	} else if (startswith(arg, "hisauthsecret=") || startswith(arg, "hisauthkey=")) {
		spr.hissecret = arg + off;
		spr.hissecret_length = strlen(spr.hissecret)+1;
		set_auth = 1;
	} else if (startswith(arg, "max-noreceive=")) {
		max_noreceive = atoi(arg+off);
		if (max_noreceive < 0) {
			fprintf(stderr,
			    "max-noreceive value must be at least 0\n");
			max_noreceive = -1;
		} else {
			set_keepalive = 1;
		}
	} else if (startswith(arg, "max-alive-missed=")) {
		maxalive = atoi(arg+off);
		if (maxalive < 0) {
			fprintf(stderr, 
			    "max-alive-missed value must be at least 0\n");
			maxalive = -1;
		} else {
			set_keepalive = 1;
		}
	} else if (strcmp(arg, "callin") == 0)
		spr.hisauthflags |= SPPP_AUTHFLAG_NOCALLOUT;
	else if (strcmp(arg, "always") == 0)
		spr.hisauthflags &= ~SPPP_AUTHFLAG_NOCALLOUT;
	else if (strcmp(arg, "norechallenge") == 0)
		spr.hisauthflags |= SPPP_AUTHFLAG_NORECHALLENGE;
	else if (strcmp(arg, "rechallenge") == 0)
		spr.hisauthflags &= ~SPPP_AUTHFLAG_NORECHALLENGE;
#ifndef __NetBSD__
	else if (strcmp(arg, "enable-vj") == 0)
		spr.defs.enable_vj = 1;
	else if (strcmp(arg, "disable-vj") == 0)
		spr.defs.enable_vj = 0;
#endif
	else if (startswith(arg, "lcp-timeout=")) {
		int timeout_arg = atoi(arg+off);
		if ((timeout_arg > 20000) || (timeout_arg <= 0))
			errx(EX_DATAERR, "bad lcp timeout value: %s",
			     arg+off);
		lcp.lcp_timeout = timeout_arg * hz / 1000;
		set_lcp = 1;
	} else if (startswith(arg, "idle-timeout=")) {
		timeout.idle_seconds = (time_t)atol(arg+off);
		set_idle_to = 1;
	} else if (startswith(arg, "max-auth-failure=")) {
		authfailset.max_failures = atoi(arg+off);
		set_auth_failure = 1;
	} else if (strcmp(arg, "clear-auth-failure") == 0) {
		clear_auth_failure_count = 1;
	} else if (startswith(arg, "query-dns=")) {
		dnssettings.query_dns = atoi(arg+off);
		set_dns = 1;
	} else
		errx(EX_DATAERR, "bad parameter: \"%s\"", arg);
}

static void
usage(void)
{
	const char * prog = getprogname();
	fprintf(stderr,
	    "usage:\n"
	    "       %s [-f config] ifname [...]\n"
	    "       %s [-v] ifname [{my|his}auth{proto|name|secret}=...] \\\n"
            "                      [callin] [always] [{no}rechallenge]\n"
            "                      [query-dns=3]\n"
	    "           to set authentication names, passwords\n"
	    "           and (optional) paramaters\n"
	    "       %s [-v] ifname lcp-timeout=ms|idle-timeout=s|\n"
	    "                      max-noreceive=s|max-alive-missed=cnt|\n"
	    "                      max-auth-failure=count|clear-auth-failure\n"
	    "           to set general parameters\n"
	    "   or\n"
	    "       %s -e ethernet-ifname ifname\n"
	    "           to connect an ethernet interface for PPPoE\n"
	    "       %s [-a access-concentrator-name] [-s service-name] ifname\n"
	    "           to specify (optional) data for PPPoE sessions\n"
	    "       %s -d ifname\n"
	    "           to dump the current PPPoE session state\n"
	    "       %s -n (1|2) ifname\n"
	    "           to print DNS addresses retrieved via query-dns\n"
	    , prog, prog, prog, prog, prog, prog, prog);
	exit(EX_USAGE);
}

static void
print_vals(const char *ifname, int phase, struct spppauthcfg *sp, int lcp_timeout,
	time_t idle_timeout, int authfailures, int max_auth_failures,
	u_int maxalive_cnt, time_t max_noreceive_time)
{
#ifndef __NetBSD__
	time_t send, recv;
#endif

	printf("%s:\tphase=%s\n", ifname, phase_name(phase));
	if (sp->myauth) {
		printf("\tmyauthproto=%s myauthname=\"%s\"\n",
		       proto_name(sp->myauth),
		       sp->myname);
	}
	if (sp->hisauth) {
		printf("\thisauthproto=%s hisauthname=\"%s\"%s\n",
		       proto_name(sp->hisauth),
		       sp->hisname,
		       authflags(sp->hisauthflags));
	}
#ifndef __NetBSD__
	if (sp->defs.pp_phase > PHASE_DEAD) {
		send = time(NULL) - sp->defs.pp_last_sent;
		recv = time(NULL) - sp->defs.pp_last_recv;
		printf("\tidle_time=%ld\n", (send<recv)? send : recv);
	}
#endif

	printf("\tlcp timeout: %.3f s\n",
	       (double)lcp_timeout / hz);

	if (idle_timeout != 0)
		printf("\tidle timeout = %lu s\n", (unsigned long)idle_timeout);
	else
		printf("\tidle timeout = disabled\n");

	if (authfailures != 0)
		printf("\tauthentication failures = %d\n", authfailures);
	printf("\tmax-auth-failure = %d\n", max_auth_failures);

	printf("\tmax-noreceive = %ld seconds\n", (long)max_noreceive_time);
	printf("\tmax-alive-missed = %u unanwsered echo requests\n", maxalive_cnt);

#ifndef __NetBSD__
	printf("\tenable_vj: %s\n",
	       sp->defs.enable_vj ? "on" : "off");
#endif
}

const char *
phase_name(int phase)
{
	switch (phase) {
	case SPPP_PHASE_DEAD:		return "dead";
	case SPPP_PHASE_ESTABLISH:	return "establish";
	case SPPP_PHASE_TERMINATE:	return "terminate";
	case SPPP_PHASE_AUTHENTICATE:	return "authenticate";
	case SPPP_PHASE_NETWORK:	return "network";
	}
	return "illegal";
}

const char *
proto_name(int proto)
{
	static char buf[12];
	switch (proto) {
	case SPPP_AUTHPROTO_PAP:	return "pap";
	case SPPP_AUTHPROTO_CHAP:	return "chap";
	case SPPP_AUTHPROTO_NONE:	return "none";
	}
	snprintf(buf, sizeof(buf), "0x%x", (unsigned)proto);
	return buf;
}

const char *
authflags(int flags)
{
	static char buf[32];
	buf[0] = '\0';
	if (flags & SPPP_AUTHFLAG_NOCALLOUT)
		strlcat(buf, " callin", sizeof(buf));
	if (flags & SPPP_AUTHFLAG_NORECHALLENGE)
		strlcat(buf, " norechallenge", sizeof(buf));
	return buf;
}

static void
print_error(const char *ifname, int error, const char * str)
{
	if (error == -1)
		fprintf(stderr, "%s: interface not found\n", ifname);
	else
		fprintf(stderr, "%s: %s: %s\n", ifname, str, strerror(error));
	exit(EX_DATAERR);
}


