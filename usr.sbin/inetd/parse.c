/*	$NetBSD: parse.c,v 1.3 2022/05/22 11:27:37 andvar Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Matthias Scheler.
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

/*
 * Copyright (c) 1983, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
#if 0
static char sccsid[] = "@(#)inetd.c	8.4 (Berkeley) 4/13/94";
#else
__RCSID("$NetBSD: parse.c,v 1.3 2022/05/22 11:27:37 andvar Exp $");
#endif
#endif /* not lint */

/*
 * This file contains code and state for loading and managing servtabs.
 * The "positional" syntax parsing is performed in this file. See parse_v2.c
 * for "key-values" syntax parsing.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "inetd.h"

static void	config(void);
static void	endconfig(void);
static struct servtab	*enter(struct servtab *);
static struct servtab	*getconfigent(char **);
#ifdef DEBUG_ENABLE
static void	print_service(const char *, struct servtab *);
#endif
static struct servtab	init_servtab(void);
static void	include_configs(char *);
static int	glob_error(const char *, int);
static void	read_glob_configs(char *);
static void	prepare_next_config(const char*);
static bool	is_same_service(const struct servtab *, const struct servtab *);
static char	*gen_file_pattern(const char *, const char *);
static bool	check_no_reinclude(const char *);
static void	include_matched_path(char *);
static void	purge_unchecked(void);
static void	freeconfig(struct servtab *);
static char	*skip(char **);

size_t	line_number;
FILE	*fconfig;
/* Temporary storage for new servtab */
static struct	servtab serv;
/* Current line from current config file */
static char	line[LINE_MAX];
char    *defhost;
#ifdef IPSEC
char *policy;
#endif

/*
 * Recursively merge loaded service definitions with any defined
 * in the current or included config files.
 */
