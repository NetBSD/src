/*	$NetBSD: bozohttpd.c,v 1.14 2009/05/23 02:26:03 mrg Exp $	*/

/*	$eterna: bozohttpd.c,v 1.159 2009/05/23 02:14:30 mrg Exp $	*/

/*
 * Copyright (c) 1997-2009 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer and
 *    dedication in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* this program is dedicated to the Great God of Processed Cheese */

/*
 * bozohttpd.c:  minimal httpd; provides only these features:
 *	- HTTP/0.9 (by virtue of ..)
 *	- HTTP/1.0
 *	- HTTP/1.1
 *	- CGI/1.1 this will only be provided for "system" scripts
 *	- automatic "missing trailing slash" redirections
 *	- configurable translation of /~user/ to ~user/public_html,
 *	  however, this does not include cgi-bin support
 *	- access lists via libwrap via inetd/tcpd
 *	- virtual hosting
 *	- not that we do not even pretend to understand MIME, but
 *	  rely only on the HTTP specification
 *	- ipv6 support
 *	- automatic `index.html' generation
 *	- configurable server name
 *	- directory index generation
 *	- daemon mode (lacks libwrap support)
 *	- .htpasswd support
 */

/*
 * requirements for minimal http/1.1 (at least, as documented in
 * <draft-ietf-http-v11-spec-rev-06> which expired may 18, 1999):
 *
 *	- 14.15: content-encoding handling. [1]
 *
 *	- 14.16: content-length handling.  this is only a SHOULD header
 *	  thus we could just not send it ever.  [1]
 *
 *	- 14.17: content-type handling. [1]
 *
 *	- 14.25/28: if-{,un}modified-since handling.  maybe do this, but
 *	  i really don't want to have to parse 3 differnet date formats
 *
 * [1] need to revisit to ensure proper behaviour
 *
 * and the following is a list of features that we do not need
 * to have due to other limits, or are too lazy.  there are more
 * of these than are listed, but these are of particular note,
 * and could perhaps be implemented.
 *
 *	- 3.5/3.6: content/transfer codings.  probably can ignore
 *	  this?  we "SHOULD"n't.  but 4.4 says we should ignore a
 *	  `content-length' header upon reciept of a `transfer-encoding'
 *	  header.
 *
 *	- 5.1.1: request methods.  only MUST support GET and HEAD,
 *	  but there are new ones besides POST that are currently
 *	  supported: OPTIONS PUT DELETE TRACE and CONNECT, plus
 *	  extensions not yet known?
 *
 * 	- 10.1: we can ignore informational status codes
 *
 *	- 10.3.3/10.3.4/10.3.8:  just use '302' codes always.
 *
 *	- 14.1/14.2/14.3/14.27: we do not support Accept: headers..
 *	  just ignore them and send the request anyway.  they are
 *	  only SHOULD.
 *
 *	- 14.5/14.16/14.35: we don't do ranges.  from section 14.35.2
 *	  `A server MAY ignore the Range header'.  but it might be nice.
 *	  since 20080301 we support simple range headers.
 *
 *	- 14.9: we aren't a cache.
 *
 *	- 14.15: content-md5 would be nice...
 *	
 *	- 14.24/14.26/14.27: be nice to support this...
 *
 *	- 14.44: not sure about this Vary: header.  ignore it for now.
 */

#ifndef INDEX_HTML
#define INDEX_HTML		"index.html"
#endif
#ifndef SERVER_SOFTWARE
#define SERVER_SOFTWARE		"bozohttpd/20090522"
#endif
#ifndef DIRECT_ACCESS_FILE
#define DIRECT_ACCESS_FILE	".bzdirect"
#endif
#ifndef REDIRECT_FILE
#define REDIRECT_FILE		".bzredirect"
#endif
#ifndef ABSREDIRECT_FILE
#define ABSREDIRECT_FILE	".bzabsredirect"
#endif

/*
 * And so it begins ..
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#ifndef __attribute__
#define __attribute__(x)
#endif /* __attribute__ */

#include "bozohttpd.h"

#ifndef MAX_WAIT_TIME
#define	MAX_WAIT_TIME	60	/* hang around for 60 seconds max */
#endif

/* variables and functions */

	int	bflag;		/* background; drop into daemon mode */
	int	fflag;		/* keep daemon mode in foreground */
static	int	eflag;		/* don't clean environ; -t/-U only */
	const char *Iflag = "http";/* bind port; default "http" */
	int	Iflag_set;
	int	dflag = 0;	/* debugging level */
	char	*myname;	/* my name */

#ifndef LOG_FTP
#define LOG_FTP LOG_DAEMON
#endif

static	char	*tflag;		/* root directory */
static	char 	*Uflag;		/* user name to switch to */
static	int	Vflag;		/* unknown vhosts go to normal slashdir */
static	int	nflag;		/* avoid gethostby*() */
static	int	rflag;		/* make sure referrer = me unless url = / */
static	int	sflag;		/* log to stderr even if it is not a TTY */
static	char	*vpath;		/* virtual directory base */

	size_t	page_size;	/* page size */
	char	*slashdir;	/* www slash directory */

	const char *server_software = SERVER_SOFTWARE;
	const char *index_html = INDEX_HTML;
	const char http_09[] = "HTTP/0.9";
	const char http_10[] = "HTTP/1.0";
	const char http_11[] = "HTTP/1.1";
	const char text_plain[]	= "text/plain";

static	void	usage(void);
static	void	alarmer(int);
volatile sig_atomic_t	alarmhit;

static	void	parse_request(char *, char **, char **, char **, char **);
static	void	clean_request(http_req *request);
static	http_req *read_request(void);
static	struct headers *addmerge_header(http_req *, char *, char *, ssize_t);
static int mmap_and_write_part(int, off_t, size_t);
static	void	process_request(http_req *);
static	int	check_direct_access(http_req *request);
static	int	transform_request(http_req *, int *);
static	void	handle_redirect(http_req *, const char *, int);

static	int	check_virtual(http_req *);
static	void	check_bzredirect(http_req *);
static	void	fix_url_percent(http_req *);
static	int	process_proto(http_req *, const char *);
static	int	process_method(http_req *, const char *);
static	void	escape_html(http_req *);

static	const char *http_errors_short(int);
static	const char *http_errors_long(int);


/* bozotic io */
int	(*bozoprintf)(const char *, ...) = printf;
ssize_t	(*bozoread)(int, void *, size_t) = read;
ssize_t	(*bozowrite)(int, const void *, size_t) = write;
int	(*bozoflush)(FILE *) = fflush;

	char	*progname;

	int	main(int, char **);

static void
usage(void)
{
	warning("usage: %s [options] slashdir [myname]", progname);
	warning("options:");
#ifdef DEBUG
	warning("   -d\t\t\tenable debug support");
#endif
	warning("   -s\t\t\talways log to stderr");
#ifndef NO_USER_SUPPORT
	warning("   -u\t\t\tenable ~user/public_html support");
	warning("   -p dir\t\tchange `public_html' directory name]");
#endif
#ifndef NO_DYNAMIC_CONTENT
	warning("   -M arg t c c11\tadd this mime extenstion");
#endif
#ifndef NO_CGIBIN_SUPPORT
#ifndef NO_DYNAMIC_CONTENT
	warning("   -C arg prog\t\tadd this CGI handler");
#endif
	warning("   -c cgibin\t\tenable cgi-bin support in this directory");
#endif
#ifndef NO_DAEMON_MODE
	warning("   -b\t\t\tbackground and go into daemon mode");
	warning("   -f\t\t\tkeep daemon mode in the foreground");
	warning("   -i address\t\tbind on this address (daemon mode only)");
	warning("   -I port\t\tbind on this port (daemon mode only)");
#endif
	warning("   -S version\t\tset server version string");
	warning("   -t dir\t\tchroot to `dir'");
	warning("   -U username\t\tchange user to `user'");
	warning("   -e\t\t\tdon't clean the environment (-t and -U only)");
	warning("   -v virtualroot\tenable virtual host support in this directory");
	warning("   -r\t\t\tmake sure sub-pages come from this host via referrer");
#ifndef NO_DIRINDEX_SUPPORT
	warning("   -X\t\t\tenable automatic directory index support");
	warning("   -H\t\t\thide files starting with a period (.) in index mode");
#endif
	warning("   -x index\t\tchange default `index.html' file name");
#ifndef NO_SSL_SUPPORT
	warning("   -Z cert privkey\tspecify path to server certificate and private key file\n"
		"\t\t\tin pem format and enable bozohttpd in SSL mode");
#endif /* NO_SSL_SUPPORT */
	error(1, "%s failed to start", progname);
}

