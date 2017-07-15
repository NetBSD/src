/*	$NetBSD: ext.h,v 1.3 2017/07/15 14:34:08 christos Exp $	*/

/* borrowed from ../../bin/csh/extern.h */
void prusage1(FILE *, const char *fmt, struct rusage *, struct rusage *,
    struct timespec *, struct timespec *);
