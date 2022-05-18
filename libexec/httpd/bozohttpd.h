/*	$NetBSD: bozohttpd.h,v 1.72 2022/05/18 00:37:11 mrg Exp $	*/

/*	$eterna: bozohttpd.h,v 1.39 2011/11/18 09:21:15 mrg Exp $	*/

/*
 * Copyright (c) 1997-2022 Matthew R. Green
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
#ifndef BOZOHTTPD_H_
#define BOZOHTTPD_H_	1

#include "netbsd_queue.h"

#include <stdbool.h>
#include <stdio.h>
#include <signal.h>

#include <sys/stat.h>

#ifndef NO_LUA_SUPPORT
#include <lua.h>
#endif

#ifndef NO_BLOCKLIST_SUPPORT
#include <blocklist.h>
void pfilter_notify(const int, const int);
#endif

/* QNX provides a lot of NetBSD things in nbutil.h */
#ifdef HAVE_NBUTIL_H
#include <nbutil.h>
#endif

/* lots of "const" but gets free()'ed etc at times, sigh */

/* headers */
typedef struct bozoheaders {
	/*const*/ char *h_header;
	/*const*/ char *h_value;	/* this gets free()'ed etc at times */
	SIMPLEQ_ENTRY(bozoheaders)	h_next;
} bozoheaders_t;
SIMPLEQ_HEAD(qheaders, bozoheaders);

#ifndef NO_LUA_SUPPORT
typedef struct lua_handler {
	const char	*name;
	int		 ref;
	SIMPLEQ_ENTRY(lua_handler)	h_next;
} lua_handler_t;

typedef struct lua_state_map {
	const char 	*script;
	const char	*prefix;
	lua_State	*L;
	SIMPLEQ_HEAD(, lua_handler)	handlers;
	SIMPLEQ_ENTRY(lua_state_map)	s_next;
} lua_state_map_t;
#endif

typedef struct bozo_content_map_t {
	const char	*name;		/* postfix of file */
	const char	*type;		/* matching content-type */
	const char	*encoding;	/* matching content-encoding */
	const char	*encoding11;	/* matching content-encoding (HTTP/1.1) */
	const char	*cgihandler;	/* optional CGI handler */
} bozo_content_map_t;

/* this struct holds the bozo constants */
typedef struct bozo_consts_t {
	const char	*http_09;	/* "HTTP/0.9" */
	const char	*http_10;	/* "HTTP/1.0" */
	const char	*http_11;	/* "HTTP/1.1" */
	const char	*text_plain;	/* "text/plain" */
} bozo_consts_t;

/* this structure encapsulates all the bozo flags and control vars */
typedef struct bozohttpd_t {
	char		*rootdir;	/* root directory */
	int		 numeric;	/* avoid gethostby*() */
	char		*virtbase;	/* virtual directory base */
	int		 unknown_slash;	/* unknown vhosts go to normal slashdir */
	int		 logstderr;	/* log to stderr (even if not tty) */
	int		 background;	/* drop into daemon mode */
	int		 foreground;	/* keep daemon mode in foreground */
	char		*pidfile;	/* path to the pid file, if any */
	size_t		 page_size;	/* page size */
	char		*slashdir;	/* www slash directory */
	char		*bindport;	/* bind port; default "http" */
	char		*bindaddress;	/* address for binding - INADDR_ANY */
	int		 debug;		/* debugging level */
	char		*virthostname;	/* my name */
	const char	*server_software;/* our brand :-) */
	const char	*index_html;	/* our home page */
	const char	*public_html;	/* ~user/public_html page */
	int		 enable_users;	/* enable public_html */
	int		 enable_cgi_users;	/* use the cgi handler */
	int		*sock;		/* bound sockets */
	int		 nsock;		/* number of above */
	struct pollfd	*fds;		/* current poll fd set */
	int		 request_times;	/* # times a request was processed */
	int		 dir_indexing;	/* handle directories */
	const char	*dir_readme;	/* include README footer in indexes */
	int		 hide_dots;	/* hide .* */
	int		 process_cgi;	/* use the cgi handler */
	char		*cgibin;	/* cgi-bin directory */
	unsigned	ssl_timeout;	/* ssl timeout */
	unsigned	initial_timeout;/* first line timeout */
	unsigned	header_timeout;	/* header lines timeout */
	unsigned	request_timeout;/* total session timeout */
	char		*ssl_min_proto;	/* minimum ssl protocol level */
#ifndef NO_LUA_SUPPORT
	int		 process_lua;	/* use the Lua handler */
	SIMPLEQ_HEAD(, lua_state_map)	lua_states;
#endif
	void		*sslinfo;	/* pointer to ssl struct */
	int		dynamic_content_map_size;/* size of dyn cont map */
	bozo_content_map_t	*dynamic_content_map;/* dynamic content map */
	size_t		 mmapsz;	/* size of region to mmap */
	char		*getln_buffer;	/* space for getln buffer */
	ssize_t		 getln_buflen;	/* length of allocated space */
	char		*errorbuf;	/* no dynamic allocation allowed */
	bozo_consts_t	 consts;	/* various constants */
} bozohttpd_t;