static void
config(void)
{
	struct servtab *sep, *cp;
	/*
	 * Current position in line, used with key-values notation,
	 * saves cp across getconfigent calls.
	 */
	char *current_pos;
	size_t n;

	/* open config file from beginning */
	fconfig = fopen(CONFIG, "r");
	if (fconfig == NULL) {
		DPRINTF("Could not open file \"%s\": %s",
		    CONFIG, strerror(errno));
		syslog(LOG_ERR, "%s: %m", CONFIG);
		return;
	}

	/* First call to nextline will advance line_number to 1 */
	line_number = 0;

	/* Start parsing at the beginning of the first line */
	current_pos = nextline(fconfig);

	while ((cp = getconfigent(&current_pos)) != NULL) {
		/* Find an already existing service definition */
		for (sep = servtab; sep != NULL; sep = sep->se_next)
			if (is_same_service(sep, cp))
				break;
		if (sep != NULL) {
			int i;

#define SWAP(type, a, b) {type c = a; a = b; b = c;}

			/*
			 * sep->se_wait may be holding the pid of a daemon
			 * that we're waiting for.  If so, don't overwrite
			 * it unless the config file explicitly says don't
			 * wait.
			 */
			if (cp->se_bi == 0 &&
			    (sep->se_wait == 1 || cp->se_wait == 0))
				sep->se_wait = cp->se_wait;
			SWAP(char *, sep->se_user, cp->se_user);
			SWAP(char *, sep->se_group, cp->se_group);
			SWAP(char *, sep->se_server, cp->se_server);
			for (i = 0; i < MAXARGV; i++)
				SWAP(char *, sep->se_argv[i], cp->se_argv[i]);
#ifdef IPSEC
			SWAP(char *, sep->se_policy, cp->se_policy);
#endif
			SWAP(service_type, cp->se_type, sep->se_type);
			SWAP(size_t, cp->se_service_max, sep->se_service_max);
			SWAP(size_t, cp->se_ip_max, sep->se_ip_max);
#undef SWAP
			if (isrpcservice(sep))
				unregister_rpc(sep);
			sep->se_rpcversl = cp->se_rpcversl;
			sep->se_rpcversh = cp->se_rpcversh;
			freeconfig(cp);
#ifdef DEBUG_ENABLE
			if (debug)
				print_service("REDO", sep);
#endif
		} else {
			sep = enter(cp);
#ifdef DEBUG_ENABLE
			if (debug)
				print_service("ADD ", sep);
#endif
		}
		sep->se_checked = 1;

		/*
		 * Remainder of config(void) checks validity of servtab options
		 * and sets up the service by setting up sockets
		 * (in setup(servtab)).
		 */
		switch (sep->se_family) {
		case AF_LOCAL:
			if (sep->se_fd != -1)
				break;
			n = strlen(sep->se_service);
			if (n >= sizeof(sep->se_ctrladdr_un.sun_path)) {
				syslog(LOG_ERR, "%s/%s: address too long",
				    sep->se_service, sep->se_proto);
				sep->se_checked = 0;
				continue;
			}
			(void)unlink(sep->se_service);
			strlcpy(sep->se_ctrladdr_un.sun_path,
			    sep->se_service, n + 1);
			sep->se_ctrladdr_un.sun_family = AF_LOCAL;
			sep->se_ctrladdr_size = (socklen_t)(n +
			    sizeof(sep->se_ctrladdr_un) -
			    sizeof(sep->se_ctrladdr_un.sun_path));
			if (!ISMUX(sep))
				setup(sep);
			break;
		case AF_INET:
#ifdef INET6
		case AF_INET6:
#endif
		    {
			struct addrinfo hints, *res;
			char *host;
			const char *port;
			int error;
			int s;

			/* check if the family is supported */
			s = socket(sep->se_family, SOCK_DGRAM, 0);
			if (s < 0) {
				syslog(LOG_WARNING,
				    "%s/%s: %s: the address family is not "
				    "supported by the kernel",
				    sep->se_service, sep->se_proto,
				    sep->se_hostaddr);
				sep->se_checked = false;
				continue;
			}
			close(s);

			memset(&hints, 0, sizeof(hints));
			hints.ai_family = sep->se_family;
			hints.ai_socktype = sep->se_socktype;
			hints.ai_flags = AI_PASSIVE;
			if (strcmp(sep->se_hostaddr, "*") == 0)
				host = NULL;
			else
				host = sep->se_hostaddr;
			if (isrpcservice(sep) || ISMUX(sep))
				port = "0";
			else
				port = sep->se_service;
			error = getaddrinfo(host, port, &hints, &res);
			if (error != 0) {
				if (error == EAI_SERVICE) {
					/* gai_strerror not friendly enough */
					syslog(LOG_WARNING, SERV_FMT ": "
					    "unknown service",
					    SERV_PARAMS(sep));
				} else {
					syslog(LOG_ERR, SERV_FMT ": %s: %s",
					    SERV_PARAMS(sep),
					    sep->se_hostaddr,
					    gai_strerror(error));
				}
				sep->se_checked = false;
				continue;
			}
			if (res->ai_next != NULL) {
				syslog(LOG_ERR, SERV_FMT
				    ": %s: resolved to multiple addr",
				    SERV_PARAMS(sep),
				    sep->se_hostaddr);
				sep->se_checked = false;
				freeaddrinfo(res);
				continue;
			}
			memcpy(&sep->se_ctrladdr, res->ai_addr,
				res->ai_addrlen);
			if (ISMUX(sep)) {
				sep->se_fd = -1;
				freeaddrinfo(res);
				continue;
			}
			sep->se_ctrladdr_size = res->ai_addrlen;
			freeaddrinfo(res);
#ifdef RPC
			if (isrpcservice(sep)) {
				struct rpcent *rp;

				sep->se_rpcprog = atoi(sep->se_service);
				if (sep->se_rpcprog == 0) {
					rp = getrpcbyname(sep->se_service);
					if (rp == 0) {
						syslog(LOG_ERR,
						    SERV_FMT
						    ": unknown service",
						    SERV_PARAMS(sep));
						sep->se_checked = false;
						continue;
					}
					sep->se_rpcprog = rp->r_number;
				}
				if (sep->se_fd == -1 && !ISMUX(sep))
					setup(sep);
				if (sep->se_fd != -1)
					register_rpc(sep);
			} else
#endif
			{
				if (sep->se_fd >= 0)
					close_sep(sep);
				if (sep->se_fd == -1 && !ISMUX(sep))
					setup(sep);
			}
		    }
		}
	}
	endconfig();
}

static struct servtab *
enter(struct servtab *cp)
{
	struct servtab *sep;

	sep = malloc(sizeof (*sep));
	if (sep == NULL) {
		syslog(LOG_ERR, "Out of memory.");
		exit(EXIT_FAILURE);
	}
	*sep = *cp;
	sep->se_fd = -1;
	sep->se_rpcprog = -1;
	sep->se_next = servtab;
	servtab = sep;
	return (sep);
}

static void
endconfig(void)
{
	if (fconfig != NULL) {
		(void) fclose(fconfig);
		fconfig = NULL;
	}
	if (defhost != NULL) {
		free(defhost);
		defhost = NULL;
	}

#ifdef IPSEC
	if (policy != NULL) {
		free(policy);
		policy = NULL;
	}
#endif

}

#define LOG_EARLY_ENDCONF() \
	ERR("Exiting %s early. Some services will be unavailable", CONFIG)

#define LOG_TOO_FEW_ARGS() \
	ERR("Expected more arguments")

