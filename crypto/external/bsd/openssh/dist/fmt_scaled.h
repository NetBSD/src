/*	$NetBSD: fmt_scaled.h,v 1.7 2017/04/18 18:41:46 christos Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
