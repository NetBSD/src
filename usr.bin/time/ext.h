/*	$NetBSD: ext.h,v 1.4 2020/04/23 07:54:53 simonb Exp $	*/

/* borrowed from ../../bin/csh/extern.h */
void prusage1(FILE *, const char *fmt, int, struct rusage *, struct rusage *,
    struct timespec *, struct timespec *);
