/*	$NetBSD: libntptest.h,v 1.2.4.2 2014/12/25 02:28:19 snj Exp $	*/

#include "tests_main.h"

extern "C" {
#include "ntp_stdlib.h"
#include "ntp_calendar.h"
};

class libntptest : public ntptest {

protected:
	static time_t timefunc(time_t*);
	static time_t nowtime;
	static void   settime(int y, int m, int d, int H, int M, int S);

};
