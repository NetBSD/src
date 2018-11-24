/*	$NetBSD: bozohttpd.c,v 1.86.4.2 2018/11/24 17:13:51 martin Exp $	*/

/*	$eterna: bozohttpd.c,v 1.178 2011/11/18 09:21:15 mrg Exp $	*/

/*
 * Copyright (c) 1997-2018 Matthew R. Green
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
 * RFC 2616 (HTTP/1.1):
 *
 *	- 14.11: content-encoding handling. [1]
 *
 *	- 14.13: content-length handling.  this is only a SHOULD header
 *	  thus we could just not send it ever.  [1]
 *
 *	- 14.17: content-type handling. [1]
 *
 *	- 14.28: if-unmodified-since handling.  if-modified-since is
 *	  done since, shouldn't be too hard for this one.
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
 *	- 14.1/14.2/14.3/14.27: we do not support Accept: headers.
 *	  just ignore them and send the request anyway.  they are
 *	  only SHOULD.
 *
 *	- 14.5/14.16/14.35: only support simple ranges: %d- and %d-%d
 *	  would be nice to support more.
 *
 *	- 14.9: we aren't a cache.
 *
 *	- 14.15: content-md5 would be nice.
 *
 *	- 14.24/14.26/14.27: if-match, if-none-match, if-range.  be
 *	  nice to support this.
 *
 *	- 14.44: Vary: seems unneeded.  ignore it for now.
 */

#ifndef INDEX_HTML
#define INDEX_HTML		"index.html"
#endif
#ifndef SERVER_SOFTWARE
#define SERVER_SOFTWARE		"bozohttpd/20181124"
#endif
#ifndef PUBLIC_HTML
#define PUBLIC_HTML		"public_html"
#endif

#ifndef USE_ARG
#define USE_ARG(x)	/*LINTED*/(void)&(x)
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
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "bozohttpd.h"

#ifndef INITIAL_TIMEOUT
#define	INITIAL_TIMEOUT		"30"	/* wait for 30 seconds initially */
#endif
#ifndef HEADER_WAIT_TIME
#define	HEADER_WAIT_TIME	"10"	/* need more headers every 10 seconds */
#endif
#ifndef TOTAL_MAX_REQ_TIME
#define	TOTAL_MAX_REQ_TIME	"600"	/* must have total request in 600 */
#endif					/* seconds */

/* if monotonic time is not available try real time. */
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC CLOCK_REALTIME
#endif

/* variables and functions */
#ifndef LOG_FTP
#define LOG_FTP LOG_DAEMON
#endif

/*
 * List of special file that we should never serve.
 */
struct {
	const char *file;
	const char *name;
} specials[] = {
	{ DIRECT_ACCESS_FILE, "rejected direct access request" },
	{ REDIRECT_FILE,      "rejected redirect request" },
	{ ABSREDIRECT_FILE,   "rejected absredirect request" },
	{ REMAP_FILE,         "rejected remap request" },
	{ AUTH_FILE,          "rejected authfile request" },
	{ NULL,               NULL },
};

volatile sig_atomic_t	timeout_hit;

/*
 * check there's enough space in the prefs and names arrays.
 */
static int
size_arrays(bozoprefs_t *bozoprefs, size_t needed)
{
	char	**temp;

	if (bozoprefs->size == 0) {
		/* only get here first time around */
		bozoprefs->name = calloc(sizeof(char *), needed);
		if (bozoprefs->name == NULL)
			return 0;
		bozoprefs->value = calloc(sizeof(char *), needed);
		if (bozoprefs->value == NULL) {
			free(bozoprefs->name);
			return 0;
		}
		bozoprefs->size = needed;
	} else if (bozoprefs->count == bozoprefs->size) {
		/* only uses 'needed' when filled array */
		temp = realloc(bozoprefs->name, sizeof(char *) * needed);
		if (temp == NULL)
			return 0;
		bozoprefs->name = temp;
		temp = realloc(bozoprefs->value, sizeof(char *) * needed);
		if (temp == NULL)
			return 0;
		bozoprefs->value = temp;
		bozoprefs->size += needed;
	}
	return 1;
}

static ssize_t
findvar(bozoprefs_t *bozoprefs, const char *name)
{
	size_t	i;

	for (i = 0; i < bozoprefs->count; i++)
		if (strcmp(bozoprefs->name[i], name) == 0)
			return (ssize_t)i;
	return -1;
}

int
bozo_set_pref(bozohttpd_t *httpd, bozoprefs_t *bozoprefs,
	      const char *name, const char *value)
{
	ssize_t	i;

	if ((i = findvar(bozoprefs, name)) < 0) {
		/* add the element to the array */
		if (!size_arrays(bozoprefs, bozoprefs->size + 15))
			return 0;
		i = bozoprefs->count++;
		bozoprefs->name[i] = bozostrdup(httpd, NULL, name);
	} else {
		/* replace the element in the array */
		if (bozoprefs->value[i]) {
			free(bozoprefs->value[i]);
			bozoprefs->value[i] = NULL;
		}
	}
	bozoprefs->value[i] = bozostrdup(httpd, NULL, value);
	return 1;
}

/*
 * get a variable's value, or NULL
 */
char *
bozo_get_pref(bozoprefs_t *bozoprefs, const char *name)
{
	ssize_t	i;

	i = findvar(bozoprefs, name);
	return i < 0 ? NULL : bozoprefs->value[i];
}

char *
bozo_http_date(char *date, size_t datelen)
{
	struct	tm *tm;
	time_t	now;

	/* Sun, 06 Nov 1994 08:49:37 GMT */
	now = time(NULL);
	tm = gmtime(&now);	/* HTTP/1.1 spec rev 06 sez GMT only */
	strftime(date, datelen, "%a, %d %b %Y %H:%M:%S GMT", tm);
	return date;
}

/*
 * convert "in" into the three parts of a request (first line).
 * we allocate into file and query, but return pointers into
 * "in" for proto and method.
 */
static void
parse_request(bozohttpd_t *httpd, char *in, char **method, char **file,
		char **query, char **proto)
{
	ssize_t	len;
	char	*val;

	USE_ARG(httpd);
	debug((httpd, DEBUG_EXPLODING, "parse in: %s", in));
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
	} else {
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
	}

	/* allocate private copies */
	*file = bozostrdup(httpd, NULL, *file);
	if (*query)
		*query = bozostrdup(httpd, NULL, *query);

	debug((httpd, DEBUG_FAT,
		"url: method: \"%s\" file: \"%s\" query: \"%s\" proto: \"%s\"",
		*method, *file, *query, *proto));
}

/*
 * cleanup a bozo_httpreq_t after use
 */
void
bozo_clean_request(bozo_httpreq_t *request)
{
	struct bozoheaders *hdr, *ohdr = NULL;

	if (request == NULL)
		return;

	/* If SSL enabled cleanup SSL structure. */
	bozo_ssl_destroy(request->hr_httpd);

	/* clean up request */
	free(request->hr_remotehost);
	free(request->hr_remoteaddr);
	free(request->hr_serverport);
	free(request->hr_virthostname);
	free(request->hr_file);
	free(request->hr_oldfile);
	free(request->hr_query);
	free(request->hr_host);
	bozo_user_free(request->hr_user);
	bozo_auth_cleanup(request);
	for (hdr = SIMPLEQ_FIRST(&request->hr_headers); hdr;
	    hdr = SIMPLEQ_NEXT(hdr, h_next)) {
		free(hdr->h_value);
		free(hdr->h_header);
		free(ohdr);
		ohdr = hdr;
	}
	free(ohdr);
	ohdr = NULL;
	for (hdr = SIMPLEQ_FIRST(&request->hr_replheaders); hdr;
	    hdr = SIMPLEQ_NEXT(hdr, h_next)) {
		free(hdr->h_value);
		free(hdr->h_header);
		free(ohdr);
		ohdr = hdr;
	}
	free(ohdr);

	free(request);
}

/*
 * send a HTTP/1.1 408 response if we timeout.
 */
/* ARGSUSED */
static void
alarmer(int sig)
{
	timeout_hit = 1;
}


/*
 * set a timeout for "initial", "header", or "request".
 */
int
bozo_set_timeout(bozohttpd_t *httpd, bozoprefs_t *prefs,
		 const char *target, const char *val)
{
	const char *cur, *timeouts[] = {
		"initial timeout",
		"header timeout",
		"request timeout",
		NULL,
	};
	/* adjust minlen if more timeouts appear with conflicting names */
	const size_t minlen = 1;
	size_t len = strlen(target);

	for (cur = timeouts[0]; len >= minlen && *cur; cur++) {
		if (strncmp(target, cur, len) == 0) {
			bozo_set_pref(httpd, prefs, cur, val);
			return 0;
		}
	}
	return 1;
}

