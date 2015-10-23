/*	$NetBSD: test-libntp.h,v 1.1.1.3 2015/10/23 17:47:45 christos Exp $	*/

#include "config.h"

#include "ntp_stdlib.h"
#include "ntp_calendar.h"

time_t timefunc(time_t *ptr);
void settime(int y, int m, int d, int H, int M, int S);
time_t nowtime;