/* Parse the next service and apply any directives, and returns it as servtab */
static struct servtab *
getconfigent(char **current_pos)
{
	struct servtab *sep = &serv;
	int argc, val;
	char *cp, *cp0, *arg, *buf0, *buf1, *sz0, *sz1;
	static char TCPMUX_TOKEN[] = "tcpmux/";
#define MUX_LEN		(sizeof(TCPMUX_TOKEN)-1)
	char *hostdelim;

	/*
	 * Pre-condition: current_pos points into line,
	 * line contains config line. Continue where the last getconfigent
	 * left off. Allows for multiple service definitions per line.
	 */
	cp = *current_pos;

	if (/*CONSTCOND*/false) {
		/*
		 * Go to the next line, but only after attempting to read the
		 * current one! Keep reading until we find a valid definition
		 * or EOF.
		 */
more:
		cp = nextline(fconfig);
	}

	if (cp == NULL) {
		/* EOF or I/O error, let config() know to exit the file */
		return NULL;
	}

	/* Comments and IPsec policies */
	if (cp[0] == '#') {
#ifdef IPSEC
		/* lines starting with #@ is not a comment, but the policy */
		if (cp[1] == '@') {
			char *p;
			for (p = cp + 2; isspace((unsigned char)*p); p++)
				;
			if (*p == '\0') {
				if (policy)
					free(policy);
				policy = NULL;
			} else {
				if (ipsecsetup_test(p) < 0) {
					ERR("Invalid IPsec policy \"%s\"", p);
					LOG_EARLY_ENDCONF();
					/*
					 * Stop reading the current config to
					 * prevent services from being run
					 * without IPsec.
					 */
					return NULL;
				} else {
					if (policy)
						free(policy);
					policy = newstr(p);
				}
			}
		}
#endif

		goto more;
	}

	/* Parse next token: listen-addr/hostname, service-spec, .include */
	arg = skip(&cp);

	if (cp == NULL) {
		goto more;
	}

	if (arg[0] == '.') {
		if (strcmp(&arg[1], "include") == 0) {
			/* include directive */
			arg = skip(&cp);
			if (arg == NULL) {
				LOG_TOO_FEW_ARGS();
				return NULL;
			}
			include_configs(arg);
			goto more;
		} else {
			ERR("Unknown directive '%s'", &arg[1]);
			goto more;
		}
	}

	/* After this point, we might need to store data in a servtab */
	*sep = init_servtab();

	/* Check for a host name. */
	hostdelim = strrchr(arg, ':');
	if (hostdelim != NULL) {
		*hostdelim = '\0';
		if (arg[0] == '[' && hostdelim > arg && hostdelim[-1] == ']') {
			hostdelim[-1] = '\0';
			sep->se_hostaddr = newstr(arg + 1);
		} else
			sep->se_hostaddr = newstr(arg);
		arg = hostdelim + 1;
		/*
		 * If the line is of the form `host:', then just change the
		 * default host for the following lines.
		 */
		if (*arg == '\0') {
			arg = skip(&cp);
			if (cp == NULL) {
				free(defhost);
				defhost = sep->se_hostaddr;
				goto more;
			}
		}
	} else {
		/* No host address found, set it to NULL to indicate absence */
		sep->se_hostaddr = NULL;
	}
	if (strncmp(arg, TCPMUX_TOKEN, MUX_LEN) == 0) {
		char *c = arg + MUX_LEN;
		if (*c == '+') {
			sep->se_type = MUXPLUS_TYPE;
			c++;
		} else
			sep->se_type = MUX_TYPE;
		sep->se_service = newstr(c);
	} else {
		sep->se_service = newstr(arg);
		sep->se_type = NORM_TYPE;
	}

	DPRINTCONF("Found service definition '%s'", sep->se_service);

	/* on/off/socktype */
	arg = skip(&cp);
	if (arg == NULL) {
		LOG_TOO_FEW_ARGS();
		freeconfig(sep);
		goto more;
	}

	/* Check for new v2 syntax */
	if (strcmp(arg, "on") == 0 || strncmp(arg, "on#", 3) == 0) {

		if (arg[2] == '#') {
			cp = nextline(fconfig);
		}

		switch(parse_syntax_v2(sep, &cp)) {
		case V2_SUCCESS:
			*current_pos = cp;
			return sep;
		case V2_SKIP:
			/*
			 * Skip invalid definitions, freeconfig is called in
			 * parse_v2.c
			 */
			*current_pos = cp;
			freeconfig(sep);
			goto more;
		case V2_ERROR:
			/*
			 * Unrecoverable error, stop reading. freeconfig
			 * is called in parse_v2.c
			 */
			LOG_EARLY_ENDCONF();
			freeconfig(sep);
			return NULL;
		}
	} else if (strcmp(arg, "off") == 0 || strncmp(arg, "off#", 4) == 0) {

		if (arg[3] == '#') {
			cp = nextline(fconfig);
		}

		/* Parse syntax the same as with 'on', but ignore the result */
		switch(parse_syntax_v2(sep, &cp)) {
		case V2_SUCCESS:
		case V2_SKIP:
			*current_pos = cp;
			freeconfig(sep);
			goto more;
		case V2_ERROR:
			/* Unrecoverable error, stop reading */
			LOG_EARLY_ENDCONF();
			freeconfig(sep);
			return NULL;
		}
	} else {
		/* continue parsing v1 */
		parse_socktype(arg, sep);
		if (sep->se_socktype == SOCK_STREAM) {
			parse_accept_filter(arg, sep);
		}
		if (sep->se_hostaddr == NULL) {
			/* Set host to current default */
			sep->se_hostaddr = newstr(defhost);
		}
	}

	/* protocol */
	arg = skip(&cp);
	if (arg == NULL) {
		LOG_TOO_FEW_ARGS();
		freeconfig(sep);
		goto more;
	}
	if (sep->se_type == NORM_TYPE &&
	    strncmp(arg, "faith/", strlen("faith/")) == 0) {
		arg += strlen("faith/");
		sep->se_type = FAITH_TYPE;
	}
	sep->se_proto = newstr(arg);

#define	MALFORMED(arg) \
do { \
	ERR("%s: malformed buffer size option `%s'", \
	    sep->se_service, (arg)); \
	freeconfig(sep); \
	goto more; \
} while (false)

#define	GETVAL(arg) \
do { \
	if (!isdigit((unsigned char)*(arg))) \
		MALFORMED(arg); \
	val = (int)strtol((arg), &cp0, 10); \
	if (cp0 != NULL) { \
		if (cp0[1] != '\0') \
			MALFORMED((arg)); \
		if (cp0[0] == 'k') \
			val *= 1024; \
		if (cp0[0] == 'm') \
			val *= 1024 * 1024; \
	} \
	if (val < 1) { \
		ERR("%s: invalid buffer size `%s'", \
		    sep->se_service, (arg)); \
		freeconfig(sep); \
		goto more; \
	} \
} while (false)

#define	ASSIGN(arg) \
do { \
	if (strcmp((arg), "sndbuf") == 0) \
		sep->se_sndbuf = val; \
	else if (strcmp((arg), "rcvbuf") == 0) \
		sep->se_rcvbuf = val; \
	else \
		MALFORMED((arg)); \
} while (false)

	/*
	 * Extract the send and receive buffer sizes before parsing
	 * the protocol.
	 */
	sep->se_sndbuf = sep->se_rcvbuf = 0;
	buf0 = buf1 = sz0 = sz1 = NULL;
	if ((buf0 = strchr(sep->se_proto, ',')) != NULL) {
		/* Not meaningful for Tcpmux services. */
		if (ISMUX(sep)) {
			ERR("%s: can't specify buffer sizes for "
			    "tcpmux services", sep->se_service);
			goto more;
		}

		/* Skip the , */
		*buf0++ = '\0';

		/* Check to see if another socket buffer size was specified. */
		if ((buf1 = strchr(buf0, ',')) != NULL) {
			/* Skip the , */
			*buf1++ = '\0';

			/* Make sure a 3rd one wasn't specified. */
			if (strchr(buf1, ',') != NULL) {
				ERR("%s: too many buffer sizes",
				    sep->se_service);
				goto more;
			}

			/* Locate the size. */
			if ((sz1 = strchr(buf1, '=')) == NULL)
				MALFORMED(buf1);

			/* Skip the = */
			*sz1++ = '\0';
		}

		/* Locate the size. */
		if ((sz0 = strchr(buf0, '=')) == NULL)
			MALFORMED(buf0);

		/* Skip the = */
		*sz0++ = '\0';

		GETVAL(sz0);
		ASSIGN(buf0);

		if (buf1 != NULL) {
			GETVAL(sz1);
			ASSIGN(buf1);
		}
	}

#undef ASSIGN
#undef GETVAL
#undef MALFORMED

	if (parse_protocol(sep)) {
		freeconfig(sep);
		goto more;
	}

	/* wait/nowait:max */
	arg = skip(&cp);
	if (arg == NULL) {
		LOG_TOO_FEW_ARGS();
		freeconfig(sep);
		goto more;
	}

	/* Rate limiting parsing */ {
		char *cp1;
		if ((cp1 = strchr(arg, ':')) == NULL)
			cp1 = strchr(arg, '.');
		if (cp1 != NULL) {
			int rstatus;
			*cp1++ = '\0';
			sep->se_service_max = (size_t)strtou(cp1, NULL, 10, 0,
			    SERVTAB_COUNT_MAX, &rstatus);

			if (rstatus != 0) {
				if (rstatus != ERANGE) {
					/* For compatibility w/ atoi parsing */
					sep->se_service_max = 0;
				}

				WRN("Improper \"max\" value '%s', "
				    "using '%zu' instead: %s",
				    cp1,
				    sep->se_service_max,
				    strerror(rstatus));
			}

		} else
			sep->se_service_max = TOOMANY;
	}
	if (parse_wait(sep, strcmp(arg, "wait") == 0)) {
		freeconfig(sep);
		goto more;
	}

	/* Parse user:group token */
	arg = skip(&cp);
	if (arg == NULL) {
		LOG_TOO_FEW_ARGS();
		freeconfig(sep);
		goto more;
	}
	char* separator = strchr(arg, ':');
	if (separator == NULL) {
		/* Backwards compatibility, allow dot instead of colon */
		separator = strchr(arg, '.');
	}

	if (separator == NULL) {
		/* Only user was specified */
		sep->se_group = NULL;
	} else {
		*separator = '\0';
		sep->se_group = newstr(separator + 1);
	}

	sep->se_user = newstr(arg);

	/* Parser server-program (path to binary or "internal") */
	arg = skip(&cp);
	if (arg == NULL) {
		LOG_TOO_FEW_ARGS();
		freeconfig(sep);
		goto more;
	}
	if (parse_server(sep, arg)) {
		freeconfig(sep);
		goto more;
	}

	argc = 0;
	for (arg = skip(&cp); cp != NULL; arg = skip(&cp)) {
		if (argc < MAXARGV)
			sep->se_argv[argc++] = newstr(arg);
	}
	while (argc <= MAXARGV)
		sep->se_argv[argc++] = NULL;
#ifdef IPSEC
	sep->se_policy = policy != NULL ? newstr(policy) : NULL;
#endif
	/* getconfigent read a positional service def, move to next line */
	*current_pos = nextline(fconfig);
	return (sep);
}