int
main(int argc, char **argv)
{
	http_req *request;
	extern	char **environ;
	char	*cleanenv[1];
	uid_t	uid;
	int	c;

	uid = 0;	/* XXX gcc */

	if ((progname = strrchr(argv[0], '/')) != NULL)
		progname++;
	else
		progname = argv[0];

	openlog(progname, LOG_PID|LOG_NDELAY, LOG_FTP);

	while ((c = getopt(argc, argv,
			   "C:HI:M:S:U:VXZ:bc:defhi:np:rst:uv:x:z:")) != -1) {
		switch(c) {

		case 'M':
#ifndef NO_DYNAMIC_CONTENT
			/* make sure there's four arguments */
			if (argc - optind < 3)
				usage();
			add_content_map_mime(optarg, argv[optind],
			    argv[optind+1], argv[optind+2]);
			optind += 3;
			break;
#else
			error(1, "dynmic mime content support is not enabled");
			/* NOTREACHED */
#endif /* NO_DYNAMIC_CONTENT */

		case 'n':
			nflag = 1;
			break;

		case 'r':
			rflag = 1;
			break;

		case 's':
			sflag = 1;
			break;

		case 'S':
			server_software = optarg;
			break;
		case 'Z':
#ifndef NO_SSL_SUPPORT
			/* make sure there's two arguments */
			if (argc - optind < 1)
				usage();
			ssl_set_opts(optarg, argv[optind++]);
			break;
#else
			error(1, "ssl support is not enabled");
			/* NOT REACHED */
#endif /* NO_SSL_SUPPORT */
		case 'U':
			Uflag = optarg;
			break;

		case 'V':
			Vflag = 1;
			break;

		case 'v':
			vpath = optarg;
			break;

		case 'x':
			index_html = optarg;
			break;

#ifndef NO_DAEMON_MODE
		case 'b':
			/*
			 * test suite support - undocumented
			 * bflag == 2 (aka, -b -b) means to
			 * only process 1 per kid
			 */
			if (bflag)
				bflag = 2;
			else
				bflag = 1;
			break;

		case 'e':
			eflag = 1;
			break;

		case 'f':
			fflag = 1;
			break;

		case 'i':
			iflag = optarg;
			break;

		case 'I':
			Iflag_set = 1;
			Iflag = optarg;
			break;
#else /* NO_DAEMON_MODE */
		case 'b':
		case 'e':
		case 'f':
		case 'i':
		case 'I':
			error(1, "Daemon mode is not enabled");
			/* NOTREACHED */
#endif /* NO_DAEMON_MODE */

#ifndef NO_CGIBIN_SUPPORT
		case 'c':
			set_cgibin(optarg);
			break;

		case 'C':
#ifndef NO_DYNAMIC_CONTENT
			/* make sure there's two arguments */
			if (argc - optind < 1)
				usage();
			add_content_map_cgi(optarg, argv[optind++]);
			break;
#else
			error(1, "dynmic CGI handler support is not enabled");
			/* NOTREACHED */
#endif /* NO_DYNAMIC_CONTENT */

#else
		case 'c':
		case 'C':
			error(1, "CGI is not enabled");
			/* NOTREACHED */
#endif /* NO_CGIBIN_SUPPORT */

		case 'd':
			dflag++;
#ifndef DEBUG
			if (dflag == 1)
				warning("Debugging is not enabled");
#endif /* !DEBUG */
			break;

#ifndef NO_USER_SUPPORT
		case 'p':
			public_html = optarg;
			break;

		case 't':
			tflag = optarg;
			break;

		case 'u':
			uflag = 1;
			break;
#else
		case 'p':
		case 't':
		case 'u':
			error(1, "User support is not enabled");
			/* NOTREACHED */
#endif /* NO_USER_SUPPORT */

#ifndef NO_DIRINDEX_SUPPORT
		case 'H':
			Hflag = 1;
			break;

		case 'X':
			Xflag = 1;
			break;

#else
		case 'H':
		case 'X':
			error(1, "directory indexing is not enabled");
			/* NOTREACHED */
#endif /* NO_DIRINDEX_SUPPORT */

		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 1) {
		myname = bozomalloc(MAXHOSTNAMELEN+1);
		/* XXX we do not check for FQDN here */
		if (gethostname(myname, MAXHOSTNAMELEN+1) < 0)
			error(1, "gethostname");
		myname[MAXHOSTNAMELEN] = '\0';
	} else if (argc == 2)
		myname = argv[1];
	else
		usage();
	
	slashdir = argv[0];
	debug((DEBUG_OBESE, "myname is %s, slashdir is %s", myname, slashdir));

#ifdef _SC_PAGESIZE
	page_size = (long)sysconf(_SC_PAGESIZE);
#else
	page_size = 4096;
#endif

	/*
	 * initialise ssl and daemon mode if necessary.
	 */
	ssl_init();
	daemon_init();

	/*
	 * prevent info leakage between different compartments.
	 * some PATH values in the environment would be invalided
	 * by chroot. cross-user settings might result in undesirable
	 * effects.
	 */
	if ((tflag != NULL || Uflag != NULL) && !eflag) {
		cleanenv[0] = NULL;
		environ = cleanenv;
	}
	
	/*
	 * look up user/group information.
	 */
	if (Uflag != NULL) {
		struct passwd *pw;

		if ((pw = getpwnam(Uflag)) == NULL)
			error(1, "getpwnam(%s): %s", Uflag, strerror(errno));
		if (initgroups(pw->pw_name, pw->pw_gid) == -1)
			error(1, "initgroups: %s", strerror(errno));
		if (setgid(pw->pw_gid) == -1)
			error(1, "setgid(%u): %s", pw->pw_gid, strerror(errno));
		uid = pw->pw_uid;
	}

	/*
	 * handle chroot.
	 */
	if (tflag != NULL) {
		if (chdir(tflag) == -1)
			error(1, "chdir(%s): %s", tflag, strerror(errno));
		if (chroot(tflag) == -1)
			error(1, "chroot(%s): %s", tflag, strerror(errno));
	}

	if (Uflag != NULL)
		if (setuid(uid) == -1)
			error(1, "setuid(%d): %s", uid, strerror(errno));

	/*
	 * read and process the HTTP request.
	 */
	do {
		request = read_request();
		if (request) {
			process_request(request);
			clean_request(request);
		}
	} while (bflag);

	return (0);
}

char *
http_date(void)
{
	static	char date[40];
	struct	tm *tm;
	time_t	now;

	/* Sun, 06 Nov 1994 08:49:37 GMT */
	now = time(NULL);
	tm = gmtime(&now);	/* HTTP/1.1 spec rev 06 sez GMT only */
	strftime(date, sizeof date, "%a, %d %b %Y %H:%M:%S GMT", tm);
	return date;
}

/*
 * convert "in" into the three parts of a request (first line).
 * we allocate into file and query, but return pointers into
 * "in" for proto and method.
 */
static void
parse_request(char *in, char **method, char **file, char **query, char **proto)
{
	ssize_t	len;
	char	*val;
	
	debug((DEBUG_EXPLODING, "parse in: %s", in));
	*method = *file = *query = *proto = NULL;

	len = (ssize_t)strlen(in);
	val = bozostrnsep(&in, " \t\n\r", &len);
	if (len < 1 || val == NULL)
		return;
	*method = val;

	while (*in == ' ' || *in == '\t')
		in++;
	val = bozostrnsep(&in, " \t\n\r", &len);
	if (len < 1) {
		if (len == 0)
			*file = val;
		else
			*file = in;
		return;
	}
	*file = val;

	*query = strchr(*file, '?');
	if (*query)
		*(*query)++ = '\0';

	if (in) {
		while (*in && (*in == ' ' || *in == '\t'))
			in++;
		if (*in)
			*proto = in;
	}

	/* allocate private copies */
	*file = strdup(*file);
	if (*query)
		*query = strdup(*query);

	debug((DEBUG_FAT, "url: method: \"%s\" file: \"%s\" query: \"%s\" proto: \"%s\"", 
	       *method, *file, *query, *proto));
}

/*
 * send a HTTP/1.1 408 response if we timeout.
 */
/* ARGSUSED */
static void
alarmer(int sig)
{
	alarmhit = 1;
}

/*
 * cleanup a http_req after use
 */
static void
clean_request(http_req *request)
{
	struct	headers *hdr, *ohdr = NULL;

	if (request == NULL)
		return;

	/* clean up request */
#define MF(x)	if (request->x) free(request->x)
	MF(hr_remotehost);
	MF(hr_remoteaddr);
	MF(hr_serverport);
	MF(hr_file);
	MF(hr_query);
#undef MF
	auth_cleanup(request);
	for (hdr = SIMPLEQ_FIRST(&request->hr_headers); hdr;
	    hdr = SIMPLEQ_NEXT(hdr, h_next)) {
		free(hdr->h_value);
		free(hdr->h_header);
		if (ohdr)
			free(ohdr);
		ohdr = hdr;
	}
	if (ohdr)
		free(ohdr);

	free(request);
}

/*
 * This function reads a http request from stdin, returning a pointer to a
 * http_req structure, describing the request.
 */
static http_req *
read_request(void)
{
	struct	sigaction	sa;
	char	*str, *val, *method, *file, *proto, *query;
	char	*host, *addr, *port;
	char	bufport[10];
	char	hbuf[NI_MAXHOST], abuf[NI_MAXHOST];
	struct	sockaddr_storage ss;
	ssize_t	len;
	int	line = 0;
	socklen_t slen;
	http_req *request;

	/*
	 * if we're in daemon mode, daemon_fork() will return here once
	 * for each child, then we can setup SSL.
	 */
	daemon_fork();
	ssl_accept();

	request = bozomalloc(sizeof *request);
	memset(request, 0, sizeof *request);
	request->hr_allow = request->hr_host = NULL;
	request->hr_content_type = request->hr_content_length = NULL;
	request->hr_range = NULL;
	request->hr_last_byte_pos = -1;
	request->hr_if_modified_since = NULL;
	request->hr_file = NULL;

	slen = sizeof(ss);
	if (getpeername(0, (struct sockaddr *)&ss, &slen) < 0)
		host = addr = NULL;
	else {
		if (getnameinfo((struct sockaddr *)&ss, slen,
		    abuf, sizeof abuf, NULL, 0, NI_NUMERICHOST) == 0)
			addr = abuf;
		else
			addr = NULL;
		if (nflag == 0 && getnameinfo((struct sockaddr *)&ss, slen,
		    hbuf, sizeof hbuf, NULL, 0, 0) == 0)
			host = hbuf;
		else
			host = NULL;
	}
	if (host != NULL)
		request->hr_remotehost = bozostrdup(host);
	if (addr != NULL)
		request->hr_remoteaddr = bozostrdup(addr);
	slen = sizeof(ss);
	if (getsockname(0, (struct sockaddr *)&ss, &slen) < 0)
		port = NULL;
	else {
		if (getnameinfo((struct sockaddr *)&ss, slen, NULL, 0,
		    bufport, sizeof bufport, NI_NUMERICSERV) == 0)
			port = bufport;
		else
			port = NULL;
	}
	if (port != NULL)
		request->hr_serverport = bozostrdup(port);

	/*
	 * setup a timer to make sure the request is not hung
	 */
	sa.sa_handler = alarmer;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGALRM);
	sa.sa_flags = 0;
	sigaction(SIGALRM, &sa, NULL);	/* XXX */

	alarm(MAX_WAIT_TIME);
	while ((str = bozodgetln(STDIN_FILENO, &len, bozoread)) != NULL) {
		alarm(0);
		if (alarmhit) {
			(void)http_error(408, NULL, "request timed out");
			goto cleanup;
		}
		line++;

		if (line == 1) {

			if (len < 1) {
				(void)http_error(404, NULL, "null method");
				goto cleanup;
			}

			warning("got request ``%s'' from host %s to port %s",
			    str,
			    host ? host : addr ? addr : "<local>",
			    port ? port : "<stdin>");
#if 0
			debug((DEBUG_FAT, "read_req, getting request: ``%s''",
			    str));
#endif

			/* we allocate return space in file and query only */
			parse_request(str, &method, &file, &query, &proto);
			request->hr_file = file;
			request->hr_query = query;
			if (method == NULL) {
				(void)http_error(404, NULL, "null method");
				goto cleanup;
			}
			if (file == NULL) {
				(void)http_error(404, NULL, "null file");
				goto cleanup;
			}

			/*
			 * note that we parse the proto first, so that we
			 * can more properly parse the method and the url.
			 */

			if (process_proto(request, proto) ||
			    process_method(request, method)) {
				goto cleanup;
			}

			debug((DEBUG_FAT, "got file \"%s\" query \"%s\"",
			    request->hr_file,
			    request->hr_query ? request->hr_query : "<none>"));

			/* http/0.9 has no header processing */
			if (request->hr_proto == http_09)
				break;
		} else {		/* incoming headers */
			struct	headers *hdr;

			if (*str == '\0')
				break;

			val = bozostrnsep(&str, ":", &len);
			debug((DEBUG_EXPLODING,
			    "read_req2: after bozostrnsep: str ``%s'' val ``%s''",
			    str, val));
			if (val == NULL || len == -1) {
				(void)http_error(404, request, "no header");
				goto cleanup;
			}
			while (*str == ' ' || *str == '\t')
				len--, str++;
			while (*val == ' ' || *val == '\t')
				val++;

			if (auth_check_headers(request, val, str, len))
				goto next_header;

			hdr = addmerge_header(request, val, str, len);

			if (strcasecmp(hdr->h_header, "content-type") == 0)
				request->hr_content_type = hdr->h_value;
			else if (strcasecmp(hdr->h_header, "content-length") == 0)
				request->hr_content_length = hdr->h_value;
			else if (strcasecmp(hdr->h_header, "host") == 0)
				request->hr_host = hdr->h_value;
			/* HTTP/1.1 rev06 draft spec: 14.20 */
			else if (strcasecmp(hdr->h_header, "expect") == 0) {
				(void)http_error(417, request, "we don't support Expect:");
				goto cleanup;
			}
			else if (strcasecmp(hdr->h_header, "referrer") == 0 ||
			         strcasecmp(hdr->h_header, "referer") == 0)
				request->hr_referrer = hdr->h_value;
			else if (strcasecmp(hdr->h_header, "range") == 0)
				request->hr_range = hdr->h_value;
			else if (strcasecmp(hdr->h_header, "if-modified-since") == 0)
				request->hr_if_modified_since = hdr->h_value;

			debug((DEBUG_FAT, "adding header %s: %s",
			    hdr->h_header, hdr->h_value));
		}
next_header:
		alarm(MAX_WAIT_TIME);
	}

	/* now, clear it all out */
	alarm(0);
	signal(SIGALRM, SIG_DFL);

	/* RFC1945, 8.3 */
	if (request->hr_method == HTTP_POST && request->hr_content_length == NULL) {
		(void)http_error(400, request, "missing content length");
		goto cleanup;
	}

	/* HTTP/1.1 draft rev-06, 14.23 & 19.6.1.1 */
	if (request->hr_proto == http_11 && request->hr_host == NULL) {
		(void)http_error(400, request, "missing Host header");
		goto cleanup;
	}

	if (request->hr_range != NULL) {
		debug((DEBUG_FAT, "hr_range: %s", request->hr_range));
		/* support only simple ranges %d- and %d-%d */
		if (strchr(request->hr_range, ',') == NULL) {
			const char *rstart, *dash;

			rstart = strchr(request->hr_range, '=');
			if (rstart != NULL) {
				rstart++;
				dash = strchr(rstart, '-');
				if (dash != NULL && dash != rstart) {
					dash++;
					request->hr_have_range = 1;
					request->hr_first_byte_pos =
					    strtoll(rstart, NULL, 10);
					if (request->hr_first_byte_pos < 0)
						request->hr_first_byte_pos = 0;
					if (*dash != '\0') {
						request->hr_last_byte_pos =
						    strtoll(dash, NULL, 10);
						if (request->hr_last_byte_pos < 0)
							request->hr_last_byte_pos = -1;
					}
				}
			}
		}
	}

	debug((DEBUG_FAT, "read_request returns url %s in request", 
	       request->hr_file));
	return (request);

cleanup:
	clean_request(request);

	/* If SSL enabled cleanup SSL structure. */
	ssl_destroy();

	return NULL;
}

