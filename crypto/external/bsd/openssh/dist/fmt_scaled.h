/*	$NetBSD: fmt_scaled.h,v 1.6 2016/12/25 00:07:47 christos Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
