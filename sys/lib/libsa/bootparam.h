/*	$NetBSD: bootparam.h,v 1.3.64.1 2007/12/07 17:33:33 yamt Exp $	*/

int bp_whoami(int);
int bp_getfile(int, char *, struct in_addr *, char *);
