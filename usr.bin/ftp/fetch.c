/*	$NetBSD: fetch.c,v 1.55 1999/06/02 02:03:57 lukem Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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
__RCSID("$NetBSD: fetch.c,v 1.55 1999/06/02 02:03:57 lukem Exp $");
#endif /* not lint */

/*
 * FTP User Program -- Command line file retrieval
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>

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
	FILE_URL_T,
	CLASSIC_URL_T
} url_t;

void    	aborthttp __P((int));
static int	auth_url __P((const char *, char **, const char *,
				const char *));
static void	base64_encode __P((const char *, size_t, char *));
static int	go_fetch __P((const char *));
static int	fetch_ftp __P((const char *));
static int	fetch_url __P((const char *, const char *, char *, char *));
static int	parse_url __P((const char *, const char *, url_t *, char **,
				char **, char **, in_port_t *, char **));
static void	url_decode __P((char *));

static int	redirect_loop;


#define	ABOUT_URL	"about:"	/* propaganda */
#define	FILE_URL	"file://"	/* file URL prefix */
#define	FTP_URL		"ftp://"	/* ftp URL prefix */
#define	HTTP_URL	"http://"	/* http URL prefix */


#define EMPTYSTRING(x)	((x) == NULL || (*(x) == '\0'))
#define FREEPTR(x)	if ((x) != NULL) { free(x); (x) = NULL; }

/*
 * Generate authorization response based on given authentication challenge.
 * Returns -1 if an error occurred, otherwise 0.
 * Sets response to a malloc(3)ed string; caller should free.
 */
static int
auth_url(challenge, response, guser, gpass)
	const char	 *challenge;
	char		**response;
	const char	 *guser;
	const char	 *gpass;
{
	char		*cp, *ep, *clear, *line, *realm, *scheme;
	char		user[BUFSIZ], *pass;
	int		rval;
	size_t		len;

	*response = NULL;
	clear = realm = scheme = NULL;
	rval = -1;
	line = xstrdup(challenge);
	cp = line;

	if (debug)
		fprintf(ttyout, "auth_url: challenge `%s'\n", challenge);

	scheme = strsep(&cp, " ");
#define SCHEME_BASIC "Basic"
	if (strncasecmp(scheme, SCHEME_BASIC, sizeof(SCHEME_BASIC) - 1) != 0) {
		warnx("Unsupported WWW Authentication challenge - `%s'",
		    challenge);
		goto cleanup_auth_url;
	}
	cp += strspn(cp, " ");

#define REALM "realm=\""
	if (strncasecmp(cp, REALM, sizeof(REALM) - 1) == 0)
		cp += sizeof(REALM) - 1;
	else {
		warnx("Unsupported WWW Authentication challenge - `%s'",
		    challenge);
		goto cleanup_auth_url;
	}
	if ((ep = strchr(cp, '\"')) != NULL) {
		size_t len = ep - cp;

		realm = (char *)xmalloc(len + 1);
		strncpy(realm, cp, len);
		realm[len] = '\0';
	} else {
		warnx("Unsupported WWW Authentication challenge - `%s'",
		    challenge);
		goto cleanup_auth_url;
	}

	if (guser != NULL) {
		strncpy(user, guser, sizeof(user) - 1);
		user[sizeof(user) - 1] = '\0';
	} else {
		fprintf(ttyout, "Username for `%s': ", realm);
		(void)fflush(ttyout);
		if (fgets(user, sizeof(user) - 1, stdin) == NULL)
			goto cleanup_auth_url;
		user[strlen(user) - 1] = '\0';
	}
	if (gpass != NULL)
		pass = (char *)gpass;
	else
		pass = getpass("Password: ");

	len = strlen(user) + strlen(pass) + 1;		/* user + ":" + pass */
	clear = (char *)xmalloc(len + 1);
	snprintf(clear, len, "%s:%s", user, pass);
	if (gpass == NULL)
		memset(pass, '\0', strlen(pass));

						/* scheme + " " + enc */
	len = strlen(scheme) + 1 + (len + 2) * 4 / 3;
	*response = (char *)xmalloc(len + 1);
	len = snprintf(*response, len, "%s ", scheme);
	base64_encode(clear, strlen(clear), *response + len);
	memset(clear, '\0', strlen(clear));
	rval = 0;

cleanup_auth_url:
	FREEPTR(clear);
	FREEPTR(line);
	FREEPTR(realm);
	return (rval);
}

/*
 * Encode len bytes starting at clear using base64 encoding into encoded,
 * which should be at least ((len + 2) * 4 / 3 + 1) in size.
 */