void
freeconfig(struct servtab *cp)
{
	int i;

	free(cp->se_hostaddr);
	free(cp->se_service);
	free(cp->se_proto);
	free(cp->se_user);
	free(cp->se_group);
	free(cp->se_server);
	for (i = 0; i < MAXARGV; i++)
		free(cp->se_argv[i]);
#ifdef IPSEC
	free(cp->se_policy);
#endif
}

/*
 * Get next token *in the current service definition* from config file.
 * Allows multi-line parse if single space or single tab-indented.
 * Things in quotes are considered single token.
 * Advances cp to next token.
 */
static char *
skip(char **cpp)
{
	char *cp = *cpp;
	char *start;
	char quote;

	if (*cpp == NULL)
		return (NULL);

again:
	while (*cp == ' ' || *cp == '\t')
		cp++;
	if (*cp == '\0') {
		int c;

		c = getc(fconfig);
		(void) ungetc(c, fconfig);
		if (c == ' ' || c == '\t')
			if ((cp = nextline(fconfig)) != NULL)
				goto again;
		*cpp = NULL;
		return (NULL);
	}
	start = cp;
	/* Parse shell-style quotes */
	quote = '\0';
	while (*cp != '\0' && (quote != '\0' || (*cp != ' ' && *cp != '\t'))) {
		if (*cp == '\'' || *cp == '"') {
			if (quote != '\0' && *cp != quote)
				cp++;
			else {
				if (quote != '\0')
					quote = '\0';
				else
					quote = *cp;
				memmove(cp, cp+1, strlen(cp));
			}
		} else
			cp++;
	}
	if (*cp != '\0')
		*cp++ = '\0';
	*cpp = cp;
	return (start);
}

