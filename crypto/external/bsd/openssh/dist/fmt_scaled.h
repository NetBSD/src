/*	$NetBSD: fmt_scaled.h,v 1.2.2.1 2014/05/22 13:21:34 yamt Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
