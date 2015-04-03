/*	$NetBSD: fmt_scaled.h,v 1.5 2015/04/03 23:58:19 christos Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
