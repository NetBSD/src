/*	$NetBSD: fetch.c,v 1.41 1998/12/27 05:49:53 lukem Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason Thorpe and Luke Mewburn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: fetch.c,v 1.41 1998/12/27 05:49:53 lukem Exp $");
#endif /* not lint */

/*
 * FTP User Program -- Command line file retrieval
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <arpa/ftp.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "ftp_var.h"

typedef enum {
	UNKNOWN_URL_T=-1,
	HTTP_URL_T,
	FTP_URL_T,
	FILE_URL_T
} url_t;

static int	parse_url __P((const char *, const char *, url_t *, char **,
				char **, char **, in_port_t *, char **));
static int	url_get __P((const char *, const char *));
void    	aborthttp __P((int));


#define	ABOUT_URL	"about:"	/* propaganda */
#define	FILE_URL	"file://"	/* file URL prefix */
#define	FTP_URL		"ftp://"	/* ftp URL prefix */
#define	HTTP_URL	"http://"	/* http URL prefix */


#define EMPTYSTRING(x)	((x) == NULL || (*(x) == '\0'))
#define FREEPTR(x)	if ((x) != NULL) { free(x); (x) = NULL; }

/*
 * Parse URL of form:
 *	<type>://[<user>[:<password>@]]<host>[:<port>]/<url-path>
 * Returns -1 if a parse error occurred, otherwise 0.
 * Sets type to url_t, each of the given char ** pointers to a
 * malloc(3)ed strings of the relevant section, and port to
 * the number given, or ftpport if ftp://, or httpport if http://.
 */
static int
parse_url(url, desc, type, user, pass, host, port, path)
	const char	 *url;
	const char	 *desc;
	url_t		 *type;
	char		**user;
	char		**pass;
	char		**host;
	in_port_t	 *port;
	char		**path;
{
	char *cp, *ep, *thost;

	if (url == NULL || desc == NULL || type == NULL || user == NULL
	    || pass == NULL || host == NULL || port == NULL || path == NULL)
		errx(1, "parse_url: invoked with NULL argument!");

	*type = UNKNOWN_URL_T;
	*user = *pass = *host = *path = NULL;
	*port = 0;

	if (strncasecmp(url, HTTP_URL, sizeof(HTTP_URL) - 1) == 0) {
		url += sizeof(HTTP_URL) - 1;
		*type = HTTP_URL_T;
		*port = httpport;
	} else if (strncasecmp(url, FTP_URL, sizeof(FTP_URL) - 1) == 0) {
		url += sizeof(FTP_URL) - 1;
		*type = FTP_URL_T;
		*port = ftpport;
	} else if (strncasecmp(url, FILE_URL, sizeof(FILE_URL) - 1) == 0) {
		url += sizeof(FILE_URL) - 1;
		*type = FILE_URL_T;
	} else {
		warnx("Invalid %s `%s'", desc, url);
cleanup_parse_url:
		FREEPTR(*user);
		FREEPTR(*pass);
		FREEPTR(*host);
		FREEPTR(*path);
		return (-1);
	}

	if (*url == '\0')
		return (0);

			/* find [user[:pass]@]host[:port] */
	ep = strchr(url, '/');
	if (ep == NULL)
		thost = xstrdup(url);
	else {
		size_t len = ep - url;
		thost = (char *)xmalloc(len + 1);
		strncpy(thost, url, len);
		thost[len] = '\0';
		*path = xstrdup(ep);
	}

	cp = strchr(thost, '@');
	if (cp != NULL) {
		*user = thost;
		*cp = '\0';
		*host = xstrdup(cp + 1);
		cp = strchr(*user, ':');
		if (cp != NULL) {
			*cp = '\0';
			*pass = xstrdup(cp + 1);
		}
	} else
		*host = thost;

			/* look for [:port] */
	cp = strrchr(*host, ':');
	if (cp != NULL) {
		long nport;

		*cp = '\0';
		nport = strtol(cp + 1, &ep, 10);
		if (nport < 1 || nport > MAX_IN_PORT_T || *ep != '\0') {
			warnx("Invalid port `%s' in %s `%s'", cp, desc, line);
			goto cleanup_parse_url;
		}
		*port = htons((in_port_t)nport);
	}

	if (debug)
		fprintf(ttyout,
		    "parse_url: user `%s', pass `%s', host %s:%d, path `%s'\n",
		    *user ? *user : "", *pass ? *pass : "", *host ? *host : "",
		    ntohs(*port), *path ? *path : "");

	return (0);
}


