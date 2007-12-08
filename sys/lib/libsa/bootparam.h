/*	$NetBSD: bootparam.h,v 1.3.128.1 2007/12/08 18:20:49 mjf Exp $	*/

int bp_whoami(int);
int bp_getfile(int, char *, struct in_addr *, char *);
