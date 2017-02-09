/* $OpenBSD: netcat.c,v 1.172 2017/02/05 01:39:14 jca Exp $ */
/*
 * Copyright (c) 2001 Eric Jackson <ericj@monkey.org>
 * Copyright (c) 2015 Bob Beck.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: netcat.c,v 1.5 2017/02/09 21:23:48 christos Exp $");

/*
 * Re-written nc(1) for OpenBSD. Original implementation by
 * *Hobbit* <hobbit@avian.org>.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/telnet.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef CRYPTO
#include <tls.h>
#else
#define TLS_WANT_POLLIN -2
#define TLS_WANT_POLLOUT -2
#endif
#include "atomicio.h"

#ifdef __NetBSD__
#define accept4(a, b, c, d) paccept((a), (b), (c), NULL, (d))
#endif

#define PORT_MAX	65535
#define UNIX_DG_TMP_SOCKET_SIZE	19

#define POLL_STDIN 0
#define POLL_NETOUT 1
#define POLL_NETIN 2
#define POLL_STDOUT 3
#define BUFSIZE 16384
#define DEFAULT_CA_FILE "/etc/ssl/cert.pem"

#define TLS_ALL	(1 << 1)
#define TLS_NOVERIFY	(1 << 2)
#define TLS_NONAME	(1 << 3)
#define TLS_CCERT	(1 << 4)
#define TLS_MUSTSTAPLE	(1 << 5)

/* Command Line Options */
int	dflag;					/* detached, no stdin */
int	Fflag;					/* fdpass sock to stdout */
unsigned int iflag;				/* Interval Flag */
int	kflag;					/* More than one connect */
int	lflag;					/* Bind to local port */
int	Nflag;					/* shutdown() network socket */
int	nflag;					/* Don't do name look up */
char   *Pflag;					/* Proxy username */
char   *pflag;					/* Localport flag */
int	rflag;					/* Random ports flag */
char   *sflag;					/* Source Address */
int	tflag;					/* Telnet Emulation */
int	uflag;					/* UDP - Default to TCP */
int	vflag;					/* Verbosity */
int	xflag;					/* Socks proxy */
int	zflag;					/* Port Scan Flag */
int	Dflag;					/* sodebug */
int	Iflag;					/* TCP receive buffer size */
int	Oflag;					/* TCP send buffer size */
int	Sflag;					/* TCP MD5 signature option */
int	Tflag = -1;				/* IP Type of Service */
#ifdef __OpenBSD__
int	rtableid = -1;
#endif

int	usetls;					/* use TLS */
char    *Cflag;					/* Public cert file */
char    *Kflag;					/* Private key file */
char    *oflag;					/* OCSP stapling file */
const char    *Rflag = DEFAULT_CA_FILE;		/* Root CA file */
int	tls_cachanged;				/* Using non-default CA file */
int     TLSopt;					/* TLS options */
char	*tls_expectname;			/* required name in peer cert */
char	*tls_expecthash;			/* required hash of peer cert */

int timeout = -1;
int family = AF_UNSPEC;
char *portlist[PORT_MAX+1];
char *unix_dg_tmp_socket;
int ttl = -1;
int minttl = -1;

void	atelnet(int, unsigned char *, unsigned int);
void	build_ports(char *);
static void	help(void) __dead;
int	local_listen(char *, char *, struct addrinfo);
struct tls;
void	readwrite(int, struct tls *);
void	fdpass(int nfd) __dead;
int	remote_connect(const char *, const char *, struct addrinfo);
int	timeout_connect(int, const struct sockaddr *, socklen_t);
int	socks_connect(const char *, const char *, struct addrinfo,
	    const char *, const char *, struct addrinfo, int, const char *);
int	udptest(int);
int	unix_bind(char *, int);
int	unix_connect(char *);
int	unix_listen(char *);
void	set_common_sockopts(int, int);
int	map_tos(char *, int *);
int	map_tls(char *, int *);
void	report_connect(const struct sockaddr *, socklen_t, char *);
void	report_tls(struct tls *tls_ctx, char * host, char *tlsexpectname);
void	usage(int);
ssize_t drainbuf(int, unsigned char *, size_t *, struct tls *);
ssize_t fillbuf(int, unsigned char *, size_t *, struct tls *);
void	tls_setup_client(struct tls *, int, char *);
struct tls *tls_setup_server(struct tls *, int, char *);