char *
nextline(FILE *fd)
{
	char *cp;

	if (fgets(line, (int)sizeof(line), fd) == NULL) {
		if (ferror(fd) != 0) {
			ERR("Error when reading next line: %s",
			    strerror(errno));
		}
		return NULL;
	}
	cp = strchr(line, '\n');
	if (cp != NULL)
		*cp = '\0';
	line_number++;
	return line;
}

char *
newstr(const char *cp)
{
	char *dp;
	if ((dp = strdup((cp != NULL) ? cp : "")) != NULL)
		return (dp);
	syslog(LOG_ERR, "strdup: %m");
	exit(EXIT_FAILURE);
	/*NOTREACHED*/
}

#ifdef DEBUG_ENABLE
/*
 * print_service:
 *	Dump relevant information to stderr
 */
static void
print_service(const char *action, struct servtab *sep)
{

	if (isrpcservice(sep))
		fprintf(stderr,
		    "%s: %s rpcprog=%d, rpcvers = %d/%d, proto=%s, "
		    "wait.max=%d.%zu, "
		    "user:group=%s:%s builtin=%lx server=%s"
#ifdef IPSEC
		    " policy=\"%s\""
#endif
		    "\n",
		    action, sep->se_service,
		    sep->se_rpcprog, sep->se_rpcversh, sep->se_rpcversl,
		    sep->se_proto, sep->se_wait, sep->se_service_max,
		    sep->se_user, sep->se_group,
		    (long)sep->se_bi, sep->se_server
#ifdef IPSEC
		    , (sep->se_policy != NULL ? sep->se_policy : "")
#endif
		    );
	else
		fprintf(stderr,
		    "%s: %s:%s proto=%s%s, wait.max=%d.%zu, user:group=%s:%s "
		    "builtin=%lx "
		    "server=%s"
#ifdef IPSEC
		    " policy=%s"
#endif
		    "\n",
		    action, sep->se_hostaddr, sep->se_service,
		    sep->se_type == FAITH_TYPE ? "faith/" : "",
		    sep->se_proto,
		    sep->se_wait, sep->se_service_max, sep->se_user,
		    sep->se_group, (long)sep->se_bi, sep->se_server
#ifdef IPSEC
		    , (sep->se_policy != NULL ? sep->se_policy : "")
#endif
		    );
}
#endif

