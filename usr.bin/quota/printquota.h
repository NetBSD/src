/*	$NetBSD: printquota.h,v 1.3 2011/03/06 20:47:59 christos Exp $ */

const char *intprt(char *, size_t, uint64_t, int, int);
const char *timeprt(char *, size_t, time_t, time_t);
#if 0
const char *timepprt(time_t, int, int);
int timeprd(const char *, time_t *);
int intrd(char *str, uint64_t *val, u_int);
#endif