jmp_buf	httpabort;

/*
 * Retrieve URL or classic ftp argument, via the proxy in $proxyvar if
 * necessary.
 * Returns -1 on failure, 0 on completed xfer, 1 if ftp connection
 * is still open (e.g, ftp xfer with trailing /)
 */
static int
url_get(url, outfile)
	const char *url;
	const char *outfile;
{
	struct sockaddr_in sin;
	int isredirected, isproxy;
	volatile int s;
	size_t len;
	char *cp, *ep;
	char *buf, *savefile;
	volatile sig_t oldintr, oldintp;
	off_t hashbytes;
	struct hostent *hp = NULL;
	int (*closefunc) __P((FILE *));
	FILE *fin, *fout;
	int retval;
	time_t mtime;
	url_t urltype;
	char *user, *pass, *host;
	in_port_t port;
	char *path;
	const char *proxyenv;

	closefunc = NULL;
	fin = fout = NULL;
	s = -1;
	buf = savefile = NULL;
	isredirected = isproxy = 0;
	retval = -1;
	proxyenv = NULL;

#ifdef __GNUC__			/* shut up gcc warnings */
	(void)&closefunc;
	(void)&fin;
	(void)&fout;
	(void)&buf;
	(void)&savefile;
	(void)&retval;
	(void)&isproxy;
#endif

	if (parse_url(url, "URL", &urltype, &user, &pass, &host, &port, &path)
	    == -1)
		goto cleanup_url_get;

	if (urltype == FILE_URL_T && ! EMPTYSTRING(host)
	    && strcasecmp(host, "localhost") != 0) {
		warnx("No support for non local file URL `%s'", url);
		goto cleanup_url_get;
	}

	if (EMPTYSTRING(path)) {
		if (urltype == FTP_URL_T)
			goto noftpautologin;
		if (urltype != HTTP_URL_T || outfile == NULL)  {
			warnx("Invalid URL (no file after host) `%s'", url);
			goto cleanup_url_get;
		}
	}

	if (outfile)
		savefile = xstrdup(outfile);
	else {
		cp = strrchr(path, '/');		/* find savefile */
		if (cp != NULL)
			savefile = xstrdup(cp + 1);
		else
			savefile = xstrdup(path);
	}
	if (EMPTYSTRING(savefile)) {
		if (urltype == FTP_URL_T)
			goto noftpautologin;
		warnx("Invalid URL (no file after directory) `%s'", url);
		goto cleanup_url_get;
	}

	filesize = -1;
	mtime = -1;
	if (urltype == FILE_URL_T) {		/* file:// URLs */
		struct stat sb;

		direction = "copied";
		fin = fopen(path, "r");
		if (fin == NULL) {
			warn("Cannot open file `%s'", path);
			goto cleanup_url_get;
		}
		if (fstat(fileno(fin), &sb) == 0) {
			mtime = sb.st_mtime;
			filesize = sb.st_size;
		}
		fprintf(ttyout, "Copying %s\n", path);
	} else {				/* ftp:// or http:// URLs */
		if (urltype == HTTP_URL_T)
			proxyenv = httpproxy;
		else if (urltype == FTP_URL_T)
			proxyenv = ftpproxy;
		direction = "retrieved";
		if (proxyenv != NULL) {				/* use proxy */
			url_t purltype;
			char *puser, *ppass, *phost;
			char *ppath;

			isproxy = 1;

				/* check URL against list of no_proxied sites */
			if (no_proxy != NULL) {
				char *np, *np_copy;
				long np_port;
				size_t hlen, plen;

				np_copy = xstrdup(no_proxy);
				hlen = strlen(host);
				while ((cp = strsep(&np_copy, " ,")) != NULL) {
					if (*cp == '\0')
						continue;
					if ((np = strchr(cp, ':')) != NULL) {
						*np = '\0';
						np_port =
						    strtol(np + 1, &ep, 10);
						if (*ep != '\0')
							continue;
						if (port !=
						    htons((in_port_t)np_port))
							continue;
					}
					plen = strlen(cp);
					if (strncasecmp(host + hlen - plen,
					    cp, plen) == 0) {
						isproxy = 0;
						break;
					}
				}
				FREEPTR(np_copy);
			}

			if (isproxy) {
				if (parse_url(proxyenv, "proxy URL", &purltype,
				    &puser, &ppass, &phost, &port, &ppath)
				    == -1)
					goto cleanup_url_get;

				if ((purltype != HTTP_URL_T
				     && purltype != FTP_URL_T) ||
				    EMPTYSTRING(phost) ||
				    (! EMPTYSTRING(ppath)
				     && strcmp(ppath, "/") != 0)) {
					warnx("Malformed proxy URL `%s'",
					    proxyenv);
					FREEPTR(puser);
					FREEPTR(ppass);
					FREEPTR(phost);
					FREEPTR(ppath);
					goto cleanup_url_get;
				}

				FREEPTR(user);
				user = puser;
				FREEPTR(pass);
				pass = ppass;
				FREEPTR(host);
				host = phost;
				FREEPTR(path);
				FREEPTR(ppath);
				path = xstrdup(url);
			}
		} /* proxyenv != NULL */

		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;

		if (isdigit((unsigned char)host[0])) {
			if (inet_aton(host, &sin.sin_addr) == 0) {
				warnx("Invalid IP address `%s'", host);
				goto cleanup_url_get;
			}
		} else {
			hp = gethostbyname(host);
			if (hp == NULL) {
				warnx("%s: %s", host, hstrerror(h_errno));
				goto cleanup_url_get;
			}
			if (hp->h_addrtype != AF_INET) {
				warnx("`%s': not an Internet address?", host);
				goto cleanup_url_get;
			}
			memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
		}

		if (port == 0) {
			warnx("Unknown port for URL `%s'", url);
			goto cleanup_url_get;
		}
		sin.sin_port = port;

		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s == -1) {
			warn("Can't create socket");
			goto cleanup_url_get;
		}

		while (xconnect(s, (struct sockaddr *)&sin,
		    sizeof(sin)) == -1) {
			if (errno == EINTR)
				continue;
			if (hp && hp->h_addr_list[1]) {
				int oerrno = errno;
				char *ia;

				ia = inet_ntoa(sin.sin_addr);
				errno = oerrno;
				warn("Connect to address `%s'", ia);
				hp->h_addr_list++;
				memcpy(&sin.sin_addr, hp->h_addr_list[0],
				    (size_t)hp->h_length);
				fprintf(ttyout, "Trying %s...\n",
				    inet_ntoa(sin.sin_addr));
				(void)close(s);
				s = socket(AF_INET, SOCK_STREAM, 0);
				if (s < 0) {
					warn("Can't create socket");
					goto cleanup_url_get;
				}
				continue;
			}
			warn("Can't connect to `%s'", host);
			goto cleanup_url_get;
		}

		fin = fdopen(s, "r+");
		/*
		 * Construct and send the request.
		 * Proxy requests don't want leading /.
		 */
		if (isproxy) {
			fprintf(ttyout, "Requesting %s\n  (via %s)\n",
			    url, proxyenv);
			fprintf(fin, "GET %s HTTP/1.0\r\n\r\n", path);
		} else {
			fprintf(ttyout, "Requesting %s\n", url);
			fprintf(fin, "GET %s HTTP/1.1\r\n", path);
			fprintf(fin, "Host: %s\r\n", host);
			fprintf(fin, "Accept: */*\r\n");
			fprintf(fin, "Connection: close\r\n\r\n");
		}
		if (fflush(fin) == EOF) {
			warn("Writing HTTP request");
			goto cleanup_url_get;
		}

				/* Read the response */
		if ((buf = fparseln(fin, &len, NULL, "\0\0\0", 0)) == NULL) {
			warn("Receiving HTTP reply");
			goto cleanup_url_get;
		}
		while (len > 0 && (buf[len-1] == '\r' || buf[len-1] == '\n'))
			buf[--len] = '\0';
		if (debug)
			fprintf(ttyout, "received `%s'\n", buf);

		cp = strchr(buf, ' ');
		if (cp == NULL)
			goto improper;
		else
			cp++;
		if (strncmp(cp, "301", 3) == 0 || strncmp(cp, "302", 3) == 0) {
			isredirected++;
		} else if (strncmp(cp, "200", 3)) {
			warnx("Error retrieving file `%s'", cp);
			goto cleanup_url_get;
		}

				/* Read the rest of the header. */
		FREEPTR(buf);
		while (1) {
			if ((buf = fparseln(fin, &len, NULL, "\0\0\0", 0))
			    == NULL) {
				warn("Receiving HTTP reply");
				goto cleanup_url_get;
			}
			while (len > 0 &&
			    (buf[len-1] == '\r' || buf[len-1] == '\n'))
				buf[--len] = '\0';
			if (len == 0)
				break;
			if (debug)
				fprintf(ttyout, "received `%s'\n", buf);

				/* Look for some headers */
			cp = buf;
#define CONTENTLEN "Content-Length: "
			if (strncasecmp(cp, CONTENTLEN,
			    sizeof(CONTENTLEN) - 1) == 0) {
				cp += sizeof(CONTENTLEN) - 1;
				filesize = strtol(cp, &ep, 10);
				if (filesize < 1 || *ep != '\0')
					goto improper;
				if (debug)
					fprintf(ttyout,
#ifndef NO_QUAD
					    "parsed length as: %qd\n",
					    (long long)filesize);
#else
					    "parsed length as: %ld\n",
					    (long)filesize);
#endif
#define LASTMOD "Last-Modified: "
			} else if (strncasecmp(cp, LASTMOD,
			    sizeof(LASTMOD) - 1) == 0) {
				struct tm parsed;
				char *t;

				cp += sizeof(LASTMOD) - 1;
							/* RFC 1123 */
				if ((t = strptime(cp,
						"%a, %d %b %Y %H:%M:%S GMT",
						&parsed))
							/* RFC 850 */
				    || (t = strptime(cp,
						"%a, %d-%b-%y %H:%M:%S GMT",
						&parsed))
							/* asctime */
				    || (t = strptime(cp,
						"%a, %b %d %H:%M:%S %Y",
						&parsed))) {
					parsed.tm_isdst = -1;
					if (*t == '\0')
						mtime = mkgmtime(&parsed);
					if (debug && mtime != -1) {
						fprintf(ttyout,
						    "parsed date as: %s",
						    ctime(&mtime));
					}
				}
#define LOCATION "Location: "
			} else if (isredirected &&
			    strncasecmp(cp, LOCATION,
				sizeof(LOCATION) - 1) == 0) {
				cp += sizeof(LOCATION) - 1;
				if (debug)
					fprintf(ttyout,
					    "parsed location as: %s\n", cp);
				if (verbose)
					fprintf(ttyout,
					    "Redirected to %s\n", cp);
				retval = url_get(cp, outfile);
				goto cleanup_url_get;
			}
		}
		FREEPTR(buf);
	}

	oldintr = oldintp = NULL;

			/* Open the output file. */
	if (strcmp(savefile, "-") == 0) {
		fout = stdout;
	} else if (*savefile == '|') {
		oldintp = signal(SIGPIPE, SIG_IGN);
		fout = popen(savefile + 1, "w");
		if (fout == NULL) {
			warn("Can't run `%s'", savefile + 1);
			goto cleanup_url_get;
		}
		closefunc = pclose;
	} else {
		fout = fopen(savefile, "w");
		if (fout == NULL) {
			warn("Can't open `%s'", savefile);
			goto cleanup_url_get;
		}
		closefunc = fclose;
	}

			/* Trap signals */
	if (setjmp(httpabort)) {
		if (oldintr)
			(void)signal(SIGINT, oldintr);
		if (oldintp)
			(void)signal(SIGPIPE, oldintp);
		goto cleanup_url_get;
	}
	oldintr = signal(SIGINT, aborthttp);

	bytes = 0;
	hashbytes = mark;
	progressmeter(-1);

			/* Finally, suck down the file. */
	buf = xmalloc(BUFSIZ);
	while ((len = fread(buf, sizeof(char), BUFSIZ, fin)) > 0) {
		bytes += len;
		if (fwrite(buf, sizeof(char), len, fout) != len) {
			warn("Writing `%s'", savefile);
			goto cleanup_url_get;
		}
		if (hash && !progress) {
			while (bytes >= hashbytes) {
				(void)putc('#', ttyout);
				hashbytes += mark;
			}
			(void)fflush(ttyout);
		}
	}
	if (hash && !progress && bytes > 0) {
		if (bytes < mark)
			(void)putc('#', ttyout);
		(void)putc('\n', ttyout);
		(void)fflush(ttyout);
	}
	if (ferror(fin)) {
		warn("Reading file");
		goto cleanup_url_get;
	}
	progressmeter(1);
	(void)fflush(fout);
	(void)signal(SIGINT, oldintr);
	if (oldintp)
		(void)signal(SIGPIPE, oldintp);
	if (closefunc == fclose && mtime != -1) {
		struct timeval tval[2];

		(void)gettimeofday(&tval[0], NULL);
		tval[1].tv_sec = mtime;
		tval[1].tv_usec = 0;
		(*closefunc)(fout);
		fout = NULL;

		if (utimes(savefile, tval) == -1) {
			fprintf(ttyout,
			    "Can't change modification time to %s",
			    asctime(localtime(&mtime)));
		}
	}
	if (bytes > 0)
		ptransfer(0);

	retval = 0;
	goto cleanup_url_get;