/*
 * add or merge this header (val: str) into the requests list
 */
static struct headers *
addmerge_header(http_req *request, char *val, char *str, ssize_t len)
{
	struct	headers *hdr;

	/* do we exist already? */
	SIMPLEQ_FOREACH(hdr, &request->hr_headers, h_next) {
		if (strcasecmp(val, hdr->h_header) == 0)
			break;
	}

	if (hdr) {
		/* yup, merge it in */
		char *nval;

		if (asprintf(&nval, "%s, %s", hdr->h_value, str) == -1) {
			(void)http_error(500, NULL,
			     "memory allocation failure");
			return NULL;
		}
		free(hdr->h_value);
		hdr->h_value = nval;
	} else {
		/* nope, create a new one */

		hdr = bozomalloc(sizeof *hdr);
		hdr->h_header = bozostrdup(val);
		if (str && *str)
			hdr->h_value = bozostrdup(str);
		else
			hdr->h_value = bozostrdup(" ");

		SIMPLEQ_INSERT_TAIL(&request->hr_headers, hdr, h_next);
		request->hr_nheaders++;
	}

	return hdr;
}

static int
mmap_and_write_part(int fd, off_t first_byte_pos, size_t sz)
{
	size_t mappedsz, wroffset;
	off_t mappedoffset;
	char *addr;
	void *mappedaddr;

	/*
	 * we need to ensure that both the size *and* offset arguments to
	 * mmap() are page-aligned.  our formala for this is:
	 *
	 *    input offset: first_byte_pos
	 *    input size: sz
	 *
	 *    mapped offset = page align truncate (input offset)
	 *    mapped size   =
	 *        page align extend (input offset - mapped offset + input size)
	 *    write offset  = input offset - mapped offset
	 *
	 * we use the write offset in all writes
	 */
	mappedoffset = first_byte_pos & ~(page_size - 1);
	mappedsz = (first_byte_pos - mappedoffset + sz + page_size - 1) &
		  ~(page_size - 1);
	wroffset = first_byte_pos - mappedoffset;

	addr = mmap(0, mappedsz, PROT_READ, MAP_SHARED, fd, mappedoffset);
	if (addr == (char *)-1) {
		warning("mmap failed: %s", strerror(errno));
		return -1;
	}
	mappedaddr = addr;

#ifdef MADV_SEQUENTIAL
	(void)madvise(addr, sz, MADV_SEQUENTIAL);
#endif
	while (sz > WRSZ) {
		if (bozowrite(STDOUT_FILENO, addr + wroffset, WRSZ) != WRSZ) {
			warning("write failed: %s", strerror(errno));
			goto out;
		}
		debug((DEBUG_OBESE, "wrote %d bytes", WRSZ));
		sz -= WRSZ;
		addr += WRSZ;
	}
	if (sz && (size_t)bozowrite(STDOUT_FILENO, addr + wroffset, sz) != sz) {
		warning("final write failed: %s", strerror(errno));
		goto out;
	}
	debug((DEBUG_OBESE, "wrote %d bytes", (int)sz));
 out:
	if (munmap(mappedaddr, mappedsz) < 0) {
		warning("munmap failed");
		return -1;
	}

	return 0;
}