void
config_root(void)
{
	struct servtab *sep;
	/* Uncheck services */
	for (sep = servtab; sep != NULL; sep = sep->se_next) {
		sep->se_checked = false;
	}
	defhost = newstr("*");
#ifdef IPSEC
	policy = NULL;
#endif
	fconfig = NULL;
	config();
	purge_unchecked();
}

static void
purge_unchecked(void)
{
	struct servtab *sep, **sepp = &servtab;
	int servtab_count = 0;
	while ((sep = *sepp) != NULL) {
		if (sep->se_checked) {
			sepp = &sep->se_next;
			servtab_count++;
			continue;
		}
		*sepp = sep->se_next;
		if (sep->se_fd >= 0)
			close_sep(sep);
		if (isrpcservice(sep))
			unregister_rpc(sep);
		if (sep->se_family == AF_LOCAL)
			(void)unlink(sep->se_service);
#ifdef DEBUG_ENABLE
		if (debug)
			print_service("FREE", sep);
#endif
		freeconfig(sep);
		free(sep);
	}
	DPRINTF("%d service(s) loaded.", servtab_count);
}

static bool
is_same_service(const struct servtab *sep, const struct servtab *cp)
{
	return
	    strcmp(sep->se_service, cp->se_service) == 0 &&
	    strcmp(sep->se_hostaddr, cp->se_hostaddr) == 0 &&
	    strcmp(sep->se_proto, cp->se_proto) == 0 &&
	    sep->se_family == cp->se_family &&
	    ISMUX(sep) == ISMUX(cp);
}

int
parse_protocol(struct servtab *sep)
{
	int val;

	if (strcmp(sep->se_proto, "unix") == 0) {
		sep->se_family = AF_LOCAL;
	} else {
		val = (int)strlen(sep->se_proto);
		if (val == 0) {
			ERR("%s: invalid protocol specified",
			    sep->se_service);
			return -1;
		}
		val = sep->se_proto[val - 1];
		switch (val) {
		case '4':	/*tcp4 or udp4*/
			sep->se_family = AF_INET;
			break;
#ifdef INET6
		case '6':	/*tcp6 or udp6*/
			sep->se_family = AF_INET6;
			break;
#endif
		default:
			/*
			 * Use 'default' IP version which is IPv4, may
			 * eventually be changed to AF_INET6
			 */
			sep->se_family = AF_INET;
			break;
		}
		if (strncmp(sep->se_proto, "rpc/", 4) == 0) {
#ifdef RPC
			char *cp1, *ccp;
			cp1 = strchr(sep->se_service, '/');
			if (cp1 == 0) {
				ERR("%s: no rpc version",
				    sep->se_service);
				return -1;
			}
			*cp1++ = '\0';
			sep->se_rpcversl = sep->se_rpcversh =
			    (int)strtol(cp1, &ccp, 0);
			if (ccp == cp1) {
		badafterall:
				ERR("%s/%s: bad rpc version",
				    sep->se_service, cp1);
				return -1;
			}
			if (*ccp == '-') {
				cp1 = ccp + 1;
				sep->se_rpcversh = (int)strtol(cp1, &ccp, 0);
				if (ccp == cp1)
					goto badafterall;
			}
#else
			ERR("%s: rpc services not supported",
			    sep->se_service);
			return -1;
#endif /* RPC */
		}
	}
	return 0;
}

