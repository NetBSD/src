/*	$NetBSD: test-libntp.h,v 1.1.1.3.6.2 2015/11/08 00:16:09 snj Exp $	*/

#include "config.h"

#include "ntp_stdlib.h"
#include "ntp_calendar.h"

time_t timefunc(time_t *ptr);
void settime(int y, int m, int d, int H, int M, int S);
time_t nowtime;