static int
parse_http_date(const char *val, time_t *timestamp)
{
	char *remainder;
	struct tm tm;

	if ((remainder = strptime(val, "%a, %d %b %Y %T GMT", &tm)) == NULL &&
	    (remainder = strptime(val, "%a, %d-%b-%y %T GMT", &tm)) == NULL &&
	    (remainder = strptime(val, "%a %b %d %T %Y", &tm)) == NULL)
		return 0; /* Invalid HTTP date format */

	if (*remainder)
		return 0; /* No trailing garbage */

	*timestamp = timegm(&tm);
	return 1;
}

/*
 * process_request does the following:
 *	- check the request is valid
 *	- process cgi-bin if necessary
 *	- transform a filename if necesarry
 *	- return the HTTP request
 */
static void
process_request(http_req *request)
{
	struct	stat sb;
	time_t timestamp;
	char	*file;
	const char *type, *encoding;
	int	fd, isindex;

	/*
	 * note that transform_request chdir()'s if required.  also note
	 * that cgi is handed here.  if transform_request() returns 0
	 * then the request has been handled already.
	 */
	if (transform_request(request, &isindex) == 0)
		return;

	file = request->hr_file;

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		debug((DEBUG_FAT, "open failed: %s", strerror(errno)));
		if (errno == EPERM)
			(void)http_error(403, request, "no permission to open file");
		else if (errno == ENOENT) {
			if (directory_index(request, file, isindex)) 
				;
			else
				(void)http_error(404, request, "no file");
		} else
			(void)http_error(500, request, "open file");
		goto cleanup_nofd;
	}
	if (fstat(fd, &sb) < 0) {
		(void)http_error(500, request, "can't fstat");
		goto cleanup;
	}
	if (S_ISDIR(sb.st_mode)) {
		handle_redirect(request, NULL, 0);
		goto cleanup;
	}

	if (request->hr_if_modified_since &&
	    parse_http_date(request->hr_if_modified_since, &timestamp) &&
	    timestamp >= sb.st_mtime) {
		/* XXX ignore subsecond of timestamp */
		bozoprintf("%s 304 Not Modified\r\n", request->hr_proto);
		bozoprintf("\r\n");
		bozoflush(stdout);
		goto cleanup;
	}

	/* validate requested range */
	if (request->hr_last_byte_pos == -1 ||
	    request->hr_last_byte_pos >= sb.st_size)
		request->hr_last_byte_pos = sb.st_size - 1;
	if (request->hr_have_range &&
	    request->hr_first_byte_pos > request->hr_last_byte_pos) {
		request->hr_have_range = 0;	/* punt */
		request->hr_first_byte_pos = 0;
		request->hr_last_byte_pos = sb.st_size - 1;
	}
	debug((DEBUG_FAT, "have_range %d first_pos %qd last_pos %qd",
	    request->hr_have_range,
	    request->hr_first_byte_pos, request->hr_last_byte_pos));
	if (request->hr_have_range)
		bozoprintf("%s 206 Partial Content\r\n", request->hr_proto);
	else
		bozoprintf("%s 200 OK\r\n", request->hr_proto);

	if (request->hr_proto != http_09) {
		type = content_type(request, file);
		encoding = content_encoding(request, file);

		print_header(request, &sb, type, encoding);
		bozoprintf("\r\n");
	}
	bozoflush(stdout);

	if (request->hr_method != HTTP_HEAD) {
		off_t szleft, cur_byte_pos;
		static size_t mmapsz = MMAPSZ;

		szleft =
		     request->hr_last_byte_pos - request->hr_first_byte_pos + 1;
		cur_byte_pos = request->hr_first_byte_pos;

 retry:
		while (szleft) {
			size_t sz;

			/* This should take care of the first unaligned chunk */
			if ((cur_byte_pos & (page_size - 1)) != 0)
				sz = cur_byte_pos & ~page_size;
			if (mmapsz < szleft)
				sz = mmapsz;
			else
				sz = szleft;
			if (mmap_and_write_part(fd, cur_byte_pos, sz)) {
				if (errno == ENOMEM) {
					mmapsz /= 2;
					if (mmapsz >= page_size)
						goto retry;
				}
				goto cleanup;
			}
			cur_byte_pos += sz;
			szleft -= sz;
		}
	}
 cleanup:
	close(fd);
 cleanup_nofd:
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	/*close(STDERR_FILENO);*/
}