int
main(int argc, char *argv[])
{
	int ch, s = -1, ret, socksv;
	char *host, *uport;
	struct addrinfo hints;
	struct servent *sv;
	socklen_t len;
	struct sockaddr_storage cliaddr;
	char *proxy = NULL, *proxyport = NULL;
	int errnum;
	struct addrinfo proxyhints;
	char unix_dg_tmp_socket_buf[UNIX_DG_TMP_SOCKET_SIZE];
#ifdef CRYPTO
	struct tls_config *tls_cfg = NULL;
	struct tls *tls_ctx = NULL;
#endif

	ret = 1;
	socksv = 5;
	host = NULL;
	uport = NULL;
	sv = NULL;

	signal(SIGPIPE, SIG_IGN);

	while ((ch = getopt(argc, argv,
	    "46C:cDde:FH:hI:i:K:klM:m:NnO:o:P:p:R:rSs:T:tUuV:vw:X:x:z")) != -1) {
		switch (ch) {
		case '4':
			family = AF_INET;
			break;
		case '6':
			family = AF_INET6;
			break;
		case 'U':
			family = AF_UNIX;
			break;
		case 'X':
			if (strcasecmp(optarg, "connect") == 0)
				socksv = -1; /* HTTP proxy CONNECT */
			else if (strcmp(optarg, "4") == 0)
				socksv = 4; /* SOCKS v.4 */
			else if (strcmp(optarg, "5") == 0)
				socksv = 5; /* SOCKS v.5 */
			else
				errx(1, "unsupported proxy protocol");
			break;
#ifdef CRYPTO
		case 'C':
			Cflag = optarg;
			break;
		case 'c':
			usetls = 1;
			break;
#endif
		case 'd':
			dflag = 1;
			break;
		case 'e':
			tls_expectname = optarg;
			break;
		case 'F':
			Fflag = 1;
			break;
#ifdef CRYPTO
		case 'H':
			tls_expecthash = optarg;
			break;
#endif
		case 'h':
			help();
			break;
		case 'i':
			iflag = strtoi(optarg, NULL, 0, 0, UINT_MAX, &errnum);
			if (errnum)
				errc(1, errnum, "bad interval `%s'", optarg);
			break;
#ifdef CRYPTO
		case 'K':
			Kflag = optarg;
			break;
#endif
		case 'k':
			kflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'M':
			ttl = strtoi(optarg, NULL, 0, 0, 255, &errnum);
			if (errnum)
				errc(1, errnum, "bad ttl `%s'", optarg);
			break;
		case 'm':
			minttl = strtoi(optarg, NULL, 0, 0, 255, &errnum);
			if (errnum)
				errc(1, errnum, "bad minttl `%s'", optarg);
			break;
		case 'N':
			Nflag = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'P':
			Pflag = optarg;
			break;
		case 'p':
			pflag = optarg;
			break;
#ifdef CRYPTO
		case 'R':
			tls_cachanged = 1;
			Rflag = optarg;
			break;
#endif
		case 'r':
			rflag = 1;
			break;
		case 's':
			sflag = optarg;
			break;
		case 't':
			tflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
#ifdef __OpenBSD__
		case 'V':
			rtableid = (int)strtoi(optarg, NULL, 0, 0, 255, &errnum);
			if (errnum)
				errc(1, errnum, "bad rtable `%s'", optarg);
			break;
#endif
		case 'v':
			vflag = 1;
			break;
		case 'w':
			timeout = strtoi(optarg, NULL, 0, 0, INT_MAX / 1000, &errnum);
			if (errnum)
				errc(1, errnum, "bad timeout `%s'", optarg);
			timeout *= 1000;
			break;
		case 'x':
			xflag = 1;
			if ((proxy = strdup(optarg)) == NULL)
				err(1, NULL);
			break;
		case 'z':
			zflag = 1;
			break;
		case 'D':
			Dflag = 1;
			break;
		case 'I':
			Iflag = strtoi(optarg, NULL, 0, 1, 65536 << 14, &errnum);
			if (errnum)
				errc(1, errnum, "bad TCP receive window `%s'",
				    optarg);
			break;
		case 'O':
			Oflag = strtoi(optarg, NULL, 0, 1, 65536 << 14, &errnum);
			if (errnum)
				errc(1, errnum, "bad TCP send window `%s'",
				    optarg);
			break;
#ifdef CRYPTO
		case 'o':
			oflag = optarg;
			break;
#endif
		case 'S':
			Sflag = 1;
			break;
#ifdef CRYPTO
		case 'T':
			if (map_tos(optarg, &Tflag))
				break;
			if (map_tls(optarg, &TLSopt))
				break;
			Tflag = (int)strtoi(optarg, NULL, 0, 0, 255, &errnum);
			if (errnum)
				errc(1, errnum, "illegal tos/tls value `%s'",
				    optarg);
			break;
#endif
		default:
			usage(1);
		}
	}
	argc -= optind;
	argv += optind;

#ifdef __OpenBSD__
	if (rtableid >= 0)
		if (setrtable(rtableid) == -1)
			err(1, "setrtable");

	if (family == AF_UNIX) {
		if (pledge("stdio rpath wpath cpath tmppath unix", NULL) == -1)
			err(1, "pledge");
	} else if (Fflag) {
		if (Pflag) {
			if (pledge("stdio inet dns sendfd tty", NULL) == -1)
				err(1, "pledge");
		} else if (pledge("stdio inet dns sendfd", NULL) == -1)
			err(1, "pledge");
	} else if (Pflag) {
		if (pledge("stdio inet dns tty", NULL) == -1)
			err(1, "pledge");
	} else if (usetls) {
		if (pledge("stdio rpath inet dns", NULL) == -1)
			err(1, "pledge");
	} else if (pledge("stdio inet dns", NULL) == -1)
		err(1, "pledge");
#endif

	/* Cruft to make sure options are clean, and used properly. */
	if (argv[0] && !argv[1] && family == AF_UNIX) {
		host = argv[0];
		uport = NULL;
	} else if (argv[0] && !argv[1]) {
		if  (!lflag)
			usage(1);
		uport = argv[0];
		host = NULL;
	} else if (argv[0] && argv[1]) {
		host = argv[0];
		uport = argv[1];
	} else
		usage(1);

	if (lflag && sflag)
		errx(1, "cannot use -s and -l");
	if (lflag && pflag)
		errx(1, "cannot use -p and -l");
	if (lflag && zflag)
		errx(1, "cannot use -z and -l");
	if (!lflag && kflag)
		errx(1, "must use -l with -k");
	if (uflag && usetls)
		errx(1, "cannot use -c and -u");
	if ((family == AF_UNIX) && usetls)
		errx(1, "cannot use -c and -U");
	if ((family == AF_UNIX) && Fflag)
		errx(1, "cannot use -F and -U");
	if (Fflag && usetls)
		errx(1, "cannot use -c and -F");
#ifdef CRYPTO
	if (TLSopt && !usetls)
		errx(1, "you must specify -c to use TLS options");
	if (Cflag && !usetls)
		errx(1, "you must specify -c to use -C");
	if (Kflag && !usetls)
		errx(1, "you must specify -c to use -K");
	if (oflag && !Cflag)
		errx(1, "you must specify -C to use -o");
	if (tls_cachanged && !usetls)
		errx(1, "you must specify -c to use -R");
	if (tls_expecthash && !usetls)
		errx(1, "you must specify -c to use -H");
	if (tls_expectname && !usetls)
		errx(1, "you must specify -c to use -e");
#endif

	/* Get name of temporary socket for unix datagram client */
	if ((family == AF_UNIX) && uflag && !lflag) {
		if (sflag) {
			unix_dg_tmp_socket = sflag;
		} else {
			int fd;
			snprintf(unix_dg_tmp_socket_buf,
			    sizeof(unix_dg_tmp_socket_buf),
			    "/tmp/%s.XXXXXXXXXX", getprogname());
			/* XXX: abstract sockets instead? */
			if ((fd = mkstemp(unix_dg_tmp_socket_buf)) == -1)
				err(1, "mktemp");
			close(fd);
			unix_dg_tmp_socket = unix_dg_tmp_socket_buf;
		}
	}

	/* Initialize addrinfo structure. */
	if (family != AF_UNIX) {
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = family;
		hints.ai_socktype = uflag ? SOCK_DGRAM : SOCK_STREAM;
		hints.ai_protocol = uflag ? IPPROTO_UDP : IPPROTO_TCP;
		if (nflag)
			hints.ai_flags |= AI_NUMERICHOST;
	}

	if (xflag) {
		if (uflag)
			errx(1, "no proxy support for UDP mode");

		if (lflag)
			errx(1, "no proxy support for listen");

		if (family == AF_UNIX)
			errx(1, "no proxy support for unix sockets");

		if (sflag)
			errx(1, "no proxy support for local source address");

		if (*proxy == '[') {
			++proxy;
			proxyport = strchr(proxy, ']');
			if (proxyport == NULL)
				errx(1, "missing closing bracket in proxy");
			*proxyport++ = '\0';
			if (*proxyport == '\0')
				/* Use default proxy port. */
				proxyport = NULL;
			else {
				if (*proxyport == ':')
					++proxyport;
				else
					errx(1, "garbage proxy port delimiter");
			}
		} else {
			proxyport = strrchr(proxy, ':');
			if (proxyport != NULL)
				*proxyport++ = '\0';
		}

		memset(&proxyhints, 0, sizeof(struct addrinfo));
		proxyhints.ai_family = family;
		proxyhints.ai_socktype = SOCK_STREAM;
		proxyhints.ai_protocol = IPPROTO_TCP;
		if (nflag)
			proxyhints.ai_flags |= AI_NUMERICHOST;
	}

#ifdef CRYPTO
	if (usetls) {
#if __OpenBSD__
		if (Pflag) {
			if (pledge("stdio inet dns tty rpath", NULL) == -1)
				err(1, "pledge");
		} else if (pledge("stdio inet dns rpath", NULL) == -1)
			err(1, "pledge");
#endif

		if (tls_init() == -1)
			errx(1, "unable to initialize TLS");
		if ((tls_cfg = tls_config_new()) == NULL)
			errx(1, "unable to allocate TLS config");
		if (Rflag && tls_config_set_ca_file(tls_cfg, Rflag) == -1)
			errx(1, "%s", tls_config_error(tls_cfg));
		if (Cflag && tls_config_set_cert_file(tls_cfg, Cflag) == -1)
			errx(1, "%s", tls_config_error(tls_cfg));
		if (Kflag && tls_config_set_key_file(tls_cfg, Kflag) == -1)
			errx(1, "%s", tls_config_error(tls_cfg));
		if (oflag && tls_config_set_ocsp_staple_file(tls_cfg, oflag) == -1)
			errx(1, "%s", tls_config_error(tls_cfg));
		if (TLSopt & TLS_ALL) {
			if (tls_config_set_protocols(tls_cfg,
			    TLS_PROTOCOLS_ALL) != 0)
				errx(1, "%s", tls_config_error(tls_cfg));
			if (tls_config_set_ciphers(tls_cfg, "all") != 0)
				errx(1, "%s", tls_config_error(tls_cfg));
		}
		if (!lflag && (TLSopt & TLS_CCERT))
			errx(1, "clientcert is only valid with -l");
		if (TLSopt & TLS_NONAME)
			tls_config_insecure_noverifyname(tls_cfg);
		if (TLSopt & TLS_NOVERIFY) {
			if (tls_expecthash != NULL)
				errx(1, "-H and -T noverify may not be used"
				    "together");
			tls_config_insecure_noverifycert(tls_cfg);
		}
		if (TLSopt & TLS_MUSTSTAPLE)
			tls_config_ocsp_require_stapling(tls_cfg);

#ifdef __OpenBSD__
		if (Pflag) {
			if (pledge("stdio inet dns tty", NULL) == -1)
				err(1, "pledge");
		} else if (pledge("stdio inet dns", NULL) == -1)
			err(1, "pledge");
#endif
	}
#endif
	if (lflag) {
#ifdef CRYPTO
		struct tls *tls_cctx = NULL;
#endif
		int connfd;
		ret = 0;

		if (family == AF_UNIX) {
			if (uflag)
				s = unix_bind(host, 0);
			else
				s = unix_listen(host);
		}

#ifdef CRYPTO
		if (usetls) {
			tls_config_verify_client_optional(tls_cfg);
			if ((tls_ctx = tls_server()) == NULL)
				errx(1, "tls server creation failed");
			if (tls_configure(tls_ctx, tls_cfg) == -1)
				errx(1, "tls configuration failed (%s)",
				    tls_error(tls_ctx));
		}
#endif
		/* Allow only one connection at a time, but stay alive. */
		for (;;) {
			if (family != AF_UNIX)
				s = local_listen(host, uport, hints);
			if (s < 0)
				err(1, NULL);
			/*
			 * For UDP and -k, don't connect the socket, let it
			 * receive datagrams from multiple socket pairs.
			 */
			if (uflag && kflag)
				readwrite(s, NULL);
			/*
			 * For UDP and not -k, we will use recvfrom() initially
			 * to wait for a caller, then use the regular functions
			 * to talk to the caller.
			 */
			else if (uflag && !kflag) {
				int rv, plen;
				char buf[16384];
				struct sockaddr_storage z;

				len = sizeof(z);
				plen = 2048;
				rv = recvfrom(s, buf, plen, MSG_PEEK,
				    (struct sockaddr *)&z, &len);
				if (rv < 0)
					err(1, "recvfrom");

				rv = connect(s, (struct sockaddr *)&z, len);
				if (rv < 0)
					err(1, "connect");

				if (vflag)
					report_connect((struct sockaddr *)&z, len, NULL);

				readwrite(s, NULL);
			} else {
				len = sizeof(cliaddr);
				connfd = accept4(s, (struct sockaddr *)&cliaddr,
				    &len, SOCK_NONBLOCK);
				if (connfd == -1) {
					/* For now, all errnos are fatal */
					err(1, "accept");
				}
				if (vflag)
					report_connect((struct sockaddr *)&cliaddr, len,
					    family == AF_UNIX ? host : NULL);
#ifdef CRYPTO
				if ((usetls) &&
				    (tls_cctx = tls_setup_server(tls_ctx, connfd, host)))
					readwrite(connfd, tls_cctx);
				if (!usetls)
#endif
					readwrite(connfd, NULL);
#ifdef CRYPTO
				if (tls_cctx) {
					int i;

					do {
						i = tls_close(tls_cctx);
					} while (i == TLS_WANT_POLLIN ||
					    i == TLS_WANT_POLLOUT);
					tls_free(tls_cctx);
					tls_cctx = NULL;
				}
#endif
				close(connfd);
			}
			if (family != AF_UNIX)
				close(s);
			else if (uflag) {
				if (connect(s, NULL, 0) < 0)
					err(1, "connect");
			}

			if (!kflag)
				break;
		}
	} else if (family == AF_UNIX) {
		ret = 0;

		if ((s = unix_connect(host)) > 0 && !zflag) {
			readwrite(s, NULL);
			close(s);
		} else
			ret = 1;

		if (uflag)
			unlink(unix_dg_tmp_socket);
		exit(ret);

	} else {
		int i = 0;

		/* Construct the portlist[] array. */
		build_ports(uport);

		/* Cycle through portlist, connecting to each port. */
		for (s = -1, i = 0; portlist[i] != NULL; i++) {
			if (s != -1)
				close(s);

#ifdef CRYPTO
			if (usetls) {
				if ((tls_ctx = tls_client()) == NULL)
					errx(1, "tls client creation failed");
				if (tls_configure(tls_ctx, tls_cfg) == -1)
					errx(1, "tls configuration failed (%s)",
					    tls_error(tls_ctx));
			}
#endif
			if (xflag)
				s = socks_connect(host, portlist[i], hints,
				    proxy, proxyport, proxyhints, socksv,
				    Pflag);
			else
				s = remote_connect(host, portlist[i], hints);

			if (s == -1)
				continue;

			ret = 0;
			if (vflag || zflag) {
				/* For UDP, make sure we are connected. */
				if (uflag) {
					if (udptest(s) == -1) {
						ret = 1;
						continue;
					}
				}

				/* Don't look up port if -n. */
				if (nflag)
					sv = NULL;
				else {
					sv = getservbyport(
					    ntohs(atoi(portlist[i])),
					    uflag ? "udp" : "tcp");
				}

				fprintf(stderr,
				    "Connection to %s %s port [%s/%s] "
				    "succeeded!\n", host, portlist[i],
				    uflag ? "udp" : "tcp",
				    sv ? sv->s_name : "*");
			}
			if (Fflag)
				fdpass(s);
			else {
#ifdef CRYPTO
				if (usetls)
					tls_setup_client(tls_ctx, s, host);
				if (!zflag)
					readwrite(s, tls_ctx);
				if (tls_ctx) {
					int j;

					do {
						j = tls_close(tls_ctx);
					} while (j == TLS_WANT_POLLIN ||
					    j == TLS_WANT_POLLOUT);
					tls_free(tls_ctx);
					tls_ctx = NULL;
				}
#else
				if (!zflag)
					readwrite(s, NULL);
#endif
			}
		}
	}

	if (s != -1)
		close(s);

#ifdef CRYPTO
	tls_config_free(tls_cfg);
#endif

	exit(ret);
}