/*
 * a list of header quirks: currently, a list of headers that
 * can't be folded into a single line.
 */
const char *header_quirks[] = { "WWW-Authenticate", NULL };

/*
 * add or merge this header (val: str) into the requests list
 */
static bozoheaders_t *
addmerge_header(bozo_httpreq_t *request, struct qheaders *headers,
		const char *val, const char *str, ssize_t len)
{
	struct	bozohttpd_t *httpd = request->hr_httpd;
	struct bozoheaders	 *hdr = NULL;
	const char		**quirk;

	USE_ARG(len);
	for (quirk = header_quirks; *quirk; quirk++)
		if (strcasecmp(*quirk, val) == 0)
			break;

	if (*quirk == NULL) {
		/* do we exist already? */
		SIMPLEQ_FOREACH(hdr, headers, h_next) {
			if (strcasecmp(val, hdr->h_header) == 0)
				break;
		}
	}

	if (hdr) {
		/* yup, merge it in */
		char *nval;

		bozoasprintf(httpd, &nval, "%s, %s", hdr->h_value, str);
		free(hdr->h_value);
		hdr->h_value = nval;
	} else {
		/* nope, create a new one */

		hdr = bozomalloc(httpd, sizeof *hdr);
		hdr->h_header = bozostrdup(httpd, request, val);
		if (str && *str)
			hdr->h_value = bozostrdup(httpd, request, str);
		else
			hdr->h_value = bozostrdup(httpd, request, " ");

		SIMPLEQ_INSERT_TAIL(headers, hdr, h_next);
		request->hr_nheaders++;
	}

	return hdr;
}

bozoheaders_t *
addmerge_reqheader(bozo_httpreq_t *request, const char *val, const char *str,
		   ssize_t len)
{

	return addmerge_header(request, &request->hr_headers, val, str, len);
}

bozoheaders_t *
addmerge_replheader(bozo_httpreq_t *request, const char *val, const char *str,
		    ssize_t len)
{

	return addmerge_header(request, &request->hr_replheaders,
	    val, str, len);
}

/*
 * as the prototype string is not constant (eg, "HTTP/1.1" is equivalent
 * to "HTTP/001.01"), we MUST parse this.
 */
static int
process_proto(bozo_httpreq_t *request, const char *proto)
{
	struct	bozohttpd_t *httpd = request->hr_httpd;
	char	majorstr[16], *minorstr;
	int	majorint, minorint;

	if (proto == NULL) {
got_proto_09:
		request->hr_proto = httpd->consts.http_09;
		debug((httpd, DEBUG_FAT, "request %s is http/0.9",
			request->hr_file));
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
			request->hr_proto = httpd->consts.http_10;
		else if (minorint == 1)
			request->hr_proto = httpd->consts.http_11;
		else
			break;

		debug((httpd, DEBUG_FAT, "request %s is %s",
		    request->hr_file, request->hr_proto));
		SIMPLEQ_INIT(&request->hr_headers);
		request->hr_nheaders = 0;
		return 0;
	}
bad:
	return bozo_http_error(httpd, 404, NULL, "unknown prototype");
}

/*
 * process each type of HTTP method, setting this HTTP requests
 * method type.
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
process_method(bozo_httpreq_t *request, const char *method)
{
	struct	bozohttpd_t *httpd = request->hr_httpd;
	struct	method_map *mmp;

	if (request->hr_proto == httpd->consts.http_11)
		request->hr_allow = "GET, HEAD, POST";

	for (mmp = method_map; mmp->name; mmp++)
		if (strcasecmp(method, mmp->name) == 0) {
			request->hr_method = mmp->type;
			request->hr_methodstr = mmp->name;
			return 0;
		}

	return bozo_http_error(httpd, 404, request, "unknown method");
}

/* check header byte count */
static int
bozo_got_header_length(bozo_httpreq_t *request, size_t len)
{
	request->hr_header_bytes += len;
	if (request->hr_header_bytes < BOZO_HEADERS_MAX_SIZE)
		return 0;

	return bozo_http_error(request->hr_httpd, 413, request,
		"too many headers");
}

/*
 * This function reads a http request from stdin, returning a pointer to a
 * bozo_httpreq_t structure, describing the request.
 */
bozo_httpreq_t *
bozo_read_request(bozohttpd_t *httpd)
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
	bozo_httpreq_t *request;
	struct timespec ots, ts;

	/*
	 * if we're in daemon mode, bozo_daemon_fork() will return here twice
	 * for each call.  once in the child, returning 0, and once in the
	 * parent, returning 1.  for each child, then we can setup SSL, and
	 * the parent can signal the caller there was no request to process
	 * and it will wait for another.
	 */
	if (bozo_daemon_fork(httpd))
		return NULL;
	if (bozo_ssl_accept(httpd))
		return NULL;

	request = bozomalloc(httpd, sizeof(*request));
	memset(request, 0, sizeof(*request));
	request->hr_httpd = httpd;
	request->hr_allow = request->hr_host = NULL;
	request->hr_content_type = request->hr_content_length = NULL;
	request->hr_range = NULL;
	request->hr_last_byte_pos = -1;
	request->hr_if_modified_since = NULL;
	request->hr_virthostname = NULL;
	request->hr_file = NULL;
	request->hr_oldfile = NULL;
	SIMPLEQ_INIT(&request->hr_replheaders);
	bozo_auth_init(request);

	slen = sizeof(ss);
	if (getpeername(0, (struct sockaddr *)(void *)&ss, &slen) < 0)
		host = addr = NULL;
	else {
		if (getnameinfo((struct sockaddr *)(void *)&ss, slen,
		    abuf, sizeof abuf, NULL, 0, NI_NUMERICHOST) == 0)
			addr = abuf;
		else
			addr = NULL;
		if (httpd->numeric == 0 &&
		    getnameinfo((struct sockaddr *)(void *)&ss, slen,
				hbuf, sizeof hbuf, NULL, 0, 0) == 0)
			host = hbuf;
		else
			host = NULL;
	}
	if (host != NULL)
		request->hr_remotehost = bozostrdup(httpd, request, host);
	if (addr != NULL)
		request->hr_remoteaddr = bozostrdup(httpd, request, addr);
	slen = sizeof(ss);

	/*
	 * Override the bound port from the request value, so it works even
	 * if passed through a proxy that doesn't rewrite the port.
	 */
	if (httpd->bindport) {
		if (strcmp(httpd->bindport, "80") != 0)
			port = httpd->bindport;
		else
			port = NULL;
	} else {
		if (getsockname(0, (struct sockaddr *)(void *)&ss, &slen) < 0)
			port = NULL;
		else {
			if (getnameinfo((struct sockaddr *)(void *)&ss, slen,
					NULL, 0, bufport, sizeof bufport,
					NI_NUMERICSERV) == 0)
				port = bufport;
			else
				port = NULL;
		}
	}
	if (port != NULL)
		request->hr_serverport = bozostrdup(httpd, request, port);

	/*
	 * setup a timer to make sure the request is not hung
	 */
	sa.sa_handler = alarmer;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGALRM);
	sa.sa_flags = 0;
	sigaction(SIGALRM, &sa, NULL);

	if (clock_gettime(CLOCK_MONOTONIC, &ots) != 0) {
		bozo_http_error(httpd, 500, NULL, "clock_gettime failed");
		goto cleanup;
	}

	alarm(httpd->initial_timeout);
	while ((str = bozodgetln(httpd, STDIN_FILENO, &len, bozo_read)) != NULL) {
		alarm(0);

		if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
			bozo_http_error(httpd, 500, NULL, "clock_gettime failed");
			goto cleanup;
		}
		/*
		 * don't timeout if old tv_sec is not more than current
		 * tv_sec, or if current tv_sec is less than the request
		 * timeout (these shouldn't happen, but the first could
		 * if monotonic time is not available.)
		 *
		 * the other timeout and header size checks should ensure
		 * that even if time it set backwards or forwards a very
		 * long way, timeout will eventually happen, even if this
		 * one fails.
		 */
		if (ts.tv_sec > ots.tv_sec &&
		    ts.tv_sec > httpd->request_timeout &&
		    ts.tv_sec - httpd->request_timeout > ots.tv_sec)
			timeout_hit = 1;

		if (timeout_hit) {
			bozo_http_error(httpd, 408, NULL, "request timed out");
			goto cleanup;
		}
		line++;

		if (line == 1) {
			if (len < 1) {
				bozo_http_error(httpd, 404, NULL, "null method");
				goto cleanup;
			}
			bozowarn(httpd,
				  "got request ``%s'' from host %s to port %s",
				  str,
				  host ? host : addr ? addr : "<local>",
				  port ? port : "<stdin>");

			/* we allocate return space in file and query only */
			parse_request(httpd, str, &method, &file, &query, &proto);
			request->hr_file = file;
			request->hr_query = query;
			if (method == NULL) {
				bozo_http_error(httpd, 404, NULL, "null method");
				goto cleanup;
			}
			if (file == NULL) {
				bozo_http_error(httpd, 404, NULL, "null file");
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

			debug((httpd, DEBUG_FAT, "got file \"%s\" query \"%s\"",
			    request->hr_file,
			    request->hr_query ? request->hr_query : "<none>"));

			/* http/0.9 has no header processing */
			if (request->hr_proto == httpd->consts.http_09)
				break;
		} else {		/* incoming headers */
			bozoheaders_t *hdr;

			if (*str == '\0')
				break;

			val = bozostrnsep(&str, ":", &len);
			debug((httpd, DEBUG_EXPLODING, "read_req2: after "
			    "bozostrnsep: str `%s' val `%s'", str, val));
			if (val == NULL || len == -1) {
				bozo_http_error(httpd, 404, request, "no header");
				goto cleanup;
			}
			while (*str == ' ' || *str == '\t')
				len--, str++;
			while (*val == ' ' || *val == '\t')
				val++;

			if (bozo_got_header_length(request, len))
				goto cleanup;

			if (bozo_auth_check_headers(request, val, str, len))
				goto next_header;

			hdr = addmerge_reqheader(request, val, str, len);

			if (strcasecmp(hdr->h_header, "content-type") == 0)
				request->hr_content_type = hdr->h_value;
			else if (strcasecmp(hdr->h_header, "content-length") == 0)
				request->hr_content_length = hdr->h_value;
			else if (strcasecmp(hdr->h_header, "host") == 0) {
				if (request->hr_host) {
					/* RFC 7230 (HTTP/1.1): 5.4 */
					bozo_http_error(httpd, 400, request,
						"Only allow one Host: header");
					goto cleanup;
				}
				request->hr_host = bozostrdup(httpd, request,
							      hdr->h_value);
			}
			/* RFC 2616 (HTTP/1.1): 14.20 */
			else if (strcasecmp(hdr->h_header, "expect") == 0) {
				bozo_http_error(httpd, 417, request,
						"we don't support Expect:");
				goto cleanup;
			}
			else if (strcasecmp(hdr->h_header, "referrer") == 0 ||
			         strcasecmp(hdr->h_header, "referer") == 0)
				request->hr_referrer = hdr->h_value;
			else if (strcasecmp(hdr->h_header, "range") == 0)
				request->hr_range = hdr->h_value;
			else if (strcasecmp(hdr->h_header,
					"if-modified-since") == 0)
				request->hr_if_modified_since = hdr->h_value;
			else if (strcasecmp(hdr->h_header,
					"accept-encoding") == 0)
				request->hr_accept_encoding = hdr->h_value;

			debug((httpd, DEBUG_FAT, "adding header %s: %s",
			    hdr->h_header, hdr->h_value));
		}
next_header:
		alarm(httpd->header_timeout);
	}

	/* now, clear it all out */
	alarm(0);
	signal(SIGALRM, SIG_DFL);

	/* RFC1945, 8.3 */
	if (request->hr_method == HTTP_POST &&
	    request->hr_content_length == NULL) {
		bozo_http_error(httpd, 400, request, "missing content length");
		goto cleanup;
	}

	/* RFC 2616 (HTTP/1.1), 14.23 & 19.6.1.1 */
	if (request->hr_proto == httpd->consts.http_11 &&
	    /*(strncasecmp(request->hr_file, "http://", 7) != 0) &&*/
	    request->hr_host == NULL) {
		bozo_http_error(httpd, 400, request, "missing Host header");
		goto cleanup;
	}

	if (request->hr_range != NULL) {
		debug((httpd, DEBUG_FAT, "hr_range: %s", request->hr_range));
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

	debug((httpd, DEBUG_FAT, "bozo_read_request returns url %s in request",
	       request->hr_file));
	return request;

cleanup:
	bozo_clean_request(request);

	return NULL;
}