void
base64_encode(clear, len, encoded)
	const char	*clear;
	size_t		 len;
	char		*encoded;
{
	static const char enc[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char *cp;
	int i;

	cp = encoded;
	for (i = 0; i < len; i += 3) {
		*(cp++) = enc[((clear[i + 0] >> 2))];
		*(cp++) = enc[((clear[i + 0] << 4) & 0x30)
			    | ((clear[i + 1] >> 4) & 0x0f)];
		*(cp++) = enc[((clear[i + 1] << 2) & 0x3c)
			    | ((clear[i + 2] >> 6) & 0x03)];
		*(cp++) = enc[((clear[i + 2]     ) & 0x3f)];
	}
	*cp = '\0';
	while (i-- > len)
		*(--cp) = '=';
}

/*
 * Decode %xx escapes in given string, `in-place'.
 */
static void
url_decode(url)
	char *url;
{
	unsigned char *p, *q;

	if (EMPTYSTRING(url))
		return;
	p = q = url;

#define HEXTOINT(x) (x - (isdigit(x) ? '0' : (islower(x) ? 'a' : 'A') - 10))
	while (*p) {
		if (p[0] == '%'
		    && p[1] && isxdigit((unsigned char)p[1])
		    && p[2] && isxdigit((unsigned char)p[2])) {
			*q++ = HEXTOINT(p[1]) * 16 + HEXTOINT(p[2]);
			p+=3;
		} else
			*q++ = *p++;
	}
	*q = '\0';
}


/*
 * Parse URL of form:
 *	<type>://[<user>[:<password>@]]<host>[:<port>]/<url-path>
 * Returns -1 if a parse error occurred, otherwise 0.
 * Only permit [<user>[:<password>@]] for ftp:// URLs
 * It's the caller's responsibility to url_decode() the returned
 * user, pass and path.
 * Sets type to url_t, each of the given char ** pointers to a
 * malloc(3)ed strings of the relevant section, and port to
 * the number given, or ftpport if ftp://, or httpport if http://.
 *
 * XXX: this is not totally RFC 1738 compliant; path will have the
 * leading `/' unless it's an ftp:// URL, as this makes things easier
 * for file:// and http:// URLs. ftp:// URLs have the `/' between the
 * host and the url-path removed, but any additional leading slashes
 * in the url-path are retained (because they imply that we should
 * later do "CWD" with a null argument).
 *
 * Examples:
 *	 input url			 output path
 *	 ---------			 -----------
 *	"http://host"			NULL
 *	"http://host/"			"/"
 *	"http://host/dir/file"		"/dir/file"
 *	"ftp://host/dir/file"		"dir/file"
 *	"ftp://host//dir/file"		"/dir/file"
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
	size_t len;

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
		len = ep - url;
		thost = (char *)xmalloc(len + 1);
		strncpy(thost, url, len);
		thost[len] = '\0';
		if (*type == FTP_URL_T)	/* skip first / for ftp URLs */
			ep++;
		*path = xstrdup(ep);
	}

	cp = strchr(thost, '@');
					/* look for user[:pass]@ in URLs */
	if (cp != NULL) {
		if (*type == FTP_URL_T)
			anonftp = 0;	/* disable anonftp */
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
			warnx("Invalid port `%s' in %s `%s'", cp, desc, url);
			goto cleanup_parse_url;
		}
		*port = htons((in_port_t)nport);
	}

	if (debug)
		fprintf(ttyout,
		    "parse_url: user `%s' pass `%s' host %s:%d path `%s'\n",
		    *user ? *user : "<null>", *pass ? *pass : "<null>",
		    *host ? *host : "<null>", ntohs(*port),
		    *path ? *path : "<null>");

	return (0);
}


jmp_buf	httpabort;

/*
 * Retrieve URL, via a proxy if necessary, using HTTP.
 * If proxyenv is set, use that for the proxy, otherwise try ftp_proxy or
 * http_proxy as appropriate.
 * Supports HTTP redirects.
 * Returns -1 on failure, 0 on completed xfer, 1 if ftp connection
 * is still open (e.g, ftp xfer with trailing /)
 */
