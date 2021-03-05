/*	$NetBSD: cgi-bozo.c,v 1.48.2.1 2021/03/05 13:34:19 martin Exp $	*/

/*	$eterna: cgi-bozo.c,v 1.40 2011/11/18 09:21:15 mrg Exp $	*/

/*
 * Copyright (c) 1997-2021 Matthew R. Green
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

/* this code implements CGI/1.2 for bozohttpd */

#ifndef NO_CGIBIN_SUPPORT

#include <sys/param.h>
#include <sys/socket.h>

#include <ctype.h>
#include <errno.h>
#include <paths.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <assert.h>

#include <netinet/in.h>

#include "bozohttpd.h"

#define CGIBIN_PREFIX		"cgi-bin/"
#define CGIBIN_PREFIX_LEN	(sizeof(CGIBIN_PREFIX)-1)

#ifndef USE_ARG
#define USE_ARG(x)	/*LINTED*/(void)&(x)
#endif

/*
 * given the file name, return a CGI interpreter
 */
static const char *
content_cgihandler(bozohttpd_t *httpd, bozo_httpreq_t *request,
		   const char *file)
{
	bozo_content_map_t	*map;

	USE_ARG(request);
	debug((httpd, DEBUG_FAT, "content_cgihandler: trying file %s", file));
	map = bozo_match_content_map(httpd, file, 0);
	if (map)
		return map->cgihandler;
	return NULL;
}

static int
parse_header(bozo_httpreq_t *request, const char *str, ssize_t len,
	     char **hdr_str, char **hdr_val)
{
	struct	bozohttpd_t *httpd = request->hr_httpd;
	char	*name, *value;

	/* if the string passed is zero-length bail out */
	if (*str == '\0')
		return -1;

	value = bozostrdup(httpd, request, str);

	/* locate the ':' separator in the header/value */
	name = bozostrnsep(&value, ":", &len);

	if (NULL == name || -1 == len) {
		free(value);
		return -1;
	}

	/* skip leading space/tab */
	while (*value == ' ' || *value == '\t')
		len--, value++;

	*hdr_str = name;
	*hdr_val = value;

	return 0;
}

/*
 * handle parsing a CGI header output, transposing a Status: header
 * into the HTTP reply (ie, instead of "200 OK").
 */
