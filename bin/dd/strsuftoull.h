/*	$NetBSD: strsuftoull.h,v 1.1 2001/11/25 10:50:06 lukem Exp $	*/

u_longlong_t strsuftoull(const char *, const char *,
    		u_longlong_t, u_longlong_t);

u_longlong_t strsuftoullx(const char *, const char *,
    		u_longlong_t, u_longlong_t, char *, size_t);
