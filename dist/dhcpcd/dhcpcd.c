/* 
 * dhcpcd - DHCP client daemon
 * Copyright 2006-2008 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

const char copyright[] = "Copyright (c) 2006-2008 Roy Marples";

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <arpa/inet.h>

#include <errno.h>
#include <getopt.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "client.h"
#include "dhcpcd.h"
#include "dhcp.h"
#include "net.h"
#include "logger.h"

/* Don't set any optional arguments here so we retain POSIX
 * compatibility with getopt */
#define OPTS "c:df:h:i:kl:m:no:pr:s:t:u:xAC:DEF:GI:LO:TV"

static int doversion = 0;
static int dohelp = 0;
static const struct option longopts[] = {
	{"script",      required_argument,  NULL, 'c'},
	{"debug",       no_argument,        NULL, 'd'},
	{"config",	required_argument,  NULL, 'f'},
	{"hostname",    optional_argument,  NULL, 'h'},
	{"classid",     optional_argument,  NULL, 'i'},
	{"release",     no_argument,        NULL, 'k'},
	{"leasetime",   required_argument,  NULL, 'l'},
	{"metric",      required_argument,  NULL, 'm'},
	{"renew",       no_argument,        NULL, 'n'},
	{"option",      required_argument,  NULL, 'o'},
	{"persistent",  no_argument,        NULL, 'p'},
	{"inform",      optional_argument,  NULL, 's'},
	{"request",     optional_argument,  NULL, 'r'},
	{"timeout",     required_argument,  NULL, 't'},
	{"userclass",   required_argument,  NULL, 'u'},
	{"exit",        no_argument,        NULL, 'x'},
	{"noarp",       no_argument,        NULL, 'A'},
	{"nohook",	required_argument,  NULL, 'C'},
	{"duid",        no_argument,        NULL, 'D'},
	{"lastlease",   no_argument,        NULL, 'E'},
	{"fqdn",        optional_argument,  NULL, 'F'},
	{"nogateway",   no_argument,        NULL, 'G'},
	{"clientid",    optional_argument,  NULL, 'I'},
	{"noipv4ll",    no_argument,        NULL, 'L'},
	{"nooption",    optional_argument,  NULL, 'O'},
	{"test",        no_argument,        NULL, 'T'},
	{"variables",   no_argument,        NULL, 'V'},
	{"help",        no_argument,        &dohelp, 1},
	{"version",     no_argument,        &doversion, 1},
#ifdef THERE_IS_NO_FORK
	{"daemonised",	no_argument,        NULL, 'X'},
	{"skiproutes",  required_argument,  NULL, 'Z'},
#endif
#ifdef CMDLINE_COMPAT
	{"nohostname",  no_argument,        NULL, 'H'},
	{"nomtu",       no_argument,        NULL, 'M'},
	{"nontp",       no_argument,        NULL, 'N'},
	{"nodns",       no_argument,        NULL, 'R'},
	{"msscr",       no_argument,        NULL, 'S'},
	{"nonis",       no_argument,        NULL, 'Y'},
#endif
	{NULL,          0,                  NULL, '\0'}
};

#ifdef THERE_IS_NO_FORK
char dhcpcd[PATH_MAX];
char **dhcpcd_argv = NULL;
int dhcpcd_argc = 0;
char *dhcpcd_skiproutes = NULL;
#define EXTRA_OPTS "XZ:"
#endif

#ifdef CMDLINE_COMPAT
# define EXTRA_OPTS "HMNRSY"
#endif

#ifndef EXTRA_OPTS
# define EXTRA_OPTS
#endif

static int
atoint(const char *s)
{
	char *t;
	long n;

	errno = 0;
	n = strtol(s, &t, 0);
	if ((errno != 0 && n == 0) || s == t ||
	    (errno == ERANGE && (n == LONG_MAX || n == LONG_MIN)))
	{
		logger(LOG_ERR, "`%s' out of range", s);
		return -1;
	}

	return (int)n;
}

static pid_t
read_pid(const char *pidfile)
{
	FILE *fp;
	pid_t pid = 0;

	if ((fp = fopen(pidfile, "r")) == NULL) {
		errno = ENOENT;
		return 0;
	}

	fscanf(fp, "%d", &pid);
	fclose(fp);

	return pid;
}