static int
fetch_url(url, proxyenv, proxyauth, wwwauth)
	const char	*url;
	const char	*proxyenv;
	char		*proxyauth;
	char		*wwwauth;
{
	struct sockaddr_in	sin;
	struct hostent		*hp;
	volatile sig_t		oldintr, oldintp;
	volatile int		s;
	int 			ischunked, isproxy, rval, hcode;
	size_t			len;
	char			*cp, *ep, *buf, *savefile;
	char			*auth, *location, *message;
	char			*user, *pass, *host, *path, *decodedpath;
	char			*puser, *ppass;
	off_t			hashbytes;
	int			 (*closefunc) __P((FILE *));
	FILE			*fin, *fout;
	time_t			mtime;
	url_t			urltype;
	in_port_t		port;

	closefunc = NULL;
	fin = fout = NULL;
	s = -1;
	buf = savefile = NULL;
	auth = location = message = NULL;
	ischunked = isproxy = hcode = 0;
	rval = 1;
	hp = NULL;
	user = pass = host = path = decodedpath = puser = ppass = NULL;

#ifdef __GNUC__			/* shut up gcc warnings */
	(void)&closefunc;
	(void)&fin;
	(void)&fout;
	(void)&buf;
	(void)&savefile;
	(void)&rval;
	(void)&isproxy;
	(void)&hcode;
	(void)&ischunked;
	(void)&message;
	(void)&location;
	(void)&auth;
	(void)&decodedpath;
#endif

	if (parse_url(url, "URL", &urltype, &user, &pass, &host, &port, &path)
	    == -1)
		goto cleanup_fetch_url;

	if (urltype == FILE_URL_T && ! EMPTYSTRING(host)
	    && strcasecmp(host, "localhost") != 0) {
		warnx("No support for non local file URL `%s'", url);
		goto cleanup_fetch_url;
	}

	if (EMPTYSTRING(path)) {
		if (urltype == FTP_URL_T) {
			rval = fetch_ftp(url);
			goto cleanup_fetch_url;
		}
		if (urltype != HTTP_URL_T || outfile == NULL)  {
			warnx("Invalid URL (no file after host) `%s'", url);
			goto cleanup_fetch_url;
		}
	}

	decodedpath = xstrdup(path);
	url_decode(decodedpath);

	if (outfile)
		savefile = xstrdup(outfile);
	else {
		cp = strrchr(decodedpath, '/');		/* find savefile */
		if (cp != NULL)
			savefile = xstrdup(cp + 1);
		else
			savefile = xstrdup(decodedpath);
	}
	if (EMPTYSTRING(savefile)) {
		if (urltype == FTP_URL_T) {
			rval = fetch_ftp(url);
			goto cleanup_fetch_url;
		}
		warnx("Invalid URL (no file after directory) `%s'", url);
		goto cleanup_fetch_url;
	} else {
		if (debug)
			fprintf(ttyout, "got savefile as `%s'\n", savefile);
	}

	filesize = -1;
	mtime = -1;
	if (urltype == FILE_URL_T) {		/* file:// URLs */
		struct stat sb;

		direction = "copied";
		fin = fopen(decodedpath, "r");
		if (fin == NULL) {
			warn("Cannot open file `%s'", decodedpath);
			goto cleanup_fetch_url;
		}
		if (fstat(fileno(fin), &sb) == 0) {
			mtime = sb.st_mtime;
			filesize = sb.st_size;
		}
		if (verbose)
			fprintf(ttyout, "Copying %s\n", decodedpath);
	} else {				/* ftp:// or http:// URLs */
		if (proxyenv == NULL) {
			if (urltype == HTTP_URL_T)
				proxyenv = httpproxy;
			else if (urltype == FTP_URL_T)
				proxyenv = ftpproxy;
		}
		direction = "retrieved";
		if (proxyenv != NULL) {				/* use proxy */
			url_t purltype;
			char *phost, *ppath;

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
					goto cleanup_fetch_url;

				if ((purltype != HTTP_URL_T
				     && purltype != FTP_URL_T) ||
				    EMPTYSTRING(phost) ||
				    (! EMPTYSTRING(ppath)
				     && strcmp(ppath, "/") != 0)) {
					warnx("Malformed proxy URL `%s'",
					    proxyenv);
					FREEPTR(phost);
					FREEPTR(ppath);
					goto cleanup_fetch_url;
				}

				FREEPTR(host);
				host = phost;
				FREEPTR(path);
				path = xstrdup(url);
				FREEPTR(ppath);
			}
		} /* proxyenv != NULL */

		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;

		if (isdigit((unsigned char)host[0])) {
			if (inet_aton(host, &sin.sin_addr) == 0) {
				warnx("Invalid IP address `%s'", host);
				goto cleanup_fetch_url;
			}
		} else {
			hp = gethostbyname(host);
			if (hp == NULL) {
				warnx("%s: %s", host, hstrerror(h_errno));
				goto cleanup_fetch_url;
			}
			if (hp->h_addrtype != AF_INET) {
				warnx("`%s': not an Internet address?", host);
				goto cleanup_fetch_url;
			}
			memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
		}

		if (port == 0) {
			warnx("Unknown port for URL `%s'", url);
			goto cleanup_fetch_url;
		}
		sin.sin_port = port;

		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s == -1) {
			warn("Can't create socket");
			goto cleanup_fetch_url;
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
				if (verbose)
					fprintf(ttyout, "Trying %s...\n",
					    inet_ntoa(sin.sin_addr));
				(void)close(s);
				s = socket(AF_INET, SOCK_STREAM, 0);
				if (s < 0) {
					warn("Can't create socket");
					goto cleanup_fetch_url;
				}
				continue;
			}
			warn("Can't connect to `%s'", host);
			goto cleanup_fetch_url;
		}

		fin = fdopen(s, "r+");
		/*
		 * Construct and send the request.
		 * Proxy requests don't want leading /.
		 */
		if (isproxy) {
			if (verbose)
				fprintf(ttyout, "Requesting %s\n  (via %s)\n",
				    url, proxyenv);
			fprintf(fin, "GET %s HTTP/1.0\r\n", path);
			if (flushcache)
				fprintf(fin, "Pragma: no-cache\r\n");
		} else {
			struct utsname unam;

			if (verbose)
				fprintf(ttyout, "Requesting %s\n", url);
			fprintf(fin, "GET %s HTTP/1.1\r\n", path);
			fprintf(fin, "Host: %s:%d\r\n", host, ntohs(port));
			fprintf(fin, "Accept: */*\r\n");
			if (uname(&unam) != -1) {
				fprintf(fin, "User-Agent: %s-%s/ftp\r\n",
				    unam.sysname, unam.release);
			}
			fprintf(fin, "Connection: close\r\n");
			if (flushcache)
				fprintf(fin, "Cache-Control: no-cache\r\n");
		}
		if (wwwauth) {
			if (verbose)
				fprintf(ttyout, "  (with authorization)\n");
			fprintf(fin, "Authorization: %s\r\n", wwwauth);
		}
		if (proxyauth) {
			if (verbose)
				fprintf(ttyout,
				    "  (with proxy authorization)\n");
			fprintf(fin, "Proxy-Authorization: %s\r\n", proxyauth);
		}
		fprintf(fin, "\r\n");
		if (fflush(fin) == EOF) {
			warn("Writing HTTP request");
			goto cleanup_fetch_url;
		}

				/* Read the response */
		if ((buf = fparseln(fin, &len, NULL, "\0\0\0", 0)) == NULL) {
			warn("Receiving HTTP reply");
			goto cleanup_fetch_url;
		}
		while (len > 0 && (buf[len-1] == '\r' || buf[len-1] == '\n'))
			buf[--len] = '\0';
		if (debug)
			fprintf(ttyout, "received `%s'\n", buf);

				/* Determine HTTP response code */
		cp = strchr(buf, ' ');
		if (cp == NULL)
			goto improper;
		else
			cp++;
		hcode = strtol(cp, &ep, 10);
		if (*ep != '\0' && !isspace((unsigned char)*ep))
			goto improper;
		message = xstrdup(cp);

				/* Read the rest of the header. */
		FREEPTR(buf);
		while (1) {
			if ((buf = fparseln(fin, &len, NULL, "\0\0\0", 0))
			    == NULL) {
				warn("Receiving HTTP reply");
				goto cleanup_fetch_url;
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
			} else if (strncasecmp(cp, LOCATION,
						sizeof(LOCATION) - 1) == 0) {
				cp += sizeof(LOCATION) - 1;
				location = xstrdup(cp);
				if (debug)
					fprintf(ttyout,
					    "parsed location as: %s\n", cp);

#define TRANSENC "Transfer-Encoding: "
			} else if (strncasecmp(cp, TRANSENC,
						sizeof(TRANSENC) - 1) == 0) {
				cp += sizeof(TRANSENC) - 1;
				if (strcasecmp(cp, "chunked") != 0) {
					warnx(
				    "Unsupported transfer encoding - `%s'",
					    cp);
					goto cleanup_fetch_url;
				}
				ischunked++;
				if (debug)
					fprintf(ttyout,
					    "using chunked encoding\n");

#define PROXYAUTH "Proxy-Authenticate: "
			} else if (strncasecmp(cp, PROXYAUTH,
						sizeof(PROXYAUTH) - 1) == 0) {
				cp += sizeof(PROXYAUTH) - 1;
				FREEPTR(auth);
				auth = xstrdup(cp);
				if (debug)
					fprintf(ttyout,
					    "parsed proxy-auth as: %s\n", cp);

#define WWWAUTH	"WWW-Authenticate: "
			} else if (strncasecmp(cp, WWWAUTH,
			    sizeof(WWWAUTH) - 1) == 0) {
				cp += sizeof(WWWAUTH) - 1;
				FREEPTR(auth);
				auth = xstrdup(cp);
				if (debug)
					fprintf(ttyout,
					    "parsed www-auth as: %s\n", cp);

			}

		}
				/* finished parsing header */
		FREEPTR(buf);

		switch (hcode) {
		case 200:
			break;
		case 300:
		case 301:
		case 302:
		case 303:
		case 305:
			if (EMPTYSTRING(location)) {
				warnx(
				"No redirection Location provided by server");
				goto cleanup_fetch_url;
			}
			if (redirect_loop++ > 5) {
				warnx("Too many redirections requested");
				goto cleanup_fetch_url;
			}
			if (hcode == 305) {
				if (verbose)
					fprintf(ttyout, "Redirected via %s\n",
					    location);
				rval = fetch_url(url, location,
				    proxyauth, wwwauth);
			} else {
				if (verbose)
					fprintf(ttyout, "Redirected to %s\n",
					    location);
				rval = go_fetch(location);
			}
			goto cleanup_fetch_url;
		case 401:
		case 407:
		    {
			char **authp;
			char *auser, *apass;

			fprintf(ttyout, "%s\n", message);
			if (EMPTYSTRING(auth)) {
				warnx(
			    "No authentication challenge provided by server");
				goto cleanup_fetch_url;
			}
			if (hcode == 401) {
				authp = &wwwauth;
				auser = user;
				apass = pass;
			} else {
				authp = &proxyauth;
				auser = puser;
				apass = ppass;
			}
			if (*authp != NULL) {
				char reply[10];

				fprintf(ttyout,
				    "Authorization failed. Retry (y/n)? ");
				if (fgets(reply, sizeof(reply), stdin) != NULL
				    && tolower(reply[0]) != 'y')
					goto cleanup_fetch_url;
				auser = NULL;
				apass = NULL;
			}
			if (auth_url(auth, authp, auser, apass) == 0) {
				rval = fetch_url(url, proxyenv,
				    proxyauth, wwwauth);
				memset(*authp, '\0', strlen(*authp));
				FREEPTR(*authp);
			}
			goto cleanup_fetch_url;
		    }
		default:
			if (message)
				warnx("Error retrieving file - `%s'", message);
			else
				warnx("Unknown error retrieving file");
			goto cleanup_fetch_url;
		}
	}		/* end of ftp:// or http:// specific setup */

	oldintr = oldintp = NULL;

			/* Open the output file. */
	if (strcmp(savefile, "-") == 0) {
		fout = stdout;
	} else if (*savefile == '|') {
		oldintp = signal(SIGPIPE, SIG_IGN);
		fout = popen(savefile + 1, "w");
		if (fout == NULL) {
			warn("Can't run `%s'", savefile + 1);
			goto cleanup_fetch_url;
		}
		closefunc = pclose;
	} else {
		fout = fopen(savefile, "w");
		if (fout == NULL) {
			warn("Can't open `%s'", savefile);
			goto cleanup_fetch_url;
		}
		closefunc = fclose;
	}

			/* Trap signals */
	if (setjmp(httpabort)) {
		if (oldintr)
			(void)signal(SIGINT, oldintr);
		if (oldintp)
			(void)signal(SIGPIPE, oldintp);
		goto cleanup_fetch_url;
	}
	oldintr = signal(SIGINT, aborthttp);

	bytes = 0;
	hashbytes = mark;
	progressmeter(-1);

			/* Finally, suck down the file. */
	buf = xmalloc(BUFSIZ + 1);
	do {
		ssize_t chunksize;

		chunksize = 0;
					/* read chunksize */
		if (ischunked) {
			if (fgets(buf, BUFSIZ, fin) == NULL) {
				warnx("Unexpected EOF reading chunksize");
				goto cleanup_fetch_url;
			}
			chunksize = strtol(buf, &ep, 16);
			if (strcmp(ep, "\r\n") != 0) {
				warnx("Unexpected data following chunksize");
				goto cleanup_fetch_url;
			}
			if (debug)
				fprintf(ttyout, "got chunksize of %qd\n",
				    (long long)chunksize);
			if (chunksize == 0)
				break;
		}
		while ((len = fread(buf, sizeof(char),
		    ischunked ? MIN(chunksize, BUFSIZ) : BUFSIZ, fin)) > 0) {
			bytes += len;
			if (fwrite(buf, sizeof(char), len, fout) != len) {
				warn("Writing `%s'", savefile);
				goto cleanup_fetch_url;
			}
			if (hash && !progress) {
				while (bytes >= hashbytes) {
					(void)putc('#', ttyout);
					hashbytes += mark;
				}
				(void)fflush(ttyout);
			}
			if (ischunked)
				chunksize -= len;
		}
					/* read CRLF after chunk*/
		if (ischunked) {
			if (fgets(buf, BUFSIZ, fin) == NULL)
				break;
			if (strcmp(buf, "\r\n") != 0) {
				warnx("Unexpected data following chunk");
				goto cleanup_fetch_url;
			}
		}
	} while (ischunked);
	if (hash && !progress && bytes > 0) {
		if (bytes < mark)
			(void)putc('#', ttyout);
		(void)putc('\n', ttyout);
	}
	if (ferror(fin)) {
		warn("Reading file");
		goto cleanup_fetch_url;
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

	rval = 0;
	goto cleanup_fetch_url;

improper:
	warnx("Improper response from `%s'", host);

cleanup_fetch_url:
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
	FREEPTR(decodedpath);
	FREEPTR(puser);
	FREEPTR(ppass);
	FREEPTR(buf);
	FREEPTR(auth);
	FREEPTR(location);
	FREEPTR(message);
	return (rval);
}