static void
finish_cgi_output(bozohttpd_t *httpd, bozo_httpreq_t *request, int in, int nph)
{
	char	buf[BOZO_WRSZ];
	char	*str;
	ssize_t	len;
	ssize_t rbytes;
	SIMPLEQ_HEAD(, bozoheaders)	headers;
	bozoheaders_t *hdr, *nhdr;
	int	write_header, nheaders = 0;

	/* much of this code is like bozo_read_request()'s header loop. */
	SIMPLEQ_INIT(&headers);
	write_header = nph == 0;
	while (nph == 0 &&
		(str = bozodgetln(httpd, in, &len, bozo_read)) != NULL) {
		char	*hdr_name, *hdr_value;

		if (parse_header(request, str, len, &hdr_name, &hdr_value))
			break;

		/*
		 * The CGI 1.{1,2} spec both say that if the cgi program
		 * returns a `Status:' header field then the server MUST
		 * return it in the response.  If the cgi program does
		 * not return any `Status:' header then the server should
		 * respond with 200 OK.
		 * The CGI 1.1 and 1.2 specification differ slightly on
		 * this in that v1.2 says that the script MUST NOT return a
		 * `Status:' header if it is returning a `Location:' header.
		 * For compatibility we are going with the CGI 1.1 behavior.
		 */
		if (strcasecmp(hdr_name, "status") == 0) {
			debug((httpd, DEBUG_OBESE,
				"%s: writing HTTP header "
				"from status %s ..", __func__, hdr_value));
			bozo_printf(httpd, "%s %s\r\n", request->hr_proto,
				    hdr_value);
			bozo_flush(httpd, stdout);
			write_header = 0;
			free(hdr_name);
			break;
		}

		hdr = bozomalloc(httpd, sizeof *hdr);
		hdr->h_header = hdr_name;
		hdr->h_value = hdr_value;
		SIMPLEQ_INSERT_TAIL(&headers, hdr, h_next);
		nheaders++;
	}

	if (write_header) {
		debug((httpd, DEBUG_OBESE,
			"%s: writing HTTP header ..", __func__));
		bozo_printf(httpd,
			"%s 200 OK\r\n", request->hr_proto);
		bozo_flush(httpd, stdout);
	}

	if (nheaders) {
		debug((httpd, DEBUG_OBESE,
			"%s:  writing delayed HTTP headers ..", __func__));
		SIMPLEQ_FOREACH_SAFE(hdr, &headers, h_next, nhdr) {
			bozo_printf(httpd, "%s: %s\r\n", hdr->h_header,
				    hdr->h_value);
			free(hdr->h_header);
			free(hdr);
		}
		bozo_printf(httpd, "\r\n");
		bozo_flush(httpd, stdout);
	}

	/* CGI programs should perform their own timeouts */
	while ((rbytes = read(in, buf, sizeof buf)) > 0) {
		ssize_t wbytes;
		char *bp = buf;

		while (rbytes) {
			wbytes = bozo_write(httpd, STDOUT_FILENO, buf,
					    (size_t)rbytes);
			if (wbytes > 0) {
				rbytes -= wbytes;
				bp += wbytes;
			} else
				bozoerr(httpd, 1,
					"cgi output write failed: %s",
					strerror(errno));
		}		
	}
}

static void
append_index_html(bozohttpd_t *httpd, char **url)
{
	*url = bozorealloc(httpd, *url,
			strlen(*url) + strlen(httpd->index_html) + 1);
	strcat(*url, httpd->index_html);
	debug((httpd, DEBUG_NORMAL,
		"append_index_html: url adjusted to `%s'", *url));
}

/* This function parse search-string according to section 4.4 of RFC3875 */
static char **
parse_search_string(bozo_httpreq_t *request, const char *query, size_t *args_len)
{
	struct	bozohttpd_t *httpd = request->hr_httpd;
	size_t i;
	char *s, *str, **args;
	
	*args_len = 0;

	/* URI MUST not contain any unencoded '=' - RFC3875, section 4.4 */
	if (strchr(query, '='))
		return NULL;

	str = bozostrdup(httpd, request, query);

	/*
	 * there's no more arguments than '+' chars in the query string as it's
	 * the separator
	 */
	*args_len = 1;
	/* count '+' in str */
	for (s = str; (s = strchr(s, '+')) != NULL; (*args_len)++)
		s++;
	
	args = bozomalloc(httpd, sizeof(*args) * (*args_len + 1));
 
	args[0] = str;
	args[*args_len] = NULL;
	for (s = str, i = 1; (s = strchr(s, '+')) != NULL; i++) {
		*s = '\0';
		s++;
		args[i] = s;
	}

	/*
	 * check if search-strings are valid:
	 *
	 * RFC3875, section 4.4:
	 *
	 * search-string = search-word *( "+" search-word )
	 * search-word   = 1*schar
	 * schar		 = unreserved | escaped | xreserved
	 * xreserved	 = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "," |
	 *		   "$"
	 * 
	 * section 2.3:
	 *
	 * hex	      = digit | "A" | "B" | "C" | "D" | "E" | "F" | "a" |
	 *		"b" | "c" | "d" | "e" | "f"
	 * escaped    = "%" hex hex
	 * unreserved = alpha | digit | mark
	 * mark	      = "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")"
	 *
	 * section 2.2:
	 *
	 * alpha	= lowalpha | hialpha
	 * lowalpha	= "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" |
	 *		  "i" | "j" | "k" | "l" | "m" | "n" | "o" | "p" |
	 *		  "q" | "r" | "s" | "t" | "u" | "v" | "w" | "x" |
	 *		  "y" | "z"
	 * hialpha	= "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" |
	 *		  "I" | "J" | "K" | "L" | "M" | "N" | "O" | "P" |
	 *		  "Q" | "R" | "S" | "T" | "U" | "V" | "W" | "X" |
	 *	          "Y" | "Z"  
	 * digit        = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" |
	 *		  "8" | "9"
	 */
#define	UNRESERVED_CHAR	"-_.!~*'()"
#define	XRESERVED_CHAR	";/?:@&=,$"

	for (i = 0; i < *args_len; i++) {
		s = args[i];
		/* search-word MUST have at least one schar */
		if (*s == '\0')
			goto parse_err;
		while (*s) {
			/* check if it's unreserved */
			if (isalpha((int)*s) || isdigit((int)*s) ||
			    strchr(UNRESERVED_CHAR, *s)) {
				s++;
				continue;
			}

			/* check if it's escaped */
			if (*s == '%') {
				if (s[1] == '\0' || s[2] == '\0')
					goto parse_err;
				if (!isxdigit((int)s[1]) ||
				    !isxdigit((int)s[2]))
					goto parse_err;
				s += 3;
				continue;
			}

			/* check if it's xreserved */

			if (strchr(XRESERVED_CHAR, *s)) {
				s++;
				continue;
			}

			goto parse_err;
		}
	}

	/* decode percent encoding */
	for (i = 0; i < *args_len; i++) {
		if (bozo_decode_url_percent(request, args[i]))
			goto parse_err;
	}

	/* allocate each arg separately */
	for (i = 0; i < *args_len; i++)
		args[i] = bozostrdup(httpd, request, args[i]);
	free(str);

	return args;

parse_err:

	free(str);
	free(args);
	*args_len = 0;

	return NULL;

}

