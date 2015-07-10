/*	$NetBSD: test-libntp.h,v 1.1.1.2 2015/07/10 13:11:14 christos Exp $	*/

time_t timefunc(time_t *ptr);
void settime(int y, int m, int d, int H, int M, int S);
time_t nowtime;