/*
 * Abort a HTTP retrieval
 */
void
aborthttp(notused)
	int notused;
{

	alarmtimer(0);
	fputs("\nHTTP fetch aborted.\n", ttyout);
	longjmp(httpabort, 1);
}

/*
 * Retrieve ftp URL or classic ftp argument using FTP.
 * Returns -1 on failure, 0 on completed xfer, 1 if ftp connection
 * is still open (e.g, ftp xfer with trailing /)
 */
static int
fetch_ftp(url)
	const char *url;
{
	char		*cp, *xargv[5], rempath[MAXPATHLEN];
	char		portnum[6];		/* large enough for "65535\0" */
	char		*host, *path, *dir, *file, *user, *pass;
	in_port_t	port;
	int		dirhasglob, filehasglob, oautologin, rval, type, xargc;
	url_t		urltype;

	host = path = dir = file = user = pass = NULL;
	port = 0;
	rval = 1;
	type = TYPE_I;

	if (strncasecmp(url, FTP_URL, sizeof(FTP_URL) - 1) == 0) {
		if ((parse_url(url, "URL", &urltype, &user, &pass,
		    &host, &port, &path) == -1) ||
		    (user != NULL && *user == '\0') ||
		    (pass != NULL && *pass == '\0') ||
		    EMPTYSTRING(host)) {
			warnx("Invalid URL `%s'", url);
			goto cleanup_fetch_ftp;
		}
		url_decode(user);
		url_decode(pass);
		/*
		 * Note: Don't url_decode(path) here.  We need to keep the
		 * distinction between "/" and "%2F" until later.
		 */

					/* check for trailing ';type=[aid]' */
		cp = strrchr(path, ';');
		if (cp != NULL) {
			if (strcasecmp(cp, ";type=a") == 0)
				type = TYPE_A;
			else if (strcasecmp(cp, ";type=i") == 0)
				type = TYPE_I;
			else if (strcasecmp(cp, ";type=d") == 0) {
				warnx(
			    "Directory listing via a URL is not supported");
				goto cleanup_fetch_ftp;
			} else {
				warnx("Invalid suffix `%s' in URL `%s'", cp,
				    url);
				goto cleanup_fetch_ftp;
			}
			*cp = 0;
		}
	} else {			/* classic style `host:file' */
		urltype = CLASSIC_URL_T;
		host = xstrdup(url);
		cp = strchr(host, ':');
		if (cp != NULL) {
			*cp = '\0';
			path = xstrdup(cp + 1);
		}
	}
	if (EMPTYSTRING(host))
		goto cleanup_fetch_ftp;

			/* Extract the file and (if present) directory name. */
	dir = path;
	if (! EMPTYSTRING(dir)) {
		/*
		 * If we are dealing with classic `host:path' syntax,
		 * then a path of the form `/file' (resulting from
		 * input of the form `host:/file') means that we should
		 * do "CWD /" before retreiving the file.  So we set
		 * dir="/" and file="file".
		 *
		 * But if we are dealing with URLs like
		 * `ftp://host/path' then a path of the form `/file'
		 * (resulting from a URL of the form `ftp://host//file')
		 * means that we should do `CWD ' (with an empty
		 * argument) before retrieving the file.  So we set
		 * dir="" and file="file".
		 *
		 * If the path does not contain / at all, we set
		 * dir=NULL.  (We get a path without any slashes if
		 * we are dealing with classic `host:file' or URL
		 * `ftp://host/file'.)
		 *
		 * In all other cases, we set dir to a string that does
		 * not include the final '/' that separates the dir part
		 * from the file part of the path.  (This will be the
		 * empty string if and only if we are dealing with a
		 * path of the form `/file' resulting from an URL of the
		 * form `ftp://host//file'.)
		 */
		cp = strrchr(dir, '/');
		if (cp == dir && urltype == CLASSIC_URL_T) {
			file = cp + 1;
			dir = "/";
		} else if (cp != NULL) {
			*cp++ = '\0';
			file = cp;
		} else {
			file = dir;
			dir = NULL;
		}
	}
	if (urltype == FTP_URL_T && file != NULL) {
		url_decode(file);	
		/* but still don't url_decode(dir) */
	}
	if (debug)
		fprintf(ttyout,
    "fetch_ftp: user `%s' pass `%s' host %s:%d path `%s' dir `%s' file `%s'\n",
		    user ? user : "<null>", pass ? pass : "<null>",
		    host ? host : "<null>", ntohs(port), path ? path : "<null>",
		    dir ? dir : "<null>", file ? file : "<null>");

	dirhasglob = filehasglob = 0;
	if (doglob && urltype == CLASSIC_URL_T) {
		if (! EMPTYSTRING(dir) && strpbrk(dir, "*?[]{}") != NULL)
			dirhasglob = 1;
		if (! EMPTYSTRING(file) && strpbrk(file, "*?[]{}") != NULL)
			filehasglob = 1;
	}

			/* Set up the connection */
	if (connected)
		disconnect(0, NULL);
	xargv[0] = __progname;
	xargv[1] = host;
	xargv[2] = NULL;
	xargc = 2;
	if (port) {
		snprintf(portnum, sizeof(portnum), "%d", ntohs(port));
		xargv[2] = portnum;
		xargv[3] = NULL;
		xargc = 3;
	}
	oautologin = autologin;
	if (user != NULL)
		autologin = 0;
	setpeer(xargc, xargv);
	autologin = oautologin;
	if ((connected == 0) || ((connected == 1)
	    && !ftp_login(host, user, pass))) {
		warnx("Can't connect or login to host `%s'", host);
		goto cleanup_fetch_ftp;
	}

	switch (type) {
	case TYPE_A:
		setascii(0, NULL);
		break;
	case TYPE_I:
		setbinary(0, NULL);
		break;
	default:
		errx(1, "fetch_ftp: unknown transfer type %d\n", type);
	}

			/*
			 * Change directories, if necessary.
			 *
			 * Note: don't use EMPTYSTRING(dir) below, because
			 * dir="" means something different from dir=NULL.
			 */
	if (dir != NULL && !dirhasglob) {
		char *nextpart;

		/*
		 * If we are dealing with a classic `host:path' (urltype
		 * is CLASSIC_URL_T) then we have a raw directory
		 * name (not encoded in any way) and we can change
		 * directories in one step.
		 *
		 * If we are dealing with an `ftp://host/path' URL
		 * (urltype is FTP_URL_T), then RFC 1738 says we need to
		 * send a separate CWD command for each unescaped "/"
		 * in the path, and we have to interpret %hex escaping
		 * *after* we find the slashes.  It's possible to get
		 * empty components here, (from multiple adjacent
		 * slashes in the path) and RFC 1738 says that we should
		 * still do `CWD ' (with a null argument) in such cases.
		 *
		 * XXX: many ftp servers don't support `CWD ', so we
		 * just skip the empty components.
		 *
		 * Examples:
		 *
		 * host:file			dir=NULL, urltype=CLASSIC_URL_T
		 *		"RETR file"
		 * ftp://host/file		dir=NULL, urltype=FTP_URL_T
		 *		"RETR file"
		 * ftp://host//file		dir="", urltype=FTP_URL_T
		 *		(no-op), "RETR file"
		 * host:/file			dir="/", urltype=CLASSIC_URL_T
		 *		"CWD /", "RETR file"
		 * ftp://host///file		dir="/", urltype=FTP_URL_T
		 *		(no-op), (no-op), "RETR file"
		 * ftp://host/%2F/file		dir="%2F", urltype=FTP_URL_T
		 *		"CWD /", "RETR file"
		 * ftp://host/foo/file		dir="foo", urltype=FTP_URL_T
		 *		"CWD foo", "RETR file"
		 * ftp://host/foo/bar/file	dir="foo/bar"
		 *		"CWD foo", "CWD bar", "RETR file"
		 * ftp://host//foo/bar/file	dir="/foo/bar"
		 *		(no-op), "CWD foo", "CWD bar", "RETR file"
		 * ftp://host/foo//bar/file	dir="foo//bar"
		 *		"CWD foo", (no-op), "CWD bar", "RETR file"
		 * ftp://host/%2F/foo/bar/file	dir="%2F/foo/bar"
		 *		"CWD /", "CWD foo", "CWD bar", "RETR file"
		 * ftp://host/%2Ffoo/bar/file	dir="%2Ffoo/bar"
		 *		"CWD /foo", "CWD bar", "RETR file"
		 * ftp://host/%2Ffoo%2Fbar/file	dir="%2Ffoo%2Fbar"
		 *		"CWD /foo/bar", "RETR file"
		 * ftp://host/%2Ffoo%2Fbar%2Ffile	dir=NULL
		 *		"RETR /foo/bar/file"
		 *
		 * Note that we don't need `dir' after this point.
		 */
		do {
			if (urltype == FTP_URL_T) {
				nextpart = strchr(dir, '/');
				if (nextpart) {
					*nextpart = '\0';
					nextpart++;
				}
				url_decode(dir);
			} else
				nextpart = NULL;
			if (debug)
				fprintf(ttyout, "dir `%s', nextpart `%s'\n",
				    dir ? dir : "<null>",
				    nextpart ? nextpart : "<null>");
			if (*dir != '\0') {
				xargv[0] = "cd";
				xargv[1] = dir;
				xargv[2] = NULL;
				dirchange = 0;
				cd(2, xargv);
				if (! dirchange)
					goto cleanup_fetch_ftp;
			}
			dir = nextpart;
		} while (dir != NULL);
	}

	if (EMPTYSTRING(file)) {
		rval = -1;
		goto cleanup_fetch_ftp;
	}

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
		if (outfile == NULL) {
			cp = strrchr(file, '/');	/* find savefile */
			if (cp != NULL)
				outfile = cp + 1;
			else
				outfile = file;
		}
		xargv[2] = (char *)outfile;
		xargv[3] = NULL;
		xargc++;
		if (restartautofetch)
			reget(xargc, xargv);
		else
			get(xargc, xargv);
	}

	if ((code / 100) == COMPLETE)
		rval = 0;