noftpautologin:
	warnx(
	    "Auto-login using ftp URLs isn't supported when using $ftp_proxy");
	goto cleanup_url_get;

improper:
	warnx("Improper response from `%s'", host);

cleanup_url_get:
	resetsockbufsize();
	if (fin != NULL)
		fclose(fin);
	else if (s != -1)
		close(s);
	if (closefunc != NULL && fout != NULL)
		(*closefunc)(fout);
	FREEPTR(savefile);
	FREEPTR(user);
	FREEPTR(pass);
	FREEPTR(host);
	FREEPTR(path);
	FREEPTR(buf);
	return (retval);
}

/*
 * Abort a http retrieval
 */
void
aborthttp(notused)
	int notused;
{

	alarmtimer(0);
	fputs("\nHTTP fetch aborted.\n", ttyout);
	(void)fflush(ttyout);
	longjmp(httpabort, 1);
}

/*
 * Retrieve multiple files from the command line, transferring
 * URLs of the form "host:path", "ftp://host/path" using the
 * ftp protocol, URLs of the form "http://host/path" using the
 * http protocol, and URLs of the form "file:///" by simple
 * copying.
 * If path has a trailing "/", the path will be cd-ed into and
 * the connection remains open, and the function will return -1
 * (to indicate the connection is alive).
 * If an error occurs the return value will be the offset+1 in
 * argv[] of the file that caused a problem (i.e, argv[x]
 * returns x+1)
 * Otherwise, 0 is returned if all files retrieved successfully.
 */
