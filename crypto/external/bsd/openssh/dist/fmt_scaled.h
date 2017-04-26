/*	$NetBSD: fmt_scaled.h,v 1.5.2.2 2017/04/26 02:52:14 pgoyette Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