static void
usage(void)
{
	printf("usage: "PACKAGE" [-dknpxADEGHLOSTV] [-c script] [-f file ] [-h hostname]\n"
	       "              [-i classID ] [-l leasetime] [-m metric] [-o option] [-r ipaddr]\n"
	       "              [-s ipaddr] [-t timeout] [-u userclass] [-F none|ptr|both]\n"
	       "              [-I clientID] [-C hookscript] <interface>\n");
}

static char * 
add_environ(struct options *options, const char *value, int uniq)
{
	char **newlist;
	char **lst = options->environ;
	size_t i = 0, l, lv;
	char *match = NULL, *p;

	match = xstrdup(value);
	p = strchr(match, '=');
	if (p)
		*p++ = '\0';
	l = strlen(match);

	while (lst && lst[i]) {
		if (match && strncmp(lst[i], match, l) == 0) {
			if (uniq) {
				free(lst[i]);
				lst[i] = xstrdup(value);
			} else {
				/* Append a space and the value to it */
				l = strlen(lst[i]);
				lv = strlen(p);
				lst[i] = xrealloc(lst[i], l + lv + 2);
				lst[i][l] = ' ';
				memcpy(lst[i] + l + 1, p, lv);
				lst[i][l + lv + 1] = '\0';
			}
			free(match);
			return lst[i];
		}
		i++;
	}

	newlist = xrealloc(lst, sizeof(char *) * (i + 2));
	newlist[i] = xstrdup(value);
	newlist[i + 1] = NULL;
	options->environ = newlist;
	free(match);
	return newlist[i];
}

