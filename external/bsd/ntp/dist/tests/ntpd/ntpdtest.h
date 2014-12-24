/*	$NetBSD: ntpdtest.h,v 1.1.1.1.6.1 2014/12/24 00:05:28 riz Exp $	*/

#include "tests_main.h"

extern "C" {
#include "ntp_stdlib.h"
#include "ntp_calendar.h"
};

class ntpdtest : public ntptest {

protected:
	static time_t timefunc(time_t*);
	static time_t nowtime;
	static void   settime(int y, int m, int d, int H, int M, int S);

};
