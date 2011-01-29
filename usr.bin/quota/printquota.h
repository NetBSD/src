/*	$NetBSD: printquota.h,v 1.1.2.3 2011/01/29 17:42:37 bouyer Exp $ */

const char *intprt(uint64_t, u_int, int);
#define HN_PRIV_UNLIMITED 0x80000000	/* print "unlimited" instead of "-" */
const char *timeprt(time_t);
int intrd(char *str, uint64_t *val, u_int);