/*
 * unix_bind()
 * Returns a unix socket bound to the given path
 */
int
unix_bind(char *path, int flags)
{
	struct sockaddr_un s_un;
	int s, save_errno;

	/* Create unix domain socket. */
	if ((s = socket(AF_UNIX, flags | (uflag ? SOCK_DGRAM : SOCK_STREAM),
	    0)) < 0)
		return (-1);

	memset(&s_un, 0, sizeof(struct sockaddr_un));
	s_un.sun_family = AF_UNIX;

	if (strlcpy(s_un.sun_path, path, sizeof(s_un.sun_path)) >=
	    sizeof(s_un.sun_path)) {
		close(s);
		errno = ENAMETOOLONG;
		return (-1);
	}

	if (bind(s, (struct sockaddr *)&s_un, sizeof(s_un)) < 0) {
		save_errno = errno;
		close(s);
		errno = save_errno;
		return (-1);
	}
	return (s);
}

#ifdef CRYPTO
void
tls_setup_client(struct tls *tls_ctx, int s, char *host)
{
	int i;

	if (tls_connect_socket(tls_ctx, s,
		tls_expectname ? tls_expectname : host) == -1) {
		errx(1, "tls connection failed (%s)",
		    tls_error(tls_ctx));
	}
	do {
		if ((i = tls_handshake(tls_ctx)) == -1)
			errx(1, "tls handshake failed (%s)",
			    tls_error(tls_ctx));
	} while (i == TLS_WANT_POLLIN || i == TLS_WANT_POLLOUT);
	if (vflag)
		report_tls(tls_ctx, host, tls_expectname);
	if (tls_expecthash && tls_peer_cert_hash(tls_ctx) &&
	    strcmp(tls_expecthash, tls_peer_cert_hash(tls_ctx)) != 0)
		errx(1, "peer certificate is not %s", tls_expecthash);
}