static int
parse_option(int opt, char *oarg, struct options *options)
{
	static int userclasses = 0;
	int i;
	int j;
	char *p;
	size_t s;

	switch(opt) {
	case 'h':
		if (!oarg)
			*options->hostname = '\0';
		else if (strlen(oarg) >= MAXHOSTNAMELEN) {
			logger(LOG_ERR,
			       "`%s' too long for HostName string, max is %d",
			       oarg, MAXHOSTNAMELEN);
			return -1;
		} else
			strlcpy(options->hostname, oarg,
				sizeof(options->hostname));
		break;
	case 'i':
		if (!oarg) {
			*options->classid = '\0';
		} else if (strlen(oarg) >= CLASS_ID_MAX_LEN) {
			logger(LOG_ERR,
			       "`%s' too long for ClassID string, max is %d",
			       oarg, CLASS_ID_MAX_LEN);
			return -1;
		} else
			strlcpy(options->classid, oarg,
				sizeof(options->classid));
		break;
	case 'l':
		if (*oarg == '-') {
			logger(LOG_ERR,
			       "leasetime must be a positive value");
			return -1;
		}
		errno = 0;
		options->leasetime = (uint32_t)strtol(oarg, NULL, 0);
		if (errno == EINVAL || errno == ERANGE) {
			logger(LOG_ERR, "`%s' out of range", oarg);
			return -1;
		}
		break;
	case 'm':
		options->metric = atoint(oarg);
		if (options->metric < 0) {
			logger(LOG_ERR, "metric must be a positive value");
			return -1;
		}
		break;
	case 'o':
		if (make_reqmask(options->reqmask, &oarg, 1) != 0) {
			logger(LOG_ERR, "unknown option `%s'", oarg);
			return -1;
		}
		break;
	case 'p':
		options->options |= DHCPCD_PERSISTENT;
		break;
	case 's':
		options->options |= DHCPCD_INFORM;
		options->options &= ~DHCPCD_ARP;
		if (!oarg || strlen(optarg) == 0) {
			options->request_address.s_addr = 0;
			break;
		} else {
			if ((p = strchr(oarg, '/'))) {
				/* nullify the slash, so the -r option
				 * can read the address */
				*p++ = '\0';
				if (sscanf(p, "%d", &i) != 1 ||
				    inet_cidrtoaddr(i, &options->request_netmask) != 0)
				{
					logger(LOG_ERR,
					       "`%s' is not a valid CIDR",
					       p);
					return -1;
				}
			}
		}
		/* FALLTHROUGH */
	case 'r':
		if (!(options->options & DHCPCD_INFORM))
			options->options |= DHCPCD_REQUEST;
		if (strlen(oarg) > 0 &&
		    !inet_aton(oarg, &options->request_address))
		{ 
			logger(LOG_ERR, "`%s' is not a valid IP address",
			       oarg);
			return -1;
		}
		break;
	case 't':
		options->timeout = atoint(oarg);
		if (options->timeout < 0) {
			logger (LOG_ERR, "timeout must be a positive value");
			return -1;
		}
		break;
	case 'u':
		j = 0;
		for (i = 0; i < userclasses; i++)
			j += (int)options->userclass[j] + 1;
		if (j + 1 + strlen(oarg) >= USERCLASS_MAX_LEN) {
			logger(LOG_ERR,
			       "userclass overrun, max is %d",
			       USERCLASS_MAX_LEN);
			return -1;
		}
		userclasses++;
		memcpy(options->userclass + j + 1 ,
		       oarg, strlen(optarg));
		options->userclass[j] = strlen(oarg);
		options->userclass_len += (strlen(oarg)) + 1;
		break;
	case 'A':
		options->options &= ~DHCPCD_ARP;
		/* IPv4LL requires ARP */
		options->options &= ~DHCPCD_IPV4LL;
		break;
	case 'C':
		/* Commas to spaces for shell */
		while ((p = strchr(oarg, ',')))
			*p = ' ';
		s = strlen("skip_hooks=") + strlen(oarg) + 1;
		p = xmalloc(sizeof(char) * s);
		snprintf(p, s, "skip_hooks=%s", oarg);
		add_environ(options, p, 0);
		free(p);
		break;
	case 'D':
		options->options |= DHCPCD_DUID;
		break;
	case 'E':
		options->options |= DHCPCD_LASTLEASE;
		break;
	case 'F':
		if (!oarg) {
			options->fqdn = FQDN_BOTH;
			break;
		}
		if (strncmp(oarg, "none", strlen(optarg)) == 0)
			options->fqdn = FQDN_NONE;
		else if (strncmp(oarg, "ptr", strlen(optarg)) == 0)
			options->fqdn = FQDN_PTR;
		else if (strncmp(oarg, "both", strlen(optarg)) == 0)
			options->fqdn = FQDN_BOTH;
		else {
			logger(LOG_ERR, "invalid value `%s' for FQDN",
			       oarg);
			return -1;
		}
		break;
	case 'G':
		options->options &= ~DHCPCD_GATEWAY;
		break;
	case 'I':
		if (oarg) {
			if (strlen(oarg) >= CLIENT_ID_MAX_LEN) {
				logger(LOG_ERR, "`%s' is too long for"
				       " ClientID, max is %d",
				       oarg, CLIENT_ID_MAX_LEN);
				return -1;
			}
			if (strlcpy(options->clientid, oarg,
				    sizeof(options->clientid)) == 0) {
				/* empty string disabled duid */
				options->options &= ~DHCPCD_DUID;
				options->options &= ~DHCPCD_CLIENTID;
			}
		} else {
			options->clientid[0] = '\0';
			options->options &= ~DHCPCD_DUID;
			options->options &= ~DHCPCD_CLIENTID;
		}
		break;
	case 'L':
		options->options &= ~DHCPCD_IPV4LL;
		break;
	default:
		return 0;
	}

	return 1;
}

static int
parse_config_line(const char *opt, char *line, struct options *options)
{
	unsigned int i;

	for (i = 0; i < sizeof(longopts) / sizeof(longopts[0]); i++) {
		if (!longopts[i].name ||
		    strcmp(longopts[i].name, opt) != 0)
			continue;

		if (longopts[i].has_arg == required_argument && !line) {
			fprintf(stderr,
				PACKAGE ": option requires an argument -- %s\n",
				opt);
			return -1;
		}

		return parse_option(longopts[i].val, line, options);
	}

	fprintf(stderr, PACKAGE ": unknown option -- %s\n", opt);
	return -1;
}