/* bozo_httpreq_t */
typedef struct bozo_httpreq_t {
	bozohttpd_t	*hr_httpd;
	int		hr_method;
#define	HTTP_GET	0x01
#define HTTP_POST	0x02
#define HTTP_HEAD	0x03
#define HTTP_OPTIONS	0x04	/* not supported */
#define HTTP_PUT	0x05	/* not supported */
#define HTTP_DELETE	0x06	/* not supported */
#define HTTP_TRACE	0x07	/* not supported */
#define HTTP_CONNECT	0x08	/* not supported */
	const char *hr_methodstr;
	char	*hr_virthostname;	/* server name (if not identical
					   to hr_httpd->virthostname) */
	char	*hr_file_free;	/* pointer to file buffer to free() */
	char	*hr_file;	/* pointer into file buffer */
	char	*hr_oldfile;	/* if we added an index_html */
	char	*hr_query;
	char	*hr_host;	/* HTTP/1.1 Host: or virtual hostname,
				   possibly including a port number */
#ifndef NO_USER_SUPPORT
	char	*hr_user;	/* username if we hit userdir request */
#endif /* !NO_USER_SUPPORT */
	const char *hr_proto;
	const char *hr_content_type;
	const char *hr_content_length;
	const char *hr_allow;
	const char *hr_referrer;
	const char *hr_range;
	const char *hr_if_modified_since;
	const char *hr_accept_encoding;
	int         hr_have_range;
	off_t       hr_first_byte_pos;
	off_t       hr_last_byte_pos;
	/*const*/ char *hr_remotehost;
	/*const*/ char *hr_remoteaddr;
	/*const*/ char *hr_serverport;
#ifdef DO_HTPASSWD
	/*const*/ char *hr_authrealm;
	/*const*/ char *hr_authuser;
	/*const*/ char *hr_authpass;
#endif
	struct qheaders		hr_headers;
	struct qheaders		hr_replheaders;
	unsigned		hr_nheaders;
	size_t			hr_header_bytes;
} bozo_httpreq_t;

/* helper to access the "active" host name from a httpd/request pair */
#define	BOZOHOST(HTTPD,REQUEST)	((REQUEST)->hr_virthostname ?		\
					(REQUEST)->hr_virthostname :	\
					(HTTPD)->virthostname)

/* structure to hold string based (name, value) pairs with preferences */
typedef struct bozoprefs_t {
	size_t		  size;		/* size of the two arrays */
	size_t		  count;	/* # of entries in arrays */
	char		**name;		/* names of each entry */
	char		**value;	/* values for the name entries */
} bozoprefs_t;

/* sun2 has a tiny VA range */
#ifdef __mc68010__
#ifndef BOZO_WRSZ
#define BOZO_WRSZ	(16 * 1024)
#endif
#ifndef BOZO_MMAPSZ
#define BOZO_MMAPSZ	(BOZO_WRSZ * 4)
#endif
#endif

/* by default write in upto 64KiB chunks, and mmap in upto 64MiB chunks */
#ifndef BOZO_WRSZ
#define BOZO_WRSZ	(64 * 1024)
#endif
#ifndef BOZO_MMAPSZ
#define BOZO_MMAPSZ	(BOZO_WRSZ * 1024)
#endif

/* only allow this many total headers bytes */
#define BOZO_HEADERS_MAX_SIZE (16 * 1024)

#define BOZO_MINBUFSIZE (4 * 1024)

/* debug flags */
#define DEBUG_NORMAL	1
#define DEBUG_FAT	2
#define DEBUG_OBESE	3
#define DEBUG_EXPLODING	4

#define	strornull(x)	((x) ? (x) : "<null>")

#if (defined(__GNUC__) && __GNUC__ >= 3) || defined(__lint__)
#define BOZO_PRINTFLIKE(x,y) __attribute__((__format__(__printf__, x,y)))
#define BOZO_DEAD __attribute__((__noreturn__))
#define BOZO_CHECKRET __attribute__((__warn_unused_result__))
#else
#define BOZO_PRINTFLIKE(x,y)
#define BOZO_DEAD
#define BOZO_CHECKRET
#endif

