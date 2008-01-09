/*	$NetBSD: bootparam.h,v 1.3.122.1 2008/01/09 01:56:36 matt Exp $	*/

int bp_whoami(int);
int bp_getfile(int, char *, struct in_addr *, char *);
