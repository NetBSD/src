/*	$NetBSD: fetch.c,v 1.230 2018/02/11 02:51:58 christos Exp $	*/

/*-
 * Copyright (c) 1997-2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Scott Aaron Bamford.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Thomas Klausner.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: fetch.c,v 1.230 2018/02/11 02:51:58 christos Exp $");
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

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "ssl.h"
#include "ftp_var.h"
#include "version.h"

typedef enum {
	UNKNOWN_URL_T=-1,
	HTTP_URL_T,
	HTTPS_URL_T,
	FTP_URL_T,
	FILE_URL_T,
	CLASSIC_URL_T
} url_t;

struct authinfo {
	char *auth;
	char *user;
	char *pass;
};

struct urlinfo {
	char *host;
	char *port;
	char *path;
	url_t utype;
	in_port_t portnum;
};

struct posinfo {
	off_t rangestart;
	off_t rangeend;
	off_t entitylen;
};

__dead static void	aborthttp(int);
__dead static void	timeouthttp(int);
#ifndef NO_AUTH
static int	auth_url(const char *, char **, const struct authinfo *);
static void	base64_encode(const unsigned char *, size_t, unsigned char *);
#endif
static int	go_fetch(const char *);
static int	fetch_ftp(const char *);
static int	fetch_url(const char *, const char *, char *, char *);
static const char *match_token(const char **, const char *);
static int	parse_url(const char *, const char *, struct urlinfo *,
    struct authinfo *);
static void	url_decode(char *);
static void	freeauthinfo(struct authinfo *);
static void	freeurlinfo(struct urlinfo *);

static int	redirect_loop;


#define	STRNEQUAL(a,b)	(strncasecmp((a), (b), sizeof((b))-1) == 0)
#define	ISLWS(x)	((x)=='\r' || (x)=='\n' || (x)==' ' || (x)=='\t')
#define	SKIPLWS(x)	do { while (ISLWS((*x))) x++; } while (0)


#define	ABOUT_URL	"about:"	/* propaganda */
#define	FILE_URL	"file://"	/* file URL prefix */
#define	FTP_URL		"ftp://"	/* ftp URL prefix */
#define	HTTP_URL	"http://"	/* http URL prefix */
#ifdef WITH_SSL
#define	HTTPS_URL	"https://"	/* https URL prefix */

#define	IS_HTTP_TYPE(urltype) \
	(((urltype) == HTTP_URL_T) || ((urltype) == HTTPS_URL_T))
#else
#define	IS_HTTP_TYPE(urltype) \
	((urltype) == HTTP_URL_T)
#endif

/*
 * Determine if token is the next word in buf (case insensitive).
 * If so, advance buf past the token and any trailing LWS, and
 * return a pointer to the token (in buf).  Otherwise, return NULL.
 * token may be preceded by LWS.
 * token must be followed by LWS or NUL.  (I.e, don't partial match).
 */
static const char *
match_token(const char **buf, const char *token)
{
	const char	*p, *orig;
	size_t		tlen;

	tlen = strlen(token);
	p = *buf;
	SKIPLWS(p);
	orig = p;
	if (strncasecmp(p, token, tlen) != 0)
		return NULL;
	p += tlen;
	if (*p != '\0' && !ISLWS(*p))
		return NULL;
	SKIPLWS(p);
	orig = *buf;
	*buf = p;
	return orig;
}

static void
initposinfo(struct posinfo *pi)
{
	pi->rangestart = pi->rangeend = pi->entitylen = -1;
}

static void
initauthinfo(struct authinfo *ai, char *auth)
{
	ai->auth = auth;
	ai->user = ai->pass = 0;
}

static void
freeauthinfo(struct authinfo *a)
{
	FREEPTR(a->user);
	if (a->pass != NULL)
		memset(a->pass, 0, strlen(a->pass));
	FREEPTR(a->pass);
}

static void
initurlinfo(struct urlinfo *ui)
{
	ui->host = ui->port = ui->path = 0;
	ui->utype = UNKNOWN_URL_T;
	ui->portnum = 0;
}

static void
copyurlinfo(struct urlinfo *dui, struct urlinfo *sui)
{
	dui->host = ftp_strdup(sui->host);
	dui->port = ftp_strdup(sui->port);
	dui->path = ftp_strdup(sui->path);
	dui->utype = sui->utype;
	dui->portnum = sui->portnum;
}

static void
freeurlinfo(struct urlinfo *ui)
{
	FREEPTR(ui->host);
	FREEPTR(ui->port);
	FREEPTR(ui->path);
}

#ifndef NO_AUTH
/*
 * Generate authorization response based on given authentication challenge.
 * Returns -1 if an error occurred, otherwise 0.
 * Sets response to a malloc(3)ed string; caller should free.
 */
static int
auth_url(const char *challenge, char **response, const struct authinfo *auth)
{
	const char	*cp, *scheme, *errormsg;
	char		*ep, *clear, *realm;
	char		 uuser[BUFSIZ], *gotpass;
	const char	*upass;
	int		 rval;
	size_t		 len, clen, rlen;

	*response = NULL;
	clear = realm = NULL;
	rval = -1;
	cp = challenge;
	scheme = "Basic";	/* only support Basic authentication */
	gotpass = NULL;

	DPRINTF("auth_url: challenge `%s'\n", challenge);

	if (! match_token(&cp, scheme)) {
		warnx("Unsupported authentication challenge `%s'",
		    challenge);
		goto cleanup_auth_url;
	}

#define	REALM "realm=\""
	if (STRNEQUAL(cp, REALM))
		cp += sizeof(REALM) - 1;
	else {
		warnx("Unsupported authentication challenge `%s'",
		    challenge);
		goto cleanup_auth_url;
	}
/* XXX: need to improve quoted-string parsing to support \ quoting, etc. */
	if ((ep = strchr(cp, '\"')) != NULL) {
		len = ep - cp;
		realm = (char *)ftp_malloc(len + 1);
		(void)strlcpy(realm, cp, len + 1);
	} else {
		warnx("Unsupported authentication challenge `%s'",
		    challenge);
		goto cleanup_auth_url;
	}

	fprintf(ttyout, "Username for `%s': ", realm);
	if (auth->user != NULL) {
		(void)strlcpy(uuser, auth->user, sizeof(uuser));
		fprintf(ttyout, "%s\n", uuser);
	} else {
		(void)fflush(ttyout);
		if (get_line(stdin, uuser, sizeof(uuser), &errormsg) < 0) {
			warnx("%s; can't authenticate", errormsg);
			goto cleanup_auth_url;
		}
	}
	if (auth->pass != NULL)
		upass = auth->pass;
	else {
		gotpass = getpass("Password: ");
		if (gotpass == NULL) {
			warnx("Can't read password");
			goto cleanup_auth_url;
		}
		upass = gotpass;
	}

	clen = strlen(uuser) + strlen(upass) + 2;	/* user + ":" + pass + "\0" */
	clear = (char *)ftp_malloc(clen);
	(void)strlcpy(clear, uuser, clen);
	(void)strlcat(clear, ":", clen);
	(void)strlcat(clear, upass, clen);
	if (gotpass)
		memset(gotpass, 0, strlen(gotpass));

						/* scheme + " " + enc + "\0" */
	rlen = strlen(scheme) + 1 + (clen + 2) * 4 / 3 + 1;
	*response = ftp_malloc(rlen);
	(void)strlcpy(*response, scheme, rlen);
	len = strlcat(*response, " ", rlen);
			/* use  `clen - 1'  to not encode the trailing NUL */
	base64_encode((unsigned char *)clear, clen - 1,
	    (unsigned char *)*response + len);
	memset(clear, 0, clen);
	rval = 0;

 cleanup_auth_url:
	FREEPTR(clear);
	FREEPTR(realm);
	return (rval);
}

/*
 * Encode len bytes starting at clear using base64 encoding into encoded,
 * which should be at least ((len + 2) * 4 / 3 + 1) in size.
 */