static int
mmap_and_write_part(bozohttpd_t *httpd, int fd, off_t first_byte_pos, size_t sz)
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
	mappedoffset = first_byte_pos & ~(httpd->page_size - 1);
	mappedsz = (size_t)
		(first_byte_pos - mappedoffset + sz + httpd->page_size - 1) &
		~(httpd->page_size - 1);
	wroffset = (size_t)(first_byte_pos - mappedoffset);

	addr = mmap(0, mappedsz, PROT_READ, MAP_SHARED, fd, mappedoffset);
	if (addr == (char *)-1) {
		bozowarn(httpd, "mmap failed: %s", strerror(errno));
		return -1;
	}
	mappedaddr = addr;

#ifdef MADV_SEQUENTIAL
	(void)madvise(addr, sz, MADV_SEQUENTIAL);
#endif
	while (sz > BOZO_WRSZ) {
		if (bozo_write(httpd, STDOUT_FILENO, addr + wroffset,
				BOZO_WRSZ) != BOZO_WRSZ) {
			bozowarn(httpd, "write failed: %s", strerror(errno));
			goto out;
		}
		debug((httpd, DEBUG_OBESE, "wrote %d bytes", BOZO_WRSZ));
		sz -= BOZO_WRSZ;
		addr += BOZO_WRSZ;
	}
	if (sz && (size_t)bozo_write(httpd, STDOUT_FILENO, addr + wroffset,
				sz) != sz) {
		bozowarn(httpd, "final write failed: %s", strerror(errno));
		goto out;
	}
	debug((httpd, DEBUG_OBESE, "wrote %d bytes", (int)sz));
 out:
	if (munmap(mappedaddr, mappedsz) < 0) {
		bozowarn(httpd, "munmap failed");
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
 * given an url, encode it ala rfc 3986.  ie, escape ? and friends.
 * note that this function returns a static buffer, and thus needs
 * to be updated for any sort of parallel processing. escape only
 * chosen characters for absolute redirects
 */
char *
bozo_escape_rfc3986(bozohttpd_t *httpd, const char *url, int absolute)
{
	static char *buf;
	static size_t buflen = 0;
	size_t len;
	const char *s;
	char *d;

	len = strlen(url);
	if (buflen < len * 3 + 1) {
		buflen = len * 3 + 1;
		buf = bozorealloc(httpd, buf, buflen);
	}

	for (len = 0, s = url, d = buf; *s;) {
		if (*s & 0x80)
			goto encode_it;
		switch (*s) {
		case ':':
		case '?':
		case '#':
		case '[':
		case ']':
		case '@':
		case '!':
		case '$':
		case '&':
		case '\'':
		case '(':
		case ')':
		case '*':
		case '+':
		case ',':
		case ';':
		case '=':
		case '%':
		case '"':
			if (absolute)
				goto leave_it;
		case '\n':
		case '\r':
		case ' ':
		encode_it:
			snprintf(d, 4, "%%%02X", *s++);
			d += 3;
			len += 3;
			break;
		leave_it:
		default:
			*d++ = *s++;
			len++;
			break;
		}
	}
	buf[len] = 0;

	return buf;
}

/*
 * do automatic redirection -- if there are query parameters or userdir for
 * the URL we will tack these on to the new (redirected) URL.
 */
static void
handle_redirect(bozo_httpreq_t *request, const char *url, int absolute)
{
	bozohttpd_t *httpd = request->hr_httpd;
	char *finalurl, *urlbuf;
#ifndef NO_USER_SUPPORT
	char *userbuf;
#endif /* !NO_USER_SUPPORT */
	char portbuf[20];
	const char *scheme, *query, *quest;
	const char *hostname = BOZOHOST(httpd, request);
	int absproto = 0; /* absolute redirect provides own schema */

	if (url == NULL) {
		bozoasprintf(httpd, &urlbuf, "/%s/", request->hr_file);
		url = urlbuf;
	} else
		urlbuf = NULL;

#ifndef NO_USER_SUPPORT
	if (request->hr_user && !absolute) {
		bozoasprintf(httpd, &userbuf, "/~%s%s", request->hr_user, url);
		url = userbuf;
	} else
		userbuf = NULL;
#endif /* !NO_USER_SUPPORT */

	if (absolute) {
		char *sep = NULL;
		const char *s;

		/*
		 * absolute redirect may specify own protocol i.e. to redirect
		 * to another schema like https:// or ftp://.
		 * Details: RFC 3986, section 3.
		 */

		/* 1. check if url contains :// */
		sep = strstr(url, "://");

		/*
		 * RFC 3986, section 3.1:
		 * scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
		 */
		if (sep) {
			for (s = url; s != sep;) {
				if (!isalnum((int)*s) &&
				    *s != '+' && *s != '-' && *s != '.')
					break;
				if (++s == sep) {
					absproto = 1;
				}
			}
		}
	}

	/* construct final redirection url */

	scheme = absproto ? "" : httpd->sslinfo ? "https://" : "http://";

	if (absolute) {
		hostname = "";
		portbuf[0] = '\0';
	} else {
		const char *defport = httpd->sslinfo ? "443" : "80";

		if (request->hr_serverport &&
		    strcmp(request->hr_serverport, defport) != 0)
			snprintf(portbuf, sizeof(portbuf), ":%s",
			    request->hr_serverport);
		else
			portbuf[0] = '\0';
	}

	url = bozo_escape_rfc3986(httpd, url, absolute);

	if (request->hr_query && strlen(request->hr_query)) {
		query = request->hr_query;
		quest = "?";
	} else {
		query = quest = "";
	}

	bozoasprintf(httpd, &finalurl, "%s%s%s%s%s%s",
		     scheme, hostname, portbuf, url, quest, query);

	bozowarn(httpd, "redirecting %s", finalurl);
	debug((httpd, DEBUG_FAT, "redirecting %s", finalurl));

	bozo_printf(httpd, "%s 301 Document Moved\r\n", request->hr_proto);
	if (request->hr_proto != httpd->consts.http_09)
		bozo_print_header(request, NULL, "text/html", NULL);
	if (request->hr_proto != httpd->consts.http_09)
		bozo_printf(httpd, "Location: %s\r\n", finalurl);
	bozo_printf(httpd, "\r\n");
	if (request->hr_method == HTTP_HEAD)
		goto head;
	bozo_printf(httpd, "<html><head><title>Document Moved</title></head>\n");
	bozo_printf(httpd, "<body><h1>Document Moved</h1>\n");
	bozo_printf(httpd, "This document had moved <a href=\"%s\">here</a>\n",
	  finalurl);
	bozo_printf(httpd, "</body></html>\n");
head:
	bozo_flush(httpd, stdout);
	free(urlbuf);
	free(finalurl);
#ifndef NO_USER_SUPPORT
	free(userbuf);
#endif /* !NO_USER_SUPPORT */
}

/*
 * Like strncmp(), but s_esc may contain characters escaped by \.
 * The len argument does not include the backslashes used for escaping,
 * that is: it gives the raw len, after unescaping the string.
 */
static int
esccmp(const char *s_plain, const char *s_esc, size_t len)
{
	bool esc = false;

	while (len) {
		if (!esc && *s_esc == '\\') {
			esc = true;
			s_esc++;
			continue;
		}
		esc = false;
		if (*s_plain == 0 || *s_esc == 0 || *s_plain != *s_esc)
			return *s_esc - *s_plain;
		s_esc++;
		s_plain++; 
		len--;
	}
	return 0;
}

/*
 * Check if the request refers to a uri that is mapped via a .bzremap.
 * We have  /requested/path:/re/mapped/to/this.html lines in there,
 * and the : separator may be use in the left hand side escaped with
 * \ to encode a path containig a : character.
 */
static void
check_remap(bozo_httpreq_t *request)
{
	bozohttpd_t *httpd = request->hr_httpd;
	char *file = request->hr_file, *newfile;
	void *fmap;
	const char *replace, *map_to, *p;
	struct stat st;
	int mapfile;
	size_t avail, len, rlen, reqlen, num_esc = 0;
	bool escaped = false;

	mapfile = open(REMAP_FILE, O_RDONLY, 0);
	if (mapfile == -1)
		return;
	debug((httpd, DEBUG_FAT, "remap file found"));
	if (fstat(mapfile, &st) == -1) {
		bozowarn(httpd, "could not stat " REMAP_FILE ", errno: %d",
		    errno);
		goto out;
	}

	fmap = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, mapfile, 0);
	if (fmap == NULL) {
		bozowarn(httpd, "could not mmap " REMAP_FILE ", error %d",
		    errno);
		goto out;
	}
	reqlen = strlen(file);
	for (p = fmap, avail = st.st_size; avail; ) {
		/*
		 * We have lines like:
		 *   /this/url:/replacement/that/url
		 * If we find a matching left hand side, replace will point
		 * to it and len will be its length. map_to will point to
		 * the right hand side and rlen wil be its length.
		 * If we have no match, both pointers will be NULL.
		 */

		/* skip empty lines */
		while ((*p == '\r' || *p == '\n') && avail) {
			p++;
			avail--;
		}
		replace = p;
		escaped = false;
		while (avail) {
			if (*p == '\r' || *p == '\n')
				break;
			if (!escaped && *p == ':')
				break;
			if (escaped) {
				escaped = false;
				num_esc++;
			} else if (*p == '\\') {
				escaped = true;
			}
			p++;
			avail--;
		}
		if (!avail || *p != ':') {
			replace = NULL;
			map_to = NULL;
			break;
		}
		len = p - replace - num_esc;
		/*
		 * reqlen < len: the left hand side is too long, can't be a
		 *   match
		 * reqlen == len: full string has to match
		 * reqlen > len: make sure there is a path separator at 'len'
		 * avail < 2: we are at eof, missing right hand side
		 */
		if (avail < 2 || reqlen < len || 
		    (reqlen == len && esccmp(file, replace, len) != 0) ||
		    (reqlen > len && (file[len] != '/' ||
					esccmp(file, replace, len) != 0))) {

			/* non-match, skip to end of line and continue */
			while (*p != '\r' && *p != '\n' && avail) {
				p++;
				avail--;
			}
			replace = NULL;
			map_to = NULL;
			continue;
		}
		p++;
		avail--;

		/* found a match, parse the target */
		map_to = p;
		while (*p != '\r' && *p != '\n' && avail) {
			p++;
			avail--;
		}
		rlen = p - map_to;
		break;
	}

	if (replace && map_to) {
		newfile = bozomalloc(httpd, strlen(file) + rlen - len + 1);
		memcpy(newfile, map_to, rlen);
		strcpy(newfile+rlen, file + len);
		debug((httpd, DEBUG_NORMAL, "remapping found '%s'",
		    newfile));
		free(request->hr_file);
		request->hr_file = newfile;
	}

	munmap(fmap, st.st_size);
out:
	close(mapfile);
}

