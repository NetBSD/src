/*	$NetBSD: fetch.c,v 1.25 1998/07/26 12:58:17 lukem Exp $	*/

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
__RCSID("$NetBSD: fetch.c,v 1.25 1998/07/26 12:58:17 lukem Exp $");
#endif /* not lint */

/*
 * FTP User Program -- Command line file retrieval
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

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

static int	url_get __P((const char *, const char *, const char *));
void    	aborthttp __P((int));


#define	ABOUT_URL	"about:"	/* propaganda */
#define	FILE_URL	"file://"	/* file URL prefix */
#define	FTP_URL		"ftp://"	/* ftp URL prefix */
#define	HTTP_URL	"http://"	/* http URL prefix */
#define FTP_PROXY	"ftp_proxy"	/* env var with ftp proxy location */
#define HTTP_PROXY	"http_proxy"	/* env var with http proxy location */


#define EMPTYSTRING(x)	((x) == NULL || (*(x) == '\0'))

jmp_buf	httpabort;

/*
 * Retrieve URL, via the proxy in $proxyvar if necessary.
 * Modifies the string argument given.
 * Returns -1 on failure, 0 on success
 */
static int
url_get(origline, proxyenv, outfile)
	const char *origline;
	const char *proxyenv;
	const char *outfile;
{
	struct sockaddr_in sin;
	int isredirected;
	in_port_t port;
	volatile int s;
	size_t len;
	char *cp, *ep, *portnum, *path;
	const char *savefile;
	char *line, *proxy, *host, *buf;
	volatile sig_t oldintr, oldintp;
	off_t hashbytes;
	struct hostent *hp = NULL;
	int (*closefunc) __P((FILE *));
	FILE *fin, *fout;
	int retval;
	enum { HTTP_URL_T=0, FTP_URL_T, FILE_URL_T } urltype;
	time_t mtime;

	closefunc = NULL;
	fin = fout = NULL;
	s = -1;
	proxy = buf = NULL;
	isredirected = 0;
	retval = -1;

#ifdef __GNUC__			/* to shut up gcc warnings */
	(void)&closefunc;
	(void)&fin;
	(void)&fout;
	(void)&buf;
	(void)&proxy;
	(void)&savefile;
	(void)&retval;
#endif

	line = strdup(origline);
	if (line == NULL)
		errx(1, "Can't allocate memory to parse URL");
	if (strncasecmp(line, HTTP_URL, sizeof(HTTP_URL) - 1) == 0) {
		host = line + sizeof(HTTP_URL) - 1;
		urltype = HTTP_URL_T;
	} else if (strncasecmp(line, FTP_URL, sizeof(FTP_URL) - 1) == 0) {
		host = line + sizeof(FTP_URL) - 1;
		urltype = FTP_URL_T;
	} else if (strncasecmp(line, FILE_URL, sizeof(FILE_URL) - 1) == 0) {
		host = line + sizeof(FILE_URL) - 1;
		urltype = FILE_URL_T;
	} else
		errx(1, "url_get: Invalid URL '%s'", line);

	path = strchr(host, '/');		/* find path */
	if (EMPTYSTRING(path)) {
		switch (urltype) {
		case FILE_URL_T:
			break;
		case FTP_URL_T:
			goto noftpautologin;
		case HTTP_URL_T:
			warnx("Invalid URL (no `/' after host): %s", origline);
			goto cleanup_url_get;
		default:
			errx(1, "Unknown URL type %d\n", urltype);
		}
	}
	*path++ = '\0';
	if (urltype == FILE_URL_T &&
	    *host != '\0' && strcmp(host, "localhost") != 0) {
		warnx("No support for non local file URL: %s", origline);
		goto cleanup_url_get;
	}
	if (EMPTYSTRING(path)) {
		if (urltype == FTP_URL_T)
			goto noftpautologin;
		warnx("Invalid URL (no file after host): %s", origline);
		goto cleanup_url_get;
	}

	if (outfile)
		savefile = outfile;
	else {
		savefile = strrchr(path, '/');		/* find savefile */
		if (savefile != NULL)
			savefile++;
		else
			savefile = path;
	}
	if (EMPTYSTRING(savefile)) {
		if (urltype == FTP_URL_T)
			goto noftpautologin;
		warnx("Invalid URL (no file after directory): %s", origline);
		goto cleanup_url_get;
	}

	filesize = -1;
	mtime = -1;
	if (urltype == FILE_URL_T) {		/* file:// URLs */
		struct stat sb;

		direction = "copied";
		*--path = '/';			/* put / back in */
		fin = fopen(path, "r");
		if (fin == NULL) {
			warn("Cannot open file %s", path);
			goto cleanup_url_get;
		}
		if (fstat(fileno(fin), &sb) == 0) {
			mtime = sb.st_mtime;
			filesize = sb.st_size;
		}
		fprintf(ttyout, "Copying %s\n", path);
	} else {				/* ftp:// or http:// URLs */
		direction = "retrieved";
		if (proxyenv != NULL) {				/* use proxy */
			proxy = strdup(proxyenv);
			if (proxy == NULL)
				errx(1, "Can't allocate memory for proxy URL.");
			if (strncasecmp(proxy, HTTP_URL,
			    sizeof(HTTP_URL) - 1) == 0)
				host = proxy + sizeof(HTTP_URL) - 1;
			else if (strncasecmp(proxy, FTP_URL,
			    sizeof(FTP_URL) - 1) == 0)
				host = proxy + sizeof(FTP_URL) - 1;
			else {
				warnx("Malformed proxy URL: %s", proxyenv);
				goto cleanup_url_get;
			}
			if (EMPTYSTRING(host)) {
				warnx("Malformed proxy URL: %s", proxyenv);
				goto cleanup_url_get;
			}
			*--path = '/';		/* add / back to real path */
			path = strchr(host, '/');
						/* remove trailing / on host */
			if (! EMPTYSTRING(path))
				*path++ = '\0';
			path = line;
		}

		portnum = strchr(host, ':');		/* find portnum */
		if (portnum != NULL)
			*portnum++ = '\0';

		if (debug)
			fprintf(ttyout,
			    "host %s, port %s, path %s, save as %s.\n",
			    host, portnum, path, savefile);

		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;

		if (isdigit((unsigned char)host[0])) {
			if (inet_aton(host, &sin.sin_addr) == 0) {
				warnx("Invalid IP address: %s", host);
				goto cleanup_url_get;
			}
		} else {
			hp = gethostbyname(host);
			if (hp == NULL) {
				warnx("%s: %s", host, hstrerror(h_errno));
				goto cleanup_url_get;
			}
			if (hp->h_addrtype != AF_INET) {
				warnx("%s: not an Internet address?", host);
				goto cleanup_url_get;
			}
			memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
		}

		if (! EMPTYSTRING(portnum)) {
			char *ep;
			long nport;

			nport = strtol(portnum, &ep, 10);
			if (nport < 1 || nport > MAX_IN_PORT_T || *ep != '\0') {
				warnx("Invalid port: %s", portnum);
				goto cleanup_url_get;
			}
			port = htons((in_port_t)nport);
		} else
			port = httpport;
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
				warn("connect to address %s", ia);
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
			warn("Can't connect to %s", host);
			goto cleanup_url_get;
		}

		fin = fdopen(s, "r+");
		/*
		 * Construct and send the request. Proxy requests don't want
		 * leading /.
		 */
		if (!proxy) {
			fprintf(ttyout, "Requesting %s\n", origline);
			fprintf(fin, "GET /%s HTTP/1.1\r\n", path);
			fprintf(fin, "Host: %s\r\n", host);
			fprintf(fin, "Connection: close\r\n\r\n");
		} else {
			fprintf(ttyout, "Requesting %s\n  (via %s)\n",
			    origline, proxyenv);
			fprintf(fin, "GET %s HTTP/1.0\r\n\r\n", path);
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
			fprintf(ttyout, "received '%s'\n", buf);

		cp = strchr(buf, ' ');
		if (cp == NULL)
			goto improper;
		else
			cp++;
		if (strncmp(cp, "301", 3) == 0 || strncmp(cp, "302", 3) == 0) {
			isredirected++;
		} else if (strncmp(cp, "200", 3)) {
			warnx("Error retrieving file: %s", cp);
			goto cleanup_url_get;
		}

				/* Read the rest of the header. */
		free(buf);
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
				fprintf(ttyout, "received '%s'\n", buf);

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
					if (*t == '\0')
						mtime = mktime(&parsed);
					if (debug && mtime != -1)
						fprintf(ttyout,
						    "parsed date as: %s",
						    ctime(&mtime));
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
				resetsockbufsize();
				if (fin != NULL)
					fclose(fin);
				else if (s != -1)
					close(s);
				if (closefunc != NULL && fout != NULL)
					(*closefunc)(fout);
				if (proxy)
					free(proxy);
				free(line);
				retval = url_get(cp, proxyenv, outfile);
				if (buf)
					free(buf);
				return retval;
			}
		}
		free(buf);
	}

	oldintr = oldintp = NULL;

			/* Open the output file. */
	if (strcmp(savefile, "-") == 0) {
		fout = stdout;
	} else if (*savefile == '|') {
		oldintp = signal(SIGPIPE, SIG_IGN);
		fout = popen(savefile + 1, "w");
		if (fout == NULL) {
			warn("Can't run %s", savefile + 1);
			goto cleanup_url_get;
		}
		closefunc = pclose;
	} else {
		fout = fopen(savefile, "w");
		if (fout == NULL) {
			warn("Can't open %s", savefile);
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
	if ((buf = malloc(BUFSIZ)) == NULL)
		errx(1, "Can't allocate memory for transfer buffer\n");
	while ((len = fread(buf, sizeof(char), BUFSIZ, fin)) > 0) {
		bytes += len;
		if (fwrite(buf, sizeof(char), len, fout) != len) {
			warn("Writing %s", savefile);
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
		if (futimes(fileno(fout), tval) == -1) {
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
	warnx("Improper response from %s", host);

cleanup_url_get:
	resetsockbufsize();
	if (fin != NULL)
		fclose(fin);
	else if (s != -1)
		close(s);
	if (closefunc != NULL && fout != NULL)
		(*closefunc)(fout);
	if (buf)
		free(buf);
	if (proxy)
		free(proxy);
	free(line);
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
 * If path has a trailing "/", then return (-1);
 * the path will be cd-ed into and the connection remains open,
 * and the function will return -1 (to indicate the connection
 * is alive).
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
	char *xargv[5];
	char *cp, *line, *host, *dir, *file, *portnum;
	char *user, *pass;
	char *ftpproxy, *httpproxy;
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

	ftpproxy = getenv(FTP_PROXY);
	httpproxy = getenv(HTTP_PROXY);

	/*
	 * Loop through as long as there's files to fetch.
	 */
	for (rval = 0; (rval == 0) && (argpos < argc); free(line), argpos++) {
		if (strchr(argv[argpos], ':') == NULL)
			break;
		host = dir = file = portnum = user = pass = NULL;

		/*
		 * We muck with the string, so we make a copy.
		 */
		line = strdup(argv[argpos]);
		if (line == NULL)
			errx(1, "Can't allocate memory for auto-fetch.");

#ifndef SMALL
		/*
		 * Check for about:*
		 */
		if (strncasecmp(line, ABOUT_URL, sizeof(ABOUT_URL) - 1) == 0) {
			cp = line + sizeof(ABOUT_URL) -1;
			if (strcasecmp(cp, "ftp") == 0) {
				fprintf(ttyout, "%s\n%s\n",
"The version of ftp has been enhanced by Luke Mewburn <lukem@netbsd.org>.",
"Execute 'man ftp' for more details");
			} else if (strcasecmp(cp, "netbsd") == 0) {
				fprintf(ttyout, "%s\n%s\n",
"NetBSD is a freely available and redistributable UNIX-like operating system.",
"For more information, see http://www.netbsd.org/index.html");
			} else {
				fprintf(ttyout,
				    "'%s' is an interesting topic.\n", cp);
			}
			continue;
		}
#endif /* SMALL */

		/*
		 * Check for file:// and http:// URLs.
		 */
		if (strncasecmp(line, HTTP_URL, sizeof(HTTP_URL) - 1) == 0 ||
		    strncasecmp(line, FILE_URL, sizeof(FILE_URL) - 1) == 0) {
			if (url_get(line, httpproxy, outfile) == -1)
				rval = argpos + 1;
			continue;
		}

		/*
		 * Try FTP URL-style arguments next. If ftpproxy is
		 * set, use url_get() instead of standard ftp.
		 * Finally, try host:file.
		 */
		host = line;
		if (strncasecmp(line, FTP_URL, sizeof(FTP_URL) - 1) == 0) {
			if (ftpproxy) {
				if (url_get(line, ftpproxy, outfile) == -1)
					rval = argpos + 1;
				continue;
			}
			host += sizeof(FTP_URL) - 1;
			dir = strchr(host, '/');

				/* look for [user:pass@]host[:port] */
			pass = strpbrk(host, ":@/");
			if (pass == NULL || *pass == '/') {
				pass = NULL;
				goto parsed_url;
			}
			if (pass == host || *pass == '@') {
bad_url:
				warnx("Invalid URL: %s", argv[argpos]);
				rval = argpos + 1;
				continue;
			}
			*pass++ = '\0';
			cp = strpbrk(pass, ":@/");
			if (cp == NULL || *cp == '/') {
				portnum = pass;
				pass = NULL;
				goto parsed_url;
			}
			if (EMPTYSTRING(cp) || *cp == ':')
				goto bad_url;
			*cp++ = '\0';
			user = host;
			if (EMPTYSTRING(user))
				goto bad_url;
			host = cp;
			portnum = strchr(host, ':');
			if (portnum != NULL)
				*portnum++ = '\0';
		} else {			/* classic style `host:file' */
			dir = strchr(host, ':');
		}
parsed_url:
		if (EMPTYSTRING(host)) {
			rval = argpos + 1;
			continue;
		}

		/*
		 * If dir is NULL, the file wasn't specified
		 * (URL looked something like ftp://host)
		 */
		if (dir != NULL)
			*dir++ = '\0';

		/*
		 * Extract the file and (if present) directory name.
		 */
		if (! EMPTYSTRING(dir)) {
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
			    "user %s:%s host %s port %s dir %s file %s\n",
			    user, pass, host, portnum, dir, file);

		/*
		 * Set up the connection if we don't have one.
		 */
		if (strcmp(host, lasthost) != 0) {
			int oautologin;

			(void)strcpy(lasthost, host);
			if (connected)
				disconnect(0, NULL);
			xargv[0] = __progname;
			xargv[1] = host;
			xargv[2] = NULL;
			xargc = 2;
			if (! EMPTYSTRING(portnum)) {
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
				continue;
			}

			/* Always use binary transfers. */
			setbinary(0, NULL);
		}
			/* cd back to '/' */
		xargv[0] = "cd";
		xargv[1] = "/";
		xargv[2] = NULL;
		cd(2, xargv);
		if (! dirchange) {
			rval = argpos + 1;
			continue;
		}

		dirhasglob = filehasglob = 0;
		if (doglob) {
			if (! EMPTYSTRING(dir) &&
			    strpbrk(dir, "*?[]{}") != NULL)
				dirhasglob = 1;
			if (! EMPTYSTRING(file) &&
			    strpbrk(file, "*?[]{}") != NULL)
				filehasglob = 1;
		}

		/* Change directories, if necessary. */
		if (! EMPTYSTRING(dir) && !dirhasglob) {
			xargv[0] = "cd";
			xargv[1] = dir;
			xargv[2] = NULL;
			cd(2, xargv);
			if (! dirchange) {
				rval = argpos + 1;
				continue;
			}
		}

		if (EMPTYSTRING(file)) {
			rval = -1;
			continue;
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
	return (rval);
}
