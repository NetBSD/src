/*	$NetBSD: util.h,v 1.7 2002/03/11 18:47:51 kristerw Exp $	*/

EXT char serrbuf[BUFSIZ];		/* buffer for stderr */

int move_file(char *, char *);
void copy_file(char *, char *);
char *savestr(char *);
void say(const char *, ...)
     __attribute__((__format__(__printf__, 1, 2)));
void fatal(const char *, ...)
     __attribute__((__format__(__printf__, 1, 2)));
void pfatal(const char *, ...)
     __attribute__((__format__(__printf__, 1, 2)));
void ask(const char *, ...)
     __attribute__((__format__(__printf__, 1, 2)));
void set_signals(int);
void ignore_signals(void);
void makedirs(char *, bool);
char *fetchname(char *, int, int);
