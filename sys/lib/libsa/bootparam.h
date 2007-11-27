/*	$NetBSD: bootparam.h,v 1.3.120.1 2007/11/27 19:38:25 joerg Exp $	*/

int bp_whoami(int);
int bp_getfile(int, char *, struct in_addr *, char *);