cleanup_fetch_ftp:
	FREEPTR(host);
	FREEPTR(path);
	FREEPTR(user);
	FREEPTR(pass);
	return (rval);
}

/*
 * Retrieve the given file to outfile.
 * Supports arguments of the form:
 *	"host:path", "ftp://host/path"	if $ftpproxy, call fetch_url() else
 *					call fetch_ftp()
 *	"http://host/path"		call fetch_url() to use HTTP
 *	"file:///path"			call fetch_url() to copy
 *	"about:..."			print a message
 *
 * Returns 1 on failure, 0 on completed xfer, -1 if ftp connection
 * is still open (e.g, ftp xfer with trailing /)
 */
static int
go_fetch(url)
	const char *url;
{

#ifndef SMALL
	/*
	 * Check for about:*
	 */
	if (strncasecmp(url, ABOUT_URL, sizeof(ABOUT_URL) - 1) == 0) {
		url += sizeof(ABOUT_URL) -1;
		if (strcasecmp(url, "ftp") == 0) {
			fprintf(ttyout, "%s\n%s\n",
"This version of ftp has been enhanced by Luke Mewburn <lukem@netbsd.org>.",
"Execute `man ftp' for more details");
		} else if (strcasecmp(url, "netbsd") == 0) {
			fprintf(ttyout, "%s\n%s\n",
"NetBSD is a freely available and redistributable UNIX-like operating system.",
"For more information, see http://www.netbsd.org/index.html");
		} else {
			fprintf(ttyout, "`%s' is an interesting topic.\n", url);
		}
		return (0);
	}
#endif /* SMALL */

	/*
	 * Check for file:// and http:// URLs.
	 */
	if (strncasecmp(url, HTTP_URL, sizeof(HTTP_URL) - 1) == 0 ||
	    strncasecmp(url, FILE_URL, sizeof(FILE_URL) - 1) == 0)
		return (fetch_url(url, NULL, NULL, NULL));

	/*
	 * Try FTP URL-style and host:file arguments next.
	 * If ftpproxy is set with an FTP URL, use fetch_url()
	 * Othewise, use fetch_ftp().
	 */
	if (ftpproxy && strncasecmp(url, FTP_URL, sizeof(FTP_URL) - 1) == 0)
		return (fetch_url(url, NULL, NULL, NULL));

	return (fetch_ftp(url));
}

/*
 * Retrieve multiple files from the command line,
 * calling go_fetch() for each file.
 *
 * If an ftp path has a trailing "/", the path will be cd-ed into and
 * the connection remains open, and the function will return -1
 * (to indicate the connection is alive).
 * If an error occurs the return value will be the offset+1 in
 * argv[] of the file that caused a problem (i.e, argv[x]
 * returns x+1)
 * Otherwise, 0 is returned if all files retrieved successfully.
 */
int
auto_fetch(argc, argv)
	int argc;
	char *argv[];
{
	volatile int	argpos;
	int		rval;

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
		redirect_loop = 0;
		anonftp = 1;		/* Handle "automatic" transfers. */
		rval = go_fetch(argv[argpos]);
		if (outfile != NULL && strcmp(outfile, "-") != 0
		    && outfile[0] != '|')
			outfile = NULL;
		if (rval > 0)
			rval = argpos + 1;
	}

	if (connected && rval != -1)
		disconnect(0, NULL);
	return (rval);
}