/*
 * deal with virtual host names; we do this:
 *	if we have a virtual path root (vpath), and we are given a
 *	virtual host spec (Host: ho.st or http://ho.st/), see if this
 *	directory exists under vpath.  if it does, use this as the
 #	new slashdir.
 */
static int
check_virtual(http_req *request)
{
	char *file = request->hr_file, *s;
	struct dirent **list;
	size_t len;
	int i;

	if (!vpath)
		goto use_slashdir;

	/*
	 * convert http://virtual.host/ to request->hr_host
	 */
	debug((DEBUG_OBESE, "checking for http:// virtual host in ``%s''", file));
	if (strncasecmp(file, "http://", 7) == 0) {
		/* we would do virtual hosting here? */
		file += 7;
		s = strchr(file, '/');
		/* HTTP/1.1 draft rev-06, 5.2: URI takes precedence over Host: */
		request->hr_host = file;
		request->hr_file = bozostrdup(s ? s : "/");
		debug((DEBUG_OBESE, "got host ``%s'' file is now ``%s''",
		    request->hr_host, request->hr_file));
	} else if (!request->hr_host)
		goto use_slashdir;


	/*
	 * ok, we have a virtual host, use scandir(3) to find a case
	 * insensitive match for the virtual host we are asked for.
	 * note that if the virtual host is the same as the master,
	 * we don't need to do anything special.
	 */
	len = strlen(request->hr_host);
	debug((DEBUG_OBESE,
	    "check_virtual: checking host `%s' under vpath `%s' for file `%s'",
	    request->hr_host, vpath, request->hr_file));
	if (strncasecmp(myname, request->hr_host, len) != 0) {
		s = 0;
		for (i = scandir(vpath, &list, 0, 0); i--; list++) {
			debug((DEBUG_OBESE, "looking at dir``%s''",
			    (*list)->d_name));
			if (strncasecmp((*list)->d_name, request->hr_host,
			    len) == 0) {
				/* found it, punch it */
				myname = (*list)->d_name;
				if (asprintf(&s, "%s/%s", vpath, myname) < 0)
					error(1, "asprintf");
				break;
			}
		}
		if (s == 0) {
			if (Vflag)
				goto use_slashdir;
			return http_error(404, request, "unknown URL");
		}
	} else
use_slashdir:
		s = slashdir;

	/*
	 * ok, nailed the correct slashdir, chdir to it
	 */
	if (chdir(s) < 0)
		return http_error(404, request, "can't chdir to slashdir");
	return 0;
}

/* make sure we're not trying to access special files */
int
check_special_files(http_req *request, const char *name)
{
	/* ensure basename(name) != special files */
	if (strcmp(name, DIRECT_ACCESS_FILE) == 0)
		return http_error(403, request,
		    "no permission to open direct access file");
	if (strcmp(name, REDIRECT_FILE) == 0)
		return http_error(403, request,
		    "no permission to open redirect file");
	if (strcmp(name, ABSREDIRECT_FILE) == 0)
		return http_error(403, request,
		    "no permission to open redirect file");
	return auth_check_special_files(request, name);
}

/*
 * checks to see if this request has a valid .bzredirect file.  returns
 * 0 on failure and 1 on success.
 */
static void
check_bzredirect(http_req *request)
{
	struct stat sb;
	char dir[MAXPATHLEN], redir[MAXPATHLEN], redirpath[MAXPATHLEN + 1];
	char *basename, *finalredir;
	int rv, absolute;

	/*
	 * if this pathname is really a directory, but doesn't end in /,
	 * use it as the directory to look for the redir file.
	 */
	snprintf(dir, sizeof(dir), "%s", request->hr_file + 1);
	debug((DEBUG_FAT, "check_bzredirect: dir %s", dir));
	basename = strrchr(dir, '/');

	if ((!basename || basename[1] != '\0') &&
	    lstat(dir, &sb) == 0 && S_ISDIR(sb.st_mode))
		/* nothing */;
	else if (basename == NULL)
		strcpy(dir, ".");
	else {
		*basename++ = '\0';
		check_special_files(request, basename);
	}

	snprintf(redir, sizeof(redir), "%s/%s", dir, REDIRECT_FILE);
	if (lstat(redir, &sb) == 0) {
		if (S_ISLNK(sb.st_mode) == 0)
			return;
		absolute = 0;
	} else {
		snprintf(redir, sizeof(redir), "%s/%s", dir, ABSREDIRECT_FILE);
		if (lstat(redir, &sb) < 0 || S_ISLNK(sb.st_mode) == 0)
			return;
		absolute = 1;
	}
	debug((DEBUG_FAT, "check_bzredirect: calling readlink"));
	rv = readlink(redir, redirpath, sizeof redirpath - 1);
	if (rv == -1 || rv == 0) {
		debug((DEBUG_FAT, "readlink failed"));
		return;
	}
	redirpath[rv] = '\0';
	debug((DEBUG_FAT, "readlink returned \"%s\"", redirpath));
	
	/* now we have the link pointer, redirect to the real place */
	if (absolute)
		finalredir = redirpath;
	else
		snprintf(finalredir = redir, sizeof(redir), "/%s/%s", dir,
			 redirpath);

	debug((DEBUG_FAT, "check_bzredirect: new redir %s", finalredir));
	handle_redirect(request, finalredir, absolute);
}

/*
 * checks to see if this request has a valid .bzdirect file.  returns
 * 0 on failure and 1 on success.
 */
