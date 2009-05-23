/*	$NetBSD: cgi-bozo.c,v 1.14 2009/05/23 02:26:03 mrg Exp $	*/

/*	$eterna: cgi-bozo.c,v 1.32 2009/05/22 21:51:38 mrg Exp $	*/

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

#include <netinet/in.h>

#include "bozohttpd.h"

#define CGIBIN_PREFIX		"cgi-bin/"
#define CGIBIN_PREFIX_LEN	(sizeof(CGIBIN_PREFIX)-1)

static	char	*cgibin;	/* cgi-bin directory */
static	int	Cflag;		/* added a cgi handler, always process_cgi() */

static	const char *	content_cgihandler(http_req *, const char *);
static	void		finish_cgi_output(http_req *request, int, int);
static	int		parse_header(const char *, ssize_t, char **, char **);
static	void		append_index_html(char **);

void
set_cgibin(char *path)
{
	cgibin = path;
	debug((DEBUG_OBESE, "cgibin (cgi-bin directory) is %s", cgibin));
}

/* help build up the environ pointer */
void
spsetenv(const char *env, const char *val, char **envp)
{
	char *s1 = bozomalloc(strlen(env) + strlen(val) + 2);

	strcpy(s1, env);
	strcat(s1, "=");
	strcat(s1, val);
	debug((DEBUG_OBESE, "spsetenv: %s", s1));
	*envp = s1;
}

/*
 * Checks if the request has asked for a cgi-bin.  Should only be called if
 * cgibin is set.  If it starts CGIBIN_PREFIX or has a ncontent handler,
 * process the cgi, otherwise just return.  Returns 0 if it did not handle
 * the request.
 */