/*
 * deal with virtual host names; we do this:
 *	if we have a virtual path root (httpd->virtbase), and we are given a
 *	virtual host spec (Host: ho.st or http://ho.st/), see if this
 *	directory exists under httpd->virtbase.  if it does, use this as the
 #	new slashdir.
 */
static int
check_virtual(bozo_httpreq_t *request)
{
	bozohttpd_t *httpd = request->hr_httpd;
	char *file = request->hr_file, *s;
	size_t len;

	/*
	 * convert http://virtual.host/ to request->hr_host
	 */
	debug((httpd, DEBUG_OBESE,
	       "checking for http:// virtual host in '%s'", file));
	if (strncasecmp(file, "http://", 7) == 0) {
		/* we would do virtual hosting here? */
		file += 7;
		/* RFC 2616 (HTTP/1.1), 5.2: URI takes precedence over Host: */
		free(request->hr_host);
		request->hr_host = bozostrdup(httpd, request, file);
		if ((s = strchr(request->hr_host, '/')) != NULL)
			*s = '\0';
		s = strchr(file, '/');
		free(request->hr_file);
		request->hr_file = bozostrdup(httpd, request, s ? s : "/");
		debug((httpd, DEBUG_OBESE, "got host '%s' file is now '%s'",
		    request->hr_host, request->hr_file));
	} else if (!request->hr_host)
		goto use_slashdir;

	/*
	 * canonicalise hr_host - that is, remove any :80.
	 */
	len = strlen(request->hr_host);
	if (len > 3 && strcmp(request->hr_host + len - 3, ":80") == 0) {
		request->hr_host[len - 3] = '\0';
		len = strlen(request->hr_host);
	}

	if (!httpd->virtbase) {
		/*
		 * if we don't use vhost support, then set virthostname if
		 * user supplied Host header. It will be used for possible
		 * redirections
		 */
		if (request->hr_host) {
			s = strrchr(request->hr_host, ':');
			if (s != NULL)
				/* truncate Host: as we want to copy it without port part */
				*s = '\0';
			request->hr_virthostname = bozostrdup(httpd, request,
			  request->hr_host);
			if (s != NULL)
				/* fix Host: again, if we truncated it */
				*s = ':';
		}
		goto use_slashdir;
	}
	
	/*
	 * ok, we have a virtual host, use opendir(3) to find a case
	 * insensitive match for the virtual host we are asked for.
	 * note that if the virtual host is the same as the master,
	 * we don't need to do anything special.
	 */
	debug((httpd, DEBUG_OBESE,
	    "check_virtual: checking host `%s' under httpd->virtbase `%s' "
	    "for file `%s'",
	    request->hr_host, httpd->virtbase, request->hr_file));
	if (strncasecmp(httpd->virthostname, request->hr_host, len) != 0) {
		s = NULL;
		DIR *dirp;
		struct dirent *d;

		if ((dirp = opendir(httpd->virtbase)) != NULL) {
			while ((d = readdir(dirp)) != NULL) {
				if (strcmp(d->d_name, ".") == 0 ||
				    strcmp(d->d_name, "..") == 0) {
					continue;
				}
				debug((httpd, DEBUG_OBESE, "looking at dir '%s'",
			 	   d->d_name));
				if (strcmp(d->d_name, request->hr_host) == 0) {
					/* found it, punch it */
					debug((httpd, DEBUG_OBESE, "found it punch it"));
					request->hr_virthostname =
					    bozostrdup(httpd, request, d->d_name);
					bozoasprintf(httpd, &s, "%s/%s",
					    httpd->virtbase,
					    request->hr_virthostname);
					break;
				}
			}
			closedir(dirp);
		}
		else {
			debug((httpd, DEBUG_FAT, "opendir %s failed: %s",
			    httpd->virtbase, strerror(errno)));
		}
		if (s == 0) {
			if (httpd->unknown_slash)
				goto use_slashdir;
			return bozo_http_error(httpd, 404, request,
						"unknown URL");
		}
	} else
use_slashdir:
		s = httpd->slashdir;

	/*
	 * ok, nailed the correct slashdir, chdir to it
	 */
	if (chdir(s) < 0)
		return bozo_http_error(httpd, 404, request,
					"can't chdir to slashdir");

	/*
	 * is there a mapping for this request?
	 */
	check_remap(request);

	return 0;
}