int
auto_fetch(argc, argv, outfile)
	int argc;
	char *argv[];
	char *outfile;
{
	static char lasthost[MAXHOSTNAMELEN];
	char portnum[6];		/* large enough for "65535\0" */
	char *xargv[5];
	const char *line;
	char *cp, *host, *path, *dir, *file;
	char *user, *pass;
	in_port_t port;
	int rval, xargc;
	volatile int argpos;
	int dirhasglob, filehasglob;
	char rempath[MAXPATHLEN];

#ifdef __GNUC__			/* to shut up gcc warnings */
	(void)&outfile;
#endif

	argpos = 0;

	if (setjmp(toplevel)) {
		if (connected)
			disconnect(0, NULL);
		return (argpos + 1);
	}
	(void)signal(SIGINT, (sig_t)intr);
	(void)signal(SIGPIPE, (sig_t)lostpeer);

	/*
	 * Loop through as long as there's files to fetch.
	 */
	for (rval = 0; (rval == 0) && (argpos < argc); argpos++) {
		if (strchr(argv[argpos], ':') == NULL)
			break;
		host = path = dir = file = user = pass = NULL;
		port = 0;
		line = argv[argpos];

#ifndef SMALL
		/*
		 * Check for about:*
		 */
		if (strncasecmp(line, ABOUT_URL, sizeof(ABOUT_URL) - 1) == 0) {
			line += sizeof(ABOUT_URL) -1;
			if (strcasecmp(line, "ftp") == 0) {
				fprintf(ttyout, "%s\n%s\n",
"This version of ftp has been enhanced by Luke Mewburn <lukem@netbsd.org>.",
"Execute 'man ftp' for more details");
			} else if (strcasecmp(line, "netbsd") == 0) {
				fprintf(ttyout, "%s\n%s\n",
"NetBSD is a freely available and redistributable UNIX-like operating system.",
"For more information, see http://www.netbsd.org/index.html");
			} else {
				fprintf(ttyout,
				    "`%s' is an interesting topic.\n", line);
			}
			continue;
		}
#endif /* SMALL */

		/*
		 * Check for file:// and http:// URLs.
		 */
		if (strncasecmp(line, HTTP_URL, sizeof(HTTP_URL) - 1) == 0 ||
		    strncasecmp(line, FILE_URL, sizeof(FILE_URL) - 1) == 0) {
			if (url_get(line, outfile) == -1)
				rval = argpos + 1;
			continue;
		}

		/*
		 * Try FTP URL-style arguments next. If ftpproxy is
		 * set, use url_get() instead of standard ftp.
		 * Finally, try host:file.
		 */
		if (strncasecmp(line, FTP_URL, sizeof(FTP_URL) - 1) == 0) {
			url_t urltype;

			if (ftpproxy) {
				if (url_get(line, outfile) == -1)
					rval = argpos + 1;
				continue;
			}
			if ((parse_url(line, "URL", &urltype, &user, &pass,
			    &host, &port, &path) == -1) ||
			    (user != NULL && *user == '\0') ||
			    (pass != NULL && *pass == '\0') ||
			    EMPTYSTRING(host)) {
				warnx("Invalid URL `%s'", argv[argpos]);
				rval = argpos + 1;
				break;
			}
		} else {			/* classic style `host:file' */
			host = xstrdup(line);
			cp = strchr(host, ':');
			if (cp != NULL) {
				*cp = '\0';
				path = xstrdup(cp + 1);
			}
		}
		if (EMPTYSTRING(host)) {
			rval = argpos + 1;
			break;
		}

		/*
		 * Extract the file and (if present) directory name.
		 */
		dir = path;
		if (! EMPTYSTRING(dir)) {
			if (*dir == '/')
				dir++;		/* skip leading / */
			cp = strrchr(dir, '/');
			if (cp != NULL) {
				*cp++ = '\0';
				file = cp;
			} else {
				file = dir;
				dir = NULL;
			}
		}
		if (debug)
			fprintf(ttyout,
"auto_fetch: user `%s', pass `%s', host %s:%d, path, `%s', dir `%s', file `%s'\n",
			    user ? user : "", pass ? pass : "",
			    host ? host : "", ntohs(port), path ? path : "",
			    dir ? dir : "", file ? file : "");

		dirhasglob = filehasglob = 0;
		if (doglob) {
			if (! EMPTYSTRING(dir) &&
			    strpbrk(dir, "*?[]{}") != NULL)
				dirhasglob = 1;
			if (! EMPTYSTRING(file) &&
			    strpbrk(file, "*?[]{}") != NULL)
				filehasglob = 1;
		}

		/*
		 * Set up the connection if we don't have one.
		 */
		if (strcasecmp(host, lasthost) != 0) {
			int oautologin;

			(void)strcpy(lasthost, host);
			if (connected)
				disconnect(0, NULL);
			xargv[0] = __progname;
			xargv[1] = host;
			xargv[2] = NULL;
			xargc = 2;
			if (port) {
				snprintf(portnum, sizeof(portnum),
				    "%d", ntohs(port));
				xargv[2] = portnum;
				xargv[3] = NULL;
				xargc = 3;
			}
			oautologin = autologin;
			if (user != NULL)
				autologin = 0;
			setpeer(xargc, xargv);
			autologin = oautologin;
			if ((connected == 0)
			 || ((connected == 1) &&
			     !ftp_login(host, user, pass)) ) {
				warnx("Can't connect or login to host `%s'",
				    host);
				rval = argpos + 1;
				break;
			}

				/* Always use binary transfers. */
			setbinary(0, NULL);
		} else {
				/* connection exists, cd back to `/' */
			xargv[0] = "cd";
			xargv[1] = "/";
			xargv[2] = NULL;
			dirchange = 0;
			cd(2, xargv);
			if (! dirchange) {
				rval = argpos + 1;
				break;
			}
		}

				/* Change directories, if necessary. */
		if (! EMPTYSTRING(dir) && !dirhasglob) {
			xargv[0] = "cd";
			xargv[1] = dir;
			xargv[2] = NULL;
			dirchange = 0;
			cd(2, xargv);
			if (! dirchange) {
				rval = argpos + 1;
				break;
			}
		}

		if (EMPTYSTRING(file)) {
			rval = -1;
			break;
		}

		if (!verbose)
			fprintf(ttyout, "Retrieving %s/%s\n", dir ? dir : "",
			    file);

		if (dirhasglob) {
			snprintf(rempath, sizeof(rempath), "%s/%s", dir, file);
			file = rempath;
		}

				/* Fetch the file(s). */
		xargc = 2;
		xargv[0] = "get";
		xargv[1] = file;
		xargv[2] = NULL;
		if (dirhasglob || filehasglob) {
			int ointeractive;

			ointeractive = interactive;
			interactive = 0;
			xargv[0] = "mget";
			mget(xargc, xargv);
			interactive = ointeractive;
		} else {
			if (outfile != NULL) {
				xargv[2] = outfile;
				xargv[3] = NULL;
				xargc++;
			}
			get(xargc, xargv);
			if (outfile != NULL && strcmp(outfile, "-") != 0
			    && outfile[0] != '|')
				outfile = NULL;
		}

		if ((code / 100) != COMPLETE)
			rval = argpos + 1;
	}
	if (connected && rval != -1)
		disconnect(0, NULL);
	FREEPTR(host);
	FREEPTR(path);
	FREEPTR(user);
	FREEPTR(pass);
	return (rval);
}
