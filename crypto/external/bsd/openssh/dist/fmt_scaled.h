/*	$NetBSD: fmt_scaled.h,v 1.4 2014/10/19 16:30:58 christos Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