/*
 * checks to see if this request has a valid .bzredirect file.  returns
 * 0 when no redirection happend, or 1 when handle_redirect() has been
 * called, -1 on error.
 */
static int
check_bzredirect(bozo_httpreq_t *request)
{
	bozohttpd_t *httpd = request->hr_httpd;
	struct stat sb;
	char dir[MAXPATHLEN], redir[MAXPATHLEN], redirpath[MAXPATHLEN + 1],
	    path[MAXPATHLEN];
	char *basename, *finalredir;
	int rv, absolute;

	/*
	 * if this pathname is really a directory, but doesn't end in /,
	 * use it as the directory to look for the redir file.
	 */
	if ((size_t)snprintf(dir, sizeof(dir), "%s", request->hr_file + 1) >=
	    sizeof(dir)) {
		bozo_http_error(httpd, 404, request, "file path too long");
		return -1;
	}
	debug((httpd, DEBUG_FAT, "check_bzredirect: dir %s", dir));
	basename = strrchr(dir, '/');

	if ((!basename || basename[1] != '\0') &&
	    lstat(dir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
		strcpy(path, dir);
		basename = dir;
	} else if (basename == NULL) {
		strcpy(path, ".");
		strcpy(dir, "");
		basename = dir;
	} else {
		*basename++ = '\0';
		strcpy(path, dir);
	}
	if (bozo_check_special_files(request, basename))
		return -1;

	debug((httpd, DEBUG_FAT, "check_bzredirect: path %s", path));

	if ((size_t)snprintf(redir, sizeof(redir), "%s/%s", path,
			     REDIRECT_FILE) >= sizeof(redir)) {
		return bozo_http_error(httpd, 404, request,
		    "redirectfile path too long");
		return -1;
	}
	if (lstat(redir, &sb) == 0) {
		if (!S_ISLNK(sb.st_mode))
			return 0;
		absolute = 0;
	} else {
		if ((size_t)snprintf(redir, sizeof(redir), "%s/%s", path,
				     ABSREDIRECT_FILE) >= sizeof(redir)) {
			bozo_http_error(httpd, 404, request,
					"redirectfile path too long");
			return -1;
		}
		if (lstat(redir, &sb) < 0 || !S_ISLNK(sb.st_mode))
			return 0;
		absolute = 1;
	}
	debug((httpd, DEBUG_FAT, "check_bzredirect: calling readlink"));
	rv = readlink(redir, redirpath, sizeof redirpath - 1);
	if (rv == -1 || rv == 0) {
		debug((httpd, DEBUG_FAT, "readlink failed"));
		return 0;
	}
	redirpath[rv] = '\0';
	debug((httpd, DEBUG_FAT, "readlink returned \"%s\"", redirpath));

	/* check if we need authentication */
	snprintf(path, sizeof(path), "%s/", dir);
	if (bozo_auth_check(request, path))
		return 1;

	/* now we have the link pointer, redirect to the real place */
	if (!absolute && redirpath[0] != '/') {
		if ((size_t)snprintf(finalredir = redir, sizeof(redir), "%s%s/%s",
		  (strlen(dir) > 0 ? "/" : ""), dir, redirpath) >= sizeof(redir)) {
			bozo_http_error(httpd, 404, request,
					"redirect path too long");
			return -1;
		}
	} else
		finalredir = redirpath;

	debug((httpd, DEBUG_FAT, "check_bzredirect: new redir %s", finalredir));
	handle_redirect(request, finalredir, absolute);
	return 1;
}

/* this fixes the %HH hack that RFC2396 requires.  */
int
bozo_decode_url_percent(bozo_httpreq_t *request, char *str)
{
	bozohttpd_t *httpd = request->hr_httpd;
	char	*s, *t, buf[3];
	char	*end;	/* if end is not-zero, we don't translate beyond that */

	end = str + strlen(str);

	/* fast forward to the first % */
	if ((s = strchr(str, '%')) == NULL)
		return 0;

	t = s;
	do {
		if (end && s >= end) {
			debug((httpd, DEBUG_EXPLODING,
				"fu_%%: past end, filling out.."));
			while (*s)
				*t++ = *s++;
			break;
		}
		debug((httpd, DEBUG_EXPLODING,
			"fu_%%: got s == %%, s[1]s[2] == %c%c",
			s[1], s[2]));
		if (s[1] == '\0' || s[2] == '\0')
			return bozo_http_error(httpd, 400, request,
			    "percent hack missing two chars afterwards");
		if (s[1] == '0' && s[2] == '0')
			return bozo_http_error(httpd, 404, request,
			    "percent hack was %00");
		if (s[1] == '2' && s[2] == 'f')
			return bozo_http_error(httpd, 404, request,
			    "percent hack was %2f (/)");

		buf[0] = *++s;
		buf[1] = *++s;
		buf[2] = '\0';
		s++;
		*t = (char)strtol(buf, NULL, 16);
		debug((httpd, DEBUG_EXPLODING,
				"fu_%%: strtol put '%02x' into *t", *t));
		if (*t++ == '\0')
			return bozo_http_error(httpd, 400, request,
			    "percent hack got a 0 back");

		while (*s && *s != '%') {
			if (end && s >= end)
				break;
			*t++ = *s++;
		}
	} while (*s);
	*t = '\0';

	debug((httpd, DEBUG_FAT, "bozo_decode_url_percent returns `%s'",
			request->hr_file));

	return 0;
}

/*
 * transform_request does this:
 *	- ``expand'' %20 crapola
 *	- punt if it doesn't start with /
 *	- look for "http://myname/" and deal with it.
 *	- maybe call bozo_process_cgi()
 *	- check for ~user and call bozo_user_transform() if so
 *	- if the length > 1, check for trailing slash.  if so,
 *	  add the index.html file
 *	- if the length is 1, return the index.html file
 *	- disallow anything ending up with a file starting
 *	  at "/" or having ".." in it.
 *	- anything else is a really weird internal error
 *	- returns malloced file to serve, if unhandled
 */
static int
transform_request(bozo_httpreq_t *request, int *isindex)
{
	bozohttpd_t *httpd = request->hr_httpd;
	char	*file, *newfile = NULL;
	size_t	len;

	file = NULL;
	*isindex = 0;
	debug((httpd, DEBUG_FAT, "tf_req: file %s", request->hr_file));

	if (bozo_decode_url_percent(request, request->hr_file) ||
	    check_virtual(request))
		goto bad_done;

	file = request->hr_file;

	if (file[0] != '/') {
		bozo_http_error(httpd, 404, request, "unknown URL");
		goto bad_done;
	}

	/* omit additional slashes at the beginning */
	while (file[1] == '/')
		file++;

	/* fix file provided by user as it's used in other handlers */
	request->hr_file = file;

	len = strlen(file);

#ifndef NO_USER_SUPPORT
	/* first of all expand user path */
	if (len > 1 && httpd->enable_users && file[1] == '~') {
		if (file[2] == '\0') {
			bozo_http_error(httpd, 404, request,
					"missing username");
			goto bad_done;
		}
		if (strchr(file + 2, '/') == NULL) {
			char *userredirecturl;

			bozoasprintf(httpd, &userredirecturl, "%s/", file);
			handle_redirect(request, userredirecturl, 0);
			free(userredirecturl);
			return 0;
		}
		debug((httpd, DEBUG_FAT, "calling bozo_user_transform"));

		if (!bozo_user_transform(request))
			return 0;
		
		file = request->hr_file;
		len = strlen(file);
	}
#endif /* NO_USER_SUPPORT */


	switch (check_bzredirect(request)) {
	case -1:
		goto bad_done;
	case 0:
		break;
	default:
		return 0;
	}

	if (len > 1) {
		debug((httpd, DEBUG_FAT, "file[len-1] == %c", file[len-1]));
		if (file[len-1] == '/') {	/* append index.html */
			*isindex = 1;
			debug((httpd, DEBUG_FAT, "appending index.html"));
			newfile = bozomalloc(httpd,
					len + strlen(httpd->index_html) + 1);
			strcpy(newfile, file + 1);
			strcat(newfile, httpd->index_html);
		} else
			newfile = bozostrdup(httpd, request, file + 1);
	} else if (len == 1) {
		debug((httpd, DEBUG_EXPLODING, "tf_req: len == 1"));
		newfile = bozostrdup(httpd, request, httpd->index_html);
		*isindex = 1;
	} else {	/* len == 0 ? */
		bozo_http_error(httpd, 500, request, "request->hr_file is nul");
		goto bad_done;
	}

	if (newfile == NULL) {
		bozo_http_error(httpd, 500, request, "internal failure");
		goto bad_done;
	}

	/*
	 * stop traversing outside our domain
	 *
	 * XXX true security only comes from our parent using chroot(2)
	 * before execve(2)'ing us.  or our own built in chroot(2) support.
	 */
	
	debug((httpd, DEBUG_FAT, "newfile: %s", newfile));
	
	if (*newfile == '/' || strcmp(newfile, "..") == 0 ||
	    strstr(newfile, "/..") || strstr(newfile, "../")) {
		bozo_http_error(httpd, 403, request, "illegal request");
		goto bad_done;
	}

	if (bozo_auth_check(request, newfile))
		goto bad_done;

	if (strlen(newfile)) {
		request->hr_oldfile = request->hr_file;
		request->hr_file = newfile;
	}

	if (bozo_process_cgi(request) ||
	    bozo_process_lua(request))
		return 0;

	debug((httpd, DEBUG_FAT, "transform_request set: %s", newfile));
	return 1;

bad_done:
	debug((httpd, DEBUG_FAT, "transform_request returning: 0"));
	free(newfile);
	return 0;
}

/*
 * can_gzip checks if the request supports and prefers gzip encoding.
 *
 * XXX: we do not consider the associated q with gzip in making our
 *      decision which is broken.
 */

static int
can_gzip(bozo_httpreq_t *request)
{
	const char	*pos;
	const char	*tmp;
	size_t		 len;

	/* First we decide if the request can be gzipped at all. */

	/* not if we already are encoded... */
	tmp = bozo_content_encoding(request, request->hr_file);
	if (tmp && *tmp)
		return 0;

	/* not if we are not asking for the whole file... */
	if (request->hr_last_byte_pos != -1 || request->hr_have_range)
		return 0;

	/* Then we determine if gzip is on the cards. */

	for (pos = request->hr_accept_encoding; pos && *pos; pos += len) {
		while (*pos == ' ')
			pos++;

		len = strcspn(pos, ";,");

		if ((len == 4 && strncasecmp("gzip", pos, 4) == 0) ||
		    (len == 6 && strncasecmp("x-gzip", pos, 6) == 0))
			return 1;

		if (pos[len] == ';')
			len += strcspn(&pos[len], ",");

		if (pos[len])
			len++;
	}

	return 0;
}

/*
 * bozo_process_request does the following:
 *	- check the request is valid
 *	- process cgi-bin if necessary
 *	- transform a filename if necesarry
 *	- return the HTTP request
 */
void
bozo_process_request(bozo_httpreq_t *request)
{
	bozohttpd_t *httpd = request->hr_httpd;
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

	fd = -1;
	encoding = NULL;
	if (can_gzip(request)) {
		bozoasprintf(httpd, &file, "%s.gz", request->hr_file);
		fd = open(file, O_RDONLY);
		if (fd >= 0)
			encoding = "gzip";
		free(file);
	}

	file = request->hr_file;

	if (fd < 0)
		fd = open(file, O_RDONLY);

	if (fd < 0) {
		debug((httpd, DEBUG_FAT, "open failed: %s", strerror(errno)));
		switch (errno) {
		case EPERM:
		case EACCES:
			bozo_http_error(httpd, 403, request,
					"no permission to open file");
			break;
		case ENAMETOOLONG:
			/*FALLTHROUGH*/
		case ENOENT:
			if (!bozo_dir_index(request, file, isindex))
				bozo_http_error(httpd, 404, request, "no file");
			break;
		default:
			bozo_http_error(httpd, 500, request, "open file");
		}
		goto cleanup_nofd;
	}
	if (fstat(fd, &sb) < 0) {
		bozo_http_error(httpd, 500, request, "can't fstat");
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
		bozo_printf(httpd, "%s 304 Not Modified\r\n",
				request->hr_proto);
		bozo_printf(httpd, "\r\n");
		bozo_flush(httpd, stdout);
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
	debug((httpd, DEBUG_FAT, "have_range %d first_pos %lld last_pos %lld",
	    request->hr_have_range,
	    (long long)request->hr_first_byte_pos,
	    (long long)request->hr_last_byte_pos));
	if (request->hr_have_range)
		bozo_printf(httpd, "%s 206 Partial Content\r\n",
				request->hr_proto);
	else
		bozo_printf(httpd, "%s 200 OK\r\n", request->hr_proto);

	if (request->hr_proto != httpd->consts.http_09) {
		type = bozo_content_type(request, file);
		if (!encoding)
			encoding = bozo_content_encoding(request, file);

		bozo_print_header(request, &sb, type, encoding);
		bozo_printf(httpd, "\r\n");
	}
	bozo_flush(httpd, stdout);

	if (request->hr_method != HTTP_HEAD) {
		off_t szleft, cur_byte_pos;

		szleft =
		     request->hr_last_byte_pos - request->hr_first_byte_pos + 1;
		cur_byte_pos = request->hr_first_byte_pos;

 retry:
		while (szleft) {
			size_t sz;

			if ((off_t)httpd->mmapsz < szleft)
				sz = httpd->mmapsz;
			else
				sz = (size_t)szleft;
			if (mmap_and_write_part(httpd, fd, cur_byte_pos, sz)) {
				if (errno == ENOMEM) {
					httpd->mmapsz /= 2;
					if (httpd->mmapsz >= httpd->page_size)
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

/* make sure we're not trying to access special files */
int
bozo_check_special_files(bozo_httpreq_t *request, const char *name)
{
	bozohttpd_t *httpd = request->hr_httpd;

	for (size_t i = 0; specials[i].file; i++)
		if (strcmp(name, specials[i].file) == 0)
			return bozo_http_error(httpd, 403, request,
					       specials[i].name);

	return 0;
}

/* generic header printing routine */
void
bozo_print_header(bozo_httpreq_t *request,
		struct stat *sbp, const char *type, const char *encoding)
{
	bozohttpd_t *httpd = request->hr_httpd;
	off_t len;
	char	date[40];
	bozoheaders_t *hdr;

	SIMPLEQ_FOREACH(hdr, &request->hr_replheaders, h_next) {
		bozo_printf(httpd, "%s: %s\r\n", hdr->h_header,
				hdr->h_value);
	}

	bozo_printf(httpd, "Date: %s\r\n", bozo_http_date(date, sizeof(date)));
	bozo_printf(httpd, "Server: %s\r\n", httpd->server_software);
	bozo_printf(httpd, "Accept-Ranges: bytes\r\n");
	if (sbp) {
		char filedate[40];
		struct	tm *tm;

		tm = gmtime(&sbp->st_mtime);
		strftime(filedate, sizeof filedate,
		    "%a, %d %b %Y %H:%M:%S GMT", tm);
		bozo_printf(httpd, "Last-Modified: %s\r\n", filedate);
	}
	if (type && *type)
		bozo_printf(httpd, "Content-Type: %s\r\n", type);
	if (encoding && *encoding)
		bozo_printf(httpd, "Content-Encoding: %s\r\n", encoding);
	if (sbp) {
		if (request->hr_have_range) {
			len = request->hr_last_byte_pos -
					request->hr_first_byte_pos +1;
			bozo_printf(httpd,
				"Content-Range: bytes %qd-%qd/%qd\r\n",
				(long long) request->hr_first_byte_pos,
				(long long) request->hr_last_byte_pos,
				(long long) sbp->st_size);
		} else
			len = sbp->st_size;
		bozo_printf(httpd, "Content-Length: %qd\r\n", (long long)len);
	}
	if (request->hr_proto == httpd->consts.http_11)
		bozo_printf(httpd, "Connection: close\r\n");
	bozo_flush(httpd, stdout);
}

#ifndef NO_DEBUG
void
debug__(bozohttpd_t *httpd, int level, const char *fmt, ...)
{
	va_list	ap;
	int savederrno;

	/* only log if the level is low enough */
	if (httpd->debug < level)
		return;

	savederrno = errno;
	va_start(ap, fmt);
	if (httpd->logstderr) {
		vfprintf(stderr, fmt, ap);
		fputs("\n", stderr);
	} else
		vsyslog(LOG_DEBUG, fmt, ap);
	va_end(ap);
	errno = savederrno;
}
#endif /* NO_DEBUG */

/* these are like warn() and err(), except for syslog not stderr */
void
bozowarn(bozohttpd_t *httpd, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (httpd->logstderr || isatty(STDERR_FILENO)) {
		//fputs("warning: ", stderr);
		vfprintf(stderr, fmt, ap);
		fputs("\n", stderr);
	} else
		vsyslog(LOG_INFO, fmt, ap);
	va_end(ap);
}

void
bozoerr(bozohttpd_t *httpd, int code, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (httpd->logstderr || isatty(STDERR_FILENO)) {
		//fputs("error: ", stderr);
		vfprintf(stderr, fmt, ap);
		fputs("\n", stderr);
	} else
		vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(code);
}

void
bozoasprintf(bozohttpd_t *httpd, char **str, const char *fmt, ...)
{
	va_list ap;
	int e;

	va_start(ap, fmt);
	e = vasprintf(str, fmt, ap);
	va_end(ap);

	if (e < 0)
		bozoerr(httpd, EXIT_FAILURE, "asprintf");
}

/*
 * this escapes HTML tags.  returns allocated escaped
 * string if needed, or NULL on allocation failure or
 * lack of escape need.
 * call with NULL httpd in error paths, to avoid recursive
 * malloc failure.  call with valid httpd in normal paths
 * to get automatic allocation failure handling.
 */
char *
bozo_escape_html(bozohttpd_t *httpd, const char *url)
{
	int	i, j;
	char	*tmp;
	size_t	len;

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
		return NULL;

	/*
	 * we need to handle being called from different
	 * pathnames.
	 */
	len = strlen(url) + j;
	if (httpd)
		tmp = bozomalloc(httpd, len);
	else if ((tmp = malloc(len)) == 0)
			return NULL;

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

	return tmp;
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
	{ 413, 	"413 Payload Too Large", "Use smaller requests", },
	{ 417,	"417 Expectation Failed","Expectations not available", },
	{ 420,	"420 Enhance Your Calm","Chill, Winston", },
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

/* the follow functions and variables are used in handling HTTP errors */
/* ARGSUSED */
int
bozo_http_error(bozohttpd_t *httpd, int code, bozo_httpreq_t *request,
		const char *msg)
{
	char portbuf[20];
	const char *header = http_errors_short(code);
	const char *reason = http_errors_long(code);
	const char *proto = (request && request->hr_proto) ?
				request->hr_proto : httpd->consts.http_11;
	int	size;
	bozoheaders_t *hdr;

	debug((httpd, DEBUG_FAT, "bozo_http_error %d: %s", code, msg));
	if (header == NULL || reason == NULL) {
		bozoerr(httpd, 1,
			"bozo_http_error() failed (short = %p, long = %p)",
			header, reason);
		return code;
	}

	if (request && request->hr_serverport &&
	    strcmp(request->hr_serverport, "80") != 0)
		snprintf(portbuf, sizeof(portbuf), ":%s",
				request->hr_serverport);
	else
		portbuf[0] = '\0';

	if (request && request->hr_file) {
		char *file = NULL, *user = NULL;
		int file_alloc = 0;
		const char *hostname = BOZOHOST(httpd, request);

		/* bozo_escape_html() failure here is just too bad. */
		file = bozo_escape_html(NULL, request->hr_file);
		if (file == NULL) 
			file = request->hr_file;
		else
			file_alloc = 1;

#ifndef NO_USER_SUPPORT
		if (request->hr_user != NULL) {
			char *user_escaped;

			user_escaped = bozo_escape_html(NULL, request->hr_user);
			if (user_escaped == NULL)
				user_escaped = request->hr_user;
			/* expand username to ~user/ */
			bozoasprintf(httpd, &user, "~%s/", user_escaped);
			if (user_escaped != request->hr_user)
				free(user_escaped);
		}
#endif /* !NO_USER_SUPPORT */

		size = snprintf(httpd->errorbuf, BUFSIZ,
		    "<html><head><title>%s</title></head>\n"
		    "<body><h1>%s</h1>\n"
		    "%s%s: <pre>%s</pre>\n"
 		    "<hr><address><a href=\"//%s%s/\">%s%s</a></address>\n"
		    "</body></html>\n",
		    header, header,
		    user ? user : "", file,
		    reason, hostname, portbuf, hostname, portbuf);
		free(user);
		if (size >= (int)BUFSIZ) {
			bozowarn(httpd,
				"bozo_http_error buffer too small, truncated");
			size = (int)BUFSIZ;
		}

		if (file_alloc)
			free(file);
	} else
		size = 0;

	bozo_printf(httpd, "%s %s\r\n", proto, header);

	if (request) {
		bozo_auth_check_401(request, code);
		SIMPLEQ_FOREACH(hdr, &request->hr_replheaders, h_next) {
			bozo_printf(httpd, "%s: %s\r\n", hdr->h_header,
					hdr->h_value);
		}
	}

	bozo_printf(httpd, "Content-Type: text/html\r\n");
	bozo_printf(httpd, "Content-Length: %d\r\n", size);
	bozo_printf(httpd, "Server: %s\r\n", httpd->server_software);
	if (request && request->hr_allow)
		bozo_printf(httpd, "Allow: %s\r\n", request->hr_allow);
	/* RFC 7231 (HTTP/1.1) 6.5.7 */
	if (code == 408 && request->hr_proto == httpd->consts.http_11)
		bozo_printf(httpd, "Connection: close\r\n");
	bozo_printf(httpd, "\r\n");
	/* According to the RFC 2616 sec. 9.4 HEAD method MUST NOT return a
	 * message-body in the response */
	if (size && request && request->hr_method != HTTP_HEAD)
		bozo_printf(httpd, "%s", httpd->errorbuf);
	bozo_flush(httpd, stdout);

	return code;
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
bozodgetln(bozohttpd_t *httpd, int fd, ssize_t *lenp,
	ssize_t (*readfn)(bozohttpd_t *, int, void *, size_t))
{
	ssize_t	len;
	int	got_cr = 0;
	char	c, *nbuffer;

	/* initialise */
	if (httpd->getln_buflen == 0) {
		/* should be plenty for most requests */
		httpd->getln_buflen = 128;
		httpd->getln_buffer = malloc((size_t)httpd->getln_buflen);
		if (httpd->getln_buffer == NULL) {
			httpd->getln_buflen = 0;
			return NULL;
		}
	}
	len = 0;

	/*
	 * we *have* to read one byte at a time, to not break cgi
	 * programs (for we pass stdin off to them).  could fix this
	 * by becoming a fd-passing program instead of just exec'ing
	 * the program
	 *
	 * the above is no longer true, we are the fd-passing
	 * program already.
	 */
	for (; readfn(httpd, fd, &c, 1) == 1; ) {
		debug((httpd, DEBUG_EXPLODING, "bozodgetln read %c", c));

		if (len >= httpd->getln_buflen - 1) {
			httpd->getln_buflen *= 2;
			debug((httpd, DEBUG_EXPLODING, "bozodgetln: "
				"reallocating buffer to buflen %zu",
				httpd->getln_buflen));
			nbuffer = bozorealloc(httpd, httpd->getln_buffer,
				(size_t)httpd->getln_buflen);
			httpd->getln_buffer = nbuffer;
		}

		httpd->getln_buffer[len++] = c;
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
	httpd->getln_buffer[len] = '\0';
	debug((httpd, DEBUG_OBESE, "bozodgetln returns: '%s' with len %zd",
	       httpd->getln_buffer, len));
	*lenp = len;
	return httpd->getln_buffer;
}

void *
bozorealloc(bozohttpd_t *httpd, void *ptr, size_t size)
{
	void	*p;

	p = realloc(ptr, size);
	if (p)
		return p;
	
	bozo_http_error(httpd, 500, NULL, "memory allocation failure");
	exit(EXIT_FAILURE);
}

void *
bozomalloc(bozohttpd_t *httpd, size_t size)
{
	void	*p;

	p = malloc(size);
	if (p)
		return p;

	bozo_http_error(httpd, 500, NULL, "memory allocation failure");
	exit(EXIT_FAILURE);
}

char *
bozostrdup(bozohttpd_t *httpd, bozo_httpreq_t *request, const char *str)
{
	char	*p;

	p = strdup(str);
	if (p)
		return p;

	if (!request)
		bozoerr(httpd, EXIT_FAILURE, "strdup");

	bozo_http_error(httpd, 500, request, "memory allocation failure");
	exit(EXIT_FAILURE);
}

/* set default values in bozohttpd_t struct */
int
bozo_init_httpd(bozohttpd_t *httpd)
{
	/* make sure everything is clean */
	(void) memset(httpd, 0x0, sizeof(*httpd));

	/* constants */
	httpd->consts.http_09 = "HTTP/0.9";
	httpd->consts.http_10 = "HTTP/1.0";
	httpd->consts.http_11 = "HTTP/1.1";
	httpd->consts.text_plain = "text/plain";

	/* mmap region size */
	httpd->mmapsz = BOZO_MMAPSZ;

	/* error buffer for bozo_http_error() */
	if ((httpd->errorbuf = malloc(BUFSIZ)) == NULL) {
		fprintf(stderr,
			"bozohttpd: memory_allocation failure\n");
		return 0;
	}
#ifndef NO_LUA_SUPPORT
	SIMPLEQ_INIT(&httpd->lua_states);
#endif
	return 1;
}

/* set default values in bozoprefs_t struct */
int
bozo_init_prefs(bozohttpd_t *httpd, bozoprefs_t *prefs)
{
	int rv = 0;

	/* make sure everything is clean */
	(void) memset(prefs, 0x0, sizeof(*prefs));

	/* set up default values */
	if (!bozo_set_pref(httpd, prefs, "server software", SERVER_SOFTWARE))
		rv = 1;
	if (!bozo_set_pref(httpd, prefs, "index.html", INDEX_HTML))
		rv = 1;
	if (!bozo_set_pref(httpd, prefs, "public_html", PUBLIC_HTML))
		rv = 1;
	if (!bozo_set_pref(httpd, prefs, "initial timeout", INITIAL_TIMEOUT))
		rv = 1;
	if (!bozo_set_pref(httpd, prefs, "header timeout", HEADER_WAIT_TIME))
		rv = 1;
	if (!bozo_set_pref(httpd, prefs, "request timeout", TOTAL_MAX_REQ_TIME))
		rv = 1;

	return rv;
}

/* set default values */
int
bozo_set_defaults(bozohttpd_t *httpd, bozoprefs_t *prefs)
{
	return bozo_init_httpd(httpd) && bozo_init_prefs(httpd, prefs);
}

/* set the virtual host name, port and root */
int
bozo_setup(bozohttpd_t *httpd, bozoprefs_t *prefs, const char *vhost,
		const char *root)
{
	struct passwd	 *pw;
	extern char	**environ;
	static char	 *cleanenv[1] = { NULL };
	uid_t		  uid;
	int		  uidset = 0;
	char		 *chrootdir;
	char		 *username;
	char		 *portnum;
	char		 *cp;
	int		  dirtyenv;

	dirtyenv = 0;

	if (vhost == NULL) {
		httpd->virthostname = bozomalloc(httpd, MAXHOSTNAMELEN+1);
		if (gethostname(httpd->virthostname, MAXHOSTNAMELEN+1) < 0)
			bozoerr(httpd, 1, "gethostname");
		httpd->virthostname[MAXHOSTNAMELEN] = '\0';
	} else {
		httpd->virthostname = bozostrdup(httpd, NULL, vhost);
	}
	httpd->slashdir = bozostrdup(httpd, NULL, root);
	if ((portnum = bozo_get_pref(prefs, "port number")) != NULL) {
		httpd->bindport = bozostrdup(httpd, NULL, portnum);
	}

	/* go over preferences now */
	if ((cp = bozo_get_pref(prefs, "numeric")) != NULL &&
	    strcmp(cp, "true") == 0) {
		httpd->numeric = 1;
	}
	if ((cp = bozo_get_pref(prefs, "log to stderr")) != NULL &&
	    strcmp(cp, "true") == 0) {
		httpd->logstderr = 1;
	}
	if ((cp = bozo_get_pref(prefs, "bind address")) != NULL) {
		httpd->bindaddress = bozostrdup(httpd, NULL, cp);
	}
	if ((cp = bozo_get_pref(prefs, "background")) != NULL) {
		httpd->background = atoi(cp);
	}
	if ((cp = bozo_get_pref(prefs, "foreground")) != NULL &&
	    strcmp(cp, "true") == 0) {
		httpd->foreground = 1;
	}
	if ((cp = bozo_get_pref(prefs, "pid file")) != NULL) {
		httpd->pidfile = bozostrdup(httpd, NULL, cp);
	}
	if ((cp = bozo_get_pref(prefs, "unknown slash")) != NULL &&
	    strcmp(cp, "true") == 0) {
		httpd->unknown_slash = 1;
	}
	if ((cp = bozo_get_pref(prefs, "virtual base")) != NULL) {
		httpd->virtbase = bozostrdup(httpd, NULL, cp);
	}
	if ((cp = bozo_get_pref(prefs, "enable users")) != NULL &&
	    strcmp(cp, "true") == 0) {
		httpd->enable_users = 1;
	}
	if ((cp = bozo_get_pref(prefs, "enable user cgibin")) != NULL &&
	    strcmp(cp, "true") == 0) {
		httpd->enable_cgi_users = 1;
	}
	if ((cp = bozo_get_pref(prefs, "dirty environment")) != NULL &&
	    strcmp(cp, "true") == 0) {
		dirtyenv = 1;
	}
	if ((cp = bozo_get_pref(prefs, "hide dots")) != NULL &&
	    strcmp(cp, "true") == 0) {
		httpd->hide_dots = 1;
	}
	if ((cp = bozo_get_pref(prefs, "directory indexing")) != NULL &&
	    strcmp(cp, "true") == 0) {
		httpd->dir_indexing = 1;
	}
	if ((cp = bozo_get_pref(prefs, "public_html")) != NULL) {
		httpd->public_html = bozostrdup(httpd, NULL, cp);
	}
	if ((cp = bozo_get_pref(prefs, "initial timeout")) != NULL) {
		httpd->initial_timeout = atoi(cp);
	}
	if ((cp = bozo_get_pref(prefs, "header timeout")) != NULL) {
		httpd->header_timeout = atoi(cp);
	}
	if ((cp = bozo_get_pref(prefs, "request timeout")) != NULL) {
		httpd->request_timeout = atoi(cp);
	}
	httpd->server_software =
	    bozostrdup(httpd, NULL, bozo_get_pref(prefs, "server software"));
	httpd->index_html =
	    bozostrdup(httpd, NULL, bozo_get_pref(prefs, "index.html"));

	/*
	 * initialise ssl and daemon mode if necessary.
	 */
	bozo_ssl_init(httpd);
	bozo_daemon_init(httpd);

	username = bozo_get_pref(prefs, "username");
	if (username != NULL) {
		if ((pw = getpwnam(username)) == NULL)
			bozoerr(httpd, 1, "getpwnam(%s): %s", username,
				strerror(errno));
		if (initgroups(pw->pw_name, pw->pw_gid) == -1)
			bozoerr(httpd, 1, "initgroups: %s", strerror(errno));
		if (setgid(pw->pw_gid) == -1)
			bozoerr(httpd, 1, "setgid(%u): %s", pw->pw_gid,
				strerror(errno));
		uid = pw->pw_uid;
		uidset = 1;
	}
	/*
	 * handle chroot.
	 */
	if ((chrootdir = bozo_get_pref(prefs, "chroot dir")) != NULL) {
		httpd->rootdir = bozostrdup(httpd, NULL, chrootdir);
		if (chdir(httpd->rootdir) == -1)
			bozoerr(httpd, 1, "chdir(%s): %s", httpd->rootdir,
				strerror(errno));
		if (chroot(httpd->rootdir) == -1)
			bozoerr(httpd, 1, "chroot(%s): %s", httpd->rootdir,
				strerror(errno));
	}

	if (uidset && setuid(uid) == -1)
		bozoerr(httpd, 1, "setuid(%d): %s", uid, strerror(errno));

	/*
	 * prevent info leakage between different compartments.
	 * some PATH values in the environment would be invalided
	 * by chroot. cross-user settings might result in undesirable
	 * effects.
	 */
	if ((chrootdir != NULL || username != NULL) && !dirtyenv)
		environ = cleanenv;

#ifdef _SC_PAGESIZE
	httpd->page_size = (long)sysconf(_SC_PAGESIZE);
#else
	httpd->page_size = 4096;
#endif
	debug((httpd, DEBUG_OBESE, "myname is %s, slashdir is %s",
			httpd->virthostname, httpd->slashdir));

	return 1;
}

int
bozo_get_version(char *buf, size_t size)
{
	return snprintf(buf, size, "%s", SERVER_SOFTWARE);
}