struct tls *
tls_setup_server(struct tls *tls_ctx, int connfd, char *host)
{
	struct tls *tls_cctx;

	if (tls_accept_socket(tls_ctx, &tls_cctx,
		connfd) == -1) {
		warnx("tls accept failed (%s)",
		    tls_error(tls_ctx));
		tls_cctx = NULL;
	} else {
		int i;

		do {
			if ((i = tls_handshake(tls_cctx)) == -1)
				warnx("tls handshake failed (%s)",
				    tls_error(tls_cctx));
		} while(i == TLS_WANT_POLLIN || i == TLS_WANT_POLLOUT);
	}
	if (tls_cctx) {
		int gotcert = tls_peer_cert_provided(tls_cctx);

		if (vflag && gotcert)
			report_tls(tls_cctx, host, tls_expectname);
		if ((TLSopt & TLS_CCERT) && !gotcert)
			warnx("No client certificate provided");
		else if (gotcert && tls_peer_cert_hash(tls_ctx) && tls_expecthash &&
		    strcmp(tls_expecthash, tls_peer_cert_hash(tls_ctx)) != 0)
			warnx("peer certificate is not %s", tls_expecthash);
		else if (gotcert && tls_expectname &&
		    (!tls_peer_cert_contains_name(tls_cctx, tls_expectname)))
			warnx("name (%s) not found in client cert",
			    tls_expectname);
		else {
			return tls_cctx;
		}
	}
	return NULL;
}
#endif

/*
 * unix_connect()
 * Returns a socket connected to a local unix socket. Returns -1 on failure.
 */
int
unix_connect(char *path)
{
	struct sockaddr_un s_un;
	int s, save_errno;

	if (uflag) {
		if ((s = unix_bind(unix_dg_tmp_socket, SOCK_CLOEXEC)) < 0)
			return (-1);
	} else {
		if ((s = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0)
			return (-1);
	}

	memset(&s_un, 0, sizeof(struct sockaddr_un));
	s_un.sun_family = AF_UNIX;

	if (strlcpy(s_un.sun_path, path, sizeof(s_un.sun_path)) >=
	    sizeof(s_un.sun_path)) {
		close(s);
		errno = ENAMETOOLONG;
		return (-1);
	}
	if (connect(s, (struct sockaddr *)&s_un, sizeof(s_un)) < 0) {
		save_errno = errno;
		close(s);
		errno = save_errno;
		return (-1);
	}
	return (s);

}

/*
 * unix_listen()
 * Create a unix domain socket, and listen on it.
 */
int
unix_listen(char *path)
{
	int s;
	if ((s = unix_bind(path, 0)) < 0)
		return (-1);

	if (listen(s, 5) < 0) {
		close(s);
		return (-1);
	}
	return (s);
}

/*
 * remote_connect()
 * Returns a socket connected to a remote host. Properly binds to a local
 * port or source address if needed. Returns -1 on failure.
 */
int
remote_connect(const char *host, const char *port, struct addrinfo hints)
{
	struct addrinfo *res, *res0;
	int s = -1, error, save_errno;

	if ((error = getaddrinfo(host, port, &hints, &res0)))
		errx(1, "getaddrinfo: %s", gai_strerror(error));

	for (res = res0; res; res = res->ai_next) {
		if ((s = socket(res->ai_family, res->ai_socktype |
		    SOCK_NONBLOCK, res->ai_protocol)) < 0)
			continue;

		/* Bind to a local port or source address if specified. */
		if (sflag || pflag) {
			struct addrinfo ahints, *ares;

#ifdef SO_BINDANY
			/* try SO_BINDANY, but don't insist */
			setsockopt(s, SOL_SOCKET, SO_BINDANY, &on, sizeof(on));
#endif
			memset(&ahints, 0, sizeof(struct addrinfo));
			ahints.ai_family = res->ai_family;
			ahints.ai_socktype = uflag ? SOCK_DGRAM : SOCK_STREAM;
			ahints.ai_protocol = uflag ? IPPROTO_UDP : IPPROTO_TCP;
			ahints.ai_flags = AI_PASSIVE;
			if ((error = getaddrinfo(sflag, pflag, &ahints, &ares)))
				errx(1, "getaddrinfo: %s", gai_strerror(error));

			if (bind(s, (struct sockaddr *)ares->ai_addr,
			    ares->ai_addrlen) < 0)
				err(1, "bind failed");
			freeaddrinfo(ares);
		}

		set_common_sockopts(s, res->ai_family);

		if (timeout_connect(s, res->ai_addr, res->ai_addrlen) == 0)
			break;
		if (vflag)
			warn("connect to %s port %s (%s) failed", host, port,
			    uflag ? "udp" : "tcp");

		save_errno = errno;
		close(s);
		errno = save_errno;
		s = -1;
	}

	freeaddrinfo(res0);

	return (s);
}

int
timeout_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
	struct pollfd pfd;
	socklen_t optlen;
	int optval;
	int ret;

	if ((ret = connect(s, name, namelen)) != 0 && errno == EINPROGRESS) {
		pfd.fd = s;
		pfd.events = POLLOUT;
		if ((ret = poll(&pfd, 1, timeout)) == 1) {
			optlen = sizeof(optval);
			if ((ret = getsockopt(s, SOL_SOCKET, SO_ERROR,
			    &optval, &optlen)) == 0) {
				errno = optval;
				ret = optval == 0 ? 0 : -1;
			}
		} else if (ret == 0) {
			errno = ETIMEDOUT;
			ret = -1;
		} else
			err(1, "poll failed");
	}

	return (ret);
}