int
process_cgi(http_req *request)
{
	char	buf[WRSZ];
	struct	headers *headp;
	const char *type, *clen, *info, *cgihandler;
	char	*query, *s, *t, *path, *env, *command, *file, *url;
	char	**envp, **curenvp, *argv[4];
	size_t	len;
	ssize_t rbytes;
	pid_t	pid;
	int	envpsize, ix, nph;
	int	sv[2];

	if (!cgibin && !Cflag)
		return 0;

	asprintf(&file, "/%s", request->hr_file);
	if (file == NULL)
		return 0;
	if (request->hr_query && strlen(request->hr_query))
		query = bozostrdup(request->hr_query);
	else
		query = NULL;

	asprintf(&url, "%s%s%s", file, query ? "?" : "", query ? query : "");
	if (url == NULL)
		goto out;
	debug((DEBUG_NORMAL, "process_cgi: url `%s'", url));

	path = NULL;
	envp = NULL;
	cgihandler = NULL;
	command = NULL;
	info = NULL;

	len = strlen(url);

	if (auth_check(request, url + 1))
		goto out;

	if (!cgibin || strncmp(url + 1, CGIBIN_PREFIX, CGIBIN_PREFIX_LEN) != 0) {
		cgihandler = content_cgihandler(request, file + 1);
		if (cgihandler == NULL) {
			debug((DEBUG_FAT, "process_cgi: no handler, returning"));
			goto out;
		}
		if (len == 0 || file[len - 1] == '/')
			append_index_html(&file);
		debug((DEBUG_NORMAL, "process_cgi: cgihandler `%s'",
		    cgihandler));
	} else if (len - 1 == CGIBIN_PREFIX_LEN)	/* url is "/cgi-bin/" */
		append_index_html(&file);

	ix = 0;
	if (cgihandler) {
		command = file + 1;
		path = bozostrdup(cgihandler);
		argv[ix++] = path;
			/* argv[] = [ path, command, query, NULL ] */
	} else {
		command = file + CGIBIN_PREFIX_LEN + 1;
		if ((s = strchr(command, '/')) != NULL) {
			info = bozostrdup(s);
			*s = '\0';
		}
		path = bozomalloc(strlen(cgibin) + 1 + strlen(command) + 1);
		strcpy(path, cgibin);
		strcat(path, "/");
		strcat(path, command);
			/* argv[] = [ command, query, NULL ] */
	}
	argv[ix++] = command;
	argv[ix++] = query;
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
	    auth_cgi_count(request) +
	    (request->hr_serverport && *request->hr_serverport ? 1 : 0);

	debug((DEBUG_FAT,
	    "process_cgi: path `%s' cmd `%s' info `%s' query `%s' nph `%d' envpsize `%d'",
	    path, command, strornull(info), strornull(query), nph, envpsize));

	envp = bozomalloc(sizeof(*envp) * envpsize);
	for (ix = 0; ix < envpsize; ix++)
		envp[ix] = NULL;
	curenvp = envp;

	SIMPLEQ_FOREACH(headp, &request->hr_headers, h_next) {
		const char *s2;
		env = bozomalloc(6 + strlen(headp->h_header) + 1 +
		    strlen(headp->h_value));

		t = env;
		strcpy(t, "HTTP_");
		t += strlen(t);
		for (s2 = headp->h_header; *s2; t++, s2++)
			if (islower((u_int)*s2))
				*t = toupper((u_int)*s2);
			else if (*s2 == '-')
				*t = '_';
			else
				*t = *s2;
		*t = '\0';
		debug((DEBUG_OBESE, "setting header %s as %s = %s",
		    headp->h_header, env, headp->h_value));
		spsetenv(env, headp->h_value, curenvp++);
		free(env);
	}

#ifndef _PATH_DEFPATH
#define _PATH_DEFPATH "/usr/bin:/bin"
#endif

	spsetenv("PATH", _PATH_DEFPATH, curenvp++);
	spsetenv("IFS", " \t\n", curenvp++);
	spsetenv("SERVER_NAME", myname, curenvp++);
	spsetenv("GATEWAY_INTERFACE", "CGI/1.1", curenvp++);
	spsetenv("SERVER_PROTOCOL", request->hr_proto, curenvp++);
	spsetenv("REQUEST_METHOD", request->hr_methodstr, curenvp++);
	spsetenv("SCRIPT_NAME", file, curenvp++);
	spsetenv("SCRIPT_FILENAME", file + 1, curenvp++);
	spsetenv("SERVER_SOFTWARE", server_software, curenvp++);
	spsetenv("REQUEST_URI", request->hr_file, curenvp++);
	spsetenv("DATE_GMT", http_date(), curenvp++);
	if (query && *query)
		spsetenv("QUERY_STRING", query, curenvp++);
	if (info && *info)
		spsetenv("PATH_INFO", info, curenvp++);
	if (type && *type)
		spsetenv("CONTENT_TYPE", type, curenvp++);
	if (clen && *clen)
		spsetenv("CONTENT_LENGTH", clen, curenvp++);
	if (request->hr_serverport && *request->hr_serverport)
		spsetenv("SERVER_PORT", request->hr_serverport, curenvp++);
	if (request->hr_remotehost && *request->hr_remotehost)
		spsetenv("REMOTE_HOST", request->hr_remotehost, curenvp++);
	if (request->hr_remoteaddr && *request->hr_remoteaddr)
		spsetenv("REMOTE_ADDR", request->hr_remoteaddr, curenvp++);
	auth_cgi_setenv(request, &curenvp);

	free(file);
	free(url);

	debug((DEBUG_FAT, "process_cgi: going exec %s, %s %s %s",
	    path, argv[0], strornull(argv[1]), strornull(argv[2])));

	if (-1 == socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, sv))
		error(1, "child socketpair failed: %s", strerror(errno));

	/*
	 * We create 2 procs: one to become the CGI, one read from
	 * the CGI and output to the network, and this parent will
	 * continue reading from the network and writing to the
	 * CGI procsss.
	 */
	switch (fork()) {
	case -1: /* eep, failure */
		error(1, "child fork failed: %s", strerror(errno));
	case 0:
		close(sv[0]);
		dup2(sv[1], STDIN_FILENO);
		dup2(sv[1], STDOUT_FILENO);
		close(2);
		close(sv[1]);
		closelog();
		daemon_closefds();

		if (-1 == execve(path, argv, envp))
			error(1, "child exec failed: %s: %s",
			      path, strerror(errno));
		/* NOT REACHED */
		error(1, "child execve returned?!");
	}

	close(sv[1]);

	/* parent: read from stdin (bozoread()) write to sv[0] */
	/* child: read from sv[0] (bozowrite()) write to stdout */
	pid = fork();
	if (pid == -1)
		error(1, "io child fork failed: %s", strerror(errno));
	else if (pid == 0) {
		/* child reader/writer */
		close(STDIN_FILENO);
		finish_cgi_output(request, sv[0], nph);
		/* if we're done output, our parent is useless... */
		kill(getppid(), SIGKILL);
		debug((DEBUG_FAT, "done processing cgi output"));
		_exit(0);
	}
	close(STDOUT_FILENO);

	/* XXX we should have some goo that times us out
	 */
	while ((rbytes = bozoread(STDIN_FILENO, buf, sizeof buf)) > 0) {
		ssize_t wbytes;
		char *bp = buf;

		while (rbytes) {
			wbytes = write(sv[0], buf , rbytes);
			if (wbytes > 0) {
				rbytes -= wbytes;
				bp += wbytes;
			} else
				error(1, "write failed: %s", strerror(errno));
		}		
	}
	debug((DEBUG_FAT, "done processing cgi input"));
	exit(0);

 out:
	if (query)
		free(query);
	if (file)
		free(file);
	if (url)
		free(url);
	return 0;
}