static int
check_direct_access(http_req *request)
{
	FILE *fp;
	struct stat sb;
	char dir[MAXPATHLEN], dirfile[MAXPATHLEN], *basename;

	snprintf(dir, sizeof(dir), "%s", request->hr_file + 1);
	debug((DEBUG_FAT, "check_bzredirect: dir %s", dir));
	basename = strrchr(dir, '/');

	if ((!basename || basename[1] != '\0') &&
	    lstat(dir, &sb) == 0 && S_ISDIR(sb.st_mode))
		/* nothing */;
	else if (basename == NULL)
		strcpy(dir, ".");
	else {
		*basename++ = '\0';
		check_special_files(request, basename);
	}

	snprintf(dirfile, sizeof(dirfile), "%s/%s", dir, DIRECT_ACCESS_FILE);
	if (stat(dirfile, &sb) < 0 ||
	    (fp = fopen(dirfile, "r")) == NULL)
		return 0;
	fclose(fp);
	return 1;
}

/*
 * transform_request does this:
 *	- ``expand'' %20 crapola
 *	- punt if it doesn't start with /
 *	- check rflag / referrer
 *	- look for "http://myname/" and deal with it.
 *	- maybe call process_cgi() 
 *	- check for ~user and call user_transform() if so
 *	- if the length > 1, check for trailing slash.  if so,
 *	  add the index.html file
 *	- if the length is 1, return the index.html file
 *	- disallow anything ending up with a file starting
 *	  at "/" or having ".." in it.
 *	- anything else is a really weird internal error
 *	- returns malloced file to serve, if unhandled
 */
int
transform_request(http_req *request, int *isindex)
{
	char	*file, *newfile = NULL;
	size_t	len;

	file = NULL;
	*isindex = 0;
	debug((DEBUG_FAT, "tf_req: file %s", request->hr_file));
	fix_url_percent(request);
	if (check_virtual(request)) {
		goto bad_done;
	}
	file = request->hr_file;

	if (file[0] != '/') {
		(void)http_error(404, request, "unknown URL");
		goto bad_done;
	}

	check_bzredirect(request);

	if (rflag) {
		int to_indexhtml = 0;

#define TOP_PAGE(x)	(strcmp((x), "/") == 0 || \
			 strcmp((x) + 1, index_html) == 0 || \
			 strcmp((x) + 1, "favicon.ico") == 0) 
		
		debug((DEBUG_EXPLODING, "checking rflag"));
		/*
		 * first check that this path isn't allowed via .bzdirect file,
		 * and then check referrer; make sure that people come via the
		 * real name... otherwise if we aren't looking at / or
		 * /index.html, redirect...  we also special case favicon.ico.
		 */
		if (check_direct_access(request))
			/* nothing */;
		else if (request->hr_referrer) {
			const char *r = request->hr_referrer;

			debug((DEBUG_FAT,
			   "checking referrer \"%s\" vs myname %s", r, myname));
			if (strncmp(r, "http://", 7) != 0 ||
			    (strncasecmp(r + 7, myname, strlen(myname)) != 0 &&
			     !TOP_PAGE(file)))
				to_indexhtml = 1;
		} else {
			const char *h = request->hr_host;

			debug((DEBUG_FAT, "url has no referrer at all"));
			/* if there's no referrer, let / or /index.html past */
			if (!TOP_PAGE(file) ||
			    (h && strncasecmp(h, myname, strlen(myname)) != 0))
				to_indexhtml = 1;
		}

		if (to_indexhtml) {
			char *slashindexhtml;

			if (asprintf(&slashindexhtml, "/%s", index_html) < 0)
				error(1, "asprintf");
			debug((DEBUG_FAT, "rflag: redirecting %s to %s", file, slashindexhtml));
			handle_redirect(request, slashindexhtml, 0);
			free(slashindexhtml);
			return 0;
		}
	}

	len = strlen(file);
	if (0) {
#ifndef NO_USER_SUPPORT
	} else if (len > 1 && uflag && file[1] == '~') {
		if (file[2] == '\0') {
			(void)http_error(404, request, "missing username");
			goto bad_done;
		}
		if (strchr(file + 2, '/') == NULL) {
			handle_redirect(request, NULL, 0);
			return 0;
		}
		debug((DEBUG_FAT, "calling user_transform"));

		return (user_transform(request, isindex));
#endif /* NO_USER_SUPPORT */
	} else if (len > 1) {
		debug((DEBUG_FAT, "file[len-1] == %c", file[len-1]));
		if (file[len-1] == '/') {	/* append index.html */
			*isindex = 1;
			debug((DEBUG_FAT, "appending index.html"));
			newfile = bozomalloc(len + strlen(index_html) + 1);
			strcpy(newfile, file + 1);
			strcat(newfile, index_html);
		} else
			newfile = bozostrdup(file + 1);
	} else if (len == 1) {
		debug((DEBUG_EXPLODING, "tf_req: len == 1"));
		newfile = bozostrdup(index_html);
		*isindex = 1;
	} else {	/* len == 0 ? */
		(void)http_error(500, request, "request->hr_file is nul?");
		goto bad_done;
	}

	if (newfile == NULL) {
		(void)http_error(500, request, "internal failure");
		goto bad_done;
	}

	/*
	 * look for "http://myname/" and deal with it as necessary.
	 */

	/*
	 * stop traversing outside our domain 
	 *
	 * XXX true security only comes from our parent using chroot(2)
	 * before execve(2)'ing us.  or our own built in chroot(2) support.
	 */
	if (*newfile == '/' || strcmp(newfile, "..") == 0 ||
	    strstr(newfile, "/..") || strstr(newfile, "../")) {
		(void)http_error(403, request, "illegal request");
		goto bad_done;
	}

	if (auth_check(request, newfile))
		goto bad_done;

	if (strlen(newfile)) {
		free(request->hr_file);
		request->hr_file = newfile;
	}

	if (process_cgi(request))
		return 0;

	debug((DEBUG_FAT, "transform_request set: %s", newfile));
	return 1;
bad_done:
	debug((DEBUG_FAT, "transform_request returning: 0"));
	if (newfile)
		free(newfile);
	return 0;
}

/*
 * do automatic redirection -- if there are query parameters for the URL
 * we will tack these on to the new (redirected) URL.
 */
static void
handle_redirect(http_req *request, const char *url, int absolute)
{
	char *urlbuf;
	char portbuf[20];
	int query = 0;
	
	if (url == NULL) {
		if (asprintf(&urlbuf, "/%s/", request->hr_file) < 0)
			error(1, "asprintf");
		url = urlbuf;
	} else
		urlbuf = NULL;

	if (request->hr_query && strlen(request->hr_query)) {
		query = 1;
	}

	if (request->hr_serverport && strcmp(request->hr_serverport, "80") != 0)
		snprintf(portbuf, sizeof(portbuf), ":%s",
		    request->hr_serverport);
	else
		portbuf[0] = '\0';
	warning("redirecting %s%s%s", myname, portbuf, url);
	debug((DEBUG_FAT, "redirecting %s", url));
	bozoprintf("%s 301 Document Moved\r\n", request->hr_proto);
	if (request->hr_proto != http_09) 
		print_header(request, NULL, "text/html", NULL);
	if (request->hr_proto != http_09) {
		bozoprintf("Location: http://");
		if (absolute == 0)
			bozoprintf("%s%s", myname, portbuf);
		if (query) {
		  bozoprintf("%s?%s\r\n", url, request->hr_query);
		} else {
		  bozoprintf("%s\r\n", url);
		}
	}
	bozoprintf("\r\n");
	if (request->hr_method == HTTP_HEAD)
		goto head;
	bozoprintf("<html><head><title>Document Moved</title></head>\n");
	bozoprintf("<body><h1>Document Moved</h1>\n");
	bozoprintf("This document had moved <a href=\"http://");
	if (query) {
	  if (absolute)
	    bozoprintf("%s?%s", url, request->hr_query);
	  else
	    bozoprintf("%s%s%s?%s", myname, portbuf, url, request->hr_query);
        } else {
	  if (absolute)
	    bozoprintf("%s", url);
	  else
	    bozoprintf("%s%s%s", myname, portbuf, url);
	}
	bozoprintf("\">here</a>\n");
	bozoprintf("</body></html>\n");
head:
	bozoflush(stdout);
	if (urlbuf)
		free(urlbuf);
}

