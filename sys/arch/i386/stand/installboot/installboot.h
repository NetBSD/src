/*	$NetBSD: installboot.h,v 1.4 1998/07/28 20:10:54 drochner Exp $	*/

ino_t createfileondev __P((char *, char *, char *, unsigned int));
void cleanupfileondev __P((char *, char *, int));

char *getmountpoint __P((char *));
void cleanupmount __P((char *));

extern int verbose;