/*
 * local_listen()
 * Returns a socket listening on a local port, binds to specified source
 * address. Returns -1 on failure.
 */
int
local_listen(char *host, char *port, struct addrinfo hints)
{
	struct addrinfo *res, *res0;
	int s = -1, ret, x = 1, save_errno;
	int error;

	/* Allow nodename to be null. */
	hints.ai_flags |= AI_PASSIVE;

	/*
	 * In the case of binding to a wildcard address
	 * default to binding to an ipv4 address.
	 */
	if (host == NULL && hints.ai_family == AF_UNSPEC)
		hints.ai_family = AF_INET;

	if ((error = getaddrinfo(host, port, &hints, &res0)))
		errx(1, "getaddrinfo: %s", gai_strerror(error));

	for (res = res0; res; res = res->ai_next) {
		if ((s = socket(res->ai_family, res->ai_socktype,
		    res->ai_protocol)) < 0)
			continue;

		ret = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &x, sizeof(x));
		if (ret == -1)
			err(1, NULL);

		set_common_sockopts(s, res->ai_family);

		if (bind(s, (struct sockaddr *)res->ai_addr,
		    res->ai_addrlen) == 0)
			break;

		save_errno = errno;
		close(s);
		errno = save_errno;
		s = -1;
	}

	if (!uflag && s != -1) {
		if (listen(s, 1) < 0)
			err(1, "listen");
	}

	freeaddrinfo(res0);

	return (s);
}

/*
 * readwrite()
 * Loop that polls on the network file descriptor and stdin.
 */
void
readwrite(int net_fd, struct tls *tls_ctx)
{
	struct pollfd pfd[4];
	int stdin_fd = STDIN_FILENO;
	int stdout_fd = STDOUT_FILENO;
	unsigned char netinbuf[BUFSIZE];
	size_t netinbufpos = 0;
	unsigned char stdinbuf[BUFSIZE];
	size_t stdinbufpos = 0;
	int n, num_fds;
	ssize_t ret;

	/* don't read from stdin if requested */
	if (dflag)
		stdin_fd = -1;

	/* stdin */
	pfd[POLL_STDIN].fd = stdin_fd;
	pfd[POLL_STDIN].events = POLLIN;

	/* network out */
	pfd[POLL_NETOUT].fd = net_fd;
	pfd[POLL_NETOUT].events = 0;

	/* network in */
	pfd[POLL_NETIN].fd = net_fd;
	pfd[POLL_NETIN].events = POLLIN;

	/* stdout */
	pfd[POLL_STDOUT].fd = stdout_fd;
	pfd[POLL_STDOUT].events = 0;

	while (1) {
		/* both inputs are gone, buffers are empty, we are done */
		if (pfd[POLL_STDIN].fd == -1 && pfd[POLL_NETIN].fd == -1 &&
		    stdinbufpos == 0 && netinbufpos == 0) {
			close(net_fd);
			return;
		}
		/* both outputs are gone, we can't continue */
		if (pfd[POLL_NETOUT].fd == -1 && pfd[POLL_STDOUT].fd == -1) {
			close(net_fd);
			return;
		}
		/* listen and net in gone, queues empty, done */
		if (lflag && pfd[POLL_NETIN].fd == -1 &&
		    stdinbufpos == 0 && netinbufpos == 0) {
			close(net_fd);
			return;
		}

		/* help says -i is for "wait between lines sent". We read and
		 * write arbitrary amounts of data, and we don't want to start
		 * scanning for newlines, so this is as good as it gets */
		if (iflag)
			sleep(iflag);

		/* poll */
		num_fds = poll(pfd, 4, timeout);

		/* treat poll errors */
		if (num_fds == -1) {
			close(net_fd);
			err(1, "polling error");
		}

		/* timeout happened */
		if (num_fds == 0)
			return;

		/* treat socket error conditions */
		for (n = 0; n < 4; n++) {
			if (pfd[n].revents & (POLLERR|POLLNVAL)) {
				pfd[n].fd = -1;
			}
		}
		/* reading is possible after HUP */
		if (pfd[POLL_STDIN].events & POLLIN &&
		    pfd[POLL_STDIN].revents & POLLHUP &&
		    !(pfd[POLL_STDIN].revents & POLLIN))
			pfd[POLL_STDIN].fd = -1;

		if (pfd[POLL_NETIN].events & POLLIN &&
		    pfd[POLL_NETIN].revents & POLLHUP &&
		    !(pfd[POLL_NETIN].revents & POLLIN))
			pfd[POLL_NETIN].fd = -1;

		if (pfd[POLL_NETOUT].revents & POLLHUP) {
			if (Nflag)
				shutdown(pfd[POLL_NETOUT].fd, SHUT_WR);
			pfd[POLL_NETOUT].fd = -1;
		}
		/* if HUP, stop watching stdout */
		if (pfd[POLL_STDOUT].revents & POLLHUP)
			pfd[POLL_STDOUT].fd = -1;
		/* if no net out, stop watching stdin */
		if (pfd[POLL_NETOUT].fd == -1)
			pfd[POLL_STDIN].fd = -1;
		/* if no stdout, stop watching net in */
		if (pfd[POLL_STDOUT].fd == -1) {
			if (pfd[POLL_NETIN].fd != -1)
				shutdown(pfd[POLL_NETIN].fd, SHUT_RD);
			pfd[POLL_NETIN].fd = -1;
		}

		/* try to read from stdin */
		if (pfd[POLL_STDIN].revents & POLLIN && stdinbufpos < BUFSIZE) {
			ret = fillbuf(pfd[POLL_STDIN].fd, stdinbuf,
			    &stdinbufpos, NULL);
			if (ret == TLS_WANT_POLLIN)
				pfd[POLL_STDIN].events = POLLIN;
			else if (ret == TLS_WANT_POLLOUT)
				pfd[POLL_STDIN].events = POLLOUT;
			else if (ret == 0 || ret == -1)
				pfd[POLL_STDIN].fd = -1;
			/* read something - poll net out */
			if (stdinbufpos > 0)
				pfd[POLL_NETOUT].events = POLLOUT;
			/* filled buffer - remove self from polling */
			if (stdinbufpos == BUFSIZE)
				pfd[POLL_STDIN].events = 0;
		}
		/* try to write to network */
		if (pfd[POLL_NETOUT].revents & POLLOUT && stdinbufpos > 0) {
			ret = drainbuf(pfd[POLL_NETOUT].fd, stdinbuf,
			    &stdinbufpos, tls_ctx);
			if (ret == TLS_WANT_POLLIN)
				pfd[POLL_NETOUT].events = POLLIN;
			else if (ret == TLS_WANT_POLLOUT)
				pfd[POLL_NETOUT].events = POLLOUT;
			else if (ret == -1)
				pfd[POLL_NETOUT].fd = -1;
			/* buffer empty - remove self from polling */
			if (stdinbufpos == 0)
				pfd[POLL_NETOUT].events = 0;
			/* buffer no longer full - poll stdin again */
			if (stdinbufpos < BUFSIZE)
				pfd[POLL_STDIN].events = POLLIN;
		}
		/* try to read from network */
		if (pfd[POLL_NETIN].revents & POLLIN && netinbufpos < BUFSIZE) {
			ret = fillbuf(pfd[POLL_NETIN].fd, netinbuf,
			    &netinbufpos, tls_ctx);
			if (ret == TLS_WANT_POLLIN)
				pfd[POLL_NETIN].events = POLLIN;
			else if (ret == TLS_WANT_POLLOUT)
				pfd[POLL_NETIN].events = POLLOUT;
			else if (ret == -1)
				pfd[POLL_NETIN].fd = -1;
			/* eof on net in - remove from pfd */
			if (ret == 0) {
				shutdown(pfd[POLL_NETIN].fd, SHUT_RD);
				pfd[POLL_NETIN].fd = -1;
			}
			/* read something - poll stdout */
			if (netinbufpos > 0)
				pfd[POLL_STDOUT].events = POLLOUT;
			/* filled buffer - remove self from polling */
			if (netinbufpos == BUFSIZE)
				pfd[POLL_NETIN].events = 0;
			/* handle telnet */
			if (tflag)
				atelnet(pfd[POLL_NETIN].fd, netinbuf,
				    netinbufpos);
		}
		/* try to write to stdout */
		if (pfd[POLL_STDOUT].revents & POLLOUT && netinbufpos > 0) {
			ret = drainbuf(pfd[POLL_STDOUT].fd, netinbuf,
			    &netinbufpos, NULL);
			if (ret == TLS_WANT_POLLIN)
				pfd[POLL_STDOUT].events = POLLIN;
			else if (ret == TLS_WANT_POLLOUT)
				pfd[POLL_STDOUT].events = POLLOUT;
			else if (ret == -1)
				pfd[POLL_STDOUT].fd = -1;
			/* buffer empty - remove self from polling */
			if (netinbufpos == 0)
				pfd[POLL_STDOUT].events = 0;
			/* buffer no longer full - poll net in again */
			if (netinbufpos < BUFSIZE)
				pfd[POLL_NETIN].events = POLLIN;
		}

		/* stdin gone and queue empty? */
		if (pfd[POLL_STDIN].fd == -1 && stdinbufpos == 0) {
			if (pfd[POLL_NETOUT].fd != -1 && Nflag)
				shutdown(pfd[POLL_NETOUT].fd, SHUT_WR);
			pfd[POLL_NETOUT].fd = -1;
		}
		/* net in gone and queue empty? */
		if (pfd[POLL_NETIN].fd == -1 && netinbufpos == 0) {
			pfd[POLL_STDOUT].fd = -1;
		}
	}
}

