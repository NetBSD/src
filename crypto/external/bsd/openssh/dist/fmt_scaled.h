/*	$NetBSD: fmt_scaled.h,v 1.2.4.1 2017/08/15 05:27:52 snj Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
