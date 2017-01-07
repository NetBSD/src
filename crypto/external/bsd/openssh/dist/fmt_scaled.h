/*	$NetBSD: fmt_scaled.h,v 1.5.2.1 2017/01/07 08:53:41 pgoyette Exp $	*/
#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE	7
#endif
int   fmt_scaled(long long, char *);
int   scan_scaled(const char *, long long *);