ssize_t
drainbuf(int fd, unsigned char *buf, size_t *bufpos, struct tls *tls)
{
	ssize_t n;
	ssize_t adjust;

#ifdef CRYPTO
	if (tls)
		n = tls_write(tls, buf, *bufpos);
	else
#endif
	{
		n = write(fd, buf, *bufpos);
		/* don't treat EAGAIN, EINTR as error */
		if (n == -1 && (errno == EAGAIN || errno == EINTR))
			n = TLS_WANT_POLLOUT;
	}
	if (n <= 0)
		return n;
	/* adjust buffer */
	adjust = *bufpos - n;
	if (adjust > 0)
		memmove(buf, buf + n, adjust);
	*bufpos -= n;
	return n;
}

ssize_t
fillbuf(int fd, unsigned char *buf, size_t *bufpos, struct tls *tls)
{
	size_t num = BUFSIZE - *bufpos;
	ssize_t n;

#ifdef CRYPTO
	if (tls)
		n = tls_read(tls, buf + *bufpos, num);
	else
#endif
	{

		n = read(fd, buf + *bufpos, num);
		/* don't treat EAGAIN, EINTR as error */
		if (n == -1 && (errno == EAGAIN || errno == EINTR))
			n = TLS_WANT_POLLIN;
	}
	if (n <= 0)
		return n;
	*bufpos += n;
	return n;
}

/*
 * fdpass()
 * Pass the connected file descriptor to stdout and exit.
 */
void
fdpass(int nfd)
{
	struct msghdr mh;
	union {
		struct cmsghdr hdr;
		char buf[CMSG_SPACE(sizeof(int))];
	} cmsgbuf;
	struct cmsghdr *cmsg;
	struct iovec iov;
	char c = '\0';
	ssize_t r;
	struct pollfd pfd;

	/* Avoid obvious stupidity */
	if (isatty(STDOUT_FILENO))
		errx(1, "Cannot pass file descriptor to tty");

	bzero(&mh, sizeof(mh));
	bzero(&cmsgbuf, sizeof(cmsgbuf));
	bzero(&iov, sizeof(iov));

	mh.msg_control = (caddr_t)&cmsgbuf.buf;
	mh.msg_controllen = sizeof(cmsgbuf.buf);
	cmsg = CMSG_FIRSTHDR(&mh);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	*(int *)CMSG_DATA(cmsg) = nfd;

	iov.iov_base = &c;
	iov.iov_len = 1;
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;

	bzero(&pfd, sizeof(pfd));
	pfd.fd = STDOUT_FILENO;
	pfd.events = POLLOUT;
	for (;;) {
		r = sendmsg(STDOUT_FILENO, &mh, 0);
		if (r == -1) {
			if (errno == EAGAIN || errno == EINTR) {
				if (poll(&pfd, 1, -1) == -1)
					err(1, "poll");
				continue;
			}
			err(1, "sendmsg");
		} else if (r != 1)
			errx(1, "sendmsg: unexpected return value %zd", r);
		else
			break;
	}
	exit(0);
}

/* Deal with RFC 854 WILL/WONT DO/DONT negotiation. */
void
atelnet(int nfd, unsigned char *buf, unsigned int size)
{
	unsigned char *p, *end;
	unsigned char obuf[4];

	if (size < 3)
		return;
	end = buf + size - 2;

	for (p = buf; p < end; p++) {
		if (*p != IAC)
			continue;

		obuf[0] = IAC;
		p++;
		if ((*p == WILL) || (*p == WONT))
			obuf[1] = DONT;
		else if ((*p == DO) || (*p == DONT))
			obuf[1] = WONT;
		else
			continue;

		p++;
		obuf[2] = *p;
		if (atomicio(vwrite, nfd, obuf, 3) != 3)
			warn("Write Error!");
	}
}


static int
strtoport(const char *portstr, int udp)
{
	struct servent *entry;
	int errnum;
	const char *proto;
	int port;

	proto = udp ? "udp" : "tcp";

	port = strtoi(portstr, NULL, 0, 1, PORT_MAX, &errnum);
	if (errnum == 0)
		return port;
	if ((entry = getservbyname(portstr, proto)) == NULL)
		errx(1, "service \"%s\" unknown", portstr);
	return ntohs(entry->s_port);
}