void
bozo_cgi_setbin(bozohttpd_t *httpd, const char *path)
{
	httpd->cgibin = bozostrdup(httpd, NULL, path);
	debug((httpd, DEBUG_OBESE, "cgibin (cgi-bin directory) is %s",
	       httpd->cgibin));
}

/* help build up the environ pointer */
void
bozo_setenv(bozohttpd_t *httpd, const char *env, const char *val,
		char **envp)
{
	char *s1 = bozomalloc(httpd, strlen(env) + strlen(val) + 2);

	strcpy(s1, env);
	strcat(s1, "=");
	strcat(s1, val);
	debug((httpd, DEBUG_OBESE, "bozo_setenv: %s", s1));
	*envp = s1;
}

/*
 * Checks if the request has asked for a cgi-bin.  Should only be called if
 * cgibin is set.  If it starts CGIBIN_PREFIX or has a ncontent handler,
 * process the cgi, otherwise just return.  Returns 0 if it did not handle
 * the request.
 */
int
bozo_process_cgi(bozo_httpreq_t *request)
{
	bozohttpd_t *httpd = request->hr_httpd;
	char	buf[BOZO_WRSZ];
	char	date[40];
	bozoheaders_t *headp;
	const char *type, *clen, *info, *cgihandler;
	char	*query, *s, *t, *path, *env, *command, *file, *url;
	char	**envp, **curenvp, **argv, **search_string_argv = NULL;
	char	**lastenvp;
	char	*uri;
	size_t	i, len, search_string_argc = 0;
	ssize_t rbytes;
	pid_t	pid;
	int	envpsize, ix, nph;
	int	sv[2];

	if (!httpd->cgibin && !httpd->process_cgi)
		return 0;

#ifndef NO_USER_SUPPORT
	if (request->hr_user && !httpd->enable_cgi_users)
		return 0;
#endif /* !NO_USER_SUPPORT */

	if (request->hr_oldfile && strcmp(request->hr_oldfile, "/") != 0)
		uri = request->hr_oldfile;
	else
		uri = request->hr_file;

	if (uri[0] == '/')
		file = bozostrdup(httpd, request, uri);
	else
		bozoasprintf(httpd, &file, "/%s", uri);

	if (request->hr_query && strlen(request->hr_query))
		query = bozostrdup(httpd, request, request->hr_query);
	else
		query = NULL;

	bozoasprintf(httpd, &url, "%s%s%s",
		     file,
		     query ? "?" : "",
		     query ? query : "");
	debug((httpd, DEBUG_NORMAL, "%s: url `%s'", __func__, url));

	path = NULL;
	envp = NULL;
	cgihandler = NULL;
	command = NULL;
	info = NULL;

	len = strlen(url);

	if (bozo_auth_check(request, url + 1))
		goto out;

	if (!httpd->cgibin ||
	    strncmp(url + 1, CGIBIN_PREFIX, CGIBIN_PREFIX_LEN) != 0) {
		cgihandler = content_cgihandler(httpd, request, file + 1);
		if (cgihandler == NULL) {
			debug((httpd, DEBUG_FAT,
				"%s: no handler, returning", __func__));
			goto out;
		}
		if (len == 0 || file[len - 1] == '/')
			append_index_html(httpd, &file);
		debug((httpd, DEBUG_NORMAL, "%s: cgihandler `%s'",
		    __func__, cgihandler));
	} else if (len - 1 == CGIBIN_PREFIX_LEN)	/* url is "/cgi-bin/" */
		append_index_html(httpd, &file);

	/* RFC3875 sect. 4.4. - search-string support */
	if (query != NULL) {
		search_string_argv = parse_search_string(request, query,
		    &search_string_argc);
	}

	debug((httpd, DEBUG_NORMAL, "parse_search_string args no: %zu",
	    search_string_argc));
	for (i = 0; i < search_string_argc; i++) {
		debug((httpd, DEBUG_FAT,
		    "search_string[%zu]: `%s'", i, search_string_argv[i]));
	}

	argv = bozomalloc(httpd, sizeof(*argv) * (3 + search_string_argc));

	ix = 0;
	if (cgihandler) {
		command = file + 1;
		path = bozostrdup(httpd, request, cgihandler);
	} else {
		command = file + CGIBIN_PREFIX_LEN + 1;
		if ((s = strchr(command, '/')) != NULL) {
			info = bozostrdup(httpd, request, s);
			*s = '\0';
		}
		path = bozomalloc(httpd,
				strlen(httpd->cgibin) + 1 + strlen(command) + 1);
		strcpy(path, httpd->cgibin);
		strcat(path, "/");
		strcat(path, command);
	}

	argv[ix++] = path;

	/* copy search-string args */
	for (i = 0; i < search_string_argc; i++)
		argv[ix++] = search_string_argv[i];

	argv[ix++] = NULL;
	nph = strncmp(command, "nph-", 4) == 0;

	type = request->hr_content_type;
	clen = request->hr_content_length;

	envpsize = 13 + request->hr_nheaders + 
	    (info && *info ? 1 : 0) +
	    (query && *query ? 1 : 0) +
	    (type && *type ? 1 : 0) +
	    (clen && *clen ? 1 : 0) +
	    (request->hr_remotehost && *request->hr_remotehost ? 1 : 0) +
	    (request->hr_remoteaddr && *request->hr_remoteaddr ? 1 : 0) +
	    (cgihandler ? 1 : 0) +
	    bozo_auth_cgi_count(request) +
	    (request->hr_serverport && *request->hr_serverport ? 1 : 0);

	debug((httpd, DEBUG_FAT,
		"%s: path `%s', cmd `%s', info `%s', "
		"query `%s', nph `%d', envpsize `%d'", __func__,
		path, command, strornull(info),
		strornull(query), nph, envpsize));

	envp = bozomalloc(httpd, sizeof(*envp) * envpsize);
	for (ix = 0; ix < envpsize; ix++)
		envp[ix] = NULL;
	curenvp = envp;
	lastenvp = envp + envpsize;

	SIMPLEQ_FOREACH(headp, &request->hr_headers, h_next) {
		const char *s2;
		env = bozomalloc(httpd, 6 + strlen(headp->h_header) + 1 +
		    strlen(headp->h_value));

		t = env;
		strcpy(t, "HTTP_");
		t += strlen(t);
		for (s2 = headp->h_header; *s2; t++, s2++)
			if (islower((unsigned)*s2))
				*t = toupper((unsigned)*s2);
			else if (*s2 == '-')
				*t = '_';
			else
				*t = *s2;
		*t = '\0';
		debug((httpd, DEBUG_OBESE, "setting header %s as %s = %s",
		    headp->h_header, env, headp->h_value));
		bozo_setenv(httpd, env, headp->h_value, curenvp++);
		free(env);
	}

#ifndef _PATH_DEFPATH
#define _PATH_DEFPATH "/usr/bin:/bin"
#endif

	bozo_setenv(httpd, "PATH", _PATH_DEFPATH, curenvp++);
	bozo_setenv(httpd, "IFS", " \t\n", curenvp++);
	bozo_setenv(httpd, "SERVER_NAME", BOZOHOST(httpd,request), curenvp++);
	bozo_setenv(httpd, "GATEWAY_INTERFACE", "CGI/1.1", curenvp++);
	bozo_setenv(httpd, "SERVER_PROTOCOL", request->hr_proto, curenvp++);
	bozo_setenv(httpd, "REQUEST_METHOD", request->hr_methodstr, curenvp++);
	bozo_setenv(httpd, "SCRIPT_NAME", file, curenvp++);
	bozo_setenv(httpd, "SCRIPT_FILENAME", file + 1, curenvp++);
	bozo_setenv(httpd, "SERVER_SOFTWARE", httpd->server_software,
			curenvp++);
	bozo_setenv(httpd, "REQUEST_URI", uri, curenvp++);
	bozo_setenv(httpd, "DATE_GMT", bozo_http_date(date, sizeof(date)),
			curenvp++);
	/* RFC3875 section 4.1.7 says that QUERY_STRING MUST be defined. */ 
	if (query && *query)
		bozo_setenv(httpd, "QUERY_STRING", query, curenvp++);
	else
		bozo_setenv(httpd, "QUERY_STRING", "", curenvp++);
	if (info && *info)
		bozo_setenv(httpd, "PATH_INFO", info, curenvp++);
	if (type && *type)
		bozo_setenv(httpd, "CONTENT_TYPE", type, curenvp++);
	if (clen && *clen)
		bozo_setenv(httpd, "CONTENT_LENGTH", clen, curenvp++);
	if (request->hr_serverport && *request->hr_serverport)
		bozo_setenv(httpd, "SERVER_PORT", request->hr_serverport,
				curenvp++);
	if (request->hr_remotehost && *request->hr_remotehost)
		bozo_setenv(httpd, "REMOTE_HOST", request->hr_remotehost,
				curenvp++);
	if (request->hr_remoteaddr && *request->hr_remoteaddr)
		bozo_setenv(httpd, "REMOTE_ADDR", request->hr_remoteaddr,
				curenvp++);
	/*
	 * Apache does this when invoking content handlers, and PHP
	 * 5.3 requires it as a "security" measure.
	 */
	if (cgihandler)
		bozo_setenv(httpd, "REDIRECT_STATUS", "200", curenvp++);
	bozo_auth_cgi_setenv(request, &curenvp);

	debug((httpd, DEBUG_FAT, "%s: going exec %s with args:", __func__,
	    path));

	for (i = 0; argv[i] != NULL; i++) {
		debug((httpd, DEBUG_FAT, "%s: argv[%zu] = `%s'", __func__,
		    i, argv[i]));
	}

	if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, sv) == -1)
		bozoerr(httpd, 1, "child socketpair failed: %s",
				strerror(errno));

	*curenvp = 0;
	assert(lastenvp > curenvp);

	/*
	 * We create 2 procs: one to become the CGI, one read from
	 * the CGI and output to the network, and this parent will
	 * continue reading from the network and writing to the
	 * CGI procsss.
	 */
	switch (fork()) {
	case -1: /* eep, failure */
		bozoerr(httpd, 1, "child fork failed: %s", strerror(errno));
		/*NOTREACHED*/
	case 0:
		close(sv[0]);
		dup2(sv[1], STDIN_FILENO);
		dup2(sv[1], STDOUT_FILENO);
		close(2);
		close(sv[1]);
		closelog();
		bozo_daemon_closefds(httpd);

		if (-1 == execve(path, argv, envp)) {
			int saveerrno = errno;
			bozo_http_error(httpd, 404, request,
				"Cannot execute CGI");
			/* don't log easy to trigger events */
			if (saveerrno != ENOENT &&
			    saveerrno != EISDIR &&
			    saveerrno != EACCES)
				bozoerr(httpd, 1, "child exec failed: %s: %s",
				      path, strerror(saveerrno));
			_exit(1);
		}
		/* NOT REACHED */
		bozoerr(httpd, 1, "child execve returned?!");
	}

	free(query);
	free(file);
	free(url);
	for (i = 0; i < search_string_argc; i++)
		free(search_string_argv[i]);
	free(search_string_argv);

	close(sv[1]);

	/* parent: read from stdin (bozo_read()) write to sv[0] */
	/* child: read from sv[0] (bozo_write()) write to stdout */
	pid = fork();
	if (pid == -1)
		bozoerr(httpd, 1, "io child fork failed: %s", strerror(errno));
	else if (pid == 0) {
		/* child reader/writer */
		close(STDIN_FILENO);
		finish_cgi_output(httpd, request, sv[0], nph);
		/* if we do SSL, send a SSL_shutdown now */
		bozo_ssl_shutdown(request->hr_httpd);
		/* if we're done output, our parent is useless... */
		kill(getppid(), SIGKILL);
		debug((httpd, DEBUG_FAT, "done processing cgi output"));
		_exit(0);
	}
	close(STDOUT_FILENO);

	/* CGI programs should perform their own timeouts */
	while ((rbytes = bozo_read(httpd, STDIN_FILENO, buf, sizeof buf)) > 0) {
		ssize_t wbytes;
		char *bp = buf;

		while (rbytes) {
			wbytes = write(sv[0], buf, (size_t)rbytes);
			if (wbytes > 0) {
				rbytes -= wbytes;
				bp += wbytes;
			} else
				bozoerr(httpd, 1, "write failed: %s",
					strerror(errno));
		}		
	}
	debug((httpd, DEBUG_FAT, "done processing cgi input"));
	exit(0);

 out:

	for (i = 0; i < search_string_argc; i++)
		free(search_string_argv[i]);
	free(search_string_argv);
	free(query);
	free(file);
	free(url);
	return 0;
}

#ifndef NO_DYNAMIC_CONTENT
/* cgi maps are simple ".postfix /path/to/prog" */
void
bozo_add_content_map_cgi(bozohttpd_t *httpd, const char *arg,
                         const char *cgihandler)
{
	bozo_content_map_t *map;

	debug((httpd, DEBUG_NORMAL, "bozo_add_content_map_cgi: name %s cgi %s",
		arg, cgihandler));

	httpd->process_cgi = 1;

	map = bozo_get_content_map(httpd, arg);
	map->name = arg;
	map->type = map->encoding = map->encoding11 = NULL;
	map->cgihandler = cgihandler;
}
#endif /* NO_DYNAMIC_CONTENT */

#endif /* NO_CGIBIN_SUPPORT */
