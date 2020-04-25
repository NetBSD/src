/*	$NetBSD: ext.h,v 1.3.8.1 2020/04/25 10:50:47 martin Exp $	*/

/* borrowed from ../../bin/csh/extern.h */
void prusage1(FILE *, const char *fmt, int, struct rusage *, struct rusage *,
    struct timespec *, struct timespec *);