/*
 * build_ports()
 * Build an array of ports in portlist[], listing each port
 * that we should try to connect to.
 */
void
build_ports(char *p)
{
	char *n;
	int hi, lo, cp;
	int x = 0;

	if ((n = strchr(p, '-')) != NULL) {
		*n = '\0';
		n++;

		/* Make sure the ports are in order: lowest->highest. */
		hi = strtoport(n, uflag);
		lo = strtoport(p, uflag);
		if (lo > hi) {
			cp = hi;
			hi = lo;
			lo = cp;
		}

		/*
		 * Initialize portlist with a random permutation.  Based on
		 * Knuth, as in ip_randomid() in sys/netinet/ip_id.c.
		 */
		if (rflag) {
			for (x = 0; x <= hi - lo; x++) {
				cp = arc4random_uniform(x + 1);
				portlist[x] = portlist[cp];
				if (asprintf(&portlist[cp], "%d", x + lo) < 0)
					err(1, "asprintf");
			}
		} else { /* Load ports sequentially. */
			for (cp = lo; cp <= hi; cp++) {
				if (asprintf(&portlist[x], "%d", cp) < 0)
					err(1, "asprintf");
				x++;
			}
		}
	} else {
		char *tmp;

		hi = strtoport(p, uflag);
		if (asprintf(&tmp, "%d", hi) != -1)
			portlist[0] = tmp;
		else
			err(1, NULL);
	}
}

/*
 * udptest()
 * Do a few writes to see if the UDP port is there.
 * Fails once PF state table is full.
 */
int
udptest(int s)
{
	int i, ret;

	for (i = 0; i <= 3; i++) {
		if (write(s, "X", 1) == 1)
			ret = 1;
		else
			ret = -1;
	}
	return (ret);
}

void
set_common_sockopts(int s, int af)
{
	int x = 1;

	if (Sflag) {
		if (setsockopt(s, IPPROTO_TCP, TCP_MD5SIG,
			&x, sizeof(x)) == -1)
			err(1, NULL);
	}
	if (Dflag) {
		if (setsockopt(s, SOL_SOCKET, SO_DEBUG,
			&x, sizeof(x)) == -1)
			err(1, NULL);
	}
	if (Tflag != -1) {
		if (af == AF_INET && setsockopt(s, IPPROTO_IP,
		    IP_TOS, &Tflag, sizeof(Tflag)) == -1)
			err(1, "set IP ToS");

		else if (af == AF_INET6 && setsockopt(s, IPPROTO_IPV6,
		    IPV6_TCLASS, &Tflag, sizeof(Tflag)) == -1)
			err(1, "set IPv6 traffic class");
	}
	if (Iflag) {
		if (setsockopt(s, SOL_SOCKET, SO_RCVBUF,
		    &Iflag, sizeof(Iflag)) == -1)
			err(1, "set TCP receive buffer size");
	}
	if (Oflag) {
		if (setsockopt(s, SOL_SOCKET, SO_SNDBUF,
		    &Oflag, sizeof(Oflag)) == -1)
			err(1, "set TCP send buffer size");
	}

	if (ttl != -1) {
		if (af == AF_INET && setsockopt(s, IPPROTO_IP,
		    IP_TTL, &ttl, sizeof(ttl)))
			err(1, "set IP TTL");

		else if (af == AF_INET6 && setsockopt(s, IPPROTO_IPV6,
		    IPV6_UNICAST_HOPS, &ttl, sizeof(ttl)))
			err(1, "set IPv6 unicast hops");
	}

	if (minttl != -1) {
		if (af == AF_INET && setsockopt(s, IPPROTO_IP,
		    IP_MINTTL, &minttl, sizeof(minttl)))
			err(1, "set IP min TTL");
#ifdef IPV6_MINHOPCOUNT
		else if (af == AF_INET6 && setsockopt(s, IPPROTO_IPV6,
		    IPV6_MINHOPCOUNT, &minttl, sizeof(minttl)))
			err(1, "set IPv6 min hop count");
#endif
	}
}

int
map_tos(char *s, int *val)
{
	/* DiffServ Codepoints and other TOS mappings */
	const struct toskeywords {
		const char	*keyword;
		int		 val;
	} *t, toskeywords[] = {
		{ "af11",		IPTOS_DSCP_AF11 },
		{ "af12",		IPTOS_DSCP_AF12 },
		{ "af13",		IPTOS_DSCP_AF13 },
		{ "af21",		IPTOS_DSCP_AF21 },
		{ "af22",		IPTOS_DSCP_AF22 },
		{ "af23",		IPTOS_DSCP_AF23 },
		{ "af31",		IPTOS_DSCP_AF31 },
		{ "af32",		IPTOS_DSCP_AF32 },
		{ "af33",		IPTOS_DSCP_AF33 },
		{ "af41",		IPTOS_DSCP_AF41 },
		{ "af42",		IPTOS_DSCP_AF42 },
		{ "af43",		IPTOS_DSCP_AF43 },
		{ "critical",		IPTOS_PREC_CRITIC_ECP },
		{ "cs0",		IPTOS_DSCP_CS0 },
		{ "cs1",		IPTOS_DSCP_CS1 },
		{ "cs2",		IPTOS_DSCP_CS2 },
		{ "cs3",		IPTOS_DSCP_CS3 },
		{ "cs4",		IPTOS_DSCP_CS4 },
		{ "cs5",		IPTOS_DSCP_CS5 },
		{ "cs6",		IPTOS_DSCP_CS6 },
		{ "cs7",		IPTOS_DSCP_CS7 },
		{ "ef",			IPTOS_DSCP_EF },
		{ "inetcontrol",	IPTOS_PREC_INTERNETCONTROL },
		{ "lowdelay",		IPTOS_LOWDELAY },
		{ "netcontrol",		IPTOS_PREC_NETCONTROL },
		{ "reliability",	IPTOS_RELIABILITY },
		{ "throughput",		IPTOS_THROUGHPUT },
		{ NULL,			-1 },
	};

	for (t = toskeywords; t->keyword != NULL; t++) {
		if (strcmp(s, t->keyword) == 0) {
			*val = t->val;
			return (1);
		}
	}

	return (0);
}

#ifdef CRYPTO
int
map_tls(char *s, int *val)
{
	const struct tlskeywords {
		const char	*keyword;
		int		 val;
	} *t, tlskeywords[] = {
		{ "tlsall",		TLS_ALL },
		{ "noverify",		TLS_NOVERIFY },
		{ "noname",		TLS_NONAME },
		{ "clientcert",		TLS_CCERT},
		{ "muststaple",		TLS_MUSTSTAPLE},
		{ NULL,			-1 },
	};

	for (t = tlskeywords; t->keyword != NULL; t++) {
		if (strcmp(s, t->keyword) == 0) {
			*val |= t->val;
			return (1);
		}
	}
	return (0);
}