/*
 * handle parsing a CGI header output, transposing a Status: header
 * into the HTTP reply (ie, instead of "200 OK").
 */
static void
finish_cgi_output(http_req *request, int in, int nph)
{
	char	buf[WRSZ];
	char	*str;
	ssize_t	len;
	ssize_t rbytes;
	SIMPLEQ_HEAD(, headers)	headers;
	struct	headers *hdr, *nhdr;
	int	write_header, nheaders = 0;

	/* much of this code is like read_request()'s header loop. hmmm... */
	SIMPLEQ_INIT(&headers);
	write_header = nph == 0;
	while (nph == 0 && (str = bozodgetln(in, &len, read)) != NULL) {
		char	*hdr_name, *hdr_value;

		if (parse_header(str, len, &hdr_name, &hdr_value))
			break;

		/*
		 * The CGI 1.{1,2} spec both say that if the cgi program
		 * returns a `Status:' header field then the server MUST
		 * return it in the response.  If the cgi program does
		 * not return any `Status:' header then the server should
		 * respond with 200 OK.
		 * XXX The CGI 1.1 and 1.2 specification differ slightly on
		 * this in that v1.2 says that the script MUST NOT return a
		 * `Status:' header if it is returning a `Location:' header.
		 * For compatibility we are going with the CGI 1.1 behavior.
		 */
		if (strcasecmp(hdr_name, "status") == 0) {
			debug((DEBUG_OBESE, "process_cgi:  writing HTTP header "
					    "from status %s ..", hdr_value));
			bozoprintf("%s %s\r\n", request->hr_proto, hdr_value);
			bozoflush(stdout);
			write_header = 0;
			free(hdr_name);
			break;
		}

		hdr = bozomalloc(sizeof *hdr);
		hdr->h_header = hdr_name;
		hdr->h_value = hdr_value;
		SIMPLEQ_INSERT_TAIL(&headers, hdr, h_next);
		nheaders++;
	}

	if (write_header) {
		debug((DEBUG_OBESE, "process_cgi:  writing HTTP header .."));
		bozoprintf("%s 200 OK\r\n", request->hr_proto);
		bozoflush(stdout);
	}

	if (nheaders) {
		debug((DEBUG_OBESE, "process_cgi:  writing delayed HTTP "
				    "headers .."));
		SIMPLEQ_FOREACH_SAFE(hdr, &headers, h_next, nhdr) {
			bozoprintf("%s: %s\r\n", hdr->h_header, hdr->h_value);
			free(hdr->h_header);
			free(hdr);
		}
		bozoprintf("\r\n");
		bozoflush(stdout);
	}

	/* XXX we should have some goo that times us out
	 */
	while ((rbytes = read(in, buf, sizeof buf)) > 0) {
		ssize_t wbytes;
		char *bp = buf;

		while (rbytes) {
			wbytes = bozowrite(STDOUT_FILENO, buf, rbytes);
			if (wbytes > 0) {
				rbytes -= wbytes;
				bp += wbytes;
			} else
				error(1, "cgi output write failed: %s",
				    strerror(errno));
		}		
	}
}

static int
parse_header(const char *str, ssize_t len, char **hdr_str, char **hdr_val)
{
	char	*name, *value;

	/* if the string passed is zero-length bail out */
	if (*str == '\0')
		return -1;

	value = bozostrdup(str);

	/* locate the ':' separator in the header/value */
	name = bozostrnsep(&value, ":", &len);

	if (NULL == name || -1 == len) {
		free(name);
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
 * given the file name, return a CGI interpreter
 */
static const char *
content_cgihandler(http_req *request, const char *file)
{
	struct	content_map	*map;

	debug((DEBUG_FAT, "content_cgihandler: trying file %s", file));
	map = match_content_map(file, 0);
	if (map)
		return (map->cgihandler);
	return (NULL);
}

static void
append_index_html(char **url)
{
	*url = bozorealloc(*url, strlen(*url) + strlen(index_html) + 1);
	strcat(*url, index_html);
	debug((DEBUG_NORMAL, "append_index_html: url adjusted to `%s'", *url));
}

#ifndef NO_DYNAMIC_CONTENT
/* cgi maps are simple ".postfix /path/to/prog" */
void
add_content_map_cgi(char *arg, char *cgihandler)
{
	struct content_map *map;

	debug((DEBUG_NORMAL, "add_content_map_cgi: name %s cgi %s", arg, cgihandler));

	Cflag = 1;

	map = get_content_map(arg);
	map->name = arg;
	map->type = map->encoding = map->encoding11 = NULL;
	map->cgihandler = cgihandler;
}
#endif /* NO_DYNAMIC_CONTENT */

#endif /* NO_CGIBIN_SUPPORT */