static void
base64_encode(const unsigned char *clear, size_t len, unsigned char *encoded)
{
	static const unsigned char enc[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned char	*cp;
	size_t	 i;

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
#endif

/*
 * Decode %xx escapes in given string, `in-place'.
 */
static void
url_decode(char *url)
{
	unsigned char *p, *q;

	if (EMPTYSTRING(url))
		return;
	p = q = (unsigned char *)url;

#define	HEXTOINT(x) (x - (isdigit(x) ? '0' : (islower(x) ? 'a' : 'A') - 10))
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
 * Parse URL of form (per RFC 3986):
 *	<type>://[<user>[:<password>]@]<host>[:<port>][/<path>]
 * Returns -1 if a parse error occurred, otherwise 0.
 * It's the caller's responsibility to url_decode() the returned
 * user, pass and path.
 *
 * Sets type to url_t, each of the given char ** pointers to a
 * malloc(3)ed strings of the relevant section, and port to
 * the number given, or ftpport if ftp://, or httpport if http://.
 *
 * XXX: this is not totally RFC 3986 compliant; <path> will have the
 * leading `/' unless it's an ftp:// URL, as this makes things easier
 * for file:// and http:// URLs.  ftp:// URLs have the `/' between the
 * host and the URL-path removed, but any additional leading slashes
 * in the URL-path are retained (because they imply that we should
 * later do "CWD" with a null argument).
 *
 * Examples:
 *	 input URL			 output path
 *	 ---------			 -----------
 *	"http://host"			"/"
 *	"http://host/"			"/"
 *	"http://host/path"		"/path"
 *	"file://host/dir/file"		"dir/file"
 *	"ftp://host"			""
 *	"ftp://host/"			""
 *	"ftp://host//"			"/"
 *	"ftp://host/dir/file"		"dir/file"
 *	"ftp://host//dir/file"		"/dir/file"
 */

static int
parse_url(const char *url, const char *desc, struct urlinfo *ui,
    struct authinfo *auth) 
{
	const char	*origurl, *tport;
	char		*cp, *ep, *thost;
	size_t		 len;

	if (url == NULL || desc == NULL || ui == NULL || auth == NULL)
		errx(1, "parse_url: invoked with NULL argument!");
	DPRINTF("parse_url: %s `%s'\n", desc, url);

	origurl = url;
	tport = NULL;

	if (STRNEQUAL(url, HTTP_URL)) {
		url += sizeof(HTTP_URL) - 1;
		ui->utype = HTTP_URL_T;
		ui->portnum = HTTP_PORT;
		tport = httpport;
	} else if (STRNEQUAL(url, FTP_URL)) {
		url += sizeof(FTP_URL) - 1;
		ui->utype = FTP_URL_T;
		ui->portnum = FTP_PORT;
		tport = ftpport;
	} else if (STRNEQUAL(url, FILE_URL)) {
		url += sizeof(FILE_URL) - 1;
		ui->utype = FILE_URL_T;
		tport = "";
#ifdef WITH_SSL
	} else if (STRNEQUAL(url, HTTPS_URL)) {
		url += sizeof(HTTPS_URL) - 1;
		ui->utype = HTTPS_URL_T;
		ui->portnum = HTTPS_PORT;
		tport = httpsport;
#endif
	} else {
		warnx("Invalid %s `%s'", desc, url);
 cleanup_parse_url:
		freeauthinfo(auth);
		freeurlinfo(ui);
		return (-1);
	}

	if (*url == '\0')
		return (0);

			/* find [user[:pass]@]host[:port] */
	ep = strchr(url, '/');
	if (ep == NULL)
		thost = ftp_strdup(url);
	else {
		len = ep - url;
		thost = (char *)ftp_malloc(len + 1);
		(void)strlcpy(thost, url, len + 1);
		if (ui->utype == FTP_URL_T)	/* skip first / for ftp URLs */
			ep++;
		ui->path = ftp_strdup(ep);
	}

	cp = strchr(thost, '@');	/* look for user[:pass]@ in URLs */
	if (cp != NULL) {
		if (ui->utype == FTP_URL_T)
			anonftp = 0;	/* disable anonftp */
		auth->user = thost;
		*cp = '\0';
		thost = ftp_strdup(cp + 1);
		cp = strchr(auth->user, ':');
		if (cp != NULL) {
			*cp = '\0';
			auth->pass = ftp_strdup(cp + 1);
		}
		url_decode(auth->user);
		if (auth->pass)
			url_decode(auth->pass);
	}

#ifdef INET6
			/*
			 * Check if thost is an encoded IPv6 address, as per
			 * RFC 3986:
			 *	`[' ipv6-address ']'
			 */
	if (*thost == '[') {
		cp = thost + 1;
		if ((ep = strchr(cp, ']')) == NULL ||
		    (ep[1] != '\0' && ep[1] != ':')) {
			warnx("Invalid address `%s' in %s `%s'",
			    thost, desc, origurl);
			goto cleanup_parse_url;
		}
		len = ep - cp;		/* change `[xyz]' -> `xyz' */
		memmove(thost, thost + 1, len);
		thost[len] = '\0';
		if (! isipv6addr(thost)) {
			warnx("Invalid IPv6 address `%s' in %s `%s'",
			    thost, desc, origurl);
			goto cleanup_parse_url;
		}
		cp = ep + 1;
		if (*cp == ':')
			cp++;
		else
			cp = NULL;
	} else
#endif /* INET6 */
		if ((cp = strchr(thost, ':')) != NULL)
			*cp++ = '\0';
	ui->host = thost;

			/* look for [:port] */
	if (cp != NULL) {
		unsigned long	nport;

		nport = strtoul(cp, &ep, 10);
		if (*cp == '\0' || *ep != '\0' ||
		    nport < 1 || nport > MAX_IN_PORT_T) {
			warnx("Unknown port `%s' in %s `%s'",
			    cp, desc, origurl);
			goto cleanup_parse_url;
		}
		ui->portnum = nport;
		tport = cp;
	}

	if (tport != NULL)
		ui->port = ftp_strdup(tport);
	if (ui->path == NULL) {
		const char *emptypath = "/";
		if (ui->utype == FTP_URL_T)	/* skip first / for ftp URLs */
			emptypath++;
		ui->path = ftp_strdup(emptypath);
	}

	DPRINTF("parse_url: user `%s' pass `%s' host %s port %s(%d) "
	    "path `%s'\n",
	    STRorNULL(auth->user), STRorNULL(auth->pass),
	    STRorNULL(ui->host), STRorNULL(ui->port),
	    ui->portnum ? ui->portnum : -1, STRorNULL(ui->path));

	return (0);
}

sigjmp_buf	httpabort;

static int
ftp_socket(const struct urlinfo *ui, void **ssl)
{
	struct addrinfo	hints, *res, *res0 = NULL;
	int error;
	int s;
	const char *host = ui->host;
	const char *port = ui->port;

	if (ui->utype != HTTPS_URL_T)
		ssl = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = 0;
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	error = getaddrinfo(host, port, &hints, &res0);
	if (error) {
		warnx("Can't LOOKUP `%s:%s': %s", host, port,
		    (error == EAI_SYSTEM) ? strerror(errno)
					  : gai_strerror(error));
		return -1;
	}

	if (res0->ai_canonname)
		host = res0->ai_canonname;

	s = -1;
	if (ssl)
		*ssl = NULL;
	for (res = res0; res; res = res->ai_next) {
		char	hname[NI_MAXHOST], sname[NI_MAXSERV];

		ai_unmapped(res);
		if (getnameinfo(res->ai_addr, res->ai_addrlen,
		    hname, sizeof(hname), sname, sizeof(sname),
		    NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
			strlcpy(hname, "?", sizeof(hname));
			strlcpy(sname, "?", sizeof(sname));
		}

		if (verbose && res0->ai_next) {
#ifdef INET6
			if(res->ai_family == AF_INET6) {
				fprintf(ttyout, "Trying [%s]:%s ...\n",
				    hname, sname);
			} else {
#endif
				fprintf(ttyout, "Trying %s:%s ...\n",
				    hname, sname);
#ifdef INET6
			}
#endif
		}

		s = socket(res->ai_family, SOCK_STREAM, res->ai_protocol);
		if (s < 0) {
			warn(
			    "Can't create socket for connection to "
			    "`%s:%s'", hname, sname);
			continue;
		}

		if (ftp_connect(s, res->ai_addr, res->ai_addrlen,
		    verbose || !res->ai_next) < 0) {
			close(s);
			s = -1;
			continue;
		}

#ifdef WITH_SSL
		if (ssl) {
			if ((*ssl = fetch_start_ssl(s, host)) == NULL) {
				close(s);
				s = -1;
				continue;
			}
		}
#endif
		break;
	}
	if (res0)
		freeaddrinfo(res0);
	return s;
}

static int
handle_noproxy(const char *host, in_port_t portnum)
{

	char *cp, *ep, *np, *np_copy, *np_iter, *no_proxy;
	unsigned long np_port;
	size_t hlen, plen;
	int isproxy = 1;

	/* check URL against list of no_proxied sites */
	no_proxy = getoptionvalue("no_proxy");
	if (EMPTYSTRING(no_proxy))
		return isproxy;

	np_iter = np_copy = ftp_strdup(no_proxy);
	hlen = strlen(host);
	while ((cp = strsep(&np_iter, " ,")) != NULL) {
		if (*cp == '\0')
			continue;
		if ((np = strrchr(cp, ':')) != NULL) {
			*np++ =  '\0';
			np_port = strtoul(np, &ep, 10);
			if (*np == '\0' || *ep != '\0')
				continue;
			if (np_port != portnum)
				continue;
		}
		plen = strlen(cp);
		if (hlen < plen)
			continue;
		if (strncasecmp(host + hlen - plen, cp, plen) == 0) {
			isproxy = 0;
			break;
		}
	}
	FREEPTR(np_copy);
	return isproxy;
}

static int
handle_proxy(const char *url, const char *penv, struct urlinfo *ui,
    struct authinfo *pauth)
{
	struct urlinfo pui;

	if (isipv6addr(ui->host) && strchr(ui->host, '%') != NULL) {
		warnx("Scoped address notation `%s' disallowed via web proxy",
		    ui->host);
		return -1;
	}

	initurlinfo(&pui);
	if (parse_url(penv, "proxy URL", &pui, pauth) == -1)
		return -1;

	if ((!IS_HTTP_TYPE(pui.utype) && pui.utype != FTP_URL_T) ||
	    EMPTYSTRING(pui.host) ||
	    (! EMPTYSTRING(pui.path) && strcmp(pui.path, "/") != 0)) {
		warnx("Malformed proxy URL `%s'", penv);
		freeurlinfo(&pui);
		return -1;
	}

	FREEPTR(pui.path);
	pui.path = ftp_strdup(url);

	freeurlinfo(ui);
	*ui = pui;

	return 0;
}

static void
print_host(FETCH *fin, const struct urlinfo *ui)
{
	char *h, *p;

	if (strchr(ui->host, ':') == NULL) {
		fetch_printf(fin, "Host: %s", ui->host);
	} else {
		/*
		 * strip off IPv6 scope identifier, since it is
		 * local to the node
		 */
		h = ftp_strdup(ui->host);
		if (isipv6addr(h) && (p = strchr(h, '%')) != NULL)
			*p = '\0';

		fetch_printf(fin, "Host: [%s]", h);
		free(h);
	}

	if ((ui->utype == HTTP_URL_T && ui->portnum != HTTP_PORT) ||
	    (ui->utype == HTTPS_URL_T && ui->portnum != HTTPS_PORT))
		fetch_printf(fin, ":%u", ui->portnum);
	fetch_printf(fin, "\r\n");
}

static void
print_agent(FETCH *fin)
{
	const char *useragent;
	if ((useragent = getenv("FTPUSERAGENT")) != NULL) {
		fetch_printf(fin, "User-Agent: %s\r\n", useragent);
	} else {
		fetch_printf(fin, "User-Agent: %s/%s\r\n",
		    FTP_PRODUCT, FTP_VERSION);
	}
}

static void
print_cache(FETCH *fin, int isproxy)
{
	fetch_printf(fin, isproxy ?
	    "Pragma: no-cache\r\n" :
	    "Cache-Control: no-cache\r\n");
}

static int
print_get(FETCH *fin, int hasleading, int isproxy, const struct urlinfo *oui,
    const struct urlinfo *ui)
{
	const char *leading = hasleading ? ", " : "  (";

	if (isproxy) {
		if (verbose) {
			fprintf(ttyout, "%svia %s:%u", leading,
			    ui->host, ui->portnum);
			leading = ", ";
			hasleading++;
		}
		fetch_printf(fin, "GET %s HTTP/1.0\r\n", ui->path);
		print_host(fin, oui);
		return hasleading;
	}

	fetch_printf(fin, "GET %s HTTP/1.1\r\n", ui->path);
	print_host(fin, ui);
	fetch_printf(fin, "Accept: */*\r\n");
	fetch_printf(fin, "Connection: close\r\n");
	if (restart_point) {
		fputs(leading, ttyout);
		fetch_printf(fin, "Range: bytes=" LLF "-\r\n",
		    (LLT)restart_point);
		fprintf(ttyout, "restarting at " LLF, (LLT)restart_point);
		hasleading++;
	}
	return hasleading;
}

static void
getmtime(const char *cp, time_t *mtime)
{
	struct tm parsed;
	const char *t;

	memset(&parsed, 0, sizeof(parsed));
	t = parse_rfc2616time(&parsed, cp);

	if (t == NULL)
		return;

	parsed.tm_isdst = -1;
	if (*t == '\0')
		*mtime = timegm(&parsed);

#ifndef NO_DEBUG
	if (ftp_debug && *mtime != -1) {
		fprintf(ttyout, "parsed time as: %s",
		    rfc2822time(localtime(mtime)));
	}
#endif
}

static int
print_proxy(FETCH *fin, int hasleading, const char *wwwauth,
    const char *proxyauth)
{
	const char *leading = hasleading ? ", " : "  (";

	if (wwwauth) {
		if (verbose) {
			fprintf(ttyout, "%swith authorization", leading);
			hasleading++;
		}
		fetch_printf(fin, "Authorization: %s\r\n", wwwauth);
	}
	if (proxyauth) {
		if (verbose) {
			fprintf(ttyout, "%swith proxy authorization", leading);
			hasleading++;
		}
		fetch_printf(fin, "Proxy-Authorization: %s\r\n", proxyauth);
	}
	return hasleading;
}

#ifdef WITH_SSL
static void
print_connect(FETCH *fin, const struct urlinfo *ui)
{
	char hname[NI_MAXHOST], *p;
	const char *h;

	if (isipv6addr(ui->host)) {
		/*
		 * strip off IPv6 scope identifier,
		 * since it is local to the node
		 */
		if ((p = strchr(ui->host, '%')) == NULL)
			snprintf(hname, sizeof(hname), "[%s]", ui->host);
		else
			snprintf(hname, sizeof(hname), "[%.*s]",
			    (int)(p - ui->host), ui->host);
		h = hname;
	} else
		h = ui->host;

	fetch_printf(fin, "CONNECT %s:%d HTTP/1.1\r\n", h, ui->portnum);
	fetch_printf(fin, "Host: %s:%d\r\n", h, ui->portnum);
}
#endif

#define C_OK 0
#define C_CLEANUP 1
#define C_IMPROPER 2

static int
getresponseline(FETCH *fin, char *buf, size_t buflen, int *len)
{
	const char *errormsg;

	alarmtimer(quit_time ? quit_time : 60);
	*len = fetch_getline(fin, buf, buflen, &errormsg);
	alarmtimer(0);
	if (*len < 0) {
		if (*errormsg == '\n')
			errormsg++;
		warnx("Receiving HTTP reply: %s", errormsg);
		return C_CLEANUP;
	}
	while (*len > 0 && (ISLWS(buf[*len-1])))
		buf[--*len] = '\0';

	if (*len)
		DPRINTF("%s: received `%s'\n", __func__, buf);
	return C_OK;
}

static int
getresponse(FETCH *fin, char **cp, size_t buflen, int *hcode)
{
	int len, rv;
	char *ep, *buf = *cp;

	*hcode = 0;
	if ((rv = getresponseline(fin, buf, buflen, &len)) != C_OK)
		return rv;

	/* Determine HTTP response code */
	*cp = strchr(buf, ' ');
	if (*cp == NULL)
		return C_IMPROPER;

	(*cp)++;

	*hcode = strtol(*cp, &ep, 10);
	if (*ep != '\0' && !isspace((unsigned char)*ep))
		return C_IMPROPER;

	return C_OK;
}

static int
parse_posinfo(const char **cp, struct posinfo *pi)
{
	char *ep;
	if (!match_token(cp, "bytes"))
		return -1;

	if (**cp == '*')
		(*cp)++;
	else {
		pi->rangestart = STRTOLL(*cp, &ep, 10);
		if (pi->rangestart < 0 || *ep != '-')
			return -1;
		*cp = ep + 1;
		pi->rangeend = STRTOLL(*cp, &ep, 10);
		if (pi->rangeend < 0 || pi->rangeend < pi->rangestart)
			return -1;
		*cp = ep;
	}
	if (**cp != '/')
		return -1;
	(*cp)++;
	if (**cp == '*')
		(*cp)++;
	else {
		pi->entitylen = STRTOLL(*cp, &ep, 10);
		if (pi->entitylen < 0)
			return -1;
		*cp = ep;
	}
	if (**cp != '\0')
		return -1;

#ifndef NO_DEBUG
	if (ftp_debug) {
		fprintf(ttyout, "parsed range as: ");
		if (pi->rangestart == -1)
			fprintf(ttyout, "*");
		else
			fprintf(ttyout, LLF "-" LLF, (LLT)pi->rangestart,
			    (LLT)pi->rangeend);
		fprintf(ttyout, "/" LLF "\n", (LLT)pi->entitylen);
	}
#endif
	return 0;
}

#ifndef NO_AUTH
static void
do_auth(int hcode, const char *url, const char *penv, struct authinfo *wauth,
    struct authinfo *pauth, char **auth, const char *message,
    volatile int *rval)
{
	struct authinfo aauth;
	char *response;

	if (hcode == 401)
		aauth = *wauth;
	else
		aauth = *pauth;

	if (verbose || aauth.auth == NULL ||
	    aauth.user == NULL || aauth.pass == NULL)
		fprintf(ttyout, "%s\n", message);
	if (EMPTYSTRING(*auth)) {
		warnx("No authentication challenge provided by server");
		return;
	}

	if (aauth.auth != NULL) {
		char reply[10];

		fprintf(ttyout, "Authorization failed. Retry (y/n)? ");
		if (get_line(stdin, reply, sizeof(reply), NULL) < 0) {
			return;
		}
		if (tolower((unsigned char)reply[0]) != 'y')
			return;

		aauth.user = NULL;
		aauth.pass = NULL;
	}

	if (auth_url(*auth, &response, &aauth) == 0) {
		*rval = fetch_url(url, penv,
		    hcode == 401 ? pauth->auth : response,
		    hcode == 401 ? response: wauth->auth);
		memset(response, 0, strlen(response));
		FREEPTR(response);
	}
}
#endif

static int
negotiate_connection(FETCH *fin, const char *url, const char *penv,
    struct posinfo *pi, time_t *mtime, struct authinfo *wauth,
    struct authinfo *pauth, volatile int *rval, volatile int *ischunked,
    char **auth)
{
	int			len, hcode, rv;
	char			buf[FTPBUFLEN], *ep;
	const char		*cp, *token;
	char 			*location, *message;

	*auth = message = location = NULL;

	/* Read the response */
	ep = buf;
	switch (getresponse(fin, &ep, sizeof(buf), &hcode)) {
	case C_CLEANUP:
		goto cleanup_fetch_url;
	case C_IMPROPER:
		goto improper;
	case C_OK:
		message = ftp_strdup(ep);
		break;
	}

	/* Read the rest of the header. */

	for (;;) {
		if ((rv = getresponseline(fin, buf, sizeof(buf), &len)) != C_OK)
			goto cleanup_fetch_url;
		if (len == 0)
			break;

	/*
	 * Look for some headers
	 */

		cp = buf;

		if (match_token(&cp, "Content-Length:")) {
			filesize = STRTOLL(cp, &ep, 10);
			if (filesize < 0 || *ep != '\0')
				goto improper;
			DPRINTF("%s: parsed len as: " LLF "\n",
			    __func__, (LLT)filesize);

		} else if (match_token(&cp, "Content-Range:")) {
			if (parse_posinfo(&cp, pi) == -1)
				goto improper;
			if (! restart_point) {
				warnx(
			    "Received unexpected Content-Range header");
				goto cleanup_fetch_url;
			}

		} else if (match_token(&cp, "Last-Modified:")) {
			getmtime(cp, mtime);

		} else if (match_token(&cp, "Location:")) {
			location = ftp_strdup(cp);
			DPRINTF("%s: parsed location as `%s'\n",
			    __func__, cp);

		} else if (match_token(&cp, "Transfer-Encoding:")) {
			if (match_token(&cp, "binary")) {
				warnx(
		"Bogus transfer encoding `binary' (fetching anyway)");
				continue;
			}
			if (! (token = match_token(&cp, "chunked"))) {
				warnx(
			    "Unsupported transfer encoding `%s'",
				    token);
				goto cleanup_fetch_url;
			}
			(*ischunked)++;
			DPRINTF("%s: using chunked encoding\n",
			    __func__);

		} else if (match_token(&cp, "Proxy-Authenticate:")
			|| match_token(&cp, "WWW-Authenticate:")) {
			if (! (token = match_token(&cp, "Basic"))) {
				DPRINTF("%s: skipping unknown auth "
				    "scheme `%s'\n", __func__, token);
				continue;
			}
			FREEPTR(*auth);
			*auth = ftp_strdup(token);
			DPRINTF("%s: parsed auth as `%s'\n",
			    __func__, cp);
		}

	}
			/* finished parsing header */

	switch (hcode) {
	case 200:
		break;
	case 206:
		if (! restart_point) {
			warnx("Not expecting partial content header");
			goto cleanup_fetch_url;
		}
		break;
	case 300:
	case 301:
	case 302:
	case 303:
	case 305:
	case 307:
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
			*rval = fetch_url(url, location,
			    pauth->auth, wauth->auth);
		} else {
			if (verbose)
				fprintf(ttyout, "Redirected to %s\n",
				    location);
			*rval = go_fetch(location);
		}
		goto cleanup_fetch_url;
#ifndef NO_AUTH
	case 401:
	case 407:
		do_auth(hcode, url, penv, wauth, pauth, auth, message, rval);
		goto cleanup_fetch_url;
#endif
	default:
		if (message)
			warnx("Error retrieving file `%s'", message);
		else
			warnx("Unknown error retrieving file");
		goto cleanup_fetch_url;
	}
	rv = C_OK;
	goto out;

cleanup_fetch_url:
	rv = C_CLEANUP;
	goto out;
improper:
	rv = C_IMPROPER;
	goto out;
out:
	FREEPTR(message);
	FREEPTR(location);
	return rv;
}		/* end of ftp:// or http:// specific setup */

#ifdef WITH_SSL
static int
connectmethod(FETCH *fin, const char *url, const char *penv,
    struct urlinfo *oui, struct urlinfo *ui, struct authinfo *wauth,
    struct authinfo *pauth, char **auth, int *hasleading, volatile int *rval)
{
	void *ssl;
	int hcode, rv;
	const char *cp;
	char buf[FTPBUFLEN], *ep;
	char *message = NULL;

	print_connect(fin, oui);

	print_agent(fin);
	*hasleading = print_proxy(fin, *hasleading, NULL, pauth->auth);

	if (verbose && *hasleading)
		fputs(")\n", ttyout);
	*hasleading = 0;

	fetch_printf(fin, "\r\n");
	if (fetch_flush(fin) == EOF) {
		warn("Writing HTTP request");
		alarmtimer(0);
		goto cleanup_fetch_url;
	}
	alarmtimer(0);

	/* Read the response */
	ep = buf;
	switch (getresponse(fin, &ep, sizeof(buf), &hcode)) {
	case C_CLEANUP:
		goto cleanup_fetch_url;
	case C_IMPROPER:
		goto improper;
	case C_OK:
		message = ftp_strdup(ep);
		break;
	}
		
	for (;;) {
		int len;
		if (getresponseline(fin, buf, sizeof(buf), &len) != C_OK)
			goto cleanup_fetch_url;
		if (len == 0)
			break;

		cp = buf;
		if (match_token(&cp, "Proxy-Authenticate:")) {
			const char *token;
			if (!(token = match_token(&cp, "Basic"))) {
				DPRINTF(
				    "%s: skipping unknown auth scheme `%s'\n",
				    __func__, token);
				continue;
			}
			FREEPTR(*auth);
			*auth = ftp_strdup(token);
			DPRINTF("%s: parsed auth as " "`%s'\n", __func__, cp);
		}
	}

	/* finished parsing header */
	switch (hcode) {
	case 200:
		break;
#ifndef NO_AUTH
	case 407:
		do_auth(hcode, url, penv, wauth, pauth, auth, message, rval);
		goto cleanup_fetch_url;
#endif
	default:
		if (message)
			warnx("Error proxy connect " "`%s'", message);
		else
			warnx("Unknown error proxy " "connect");
		goto cleanup_fetch_url;
	}

	if ((ssl = fetch_start_ssl(fetch_fileno(fin), oui->host)) == NULL)
		goto cleanup_fetch_url;
	fetch_set_ssl(fin, ssl);

	rv = C_OK;
	goto out;
improper:
	rv = C_IMPROPER;
	goto out;
cleanup_fetch_url:
	rv = C_CLEANUP;
	goto out;
out:
	FREEPTR(message);
	return rv;
}
#endif

/*
 * Retrieve URL, via a proxy if necessary, using HTTP.
 * If proxyenv is set, use that for the proxy, otherwise try ftp_proxy or
 * http_proxy/https_proxy as appropriate.
 * Supports HTTP redirects.
 * Returns 1 on failure, 0 on completed xfer, -1 if ftp connection
 * is still open (e.g, ftp xfer with trailing /)
 */
static int
fetch_url(const char *url, const char *proxyenv, char *proxyauth, char *wwwauth)
{
	sigfunc volatile	oldint;
	sigfunc volatile	oldpipe;
	sigfunc volatile	oldalrm;
	sigfunc volatile	oldquit;
	int volatile		s;
	struct stat		sb;
	int volatile		isproxy;
	int volatile 		rval, ischunked;
	size_t			flen;
	static size_t		bufsize;
	static char		*xferbuf;
	const char		*cp;
	char			*ep;
	char			*volatile auth;
	char			*volatile savefile;
	char			*volatile location;
	char			*volatile message;
	char			*volatile decodedpath;
	struct authinfo 	wauth, pauth;
	struct posinfo		pi;
	off_t			hashbytes;
	int			(*volatile closefunc)(FILE *);
	FETCH			*volatile fin;
	FILE			*volatile fout;
	const char		*volatile penv = proxyenv;
	struct urlinfo		ui, oui;
	time_t			mtime;
	void			*ssl = NULL;

	DPRINTF("%s: `%s' proxyenv `%s'\n", __func__, url, STRorNULL(penv));

	oldquit = oldalrm = oldint = oldpipe = NULL;
	closefunc = NULL;
	fin = NULL;
	fout = NULL;
	s = -1;
	savefile = NULL;
	auth = location = message = NULL;
	ischunked = isproxy = 0;
	rval = 1;

	initurlinfo(&ui);
	initurlinfo(&oui);
	initauthinfo(&wauth, wwwauth);
	initauthinfo(&pauth, proxyauth);

	decodedpath = NULL;

	if (sigsetjmp(httpabort, 1))
		goto cleanup_fetch_url;

	if (parse_url(url, "URL", &ui, &wauth) == -1)
		goto cleanup_fetch_url;

	copyurlinfo(&oui, &ui);

	if (ui.utype == FILE_URL_T && ! EMPTYSTRING(ui.host)
	    && strcasecmp(ui.host, "localhost") != 0) {
		warnx("No support for non local file URL `%s'", url);
		goto cleanup_fetch_url;
	}

	if (EMPTYSTRING(ui.path)) {
		if (ui.utype == FTP_URL_T) {
			rval = fetch_ftp(url);
			goto cleanup_fetch_url;
		}
		if (!IS_HTTP_TYPE(ui.utype) || outfile == NULL)  {
			warnx("Invalid URL (no file after host) `%s'", url);
			goto cleanup_fetch_url;
		}
	}

	decodedpath = ftp_strdup(ui.path);
	url_decode(decodedpath);

	if (outfile)
		savefile = outfile;
	else {
		cp = strrchr(decodedpath, '/');		/* find savefile */
		if (cp != NULL)
			savefile = ftp_strdup(cp + 1);
		else
			savefile = ftp_strdup(decodedpath);
		/*
		 * Use the first URL we requested not the name after a
		 * possible redirect, but careful to save it because our
		 * "safety" check is the match to outfile.
		 */
		outfile = ftp_strdup(savefile);
	}
	DPRINTF("%s: savefile `%s'\n", __func__, savefile);
	if (EMPTYSTRING(savefile)) {
		if (ui.utype == FTP_URL_T) {
			rval = fetch_ftp(url);
			goto cleanup_fetch_url;
		}
		warnx("No file after directory (you must specify an "
		    "output file) `%s'", url);
		goto cleanup_fetch_url;
	}

	restart_point = 0;
	filesize = -1;
	initposinfo(&pi);
	mtime = -1;
	if (restartautofetch) {
		if (stat(savefile, &sb) == 0)
			restart_point = sb.st_size;
	}
	if (ui.utype == FILE_URL_T) {		/* file:// URLs */
		direction = "copied";
		fin = fetch_open(decodedpath, "r");
		if (fin == NULL) {
			warn("Can't open `%s'", decodedpath);
			goto cleanup_fetch_url;
		}
		if (fstat(fetch_fileno(fin), &sb) == 0) {
			mtime = sb.st_mtime;
			filesize = sb.st_size;
		}
		if (restart_point) {
			if (lseek(fetch_fileno(fin), restart_point, SEEK_SET) < 0) {
				warn("Can't seek to restart `%s'",
				    decodedpath);
				goto cleanup_fetch_url;
			}
		}
		if (verbose) {
			fprintf(ttyout, "Copying %s", decodedpath);
			if (restart_point)
				fprintf(ttyout, " (restarting at " LLF ")",
				    (LLT)restart_point);
			fputs("\n", ttyout);
		}
		if (0 == rcvbuf_size) {
			rcvbuf_size = 8 * 1024; /* XXX */
		}
	} else {				/* ftp:// or http:// URLs */
		int hasleading;

		if (penv == NULL) {
#ifdef WITH_SSL
			if (ui.utype == HTTPS_URL_T)
				penv = getoptionvalue("https_proxy");
#endif
			if (penv == NULL && IS_HTTP_TYPE(ui.utype))
				penv = getoptionvalue("http_proxy");
			else if (ui.utype == FTP_URL_T)
				penv = getoptionvalue("ftp_proxy");
		}
		direction = "retrieved";
		if (! EMPTYSTRING(penv)) {			/* use proxy */

			isproxy = handle_noproxy(ui.host, ui.portnum);

			if (isproxy == 0 && ui.utype == FTP_URL_T) {
				rval = fetch_ftp(url);
				goto cleanup_fetch_url;
			}

			if (isproxy) {
				if (restart_point) {
					warnx(
					    "Can't restart via proxy URL `%s'",
					    penv);
					goto cleanup_fetch_url;
				}
				if (handle_proxy(url, penv, &ui, &pauth) < 0)
					goto cleanup_fetch_url;
			}
		} /* ! EMPTYSTRING(penv) */

		s = ftp_socket(&ui, &ssl);
		if (s < 0) {
			warnx("Can't connect to `%s:%s'", ui.host, ui.port);
			goto cleanup_fetch_url;
		}

		oldalrm = xsignal(SIGALRM, timeouthttp);
		alarmtimer(quit_time ? quit_time : 60);
		fin = fetch_fdopen(s, "r+");
		fetch_set_ssl(fin, ssl);
		alarmtimer(0);

		alarmtimer(quit_time ? quit_time : 60);
		/*
		 * Construct and send the request.
		 */
		if (verbose)
			fprintf(ttyout, "Requesting %s\n", url);

		hasleading = 0;
#ifdef WITH_SSL
		if (isproxy && oui.utype == HTTPS_URL_T) {
			switch (connectmethod(fin, url, penv, &oui, &ui,
			    &wauth, &pauth, __UNVOLATILE(&auth), &hasleading,
			    &rval)) {
			case C_CLEANUP:
				goto cleanup_fetch_url;
			case C_IMPROPER:
				goto improper;
			case C_OK:
				break;
			default:
				abort();
			}
		}
#endif

		hasleading = print_get(fin, hasleading, isproxy, &oui, &ui);

		if (flushcache)
			print_cache(fin, isproxy);

		print_agent(fin);
		hasleading = print_proxy(fin, hasleading, wauth.auth,
		     auth ? NULL : pauth.auth);
		if (hasleading) {
			hasleading = 0;
			if (verbose)
				fputs(")\n", ttyout);
		}

		fetch_printf(fin, "\r\n");
		if (fetch_flush(fin) == EOF) {
			warn("Writing HTTP request");
			alarmtimer(0);
			goto cleanup_fetch_url;
		}
		alarmtimer(0);

		switch (negotiate_connection(fin, url, penv, &pi,
		    &mtime, &wauth, &pauth, &rval, &ischunked,
		    __UNVOLATILE(&auth))) {
		case C_OK:
			break;
		case C_CLEANUP:
			goto cleanup_fetch_url;
		case C_IMPROPER:
			goto improper;
		default:
			abort();
		}
	}

	/* Open the output file. */

	/*
	 * Only trust filenames with special meaning if they came from
	 * the command line
	 */
	if (outfile == savefile) {
		if (strcmp(savefile, "-") == 0) {
			fout = stdout;
		} else if (*savefile == '|') {
			oldpipe = xsignal(SIGPIPE, SIG_IGN);
			fout = popen(savefile + 1, "w");
			if (fout == NULL) {
				warn("Can't execute `%s'", savefile + 1);
				goto cleanup_fetch_url;
			}
			closefunc = pclose;
		}
	}
	if (fout == NULL) {
		if ((pi.rangeend != -1 && pi.rangeend <= restart_point) ||
		    (pi.rangestart == -1 &&
		    filesize != -1 && filesize <= restart_point)) {
			/* already done */
			if (verbose)
				fprintf(ttyout, "already done\n");
			rval = 0;
			goto cleanup_fetch_url;
		}
		if (restart_point && pi.rangestart != -1) {
			if (pi.entitylen != -1)
				filesize = pi.entitylen;
			if (pi.rangestart != restart_point) {
				warnx(
				    "Size of `%s' differs from save file `%s'",
				    url, savefile);
				goto cleanup_fetch_url;
			}
			fout = fopen(savefile, "a");
		} else
			fout = fopen(savefile, "w");
		if (fout == NULL) {
			warn("Can't open `%s'", savefile);
			goto cleanup_fetch_url;
		}
		closefunc = fclose;
	}

			/* Trap signals */
	oldquit = xsignal(SIGQUIT, psummary);
	oldint = xsignal(SIGINT, aborthttp);

	assert(rcvbuf_size > 0);
	if ((size_t)rcvbuf_size > bufsize) {
		if (xferbuf)
			(void)free(xferbuf);
		bufsize = rcvbuf_size;
		xferbuf = ftp_malloc(bufsize);
	}

	bytes = 0;
	hashbytes = mark;
	if (oldalrm) {
		(void)xsignal(SIGALRM, oldalrm);
		oldalrm = NULL;
	}
	progressmeter(-1);

			/* Finally, suck down the file. */
	do {
		long chunksize;
		short lastchunk;

		chunksize = 0;
		lastchunk = 0;
					/* read chunk-size */
		if (ischunked) {
			if (fetch_getln(xferbuf, bufsize, fin) == NULL) {
				warnx("Unexpected EOF reading chunk-size");
				goto cleanup_fetch_url;
			}
			errno = 0;
			chunksize = strtol(xferbuf, &ep, 16);
			if (ep == xferbuf) {
				warnx("Invalid chunk-size");
				goto cleanup_fetch_url;
			}
			if (errno == ERANGE || chunksize < 0) {
				errno = ERANGE;
				warn("Chunk-size `%.*s'",
				    (int)(ep-xferbuf), xferbuf);
				goto cleanup_fetch_url;
			}

				/*
				 * XXX:	Work around bug in Apache 1.3.9 and
				 *	1.3.11, which incorrectly put trailing
				 *	space after the chunk-size.
				 */
			while (*ep == ' ')
				ep++;

					/* skip [ chunk-ext ] */
			if (*ep == ';') {
				while (*ep && *ep != '\r')
					ep++;
			}

			if (strcmp(ep, "\r\n") != 0) {
				warnx("Unexpected data following chunk-size");
				goto cleanup_fetch_url;
			}
			DPRINTF("%s: got chunk-size of " LLF "\n", __func__,
			    (LLT)chunksize);
			if (chunksize == 0) {
				lastchunk = 1;
				goto chunkdone;
			}
		}
					/* transfer file or chunk */
		while (1) {
			struct timeval then, now, td;
			volatile off_t bufrem;

			if (rate_get)
				(void)gettimeofday(&then, NULL);
			bufrem = rate_get ? rate_get : (off_t)bufsize;
			if (ischunked)
				bufrem = MIN(chunksize, bufrem);
			while (bufrem > 0) {
				flen = fetch_read(xferbuf, sizeof(char),
				    MIN((off_t)bufsize, bufrem), fin);
				if (flen <= 0)
					goto chunkdone;
				bytes += flen;
				bufrem -= flen;
				if (fwrite(xferbuf, sizeof(char), flen, fout)
				    != flen) {
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
				if (ischunked) {
					chunksize -= flen;
					if (chunksize <= 0)
						break;
				}
			}
			if (rate_get) {
				while (1) {
					(void)gettimeofday(&now, NULL);
					timersub(&now, &then, &td);
					if (td.tv_sec > 0)
						break;
					usleep(1000000 - td.tv_usec);
				}
			}
			if (ischunked && chunksize <= 0)
				break;
		}
					/* read CRLF after chunk*/
 chunkdone:
		if (ischunked) {
			if (fetch_getln(xferbuf, bufsize, fin) == NULL) {
				alarmtimer(0);
				warnx("Unexpected EOF reading chunk CRLF");
				goto cleanup_fetch_url;
			}
			if (strcmp(xferbuf, "\r\n") != 0) {
				warnx("Unexpected data following chunk");
				goto cleanup_fetch_url;
			}
			if (lastchunk)
				break;
		}
	} while (ischunked);

/* XXX: deal with optional trailer & CRLF here? */

	if (hash && !progress && bytes > 0) {
		if (bytes < mark)
			(void)putc('#', ttyout);
		(void)putc('\n', ttyout);
	}
	if (fetch_error(fin)) {
		warn("Reading file");
		goto cleanup_fetch_url;
	}
	progressmeter(1);
	(void)fflush(fout);
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
			    rfc2822time(localtime(&mtime)));
		}
	}
	if (bytes > 0)
		ptransfer(0);
	bytes = 0;

	rval = 0;
	goto cleanup_fetch_url;

 improper:
	warnx("Improper response from `%s:%s'", ui.host, ui.port);

 cleanup_fetch_url:
	if (oldint)
		(void)xsignal(SIGINT, oldint);
	if (oldpipe)
		(void)xsignal(SIGPIPE, oldpipe);
	if (oldalrm)
		(void)xsignal(SIGALRM, oldalrm);
	if (oldquit)
		(void)xsignal(SIGQUIT, oldpipe);
	if (fin != NULL)
		fetch_close(fin);
	else if (s != -1)
		close(s);
	if (closefunc != NULL && fout != NULL)
		(*closefunc)(fout);
	if (savefile != outfile)
		FREEPTR(savefile);
	freeurlinfo(&ui);
	freeurlinfo(&oui);
	freeauthinfo(&wauth);
	freeauthinfo(&pauth);
	FREEPTR(decodedpath);
	FREEPTR(auth);
	FREEPTR(location);
	FREEPTR(message);
	return (rval);
}

/*
 * Abort a HTTP retrieval
 */
static void
aborthttp(int notused)
{
	char msgbuf[100];
	int len;

	sigint_raised = 1;
	alarmtimer(0);
	if (fromatty) {
		len = snprintf(msgbuf, sizeof(msgbuf),
		    "\n%s: HTTP fetch aborted.\n", getprogname());
		if (len > 0)
			write(fileno(ttyout), msgbuf, len);
	}
	siglongjmp(httpabort, 1);
}

static void
timeouthttp(int notused)
{
	char msgbuf[100];
	int len;

	alarmtimer(0);
	if (fromatty) {
		len = snprintf(msgbuf, sizeof(msgbuf),
		    "\n%s: HTTP fetch timeout.\n", getprogname());
		if (len > 0)
			write(fileno(ttyout), msgbuf, len);
	}
	siglongjmp(httpabort, 1);
}

/*
 * Retrieve ftp URL or classic ftp argument using FTP.
 * Returns 1 on failure, 0 on completed xfer, -1 if ftp connection
 * is still open (e.g, ftp xfer with trailing /)
 */
static int
fetch_ftp(const char *url)
{
	char		*cp, *xargv[5], rempath[MAXPATHLEN];
	char		*dir, *file;
	char		 cmdbuf[MAXPATHLEN];
	char		 dirbuf[4];
	int		 dirhasglob, filehasglob, rval, transtype, xargc;
	int		 oanonftp, oautologin;
	struct authinfo  auth;
	struct urlinfo	 ui;

	DPRINTF("fetch_ftp: `%s'\n", url);
	dir = file = NULL;
	rval = 1;
	transtype = TYPE_I;

	initurlinfo(&ui);
	initauthinfo(&auth, NULL);

	if (STRNEQUAL(url, FTP_URL)) {
		if ((parse_url(url, "URL", &ui, &auth) == -1) ||
		    (auth.user != NULL && *auth.user == '\0') ||
		    EMPTYSTRING(ui.host)) {
			warnx("Invalid URL `%s'", url);
			goto cleanup_fetch_ftp;
		}
		/*
		 * Note: Don't url_decode(path) here.  We need to keep the
		 * distinction between "/" and "%2F" until later.
		 */

					/* check for trailing ';type=[aid]' */
		if (! EMPTYSTRING(ui.path) && (cp = strrchr(ui.path, ';')) != NULL) {
			if (strcasecmp(cp, ";type=a") == 0)
				transtype = TYPE_A;
			else if (strcasecmp(cp, ";type=i") == 0)
				transtype = TYPE_I;
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
	} else {			/* classic style `[user@]host:[file]' */
		ui.utype = CLASSIC_URL_T;
		ui.host = ftp_strdup(url);
		cp = strchr(ui.host, '@');
		if (cp != NULL) {
			*cp = '\0';
			auth.user = ui.host;
			anonftp = 0;	/* disable anonftp */
			ui.host = ftp_strdup(cp + 1);
		}
		cp = strchr(ui.host, ':');
		if (cp != NULL) {
			*cp = '\0';
			ui.path = ftp_strdup(cp + 1);
		}
	}
	if (EMPTYSTRING(ui.host))
		goto cleanup_fetch_ftp;

			/* Extract the file and (if present) directory name. */
	dir = ui.path;
	if (! EMPTYSTRING(dir)) {
		/*
		 * If we are dealing with classic `[user@]host:[path]' syntax,
		 * then a path of the form `/file' (resulting from input of the
		 * form `host:/file') means that we should do "CWD /" before
		 * retrieving the file.  So we set dir="/" and file="file".
		 *
		 * But if we are dealing with URLs like `ftp://host/path' then
		 * a path of the form `/file' (resulting from a URL of the form
		 * `ftp://host//file') means that we should do `CWD ' (with an
		 * empty argument) before retrieving the file.  So we set
		 * dir="" and file="file".
		 *
		 * If the path does not contain / at all, we set dir=NULL.
		 * (We get a path without any slashes if we are dealing with
		 * classic `[user@]host:[file]' or URL `ftp://host/file'.)
		 *
		 * In all other cases, we set dir to a string that does not
		 * include the final '/' that separates the dir part from the
		 * file part of the path.  (This will be the empty string if
		 * and only if we are dealing with a path of the form `/file'
		 * resulting from an URL of the form `ftp://host//file'.)
		 */
		cp = strrchr(dir, '/');
		if (cp == dir && ui.utype == CLASSIC_URL_T) {
			file = cp + 1;
			(void)strlcpy(dirbuf, "/", sizeof(dirbuf));
			dir = dirbuf;
		} else if (cp != NULL) {
			*cp++ = '\0';
			file = cp;
		} else {
			file = dir;
			dir = NULL;
		}
	} else
		dir = NULL;
	if (ui.utype == FTP_URL_T && file != NULL) {
		url_decode(file);
		/* but still don't url_decode(dir) */
	}
	DPRINTF("fetch_ftp: user `%s' pass `%s' host %s port %s "
	    "path `%s' dir `%s' file `%s'\n",
	    STRorNULL(auth.user), STRorNULL(auth.pass),
	    STRorNULL(ui.host), STRorNULL(ui.port),
	    STRorNULL(ui.path), STRorNULL(dir), STRorNULL(file));

	dirhasglob = filehasglob = 0;
	if (doglob &&
	    (ui.utype == CLASSIC_URL_T || ui.utype == FTP_URL_T)) {
		if (! EMPTYSTRING(dir) && strpbrk(dir, "*?[]{}") != NULL)
			dirhasglob = 1;
		if (! EMPTYSTRING(file) && strpbrk(file, "*?[]{}") != NULL)
			filehasglob = 1;
	}

			/* Set up the connection */
	oanonftp = anonftp;
	if (connected)
		disconnect(0, NULL);
	anonftp = oanonftp;
	(void)strlcpy(cmdbuf, getprogname(), sizeof(cmdbuf));
	xargv[0] = cmdbuf;
	xargv[1] = ui.host;
	xargv[2] = NULL;
	xargc = 2;
	if (ui.port) {
		xargv[2] = ui.port;
		xargv[3] = NULL;
		xargc = 3;
	}
	oautologin = autologin;
		/* don't autologin in setpeer(), use ftp_login() below */
	autologin = 0;
	setpeer(xargc, xargv);
	autologin = oautologin;
	if ((connected == 0) ||
	    (connected == 1 && !ftp_login(ui.host, auth.user, auth.pass))) {
		warnx("Can't connect or login to host `%s:%s'",
			ui.host, ui.port ? ui.port : "?");
		goto cleanup_fetch_ftp;
	}

	switch (transtype) {
	case TYPE_A:
		setascii(1, xargv);
		break;
	case TYPE_I:
		setbinary(1, xargv);
		break;
	default:
		errx(1, "fetch_ftp: unknown transfer type %d", transtype);
	}

		/*
		 * Change directories, if necessary.
		 *
		 * Note: don't use EMPTYSTRING(dir) below, because
		 * dir=="" means something different from dir==NULL.
		 */
	if (dir != NULL && !dirhasglob) {
		char *nextpart;

		/*
		 * If we are dealing with a classic `[user@]host:[path]'
		 * (urltype is CLASSIC_URL_T) then we have a raw directory
		 * name (not encoded in any way) and we can change
		 * directories in one step.
		 *
		 * If we are dealing with an `ftp://host/path' URL
		 * (urltype is FTP_URL_T), then RFC 3986 says we need to
		 * send a separate CWD command for each unescaped "/"
		 * in the path, and we have to interpret %hex escaping
		 * *after* we find the slashes.  It's possible to get
		 * empty components here, (from multiple adjacent
		 * slashes in the path) and RFC 3986 says that we should
		 * still do `CWD ' (with a null argument) in such cases.
		 *
		 * Many ftp servers don't support `CWD ', so if there's an
		 * error performing that command, bail out with a descriptive
		 * message.
		 *
		 * Examples:
		 *
		 * host:			dir="", urltype=CLASSIC_URL_T
		 *		logged in (to default directory)
		 * host:file			dir=NULL, urltype=CLASSIC_URL_T
		 *		"RETR file"
		 * host:dir/			dir="dir", urltype=CLASSIC_URL_T
		 *		"CWD dir", logged in
		 * ftp://host/			dir="", urltype=FTP_URL_T
		 *		logged in (to default directory)
		 * ftp://host/dir/		dir="dir", urltype=FTP_URL_T
		 *		"CWD dir", logged in
		 * ftp://host/file		dir=NULL, urltype=FTP_URL_T
		 *		"RETR file"
		 * ftp://host//file		dir="", urltype=FTP_URL_T
		 *		"CWD ", "RETR file"
		 * host:/file			dir="/", urltype=CLASSIC_URL_T
		 *		"CWD /", "RETR file"
		 * ftp://host///file		dir="/", urltype=FTP_URL_T
		 *		"CWD ", "CWD ", "RETR file"
		 * ftp://host/%2F/file		dir="%2F", urltype=FTP_URL_T
		 *		"CWD /", "RETR file"
		 * ftp://host/foo/file		dir="foo", urltype=FTP_URL_T
		 *		"CWD foo", "RETR file"
		 * ftp://host/foo/bar/file	dir="foo/bar"
		 *		"CWD foo", "CWD bar", "RETR file"
		 * ftp://host//foo/bar/file	dir="/foo/bar"
		 *		"CWD ", "CWD foo", "CWD bar", "RETR file"
		 * ftp://host/foo//bar/file	dir="foo//bar"
		 *		"CWD foo", "CWD ", "CWD bar", "RETR file"
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
			if (ui.utype == FTP_URL_T) {
				nextpart = strchr(dir, '/');
				if (nextpart) {
					*nextpart = '\0';
					nextpart++;
				}
				url_decode(dir);
			} else
				nextpart = NULL;
			DPRINTF("fetch_ftp: dir `%s', nextpart `%s'\n",
			    STRorNULL(dir), STRorNULL(nextpart));
			if (ui.utype == FTP_URL_T || *dir != '\0') {
				(void)strlcpy(cmdbuf, "cd", sizeof(cmdbuf));
				xargv[0] = cmdbuf;
				xargv[1] = dir;
				xargv[2] = NULL;
				dirchange = 0;
				cd(2, xargv);
				if (! dirchange) {
					if (*dir == '\0' && code == 500)
						fprintf(stderr,
"\n"
"ftp: The `CWD ' command (without a directory), which is required by\n"
"     RFC 3986 to support the empty directory in the URL pathname (`//'),\n"
"     conflicts with the server's conformance to RFC 959.\n"
"     Try the same URL without the `//' in the URL pathname.\n"
"\n");
					goto cleanup_fetch_ftp;
				}
			}
			dir = nextpart;
		} while (dir != NULL);
	}

	if (EMPTYSTRING(file)) {
		rval = -1;
		goto cleanup_fetch_ftp;
	}

	if (dirhasglob) {
		(void)strlcpy(rempath, dir,	sizeof(rempath));
		(void)strlcat(rempath, "/",	sizeof(rempath));
		(void)strlcat(rempath, file,	sizeof(rempath));
		file = rempath;
	}

			/* Fetch the file(s). */
	xargc = 2;
	(void)strlcpy(cmdbuf, "get", sizeof(cmdbuf));
	xargv[0] = cmdbuf;
	xargv[1] = file;
	xargv[2] = NULL;
	if (dirhasglob || filehasglob) {
		int ointeractive;

		ointeractive = interactive;
		interactive = 0;
		if (restartautofetch)
			(void)strlcpy(cmdbuf, "mreget", sizeof(cmdbuf));
		else
			(void)strlcpy(cmdbuf, "mget", sizeof(cmdbuf));
		xargv[0] = cmdbuf;
		mget(xargc, xargv);
		interactive = ointeractive;
	} else {
		char *destfile = outfile;
		if (destfile == NULL) {
			cp = strrchr(file, '/');	/* find savefile */
			if (cp != NULL)
				destfile = cp + 1;
			else
				destfile = file;
		}
		xargv[2] = (char *)destfile;
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
	freeurlinfo(&ui);
	freeauthinfo(&auth);
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
go_fetch(const char *url)
{
	char *proxyenv;
	char *p;

#ifndef NO_ABOUT
	/*
	 * Check for about:*
	 */
	if (STRNEQUAL(url, ABOUT_URL)) {
		url += sizeof(ABOUT_URL) -1;
		if (strcasecmp(url, "ftp") == 0 ||
		    strcasecmp(url, "tnftp") == 0) {
			fputs(
"This version of ftp has been enhanced by Luke Mewburn <lukem@NetBSD.org>\n"
"for the NetBSD project.  Execute `man ftp' for more details.\n", ttyout);
		} else if (strcasecmp(url, "lukem") == 0) {
			fputs(
"Luke Mewburn is the author of most of the enhancements in this ftp client.\n"
"Please email feedback to <lukem@NetBSD.org>.\n", ttyout);
		} else if (strcasecmp(url, "netbsd") == 0) {
			fputs(
"NetBSD is a freely available and redistributable UNIX-like operating system.\n"
"For more information, see http://www.NetBSD.org/\n", ttyout);
		} else if (strcasecmp(url, "version") == 0) {
			fprintf(ttyout, "Version: %s %s%s\n",
			    FTP_PRODUCT, FTP_VERSION,
#ifdef INET6
			    ""
#else
			    " (-IPv6)"
#endif
			);
		} else {
			fprintf(ttyout, "`%s' is an interesting topic.\n", url);
		}
		fputs("\n", ttyout);
		return (0);
	}
#endif

	/*
	 * Check for file:// and http:// URLs.
	 */
	if (STRNEQUAL(url, HTTP_URL)
#ifdef WITH_SSL
	    || STRNEQUAL(url, HTTPS_URL)
#endif
	    || STRNEQUAL(url, FILE_URL))
		return (fetch_url(url, NULL, NULL, NULL));

	/*
	 * If it contains "://" but does not begin with ftp://
	 * or something that was already handled, then it's
	 * unsupported.
	 *
	 * If it contains ":" but not "://" then we assume the
	 * part before the colon is a host name, not an URL scheme,
	 * so we don't try to match that here.
	 */
	if ((p = strstr(url, "://")) != NULL && ! STRNEQUAL(url, FTP_URL))
		errx(1, "Unsupported URL scheme `%.*s'", (int)(p - url), url);

	/*
	 * Try FTP URL-style and host:file arguments next.
	 * If ftpproxy is set with an FTP URL, use fetch_url()
	 * Othewise, use fetch_ftp().
	 */
	proxyenv = getoptionvalue("ftp_proxy");
	if (!EMPTYSTRING(proxyenv) && STRNEQUAL(url, FTP_URL))
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
auto_fetch(int argc, char *argv[])
{
	volatile int	argpos, rval;

	argpos = rval = 0;

	if (sigsetjmp(toplevel, 1)) {
		if (connected)
			disconnect(0, NULL);
		if (rval > 0)
			rval = argpos + 1;
		return (rval);
	}
	(void)xsignal(SIGINT, intr);
	(void)xsignal(SIGPIPE, lostpeer);

	/*
	 * Loop through as long as there's files to fetch.
	 */
	for (; (rval == 0) && (argpos < argc); argpos++) {
		if (strchr(argv[argpos], ':') == NULL)
			break;
		redirect_loop = 0;
		if (!anonftp)
			anonftp = 2;	/* Handle "automatic" transfers. */
		rval = go_fetch(argv[argpos]);
		if (outfile != NULL && strcmp(outfile, "-") != 0
		    && outfile[0] != '|') {
			FREEPTR(outfile);
		}
		if (rval > 0)
			rval = argpos + 1;
	}

	if (connected && rval != -1)
		disconnect(0, NULL);
	return (rval);
}


/*
 * Upload multiple files from the command line.
 *
 * If an error occurs the return value will be the offset+1 in
 * argv[] of the file that caused a problem (i.e, argv[x]
 * returns x+1)
 * Otherwise, 0 is returned if all files uploaded successfully.
 */
int
auto_put(int argc, char **argv, const char *uploadserver)
{
	char	*uargv[4], *path, *pathsep;
	int	 uargc, rval, argpos;
	size_t	 len;
	char	 cmdbuf[MAX_C_NAME];

	(void)strlcpy(cmdbuf, "mput", sizeof(cmdbuf));
	uargv[0] = cmdbuf;
	uargv[1] = argv[0];
	uargc = 2;
	uargv[2] = uargv[3] = NULL;
	pathsep = NULL;
	rval = 1;

	DPRINTF("auto_put: target `%s'\n", uploadserver);

	path = ftp_strdup(uploadserver);
	len = strlen(path);
	if (path[len - 1] != '/' && path[len - 1] != ':') {
			/*
			 * make sure we always pass a directory to auto_fetch
			 */
		if (argc > 1) {		/* more than one file to upload */
			len = strlen(uploadserver) + 2;	/* path + "/" + "\0" */
			free(path);
			path = (char *)ftp_malloc(len);
			(void)strlcpy(path, uploadserver, len);
			(void)strlcat(path, "/", len);
		} else {		/* single file to upload */
			(void)strlcpy(cmdbuf, "put", sizeof(cmdbuf));
			uargv[0] = cmdbuf;
			pathsep = strrchr(path, '/');
			if (pathsep == NULL) {
				pathsep = strrchr(path, ':');
				if (pathsep == NULL) {
					warnx("Invalid URL `%s'", path);
					goto cleanup_auto_put;
				}
				pathsep++;
				uargv[2] = ftp_strdup(pathsep);
				pathsep[0] = '/';
			} else
				uargv[2] = ftp_strdup(pathsep + 1);
			pathsep[1] = '\0';
			uargc++;
		}
	}
	DPRINTF("auto_put: URL `%s' argv[2] `%s'\n",
	    path, STRorNULL(uargv[2]));

			/* connect and cwd */
	rval = auto_fetch(1, &path);
	if(rval >= 0)
		goto cleanup_auto_put;

	rval = 0;

			/* target filename provided; upload 1 file */
			/* XXX : is this the best way? */
	if (uargc == 3) {
		uargv[1] = argv[0];
		put(uargc, uargv);
		if ((code / 100) != COMPLETE)
			rval = 1;
	} else {	/* otherwise a target dir: upload all files to it */
		for(argpos = 0; argv[argpos] != NULL; argpos++) {
			uargv[1] = argv[argpos];
			mput(uargc, uargv);
			if ((code / 100) != COMPLETE) {
				rval = argpos + 1;
				break;
			}
		}
	}

 cleanup_auto_put:
	free(path);
	FREEPTR(uargv[2]);
	return (rval);
}
