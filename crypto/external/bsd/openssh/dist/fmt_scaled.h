/*	$NetBSD: fmt_scaled.h,v 1.8 2019/02/04 04:36:41 mrg Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	40
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