void
report_tls(struct tls * tls_ctx, char * host, char *tlsexpectname)
{
	time_t t;
	const char *ocsp_url;

	fprintf(stderr, "TLS handshake negotiated %s/%s with host %s\n",
	    tls_conn_version(tls_ctx), tls_conn_cipher(tls_ctx), host);
	fprintf(stderr, "Peer name: %s\n",
	    tlsexpectname ? tlsexpectname : host);
	if (tls_peer_cert_subject(tls_ctx))
		fprintf(stderr, "Subject: %s\n",
		    tls_peer_cert_subject(tls_ctx));
	if (tls_peer_cert_issuer(tls_ctx))
		fprintf(stderr, "Issuer: %s\n",
		    tls_peer_cert_issuer(tls_ctx));
	if ((t = tls_peer_cert_notbefore(tls_ctx)) != -1)
		fprintf(stderr, "Valid From: %s", ctime(&t));
	if ((t = tls_peer_cert_notafter(tls_ctx)) != -1)
		fprintf(stderr, "Valid Until: %s", ctime(&t));
	if (tls_peer_cert_hash(tls_ctx))
		fprintf(stderr, "Cert Hash: %s\n",
		    tls_peer_cert_hash(tls_ctx));
	ocsp_url = tls_peer_ocsp_url(tls_ctx);
	if (ocsp_url != NULL)
		fprintf(stderr, "OCSP URL: %s\n", ocsp_url);
	switch (tls_peer_ocsp_response_status(tls_ctx)) {
	case TLS_OCSP_RESPONSE_SUCCESSFUL:
		fprintf(stderr, "OCSP Stapling: %s\n",
		    tls_peer_ocsp_result(tls_ctx) == NULL ?  "" :
		    tls_peer_ocsp_result(tls_ctx));
		fprintf(stderr,
		    "  response_status=%d cert_status=%d crl_reason=%d\n",
		    tls_peer_ocsp_response_status(tls_ctx),
		    tls_peer_ocsp_cert_status(tls_ctx),
		    tls_peer_ocsp_crl_reason(tls_ctx));
		t = tls_peer_ocsp_this_update(tls_ctx);
		fprintf(stderr, "  this update: %s",
		    t != -1 ? ctime(&t) : "\n");
		t =  tls_peer_ocsp_next_update(tls_ctx);
		fprintf(stderr, "  next update: %s",
		    t != -1 ? ctime(&t) : "\n");
		t =  tls_peer_ocsp_revocation_time(tls_ctx);
		fprintf(stderr, "  revocation: %s",
		    t != -1 ? ctime(&t) : "\n");
		break;
	case -1:
		break;
	default:
		fprintf(stderr, "OCSP Stapling:  failure - response_status %d (%s)\n",
		    tls_peer_ocsp_response_status(tls_ctx),
		    tls_peer_ocsp_result(tls_ctx) == NULL ?  "" :
		    tls_peer_ocsp_result(tls_ctx));
		break;

	}
}
#endif

void
report_connect(const struct sockaddr *sa, socklen_t salen, char *path)
{
	char remote_host[NI_MAXHOST];
	char remote_port[NI_MAXSERV];
	int herr;
	int flags = NI_NUMERICSERV;

	if (path != NULL) {
		fprintf(stderr, "Connection on %s received!\n", path);
		return;
	}

	if (nflag)
		flags |= NI_NUMERICHOST;

	if ((herr = getnameinfo(sa, salen,
	    remote_host, sizeof(remote_host),
	    remote_port, sizeof(remote_port),
	    flags)) != 0) {
		if (herr == EAI_SYSTEM)
			err(1, "getnameinfo");
		else
			errx(1, "getnameinfo: %s", gai_strerror(herr));
	}

	fprintf(stderr,
	    "Connection from %s %s "
	    "received!\n", remote_host, remote_port);
}

void
help(void)
{
	usage(0);
	fprintf(stderr, "\tCommand Summary:\n"

	"\t-4		Use IPv4\n"
	"\t-6		Use IPv6\n"
#ifdef CRYPTO
	"\t-C certfile	Public key file\n"
	"\t-c		Use TLS\n"
#endif
	"\t-D		Enable the debug socket option\n"
	"\t-d		Detach from stdin\n"
#ifdef CRYPTO
	"\t-e name\t	Required name in peer certificate\n"
#endif
	"\t-F		Pass socket fd\n"
#ifdef CRYPTO
	"\t-H hash\t	Hash string of peer certificate\n"
#endif
	"\t-h		This help text\n"
	"\t-I length	TCP receive buffer length\n"
	"\t-i interval	Delay interval for lines sent, ports scanned\n"
#ifdef CRYPTO
	"\t-K keyfile	Private key file\n"
#endif
	"\t-k		Keep inbound sockets open for multiple connects\n"
	"\t-l		Listen mode, for inbound connects\n"
	"\t-M ttl		Outgoing TTL / Hop Limit\n"
	"\t-m minttl	Minimum incoming TTL / Hop Limit\n"
	"\t-N		Shutdown the network socket after EOF on stdin\n"
	"\t-n		Suppress name/port resolutions\n"
	"\t-O length	TCP send buffer length\n"
#ifdef CRYPTO
	"\t-o staplefile	Staple file\n"
#endif
	"\t-P proxyuser\tUsername for proxy authentication\n"
	"\t-p port\t	Specify local port for remote connects\n"
#ifdef CRYPTO
	"\t-R CAfile	CA bundle\n"
#endif
	"\t-r		Randomize remote ports\n"
	"\t-S		Enable the TCP MD5 signature option\n"
	"\t-s source	Local source address\n"
#ifdef CRYPTO
	"\t-T keyword	TOS value or TLS options\n"
#endif
	"\t-t		Answer TELNET negotiation\n"
	"\t-U		Use UNIX domain socket\n"
	"\t-u		UDP mode\n"
#ifdef __OpenBSD__
	"\t-V rtable	Specify alternate routing table\n"
#endif
	"\t-v		Verbose\n"
	"\t-w timeout	Timeout for connects and final net reads\n"
	"\t-X proto	Proxy protocol: \"4\", \"5\" (SOCKS) or \"connect\"\n"
	"\t-x addr[:port]\tSpecify proxy address and port\n"
	"\t-z		Zero-I/O mode [used for scanning]\n"
	"Port numbers can be individual or ranges: lo-hi [inclusive]\n");
	exit(1);
}

void
usage(int ret)
{
	fprintf(stderr,
	    "Usage: %s [-46%sDdFhklNnrStUuvz] [-e name] [-I length]\n"
#ifdef CRYPTO
	    "\t  [-C certfile] [-H hash] [-K keyfile] [-R CAfile] "
	    "[-T keyword] [-o staplefile]\n"
#endif
	    "\t  [-i interval] [-M ttl] [-m minttl] [-O length]\n"
	    "\t  [-P proxy_username] [-p source_port]\n"
	    "\t  [-s source] "
#ifdef __OpenBSD__
	    "[-V rtable] "
#endif
	    "[-w timeout] [-X proxy_protocol]\n"
	    "\t  [-x proxy_address[:port]] [destination] [port]\n",
	    getprogname(),
#ifdef CRYPTO
	    "c"
#else
	    ""
#endif
	);
	if (ret)
		exit(1);
}
