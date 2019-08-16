/*	$NetBSD: bootparamd.c,v 1.46.44.1 2019/08/16 19:25:31 martin Exp $	*/

/*
 * This code is not copyright, and is placed in the public domain.
 * Feel free to use and modify. Please send modifications and/or
 * suggestions + bug fixes to Klas Heggemann <klas@nada.kth.se>
 *
 * Various small changes by Theo de Raadt <deraadt@fsa.ca>
 * Parser rewritten (adding YP support) by Roland McGrath <roland@frob.com>
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: bootparamd.c,v 1.46.44.1 2019/08/16 19:25:31 martin Exp $");
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <fnmatch.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <util.h>
#include <ifaddrs.h>

#include <net/if.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpcsvc/bootparam_prot.h>
#ifdef YP
#include <rpcsvc/ypclnt.h>
#endif

#include "pathnames.h"

#define MAXLEN 800

static char hostname[MAX_MACHINE_NAME];
static char askname[MAX_MACHINE_NAME];
static char domain_name[MAX_MACHINE_NAME];

extern void bootparamprog_1(struct svc_req *, SVCXPRT *);

extern int	_rpcsvcdirty;
int	_rpcpmstart = 0;
int     debug = 0;
int     dolog = 0;
struct in_addr route_addr;
struct sockaddr_in my_addr;
const char *bootpfile = _PATH_BOOTPARAMS;
char   *iface = NULL;

static int	lookup_bootparam(char *, char *, char *, char **, char **);
__dead static void	usage(void);
static int get_localaddr(const char *, struct sockaddr_in *);


/*
 * ever familiar
 */
int
main(int argc, char  *argv[])
{
	SVCXPRT *transp;
	struct hostent *he;
	struct stat buf;
	int    c;

	while ((c = getopt(argc, argv, "di:sr:f:")) != -1)
		switch (c) {
		case 'd':
			debug = 1;
			break;
		case 'i':
			iface = optarg;
			break;
		case 'r':
			if (isdigit((unsigned char)*optarg)) {
				if (inet_aton(optarg, &route_addr) != 0)
					break;
			}
			he = gethostbyname(optarg);
			if (he == 0) {
				warnx("no such host: %s", optarg);
				usage();
			}
			memmove(&route_addr.s_addr, he->h_addr, he->h_length);
			break;
		case 'f':
			bootpfile = optarg;
			break;
		case 's':
			dolog = 1;
#ifndef LOG_DAEMON
			openlog("rpc.bootparamd", 0, 0);
#else
			openlog("rpc.bootparamd", 0, LOG_DAEMON);
			setlogmask(LOG_UPTO(LOG_NOTICE));
#endif
			break;
		default:
			usage();
		}

	if (stat(bootpfile, &buf))
		err(1, "%s", bootpfile);

	if (route_addr.s_addr == 0) {
		if (get_localaddr(NULL, &my_addr) != 0)
			errx(1, "router address not found");
		route_addr.s_addr = my_addr.sin_addr.s_addr;
	}
	if (!debug) {
		if (daemon(0, 0))
			err(1, "can't detach from terminal");
	}
	pidfile(NULL);

	(void) pmap_unset(BOOTPARAMPROG, BOOTPARAMVERS);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL)
		errx(1, "can't create udp service");

	if (!svc_register(transp, BOOTPARAMPROG, BOOTPARAMVERS, bootparamprog_1,
	    IPPROTO_UDP))
/*
 * Do NOT change the "%u" in the format string below to "%lu". If your
 * build fails update the "rpcgen" program and use "make cleandir" and
 * "make includes" in "src/lib/librpcsvc" afterwards.
 */
		errx(1, "unable to register BOOTPARAMPROG version %u, udp",
		    BOOTPARAMVERS);

	svc_run();
	errx(1, "svc_run returned");
}