int
main(int argc, char **argv)
{
	struct options *options;
	int opt;
	int option_index = 0;
	char *prefix;
	pid_t pid;
	int debug = 0;
	int i, r;
	int pidfd = -1;
	int sig = 0;
	int retval = EXIT_FAILURE;
	char *line, *option, *p, *lp, *buffer = NULL;
	char lt;
	size_t len = 0;
	FILE *f;
	char *cf = NULL;
	char *intf = NULL;

	closefrom(3);
	openlog(PACKAGE, LOG_PID, LOG_LOCAL0);

#ifdef THERE_IS_NO_FORK
	dhcpcd_argv = argv;
	dhcpcd_argc = argc;
	if (!realpath(argv[0], dhcpcd)) {
		fprintf(stderr, "unable to resolve the path `%s': %s",
			argv[0], strerror(errno));
		goto abort;
	}
#endif

	options = xzalloc(sizeof(*options));
	options->script = SCRIPT;
	snprintf(options->classid, CLASS_ID_MAX_LEN, "%s %s",
		 PACKAGE, VERSION);

	options->options |= DHCPCD_GATEWAY | DHCPCD_DAEMONISE | DHCPCD_CLIENTID;
#ifdef ENABLE_ARP
	options->options |= DHCPCD_ARP;
 #ifdef ENABLE_IPV4LL
	options->options |= DHCPCD_IPV4LL;
 #endif
#endif

	options->timeout = DEFAULT_TIMEOUT;

#ifdef CMDLINE_COMPAT
	add_reqmask(options->reqmask, DHCP_DNSSERVER);
	add_reqmask(options->reqmask, DHCP_DNSDOMAIN);
	add_reqmask(options->reqmask, DHCP_DNSSEARCH);
	add_reqmask(options->reqmask, DHCP_NISSERVER);
	add_reqmask(options->reqmask, DHCP_NISDOMAIN);
	add_reqmask(options->reqmask, DHCP_NTPSERVER);

	/* If the duid file exists, then enable duid by default
	 * This means we don't break existing clients that easily :) */
# ifdef ENABLE_DUID
	if ((f = fopen(DUID, "r"))) {
		options->options |= DHCPCD_DUID;
		fclose(f);
	}
# endif
#endif

	gethostname(options->hostname, sizeof(options->hostname));
	if (strcmp(options->hostname, "(none)") == 0 ||
	    strcmp(options->hostname, "localhost") == 0)
		*options->hostname = '\0';

	while ((opt = getopt_long(argc, argv, OPTS EXTRA_OPTS,
				  longopts, &option_index)) != -1)
	{
		switch (opt) {
		case 0:
			if (longopts[option_index].flag)
				break;
			logger(LOG_ERR,	"option `%s' should set a flag",
			       longopts[option_index].name);
			goto abort;
		case 'f':
			cf = optarg;
			break;
		case 'V':
			print_options();
			goto abort;
		case '?':
			usage();
			goto abort;
		}
	}

	if (doversion) {
		printf(""PACKAGE" "VERSION"\n");
		printf("Compile time options:"
#ifdef ENABLE_ARP
		       " ARP"
#endif
#ifdef ENABLE_DUID
		       " DUID"
#endif
#ifdef ENABLE_IPV4LL
		       " IPV4LL"
#endif
#ifdef THERE_IS_NO_FORK
		       " THERE_IS_NO_FORK"
#endif
		       "\n");
	}

	if (dohelp)
		usage();

	if (optind < argc) {
		if (strlen(argv[optind]) >= IF_NAMESIZE) {
			logger(LOG_ERR,
			       "`%s' too long for an interface name (max=%d)",
			       argv[optind], IF_NAMESIZE);
			goto abort;
		}
		strlcpy(options->interface, argv[optind],
			sizeof(options->interface));
	} else {
		/* If only version was requested then exit now */
		if (doversion || dohelp) {
			retval = 0;
			goto abort;
		}

		logger(LOG_ERR, "no interface specified");
		goto abort;
	}

	/* Parse our options file */
	f = fopen(cf ? cf : CONFIG, "r");
	if (f) {
		r = 1;
		while ((get_line(&buffer, &len, f))) {
			line = buffer;
			while ((option = strsep(&line, " \t")))
				if (*option != '\0')
					break;
			if (!option || *option == '\0' || *option == '#')
				continue;
			/* Trim leading whitespace */
			if (line) {
				while (*line != '\0' && (*line == ' ' || *line == '\t'))
					line++;
			}
			/* Trim trailing whitespace */
			lt = '\\';
			if (line && *line) {
				p = line + strlen(line) - 1;
				while (p != line && (*p == ' ' || *p == '\t')) {
					/* Remember the last char trimmed */
					lt = *p;
					*p-- = '\0';
				}
			}
			/* Remove quotes if present */
			if (line && *line == '"') {
				p = line + strlen(line) - 1;
				if (*p == '"') {
					line++;
					*p = '\0';
				}
			}
			/* Process escapes */
			lp = p = line;
			while (p && *p) {
				if (*p == '\\')
					p++;
				/* EOL? */
				if (*p == '\0') {
					/* Restore the last char trimmed */
					*lp++ = lt;
					break;
				}
				*lp++ = *p++;
			}
			if (lp)
				*lp = '\0';
			if (strcmp(option, "interface") == 0) {
				free(intf);
				intf = xstrdup(line);
				continue;
			}
			/* If we're in an interface block don't use these
			 * options unless it's for us */
			if (intf && strcmp(intf, options->interface) != 0)
				continue;
			r = parse_config_line(option, line, options);
			if (r != 1)
				break;
		}
		free(buffer);
		free(intf);
		fclose(f);
		if (r == 0)
			usage();
		if (r != 1)
			goto abort;
	} else {
		if (errno != ENOENT || cf) {
			logger(LOG_ERR, "fopen `%s': %s", cf ? cf : CONFIG,
			       strerror(errno));
			goto abort;
		}
	}

	optind = 0;
	while ((opt = getopt_long(argc, argv, OPTS EXTRA_OPTS,
				  longopts, &option_index)) != -1)
	{
		switch (opt) {
		case 'd':
			debug++;
			switch (debug) {
			case 1:
				setloglevel(LOG_DEBUG);
				break;
			case 2:
				options->options &= ~DHCPCD_DAEMONISE;
				break;
			}
			break;
		case 'f':
			break;
#ifdef THERE_IS_NO_FORK
		case 'X':
			options->options |= DHCPCD_DAEMONISED;
			close_fds();
			break;
		case 'Z':
			dhcpcd_skiproutes = xstrdup(optarg);
			break;
#endif
		case 'k':
			sig = SIGHUP;
			break;
		case 'n':
			sig = SIGALRM;
			break;
		case 'x':
			sig = SIGTERM;
			break;
		case 'O':
			if (make_reqmask(options->reqmask, &optarg, -1) != 0) {
				logger(LOG_ERR, "unknown option `%s'", optarg);
				return -1;
			}
			if (make_reqmask(options->nomask, &optarg, 1) != 0) {
				logger(LOG_ERR, "unknown option `%s'", optarg);
				return -1;
			}
			break;
		case 'T':
			options->options |= DHCPCD_TEST | DHCPCD_PERSISTENT;
			break;
#ifdef CMDLINE_COMPAT
		case 'H': /* FALLTHROUGH */
		case 'M':
			break;
		case 'N':
			del_reqmask(options->reqmask, DHCP_NTPSERVER);
			break;
		case 'R':
			del_reqmask(options->reqmask, DHCP_DNSSERVER);
			del_reqmask(options->reqmask, DHCP_DNSDOMAIN);
			del_reqmask(options->reqmask, DHCP_DNSSEARCH);
			break;
		case 'S':
			add_reqmask(options->reqmask, DHCP_MSCSR);
			break;
		case 'Y':
			del_reqmask(options->reqmask, DHCP_NISSERVER);
			del_reqmask(options->reqmask, DHCP_NISDOMAIN);
			break;
#endif
		default:
			i = parse_option(opt, optarg, options);
			if (i == 1)
				break;
			if (i == 0)
				usage();
			goto abort;
		}
	}

	if ((p = strchr(options->hostname, '.'))) {
		if (options->fqdn == FQDN_DISABLE)
			*p = '\0';
	} else {
		if (options->fqdn != FQDN_DISABLE) {
			logger(LOG_WARNING, "hostname `%s' is not a FQDN",
			       options->hostname);
			options->fqdn = FQDN_DISABLE;
		}
	}
	if (options->fqdn != FQDN_DISABLE)
		del_reqmask(options->reqmask, DHCP_HOSTNAME);

	if (options->request_address.s_addr == 0 &&
	    options->options & DHCPCD_INFORM)
	{
		if (get_address(options->interface,
				&options->request_address,
				&options->request_netmask) == 0)
			options->options |= DHCPCD_KEEPADDRESS;
	}

	if (IN_LINKLOCAL(ntohl(options->request_address.s_addr))) {
		logger(LOG_ERR,
		       "you are not allowed to request a link local address");
		goto abort;
	}

	if (geteuid())
		logger(LOG_WARNING, PACKAGE " will not work correctly unless"
		       " run as root");

	prefix = xmalloc(sizeof(char) * (IF_NAMESIZE + 3));
	snprintf(prefix, IF_NAMESIZE, "%s: ", options->interface);
	setlogprefix(prefix);
	snprintf(options->pidfile, sizeof(options->pidfile), PIDFILE,
		 options->interface);
	free(prefix);

	chdir("/");
	umask(022);

	if (options->options & DHCPCD_TEST) {
		if (options->options & DHCPCD_REQUEST ||
		    options->options & DHCPCD_INFORM) {
			logger(LOG_ERR,
			       "cannot test with --inform or --request");
			goto abort;
		}

		if (options->options & DHCPCD_LASTLEASE) {
			logger(LOG_ERR, "cannot test with --lastlease");
			goto abort;
		}

		if (sig != 0) {
			logger(LOG_ERR,
			       "cannot test with --release or --renew");
			goto abort;
		}
	}

	if (sig != 0) {
		i = -1;
		pid = read_pid(options->pidfile);
		if (pid != 0)
			logger(LOG_INFO, "sending signal %d to pid %d",
			       sig, pid);

		if (!pid || (i = kill(pid, sig)))
			logger(sig == SIGALRM ? LOG_INFO : LOG_ERR,
			       ""PACKAGE" not running");

		if (pid != 0 && (sig != SIGALRM || i != 0))
			unlink(options->pidfile);

		if (i == 0) {
			retval = EXIT_SUCCESS;
			goto abort;
		}

		if (sig != SIGALRM)
			goto abort;	
	}

	if (!(options->options & DHCPCD_TEST) &&
	    !(options->options & DHCPCD_DAEMONISED))
	{
		if ((pid = read_pid(options->pidfile)) > 0 &&
		    kill(pid, 0) == 0)
		{
			logger(LOG_ERR, ""PACKAGE
			       " already running on pid %d (%s)",
			       pid, options->pidfile);
			goto abort;
		}

		pidfd = open(options->pidfile,
			     O_WRONLY | O_CREAT | O_NONBLOCK, 0664);
		if (pidfd == -1) {
			logger(LOG_ERR, "open `%s': %s",
			       options->pidfile, strerror(errno));
			goto abort;
		}

		/* Lock the file so that only one instance of dhcpcd runs
		 * on an interface */
		if (flock(pidfd, LOCK_EX | LOCK_NB) == -1) {
			logger(LOG_ERR, "flock `%s': %s",
			       options->pidfile, strerror(errno));
			goto abort;
		}

		close_on_exec(pidfd);
		writepid(pidfd, getpid());
		logger(LOG_INFO, PACKAGE " " VERSION " starting");
	}

	if (dhcp_run(options, &pidfd) == 0)
		retval = EXIT_SUCCESS;

abort:
	/* If we didn't daemonise then we need to punt the pidfile now */
	if (pidfd > -1) {
		close(pidfd);
		unlink(options->pidfile);
	}
	if (options->environ) {
		len = 0;
		while (options->environ[len])
			free(options->environ[len++]);
		free(options->environ);
	}
	free(options);

#ifdef THERE_IS_NO_FORK
	/* There may have been an error before the dhcp_run function
	 * clears this, so just do it here to be safe */
	free(dhcpcd_skiproutes);
#endif

	exit(retval);
	/* NOTREACHED */
}
