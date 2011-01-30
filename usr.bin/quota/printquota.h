/*	$NetBSD: printquota.h,v 1.1.2.5 2011/01/30 20:54:22 bouyer Exp $ */

const char *intprt(uint64_t, u_int, int, int);
const char *timeprt(time_t, time_t, int space);
int intrd(char *str, uint64_t *val, u_int);