int
parse_wait(struct servtab *sep, int wait)
{
	if (!ISMUX(sep)) {
		sep->se_wait = wait;
		return 0;
	}
	/*
	 * Silently enforce "nowait" for TCPMUX services since
	 * they don't have an assigned port to listen on.
	 */
	sep->se_wait = 0;

	if (strncmp(sep->se_proto, "tcp", 3)) {
		ERR("bad protocol for tcpmux service %s",
			sep->se_service);
		return -1;
	}
	if (sep->se_socktype != SOCK_STREAM) {
		ERR("bad socket type for tcpmux service %s",
		    sep->se_service);
		return -1;
	}
	return 0;
}

int
parse_server(struct servtab *sep, const char *arg)
{
	sep->se_server = newstr(arg);
	if (strcmp(sep->se_server, "internal") != 0) {
		sep->se_bi = NULL;
		return 0;
	}
	
	if (!try_biltin(sep)) {
		ERR("Internal service %s unknown", sep->se_service);
		return -1;
	}
	return 0;
}

/* TODO test to make sure accept filter still works */
void
parse_accept_filter(char *arg, struct servtab *sep)
{
	char *accf, *accf_arg;
	/* one and only one accept filter */
	accf = strchr(arg, ':');
	if (accf == NULL)
		return;
	if (accf != strrchr(arg, ':') || *(accf + 1) == '\0') {
		/* more than one ||  nothing beyond */
		sep->se_socktype = -1;
		return;
	}

	accf++;			/* skip delimiter */
	strlcpy(sep->se_accf.af_name, accf, sizeof(sep->se_accf.af_name));
	accf_arg = strchr(accf, ',');
	if (accf_arg == NULL)	/* zero or one arg, no more */
		return;

	if (strrchr(accf, ',') != accf_arg) {
		sep->se_socktype = -1;
	} else {
		accf_arg++;
		strlcpy(sep->se_accf.af_arg, accf_arg,
		    sizeof(sep->se_accf.af_arg));
	}
}

void
parse_socktype(char* arg, struct servtab* sep)
{
	/* stream socket may have an accept filter, only check first chars */
	if (strncmp(arg, "stream", sizeof("stream") - 1) == 0)
		sep->se_socktype = SOCK_STREAM;
	else if (strcmp(arg, "dgram") == 0)
		sep->se_socktype = SOCK_DGRAM;
	else if (strcmp(arg, "rdm") == 0)
		sep->se_socktype = SOCK_RDM;
	else if (strcmp(arg, "seqpacket") == 0)
		sep->se_socktype = SOCK_SEQPACKET;
	else if (strcmp(arg, "raw") == 0)
		sep->se_socktype = SOCK_RAW;
	else
		sep->se_socktype = -1;
}

static struct servtab
init_servtab(void)
{
	/* This does not set every field to default. See enter() as well */
	return (struct servtab) {
		/*
		 * Set se_max to non-zero so uninitialized value is not
	 	 * a valid value. Useful in v2 syntax parsing.
		 */
		.se_service_max = SERVTAB_UNSPEC_SIZE_T,
		.se_ip_max = SERVTAB_UNSPEC_SIZE_T,
		.se_wait = SERVTAB_UNSPEC_VAL,
		.se_socktype = SERVTAB_UNSPEC_VAL,
		.se_rl_ip_list = SLIST_HEAD_INITIALIZER(se_ip_list_head)
		/* All other fields initialized to 0 or null */
	};
}

/* Include directives bookkeeping structure */
struct file_list {
	/* Absolute path used for checking for circular references */
	char *abs;
	/* Pointer to the absolute path of the parent config file,
	 * on the stack */
	struct file_list *next;
} *file_list_head;

static void
include_configs(char *pattern)
{
	/* Allocate global per-config state on the thread stack */
	const char* save_CONFIG;
	FILE	*save_fconfig;
	size_t	save_line_number;
	char    *save_defhost;
	struct	file_list new_file;
#ifdef IPSEC
	char *save_policy;
#endif

	/* Store current globals on the stack */
	save_CONFIG = CONFIG;
	save_fconfig = fconfig;
	save_line_number = line_number;
	save_defhost = defhost;
	new_file.abs = realpath(CONFIG, NULL);
	new_file.next = file_list_head;
#ifdef IPSEC
	save_policy = policy;
#endif
	/* Put new_file at the top of the config stack */
	file_list_head = &new_file;
	read_glob_configs(pattern);
	free(new_file.abs);
	/* Pop new_file off the stack */
	file_list_head = new_file.next;

	/* Restore global per-config state */
	CONFIG = save_CONFIG;
	fconfig = save_fconfig;
	line_number = save_line_number;
	defhost = save_defhost;
#ifdef IPSEC
	policy = save_policy;
#endif
}