/* generic header printing routine */
void
print_header(http_req *request, struct stat *sbp, const char *type,
	     const char *encoding)
{
	off_t len;

	bozoprintf("Date: %s\r\n", http_date());
	bozoprintf("Server: %s\r\n", server_software);
	bozoprintf("Accept-Ranges: bytes\r\n");
	if (sbp) {
		char filedate[40];
		struct	tm *tm;

		tm = gmtime(&sbp->st_mtime);
		strftime(filedate, sizeof filedate,
		    "%a, %d %b %Y %H:%M:%S GMT", tm);
		bozoprintf("Last-Modified: %s\r\n", filedate);
	}
	if (type && *type)
		bozoprintf("Content-Type: %s\r\n", type);
	if (encoding && *encoding)
		bozoprintf("Content-Encoding: %s\r\n", encoding);
	if (sbp) {
		if (request->hr_have_range) {
			len = request->hr_last_byte_pos - request->hr_first_byte_pos +1;
			bozoprintf("Content-Range: bytes %qd-%qd/%qd\r\n",
			    (long long) request->hr_first_byte_pos,
			    (long long) request->hr_last_byte_pos,
			    (long long) sbp->st_size);
		}
		else
			len = sbp->st_size;
		bozoprintf("Content-Length: %qd\r\n", (long long)len);
	}
	if (request && request->hr_proto == http_11)
		bozoprintf("Connection: close\r\n");
	bozoflush(stdout);
}

/* this escape HTML tags */
static void
escape_html(http_req *request)
{
	int	i, j;
	char	*url = request->hr_file, *tmp;

	for (i = 0, j = 0; url[i]; i++) {
		switch (url[i]) {
		case '<':
		case '>':
			j += 4;
			break;
		case '&':
			j += 5;
			break;
		}
	}

	if (j == 0)
		return;

	if ((tmp = (char *) malloc(strlen(url) + j)) == 0)
		/*
		 * ouch, but we are only called from an error context, and
		 * most paths here come from malloc(3) failures anyway...
		 * we could completely punt and just exit, but isn't returning
		 * an not-quite-correct error better than nothing at all?
		 */
		return;

	for (i = 0, j = 0; url[i]; i++) {
		switch (url[i]) {
		case '<':
			memcpy(tmp + j, "&lt;", 4);
			j += 4;
			break;
		case '>':
			memcpy(tmp + j, "&gt;", 4);
			j += 4;
			break;
		case '&':
			memcpy(tmp + j, "&amp;", 5);
			j += 5;
			break;
		default:
			tmp[j++] = url[i];
		}
	}
	tmp[j] = 0;

	free(request->hr_file);
	request->hr_file = tmp;
}

/* this fixes the %HH hack that RFC2396 requires.  */
static void
fix_url_percent(http_req *request)
{
	char	*s, *t, buf[3], *url;
	char	*end;	/* if end is not-zero, we don't translate beyond that */

	url = request->hr_file;

	end = url + strlen(url);

	/* fast forward to the first % */
	if ((s = strchr(url, '%')) == NULL)
		return;

	t = s;
	do {
		if (end && s >= end) {
			debug((DEBUG_EXPLODING, "fu_%%: past end, filling out.."));
			while (*s)
				*t++ = *s++;
			break;
		}
		debug((DEBUG_EXPLODING, "fu_%%: got s == %%, s[1]s[2] == %c%c",
		    s[1], s[2]));
		if (s[1] == '\0' || s[2] == '\0') {
			(void)http_error(400, request,
			    "percent hack missing two chars afterwards");
			goto copy_rest;
		}
		if (s[1] == '0' && s[2] == '0') {
			(void)http_error(404, request, "percent hack was %00");
			goto copy_rest;
		}
		if (s[1] == '2' && s[2] == 'f') {
			(void)http_error(404, request, "percent hack was %2f (/)");
			goto copy_rest;
		}
			
		buf[0] = *++s;
		buf[1] = *++s;
		buf[2] = '\0';
		s++;
		*t = (char)strtol(buf, NULL, 16);
		debug((DEBUG_EXPLODING, "fu_%%: strtol put '%02x' into *t", *t));
		if (*t++ == '\0') {
			(void)http_error(400, request, "percent hack got a 0 back");
			goto copy_rest;
		}

		while (*s && *s != '%') {
			if (end && s >= end)
				break;
			*t++ = *s++;
		}
	} while (*s);
copy_rest:
	while (*s) {
		if (s >= end)
			break;
		*t++ = *s++;
	}
	*t = '\0';
	debug((DEBUG_FAT, "fix_url_percent returns %s in url", request->hr_file));
}

/*
 * process each type of HTTP method, setting this HTTP requests
 # method type.
 */
static struct method_map {
	const char *name;
	int	type;
} method_map[] = {
	{ "GET", 	HTTP_GET, },
	{ "POST",	HTTP_POST, },
	{ "HEAD",	HTTP_HEAD, },
#if 0	/* other non-required http/1.1 methods */
	{ "OPTIONS",	HTTP_OPTIONS, },
	{ "PUT",	HTTP_PUT, },
	{ "DELETE",	HTTP_DELETE, },
	{ "TRACE",	HTTP_TRACE, },
	{ "CONNECT",	HTTP_CONNECT, },
#endif
	{ NULL,		0, },
};

static int
process_method(http_req *request, const char *method)
{
	struct	method_map *mmp;

	if (request->hr_proto == http_11)
		request->hr_allow = "GET, HEAD, POST";

	for (mmp = method_map; mmp->name; mmp++)
		if (strcasecmp(method, mmp->name) == 0) {
			request->hr_method = mmp->type;
			request->hr_methodstr = mmp->name;
			return 0;
		}

	return http_error(404, request, "unknown method");
}

/*
 * as the prototype string is not constant (eg, "HTTP/1.1" is equivalent
 * to "HTTP/001.01"), we MUST parse this.
 */
static int
process_proto(http_req *request, const char *proto)
{
	char	majorstr[16], *minorstr;
	int	majorint, minorint;

	if (proto == NULL) {
got_proto_09:
		request->hr_proto = http_09;
		debug((DEBUG_FAT, "request %s is http/0.9", request->hr_file));
		return 0;
	}

	if (strncasecmp(proto, "HTTP/", 5) != 0)
		goto bad;
	strncpy(majorstr, proto + 5, sizeof majorstr);
	majorstr[sizeof(majorstr)-1] = 0;
	minorstr = strchr(majorstr, '.');
	if (minorstr == NULL)
		goto bad;
	*minorstr++ = 0;

	majorint = atoi(majorstr);
	minorint = atoi(minorstr);

	switch (majorint) {
	case 0:
		if (minorint != 9)
			break;
		goto got_proto_09;
	case 1:
		if (minorint == 0)
			request->hr_proto = http_10;
		else if (minorint == 1)
			request->hr_proto = http_11;
		else
			break;

		debug((DEBUG_FAT, "request %s is %s", request->hr_file,
		    request->hr_proto));
		SIMPLEQ_INIT(&request->hr_headers);
		request->hr_nheaders = 0;
		return 0;
	}
bad:
	return http_error(404, NULL, "unknown prototype");
}

