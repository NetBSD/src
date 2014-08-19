/*	$NetBSD: fmt_scaled.h,v 1.2.8.1 2014/08/19 23:45:24 tls Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
