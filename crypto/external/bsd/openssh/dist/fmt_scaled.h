/*	$NetBSD: fmt_scaled.h,v 1.7.12.1 2019/06/10 21:41:12 christos Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	40
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