static void
prepare_next_config(const char *file_name)
{
	/* Setup new state that is normally only done in main */
	CONFIG = file_name;

	/* Inherit default host and IPsec policy */
	defhost = newstr(defhost);

#ifdef IPSEC
	policy = (policy == NULL) ? NULL : newstr(policy);
#endif
}

static void
read_glob_configs(char *pattern)
{
	glob_t results;
	char *full_pattern;
	int glob_result;
	full_pattern = gen_file_pattern(CONFIG, pattern);

	DPRINTCONF("Found include directive '%s'", full_pattern);

	glob_result = glob(full_pattern, GLOB_NOSORT, glob_error, &results);
	switch(glob_result) {
	case 0:
		/* No glob errors */
		break;
	case GLOB_ABORTED:
		ERR("Error while searching for include files");
		break;
	case GLOB_NOMATCH:
		/* It's fine if no files were matched. */
		DPRINTCONF("No files matched pattern '%s'", full_pattern);
		break;
	case GLOB_NOSPACE:
		ERR("Error when searching for include files: %s",
		    strerror(errno));
		break;
	default:
		ERR("Unknown glob(3) error %d", errno);
		break;
	}
	free(full_pattern);

	for (size_t i = 0; i < results.gl_pathc; i++) {
		include_matched_path(results.gl_pathv[i]);
	}

	globfree(&results);
}

static void
include_matched_path(char *glob_path)
{
	struct stat sb;
	char *tmp;

	if (lstat(glob_path, &sb) != 0) {
		ERR("Error calling stat on path '%s': %s", glob_path,
		    strerror(errno));
		return;
	}

	if (!S_ISREG(sb.st_mode) && !S_ISLNK(sb.st_mode)) {
		DPRINTCONF("'%s' is not a file.", glob_path);
		ERR("The matched path '%s' is not a regular file", glob_path);
		return;
	}

	DPRINTCONF("Include '%s'", glob_path);

	if (S_ISLNK(sb.st_mode)) {
		tmp = glob_path;
		glob_path = realpath(tmp, NULL);
	}

	/* Ensure the file is not being reincluded .*/
	if (check_no_reinclude(glob_path)) {
		prepare_next_config(glob_path);
		config();
	} else {
		DPRINTCONF("File '%s' already included in current include "
		    "chain", glob_path);
		WRN("Including file '%s' would cause a circular "
		    "dependency", glob_path);
	}

	if (S_ISLNK(sb.st_mode)) {
		free(glob_path);
		glob_path = tmp;
	}
}

static bool
check_no_reinclude(const char *glob_path)
{
	struct file_list *cur = file_list_head;
	char *abs_path = realpath(glob_path, NULL);

	if (abs_path == NULL) {
		ERR("Error checking real path for '%s': %s",
			glob_path, strerror(errno));
		return false;
	}

	DPRINTCONF("Absolute path '%s'", abs_path);

	for (cur = file_list_head; cur != NULL; cur = cur->next) {
		if (strcmp(cur->abs, abs_path) == 0) {
			/* file included more than once */
			/* TODO relative or abs path for logging error? */
			free(abs_path);
			return false;
		}
	}
	free(abs_path);
	return true;
}

/* Resolve the pattern relative to the config file the pattern is from  */
static char *
gen_file_pattern(const char *cur_config, const char *pattern)
{
	if (pattern[0] == '/') {
		/* Absolute paths don't need any normalization */
		return newstr(pattern);
	}

	/* pattern is relative */
	/* Find the end of the file's directory */
	size_t i, last = 0;
	for (i = 0; cur_config[i] != '\0'; i++) {
		if (cur_config[i] == '/') {
			last = i;
		}
	}

	if (last == 0) {
		/* cur_config is just a filename, pattern already correct */
		return newstr(pattern);
	}

	/* Relativize pattern to cur_config file's directory */
	char *full_pattern = malloc(last + 1 + strlen(pattern) + 1);
	if (full_pattern == NULL) {
		syslog(LOG_ERR, "Out of memory.");
		exit(EXIT_FAILURE);
	}
	memcpy(full_pattern, cur_config, last);
	full_pattern[last] = '/';
	strcpy(&full_pattern[last + 1], pattern);
	return full_pattern;
}

static int
glob_error(const char *path, int error)
{
	WRN("Error while resolving path '%s': %s", path, strerror(error));
	return 0;
}