#ifdef NO_DEBUG
#define	debug(x)
#define have_debug	(0)
#else
void	debug__(bozohttpd_t *, int, const char *, ...) BOZO_PRINTFLIKE(3, 4);
#define debug(x)	debug__ x
#define have_debug	(1)
#endif /* NO_DEBUG */

#define BOZO_HTTP_PORT	"80"
#define BOZO_HTTPS_PORT	"443"

/*
 * bozohttpd special files.  avoid serving these out.
 *
 * When you add some .bz* file, make sure to also check it in
 * bozo_check_special_files()
 */

#ifndef REDIRECT_FILE
#define REDIRECT_FILE		".bzredirect"
#endif
#ifndef ABSREDIRECT_FILE
#define ABSREDIRECT_FILE	".bzabsredirect"
#endif
#ifndef REMAP_FILE
#define REMAP_FILE		".bzremap"
#endif
#ifndef AUTH_FILE
#define AUTH_FILE		".htpasswd"
#endif

/* be sure to always return this error up */
int	bozo_http_error(bozohttpd_t *, int, bozo_httpreq_t *, const char *);

int	bozo_check_special_files(bozo_httpreq_t *, const char *, bool) BOZO_CHECKRET;
char	*bozo_http_date(char *, size_t);
void	bozo_print_header(bozo_httpreq_t *, struct stat *, const char *,
			  const char *);
char	*bozo_escape_rfc3986(bozohttpd_t *httpd, const char *url, int absolute);
char	*bozo_escape_html(bozohttpd_t *httpd, const char *url);
int	bozo_decode_url_percent(bozo_httpreq_t *, char *);

/* these are similar to libc functions, no underscore here */
void	bozowarn(bozohttpd_t *, const char *, ...)
		BOZO_PRINTFLIKE(2, 3);
void	bozoerr(bozohttpd_t *, int, const char *, ...)
		BOZO_PRINTFLIKE(3, 4)
		BOZO_DEAD;
void	bozoasprintf(bozohttpd_t *, char **, const char *, ...)
		BOZO_PRINTFLIKE(3, 4);
char	*bozodgetln(bozohttpd_t *, int, ssize_t *, ssize_t (*)(bozohttpd_t *,
		    int, void *, size_t));
char	*bozostrnsep(char **, const char *, ssize_t *);
void	*bozomalloc(bozohttpd_t *, size_t);
void	*bozorealloc(bozohttpd_t *, void *, size_t);
char	*bozostrdup(bozohttpd_t *, bozo_httpreq_t *, const char *);

#define bozo_noop	do { /* nothing */ } while (/*CONSTCOND*/0)

#define have_core					(1)

/* ssl-bozo.c */
#ifdef NO_SSL_SUPPORT
#define bozo_ssl_set_opts(w, x, y)			bozo_noop
#define bozo_ssl_set_ciphers(w, x)			bozo_noop
#define bozo_ssl_init(x)				bozo_noop
#define bozo_ssl_accept(x)				(0)
#define bozo_ssl_shutdown(x)				bozo_noop
#define bozo_ssl_destroy(x)				bozo_noop
#define have_ssl					(0)
#else
void	bozo_ssl_set_opts(bozohttpd_t *, const char *, const char *);
void	bozo_ssl_set_ciphers(bozohttpd_t *, const char *);
void	bozo_ssl_init(bozohttpd_t *);
int	bozo_ssl_accept(bozohttpd_t *);
void	bozo_ssl_shutdown(bozohttpd_t *);
void	bozo_ssl_destroy(bozohttpd_t *);
#define have_ssl					(1)
#endif


/* auth-bozo.c */
#ifdef DO_HTPASSWD
void	bozo_auth_init(bozo_httpreq_t *);
int	bozo_auth_check(bozo_httpreq_t *, const char *);
void	bozo_auth_cleanup(bozo_httpreq_t *);
int	bozo_auth_check_headers(bozo_httpreq_t *, char *, char *, ssize_t);
void	bozo_auth_check_401(bozo_httpreq_t *, int);
void	bozo_auth_cgi_setenv(bozo_httpreq_t *, char ***);
int	bozo_auth_cgi_count(bozo_httpreq_t *);
#else
#define	bozo_auth_init(x)				bozo_noop
#define	bozo_auth_check(x, y)				(0)
#define	bozo_auth_cleanup(x)				bozo_noop
#define	bozo_auth_check_headers(y, z, a, b)		(0)
#define	bozo_auth_check_401(x, y)			bozo_noop
#define	bozo_auth_cgi_setenv(x, y)			bozo_noop
#define	bozo_auth_cgi_count(x)				(0)
#endif /* DO_HTPASSWD */


