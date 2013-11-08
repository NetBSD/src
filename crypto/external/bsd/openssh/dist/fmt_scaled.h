/*	$NetBSD: fmt_scaled.h,v 1.3 2013/11/08 19:18:25 christos Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