bp_whoami_res *
bootparamproc_whoami_1_svc(bp_whoami_arg *whoami, struct svc_req *rqstp)
{
	static bp_whoami_res res;
	struct hostent *he;
	struct in_addr haddr;
	int e;

	if (debug)
		warnx("whoami got question for %d.%d.%d.%d",
		    255 & whoami->client_address.bp_address_u.ip_addr.net,
		    255 & whoami->client_address.bp_address_u.ip_addr.host,
		    255 & whoami->client_address.bp_address_u.ip_addr.lh,
		    255 & whoami->client_address.bp_address_u.ip_addr.impno);
	if (dolog)
		syslog(LOG_NOTICE, "whoami got question for %d.%d.%d.%d",
		    255 & whoami->client_address.bp_address_u.ip_addr.net,
		    255 & whoami->client_address.bp_address_u.ip_addr.host,
		    255 & whoami->client_address.bp_address_u.ip_addr.lh,
		    255 & whoami->client_address.bp_address_u.ip_addr.impno);

	memmove((char *) &haddr,
	    (char *) &whoami->client_address.bp_address_u.ip_addr,
	    sizeof(haddr));
	he = gethostbyaddr((char *) &haddr, sizeof(haddr), AF_INET);
	if (he) {
		(void)strlcpy(askname, he->h_name, sizeof(askname));
	} else {
		(void)strlcpy(askname, inet_ntoa(haddr), sizeof(askname));
	}

	if (debug)
		warnx("This is host %s", askname);
	if (dolog)
		syslog(LOG_NOTICE, "This is host %s", askname);

	if ((e = lookup_bootparam(askname, hostname, NULL, NULL, NULL)) == 0) {
		res.client_name = hostname;
		getdomainname(domain_name, MAX_MACHINE_NAME);
		res.domain_name = domain_name;

		if (res.router_address.address_type != IP_ADDR_TYPE) {
			res.router_address.address_type = IP_ADDR_TYPE;
			memmove(&res.router_address.bp_address_u.ip_addr,
			    &route_addr.s_addr,4);
		}
		if (debug)
			warnx("Returning %s   %s    %d.%d.%d.%d",
			    res.client_name, res.domain_name,
			    255 & res.router_address.bp_address_u.ip_addr.net,
			    255 & res.router_address.bp_address_u.ip_addr.host,
			    255 & res.router_address.bp_address_u.ip_addr.lh,
			    255 &res.router_address.bp_address_u.ip_addr.impno);
		if (dolog)
			syslog(LOG_NOTICE, "Returning %s   %s    %d.%d.%d.%d",
			    res.client_name, res.domain_name,
			    255 & res.router_address.bp_address_u.ip_addr.net,
			    255 & res.router_address.bp_address_u.ip_addr.host,
			    255 & res.router_address.bp_address_u.ip_addr.lh,
			    255 &res.router_address.bp_address_u.ip_addr.impno);

		return (&res);
	}
	errno = e;
	if (debug)
		warn("whoami failed");
	if (dolog)
		syslog(LOG_NOTICE, "whoami failed %m");
	return (NULL);
}


bp_getfile_res *
bootparamproc_getfile_1_svc(bp_getfile_arg *getfile, struct svc_req *rqstp)
{
	static bp_getfile_res res;
	struct hostent *he;
	int     error;

	if (debug)
		warnx("getfile got question for \"%s\" and file \"%s\"",
		    getfile->client_name, getfile->file_id);

	if (dolog)
		syslog(LOG_NOTICE,
		    "getfile got question for \"%s\" and file \"%s\"",
		    getfile->client_name, getfile->file_id);

	he = NULL;
	he = gethostbyname(getfile->client_name);
	if (!he) {
		if (debug)
			warnx("getfile can't resolve client %s",
			    getfile->client_name);
		if (dolog)
			syslog(LOG_NOTICE, "getfile can't resolve client %s",
			    getfile->client_name);
		return (NULL);
	}

	(void)strlcpy(askname, he->h_name, sizeof(askname));
	error = lookup_bootparam(askname, NULL, getfile->file_id,
	    &res.server_name, &res.server_path);
	if (error == 0) {
		he = gethostbyname(res.server_name);
		if (!he) {
			if (debug)
				warnx("getfile can't resolve server %s for %s",
			    res.server_name, getfile->client_name);
		if (dolog)
			syslog(LOG_NOTICE,
			    "getfile can't resolve server %s for %s",
			    res.server_name, getfile->client_name);
		return (NULL);

		}
		memmove(&res.server_address.bp_address_u.ip_addr,
		    he->h_addr, 4);
		res.server_address.address_type = IP_ADDR_TYPE;
	} else if (error == ENOENT && !strcmp(getfile->file_id, "dump")) {
		/* Special for dump, answer with null strings. */
		res.server_name[0] = '\0';
		res.server_path[0] = '\0';
		memset(&res.server_address.bp_address_u.ip_addr, 0, 4);
	} else {
		if (debug)
			warnx("getfile lookup failed for %s",
			    getfile->client_name);
		if (dolog)
			syslog(LOG_NOTICE,
			    "getfile lookup failed for %s",
			    getfile->client_name);
		return (NULL);
	}

	if (debug)
		warnx(
		    "returning server:%s path:%s address: %d.%d.%d.%d",
		    res.server_name, res.server_path,
		    255 & res.server_address.bp_address_u.ip_addr.net,
		    255 & res.server_address.bp_address_u.ip_addr.host,
		    255 & res.server_address.bp_address_u.ip_addr.lh,
		    255 & res.server_address.bp_address_u.ip_addr.impno);
	if (dolog)
		syslog(LOG_NOTICE,
		    "returning server:%s path:%s address: %d.%d.%d.%d",
		    res.server_name, res.server_path,
		    255 & res.server_address.bp_address_u.ip_addr.net,
		    255 & res.server_address.bp_address_u.ip_addr.host,
		    255 & res.server_address.bp_address_u.ip_addr.lh,
		    255 & res.server_address.bp_address_u.ip_addr.impno);
	return (&res);
}