/* cgi-bozo.c */
#ifdef NO_CGIBIN_SUPPORT
#define bozo_cgi_setbin(h,s)				bozo_noop
#define	bozo_process_cgi(h)				(0)
#define have_cgibin					(0)
#else
void	bozo_cgi_setbin(bozohttpd_t *, const char *);
void	bozo_setenv(bozohttpd_t *, const char *, const char *, char **);
int	bozo_process_cgi(bozo_httpreq_t *);
#define have_cgibin					(1)
#endif /* NO_CGIBIN_SUPPORT */


/* lua-bozo.c */
#ifdef NO_LUA_SUPPORT
#define bozo_process_lua(h)				(0)
#define bozo_add_lua_map(h,s,t)				bozo_noop
#define have_lua					(0)
#else
void	bozo_add_lua_map(bozohttpd_t *, const char *, const char *);
int	bozo_process_lua(bozo_httpreq_t *);
#define have_lua					(1)
#endif /* NO_LUA_SUPPORT */


/* daemon-bozo.c */
#ifdef NO_DAEMON_MODE
#define bozo_daemon_init(x)				bozo_noop
#define bozo_daemon_fork(x)				(0)
#define bozo_daemon_closefds(x)				bozo_noop
#define have_daemon_mode				(0)
#else
void	bozo_daemon_init(bozohttpd_t *);
int	bozo_daemon_fork(bozohttpd_t *);
void	bozo_daemon_closefds(bozohttpd_t *);
#define have_daemon_mode				(1)
#endif /* NO_DAEMON_MODE */


/* tilde-luzah-bozo.c */
#ifdef NO_USER_SUPPORT
#define bozo_user_transform(x)				(0)
#define bozo_user_free(x)				/* nothing */
#define have_user					(0)
#else
int	bozo_user_transform(bozo_httpreq_t *);
#define bozo_user_free(x)				free(x)
#define have_user					(1)
#endif /* NO_USER_SUPPORT */


/* dir-index-bozo.c */
#ifdef NO_DIRINDEX_SUPPORT
#define bozo_dir_index(a, b, c)				(0)
#define have_dirindex					(0)
#else
int	bozo_dir_index(bozo_httpreq_t *, const char *, int);
#define have_dirindex					(1)
#endif /* NO_DIRINDEX_SUPPORT */


/* content-bozo.c */
const char *bozo_content_type(bozo_httpreq_t *, const char *);
const char *bozo_content_encoding(bozo_httpreq_t *, const char *);
bozo_content_map_t *bozo_match_content_map(bozohttpd_t *, const char *, int);
bozo_content_map_t *bozo_get_content_map(bozohttpd_t *, const char *);
#ifdef NO_DYNAMIC_CONTENT
#define bozo_add_content_map_mime(h,s,t,u,v)		bozo_noop
#define have_dynamic_content				(0)
#else
void	bozo_add_content_map_mime(bozohttpd_t *, const char *, const char *,
				  const char *, const char *);
#define have_dynamic_content				(1)
#endif

/* additional cgi-bozo.c */
#if have_cgibin && have_dynamic_content
void	bozo_add_content_map_cgi(bozohttpd_t *, const char *, const char *);
#else
#define	bozo_add_content_map_cgi(h,s,t)
#endif

/* I/O */
int bozo_printf(bozohttpd_t *, const char *, ...) BOZO_PRINTFLIKE(2, 3);
ssize_t bozo_read(bozohttpd_t *, int, void *, size_t);
ssize_t bozo_write(bozohttpd_t *, int, const void *, size_t);
int bozo_flush(bozohttpd_t *, FILE *);

/* misc */
int bozo_init_httpd(bozohttpd_t *);
int bozo_init_prefs(bozohttpd_t *, bozoprefs_t *);
int bozo_set_defaults(bozohttpd_t *, bozoprefs_t *);
int bozo_setup(bozohttpd_t *, bozoprefs_t *, const char *, const char *);
void bozo_cleanup(bozohttpd_t *, bozoprefs_t *);
bozo_httpreq_t *bozo_read_request(bozohttpd_t *);
void bozo_process_request(bozo_httpreq_t *);
void bozo_clean_request(bozo_httpreq_t *);
int bozo_set_timeout(bozohttpd_t *, bozoprefs_t *, const char *, const char *);
bozoheaders_t *addmerge_reqheader(bozo_httpreq_t *, const char *,
				  const char *, ssize_t);
bozoheaders_t *addmerge_replheader(bozo_httpreq_t *, const char *,
				   const char *, ssize_t);

/* variables */
int bozo_set_pref(bozohttpd_t *, bozoprefs_t *, const char *, const char *);
char *bozo_get_pref(bozoprefs_t *, const char *);

int bozo_get_version(char */*buf*/, size_t /*size*/);

extern volatile sig_atomic_t	bozo_timeout_hit;

#endif	/* BOZOHTTPD_H_ */
