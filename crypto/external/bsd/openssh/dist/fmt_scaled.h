/*	$NetBSD: fmt_scaled.h,v 1.6.2.1 2017/04/21 16:50:56 bouyer Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