#ifdef DEBUG
void
debug__(int level, const char *fmt, ...)
{
	va_list	ap;
	int savederrno;
	
	/* only log if the level is low enough */
	if (dflag < level)
		return;

	savederrno = errno;
	va_start(ap, fmt);
	if (sflag) {
		vfprintf(stderr, fmt, ap);
		fputs("\n", stderr);
	} else
		vsyslog(LOG_DEBUG, fmt, ap);
	va_end(ap);
	errno = savederrno;
}
#endif /* DEBUG */

/* these are like warn() and err(), except for syslog not stderr */
void
warning(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (sflag || isatty(STDERR_FILENO)) {
		//fputs("warning: ", stderr);
		vfprintf(stderr, fmt, ap);
		fputs("\n", stderr);
	} else
		vsyslog(LOG_INFO, fmt, ap);
	va_end(ap);
}

void
error(int code, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (sflag || isatty(STDERR_FILENO)) {
		//fputs("error: ", stderr);
		vfprintf(stderr, fmt, ap);
		fputs("\n", stderr);
	} else
		vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(code);
}

/* the follow functions and variables are used in handling HTTP errors */
/* ARGSUSED */
int
http_error(int code, http_req *request, const char *msg)
{
	static	char buf[BUFSIZ];
	char portbuf[20];
	const char *header = http_errors_short(code);
	const char *reason = http_errors_long(code);
	const char *proto = (request && request->hr_proto) ? request->hr_proto : http_11;
	int	size;

	debug((DEBUG_FAT, "http_error %d: %s", code, msg));
	if (header == NULL || reason == NULL) {
		error(1, "http_error() failed (short = %p, long = %p)",
		    header, reason);
		return code;
	}

	if (request && request->hr_serverport &&
	    strcmp(request->hr_serverport, "80") != 0)
		snprintf(portbuf, sizeof(portbuf), ":%s", request->hr_serverport);
	else
		portbuf[0] = '\0';

	if (request && request->hr_file) {
		escape_html(request);
		size = snprintf(buf, sizeof buf,
		    "<html><head><title>%s</title></head>\n"
		    "<body><h1>%s</h1>\n"
		    "%s: <pre>%s</pre>\n"
 		    "<hr><address><a href=\"http://%s%s/\">%s%s</a></address>\n"
		    "</body></html>\n",
		    header, header, request->hr_file, reason,
		    myname, portbuf, myname, portbuf);
		if (size >= (int)sizeof buf) {
			warning("http_error buffer too small, truncated");
			size = (int)sizeof buf;
		}
	} else
		size = 0;

	bozoprintf("%s %s\r\n", proto, header);
	auth_check_401(request, code);

	bozoprintf("Content-Type: text/html\r\n");
	bozoprintf("Content-Length: %d\r\n", size);
	bozoprintf("Server: %s\r\n", server_software);
	if (request && request->hr_allow)
		bozoprintf("Allow: %s\r\n", request->hr_allow);
	bozoprintf("\r\n");
	if (size)
		bozoprintf("%s", buf);
	bozoflush(stdout);

	return code;
}

/* short map between error code, and short/long messages */
static struct errors_map {
	int	code;			/* HTTP return code */
	const char *shortmsg;		/* short version of message */
	const char *longmsg;		/* long version of message */
} errors_map[] = {
	{ 400,	"400 Bad Request",	"The request was not valid", },
	{ 401,	"401 Unauthorized",	"No authorization", },
	{ 403,	"403 Forbidden",	"Access to this item has been denied",},
	{ 404, 	"404 Not Found",	"This item has not been found", },
	{ 408, 	"408 Request Timeout",	"This request took too long", },
	{ 417,	"417 Expectation Failed","Expectations not available", },
	{ 500,	"500 Internal Error",	"An error occured on the server", },
	{ 501,	"501 Not Implemented",	"This request is not available", },
	{ 0,	NULL,			NULL, },
};

static const char *help = "DANGER! WILL ROBINSON! DANGER!";

static const char *
http_errors_short(int code)
{
	struct errors_map *ep;

	for (ep = errors_map; ep->code; ep++)
		if (ep->code == code)
			return (ep->shortmsg);
	return (help);
}

static const char *
http_errors_long(int code)
{
	struct errors_map *ep;

	for (ep = errors_map; ep->code; ep++)
		if (ep->code == code)
			return (ep->longmsg);
	return (help);
}

/* Below are various modified libc functions */

/*
 * returns -1 in lenp if the string ran out before finding a delimiter,
 * but is otherwise the same as strsep.  Note that the length must be
 * correctly passed in.
 */
char *
bozostrnsep(char **strp, const char *delim, ssize_t	*lenp)
{
	char	*s;
	const	char *spanp;
	int	c, sc;
	char	*tok;

	if ((s = *strp) == NULL)
		return (NULL);
	for (tok = s;;) {
		if (lenp && --(*lenp) == -1)
			return (NULL);
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = '\0';
				*strp = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

/*
 * inspired by fgetln(3), but works for fd's.  should work identically
 * except it, however, does *not* return the newline, and it does nul
 * terminate the string.
 */
char *
bozodgetln(int fd, ssize_t *lenp, ssize_t (*readfn)(int, void *, size_t))
{
	static	char *buffer;
	static	ssize_t buflen = 0;
	ssize_t	len;
	int	got_cr = 0;
	char	c, *nbuffer;

	/* initialise */
	if (buflen == 0) {
		buflen = 128;	/* should be plenty for most requests */
		buffer = malloc(buflen);
		if (buffer == NULL) {
			buflen = 0;
			return NULL;
		}
	}
	len = 0;

	/*
	 * we *have* to read one byte at a time, to not break cgi
	 * programs (for we pass stdin off to them).  could fix this
	 * by becoming a fd-passing program instead of just exec'ing
	 * the program
	 */
	for (; readfn(fd, &c, 1) == 1; ) {
		debug((DEBUG_EXPLODING, "bozodgetln read %c", c));

		if (len >= buflen - 1) {
			buflen *= 2;
			debug((DEBUG_EXPLODING, "bozodgetln: "
			    "reallocating buffer to buflen %zu", buflen));
			nbuffer = bozorealloc(buffer, buflen);
			buffer = nbuffer;
		}

		buffer[len++] = c;
		if (c == '\r') {
			got_cr = 1;
			continue;
		} else if (c == '\n') {
			/*
			 * HTTP/1.1 spec says to ignore CR and treat
			 * LF as the real line terminator.  even though
			 * the same spec defines CRLF as the line
			 * terminator, it is recommended in section 19.3
			 * to do the LF trick for tolerance.
			 */
			if (got_cr)
				len -= 2;
			else
				len -= 1;
			break;
		}

	}
	buffer[len] = '\0';
	debug((DEBUG_OBESE, "bozodgetln returns: ``%s'' with len %d",
	       buffer, len));
	*lenp = len;
	return (buffer);
}

void *
bozorealloc(void *ptr, size_t size)
{
	void	*p;

	p = realloc(ptr, size);
	if (p == NULL) {
		(void)http_error(500, NULL, "memory allocation failure");
		exit(1);
	}
	return (p);
}

void *
bozomalloc(size_t size)
{
	void	*p;

	p = malloc(size);
	if (p == NULL) {
		(void)http_error(500, NULL, "memory allocation failure");
		exit(1);
	}
	return (p);
}

char *
bozostrdup(const char *str)
{
	char	*p;

	p = strdup(str);
	if (p == NULL) {
		(void)http_error(500, NULL, "memory allocation failure");
		exit(1);
	}
	return (p);
}
