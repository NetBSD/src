/*	$NetBSD: identd.h,v 1.6 1999/05/18 04:49:41 jwise Exp $	*/

/*
** identd.h                 Common variables for the Pidentd daemon
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 6 Dec 1992
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef __IDENTD_H__
#define __IDENTD_H__

extern char version[];
extern char *lie_string;

extern char *path_unix;
extern char *path_kmem;

extern int verbose_flag;
extern int debug_flag;
extern int syslog_flag;
extern int multi_flag;
extern int other_flag;
extern int unknown_flag;
extern int noident_flag;
extern int crypto_flag;
extern int liar_flag;

extern char *charset_name;
extern char *indirect_host;
extern char *indirect_password;

#ifdef ALLOW_FORMAT
extern int format_flag;
extern char *format;
#endif

extern int lport;
extern int fport;

extern char *gethost __P((struct in_addr *));

extern int k_open __P((void));
#ifndef ALLOW_FORMAT
extern int k_getuid __P((struct in_addr *, int, struct in_addr *, int, int *));
#else
extern int k_getuid __P((struct in_addr *, int, struct in_addr *, int, int *, int *, char **, char **));
#endif
extern int parse __P((FILE *, struct in_addr *, struct in_addr *));
extern int parse_config __P((char *, int));

#ifdef INCLUDE_PROXY
int proxy __P((struct in_addr *, struct in_addr *, int, int, struct timeval *));
#else
int proxy __P((void *, void *, int, int, void *));
#endif

#endif
