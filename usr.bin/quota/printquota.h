/*	$NetBSD: printquota.h,v 1.1.2.7 2011/01/30 22:49:32 bouyer Exp $ */

const char *intprt(uint64_t, u_int, int, int);
const char *timeprt(time_t, time_t, int);
const char *timepprt(time_t, int, int);
int timeprd(const char *, time_t *);
int intrd(char *str, uint64_t *val, u_int);