static int
lookup_bootparam(char *client, char *client_canonical, char *id,
    char **server, char **path)
{
	FILE   *f = fopen(bootpfile, "r");
#ifdef YP
	static char *ypbuf = NULL;
	static int ypbuflen = 0;
#endif
	static char buf[BUFSIZ];
	char   *canon = NULL, *bp, *word = NULL;
	size_t  idlen = id == NULL ? 0 : strlen(id);
	int     contin = 0;
	int     found = 0;

	if (f == NULL)
		return EINVAL;	/* ? */

	while (fgets(buf, sizeof buf, f)) {
		int     wascontin = contin;
		contin = buf[strlen(buf) - 2] == '\\';
		bp = buf + strspn(buf, " \t\n");

		switch (wascontin) {
		case -1:
			/* Continuation of uninteresting line */
			contin *= -1;
			continue;
		case 0:
			/* New line */
			contin *= -1;
			if (*bp == '#')
				continue;
			if ((word = strsep(&bp, " \t\n")) == NULL)
				continue;
#ifdef YP
			/* A + in the file means try YP now */
			if (!strcmp(word, "+")) {
				char   *ypdom;

				if (yp_get_default_domain(&ypdom) ||
				    yp_match(ypdom, "bootparams", client,
					strlen(client), &ypbuf, &ypbuflen))
					continue;
				bp = ypbuf;
				word = client;
				contin *= -1;
				break;
			}
#endif
			if (debug)
				warnx("match %s with %s", word, client);

#define	HASGLOB(str) \
	(strchr(str, '*') != NULL || \
	 strchr(str, '?') != NULL || \
	 strchr(str, '[') != NULL || \
	 strchr(str, ']') != NULL)

			/* See if this line's client is the one we are
			 * looking for */
			if (fnmatch(word, client, FNM_CASEFOLD) == 0) {
				/*
				 * Match.  The token may be globbed, we
				 * can't just return that as the canonical
				 * name.  Check to see if the token has any
				 * globbing characters in it (*, ?, [, ]).
				 * If so, just return the name we already
				 * have.  Otherwise, return the token.
				 */
				if (HASGLOB(word))
					canon = client;
				else
					canon = word;
			} else {
				struct hostent *hp;
				/*
				 * If it didn't match, try getting the
				 * canonical host name of the client
				 * on this line, if it's not a glob,
				 * and comparing it to the client we
				 * are looking up.
				 */
				if (HASGLOB(word)) {
					if (debug)
						warnx("Skipping non-match: %s",
						    word);
					continue;
				}
				if ((hp = gethostbyname(word)) == NULL) {
					if (debug)
						warnx(
					    "Unknown bootparams host %s",
					    word);
					if (dolog)
						syslog(LOG_NOTICE,
					    "Unknown bootparams host %s",
					    word);
					continue;
				}
				if (strcasecmp(hp->h_name, client) != 0)
					continue;
				canon = hp->h_name;
			}

#undef HASGLOB

			contin *= -1;
			break;
		case 1:
			/* Continued line we want to parse below */
			break;
		}

		assert(canon != NULL);
		if (client_canonical)
			strncpy(client_canonical, canon, MAX_MACHINE_NAME);

		/* We have found a line for CLIENT */
		if (id == NULL) {
			(void) fclose(f);
			return 0;
		}

		/* Look for a value for the parameter named by ID */
		while ((word = strsep(&bp, " \t\n")) != NULL) {
			if (!strncmp(word, id, idlen) && word[idlen] == '=') {
				/* We have found the entry we want */
				*server = &word[idlen + 1];
				*path = strchr(*server, ':');
				if (*path == NULL)
					/* Malformed entry */
					continue;
				*(*path)++ = '\0';
				(void) fclose(f);
				return 0;
			}
		}

		found = 1;
	}

	(void) fclose(f);
	return found ? ENOENT : EPERM;
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage: %s [-ds] [-i interface] [-r router] [-f bootparamsfile]\n",
	    getprogname());
	exit(1);
}

static int
get_localaddr(const char *ifname, struct sockaddr_in *sin)
{
	struct ifaddrs *ifap, *ifa;

	if (getifaddrs(&ifap) != 0)
		return -1;

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifname && strcmp(ifname, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;
		if (ifa->ifa_addr->sa_len != sizeof(*sin))
			continue;

		/* no loopback please */
#ifdef IFF_LOOPBACK
		if (ifa->ifa_flags & IFF_LOOPBACK)
			continue;
#else
		if (strncmp(ifa->ifa_name, "lo", 2) == 0 &&
		    (isdigit(ifa->ifa_name[2]) || ifa->ifa_name[2] == '\0'))
			continue;
#endif

		if (!iface || strcmp(ifa->ifa_name, iface) == 0)
			;
		else
			continue;

		/* candidate found */
		memcpy(sin, ifa->ifa_addr, ifa->ifa_addr->sa_len);
		freeifaddrs(ifap);
		return 0;
	}

	/* no candidate */
	freeifaddrs(ifap);
	return -1;
}
