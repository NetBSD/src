/*	$NetBSD: test-libntp.h,v 1.1.1.5 2016/01/08 21:21:33 christos Exp $	*/

#include "config.h"

#include "ntp_stdlib.h"
#include "ntp_calendar.h"

time_t timefunc(time_t *ptr);
void settime(int y, int m, int d, int H, int M, int S);
time_t nowtime;
