/*	$NetBSD: printquota.h,v 1.2 2011/03/06 17:08:42 bouyer Exp $ */

const char *intprt(uint64_t, u_int, int, int);
const char *timeprt(time_t, time_t, int);
const char *timepprt(time_t, int, int);
int timeprd(const char *, time_t *);
int intrd(char *str, uint64_t *val, u_int);

