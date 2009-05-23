/*	$NetBSD: bozohttpd.h,v 1.11 2009/05/23 02:26:03 mrg Exp $	*/

/*	$eterna: bozohttpd.h,v 1.27 2009/05/22 21:51:38 mrg Exp $	*/

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
#include <sys/queue.h>
#include <sys/stat.h>

#include <stdio.h>

/* lots of "const" but gets free()'ed etc at times, sigh */

/* headers */
struct headers {
	/*const*/ char *h_header;
	/*const*/ char *h_value;	/* this gets free()'ed etc at times */
	SIMPLEQ_ENTRY(headers)	h_next;
};

/* http_req */
typedef struct {
	int	hr_method;
#define	HTTP_GET	0x01
#define HTTP_POST	0x02
#define HTTP_HEAD	0x03
#define HTTP_OPTIONS	0x04	/* not supported */
#define HTTP_PUT	0x05	/* not supported */
#define HTTP_DELETE	0x06	/* not supported */
#define HTTP_TRACE	0x07	/* not supported */
#define HTTP_CONNECT	0x08	/* not supported */
	const char *hr_methodstr;
	char	*hr_file;
	char	*hr_query;  
	const char *hr_proto;
	const char *hr_content_type;
	const char *hr_content_length;
	const char *hr_allow;
	const char *hr_host;		/* HTTP/1.1 Host: */
	const char *hr_referrer;
	const char *hr_range;
	const char *hr_if_modified_since;
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
	SIMPLEQ_HEAD(, headers)	hr_headers;
	int	hr_nheaders;
} http_req;

struct content_map {
	const char *name;	/* postfix of file */
	const char *type;	/* matching content-type */
	const char *encoding;	/* matching content-encoding */
	const char *encoding11;	/* matching content-encoding (HTTP/1.1) */
	const char *cgihandler;	/* optional CGI handler */
};

/* write in upto 64KiB chunks, and mmap in upto 64MiB chunks */
#define WRSZ	(64 * 1024)
#define MMAPSZ	(WRSZ * 1024)

/* debug flags */
#define DEBUG_NORMAL	1
#define DEBUG_FAT	2
#define DEBUG_OBESE	3
#define DEBUG_EXPLODING	4

#define	strornull(x)	((x) ? (x) : "<null>")

extern	int	dflag;
extern	const char *index_html;
extern	const char *server_software;
extern	char	*myname;

#ifdef DEBUG
void	debug__(int, const char *, ...)
			__attribute__((__format__(__printf__, 2, 3)));
#define debug(x)	debug__ x
#else
#define	debug(x)	
#endif /* DEBUG */

void	warning(const char *, ...)
		__attribute__((__format__(__printf__, 1, 2)));
void	error(int, const char *, ...)
		__attribute__((__format__(__printf__, 2, 3)))
		__attribute__((__noreturn__));
int	http_error(int, http_req *, const char *)
		__attribute__((__noreturn__));

int	check_special_files(http_req *, const char *);
char	*http_date(void);
void	print_header(http_req *, struct stat *, const char *, const char *);

char	*bozodgetln(int, ssize_t *, ssize_t	(*)(int, void *, size_t));
char	*bozostrnsep(char **, const char *, ssize_t *);

void	*bozomalloc(size_t);
void	*bozorealloc(void *, size_t);
char	*bozostrdup(const char *);

extern	const char *Iflag;
extern	int	Iflag_set;
extern	int	bflag, fflag;
extern	char	*slashdir;
extern	const char http_09[];
extern	const char http_10[];
extern	const char http_11[];
extern	const char text_plain[];

/* bozotic io */
extern	int	(*bozoprintf)(const char *, ...);
extern	ssize_t	(*bozoread)(int, void *, size_t);
extern	ssize_t	(*bozowrite)(int, const void *, size_t);
extern	int	(*bozoflush)(FILE *);


/* ssl-bozo.c */
#ifndef NO_SSL_SUPPORT
extern	void	ssl_set_opts(char *, char *);
extern	void	ssl_init(void);
extern	void	ssl_accept(void);
extern	void	ssl_destroy(void);
#else
#define ssl_set_opts(x, y)	/* nothing */
#define ssl_init()		/* nothing */
#define ssl_accept()		/* nothing */
#define ssl_destroy()		/* nothing */
#endif


/* auth-bozo.c */
#ifdef DO_HTPASSWD
extern	int	auth_check(http_req *, const char *);
extern	void	auth_cleanup(http_req *);
extern	int	auth_check_headers(http_req *, char *, char *, ssize_t);
extern	int	auth_check_special_files(http_req *, const char *);
extern	void	auth_check_401(http_req *, int);
extern	void	auth_cgi_setenv(http_req *, char ***);
extern	int	auth_cgi_count(http_req *);
#else
#define		auth_check(x, y)		0
#define		auth_cleanup(x)			/* nothing */
#define		auth_check_headers(x, y, z, a)	0
#define		auth_check_special_files(x, y)	0
#define		auth_check_401(x, y)		/* nothing */
#define		auth_cgi_setenv(x, y)		/* nothing */
#define		auth_cgi_count(x)		0
#endif /* DO_HTPASSWD */


/* cgi-bozo.c */
#ifndef NO_CGIBIN_SUPPORT
extern	void	set_cgibin(char *);
extern	void	spsetenv(const char *env, const char *val, char **envp);
extern	int	process_cgi(http_req *);
extern	void	add_content_map_cgi(char *, char *);
#else
#define	process_cgi(r)				0
#endif /* NO_CGIBIN_SUPPORT */


/* daemon-bozo.c */
extern	const char *Iflag;
extern	int	Iflag_set;
#ifndef NO_DAEMON_MODE
extern	char	*iflag;

extern	void	daemon_init(void);
extern	void	daemon_fork(void);
extern	void	daemon_closefds(void);
#else
#define daemon_init()				/* nothing */
#define daemon_fork()				/* nothing */
#define daemon_closefds()			/* nothing */
#endif /* NO_DAEMON_MODE */


/* tilde-luzah-bozo.c */
#ifndef NO_USER_SUPPORT
extern	int	uflag;
extern	const char *public_html;

int	user_transform(http_req *, int *);
#else
#define uflag					0
#define user_transform(a, b)			0
#endif /* NO_USER_SUPPORT */


/* dir-index-bozo.c */
#ifndef NO_DIRINDEX_SUPPORT
extern	int	Xflag;
extern	int	Hflag;
int	directory_index(http_req *, const char *, int);
#else
#define directory_index(a, b, c)		0
#endif /* NO_DIRINDEX_SUPPORT */


/* content-bozo.c */
extern	const char *content_type(http_req *, const char *);
extern	const char *content_encoding(http_req *, const char *);
extern	struct content_map *match_content_map(const char *, int);
extern	struct content_map *get_content_map(const char *);
#ifndef NO_DYNAMIC_CONTENT
void	add_content_map_mime(char *, char *, char *, char *);
#endif
